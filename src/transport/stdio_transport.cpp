#include "mcpp/transport/stdio_transport.h"
#include "mcpp/utils/logger.h"
#include "mcpp/utils/error.h"
#include <iostream>
#include <sstream>

namespace mcpp::transport {

StdioTransport::StdioTransport(std::istream& input, std::ostream& output)
    : input_(input), output_(output) {
    MCP_LOG_INFO("StdioTransport created");
}

StdioTransport::~StdioTransport() {
    stop();
    MCP_LOG_INFO("StdioTransport destroyed");
}

void StdioTransport::send(const core::JsonRpcMessage& message) {
    std::lock_guard<std::mutex> lock(send_mutex_);

    if (!is_open()) {
        throw utils::TransportException("Transport is not open");
    }

    try {
        auto json = message.to_json();
        std::string json_line = json.dump();

        output_ << json_line << std::endl;
        output_.flush();

        MCP_LOG_DEBUG("Sent message: {}", json_line);
    }
    catch (const std::exception& e) {
        MCP_LOG_ERROR("Failed to send message: {}", e.what());
        throw utils::TransportException("Failed to send message: " + std::string(e.what()));
    }
}

std::future<std::unique_ptr<core::JsonRpcMessage>> StdioTransport::receive() {
    std::lock_guard<std::mutex> lock(receive_mutex_);

    if (!is_open()) {
        std::promise<std::unique_ptr<core::JsonRpcMessage>> promise;
        auto future = promise.get_future();
        promise.set_exception(std::make_exception_ptr(
            utils::TransportException("Transport is not open")));
        return future;
    }

    // Create a pending receive
    auto pending = std::make_unique<PendingReceive>();
    auto future = pending->promise.get_future();

    {
        std::lock_guard<std::mutex> pending_lock(pending_mutex_);

        // Check if there are messages in the queue
        {
            std::lock_guard<std::mutex> queue_lock(queue_mutex_);
            if (!message_queue_.empty()) {
                auto message = std::move(message_queue_.front());
                message_queue_.pop();
                pending->promise.set_value(std::move(message));
                return future;
            }
        }

        // No messages in queue, add to pending receives
        pending_receives_.push(std::move(pending));
    }

    return future;
}

void StdioTransport::close() {
    stop();
}

bool StdioTransport::is_open() const {
    // For stdio transport, we consider it open if it's not explicitly stopped
    // even if reader thread has ended (EOF received)
    return !stopped_.load();
}

std::string StdioTransport::get_description() const {
    return "StdioTransport (stdin/stdout)";
}

void StdioTransport::start() {
    if (running_.load()) {
        return; // Already running
    }

    stopped_ = false;
    running_ = true;

    reader_thread_ = std::make_unique<std::thread>(&StdioTransport::reader_loop, this);

    MCP_LOG_INFO("StdioTransport started");
}

void StdioTransport::stop() {
    if (!running_.load()) {
        return; // Already stopped
    }

    running_ = false;
    stopped_ = true;

    // Wake up any pending receives
    {
        std::lock_guard<std::mutex> pending_lock(pending_mutex_);
        while (!pending_receives_.empty()) {
            auto pending = std::move(pending_receives_.front());
            pending_receives_.pop();
            pending->promise.set_exception(std::make_exception_ptr(
                utils::TransportException("Transport stopped")));
        }
    }

    // Wake up the reader thread
    queue_cv_.notify_all();
    pending_cv_.notify_all();

    if (reader_thread_ && reader_thread_->joinable()) {
        reader_thread_->join();
        reader_thread_.reset();
    }

    MCP_LOG_INFO("StdioTransport stopped");
}

void StdioTransport::reader_loop() {
    MCP_LOG_DEBUG("Reader thread started");

    try {
        while (running_.load()) {
            std::string json_line = read_json_line();

            if (json_line.empty()) {
                if (!running_.load()) {
                    break; // Normal shutdown
                }
                continue; // Skip empty lines
            }

            process_message(json_line);
        }
    }
    catch (const std::exception& e) {
        MCP_LOG_ERROR("Reader thread error: {}", e.what());

        // Notify all pending receives about the error
        std::lock_guard<std::mutex> pending_lock(pending_mutex_);
        while (!pending_receives_.empty()) {
            auto pending = std::move(pending_receives_.front());
            pending_receives_.pop();
            pending->promise.set_exception(std::make_exception_ptr(
                utils::TransportException("Reader thread error: " + std::string(e.what()))));
        }
    }

    MCP_LOG_DEBUG("Reader thread stopped");
}

std::string StdioTransport::read_json_line() {
    std::string line;

    // Use a timeout approach to allow checking running_ flag
    while (running_.load()) {
        if (std::getline(input_, line)) {
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (!line.empty()) {
                return line;
            }
        } else {
            // End of input - don't immediately stop, allow processing of last messages
            running_ = false;
            // Don't break immediately - let the loop exit naturally on next iteration
        }
    }

    return std::string();
}

void StdioTransport::process_message(const std::string& json_line) {
    try {
        MCP_LOG_DEBUG("Received raw JSON: {}", json_line);

        auto json = nlohmann::json::parse(json_line);
        auto message = core::JsonRpcMessage::from_json(json);

        if (!message || !message->is_valid()) {
            MCP_LOG_WARN("Received invalid message: {}", json_line);
            return;
        }

        fulfill_pending_receive(std::move(message));
    }
    catch (const nlohmann::json::parse_error& e) {
        MCP_LOG_ERROR("JSON parse error: {} for line: {}", e.what(), json_line);
    }
    catch (const std::exception& e) {
        MCP_LOG_ERROR("Error processing message: {}", e.what());
    }
}

void StdioTransport::fulfill_pending_receive(std::unique_ptr<core::JsonRpcMessage> message) {
    std::lock_guard<std::mutex> pending_lock(pending_mutex_);

    if (!pending_receives_.empty()) {
        // There's a pending receive, fulfill it
        auto pending = std::move(pending_receives_.front());
        pending_receives_.pop();
        pending->promise.set_value(std::move(message));
    } else {
        // No pending receives, queue the message
        std::lock_guard<std::mutex> queue_lock(queue_mutex_);
        message_queue_.push(std::move(message));
        queue_cv_.notify_one();
    }
}

} // namespace mcpp::transport
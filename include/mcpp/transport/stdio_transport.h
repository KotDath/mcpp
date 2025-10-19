#pragma once

#include "mcpp/transport/transport.h"
#include <istream>
#include <ostream>
#include <iostream>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <thread>

namespace mcpp::transport {

/**
 * @brief Stdio transport for MCP communication
 *
 * This transport uses stdin/stdout for bidirectional communication.
 * It's the primary transport used by Claude Desktop and other local MCP integrations.
 * Messages are sent in JSON Lines format (one JSON object per line).
 */
class StdioTransport : public Transport {
public:
    /**
     * @brief Constructor
     * @param input Input stream (defaults to std::cin)
     * @param output Output stream (defaults to std::cout)
     */
    explicit StdioTransport(std::istream& input = std::cin,
                            std::ostream& output = std::cout);

    /**
     * @brief Destructor
     */
    ~StdioTransport() override;

    // Transport interface implementation
    void send(const core::JsonRpcMessage& message) override;
    std::future<std::unique_ptr<core::JsonRpcMessage>> receive() override;
    void close() override;
    bool is_open() const override;
    std::string get_description() const override;

    /**
     * @brief Start the background reader thread
     * This must be called before using receive()
     */
    void start();

    /**
     * @brief Stop the background reader thread
     */
    void stop();

private:
    std::istream& input_;
    std::ostream& output_;

    // Thread management
    std::unique_ptr<std::thread> reader_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopped_{false};

    // Message queue
    std::queue<std::unique_ptr<core::JsonRpcMessage>> message_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // Synchronization for receive futures
    struct PendingReceive {
        std::promise<std::unique_ptr<core::JsonRpcMessage>> promise;
    };

    std::queue<std::unique_ptr<PendingReceive>> pending_receives_;
    mutable std::mutex pending_mutex_;
    std::condition_variable pending_cv_;

    // Thread safety
    mutable std::mutex send_mutex_;
    mutable std::mutex receive_mutex_;

    // Private methods
    void reader_loop();
    std::string read_json_line();
    void process_message(const std::string& json_line);
    void fulfill_pending_receive(std::unique_ptr<core::JsonRpcMessage> message);
};

} // namespace mcpp::transport
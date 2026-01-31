// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "mcpp/transport/stdio_transport.h"

#include <fcntl.h>
#include <cstdio>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace mcpp {
namespace transport {

bool StdioTransport::spawn(
    const std::string& command,
    const std::vector<std::string>& args,
    StdioTransport& out_transport,
    std::string& error_message
) {
    // Build command string with arguments
    std::string full_command = command;
    for (const auto& arg : args) {
        full_command += " ";
        full_command += arg;
    }

    // Open pipe for reading and writing
    FILE* pipe = popen(full_command.c_str(), "r+");
    if (!pipe) {
        error_message = "Failed to spawn subprocess: " + full_command;
        return false;
    }

    // Make pipe non-blocking for reads
    int fd = fileno(pipe);
    if (fd == -1) {
        error_message = "Failed to get file descriptor for pipe";
        pclose(pipe);
        return false;
    }

    // Note: popen() doesn't provide direct PID access
    // The PID tracking is limited; actual subprocess management
    // happens through the pipe lifecycle
    pid_t pid = 0;

    // Create transport object via placement new
    // (out_transport is already constructed, we need to reassign it)
    out_transport.~StdioTransport();
    new (&out_transport) StdioTransport(pipe, pid);

    return true;
}

StdioTransport::StdioTransport(FILE* pipe, pid_t pid)
    : pipe_(pipe), pid_(pid), running_(false) {}

bool StdioTransport::connect() {
    if (!pipe_) {
        return false;
    }

    running_ = true;
    read_thread_ = std::thread(&StdioTransport::read_loop, this);
    return true;
}

void StdioTransport::disconnect() {
    running_ = false;
    if (read_thread_.joinable()) {
        read_thread_.join();
    }
}

bool StdioTransport::is_connected() const {
    return running_ && pipe_ != nullptr;
}

bool StdioTransport::send(std::string_view message) {
    if (!pipe_ || !running_) {
        return false;
    }

    // Append newline delimiter per MCP spec
    std::string full_message(message);
    full_message += '\n';

    size_t written = fwrite(full_message.data(), 1, full_message.size(), pipe_);
    fflush(pipe_);

    return written == full_message.size();
}

void StdioTransport::set_message_callback(MessageCallback cb) {
    message_callback_ = std::move(cb);
}

void StdioTransport::set_error_callback(ErrorCallback cb) {
    error_callback_ = std::move(cb);
}

void StdioTransport::read_loop() {
    char buffer[4096];
    std::string line_buffer;

    while (running_ && pipe_) {
        if (fgets(buffer, sizeof(buffer), pipe_)) {
            line_buffer += buffer;

            // Process complete lines (newline-delimited)
            size_t pos;
            while ((pos = line_buffer.find('\n')) != std::string::npos) {
                std::string line = line_buffer.substr(0, pos);
                line_buffer.erase(0, pos + 1);

                if (message_callback_) {
                    message_callback_(line);
                }
            }
        } else {
            // EOF or error
            if (error_callback_) {
                error_callback_("Read error or EOF");
            }
            break;
        }
    }
}

StdioTransport::~StdioTransport() {
    disconnect();

    if (pipe_) {
        pclose(pipe_);
        pipe_ = nullptr;
    }
}

} // namespace transport
} // namespace mcpp

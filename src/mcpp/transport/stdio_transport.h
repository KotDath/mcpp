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

#ifndef MCPP_TRANSPORT_STDIO_TRANSPORT_H
#define MCPP_TRANSPORT_STDIO_TRANSPORT_H

#include <atomic>
#include <cstdio>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <unistd.h>  // for pid_t

#include "mcpp/transport/transport.h"

namespace mcpp {
namespace transport {

/**
 * @brief Stdio transport for subprocess communication
 *
 * StdioTransport implements the Transport interface for communicating with
 * subprocesses via stdin/stdout. This is the primary transport for MCP servers.
 *
 * Features:
 * - Spawns subprocesses using popen()
 * - Newline-delimited JSON messaging per MCP spec
 * - Background read thread for incoming messages
 * - RAII cleanup (closes pipe and waits for subprocess on destruction)
 *
 * @note Messages are newline-delimited - each JSON-RPC message must end with '\n'
 */
class StdioTransport : public Transport {
public:
    /**
     * @brief Spawn a subprocess for stdio communication
     *
     * Creates a subprocess using popen() and returns a StdioTransport
     * connected to the subprocess's stdin/stdout.
     *
     * @param command Command to execute (e.g., "node", "python", "/path/to/server")
     * @param args Command-line arguments to pass to the subprocess
     * @param out_transport Output parameter that receives the transport object
     * @param error_message Output parameter that receives error description on failure
     * @return true if spawn succeeded, false otherwise
     *
     * @note The subprocess is spawned with the equivalent of: `command arg1 arg2 ...`
     * @note The caller is responsible for calling connect() after successful spawn
     */
    static bool spawn(
        const std::string& command,
        const std::vector<std::string>& args,
        StdioTransport& out_transport,
        std::string& error_message
    );

    /**
     * @brief Destructor - cleanup subprocess
     *
     * Calls disconnect() to stop the read thread, then closes the pipe
     * using pclose() which waits for the subprocess to exit.
     */
    ~StdioTransport() override;

    // Non-copyable, non-movable (subprocess handle)
    StdioTransport(const StdioTransport&) = delete;
    StdioTransport& operator=(const StdioTransport&) = delete;
    StdioTransport(StdioTransport&&) = delete;
    StdioTransport& operator=(StdioTransport&&) = delete;

    /**
     * @brief Start the read thread
     *
     * Begins reading from the subprocess's stdout in a background thread.
     * Must be called after a successful spawn().
     *
     * @return true if started successfully, false if no pipe is available
     */
    bool connect() override;

    /**
     * @brief Stop the read thread
     *
     * Signals the read thread to stop and waits for it to join.
     * The pipe remains open after this call.
     */
    void disconnect() override;

    /**
     * @brief Check if the transport is connected
     *
     * @return true if the read thread is running and pipe is open
     */
    bool is_connected() const override;

    /**
     * @brief Send a message to the subprocess
     *
     * Writes the message to the subprocess's stdin with a newline delimiter.
     * The message should be a complete JSON-RPC string (already serialized).
     *
     * @param message The JSON-RPC message to send
     * @return true if the message was sent successfully, false otherwise
     */
    bool send(std::string_view message) override;

    /**
     * @brief Set the callback for received messages
     *
     * The callback is invoked for each complete line (newline-delimited JSON)
     * received from the subprocess's stdout.
     *
     * @param cb The callback function to invoke for each received message
     */
    void set_message_callback(MessageCallback cb) override;

    /**
     * @brief Set the callback for transport errors
     *
     * The callback is invoked when an error occurs (e.g., read error, EOF).
     *
     * @param cb The callback function to invoke on transport errors
     */
    void set_error_callback(ErrorCallback cb) override;

private:
    /**
     * @brief Private constructor for use by spawn()
     *
     * @param pipe The FILE* from popen()
     * @param pid The process ID (may be 0 if popen doesn't provide it)
     */
    StdioTransport(FILE* pipe, pid_t pid);

    /**
     * @brief Background thread function for reading from subprocess
     *
     * Continuously reads from the subprocess's stdout, buffers partial lines,
     * and invokes the message callback for each complete newline-delimited line.
     */
    void read_loop();

    FILE* pipe_;                       ///< Pipe for stdin/stdout communication
    pid_t pid_;                        ///< Subprocess PID (may be 0 if unavailable)
    std::atomic<bool> running_;        ///< Whether the read thread is running
    std::thread read_thread_;          ///< Background thread for reading stdout
    MessageCallback message_callback_; ///< Callback for received messages
    ErrorCallback error_callback_;     ///< Callback for transport errors
};

} // namespace transport
} // namespace mcpp

#endif // MCPP_TRANSPORT_STDIO_TRANSPORT_H

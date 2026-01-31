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

} // namespace transport
} // namespace mcpp

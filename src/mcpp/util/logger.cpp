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

#include "mcpp/util/logger.h"

#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>

// Check if spdlog is available
#if __has_include(<spdlog/spdlog.h>)
    #define MCPP_HAS_SPDLOG 1
    #include <spdlog/spdlog.h>
#else
    #define MCPP_HAS_SPDLOG 0
#endif

namespace mcpp::util {

//=============================================================================
// Logger::Span Implementation
//=============================================================================

Span::Span(std::string_view name,
           std::map<std::string, std::string> context)
    : name_(name)
    , context_(std::move(context))
    , start_time_(std::chrono::steady_clock::now()) {
}

Span::~Span() {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time_
    ).count();

    // Add duration to context
    context_["_duration_us"] = std::to_string(duration);
    context_["_completed"] = "true";

    // Log span completion
    Logger::global().log(Level::Debug,
        "Span completed: " + name_,
        context_);
}

void Span::add_context(std::string_view key, std::string_view value) {
    context_[std::string(key)] = std::string(value);
}

//=============================================================================
// Logger Implementation
//=============================================================================

Logger& Logger::global() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : min_level_(Level::Info)
    , enable_payload_(false)
    , max_payload_size_(1024) {

#if MCPP_HAS_SPDLOG
    // Try to use spdlog if available
    try {
        // Set spdlog pattern to match our format
        spdlog::set_pattern("[%l] %v");
        spdlog::set_level(spdlog::level::info);
    } catch (const std::exception& e) {
        // Fall back to stderr if spdlog init fails
        std::cerr << "[Logger] spdlog initialization failed, using stderr: "
                  << e.what() << std::endl;
    }
#endif
}

void Logger::log(Level level,
                 std::string_view message,
                 const std::map<std::string, std::string>& context) {
    if (level < min_level_) {
        return;  // Filtered out by level
    }

    std::ostringstream oss;
    oss << "[" << level_to_string(level) << "]";

    // Add context if present
    if (!context.empty()) {
        oss << " " << format_context(context);
    }

    oss << " - " << message;

    log_impl(level, oss.str());
}

void Logger::trace(std::string_view message,
                   const std::map<std::string, std::string>& context) {
    log(Level::Trace, message, context);
}

void Logger::debug(std::string_view message,
                   const std::map<std::string, std::string>& context) {
    log(Level::Debug, message, context);
}

void Logger::info(std::string_view message,
                  const std::map<std::string, std::string>& context) {
    log(Level::Info, message, context);
}

void Logger::warn(std::string_view message,
                  const std::map<std::string, std::string>& context) {
    log(Level::Warn, message, context);
}

void Logger::error(std::string_view message,
                   const std::map<std::string, std::string>& context) {
    log(Level::Error, message, context);
}

void Logger::set_level(Level level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;

#if MCPP_HAS_SPDLOG
    try {
        switch (level) {
            case Level::Trace:
                spdlog::set_level(spdlog::level::trace);
                break;
            case Level::Debug:
                spdlog::set_level(spdlog::level::debug);
                break;
            case Level::Info:
                spdlog::set_level(spdlog::level::info);
                break;
            case Level::Warn:
                spdlog::set_level(spdlog::level::warn);
                break;
            case Level::Error:
                spdlog::set_level(spdlog::level::err);
                break;
        }
    } catch (...) {
        // Ignore spdlog errors
    }
#endif
}

Logger::Level Logger::level() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return min_level_;
}

void Logger::enable_payload_logging(bool enable, size_t max_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    enable_payload_ = enable;
    max_payload_size_ = max_size;
}

bool Logger::payload_logging_enabled() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return enable_payload_;
}

size_t Logger::max_payload_size() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return max_payload_size_;
}

std::string Logger::format_payload(const nlohmann::json& payload) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!enable_payload_) {
        return "(payload logging disabled)";
    }

    try {
        std::string payload_str = payload.dump();
        if (payload_str.size() > max_payload_size_) {
            return payload_str.substr(0, max_payload_size_) + "... (" +
                   std::to_string(payload_str.size() - max_payload_size_) +
                   " more bytes)";
        }
        return payload_str;
    } catch (const std::exception& e) {
        return "(payload serialization error: " + std::string(e.what()) + ")";
    }
}

std::string_view Logger::level_to_string(Level level) noexcept {
    switch (level) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
        default:            return "UNKNOWN";
    }
}

std::optional<Level> Logger::string_to_level(std::string_view level) noexcept {
    if (level == "TRACE" || level == "trace") return Level::Trace;
    if (level == "DEBUG" || level == "debug") return Level::Debug;
    if (level == "INFO"  || level == "info")  return Level::Info;
    if (level == "WARN"  || level == "warn" ||
        level == "WARNING" || level == "warning") return Level::Warn;
    if (level == "ERROR" || level == "error") return Level::Error;
    return std::nullopt;
}

void Logger::log_impl(Level level, const std::string& formatted_message) {
#if MCPP_HAS_SPDLOG
    try {
        switch (level) {
            case Level::Trace:
                spdlog::trace(formatted_message);
                break;
            case Level::Debug:
                spdlog::debug(formatted_message);
                break;
            case Level::Info:
                spdlog::info(formatted_message);
                break;
            case Level::Warn:
                spdlog::warn(formatted_message);
                break;
            case Level::Error:
                spdlog::error(formatted_message);
                break;
        }
        return;
    } catch (...) {
        // Fall through to stderr on spdlog error
    }
#endif

    // Fallback to stderr
    std::cerr << formatted_message << std::endl;
}

std::string Logger::format_context(const std::map<std::string, std::string>& context) {
    if (context.empty()) {
        return "";
    }

    std::ostringstream oss;
    bool first = true;
    for (const auto& [key, value] : context) {
        if (!first) {
            oss << " ";
        }
        oss << key << "=" << value;
        first = false;
    }
    return oss.str();
}

} // namespace mcpp::util

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

#ifndef MCPP_UTIL_LOGGER_H
#define MCPP_UTIL_LOGGER_H

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace mcpp::util {

/**
 * @brief Structured logging with spdlog backend and stderr fallback
 *
 * Logger provides thread-safe structured logging with level filtering,
 * contextual key-value pairs for request tracking, and optional payload
 * logging with size limits.
 *
 * The logger automatically uses spdlog if available, falling back to
 * stderr-based logging for portability.
 *
 * Thread safety: All methods are thread-safe and may be called concurrently.
 */
class Logger {
public:
    /**
     * @brief Log level enumeration
     *
     * Levels follow standard logging conventions from Trace (most verbose)
     * to Error (least verbose but most critical).
     */
    enum class Level {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4
    };

    /**
     * @brief Span for automatic request duration tracking
     *
     * Span logs its duration and context on destruction, enabling automatic
     * request timing and distributed tracing. Follows the rust-sdk tracing
     * instrument pattern.
     *
     * Usage:
     *   {
     *       Logger::Span span("process_request", {{"request_id", "123"}});
     *       // ... do work ...
     *   } // Span destructor logs duration and context
     */
    class Span {
    public:
        /**
         * @brief Construct a new Span
         *
         * @param name Name of the span (e.g., operation name)
         * @param context Optional key-value context for this span
         */
        Span(std::string_view name,
             std::map<std::string, std::string> context = {});

        /**
         * @brief Destructor - logs span duration and context
         */
        ~Span();

        /**
         * @brief Add context to this span
         *
         * @param key Context key
         * @param value Context value
         */
        void add_context(std::string_view key, std::string_view value);

        /**
         * @brief Get the span name
         */
        const std::string& name() const noexcept { return name_; }

        /**
         * @brief Get the span context
         */
        const std::map<std::string, std::string>& context() const noexcept { return context_; }

    private:
        std::string name_;
        std::map<std::string, std::string> context_;
        std::chrono::steady_clock::time_point start_time_;
    };

    /**
     * @brief Get the global logger singleton
     *
     * Thread-safe singleton accessor using std::call_once.
     *
     * @return Reference to the global logger instance
     */
    static Logger& global();

    /**
     * @brief Non-copyable
     */
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Non-movable
     */
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief Log a message with optional context
     *
     * Formats the message as "[LEVEL] key1=value1 key2=value2 - message"
     * for structured log parsing following gopher-mcp LoggingFilter pattern.
     *
     * Thread safety: May be called concurrently from multiple threads.
     *
     * @param level Log level for this message
     * @param message The message to log
     * @param context Optional key-value pairs for structured context
     */
    void log(Level level,
             std::string_view message,
             const std::map<std::string, std::string>& context = {});

    /**
     * @brief Log a message at Trace level
     */
    void trace(std::string_view message,
               const std::map<std::string, std::string>& context = {});

    /**
     * @brief Log a message at Debug level
     */
    void debug(std::string_view message,
               const std::map<std::string, std::string>& context = {});

    /**
     * @brief Log a message at Info level
     */
    void info(std::string_view message,
              const std::map<std::string, std::string>& context = {});

    /**
     * @brief Log a message at Warn level
     */
    void warn(std::string_view message,
              const std::map<std::string, std::string>& context = {});

    /**
     * @brief Log a message at Error level
     */
    void error(std::string_view message,
               const std::map<std::string, std::string>& context = {});

    /**
     * @brief Set the minimum log level
     *
     * Messages below this level will be filtered out. Can be changed
     * at runtime without restart for dynamic log control.
     *
     * Thread safety: May be called concurrently with logging operations.
     *
     * @param level New minimum log level
     */
    void set_level(Level level);

    /**
     * @brief Get the current log level
     *
     * @return Current minimum log level
     */
    Level level() const noexcept;

    /**
     * @brief Enable or disable payload logging
     *
     * When enabled, JSON payloads are logged (truncated at max_size to
     * avoid log spam). When disabled, only message metadata is logged.
     *
     * Thread safety: May be called concurrently with logging operations.
     *
     * @param enable True to enable payload logging
     * @param max_size Maximum payload size to log (default 1024 bytes)
     */
    void enable_payload_logging(bool enable, size_t max_size = 1024);

    /**
     * @brief Check if payload logging is enabled
     *
     * @return True if payload logging is enabled
     */
    bool payload_logging_enabled() const noexcept;

    /**
     * @brief Get the maximum payload size for logging
     *
     * @return Maximum bytes of payload to log
     */
    size_t max_payload_size() const noexcept;

    /**
     * @brief Format a JSON payload for logging
     *
     * Truncates the payload at max_payload_size() and formats it
     * for safe logging (avoids log spam from large JSON).
     *
     * @param payload JSON payload to format
     * @return Truncated and formatted string representation
     */
    std::string format_payload(const nlohmann::json& payload) const;

    /**
     * @brief Convert level enum to string
     */
    static std::string_view level_to_string(Level level) noexcept;

    /**
     * @brief Convert string to level enum
     */
    static std::optional<Level> string_to_level(std::string_view level) noexcept;

private:
    /**
     * @brief Private constructor for singleton pattern
     */
    Logger();

    /**
     * @brief Internal logging implementation
     *
     * @param level Log level
     * @param formatted_message Pre-formatted message string
     */
    void log_impl(Level level, const std::string& formatted_message);

    /**
     * @brief Format context map as "key=value key2=value2"
     */
    static std::string format_context(const std::map<std::string, std::string>& context);

    mutable std::mutex mutex_;
    Level min_level_;
    bool enable_payload_;
    size_t max_payload_size_;
};

/**
 * @brief Convenience function for logging to the global logger
 */
inline Logger& logger() { return Logger::global(); }

/**
 * @brief Debug logging macro for stderr-only output
 *
 * MCPP_DEBUG_LOG writes debug messages directly to stderr, avoiding
 * any stdout pollution that could corrupt the JSON-RPC stdio protocol.
 *
 * Use this for temporary debug output or diagnostics in library code paths.
 * For structured logging, use the Logger class instead.
 *
 * Usage:
 *   MCPP_DEBUG_LOG("Request validation failed: code=%d", error_code);
 *   MCPP_DEBUG_LOG("Processing request with method=%s", method.c_str());
 *
 * Format uses printf-style formatting with std::fprintf.
 *
 * @note This macro is always active (no debug build gating) to ensure
 *       debug output is always routed to stderr, never stdout.
 */
#ifndef MCPP_DEBUG_LOG
#define MCPP_DEBUG_LOG(fmt, ...) std::fprintf(stderr, "[MCPP_DEBUG] " fmt "\n", ##__VA_ARGS__)
#endif

} // namespace mcpp::util

#endif // MCPP_UTIL_LOGGER_H

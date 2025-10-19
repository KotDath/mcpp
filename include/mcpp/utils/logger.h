#pragma once

#include <string>
#include <memory>
#include <spdlog/spdlog.h>

namespace mcpp::utils {

/**
 * @brief MCP Logger utility class
 *
 * Provides centralized logging functionality for the MCP library.
 * Uses spdlog as the underlying logging framework.
 */
class Logger {
public:
    /**
     * @brief Initialize the logger
     */
    static void initialize();

    /**
     * @brief Get the logger instance
     */
    static std::shared_ptr<spdlog::logger> get();

    /**
     * @brief Set log level
     */
    static void set_level(spdlog::level::level_enum level);

    /**
     * @brief Enable/disable console logging
     */
    static void enable_console(bool enable = true);

    /**
     * @brief Enable/disable file logging
     */
    static void enable_file(const std::string& filename);

    /**
     * @brief Log methods
     */
    template<typename... Args>
    static void trace(const std::string& format, Args&&... args) {
        if (logger_) {
            logger_->trace(format, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    static void debug(const std::string& format, Args&&... args) {
        if (logger_) {
            logger_->debug(format, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    static void info(const std::string& format, Args&&... args) {
        if (logger_) {
            logger_->info(format, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    static void warn(const std::string& format, Args&&... args) {
        if (logger_) {
            logger_->warn(format, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    static void error(const std::string& format, Args&&... args) {
        if (logger_) {
            logger_->error(format, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    static void critical(const std::string& format, Args&&... args) {
        if (logger_) {
            logger_->critical(format, std::forward<Args>(args)...);
        }
    }

private:
    static std::shared_ptr<spdlog::logger> logger_;
    static bool initialized_;
};

// Convenience macros
#define MCP_LOG_TRACE(...) ::mcpp::utils::Logger::trace(__VA_ARGS__)
#define MCP_LOG_DEBUG(...) ::mcpp::utils::Logger::debug(__VA_ARGS__)
#define MCP_LOG_INFO(...) ::mcpp::utils::Logger::info(__VA_ARGS__)
#define MCP_LOG_WARN(...) ::mcpp::utils::Logger::warn(__VA_ARGS__)
#define MCP_LOG_ERROR(...) ::mcpp::utils::Logger::error(__VA_ARGS__)
#define MCP_LOG_CRITICAL(...) ::mcpp::utils::Logger::critical(__VA_ARGS__)

} // namespace mcpp::utils
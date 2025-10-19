#include "mcpp/utils/logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace mcpp::utils {

std::shared_ptr<spdlog::logger> Logger::logger_;
bool Logger::initialized_ = false;

void Logger::initialize() {
    if (initialized_) {
        return;
    }

    try {
        // Create a multi-sink logger
        std::vector<spdlog::sink_ptr> sinks;

        // Console sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
        console_sink->set_level(spdlog::level::info);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
        sinks.push_back(console_sink);

        // Create the logger
        logger_ = std::make_shared<spdlog::logger>("mcpp", sinks.begin(), sinks.end());
        logger_->set_level(spdlog::level::info);
        logger_->flush_on(spdlog::level::err);

        spdlog::set_default_logger(logger_);

        initialized_ = true;
        info("MCP Logger initialized");
    }
    catch (const spdlog::spdlog_ex& ex) {
        // If spdlog fails, we can't log the error, but we should still mark as initialized
        initialized_ = true;
    }
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!initialized_) {
        initialize();
    }
    return logger_;
}

void Logger::set_level(spdlog::level::level_enum level) {
    if (logger_) {
        logger_->set_level(level);
    }
}

void Logger::enable_console(bool enable) {
    if (!initialized_) {
        initialize();
    }

    if (enable && !logger_->sinks().empty()) {
        logger_->sinks()[0]->set_level(spdlog::level::trace);
    } else if (!enable && !logger_->sinks().empty()) {
        logger_->sinks()[0]->set_level(spdlog::level::off);
    }
}

void Logger::enable_file(const std::string& filename) {
    if (!initialized_) {
        initialize();
    }

    try {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_st>(filename, true);
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");

        logger_->sinks().push_back(file_sink);
        info("File logging enabled: {}", filename);
    }
    catch (const spdlog::spdlog_ex& ex) {
        // If file logging fails, continue without it
        if (logger_) {
            logger_->warn("Failed to enable file logging: {}", ex.what());
        }
    }
}

} // namespace mcpp::utils
#include "Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <iostream>

namespace VaporFrame {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

bool Logger::initialize(const std::string& logFile) {
    if (initialized) {
        return true;
    }
    
    try {
        // Create console sink with colors
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(spdlog::level::trace);
        
        // Create rotating file sink
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logFile, 1024 * 1024 * 5, 3); // 5MB max file size, 3 rotated files
        fileSink->set_level(spdlog::level::trace);
        
        // Create logger with both sinks
        logger = std::make_shared<spdlog::logger>("VaporFrame", 
            spdlog::sinks_init_list{consoleSink, fileSink});
        
        // Set as default logger
        spdlog::set_default_logger(logger);
        
        // Set default level
        logger->set_level(spdlog::level::info);
        
        // Set pattern
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
        
        initialized = true;
        
        VF_LOG_INFO("VaporFrame Engine Logger initialized successfully");
        VF_LOG_INFO("Log file: {}", logFile);
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        return false;
    }
}

void Logger::shutdown() {
    if (logger) {
        VF_LOG_INFO("Shutting down VaporFrame Engine Logger");
        spdlog::shutdown();
        logger.reset();
        initialized = false;
    }
}

void Logger::setLevel(LogLevel level) {
    if (logger) {
        logger->set_level(static_cast<spdlog::level::level_enum>(level));
        VF_LOG_INFO("Log level set to: {}", static_cast<int>(level));
    }
}

} // namespace VaporFrame 
#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>

namespace VaporFrame {

enum class LogLevel {
    Trace = SPDLOG_LEVEL_TRACE,
    Debug = SPDLOG_LEVEL_DEBUG,
    Info = SPDLOG_LEVEL_INFO,
    Warn = SPDLOG_LEVEL_WARN,
    Error = SPDLOG_LEVEL_ERROR,
    Critical = SPDLOG_LEVEL_CRITICAL
};

class Logger {
public:
    static Logger& getInstance();
    
    // Initialize the logging system
    bool initialize(const std::string& logFile = "vaporframe.log");
    
    // Shutdown the logging system
    void shutdown();
    
    // Set the global log level
    void setLevel(LogLevel level);
    
    // Get the underlying spdlog logger
    std::shared_ptr<spdlog::logger> getLogger() const { return logger; }
    
    // Convenience methods for logging
    template<typename... Args>
    void trace(const char* fmt, const Args&... args) {
        if (logger) logger->trace(fmt, args...);
    }
    
    template<typename... Args>
    void debug(const char* fmt, const Args&... args) {
        if (logger) logger->debug(fmt, args...);
    }
    
    template<typename... Args>
    void info(const char* fmt, const Args&... args) {
        if (logger) logger->info(fmt, args...);
    }
    
    template<typename... Args>
    void warn(const char* fmt, const Args&... args) {
        if (logger) logger->warn(fmt, args...);
    }
    
    template<typename... Args>
    void error(const char* fmt, const Args&... args) {
        if (logger) logger->error(fmt, args...);
    }
    
    template<typename... Args>
    void critical(const char* fmt, const Args&... args) {
        if (logger) logger->critical(fmt, args...);
    }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::shared_ptr<spdlog::logger> logger;
    bool initialized = false;
};

// Global logging macros for convenience
#define VF_LOG_TRACE(...)    VaporFrame::Logger::getInstance().trace(__VA_ARGS__)
#define VF_LOG_DEBUG(...)    VaporFrame::Logger::getInstance().debug(__VA_ARGS__)
#define VF_LOG_INFO(...)     VaporFrame::Logger::getInstance().info(__VA_ARGS__)
#define VF_LOG_WARN(...)     VaporFrame::Logger::getInstance().warn(__VA_ARGS__)
#define VF_LOG_ERROR(...)    VaporFrame::Logger::getInstance().error(__VA_ARGS__)
#define VF_LOG_CRITICAL(...) VaporFrame::Logger::getInstance().critical(__VA_ARGS__)

} // namespace VaporFrame 
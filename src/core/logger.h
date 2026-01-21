#pragma once

#include <string>
#include <vector>
#include <cstdarg>
#include <memory>

/**
 * Log levels inspired by loguru (Python)
 */
enum class LogLevel
{
    TRACE = 0,    // Most verbose
    DEBUG = 1,    // Debug information
    INFO = 2,     // General information
    SUCCESS = 3,  // Success messages
    WARNING = 4,  // Warnings
    ERR = 5       // Errors (ERROR is reserved in Windows)
};

/**
 * Color codes for console and in-game rendering
 */
struct LogColor
{
    unsigned char r, g, b;
    
    static LogColor Trace()   { return { 128, 128, 128 }; }  // Gray
    static LogColor Debug()   { return { 100, 149, 237 }; }  // Cornflower blue
    static LogColor Info()    { return { 255, 255, 255 }; }  // White
    static LogColor Success() { return { 50, 205, 50 }; }    // Lime green
    static LogColor Warning() { return { 255, 165, 0 }; }    // Orange
    static LogColor Error()   { return { 255, 69, 69 }; }    // Red
};

/**
 * Log entry structure for storing logs
 */
struct LogEntry
{
    LogLevel level;
    std::string message;
    std::string timestamp;
    LogColor color;
};

/**
 * Logger class (Singleton)
 * 
 * Provides colored logging to console (Windows) and stores entries
 * for in-game console rendering via AppConsole.
 * 
 * Inspired by loguru (Python) but simplified for game use.
 * 
 * Usage:
 *   Logger::instance().init(true);  // true = create console window
 *   LOG_INFO("Player spawned at x=%d", x);
 *   LOG_ERROR("Failed to load: %s", filename);
 */
class Logger
{
private:
    static std::unique_ptr<Logger> s_instance;
    
    bool consoleEnabled;
    bool consoleCreated;
    LogLevel minLevel;
    std::vector<LogEntry> entries;
    size_t maxEntries;
    
    // Windows console handles
    void* consoleHandle;
    FILE* fpStdout;
    FILE* fpStderr;
    
    // Private constructor for singleton
    Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Friend declaration for make_unique
    friend std::unique_ptr<Logger> std::make_unique<Logger>();
    
    // Internal logging
    void logInternal(LogLevel level, const char* format, va_list args);
    void setConsoleColor(LogLevel level);
    void resetConsoleColor();
    const char* getLevelPrefix(LogLevel level) const;
    LogColor getLevelColor(LogLevel level) const;
    std::string getCurrentTimestamp() const;

public:
    // Singleton accessor
    static Logger& instance();
    static void destroy();
    
    // Initialization
    bool init(bool createConsole = false, LogLevel minLogLevel = LogLevel::TRACE);
    void shutdown();
    
    // Logging methods
    void trace(const char* format, ...);
    void debug(const char* format, ...);
    void info(const char* format, ...);
    void success(const char* format, ...);
    void warning(const char* format, ...);
    void error(const char* format, ...);
    
    // Generic log
    void log(LogLevel level, const char* format, ...);
    
    // Log entry access (for AppConsole)
    const std::vector<LogEntry>& getEntries() const { return entries; }
    void clearEntries() { entries.clear(); }
    size_t getEntryCount() const { return entries.size(); }
    
    // Configuration
    void setMinLevel(LogLevel level) { minLevel = level; }
    void setMaxEntries(size_t max) { maxEntries = max; }
    LogLevel getMinLevel() const { return minLevel; }
    
    // State
    bool isConsoleEnabled() const { return consoleEnabled; }
    bool isConsoleCreated() const { return consoleCreated; }
};

// Convenience macros for logging
#define LOG_TRACE(...)   Logger::instance().trace(__VA_ARGS__)
#define LOG_DEBUG(...)   Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...)    Logger::instance().info(__VA_ARGS__)
#define LOG_SUCCESS(...) Logger::instance().success(__VA_ARGS__)
#define LOG_WARNING(...) Logger::instance().warning(__VA_ARGS__)
#define LOG_ERROR(...)   Logger::instance().error(__VA_ARGS__)

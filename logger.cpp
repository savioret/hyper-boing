#include "logger.h"
#include <cstdio>
#include <cstring>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#endif

// Initialize static singleton instance
std::unique_ptr<Logger> Logger::s_instance = nullptr;

Logger::Logger()
    : consoleEnabled(false), consoleCreated(false), minLevel(LogLevel::TRACE),
      maxEntries(1000), consoleHandle(nullptr), fpStdout(nullptr), fpStderr(nullptr)
{
}

Logger& Logger::instance()
{
    if (!s_instance)
    {
        s_instance = std::make_unique<Logger>();
    }
    return *s_instance;
}

void Logger::destroy()
{
    if (s_instance)
    {
        s_instance->shutdown();
        s_instance.reset();
    }
}

bool Logger::init(bool createConsole, LogLevel minLogLevel)
{
    minLevel = minLogLevel;
    consoleEnabled = createConsole;
    
    if (!createConsole)
    {
        return true;
    }
    
#ifdef _WIN32
    // Allocate a console for this process
    if (AllocConsole())
    {
        consoleCreated = true;
        
        // Get console handle for color manipulation
        consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        
        // Redirect stdout to console
        freopen_s(&fpStdout, "CONOUT$", "w", stdout);
        
        // Redirect stderr to console
        freopen_s(&fpStderr, "CONOUT$", "w", stderr);
        
        // Set console title
        SetConsoleTitle(TEXT("Hyper Boing - Debug Console"));
        
        // Enable virtual terminal processing for ANSI colors (Windows 10+)
        DWORD consoleMode = 0;
        GetConsoleMode(consoleHandle, &consoleMode);
        consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(consoleHandle, consoleMode);
        
        // Print header
        printf("\n");
        setConsoleColor(LogLevel::INFO);
        printf("=============================================\n");
        printf("      HYPER BOING - DEBUG CONSOLE\n");
        printf("=============================================\n");
        resetConsoleColor();
        printf("\n");
        
        info("Console initialized successfully");
        info("Log level: %s", getLevelPrefix(minLevel));
        printf("\n");
        
        return true;
    }
    else
    {
        return false;
    }
#else
    // On non-Windows platforms, just use stdout
    consoleCreated = true;
    printf("\n=== HYPER BOING - DEBUG CONSOLE ===\n\n");
    return true;
#endif
}

void Logger::shutdown()
{
#ifdef _WIN32
    if (consoleCreated)
    {
        printf("\n");
        info("Console shutting down...");
        printf("Press any key to close console...\n");
        getchar();
        
        if (fpStdout) fclose(fpStdout);
        if (fpStderr) fclose(fpStderr);
        FreeConsole();
        
        consoleCreated = false;
    }
#endif
    
    entries.clear();
}

void Logger::setConsoleColor(LogLevel level)
{
#ifdef _WIN32
    if (!consoleHandle) return;
    
    WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // Default white
    
    switch (level)
    {
        case LogLevel::TRACE:
            color = FOREGROUND_INTENSITY; // Dark gray
            break;
        case LogLevel::DEBUG:
            color = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // Cyan
            break;
        case LogLevel::INFO:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // White
            break;
        case LogLevel::SUCCESS:
            color = FOREGROUND_GREEN | FOREGROUND_INTENSITY; // Bright green
            break;
        case LogLevel::WARNING:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // Yellow
            break;
        case LogLevel::ERR:
            color = FOREGROUND_RED | FOREGROUND_INTENSITY; // Bright red
            break;
    }
    
    SetConsoleTextAttribute((HANDLE)consoleHandle, color);
#endif
}

void Logger::resetConsoleColor()
{
#ifdef _WIN32
    if (!consoleHandle) return;
    SetConsoleTextAttribute((HANDLE)consoleHandle, 
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
}

const char* Logger::getLevelPrefix(LogLevel level) const
{
    switch (level)
    {
        case LogLevel::TRACE:   return "TRACE";
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO ";
        case LogLevel::SUCCESS: return "OK   ";
        case LogLevel::WARNING: return "WARN ";
        case LogLevel::ERR:     return "ERROR";
        default:                return "?????";
    }
}

LogColor Logger::getLevelColor(LogLevel level) const
{
    switch (level)
    {
        case LogLevel::TRACE:   return LogColor::Trace();
        case LogLevel::DEBUG:   return LogColor::Debug();
        case LogLevel::INFO:    return LogColor::Info();
        case LogLevel::SUCCESS: return LogColor::Success();
        case LogLevel::WARNING: return LogColor::Warning();
        case LogLevel::ERR:     return LogColor::Error();
        default:                return LogColor::Info();
    }
}

std::string Logger::getCurrentTimestamp() const
{
    time_t now = time(nullptr);
    struct tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &now);
#else
    localtime_r(&now, &timeInfo);
#endif
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeInfo);
    return std::string(buffer);
}

void Logger::logInternal(LogLevel level, const char* format, va_list args)
{
    // Check minimum log level
    if (static_cast<int>(level) < static_cast<int>(minLevel))
    {
        return;
    }
    
    // Format the message
    char messageBuffer[2048];
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    
    // Get timestamp
    std::string timestamp = getCurrentTimestamp();
    
    // Create log entry
    LogEntry entry;
    entry.level = level;
    entry.message = messageBuffer;
    entry.timestamp = timestamp;
    entry.color = getLevelColor(level);
    
    // Store entry (with limit)
    if (entries.size() >= maxEntries)
    {
        entries.erase(entries.begin());
    }
    entries.push_back(entry);
    
    // Print to console if enabled
    if (consoleEnabled && consoleCreated)
    {
        setConsoleColor(level);
        printf("[%s] [%s] %s\n", timestamp.c_str(), getLevelPrefix(level), messageBuffer);
        resetConsoleColor();
        fflush(stdout);
    }
}

void Logger::trace(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(LogLevel::TRACE, format, args);
    va_end(args);
}

void Logger::debug(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(LogLevel::DEBUG, format, args);
    va_end(args);
}

void Logger::info(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(LogLevel::INFO, format, args);
    va_end(args);
}

void Logger::success(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(LogLevel::SUCCESS, format, args);
    va_end(args);
}

void Logger::warning(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(LogLevel::WARNING, format, args);
    va_end(args);
}

void Logger::error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(LogLevel::ERR, format, args);
    va_end(args);
}

void Logger::log(LogLevel level, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(level, format, args);
    va_end(args);
}

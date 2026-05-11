#ifndef CADMIUM_CORE_LOGGER_HPP
#define CADMIUM_CORE_LOGGER_HPP

#include <format>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace Cadmium
{

//  Log levels

enum class LogLevel : uint8_t
{
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

inline const char* LogLevelName(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        default:              return "?????";
    }
}

// ANSI color codes for stdout sink
inline const char* LogLevelColor(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Trace: return "\033[37m";    // white
        case LogLevel::Debug: return "\033[36m";    // cyan
        case LogLevel::Info:  return "\033[32m";    // green
        case LogLevel::Warn:  return "\033[33m";    // yellow
        case LogLevel::Error: return "\033[31m";    // red
        case LogLevel::Fatal: return "\033[35m";    // magenta
        default:              return "\033[0m";
    }
}

static constexpr const char* k_AnsiReset = "\033[0m";

//  Log record
// What gets passed to each sink.

struct LogRecord
{
    LogLevel    level;
    std::string category;
    std::string message;
};

//  Sink type

using LogSink = std::function<void(const LogRecord&)>;

//  Logger

class Logger
{
public:
    //  Sink registration
    // Returns a sink ID that can be used to remove the sink later.
    // Thread-safe.

    uint32_t AddSink(LogSink sink)
    {
        std::lock_guard lock(m_Mutex);
        uint32_t id = m_NextSinkId++;
        m_Sinks.push_back({ id, std::move(sink) });
        return id;
    }

    void RemoveSink(uint32_t id)
    {
        std::lock_guard lock(m_Mutex);
        auto it = std::find_if(m_Sinks.begin(), m_Sinks.end(),
            [id](const RegisteredSink& s) { return s.id == id; });
        if (it != m_Sinks.end())
            m_Sinks.erase(it);
    }

    //  Level filter

    void     SetLevel(LogLevel level) { m_Level = level; }
    LogLevel GetLevel()         const { return m_Level;  }

    //  Core dispatch

    void Log(LogLevel level,
             const std::string& category,
             const std::string& message)
    {
        if (level < m_Level) return;

        LogRecord record{ level, category, message };

        std::lock_guard lock(m_Mutex);
        for (auto& s : m_Sinks)
            s.sink(record);
    }

    //  Formatted dispatch

    template<typename... Args>
    void Log(LogLevel level,
             const std::string& category,
             std::format_string<Args...> fmt,
             Args&&... args)
    {
        if (level < m_Level) return;
        Log(level, category, std::format(fmt, std::forward<Args>(args)...));
    }

private:
    struct RegisteredSink
    {
        uint32_t id;
        LogSink  sink;
    };

    std::vector<RegisteredSink> m_Sinks;
    std::mutex                  m_Mutex;
    LogLevel                    m_Level{LogLevel::Trace};
    uint32_t                    m_NextSinkId{1};
};

//  Global instance

Logger& GetLogger();

//  Free function API
// These are what engine code actually calls.
// Category is a short string identifying the subsystem: "Assets", "ECS" etc.

namespace Log
{
    template<typename... Args>
    void Trace(const std::string& category,
               std::format_string<Args...> fmt,
               Args&&... args)
    {
        GetLogger().Log(LogLevel::Trace, category, fmt,
                        std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Debug(const std::string& category,
               std::format_string<Args...> fmt,
               Args&&... args)
    {
        GetLogger().Log(LogLevel::Debug, category, fmt,
                        std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Info(const std::string& category,
              std::format_string<Args...> fmt,
              Args&&... args)
    {
        GetLogger().Log(LogLevel::Info, category, fmt,
                        std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Warn(const std::string& category,
              std::format_string<Args...> fmt,
              Args&&... args)
    {
        GetLogger().Log(LogLevel::Warn, category, fmt,
                        std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Error(const std::string& category,
               std::format_string<Args...> fmt,
               Args&&... args)
    {
        GetLogger().Log(LogLevel::Error, category, fmt,
                        std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Fatal(const std::string& category,
               std::format_string<Args...> fmt,
               Args&&... args)
    {
        GetLogger().Log(LogLevel::Fatal, category, fmt,
                        std::forward<Args>(args)...);
    }
}

//  Built-in sinks

// Writes colored output to stdout.
// Call once at engine startup, keep the returned ID if you want to remove it.
inline uint32_t AddStdoutSink(Logger& logger = GetLogger())
{
    return logger.AddSink([](const LogRecord& record)
    {
        std::string output = std::format(
            "{}[{}][{}] {}{}\n",
            LogLevelColor(record.level),
            LogLevelName(record.level),
            record.category,
            record.message,
            k_AnsiReset);

        std::fputs(output.c_str(), stdout);
    });
}

} // namespace Cadmium

#endif // CADMIUM_CORE_LOGGER_HPP

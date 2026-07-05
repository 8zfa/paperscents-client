#pragma once
#include <string>
#include <vector>
#include <mutex>

enum LogLevel { LogInfo, LogSuccess, LogWarning, LogError };

struct LogEntry {
    LogLevel Level;
    std::string Message;
    double Time;
};

class Logger {
public:
    static Logger& Get();
    void Info(const std::string& msg);
    void Success(const std::string& msg);
    void Warning(const std::string& msg);
    void Error(const std::string& msg);
    const std::vector<LogEntry>& GetEntries() const;
    void Clear();
private:
    Logger() = default;
    std::vector<LogEntry> m_Entries;
    mutable std::mutex m_Mutex;
};

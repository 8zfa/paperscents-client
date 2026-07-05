#include "logger.h"
#include <ctime>
#include <fstream>

Logger& Logger::Get() {
    static Logger instance;
    return instance;
}

static std::string Timestamp() {
    time_t now = time(nullptr);
    char buf[20];
    tm ts;
    localtime_s(&ts, &now);
    strftime(buf, sizeof(buf), "%H:%M:%S", &ts);
    return buf;
}

void Logger::Info(const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Entries.push_back({ LogInfo, msg, (double)clock() / CLOCKS_PER_SEC });
}

void Logger::Success(const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Entries.push_back({ LogSuccess, msg, (double)clock() / CLOCKS_PER_SEC });
}

void Logger::Warning(const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Entries.push_back({ LogWarning, msg, (double)clock() / CLOCKS_PER_SEC });
}

void Logger::Error(const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Entries.push_back({ LogError, msg, (double)clock() / CLOCKS_PER_SEC });
}

const std::vector<LogEntry>& Logger::GetEntries() const {
    return m_Entries;
}

void Logger::Clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Entries.clear();
}

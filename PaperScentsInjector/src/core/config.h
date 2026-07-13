#pragma once
#include <string>

struct AppConfig {
    bool AutoInject = false;
    std::string LastDllPath;
    int WindowX = 100;
    int WindowY = 100;
    int WindowW = 580;
    int WindowH = 440;
    bool LogVisible = false;
};

class ConfigManager {
public:
    static ConfigManager& Get();
    void Load();
    void Save();
    AppConfig& GetConfig() { return m_Config; }
    void MarkDirty();
private:
    ConfigManager() = default;
    AppConfig m_Config;
    bool m_Dirty = false;
    std::string GetPath();
};

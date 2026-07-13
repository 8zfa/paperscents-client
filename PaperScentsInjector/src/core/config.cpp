#include "config.h"
#include <fstream>
#include <ShlObj.h>
#include <json.hpp>

using json = nlohmann::json;

std::string ConfigManager::GetPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        std::string dir = std::string(path) + "\\PaperScents";
        CreateDirectoryA(dir.c_str(), nullptr);
        return dir + "\\config.json";
    }
    return "config.json";
}

void ConfigManager::Load() {
    std::ifstream f(GetPath());
    if (!f.is_open()) return;
    try {
        json j;
        f >> j;
        m_Config.AutoInject = j.value("autoInject", false);
        m_Config.LastDllPath = j.value("lastDllPath", "");
        m_Config.WindowX = j.value("windowX", 100);
        m_Config.WindowY = j.value("windowY", 100);
        m_Config.WindowW = j.value("windowW", 580);
        m_Config.WindowH = j.value("windowH", 440);
        m_Config.LogVisible = j.value("logVisible", false);
    } catch (...) {}
}

void ConfigManager::Save() {
    json j;
    j["autoInject"] = m_Config.AutoInject;
    j["lastDllPath"] = m_Config.LastDllPath;
    j["windowX"] = m_Config.WindowX;
    j["windowY"] = m_Config.WindowY;
    j["windowW"] = m_Config.WindowW;
    j["windowH"] = m_Config.WindowH;
    j["logVisible"] = m_Config.LogVisible;

    std::ofstream f(GetPath());
    if (f.is_open()) f << j.dump(2);
}

ConfigManager& ConfigManager::Get() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::MarkDirty() {
    m_Dirty = true;
}

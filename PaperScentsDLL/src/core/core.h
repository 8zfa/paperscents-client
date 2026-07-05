#pragma once
#include <Windows.h>
#include "java/java.h"
#include "utilities/logger.h"

class Core
{
public:
    static Core& GetInstance();

    bool Init(HMODULE hModule);
    void Shutdown();

    Java* GetJava() { return &m_Java; }
    HMODULE GetModule() { return m_hModule; }
    bool IsInitialized() { return m_Initialized; }

private:
    Core() = default;
    ~Core() = default;
    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;

    HMODULE m_hModule = nullptr;
    Java m_Java;
    bool m_Initialized = false;
};

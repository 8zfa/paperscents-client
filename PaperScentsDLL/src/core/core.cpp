#include "core.h"
#include "sdk/sdk.h"
#include "sdk/strayCache.h"
#include "modulehandler/modulehandler.h"
#include "rendering/uirendering.h"
#include "rendering/menu/menu.h"
#include "d3dhook/d3dhook.h"
#include "utilities/time.h"

extern void RegisterModules();

Core& Core::GetInstance()
{
    static Core instance;
    return instance;
}

bool Core::Init(HMODULE hModule)
{
    m_hModule = hModule;

    Logger::Init();
    Logger::Log("PaperScents DLL initializing...");

    Time::Init();

    if (!m_Java.Init())
    {
        Logger::Log("Failed to attach to JVM");
        return false;
    }
    Logger::Log("JVM attached successfully");

    JNIEnv* env = m_Java.GetEnv();

    StrayCache::GetInstance().Initialize(env);

    if (!SDK::GetInstance().Init(env))
    {
        Logger::Log("Failed to initialize SDK");
    }

    ModuleHandler::GetInstance().Initialize();
    RegisterModules();
    Logger::Log("Modules registered: %zu", ModuleHandler::GetInstance().GetModules().size());

    UIRendering::GetInstance().Initialize();
    Menu::GetInstance().Initialize();

    if (!InitD3DHook())
    {
        Logger::Log("D3D9 hook failed, modules will not render");
    }

    m_Initialized = true;
    Logger::Log("PaperScents DLL initialized");
    return true;
}

void Core::Shutdown()
{
    if (!m_Initialized)
        return;

    Logger::Log("PaperScents DLL shutting down...");

    ShutdownD3DHook();

    Menu::GetInstance().Shutdown();
    UIRendering::GetInstance().Shutdown();
    ModuleHandler::GetInstance().Shutdown();
    SDK::GetInstance().Shutdown();
    m_Java.Shutdown();
    Logger::Shutdown();
    m_Initialized = false;
}

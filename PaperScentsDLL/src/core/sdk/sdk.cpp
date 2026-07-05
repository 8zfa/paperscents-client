#include "sdk.h"
#include "../utilities/logger.h"
#include "../core.h"

SDK& SDK::GetInstance()
{
    static SDK instance;
    return instance;
}

bool SDK::Init(JNIEnv* env)
{
    m_Env = env;
    Logger::Log("SDK initializing with Lunar obfuscation mappings...");

    m_Minecraft = Minecraft::GetInstance();
    if (!m_Minecraft->Initialize(env))
    {
        Logger::Log("Failed to initialize Minecraft SDK class");
        return false;
    }
    Logger::Log("Minecraft SDK class initialized (obf: ave)");

    Logger::Log("SDK initialized with Lunar obfuscation mappings");
    return true;
}

void SDK::Shutdown()
{
    Logger::Log("SDK shutting down...");
    m_Env = nullptr;
    m_Minecraft = nullptr;
    m_Player = nullptr;
}

jclass SDK::FindClass(const char* className)
{
    if (!m_Env) return nullptr;
    Java* java = Core::GetInstance().GetJava();
    if (java) return java->FindClass(className);
    return m_Env->FindClass(className);
}

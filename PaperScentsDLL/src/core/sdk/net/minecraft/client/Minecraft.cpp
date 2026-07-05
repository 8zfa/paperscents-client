#include "Minecraft.h"
#include "../utilities/logger.h"
#include "core/core.h"
#include <thread>

Minecraft* Minecraft::GetInstance()
{
    static Minecraft instance;
    return &instance;
}

bool Minecraft::Initialize(JNIEnv* env)
{
    m_Env = env;

    const char* classNames[] = {
        "net.minecraft.client.Minecraft",
        "net.minecraft.client.ave",
        "com.moonsworth.lunar.bridge.client.MinecraftBridge",
        "com.moonsworth.lunar.IORHRHHHRHOOIICCOIIIORROC.HCOCCOORCORCCCOHCIOIICRRR.ICIIORCRCCROIICHRIRRIHOCR"
    };

    for (const char* name : classNames)
    {
        jclass cls = env->FindClass(name);
        if (cls)
        {
            m_Class = (jclass)env->NewGlobalRef(cls);
            Logger::Log("Found Minecraft class: %s", name);
            return true;
        }
        env->ExceptionClear();
    }

    Logger::Log("Direct FindClass failed, trying classloader from thread scan...");

    Java* java = Core::GetInstance().GetJava();

    for (const char* name : classNames)
    {
        jclass cls = java->FindClass(name);
        if (cls)
        {
            m_Class = (jclass)env->NewGlobalRef(cls);
            Logger::Log("Found Minecraft class via classloader: %s", name);
            return true;
        }
    }

    Logger::Log("Class not found yet, retrying after 2s...");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    for (const char* name : classNames)
    {
        jclass cls = java->FindClass(name);
        if (cls)
        {
            m_Class = (jclass)env->NewGlobalRef(cls);
            Logger::Log("Found Minecraft class on retry: %s", name);
            return true;
        }
    }

    Logger::Log("Failed to find Minecraft class (all methods exhausted)");
    return false;
}

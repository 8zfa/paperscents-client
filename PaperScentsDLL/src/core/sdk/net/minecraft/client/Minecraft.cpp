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
        "ave",
        "net.minecraft.client.Minecraft",
        "com.moonsworth.lunar.bridge.client.MinecraftBridge"
    };

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

    Logger::Log("Failed to find Minecraft class (all methods exhausted)");
    return false;
}

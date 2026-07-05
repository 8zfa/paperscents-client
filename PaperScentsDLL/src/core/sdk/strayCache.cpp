#include "strayCache.h"
#include "../core.h"
#include "../utilities/logger.h"

jclass StrayCache::Minecraft = nullptr;
jclass StrayCache::World = nullptr;
jclass StrayCache::Entity = nullptr;
jclass StrayCache::EntityLivingBase = nullptr;
jclass StrayCache::EntityPlayer = nullptr;
jclass StrayCache::EntityPlayerSP = nullptr;

StrayCache& StrayCache::GetInstance()
{
    static StrayCache instance;
    return instance;
}

void StrayCache::Initialize(JNIEnv* env)
{
    m_Env = env;
    Logger::Log("StrayCache initializing...");

    Java* java = Core::GetInstance().GetJava();
    if (!java) { Logger::Log("StrayCache: Java instance not available"); return; }

    auto cache = [&](const char* obf, const char* deobf, const char* label, jclass& out)
    {
        jclass cls = java->FindClass(obf, deobf);
        if (cls) { out = (jclass)env->NewGlobalRef(cls); Logger::Log("Cached %s: %s/%s", label, obf, deobf); }
        else Logger::Log("Failed to cache %s", label);
    };

    cache("ave", "net/minecraft/client/Minecraft", "Minecraft", Minecraft);
    cache("adm", "net/minecraft/world/World", "World", World);
    cache("pk", "net/minecraft/entity/Entity", "Entity", Entity);
    cache("pr", "net/minecraft/entity/EntityLivingBase", "EntityLivingBase", EntityLivingBase);
    cache("qh", "net/minecraft/entity/player/EntityPlayer", "EntityPlayer", EntityPlayer);
    cache("bew", "net/minecraft/client/entity/EntityPlayerSP", "EntityPlayerSP", EntityPlayerSP);

    Logger::Log("StrayCache initialized");
}

jclass StrayCache::GetClass(const std::string& name)
{
    auto it = m_Classes.find(name);
    if (it != m_Classes.end())
        return it->second;

    Java* java = Core::GetInstance().GetJava();
    jclass clazz = java ? java->FindClass(name) : nullptr;
    if (clazz && m_Env)
    {
        m_Classes[name] = (jclass)m_Env->NewGlobalRef(clazz);
    }
    return clazz;
}

jmethodID StrayCache::GetMethodID(jclass clazz, const std::string& name, const std::string& sig)
{
    return m_Env ? m_Env->GetMethodID(clazz, name.c_str(), sig.c_str()) : nullptr;
}

jfieldID StrayCache::GetFieldID(jclass clazz, const std::string& name, const std::string& sig)
{
    return m_Env ? m_Env->GetFieldID(clazz, name.c_str(), sig.c_str()) : nullptr;
}
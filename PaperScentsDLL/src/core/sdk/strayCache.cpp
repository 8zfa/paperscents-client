#include "strayCache.h"
#include "../utilities/logger.h"

StrayCache& StrayCache::GetInstance()
{
    static StrayCache instance;
    return instance;
}

void StrayCache::Initialize(JNIEnv* env)
{
    m_Env = env;
    Logger::Log("StrayCache initializing...");
}

jclass StrayCache::GetClass(const std::string& name)
{
    auto it = m_Classes.find(name);
    if (it != m_Classes.end())
        return it->second;

    jclass clazz = m_Env->FindClass(name.c_str());
    if (clazz)
    {
        m_Classes[name] = (jclass)m_Env->NewGlobalRef(clazz);
    }
    return clazz;
}

jmethodID StrayCache::GetMethodID(jclass clazz, const std::string& name, const std::string& sig)
{
    return m_Env->GetMethodID(clazz, name.c_str(), sig.c_str());
}

jfieldID StrayCache::GetFieldID(jclass clazz, const std::string& name, const std::string& sig)
{
    return m_Env->GetFieldID(clazz, name.c_str(), sig.c_str());
}

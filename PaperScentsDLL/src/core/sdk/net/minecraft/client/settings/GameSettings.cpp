#include "GameSettings.h"
#include "../utilities/logger.h"

bool GameSettings::Initialize(JNIEnv* env, jobject obj)
{
    m_Env = env;
    if (obj) m_Object = env->NewGlobalRef(obj);
    m_Class = env->FindClass("avh");
    if (!m_Class)
    {
        Logger::Log("Failed to find GameSettings class (avh)");
        return false;
    }
    m_Class = (jclass)env->NewGlobalRef(m_Class);
    Logger::Log("GameSettings initialized (obf: avh)");
    return true;
}

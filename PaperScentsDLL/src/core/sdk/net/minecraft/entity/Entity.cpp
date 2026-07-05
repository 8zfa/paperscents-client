#include "Entity.h"
#include "../utilities/logger.h"

bool Entity::Initialize(JNIEnv* env, jobject obj)
{
    m_Env = env;
    m_Object = env->NewGlobalRef(obj);
    m_Class = env->FindClass("pk");
    if (!m_Class)
    {
        Logger::Log("Failed to find Entity class (pk)");
        return false;
    }
    m_Class = (jclass)env->NewGlobalRef(m_Class);
    Logger::Log("Entity initialized (obf: pk)");
    return true;
}

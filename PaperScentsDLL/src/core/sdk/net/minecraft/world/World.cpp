#include "World.h"
#include "../utilities/logger.h"

bool World::Initialize(JNIEnv* env, jobject obj)
{
    m_Env = env;
    if (obj) m_Object = env->NewGlobalRef(obj);
    m_Class = env->FindClass("adm");
    if (!m_Class)
    {
        Logger::Log("Failed to find World class (adm)");
        return false;
    }
    m_Class = (jclass)env->NewGlobalRef(m_Class);
    Logger::Log("World initialized (obf: adm)");
    return true;
}

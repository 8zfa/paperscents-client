#include "EntityPlayerSP.h"
#include "../utilities/logger.h"

bool EntityPlayerSP::Initialize(JNIEnv* env, jobject obj)
{
    m_Env = env;
    m_Object = env->NewGlobalRef(obj);
    m_Class = env->FindClass("bew");
    if (!m_Class)
    {
        Logger::Log("Failed to find EntityPlayerSP class (bew)");
        return false;
    }
    m_Class = (jclass)env->NewGlobalRef(m_Class);
    Logger::Log("EntityPlayerSP initialized (obf: bew)");
    return true;
}

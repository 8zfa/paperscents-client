#include "EntityPlayer.h"
#include "../utilities/logger.h"

bool EntityPlayer::Initialize(JNIEnv* env, jobject obj)
{
    m_Env = env;
    m_Object = env->NewGlobalRef(obj);
    m_Class = env->FindClass("wn");
    if (!m_Class)
    {
        Logger::Log("Failed to find EntityPlayer class (wn)");
        return false;
    }
    m_Class = (jclass)env->NewGlobalRef(m_Class);
    Logger::Log("EntityPlayer initialized (obf: wn)");
    return true;
}

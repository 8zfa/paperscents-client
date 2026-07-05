#include "EntityLivingBase.h"
#include "../utilities/logger.h"

bool EntityLivingBase::Initialize(JNIEnv* env, jobject obj)
{
    m_Env = env;
    m_Object = env->NewGlobalRef(obj);
    m_Class = env->FindClass("pr");
    if (!m_Class)
    {
        Logger::Log("Failed to find EntityLivingBase class (pr)");
        return false;
    }
    m_Class = (jclass)env->NewGlobalRef(m_Class);
    Logger::Log("EntityLivingBase initialized (obf: pr)");
    return true;
}

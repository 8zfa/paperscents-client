#include "Vec3.h"
#include "../utilities/logger.h"

bool Vec3::Initialize(JNIEnv* env, jobject obj)
{
    m_Env = env;
    if (obj) m_Object = env->NewGlobalRef(obj);
    m_Class = env->FindClass("aui");
    if (!m_Class)
    {
        Logger::Log("Failed to find Vec3 class (aui)");
        return false;
    }
    m_Class = (jclass)env->NewGlobalRef(m_Class);
    Logger::Log("Vec3 initialized (obf: aui)");
    return true;
}

double Vec3::GetX()
{
    if (!m_Env || !m_Object) return 0.0;
    jfieldID fid = m_Env->GetFieldID(m_Class, "x", "D");
    if (!fid) return 0.0;
    return m_Env->GetDoubleField(m_Object, fid);
}

double Vec3::GetY()
{
    if (!m_Env || !m_Object) return 0.0;
    jfieldID fid = m_Env->GetFieldID(m_Class, "y", "D");
    if (!fid) return 0.0;
    return m_Env->GetDoubleField(m_Object, fid);
}

double Vec3::GetZ()
{
    if (!m_Env || !m_Object) return 0.0;
    jfieldID fid = m_Env->GetFieldID(m_Class, "z", "D");
    if (!fid) return 0.0;
    return m_Env->GetDoubleField(m_Object, fid);
}

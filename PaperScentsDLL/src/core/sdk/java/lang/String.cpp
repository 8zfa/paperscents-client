#include "String.h"

JString& JString::GetInstance()
{
    static JString instance;
    return instance;
}

bool JString::Initialize(JNIEnv* env)
{
    m_Env = env;
    return true;
}

std::string JString::ToString(jstring str)
{
    if (!m_Env || !str) return "";
    const char* chars = m_Env->GetStringUTFChars(str, nullptr);
    if (!chars) return "";
    std::string result(chars);
    m_Env->ReleaseStringUTFChars(str, chars);
    return result;
}

jstring JString::FromString(const std::string& str)
{
    if (!m_Env) return nullptr;
    return m_Env->NewStringUTF(str.c_str());
}

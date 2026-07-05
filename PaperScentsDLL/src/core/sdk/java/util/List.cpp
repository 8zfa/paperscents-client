#include "List.h"

bool JList::Initialize(JNIEnv* env)
{
    m_Env = env;
    m_Class = env->FindClass("java/util/List");
    if (!m_Class) return false;
    m_Class = (jclass)env->NewGlobalRef(m_Class);
    m_SizeMethod = env->GetMethodID(m_Class, "size", "()I");
    m_GetMethod = env->GetMethodID(m_Class, "get", "(I)Ljava/lang/Object;");
    return true;
}

int JList::Size(jobject list)
{
    if (!m_Env || !list || !m_SizeMethod) return 0;
    return m_Env->CallIntMethod(list, m_SizeMethod);
}

jobject JList::Get(jobject list, int index)
{
    if (!m_Env || !list || !m_GetMethod) return nullptr;
    return m_Env->CallObjectMethod(list, m_GetMethod, index);
}

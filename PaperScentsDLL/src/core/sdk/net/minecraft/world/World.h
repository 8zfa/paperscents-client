#pragma once
#include <jni.h>

class World
{
public:
    bool Initialize(JNIEnv* env, jobject obj);

    jclass GetClass() { return m_Class; }
    jobject GetObject() { return m_Object; }

    static constexpr const char* GetObfClassName() { return "adm"; }

private:
    JNIEnv* m_Env = nullptr;
    jclass m_Class = nullptr;
    jobject m_Object = nullptr;
};

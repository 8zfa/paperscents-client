#pragma once
#include <jni.h>

class Vec3
{
public:
    bool Initialize(JNIEnv* env, jobject obj);

    jclass GetClass() { return m_Class; }
    jobject GetObject() { return m_Object; }

    double GetX();
    double GetY();
    double GetZ();

    static constexpr const char* GetObfClassName() { return "aui"; }

private:
    JNIEnv* m_Env = nullptr;
    jclass m_Class = nullptr;
    jobject m_Object = nullptr;
};

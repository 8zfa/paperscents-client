#pragma once
#include <jni.h>

class JList
{
public:
    bool Initialize(JNIEnv* env);

    int Size(jobject list);
    jobject Get(jobject list, int index);

private:
    JNIEnv* m_Env = nullptr;
    jclass m_Class = nullptr;
    jmethodID m_SizeMethod = nullptr;
    jmethodID m_GetMethod = nullptr;
};

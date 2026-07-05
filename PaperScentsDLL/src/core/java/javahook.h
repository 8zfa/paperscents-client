#pragma once
#include <jni.h>

class JavaHook
{
public:
    static JavaHook& GetInstance();

    bool Initialize(JNIEnv* env);
    void Shutdown();

private:
    JavaHook() = default;
    ~JavaHook() = default;
    JavaHook(const JavaHook&) = delete;
    JavaHook& operator=(const JavaHook&) = delete;

    JNIEnv* m_Env = nullptr;
};

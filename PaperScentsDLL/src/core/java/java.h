#pragma once
#include <Windows.h>
#include <jni.h>
#include <string>

class Java
{
public:
    bool Init();
    void Shutdown();

    JNIEnv* GetEnv();
    static JNIEnv* GetThreadEnv();
    JavaVM* GetJVM() { return m_Jvm; }
    bool IsValid() { return m_Jvm != nullptr; }

    jclass FindClass(const std::string& name);
    jclass FindClass(const std::string& obfName, const std::string& deobfName);

private:
    bool FindJVM();
    bool AttachAsDaemon();
    void SetupLunarClassLoader();
    void TryFindClassLoader();
    bool ProbeClassLoader(jobject classLoader);

    JavaVM* m_Jvm = nullptr;
    HMODULE m_JvmModule = nullptr;

    jobject m_ClassLoader = nullptr;
    jmethodID m_FindClassMethod = nullptr;
    jmethodID m_LoadClassMethod = nullptr;
};

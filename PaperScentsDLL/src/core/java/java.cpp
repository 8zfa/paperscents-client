#include "java.h"
#include "../core.h"
#include "../utilities/logger.h"
#include <vector>
#include <psapi.h>
#pragma comment(lib, "psapi")

typedef jint(JNICALL* JNI_GetCreatedJavaVMs_t)(JavaVM**, jsize, jsize*);

JNIEnv* Java::GetThreadEnv()
{
    auto* java = Core::GetInstance().GetJava();
    return (java && java->IsValid()) ? java->GetEnv() : nullptr;
}

JNIEnv* Java::GetEnv()
{
    if (!m_Jvm) return nullptr;
    JNIEnv* env = nullptr;
    jint result = m_Jvm->GetEnv((void**)&env, JNI_VERSION_1_8);
    if (result == JNI_EDETACHED)
    {
        JavaVMAttachArgs args{ JNI_VERSION_1_8, "PaperScents", nullptr };
        m_Jvm->AttachCurrentThreadAsDaemon((void**)&env, &args);
    }
    return env;
}

bool Java::Init()
{
    if (!FindJVM())
    {
        Logger::Log("Could not find JVM in process");
        return false;
    }

    if (!AttachAsDaemon())
    {
        Logger::Log("Failed to attach to JVM as daemon");
        return false;
    }

    SetupLunarClassLoader();
    return true;
}

void Java::Shutdown()
{
    JNIEnv* env = GetEnv();
    if (m_ClassLoader && env)
    {
        env->DeleteGlobalRef(m_ClassLoader);
        m_ClassLoader = nullptr;
    }

    if (m_JvmModule)
    {
        FreeLibrary(m_JvmModule);
        m_JvmModule = nullptr;
    }

    m_Jvm = nullptr;
}

void Java::SetupLunarClassLoader()
{
    JNIEnv* env = GetEnv();
    if (!env) return;

    jclass threadClass = env->FindClass("java/lang/Thread");
    jclass mapClass = env->FindClass("java/util/Map");
    jclass setClass = env->FindClass("java/util/Set");
    jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");

    if (!threadClass || !mapClass || !setClass || !classLoaderClass)
    {
        env->ExceptionClear();
        Logger::Log("Failed to find thread/collection classes for classloader setup");
        return;
    }

    jmethodID getAllStackTraces = env->GetStaticMethodID(threadClass, "getAllStackTraces", "()Ljava/util/Map;");
    jmethodID keySet = env->GetMethodID(mapClass, "keySet", "()Ljava/util/Set;");
    jmethodID toArray = env->GetMethodID(setClass, "toArray", "()[Ljava/lang/Object;");
    jmethodID getContextCL = env->GetMethodID(threadClass, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
    m_FindClassMethod = env->GetMethodID(classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    m_LoadClassMethod = env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    if (!getAllStackTraces || !keySet || !toArray || !getContextCL || !m_FindClassMethod)
    {
        env->ExceptionClear();
        Logger::Log("Failed to get method IDs for classloader setup");
        goto cleanup;
    }

    jobject stackTracesMap = env->CallStaticObjectMethod(threadClass, getAllStackTraces);
    jobject threadsSet = env->CallObjectMethod(stackTracesMap, keySet);
    jobjectArray threads = (jobjectArray)env->CallObjectMethod(threadsSet, toArray);
    jint threadCount = env->GetArrayLength(threads);

    const char* probeNames[] = {
        "net.minecraft.client.Minecraft",
        "ave",
        "com.moonsworth.lunar.bridge.client.MinecraftBridge"
    };

    for (int i = 0; i < threadCount; i++)
    {
        jobject thread = env->GetObjectArrayElement(threads, i);
        jobject classLoader = env->CallObjectMethod(thread, getContextCL);

        if (classLoader)
        {
            for (const char* probe : probeNames)
            {
                jstring jname = env->NewStringUTF(probe);
                jclass probeClass = nullptr;
                if (m_LoadClassMethod)
                    probeClass = (jclass)env->CallObjectMethod(classLoader, m_LoadClassMethod, jname);
                if (!probeClass)
                {
                    env->ExceptionClear();
                    probeClass = (jclass)env->CallObjectMethod(classLoader, m_FindClassMethod, jname);
                }
                env->DeleteLocalRef(jname);

                if (probeClass)
                {
                    m_ClassLoader = env->NewGlobalRef(classLoader);
                    Logger::Log("Found classloader via thread %d, can load %s", i, probe);
                    env->DeleteLocalRef(probeClass);
                    env->DeleteLocalRef(classLoader);
                    env->DeleteLocalRef(thread);
                    env->DeleteLocalRef(threads);
                    env->DeleteLocalRef(threadsSet);
                    env->DeleteLocalRef(stackTracesMap);
                    goto cleanup;
                }
                env->ExceptionClear();
            }
        }

        env->DeleteLocalRef(thread);
    }

    Logger::Log("Failed to find classloader via thread scanning");

cleanup:
    env->DeleteLocalRef(threadClass);
    env->DeleteLocalRef(mapClass);
    env->DeleteLocalRef(setClass);
    env->DeleteLocalRef(classLoaderClass);
}

bool Java::ProbeClassLoader(jobject classLoader)
{
    JNIEnv* env = GetEnv();
    if (!classLoader || !m_LoadClassMethod || !env) return false;

    const char* probeNames[] = {
        "net.minecraft.client.Minecraft",
        "ave",
        "com.moonsworth.lunar.bridge.client.MinecraftBridge"
    };

    for (const char* probe : probeNames)
    {
        jstring jname = env->NewStringUTF(probe);
        jclass probeClass = (jclass)env->CallObjectMethod(classLoader, m_LoadClassMethod, jname);
        if (!probeClass)
        {
            env->ExceptionClear();
            if (m_FindClassMethod)
                probeClass = (jclass)env->CallObjectMethod(classLoader, m_FindClassMethod, jname);
        }
        env->DeleteLocalRef(jname);

        if (probeClass)
        {
            env->DeleteLocalRef(probeClass);
            return true;
        }
        env->ExceptionClear();
    }
    return false;
}

void Java::TryFindClassLoader()
{
    JNIEnv* env = GetEnv();
    if (m_ClassLoader || !env) return;

    jclass threadClass = env->FindClass("java/lang/Thread");
    if (!threadClass) { env->ExceptionClear(); return; }

    jmethodID currentThread = env->GetStaticMethodID(threadClass, "currentThread", "()Ljava/lang/Thread;");
    jmethodID getContextCL = env->GetMethodID(threadClass, "getContextClassLoader", "()Ljava/lang/ClassLoader;");

    if (currentThread && getContextCL)
    {
        jobject thread = env->CallStaticObjectMethod(threadClass, currentThread);
        jobject cl = env->CallObjectMethod(thread, getContextCL);
        if (cl && ProbeClassLoader(cl))
        {
            m_ClassLoader = env->NewGlobalRef(cl);
            Logger::Log("Found classloader via current thread");
            env->DeleteLocalRef(cl);
            env->DeleteLocalRef(thread);
            env->DeleteLocalRef(threadClass);
            return;
        }
        if (cl) env->DeleteLocalRef(cl);
        env->ExceptionClear();
        env->DeleteLocalRef(thread);
    }
    env->DeleteLocalRef(threadClass);
}

jclass Java::FindClass(const std::string& name)
{
    JNIEnv* env = GetEnv();
    if (!env) return nullptr;
    jclass cls = env->FindClass(name.c_str());
    if (cls) return cls;
    env->ExceptionClear();

    if (m_ClassLoader)
    {
        std::string dotName = name;
        for (auto& c : dotName) if (c == '/') c = '.';
        jstring jname = env->NewStringUTF(dotName.c_str());
        if (m_LoadClassMethod)
        {
            cls = (jclass)env->CallObjectMethod(m_ClassLoader, m_LoadClassMethod, jname);
            if (cls) { env->DeleteLocalRef(jname); return cls; }
            env->ExceptionClear();
        }
        if (m_FindClassMethod)
        {
            cls = (jclass)env->CallObjectMethod(m_ClassLoader, m_FindClassMethod, jname);
            if (cls) { env->DeleteLocalRef(jname); return cls; }
            env->ExceptionClear();
        }
        env->DeleteLocalRef(jname);
    }

    return nullptr;
}

jclass Java::FindClass(const std::string& obfName, const std::string& deobfName)
{
    JNIEnv* env = GetEnv();
    if (!env) return nullptr;
    jclass cls = env->FindClass(obfName.c_str());
    if (cls) return cls;
    env->ExceptionClear();

    cls = env->FindClass(deobfName.c_str());
    if (cls) return cls;
    env->ExceptionClear();

    if (!m_ClassLoader)
        TryFindClassLoader();

    if (m_ClassLoader)
    {
        std::string deobfDot = deobfName;
        for (auto& c : deobfDot) if (c == '/') c = '.';
        jstring jname = env->NewStringUTF(deobfDot.c_str());
        if (m_LoadClassMethod)
        {
            cls = (jclass)env->CallObjectMethod(m_ClassLoader, m_LoadClassMethod, jname);
            if (cls) { env->DeleteLocalRef(jname); return cls; }
            env->ExceptionClear();
        }
        env->DeleteLocalRef(jname);

        jstring jobf = env->NewStringUTF(obfName.c_str());
        if (m_LoadClassMethod)
        {
            cls = (jclass)env->CallObjectMethod(m_ClassLoader, m_LoadClassMethod, jobf);
            if (cls) { env->DeleteLocalRef(jobf); return cls; }
            env->ExceptionClear();
        }
        env->DeleteLocalRef(jobf);
    }

    return nullptr;
}

bool Java::FindJVM()
{
    const char* searchModules[] = {
        "jvm.dll",
        "hotspot.dll"
    };

    for (const char* modName : searchModules)
    {
        HMODULE hMod = GetModuleHandleA(modName);
        if (hMod)
        {
            m_JvmModule = hMod;
            Logger::Log("Found JVM module: %s", modName);
            return true;
        }
    }

    HMODULE hMods[1024];
    HANDLE hProcess = GetCurrentProcess();
    DWORD cbNeeded;

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            char modName[MAX_PATH];
            if (GetModuleBaseNameA(hProcess, hMods[i], modName, sizeof(modName)))
            {
                if (_stricmp(modName, "jvm.dll") == 0)
                {
                    m_JvmModule = hMods[i];
                    Logger::Log("Found jvm.dll via enumeration");
                    return true;
                }
            }
        }
    }

    Logger::Log("jvm.dll not found in process");
    return false;
}

bool Java::AttachAsDaemon()
{
    JNI_GetCreatedJavaVMs_t GetCreatedJavaVMs =
        (JNI_GetCreatedJavaVMs_t)GetProcAddress(m_JvmModule, "JNI_GetCreatedJavaVMs");

    if (!GetCreatedJavaVMs)
    {
        Logger::Log("Failed to get JNI_GetCreatedJavaVMs function");
        return false;
    }

    jsize vmCount = 0;
    JavaVM* vmBuf[1];
    jint result = GetCreatedJavaVMs(vmBuf, 1, &vmCount);

    if (result != JNI_OK || vmCount == 0)
    {
        Logger::Log("No Java VMs created yet");
        return false;
    }

    m_Jvm = vmBuf[0];

    JNIEnv* env = nullptr;
    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_8;
    args.name = "PaperScents";
    args.group = nullptr;

    result = m_Jvm->AttachCurrentThreadAsDaemon((void**)&env, &args);

    if (result != JNI_OK || !env)
    {
        Logger::Log("AttachCurrentThreadAsDaemon failed");
        return false;
    }

    Logger::Log("Attached to JVM v1.8 as daemon thread");
    return true;
}

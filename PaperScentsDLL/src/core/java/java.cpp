#include "java.h"
#include "../utilities/logger.h"
#include <vector>
#include <psapi.h>
#pragma comment(lib, "psapi")

typedef jint(JNICALL* JNI_GetCreatedJavaVMs_t)(JavaVM**, jsize, jsize*);

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
    if (m_ClassLoader && m_Env)
    {
        m_Env->DeleteGlobalRef(m_ClassLoader);
        m_ClassLoader = nullptr;
    }

    if (m_Env && m_Jvm)
    {
        m_Jvm->DetachCurrentThread();
        m_Env = nullptr;
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
    jclass threadClass = m_Env->FindClass("java/lang/Thread");
    jclass mapClass = m_Env->FindClass("java/util/Map");
    jclass setClass = m_Env->FindClass("java/util/Set");
    jclass classLoaderClass = m_Env->FindClass("java/lang/ClassLoader");

    if (!threadClass || !mapClass || !setClass || !classLoaderClass)
    {
        m_Env->ExceptionClear();
        Logger::Log("Failed to find thread/collection classes for classloader setup");
        return;
    }

    jmethodID getAllStackTraces = m_Env->GetStaticMethodID(threadClass, "getAllStackTraces", "()Ljava/util/Map;");
    jmethodID keySet = m_Env->GetMethodID(mapClass, "keySet", "()Ljava/util/Set;");
    jmethodID toArray = m_Env->GetMethodID(setClass, "toArray", "()[Ljava/lang/Object;");
    jmethodID getContextCL = m_Env->GetMethodID(threadClass, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
    m_FindClassMethod = m_Env->GetMethodID(classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    m_LoadClassMethod = m_Env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    if (!getAllStackTraces || !keySet || !toArray || !getContextCL || !m_FindClassMethod)
    {
        m_Env->ExceptionClear();
        Logger::Log("Failed to get method IDs for classloader setup");
        return;
    }

    jobject stackTracesMap = m_Env->CallStaticObjectMethod(threadClass, getAllStackTraces);
    jobject threadsSet = m_Env->CallObjectMethod(stackTracesMap, keySet);
    jobjectArray threads = (jobjectArray)m_Env->CallObjectMethod(threadsSet, toArray);
    jint threadCount = m_Env->GetArrayLength(threads);

    const char* probeNames[] = {
        "net.minecraft.client.Minecraft",
        "ave",
        "com.moonsworth.lunar.bridge.client.MinecraftBridge"
    };

    for (int i = 0; i < threadCount; i++)
    {
        jobject thread = m_Env->GetObjectArrayElement(threads, i);
        jobject classLoader = m_Env->CallObjectMethod(thread, getContextCL);

        if (classLoader)
        {
            for (const char* probe : probeNames)
            {
                jstring jname = m_Env->NewStringUTF(probe);
                jclass probeClass = nullptr;
                if (m_LoadClassMethod)
                    probeClass = (jclass)m_Env->CallObjectMethod(classLoader, m_LoadClassMethod, jname);
                if (!probeClass)
                {
                    m_Env->ExceptionClear();
                    probeClass = (jclass)m_Env->CallObjectMethod(classLoader, m_FindClassMethod, jname);
                }
                m_Env->DeleteLocalRef(jname);

                if (probeClass)
                {
                    m_ClassLoader = m_Env->NewGlobalRef(classLoader);
                    Logger::Log("Found classloader via thread %d, can load %s", i, probe);
                    m_Env->DeleteLocalRef(probeClass);
                    m_Env->DeleteLocalRef(classLoader);
                    m_Env->DeleteLocalRef(thread);
                    goto cleanup;
                }
                m_Env->ExceptionClear();
            }
        }

        m_Env->DeleteLocalRef(thread);
    }

    Logger::Log("Failed to find classloader via thread scanning");

cleanup:
    m_Env->DeleteLocalRef(threads);
    m_Env->DeleteLocalRef(threadsSet);
    m_Env->DeleteLocalRef(stackTracesMap);
    m_Env->DeleteLocalRef(threadClass);
    m_Env->DeleteLocalRef(mapClass);
    m_Env->DeleteLocalRef(setClass);
    m_Env->DeleteLocalRef(classLoaderClass);
}

bool Java::ProbeClassLoader(jobject classLoader)
{
    if (!classLoader || !m_LoadClassMethod) return false;

    const char* probeNames[] = {
        "net.minecraft.client.Minecraft",
        "ave",
        "com.moonsworth.lunar.bridge.client.MinecraftBridge"
    };

    for (const char* probe : probeNames)
    {
        jstring jname = m_Env->NewStringUTF(probe);
        jclass probeClass = (jclass)m_Env->CallObjectMethod(classLoader, m_LoadClassMethod, jname);
        if (!probeClass)
        {
            m_Env->ExceptionClear();
            if (m_FindClassMethod)
                probeClass = (jclass)m_Env->CallObjectMethod(classLoader, m_FindClassMethod, jname);
        }
        m_Env->DeleteLocalRef(jname);

        if (probeClass)
        {
            m_Env->DeleteLocalRef(probeClass);
            return true;
        }
        m_Env->ExceptionClear();
    }
    return false;
}

void Java::TryFindClassLoader()
{
    if (m_ClassLoader) return;

    jclass threadClass = m_Env->FindClass("java/lang/Thread");
    if (!threadClass) { m_Env->ExceptionClear(); return; }

    jmethodID currentThread = m_Env->GetStaticMethodID(threadClass, "currentThread", "()Ljava/lang/Thread;");
    jmethodID getContextCL = m_Env->GetMethodID(threadClass, "getContextClassLoader", "()Ljava/lang/ClassLoader;");

    if (currentThread && getContextCL)
    {
        jobject thread = m_Env->CallStaticObjectMethod(threadClass, currentThread);
        jobject cl = m_Env->CallObjectMethod(thread, getContextCL);
        if (cl && ProbeClassLoader(cl))
        {
            m_ClassLoader = m_Env->NewGlobalRef(cl);
            Logger::Log("Found classloader via current thread");
            m_Env->DeleteLocalRef(cl);
            m_Env->DeleteLocalRef(thread);
            m_Env->DeleteLocalRef(threadClass);
            return;
        }
        if (cl) m_Env->DeleteLocalRef(cl);
        m_Env->ExceptionClear();
        m_Env->DeleteLocalRef(thread);
    }
    m_Env->DeleteLocalRef(threadClass);
}

jclass Java::FindClass(const std::string& name)
{
    if (!m_Env) return nullptr;
    jclass cls = m_Env->FindClass(name.c_str());
    if (cls) return cls;
    m_Env->ExceptionClear();

    if (m_ClassLoader)
    {
        std::string dotName = name;
        for (auto& c : dotName) if (c == '/') c = '.';
        jstring jname = m_Env->NewStringUTF(dotName.c_str());
        if (m_LoadClassMethod)
        {
            cls = (jclass)m_Env->CallObjectMethod(m_ClassLoader, m_LoadClassMethod, jname);
            if (cls) { m_Env->DeleteLocalRef(jname); return cls; }
            m_Env->ExceptionClear();
        }
        if (m_FindClassMethod)
        {
            cls = (jclass)m_Env->CallObjectMethod(m_ClassLoader, m_FindClassMethod, jname);
            if (cls) { m_Env->DeleteLocalRef(jname); return cls; }
            m_Env->ExceptionClear();
        }
        m_Env->DeleteLocalRef(jname);
    }

    return nullptr;
}

jclass Java::FindClass(const std::string& obfName, const std::string& deobfName)
{
    jclass cls = m_Env->FindClass(obfName.c_str());
    if (cls) return cls;
    m_Env->ExceptionClear();

    cls = m_Env->FindClass(deobfName.c_str());
    if (cls) return cls;
    m_Env->ExceptionClear();

    if (!m_ClassLoader)
        TryFindClassLoader();

    if (m_ClassLoader)
    {
        std::string deobfDot = deobfName;
        for (auto& c : deobfDot) if (c == '/') c = '.';
        jstring jname = m_Env->NewStringUTF(deobfDot.c_str());
        if (m_LoadClassMethod)
        {
            cls = (jclass)m_Env->CallObjectMethod(m_ClassLoader, m_LoadClassMethod, jname);
            if (cls) { m_Env->DeleteLocalRef(jname); return cls; }
            m_Env->ExceptionClear();
        }
        m_Env->DeleteLocalRef(jname);

        jstring jobf = m_Env->NewStringUTF(obfName.c_str());
        if (m_LoadClassMethod)
        {
            cls = (jclass)m_Env->CallObjectMethod(m_ClassLoader, m_LoadClassMethod, jobf);
            if (cls) { m_Env->DeleteLocalRef(jobf); return cls; }
            m_Env->ExceptionClear();
        }
        m_Env->DeleteLocalRef(jobf);
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

    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_8;
    args.name = "PaperScents";
    args.group = nullptr;

    result = m_Jvm->AttachCurrentThreadAsDaemon((void**)&m_Env, &args);

    if (result != JNI_OK || !m_Env)
    {
        Logger::Log("AttachCurrentThreadAsDaemon failed");
        return false;
    }

    Logger::Log("Attached to JVM v1.8 as daemon thread");
    return true;
}

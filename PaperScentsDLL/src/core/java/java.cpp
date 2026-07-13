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
    if (m_BridgeMinecraftClass && env)
    {
        env->DeleteGlobalRef(m_BridgeMinecraftClass);
        m_BridgeMinecraftClass = nullptr;
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

    // --- Helper: try to load a class using a given classloader ---
    auto tryLoad = [&](jobject cl, const char* className) -> jclass {
        jstring jname = env->NewStringUTF(className);
        jclass result = nullptr;
        if (m_LoadClassMethod)
            result = (jclass)env->CallObjectMethod(cl, m_LoadClassMethod, jname);
        if (!result)
        {
            env->ExceptionClear();
            if (m_FindClassMethod)
                result = (jclass)env->CallObjectMethod(cl, m_FindClassMethod, jname);
        }
        env->DeleteLocalRef(jname);
        return result;
    };

    if (!getAllStackTraces || !keySet || !toArray || !getContextCL || !m_FindClassMethod)
    {
        env->ExceptionClear();
        Logger::Log("Failed to get method IDs for classloader setup");
        env->DeleteLocalRef(threadClass);
        env->DeleteLocalRef(mapClass);
        env->DeleteLocalRef(setClass);
        env->DeleteLocalRef(classLoaderClass);
        return;
    }

    const char* bridgeClass = "com.moonsworth.lunar.bridge.client.MinecraftBridge";
    const char* mcClass = "net.minecraft.client.Minecraft";

    jobjectArray threads = nullptr;
    jobject threadsSet = nullptr;
    jobject stackTracesMap = nullptr;
    jint threadCount = 0;

    // Only build thread list if we need it (phases 1 & 2)
    auto scanThreads = [&]() {
        if (threads) return; // already built
        stackTracesMap = env->CallStaticObjectMethod(threadClass, getAllStackTraces);
        threadsSet = env->CallObjectMethod(stackTracesMap, keySet);
        threads = (jobjectArray)env->CallObjectMethod(threadsSet, toArray);
        threadCount = env->GetArrayLength(threads);
    };

    // --- Phase 1: thread scanning for BRIDGE-capable classloader ---
    {
        scanThreads();
        for (int i = 0; i < threadCount; i++)
        {
            jobject thread = env->GetObjectArrayElement(threads, i);
            jobject cl = env->CallObjectMethod(thread, getContextCL);
            if (cl)
            {
                jclass probe = tryLoad(cl, bridgeClass);
                if (probe)
                {
                    m_ClassLoader = env->NewGlobalRef(cl);
                    Logger::Log("Found classloader via thread %d — can load bridge interface", i);
                    env->DeleteLocalRef(probe);
                    env->DeleteLocalRef(cl);
                    env->DeleteLocalRef(thread);
                    goto done;
                }
                env->ExceptionClear();
            }
            env->DeleteLocalRef(cl);
            env->DeleteLocalRef(thread);
        }
        Logger::Log("Phase 1: no thread context classloader can load bridge interface");
    }

    // --- Phase 2: thread scanning for MINECRAFT-capable classloader ---
    // Fallback — the old approach. This classloader may NOT be able to load
    // bridges, but we can use it for SDK classes and try other methods for bridges.
    {
        for (int i = 0; i < threadCount; i++)
        {
            jobject thread = env->GetObjectArrayElement(threads, i);
            jobject cl = env->CallObjectMethod(thread, getContextCL);
            if (cl)
            {
                jclass probe = tryLoad(cl, mcClass);
                if (probe)
                {
                    m_ClassLoader = env->NewGlobalRef(cl);
                    Logger::Log("Phase 2: found classloader via thread %d — can load Minecraft", i);
                    env->DeleteLocalRef(probe);
                    env->DeleteLocalRef(cl);
                    env->DeleteLocalRef(thread);
                    goto after_phase2;
                }
                env->ExceptionClear();
            }
            env->DeleteLocalRef(cl);
            env->DeleteLocalRef(thread);
        }
        Logger::Log("Phase 2: no thread context classloader can load Minecraft either");
    }
after_phase2:

    // --- Phase 3: get Minecraft Class object; try its interfaces for bridge ---
    // In Lunar, Minecraft implements MinecraftBridge. If we can find that interface
    // through getInterfaces(), we can get its classloader directly — even if the
    // interface's classloader isn't a context classloader of any thread.
    {
        Logger::Log("Phase 3: trying to find bridge via Minecraft's interfaces...");
        jclass classClass = env->FindClass("java/lang/Class");
        jmethodID getClassLoaderMid = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
        jmethodID getInterfacesMid = env->GetMethodID(classClass, "getInterfaces", "()[Ljava/lang/Class;");
        jmethodID getNameMid = env->GetMethodID(classClass, "getName", "()Ljava/lang/String;");

        // Try to get Minecraft Class object
        jclass mcClassObj = env->FindClass("net/minecraft/client/Minecraft");
        if (!mcClassObj) { env->ExceptionClear(); mcClassObj = (jclass)env->FindClass("ave"); env->ExceptionClear(); }
        if (!mcClassObj && m_ClassLoader) { mcClassObj = tryLoad(m_ClassLoader, mcClass); env->ExceptionClear(); }

        if (mcClassObj)
        {
            Logger::Log("Phase 3: got Minecraft class object");

            // Method A: Check Minecraft's interfaces for a bridge interface
            jobjectArray ifaces = (jobjectArray)env->CallObjectMethod(mcClassObj, getInterfacesMid);
            if (ifaces)
            {
                jint ifCount = env->GetArrayLength(ifaces);
                Logger::Log("Phase 3: Minecraft has %d interfaces", ifCount);
                for (jint i = 0; i < ifCount; i++)
                {
                    jclass iface = (jclass)env->GetObjectArrayElement(ifaces, i);
                    if (!iface) continue;
                    jstring ifName = (jstring)env->CallObjectMethod(iface, getNameMid);
                    const char* ifNameStr = ifName ? env->GetStringUTFChars(ifName, nullptr) : "null";
                    Logger::Log("Phase 3: interface[%d] = %s", i, ifNameStr ? ifNameStr : "null");
                    // Accept any Lunar interface (bridge or obfuscated), not just bridge.*
                    if (ifNameStr && strstr(ifNameStr, "com.moonsworth.lunar.") == ifNameStr)
                    {
                        Logger::Log("Phase 3: FOUND Lunar interface: %s", ifNameStr);
                        jobject brLoader = env->CallObjectMethod(iface, getClassLoaderMid);
                        if (brLoader)
                        {
                            if (m_ClassLoader) env->DeleteGlobalRef(m_ClassLoader);
                            m_ClassLoader = env->NewGlobalRef(brLoader);
                            Logger::Log("Phase 3: classloader from %s", ifNameStr);

                            // Store the bridge interface so BridgeHelper can use it directly
                            m_BridgeMinecraftClass = (jclass)env->NewGlobalRef(iface);
                            Logger::Log("Phase 3: stored bridge interface as m_BridgeMinecraftClass");
                            env->DeleteLocalRef(brLoader);
                        }
                        env->ReleaseStringUTFChars(ifName, ifNameStr);
                        env->DeleteLocalRef(iface);
                        env->DeleteLocalRef(ifaces);
                        env->DeleteLocalRef(mcClassObj);
                        env->DeleteLocalRef(classClass);
                        goto done;
                    }
                    if (ifNameStr) env->ReleaseStringUTFChars(ifName, ifNameStr);
                    env->DeleteLocalRef(iface);
                }
                env->DeleteLocalRef(ifaces);
            }

            // Method B: Try Minecraft.getClassLoader() to see if it's a custom loader
            jobject mcLoader = env->CallObjectMethod(mcClassObj, getClassLoaderMid);
            if (mcLoader)
            {
                jclass probe = tryLoad(mcLoader, bridgeClass);
                if (probe)
                {
                    if (m_ClassLoader) env->DeleteGlobalRef(m_ClassLoader);
                    m_ClassLoader = env->NewGlobalRef(mcLoader);
                    Logger::Log("Phase 3B: found classloader via Minecraft.getClassLoader()");
                    env->DeleteLocalRef(probe);
                }
                env->ExceptionClear();
                env->DeleteLocalRef(mcLoader);
            }
            env->DeleteLocalRef(mcClassObj);
        }
        else
        {
            Logger::Log("Phase 3: could not get Minecraft Class object by any method");
        }
        env->DeleteLocalRef(classClass);
    }

    // --- Phase 4: try Class.forName to load bridge class directly ---
    {
        Logger::Log("Phase 4: trying Class.forName for bridge...");
        jclass classClass = env->FindClass("java/lang/Class");
        jmethodID forName = env->GetStaticMethodID(classClass, "forName", "(Ljava/lang/String;)Ljava/lang/Class;");
        jmethodID getClassLoaderMid = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");

        if (forName && getClassLoaderMid)
        {
            jstring jbridge = env->NewStringUTF(bridgeClass);
            jclass bridgeCls = (jclass)env->CallStaticObjectMethod(classClass, forName, jbridge);
            env->ExceptionClear();
            if (bridgeCls)
            {
                Logger::Log("Phase 4: Class.forName succeeded for bridge");
                jobject brLoader = env->CallObjectMethod(bridgeCls, getClassLoaderMid);
                if (brLoader)
                {
                    if (m_ClassLoader) env->DeleteGlobalRef(m_ClassLoader);
                    m_ClassLoader = env->NewGlobalRef(brLoader);
                    Logger::Log("Phase 4: found classloader via Class.forName bridge -> getClassLoader()");
                    env->DeleteLocalRef(brLoader);
                }
                env->DeleteLocalRef(bridgeCls);
            }
            else
            {
                // Log the exception to understand why Class.forName failed
                jthrowable exc = env->ExceptionOccurred();
                if (exc) {
                    jclass throwableClass = env->FindClass("java/lang/Throwable");
                    jmethodID getMessage = env->GetMethodID(throwableClass, "getMessage", "()Ljava/lang/String;");
                    jstring msg = (jstring)env->CallObjectMethod(exc, getMessage);
                    const char* msgStr = msg ? env->GetStringUTFChars(msg, nullptr) : "null";
                    Logger::Log("Phase 4: Class.forName failed: %s", msgStr ? msgStr : "null");
                    if (msg) env->ReleaseStringUTFChars(msg, msgStr);
                    env->DeleteLocalRef(exc);
                    env->DeleteLocalRef(throwableClass);
                }
                env->ExceptionClear();
            }
            env->DeleteLocalRef(jbridge);
        }
        env->DeleteLocalRef(classClass);
    }

done:
    if (m_ClassLoader)
        Logger::Log("ClassLoader found and ready");
    else
        Logger::Log("FAILED to find any classloader — SDK/bridge loading will fail");

    if (threads) env->DeleteLocalRef(threads);
    if (threadsSet) env->DeleteLocalRef(threadsSet);
    if (stackTracesMap) env->DeleteLocalRef(stackTracesMap);

    env->DeleteLocalRef(threadClass);
    env->DeleteLocalRef(mapClass);
    env->DeleteLocalRef(setClass);
    env->DeleteLocalRef(classLoaderClass);
}

bool Java::ProbeClassLoader(jobject classLoader)
{
    JNIEnv* env = GetEnv();
    if (!classLoader || !m_LoadClassMethod || !env) return false;

    // Only accept classloaders that can load the bridge interface
    const char* bridgeProbe = "com.moonsworth.lunar.bridge.client.MinecraftBridge";
    jstring jname = env->NewStringUTF(bridgeProbe);
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

    // Try env->FindClass with slash-separated names (JNI format)
    auto toSlash = [](const std::string& n) {
        std::string s = n;
        for (auto& c : s) if (c == '.') c = '/';
        return s;
    };
    std::string obfSlash = toSlash(obfName);
    std::string deobfSlash = toSlash(deobfName);

    jclass cls = env->FindClass(obfSlash.c_str());
    if (cls) return cls;
    env->ExceptionClear();

    if (obfSlash != deobfSlash)
    {
        cls = env->FindClass(deobfSlash.c_str());
        if (cls) return cls;
        env->ExceptionClear();
    }

    if (!m_ClassLoader)
        TryFindClassLoader();

    if (m_ClassLoader)
    {
        // ClassLoader.loadClass expects dot-separated names
        std::string deobfDot = deobfName;
        for (auto& c : deobfDot) if (c == '/') c = '.';
        jstring jname = env->NewStringUTF(deobfDot.c_str());
        if (m_LoadClassMethod)
        {
            cls = (jclass)env->CallObjectMethod(m_ClassLoader, m_LoadClassMethod, jname);
            if (cls) { env->DeleteLocalRef(jname); return cls; }
            env->ExceptionClear();
        }
        if (!cls && m_FindClassMethod)
        {
            cls = (jclass)env->CallObjectMethod(m_ClassLoader, m_FindClassMethod, jname);
            if (cls) { env->DeleteLocalRef(jname); return cls; }
            env->ExceptionClear();
        }
        env->DeleteLocalRef(jname);

        if (!cls)
        {
            std::string obfDot = obfName;
            for (auto& c : obfDot) if (c == '/') c = '.';
            if (obfDot != deobfDot)
            {
                jstring jobf = env->NewStringUTF(obfDot.c_str());
                if (m_LoadClassMethod)
                {
                    cls = (jclass)env->CallObjectMethod(m_ClassLoader, m_LoadClassMethod, jobf);
                    if (cls) { env->DeleteLocalRef(jobf); return cls; }
                    env->ExceptionClear();
                }
                if (!cls && m_FindClassMethod)
                {
                    cls = (jclass)env->CallObjectMethod(m_ClassLoader, m_FindClassMethod, jobf);
                    if (cls) { env->DeleteLocalRef(jobf); return cls; }
                    env->ExceptionClear();
                }
                env->DeleteLocalRef(jobf);
            }
        }
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

#pragma once
#include <jni.h>
#include <unordered_map>
#include <string>

class StrayCache
{
public:
    static StrayCache& GetInstance();

    void Initialize(JNIEnv* env);

    jclass GetClass(const std::string& name);
    jmethodID GetMethodID(jclass clazz, const std::string& name, const std::string& sig);
    jfieldID GetFieldID(jclass clazz, const std::string& name, const std::string& sig);

private:
    StrayCache() = default;
    ~StrayCache() = default;
    StrayCache(const StrayCache&) = delete;
    StrayCache& operator=(const StrayCache&) = delete;

    JNIEnv* m_Env = nullptr;
    std::unordered_map<std::string, jclass> m_Classes;
};

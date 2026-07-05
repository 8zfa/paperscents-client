#pragma once
#include <jni.h>
#include <string>

class JString
{
public:
    static JString& GetInstance();

    bool Initialize(JNIEnv* env);

    std::string ToString(jstring str);
    jstring FromString(const std::string& str);

private:
    JString() = default;
    ~JString() = default;
    JString(const JString&) = delete;
    JString& operator=(const JString&) = delete;

    JNIEnv* m_Env = nullptr;
};

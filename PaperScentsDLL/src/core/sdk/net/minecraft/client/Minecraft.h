#pragma once
#include <jni.h>
#include "entity/EntityPlayerSP.h"

class Minecraft
{
public:
    static Minecraft* GetInstance();

    bool Initialize(JNIEnv* env);

    jclass GetClass() { return m_Class; }
    jobject GetObject() { return m_Object; }

    EntityPlayerSP* GetPlayer() { return m_Player; }

    static constexpr const char* GetObfClassName() { return "ave"; }

private:
    JNIEnv* m_Env = nullptr;
    jclass m_Class = nullptr;
    jobject m_Object = nullptr;
    EntityPlayerSP* m_Player = nullptr;
};

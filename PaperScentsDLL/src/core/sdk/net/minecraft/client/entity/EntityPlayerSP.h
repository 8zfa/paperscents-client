#pragma once
#include <jni.h>
#include "../../entity/player/EntityPlayer.h"

class EntityPlayerSP : public EntityPlayer
{
public:
    bool Initialize(JNIEnv* env, jobject obj);

    static constexpr const char* GetObfClassName() { return "bew"; }

private:
    JNIEnv* m_Env = nullptr;
};

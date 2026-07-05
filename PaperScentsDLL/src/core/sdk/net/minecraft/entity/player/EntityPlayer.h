#pragma once
#include <jni.h>
#include "../EntityLivingBase.h"

class EntityPlayer : public EntityLivingBase
{
public:
    bool Initialize(JNIEnv* env, jobject obj) override;

    static constexpr const char* GetObfClassName() { return "wn"; }

protected:
    JNIEnv* m_Env = nullptr;
};

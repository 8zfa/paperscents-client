#pragma once
#include "Entity.h"

class EntityLivingBase : public Entity
{
public:
    bool Initialize(JNIEnv* env, jobject obj) override;

    static constexpr const char* GetObfClassName() { return "pr"; }

protected:
    JNIEnv* m_Env = nullptr;
};

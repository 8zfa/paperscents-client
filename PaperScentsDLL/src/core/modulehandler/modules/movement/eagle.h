#pragma once
#include "../../modulebase.h"
#include <jni.h>

class EagleModule : public ModuleBase
{
public:
    EagleModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    bool ShouldSneak(JNIEnv* env, jobject player);
    int m_SneakDelay = 0;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

#pragma once
#include "../../modulebase.h"
#include <jni.h>

class FullBrightModule : public ModuleBase
{
public:
    FullBrightModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    void SetGamma();
    void ApplyNightVision(JNIEnv* env, jobject player);
    void RemoveNightVision(JNIEnv* env, jobject player);
    float m_PrevGamma = NAN;
    bool m_AppliedNightVision = false;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

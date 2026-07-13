#pragma once
#include "../../modulebase.h"
#include <jni.h>

class FastPlaceModule : public ModuleBase
{
public:
    FastPlaceModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    bool CanPlace(JNIEnv* env, jobject player);
    int m_DelayTicks = 0;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

#pragma once
#include "../../modulebase.h"
#include <jni.h>

class VelocityModule : public ModuleBase
{
public:
    VelocityModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    int m_HurtTimePrev = 0;
    int m_JumpCooldown = 0;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

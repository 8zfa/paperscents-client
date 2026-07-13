#pragma once
#include "../../modulebase.h"
#include <jni.h>

class WTapModule : public ModuleBase
{
public:
    WTapModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    int m_DelayTicks = 0;
    int m_DurationTicks = 0;
    bool m_Active = false;
    int m_PrevHurtTime = 0;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

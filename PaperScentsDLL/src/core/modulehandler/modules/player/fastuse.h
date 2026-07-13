#pragma once
#include "../../modulebase.h"
#include "../../../java/java.h"

class FastUseModule : public ModuleBase
{
public:
    FastUseModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    jfieldID m_TimerField = nullptr;
    jfieldID m_SpeedField = nullptr;
    jobject m_TimerObj = nullptr;
    bool m_Cached = false;
    float m_OriginalSpeed = 1.0f;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

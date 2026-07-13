#pragma once
#include "../../modulebase.h"
#include <jni.h>
#include <chrono>

class CriticalsModule : public ModuleBase
{
public:
    CriticalsModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    std::chrono::steady_clock::time_point m_LastCrit;
    bool m_AppliedThisTick = false;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

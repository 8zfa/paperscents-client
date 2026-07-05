#pragma once
#include "../../modulebase.h"
#include <chrono>

class WTapModule : public ModuleBase
{
public:
    WTapModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    enum State { None, Waiting, Tapping };
    State m_State = None;
    std::chrono::steady_clock::time_point m_Timer;
    long m_WaitMs = 0;
    long m_TapMs = 0;
    bool m_AttackedThisFrame = false;
    bool m_WasForwardDown = false;
    long m_Hits = 0;
    long m_TargetHits = 0;
};

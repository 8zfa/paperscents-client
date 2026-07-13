#pragma once
#include "../../modulebase.h"
#include <chrono>

class AutoTipModule : public ModuleBase
{
public:
    AutoTipModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    std::chrono::steady_clock::time_point m_LastTip;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

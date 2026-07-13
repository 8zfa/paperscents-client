#pragma once
#include "../../modulebase.h"
#include <chrono>

class RodAuraModule : public ModuleBase
{
public:
    RodAuraModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    std::chrono::steady_clock::time_point m_LastUse;
    int m_FrameCounter = 0;
    int m_UpdateInterval = 3;
};

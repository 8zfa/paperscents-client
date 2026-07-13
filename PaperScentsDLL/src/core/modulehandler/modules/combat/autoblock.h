#pragma once
#include "../../modulebase.h"
#include <chrono>

class AutoBlockModule : public ModuleBase
{
public:
    AutoBlockModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    std::chrono::steady_clock::time_point m_LastBlock;
    bool m_WasBlocking = false;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

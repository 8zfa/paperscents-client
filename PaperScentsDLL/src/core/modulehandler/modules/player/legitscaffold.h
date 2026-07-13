#pragma once
#include "../../modulebase.h"
#include <chrono>

class LegitScaffoldModule : public ModuleBase
{
public:
    LegitScaffoldModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    std::chrono::steady_clock::time_point m_LastPlace;
    bool m_Towered = false;
    bool m_WasKeyDown = false;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

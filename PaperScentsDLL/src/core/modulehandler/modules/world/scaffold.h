#pragma once
#include "../../modulebase.h"
#include <chrono>

class ScaffoldModule : public ModuleBase
{
public:
    ScaffoldModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    int m_LastBlockSlot = -1;
    std::chrono::steady_clock::time_point m_LastPlace;
    int m_PlaceTicks = 0;
    bool m_Towering = false;
};

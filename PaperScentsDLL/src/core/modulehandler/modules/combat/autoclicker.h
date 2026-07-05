#pragma once
#include "../../modulebase.h"
#include <chrono>

class AutoClickerModule : public ModuleBase
{
public:
    AutoClickerModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    std::chrono::steady_clock::time_point m_LastLClick, m_LastRClick;
};

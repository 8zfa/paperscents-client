#pragma once
#include "../../modulebase.h"
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
    bool m_WasOnGround = false;
    bool m_AppliedThisTick = false;
};

#pragma once
#include "../../modulebase.h"
#include <chrono>

class KillAuraModule : public ModuleBase
{
public:
    KillAuraModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    std::chrono::steady_clock::time_point m_LastAttack;
};

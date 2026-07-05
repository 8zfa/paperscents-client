#pragma once
#include "../../modulebase.h"
#include <chrono>

class TriggerBotModule : public ModuleBase
{
public:
    TriggerBotModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    std::chrono::steady_clock::time_point m_LastAttack;
};

#pragma once
#include "../../modulebase.h"
#include <chrono>

class VelocityModule : public ModuleBase
{
public:
    VelocityModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    std::chrono::steady_clock::time_point m_LastJump;
    int m_JumpCooldown = 0;
};

#pragma once
#include "../../modulebase.h"
#include <chrono>

class AutoPearlModule : public ModuleBase
{
public:
    AutoPearlModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    std::chrono::steady_clock::time_point m_LastClick;
};

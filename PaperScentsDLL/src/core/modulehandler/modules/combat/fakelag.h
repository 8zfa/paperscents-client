#pragma once
#include "../../modulebase.h"
#include <chrono>

class FakeLagModule : public ModuleBase
{
public:
    FakeLagModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    bool m_Lagging;
    std::chrono::steady_clock::time_point m_LagTimer;
};

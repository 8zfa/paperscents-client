#pragma once
#include "../../modulebase.h"
#include <chrono>

class MiddleClickPearlModule : public ModuleBase
{
public:
    MiddleClickPearlModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    std::chrono::steady_clock::time_point m_LastClick;
    bool m_WasPressed = false;
};

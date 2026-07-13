#pragma once
#include "../../modulebase.h"

class FastMineModule : public ModuleBase
{
public:
    FastMineModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

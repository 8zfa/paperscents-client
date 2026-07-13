#pragma once
#include "../../modulebase.h"

class SpeedModule : public ModuleBase
{
public:
    SpeedModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

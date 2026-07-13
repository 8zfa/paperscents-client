#pragma once
#include "../../modulebase.h"

class HighJumpModule : public ModuleBase
{
public:
    HighJumpModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    bool m_OnGround = false;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

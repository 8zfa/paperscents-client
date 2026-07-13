#pragma once
#include "../../modulebase.h"

class LongJumpModule : public ModuleBase
{
public:
    LongJumpModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    bool m_Jumping = false;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

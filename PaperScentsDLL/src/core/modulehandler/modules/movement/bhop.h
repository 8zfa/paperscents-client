#pragma once
#include "../../modulebase.h"

class BHopModule : public ModuleBase
{
public:
    BHopModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

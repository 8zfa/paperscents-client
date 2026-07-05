#pragma once
#include "../../modulebase.h"

class HitBoxModule : public ModuleBase
{
public:
    HitBoxModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    int m_FrameCounter = 0;
};

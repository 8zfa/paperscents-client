#pragma once
#include "../../modulebase.h"

class ZoomModule : public ModuleBase
{
public:
    ZoomModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    float m_OriginalFov = 90.0f;
    bool m_Cached = false;
    bool m_WasZooming = false;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

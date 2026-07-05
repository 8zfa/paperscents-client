#pragma once
#include "../../modulebase.h"
#include <windows.h>

class FreeCamModule : public ModuleBase
{
public:
    FreeCamModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    double m_OriginalX = 0.0;
    double m_OriginalY = 0.0;
    double m_OriginalZ = 0.0;
    float m_OriginalYaw = 0.0f;
    float m_OriginalPitch = 0.0f;
    POINT m_LastCursor = {};
    bool m_HasStoredPosition = false;
};

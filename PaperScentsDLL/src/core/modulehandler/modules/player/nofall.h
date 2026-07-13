#pragma once
#include "../../modulebase.h"
#include <jni.h>

class NoFallModule : public ModuleBase
{
public:
    NoFallModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    double m_LastSafeX = 0.0, m_LastSafeY = 0.0, m_LastSafeZ = 0.0;
    bool m_HasLastSafe = false;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

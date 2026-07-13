#pragma once
#include "../../modulebase.h"
#include <jni.h>

class ReachModule : public ModuleBase
{
public:
    ReachModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    bool m_WasDown = false;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

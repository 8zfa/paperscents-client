#pragma once
#include "../../modulebase.h"
#include <jni.h>

class AutoEatModule : public ModuleBase
{
public:
    AutoEatModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    bool m_IsEating = false;
    int m_LastFoodLevel = 20;
};

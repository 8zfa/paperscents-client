#pragma once
#include "../../modulebase.h"

class TimerModule : public ModuleBase
{
public:
    TimerModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

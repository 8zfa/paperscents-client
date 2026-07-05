#pragma once
#include "../../modulebase.h"

class TimeChangerModule : public ModuleBase
{
public:
    TimeChangerModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

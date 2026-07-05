#pragma once
#include "../../modulebase.h"

class StepModule : public ModuleBase
{
public:
    StepModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

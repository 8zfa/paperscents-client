#pragma once
#include "../../modulebase.h"

class KeepSprintModule : public ModuleBase
{
public:
    KeepSprintModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

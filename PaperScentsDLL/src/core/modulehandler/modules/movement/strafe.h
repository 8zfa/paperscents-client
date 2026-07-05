#pragma once
#include "../../modulebase.h"

class StrafeModule : public ModuleBase
{
public:
    StrafeModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

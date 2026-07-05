#pragma once
#include "../../modulebase.h"

class LiquidWalkModule : public ModuleBase
{
public:
    LiquidWalkModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

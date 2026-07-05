#pragma once
#include "../../modulebase.h"

class FastMineModule : public ModuleBase
{
public:
    FastMineModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

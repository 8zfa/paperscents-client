#pragma once
#include "../../modulebase.h"

class NoSlowModule : public ModuleBase
{
public:
    NoSlowModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

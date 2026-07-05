#pragma once
#include "../../modulebase.h"

class TeamsModule : public ModuleBase
{
public:
    TeamsModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

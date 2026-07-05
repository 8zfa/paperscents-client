#pragma once
#include "../../modulebase.h"

class SprintModule : public ModuleBase
{
public:
    SprintModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

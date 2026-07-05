#pragma once
#include "../../modulebase.h"

class NoHurtCamModule : public ModuleBase
{
public:
    NoHurtCamModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

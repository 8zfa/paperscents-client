#pragma once
#include "../../modulebase.h"

class TargetHUDModule : public ModuleBase
{
public:
    TargetHUDModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnRender() override;
};

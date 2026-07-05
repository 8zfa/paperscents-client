#pragma once
#include "../../modulebase.h"

class DirectionHUDModule : public ModuleBase
{
public:
    DirectionHUDModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnRender() override;
};

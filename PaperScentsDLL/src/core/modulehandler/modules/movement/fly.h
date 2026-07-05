#pragma once
#include "../../modulebase.h"

class FlyModule : public ModuleBase
{
public:
    FlyModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

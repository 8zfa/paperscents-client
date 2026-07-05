#pragma once
#include "../../modulebase.h"

class FullBrightModule : public ModuleBase
{
public:
    FullBrightModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

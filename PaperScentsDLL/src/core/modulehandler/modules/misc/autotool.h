#pragma once
#include "../../modulebase.h"

class AutoToolModule : public ModuleBase
{
public:
    AutoToolModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

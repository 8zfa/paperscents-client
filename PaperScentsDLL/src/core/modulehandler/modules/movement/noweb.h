#pragma once
#include "../../modulebase.h"

class NoWebModule : public ModuleBase
{
public:
    NoWebModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

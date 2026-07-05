#pragma once
#include "../../modulebase.h"

class AutoRespawnModule : public ModuleBase
{
public:
    AutoRespawnModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

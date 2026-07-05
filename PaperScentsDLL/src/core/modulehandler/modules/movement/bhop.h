#pragma once
#include "../../modulebase.h"

class BHopModule : public ModuleBase
{
public:
    BHopModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

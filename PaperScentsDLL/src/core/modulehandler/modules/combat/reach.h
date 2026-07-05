#pragma once
#include "../../modulebase.h"

class ReachModule : public ModuleBase
{
public:
    ReachModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

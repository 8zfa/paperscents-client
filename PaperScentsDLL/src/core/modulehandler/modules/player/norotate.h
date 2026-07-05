#pragma once
#include "../../modulebase.h"

class NoRotateModule : public ModuleBase
{
public:
    NoRotateModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

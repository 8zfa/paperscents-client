#pragma once
#include "../../modulebase.h"

class TracersModule : public ModuleBase
{
public:
    TracersModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnRender() override;
};

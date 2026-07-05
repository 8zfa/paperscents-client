#pragma once
#include "../../modulebase.h"

class WatermarkModule : public ModuleBase
{
public:
    WatermarkModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnRender() override;
};

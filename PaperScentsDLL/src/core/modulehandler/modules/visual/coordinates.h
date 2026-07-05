#pragma once
#include "../../modulebase.h"

class CoordinatesModule : public ModuleBase
{
public:
    CoordinatesModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnRender() override;
};

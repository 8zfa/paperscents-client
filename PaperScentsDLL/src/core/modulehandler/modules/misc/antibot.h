#pragma once
#include "../../modulebase.h"

class AntiBotModule : public ModuleBase
{
public:
    AntiBotModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

#pragma once
#include "../../modulebase.h"

class BowAimModule : public ModuleBase
{
public:
    BowAimModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

#pragma once
#include "../../modulebase.h"

class AimAssistModule : public ModuleBase
{
public:
    AimAssistModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
    void OnRender() override;
};

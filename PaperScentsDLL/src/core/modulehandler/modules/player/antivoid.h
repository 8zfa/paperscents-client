#pragma once
#include "../../modulebase.h"

class AntiVoidModule : public ModuleBase
{
public:
    AntiVoidModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

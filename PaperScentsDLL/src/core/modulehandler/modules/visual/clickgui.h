#pragma once
#include "../../modulebase.h"

class ClickGUIModule : public ModuleBase
{
public:
    ClickGUIModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override {}
};

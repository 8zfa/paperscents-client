#pragma once
#include "../../modulebase.h"

class AutoGGModule : public ModuleBase
{
public:
    AutoGGModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    bool m_Triggered = false;
};

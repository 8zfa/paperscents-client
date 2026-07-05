#pragma once
#include "../../modulebase.h"

class PhaseModule : public ModuleBase
{
public:
    PhaseModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    bool m_NoClipRestore = false;
};

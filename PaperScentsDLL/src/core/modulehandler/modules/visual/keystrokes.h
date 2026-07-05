#pragma once
#include "../../modulebase.h"
#include <unordered_map>

class KeyStrokesModule : public ModuleBase
{
public:
    KeyStrokesModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnRender() override;

private:
    std::unordered_map<int, bool> m_PrevState;
};

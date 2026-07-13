#pragma once
#include "../../modulebase.h"
#include <vector>
#include <string>

class ChatFilterModule : public ModuleBase
{
public:
    ChatFilterModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

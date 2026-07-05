#pragma once
#include "../../modulebase.h"

class ChatFilterModule : public ModuleBase
{
public:
    ChatFilterModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};

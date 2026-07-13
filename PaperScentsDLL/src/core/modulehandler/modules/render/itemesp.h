#pragma once
#include "../../modulebase.h"
#include "../../../java/java.h"
#include <vector>
#include <string>
#include <imgui.h>

struct ItemESPData
{
    double x, y, z;
    std::string displayName;
};

class ItemESPModule : public ModuleBase
{
public:
    ItemESPModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
    void OnRender() override;

private:
    std::vector<ItemESPData> m_RenderData;
    int m_FrameCounter = 0;
    int m_UpdateInterval = 3;
};

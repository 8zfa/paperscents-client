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
    bool WorldToScreen(JNIEnv* env, double x, double y, double z, float& sx, float& sy);
    std::vector<ItemESPData> m_RenderData;
};

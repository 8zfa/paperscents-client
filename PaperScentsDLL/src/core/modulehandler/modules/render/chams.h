#pragma once
#include "../../modulebase.h"
#include "../../../java/java.h"
#include <vector>
#include <string>
#include <imgui.h>

struct ChamsData
{
    double x, y, z;
    float height;
    float width;
    std::string name;
};

class ChamsModule : public ModuleBase
{
public:
    ChamsModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
    void OnRender() override;

private:
    bool WorldToScreen(JNIEnv* env, double x, double y, double z, float& sx, float& sy);
    std::vector<ChamsData> m_RenderData;
    int m_FrameCounter = 0;
};

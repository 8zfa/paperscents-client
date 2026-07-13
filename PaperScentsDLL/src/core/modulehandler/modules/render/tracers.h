#pragma once
#include "../../modulebase.h"
#include "../../../java/java.h"
#include <vector>
#include <string>
#include <imgui.h>

struct TracerData
{
    double x, y, z;
    bool isPlayer;
};

class TracersModule : public ModuleBase
{
public:
    TracersModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
    void OnRender() override;

private:
    std::vector<TracerData> m_RenderData;
    int m_FrameCounter = 0;
    int m_UpdateInterval = 3;
};

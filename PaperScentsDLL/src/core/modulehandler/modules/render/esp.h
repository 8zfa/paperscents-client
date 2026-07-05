#pragma once
#include "../../modulebase.h"
#include <vector>

struct ESPData
{
    float boxLeft, boxTop, boxRight, boxBottom;
    std::string name;
    float dist;
    float health;
    float maxHealth;
    float opacityFade;
    bool isFriend;
};

class ESPModule : public ModuleBase
{
public:
    ESPModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
    void OnRender() override;

private:
    std::vector<ESPData> m_RenderData;
    bool WorldToScreen(double x, double y, double z, float& sx, float& sy);
};

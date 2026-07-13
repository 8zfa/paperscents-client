#pragma once
#include "../../modulebase.h"
#include <unordered_map>
#include <string>
#include <vector>

class ArrayListModule : public ModuleBase
{
public:
    ArrayListModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnRender() override;

    struct AnimEntry {
        float Anim = 0.0f;
        float Width = 0.0f;
        float TargetWidth = 0.0f;
    };

private:
    std::unordered_map<std::string, AnimEntry> m_Animations;
    std::vector<ModuleBase*> GetSortedModules();
    ImColor GetRainbow(float speed, float offset);
};

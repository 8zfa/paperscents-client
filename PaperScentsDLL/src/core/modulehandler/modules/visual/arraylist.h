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

    void SetModulePos(float x, float y) { m_PosX = x; m_PosY = y; }
    float GetPosX() const { return m_PosX; }
    float GetPosY() const { return m_PosY; }

    struct AnimEntry {
        float Anim = 0.0f;
        float Width = 0.0f;
    };

private:
    float m_PosX = 4.0f, m_PosY = 4.0f;
    std::unordered_map<std::string, AnimEntry> m_Animations;
    std::vector<ModuleBase*> GetSortedModules();
    ImColor GetRainbow(float speed, float offset);
};

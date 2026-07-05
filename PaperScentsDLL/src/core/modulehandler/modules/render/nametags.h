#pragma once
#include "../../modulebase.h"
#include <string>
#include <vector>

struct NameTagData
{
    float x, y;
    std::string text;
    float opacity;
};

class NametagsModule : public ModuleBase
{
public:
    NametagsModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
    void OnRender() override;

private:
    std::vector<NameTagData> m_Tags;
    int m_FrameCounter = 0;
    static std::string StripColor(const std::string& input);
};

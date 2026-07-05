#include "watermark.h"
#include "../../../utilities/logger.h"
#include <imgui.h>

WatermarkModule::WatermarkModule()
    : ModuleBase("Watermark", "Show client watermark", Category::Render)
{
    AddSetting<BooleanSetting>("Rainbow", false);
    AddSetting<ColorSetting>("Color", ImColor(0.42f, 0.39f, 1.0f, 1.0f));
}

void WatermarkModule::OnEnable() {}
void WatermarkModule::OnDisable() {}

void WatermarkModule::OnRender()
{
    if (!IsEnabled()) return;
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    bool rainbow = ((BooleanSetting*)FindSetting("Rainbow"))->GetValue();
    ImColor col = ((ColorSetting*)FindSetting("Color"))->GetValue();

    std::string text = "PaperScents v1.0";
    ImVec2 size = ImGui::CalcTextSize(text.c_str());

    float hue = rainbow ? fmodf((float)ImGui::GetTime() * 0.5f, 1.0f) : 0.0f;
    ImColor useCol = rainbow ? ImColor::HSV(hue, 0.8f, 1.0f) : col;

    draw->AddText(ImVec2(3.0f, 3.0f), IM_COL32(0, 0, 0, 180), text.c_str());
    draw->AddText(ImVec2(2.0f, 2.0f), useCol, text.c_str());
}

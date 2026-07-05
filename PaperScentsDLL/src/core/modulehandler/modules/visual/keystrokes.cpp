#include "keystrokes.h"
#include "../../../utilities/logger.h"
#include <imgui.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

struct KeyInfo
{
    const char* Label;
    int VKey;
    float X, Y;
};

KeyStrokesModule::KeyStrokesModule()
    : ModuleBase("KeyStrokes", "Display key presses on screen", Category::Render)
{
    AddSetting<NumberSetting>("X", 4.0f, 0.0f, 2000.0f, 1.0f);
    AddSetting<NumberSetting>("Y", 100.0f, 0.0f, 2000.0f, 1.0f);
    AddSetting<NumberSetting>("Size", 30.0f, 20.0f, 60.0f, 2.0f, "Key button size");
    AddSetting<NumberSetting>("Rounding", 4.0f, 0.0f, 12.0f, 1.0f);
    AddSetting<ColorSetting>("BgColor", ImColor(0.0f, 0.0f, 0.0f, 0.5f));
    AddSetting<ColorSetting>("PressColor", ImColor(0.42f, 0.39f, 1.0f, 0.6f));
    AddSetting<BooleanSetting>("ShowMouse", true, "Show LMB/RMB buttons");
}

void KeyStrokesModule::OnEnable() { Logger::Log("KeyStrokes enabled"); }
void KeyStrokesModule::OnDisable() { Logger::Log("KeyStrokes disabled"); }

void KeyStrokesModule::OnRender()
{
    if (!IsEnabled()) return;

    float x = ((NumberSetting*)FindSetting("X"))->GetValue();
    float y = ((NumberSetting*)FindSetting("Y"))->GetValue();
    float size = ((NumberSetting*)FindSetting("Size"))->GetValue();
    float rounding = ((NumberSetting*)FindSetting("Rounding"))->GetValue();
    ImColor bgColor = ((ColorSetting*)FindSetting("BgColor"))->GetValue();
    ImColor pressColor = ((ColorSetting*)FindSetting("PressColor"))->GetValue();
    bool showMouse = ((BooleanSetting*)FindSetting("ShowMouse"))->GetValue();
    ImU32 bg = bgColor;
    ImU32 pressed = pressColor;

    float gap = 2.0f;
    float totalW = size * 3 + gap * 2;
    float totalH = size * 2 + gap;
    if (showMouse) totalH += size + gap;

    ImDrawList* draw = ImGui::GetForegroundDrawList();

    KeyInfo keys[] = {
        {"W", 'W', x + size + gap, y},
        {"A", 'A', x, y + size + gap},
        {"S", 'S', x + size + gap, y + size + gap},
        {"D", 'D', x + (size + gap) * 2, y + size + gap},
    };

    for (const auto& k : keys)
    {
        bool held = GetAsyncKeyState(k.VKey) & 0x8000;
        ImU32 col = held ? pressed : bg;
        ImVec2 a(k.X, k.Y);
        ImVec2 b(k.X + size, k.Y + size);
        draw->AddRectFilled(a, b, col, rounding);
        ImVec2 textSize = ImGui::CalcTextSize(k.Label);
        ImVec2 textPos(a.x + (size - textSize.x) * 0.5f, a.y + (size - textSize.y) * 0.5f);
        draw->AddText(textPos, IM_COL32(255, 255, 255, 255), k.Label);
    }

    if (showMouse)
    {
        float mouseY = y + size * 2 + gap * 2;
        float mouseW = (size * 3 + gap * 2) * 0.5f - gap * 0.5f;

        bool lmb = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
        bool rmb = GetAsyncKeyState(VK_RBUTTON) & 0x8000;

        ImVec2 lmbMin(x, mouseY);
        ImVec2 lmbMax(x + mouseW, mouseY + size * 0.6f);
        draw->AddRectFilled(lmbMin, lmbMax, lmb ? pressed : bg, rounding);
        const char* lmbText = "LMB";
        ImVec2 lmbTS = ImGui::CalcTextSize(lmbText);
        draw->AddText(ImVec2(lmbMin.x + (mouseW - lmbTS.x) * 0.5f, lmbMin.y + (size * 0.6f - lmbTS.y) * 0.5f),
                      IM_COL32(255, 255, 255, 255), lmbText);

        ImVec2 rmbMin(x + mouseW + gap, mouseY);
        ImVec2 rmbMax(x + mouseW * 2 + gap, mouseY + size * 0.6f);
        draw->AddRectFilled(rmbMin, rmbMax, rmb ? pressed : bg, rounding);
        const char* rmbText = "RMB";
        ImVec2 rmbTS = ImGui::CalcTextSize(rmbText);
        draw->AddText(ImVec2(rmbMin.x + (mouseW - rmbTS.x) * 0.5f, rmbMin.y + (size * 0.6f - rmbTS.y) * 0.5f),
                      IM_COL32(255, 255, 255, 255), rmbText);
    }
}

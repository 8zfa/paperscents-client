#define IMGUI_DEFINE_MATH_OPERATORS
#include "gui.hpp"
#include <imgui_internal.h>

bool NeverloseGUI::Tab(const char* label, bool selected)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiID id = window->GetID(label);
    ImVec2 pos = window->DC.CursorPos;
    ImDrawList* draw = window->DrawList;
    float width = ImGui::GetWindowWidth();

    ImRect bb(pos, pos + ImVec2(width, 30.0f));
    ImGui::ItemAdd(bb, id);
    ImGui::ItemSize(bb, ImGui::GetStyle().FramePadding.y);

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    static std::unordered_map<ImGuiID, float> animValues;
    float& val = animValues[id];
    val = ImLerp(val, selected ? 1.0f : 0.0f, 0.05f);

    draw->AddRectFilled(bb.Min, bb.Max, g_Neverlose.frameActive.ToImColor(0.5f * val), 5);

    ImVec2 labelSize = ImGui::CalcTextSize(label);
    draw->AddText(bb.Min + ImVec2(35.0f, bb.GetCenter().y - bb.Min.y - labelSize.y * 0.5f),
        ImGui::GetColorU32(ImGuiCol_Text), label);

    return pressed;
}

void NeverloseGUI::GroupBox(const char* name, ImVec2 size)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 pos = window->DC.CursorPos;

    ImGui::BeginChild((std::string(name) + ".main").c_str(), size, false,
        ImGuiWindowFlags_NoScrollbar);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(pos + ImVec2(0.0f, 20.0f), pos + size, g_Neverlose.groupBoxBg.ToImColor(), 6);
    draw->AddRect(pos + ImVec2(0.0f, 20.0f), pos + size, g_Neverlose.border.ToImColor(), 6);

    draw->AddText(pos + ImVec2(12.0f, 0.0f), ImGui::GetColorU32(ImGuiCol_Text, 0.5f), name);

    ImGui::SetCursorPos(ImVec2(12.0f, 21.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 10.0f));
    ImGui::BeginChild(name, ImVec2(size.x - 24.0f, size.y - 21.0f), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysUseWindowPadding);

    ImGui::BeginGroup();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 10.0f));
}

void NeverloseGUI::EndGroupBox()
{
    ImGui::PopStyleVar(2);
    ImGui::EndGroup();
    ImGui::EndChild();
    ImGui::EndChild();
}

void NeverloseGUI::GroupTitle(const char* name)
{
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
    ImGui::Text("%s", name);
    ImGui::PopStyleColor();
}

void NeverloseGUI::ApplyStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                 = g_Neverlose.text.ToVec4();
    colors[ImGuiCol_TextDisabled]         = g_Neverlose.textDisabled.ToVec4();
    colors[ImGuiCol_WindowBg]             = ImVec4(0.10f, 0.10f, 0.18f, 1.0f);
    colors[ImGuiCol_ChildBg]              = ImVec4(0.02f, 0.04f, 0.06f, 1.0f);
    colors[ImGuiCol_PopupBg]              = ImVec4(0.04f, 0.07f, 0.14f, 0.94f);
    colors[ImGuiCol_Border]               = g_Neverlose.border.ToVec4();
    colors[ImGuiCol_FrameBg]              = g_Neverlose.button.ToVec4();
    colors[ImGuiCol_FrameBgHovered]       = g_Neverlose.buttonHovered.ToVec4();
    colors[ImGuiCol_FrameBgActive]        = g_Neverlose.buttonActive.ToVec4();
    colors[ImGuiCol_TitleBg]              = g_Neverlose.frameActive.ToVec4();
    colors[ImGuiCol_TitleBgActive]        = g_Neverlose.frameActive.ToVec4();
    colors[ImGuiCol_CheckMark]            = g_Neverlose.accent.ToVec4();
    colors[ImGuiCol_SliderGrab]           = g_Neverlose.accent.ToVec4();
    colors[ImGuiCol_SliderGrabActive]     = g_Neverlose.accent.ToVec4(0.8f);
    colors[ImGuiCol_Button]               = g_Neverlose.button.ToVec4();
    colors[ImGuiCol_ButtonHovered]        = g_Neverlose.buttonHovered.ToVec4();
    colors[ImGuiCol_ButtonActive]         = g_Neverlose.buttonActive.ToVec4();
    colors[ImGuiCol_Header]               = g_Neverlose.frameActive.ToVec4(0.8f);
    colors[ImGuiCol_HeaderHovered]        = g_Neverlose.frameActive.ToVec4();
    colors[ImGuiCol_HeaderActive]         = g_Neverlose.accent.ToVec4(0.6f);
    colors[ImGuiCol_Separator]            = g_Neverlose.border.ToVec4();
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.02f, 0.02f, 0.03f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab]        = g_Neverlose.frameInactive.ToVec4();
    colors[ImGuiCol_ScrollbarGrabHovered] = g_Neverlose.frameActive.ToVec4();
    colors[ImGuiCol_ScrollbarGrabActive]  = g_Neverlose.accent.ToVec4(0.6f);

    style.FrameRounding    = 4.0f;
    style.WindowRounding   = 6.0f;
    style.GrabRounding     = 4.0f;
    style.ScrollbarSize    = 8.0f;
    style.ItemSpacing      = ImVec2(8.0f, 4.0f);
    style.WindowPadding    = ImVec2(0.0f, 0.0f);
    style.FramePadding     = ImVec2(6.0f, 4.0f);
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize  = 0.0f;
    style.PopupBorderSize  = 0.0f;
}

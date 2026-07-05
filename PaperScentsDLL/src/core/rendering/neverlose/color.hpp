#pragma once
#include <imgui.h>

struct Color
{
    float r, g, b, a;

    ImColor ToImColor(float alpha = 1.0f) const
    {
        return ImColor(r, g, b, a * alpha);
    }

    ImVec4 ToVec4(float alpha = 1.0f) const
    {
        return ImVec4(r, g, b, a * alpha);
    }

    ImU32 ToU32(float alpha = 1.0f) const
    {
        return ImGui::GetColorU32(ImVec4(r, g, b, a * alpha));
    }
};

struct NeverloseColors
{
    Color accent        = { 0.30f, 0.49f, 1.00f, 1.00f };
    Color accentDim     = { 0.20f, 0.35f, 0.80f, 1.00f };
    Color accentGlow    = { 0.30f, 0.49f, 1.00f, 0.20f };
    Color text          = { 1.00f, 1.00f, 1.00f, 1.00f };
    Color textDim       = { 0.75f, 0.76f, 0.80f, 1.00f };
    Color textDisabled  = { 0.51f, 0.52f, 0.56f, 1.00f };
    Color border        = { 1.00f, 1.00f, 1.00f, 0.06f };
    Color borderLight   = { 1.00f, 1.00f, 1.00f, 0.10f };
    Color frameInactive = { 0.02f, 0.04f, 0.07f, 1.00f };
    Color frameActive   = { 0.04f, 0.07f, 0.14f, 1.00f };
    Color button        = { 0.04f, 0.04f, 0.06f, 1.00f };
    Color buttonHovered = { 0.05f, 0.05f, 0.08f, 1.00f };
    Color buttonActive  = { 0.07f, 0.07f, 0.10f, 1.00f };
    Color groupBoxBg    = { 0.02f, 0.04f, 0.06f, 1.00f };
    Color moduleBg      = { 0.06f, 0.06f, 0.10f, 1.00f };
    Color moduleBgHover = { 0.08f, 0.08f, 0.14f, 1.00f };
    Color moduleBgOn    = { 0.10f, 0.12f, 0.22f, 1.00f };
    Color positive      = { 0.20f, 0.80f, 0.40f, 1.00f };
    Color warning       = { 0.95f, 0.70f, 0.10f, 1.00f };
    Color danger        = { 0.90f, 0.25f, 0.20f, 1.00f };
    Color headerBg      = { 0.01f, 0.02f, 0.04f, 0.95f };
};

inline NeverloseColors g_Neverlose;

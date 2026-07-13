#define IMGUI_DEFINE_MATH_OPERATORS
#include "arraylist.h"
#include "../../modulehandler.h"
#include "../../../rendering/paperScentsLogo.h"
#include <algorithm>
#include <cmath>

#include <imgui.h>
#include <imgui_internal.h>
#include <Windows.h>
#include <gl/GL.h>

ArrayListModule::ArrayListModule()
    : ModuleBase("ArrayList", "Show enabled modules on screen", Category::Render)
{
    AddSetting<NumberSetting>("X", 4.0f, 0.0f, 2000.0f, 1.0f, "X position");
    AddSetting<NumberSetting>("Y", 4.0f, 0.0f, 2000.0f, 1.0f, "Y position");
    AddSetting<NumberSetting>("Spacing", 2.0f, 0.0f, 20.0f, 1.0f, "Spacing between entries");
    AddSetting<EnumSetting>("Alignment", 1, std::vector<std::string>{"Left", "Right"}, "Text alignment");
    AddSetting<EnumSetting>("Sort", 0, std::vector<std::string>{"Name Length", "Alphabetical", "By Category"}, "Sort mode");
    AddSetting<BooleanSetting>("Reverse", false, "Reverse sort order");
    AddSetting<ColorSetting>("TextColor", ImColor(1.0f, 1.0f, 1.0f, 1.0f), "Module name color");
    AddSetting<ColorSetting>("AccentColor", ImColor(0.35f, 0.67f, 0.98f, 1.0f), "Accent color");
    AddSetting<BooleanSetting>("Rainbow", false, "Rainbow text animation");
    AddSetting<NumberSetting>("RainbowSpeed", 1.0f, 0.1f, 5.0f, 0.1f, "Rainbow speed");
    AddSetting<NumberSetting>("RainbowSaturation", 0.8f, 0.1f, 1.0f, 0.05f, "Rainbow saturation");
    AddSetting<BooleanSetting>("Background", true, "Show entry background");
    AddSetting<ColorSetting>("BgColor", ImColor(0.0f, 0.0f, 0.0f, 0.5f), "Background color");
    AddSetting<NumberSetting>("BgOpacity", 0.45f, 0.0f, 1.0f, 0.05f, "Background opacity");
    AddSetting<NumberSetting>("Rounding", 3.0f, 0.0f, 12.0f, 1.0f, "Corner rounding");
    AddSetting<EnumSetting>("Animation", 1, std::vector<std::string>{"None", "Slide", "Fade"}, "Entry animation");
    AddSetting<NumberSetting>("AnimSpeed", 0.18f, 0.05f, 1.0f, 0.05f, "Animation speed");
    AddSetting<BooleanSetting>("ShowKeybind", false, "Show keybind");
    AddSetting<BooleanSetting>("ShowBar", true, "Show accent bar");
    AddSetting<NumberSetting>("BarWidth", 2.0f, 1.0f, 6.0f, 0.5f, "Accent bar width");
    AddSetting<BooleanSetting>("Shadow", true, "Text shadow");
    AddSetting<BooleanSetting>("Uppercase", false, "Uppercase names");
    AddSetting<BooleanSetting>("HideEmpty", true, "Hide when no modules enabled");
}

static GLuint g_LogoTexture = 0;

static void LoadLogoTexture()
{
    if (g_LogoTexture) return;
    glGenTextures(1, &g_LogoTexture);
    glBindTexture(GL_TEXTURE_2D, g_LogoTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_LogoWidth, g_LogoHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, g_LogoData);
}

void ArrayListModule::OnEnable() {}
void ArrayListModule::OnDisable() {}

ImColor ArrayListModule::GetRainbow(float speed, float offset)
{
    float hue = (float)(ImGui::GetTime() * speed * 0.5f + offset);
    hue -= floorf(hue);
    return ImColor::HSV(hue, ((NumberSetting*)FindSetting("RainbowSaturation"))->GetValue(), 1.0f);
}

std::vector<ModuleBase*> ArrayListModule::GetSortedModules()
{
    auto& all = ModuleHandler::GetInstance().GetModules();
    std::vector<ModuleBase*> list;
    for (auto* m : all)
    {
        if (m->IsEnabled() && m->IsVisible())
            list.push_back(m);
    }

    int sortMode = ((EnumSetting*)FindSetting("Sort"))->GetValue();
    bool reverse = ((BooleanSetting*)FindSetting("Reverse"))->GetValue();

    switch (sortMode)
    {
    case 0: // Name Length (most letters first)
        std::sort(list.begin(), list.end(), [](ModuleBase* a, ModuleBase* b) {
            size_t la = a->GetName().size();
            size_t lb = b->GetName().size();
            if (la != lb) return la > lb;
            return a->GetName() < b->GetName();
        });
        break;
    case 1: // Alphabetical
        std::sort(list.begin(), list.end(), [](ModuleBase* a, ModuleBase* b) {
            return a->GetName() < b->GetName();
        });
        break;
    case 2: // By Category
        std::sort(list.begin(), list.end(), [](ModuleBase* a, ModuleBase* b) {
            if (a->GetCategory() != b->GetCategory())
                return (int)a->GetCategory() < (int)b->GetCategory();
            return a->GetName() < b->GetName();
        });
        break;
    }

    if (reverse)
        std::reverse(list.begin(), list.end());

    return list;
}

void ArrayListModule::OnRender()
{
    if (!IsEnabled()) return;
    auto modules = GetSortedModules();

    bool hideEmpty = ((BooleanSetting*)FindSetting("HideEmpty")) ? ((BooleanSetting*)FindSetting("HideEmpty"))->GetValue() : true;

    float x = ((NumberSetting*)FindSetting("X"))->GetValue();
    float y = ((NumberSetting*)FindSetting("Y"))->GetValue();
    float spacing = ((NumberSetting*)FindSetting("Spacing"))->GetValue();
    int align = ((EnumSetting*)FindSetting("Alignment"))->GetValue();
    float rounding = ((NumberSetting*)FindSetting("Rounding"))->GetValue();
    float animSpeed = ((NumberSetting*)FindSetting("AnimSpeed"))->GetValue();
    int animType = ((EnumSetting*)FindSetting("Animation"))->GetValue();
    bool showKeybind = ((BooleanSetting*)FindSetting("ShowKeybind"))->GetValue();
    bool showBar = ((BooleanSetting*)FindSetting("ShowBar"))->GetValue();
    float barWidth = ((NumberSetting*)FindSetting("BarWidth"))->GetValue();
    ImColor textColor = ((ColorSetting*)FindSetting("TextColor"))->GetValue();
    ImColor accentColor = ((ColorSetting*)FindSetting("AccentColor"))->GetValue();
    bool rainbow = ((BooleanSetting*)FindSetting("Rainbow"))->GetValue();
    float rainbowSpeed = ((NumberSetting*)FindSetting("RainbowSpeed"))->GetValue();
    bool showBg = ((BooleanSetting*)FindSetting("Background"))->GetValue();
    ImColor bgColor = ((ColorSetting*)FindSetting("BgColor"))->GetValue();
    float bgOpacity = ((NumberSetting*)FindSetting("BgOpacity"))->GetValue();
    bool shadow = ((BooleanSetting*)FindSetting("Shadow"))->GetValue();
    bool uppercase = ((BooleanSetting*)FindSetting("Uppercase"))->GetValue();

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImVec2 display = ImGui::GetIO().DisplaySize;
    float dt = ImGui::GetIO().DeltaTime;

    // PaperScents logo header
    LoadLogoTexture();
    float logoSize = ImGui::GetFont()->FontSize + 4.0f;
    float logoX = x;
    float logoY = y;
    float textH = ImGui::GetFont()->FontSize;

    ImColor hdrCol = rainbow ? GetRainbow(rainbowSpeed, 0.0f) : accentColor;
    if (showBg)
    {
        float hdrW = logoSize + ImGui::CalcTextSize("PaperScents").x + 14.0f;
        if (align == 1) logoX = display.x - hdrW - x;
        ImVec2 hdrBgMin(logoX, logoY);
        ImVec2 hdrBgMax(logoX + hdrW, logoY + logoSize + 4.0f);
        ImColor hdrBg = bgColor;
        hdrBg.Value.w = bgOpacity;
        draw->AddRectFilled(hdrBgMin, hdrBgMax, hdrBg, rounding);
        if (showBar)
            draw->AddRectFilled(hdrBgMin, ImVec2(hdrBgMin.x + barWidth, hdrBgMax.y), hdrCol, rounding, ImDrawFlags_RoundCornersLeft);
    }

    float textOff = logoX + 6.0f + (showBar ? barWidth : 0.0f);
    draw->AddImage((ImTextureID)(uint64_t)g_LogoTexture, ImVec2(textOff, logoY + 2.0f), ImVec2(textOff + logoSize, logoY + 2.0f + logoSize));
    ImVec2 textPos(textOff + logoSize + 4.0f, logoY + 4.0f);
    if (shadow) draw->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 120), "PaperScents");
    draw->AddText(textPos, hdrCol, "PaperScents");

    float curY = logoY + logoSize + 6.0f + spacing;

    for (size_t i = 0; i < modules.size(); i++)
    {
        auto* mod = modules[i];
        auto& anim = m_Animations[mod->GetName()];

        std::string label = mod->GetName();
        if (uppercase)
            std::transform(label.begin(), label.end(), label.begin(), ::toupper);
        if (showKeybind && mod->GetKeybind() > 0)
            label += " [" + KeybindSetting::KeyToString(mod->GetKeybind()) + "]";

        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        float entryW = textSize.x + 12.0f;
        float entryH = textH + 6.0f;

        // Animation
        anim.TargetWidth = entryW;
        float speed = animSpeed > 0.0f ? (dt / animSpeed) : 10.0f;
        anim.Width = ImLerp(anim.Width, anim.TargetWidth, ImMin(speed * 4.0f, 1.0f));
        if (fabsf(anim.Width - anim.TargetWidth) < 0.5f) anim.Width = anim.TargetWidth;

        float targetAlpha = 1.0f;
        anim.Anim = ImLerp(anim.Anim, targetAlpha, ImMin(speed * 5.0f, 1.0f));
        if (anim.Anim > 0.99f) anim.Anim = 1.0f;
        float alpha = anim.Anim;

        float slideOffset = (animType == 1) ? (1.0f - anim.Anim) * 30.0f : 0.0f;
        float fadeMult = (animType == 2) ? anim.Anim : 1.0f;

        float drawX = x - slideOffset;
        float drawY = curY;

        // Background
        if (showBg)
        {
            ImColor bg = bgColor;
            bg.Value.w = bgOpacity * alpha * fadeMult;

            float bgX;
            if (align == 1)
                bgX = display.x - anim.Width - x;
            else
                bgX = drawX;

            ImVec2 bgMin(bgX, drawY);
            ImVec2 bgMax(bgX + anim.Width, drawY + entryH);
            draw->AddRectFilled(bgMin, bgMax, bg, rounding);

            // Accent bar
            if (showBar)
            {
                ImColor bar = rainbow ? GetRainbow(rainbowSpeed, (float)i * 0.3f) : accentColor;
                bar.Value.w = alpha * fadeMult;
                if (align == 1)
                    draw->AddRectFilled(ImVec2(bgMax.x - barWidth, bgMin.y), bgMax, bar, rounding, ImDrawFlags_RoundCornersRight);
                else
                    draw->AddRectFilled(bgMin, ImVec2(bgMin.x + barWidth, bgMax.y), bar, rounding, ImDrawFlags_RoundCornersLeft);
            }
        }

        // Text
        ImColor col = rainbow ? GetRainbow(rainbowSpeed, (float)i * 0.3f) : textColor;
        col.Value.w *= alpha * fadeMult;

        ImVec2 textPos;
        float textX;
        if (align == 1)
            textX = display.x - anim.Width - x + 6.0f;
        else
            textX = drawX + 6.0f + (showBar ? barWidth : 0.0f);

        textPos = ImVec2(textX, drawY + 3.0f);

        // Shadow
        if (shadow)
        {
            ImU32 shadowCol = IM_COL32(0, 0, 0, (int)(120 * alpha * fadeMult));
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(textPos.x + 1, textPos.y + 1), shadowCol, label.c_str());
        }

        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), textPos, col, label.c_str());

        curY += entryH + spacing;
    }
}

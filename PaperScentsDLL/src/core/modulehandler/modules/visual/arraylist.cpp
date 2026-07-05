#define IMGUI_DEFINE_MATH_OPERATORS
#include "arraylist.h"
#include "../../modulehandler.h"
#include <algorithm>
#include <cmath>

#include <imgui.h>
#include <imgui_internal.h>

ArrayListModule::ArrayListModule()
    : ModuleBase("ArrayList", "Show enabled modules on screen", Category::Render)
{
    AddSetting<NumberSetting>("X", 4.0f, 0.0f, 2000.0f, 1.0f, "X position");
    AddSetting<NumberSetting>("Y", 4.0f, 0.0f, 2000.0f, 1.0f, "Y position");
    AddSetting<NumberSetting>("Spacing", 2.0f, 0.0f, 20.0f, 1.0f, "Spacing between modules");
    AddSetting<EnumSetting>("Alignment", 1, std::vector<std::string>{"Left", "Right", "Center"}, "Text alignment");
    AddSetting<EnumSetting>("Sort", 0, std::vector<std::string>{"Alphabetical", "By Category", "Enabled First"}, "Sort mode");
    AddSetting<BooleanSetting>("Reverse", false, "Reverse sort order");
    AddSetting<ColorSetting>("TextColor", ImColor(1.0f, 1.0f, 1.0f, 1.0f), "Default text color");
    AddSetting<ColorSetting>("AccentColor", ImColor(0.42f, 0.39f, 1.0f, 1.0f), "Accent color");
    AddSetting<BooleanSetting>("Rainbow", false, "Rainbow text animation");
    AddSetting<NumberSetting>("RainbowSpeed", 1.0f, 0.1f, 5.0f, 0.1f, "Rainbow animation speed");
    AddSetting<BooleanSetting>("Background", true, "Show module background");
    AddSetting<ColorSetting>("BgColor", ImColor(0.0f, 0.0f, 0.0f, 0.5f), "Background color");
    AddSetting<NumberSetting>("BgOpacity", 0.5f, 0.0f, 1.0f, 0.05f, "Background opacity");
    AddSetting<NumberSetting>("Rounding", 4.0f, 0.0f, 12.0f, 1.0f, "Corner radius");
    AddSetting<EnumSetting>("Animation", 0, std::vector<std::string>{"Fade", "Slide", "Scale"}, "Entry animation");
    AddSetting<NumberSetting>("AnimSpeed", 0.25f, 0.05f, 1.0f, 0.05f, "Animation speed in seconds");
    AddSetting<BooleanSetting>("ShowKeybind", false, "Show keybind next to name");
    AddSetting<BooleanSetting>("ShowIcon", true, "Show state icon");
    AddSetting<BooleanSetting>("HideEmpty", true, "Hide when no modules enabled");
}

void ArrayListModule::OnEnable() {}
void ArrayListModule::OnDisable() {}

ImColor ArrayListModule::GetRainbow(float speed, float offset)
{
    float hue = (float)(ImGui::GetTime() * speed * 0.5f + offset);
    hue -= floorf(hue);
    return ImColor::HSV(hue, 0.8f, 1.0f);
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

    auto sortMode = (EnumSetting*)FindSetting("Sort");
    bool reverse = ((BooleanSetting*)FindSetting("Reverse"))->GetValue();

    if (sortMode)
    {
        int mode = sortMode->GetValue();
        if (mode == 0)
            std::sort(list.begin(), list.end(), [](ModuleBase* a, ModuleBase* b) { return a->GetName() < b->GetName(); });
        else if (mode == 1)
            std::sort(list.begin(), list.end(), [](ModuleBase* a, ModuleBase* b) {
                if (a->GetCategoryName() == b->GetCategoryName())
                    return a->GetName() < b->GetName();
                return std::string(a->GetCategoryName()) < std::string(b->GetCategoryName());
            });
    }

    if (reverse)
        std::reverse(list.begin(), list.end());

    return list;
}

void ArrayListModule::OnRender()
{
    if (!IsEnabled()) return;
    auto modules = GetSortedModules();

    bool hideEmpty = ((BooleanSetting*)FindSetting("HideEmpty"))->GetValue();
    if (hideEmpty && modules.empty()) return;

    float x = ((NumberSetting*)FindSetting("X"))->GetValue();
    float y = ((NumberSetting*)FindSetting("Y"))->GetValue();
    float spacing = ((NumberSetting*)FindSetting("Spacing"))->GetValue();
    int align = ((EnumSetting*)FindSetting("Alignment"))->GetValue();
    float rounding = ((NumberSetting*)FindSetting("Rounding"))->GetValue();
    float animSpeed = ((NumberSetting*)FindSetting("AnimSpeed"))->GetValue();
    int animType = ((EnumSetting*)FindSetting("Animation"))->GetValue();
    bool showKeybind = ((BooleanSetting*)FindSetting("ShowKeybind"))->GetValue();
    bool showIcon = ((BooleanSetting*)FindSetting("ShowIcon"))->GetValue();
    ImColor textColor = ((ColorSetting*)FindSetting("TextColor"))->GetValue();
    ImColor accentColor = ((ColorSetting*)FindSetting("AccentColor"))->GetValue();
    bool rainbow = ((BooleanSetting*)FindSetting("Rainbow"))->GetValue();
    float rainbowSpeed = ((NumberSetting*)FindSetting("RainbowSpeed"))->GetValue();
    bool showBg = ((BooleanSetting*)FindSetting("Background"))->GetValue();
    ImColor bgColor = ((ColorSetting*)FindSetting("BgColor"))->GetValue();
    float bgOpacity = ((NumberSetting*)FindSetting("BgOpacity"))->GetValue();

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    float curY = y;

    for (size_t i = 0; i < modules.size(); i++)
    {
        auto* mod = modules[i];
        auto& anim = m_Animations[mod->GetName()];

        float target = 1.0f;
        float speed = animSpeed > 0.0f ? (ImGui::GetIO().DeltaTime / animSpeed) : 1.0f;
        anim.Anim = ImLerp(anim.Anim, target, speed * 2.0f);
        if (anim.Anim > 0.99f) anim.Anim = 1.0f;

        std::string label = mod->GetName();
        if (showKeybind && mod->GetKeybind() > 0)
            label += " [" + KeybindSetting::KeyToString(mod->GetKeybind()) + "]";

        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());

        float iconW = showIcon ? 14.0f : 0.0f;
        float totalW = textSize.x + iconW + 8.0f;

        anim.Width = ImLerp(anim.Width, totalW, speed * 2.0f);
        if (anim.Width > totalW - 1.0f) anim.Width = totalW;

        float alpha = anim.Anim;
        float slideOffset = (animType == 1) ? (1.0f - anim.Anim) * 40.0f : 0.0f;
        float scale = (animType == 2) ? anim.Anim : 1.0f;

        float drawX = x - slideOffset;
        float drawY = curY;

        if (showBg)
        {
            ImColor bg = bgColor;
            bg.Value.w = bgOpacity * alpha;
            float bgX = (align == 1) ? drawX + textSize.x + iconW + 8.0f - anim.Width : drawX;
            ImVec2 bgMin(bgX, drawY);
            ImVec2 bgMax(bgX + anim.Width * scale, drawY + textSize.y + 4.0f);
            draw->AddRectFilled(bgMin, bgMax, bg, rounding);

            ImColor accent = rainbow ? GetRainbow(rainbowSpeed, (float)i * 0.3f) : accentColor;
            accent.Value.w = alpha;
            float barX = (align == 1) ? bgMax.x - 2.0f : bgMin.x;
            draw->AddRectFilled(ImVec2(barX, bgMin.y), ImVec2(barX + 2.0f, bgMax.y), accent, rounding, 0);
        }

        ImColor col = rainbow ? GetRainbow(rainbowSpeed, (float)i * 0.3f) : textColor;
        col.Value.w *= alpha;
        ImVec2 textPos;
        if (align == 1)
            textPos = ImVec2(drawX + anim.Width * scale - textSize.x - iconW - 4.0f, drawY + 2.0f);
        else
            textPos = ImVec2(drawX + 4.0f + iconW, drawY + 2.0f);

        draw->AddText(textPos, col, label.c_str());

        if (showIcon)
        {
            ImColor iconCol = rainbow ? GetRainbow(rainbowSpeed, (float)i * 0.3f + 0.5f) : accentColor;
            iconCol.Value.w *= alpha;
            ImVec2 iconPos(textPos.x - iconW + 4.0f, textPos.y + textSize.y * 0.5f - 3.0f);
            draw->AddCircleFilled(iconPos + ImVec2(3, 3), 3, iconCol);
        }

        curY += textSize.y + 4.0f + spacing;
    }
}

#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.h"
#include "../../utilities/logger.h"
#include "../../modulehandler/modulehandler.h"
#include "../../modulehandler/settings/settings.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <Windows.h>

Menu& Menu::GetInstance()
{
    static Menu instance;
    return instance;
}

const char* Menu::s_CatIcons[5] = { "\xe2\x9a\x94", "\xe2\x9a\xbd", "\xe2\x9c\xa8", "\xe2\x9a\x91", "\xe2\x9a\x99" };
const char* Menu::s_CatNames[5] = { "Combat", "Movement", "Render", "Player", "Misc" };

#undef RGB
static inline ImU32 Col32(int r, int g, int b, int a = 255) { return IM_COL32(r, g, b, a); }

static float Lerp(float cur, float tgt, float speed)
{
    float d = tgt - cur;
    if (fabsf(d) < 0.001f) return tgt;
    float dt = ImMax(ImGui::GetIO().DeltaTime, 0.0f);
    return cur + d * ImMin(speed * dt, 1.0f);
}

static void DrawRndRect(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col, float r)
{
    dl->AddRectFilled(a, b, col, r);
}

// Interpolate between two colors
static ImU32 InterpCol(ImU32 ca, ImU32 cb, float t)
{
    if (t <= 0) return ca; if (t >= 1) return cb;
    int a1 = (ca >> 24) & 0xFF, r1 = (ca >> 16) & 0xFF, g1 = (ca >> 8) & 0xFF, b1 = ca & 0xFF;
    int a2 = (cb >> 24) & 0xFF, r2 = (cb >> 16) & 0xFF, g2 = (cb >> 8) & 0xFF, b2 = cb & 0xFF;
    return IM_COL32(
        (int)(r1 + (r2 - r1) * t),
        (int)(g1 + (g2 - g1) * t),
        (int)(b1 + (b2 - b1) * t),
        (int)(a1 + (a2 - a1) * t)
    );
}

// Draw a gradient rect (left->right)
static void DrawGradH(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 cl, ImU32 cr)
{
    dl->AddRectFilledMultiColor(a, b, cl, cr, cr, cl);
}

// Draw a gradient rect (top->bottom)
static void DrawGradV(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 ct, ImU32 cb)
{
    dl->AddRectFilledMultiColor(a, b, ct, ct, cb, cb);
}

// Draw a circle filled
static void DrawCircle(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
{
    dl->AddCircleFilled(c, r, col, 24);
}

// Draw a good circle (like Tenacity's RenderUtil.drawGoodCircle)
static void DrawGoodCircle(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
{
    dl->AddCircleFilled(c, r, col, 32);
}

// Draw rounded rect with gradient corner-to-corner (for module icon)
static void DrawGradCornerLR(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, ImU32 c1, ImU32 c2)
{
    dl->AddRectFilledMultiColor(a, b, c1, c2, c2, c1);
}

// Draw a toggle pill (boolean setting)
static void DrawTogglePill(ImDrawList* dl, ImVec2 pos, float w, float h, bool on, float anim)
{
    ImU32 bg = on ? Col32(90, 170, 250, 255) : Col32(68, 71, 78, 255);
    DrawRndRect(dl, pos, ImVec2(pos.x + w, pos.y + h), bg, h * 0.5f);
    float cx = on ? (pos.x + w - h * 0.5f) : (pos.x + h * 0.5f);
    DrawCircle(dl, ImVec2(cx, pos.y + h * 0.5f), h * 0.5f - 2, Col32(255, 255, 255, 255));
}

// Draw a 3-dot button
static void DrawThreeDots(ImDrawList* dl, float x, float y, ImU32 col)
{
    DrawCircle(dl, ImVec2(x, y + 0), 1.5f, col);
    DrawCircle(dl, ImVec2(x, y + 8), 1.5f, col);
    DrawCircle(dl, ImVec2(x, y + 16), 1.5f, col);
}

// --- Main Render ---
void Menu::Initialize()
{
    m_Open = false;
    m_OpeningAnim = 0.0f;
    m_SidebarAnim = 0.0f;
    m_FirstOpen = true;
    m_CurrentCat = 0;
    m_SelMod.clear();
    m_BindingMod.clear();
    m_DragSetting.clear();
    for (int i = 0; i < 5; i++) m_CatScroll[i] = 0;
    LoadConfig();
    m_Initialized = true;
}

void Menu::Render()
{
    if (!m_Open && m_OpeningAnim < 0.01f) return;
    if (!m_Initialized) Initialize();

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 disp = io.DisplaySize;
    float dt = io.DeltaTime;
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // --- Constants ---
    const float BASE_W = 680.0f;
    const float WIN_H = 500.0f;
    const float SIDEBAR_EXP = 155.0f;
    const float SIDEBAR_COL = 65.0f;
    const float SETTINGS_W = 210.0f;
    const float MOD_H = 50.0f;
    const float CAT_BTN_H = 44.0f;
    const float RADIUS = 8.0f;

    // Colors
    const ImU32 COL_BG = Col32(30, 31, 35);
    const ImU32 COL_SIDEBAR = Col32(47, 49, 54);
    const ImU32 COL_MODULE = Col32(47, 49, 54);
    const ImU32 COL_ICON_BG = Col32(68, 71, 78);
    const ImU32 COL_LIGHTER = Col32(68, 71, 78);
    const ImU32 COL_TEXT = Col32(255, 255, 255);
    const ImU32 COL_TEXT_DIM = Col32(128, 134, 141);
    const ImU32 COL_ACCENT1 = Col32(90, 170, 250);
    const ImU32 COL_ACCENT2 = Col32(60, 120, 220);
    const ImU32 COL_SETTINGS_BG = Col32(47, 49, 54);
    const ImU32 COL_HOVER_BG = Col32(58, 60, 66);

    // --- Opening animation ---
    if (m_Open) m_OpeningAnim = Lerp(m_OpeningAnim, 1.0f, 5.0f);
    else m_OpeningAnim = Lerp(m_OpeningAnim, 0.0f, 6.0f);
    m_OpeningAnim = ImClamp(m_OpeningAnim, 0.0f, 1.0f);

    if (m_OpeningAnim < 0.01f && !m_Open) return;

    float openT = m_OpeningAnim;
    float easeOpen = 1.0f - (1.0f - openT) * (1.0f - openT);

    // Center window on first open
    if (m_FirstOpen && m_Open)
    {
        m_WinX = disp.x * 0.5f - BASE_W * 0.5f;
        m_WinY = disp.y * 0.5f - WIN_H * 0.5f;
        m_FirstOpen = false;
    }

    // --- Slide-in/out from left ---
    float winX = m_WinX;
    float winY = m_WinY;
    float offX = disp.x + BASE_W * 0.5f;
    winX -= offX * (1.0f - easeOpen);

    // --- Sidebar hover ---
    float sbTarget = m_SidebarAnim;
    if (m_Open)
    {
        float sbW = SIDEBAR_EXP - (SIDEBAR_EXP - SIDEBAR_COL) * m_SidebarAnim;
        bool hoverSb = io.MousePos.x >= winX && io.MousePos.x <= winX + sbW &&
                       io.MousePos.y >= winY && io.MousePos.y <= winY + WIN_H;
        sbTarget = hoverSb ? 0.0f : 1.0f;
    }
    m_SidebarAnim = ImClamp(Lerp(m_SidebarAnim, sbTarget, 8.0f), 0.0f, 1.0f);

    float sidebarW = SIDEBAR_EXP - (SIDEBAR_EXP - SIDEBAR_COL) * m_SidebarAnim;
    float sidebarFrac = (sidebarW - SIDEBAR_COL) / (SIDEBAR_EXP - SIDEBAR_COL); // 1=expanded, 0=collapsed

    // --- Settings expand ---
    bool hasSettings = !m_SelMod.empty();
    if (hasSettings)
        m_SettingsAnim = Lerp(m_SettingsAnim, 1.0f, 8.0f);
    else
    {
        m_SettingsAnim = Lerp(m_SettingsAnim, 0.0f, 6.0f);
        m_SettingsScroll = 0;
    }
    m_SettingsAnim = ImClamp(m_SettingsAnim, 0.0f, 1.0f);

    float settingsW = SETTINGS_W * m_SettingsAnim;
    float totalW = BASE_W + settingsW;
    float modAreaX = winX + sidebarW;
    float modAreaW = totalW - sidebarW - settingsW;

    // --- Draw main window background ---
    DrawRndRect(dl, ImVec2(winX, winY), ImVec2(winX + totalW, winY + WIN_H), COL_BG, RADIUS);

    // --- Draw sidebar ---
    DrawRndRect(dl, ImVec2(winX, winY), ImVec2(winX + sidebarW, winY + WIN_H), COL_SIDEBAR, RADIUS);

    // Divider line
    float divX = winX + sidebarW;
    dl->AddLine(ImVec2(divX, winY + 35), ImVec2(divX, winY + WIN_H - 8), COL_LIGHTER, 1.0f);

    // Logo area (simplified - just "PS" text)
    dl->AddText(ImGui::GetFont(), 14.0f,
        ImVec2(winX + 10 + (8 * sidebarFrac), winY + 10),
        COL_ACCENT1, "PS");

    // Version text
    float verX = winX + 10 + ImGui::CalcTextSize("PS").x + 12 + (8 * sidebarFrac);
    dl->AddText(ImGui::GetFont(), 10.0f,
        ImVec2(verX, winY + 13),
        COL_TEXT_DIM, "1.0");

    // Divider line below logo
    float divY = winY + 33;
    dl->AddLine(ImVec2(winX + 10, divY), ImVec2(winX + sidebarW - 10 * sidebarFrac, divY), COL_LIGHTER, 1.0f);

    // --- Category buttons ---
    float catY = divY + 12;
    auto& modules = ModuleHandler::GetInstance().GetModules();

    for (int ci = 0; ci < 5; ci++)
    {
        bool isCurrent = (ci == m_CurrentCat);
        float btnY = catY + ci * CAT_BTN_H;

        // Hover detection
        bool hoverCat = io.MousePos.x >= winX + 6 && io.MousePos.x <= winX + sidebarW - 6 &&
                        io.MousePos.y >= btnY && io.MousePos.y <= btnY + CAT_BTN_H;

        // Category icon
        float iconX = winX + 12 + (4 * sidebarFrac);
        dl->AddText(ImGui::GetFont(), 14.0f,
            ImVec2(iconX, btnY + 6),
            isCurrent ? COL_ACCENT1 : (hoverCat ? COL_TEXT : COL_TEXT_DIM),
            s_CatIcons[ci]);

        // Category name (fades as sidebar collapses)
        if (sidebarFrac > 0.01f)
        {
            float nameAlpha = sidebarFrac;
            ImU32 nameCol = isCurrent ? InterpCol(Col32(0,0,0,0), COL_ACCENT1, nameAlpha)
                                      : InterpCol(Col32(0,0,0,0), hoverCat ? COL_TEXT : COL_TEXT_DIM, nameAlpha);
            float nameX = winX + 32 + (8 * sidebarFrac);
            dl->AddText(ImGui::GetFont(), 13.0f,
                ImVec2(nameX, btnY + 6),
                nameCol, s_CatNames[ci]);
        }

        // Click
        if (hoverCat && ImGui::IsMouseClicked(0))
        {
            m_CurrentCat = ci;
            m_SelMod.clear();
        }
    }

    // --- Module list area ---
    float modListX = modAreaX + 8;
    float modListY = winY + 10;
    float modListW = modAreaW - 16;

    // Get modules for this category
    std::vector<ModuleBase*> catMods;
    for (auto* m : modules)
        if ((int)m->GetCategory() == m_CurrentCat && m->IsVisible())
            catMods.push_back(m);

    // Calculate scroll
    float totalModH = catMods.size() * (MOD_H + 6);
    float visibleH = WIN_H - 40;
    float maxScroll = ImMax(0.0f, totalModH - visibleH);
    if (m_CatScroll[m_CurrentCat] > 0) m_CatScroll[m_CurrentCat] = 0;
    if (m_CatScroll[m_CurrentCat] < -maxScroll) m_CatScroll[m_CurrentCat] = -maxScroll;

    // Clip to module area
    dl->PushClipRect(ImVec2(modListX, winY + 8), ImVec2(modListX + modListW, winY + WIN_H - 8), true);

    float curY = modListY + m_CatScroll[m_CurrentCat];

    for (auto* mod : catMods)
    {
        std::string name = mod->GetName();
        auto& ma = m_ModAnims[name];
        float modW = modListW;

        // Animate toggle
        float togTgt = mod->IsEnabled() ? 1.0f : 0.0f;
        ma.toggle = Lerp(ma.toggle, togTgt, 8.0f);

        // Hover
        bool hoverMod = io.MousePos.x >= modListX && io.MousePos.x <= modListX + modW &&
                        io.MousePos.y >= curY && io.MousePos.y <= curY + MOD_H;
        float hoverTgt = hoverMod ? 1.0f : 0.0f;
        ma.hover = Lerp(ma.hover, hoverTgt, 10.0f);

        bool isBinding = (m_BindingMod == name);

        // Module background
        ImU32 modBg = InterpCol(COL_MODULE, COL_HOVER_BG, ma.hover);
        if (ma.toggle > 0.01f)
        {
            float t = ma.toggle;
            int r = (30 + (int)((90 - 30) * t * 0.3f));
            int g = (31 + (int)((170 - 31) * t * 0.3f));
            int b = (35 + (int)((250 - 35) * t * 0.3f));
            modBg = Col32(ImMin(r, 60), ImMin(g, 80), ImMin(b, 120), 255);
        }
        DrawRndRect(dl, ImVec2(modListX, curY), ImVec2(modListX + modW, curY + MOD_H), modBg, 5);

        // Icon area (34x34 rounded rect on left)
        float iconR = 5;
        DrawRndRect(dl, ImVec2(modListX + 1, curY + 1), ImVec2(modListX + MOD_H - 1, curY + MOD_H - 1), COL_ICON_BG, iconR);

        // Gradient fill when enabled (scales with toggle anim)
        if (ma.toggle > 0.01f)
        {
            float scale = 0.3f + 0.7f * ma.toggle;
            float pad = (MOD_H - 2) * (1.0f - scale) * 0.5f;
            float ix1 = modListX + 1 + pad;
            float iy1 = curY + 1 + pad;
            float ix2 = modListX + MOD_H - 1 - pad;
            float iy2 = curY + MOD_H - 1 - pad;
            ImU32 grad1 = Col32(90, 170, 250, (int)(255 * ma.toggle));
            ImU32 grad2 = Col32(60, 120, 220, (int)(255 * ma.toggle));
            DrawGradCornerLR(dl, ImVec2(ix1, iy1), ImVec2(ix2, iy2), iconR, grad1, grad2);
        }

        // Inner circle (empty center)
        DrawGoodCircle(dl, ImVec2(modListX + MOD_H * 0.5f, curY + MOD_H * 0.5f), 5, COL_MODULE);

        // Checkmark (scales with elastic-like anim when enabling)
        float checkScale = ImMin(ma.toggle * 1.2f, 1.0f);
        if (checkScale > 0.01f)
        {
            // Draw a simple checkmark character
            float cx = modListX + MOD_H * 0.5f;
            float cy = curY + MOD_H * 0.5f;
            // Use a simple checkmark: "/" + "\" rendered as lines
            float s = 4 * checkScale;
            dl->AddLine(ImVec2(cx - s, cy), ImVec2(cx, cy + s), Col32(255, 255, 255, (int)(255 * checkScale)), 2.0f);
            dl->AddLine(ImVec2(cx, cy + s), ImVec2(cx + s + 1, cy - s), Col32(255, 255, 255, (int)(255 * checkScale)), 2.0f);
        }

        // Module name
        float nameX = modListX + MOD_H + 10;
        float nameY = curY + (MOD_H - ImGui::GetFontSize()) * 0.5f;
        float textAlpha = 0.5f + 0.5f * ma.toggle;
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
            ImVec2(nameX, nameY),
            Col32(255, 255, 255, (int)(255 * textAlpha)), name.c_str());

        // Description on hover
        if (hoverMod && !isBinding)
        {
            float descX = nameX + ImGui::CalcTextSize(name.c_str()).x + 10;
            float descAvail = modListX + modW - descX - 30;
            if (descAvail > 20)
            {
                std::string desc = mod->GetDescription();
                if (!desc.empty())
                {
                    dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() - 2,
                        ImVec2(descX, nameY + 2),
                        COL_TEXT_DIM, desc.c_str());
                }
            }
        }

        // Bind text
        if (isBinding)
        {
            float bindX = nameX + ImGui::CalcTextSize(name.c_str()).x + 10;
            std::string bindText = "Press a key...";
            dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() - 2,
                ImVec2(bindX, nameY + 2),
                COL_ACCENT1, bindText.c_str());
        }

        // 3-dot menu on right (shows when hovered or settings open for this module)
        bool showDots = hoverMod || m_SelMod == name;
        if (showDots)
        {
            ImU32 dotsCol = (m_SelMod == name) ? COL_ACCENT1 : COL_TEXT_DIM;
            DrawThreeDots(dl, modListX + modW - 10, curY + 8, dotsCol);
        }

        // Click handling
        if (hoverMod && ImGui::IsMouseClicked(0))
        {
            mod->Toggle();
        }
        if (hoverMod && ImGui::IsMouseClicked(1))
        {
            if (!mod->GetSettings().empty())
            {
                if (m_SelMod == name)
                    m_SelMod.clear();
                else
                {
                    m_SelMod = name;
                    m_SettingsScroll = 0;
                }
            }
        }
        if (hoverMod && ImGui::IsMouseClicked(2))
        {
            m_BindingMod = name;
        }

        curY += MOD_H + 6;
    }

    dl->PopClipRect();

    // --- Settings panel ---
    if (m_SettingsAnim > 0.01f && !m_SelMod.empty())
    {
        float setX = winX + BASE_W;
        float setY = winY + 10;
        float setW = settingsW - 10;
        float setH = WIN_H - 20;

        // Find the module
        ModuleBase* selMod = nullptr;
        for (auto* m : modules)
            if (m->GetName() == m_SelMod) { selMod = m; break; }

        if (selMod)
        {
            // Settings background
            DrawRndRect(dl, ImVec2(setX, winY), ImVec2(setX + settingsW, winY + WIN_H),
                COL_SETTINGS_BG, RADIUS);

            // Module name header
            dl->AddText(ImGui::GetFont(), 14.0f,
                ImVec2(setX + 10, winY + 12),
                COL_TEXT, selMod->GetName().c_str());

            // Divider
            dl->AddLine(ImVec2(setX + 8, winY + 34), ImVec2(setX + settingsW - 8, winY + 34), COL_LIGHTER, 1.0f);

            // Settings list with scroll
            auto& settings = selMod->GetSettings();

            // Calculate max scroll
            float setTotalH = 0;
            for (auto* s : settings)
            {
                if (s->GetType() == SettingType::Keybind) continue;
                setTotalH += 30.0f;
            }
            float setMaxScroll = ImMax(0.0f, setTotalH - (WIN_H - 60));
            if (m_SettingsScroll > 0) m_SettingsScroll = 0;
            if (m_SettingsScroll < -setMaxScroll) m_SettingsScroll = -setMaxScroll;

            // Scroll on hover
            bool hoverSetArea = io.MousePos.x >= setX && io.MousePos.x <= setX + settingsW &&
                                io.MousePos.y >= winY + 40 && io.MousePos.y <= winY + WIN_H;
            if (hoverSetArea)
                m_SettingsScroll += io.MouseWheel * 20.0f;

            // Clip
            dl->PushClipRect(ImVec2(setX + 4, winY + 38), ImVec2(setX + settingsW - 4, winY + WIN_H - 4), true);

            float setCurY = winY + 42 + m_SettingsScroll;

            for (auto* setting : settings)
            {
                if (setting->GetType() == SettingType::Keybind) continue;
                float setRowH = 28.0f;
                if (setCurY + setRowH < winY + 38) { setCurY += setRowH; continue; }
                if (setCurY > winY + WIN_H - 4) break;

                std::string sName = setting->GetName();

                // Boolean toggle
                if (setting->GetType() == SettingType::Boolean)
                {
                    BooleanSetting* bs = (BooleanSetting*)setting;
                    bool val = bs->GetValue();

                    dl->AddText(ImGui::GetFont(), 12.0f,
                        ImVec2(setX + 10, setCurY + 4),
                        COL_TEXT, sName.c_str());

                    // Toggle pill
                    float pillW = 28.0f, pillH = 12.0f;
                    float pillX = setX + settingsW - pillW - 12;
                    float pillY = setCurY + (setRowH - pillH) * 0.5f;
                    DrawTogglePill(dl, ImVec2(pillX, pillY), pillW, pillH, val, 0);

                    // Click to toggle
                    if (io.MousePos.x >= pillX && io.MousePos.x <= pillX + pillW &&
                        io.MousePos.y >= pillY && io.MousePos.y <= pillY + pillH &&
                        ImGui::IsMouseClicked(0))
                    {
                        bs->SetValue(!val);
                    }
                }
                // Number slider
                else if (setting->GetType() == SettingType::Number)
                {
                    NumberSetting* ns = (NumberSetting*)setting;
                    float val = ns->GetValue();

                    char buf[64];
                    snprintf(buf, sizeof(buf), "%s: %.2f", sName.c_str(), val);
                    dl->AddText(ImGui::GetFont(), 11.0f,
                        ImVec2(setX + 10, setCurY + 2),
                        COL_TEXT, buf);

                    // Slider track
                    float trX = setX + 10;
                    float trY = setCurY + 18;
                    float trW = settingsW - 30;
                    float trH = 4.0f;
                    float pct = (val - ns->GetMin()) / (ns->GetMax() - ns->GetMin());
                    pct = ImClamp(pct, 0.0f, 1.0f);

                    // Background track
                    DrawRndRect(dl, ImVec2(trX, trY), ImVec2(trX + trW, trY + trH),
                        Col32(30, 31, 35, 255), trH * 0.5f);

                    // Filled
                    DrawRndRect(dl, ImVec2(trX, trY), ImVec2(trX + trW * pct, trY + trH),
                        COL_ACCENT1, trH * 0.5f);

                    // Knob
                    float knobX = trX + trW * pct;
                    float knobY = trY + trH * 0.5f;
                    DrawCircle(dl, ImVec2(knobX, knobY), 4.0f, COL_TEXT);

                    // Drag handling
                    bool hoverSlider = io.MousePos.x >= trX - 5 && io.MousePos.x <= trX + trW + 5 &&
                                       io.MousePos.y >= trY - 5 && io.MousePos.y <= trY + trH + 5;

                    if (m_DragSetting == sName)
                    {
                        float newPct = (io.MousePos.x - trX) / trW;
                        newPct = ImClamp(newPct, 0.0f, 1.0f);
                        float newVal = ns->GetMin() + newPct * (ns->GetMax() - ns->GetMin());
                        ns->SetValue(newVal);
                        if (!ImGui::IsMouseDown(0))
                            m_DragSetting.clear();
                    }
                    else if (hoverSlider && ImGui::IsMouseClicked(0))
                    {
                        m_DragSetting = sName;
                    }
                }
                // Enum dropdown
                else if (setting->GetType() == SettingType::Enum)
                {
                    EnumSetting* es = (EnumSetting*)setting;
                    int val = es->GetValue();
                    const auto& opts = es->GetOptions();

                    dl->AddText(ImGui::GetFont(), 12.0f,
                        ImVec2(setX + 10, setCurY + 2),
                        COL_TEXT, sName.c_str());

                    if (val >= 0 && val < (int)opts.size())
                    {
                        dl->AddText(ImGui::GetFont(), 11.0f,
                            ImVec2(setX + 10, setCurY + 16),
                            COL_ACCENT1, opts[val].c_str());
                    }

                    bool hoverEnum = io.MousePos.x >= setX + 6 && io.MousePos.x <= setX + settingsW - 6 &&
                                     io.MousePos.y >= setCurY && io.MousePos.y <= setCurY + setRowH;
                    if (hoverEnum && ImGui::IsMouseClicked(0))
                    {
                        int next = (val + 1) % (int)opts.size();
                        es->SetValue(next);
                    }
                }
                // Color swatch
                else if (setting->GetType() == SettingType::Color)
                {
                    ColorSetting* cs = (ColorSetting*)setting;
                    ImVec4 col = cs->GetVec4();

                    dl->AddText(ImGui::GetFont(), 12.0f,
                        ImVec2(setX + 10, setCurY + 4),
                        COL_TEXT, sName.c_str());

                    float swSize = 14.0f;
                    float swX = setX + settingsW - swSize - 14;
                    float swY = setCurY + (setRowH - swSize) * 0.5f;
                    DrawRndRect(dl, ImVec2(swX, swY), ImVec2(swX + swSize, swY + swSize),
                        Col32((int)(col.x * 255), (int)(col.y * 255), (int)(col.z * 255), 255), 3);
                }

                setCurY += setRowH;
            }

            dl->PopClipRect();
        }
    }

    // --- Window drag ---
    bool hoverTitle = io.MousePos.x >= winX && io.MousePos.x <= winX + totalW &&
                      io.MousePos.y >= winY && io.MousePos.y <= winY + 30;
    if (hoverTitle && ImGui::IsMouseClicked(0) && !m_Dragging)
    {
        m_Dragging = true;
        m_DragOffX = io.MousePos.x - m_WinX;
        m_DragOffY = io.MousePos.y - m_WinY;
    }
    if (m_Dragging)
    {
        if (ImGui::IsMouseDown(0))
        {
            m_WinX = io.MousePos.x - m_DragOffX;
            m_WinY = io.MousePos.y - m_DragOffY;
        }
        else
        {
            m_Dragging = false;
        }
    }

    // --- Mouse wheel in module area ---
    bool hoverModArea = io.MousePos.x >= modAreaX && io.MousePos.x <= modAreaX + modAreaW &&
                        io.MousePos.y >= winY && io.MousePos.y <= winY + WIN_H;
    bool hoveringSettings = hasSettings &&
        io.MousePos.x >= winX + BASE_W && io.MousePos.x <= winX + totalW &&
        io.MousePos.y >= winY + 40 && io.MousePos.y <= winY + WIN_H;
    if (hoverModArea && !hoveringSettings)
    {
        m_CatScroll[m_CurrentCat] += io.MouseWheel * 30.0f;
    }

    // --- Keybind handling ---
    if (!m_BindingMod.empty())
    {
        for (int key = 0; key < 256; key++)
        {
            if (ImGui::IsKeyPressed((ImGuiKey)key, false))
            {
                ModuleBase* bindMod = nullptr;
                for (auto* m : modules)
                    if (m->GetName() == m_BindingMod) { bindMod = m; break; }
                if (bindMod)
                {
                    if (key == VK_ESCAPE || key == VK_SPACE || key == VK_DELETE)
                        bindMod->SetKeybind(0);
                    else
                        bindMod->SetKeybind(key);
                }
                m_BindingMod.clear();
                break;
            }
        }
    }

    // --- Close on Escape ---
    if (m_Open && ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        Logger::Log("Escape pressed in Menu::Render()");
        if (!m_SelMod.empty())
            m_SelMod.clear();
        else
            Close();
    }

    // Force cursor visible every frame while menu is open
    if (m_Open)
    {
        ShowCursor(TRUE);
        ClipCursor(NULL);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
}

void Menu::SaveConfig()
{
    try
    {
        std::string path = std::getenv("APPDATA");
        path += "/PaperScents";
        std::string filePath = path + "/config.json";

        std::string mkdir = "if not exist \"" + path + "\" mkdir \"" + path + "\"";
        system(mkdir.c_str());

        auto& modules = ModuleHandler::GetInstance().GetModules();
        nlohmann::json j;
        for (auto* mod : modules)
            j["modules"][mod->GetName()] = mod->Serialize();

        std::ofstream file(filePath);
        if (file.is_open())
        {
            file << j.dump(4);
            file.close();
        }
    }
    catch (...) {}
}

void Menu::LoadConfig()
{
    try
    {
        std::string path = std::getenv("APPDATA");
        path += "/PaperScents/config.json";
        std::ifstream file(path);
        if (!file.is_open()) return;

        nlohmann::json j;
        file >> j;
        file.close();

        if (!j.contains("modules")) return;

        auto& modules = ModuleHandler::GetInstance().GetModules();
        for (auto& [modName, modData] : j["modules"].items())
        {
            for (auto* mod : modules)
            {
                if (mod->GetName() == modName)
                {
                    mod->Deserialize(modData);
                    break;
                }
            }
        }
    }
    catch (...) {}
}

void Menu::Toggle()
{
    if (m_Open) Close(); else Open();
}

void Menu::Open()
{
    if (m_Open) { Logger::Log("Menu::Open() skipped — already open"); return; }
    m_Open = true;
    m_OpeningAnim = 0.0f;
    Logger::Log("Menu::Open() called, m_Open=true");
}

void Menu::Close()
{
    if (!m_Open) { Logger::Log("Menu::Close() skipped — already closed"); return; }
    m_Open = false;
    Logger::Log("Menu::Close() called, m_Open=false");
}

void Menu::Shutdown()
{
    SaveConfig();
    m_Initialized = false;
}

#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.h"
#include "../neverlose/gui.hpp"
#include "../../utilities/logger.h"
#include "../../modulehandler/modulehandler.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cctype>
#include <fstream>
#include <Windows.h>

Menu& Menu::GetInstance()
{
    static Menu instance;
    return instance;
}

void Menu::Initialize()
{
    m_Categories = { "Combat", "Movement", "Render", "Player", "Misc", "Scripts" };
    LoadConfig();
    m_Initialized = true;
}

#define col4(r,g,b,a) ImVec4(r/255.f,g/255.f,b/255.f,a/255.f)
#define col3(r,g,b) ImVec4(r/255.f,g/255.f,b/255.f,1.f)

static void ApplyModernTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(0, 0);
    style.FramePadding = ImVec2(10, 6);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(8, 4);
    style.ScrollbarSize = 6;
    style.GrabMinSize = 6;
    style.WindowRounding = 10;
    style.FrameRounding = 6;
    style.PopupRounding = 8;
    style.ScrollbarRounding = 4;
    style.GrabRounding = 6;
    style.TabRounding = 6;
    style.ChildRounding = 8;
    style.WindowBorderSize = 0;
    style.FrameBorderSize = 0;
    style.PopupBorderSize = 0;


    ImVec4 bg = col3(14, 14, 18);
    ImVec4 bg2 = col3(20, 20, 26);
    ImVec4 bg3 = col3(26, 26, 34);
    ImVec4 surface = col3(32, 32, 42);
    ImVec4 accent = col4(90, 170, 250, 255);
    ImVec4 accentHover = col4(120, 195, 255, 255);
    ImVec4 text = col3(210, 210, 220);
    ImVec4 textDim = col3(120, 120, 140);

    style.Colors[ImGuiCol_WindowBg] = bg;
    style.Colors[ImGuiCol_ChildBg] = bg2;
    style.Colors[ImGuiCol_PopupBg] = col3(22, 22, 30);
    style.Colors[ImGuiCol_Border] = col3(40, 40, 52);
    style.Colors[ImGuiCol_FrameBg] = col3(28, 28, 38);
    style.Colors[ImGuiCol_FrameBgHovered] = col3(38, 38, 50);
    style.Colors[ImGuiCol_FrameBgActive] = col3(48, 48, 62);
    style.Colors[ImGuiCol_TitleBg] = col3(16, 16, 22);
    style.Colors[ImGuiCol_TitleBgActive] = col3(20, 20, 28);
    style.Colors[ImGuiCol_Button] = surface;
    style.Colors[ImGuiCol_ButtonHovered] = col3(42, 42, 55);
    style.Colors[ImGuiCol_ButtonActive] = col3(52, 52, 68);
    style.Colors[ImGuiCol_Header] = surface;
    style.Colors[ImGuiCol_HeaderHovered] = col3(42, 42, 55);
    style.Colors[ImGuiCol_HeaderActive] = col3(52, 52, 68);
    style.Colors[ImGuiCol_CheckMark] = accent;
    style.Colors[ImGuiCol_SliderGrab] = accent;
    style.Colors[ImGuiCol_SliderGrabActive] = accentHover;
    style.Colors[ImGuiCol_Text] = text;
    style.Colors[ImGuiCol_TextDisabled] = textDim;
    style.Colors[ImGuiCol_Separator] = col3(40, 40, 52);
    style.Colors[ImGuiCol_ResizeGrip] = surface;
    style.Colors[ImGuiCol_ResizeGripHovered] = col3(42, 42, 55);
    style.Colors[ImGuiCol_ResizeGripActive] = col3(52, 52, 68);
    style.Colors[ImGuiCol_Tab] = bg2;
    style.Colors[ImGuiCol_TabHovered] = surface;
    style.Colors[ImGuiCol_TabActive] = surface;
    style.Colors[ImGuiCol_TabUnfocused] = bg2;
    style.Colors[ImGuiCol_TabUnfocusedActive] = bg3;
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0,0,0,0);
    style.Colors[ImGuiCol_ScrollbarGrab] = col3(45, 45, 58);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = col3(60, 60, 75);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = col3(75, 75, 90);
}

void Menu::Render()
{
    if (!m_Open) return;
    if (!m_Initialized) Initialize();

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center = io.DisplaySize;
    ImGui::SetNextWindowSize(ImVec2(860, 540), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(center.x * 0.5f, center.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

    ApplyModernTheme();

    if (ImGui::Begin("PaperScents", &m_Open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
    {
        ImVec2 winSize = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos();

        // Gradient title bar
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0,0,0,0));
        ImGui::BeginChild("##titlebar", ImVec2(winSize.x, 42), false);
        ImVec2 tPos = ImGui::GetCursorScreenPos();
        dl->AddRectFilledMultiColor(
            ImVec2(pos.x, pos.y),
            ImVec2(pos.x + winSize.x, pos.y + 42),
            IM_COL32(30, 60, 120, 255),
            IM_COL32(20, 40, 90, 255),
            IM_COL32(40, 30, 100, 255),
            IM_COL32(30, 20, 80, 255));

        ImGui::SetCursorPos(ImVec2(16, 10));
        ImGui::TextColored(ImVec4(1,1,1,0.9f), "PaperScents");
        ImGui::SameLine(winSize.x - 100);
        ImGui::SetCursorPosY(9);
        ImGui::TextColored(ImVec4(1,1,1,0.4f), "1.8.9");
        ImGui::SameLine(winSize.x - 44);
        ImGui::SetCursorPosY(9);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.1f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,0.2f,0.2f,0.2f));
        if (ImGui::Button("X", ImVec2(28, 24)))
            m_Open = false;
        ImGui::PopStyleColor(3);
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Search bar
        ImGui::PushStyleColor(ImGuiCol_ChildBg, col3(16, 16, 22));
        ImGui::BeginChild("##searchbar", ImVec2(winSize.x, 44), false);
        ImGui::SetCursorPos(ImVec2(14, 10));
        ImGui::PushItemWidth(220);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, col3(26, 26, 36));
        ImGui::PushStyleColor(ImGuiCol_Text, col3(140, 140, 160));
        char searchBuf[128] = {};
        strncpy_s(searchBuf, m_Filter.c_str(), sizeof(searchBuf) - 1);
        if (ImGui::InputTextWithHint("##search", "Search modules...", searchBuf, sizeof(searchBuf)))
            m_Filter = searchBuf;
        ImGui::PopStyleColor(2);
        ImGui::PopItemWidth();

        // Module count
        auto& modules = ModuleHandler::GetInstance().GetModules();
        int count = 0;
        for (auto* mod : modules)
            if (mod->GetCategory() == (Category)m_ActiveCategory)
                count++;
        ImGui::SameLine(winSize.x - 120);
        ImGui::SetCursorPosY(12);
        ImGui::TextColored(col3(100, 100, 130), "%d modules", count);
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 0));

        // Body
        ImGui::PushStyleColor(ImGuiCol_ChildBg, col3(14, 14, 18));
        ImGui::BeginChild("##body", ImVec2(winSize.x, winSize.y - 88), false);

        RenderSidebar();
        ImGui::SameLine();
        RenderModuleCards();

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    ImGui::End();
}

void Menu::RenderSidebar()
{
    float sidebarW = 150;
    float bodyH = ImGui::GetContentRegionAvail().y;
    ImVec2 wPos = ImGui::GetWindowPos();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, col3(16, 16, 22));
    ImGui::BeginChild("##sidebar", ImVec2(sidebarW, bodyH), false);
    ImGui::SetCursorPosY(8);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    for (int i = 0; i < (int)m_Categories.size(); i++)
    {
        bool active = (i == m_ActiveCategory);
        ImVec2 btnPos = ImGui::GetCursorScreenPos();
        ImGui::SetCursorPosX(8);

        ImGui::PushStyleColor(ImGuiCol_Text, active ? ImVec4(1,1,1,1) : col3(130, 130, 150));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col4(255,255,255,6));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, col4(255,255,255,10));

        if (ImGui::Button(m_Categories[i], ImVec2(sidebarW - 16, 36)))
            m_ActiveCategory = i;

        // Active indicator bar
        if (active)
        {
            dl->AddRectFilled(
                ImVec2(btnPos.x + 2, btnPos.y + 4),
                ImVec2(btnPos.x + 4, btnPos.y + 32),
                IM_COL32(90, 170, 250, 255), 2);
        }

        ImGui::PopStyleColor(4);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

static void AnimatedToggle(const char* label, bool* value, float width = 40)
{
    ImGui::PushID(label);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float height = 20;
    float radius = height * 0.5f;

    ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, ImGui::GetID(label)))
    {
        ImGui::PopID();
        return;
    }

    bool hovered = ImGui::IsItemHovered();
    bool clicked = ImGui::IsMouseClicked(0) && hovered;
    if (clicked) *value = !*value;

    float t = *value ? 1.0f : 0.0f;
    ImVec4 bgCol;
    if (*value)
        bgCol = hovered ? col4(100, 200, 255, 255) : col4(80, 170, 230, 255);
    else
        bgCol = hovered ? col4(55, 55, 70, 255) : col4(45, 45, 58, 255);

    dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(bgCol.x*255, bgCol.y*255, bgCol.z*255, bgCol.w*255), radius);
    float circleX = pos.x + (t * (width - height)) + radius;
    dl->AddCircleFilled(ImVec2(circleX, pos.y + radius), radius - 3, *value ? IM_COL32(255,255,255,255) : IM_COL32(180,180,190,255));

    ImGui::PopID();
}

void Menu::RenderModuleCards()
{
    auto& modules = ModuleHandler::GetInstance().GetModules();
    float availW = ImGui::GetContentRegionAvail().x - 158;
    float bodyH = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("##cards", ImVec2(availW, bodyH), false);

    Category targetCat = (Category)m_ActiveCategory;
    std::string filter = m_Filter;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

    int cardIdx = 0;
    for (auto* mod : modules)
    {
        if (mod->GetCategory() != targetCat) continue;

        std::string name = mod->GetName();
        std::string nameLower = name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        if (!filter.empty() && nameLower.find(filter) == std::string::npos)
            continue;

        if (cardIdx % 2 != 0)
            ImGui::SameLine();
        cardIdx++;

        float cardW = (availW - 12) / 2.0f;
        float cardH = 0;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, col3(20, 20, 27));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);

        // Subtle border based on enabled state
        ImVec4 borderCol = mod->IsEnabled() ? col4(80, 170, 230, 80) : col4(50, 50, 65, 60);
        ImGui::PushStyleColor(ImGuiCol_Border, borderCol);

        ImGui::BeginChild((std::string("##mod_") + name).c_str(), ImVec2(cardW, 0), true);

        ImGui::PopStyleColor(); // border
        ImGui::PopStyleVar(2);  // rounding + border size

        ImGui::Dummy(ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 6));

        // Row: name + toggle
        ImGui::SetCursorPosX(12);
        ImGui::SetCursorPosY(10);
        ImVec4 titleCol = mod->IsEnabled() ? ImVec4(1,1,1,1) : col3(150, 150, 170);
        ImGui::PushStyleColor(ImGuiCol_Text, titleCol);
        ImGui::Text("%s", name.c_str());
        ImGui::PopStyleColor();

        ImGui::SameLine(cardW - 56);
        ImGui::SetCursorPosY(9);
        bool enabled = mod->IsEnabled();
        AnimatedToggle("##toggle", &enabled, 42);
        if (enabled != mod->IsEnabled())
            mod->Toggle();
        bool isEn = mod->IsEnabled();

        // Description
        ImGui::SetCursorPosX(12);
        ImGui::PushStyleColor(ImGuiCol_Text, col3(100, 100, 125));
        ImGui::TextWrapped("%s", mod->GetDescription().c_str());
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 4));

        // Keybind chip
        ImGui::SetCursorPosX(12);
        bool isBinding = (m_WaitingBind == mod->GetName());
        if (isBinding)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, col3(100, 200, 130));
            ImGui::Text("Press a key...  [Esc = clear]");
            ImGui::PopStyleColor();
            for (int k = 1; k < 256; k++)
            {
                if (GetAsyncKeyState(k) & 0x8000)
                {
                    mod->SetKeybind((k == VK_ESCAPE) ? 0 : k);
                    m_WaitingBind.clear();
                    break;
                }
            }
        }
        else
        {
            std::string keyText = KeybindSetting::KeyToString(mod->GetKeybind());
            std::string btnLabel = "Bind: " + (keyText.empty() ? "None" : keyText);
            ImVec2 btnSize = ImVec2(cardW - 24, 24);
            ImGui::PushStyleColor(ImGuiCol_Button, col3(28, 28, 38));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col3(38, 38, 50));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, col3(48, 48, 62));
            ImGui::PushStyleColor(ImGuiCol_Text, col3(130, 130, 155));
            if (ImGui::Button(btnLabel.c_str(), btnSize))
                m_WaitingBind = mod->GetName();
            ImGui::PopStyleColor(4);
        }

        ImGui::Dummy(ImVec2(0, 2));

        // Expandable settings
        if (!mod->GetSettings().empty())
        {
            ImGui::SetCursorPosX(12);
            ImGui::PushStyleColor(ImGuiCol_Header, col3(26, 26, 36));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, col3(36, 36, 48));
            ImGui::PushStyleColor(ImGuiCol_Text, col3(140, 140, 165));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
            bool open = ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_None);
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);

            if (open)
            {
                ImGui::SetCursorPosX(8);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 6));
                ImGui::Indent(8);
                for (auto* setting : mod->GetSettings())
                    setting->Render();
                ImGui::Unindent(8);
                ImGui::PopStyleVar();
            }
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::PopStyleColor(); // child bg
    }

    ImGui::EndChild();
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

void Menu::Shutdown()
{
    SaveConfig();
    m_Initialized = false;
}

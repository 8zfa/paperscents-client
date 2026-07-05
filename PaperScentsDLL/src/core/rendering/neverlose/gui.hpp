#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "color.hpp"

class NeverloseGUI
{
public:
    float m_Anim = 0.0f;
    int m_CurrentTab = 0;

    struct TabInfo {
        std::string Name;
        std::string Icon; // Unicode symbol or emoji
    };

    std::vector<TabInfo> m_Tabs;

    bool Tab(const char* label, bool selected);
    void GroupBox(const char* name, ImVec2 size);
    void EndGroupBox();
    void GroupTitle(const char* name);

    void ApplyStyle();
};

inline NeverloseGUI g_NeverloseGUI;

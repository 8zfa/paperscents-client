#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <json.hpp>

class Menu
{
public:
    static Menu& GetInstance();

    void Initialize();
    void Render();
    void Shutdown();

    bool IsOpen() const { return m_Open; }
    void Toggle();
    void Open();
    void Close();

    void SaveConfig();
    void LoadConfig();

private:
    Menu() = default;
    ~Menu() = default;
    Menu(const Menu&) = delete;
    Menu& operator=(const Menu&) = delete;

    bool m_Open = false;
    bool m_Initialized = false;

    // Window drag
    float m_WinX = 40, m_WinY = 40;
    bool m_Dragging = false;
    float m_DragOffX = 0, m_DragOffY = 0;
    bool m_FirstOpen = true;

    // Opening animation (slide from left)
    float m_OpeningAnim = 0.0f;

    // Sidebar expand/collapse on hover
    float m_SidebarAnim = 0.0f;

    int m_CurrentCat = 0; // 0-4

    // Per-category scroll
    float m_CatScroll[5] = {};

    // Module hover/toggle anims
    struct ModAnim {
        float toggle = 0.0f;
        float hover = 0.0f;
    };
    std::unordered_map<std::string, ModAnim> m_ModAnims;

    // Settings panel
    std::string m_SelMod;
    float m_SettingsAnim = 0.0f;
    float m_SettingsScroll = 0.0f;
    bool m_SettingsRightClicked = false;

    // Keybind binding
    std::string m_BindingMod;

    // Slider dragging
    std::string m_DragSetting;

    // Category icons
    static const char* s_CatIcons[5];
    static const char* s_CatNames[5];
};

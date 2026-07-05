#pragma once
#include <string>
#include <vector>
#include <json.hpp>

class Menu
{
public:
    static Menu& GetInstance();

    void Initialize();
    void Render();
    void Shutdown();

    bool IsOpen() const { return m_Open; }
    void Toggle() { m_Open = !m_Open; }
    void Open() { m_Open = true; }
    void Close() { m_Open = false; }

    void SaveConfig();
    void LoadConfig();

private:
    Menu() = default;
    ~Menu() = default;
    Menu(const Menu&) = delete;
    Menu& operator=(const Menu&) = delete;

    void RenderSidebar();
    void RenderModuleCards();

    bool m_Open = false;
    bool m_Initialized = false;
    std::string m_Filter;
    int m_ActiveCategory = 0;
    std::string m_WaitingBind;
    std::vector<const char*> m_Categories;
};

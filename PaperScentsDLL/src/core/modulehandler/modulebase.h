#pragma once
#include <string>
#include <vector>
#include <memory>
#include "settings/settings.h"

enum class Category
{
    Combat,
    Movement,
    Render,
    Player,
    Misc,
    Scripts
};

inline const char* CategoryToString(Category c)
{
    switch (c)
    {
    case Category::Combat:    return "Combat";
    case Category::Movement:  return "Movement";
    case Category::Render:    return "Render";
    case Category::Player:    return "Player";
    case Category::Misc:      return "Misc";
    case Category::Scripts:   return "Scripts";
    default: return "Unknown";
    }
}

class ModuleBase
{
public:
    ModuleBase(const std::string& name, const std::string& desc, Category cat)
        : m_Name(name), m_Description(desc), m_Category(cat), m_Enabled(false), m_Visible(true), m_Keybind(0) {}

    virtual ~ModuleBase()
    {
        for (auto* s : m_Settings)
            delete s;
    }

    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnUpdate() {}
    virtual void OnRender() {}

    const std::string& GetName() const { return m_Name; }
    const std::string& GetDescription() const { return m_Description; }
    Category GetCategory() const { return m_Category; }
    const char* GetCategoryName() const { return CategoryToString(m_Category); }
    bool IsEnabled() const { return m_Enabled; }
    bool IsVisible() const { return m_Visible; }
    int GetKeybind() const { return m_Keybind; }
    const std::vector<Setting*>& GetSettings() const { return m_Settings; }

    void SetVisible(bool v) { m_Visible = v; }
    void SetKeybind(int k) { m_Keybind = k; }

    void SetEnabled(bool enabled)
    {
        if (enabled && !m_Enabled) { m_Enabled = true; OnEnable(); }
        else if (!enabled && m_Enabled) { m_Enabled = false; OnDisable(); }
    }

    void Toggle() { SetEnabled(!m_Enabled); }

    template<typename T, typename... Args>
    T* AddSetting(Args&&... args)
    {
        T* setting = new T(std::forward<Args>(args)...);
        m_Settings.push_back(setting);
        return setting;
    }

    Setting* FindSetting(const std::string& name)
    {
        for (auto* s : m_Settings)
            if (s->GetName() == name) return s;
        return nullptr;
    }

    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);

private:
    std::string m_Name;
    std::string m_Description;
    Category m_Category;
    bool m_Enabled;
    bool m_Visible;
    int m_Keybind;
    std::vector<Setting*> m_Settings;
};

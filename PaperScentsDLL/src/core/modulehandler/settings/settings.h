#pragma once
#include <string>
#include <vector>
#include <imgui.h>
#include <json.hpp>

enum class SettingType { Boolean, Number, Enum, Keybind, Color };

class Setting
{
public:
    Setting(const std::string& name, const std::string& desc, SettingType type)
        : m_Name(name), m_Description(desc), m_Type(type) {}
    virtual ~Setting() = default;

    const std::string& GetName() const { return m_Name; }
    const std::string& GetDescription() const { return m_Description; }
    SettingType GetType() const { return m_Type; }

    virtual void Render() = 0;
    virtual nlohmann::json Serialize() = 0;
    virtual void Deserialize(const nlohmann::json& j) = 0;

protected:
    std::string m_Name;
    std::string m_Description;
    SettingType m_Type;
};

class BooleanSetting : public Setting
{
public:
    BooleanSetting(const std::string& name, bool value, const std::string& desc = "")
        : Setting(name, desc, SettingType::Boolean), m_Value(value) {}

    bool GetValue() const { return m_Value; }
    void SetValue(bool v) { m_Value = v; }

    void Render() override
    {
        ImGui::PushID(this);
        ImGui::Checkbox(m_Name.c_str(), &m_Value);
        if (ImGui::IsItemHovered() && !m_Description.empty())
            ImGui::SetTooltip("%s", m_Description.c_str());
        ImGui::PopID();
    }

    nlohmann::json Serialize() override { return m_Value; }
    void Deserialize(const nlohmann::json& j) override { m_Value = j; }

private:
    bool m_Value;
};

class NumberSetting : public Setting
{
public:
    NumberSetting(const std::string& name, float value, float min, float max, float inc, const std::string& desc = "")
        : Setting(name, desc, SettingType::Number), m_Value(value), m_Min(min), m_Max(max), m_Increment(inc) {}

    float GetValue() const { return m_Value; }
    float GetMin() const { return m_Min; }
    float GetMax() const { return m_Max; }
    void SetValue(float v) { m_Value = v; }
    int GetIntValue() const { return (int)m_Value; }

    void Render() override
    {
        ImGui::PushID(this);
        ImGui::SetNextItemWidth(140);
        if (m_Increment >= 1.0f)
        {
            int val = (int)m_Value;
            if (ImGui::SliderInt(m_Name.c_str(), &val, (int)m_Min, (int)m_Max))
                m_Value = (float)val;
        }
        else
        {
            ImGui::SliderFloat(m_Name.c_str(), &m_Value, m_Min, m_Max, "%.2f");
        }
        if (ImGui::IsItemHovered() && !m_Description.empty())
            ImGui::SetTooltip("%s", m_Description.c_str());
        ImGui::PopID();
    }

    nlohmann::json Serialize() override { return m_Value; }
    void Deserialize(const nlohmann::json& j) override { m_Value = j; }

private:
    float m_Value, m_Min, m_Max, m_Increment;
};

class EnumSetting : public Setting
{
public:
    EnumSetting(const std::string& name, int value, const std::vector<std::string>& options, const std::string& desc = "")
        : Setting(name, desc, SettingType::Enum), m_Value(value), m_Options(options) {}

    int GetValue() const { return m_Value; }
    std::string GetSelected() const { return m_Options[m_Value]; }
    void SetValue(int v) { m_Value = v; }
    const std::vector<std::string>& GetOptions() const { return m_Options; }

    void Render() override
    {
        ImGui::PushID(this);
        ImGui::SetNextItemWidth(140);
        std::string preview = m_Options[m_Value];
        if (ImGui::BeginCombo(m_Name.c_str(), preview.c_str()))
        {
            for (size_t i = 0; i < m_Options.size(); i++)
            {
                bool selected = (int)i == m_Value;
                if (ImGui::Selectable(m_Options[i].c_str(), selected))
                    m_Value = (int)i;
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered() && !m_Description.empty())
            ImGui::SetTooltip("%s", m_Description.c_str());
        ImGui::PopID();
    }

    nlohmann::json Serialize() override { return m_Value; }
    void Deserialize(const nlohmann::json& j) override { m_Value = j; }

private:
    int m_Value;
    std::vector<std::string> m_Options;
};

class KeybindSetting : public Setting
{
public:
    KeybindSetting(const std::string& name, int value, const std::string& desc = "")
        : Setting(name, desc, SettingType::Keybind), m_Value(value) {}

    int GetValue() const { return m_Value; }
    void SetValue(int v) { m_Value = v; }

    void Render() override
    {
        ImGui::PushID(this);
        std::string label = m_Name + ": " + KeyToString(m_Value);
        ImGui::Text("%s", label.c_str());
        if (ImGui::IsItemHovered() && !m_Description.empty())
            ImGui::SetTooltip("%s", m_Description.c_str());
        ImGui::PopID();
    }

    static std::string KeyToString(int key);

    nlohmann::json Serialize() override { return m_Value; }
    void Deserialize(const nlohmann::json& j) override { m_Value = j; }

private:
    int m_Value;
};

class ColorSetting : public Setting
{
public:
    ColorSetting(const std::string& name, ImColor value, const std::string& desc = "")
        : Setting(name, desc, SettingType::Color), m_Value(value) {}

    ImColor GetValue() const { return m_Value; }
    ImVec4 GetVec4() const { return ImVec4(m_Value.Value.x, m_Value.Value.y, m_Value.Value.z, m_Value.Value.w); }
    ImU32 GetU32() const { return m_Value; }
    void SetValue(ImColor v) { m_Value = v; }

    void Render() override
    {
        ImGui::PushID(this);
        float col[4] = { m_Value.Value.x, m_Value.Value.y, m_Value.Value.z, m_Value.Value.w };
        if (ImGui::ColorEdit4(m_Name.c_str(), col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
            m_Value = ImColor(col[0], col[1], col[2], col[3]);
        if (ImGui::IsItemHovered() && !m_Description.empty())
            ImGui::SetTooltip("%s", m_Description.c_str());
        ImGui::PopID();
    }

    nlohmann::json Serialize() override
    {
        return { { "r", m_Value.Value.x }, { "g", m_Value.Value.y }, { "b", m_Value.Value.z }, { "a", m_Value.Value.w } };
    }

    void Deserialize(const nlohmann::json& j) override
    {
        m_Value = ImColor(j["r"].get<float>(), j["g"].get<float>(), j["b"].get<float>(), j["a"].get<float>());
    }

private:
    ImColor m_Value;
};

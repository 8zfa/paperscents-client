#include "modulebase.h"
#include <json.hpp>

nlohmann::json ModuleBase::Serialize() const
{
    nlohmann::json j;
    j["name"] = m_Name;
    j["enabled"] = m_Enabled;
    j["visible"] = m_Visible;
    j["keybind"] = m_Keybind;
    for (auto* s : m_Settings)
        j["settings"][s->GetName()] = s->Serialize();
    return j;
}

void ModuleBase::Deserialize(const nlohmann::json& j)
{
    m_Enabled = j.value("enabled", false);
    m_Visible = j.value("visible", true);
    m_Keybind = j.value("keybind", 0);
    if (j.contains("settings"))
    {
        for (auto& [key, val] : j["settings"].items())
        {
            for (auto* s : m_Settings)
            {
                if (s->GetName() == key)
                {
                    s->Deserialize(val);
                    break;
                }
            }
        }
    }
}

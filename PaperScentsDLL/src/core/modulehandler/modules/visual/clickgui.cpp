#include "clickgui.h"
#include "../../../utilities/logger.h"
#include "../../../rendering/menu/menu.h"

ClickGUIModule::ClickGUIModule()
    : ModuleBase("ClickGUI", "Open the ClickGUI", Category::Render)
{
    SetKeybind(VK_RSHIFT);
}

void ClickGUIModule::OnEnable()
{
    Menu::GetInstance().Open();
    Logger::Log("ClickGUI opened");
}

void ClickGUIModule::OnDisable()
{
    Menu::GetInstance().Close();
    Logger::Log("ClickGUI closed");
}

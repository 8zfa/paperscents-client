#include "clickgui.h"
#include "../../../utilities/logger.h"
#include "../../../rendering/menu/menu.h"

ClickGUIModule::ClickGUIModule()
    : ModuleBase("ClickGUI", "Open the ClickGUI", Category::Render)
{
    SetKeybind(VK_RSHIFT);
    SetVisible(false);
}

void ClickGUIModule::OnEnable()
{
    Menu::GetInstance().Open();
}

void ClickGUIModule::OnDisable()
{
    Menu::GetInstance().Close();
}

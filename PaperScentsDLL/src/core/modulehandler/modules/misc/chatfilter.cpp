#include "chatfilter.h"
#include "../../../utilities/logger.h"

ChatFilterModule::ChatFilterModule()
    : ModuleBase("ChatFilter", "Filter chat messages", Category::Misc)
{
    AddSetting<BooleanSetting>("FilterAdvertisements", true, "Filter advertisement messages");
    AddSetting<BooleanSetting>("FilterSwearWords", false, "Filter swear words");
}

void ChatFilterModule::OnEnable()
{
    Logger::Log("ChatFilter enabled - requires packet hook for full functionality");
}

void ChatFilterModule::OnDisable()
{
    Logger::Log("ChatFilter disabled");
}

void ChatFilterModule::OnUpdate()
{
    // Chat filtering requires a packet hook to intercept incoming chat packets.
    // This is a stub implementation until packet interception is added.
}

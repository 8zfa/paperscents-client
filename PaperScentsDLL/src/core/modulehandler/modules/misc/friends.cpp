#include "friends.h"
#include "../../../utilities/logger.h"
#include <algorithm>

std::vector<std::string> FriendsModule::m_Friends;

FriendsModule::FriendsModule()
    : ModuleBase("Friends", "Manage friends list for ESP highlighting", Category::Misc)
{
    AddSetting<BooleanSetting>("HighlightFriends", true, "Highlight friends in ESP");
    AddSetting<ColorSetting>("FriendColor", ImColor(0.0f, 0.8f, 0.4f, 1.0f), "Friend highlight color");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void FriendsModule::OnEnable() { Logger::Log("Friends enabled - %d friends loaded", (int)m_Friends.size()); }
void FriendsModule::OnDisable() { Logger::Log("Friends disabled"); }

void FriendsModule::OnUpdate()
{
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    // Friends list is a static utility - other modules call IsFriend()
    // No per-frame game interaction needed
}

void FriendsModule::AddFriend(const std::string& name)
{
    if (name.empty()) return;
    auto it = std::find(m_Friends.begin(), m_Friends.end(), name);
    if (it == m_Friends.end())
    {
        m_Friends.push_back(name);
        Logger::Log("Added friend: %s", name.c_str());
    }
}

void FriendsModule::RemoveFriend(const std::string& name)
{
    auto it = std::find(m_Friends.begin(), m_Friends.end(), name);
    if (it != m_Friends.end())
    {
        m_Friends.erase(it);
        Logger::Log("Removed friend: %s", name.c_str());
    }
}

bool FriendsModule::IsFriend(const std::string& name)
{
    return std::find(m_Friends.begin(), m_Friends.end(), name) != m_Friends.end();
}

const std::vector<std::string>& FriendsModule::GetFriends()
{
    return m_Friends;
}

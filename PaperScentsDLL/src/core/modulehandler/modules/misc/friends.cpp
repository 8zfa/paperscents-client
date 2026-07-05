#include "friends.h"
#include "../../../utilities/logger.h"
#include <algorithm>

std::vector<std::string> FriendsModule::m_Friends;

FriendsModule::FriendsModule()
    : ModuleBase("Friends", "Manage friends list", Category::Misc)
{
}

void FriendsModule::OnEnable() { Logger::Log("Friends enabled"); }
void FriendsModule::OnDisable() { Logger::Log("Friends disabled"); }
void FriendsModule::OnUpdate() {}

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

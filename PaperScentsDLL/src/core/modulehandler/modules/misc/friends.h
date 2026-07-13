#pragma once
#include "../../modulebase.h"
#include <vector>
#include <string>

class FriendsModule : public ModuleBase
{
public:
    FriendsModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

    static void AddFriend(const std::string& name);
    static void RemoveFriend(const std::string& name);
    static bool IsFriend(const std::string& name);
    static const std::vector<std::string>& GetFriends();

private:
    static std::vector<std::string> m_Friends;
    char m_InputBuf[128] = {};
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

#include "criticals.h"
#include "../../../java/java.h"
#include "../../../sdk/strayCache.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <cstdlib>

CriticalsModule::CriticalsModule()
    : ModuleBase("Criticals", "Always land critical hits", Category::Combat)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Packet", "MiniJump", "Position"});
    m_LastCrit = std::chrono::steady_clock::now();
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void CriticalsModule::OnEnable() { Logger::Log("Criticals enabled"); }
void CriticalsModule::OnDisable() { Logger::Log("Criticals disabled"); }

void CriticalsModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!BridgeHelper::Initialize(env)) return;

    jobject player = BridgeHelper::GetPlayer(env);
    if (!player) return;

    bool onGround = false;
    if (BridgeHelper::EntityBridge_IsOnGround)
        onGround = env->CallBooleanMethod(player, BridgeHelper::EntityBridge_IsOnGround) == JNI_TRUE;

    bool attacking = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    auto now = std::chrono::steady_clock::now();
    bool cooldown = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastCrit).count() < 200;

    if (onGround && attacking && !m_AppliedThisTick && !cooldown)
    {
        int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();
        if ((mode == 0 || mode == 2) && BridgeHelper::EntityBridge_GetPosY)
        {
            double py = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetPosY);
            // posY/onGround set via notch fallback (vanilla only; Lunar needs bridge setters)
            if (StrayCache::Entity_posY)
                env->SetDoubleField(player, StrayCache::Entity_posY, py + 0.0625);
            if (StrayCache::Entity_onGround)
                env->SetBooleanField(player, StrayCache::Entity_onGround, JNI_FALSE);
            if (mode == 0 && BridgeHelper::EntityBridge_SetMotionY)
                env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionY, -0.1);
        }
        else if (mode == 1)
        {
            if (BridgeHelper::EntityBridge_SetMotionY)
                env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionY, 0.2);
            if (StrayCache::Entity_onGround)
                env->SetBooleanField(player, StrayCache::Entity_onGround, JNI_FALSE);
        }
        m_AppliedThisTick = true;
    }
    else if (!onGround && m_AppliedThisTick)
    {
        m_LastCrit = now;
        m_AppliedThisTick = false;
    }
    else if (onGround)
    {
        m_AppliedThisTick = false;
    }
    env->DeleteLocalRef(player);
}

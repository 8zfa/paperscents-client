#include "autoblock.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../sdk/bridgeHelper.h"
#include <cstdlib>

AutoBlockModule::AutoBlockModule()
    : ModuleBase("AutoBlock", "Automatically block when attacking", Category::Combat)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Single", "Custom", "Always"});
    AddSetting<NumberSetting>("BlockChance", 80.0f, 0.0f, 100.0f, 5.0f);
    m_LastBlock = std::chrono::steady_clock::now();
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void AutoBlockModule::OnEnable() { Logger::Log("AutoBlock enabled"); }
void AutoBlockModule::OnDisable() { Logger::Log("AutoBlock disabled"); }

void AutoBlockModule::OnUpdate()
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

    jobject gs = BridgeHelper::GetGameSettings(env);
    if (!gs) return;

    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();
    float blockChance = ((NumberSetting*)FindSetting("BlockChance"))->GetValue();

    if (!BridgeHelper::GSBridge_KeyBindUseItem || !BridgeHelper::GSBridge_SetKeyBindState)
    { env->DeleteLocalRef(gs); return; }

    jobject kb = env->CallObjectMethod(gs, BridgeHelper::GSBridge_KeyBindUseItem);
    if (!kb) { env->DeleteLocalRef(gs); return; }

    bool attacking = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool shouldBlock = false;

    if (mode == 0) shouldBlock = attacking;
    else if (mode == 1) shouldBlock = attacking && ((::rand() % 100) < blockChance);
    else if (mode == 2) shouldBlock = true;

    auto now = std::chrono::steady_clock::now();
    bool cooldown = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastBlock).count() < 50;

    if (shouldBlock && !cooldown)
    {
        env->CallVoidMethod(gs, BridgeHelper::GSBridge_SetKeyBindState, kb, JNI_TRUE);
        m_LastBlock = now;
        m_WasBlocking = true;
    }
    else if (!shouldBlock && m_WasBlocking)
    {
        env->CallVoidMethod(gs, BridgeHelper::GSBridge_SetKeyBindState, kb, JNI_FALSE);
        m_WasBlocking = false;
    }

    env->DeleteLocalRef(kb);
    env->DeleteLocalRef(gs);
}

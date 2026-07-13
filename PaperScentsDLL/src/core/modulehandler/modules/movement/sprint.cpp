#include "sprint.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"

SprintModule::SprintModule()
    : ModuleBase("Sprint", "Automatically sprints", Category::Movement)
{
    AddSetting<BooleanSetting>("OmniSprint", false, "Sprint in all directions");
    AddSetting<BooleanSetting>("SprintInWater", false, "Sprint while in water");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void SprintModule::OnEnable() { Logger::Log("Sprint enabled"); }
void SprintModule::OnDisable() { Logger::Log("Sprint disabled"); }

void SprintModule::OnUpdate()
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

    if (BridgeHelper::EntityBridge_IsSneaking && env->CallBooleanMethod(player, BridgeHelper::EntityBridge_IsSneaking))
    { env->DeleteLocalRef(player); return; }

    if (BridgeHelper::ELBridge_IsUsingItem && env->CallBooleanMethod(player, BridgeHelper::ELBridge_IsUsingItem))
    { env->DeleteLocalRef(player); return; }

    bool water = ((BooleanSetting*)FindSetting("SprintInWater"))->GetValue();
    if (!water && BridgeHelper::ELBridge_IsInWater && env->CallBooleanMethod(player, BridgeHelper::ELBridge_IsInWater))
    { env->DeleteLocalRef(player); return; }

    if (BridgeHelper::EPSPBridge_SetSprinting)
        env->CallVoidMethod(player, BridgeHelper::EPSPBridge_SetSprinting, JNI_TRUE);

    env->DeleteLocalRef(player);
}

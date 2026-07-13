#include "keepsprint.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"

KeepSprintModule::KeepSprintModule()
    : ModuleBase("KeepSprint", "Keep sprinting when hitting", Category::Combat)
{
    AddSetting<BooleanSetting>("KeepSprint", true);
    AddSetting<NumberSetting>("Slowdown", 100.0f, 0.0f, 100.0f, 1.0f);
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void KeepSprintModule::OnEnable() { Logger::Log("KeepSprint enabled"); }
void KeepSprintModule::OnDisable() { Logger::Log("KeepSprint disabled"); }

void KeepSprintModule::OnUpdate()
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

    bool onGround = BridgeHelper::EntityBridge_IsOnGround
        ? env->CallBooleanMethod(player, BridgeHelper::EntityBridge_IsOnGround) == JNI_TRUE : false;
    if (!onGround) { env->DeleteLocalRef(player); return; }

    bool keepSprint = ((BooleanSetting*)FindSetting("KeepSprint"))->GetValue();
    float slowPct = ((NumberSetting*)FindSetting("Slowdown"))->GetValue() / 100.0f;

    int hurtTime = BridgeHelper::ELBridge_GetHurtTime
        ? env->CallIntMethod(player, BridgeHelper::ELBridge_GetHurtTime) : 0;
    bool justHit = hurtTime > 0;

    if (justHit)
    {
        if (keepSprint && BridgeHelper::EPSPBridge_SetSprinting)
            env->CallVoidMethod(player, BridgeHelper::EPSPBridge_SetSprinting, JNI_TRUE);

        if (slowPct < 1.0f && BridgeHelper::EntityBridge_GetMotionX && BridgeHelper::EntityBridge_SetMotionX &&
            BridgeHelper::EntityBridge_GetMotionZ && BridgeHelper::EntityBridge_SetMotionZ)
        {
            double mx = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionX);
            double mz = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionZ);
            env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionX, mx * slowPct);
            env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionZ, mz * slowPct);
        }
    }

    env->DeleteLocalRef(player);
}

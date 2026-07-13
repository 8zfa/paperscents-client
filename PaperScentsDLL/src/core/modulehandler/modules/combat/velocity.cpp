#include "velocity.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"

VelocityModule::VelocityModule()
    : ModuleBase("Velocity", "Modify knockback", Category::Combat)
{
    AddSetting<NumberSetting>("Horizontal", 0.0f, 0.0f, 100.0f, 1.0f);
    AddSetting<NumberSetting>("Vertical", 0.0f, 0.0f, 100.0f, 1.0f);
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void VelocityModule::OnEnable() { Logger::Log("Velocity enabled"); }
void VelocityModule::OnDisable() { Logger::Log("Velocity disabled"); }

void VelocityModule::OnUpdate()
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

    int hurtTime = BridgeHelper::ELBridge_GetHurtTime
        ? env->CallIntMethod(player, BridgeHelper::ELBridge_GetHurtTime) : 0;
    if (hurtTime <= 0) { env->DeleteLocalRef(player); return; }

    float hPct = ((NumberSetting*)FindSetting("Horizontal"))->GetValue() / 100.0f;
    float vPct = ((NumberSetting*)FindSetting("Vertical"))->GetValue() / 100.0f;

    if (BridgeHelper::EntityBridge_GetMotionX && BridgeHelper::EntityBridge_SetMotionX &&
        BridgeHelper::EntityBridge_GetMotionY && BridgeHelper::EntityBridge_SetMotionY &&
        BridgeHelper::EntityBridge_GetMotionZ && BridgeHelper::EntityBridge_SetMotionZ)
    {
        double mx = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionX);
        double my = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionY);
        double mz = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionZ);
        env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionX, mx * hPct);
        env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionY, my * vPct);
        env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionZ, mz * hPct);
    }

    env->DeleteLocalRef(player);
}

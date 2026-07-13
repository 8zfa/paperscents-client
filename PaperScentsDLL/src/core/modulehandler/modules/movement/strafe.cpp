#include "strafe.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <cmath>
#include <Windows.h>

StrafeModule::StrafeModule()
    : ModuleBase("Strafe", "Perfect movement strafing", Category::Movement)
{
    AddSetting<BooleanSetting>("AutoJump", true, "Automatically jump when strafing");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void StrafeModule::OnEnable() { Logger::Log("Strafe enabled"); }
void StrafeModule::OnDisable() { Logger::Log("Strafe disabled"); }

void StrafeModule::OnUpdate()
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

    bool autoJump = ((BooleanSetting*)FindSetting("AutoJump"))->GetValue();

    bool forward = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool backward = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool left = (GetAsyncKeyState('A') & 0x8000) != 0;
    bool right = (GetAsyncKeyState('D') & 0x8000) != 0;

    int forwardV = (forward ? 1 : 0) - (backward ? 1 : 0);
    int strafeV = (right ? 1 : 0) - (left ? 1 : 0);

    if (forwardV == 0 && strafeV == 0) { env->DeleteLocalRef(player); return; }

    bool onGround = BridgeHelper::EntityBridge_IsOnGround
        ? env->CallBooleanMethod(player, BridgeHelper::EntityBridge_IsOnGround) == JNI_TRUE : false;

    double mx = 0, mz = 0;
    if (BridgeHelper::EntityBridge_GetMotionX) mx = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionX);
    if (BridgeHelper::EntityBridge_GetMotionZ) mz = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionZ);
    double currentSpeed = std::sqrt(mx * mx + mz * mz);

    float yaw = 0;
    if (BridgeHelper::EntityBridge_GetRotationYaw) yaw = (float)env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetRotationYaw);
    float rad = yaw * 0.017453292f;
    double sin = std::sin(rad);
    double cos = std::cos(rad);

    double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
    double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
    double len = std::sqrt(targetX * targetX + targetZ * targetZ);

    if (len > 0.0)
    {
        targetX /= len;
        targetZ /= len;
        if (BridgeHelper::EntityBridge_SetMotionX) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionX, targetX * currentSpeed);
        if (BridgeHelper::EntityBridge_SetMotionZ) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionZ, targetZ * currentSpeed);
    }

    if (autoJump && onGround && BridgeHelper::EntityBridge_SetMotionY)
        env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionY, 0.42);

    env->DeleteLocalRef(player);
}

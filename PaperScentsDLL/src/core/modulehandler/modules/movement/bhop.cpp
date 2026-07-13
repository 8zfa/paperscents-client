#include "bhop.h"
#include "../../../java/java.h"
#include "../../../sdk/strayCache.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <cmath>
#include <Windows.h>

BHopModule::BHopModule()
    : ModuleBase("BHop", "Bunny hop for speed", Category::Movement)
{
    AddSetting<NumberSetting>("Speed", 1.0f, 0.1f, 5.0f, 0.1f, "Speed multiplier");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void BHopModule::OnEnable() { Logger::Log("BHop enabled"); }

void BHopModule::OnDisable()
{
    Logger::Log("BHop disabled");
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    // Restore timer speed via bridge helpers
    if (!BridgeHelper::Initialize(env)) return;
    jobject mc = BridgeHelper::GetMinecraftInstance(env);
    if (!mc) return;
    // Timer fallback via StrayCache (notch) — won't work on Lunar
    if (StrayCache::Minecraft)
    {
        jfieldID timerField = env->GetFieldID(StrayCache::Minecraft, "timer", "Lnet/minecraft/util/Timer;");
        if (!timerField) env->ExceptionClear();
        if (timerField)
        {
            jobject timerObj = env->GetObjectField(mc, timerField);
            if (timerObj)
            {
                jclass timerClass = env->FindClass("net/minecraft/util/Timer");
                if (timerClass)
                {
                    jfieldID timerSpeedField = env->GetFieldID(timerClass, "timerSpeed", "F");
                    if (timerSpeedField) { env->ExceptionClear(); env->SetFloatField(timerObj, timerSpeedField, 1.0f); }
                    env->DeleteLocalRef(timerClass);
                }
                env->DeleteLocalRef(timerObj);
            }
        }
    }
}

void BHopModule::OnUpdate()
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

    float speedMult = ((NumberSetting*)FindSetting("Speed"))->GetValue();

    bool onGround = BridgeHelper::EntityBridge_IsOnGround
        ? env->CallBooleanMethod(player, BridgeHelper::EntityBridge_IsOnGround) == JNI_TRUE : false;
    bool forward = (GetAsyncKeyState('W') & 0x8000) != 0;

    if (onGround && forward && BridgeHelper::EntityBridge_SetMotionY)
        env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionY, 0.42);

    double mx = BridgeHelper::EntityBridge_GetMotionX ? env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionX) : 0;
    double mz = BridgeHelper::EntityBridge_GetMotionZ ? env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionZ) : 0;
    double currentSpeed = std::sqrt(mx * mx + mz * mz);

    if (currentSpeed > 0.0 && BridgeHelper::EntityBridge_SetMotionX && BridgeHelper::EntityBridge_SetMotionZ)
    {
        double newSpeed = currentSpeed * speedMult;
        env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionX, (mx / currentSpeed) * newSpeed);
        env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionZ, (mz / currentSpeed) * newSpeed);
    }

    // Timer speed boost (vanilla only via StrayCache)
    if (StrayCache::Minecraft)
    {
        jobject mc = BridgeHelper::GetMinecraftInstance(env);
        if (mc)
        {
            jfieldID timerField = env->GetFieldID(StrayCache::Minecraft, "timer", "Lnet/minecraft/util/Timer;");
            if (!timerField) env->ExceptionClear();
            if (timerField)
            {
                jobject timerObj = env->GetObjectField(mc, timerField);
                if (timerObj)
                {
                    jclass timerClass = env->FindClass("net/minecraft/util/Timer");
                    if (timerClass)
                    {
                        jfieldID timerSpeedField = env->GetFieldID(timerClass, "timerSpeed", "F");
                        if (timerSpeedField)
                        {
                            env->ExceptionClear();
                            float boost = 1.0f + (speedMult - 1.0f) * 0.5f;
                            env->SetFloatField(timerObj, timerSpeedField, boost);
                        }
                        env->DeleteLocalRef(timerClass);
                    }
                    env->DeleteLocalRef(timerObj);
                }
            }
        }
    }

    env->DeleteLocalRef(player);
}

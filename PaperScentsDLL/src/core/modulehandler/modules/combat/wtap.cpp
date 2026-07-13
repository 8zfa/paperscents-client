#include "wtap.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../sdk/bridgeHelper.h"

WTapModule::WTapModule()
    : ModuleBase("WTap", "Press W after hitting to reset sprint", Category::Combat)
{
    AddSetting<NumberSetting>("Delay", 5.0f, 0.0f, 10.0f, 1.0f);
    AddSetting<NumberSetting>("Duration", 2.0f, 1.0f, 5.0f, 1.0f);
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void WTapModule::OnEnable() { Logger::Log("WTap enabled"); m_Active = false; m_PrevHurtTime = 0; }
void WTapModule::OnDisable() { Logger::Log("WTap disabled"); m_Active = false; m_DelayTicks = 0; m_DurationTicks = 0; m_PrevHurtTime = 0; }

void WTapModule::OnUpdate()
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

    if (m_DelayTicks > 0) m_DelayTicks--;
    if (m_DurationTicks > 0) m_DurationTicks--;

    if (m_Active)
    {
        if (m_DurationTicks > 0 && BridgeHelper::GSBridge_SetKeyBindState && BridgeHelper::GSBridge_KeyBindForward)
        {
            jobject gs = BridgeHelper::GetGameSettings(env);
            if (gs)
            {
                jobject kb = env->CallObjectMethod(gs, BridgeHelper::GSBridge_KeyBindForward);
                if (kb)
                {
                    env->CallVoidMethod(gs, BridgeHelper::GSBridge_SetKeyBindState, kb, JNI_FALSE);
                    env->DeleteLocalRef(kb);
                }
                env->DeleteLocalRef(gs);
            }
        }
        if (m_DurationTicks <= 0) m_Active = false;
        env->DeleteLocalRef(player);
        return;
    }

    int hurtTime = BridgeHelper::ELBridge_GetHurtTime
        ? env->CallIntMethod(player, BridgeHelper::ELBridge_GetHurtTime) : 0;
    bool attacked = (hurtTime > m_PrevHurtTime && hurtTime > 0);
    m_PrevHurtTime = hurtTime;
    if (!attacked) { env->DeleteLocalRef(player); return; }

    bool sprinting = false;
    if (BridgeHelper::EPSPBridge_GetSprintingTicksLeft)
        sprinting = env->CallIntMethod(player, BridgeHelper::EPSPBridge_GetSprintingTicksLeft) > 0;
    if (!sprinting) { env->DeleteLocalRef(player); return; }

    m_Active = true;
    m_DelayTicks = (int)((NumberSetting*)FindSetting("Delay"))->GetValue();
    m_DurationTicks = (int)((NumberSetting*)FindSetting("Duration"))->GetValue();
    env->DeleteLocalRef(player);
}

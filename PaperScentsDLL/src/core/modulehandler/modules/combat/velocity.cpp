#include "velocity.h"
#include "../../../utilities/logger.h"
#include "../../../core.h"
#include "../../../utilities/jni_helpers.h"
#include <jni.h>
#include <cmath>

VelocityModule::VelocityModule()
    : ModuleBase("Velocity", "Modify knockback", Category::Combat)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Reduce", "Jump Reset"});
    AddSetting<NumberSetting>("Horizontal", 0.0f, 0.0f, 100.0f, 1.0f);
    AddSetting<NumberSetting>("Vertical", 0.0f, 0.0f, 100.0f, 1.0f);
    AddSetting<BooleanSetting>("OnlyInAir", false);
    AddSetting<NumberSetting>("JumpChance", 80.0f, 0.0f, 100.0f, 1.0f);
    m_LastJump = std::chrono::steady_clock::now();
}

void VelocityModule::OnEnable() { Logger::Log("Velocity enabled"); }
void VelocityModule::OnDisable() { Logger::Log("Velocity disabled"); }

void VelocityModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;
    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    Java* java = Core::GetInstance().GetJava();
    jclass eCls = java->FindClass("pk", "net/minecraft/entity/Entity");
    if (!eCls) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jfieldID hrtFid = env->GetFieldID(eCls, "W", "I");
    if (!hrtFid) { env->ExceptionClear(); hrtFid = env->GetFieldID(eCls, "hurtResistantTime", "I"); }
    env->ExceptionClear();
    int hrt = hrtFid ? env->GetIntField(player, hrtFid) : 0;

    jfieldID mxFid = env->GetFieldID(eCls, "v", "D");
    if (!mxFid) { env->ExceptionClear(); mxFid = env->GetFieldID(eCls, "motionX", "D"); }
    jfieldID myFid = env->GetFieldID(eCls, "w", "D");
    if (!myFid) { env->ExceptionClear(); myFid = env->GetFieldID(eCls, "motionY", "D"); }
    jfieldID mzFid = env->GetFieldID(eCls, "x", "D");
    if (!mzFid) { env->ExceptionClear(); mzFid = env->GetFieldID(eCls, "motionZ", "D"); }
    env->ExceptionClear();

    jfieldID ogFid = env->GetFieldID(eCls, "C", "Z");
    if (!ogFid) { env->ExceptionClear(); ogFid = env->GetFieldID(eCls, "onGround", "Z"); }

    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();

    if (mode == 0)
    {
        float hPct = ((NumberSetting*)FindSetting("Horizontal"))->GetValue() / 100.0f;
        float vPct = ((NumberSetting*)FindSetting("Vertical"))->GetValue() / 100.0f;
        bool onlyAir = ((BooleanSetting*)FindSetting("OnlyInAir"))->GetValue();

        if (hPct >= 1.0f && vPct >= 1.0f) { env->DeleteLocalRef(eCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

        bool onGround = ogFid ? (env->GetBooleanField(player, ogFid) == JNI_TRUE) : false;
        if (onlyAir && onGround) { env->DeleteLocalRef(eCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }
        if (!mxFid || !myFid || !mzFid) { env->DeleteLocalRef(eCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

        double mX = env->GetDoubleField(player, mxFid);
        double mY = env->GetDoubleField(player, myFid);
        double mZ = env->GetDoubleField(player, mzFid);

        double hSpeed = std::sqrt(mX * mX + mZ * mZ);
        double vSpeed = fabs(mY);
        if (hSpeed > 0.35 && vSpeed > 0.32)
        {
            if (hPct < 1.0f) { env->SetDoubleField(player, mxFid, mX * hPct); env->SetDoubleField(player, mzFid, mZ * hPct); }
            if (vPct < 1.0f) env->SetDoubleField(player, myFid, mY * vPct);
        }
    }
    else if (mode == 1)
    {
        float chance = ((NumberSetting*)FindSetting("JumpChance"))->GetValue();
        bool onGround = ogFid ? (env->GetBooleanField(player, ogFid) == JNI_TRUE) : false;
        auto now = std::chrono::steady_clock::now();
        if (m_JumpCooldown > 0) m_JumpCooldown--;

        if (hrt == 10 && !onGround && m_JumpCooldown == 0)
        {
            if ((rand() % 100) < chance)
            {
                jclass playerCls = env->GetObjectClass(player);
                jmethodID jumpMid = env->GetMethodID(playerCls, "bI", "()V");
                if (!jumpMid) { env->ExceptionClear(); jumpMid = env->GetMethodID(playerCls, "jump", "()V"); }
                if (jumpMid) { env->CallVoidMethod(player, jumpMid); m_JumpCooldown = 5; }
                env->DeleteLocalRef(playerCls);
            }
        }
    }

    env->DeleteLocalRef(eCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc);
}

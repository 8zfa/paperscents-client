#include "velocity.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <jni.h>
#include <cmath>
#include <Windows.h>

static JNIEnv* GetEnv()
{
    auto* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return nullptr;
    return java->GetEnv();
}

static jobject GetMc()
{
    JNIEnv* env = GetEnv();
    if (!env) return nullptr;
    jclass cls = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!cls) { env->ExceptionClear(); cls = env->FindClass("net/minecraft/client/Minecraft"); }
    if (!cls) return nullptr;
    jmethodID get = env->GetStaticMethodID(cls, "A", "()Leave;");
    if (!get) { env->ExceptionClear(); get = env->GetStaticMethodID(cls, "getMinecraft", "()Lnet/minecraft/client/Minecraft;"); }
    if (!get) { env->DeleteLocalRef(cls); return nullptr; }
    env->ExceptionClear();
    jobject mc = env->CallStaticObjectMethod(cls, get);
    env->DeleteLocalRef(cls);
    return mc;
}

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

    JNIEnv* env = GetEnv();
    if (!env) return;

    jobject mc = GetMc();
    if (!mc) return;

    jclass mcCls = env->GetObjectClass(mc);
    if (!mcCls) { env->DeleteLocalRef(mc); return; }

    // Get player
    jfieldID pfid = env->GetFieldID(mcCls, "u", "Lbew;");
    if (!pfid) { env->ExceptionClear(); pfid = env->GetFieldID(mcCls, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;"); }
    jobject player = pfid ? env->GetObjectField(mc, pfid) : nullptr;
    if (!player) { env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    // Get hurt resistant time
    jclass eCls = Core::GetInstance().GetJava()->FindClass("pk", "net/minecraft/entity/Entity");
    if (!eCls) { env->ExceptionClear(); eCls = env->FindClass("net/minecraft/entity/Entity"); }
    if (!eCls) { env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
    jfieldID hrtFid = env->GetFieldID(eCls, "W", "I");
    if (!hrtFid) { env->ExceptionClear(); hrtFid = env->GetFieldID(eCls, "hurtResistantTime", "I"); }
    env->ExceptionClear();
    int hrt = 0;
    if (hrtFid) hrt = env->GetIntField(player, hrtFid);

    // Get motion
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

    if (mode == 0) // Reduce mode
    {
        float hPct = ((NumberSetting*)FindSetting("Horizontal"))->GetValue() / 100.0f;
        float vPct = ((NumberSetting*)FindSetting("Vertical"))->GetValue() / 100.0f;
        bool onlyAir = ((BooleanSetting*)FindSetting("OnlyInAir"))->GetValue();

        if (hPct >= 1.0f && vPct >= 1.0f) { env->DeleteLocalRef(eCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

        bool onGround = false;
        if (ogFid) onGround = env->GetBooleanField(player, ogFid) == JNI_TRUE;
        if (onlyAir && onGround) { env->DeleteLocalRef(eCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

        if (!mxFid || !myFid || !mzFid) { env->DeleteLocalRef(eCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

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
    else if (mode == 1) // Jump Reset mode
    {
        float chance = ((NumberSetting*)FindSetting("JumpChance"))->GetValue();
        bool onGround = false;
        if (ogFid) onGround = env->GetBooleanField(player, ogFid) == JNI_TRUE;

        auto now = std::chrono::steady_clock::now();
        if (m_JumpCooldown > 0) m_JumpCooldown--;

        // Detect knockback: hurtResistantTime == 10 (just hit) and not on ground
        if (hrt == 10 && !onGround && m_JumpCooldown == 0)
        {
            if ((rand() % 100) < chance)
            {
                // Jump to reset velocity
                jclass playerCls = env->GetObjectClass(player);
                jmethodID jumpMid = env->GetMethodID(playerCls, "bI", "()V");
                if (!jumpMid) { env->ExceptionClear(); jumpMid = env->GetMethodID(playerCls, "jump", "()V"); }
                env->ExceptionClear();
                if (jumpMid)
                {
                    env->CallVoidMethod(player, jumpMid);
                    m_LastJump = now;
                    m_JumpCooldown = 5; // prevent re-triggering
                }
                env->DeleteLocalRef(playerCls);
            }
        }
    }

    env->DeleteLocalRef(eCls);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mcCls);
    env->DeleteLocalRef(mc);
}

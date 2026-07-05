#include "wtap.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

static JNIEnv* GetEnv()
{
    auto* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return nullptr;
    return java->GetEnv();
}

static double GetDouble(JNIEnv* env, jobject obj, const char* obf, const char* deobf)
{
    jclass cls = env->GetObjectClass(obj);
    if (!cls) return 0.0;
    jfieldID fid = env->GetFieldID(cls, obf, "D");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(cls, deobf, "D"); }
    double v = 0.0;
    if (fid) v = env->GetDoubleField(obj, fid);
    env->DeleteLocalRef(cls);
    return v;
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

WTapModule::WTapModule()
    : ModuleBase("WTap", "Press W after hitting to reset sprint", Category::Combat)
{
    AddSetting<NumberSetting>("Chance", 100.0f, 0.0f, 100.0f, 1.0f);
    AddSetting<NumberSetting>("Range", 3.5f, 1.0f, 6.0f, 0.1f);
    AddSetting<NumberSetting>("TapMs", 80.0f, 10.0f, 300.0f, 5.0f);
    AddSetting<NumberSetting>("WaitMs", 30.0f, 10.0f, 300.0f, 5.0f);
    AddSetting<NumberSetting>("Hits", 1.0f, 1.0f, 10.0f, 1.0f);
    m_TargetHits = 1;
}

void WTapModule::OnEnable() { Logger::Log("WTap enabled"); }
void WTapModule::OnDisable() { Logger::Log("WTap disabled"); }

void WTapModule::OnUpdate()
{
    if (!IsEnabled()) return;
    JNIEnv* env = GetEnv();
    if (!env) return;
    env->ExceptionClear();

    jobject mc = GetMc();
    if (!mc) return;

    jclass mcCls = env->GetObjectClass(mc);
    if (!mcCls) { env->DeleteLocalRef(mc); return; }

    jfieldID pfid = env->GetFieldID(mcCls, "u", "Lbew;");
    if (!pfid) { env->ExceptionClear(); pfid = env->GetFieldID(mcCls, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;"); }
    jobject player = pfid ? env->GetObjectField(mc, pfid) : nullptr;
    if (!player) { env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    jfieldID wfid = env->GetFieldID(mcCls, "f", "Ladm;");
    if (!wfid) { env->ExceptionClear(); wfid = env->GetFieldID(mcCls, "theWorld", "Lnet/minecraft/world/World;"); }
    jobject world = wfid ? env->GetObjectField(mc, wfid) : nullptr;
    if (!world) { env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    // Detect attack via isSwingInProgress
    jclass eCls = Core::GetInstance().GetJava()->FindClass("pk", "net/minecraft/entity/Entity");
    if (!eCls) { env->ExceptionClear(); eCls = env->FindClass("net/minecraft/entity/Entity"); }
    if (eCls)
    {
        jfieldID swingFid = env->GetFieldID(eCls, "a", "Z");
        if (!swingFid) { env->ExceptionClear(); swingFid = env->GetFieldID(eCls, "isSwingInProgress", "Z"); }
        env->ExceptionClear();
        static bool prevSwing = false;
        bool swing = false;
        if (swingFid) swing = env->GetBooleanField(player, swingFid) == JNI_TRUE;

        if (swing && !prevSwing)
        {
            m_AttackedThisFrame = true;
        }
        prevSwing = swing;
    }

    float range = ((NumberSetting*)FindSetting("Range"))->GetValue();
    float chance = ((NumberSetting*)FindSetting("Chance"))->GetValue();
    long tapMs = (long)((NumberSetting*)FindSetting("TapMs"))->GetValue();
    long waitMs = (long)((NumberSetting*)FindSetting("WaitMs"))->GetValue();
    long hitsPer = (long)((NumberSetting*)FindSetting("Hits"))->GetValue();

    auto now = std::chrono::steady_clock::now();

    // Check state
    if (m_State == Waiting)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_Timer).count();
        if (elapsed >= m_WaitMs)
        {
            // Release W
            keybd_event('W', 0, KEYEVENTF_KEYUP, 0);
            m_State = Tapping;
            m_Timer = now;
            m_TapMs = tapMs;
        }
    }
    else if (m_State == Tapping)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_Timer).count();
        if (elapsed >= m_TapMs)
        {
            // Re-press W if it was held before
            if (m_WasForwardDown)
                keybd_event('W', 0, 0, 0);
            m_State = None;
        }
    }

    // If attacked this frame, maybe trigger WTap
    if (m_AttackedThisFrame && m_State == None)
    {
        m_AttackedThisFrame = false;
        m_Hits++;

        if (m_Hits >= m_TargetHits)
        {
            m_Hits = 0;
            // Randomize target hits for next time
            int hitsRange = (int)hitsPer;
            m_TargetHits = (int)hitsPer; // always at least hitsPer
            if (hitsRange > 1) m_TargetHits += rand() % (hitsRange);

            // Check chance
            if ((rand() % 100) >= chance) { env->DeleteLocalRef(eCls); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

            // Check range to nearest entity (approximate: check if objectMouseOver has entity)
            jfieldID moFid = env->GetFieldID(mcCls, "y", "Lbex;");
            if (!moFid) { env->ExceptionClear(); moFid = env->GetFieldID(mcCls, "objectMouseOver", "Lnet/minecraft/util/MovingObjectPosition;"); }
            env->ExceptionClear();
            if (moFid)
            {
                jobject mo = env->GetObjectField(mc, moFid);
                if (mo)
                {
                    jclass moCls = env->GetObjectClass(mo);
                    jfieldID entHitFid = env->GetFieldID(moCls, "d", "Lpk;");
                    if (!entHitFid) { env->ExceptionClear(); entHitFid = env->GetFieldID(moCls, "entityHit", "Lnet/minecraft/entity/Entity;"); }
                    env->ExceptionClear();
                    if (entHitFid)
                    {
                        jobject hitEnt = env->GetObjectField(mo, entHitFid);
                        if (hitEnt)
                        {
                            // Check if it's a player
                            jclass playerClass = Core::GetInstance().GetJava()->FindClass("pr", "net/minecraft/entity/EntityLivingBase");
                            if (!playerClass) { env->ExceptionClear(); playerClass = env->FindClass("net/minecraft/entity/EntityLivingBase"); }
                            env->ExceptionClear();
                            if (playerClass)
                            {
                                jboolean isPlayer = env->IsInstanceOf(hitEnt, playerClass);
                                if (isPlayer)
                                {
                                    // Calculate distance
                                    double hx = GetDouble(env, hitEnt, "r", "posX");
                                    double hy = GetDouble(env, hitEnt, "s", "posY");
                                    double hz = GetDouble(env, hitEnt, "t", "posZ");
                                    double px = GetDouble(env, player, "r", "posX");
                                    double py = GetDouble(env, player, "s", "posY");
                                    double pz = GetDouble(env, player, "t", "posZ");
                                    double dist = std::sqrt((hx-px)*(hx-px) + (hy-py)*(hy-py) + (hz-pz)*(hz-pz));
                                    if (dist <= range + 1.0)
                                    {
                                        m_WasForwardDown = (GetAsyncKeyState('W') & 0x8000) != 0;
                                        // Release W now
                                        keybd_event('W', 0, KEYEVENTF_KEYUP, 0);
                                        m_State = Waiting;
                                        m_Timer = now;
                                        m_WaitMs = waitMs;
                                    }
                                }
                                env->DeleteLocalRef(playerClass);
                            }
                            env->DeleteLocalRef(hitEnt);
                        }
                    }
                    env->DeleteLocalRef(moCls);
                    env->DeleteLocalRef(mo);
                }
            }
        }
    }
    else
    {
        m_AttackedThisFrame = false;
    }

    env->DeleteLocalRef(eCls);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mcCls);
    env->DeleteLocalRef(mc);
}

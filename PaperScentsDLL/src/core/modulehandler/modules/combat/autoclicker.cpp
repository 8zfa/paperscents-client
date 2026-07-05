#define NOMINMAX
#include "autoclicker.h"
#include "../../../utilities/logger.h"
#include "../../../core.h"
#include "../../../rendering/menu/menu.h"
#include "../../../utilities/jni_helpers.h"
#include <Windows.h>
#include <cstdlib>
#include <algorithm>

extern HWND GetGameWindowHandle();

AutoClickerModule::AutoClickerModule()
    : ModuleBase("AutoClicker", "Automatically clicks when holding", Category::Combat)
{
    AddSetting<BooleanSetting>("LeftClick", true, "Auto-click left mouse");
    AddSetting<NumberSetting>("LeftMinCPS", 7.0f, 1.0f, 20.0f, 1.0f);
    AddSetting<NumberSetting>("LeftMaxCPS", 12.0f, 1.0f, 20.0f, 1.0f);
    AddSetting<BooleanSetting>("BreakBlocks", false, "Only left-click when targeting a block");
    AddSetting<BooleanSetting>("RightClick", false, "Auto-click right mouse");
    AddSetting<NumberSetting>("RightMinCPS", 8.0f, 1.0f, 20.0f, 1.0f);
    AddSetting<NumberSetting>("RightMaxCPS", 11.0f, 1.0f, 20.0f, 1.0f);
    AddSetting<BooleanSetting>("Randomize", true, "Randomize CPS between min and max");
    m_LastLClick = std::chrono::steady_clock::now();
    m_LastRClick = std::chrono::steady_clock::now();
}

void AutoClickerModule::OnEnable() { Logger::Log("AutoClicker enabled"); }
void AutoClickerModule::OnDisable() { Logger::Log("AutoClicker disabled"); }

static void SwingItem(JNIEnv* env, jobject player)
{
    if (!env || !player) return;
    jclass playerCls = env->GetObjectClass(player);
    jmethodID swing = env->GetMethodID(playerCls, "a", "()V");
    if (!swing) { env->ExceptionClear(); swing = env->GetMethodID(playerCls, "swingItem", "()V"); }
    if (swing) { env->ExceptionClear(); env->CallVoidMethod(player, swing); }
    env->DeleteLocalRef(playerCls);
}

static bool IsLookingAtBlock(JNIEnv* env)
{
    jobject mc = GetMinecraftObject(env);
    if (!mc) return false;
    jclass mcObjCls = env->GetObjectClass(mc);
    if (!mcObjCls) { env->DeleteLocalRef(mc); return false; }

    const char* omoFields[] = { "e", "field_71476_x", "objectMouseOver" };
    jfieldID omoFid = nullptr;
    for (auto f : omoFields)
    {
        omoFid = env->GetFieldID(mcObjCls, f, "Lazk;");
        if (!omoFid) { env->ExceptionClear(); omoFid = env->GetFieldID(mcObjCls, f, "Lnet/minecraft/util/MovingObjectPosition;"); }
        if (omoFid) break;
    }
    if (!omoFid) { env->DeleteLocalRef(mcObjCls); env->DeleteLocalRef(mc); return false; }
    jobject mop = env->GetObjectField(mc, omoFid);
    env->DeleteLocalRef(mcObjCls);
    env->DeleteLocalRef(mc);
    if (!mop) return false;

    jclass mopCls = env->GetObjectClass(mop);
    const char* typeFields[] = { "a", "field_78430_b", "typeOfHit" };
    jfieldID typeFid = nullptr;
    for (auto f : typeFields)
    {
        typeFid = env->GetFieldID(mopCls, f, "Lazl;");
        if (!typeFid) { env->ExceptionClear(); typeFid = env->GetFieldID(mopCls, f, "Lnet/minecraft/util/MovingObjectPosition$MovingObjectType;"); }
        if (typeFid) break;
    }
    if (!typeFid) { env->DeleteLocalRef(mopCls); env->DeleteLocalRef(mop); return false; }
    jobject typeObj = env->GetObjectField(mop, typeFid);
    env->DeleteLocalRef(mopCls);
    env->DeleteLocalRef(mop);
    if (!typeObj) return false;

    jclass enumCls = env->GetObjectClass(typeObj);
    jmethodID ordinal = env->GetMethodID(enumCls, "ordinal", "()I");
    jint typeOrdinal = ordinal ? env->CallIntMethod(typeObj, ordinal) : -1;
    env->DeleteLocalRef(enumCls);
    env->DeleteLocalRef(typeObj);

    return typeOrdinal == 1;
}

static void JniClickLeft(JNIEnv* env, jobject mc)
{
    if (!env || !mc) return;
    jclass mcCls = env->GetObjectClass(mc);
    if (!mcCls) return;
    jmethodID clickMouse = env->GetMethodID(mcCls, "c", "()V");
    if (!clickMouse) { env->ExceptionClear(); clickMouse = env->GetMethodID(mcCls, "clickMouse", "()V"); }
    if (clickMouse) { env->ExceptionClear(); env->CallVoidMethod(mc, clickMouse); }
    env->DeleteLocalRef(mcCls);
}

static void JniClickRight(JNIEnv* env, jobject mc)
{
    if (!env || !mc) return;
    jclass mcCls = env->GetObjectClass(mc);
    if (!mcCls) return;
    jmethodID rightClick = env->GetMethodID(mcCls, "d", "()V");
    if (!rightClick) { env->ExceptionClear(); rightClick = env->GetMethodID(mcCls, "rightClickMouse", "()V"); }
    if (rightClick) { env->ExceptionClear(); env->CallVoidMethod(mc, rightClick); }
    env->DeleteLocalRef(mcCls);
}

void AutoClickerModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (Menu::GetInstance().IsOpen()) return;

    auto now = std::chrono::steady_clock::now();
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    auto lc = (BooleanSetting*)FindSetting("LeftClick");
    auto rc = (BooleanSetting*)FindSetting("RightClick");
    auto randomize = (BooleanSetting*)FindSetting("Randomize");
    auto breakBlocks = (BooleanSetting*)FindSetting("BreakBlocks");

    int lMin = (int)((NumberSetting*)FindSetting("LeftMinCPS"))->GetValue();
    int lMax = (int)((NumberSetting*)FindSetting("LeftMaxCPS"))->GetValue();
    int rMin = (int)((NumberSetting*)FindSetting("RightMinCPS"))->GetValue();
    int rMax = (int)((NumberSetting*)FindSetting("RightMaxCPS"))->GetValue();

    if (lc && lc->GetValue() && (GetAsyncKeyState(VK_LBUTTON) & 0x8000))
    {
        if (breakBlocks && breakBlocks->GetValue())
        {
            if (!IsLookingAtBlock(env)) { m_LastLClick = now; return; }
        }
        int cps = randomize && randomize->GetValue() ? (::rand() % (lMax - lMin + 1) + lMin) : (lMin + lMax) / 2;
        int ms = 1000 / std::max(1, cps);
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastLClick).count() >= ms)
        {
            // Try JNI click first, fall back to Win32 messages
            jobject mc = GetMinecraftObject(env);
            if (mc)
            {
                jobject player = GetPlayerObject(env, mc);
                JniClickLeft(env, mc);
                if (player) { SwingItem(env, player); env->DeleteLocalRef(player); }
                env->DeleteLocalRef(mc);
            }
            m_LastLClick = now;
        }
    }

    if (rc && rc->GetValue() && (GetAsyncKeyState(VK_RBUTTON) & 0x8000))
    {
        int cps = randomize && randomize->GetValue() ? (::rand() % (rMax - rMin + 1) + rMin) : (rMin + rMax) / 2;
        int ms = 1000 / std::max(1, cps);
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastRClick).count() >= ms)
        {
            jobject mc = GetMinecraftObject(env);
            if (mc)
            {
                jobject player = GetPlayerObject(env, mc);
                JniClickRight(env, mc);
                if (player) { SwingItem(env, player); env->DeleteLocalRef(player); }
                env->DeleteLocalRef(mc);
            }
            m_LastRClick = now;
        }
    }
}

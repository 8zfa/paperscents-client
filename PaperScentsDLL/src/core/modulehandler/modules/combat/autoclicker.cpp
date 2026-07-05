#define NOMINMAX
#include "autoclicker.h"
#include "../../../utilities/logger.h"
#include "../../../core.h"
#include "../../../rendering/menu/menu.h"
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

static bool IsLookingAtBlock(JNIEnv* env)
{
    Java* java = Core::GetInstance().GetJava();
    if (!java) return false;
    jclass mcCls = java->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcCls) return false;
    jmethodID getMc = env->GetStaticMethodID(mcCls, "A", "()Lave;");
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcCls, "getMinecraft", "()Lnet/minecraft/client/Minecraft;"); }
    if (!getMc) { env->DeleteLocalRef(mcCls); return false; }
    env->ExceptionClear();
    jobject mc = env->CallStaticObjectMethod(mcCls, getMc);
    env->DeleteLocalRef(mcCls);
    if (!mc) return false;

    jclass mcObjCls = env->GetObjectClass(mc);
    const char* omoFields[] = { "e", "field_71476_x", "objectMouseOver" };
    jfieldID omoFid = nullptr;
    jclass mopCls = java->FindClass("azk", "net/minecraft/util/MovingObjectPosition");
    for (auto f : omoFields)
    {
        if (mopCls)
        {
            char sig[64]; snprintf(sig, sizeof(sig), "L%s;", "azk");
            omoFid = env->GetFieldID(mcObjCls, f, sig);
            if (!omoFid) { env->ExceptionClear(); omoFid = env->GetFieldID(mcObjCls, f, "Lnet/minecraft/util/MovingObjectPosition;"); }
        }
        else
        {
            omoFid = env->GetFieldID(mcObjCls, f, "Ljava/lang/Object;");
        }
        if (omoFid) break;
        env->ExceptionClear();
    }
    env->ExceptionClear();
    jobject mop = omoFid ? env->GetObjectField(mc, omoFid) : nullptr;
    env->DeleteLocalRef(mcObjCls);
    env->DeleteLocalRef(mc);
    if (!mop) return false;

    if (!mopCls) mopCls = env->GetObjectClass(mop);
    const char* typeFields[] = { "a", "field_78430_b", "typeOfHit" };
    jfieldID typeFid = nullptr;
    jclass typeEnumCls = java->FindClass("azl", "net/minecraft/util/MovingObjectPosition$MovingObjectType");
    for (auto f : typeFields)
    {
        if (typeEnumCls)
        {
            char sig[64]; snprintf(sig, sizeof(sig), "L%s;", "azl");
            typeFid = env->GetFieldID(mopCls, f, sig);
            if (!typeFid) { env->ExceptionClear(); typeFid = env->GetFieldID(mopCls, f, "Lnet/minecraft/util/MovingObjectPosition$MovingObjectType;"); }
        }
        else
        {
            typeFid = env->GetFieldID(mopCls, f, "Ljava/lang/Enum;");
        }
        if (typeFid) break;
        env->ExceptionClear();
    }
    env->ExceptionClear();
    jobject typeObj = typeFid ? env->GetObjectField(mop, typeFid) : nullptr;
    if (mopCls != java->FindClass("azk", "net/minecraft/util/MovingObjectPosition")) env->DeleteLocalRef(mopCls);
    env->DeleteLocalRef(mop);
    if (!typeObj) return false;

    jclass enumCls = env->GetObjectClass(typeObj);
    jmethodID ordinal = env->GetMethodID(enumCls, "ordinal", "()I");
    env->ExceptionClear();
    jint typeOrdinal = ordinal ? env->CallIntMethod(typeObj, ordinal) : -1;
    env->DeleteLocalRef(enumCls);
    env->DeleteLocalRef(typeObj);

    return typeOrdinal == 1;
}

void AutoClickerModule::Click(int btn)
{
    HWND hwnd = GetGameWindowHandle();
    if (!hwnd) return;
    POINT pt;
    GetCursorPos(&pt);
    WPARAM wp = btn == 0 ? MK_LBUTTON : MK_RBUTTON;
    UINT downMsg = btn == 0 ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
    UINT upMsg = btn == 0 ? WM_LBUTTONUP : WM_RBUTTONUP;
    SendMessage(hwnd, downMsg, wp, MAKELPARAM(pt.x, pt.y));
    SendMessage(hwnd, upMsg, 0, MAKELPARAM(pt.x, pt.y));
}

void AutoClickerModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (Menu::GetInstance().IsOpen()) return;

    auto now = std::chrono::steady_clock::now();

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
            JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
            if (!env || !IsLookingAtBlock(env)) { m_LastLClick = now; return; }
        }
        int cps = randomize && randomize->GetValue() ? (::rand() % (lMax - lMin + 1) + lMin) : (lMin + lMax) / 2;
        int ms = 1000 / std::max(1, cps);
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastLClick).count() >= ms)
        {
            Click(0);
            m_LastLClick = now;
        }
    }

    if (rc && rc->GetValue() && (GetAsyncKeyState(VK_RBUTTON) & 0x8000))
    {
        int cps = randomize && randomize->GetValue() ? (::rand() % (rMax - rMin + 1) + rMin) : (rMin + rMax) / 2;
        int ms = 1000 / std::max(1, cps);
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastRClick).count() >= ms)
        {
            Click(1);
            m_LastRClick = now;
        }
    }
}

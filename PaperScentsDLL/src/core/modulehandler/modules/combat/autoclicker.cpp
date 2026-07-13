#define NOMINMAX
#include "autoclicker.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include "../../../rendering/menu/menu.h"
#include <Windows.h>
#include <cstdlib>
#include <algorithm>
#include <random>

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
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void AutoClickerModule::OnEnable() { Logger::Log("AutoClicker enabled"); }
void AutoClickerModule::OnDisable() { Logger::Log("AutoClicker disabled"); }

static void Win32Click(HWND hwnd, bool right)
{
    if (!hwnd) return;
    UINT msgDown = right ? WM_RBUTTONDOWN : WM_LBUTTONDOWN;
    UINT msgUp   = right ? WM_RBUTTONUP   : WM_LBUTTONUP;
    WPARAM wDown = right ? MK_RBUTTON     : MK_LBUTTON;
    PostMessage(hwnd, msgDown, wDown, MAKELPARAM(0, 0));
    Sleep(1);
    PostMessage(hwnd, msgUp, 0, MAKELPARAM(0, 0));
}

static bool IsLeftMouseHeld() { return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0; }
static bool IsRightMouseHeld() { return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0; }

void AutoClickerModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }
    if (Menu::GetInstance().IsOpen()) return;

    auto now = std::chrono::steady_clock::now();

    auto lc = (BooleanSetting*)FindSetting("LeftClick");
    auto rc = (BooleanSetting*)FindSetting("RightClick");
    auto randomize = (BooleanSetting*)FindSetting("Randomize");
    auto breakBlocks = (BooleanSetting*)FindSetting("BreakBlocks");
    auto lMinS = (NumberSetting*)FindSetting("LeftMinCPS");
    auto lMaxS = (NumberSetting*)FindSetting("LeftMaxCPS");
    auto rMinS = (NumberSetting*)FindSetting("RightMinCPS");
    auto rMaxS = (NumberSetting*)FindSetting("RightMaxCPS");
    if (!lc || !rc || !randomize || !breakBlocks || !lMinS || !lMaxS || !rMinS || !rMaxS) return;

    int lMin = (int)lMinS->GetValue(), lMax = (int)lMaxS->GetValue();
    int rMin = (int)rMinS->GetValue(), rMax = (int)rMaxS->GetValue();

    HWND hwnd = GetGameWindowHandle();
    if (!hwnd) return;

    if (lc->GetValue() && IsLeftMouseHeld())
    {
        if (breakBlocks->GetValue())
        {
            JNIEnv* env = Java::GetThreadEnv();
            if (env)
            {
                env->ExceptionClear();
                bool bridged = BridgeHelper::Initialize(env);
                jobject mc = bridged ? BridgeHelper::GetMinecraftInstance(env) : nullptr;
                if (!mc)
                {
                    Java* java = Core::GetInstance().GetJava();
                    if (java && java->IsValid())
                    {
                        jclass mcCls = java->FindClass("ave", "net/minecraft/client/Minecraft");
                        if (mcCls)
                        {
                            jmethodID getMc = env->GetStaticMethodID(mcCls, "A", "()Lave;");
                            if (getMc) mc = env->CallStaticObjectMethod(mcCls, getMc);
                            env->DeleteLocalRef(mcCls);
                        }
                    }
                }
                if (mc)
                {
                    jclass mcObjCls = env->GetObjectClass(mc);
                    jfieldID omoFid = env->GetFieldID(mcObjCls, "e", "Lazk;");
                    if (!omoFid) { env->ExceptionClear(); omoFid = env->GetFieldID(mcObjCls, "objectMouseOver", "Lnet/minecraft/util/MovingObjectPosition;"); }
                    if (omoFid)
                    {
                        env->ExceptionClear();
                        jobject mop = env->GetObjectField(mc, omoFid);
                        if (mop)
                        {
                            jclass mopCls = env->GetObjectClass(mop);
                            jfieldID typeFid = env->GetFieldID(mopCls, "a", "Lazl;");
                            if (!typeFid) { env->ExceptionClear(); typeFid = env->GetFieldID(mopCls, "typeOfHit", "Lnet/minecraft/util/MovingObjectPosition$MovingObjectType;"); }
                            if (typeFid)
                            {
                                env->ExceptionClear();
                                jobject typeObj = env->GetObjectField(mop, typeFid);
                                if (typeObj)
                                {
                                    jclass enumCls = env->GetObjectClass(typeObj);
                                    jmethodID ordinal = env->GetMethodID(enumCls, "ordinal", "()I");
                                    if (ordinal)
                                    {
                                        env->ExceptionClear();
                                        jint typeOrdinal = env->CallIntMethod(typeObj, ordinal);
                                        if (typeOrdinal != 1) { m_LastLClick = now; env->DeleteLocalRef(enumCls); env->DeleteLocalRef(typeObj); env->DeleteLocalRef(mopCls); env->DeleteLocalRef(mop); env->DeleteLocalRef(mcObjCls); env->DeleteLocalRef(mc); return; }
                                    }
                                    env->DeleteLocalRef(enumCls);
                                }
                                env->DeleteLocalRef(typeObj);
                            }
                            env->DeleteLocalRef(mopCls);
                            env->DeleteLocalRef(mop);
                        }
                    }
                    env->DeleteLocalRef(mcObjCls);
                    env->DeleteLocalRef(mc);
                }
                env->ExceptionClear();
            }
        }
        int cps = randomize->GetValue() ? (::rand() % (lMax - lMin + 1) + lMin) : (lMin + lMax) / 2;
        int ms = 1000 / std::max(1, cps);
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastLClick).count() >= ms)
        { Win32Click(hwnd, false); m_LastLClick = now; }
    }

    if (rc->GetValue() && IsRightMouseHeld())
    {
        int cps = randomize->GetValue() ? (::rand() % (rMax - rMin + 1) + rMin) : (rMin + rMax) / 2;
        int ms = 1000 / std::max(1, cps);
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastRClick).count() >= ms)
        { Win32Click(hwnd, true); m_LastRClick = now; }
    }
}

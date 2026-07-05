#include "scaffold.h"
#include "../../../core.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../utilities/logger.h"
#include "../../../rendering/menu/menu.h"
#include <cmath>
#include <cstdlib>
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

static void SetFloat(JNIEnv* env, jobject obj, const char* obf, const char* deobf, float val)
{
    jclass cls = env->GetObjectClass(obj);
    if (!cls) return;
    jfieldID fid = env->GetFieldID(cls, obf, "F");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(cls, deobf, "F"); }
    if (fid) env->SetFloatField(obj, fid, val);
    env->DeleteLocalRef(cls);
}

ScaffoldModule::ScaffoldModule()
    : ModuleBase("Scaffold", "Auto-place blocks under you", Category::Player)
{
    AddSetting<BooleanSetting>("Tower", true);
    AddSetting<NumberSetting>("Delay", 3.0f, 0.0f, 10.0f, 1.0f);
    AddSetting<BooleanSetting>("Swing", true);
    m_LastPlace = std::chrono::steady_clock::now();
}

void ScaffoldModule::OnEnable() { Logger::Log("Scaffold enabled"); }
void ScaffoldModule::OnDisable() { Logger::Log("Scaffold disabled"); }

void ScaffoldModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (Menu::GetInstance().IsOpen()) return;
    JNIEnv* env = GetEnv();
    if (!env) return;
    env->ExceptionClear();

    Java* java = Core::GetInstance().GetJava();

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jobject world = GetWorldObject(env, mc);
    if (!world) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    double pX = GetDouble(env, player, "r", "posX");
    double pY = GetDouble(env, player, "s", "posY");
    double pZ = GetDouble(env, player, "t", "posZ");

    // Get inventory
    jclass playerCls = env->GetObjectClass(player);
    jfieldID invFid = env->GetFieldID(playerCls, "e", "Lzo;");
    if (!invFid) { env->ExceptionClear(); invFid = env->GetFieldID(playerCls, "inventory", "Lnet/minecraft/entity/player/InventoryPlayer;"); }
    env->ExceptionClear();
    jobject inventory = invFid ? env->GetObjectField(player, invFid) : nullptr;
    if (!inventory) { env->DeleteLocalRef(playerCls); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jclass invCls = env->GetObjectClass(inventory);
    jfieldID ciFid = env->GetFieldID(invCls, "d", "I");
    if (!ciFid) { env->ExceptionClear(); ciFid = env->GetFieldID(invCls, "currentItem", "I"); }
    env->ExceptionClear();
    int currentSlot = 0;
    if (ciFid) currentSlot = env->GetIntField(inventory, ciFid);

    // Find a block in hotbar
    int blockSlot = -1;
    jmethodID getStack = env->GetMethodID(invCls, "a", "(I)Lzx;");
    if (!getStack) { env->ExceptionClear(); getStack = env->GetMethodID(invCls, "getStackInSlot", "(I)Lnet/minecraft/item/ItemStack;"); }
    env->ExceptionClear();

    jclass itemStackCls = nullptr;
    jmethodID getItem = nullptr;
    jclass itemBlockCls = nullptr;

    if (getStack)
    {
        itemStackCls = java->FindClass("zx", "net/minecraft/item/ItemStack");
        if (itemStackCls) { getItem = env->GetMethodID(itemStackCls, "a", "()Lxi;"); if (!getItem) { env->ExceptionClear(); getItem = env->GetMethodID(itemStackCls, "getItem", "()Lnet/minecraft/item/Item;"); } }
        env->ExceptionClear();
        itemBlockCls = java->FindClass("adz", "net/minecraft/item/ItemBlock");

        for (int i = 36; i <= 44; i++)
        {
            jobject stack = env->CallObjectMethod(inventory, getStack, i);
            if (stack)
            {
                if (getItem && itemBlockCls)
                {
                    jobject item = env->CallObjectMethod(stack, getItem);
                    if (item)
                    {
                        if (env->IsInstanceOf(item, itemBlockCls))
                        {
                            blockSlot = i - 36;
                            env->DeleteLocalRef(item);
                            env->DeleteLocalRef(stack);
                            break;
                        }
                        env->DeleteLocalRef(item);
                    }
                }
                env->DeleteLocalRef(stack);
            }
        }
    }

    if (blockSlot == -1) { env->DeleteLocalRef(invCls); env->DeleteLocalRef(inventory); env->DeleteLocalRef(playerCls); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    // Tower check
    bool tower = ((BooleanSetting*)FindSetting("Tower"))->GetValue();
    jfieldID jumpFid = env->GetFieldID(env->GetObjectClass(mc), "Y", "Lauj;");
    if (!jumpFid) { env->ExceptionClear(); jumpFid = env->GetFieldID(env->GetObjectClass(mc), "gameSettings", "Lnet/minecraft/client/settings/GameSettings;"); }
    env->ExceptionClear();
    bool jumpDown = false;
    if (jumpFid)
    {
        jobject gs = env->GetObjectField(mc, jumpFid);
        if (gs)
        {
            jclass gsCls = env->GetObjectClass(gs);
            jfieldID keyJump = env->GetFieldID(gsCls, "d", "Laui;");
            if (!keyJump) { env->ExceptionClear(); keyJump = env->GetFieldID(gsCls, "keyBindJump", "Lnet/minecraft/client/settings/KeyBinding;"); }
            env->ExceptionClear();
            if (keyJump)
            {
                jobject kb = env->GetObjectField(gs, keyJump);
                if (kb)
                {
                    jclass kbCls = env->GetObjectClass(kb);
                    jfieldID pressedFid = env->GetFieldID(kbCls, "e", "Z");
                    if (!pressedFid) { env->ExceptionClear(); pressedFid = env->GetFieldID(kbCls, "pressed", "Z"); }
                    env->ExceptionClear();
                    if (pressedFid) jumpDown = env->GetBooleanField(kb, pressedFid) == JNI_TRUE;
                    env->DeleteLocalRef(kbCls);
                    env->DeleteLocalRef(kb);
                }
            }
            env->DeleteLocalRef(gsCls);
            env->DeleteLocalRef(gs);
        }
    }

    m_Towering = tower && jumpDown;

    // Calculate block position under us
    int bX = (int)floor(pX);
    int bY = (int)floor(m_Towering ? pY : pY - 0.5);
    int bZ = (int)floor(pZ);

    double fracX = pX - floor(pX);
    double fracZ = pZ - floor(pZ);
    if (fracX < 0.3) bX--;
    if (fracX > 0.7) bX++;
    if (fracZ < 0.3) bZ--;
    if (fracZ > 0.7) bZ++;

    // Set hotbar slot
    if (ciFid) env->SetIntField(inventory, ciFid, blockSlot);

    // Tick delay
    long delay = (long)((NumberSetting*)FindSetting("Delay"))->GetValue();
    m_PlaceTicks++;
    if (m_PlaceTicks < delay) { env->DeleteLocalRef(invCls); env->DeleteLocalRef(inventory); env->DeleteLocalRef(playerCls); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }
    m_PlaceTicks = 0;

    // Set rotation straight down
    SetFloat(env, player, "y", "rotationYaw", 0.0f);
    SetFloat(env, player, "z", "rotationPitch", 90.0f);

    // Send right-click via SendMessage
    HWND hwnd = nullptr;
    {
        HMODULE mod = GetModuleHandleA("PaperScentsDLL.dll");
        if (mod)
        {
            HWND(*getHWND)() = (HWND(*)())GetProcAddress(mod, "GetGameWindowHandle");
            if (getHWND) hwnd = getHWND();
        }
    }

    if (hwnd)
    {
        POINT pt;
        GetCursorPos(&pt);
        SendMessage(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(pt.x, pt.y));
        SendMessage(hwnd, WM_RBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
        if (((BooleanSetting*)FindSetting("Swing"))->GetValue())
            SendMessage(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(pt.x, pt.y));
    }

    // Tower: set motionY
    if (m_Towering && pY - floor(pY) < 0.7)
    {
        jclass eCls = java->FindClass("pk", "net/minecraft/entity/Entity");
        if (eCls)
        {
            jfieldID myFid = env->GetFieldID(eCls, "w", "D");
            if (!myFid) { env->ExceptionClear(); myFid = env->GetFieldID(eCls, "motionY", "D"); }
            if (myFid) env->SetDoubleField(player, myFid, 0.42);
            env->DeleteLocalRef(eCls);
        }
    }

    env->DeleteLocalRef(invCls);
    env->DeleteLocalRef(inventory);
    env->DeleteLocalRef(playerCls);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

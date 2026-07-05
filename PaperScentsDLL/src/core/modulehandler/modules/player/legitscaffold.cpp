#define NOMINMAX
#include "legitscaffold.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../rendering/menu/menu.h"
#include <cmath>
#include <algorithm>
#include <Windows.h>

extern HWND GetGameWindowHandle();

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
    Java* java = Core::GetInstance().GetJava();
    jclass cls = java->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!cls) return nullptr;
    jmethodID get = env->GetStaticMethodID(cls, "A", "()Lave;");
    if (!get) { env->ExceptionClear(); get = env->GetStaticMethodID(cls, "getMinecraft", "()Lnet/minecraft/client/Minecraft;"); }
    if (!get) { env->DeleteLocalRef(cls); return nullptr; }
    env->ExceptionClear();
    jobject mc = env->CallStaticObjectMethod(cls, get);
    env->DeleteLocalRef(cls);
    return mc;
}

static double GetDouble(JNIEnv* env, jobject obj, const char* obf, const char* deobf)
{
    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(cls, obf, "D");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(cls, deobf, "D"); }
    env->ExceptionClear();
    double val = fid ? env->GetDoubleField(obj, fid) : 0.0;
    env->DeleteLocalRef(cls);
    return val;
}

static float GetFloat(JNIEnv* env, jobject obj, const char* obf, const char* deobf)
{
    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(cls, obf, "F");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(cls, deobf, "F"); }
    env->ExceptionClear();
    float val = fid ? env->GetFloatField(obj, fid) : 0.0f;
    env->DeleteLocalRef(cls);
    return val;
}

static void SetFloat(JNIEnv* env, jobject obj, const char* obf, const char* deobf, float val)
{
    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(cls, obf, "F");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(cls, deobf, "F"); }
    if (fid) env->SetFloatField(obj, fid, val);
    env->DeleteLocalRef(cls);
}

LegitScaffoldModule::LegitScaffoldModule()
    : ModuleBase("LegitScaffold", "Legit auto block placement", Category::Player)
{
    AddSetting<NumberSetting>("PlaceDelay", 2.0f, 1.0f, 20.0f, 1.0f);
    AddSetting<BooleanSetting>("AutoJump", true);
    AddSetting<BooleanSetting>("SmoothRotation", false);
    AddSetting<NumberSetting>("RotationSpeed", 1.0f, 0.1f, 5.0f, 0.1f);
    AddSetting<BooleanSetting>("OnlyWhileSneaking", false);
    AddSetting<BooleanSetting>("TowerMode", false);
    m_LastPlace = std::chrono::steady_clock::now();
}

void LegitScaffoldModule::OnEnable() { Logger::Log("LegitScaffold enabled"); }
void LegitScaffoldModule::OnDisable() { Logger::Log("LegitScaffold disabled"); }

void LegitScaffoldModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (Menu::GetInstance().IsOpen()) return;

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
    jobject wObj = wfid ? env->GetObjectField(mc, wfid) : nullptr;
    if (!wObj) { env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    double pX = GetDouble(env, player, "r", "posX");
    double pY = GetDouble(env, player, "s", "posY");
    double pZ = GetDouble(env, player, "t", "posZ");
    float pYaw = GetFloat(env, player, "yaw", "rotationYaw");
    float pPitch = GetFloat(env, player, "pitch", "rotationPitch");

    // Check onGround
    jclass entityCls = env->GetObjectClass(player);
    jfieldID ogFid = env->GetFieldID(entityCls, "I", "Z");
    if (!ogFid) { env->ExceptionClear(); ogFid = env->GetFieldID(entityCls, "onGround", "Z"); }
    env->ExceptionClear();
    bool onGround = ogFid ? env->GetBooleanField(player, ogFid) : false;
    env->DeleteLocalRef(entityCls);

    // Check sneaking
    jclass eCls = Core::GetInstance().GetJava()->FindClass("pk", "net/minecraft/entity/Entity");
    jmethodID isSneaking = nullptr;
    if (eCls)
    {
        isSneaking = env->GetMethodID(eCls, "n", "()Z");
        if (!isSneaking) { env->ExceptionClear(); isSneaking = env->GetMethodID(eCls, "isSneaking", "()Z"); }
        env->ExceptionClear();
    }
    bool sneaking = isSneaking ? env->CallBooleanMethod(player, isSneaking) : false;
    if (eCls) env->DeleteLocalRef(eCls);

    bool onlyWhileSneaking = ((BooleanSetting*)FindSetting("OnlyWhileSneaking"))->GetValue();
    if (onlyWhileSneaking && !sneaking) { env->DeleteLocalRef(wObj); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    // Check W key
    bool wDown = GetAsyncKeyState('W') & 0x8000;
    bool spaceDown = GetAsyncKeyState(VK_SPACE) & 0x8000;
    bool towerMode = ((BooleanSetting*)FindSetting("TowerMode"))->GetValue();

    int blockX = (int)std::floor(pX);
    int blockY = (int)std::floor(pY - 1.0);
    int blockZ = (int)std::floor(pZ);

    // Tower mode: place above when holding space
    if (towerMode && spaceDown)
    {
        blockY = (int)std::floor(pY);
        int aboveY = (int)std::floor(pY) + 2;
        // Check if block at player head is air
        jclass wCls = env->GetObjectClass(wObj);
        jmethodID isAir = env->GetMethodID(wCls, "e", "(Lcjx;)Z");
        if (!isAir) { env->ExceptionClear(); isAir = env->GetMethodID(wCls, "isAirBlock", "(Lnet/minecraft/util/BlockPos;)Z"); }
        env->ExceptionClear();
        if (!isAir) { env->DeleteLocalRef(wCls); env->DeleteLocalRef(wObj); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
        env->DeleteLocalRef(wCls);
        blockY = aboveY;
        m_Towered = true;
    }
    else
    {
        m_Towered = false;
    }

    // Check delay
    int delay = (int)((NumberSetting*)FindSetting("PlaceDelay"))->GetValue();
    auto now = std::chrono::steady_clock::now();
    int elapsedMs = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastPlace).count();
    int tickMs = 50;
    if (elapsedMs < delay * tickMs) { env->DeleteLocalRef(wObj); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    // Check if block below is air
    jclass wCls = env->GetObjectClass(wObj);
    jmethodID getBlock = env->GetMethodID(wCls, "a", "(III)Lbbp;");
    if (!getBlock) { env->ExceptionClear(); getBlock = env->GetMethodID(wCls, "getBlock", "(III)Lnet/minecraft/block/Block;"); }
    env->ExceptionClear();
    if (getBlock)
    {
        jobject block = env->CallObjectMethod(wObj, getBlock, blockX, blockY, blockZ);
        if (block)
        {
            jclass bCls = env->GetObjectClass(block);
            jmethodID isAirMethod = env->GetMethodID(bCls, "a", "()Z");
            if (!isAirMethod) { env->ExceptionClear(); isAirMethod = env->GetMethodID(bCls, "isAir", "()Z"); }
            env->ExceptionClear();
            bool isBlockAir = isAirMethod ? env->CallBooleanMethod(block, isAirMethod) : true;
            env->DeleteLocalRef(bCls);
            env->DeleteLocalRef(block);
            if (!isBlockAir) { env->DeleteLocalRef(wCls); env->DeleteLocalRef(wObj); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
        }
    }
    env->DeleteLocalRef(wCls);

    // Only place if moving forward
    if (!wDown && !spaceDown) { env->DeleteLocalRef(wObj); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    // Find a block in hotbar
    jclass playerCls = env->GetObjectClass(player);
    jfieldID invFid = env->GetFieldID(playerCls, "e", "Lzo;");
    if (!invFid) { env->ExceptionClear(); invFid = env->GetFieldID(playerCls, "inventory", "Lnet/minecraft/entity/player/InventoryPlayer;"); }
    env->ExceptionClear();
    jobject inventory = invFid ? env->GetObjectField(player, invFid) : nullptr;
    if (!inventory) { env->DeleteLocalRef(playerCls); env->DeleteLocalRef(wObj); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    jclass invCls = env->GetObjectClass(inventory);
    jfieldID ciFid = env->GetFieldID(invCls, "d", "I");
    if (!ciFid) { env->ExceptionClear(); ciFid = env->GetFieldID(invCls, "currentItem", "I"); }
    env->ExceptionClear();
    int currentSlot = ciFid ? env->GetIntField(inventory, ciFid) : 0;

    int blockSlot = -1;
    jmethodID getStack = env->GetMethodID(invCls, "a", "(I)Lzx;");
    if (!getStack) { env->ExceptionClear(); getStack = env->GetMethodID(invCls, "getStackInSlot", "(I)Lnet/minecraft/item/ItemStack;"); }
    env->ExceptionClear();

    jclass itemBlockCls = nullptr;
    if (getStack)
    {
        for (int i = 0; i < 9; i++)
        {
            jobject stack = env->CallObjectMethod(inventory, getStack, i);
            if (stack)
            {
                jclass stackCls = env->GetObjectClass(stack);
                jmethodID getItem = env->GetMethodID(stackCls, "a", "()Lxi;");
                if (!getItem) { env->ExceptionClear(); getItem = env->GetMethodID(stackCls, "getItem", "()Lnet/minecraft/item/Item;"); }
                env->ExceptionClear();
                if (getItem)
                {
                    jobject item = env->CallObjectMethod(stack, getItem);
                    if (item)
                    {
                        if (!itemBlockCls) itemBlockCls = Core::GetInstance().GetJava()->FindClass("abm", "net/minecraft/item/ItemBlock");
                        if (itemBlockCls && env->IsInstanceOf(item, itemBlockCls))
                        {
                            blockSlot = i;
                            env->DeleteLocalRef(item);
                            env->DeleteLocalRef(stackCls);
                            env->DeleteLocalRef(stack);
                            break;
                        }
                        env->DeleteLocalRef(item);
                    }
                }
                env->DeleteLocalRef(stackCls);
                env->DeleteLocalRef(stack);
            }
        }
    }

    if (blockSlot == -1) { env->DeleteLocalRef(invCls); env->DeleteLocalRef(inventory); env->DeleteLocalRef(playerCls); env->DeleteLocalRef(wObj); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    // Switch to block slot if needed
    if (blockSlot != currentSlot)
    {
        env->SetIntField(inventory, ciFid, blockSlot);
    }

    // Smooth rotation
    bool smoothRot = ((BooleanSetting*)FindSetting("SmoothRotation"))->GetValue();
    float rotSpeed = ((NumberSetting*)FindSetting("RotationSpeed"))->GetValue();

    if (smoothRot)
    {
        float targetYaw = (float)(std::atan2(blockZ - pZ + 0.5, blockX - pX + 0.5) * 180.0 / 3.14159265);
        float diff = targetYaw - pYaw;
        while (diff > 180) diff -= 360;
        while (diff < -180) diff += 360;
        float step = std::min(std::abs(diff), rotSpeed * 10.0f) * (diff > 0 ? 1.0f : -1.0f);
        SetFloat(env, player, "yaw", "rotationYaw", pYaw + step);
    }

    // Place block - right click
    HWND hwnd = GetGameWindowHandle();
    if (hwnd)
    {
        POINT pt;
        GetCursorPos(&pt);
        SendMessage(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(pt.x, pt.y));
        SendMessage(hwnd, WM_RBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
    }

    // Auto jump
    bool autoJump = ((BooleanSetting*)FindSetting("AutoJump"))->GetValue();
    if (autoJump && !spaceDown && wDown && onGround)
    {
        // Simulate space press for jump
        SendMessage(GetGameWindowHandle(), WM_KEYDOWN, VK_SPACE, 0);
        SendMessage(GetGameWindowHandle(), WM_KEYUP, VK_SPACE, 0);
    }

    m_LastPlace = now;

    // Restore slot if we switched
    if (blockSlot != currentSlot)
    {
        env->SetIntField(inventory, ciFid, currentSlot);
    }

    env->DeleteLocalRef(invCls);
    env->DeleteLocalRef(inventory);
    env->DeleteLocalRef(playerCls);
    env->DeleteLocalRef(wObj);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mcCls);
    env->DeleteLocalRef(mc);
}

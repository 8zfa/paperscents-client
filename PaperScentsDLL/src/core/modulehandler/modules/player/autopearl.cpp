#include "autopearl.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <Windows.h>

AutoPearlModule::AutoPearlModule()
    : ModuleBase("AutoPearl", "Auto throw pearl at crosshair", Category::Player)
{
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void AutoPearlModule::OnEnable() { Logger::Log("AutoPearl enabled"); m_LastClick = std::chrono::steady_clock::now(); }
void AutoPearlModule::OnDisable() { Logger::Log("AutoPearl disabled"); }

void AutoPearlModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }
    if (!(GetAsyncKeyState(VK_MBUTTON) & 0x8000)) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastClick).count();
    if (elapsed < 300) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    env->ExceptionClear();

    bool bridged = BridgeHelper::Initialize(env);
    jobject player = bridged ? BridgeHelper::GetPlayer(env) : nullptr;
    jobject mc = nullptr;

    if (!player)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java || !java->IsValid()) return;
        jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
        if (!mcClass) return;
        jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
        if (!getMc) { env->DeleteLocalRef(mcClass); return; }
        mc = env->CallStaticObjectMethod(mcClass, getMc);
        if (!mc) { env->DeleteLocalRef(mcClass); return; }
        for (auto f : {"s", "t", "h", "thePlayer"})
        {
            jfieldID pf = env->GetFieldID(mcClass, f, "Lbew;");
            if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
            if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
        }
        if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    }
    else
    {
        mc = BridgeHelper::GetMinecraftInstance(env);
    }

    // Find ender pearl in inventory
    jclass playerClass = env->GetObjectClass(player);
    jfieldID inventoryField = env->GetFieldID(playerClass, "inventory", "Lnet/minecraft/entity/player/InventoryPlayer;");
    if (!inventoryField) { env->ExceptionClear(); inventoryField = env->GetFieldID(playerClass, "d", "Lbja;"); }
    env->ExceptionClear();

    if (!inventoryField) { env->DeleteLocalRef(playerClass); env->DeleteLocalRef(player); if (!bridged && mc) env->DeleteLocalRef(mc); return; }

    jobject inventory = env->GetObjectField(player, inventoryField);
    if (!inventory) { env->DeleteLocalRef(playerClass); env->DeleteLocalRef(player); if (!bridged && mc) env->DeleteLocalRef(mc); return; }

    jclass invClass = env->GetObjectClass(inventory);
    jfieldID mainInvField = env->GetFieldID(invClass, "mainInventory", "[Lnet/minecraft/item/ItemStack;");
    if (!mainInvField) { env->ExceptionClear(); mainInvField = env->GetFieldID(invClass, "a", "[Lqa;"); }
    env->ExceptionClear();

    jobjectArray mainInv = mainInvField ? (jobjectArray)env->GetObjectField(inventory, mainInvField) : nullptr;
    if (!mainInv) { env->DeleteLocalRef(invClass); env->DeleteLocalRef(inventory); env->DeleteLocalRef(playerClass); env->DeleteLocalRef(player); if (!bridged && mc) env->DeleteLocalRef(mc); return; }

    jfieldID currentItemField = env->GetFieldID(invClass, "currentItem", "I");
    if (!currentItemField) { env->ExceptionClear(); currentItemField = env->GetFieldID(invClass, "f", "I"); }
    env->ExceptionClear();

    int currentSlot = currentItemField ? env->GetIntField(inventory, currentItemField) : -1;
    int pearlSlot = -1;

    jclass itemStackClass = env->FindClass("net/minecraft/item/ItemStack");
    jclass itemClass = env->FindClass("net/minecraft/item/Item");

    for (int i = 36; i <= 44; i++)
    {
        jobject stack = env->GetObjectArrayElement(mainInv, i);
        if (!stack) continue;
        if (itemStackClass)
        {
            jmethodID getItemMethod = env->GetMethodID(itemStackClass, "getItem", "()Lnet/minecraft/item/Item;");
            if (!getItemMethod) { env->ExceptionClear(); getItemMethod = env->GetMethodID(itemStackClass, "c", "()Lpi;"); }
            env->ExceptionClear();
            if (getItemMethod)
            {
                jobject item = env->CallObjectMethod(stack, getItemMethod);
                if (item && itemClass)
                {
                    jmethodID getUnloc = env->GetMethodID(itemClass, "getUnlocalizedName", "()Ljava/lang/String;");
                    if (!getUnloc) { env->ExceptionClear(); getUnloc = env->GetMethodID(itemClass, "a", "()Ljava/lang/String;"); }
                    env->ExceptionClear();
                    if (getUnloc)
                    {
                        jstring name = (jstring)env->CallObjectMethod(item, getUnloc);
                        if (name)
                        {
                            const char* nameStr = env->GetStringUTFChars(name, nullptr);
                            if (nameStr && (strstr(nameStr, "enderPearl") != nullptr || strstr(nameStr, "ender_pearl") != nullptr))
                                pearlSlot = i - 36;
                            if (nameStr) env->ReleaseStringUTFChars(name, nameStr);
                            env->DeleteLocalRef(name);
                        }
                    }
                    env->DeleteLocalRef(item);
                }
            }
        }
        env->DeleteLocalRef(stack);
        if (pearlSlot >= 0) break;
    }

    if (itemClass) env->DeleteLocalRef(itemClass);
    if (itemStackClass) env->DeleteLocalRef(itemStackClass);

    if (pearlSlot >= 0 && pearlSlot != currentSlot && currentItemField)
        env->SetIntField(inventory, currentItemField, pearlSlot);

    if (pearlSlot >= 0 && mc)
    {
        jclass mcCls = env->GetObjectClass(mc);
        jmethodID rightClick = env->GetMethodID(mcCls, "rightClickMouse", "()V");
        if (!rightClick) { env->ExceptionClear(); rightClick = env->GetMethodID(mcCls, "d", "()V"); }
        env->ExceptionClear();
        if (rightClick) { env->CallVoidMethod(mc, rightClick); m_LastClick = std::chrono::steady_clock::now(); }
        env->DeleteLocalRef(mcCls);
    }

    env->DeleteLocalRef(mainInv);
    env->DeleteLocalRef(invClass);
    env->DeleteLocalRef(inventory);
    env->DeleteLocalRef(playerClass);
    env->DeleteLocalRef(player);
    if (!bridged && mc) env->DeleteLocalRef(mc);
}

#include "autoeat.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"

AutoEatModule::AutoEatModule()
    : ModuleBase("AutoEat", "Auto eat when hungry", Category::Player)
{
    AddSetting<NumberSetting>("HungerThreshold", 6.0f, 1.0f, 19.0f, 1.0f, "Food level to eat at");
    AddSetting<BooleanSetting>("SprintReset", true, "Resume sprinting after eating");
    AddSetting<BooleanSetting>("PauseOnDamage", false, "Pause eating when damaged");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void AutoEatModule::OnEnable() { Logger::Log("AutoEat enabled"); m_IsEating = false; m_LastFoodLevel = 20; }
void AutoEatModule::OnDisable() { Logger::Log("AutoEat disabled"); m_IsEating = false; }

void AutoEatModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }
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

    // Get food stats
    jclass playerClass = env->GetObjectClass(player);
    if (!playerClass) { env->DeleteLocalRef(player); if (!bridged) { env->DeleteLocalRef(mc); } return; }

    jmethodID getFoodStats = env->GetMethodID(playerClass, "getFoodStats", "()Lnet/minecraft/util/FoodStats;");
    if (!getFoodStats) { env->ExceptionClear(); getFoodStats = env->GetMethodID(playerClass, "ao", "()Lbms;"); }
    env->ExceptionClear();

    int foodLevel = 20;
    jobject foodStats = nullptr;
    jclass foodStatsClass = nullptr;

    if (getFoodStats)
    {
        foodStats = env->CallObjectMethod(player, getFoodStats);
        if (foodStats)
        {
            foodStatsClass = env->GetObjectClass(foodStats);
            if (foodStatsClass)
            {
                jmethodID getFoodLevel = env->GetMethodID(foodStatsClass, "getFoodLevel", "()I");
                if (!getFoodLevel) { env->ExceptionClear(); getFoodLevel = env->GetMethodID(foodStatsClass, "a", "()I"); }
                env->ExceptionClear();
                if (getFoodLevel) foodLevel = env->CallIntMethod(foodStats, getFoodLevel);
            }
        }
    }

    // Is using item check
    bool usingItem = false;
    if (bridged && BridgeHelper::ELBridge_IsUsingItem)
        usingItem = env->CallBooleanMethod(player, BridgeHelper::ELBridge_IsUsingItem) != 0;
    else
    {
        jmethodID isUsingItemMeth = env->GetMethodID(playerClass, "isUsingItem", "()Z");
        if (!isUsingItemMeth) { env->ExceptionClear(); isUsingItemMeth = env->GetMethodID(playerClass, "e", "()Z"); }
        if (isUsingItemMeth) { env->ExceptionClear(); usingItem = env->CallBooleanMethod(player, isUsingItemMeth); }
    }

    float threshold = ((NumberSetting*)FindSetting("HungerThreshold"))->GetValue();
    bool pauseOnDamage = ((BooleanSetting*)FindSetting("PauseOnDamage"))->GetValue();
    bool takingDamage = false;

    if (pauseOnDamage && m_IsEating)
    {
        int hurtTime = 0;
        if (bridged && BridgeHelper::ELBridge_GetHurtTime)
            hurtTime = env->CallIntMethod(player, BridgeHelper::ELBridge_GetHurtTime);
        else
        {
            jfieldID hurtTimeField = env->GetFieldID(playerClass, "hurtTime", "I");
            if (!hurtTimeField) { env->ExceptionClear(); hurtTimeField = env->GetFieldID(env->FindClass("net/minecraft/entity/EntityLivingBase"), "y", "I"); }
            if (hurtTimeField) { env->ExceptionClear(); hurtTime = env->GetIntField(player, hurtTimeField); }
        }
        takingDamage = hurtTime > 0;
    }

    if (m_IsEating)
    {
        if (!usingItem || takingDamage)
        {
            m_IsEating = false;
            if (((BooleanSetting*)FindSetting("SprintReset"))->GetValue() && foodLevel > m_LastFoodLevel)
            {
                bool sprintSet = false;
                if (bridged && BridgeHelper::EPSPBridge_SetSprinting)
                { env->CallVoidMethod(player, BridgeHelper::EPSPBridge_SetSprinting, JNI_TRUE); sprintSet = true; }
                if (!sprintSet)
                {
                    jmethodID setSprinting = env->GetMethodID(playerClass, "setSprinting", "(Z)V");
                    if (!setSprinting) { env->ExceptionClear(); setSprinting = env->GetMethodID(playerClass, "k", "(Z)V"); }
                    if (setSprinting) { env->ExceptionClear(); env->CallVoidMethod(player, setSprinting, JNI_TRUE); }
                }
            }
        }
    }
    else
    {
        if (foodLevel < (int)threshold && !usingItem && !takingDamage)
        {
            jfieldID inventoryField = env->GetFieldID(playerClass, "inventory", "Lnet/minecraft/entity/player/InventoryPlayer;");
            if (!inventoryField) { env->ExceptionClear(); inventoryField = env->GetFieldID(playerClass, "d", "Lbja;"); }
            env->ExceptionClear();

            if (inventoryField)
            {
                jobject inventory = env->GetObjectField(player, inventoryField);
                if (inventory)
                {
                    jclass invClass = env->GetObjectClass(inventory);
                    if (invClass)
                    {
                        jfieldID mainInvField = env->GetFieldID(invClass, "mainInventory", "[Lnet/minecraft/item/ItemStack;");
                        if (!mainInvField) { env->ExceptionClear(); mainInvField = env->GetFieldID(invClass, "a", "[Lqa;"); }
                        env->ExceptionClear();

                        if (mainInvField)
                        {
                            jobjectArray mainInv = (jobjectArray)env->GetObjectField(inventory, mainInvField);
                            if (mainInv)
                            {
                                jfieldID currentItemField = env->GetFieldID(invClass, "currentItem", "I");
                                if (!currentItemField) { env->ExceptionClear(); currentItemField = env->GetFieldID(invClass, "f", "I"); }
                                env->ExceptionClear();

                                if (currentItemField)
                                {
                                    int currentSlot = env->GetIntField(inventory, currentItemField);
                                    int foodSlot = -1;

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
                                                    jmethodID getUnlocName = env->GetMethodID(itemClass, "getUnlocalizedName", "()Ljava/lang/String;");
                                                    if (!getUnlocName) { env->ExceptionClear(); getUnlocName = env->GetMethodID(itemClass, "a", "()Ljava/lang/String;"); }
                                                    env->ExceptionClear();

                                                    if (getUnlocName)
                                                    {
                                                        jstring name = (jstring)env->CallObjectMethod(item, getUnlocName);
                                                        if (name)
                                                        {
                                                            const char* nameStr = env->GetStringUTFChars(name, nullptr);
                                                            if (nameStr)
                                                            {
                                                                if (strstr(nameStr, "food") != nullptr) foodSlot = i - 36;
                                                                env->ReleaseStringUTFChars(name, nameStr);
                                                            }
                                                            env->DeleteLocalRef(name);
                                                        }
                                                    }
                                                    env->DeleteLocalRef(item);
                                                }
                                            }
                                        }
                                        env->DeleteLocalRef(stack);
                                        if (foodSlot >= 0) break;
                                    }

                                    if (itemClass) env->DeleteLocalRef(itemClass);
                                    if (itemStackClass) env->DeleteLocalRef(itemStackClass);

                                    if (foodSlot >= 0 && foodSlot != currentSlot)
                                        env->SetIntField(inventory, currentItemField, foodSlot);

                                    if (foodSlot >= 0)
                                    {
                                        // rightClickMouse via notch
                                        jmethodID rightClick = env->GetMethodID(playerClass, "a", "(Ladq;)V");
                                        if (!rightClick)
                                        {
                                            // Try itemStack-based right-click
                                            if (mc)
                                            {
                                                jclass mcCls = env->GetObjectClass(mc);
                                                jmethodID rightClickMc = env->GetMethodID(mcCls, "d", "()V");
                                                if (!rightClickMc) { env->ExceptionClear(); rightClickMc = env->GetMethodID(mcCls, "rightClickMouse", "()V"); }
                                                if (rightClickMc) { env->ExceptionClear(); env->CallVoidMethod(mc, rightClickMc); }
                                                env->DeleteLocalRef(mcCls);
                                            }
                                        }
                                        else
                                        {
                                            env->ExceptionClear();
                                            // Try right-click with held item
                                        }

                                        m_IsEating = true;
                                        m_LastFoodLevel = foodLevel;
                                    }
                                }
                            }
                        }
                        env->DeleteLocalRef(invClass);
                    }
                    env->DeleteLocalRef(inventory);
                }
            }
        }
    }

    if (foodStatsClass) env->DeleteLocalRef(foodStatsClass);
    if (foodStats) env->DeleteLocalRef(foodStats);
    env->DeleteLocalRef(playerClass);
    env->DeleteLocalRef(player);
    if (!bridged && mc) env->DeleteLocalRef(mc);
}

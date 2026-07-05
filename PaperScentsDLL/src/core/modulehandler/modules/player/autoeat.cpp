#include "autoeat.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../java/java.h"
#include "../../../sdk/strayCache.h"

AutoEatModule::AutoEatModule()
    : ModuleBase("AutoEat", "Auto eat when hungry", Category::Player)
{
    AddSetting<NumberSetting>("HungerThreshold", 6.0f, 1.0f, 19.0f, 1.0f, "Food level to eat at");
    AddSetting<BooleanSetting>("SprintReset", true, "Resume sprinting after eating");
    AddSetting<BooleanSetting>("PauseOnDamage", false, "Pause eating when damaged");
}

void AutoEatModule::OnEnable()
{
    Logger::Log("AutoEat enabled");
    m_IsEating = false;
    m_LastFoodLevel = 20;
}

void AutoEatModule::OnDisable()
{
    Logger::Log("AutoEat disabled");
    m_IsEating = false;
}

void AutoEatModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player)
    {
        env->DeleteLocalRef(mc);
        return;
    }

    jclass playerClass = env->GetObjectClass(player);
    if (!playerClass)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }

    jmethodID getFoodStats = env->GetMethodID(playerClass, "ao", "()Lbms;");
    if (!getFoodStats)
    {
        env->ExceptionClear();
        getFoodStats = env->GetMethodID(playerClass, "getFoodStats", "()Lnet/minecraft/util/FoodStats;");
    }
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
                jmethodID getFoodLevel = env->GetMethodID(foodStatsClass, "a", "()I");
                if (!getFoodLevel)
                {
                    env->ExceptionClear();
                    getFoodLevel = env->GetMethodID(foodStatsClass, "getFoodLevel", "()I");
                }
                env->ExceptionClear();

                if (getFoodLevel)
                    foodLevel = env->CallIntMethod(foodStats, getFoodLevel);
            }
        }
    }

    jmethodID isUsingItem = env->GetMethodID(playerClass, "e", "()Z");
    if (!isUsingItem)
    {
        env->ExceptionClear();
        isUsingItem = env->GetMethodID(playerClass, "isUsingItem", "()Z");
    }
    env->ExceptionClear();

    bool usingItem = false;
    if (isUsingItem)
        usingItem = env->CallBooleanMethod(player, isUsingItem);

    float threshold = ((NumberSetting*)FindSetting("HungerThreshold"))->GetValue();
    bool pauseOnDamage = ((BooleanSetting*)FindSetting("PauseOnDamage"))->GetValue();
    bool takingDamage = false;

    if (pauseOnDamage && m_IsEating)
    {
        if (StrayCache::EntityLivingBase)
        {
            jfieldID hurtTimeField = env->GetFieldID(StrayCache::EntityLivingBase, "y", "I");
            if (!hurtTimeField)
            {
                env->ExceptionClear();
                hurtTimeField = env->GetFieldID(StrayCache::EntityLivingBase, "hurtTime", "I");
            }
            env->ExceptionClear();

            if (hurtTimeField)
            {
                int hurtTime = env->GetIntField(player, hurtTimeField);
                takingDamage = hurtTime > 0;
            }
        }
    }

    if (m_IsEating)
    {
        if (!usingItem || takingDamage)
        {
            m_IsEating = false;

            if (((BooleanSetting*)FindSetting("SprintReset"))->GetValue() && foodLevel > m_LastFoodLevel)
            {
                jmethodID setSprinting = env->GetMethodID(playerClass, "k", "(Z)V");
                if (!setSprinting)
                {
                    env->ExceptionClear();
                    setSprinting = env->GetMethodID(playerClass, "setSprinting", "(Z)V");
                }
                env->ExceptionClear();

                if (setSprinting)
                    env->CallVoidMethod(player, setSprinting, JNI_TRUE);
            }
        }
    }
    else
    {
        if (foodLevel < (int)threshold && !usingItem && !takingDamage)
        {
            jfieldID inventoryField = env->GetFieldID(playerClass, "d", "Lbja;");
            if (!inventoryField)
            {
                env->ExceptionClear();
                inventoryField = env->GetFieldID(playerClass, "inventory", "Lnet/minecraft/entity/player/InventoryPlayer;");
            }
            env->ExceptionClear();

            if (inventoryField)
            {
                jobject inventory = env->GetObjectField(player, inventoryField);
                if (inventory)
                {
                    jclass invClass = env->GetObjectClass(inventory);
                    if (invClass)
                    {
                        jfieldID mainInvField = env->GetFieldID(invClass, "a", "[Lqa;");
                        if (!mainInvField)
                        {
                            env->ExceptionClear();
                            mainInvField = env->GetFieldID(invClass, "mainInventory", "[Lnet/minecraft/item/ItemStack;");
                        }
                        env->ExceptionClear();

                        if (mainInvField)
                        {
                            jobjectArray mainInv = (jobjectArray)env->GetObjectField(inventory, mainInvField);
                            if (mainInv)
                            {
                                jfieldID currentItemField = env->GetFieldID(invClass, "f", "I");
                                if (!currentItemField)
                                {
                                    env->ExceptionClear();
                                    currentItemField = env->GetFieldID(invClass, "currentItem", "I");
                                }
                                env->ExceptionClear();

                                if (currentItemField)
                                {
                                    int currentSlot = env->GetIntField(inventory, currentItemField);
                                    int foodSlot = -1;

                                    jclass itemStackClass = Core::GetInstance().GetJava()->FindClass("qa", "net/minecraft/item/ItemStack");
                                    jclass itemClass = Core::GetInstance().GetJava()->FindClass("pi", "net/minecraft/item/Item");

                                    for (int i = 36; i <= 44; i++)
                                    {
                                        jobject stack = env->GetObjectArrayElement(mainInv, i);
                                        if (!stack) continue;

                                        if (itemStackClass)
                                        {
                                            jmethodID getItemMethod = env->GetMethodID(itemStackClass, "c", "()Lpi;");
                                            if (!getItemMethod)
                                            {
                                                env->ExceptionClear();
                                                getItemMethod = env->GetMethodID(itemStackClass, "getItem", "()Lnet/minecraft/item/Item;");
                                            }
                                            env->ExceptionClear();

                                            if (getItemMethod)
                                            {
                                                jobject item = env->CallObjectMethod(stack, getItemMethod);
                                                if (item && itemClass)
                                                {
                                                    jmethodID getUnlocName = env->GetMethodID(itemClass, "a", "()Ljava/lang/String;");
                                                    if (!getUnlocName)
                                                    {
                                                        env->ExceptionClear();
                                                        getUnlocName = env->GetMethodID(itemClass, "getUnlocalizedName", "()Ljava/lang/String;");
                                                    }
                                                    env->ExceptionClear();

                                                    if (getUnlocName)
                                                    {
                                                        jstring name = (jstring)env->CallObjectMethod(item, getUnlocName);
                                                        if (name)
                                                        {
                                                            const char* nameStr = env->GetStringUTFChars(name, nullptr);
                                                            if (nameStr)
                                                            {
                                                                if (strstr(nameStr, "food") != nullptr)
                                                                    foodSlot = i - 36;
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
                                        jmethodID rightClick = env->GetMethodID(StrayCache::Minecraft, "d", "()V");
                                        if (!rightClick)
                                        {
                                            env->ExceptionClear();
                                            rightClick = env->GetMethodID(StrayCache::Minecraft, "rightClickMouse", "()V");
                                        }
                                        env->ExceptionClear();

                                        if (rightClick)
                                            env->CallVoidMethod(mc, rightClick);

                                        m_IsEating = true;
                                        m_LastFoodLevel = foodLevel;
                                    }
                                }
                            }
                            env->DeleteLocalRef(mainInv);
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
    env->DeleteLocalRef(mc);
}

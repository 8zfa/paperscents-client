#include "sprint.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <jni.h>

SprintModule::SprintModule()
    : ModuleBase("Sprint", "Automatically sprints", Category::Movement)
{
    AddSetting<BooleanSetting>("OmniSprint", false, "Sprint in all directions");
    AddSetting<BooleanSetting>("SprintInWater", false, "Sprint while in water");
}

void SprintModule::OnEnable() { Logger::Log("Sprint enabled"); }
void SprintModule::OnDisable() { Logger::Log("Sprint disabled"); }

void SprintModule::OnUpdate()
{
    if (!IsEnabled()) return;
    Java* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return;
    JNIEnv* env = java->GetEnv();
    if (!env) return;

    bool omni = ((BooleanSetting*)FindSetting("OmniSprint"))->GetValue();
    bool water = ((BooleanSetting*)FindSetting("SprintInWater"))->GetValue();

    jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) return;
    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    const char* playerFields[] = { "s", "t", "h", "thePlayer" };
    jobject player = nullptr;
    jfieldID pf = nullptr;
    for (auto f : playerFields)
    {
        pf = env->GetFieldID(mcClass, f, "Lbew;");
        if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
        if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
    }
    if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass entityClass = java->FindClass("vg", "net/minecraft/entity/Entity");
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass livingClass = java->FindClass("sa", "net/minecraft/entity/EntityLivingBase");
    if (!livingClass) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    // Check if sneaking
    jmethodID isSneaking = nullptr;
    const char* sneakMethods[] = { "n", "am", "an", "isSneaking" };
    for (auto m : sneakMethods)
    {
        isSneaking = env->GetMethodID(entityClass, m, "()Z");
        if (isSneaking) break;
        env->ExceptionClear();
    }
    if (isSneaking && env->CallBooleanMethod(player, isSneaking))
    {
        env->DeleteLocalRef(livingClass);
        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    // Check if using item
    jmethodID isUsingItem = nullptr;
    const char* useMethods[] = { "e", "g", "h", "isUsingItem" };
    for (auto m : useMethods)
    {
        isUsingItem = env->GetMethodID(livingClass, m, "()Z");
        if (isUsingItem) break;
        env->ExceptionClear();
    }
    if (isUsingItem && env->CallBooleanMethod(player, isUsingItem))
    {
        env->DeleteLocalRef(livingClass);
        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    // Check if in water
    if (!water)
    {
        jmethodID isInWater = nullptr;
        const char* waterMethods[] = { "H", "L", "isInWater", "T", "U" };
        for (auto m : waterMethods)
        {
            isInWater = env->GetMethodID(entityClass, m, "()Z");
            if (isInWater) break;
            env->ExceptionClear();
        }
        if (isInWater && env->CallBooleanMethod(player, isInWater))
        {
            env->DeleteLocalRef(livingClass);
            env->DeleteLocalRef(entityClass);
            env->DeleteLocalRef(player);
            env->DeleteLocalRef(mc);
            env->DeleteLocalRef(mcClass);
            return;
        }
    }

    // Check server-side sprint eligibility (food level > 6)
    jclass playerClass = java->FindClass("vw", "net/minecraft/client/entity/EntityPlayerSP");
    if (!playerClass) playerClass = java->FindClass("bew", "net/minecraft/client/entity/EntityPlayerSP");
    if (playerClass)
    {
        jmethodID getFoodStats = nullptr;
        const char* foodMethods[] = { "ao", "ap", "getFoodStats", "cq", "cr" };
        for (auto m : foodMethods)
        {
            getFoodStats = env->GetMethodID(playerClass, m, "()Lbms;");
            if (!getFoodStats) getFoodStats = env->GetMethodID(playerClass, m, "()Lnet/minecraft/util/FoodStats;");
            if (getFoodStats) break;
            env->ExceptionClear();
        }
        if (getFoodStats)
        {
            jobject foodStats = env->CallObjectMethod(player, getFoodStats);
            if (foodStats)
            {
                jclass foodClass = env->GetObjectClass(foodStats);
                jmethodID getFoodLevel = nullptr;
                const char* levelMethods[] = { "a", "b", "getFoodLevel" };
                for (auto m : levelMethods)
                {
                    getFoodLevel = env->GetMethodID(foodClass, m, "()I");
                    if (getFoodLevel) break;
                    env->ExceptionClear();
                }
                if (getFoodLevel)
                {
                    jint foodLevel = env->CallIntMethod(foodStats, getFoodLevel);
                    if (foodLevel <= 6)
                    {
                        env->DeleteLocalRef(foodClass);
                        env->DeleteLocalRef(foodStats);
                        env->DeleteLocalRef(playerClass);
                        env->DeleteLocalRef(livingClass);
                        env->DeleteLocalRef(entityClass);
                        env->DeleteLocalRef(player);
                        env->DeleteLocalRef(mc);
                        env->DeleteLocalRef(mcClass);
                        return;
                    }
                }
                env->DeleteLocalRef(foodClass);
                env->DeleteLocalRef(foodStats);
            }
        }
        env->DeleteLocalRef(playerClass);
    }

    // Check if not sprinting yet
    jmethodID isSprinting = nullptr;
    const char* sprintCheckMethods[] = { "f", "g", "isSprinting", "q", "r" };
    for (auto m : sprintCheckMethods)
    {
        isSprinting = env->GetMethodID(entityClass, m, "()Z");
        if (isSprinting) break;
        env->ExceptionClear();
    }
    bool alreadySprinting = isSprinting && env->CallBooleanMethod(player, isSprinting);

    if (!alreadySprinting)
    {
        jmethodID setSprinting = nullptr;
        const char* setSprintMethods[] = { "b", "c", "a", "setSprinting", "d" };
        for (auto m : setSprintMethods)
        {
            setSprinting = env->GetMethodID(entityClass, m, "(Z)V");
            if (setSprinting) break;
            env->ExceptionClear();
        }
        if (setSprinting)
            env->CallVoidMethod(player, setSprinting, JNI_TRUE);
    }

    env->DeleteLocalRef(livingClass);
    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

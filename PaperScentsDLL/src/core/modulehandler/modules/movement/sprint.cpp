#include "sprint.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"

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
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;
    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    bool omni = ((BooleanSetting*)FindSetting("OmniSprint"))->GetValue();
    bool water = ((BooleanSetting*)FindSetting("SprintInWater"))->GetValue();

    Java* java = Core::GetInstance().GetJava();
    jclass entityClass = java->FindClass("pk", "net/minecraft/entity/Entity");
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }
    jclass livingClass = java->FindClass("pr", "net/minecraft/entity/EntityLivingBase");
    if (!livingClass) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jmethodID isSneaking = env->GetMethodID(entityClass, "n", "()Z");
    if (!isSneaking) { env->ExceptionClear(); isSneaking = env->GetMethodID(entityClass, "isSneaking", "()Z"); }
    if (isSneaking)
    {
        env->ExceptionClear();
        if (env->CallBooleanMethod(player, isSneaking))
        {
            env->DeleteLocalRef(livingClass); env->DeleteLocalRef(entityClass);
            env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return;
        }
    }

    jmethodID isUsingItem = env->GetMethodID(livingClass, "e", "()Z");
    if (!isUsingItem) { env->ExceptionClear(); isUsingItem = env->GetMethodID(livingClass, "isUsingItem", "()Z"); }
    if (isUsingItem)
    {
        env->ExceptionClear();
        if (env->CallBooleanMethod(player, isUsingItem))
        {
            env->DeleteLocalRef(livingClass); env->DeleteLocalRef(entityClass);
            env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return;
        }
    }

    if (!water)
    {
        jmethodID isInWater = env->GetMethodID(entityClass, "H", "()Z");
        if (!isInWater) { env->ExceptionClear(); isInWater = env->GetMethodID(entityClass, "isInWater", "()Z"); }
        if (isInWater)
        {
            env->ExceptionClear();
            if (env->CallBooleanMethod(player, isInWater))
            {
                env->DeleteLocalRef(livingClass); env->DeleteLocalRef(entityClass);
                env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return;
            }
        }
    }

    jmethodID setSprinting = env->GetMethodID(entityClass, "b", "(Z)V");
    if (!setSprinting) { env->ExceptionClear(); setSprinting = env->GetMethodID(entityClass, "setSprinting", "(Z)V"); }
    if (setSprinting)
    {
        env->ExceptionClear();
        jmethodID isSprinting = env->GetMethodID(entityClass, "f", "()Z");
        if (!isSprinting) { env->ExceptionClear(); isSprinting = env->GetMethodID(entityClass, "isSprinting", "()Z"); }
        if (isSprinting) { env->ExceptionClear(); if (!env->CallBooleanMethod(player, isSprinting)) env->CallVoidMethod(player, setSprinting, JNI_TRUE); }
        else env->CallVoidMethod(player, setSprinting, JNI_TRUE);
    }

    env->DeleteLocalRef(livingClass); env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player); env->DeleteLocalRef(mc);
}

#include "autorespawn.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"

AutoRespawnModule::AutoRespawnModule()
    : ModuleBase("AutoRespawn", "Auto respawn on death", Category::Player)
{
}

void AutoRespawnModule::OnEnable() { Logger::Log("AutoRespawn enabled"); }
void AutoRespawnModule::OnDisable() { Logger::Log("AutoRespawn disabled"); }

void AutoRespawnModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
    if (!entityClass) { env->ExceptionClear(); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jfieldID isDeadField = env->GetFieldID(entityClass, "J", "Z");
    if (!isDeadField) { env->ExceptionClear(); isDeadField = env->GetFieldID(entityClass, "isDead", "Z"); }
    env->ExceptionClear();

    bool isDead = false;
    if (isDeadField)
        isDead = env->GetBooleanField(player, isDeadField) == JNI_TRUE;

    if (isDead)
    {
        jclass mcClass = env->GetObjectClass(mc);
        jmethodID displayScreen = env->GetMethodID(mcClass, "a", "(Lbik;)V");
        if (!displayScreen) { env->ExceptionClear(); displayScreen = env->GetMethodID(mcClass, "displayGuiScreen", "(Lnet/minecraft/client/gui/GuiScreen;)V"); }
        env->ExceptionClear();

        if (displayScreen)
            env->CallVoidMethod(mc, displayScreen, nullptr);

        env->DeleteLocalRef(mcClass);
    }

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

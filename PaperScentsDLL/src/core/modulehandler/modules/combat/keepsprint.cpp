#include "keepsprint.h"
#include "../../../core.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../utilities/logger.h"
#include <jni.h>

KeepSprintModule::KeepSprintModule()
    : ModuleBase("KeepSprint", "Prevents sprint reset on damage", Category::Combat)
{
}

void KeepSprintModule::OnEnable() { Logger::Log("KeepSprint enabled"); }
void KeepSprintModule::OnDisable() { Logger::Log("KeepSprint disabled"); }

void KeepSprintModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;
    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    Java* java = Core::GetInstance().GetJava();
    jclass entityClass = java->FindClass("pk", "net/minecraft/entity/Entity");
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jfieldID hurtFid = env->GetFieldID(entityClass, "W", "I");
    if (!hurtFid) { env->ExceptionClear(); hurtFid = env->GetFieldID(entityClass, "hurtTime", "I"); }
    env->ExceptionClear();

    if (hurtFid)
    {
        jint hurtTime = env->GetIntField(player, hurtFid);
        if (hurtTime > 0)
        {
            jmethodID isSprintMethod = env->GetMethodID(entityClass, "f", "()Z");
            if (!isSprintMethod) { env->ExceptionClear(); isSprintMethod = env->GetMethodID(entityClass, "isSprinting", "()Z"); }
            env->ExceptionClear();

            if (isSprintMethod)
            {
                jboolean sprinting = env->CallBooleanMethod(player, isSprintMethod);
                if (!sprinting)
                {
                    jmethodID setSprint = env->GetMethodID(entityClass, "b", "(Z)V");
                    if (!setSprint) { env->ExceptionClear(); setSprint = env->GetMethodID(entityClass, "setSprinting", "(Z)V"); }
                    env->ExceptionClear();

                    if (setSprint)
                        env->CallVoidMethod(player, setSprint, JNI_TRUE);
                }
            }
        }
    }

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

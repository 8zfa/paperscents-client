#include "phase.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <jni.h>
#include <cmath>
#include <Windows.h>

PhaseModule::PhaseModule()
    : ModuleBase("Phase", "Walk through blocks", Category::Movement)
{
    AddSetting<BooleanSetting>("Clip", false, "Teleport forward when sneaking against a wall");
}

void PhaseModule::OnEnable()
{
    Logger::Log("Phase enabled");
    m_NoClipRestore = false;
}

void PhaseModule::OnDisable()
{
    Logger::Log("Phase disabled");
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    env->ExceptionClear();

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;
    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jclass entityClass = StrayCache::Entity;
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jfieldID noClipField = env->GetFieldID(entityClass, "z", "Z");
    if (!noClipField) { env->ExceptionClear(); noClipField = env->GetFieldID(entityClass, "noClip", "Z"); }
    env->ExceptionClear();

    if (noClipField)
        env->SetBooleanField(player, noClipField, m_NoClipRestore ? JNI_TRUE : JNI_FALSE);

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

void PhaseModule::OnUpdate()
{
    if (!IsEnabled()) return;
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    env->ExceptionClear();

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;
    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jclass entityClass = StrayCache::Entity;
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jfieldID noClipField = env->GetFieldID(entityClass, "z", "Z");
    if (!noClipField) { env->ExceptionClear(); noClipField = env->GetFieldID(entityClass, "noClip", "Z"); }
    env->ExceptionClear();

    if (noClipField)
        env->SetBooleanField(player, noClipField, JNI_TRUE);

    bool clip = ((BooleanSetting*)FindSetting("Clip"))->GetValue();
    if (!clip)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }

    jmethodID isSneaking = env->GetMethodID(entityClass, "n", "()Z");
    if (!isSneaking) { env->ExceptionClear(); isSneaking = env->GetMethodID(entityClass, "isSneaking", "()Z"); }
    env->ExceptionClear();

    if (!isSneaking)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }

    bool sneaking = env->CallBooleanMethod(player, isSneaking);
    env->ExceptionClear();

    if (!sneaking)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }

    jfieldID collidedHorizField = env->GetFieldID(entityClass, "aa", "Z");
    if (!collidedHorizField) { env->ExceptionClear(); collidedHorizField = env->GetFieldID(entityClass, "isCollidedHorizontally", "Z"); }
    env->ExceptionClear();

    if (!collidedHorizField || !env->GetBooleanField(player, collidedHorizField))
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }

    jfieldID posXField = env->GetFieldID(entityClass, "r", "D");
    if (!posXField) { env->ExceptionClear(); posXField = env->GetFieldID(entityClass, "posX", "D"); }
    env->ExceptionClear();

    jfieldID posZField = env->GetFieldID(entityClass, "t", "D");
    if (!posZField) { env->ExceptionClear(); posZField = env->GetFieldID(entityClass, "posZ", "D"); }
    env->ExceptionClear();

    jfieldID yawField = env->GetFieldID(entityClass, "y", "F");
    if (!yawField) { env->ExceptionClear(); yawField = env->GetFieldID(entityClass, "rotationYaw", "F"); }
    env->ExceptionClear();

    if (!posXField || !posZField || !yawField)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }

    float yaw = env->GetFloatField(player, yawField);
    float rad = yaw * 0.017453292f;
    double px = env->GetDoubleField(player, posXField);
    double pz = env->GetDoubleField(player, posZField);

    env->SetDoubleField(player, posXField, px + (-std::sin(rad)) * 0.5);
    env->SetDoubleField(player, posZField, pz + std::cos(rad) * 0.5);

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

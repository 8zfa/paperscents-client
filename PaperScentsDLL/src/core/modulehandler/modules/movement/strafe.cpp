#include "strafe.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <jni.h>
#include <cmath>
#include <Windows.h>

StrafeModule::StrafeModule()
    : ModuleBase("Strafe", "Perfect movement strafing", Category::Movement)
{
    AddSetting<BooleanSetting>("AutoJump", true, "Automatically jump when strafing");
}

void StrafeModule::OnEnable() { Logger::Log("Strafe enabled"); }
void StrafeModule::OnDisable() { Logger::Log("Strafe disabled"); }

void StrafeModule::OnUpdate()
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

    bool autoJump = ((BooleanSetting*)FindSetting("AutoJump"))->GetValue();

    jfieldID motionXField = env->GetFieldID(entityClass, "v", "D");
    if (!motionXField) { env->ExceptionClear(); motionXField = env->GetFieldID(entityClass, "motionX", "D"); }
    env->ExceptionClear();

    jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
    if (!motionYField) { env->ExceptionClear(); motionYField = env->GetFieldID(entityClass, "motionY", "D"); }
    env->ExceptionClear();

    jfieldID motionZField = env->GetFieldID(entityClass, "x", "D");
    if (!motionZField) { env->ExceptionClear(); motionZField = env->GetFieldID(entityClass, "motionZ", "D"); }
    env->ExceptionClear();

    jfieldID onGroundField = env->GetFieldID(entityClass, "C", "Z");
    if (!onGroundField) { env->ExceptionClear(); onGroundField = env->GetFieldID(entityClass, "onGround", "Z"); }
    env->ExceptionClear();

    jfieldID yawField = env->GetFieldID(entityClass, "y", "F");
    if (!yawField) { env->ExceptionClear(); yawField = env->GetFieldID(entityClass, "rotationYaw", "F"); }
    env->ExceptionClear();

    if (!motionXField || !motionYField || !motionZField || !onGroundField || !yawField)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }

    bool forward = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool backward = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool left = (GetAsyncKeyState('A') & 0x8000) != 0;
    bool right = (GetAsyncKeyState('D') & 0x8000) != 0;

    int forwardV = (forward ? 1 : 0) - (backward ? 1 : 0);
    int strafeV = (right ? 1 : 0) - (left ? 1 : 0);

    if (forwardV == 0 && strafeV == 0)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }

    bool onGround = env->GetBooleanField(player, onGroundField);
    double mx = env->GetDoubleField(player, motionXField);
    double mz = env->GetDoubleField(player, motionZField);
    double currentSpeed = std::sqrt(mx * mx + mz * mz);

    float yaw = env->GetFloatField(player, yawField);
    float rad = yaw * 0.017453292f;
    double sin = std::sin(rad);
    double cos = std::cos(rad);

    double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
    double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
    double len = std::sqrt(targetX * targetX + targetZ * targetZ);

    if (len > 0.0)
    {
        targetX /= len;
        targetZ /= len;
        env->SetDoubleField(player, motionXField, targetX * currentSpeed);
        env->SetDoubleField(player, motionZField, targetZ * currentSpeed);
    }

    if (autoJump && onGround)
        env->SetDoubleField(player, motionYField, 0.42);

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

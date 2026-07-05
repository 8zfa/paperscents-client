#include "liquidwalk.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <jni.h>
#include <cmath>
#include <Windows.h>

LiquidWalkModule::LiquidWalkModule()
    : ModuleBase("LiquidWalk", "Walk on water and lava", Category::Movement)
{
    AddSetting<NumberSetting>("Speed", 0.3f, 0.1f, 1.0f, 0.1f, "Movement speed on liquids");
}

void LiquidWalkModule::OnEnable() { Logger::Log("LiquidWalk enabled"); }
void LiquidWalkModule::OnDisable() { Logger::Log("LiquidWalk disabled"); }

void LiquidWalkModule::OnUpdate()
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

    float speed = ((NumberSetting*)FindSetting("Speed"))->GetValue();

    jmethodID isInWater = env->GetMethodID(entityClass, "H", "()Z");
    if (!isInWater) { env->ExceptionClear(); isInWater = env->GetMethodID(entityClass, "isInWater", "()Z"); }
    env->ExceptionClear();

    jmethodID isInLava = env->GetMethodID(entityClass, "I", "()Z");
    if (!isInLava) { env->ExceptionClear(); isInLava = env->GetMethodID(entityClass, "isInLava", "()Z"); }
    env->ExceptionClear();

    jmethodID isSneaking = env->GetMethodID(entityClass, "n", "()Z");
    if (!isSneaking) { env->ExceptionClear(); isSneaking = env->GetMethodID(entityClass, "isSneaking", "()Z"); }
    env->ExceptionClear();

    bool inWater = isInWater ? env->CallBooleanMethod(player, isInWater) : false;
    env->ExceptionClear();
    bool inLava = isInLava ? env->CallBooleanMethod(player, isInLava) : false;
    env->ExceptionClear();
    bool sneaking = isSneaking ? env->CallBooleanMethod(player, isSneaking) : false;
    env->ExceptionClear();

    if ((!inWater && !inLava) || sneaking)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }

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

    if (!motionXField || !motionYField || !motionZField || !onGroundField)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }

    bool forward = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool backward = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool left = (GetAsyncKeyState('A') & 0x8000) != 0;
    bool right = (GetAsyncKeyState('D') & 0x8000) != 0;
    bool jump = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    int forwardV = (forward ? 1 : 0) - (backward ? 1 : 0);
    int strafeV = (right ? 1 : 0) - (left ? 1 : 0);

    if (forwardV != 0 || strafeV != 0)
    {
        float yaw = yawField ? env->GetFloatField(player, yawField) : 0.0f;
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
            env->SetDoubleField(player, motionXField, targetX * speed);
            env->SetDoubleField(player, motionZField, targetZ * speed);
        }
    }

    if (jump)
        env->SetDoubleField(player, motionYField, speed);

    env->SetBooleanField(player, onGroundField, JNI_TRUE);

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

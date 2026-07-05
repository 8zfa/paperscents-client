#include "bhop.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <jni.h>
#include <cmath>
#include <Windows.h>

BHopModule::BHopModule()
    : ModuleBase("BHop", "Bunny hop for speed", Category::Movement)
{
    AddSetting<NumberSetting>("Speed", 1.0f, 0.1f, 5.0f, 0.1f, "Speed multiplier");
}

void BHopModule::OnEnable() { Logger::Log("BHop enabled"); }
void BHopModule::OnDisable()
{
    Logger::Log("BHop disabled");
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    env->ExceptionClear();

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jclass mcClass = StrayCache::Minecraft;
    if (!mcClass) { env->DeleteLocalRef(mc); return; }

    jfieldID timerField = env->GetFieldID(mcClass, "am", "Lavk;");
    if (!timerField) { env->ExceptionClear(); timerField = env->GetFieldID(mcClass, "timer", "Lavk;"); }
    env->ExceptionClear();

    if (!timerField) { env->DeleteLocalRef(mc); return; }

    jobject timerObj = env->GetObjectField(mc, timerField);
    if (timerObj)
    {
        jclass timerClass = env->FindClass("avk");
        if (!timerClass) { env->ExceptionClear(); timerClass = env->FindClass("net/minecraft/util/Timer"); }
        env->ExceptionClear();

        if (timerClass)
        {
            jfieldID timerSpeedField = env->GetFieldID(timerClass, "a", "F");
            if (!timerSpeedField) { env->ExceptionClear(); timerSpeedField = env->GetFieldID(timerClass, "timerSpeed", "F"); }
            env->ExceptionClear();

            if (timerSpeedField)
                env->SetFloatField(timerObj, timerSpeedField, 1.0f);

            env->DeleteLocalRef(timerClass);
        }
        env->DeleteLocalRef(timerObj);
    }

    env->DeleteLocalRef(mc);
}

void BHopModule::OnUpdate()
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

    float speedMult = ((NumberSetting*)FindSetting("Speed"))->GetValue();

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

    if (!motionXField || !motionYField || !motionZField || !onGroundField)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }

    bool onGround = env->GetBooleanField(player, onGroundField);
    bool forward = (GetAsyncKeyState('W') & 0x8000) != 0;

    if (onGround && forward)
        env->SetDoubleField(player, motionYField, 0.42);

    double mx = env->GetDoubleField(player, motionXField);
    double mz = env->GetDoubleField(player, motionZField);
    double currentSpeed = std::sqrt(mx * mx + mz * mz);

    if (currentSpeed > 0.0)
    {
        double newSpeed = currentSpeed * speedMult;
        env->SetDoubleField(player, motionXField, (mx / currentSpeed) * newSpeed);
        env->SetDoubleField(player, motionZField, (mz / currentSpeed) * newSpeed);
    }

    // Timer speed boost
    jclass mcClass = StrayCache::Minecraft;
    if (mcClass)
    {
        jfieldID timerField = env->GetFieldID(mcClass, "am", "Lavk;");
        if (!timerField) { env->ExceptionClear(); timerField = env->GetFieldID(mcClass, "timer", "Lavk;"); }
        env->ExceptionClear();

        if (timerField)
        {
            jobject timerObj = env->GetObjectField(mc, timerField);
            if (timerObj)
            {
                jclass timerClass = env->FindClass("avk");
                if (!timerClass) { env->ExceptionClear(); timerClass = env->FindClass("net/minecraft/util/Timer"); }
                env->ExceptionClear();

                if (timerClass)
                {
                    jfieldID timerSpeedField = env->GetFieldID(timerClass, "a", "F");
                    if (!timerSpeedField) { env->ExceptionClear(); timerSpeedField = env->GetFieldID(timerClass, "timerSpeed", "F"); }
                    env->ExceptionClear();

                    if (timerSpeedField)
                    {
                        float boost = 1.0f + (speedMult - 1.0f) * 0.5f;
                        env->SetFloatField(timerObj, timerSpeedField, boost);
                    }

                    env->DeleteLocalRef(timerClass);
                }
                env->DeleteLocalRef(timerObj);
            }
        }
    }

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

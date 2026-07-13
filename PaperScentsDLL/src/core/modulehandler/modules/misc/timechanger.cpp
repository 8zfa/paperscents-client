#include "timechanger.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"

TimeChangerModule::TimeChangerModule()
    : ModuleBase("TimeChanger", "Change in-game time", Category::Misc)
{
    AddSetting<EnumSetting>("Time", 0, std::vector<std::string>{"Day", "Night", "Sunset", "Custom"});
    AddSetting<NumberSetting>("CustomTime", 6000.0f, 0.0f, 24000.0f, 1000.0f, "Custom world time");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void TimeChangerModule::OnEnable() { Logger::Log("TimeChanger enabled"); }
void TimeChangerModule::OnDisable() { Logger::Log("TimeChanger disabled"); }

void TimeChangerModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jclass mcClass = env->GetObjectClass(mc);
    jfieldID worldField = env->GetFieldID(mcClass, "f", "Ladm;");
    if (!worldField) { env->ExceptionClear(); worldField = env->GetFieldID(mcClass, "theWorld", "Lnet/minecraft/world/World;"); }
    env->ExceptionClear();

    if (!worldField) { env->DeleteLocalRef(mcClass); env->DeleteLocalRef(mc); return; }

    jobject world = env->GetObjectField(mc, worldField);
    if (!world) { env->DeleteLocalRef(mcClass); env->DeleteLocalRef(mc); return; }

    jclass worldClass = env->GetObjectClass(world);
    jmethodID setTime = env->GetMethodID(worldClass, "a", "(J)V");
    if (!setTime) { env->ExceptionClear(); setTime = env->GetMethodID(worldClass, "setWorldTime", "(J)V"); }
    env->ExceptionClear();

    if (!setTime) { env->DeleteLocalRef(worldClass); env->DeleteLocalRef(world); env->DeleteLocalRef(mcClass); env->DeleteLocalRef(mc); return; }

    int mode = ((EnumSetting*)FindSetting("Time"))->GetValue();
    jlong timeValue = 6000;

    switch (mode)
    {
    case 0: timeValue = 6000; break;
    case 1: timeValue = 18000; break;
    case 2: timeValue = 12000; break;
    case 3: timeValue = (jlong)((NumberSetting*)FindSetting("CustomTime"))->GetValue(); break;
    }

    env->CallVoidMethod(world, setTime, timeValue);

    env->DeleteLocalRef(worldClass);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(mcClass);
    env->DeleteLocalRef(mc);
}

#include "timer.h"
#include "../../../java/java.h"
#include "../../../sdk/strayCache.h"
#include "../../../utilities/logger.h"

TimerModule::TimerModule()
    : ModuleBase("Timer", "Change game speed", Category::Movement)
{
    AddSetting<NumberSetting>("Speed", 1.0f, 0.1f, 10.0f, 0.1f, "Game speed multiplier");
}

void TimerModule::OnEnable() { Logger::Log("Timer enabled"); }

void TimerModule::OnDisable()
{
    Logger::Log("Timer disabled");
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!StrayCache::Minecraft || !StrayCache::MinecraftInstance) return;

    jfieldID timerField = env->GetFieldID(StrayCache::Minecraft, "am", "Lavk;");
    if (!timerField) { env->ExceptionClear(); timerField = env->GetFieldID(StrayCache::Minecraft, "timer", "Lavk;"); }
    if (!timerField) return;
    env->ExceptionClear();

    jobject timer = env->GetObjectField(StrayCache::MinecraftInstance, timerField);
    if (!timer) return;

    jclass timerClass = env->GetObjectClass(timer);
    jfieldID speedField = env->GetFieldID(timerClass, "a", "F");
    if (!speedField) { env->ExceptionClear(); speedField = env->GetFieldID(timerClass, "timerSpeed", "F"); }
    env->ExceptionClear();

    if (speedField)
        env->SetFloatField(timer, speedField, 1.0f);

    env->DeleteLocalRef(timerClass);
    env->DeleteLocalRef(timer);
}

void TimerModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!StrayCache::Minecraft || !StrayCache::MinecraftInstance) return;

    jfieldID timerField = env->GetFieldID(StrayCache::Minecraft, "am", "Lavk;");
    if (!timerField) { env->ExceptionClear(); timerField = env->GetFieldID(StrayCache::Minecraft, "timer", "Lavk;"); }
    if (!timerField) return;
    env->ExceptionClear();

    jobject timer = env->GetObjectField(StrayCache::MinecraftInstance, timerField);
    if (!timer) return;

    jclass timerClass = env->GetObjectClass(timer);
    jfieldID speedField = env->GetFieldID(timerClass, "a", "F");
    if (!speedField) { env->ExceptionClear(); speedField = env->GetFieldID(timerClass, "timerSpeed", "F"); }
    env->ExceptionClear();

    if (speedField)
    {
        float speed = ((NumberSetting*)FindSetting("Speed"))->GetValue();
        env->SetFloatField(timer, speedField, speed);
    }

    env->DeleteLocalRef(timerClass);
    env->DeleteLocalRef(timer);
}

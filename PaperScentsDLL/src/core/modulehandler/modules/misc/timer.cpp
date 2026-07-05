#include "timer.h"
#include "../../../core.h"
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
    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    jclass mcClass = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) { env->ExceptionClear(); mcClass = env->FindClass("net/minecraft/client/Minecraft"); }
    if (!mcClass) return;

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "a", "()Lave;"); }
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "getMinecraft", "()Lave;"); }
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    env->ExceptionClear();

    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    jfieldID timerField = env->GetFieldID(mcClass, "am", "Lavk;");
    if (!timerField) { env->ExceptionClear(); timerField = env->GetFieldID(mcClass, "timer", "Lavk;"); }
    env->ExceptionClear();

    if (timerField)
    {
        jobject timer = env->GetObjectField(mc, timerField);
        if (timer)
        {
            jclass timerClass = env->GetObjectClass(timer);
            jfieldID speedField = env->GetFieldID(timerClass, "a", "F");
            if (!speedField) { env->ExceptionClear(); speedField = env->GetFieldID(timerClass, "timerSpeed", "F"); }
            env->ExceptionClear();

            if (speedField)
                env->SetFloatField(timer, speedField, 1.0f);
            env->DeleteLocalRef(timerClass);
            env->DeleteLocalRef(timer);
        }
    }

    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

void TimerModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    jclass mcClass = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) { env->ExceptionClear(); mcClass = env->FindClass("net/minecraft/client/Minecraft"); }
    if (!mcClass) return;

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "a", "()Lave;"); }
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "getMinecraft", "()Lave;"); }
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    env->ExceptionClear();

    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    jfieldID timerField = env->GetFieldID(mcClass, "am", "Lavk;");
    if (!timerField) { env->ExceptionClear(); timerField = env->GetFieldID(mcClass, "timer", "Lavk;"); }
    env->ExceptionClear();

    if (timerField)
    {
        jobject timer = env->GetObjectField(mc, timerField);
        if (timer)
        {
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
    }

    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

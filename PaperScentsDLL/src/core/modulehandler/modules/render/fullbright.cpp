#include "fullbright.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"

FullBrightModule::FullBrightModule()
    : ModuleBase("FullBright", "Full brightness", Category::Render)
{
    AddSetting<NumberSetting>("Gamma", 100.0f, 1.0f, 100.0f, 1.0f);
}

void FullBrightModule::OnEnable()
{
    Logger::Log("FullBright enabled");
    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    jclass mcClass = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass)
    {
        env->ExceptionClear();
        mcClass = env->FindClass("net/minecraft/client/Minecraft");
    }
    if (!mcClass) return;

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc)
    {
        env->ExceptionClear();
        getMc = env->GetStaticMethodID(mcClass, "a", "()Lave;");
    }
    if (!getMc)
    {
        env->ExceptionClear();
        getMc = env->GetStaticMethodID(mcClass, "getMinecraft", "()Lave;");
    }
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    env->ExceptionClear();

    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    jfieldID gsField = env->GetFieldID(mcClass, "t", "Lavh;");
    if (!gsField)
    {
        env->ExceptionClear();
        gsField = env->GetFieldID(mcClass, "gameSettings", "Lavh;");
    }
    env->ExceptionClear();

    if (gsField)
    {
        jobject gs = env->GetObjectField(mc, gsField);
        if (gs)
        {
            jclass gsClass = env->GetObjectClass(gs);
            jfieldID gammaField = env->GetFieldID(gsClass, "v", "F");
            if (!gammaField)
            {
                env->ExceptionClear();
                gammaField = env->GetFieldID(gsClass, "gammaSetting", "F");
            }
            env->ExceptionClear();

            if (gammaField)
            {
                float val = ((NumberSetting*)FindSetting("Gamma"))->GetValue();
                env->SetFloatField(gs, gammaField, val);
            }
            env->DeleteLocalRef(gsClass);
            env->DeleteLocalRef(gs);
        }
    }

    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

void FullBrightModule::OnDisable()
{
    Logger::Log("FullBright disabled");
    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    jclass mcClass = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass)
    {
        env->ExceptionClear();
        mcClass = env->FindClass("net/minecraft/client/Minecraft");
    }
    if (!mcClass) return;

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc)
    {
        env->ExceptionClear();
        getMc = env->GetStaticMethodID(mcClass, "a", "()Lave;");
    }
    if (!getMc)
    {
        env->ExceptionClear();
        getMc = env->GetStaticMethodID(mcClass, "getMinecraft", "()Lave;");
    }
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    env->ExceptionClear();

    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    jfieldID gsField = env->GetFieldID(mcClass, "t", "Lavh;");
    if (!gsField)
    {
        env->ExceptionClear();
        gsField = env->GetFieldID(mcClass, "gameSettings", "Lavh;");
    }
    env->ExceptionClear();

    if (gsField)
    {
        jobject gs = env->GetObjectField(mc, gsField);
        if (gs)
        {
            jclass gsClass = env->GetObjectClass(gs);
            jfieldID gammaField = env->GetFieldID(gsClass, "v", "F");
            if (!gammaField)
            {
                env->ExceptionClear();
                gammaField = env->GetFieldID(gsClass, "gammaSetting", "F");
            }
            env->ExceptionClear();

            if (gammaField)
                env->SetFloatField(gs, gammaField, 1.0f);
            env->DeleteLocalRef(gsClass);
            env->DeleteLocalRef(gs);
        }
    }

    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

void FullBrightModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    jclass mcClass = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass)
    {
        env->ExceptionClear();
        mcClass = env->FindClass("net/minecraft/client/Minecraft");
    }
    if (!mcClass) return;

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc)
    {
        env->ExceptionClear();
        getMc = env->GetStaticMethodID(mcClass, "a", "()Lave;");
    }
    if (!getMc)
    {
        env->ExceptionClear();
        getMc = env->GetStaticMethodID(mcClass, "getMinecraft", "()Lave;");
    }
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    env->ExceptionClear();

    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    jfieldID gsField = env->GetFieldID(mcClass, "t", "Lavh;");
    if (!gsField)
    {
        env->ExceptionClear();
        gsField = env->GetFieldID(mcClass, "gameSettings", "Lavh;");
    }
    env->ExceptionClear();

    if (gsField)
    {
        jobject gs = env->GetObjectField(mc, gsField);
        if (gs)
        {
            jclass gsClass = env->GetObjectClass(gs);
            jfieldID gammaField = env->GetFieldID(gsClass, "v", "F");
            if (!gammaField)
            {
                env->ExceptionClear();
                gammaField = env->GetFieldID(gsClass, "gammaSetting", "F");
            }
            env->ExceptionClear();

            if (gammaField)
            {
                float current = env->GetFloatField(gs, gammaField);
                if (current < 50.0f)
                {
                    float val = ((NumberSetting*)FindSetting("Gamma"))->GetValue();
                    env->SetFloatField(gs, gammaField, val);
                }
            }
            env->DeleteLocalRef(gsClass);
            env->DeleteLocalRef(gs);
        }
    }

    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

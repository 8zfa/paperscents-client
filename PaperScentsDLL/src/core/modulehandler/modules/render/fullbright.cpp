#include "fullbright.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../sdk/strayCache.h"

FullBrightModule::FullBrightModule()
    : ModuleBase("FullBright", "Full brightness", Category::Render)
{
    AddSetting<NumberSetting>("Gamma", 100.0f, 1.0f, 100.0f, 1.0f);
}

void FullBrightModule::OnEnable()
{
    Logger::Log("FullBright enabled");
    SetGamma();
}

void FullBrightModule::OnDisable()
{
    Logger::Log("FullBright disabled");
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!StrayCache::Minecraft || !StrayCache::MinecraftInstance) return;
    jfieldID gsField = env->GetFieldID(StrayCache::Minecraft, "t", "Lavh;");
    if (!gsField) { env->ExceptionClear(); gsField = env->GetFieldID(StrayCache::Minecraft, "gameSettings", "Lavh;"); }
    if (!gsField) return;
    env->ExceptionClear();
    jobject gs = env->GetObjectField(StrayCache::MinecraftInstance, gsField);
    if (!gs) return;
    jclass gsClass = env->GetObjectClass(gs);
    jfieldID gammaField = env->GetFieldID(gsClass, "v", "F");
    if (!gammaField) { env->ExceptionClear(); gammaField = env->GetFieldID(gsClass, "gammaSetting", "F"); }
    if (gammaField) env->SetFloatField(gs, gammaField, 1.0f);
    env->DeleteLocalRef(gsClass);
    env->DeleteLocalRef(gs);
}

void FullBrightModule::SetGamma()
{
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!StrayCache::Minecraft || !StrayCache::MinecraftInstance) return;
    jfieldID gsField = env->GetFieldID(StrayCache::Minecraft, "t", "Lavh;");
    if (!gsField) { env->ExceptionClear(); gsField = env->GetFieldID(StrayCache::Minecraft, "gameSettings", "Lavh;"); }
    if (!gsField) return;
    env->ExceptionClear();
    jobject gs = env->GetObjectField(StrayCache::MinecraftInstance, gsField);
    if (!gs) return;
    jclass gsClass = env->GetObjectClass(gs);
    jfieldID gammaField = env->GetFieldID(gsClass, "v", "F");
    if (!gammaField) { env->ExceptionClear(); gammaField = env->GetFieldID(gsClass, "gammaSetting", "F"); }
    env->ExceptionClear();
    if (gammaField)
    {
        float val = ((NumberSetting*)FindSetting("Gamma"))->GetValue();
        env->SetFloatField(gs, gammaField, val);
    }
    env->DeleteLocalRef(gsClass);
    env->DeleteLocalRef(gs);
}

void FullBrightModule::OnUpdate()
{
    if (!IsEnabled()) return;
    SetGamma();
}

#include "step.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"

StepModule::StepModule()
    : ModuleBase("Step", "Step up full blocks", Category::Movement)
{
    AddSetting<NumberSetting>("Height", 2.0f, 1.0f, 10.0f, 1.0f, "Block height to step");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void StepModule::OnEnable() { Logger::Log("Step enabled"); }

void StepModule::OnDisable()
{
    Logger::Log("Step disabled");
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!BridgeHelper::Initialize(env)) return;

    jobject player = BridgeHelper::GetPlayer(env);
    if (!player) return;

    // Step height via notch fallback (vanilla only)
    jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
    if (entityClass)
    {
        jfieldID stepField = env->GetFieldID(entityClass, "stepHeight", "F");
        if (!stepField) env->ExceptionClear();
        if (stepField) { env->ExceptionClear(); env->SetFloatField(player, stepField, 0.5f); }
        env->DeleteLocalRef(entityClass);
    }
    env->DeleteLocalRef(player);
}

void StepModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!BridgeHelper::Initialize(env)) return;

    float height = ((NumberSetting*)FindSetting("Height"))->GetValue();
    jobject player = BridgeHelper::GetPlayer(env);
    if (!player) return;

    // Step height via notch fallback (vanilla only — no bridge setter exists)
    jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
    if (entityClass)
    {
        jfieldID stepField = env->GetFieldID(entityClass, "stepHeight", "F");
        if (!stepField) env->ExceptionClear();
        if (stepField) { env->ExceptionClear(); env->SetFloatField(player, stepField, height); }
        env->DeleteLocalRef(entityClass);
    }
    env->DeleteLocalRef(player);
}

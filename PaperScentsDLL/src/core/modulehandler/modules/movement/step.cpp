#include "step.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <jni.h>

StepModule::StepModule()
    : ModuleBase("Step", "Step up full blocks", Category::Movement)
{
    AddSetting<NumberSetting>("Height", 2.0f, 1.0f, 10.0f, 1.0f, "Block height to step");
}

void StepModule::OnEnable()
{
    Logger::Log("Step enabled");
}

void StepModule::OnDisable()
{
    Logger::Log("Step disabled");

    Java* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return;
    JNIEnv* env = java->GetEnv();
    if (!env) return;

    jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) return;
    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    const char* playerFields[] = { "s", "t", "h", "thePlayer" };
    jobject player = nullptr;
    jfieldID pf = nullptr;
    for (auto f : playerFields)
    {
        pf = env->GetFieldID(mcClass, f, "Lbew;");
        if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
        if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
    }

    if (player)
    {
        jclass entityClass = java->FindClass("vg", "net/minecraft/entity/Entity");
        if (entityClass)
        {
            jfieldID stepField = env->GetFieldID(entityClass, "y", "F");
            if (!stepField) stepField = env->GetFieldID(entityClass, "stepHeight", "F");
            if (!stepField) stepField = env->GetFieldID(entityClass, "z", "F");
            if (stepField)
                env->SetFloatField(player, stepField, 0.5f);
            env->DeleteLocalRef(entityClass);
        }
        env->DeleteLocalRef(player);
    }

    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

void StepModule::OnUpdate()
{
    if (!IsEnabled()) return;
    Java* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return;
    JNIEnv* env = java->GetEnv();
    if (!env) return;
    env->ExceptionClear();

    float height = ((NumberSetting*)FindSetting("Height"))->GetValue();

    jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) return;
    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    const char* playerFields[] = { "s", "t", "h", "thePlayer" };
    jobject player = nullptr;
    jfieldID pf = nullptr;
    for (auto f : playerFields)
    {
        pf = env->GetFieldID(mcClass, f, "Lbew;");
        if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
        if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
    }
    if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass entityClass = java->FindClass("vg", "net/minecraft/entity/Entity");
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    // Set step height - try multiple field names
    jfieldID stepField = env->GetFieldID(entityClass, "y", "F");
    if (!stepField) stepField = env->GetFieldID(entityClass, "stepHeight", "F");
    if (!stepField) stepField = env->GetFieldID(entityClass, "z", "F");
    if (!stepField) stepField = env->GetFieldID(entityClass, "A", "F");

    if (stepField)
        env->SetFloatField(player, stepField, height);

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

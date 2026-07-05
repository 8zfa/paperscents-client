#include "reach.h"
#include "../../../utilities/logger.h"
#include "../../../core.h"
#include <jni.h>

ReachModule::ReachModule()
    : ModuleBase("Reach", "Extends attack reach", Category::Combat)
{
    AddSetting<NumberSetting>("Reach", 3.5f, 3.0f, 6.0f, 0.1f, "Attack reach distance");
}

void ReachModule::OnEnable() { Logger::Log("Reach enabled"); }
void ReachModule::OnDisable() { Logger::Log("Reach disabled"); }

void ReachModule::OnUpdate()
{
    if (!IsEnabled()) return;
    Java* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return;
    JNIEnv* env = java->GetEnv();
    if (!env) return;
    env->ExceptionClear();

    float reach = ((NumberSetting*)FindSetting("Reach"))->GetValue();

    jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) return;
    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) return;
    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) return;

    const char* pcFields[] = { "v", "field_71442_b", "playerController" };
    jobject pc = nullptr;
    jfieldID pf = nullptr;
    for (auto f : pcFields)
    {
        pf = env->GetFieldID(mcClass, f, "Lbhl;");
        if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/multiplayer/PlayerControllerMP;");
        if (pf) { pc = env->GetObjectField(mc, pf); if (pc) break; }
    }
    if (pc)
    {
        jclass pcClass = env->GetObjectClass(pc);
        const char* rf[] = { "g", "field_78747_a", "blockReachDistance" };
        for (auto f : rf)
        {
            jfieldID reachField = env->GetFieldID(pcClass, f, "F");
            if (reachField) { env->SetFloatField(pc, reachField, reach); break; }
        }
        env->DeleteLocalRef(pcClass);
        env->DeleteLocalRef(pc);
    }
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

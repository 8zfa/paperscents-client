#include "reach.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../sdk/strayCache.h"

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
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!StrayCache::Minecraft || !StrayCache::MinecraftInstance) return;

    float reach = ((NumberSetting*)FindSetting("Reach"))->GetValue();

    const char* pcFields[] = { "v", "field_71442_b", "playerController" };
    jobject pc = nullptr;
    jfieldID pf = nullptr;
    for (auto f : pcFields)
    {
        pf = env->GetFieldID(StrayCache::Minecraft, f, "Lbhl;");
        if (!pf) pf = env->GetFieldID(StrayCache::Minecraft, f, "Lnet/minecraft/client/multiplayer/PlayerControllerMP;");
        if (pf) { pc = env->GetObjectField(StrayCache::MinecraftInstance, pf); if (pc) break; }
    }
    if (!pc) return;

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

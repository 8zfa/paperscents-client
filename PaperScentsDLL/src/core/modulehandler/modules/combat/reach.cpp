#include "reach.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../sdk/bridgeHelper.h"
#include <cstdlib>

ReachModule::ReachModule()
    : ModuleBase("Reach", "Extends attack reach", Category::Combat)
{
    AddSetting<NumberSetting>("Reach", 3.5f, 3.0f, 6.0f, 0.1f);
    AddSetting<NumberSetting>("Chance", 100.0f, 0.0f, 100.0f, 1.0f);
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void ReachModule::OnEnable() { Logger::Log("Reach enabled"); m_WasDown = false; }
void ReachModule::OnDisable() { Logger::Log("Reach disabled"); m_WasDown = false; }

void ReachModule::OnUpdate()
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

    float reach = ((NumberSetting*)FindSetting("Reach"))->GetValue();
    float chance = ((NumberSetting*)FindSetting("Chance"))->GetValue();
    if (chance < 100.0f && (rand() % 100) >= (int)chance) return;

    // Set blockReachDistance via PlayerController bridge
    if (BridgeHelper::McBridge_GetPlayerController)
    {
        jobject pc = BridgeHelper::GetPlayerController(env);
        if (pc)
        {
            jclass pcClass = env->GetObjectClass(pc);
            const char* brf[] = { "g", "blockReachDistance", "field_78747_a" };
            jfieldID brField = nullptr;
            for (auto f : brf)
            {
                brField = env->GetFieldID(pcClass, f, "F");
                if (brField) { env->ExceptionClear(); break; }
                env->ExceptionClear();
            }
            if (brField) env->SetFloatField(pc, brField, reach);
            env->DeleteLocalRef(pcClass);
            env->DeleteLocalRef(pc);
        }
    }

    bool isDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    if (isDown && !m_WasDown)
    {
        if (!BridgeHelper::McBridge_GetPlayer || !BridgeHelper::EntityBridge_GetWorld ||
            !BridgeHelper::WorldBridge_GetEntities || !BridgeHelper::EntityBridge_GetDistanceToEntity)
        { m_WasDown = isDown; return; }

        jobject player = BridgeHelper::GetPlayer(env);
        if (!player) { m_WasDown = isDown; return; }

        jobject world = BridgeHelper::GetWorldFromEntity(env, player);
        if (!world) { env->DeleteLocalRef(player); m_WasDown = isDown; return; }

        jobject entityList = env->CallObjectMethod(world, BridgeHelper::WorldBridge_GetEntities);
        if (!entityList) { env->DeleteLocalRef(world); env->DeleteLocalRef(player); m_WasDown = isDown; return; }

        jclass listClass = env->FindClass("java/util/List");
        jmethodID listSize = env->GetMethodID(listClass, "size", "()I");
        jmethodID listGet = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
        env->DeleteLocalRef(listClass);

        if (!listSize || !listGet) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(world); env->DeleteLocalRef(player); m_WasDown = isDown; return; }

        jint sz = env->CallIntMethod(entityList, listSize);
        jobject best = nullptr;
        double bestDist = reach + 1.0;

        for (jint i = 0; i < sz; i++)
        {
            jobject entity = env->CallObjectMethod(entityList, listGet, i);
            if (!entity) continue;
            if (env->IsSameObject(player, entity)) { env->DeleteLocalRef(entity); continue; }
            double dist = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetDistanceToEntity, entity);
            if (dist > reach) { env->DeleteLocalRef(entity); continue; }
            if (dist < bestDist) { bestDist = dist; if (best) env->DeleteLocalRef(best); best = entity; }
            else env->DeleteLocalRef(entity);
        }

        env->DeleteLocalRef(entityList);

        if (best)
        {
            BridgeHelper::SimulateAttack(env);
            env->DeleteLocalRef(best);
        }

        env->DeleteLocalRef(world);
        env->DeleteLocalRef(player);
    }
    m_WasDown = isDown;
}

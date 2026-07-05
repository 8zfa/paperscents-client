#include "ka.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <jni.h>
#include <algorithm>
#include <cmath>
#include <chrono>

KillAuraModule::KillAuraModule()
    : ModuleBase("KillAura", "Attacks entities automatically", Category::Combat)
{
    AddSetting<NumberSetting>("Range", 4.2f, 1.0f, 6.0f, 0.1f, "Attack range");
    AddSetting<NumberSetting>("CPS", 10.0f, 1.0f, 20.0f, 1.0f, "Clicks per second");
    AddSetting<BooleanSetting>("Players", true, "Target players");
    AddSetting<BooleanSetting>("Mobs", false, "Target mobs");
    AddSetting<BooleanSetting>("AutoBlock", false, "Auto-block with sword");
    AddSetting<BooleanSetting>("ThroughWalls", false, "Attack through walls");
    m_LastAttack = std::chrono::steady_clock::now();
}

void KillAuraModule::OnEnable() { Logger::Log("KillAura enabled"); }
void KillAuraModule::OnDisable() { Logger::Log("KillAura disabled"); }

void KillAuraModule::OnUpdate()
{
    if (!IsEnabled()) return;

    auto now = std::chrono::steady_clock::now();
    float cps = ((NumberSetting*)FindSetting("CPS"))->GetValue();
    int delayMs = (int)(1000.0f / (std::max)(cps, 1.0f));
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastAttack).count() < delayMs)
        return;

    Java* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return;
    JNIEnv* env = java->GetEnv();
    if (!env) return;

    jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) { env->ExceptionClear(); return; }

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Leave;");
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    const char* worldFields[] = { "f", "field_71441_e", "theWorld" };
    jfieldID wf = nullptr;
    for (auto f : worldFields)
    {
        wf = env->GetFieldID(mcClass, f, "Ladm;");
        if (!wf) wf = env->GetFieldID(mcClass, f, "Lnet/minecraft/world/World;");
        if (wf) break;
    }
    if (!wf) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    jobject world = env->GetObjectField(mc, wf);
    if (!world) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    const char* playerFields[] = { "u", "field_71439_g", "thePlayer" };
    jfieldID pf = nullptr;
    for (auto f : playerFields)
    {
        pf = env->GetFieldID(mcClass, f, "Lbew;");
        if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
        if (pf) break;
    }
    if (!pf) { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    jobject player = env->GetObjectField(mc, pf);
    if (!player) { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    const char* pcFields[] = { "v", "field_71442_b", "playerController" };
    jfieldID pcf = nullptr;
    for (auto f : pcFields)
    {
        pcf = env->GetFieldID(mcClass, f, "Lbhl;");
        if (!pcf) pcf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/multiplayer/PlayerControllerMP;");
        if (pcf) break;
    }
    if (!pcf) { env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    jobject pc = env->GetObjectField(mc, pcf);
    if (!pc) { env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass worldClass = java->FindClass("adm", "net/minecraft/world/World");
    if (!worldClass) { env->DeleteLocalRef(pc); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    const char* worldEntityFields[] = { "e", "field_73010_i", "playerEntities" };
    jfieldID wef = nullptr;
    for (auto f : worldEntityFields)
    {
        wef = env->GetFieldID(worldClass, f, "Ljava/util/List;");
        if (wef) break;
    }
    if (!wef) { env->DeleteLocalRef(worldClass); env->DeleteLocalRef(pc); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    jobject entityList = env->GetObjectField(world, wef);
    if (!entityList) { env->DeleteLocalRef(worldClass); env->DeleteLocalRef(pc); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass listClass = env->FindClass("java/util/List");
    if (!listClass) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(worldClass); env->DeleteLocalRef(pc); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    jmethodID listSize = env->GetMethodID(listClass, "size", "()I");
    jmethodID listGet = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    if (!listSize || !listGet) { env->DeleteLocalRef(listClass); env->DeleteLocalRef(entityList); env->DeleteLocalRef(worldClass); env->DeleteLocalRef(pc); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jint size = env->CallIntMethod(entityList, listSize);
    if (size <= 0) { env->DeleteLocalRef(listClass); env->DeleteLocalRef(entityList); env->DeleteLocalRef(worldClass); env->DeleteLocalRef(pc); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    float range = ((NumberSetting*)FindSetting("Range"))->GetValue();
    bool targetPlayers = ((BooleanSetting*)FindSetting("Players"))->GetValue();
    bool targetMobs = ((BooleanSetting*)FindSetting("Mobs"))->GetValue();
    bool throughWalls = ((BooleanSetting*)FindSetting("ThroughWalls"))->GetValue();

    jclass entityClass = java->FindClass("pk", "net/minecraft/entity/Entity");
    if (!entityClass) { env->DeleteLocalRef(listClass); env->DeleteLocalRef(entityList); env->DeleteLocalRef(worldClass); env->DeleteLocalRef(pc); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jmethodID getEntityId = env->GetMethodID(entityClass, "q", "()I");
    jmethodID getDistance = env->GetMethodID(entityClass, "a", "(Lpk;)D");
    if (!getEntityId || !getDistance) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(listClass); env->DeleteLocalRef(entityList); env->DeleteLocalRef(worldClass); env->DeleteLocalRef(pc); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jint playerId = env->CallIntMethod(player, getEntityId);

    jfieldID isDeadField = env->GetFieldID(entityClass, "J", "Z");

    jmethodID canSee = env->GetMethodID(entityClass, "b", "(Lpk;)Z");

    jclass playerClass = java->FindClass("wn", "net/minecraft/entity/player/EntityPlayer");

    jobject bestTarget = nullptr;
    double bestDist = range;

    for (jint i = 0; i < size; i++)
    {
        jobject entity = env->CallObjectMethod(entityList, listGet, i);
        if (!entity) continue;

        jint eid = env->CallIntMethod(entity, getEntityId);
        if (eid == playerId) { env->DeleteLocalRef(entity); continue; }

        if (isDeadField)
        {
            jboolean dead = env->GetBooleanField(entity, isDeadField);
            if (dead) { env->DeleteLocalRef(entity); continue; }
        }

        bool isPlayer = playerClass ? (env->IsInstanceOf(entity, playerClass) == JNI_TRUE) : true;

        if (targetPlayers && !targetMobs)
        {
            if (!isPlayer) { env->DeleteLocalRef(entity); continue; }
        }
        else if (!targetPlayers && targetMobs)
        {
            if (isPlayer) { env->DeleteLocalRef(entity); continue; }
        }
        else if (!targetPlayers && !targetMobs)
        {
            env->DeleteLocalRef(entity);
            continue;
        }

        jdouble dist = env->CallDoubleMethod(player, getDistance, entity);
        if (dist > range || dist >= bestDist) { env->DeleteLocalRef(entity); continue; }

        if (!throughWalls && canSee)
        {
            jboolean seen = env->CallBooleanMethod(player, canSee, entity);
            if (!seen) { env->DeleteLocalRef(entity); continue; }
        }

        if (bestTarget) env->DeleteLocalRef(bestTarget);
        bestTarget = entity;
        bestDist = dist;
    }

    if (playerClass) env->DeleteLocalRef(playerClass);
    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(worldClass);

    if (bestTarget)
    {
        jclass pcClass = env->GetObjectClass(pc);

        const char* attackNames[] = { "a", "b", "attackEntity" };
        jmethodID attackMethod = nullptr;
        for (auto n : attackNames)
        {
            attackMethod = env->GetMethodID(pcClass, n, "(Lwn;Lpk;)V");
            if (!attackMethod) attackMethod = env->GetMethodID(pcClass, n, "(Lnet/minecraft/client/entity/EntityPlayerSP;Lnet/minecraft/entity/Entity;)V");
            if (attackMethod) break;
        }

        if (attackMethod)
        {
            env->CallVoidMethod(pc, attackMethod, player, bestTarget);
            m_LastAttack = std::chrono::steady_clock::now();
        }

        jclass playerObjClass = env->GetObjectClass(player);
        jmethodID swingMethod = nullptr;
        const char* swingNames[] = { "a", "b", "d", "i", "swingItem" };
        for (auto n : swingNames)
        {
            swingMethod = env->GetMethodID(playerObjClass, n, "()V");
            if (swingMethod) break;
        }
        if (swingMethod)
            env->CallVoidMethod(player, swingMethod);

        env->DeleteLocalRef(playerObjClass);
        env->DeleteLocalRef(pcClass);
        env->DeleteLocalRef(bestTarget);
    }

    env->DeleteLocalRef(entityList);
    env->DeleteLocalRef(pc);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

#include "triggerbot.h"
#include "../../../utilities/logger.h"
#include "../../../java/java.h"
#include "../../../utilities/jni_helpers.h"
#include <algorithm>

TriggerBotModule::TriggerBotModule()
    : ModuleBase("TriggerBot", "Automatically attacks when crosshair is on entity", Category::Combat)
{
    AddSetting<NumberSetting>("Range", 4.0f, 1.0f, 6.0f, 0.1f, "Attack range");
    AddSetting<BooleanSetting>("Players", true, "Target players");
    AddSetting<BooleanSetting>("Mobs", false, "Target mobs");
    AddSetting<NumberSetting>("Delay", 0.0f, 0.0f, 500.0f, 50.0f, "Delay between attacks in ms");
    m_LastAttack = std::chrono::steady_clock::now();
}

void TriggerBotModule::OnEnable() { Logger::Log("TriggerBot enabled"); }
void TriggerBotModule::OnDisable() { Logger::Log("TriggerBot disabled"); }

void TriggerBotModule::OnUpdate()
{
    if (!IsEnabled()) return;

    auto now = std::chrono::steady_clock::now();
    int delayMs = (int)((NumberSetting*)FindSetting("Delay"))->GetValue();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastAttack).count() < delayMs)
        return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jclass mcCls = env->GetObjectClass(mc);
    if (!mcCls) { env->DeleteLocalRef(mc); return; }

    const char* omoFields[] = { "e", "field_71476_x", "objectMouseOver" };
    jfieldID omoFid = nullptr;
    for (auto f : omoFields)
    {
        omoFid = env->GetFieldID(mcCls, f, "Lazk;");
        if (!omoFid) { env->ExceptionClear(); omoFid = env->GetFieldID(mcCls, f, "Lnet/minecraft/util/MovingObjectPosition;"); env->ExceptionClear(); }
        if (omoFid) break;
    }
    if (!omoFid) { env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    jobject mop = env->GetObjectField(mc, omoFid);
    env->DeleteLocalRef(mcCls);
    if (!mop) { env->DeleteLocalRef(mc); return; }

    jclass mopCls = env->GetObjectClass(mop);
    const char* typeFields[] = { "a", "field_78430_b", "typeOfHit" };
    jfieldID typeFid = nullptr;
    for (auto f : typeFields)
    {
        typeFid = env->GetFieldID(mopCls, f, "Lazl;");
        if (!typeFid) { env->ExceptionClear(); typeFid = env->GetFieldID(mopCls, f, "Lnet/minecraft/util/MovingObjectPosition$MovingObjectType;"); env->ExceptionClear(); }
        if (typeFid) break;
    }
    if (!typeFid) { env->DeleteLocalRef(mopCls); env->DeleteLocalRef(mop); env->DeleteLocalRef(mc); return; }

    jobject typeObj = env->GetObjectField(mop, typeFid);
    env->DeleteLocalRef(mopCls);
    if (!typeObj) { env->DeleteLocalRef(mop); env->DeleteLocalRef(mc); return; }

    jclass enumCls = env->GetObjectClass(typeObj);
    jmethodID ordinal = env->GetMethodID(enumCls, "ordinal", "()I");
    jint typeOrdinal = ordinal ? env->CallIntMethod(typeObj, ordinal) : -1;
    env->DeleteLocalRef(enumCls);
    env->DeleteLocalRef(typeObj);

    if (typeOrdinal != 0) { env->DeleteLocalRef(mop); env->DeleteLocalRef(mc); return; }

    const char* entityHitFields[] = { "d", "field_78707_b", "entityHit" };
    jfieldID hitFid = nullptr;
    for (auto f : entityHitFields)
    {
        hitFid = env->GetFieldID(mopCls, f, "Lpk;");
        if (!hitFid) { env->ExceptionClear(); hitFid = env->GetFieldID(mopCls, f, "Lnet/minecraft/entity/Entity;"); env->ExceptionClear(); }
        if (hitFid) break;
    }
    env->DeleteLocalRef(mopCls);

    if (!hitFid) { env->DeleteLocalRef(mop); env->DeleteLocalRef(mc); return; }
    jobject hitEntity = env->GetObjectField(mop, hitFid);
    env->DeleteLocalRef(mop);
    if (!hitEntity) { env->DeleteLocalRef(mc); return; }

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(hitEntity); env->DeleteLocalRef(mc); return; }

    float range = ((NumberSetting*)FindSetting("Range"))->GetValue();
    bool targetPlayers = ((BooleanSetting*)FindSetting("Players"))->GetValue();
    bool targetMobs = ((BooleanSetting*)FindSetting("Mobs"))->GetValue();

    jclass entityClass = env->GetObjectClass(hitEntity);
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(hitEntity); env->DeleteLocalRef(mc); return; }

    jclass entityPlayerClass = nullptr;
    jmethodID getMc = env->GetStaticMethodID(env->FindClass("ave"), "A", "()Lave;");
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(env->FindClass("net/minecraft/client/Minecraft"), "getMinecraft", "()Lnet/minecraft/client/Minecraft;"); env->ExceptionClear(); }
    if (getMc) entityPlayerClass = env->FindClass("wn");

    bool isPlayer = entityPlayerClass ? (env->IsInstanceOf(hitEntity, entityPlayerClass) == JNI_TRUE) : false;

    if (targetPlayers && !targetMobs) { if (!isPlayer) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(player); env->DeleteLocalRef(hitEntity); env->DeleteLocalRef(mc); return; } }
    else if (!targetPlayers && targetMobs) { if (isPlayer) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(player); env->DeleteLocalRef(hitEntity); env->DeleteLocalRef(mc); return; } }
    else if (!targetPlayers && !targetMobs) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(player); env->DeleteLocalRef(hitEntity); env->DeleteLocalRef(mc); return; }

    jmethodID getDistance = env->GetMethodID(entityClass, "a", "(Lpk;)D");
    if (!getDistance) { env->ExceptionClear(); getDistance = env->GetMethodID(entityClass, "getDistanceToEntity", "(Lnet/minecraft/entity/Entity;)D"); env->ExceptionClear(); }

    if (!getDistance) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(player); env->DeleteLocalRef(hitEntity); env->DeleteLocalRef(mc); return; }

    jdouble dist = env->CallDoubleMethod(player, getDistance, hitEntity);
    if (dist > range) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(player); env->DeleteLocalRef(hitEntity); env->DeleteLocalRef(mc); return; }

    jclass pcClass = env->FindClass("bhl");
    if (!pcClass) { env->ExceptionClear(); pcClass = env->FindClass("net/minecraft/client/multiplayer/PlayerControllerMP"); env->ExceptionClear(); }

    if (pcClass)
    {
        const char* pcFields[] = { "v", "field_71442_b", "playerController" };
        jfieldID pcFid = nullptr;
        jclass mcObjClass = env->GetObjectClass(mc);
        for (auto f : pcFields)
        {
            pcFid = env->GetFieldID(mcObjClass, f, "Lbhl;");
            if (!pcFid) { env->ExceptionClear(); pcFid = env->GetFieldID(mcObjClass, f, "Lnet/minecraft/client/multiplayer/PlayerControllerMP;"); env->ExceptionClear(); }
            if (pcFid) break;
        }
        if (pcFid)
        {
            jobject pc = env->GetObjectField(mc, pcFid);
            if (pc)
            {
                const char* attackNames[] = { "a", "b", "attackEntity" };
                jmethodID attackMethod = nullptr;
                for (auto n : attackNames)
                {
                    attackMethod = env->GetMethodID(pcClass, n, "(Lwn;Lpk;)V");
                    if (!attackMethod) { env->ExceptionClear(); attackMethod = env->GetMethodID(pcClass, n, "(Lnet/minecraft/client/entity/EntityPlayerSP;Lnet/minecraft/entity/Entity;)V"); env->ExceptionClear(); }
                    if (attackMethod) break;
                }
                if (attackMethod)
                {
                    env->CallVoidMethod(pc, attackMethod, player, hitEntity);
                    m_LastAttack = now;
                }
                env->DeleteLocalRef(pc);
            }
        }
        env->DeleteLocalRef(mcObjClass);
    }

    jclass playerObjClass = env->GetObjectClass(player);
    if (playerObjClass)
    {
        const char* swingNames[] = { "a", "b", "d", "i", "swingItem" };
        jmethodID swingMethod = nullptr;
        for (auto n : swingNames)
        {
            swingMethod = env->GetMethodID(playerObjClass, n, "()V");
            if (swingMethod) break;
        }
        if (swingMethod)
            env->CallVoidMethod(player, swingMethod);
        env->DeleteLocalRef(playerObjClass);
    }

    if (entityPlayerClass) env->DeleteLocalRef(entityPlayerClass);
    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(hitEntity);
    env->DeleteLocalRef(mc);
}

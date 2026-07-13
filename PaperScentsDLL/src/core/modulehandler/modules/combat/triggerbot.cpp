#include "triggerbot.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <algorithm>

TriggerBotModule::TriggerBotModule()
    : ModuleBase("TriggerBot", "Automatically attacks when crosshair is on entity", Category::Combat)
{
    AddSetting<NumberSetting>("Range", 4.0f, 1.0f, 6.0f, 0.1f, "Attack range");
    AddSetting<BooleanSetting>("Players", true, "Target players");
    AddSetting<BooleanSetting>("Mobs", false, "Target mobs");
    AddSetting<NumberSetting>("Delay", 0.0f, 0.0f, 500.0f, 50.0f, "Delay between attacks in ms");
    m_LastAttack = std::chrono::steady_clock::now();
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void TriggerBotModule::OnEnable() { Logger::Log("TriggerBot enabled"); }
void TriggerBotModule::OnDisable() { Logger::Log("TriggerBot disabled"); }

void TriggerBotModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }
    auto now = std::chrono::steady_clock::now();
    int delayMs = (int)((NumberSetting*)FindSetting("Delay"))->GetValue();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastAttack).count() < delayMs) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    env->ExceptionClear();

    bool bridged = BridgeHelper::Initialize(env);
    jobject player = bridged ? BridgeHelper::GetPlayer(env) : nullptr;
    jobject mc = nullptr;

    if (!player)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java || !java->IsValid()) return;
        jclass mcCls = java->FindClass("ave", "net/minecraft/client/Minecraft");
        if (!mcCls) return;
        jmethodID getMc = env->GetStaticMethodID(mcCls, "A", "()Lave;");
        if (!getMc) { env->DeleteLocalRef(mcCls); return; }
        mc = env->CallStaticObjectMethod(mcCls, getMc);
        if (!mc) { env->DeleteLocalRef(mcCls); return; }
        for (auto f : {"s", "t", "h", "thePlayer"})
        {
            jfieldID pf = env->GetFieldID(mcCls, f, "Lbew;");
            if (!pf) pf = env->GetFieldID(mcCls, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
            if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
        }
        if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcCls); return; }
    }
    else
    {
        mc = BridgeHelper::GetMinecraftInstance(env);
    }

    // Get objectMouseOver
    jclass mcCls = env->GetObjectClass(mc);
    const char* omoFields[] = { "e", "field_71476_x", "objectMouseOver" };
    jfieldID omoFid = nullptr;
    for (auto f : omoFields)
    {
        omoFid = env->GetFieldID(mcCls, f, "Lazk;");
        if (!omoFid) { env->ExceptionClear(); omoFid = env->GetFieldID(mcCls, f, "Lnet/minecraft/util/MovingObjectPosition;"); env->ExceptionClear(); }
        if (omoFid) break;
    }
    if (!omoFid) { env->DeleteLocalRef(mcCls); if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; }

    jobject mop = env->GetObjectField(mc, omoFid);
    env->DeleteLocalRef(mcCls);
    if (!mop) { if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; }

    jclass mopCls = env->GetObjectClass(mop);
    const char* typeFields[] = { "a", "field_78430_b", "typeOfHit" };
    jfieldID typeFid = nullptr;
    for (auto f : typeFields)
    {
        typeFid = env->GetFieldID(mopCls, f, "Lazl;");
        if (!typeFid) { env->ExceptionClear(); typeFid = env->GetFieldID(mopCls, f, "Lnet/minecraft/util/MovingObjectPosition$MovingObjectType;"); env->ExceptionClear(); }
        if (typeFid) break;
    }
    if (!typeFid) { env->DeleteLocalRef(mopCls); env->DeleteLocalRef(mop); if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; }

    jobject typeObj = env->GetObjectField(mop, typeFid);
    env->DeleteLocalRef(mopCls);
    if (!typeObj) { env->DeleteLocalRef(mop); if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; }

    jclass enumCls = env->GetObjectClass(typeObj);
    jmethodID ordinal = env->GetMethodID(enumCls, "ordinal", "()I");
    jint typeOrdinal = ordinal ? env->CallIntMethod(typeObj, ordinal) : -1;
    env->DeleteLocalRef(enumCls);
    env->DeleteLocalRef(typeObj);

    if (typeOrdinal != 0) { env->DeleteLocalRef(mop); if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; }

    const char* entityHitFields[] = { "d", "field_78707_b", "entityHit" };
    jfieldID hitFid = nullptr;
    for (auto f : entityHitFields)
    {
        hitFid = env->GetFieldID(mopCls, f, "Lpk;");
        if (!hitFid) { env->ExceptionClear(); hitFid = env->GetFieldID(mopCls, f, "Lnet/minecraft/entity/Entity;"); env->ExceptionClear(); }
        if (hitFid) break;
    }
    env->DeleteLocalRef(mopCls);

    if (!hitFid) { env->DeleteLocalRef(mop); if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; }
    jobject hitEntity = env->GetObjectField(mop, hitFid);
    env->DeleteLocalRef(mop);
    if (!hitEntity) { if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; }

    float range = ((NumberSetting*)FindSetting("Range"))->GetValue();
    bool targetPlayers = ((BooleanSetting*)FindSetting("Players"))->GetValue();
    bool targetMobs = ((BooleanSetting*)FindSetting("Mobs"))->GetValue();

    jclass entityClass = env->GetObjectClass(hitEntity);
    if (!entityClass) { env->DeleteLocalRef(hitEntity); if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; }

    // Entity type check
    jclass entityPlayerClass = env->FindClass("net/minecraft/entity/player/EntityPlayer");
    if (!entityPlayerClass) { env->ExceptionClear(); entityPlayerClass = env->FindClass("wn"); }
    bool isPlayer = entityPlayerClass ? (env->IsInstanceOf(hitEntity, entityPlayerClass) == JNI_TRUE) : false;

    if (targetPlayers && !targetMobs) { if (!isPlayer) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(entityPlayerClass); env->DeleteLocalRef(hitEntity); if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; } }
    else if (!targetPlayers && targetMobs) { if (isPlayer) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(entityPlayerClass); env->DeleteLocalRef(hitEntity); if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; } }
    else if (!targetPlayers && !targetMobs) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(entityPlayerClass); env->DeleteLocalRef(hitEntity); if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; }

    jmethodID getDistance = env->GetMethodID(entityClass, "getDistanceToEntity", "(Lnet/minecraft/entity/Entity;)D");
    if (!getDistance) { env->ExceptionClear(); getDistance = env->GetMethodID(entityClass, "a", "(Lpk;)D"); }
    if (!getDistance) { env->ExceptionClear(); env->DeleteLocalRef(entityClass); env->DeleteLocalRef(entityPlayerClass); env->DeleteLocalRef(hitEntity); if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; }

    jdouble dist = env->CallDoubleMethod(player, getDistance, hitEntity);
    if (dist > range) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(entityPlayerClass); env->DeleteLocalRef(hitEntity); if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); } return; }

    // Attack via bridge or notch
    if (bridged)
    {
        BridgeHelper::SimulateAttack(env);
    }
    else
    {
        jclass pcClass = env->FindClass("net/minecraft/client/multiplayer/PlayerControllerMP");
        if (!pcClass) { env->ExceptionClear(); pcClass = env->FindClass("bhl"); }
        if (pcClass)
        {
            jclass mcObjClass = env->GetObjectClass(mc);
            jfieldID pcFid = env->GetFieldID(mcObjClass, "v", "Lbhl;");
            if (!pcFid) { env->ExceptionClear(); pcFid = env->GetFieldID(mcObjClass, "playerController", "Lnet/minecraft/client/multiplayer/PlayerControllerMP;"); }
            if (pcFid)
            {
                jobject pc = env->GetObjectField(mc, pcFid);
                if (pc)
                {
                    jmethodID attackMethod = env->GetMethodID(pcClass, "attackEntity", "(Lnet/minecraft/client/entity/EntityPlayerSP;Lnet/minecraft/entity/Entity;)V");
                    if (!attackMethod) { env->ExceptionClear(); attackMethod = env->GetMethodID(pcClass, "a", "(Lwn;Lpk;)V"); }
                    if (attackMethod) { env->ExceptionClear(); env->CallVoidMethod(pc, attackMethod, player, hitEntity); }
                    env->DeleteLocalRef(pc);
                }
            }
            env->DeleteLocalRef(mcObjClass);
        }
    }

    // Swing item
    jclass playerObjClass = env->GetObjectClass(player);
    jmethodID swingMethod = env->GetMethodID(playerObjClass, "swingItem", "()V");
    if (!swingMethod) { env->ExceptionClear(); swingMethod = env->GetMethodID(playerObjClass, "d", "()V"); }
    if (swingMethod) { env->ExceptionClear(); env->CallVoidMethod(player, swingMethod); }
    env->DeleteLocalRef(playerObjClass);

    m_LastAttack = now;

    if (entityPlayerClass) env->DeleteLocalRef(entityPlayerClass);
    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(hitEntity);
    if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); }
}

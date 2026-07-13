#include "ka.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../sdk/bridgeHelper.h"
#include <cmath>

KillAuraModule::KillAuraModule()
    : ModuleBase("KillAura", "Attacks entities automatically", Category::Combat)
{
    AddSetting<NumberSetting>("Range", 4.2f, 1.0f, 6.0f, 0.1f);
    AddSetting<NumberSetting>("CPS", 10.0f, 1.0f, 20.0f, 1.0f);
    AddSetting<NumberSetting>("FOV", 180.0f, 30.0f, 360.0f, 5.0f);
    AddSetting<BooleanSetting>("Players", true);
    AddSetting<BooleanSetting>("Mobs", false);
    AddSetting<BooleanSetting>("ThroughWalls", false);
    AddSetting<BooleanSetting>("AutoBlock", false);
    AddSetting<BooleanSetting>("WeaponOnly", true);
    m_LastAttack = std::chrono::steady_clock::now();
    m_Blocking = false;
    m_CurrentTargetId = -1;
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void KillAuraModule::OnEnable() { Logger::Log("KillAura enabled"); m_Blocking = false; }

void KillAuraModule::OnDisable()
{
    Logger::Log("KillAura disabled");
    Unblock();
    m_Blocking = false;
    m_CurrentTargetId = -1;
}

static bool CheckFOV(JNIEnv* env, jobject player, jobject entity, float maxFov)
{
    if (maxFov >= 360.0f) return true;
    if (!BridgeHelper::EntityBridge_GetRotationYaw || !BridgeHelper::EntityBridge_GetRotationPitch ||
        !BridgeHelper::EntityBridge_GetPosX || !BridgeHelper::EntityBridge_GetPosY || !BridgeHelper::EntityBridge_GetPosZ)
        return true;

    float pYaw = (float)env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetRotationYaw);
    float pPitch = (float)env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetRotationPitch);
    double eX = env->CallDoubleMethod(entity, BridgeHelper::EntityBridge_GetPosX);
    double eY = env->CallDoubleMethod(entity, BridgeHelper::EntityBridge_GetPosY);
    double eZ = env->CallDoubleMethod(entity, BridgeHelper::EntityBridge_GetPosZ);
    double pX = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetPosX);
    double pY = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetPosY) + 1.62;
    double pZ = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetPosZ);

    double dX = eX - pX;
    double dY = (eY + (eY - pY) * 0.5) - pY;
    double dZ = eZ - pZ;
    double dist = std::sqrt(dX * dX + dY * dY + dZ * dZ);
    if (dist < 0.1) return true;

    float tYaw = (float)(std::atan2(dZ, dX) * 180.0 / 3.14159265) - 90.0f;
    float tPitch = -(float)(std::asin(dY / dist) * 180.0 / 3.14159265);
    float dYaw = std::abs(tYaw - pYaw);
    if (dYaw > 180) dYaw = 360 - dYaw;
    float dPitch = std::abs(tPitch - pPitch);
    return dYaw <= maxFov && dPitch <= maxFov * 0.5f;
}

bool KillAuraModule::IsValidEntity(JNIEnv* env, jobject entity, jobject player, float range, int fov, bool throughWalls)
{
    if (!entity) return false;

    if (BridgeHelper::EntityBridge_IsDead && env->CallBooleanMethod(entity, BridgeHelper::EntityBridge_IsDead))
        return false;

    if (BridgeHelper::EntityBridge_GetEntityId && env->CallIntMethod(entity, BridgeHelper::EntityBridge_GetEntityId) == m_CurrentTargetId)
        return false;

    if (BridgeHelper::EntityBridge_GetDistanceToEntity)
    {
        double dist = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetDistanceToEntity, entity);
        if (dist > range || dist < 0.1) return false;
    }

    if (!CheckFOV(env, player, entity, (float)fov)) return false;

    return true;
}

void KillAuraModule::Unblock()
{
    JNIEnv* env = Java::GetThreadEnv();
    if (!env || !m_Blocking) return;
    if (!BridgeHelper::Initialize(env)) return;

    jobject gs = BridgeHelper::GetGameSettings(env);
    if (gs && BridgeHelper::GSBridge_KeyBindUseItem && BridgeHelper::GSBridge_SetKeyBindState)
    {
        jobject kb = env->CallObjectMethod(gs, BridgeHelper::GSBridge_KeyBindUseItem);
        if (kb)
        {
            env->CallVoidMethod(gs, BridgeHelper::GSBridge_SetKeyBindState, kb, JNI_FALSE);
            env->DeleteLocalRef(kb);
        }
    }
    if (gs) env->DeleteLocalRef(gs);
    m_Blocking = false;
}

void KillAuraModule::Block(JNIEnv* env)
{
    if (!BridgeHelper::Initialize(env)) return;

    jobject gs = BridgeHelper::GetGameSettings(env);
    if (gs && BridgeHelper::GSBridge_KeyBindUseItem && BridgeHelper::GSBridge_SetKeyBindState)
    {
        jobject kb = env->CallObjectMethod(gs, BridgeHelper::GSBridge_KeyBindUseItem);
        if (kb)
        {
            env->CallVoidMethod(gs, BridgeHelper::GSBridge_SetKeyBindState, kb, JNI_TRUE);
            env->DeleteLocalRef(kb);
        }
    }
    if (gs) env->DeleteLocalRef(gs);
    m_Blocking = true;
}

void KillAuraModule::Attack(JNIEnv* env, jobject entity)
{
    if (!entity) return;
    BridgeHelper::SimulateAttack(env);
    m_LastAttack = std::chrono::steady_clock::now();
}

void KillAuraModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }
    auto now = std::chrono::steady_clock::now();
    float cps = ((NumberSetting*)FindSetting("CPS"))->GetValue();
    int delayMs = (int)(1000.0f / (std::max)(cps, 1.0f));
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastAttack).count() < delayMs)
    {
        if (m_Blocking) Unblock();
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!BridgeHelper::Initialize(env)) return;

    jobject player = BridgeHelper::GetPlayer(env);
    if (!player) return;

    jobject world = BridgeHelper::GetWorldFromEntity(env, player);
    if (!world) { env->DeleteLocalRef(player); return; }

    float range = ((NumberSetting*)FindSetting("Range"))->GetValue();
    int fov = (int)((NumberSetting*)FindSetting("FOV"))->GetValue();
    bool targetPlayers = ((BooleanSetting*)FindSetting("Players"))->GetValue();
    bool targetMobs = ((BooleanSetting*)FindSetting("Mobs"))->GetValue();
    bool throughWalls = ((BooleanSetting*)FindSetting("ThroughWalls"))->GetValue();
    bool autoBlock = ((BooleanSetting*)FindSetting("AutoBlock"))->GetValue();
    bool weaponOnly = ((BooleanSetting*)FindSetting("WeaponOnly"))->GetValue();

    if (!BridgeHelper::WorldBridge_GetEntities) { env->DeleteLocalRef(world); env->DeleteLocalRef(player); return; }
    jobject entityList = env->CallObjectMethod(world, BridgeHelper::WorldBridge_GetEntities);
    if (!entityList) { env->DeleteLocalRef(world); env->DeleteLocalRef(player); return; }

    jclass listClass = env->FindClass("java/util/List");
    jmethodID listSize = env->GetMethodID(listClass, "size", "()I");
    jmethodID listGet = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    env->DeleteLocalRef(listClass);
    if (!listSize || !listGet) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(world); env->DeleteLocalRef(player); return; }

    if (weaponOnly)
    {
        bool hasWeapon = false;
        if (BridgeHelper::ELBridge_GetHeldItem)
        {
            jobject heldStack = env->CallObjectMethod(player, BridgeHelper::ELBridge_GetHeldItem);
            if (heldStack && BridgeHelper::ItemStackBridge_GetItem)
            {
                jobject item = env->CallObjectMethod(heldStack, BridgeHelper::ItemStackBridge_GetItem);
                if (item)
                {
                    if (BridgeHelper::ItemSwordBridge && env->IsInstanceOf(item, BridgeHelper::ItemSwordBridge))
                        hasWeapon = true;
                    env->DeleteLocalRef(item);
                }
            }
            if (heldStack) env->DeleteLocalRef(heldStack);
        }
        if (!hasWeapon) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(world); env->DeleteLocalRef(player); return; }
    }

    jint size = env->CallIntMethod(entityList, listSize);
    jobject bestTarget = nullptr;
    double bestDist = range;

    for (jint i = 0; i < size; i++)
    {
        jobject entity = env->CallObjectMethod(entityList, listGet, i);
        if (!entity) continue;
        if (env->IsSameObject(player, entity)) { env->DeleteLocalRef(entity); continue; }

        bool isPlayer = BridgeHelper::EntityPlayerSPBridge && env->IsInstanceOf(entity, BridgeHelper::EntityPlayerSPBridge);
        bool isMob = BridgeHelper::EntityMobBridge && env->IsInstanceOf(entity, BridgeHelper::EntityMobBridge);
        bool isAnimal = BridgeHelper::EntityAnimalBridge && env->IsInstanceOf(entity, BridgeHelper::EntityAnimalBridge);

        bool valid = false;
        if (targetPlayers && isPlayer) valid = true;
        else if (targetMobs && (isMob || isAnimal)) valid = true;
        if (!valid) { env->DeleteLocalRef(entity); continue; }

        if (!IsValidEntity(env, entity, player, range, fov, throughWalls)) { env->DeleteLocalRef(entity); continue; }

        if (BridgeHelper::EntityBridge_GetDistanceToEntity)
        {
            double dist = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetDistanceToEntity, entity);
            if (dist < bestDist)
            {
                if (bestTarget) env->DeleteLocalRef(bestTarget);
                bestTarget = entity;
                bestDist = dist;
            }
            else env->DeleteLocalRef(entity);
        }
        else env->DeleteLocalRef(entity);
    }

    if (bestTarget)
    {
        Attack(env, bestTarget);
        if (autoBlock && BridgeHelper::GSBridge_KeyBindUseItem) Block(env);
        else if (m_Blocking) Unblock();
        env->DeleteLocalRef(bestTarget);
    }
    else if (m_Blocking) Unblock();

    env->DeleteLocalRef(entityList);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(player);
}

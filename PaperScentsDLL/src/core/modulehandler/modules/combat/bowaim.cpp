#include "bowaim.h"
#include "../../../core.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../utilities/logger.h"
#include <jni.h>
#include <cmath>
#include <string>

BowAimModule::BowAimModule()
    : ModuleBase("BowAim", "Auto-aims bow at entities", Category::Combat)
{
    AddSetting<BooleanSetting>("Players", true);
    AddSetting<BooleanSetting>("Mobs", false);
    AddSetting<NumberSetting>("Range", 50.0f, 10.0f, 100.0f, 1.0f);
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void BowAimModule::OnEnable() { Logger::Log("BowAim enabled"); }
void BowAimModule::OnDisable() { Logger::Log("BowAim disabled"); }

void BowAimModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;
    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    Java* java = Core::GetInstance().GetJava();

    jclass playerCls = env->GetObjectClass(player);
    if (!playerCls) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jmethodID usingItemMid = env->GetMethodID(playerCls, "e", "()Z");
    if (!usingItemMid) { env->ExceptionClear(); usingItemMid = env->GetMethodID(playerCls, "isUsingItem", "()Z"); }
    env->ExceptionClear();
    if (!usingItemMid) { env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jboolean usingItem = env->CallBooleanMethod(player, usingItemMid);
    if (!usingItem) { env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jmethodID heldItemMid = env->GetMethodID(playerCls, "b", "()Ljava/lang/ItemStack;");
    if (!heldItemMid) { env->ExceptionClear(); heldItemMid = env->GetMethodID(playerCls, "getHeldItem", "()Ljava/lang/ItemStack;"); }
    env->ExceptionClear();
    if (!heldItemMid) { env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jobject itemStack = env->CallObjectMethod(player, heldItemMid);
    if (!itemStack) { env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jclass stackCls = env->GetObjectClass(itemStack);
    if (!stackCls) { env->DeleteLocalRef(itemStack); env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jmethodID getItemMid = env->GetMethodID(stackCls, "c", "()Lqa;");
    if (!getItemMid) { env->ExceptionClear(); getItemMid = env->GetMethodID(stackCls, "getItem", "()Lnet/minecraft/item/Item;"); }
    env->ExceptionClear();
    if (!getItemMid) { env->DeleteLocalRef(stackCls); env->DeleteLocalRef(itemStack); env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jobject item = env->CallObjectMethod(itemStack, getItemMid);
    if (!item) { env->DeleteLocalRef(stackCls); env->DeleteLocalRef(itemStack); env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jclass itemCls = env->GetObjectClass(item);
    jmethodID unlocMid = env->GetMethodID(itemCls, "c", "()Ljava/lang/String;");
    if (!unlocMid) { env->ExceptionClear(); unlocMid = env->GetMethodID(itemCls, "getUnlocalizedName", "()Ljava/lang/String;"); }
    env->ExceptionClear();

    bool isBow = false;
    if (unlocMid)
    {
        jobject nameObj = env->CallObjectMethod(item, unlocMid);
        if (nameObj)
        {
            const char* str = env->GetStringUTFChars((jstring)nameObj, nullptr);
            if (str)
            {
                isBow = std::string(str).find("bow") != std::string::npos;
                env->ReleaseStringUTFChars((jstring)nameObj, str);
            }
            env->DeleteLocalRef(nameObj);
        }
    }

    env->DeleteLocalRef(itemCls);
    env->DeleteLocalRef(item);
    env->DeleteLocalRef(stackCls);
    env->DeleteLocalRef(itemStack);

    if (!isBow) { env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jobject world = GetWorldObject(env, mc);
    if (!world) { env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jclass worldCls = env->GetObjectClass(world);
    jfieldID efid = env->GetFieldID(worldCls, "e", "Ljava/util/List;");
    if (!efid) { env->ExceptionClear(); efid = env->GetFieldID(worldCls, "playerEntities", "Ljava/util/List;"); }
    env->ExceptionClear();
    if (!efid) { env->DeleteLocalRef(worldCls); env->DeleteLocalRef(world); env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jobject entityList = env->GetObjectField(world, efid);
    if (!entityList) { env->DeleteLocalRef(worldCls); env->DeleteLocalRef(world); env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jclass listCls = env->FindClass("java/util/List");
    if (!listCls) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(worldCls); env->DeleteLocalRef(world); env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jmethodID sizeMid = env->GetMethodID(listCls, "size", "()I");
    jmethodID getMid = env->GetMethodID(listCls, "get", "(I)Ljava/lang/Object;");
    if (!sizeMid || !getMid) { env->DeleteLocalRef(listCls); env->DeleteLocalRef(entityList); env->DeleteLocalRef(worldCls); env->DeleteLocalRef(world); env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jint size = env->CallIntMethod(entityList, sizeMid);
    if (size <= 0) { env->DeleteLocalRef(listCls); env->DeleteLocalRef(entityList); env->DeleteLocalRef(worldCls); env->DeleteLocalRef(world); env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jclass entityCls = java->FindClass("pk", "net/minecraft/entity/Entity");
    if (!entityCls) { env->DeleteLocalRef(listCls); env->DeleteLocalRef(entityList); env->DeleteLocalRef(worldCls); env->DeleteLocalRef(world); env->DeleteLocalRef(playerCls); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jmethodID eidMid = env->GetMethodID(entityCls, "q", "()I");
    if (!eidMid) { env->ExceptionClear(); eidMid = env->GetMethodID(entityCls, "getEntityId", "()I"); }
    env->ExceptionClear();

    jfieldID deadFid = env->GetFieldID(entityCls, "J", "Z");
    if (!deadFid) { env->ExceptionClear(); deadFid = env->GetFieldID(entityCls, "isDead", "Z"); }
    env->ExceptionClear();

    jfieldID posXFid = env->GetFieldID(entityCls, "r", "D");
    if (!posXFid) { env->ExceptionClear(); posXFid = env->GetFieldID(entityCls, "posX", "D"); }
    jfieldID posYFid = env->GetFieldID(entityCls, "s", "D");
    if (!posYFid) { env->ExceptionClear(); posYFid = env->GetFieldID(entityCls, "posY", "D"); }
    jfieldID posZFid = env->GetFieldID(entityCls, "t", "D");
    if (!posZFid) { env->ExceptionClear(); posZFid = env->GetFieldID(entityCls, "posZ", "D"); }
    env->ExceptionClear();

    jclass targetPlayerCls = java->FindClass("wn", "net/minecraft/entity/player/EntityPlayer");

    bool targetPlayers = ((BooleanSetting*)FindSetting("Players"))->GetValue();
    bool targetMobs = ((BooleanSetting*)FindSetting("Mobs"))->GetValue();
    float range = ((NumberSetting*)FindSetting("Range"))->GetValue();

    jint playerId = eidMid ? env->CallIntMethod(player, eidMid) : -1;
    double pX = posXFid ? env->GetDoubleField(player, posXFid) : 0.0;
    double pY = posYFid ? env->GetDoubleField(player, posYFid) : 0.0;
    double pZ = posZFid ? env->GetDoubleField(player, posZFid) : 0.0;

    jobject bestTarget = nullptr;
    double bestDist = range;

    for (jint i = 0; i < size; i++)
    {
        jobject entity = env->CallObjectMethod(entityList, getMid, i);
        if (!entity) continue;

        if (eidMid)
        {
            jint eid = env->CallIntMethod(entity, eidMid);
            if (eid == playerId) { env->DeleteLocalRef(entity); continue; }
        }

        if (deadFid)
        {
            jboolean dead = env->GetBooleanField(entity, deadFid);
            if (dead) { env->DeleteLocalRef(entity); continue; }
        }

        if (targetPlayerCls)
        {
            bool isPlayer = env->IsInstanceOf(entity, targetPlayerCls) == JNI_TRUE;
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
        }

        if (!posXFid || !posYFid || !posZFid) { env->DeleteLocalRef(entity); continue; }

        double eX = env->GetDoubleField(entity, posXFid);
        double eY = env->GetDoubleField(entity, posYFid);
        double eZ = env->GetDoubleField(entity, posZFid);

        double dx = eX - pX;
        double dy = eY - pY;
        double dz = eZ - pZ;
        double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist > range || dist >= bestDist) { env->DeleteLocalRef(entity); continue; }

        if (bestTarget) env->DeleteLocalRef(bestTarget);
        bestTarget = entity;
        bestDist = dist;
    }

    if (targetPlayerCls) env->DeleteLocalRef(targetPlayerCls);

    if (bestTarget && posXFid && posYFid && posZFid)
    {
        double tX = env->GetDoubleField(bestTarget, posXFid);
        double tY = env->GetDoubleField(bestTarget, posYFid);
        double tZ = env->GetDoubleField(bestTarget, posZFid);

        double dx = tX - pX;
        double dy = (tY + 1.2) - (pY + 1.62);
        double dz = tZ - pZ;
        double dist2D = std::sqrt(dx * dx + dz * dz);

        float yaw = -(float)(atan2(dx, dz) * 180.0f / 3.141592653589793f);
        float pitch = -(float)(atan2(dy, dist2D) * 180.0f / 3.141592653589793f);

        if (pitch > 90.0f) pitch = 90.0f;
        if (pitch < -90.0f) pitch = -90.0f;

        jfieldID yawFid = env->GetFieldID(entityCls, "v", "F");
        if (!yawFid) { env->ExceptionClear(); yawFid = env->GetFieldID(entityCls, "rotationYaw", "F"); }
        env->ExceptionClear();
        jfieldID pitchFid = env->GetFieldID(entityCls, "w", "F");
        if (!pitchFid) { env->ExceptionClear(); pitchFid = env->GetFieldID(entityCls, "rotationPitch", "F"); }
        env->ExceptionClear();

        if (yawFid) env->SetFloatField(player, yawFid, yaw);
        if (pitchFid) env->SetFloatField(player, pitchFid, pitch);

        env->DeleteLocalRef(bestTarget);
    }

    env->DeleteLocalRef(entityCls);
    env->DeleteLocalRef(listCls);
    env->DeleteLocalRef(entityList);
    env->DeleteLocalRef(worldCls);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(playerCls);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

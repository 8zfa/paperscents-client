#include "rodaura.h"
#include "../../../utilities/logger.h"
#include "../../../java/java.h"
#include "../../../utilities/jni_helpers.h"
#include <algorithm>
#include <string>

RodAuraModule::RodAuraModule()
    : ModuleBase("RodAura", "Automatically uses fishing rod on nearby entities", Category::Combat)
{
    AddSetting<NumberSetting>("Range", 4.0f, 1.0f, 6.0f, 0.1f, "Rod range");
    AddSetting<NumberSetting>("Delay", 1000.0f, 500.0f, 2000.0f, 50.0f, "Delay between rod uses in ms");
    m_LastUse = std::chrono::steady_clock::now();
}

void RodAuraModule::OnEnable() { Logger::Log("RodAura enabled"); }
void RodAuraModule::OnDisable() { Logger::Log("RodAura disabled"); }

void RodAuraModule::OnUpdate()
{
    if (!IsEnabled()) return;

    auto now = std::chrono::steady_clock::now();
    int delayMs = (int)((NumberSetting*)FindSetting("Delay"))->GetValue();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastUse).count() < delayMs)
        return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jclass playerClass = env->GetObjectClass(player);
    if (!playerClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    const char* heldItemNames[] = { "b", "field_71071_by", "getHeldItem" };
    jmethodID heldItemMethod = nullptr;
    for (auto n : heldItemNames)
    {
        heldItemMethod = env->GetMethodID(playerClass, n, "()Ladd;");
        if (!heldItemMethod) { env->ExceptionClear(); heldItemMethod = env->GetMethodID(playerClass, n, "()Lnet/minecraft/item/ItemStack;"); env->ExceptionClear(); }
        if (heldItemMethod) break;
    }
    if (!heldItemMethod) { env->DeleteLocalRef(playerClass); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jobject itemStack = env->CallObjectMethod(player, heldItemMethod);
    env->DeleteLocalRef(playerClass);

    if (!itemStack) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jclass itemStackClass = env->GetObjectClass(itemStack);
    if (!itemStackClass) { env->DeleteLocalRef(itemStack); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jmethodID getItemMethod = env->GetMethodID(itemStackClass, "a", "()Ladq;");
    if (!getItemMethod) { env->ExceptionClear(); getItemMethod = env->GetMethodID(itemStackClass, "getItem", "()Lnet/minecraft/item/Item;"); env->ExceptionClear(); }
    if (!getItemMethod) { env->DeleteLocalRef(itemStackClass); env->DeleteLocalRef(itemStack); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jobject item = env->CallObjectMethod(itemStack, getItemMethod);
    env->DeleteLocalRef(itemStackClass);
    env->DeleteLocalRef(itemStack);

    if (!item) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jclass itemClass = env->GetObjectClass(item);
    if (!itemClass) { env->DeleteLocalRef(item); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jmethodID getUnlocalizedName = env->GetMethodID(itemClass, "a", "()Ljava/lang/String;");
    if (!getUnlocalizedName) { env->ExceptionClear(); getUnlocalizedName = env->GetMethodID(itemClass, "getUnlocalizedName", "()Ljava/lang/String;"); env->ExceptionClear(); }

    bool isRod = false;
    if (getUnlocalizedName)
    {
        jstring nameStr = (jstring)env->CallObjectMethod(item, getUnlocalizedName);
        if (nameStr)
        {
            const char* nameChars = env->GetStringUTFChars(nameStr, nullptr);
            if (nameChars)
            {
                std::string name(nameChars);
                isRod = (name.find("fishingRod") != std::string::npos || name.find("rod") != std::string::npos);
                env->ReleaseStringUTFChars(nameStr, nameChars);
            }
            env->DeleteLocalRef(nameStr);
        }
    }
    env->DeleteLocalRef(itemClass);
    env->DeleteLocalRef(item);

    if (!isRod) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jobject world = GetWorldObject(env, mc);
    if (!world) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jclass worldClass = env->GetObjectClass(world);
    if (!worldClass) { env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    const char* entityListFields[] = { "e", "field_73010_i", "playerEntities" };
    jfieldID listFid = nullptr;
    for (auto f : entityListFields)
    {
        listFid = env->GetFieldID(worldClass, f, "Ljava/util/List;");
        if (listFid) break;
    }
    if (!listFid) { env->DeleteLocalRef(worldClass); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jobject entityList = env->GetObjectField(world, listFid);
    env->DeleteLocalRef(worldClass);
    if (!entityList) { env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jclass listClass = env->FindClass("java/util/List");
    if (!listClass) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jmethodID listSize = env->GetMethodID(listClass, "size", "()I");
    jmethodID listGet = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    if (!listSize || !listGet) { env->DeleteLocalRef(listClass); env->DeleteLocalRef(entityList); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jint size = env->CallIntMethod(entityList, listSize);
    if (size <= 0) { env->DeleteLocalRef(listClass); env->DeleteLocalRef(entityList); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jclass entityClass = env->FindClass("pk");
    if (!entityClass) { env->ExceptionClear(); entityClass = env->FindClass("net/minecraft/entity/Entity"); env->ExceptionClear(); }
    if (!entityClass) { env->DeleteLocalRef(listClass); env->DeleteLocalRef(entityList); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jmethodID getEntityId = env->GetMethodID(entityClass, "q", "()I");
    if (!getEntityId) { env->ExceptionClear(); getEntityId = env->GetMethodID(entityClass, "getEntityId", "()I"); env->ExceptionClear(); }
    jmethodID getDistance = env->GetMethodID(entityClass, "a", "(Lpk;)D");
    if (!getDistance) { env->ExceptionClear(); getDistance = env->GetMethodID(entityClass, "getDistanceToEntity", "(Lnet/minecraft/entity/Entity;)D"); env->ExceptionClear(); }
    jfieldID isDeadField = env->GetFieldID(entityClass, "J", "Z");
    if (!isDeadField) { env->ExceptionClear(); isDeadField = env->GetFieldID(entityClass, "isDead", "Z"); env->ExceptionClear(); }

    float range = ((NumberSetting*)FindSetting("Range"))->GetValue();

    jint playerId = 0;
    jmethodID getPlayerId = env->GetMethodID(entityClass, "q", "()I");
    if (!getPlayerId) { env->ExceptionClear(); getPlayerId = env->GetMethodID(entityClass, "getEntityId", "()I"); env->ExceptionClear(); }
    if (getPlayerId) playerId = env->CallIntMethod(player, getPlayerId);

    jobject bestTarget = nullptr;
    double bestDist = range;

    for (jint i = 0; i < size; i++)
    {
        jobject entity = env->CallObjectMethod(entityList, listGet, i);
        if (!entity) continue;

        jint eid = 0;
        if (getEntityId) eid = env->CallIntMethod(entity, getEntityId);
        if (eid == playerId) { env->DeleteLocalRef(entity); continue; }

        if (isDeadField)
        {
            jboolean dead = env->GetBooleanField(entity, isDeadField);
            if (dead) { env->DeleteLocalRef(entity); continue; }
        }

        jdouble dist = env->CallDoubleMethod(player, getDistance, entity);
        if (dist > range || dist >= bestDist) { env->DeleteLocalRef(entity); continue; }

        if (bestTarget) env->DeleteLocalRef(bestTarget);
        bestTarget = entity;
        bestDist = dist;
    }

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(entityList);
    env->DeleteLocalRef(world);

    if (bestTarget)
    {
        jclass mcObjClass = env->GetObjectClass(mc);
        if (mcObjClass)
        {
            const char* rightClickNames[] = { "d", "b", "rightClickMouse" };
            jmethodID rightClickMethod = nullptr;
            for (auto n : rightClickNames)
            {
                rightClickMethod = env->GetMethodID(mcObjClass, n, "()V");
                if (rightClickMethod) break;
            }
            if (rightClickMethod)
            {
                env->CallVoidMethod(mc, rightClickMethod);
                m_LastUse = now;
            }
            env->DeleteLocalRef(mcObjClass);
        }
        env->DeleteLocalRef(bestTarget);
    }

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

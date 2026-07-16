#include "esp.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include "../../../sdk/bridgeHelper.h"
#include <imgui.h>
#include <cmath>
#include <cfloat>
#include <vector>
#include <string>

ESPModule::ESPModule()
    : ModuleBase("ESP", "See entities through walls", Category::Render)
{
    AddSetting<NumberSetting>("FadeDistance", 3.0f, 0.0f, 10.0f, 0.1f);
    AddSetting<BooleanSetting>("HealthBar", true);
    AddSetting<BooleanSetting>("FilledBox", true);
    AddSetting<ColorSetting>("FilledColor", ImColor(0, 0, 0, 40));
    AddSetting<BooleanSetting>("Outline", true);
    AddSetting<ColorSetting>("OutlineColor", ImColor(0, 0, 0, 64));
    AddSetting<NumberSetting>("OutlineThickness", 1.5f, 0.5f, 5.0f, 0.1f);
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
    AddSetting<NumberSetting>("Max Distance", 300.0f, 0.0f, 1000.0f, 10.0f, "Maximum render distance");
}

void ESPModule::OnEnable() { Logger::Log("ESP enabled"); }
void ESPModule::OnDisable() { Logger::Log("ESP disabled"); }

static float GetHeight(JNIEnv* env, jobject entity)
{
    if (StrayCache::Entity_height) return env->GetFloatField(entity, StrayCache::Entity_height);
    return 1.8f;
}

static float GetWidth(JNIEnv* env, jobject entity)
{
    if (StrayCache::Entity_width) return env->GetFloatField(entity, StrayCache::Entity_width);
    return 0.6f;
}

static std::string GetEntName(JNIEnv* env, jobject entity)
{
    if (StrayCache::Entity_getName)
    {
        jobject nameObj = env->CallObjectMethod(entity, StrayCache::Entity_getName);
        if (nameObj)
        {
            const char* str = env->GetStringUTFChars((jstring)nameObj, nullptr);
            std::string result(str ? str : "");
            if (str) env->ReleaseStringUTFChars((jstring)nameObj, str);
            env->DeleteLocalRef(nameObj);
            return result;
        }
    }
    return "";
}

void ESPModule::OnUpdate()
{
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) {
        Logger::Log("[ESP] No JNI env");
        return;
    }

    jobject player = nullptr;
    jobject world = nullptr;
    jobject entityList = nullptr;
    jclass listClass = nullptr;
    jmethodID listSize = nullptr;
    jmethodID listGet = nullptr;
    bool bridgeSuccess = false;

    if (BridgeHelper::Initialize(env)) {
        player = BridgeHelper::GetPlayer(env);
        if (player) {
            world = BridgeHelper::GetWorldFromEntity(env, player);
            if (world) {
                if (BridgeHelper::WorldBridge_GetPlayerEntities) {
                    entityList = env->CallObjectMethod(world, BridgeHelper::WorldBridge_GetPlayerEntities);
                    if (entityList) {
                        listClass = env->FindClass("java/util/List");
                        if (listClass) {
                            listSize = env->GetMethodID(listClass, "size", "()I");
                            listGet = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
                            if (listSize && listGet) {
                                bridgeSuccess = true;
                            }
                        }
                    }
                }
                // Cleanup local refs if not using bridge
                if (!bridgeSuccess) {
                    if (listClass) env->DeleteLocalRef(listClass);
                    if (entityList) env->DeleteLocalRef(entityList);
                    if (world) env->DeleteLocalRef(world);
                    if (player) env->DeleteLocalRef(player);
                }
            }
        }
    }

    if (bridgeSuccess) {
        // Use bridge data
        double pX = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetPosX);
        double pY = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetPosY);
        double pZ = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetPosZ);
        jint eSize = env->CallIntMethod(entityList, listSize);
        Logger::Log("[ESP] Bridge got %d entities", eSize);
        float fadeDist = ((NumberSetting*)FindSetting("FadeDistance"))->GetValue();
        float maxDistance = ((NumberSetting*)FindSetting("Max Distance"))->GetValue();
        jint localId = BridgeHelper::EntityBridge_GetEntityId ? env->CallIntMethod(player, BridgeHelper::EntityBridge_GetEntityId) : -1;
        std::vector<ESPData> newData;

        for (jint i = 0; i < eSize; i++) {
            jobject ent = env->CallObjectMethod(entityList, listGet, i);
            if (!ent) continue;

            jint entId = BridgeHelper::EntityBridge_GetEntityId ? env->CallIntMethod(ent, BridgeHelper::EntityBridge_GetEntityId) : -1;
            if (entId == localId || entId < 0) { env->DeleteLocalRef(ent); continue; }

            if (BridgeHelper::EntityBridge_IsDead && env->CallBooleanMethod(ent, BridgeHelper::EntityBridge_IsDead)) {
                env->DeleteLocalRef(ent);
                continue;
            }

            double eX = env->CallDoubleMethod(ent, BridgeHelper::EntityBridge_GetPosX);
            double eY = env->CallDoubleMethod(ent, BridgeHelper::EntityBridge_GetPosY);
            double eZ = env->CallDoubleMethod(ent, BridgeHelper::EntityBridge_GetPosZ);

            // Use StrayCache for name, height, width (bridge may not have these)
            std::string name = GetEntName(env, ent);
            float height = GetHeight(env, ent);
            float width = GetWidth(env, ent);

            float health = 20.0f, maxHealth = 20.0f;
            if (BridgeHelper::ELBridge_GetHealth)
                health = env->CallFloatMethod(ent, BridgeHelper::ELBridge_GetHealth);
            if (BridgeHelper::ELBridge_GetMaxHealth)
                maxHealth = env->CallFloatMethod(ent, BridgeHelper::ELBridge_GetMaxHealth);

            float dist = (float)std::sqrt((eX - pX) * (eX - pX) + (eY - pY) * (eY - pY) + (eZ - pZ) * (eZ - pZ));
            if (dist > maxDistance) {
                env->DeleteLocalRef(ent);
                continue;
            }

            float opacityFade = 1.0f;
            if ((dist - 1.0f) <= fadeDist && fadeDist > 0.0f)
                opacityFade = (dist - 1.0f) / fadeDist;

            float hw = width * 0.5f, hh = height;
            double corners[8][3] = {
                {eX - hw, eY,      eZ - hw}, {eX + hw, eY,      eZ - hw},
                {eX + hw, eY + hh, eZ - hw}, {eX - hw, eY + hh, eZ - hw},
                {eX - hw, eY,      eZ + hw}, {eX + hw, eY,      eZ + hw},
                {eX + hw, eY + hh, eZ + hw}, {eX - hw, eY + hh, eZ + hw},
            };

            float left = FLT_MAX, top_ = FLT_MAX, right = FLT_MIN, bottom = FLT_MIN;
            for (int c = 0; c < 8; c++) {
                float sx, sy;
                if (WorldToScreen(env, corners[c][0], corners[c][1], corners[c][2], sx, sy)) {
                    left = fmin(left, sx);
                    top_ = fmin(top_, sy);
                    right = fmax(right, sx);
                    bottom = fmax(bottom, sy);
                }
            }

            if (left >= right || top_ >= bottom) {
                env->DeleteLocalRef(ent);
                continue;
            }

            newData.push_back({ left, top_, right, bottom, name, dist, health, maxHealth, opacityFade });
            env->DeleteLocalRef(ent);
        }

        env->DeleteLocalRef(entityList);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(player);
        m_RenderData = newData;
        return;
    }

    // Fallback to StrayCache (using helper functions from jni_helpers.h)
    jobject mc = GetMinecraftObject(env);
    if (!mc) {
        Logger::Log("[ESP] No Minecraft instance");
        return;
    }
    jobject worldObj = GetWorldObject(env, mc);
    if (!worldObj) {
        env->DeleteLocalRef(mc);
        Logger::Log("[ESP] No world");
        return;
    }
    if (!StrayCache::World_playerEntities || !StrayCache::ListClass || !StrayCache::List_size || !StrayCache::List_get) {
        env->DeleteLocalRef(worldObj);
        env->DeleteLocalRef(mc);
        Logger::Log("[ESP] StrayCache missing playerEntities or List helpers");
        return;
    }

    jobject entityListObj = env->GetObjectField(worldObj, StrayCache::World_playerEntities);
    if (!entityListObj) {
        env->DeleteLocalRef(worldObj);
        env->DeleteLocalRef(mc);
        return;
    }
    jobject playerObj = GetPlayerObject(env, mc);
    if (!playerObj) {
        env->DeleteLocalRef(entityListObj);
        env->DeleteLocalRef(worldObj);
        env->DeleteLocalRef(mc);
        return;
    }

    double pX = GetEntityPosX(env, playerObj);
    double pY = GetEntityPosY(env, playerObj);
    double pZ = GetEntityPosZ(env, playerObj);
    jint eSize = env->CallIntMethod(entityListObj, StrayCache::List_size);
    Logger::Log("[ESP] StrayCache fallback got %d entities", eSize);
    float fadeDist = ((NumberSetting*)FindSetting("FadeDistance"))->GetValue();
    float maxDistance = ((NumberSetting*)FindSetting("Max Distance"))->GetValue();
    int localId = GetEntityId(env, playerObj);
    std::vector<ESPData> newData;

    for (jint i = 0; i < eSize; i++) {
        jobject ent = env->CallObjectMethod(entityListObj, StrayCache::List_get, i);
        if (!ent) continue;
        int entId = GetEntityId(env, ent);
        if (entId == localId || entId < 0) {
            env->DeleteLocalRef(ent);
            continue;
        }
        if (IsEntityDead(env, ent)) {
            env->DeleteLocalRef(ent);
            continue;
        }
        std::string name = GetEntityName(env, ent);
        if (name.empty()) {
            env->DeleteLocalRef(ent);
            continue;
        }
        double eX = GetEntityPosX(env, ent);
        double eY = GetEntityPosY(env, ent);
        double eZ = GetEntityPosZ(env, ent);
        float height = GetEntityHeight(env, ent);
        float width = GetEntityWidth(env, ent);
        float health = GetEntityLivingBaseHealth(env, ent);
        float maxHealth = GetEntityLivingBaseMaxHealth(env, ent);
        float dist = (float)std::sqrt((eX - pX)*(eX - pX) + (eY - pY)*(eY - pY) + (eZ - pZ)*(eZ - pZ));
        if (dist > maxDistance) {
            env->DeleteLocalRef(ent);
            continue;
        }
        float opacityFade = 1.0f;
        if ((dist - 1.0f) <= fadeDist && fadeDist > 0.0f)
            opacityFade = (dist - 1.0f) / fadeDist;
        float hw = width * 0.5f, hh = height;
        double corners[8][3] = {
            {eX - hw, eY,      eZ - hw}, {eX + hw, eY,      eZ - hw},
            {eX + hw, eY + hh, eZ - hw}, {eX - hw, eY + hh, eZ - hw},
            {eX - hw, eY,      eZ + hw}, {eX + hw, eY,      eZ + hw},
            {eX + hw, eY + hh, eZ + hw}, {eX - hw, eY + hh, eZ + hw},
        };
        float left = FLT_MAX, top_ = FLT_MAX, right = FLT_MIN, bottom = FLT_MIN;
        for (int c = 0; c < 8; c++) {
            float sx, sy;
            if (WorldToScreen(env, corners[c][0], corners[c][1], corners[c][2], sx, sy)) {
                left = fmin(left, sx);
                top_ = fmin(top_, sy);
                right = fmax(right, sx);
                bottom = fmax(bottom, sy);
            }
        }
        if (left >= right || top_ >= bottom) {
            env->DeleteLocalRef(ent);
            continue;
        }
        newData.push_back({ left, top_, right, bottom, name, dist, health, maxHealth, opacityFade });
        env->DeleteLocalRef(ent);
    }

    env->DeleteLocalRef(entityListObj);
    env->DeleteLocalRef(worldObj);
    env->DeleteLocalRef(mc);
    m_RenderData = newData;
}

void ESPModule::OnRender()
{
    // TODO: implement render
}
            
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
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) { Logger::Log("[ESP] No JNI env"); return; }

    if (!BridgeHelper::Initialize(env))
    {
        Logger::Log("[ESP] BridgeHelper init failed, trying StrayCache...");
        // Fallback to StrayCache entity list
        jobject mc = GetMinecraftObject(env);
        if (!mc) return;
        jobject world = GetWorldObject(env, mc);
        if (!world) { env->DeleteLocalRef(mc); return; }
        if (!StrayCache::World_playerEntities || !StrayCache::ListClass || !StrayCache::List_size || !StrayCache::List_get)
        { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); Logger::Log("[ESP] StrayCache also missing playerEntities"); return; }

        jobject entityList = env->GetObjectField(world, StrayCache::World_playerEntities);
        if (!entityList) { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }
        jobject player = GetPlayerObject(env, mc);
        if (!player) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }
        double pX = GetEntityPosX(env, player);
        double pY = GetEntityPosY(env, player);
        double pZ = GetEntityPosZ(env, player);
        jint eSize = env->CallIntMethod(entityList, StrayCache::List_size);
        Logger::Log("[ESP] StrayCache fallback got %d entities", eSize);
        float fadeDist = ((NumberSetting*)FindSetting("FadeDistance"))->GetValue();
        float maxDistance = ((NumberSetting*)FindSetting("Max Distance"))->GetValue();
        int localId = GetEntityId(env, player);
        std::vector<ESPData> newData;
        for (jint i = 0; i < eSize; i++)
        {
            jobject ent = env->CallObjectMethod(entityList, StrayCache::List_get, i);
            if (!ent) continue;
            int entId = GetEntityId(env, ent);
            if (entId == localId || entId < 0) { env->DeleteLocalRef(ent); continue; }
            if (IsEntityDead(env, ent)) { env->DeleteLocalRef(ent); continue; }
            std::string name = GetEntName(env, ent);
            if (name.empty()) { env->DeleteLocalRef(ent); continue; }
            double eX = GetEntityPosX(env, ent);
            double eY = GetEntityPosY(env, ent);
            double eZ = GetEntityPosZ(env, ent);
            float height = GetHeight(env, ent);
            float width = GetWidth(env, ent);
            float health = 20.0f, maxHealth = 20.0f;
            if (StrayCache::EntityLivingBase_getHealth) health = env->CallFloatMethod(ent, StrayCache::EntityLivingBase_getHealth);
            if (StrayCache::EntityLivingBase_getMaxHealth) maxHealth = env->CallFloatMethod(ent, StrayCache::EntityLivingBase_getMaxHealth);
            float dist = (float)std::sqrt((eX-pX)*(eX-pX) + (eY-pY)*(eY-pY) + (eZ-pZ)*(eZ-pZ));
            if (dist > maxDistance) { env->DeleteLocalRef(ent); continue; }
            float opacityFade = 1.0f;
            if ((dist - 1.0f) <= fadeDist && fadeDist > 0.0f) opacityFade = (dist - 1.0f) / fadeDist;
            float hw = width * 0.5f, hh = height;
            double corners[8][3] = {
                {eX - hw, eY,      eZ - hw}, {eX + hw, eY,      eZ - hw},
                {eX + hw, eY + hh, eZ - hw}, {eX - hw, eY + hh, eZ - hw},
                {eX - hw, eY,      eZ + hw}, {eX + hw, eY,      eZ + hw},
                {eX + hw, eY + hh, eZ + hw}, {eX - hw, eY + hh, eZ + hw},
            };
            float left = FLT_MAX, top_ = FLT_MAX, right = FLT_MIN, bottom = FLT_MIN;
            for (int c = 0; c < 8; c++)
            {
                float sx, sy;
                if (WorldToScreen(env, corners[c][0], corners[c][1], corners[c][2], sx, sy))
                { left = fmin(left, sx); top_ = fmin(top_, sy); right = fmax(right, sx); bottom = fmax(bottom, sy); }
            }
            if (left >= right || top_ >= bottom) { env->DeleteLocalRef(ent); continue; }
            newData.push_back({ left, top_, right, bottom, name, dist, health, maxHealth, opacityFade });
            env->DeleteLocalRef(ent);
        }
        env->DeleteLocalRef(entityList);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(mc);
        m_RenderData = newData;
        return;
    }

    // BridgeHelper path
    jobject player = BridgeHelper::GetPlayer(env);
    if (!player) { Logger::Log("[ESP] No player"); return; }

    jobject world = BridgeHelper::GetWorldFromEntity(env, player);
    if (!world) { env->DeleteLocalRef(player); Logger::Log("[ESP] No world"); return; }

    if (!BridgeHelper::WorldBridge_GetPlayerEntities)
    { env->DeleteLocalRef(world); env->DeleteLocalRef(player); Logger::Log("[ESP] No GetPlayerEntities method"); return; }

    jobject entityList = env->CallObjectMethod(world, BridgeHelper::WorldBridge_GetPlayerEntities);
    if (!entityList) { env->DeleteLocalRef(world); env->DeleteLocalRef(player); Logger::Log("[ESP] Entity list null"); return; }

    jclass listClass = env->FindClass("java/util/List");
    jmethodID listSize = env->GetMethodID(listClass, "size", "()I");
    jmethodID listGet = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    env->DeleteLocalRef(listClass);
    if (!listSize || !listGet) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(world); env->DeleteLocalRef(player); return; }

    double pX = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetPosX);
    double pY = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetPosY);
    double pZ = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetPosZ);

    jint eSize = env->CallIntMethod(entityList, listSize);
    Logger::Log("[ESP] Bridge got %d entities", eSize);

    float fadeDist = ((NumberSetting*)FindSetting("FadeDistance"))->GetValue();
    float maxDistance = ((NumberSetting*)FindSetting("Max Distance"))->GetValue();
    jint localId = BridgeHelper::EntityBridge_GetEntityId ? env->CallIntMethod(player, BridgeHelper::EntityBridge_GetEntityId) : -1;
    std::vector<ESPData> newData;

    for (jint i = 0; i < eSize; i++)
    {
        jobject ent = env->CallObjectMethod(entityList, listGet, i);
        if (!ent) continue;

        jint entId = BridgeHelper::EntityBridge_GetEntityId ? env->CallIntMethod(ent, BridgeHelper::EntityBridge_GetEntityId) : -1;
        if (entId == localId || entId < 0) { env->DeleteLocalRef(ent); continue; }

        if (BridgeHelper::EntityBridge_IsDead && env->CallBooleanMethod(ent, BridgeHelper::EntityBridge_IsDead))
        { env->DeleteLocalRef(ent); continue; }

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

        float dist = (float)std::sqrt((eX-pX)*(eX-pX) + (eY-pY)*(eY-pY) + (eZ-pZ)*(eZ-pZ));
        if (dist > maxDistance) { env->DeleteLocalRef(ent); continue; }

        float opacityFade = 1.0f;
        if ((dist - 1.0f) <= fadeDist && fadeDist > 0.0f) opacityFade = (dist - 1.0f) / fadeDist;

        float hw = width * 0.5f, hh = height;
        double corners[8][3] = {
            {eX - hw, eY,      eZ - hw}, {eX + hw, eY,      eZ - hw},
            {eX + hw, eY + hh, eZ - hw}, {eX - hw, eY + hh, eZ - hw},
            {eX - hw, eY,      eZ + hw}, {eX + hw, eY,      eZ + hw},
            {eX + hw, eY + hh, eZ + hw}, {eX - hw, eY + hh, eZ + hw},
        };

        float left = FLT_MAX, top_ = FLT_MAX, right = FLT_MIN, bottom = FLT_MIN;
        for (int c = 0; c < 8; c++)
        {
            float sx, sy;
            if (WorldToScreen(env, corners[c][0], corners[c][1], corners[c][2], sx, sy))
            { left = fmin(left, sx); top_ = fmin(top_, sy); right = fmax(right, sx); bottom = fmax(bottom, sy); }
        }

        if (left >= right || top_ >= bottom) { env->DeleteLocalRef(ent); continue; }

        newData.push_back({ left, top_, right, bottom, name, dist, health, maxHealth, opacityFade });
        env->DeleteLocalRef(ent);
    }

    env->DeleteLocalRef(entityList);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(player);

    m_RenderData = newData;
}

void ESPModule::OnRender()
{
    if (!IsEnabled() || m_RenderData.empty()) return;

    bool filled = ((BooleanSetting*)FindSetting("FilledBox"))->GetValue();
    bool outline = ((BooleanSetting*)FindSetting("Outline"))->GetValue();
    bool healthBar = ((BooleanSetting*)FindSetting("HealthBar"))->GetValue();
    float outThick = ((NumberSetting*)FindSetting("OutlineThickness"))->GetValue();
    ImColor fillCol = ((ColorSetting*)FindSetting("FilledColor"))->GetValue();
    ImColor outCol = ((ColorSetting*)FindSetting("OutlineColor"))->GetValue();

    ImDrawList* draw = ImGui::GetForegroundDrawList();

    for (auto& d : m_RenderData)
    {
        ImVec2 tl(d.boxLeft, d.boxTop);
        ImVec2 br(d.boxRight, d.boxBottom);
        float boxW = br.x - tl.x, boxH = br.y - tl.y;

        ImColor fCol(fillCol.Value.x, fillCol.Value.y, fillCol.Value.z, fillCol.Value.w * d.opacityFade);
        ImColor oCol(outCol.Value.x, outCol.Value.y, outCol.Value.z, outCol.Value.w * d.opacityFade);

        if (filled) draw->AddRectFilled(tl, br, fCol);
        if (outline)
        {
            draw->AddRect(tl, br, oCol, 0.0f, 0, outThick);
            draw->AddRect(ImVec2(tl.x-1, tl.y-1), ImVec2(br.x+1, br.y+1), oCol, 0.0f, 0, 0.5f);
            draw->AddRect(ImVec2(tl.x+1, tl.y+1), ImVec2(br.x-1, br.y-1), oCol, 0.0f, 0, 0.5f);
        }

        if (healthBar && d.maxHealth > 0.0f)
        {
            float hFrac = fmax(0.0f, fmin(1.0f, d.health / d.maxHealth));
            float barW = 2.5f;
            ImVec2 barTL(tl.x - barW - 1.5f, tl.y);
            ImVec2 barBR(tl.x - 1.0f, br.y);
            draw->AddRectFilled(barTL, barBR, ImColor(0, 0, 0, (int)(180 * d.opacityFade)));
            float barH = boxH * hFrac;
            ImVec2 healthTL(barTL.x, barBR.y - barH);
            int r = (int)(255 * (1.0f - hFrac));
            int g = (int)(255 * hFrac);
            draw->AddRectFilled(healthTL, barBR, ImColor(r, g, 0, (int)(255 * d.opacityFade)));
        }
    }
}
#include "chams.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <imgui.h>
#include <cmath>
#include <cfloat>

ChamsModule::ChamsModule()
    : ModuleBase("Chams", "See entities through walls", Category::Render)
{
    AddSetting<ColorSetting>("Color", ImColor(0.0f, 1.0f, 0.0f, 0.3f));
    AddSetting<BooleanSetting>("Players", true, "Show chams on players");
    AddSetting<BooleanSetting>("Mobs", false, "Show chams on mobs");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
    AddSetting<NumberSetting>("Max Distance", 200.0f, 0.0f, 500.0f, 10.0f, "Maximum render distance");
}

void ChamsModule::OnEnable() { Logger::Log("Chams enabled"); }
void ChamsModule::OnDisable() { Logger::Log("Chams disabled"); }

void ChamsModule::OnUpdate()
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

    jobject world = GetWorldObject(env, mc);
    if (!world) { env->DeleteLocalRef(mc); return; }

    if (!StrayCache::World || !StrayCache::World_loadedEntityList || !StrayCache::ListClass || !StrayCache::List_size || !StrayCache::List_get)
    { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jobject listObj = env->GetObjectField(world, StrayCache::World_loadedEntityList);
    if (!listObj) { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jobject localPlayer = GetPlayerObject(env, mc);
    if (!localPlayer) { env->DeleteLocalRef(listObj); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    bool showPlayers = ((BooleanSetting*)FindSetting("Players"))->GetValue();
    bool showMobs = ((BooleanSetting*)FindSetting("Mobs"))->GetValue();
    float maxDistance = ((NumberSetting*)FindSetting("Max Distance"))->GetValue();

    jint eSize = env->CallIntMethod(listObj, StrayCache::List_size);
    int localId = GetEntityId(env, localPlayer);

    std::vector<ChamsData> newData;

    for (jint i = 0; i < eSize; i++)
    {
        jobject entity = env->CallObjectMethod(listObj, StrayCache::List_get, i);
        if (!entity) continue;

        if (env->IsSameObject(entity, localPlayer)) { env->DeleteLocalRef(entity); continue; }

        bool isPlayer = env->IsInstanceOf(entity, StrayCache::EntityPlayer);
        bool isLivingBase = env->IsInstanceOf(entity, StrayCache::EntityLivingBase);

        if (isPlayer && !showPlayers) { env->DeleteLocalRef(entity); continue; }
        if (!isPlayer && isLivingBase && !showMobs) { env->DeleteLocalRef(entity); continue; }
        if (!isLivingBase) { env->DeleteLocalRef(entity); continue; }

        if (IsEntityDead(env, entity)) { env->DeleteLocalRef(entity); continue; }

        double eX = GetEntityPosX(env, entity);
        double eY = GetEntityPosY(env, entity);
        double eZ = GetEntityPosZ(env, entity);

        // Distance check
        double pX = GetEntityPosX(env, localPlayer);
        double pY = GetEntityPosY(env, localPlayer);
        double pZ = GetEntityPosZ(env, localPlayer);
        double dist = std::sqrt((eX-pX)*(eX-pX) + (eY-pY)*(eY-pY) + (eZ-pZ)*(eZ-pZ));
        if (dist > maxDistance) { env->DeleteLocalRef(entity); continue; }

        float h = GetEntityHeight(env, entity);
        float w = GetEntityWidth(env, entity);

        std::string name = GetEntityName(env, entity);

        newData.push_back({ eX, eY, eZ, h, w, name });
        env->DeleteLocalRef(entity);
    }

    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(listObj);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(mc);

    m_RenderData = newData;
}

void ChamsModule::OnRender()
{
    if (!IsEnabled() || m_RenderData.empty()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImColor color = ((ColorSetting*)FindSetting("Color"))->GetValue();
    ImU32 col = color;

    for (auto& data : m_RenderData)
    {
        double x = data.x;
        double y = data.y;
        double z = data.z;
        float h = data.height;
        float w = data.width;

        double hw = w * 0.5;
        double corners[8][3] = {
            {x - hw, y,      z - hw}, {x + hw, y,      z - hw},
            {x + hw, y + h,  z - hw}, {x - hw, y + h,  z - hw},
            {x - hw, y,      z + hw}, {x + hw, y,      z + hw},
            {x + hw, y + h,  z + hw}, {x - hw, y + h,  z + hw},
        };

        float left = FLT_MAX, top = FLT_MAX, right = FLT_MIN, bottom = FLT_MIN;
        bool anyValid = false;
        for (int c = 0; c < 8; c++)
        {
            float sx, sy;
            if (WorldToScreen(env, corners[c][0], corners[c][1], corners[c][2], sx, sy))
            {
                anyValid = true;
                left = fminf(left, sx);
                top = fminf(top, sy);
                right = fmaxf(right, sx);
                bottom = fmaxf(bottom, sy);
            }
        }

        if (!anyValid || left >= right || top >= bottom) continue;

        draw->AddRectFilled(ImVec2(left, top), ImVec2(right, bottom), col);
        draw->AddRect(ImVec2(left, top), ImVec2(right, bottom), ImColor(0, 0, 0, 200), 0.0f, 0, 1.5f);
    }
}
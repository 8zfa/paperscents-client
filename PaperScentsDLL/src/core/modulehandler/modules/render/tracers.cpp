#include "tracers.h"
#include "../../../java/java.h"
#include "../../../sdk/strayCache.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../utilities/logger.h"
#include <imgui.h>
#include <cmath>

TracersModule::TracersModule()
    : ModuleBase("Tracers", "Draw lines to nearby entities", Category::Render)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Line", "Arrow"});
    AddSetting<BooleanSetting>("Players", true, "Show tracers to players");
    AddSetting<BooleanSetting>("Mobs", false, "Show tracers to mobs");
    AddSetting<ColorSetting>("Color", ImColor(1.0f, 1.0f, 1.0f, 0.8f));
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void TracersModule::OnEnable() { Logger::Log("Tracers enabled"); }
void TracersModule::OnDisable() { Logger::Log("Tracers disabled"); }

void TracersModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!StrayCache::Minecraft || !StrayCache::MinecraftInstance || !StrayCache::World || !StrayCache::Entity || !StrayCache::EntityPlayer) return;

    jobject mc = StrayCache::MinecraftInstance;
    jobject localPlayer = GetPlayerObject(env, mc);
    if (!localPlayer) return;

    jobject world = GetWorldObject(env, mc);
    if (!world) { env->DeleteLocalRef(localPlayer); return; }

    if (!StrayCache::World_loadedEntityList || !StrayCache::ListClass || !StrayCache::List_size || !StrayCache::List_get)
    { env->DeleteLocalRef(world); env->DeleteLocalRef(localPlayer); return; }

    jobject listObj = env->GetObjectField(world, StrayCache::World_loadedEntityList);
    if (!listObj) { env->DeleteLocalRef(world); env->DeleteLocalRef(localPlayer); return; }

    bool showPlayers = ((BooleanSetting*)FindSetting("Players"))->GetValue();
    bool showMobs = ((BooleanSetting*)FindSetting("Mobs"))->GetValue();

    jint size = env->CallIntMethod(listObj, StrayCache::List_size);
    std::vector<TracerData> newData;

    for (int i = 0; i < size; i++)
    {
        jobject entity = env->CallObjectMethod(listObj, StrayCache::List_get, i);
        if (!entity) continue;

        if (env->IsSameObject(entity, localPlayer)) { env->DeleteLocalRef(entity); continue; }

        bool isPlayer = env->IsInstanceOf(entity, StrayCache::EntityPlayer);

        if (isPlayer && !showPlayers) { env->DeleteLocalRef(entity); continue; }
        if (!isPlayer && !showMobs) { env->DeleteLocalRef(entity); continue; }

        double ex = GetEntityPosX(env, entity);
        double ey = GetEntityPosY(env, entity);
        double ez = GetEntityPosZ(env, entity);

        newData.push_back({ ex, ey, ez, isPlayer });
        env->DeleteLocalRef(entity);
    }

    env->DeleteLocalRef(listObj);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(localPlayer);

    m_RenderData = newData;
}

void TracersModule::OnRender()
{
    if (!IsEnabled() || m_RenderData.empty()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!StrayCache::MinecraftInstance || !StrayCache::Minecraft || !StrayCache::RenderManagerClass ||
        !StrayCache::Minecraft_renderManager || !StrayCache::RenderManager_viewerPosX ||
        !StrayCache::RenderManager_viewerPosY || !StrayCache::RenderManager_viewerPosZ) return;

    jobject mc = StrayCache::MinecraftInstance;
    jobject rm = env->GetObjectField(mc, StrayCache::Minecraft_renderManager);
    if (!rm) return;

    double vx = env->GetDoubleField(rm, StrayCache::RenderManager_viewerPosX);
    double vy = env->GetDoubleField(rm, StrayCache::RenderManager_viewerPosY);
    double vz = env->GetDoubleField(rm, StrayCache::RenderManager_viewerPosZ);
    env->DeleteLocalRef(rm);

    ImColor color = ((ColorSetting*)FindSetting("Color"))->GetValue();
    ImU32 col = color;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImVec2 center(screen.x * 0.5f, screen.y * 0.5f);

    double fov = 70.0 * 0.5 * 3.14159 / 180.0;
    double tanFov = tan(fov);
    double aspect = screen.x / screen.y;

    for (auto& data : m_RenderData)
    {
        double dx = data.x - vx;
        double dy = (data.y + 1.0) - vy;
        double dz = data.z - vz;

        if (fabs(dz) < 0.01) dz = 0.01;

        double ax = dx / dz;
        double ay = dy / dz;

        ImVec2 screenPos;
        screenPos.x = (float)(center.x + center.x * ax / (tanFov * aspect));
        screenPos.y = (float)(center.y - center.y * ay / tanFov);

        if (screenPos.x < 0 || screenPos.x > screen.x || screenPos.y < 0 || screenPos.y > screen.y) continue;

        draw->AddLine(center, screenPos, col, 1.0f);

        int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();
        if (mode == 1)
        {
            float angle = atan2f(center.y - screenPos.y, center.x - screenPos.x) + 3.14159f * 0.5f;
            float arrowSize = 8.0f;
            ImVec2 a1(screenPos.x + cosf(angle - 0.5f) * arrowSize, screenPos.y + sinf(angle - 0.5f) * arrowSize);
            ImVec2 a2(screenPos.x + cosf(angle + 0.5f) * arrowSize, screenPos.y + sinf(angle + 0.5f) * arrowSize);
            draw->AddTriangleFilled(screenPos, a1, a2, col);
        }
    }
}

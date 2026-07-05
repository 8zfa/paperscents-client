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
}

void TracersModule::OnEnable() { Logger::Log("Tracers enabled"); }
void TracersModule::OnDisable() { Logger::Log("Tracers disabled"); }

void TracersModule::OnRender()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!StrayCache::Minecraft || !StrayCache::MinecraftInstance || !StrayCache::World || !StrayCache::Entity || !StrayCache::EntityPlayer) return;

    jobject mc = StrayCache::MinecraftInstance;
    jobject localPlayer = GetPlayerObject(env, mc);
    if (!localPlayer) return;

    jobject world = GetWorldObject(env, mc);
    if (!world) { env->DeleteLocalRef(localPlayer); return; }

    jfieldID entitiesField = env->GetFieldID(StrayCache::World, "g", "Ljava/util/List;");
    if (!entitiesField) { env->ExceptionClear(); entitiesField = env->GetFieldID(StrayCache::World, "loadedEntityList", "Ljava/util/List;"); }
    env->ExceptionClear();

    if (!entitiesField) { env->DeleteLocalRef(world); env->DeleteLocalRef(localPlayer); return; }

    jobject listObj = env->GetObjectField(world, entitiesField);
    if (!listObj) { env->DeleteLocalRef(world); env->DeleteLocalRef(localPlayer); return; }

    jclass listClass = env->FindClass("java/util/List");
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    if (!sizeMethod || !getMethod)
    {
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(listObj);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(localPlayer);
        return;
    }

    bool showPlayers = ((BooleanSetting*)FindSetting("Players"))->GetValue();
    bool showMobs = ((BooleanSetting*)FindSetting("Mobs"))->GetValue();
    ImColor color = ((ColorSetting*)FindSetting("Color"))->GetValue();
    ImU32 col = color;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImVec2 center(screen.x * 0.5f, screen.y * 0.5f);

    jfieldID posXID = env->GetFieldID(StrayCache::Entity, "r", "D");
    if (!posXID) { env->ExceptionClear(); posXID = env->GetFieldID(StrayCache::Entity, "posX", "D"); }
    env->ExceptionClear();
    jfieldID posYID = env->GetFieldID(StrayCache::Entity, "s", "D");
    if (!posYID) { env->ExceptionClear(); posYID = env->GetFieldID(StrayCache::Entity, "posY", "D"); }
    env->ExceptionClear();
    jfieldID posZID = env->GetFieldID(StrayCache::Entity, "t", "D");
    if (!posZID) { env->ExceptionClear(); posZID = env->GetFieldID(StrayCache::Entity, "posZ", "D"); }
    env->ExceptionClear();

    jclass renderManagerClass = env->FindClass("cgs");
    if (!renderManagerClass) { env->ExceptionClear(); renderManagerClass = env->FindClass("net/minecraft/client/renderer/entity/RenderManager"); }

    jfieldID rmField = nullptr;
    jobject rm = nullptr;
    jfieldID viewX = nullptr, viewY = nullptr, viewZ = nullptr;

    if (renderManagerClass)
    {
        rmField = env->GetFieldID(StrayCache::Minecraft, "ad", "Lcgs;");
        if (!rmField) { env->ExceptionClear(); rmField = env->GetFieldID(StrayCache::Minecraft, "renderManager", "Lcgs;"); }
        env->ExceptionClear();

        if (rmField)
        {
            rm = env->GetObjectField(mc, rmField);
            if (rm)
            {
                viewX = env->GetFieldID(renderManagerClass, "c", "D");
                if (!viewX) { env->ExceptionClear(); viewX = env->GetFieldID(renderManagerClass, "viewerPosX", "D"); }
                env->ExceptionClear();
                viewY = env->GetFieldID(renderManagerClass, "d", "D");
                if (!viewY) { env->ExceptionClear(); viewY = env->GetFieldID(renderManagerClass, "viewerPosY", "D"); }
                env->ExceptionClear();
                viewZ = env->GetFieldID(renderManagerClass, "e", "D");
                if (!viewZ) { env->ExceptionClear(); viewZ = env->GetFieldID(renderManagerClass, "viewerPosZ", "D"); }
                env->ExceptionClear();
            }
        }
    }

    int size = env->CallIntMethod(listObj, sizeMethod);

    for (int i = 0; i < size; i++)
    {
        jobject entity = env->CallObjectMethod(listObj, getMethod, i);
        if (!entity) continue;

        if (env->IsSameObject(entity, localPlayer)) { env->DeleteLocalRef(entity); continue; }

        bool isPlayer = env->IsInstanceOf(entity, StrayCache::EntityPlayer);

        if (isPlayer && !showPlayers) { env->DeleteLocalRef(entity); continue; }
        if (!isPlayer && !showMobs) { env->DeleteLocalRef(entity); continue; }

        double ex = posXID ? env->GetDoubleField(entity, posXID) : 0.0;
        double ey = posYID ? env->GetDoubleField(entity, posYID) : 0.0;
        double ez = posZID ? env->GetDoubleField(entity, posZID) : 0.0;

        double vx = (viewX && rm) ? env->GetDoubleField(rm, viewX) : 0.0;
        double vy = (viewY && rm) ? env->GetDoubleField(rm, viewY) : 0.0;
        double vz = (viewZ && rm) ? env->GetDoubleField(rm, viewZ) : 0.0;

        double dx = ex - vx;
        double dy = (ey + 1.0) - vy;
        double dz = ez - vz;

        if (fabs(dz) < 0.01) dz = 0.01;

        ImVec2 screenPos;
        double fov = 70.0 * 0.5 * 3.14159 / 180.0;
        double tanFov = tan(fov);
        double aspect = screen.x / screen.y;

        double ax = dx / dz;
        double ay = dy / dz;

        screenPos.x = (float)(center.x + center.x * ax / (tanFov * aspect));
        screenPos.y = (float)(center.y - center.y * ay / tanFov);

        if (screenPos.x < 0 || screenPos.x > screen.x || screenPos.y < 0 || screenPos.y > screen.y) { env->DeleteLocalRef(entity); continue; }

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

        env->DeleteLocalRef(entity);
    }

    if (rm) env->DeleteLocalRef(rm);
    if (renderManagerClass) env->DeleteLocalRef(renderManagerClass);
    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(listObj);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(localPlayer);
}

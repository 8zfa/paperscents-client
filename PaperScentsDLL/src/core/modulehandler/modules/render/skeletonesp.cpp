#include "skeletonesp.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <imgui.h>
#include <cmath>
#include <cfloat>

SkeletonESPModule::SkeletonESPModule()
    : ModuleBase("SkeletonESP", "Draws skeleton bones on players", Category::Render)
{
    AddSetting<ColorSetting>("Color", ImColor(1.0f, 1.0f, 1.0f, 1.0f));
    AddSetting<NumberSetting>("LineWidth", 1.5f, 0.5f, 3.0f, 0.1f);
    AddSetting<NumberSetting>("Range", 50.0f, 10.0f, 100.0f, 1.0f);
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void SkeletonESPModule::OnEnable() { Logger::Log("SkeletonESP enabled"); }
void SkeletonESPModule::OnDisable() { Logger::Log("SkeletonESP disabled"); }

void SkeletonESPModule::OnUpdate()
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

    if (!StrayCache::World || !StrayCache::World_playerEntities || !StrayCache::ListClass || !StrayCache::List_size || !StrayCache::List_get)
    { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jobject playerList = env->GetObjectField(world, StrayCache::World_playerEntities);
    if (!playerList) { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jobject localPlayer = GetPlayerObject(env, mc);
    if (!localPlayer) { env->DeleteLocalRef(playerList); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jint eSize = env->CallIntMethod(playerList, StrayCache::List_size);

    double pX = GetEntityPosX(env, localPlayer);
    double pY = GetEntityPosY(env, localPlayer);
    double pZ = GetEntityPosZ(env, localPlayer);

    float range = ((NumberSetting*)FindSetting("Range"))->GetValue();
    std::vector<SkeletonData> newData;

    for (jint i = 0; i < eSize; i++)
    {
        jobject ent = env->CallObjectMethod(playerList, StrayCache::List_get, i);
        if (!ent) continue;

        if (env->IsSameObject(ent, localPlayer)) { env->DeleteLocalRef(ent); continue; }

        bool isPlayer = StrayCache::EntityPlayer && env->IsInstanceOf(ent, StrayCache::EntityPlayer);
        if (!isPlayer) { env->DeleteLocalRef(ent); continue; }

        if (IsEntityDead(env, ent)) { env->DeleteLocalRef(ent); continue; }

        double eX = GetEntityPosX(env, ent);
        double eY = GetEntityPosY(env, ent);
        double eZ = GetEntityPosZ(env, ent);

        double dist = std::sqrt((eX - pX) * (eX - pX) + (eY - pY) * (eY - pY) + (eZ - pZ) * (eZ - pZ));
        if (dist > range) { env->DeleteLocalRef(ent); continue; }

        float height = GetEntityHeight(env, ent);

        float limbSwing = 0.0f, limbSwingAmount = 0.0f;
        if (StrayCache::EntityLivingBase)
        {
            static jfieldID limbSwingID = nullptr, limbSwingAmountID = nullptr;
            if (!limbSwingID)
            {
                limbSwingID = env->GetFieldID(StrayCache::EntityLivingBase, "R", "F");
                if (!limbSwingID) { env->ExceptionClear(); limbSwingID = env->GetFieldID(StrayCache::EntityLivingBase, "limbSwing", "F"); }
                env->ExceptionClear();
                limbSwingAmountID = env->GetFieldID(StrayCache::EntityLivingBase, "S", "F");
                if (!limbSwingAmountID) { env->ExceptionClear(); limbSwingAmountID = env->GetFieldID(StrayCache::EntityLivingBase, "limbSwingAmount", "F"); }
                env->ExceptionClear();
            }
            if (limbSwingID) limbSwing = env->GetFloatField(ent, limbSwingID);
            if (limbSwingAmountID) limbSwingAmount = env->GetFloatField(ent, limbSwingAmountID);
        }

        newData.push_back({ eX, eY, eZ, limbSwing, limbSwingAmount, height });
        env->DeleteLocalRef(ent);
    }

    env->DeleteLocalRef(playerList);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);

    m_RenderData = newData;
}

void SkeletonESPModule::OnRender()
{
    if (!IsEnabled() || m_RenderData.empty()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImColor color = ((ColorSetting*)FindSetting("Color"))->GetValue();
    ImU32 col = color;
    float lineWidth = ((NumberSetting*)FindSetting("LineWidth"))->GetValue();

    for (auto& data : m_RenderData)
    {
        double x = data.posX;
        double y = data.posY;
        double z = data.posZ;
        float h = data.height;
        float swing = data.limbSwing;
        float swingAmount = data.limbSwingAmount;

        double headY = y + h;
        double neckY = y + h * 0.85;
        double shoulderY = y + h * 0.75;
        double spineY = y + h * 0.5;
        double pelvisY = y + h * 0.25;
        double footY = y;

        float armSwing = sinf(swing) * swingAmount * 0.5f;
        float legSwing = -armSwing;

        double armHSpread = h * 0.3;
        double legSpread = h * 0.15;

        SkeletonBone bones[] = {
            { x, headY, z },
            { x, neckY, z },
            { x, shoulderY, z },
            { x - armHSpread - armSwing, shoulderY, z },
            { x + armHSpread + armSwing, shoulderY, z },
            { x, spineY, z },
            { x, pelvisY, z },
            { x - legSpread + legSwing, footY, z },
            { x + legSpread - legSwing, footY, z },
        };
        float sx[9], sy[9];
        bool valid[9];
        for (int i = 0; i < 9; i++)
            valid[i] = WorldToScreen(env, bones[i].x, bones[i].y, bones[i].z, sx[i], sy[i]);

        auto drawLine = [&](int a, int b) {
            if (valid[a] && valid[b])
                draw->AddLine(ImVec2(sx[a], sy[a]), ImVec2(sx[b], sy[b]), col, lineWidth);
        };

        drawLine(0, 1);
        drawLine(1, 2);
        drawLine(2, 3);
        drawLine(2, 4);
        drawLine(2, 5);
        drawLine(5, 6);
        drawLine(6, 7);
        drawLine(6, 8);
    }
}

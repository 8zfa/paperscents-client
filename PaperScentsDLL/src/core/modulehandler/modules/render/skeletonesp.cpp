#include "skeletonesp.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <imgui.h>
#include <cmath>
#include <cfloat>

bool SkeletonESPModule::WorldToScreen(JNIEnv* env, double x, double y, double z, float& sx, float& sy)
{
    jclass mcCls = env->GetObjectClass(StrayCache::MinecraftInstance);
    if (!mcCls) return false;

    jfieldID rmField = env->GetFieldID(mcCls, "ad", "Lcgs;");
    if (!rmField) { env->ExceptionClear(); rmField = env->GetFieldID(mcCls, "renderManager", "Lnet/minecraft/client/renderer/entity/RenderManager;"); }
    env->ExceptionClear();
    if (!rmField) { env->DeleteLocalRef(mcCls); return false; }

    jobject rm = env->GetObjectField(StrayCache::MinecraftInstance, rmField);
    env->DeleteLocalRef(mcCls);
    if (!rm) return false;

    jclass rmCls = env->GetObjectClass(rm);
    if (!rmCls) { env->DeleteLocalRef(rm); return false; }

    jfieldID vxID = env->GetFieldID(rmCls, "c", "D");
    if (!vxID) { env->ExceptionClear(); vxID = env->GetFieldID(rmCls, "viewerPosX", "D"); }
    env->ExceptionClear();
    jfieldID vyID = env->GetFieldID(rmCls, "d", "D");
    if (!vyID) { env->ExceptionClear(); vyID = env->GetFieldID(rmCls, "viewerPosY", "D"); }
    env->ExceptionClear();
    jfieldID vzID = env->GetFieldID(rmCls, "e", "D");
    if (!vzID) { env->ExceptionClear(); vzID = env->GetFieldID(rmCls, "viewerPosZ", "D"); }
    env->ExceptionClear();

    if (!vxID || !vyID || !vzID) { env->DeleteLocalRef(rmCls); env->DeleteLocalRef(rm); return false; }

    double vx = env->GetDoubleField(rm, vxID);
    double vy = env->GetDoubleField(rm, vyID);
    double vz = env->GetDoubleField(rm, vzID);

    env->DeleteLocalRef(rmCls);
    env->DeleteLocalRef(rm);

    double dx = x - vx;
    double dy = y - vy;
    double dz = z - vz;
    if (fabs(dz) < 0.01) dz = 0.01;

    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImVec2 center(screen.x * 0.5f, screen.y * 0.5f);
    double fov = 70.0 * 0.5 * 3.141592653589793 / 180.0;
    double tanFov = tan(fov);
    double aspect = screen.x / screen.y;
    double ax = dx / dz;
    double ay = dy / dz;
    sx = (float)(center.x + center.x * ax / (tanFov * aspect));
    sy = (float)(center.y - center.y * ay / tanFov);

    return sx >= 0 && sx <= screen.x && sy >= 0 && sy <= screen.y;
}

SkeletonESPModule::SkeletonESPModule()
    : ModuleBase("SkeletonESP", "Draws skeleton bones on players", Category::Render)
{
    AddSetting<ColorSetting>("Color", ImColor(1.0f, 1.0f, 1.0f, 1.0f));
    AddSetting<NumberSetting>("LineWidth", 1.5f, 0.5f, 3.0f, 0.1f);
    AddSetting<NumberSetting>("Range", 50.0f, 10.0f, 100.0f, 1.0f);
}

void SkeletonESPModule::OnEnable() { Logger::Log("SkeletonESP enabled"); }
void SkeletonESPModule::OnDisable() { Logger::Log("SkeletonESP disabled"); }

void SkeletonESPModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject world = GetWorldObject(env, mc);
    if (!world) { env->DeleteLocalRef(mc); return; }

    jclass wCls = env->GetObjectClass(world);
    if (!wCls) { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jobject playerList = nullptr;
    jmethodID getPlayers = env->GetMethodID(wCls, "j", "()Ljava/util/List;");
    if (!getPlayers) { env->ExceptionClear(); getPlayers = env->GetMethodID(wCls, "getPlayerEntities", "()Ljava/util/List;"); }
    if (getPlayers) { env->ExceptionClear(); playerList = env->CallObjectMethod(world, getPlayers); }
    if (!playerList)
    {
        jfieldID eflid = env->GetFieldID(wCls, "e", "Ljava/util/List;");
        if (!eflid) { env->ExceptionClear(); eflid = env->GetFieldID(wCls, "playerEntities", "Ljava/util/List;"); }
        env->ExceptionClear();
        if (eflid) playerList = env->GetObjectField(world, eflid);
    }
    if (!playerList) { env->DeleteLocalRef(wCls); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jobject localPlayer = GetPlayerObject(env, mc);
    if (!localPlayer) { env->DeleteLocalRef(playerList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jclass lCls = env->FindClass("java/util/List");
    jmethodID lSize = env->GetMethodID(lCls, "size", "()I");
    jmethodID lGet = env->GetMethodID(lCls, "get", "(I)Ljava/lang/Object;");
    jint eSize = env->CallIntMethod(playerList, lSize);

    double pX = 0.0, pY = 0.0, pZ = 0.0;
    jclass eCls = StrayCache::Entity;
    if (!eCls) { env->DeleteLocalRef(lCls); env->DeleteLocalRef(localPlayer); env->DeleteLocalRef(playerList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jfieldID posXID = env->GetFieldID(eCls, "r", "D");
    if (!posXID) { env->ExceptionClear(); posXID = env->GetFieldID(eCls, "posX", "D"); }
    env->ExceptionClear();
    jfieldID posYID = env->GetFieldID(eCls, "s", "D");
    if (!posYID) { env->ExceptionClear(); posYID = env->GetFieldID(eCls, "posY", "D"); }
    env->ExceptionClear();
    jfieldID posZID = env->GetFieldID(eCls, "t", "D");
    if (!posZID) { env->ExceptionClear(); posZID = env->GetFieldID(eCls, "posZ", "D"); }
    env->ExceptionClear();

    jfieldID isDeadID = env->GetFieldID(eCls, "J", "Z");
    if (!isDeadID) { env->ExceptionClear(); isDeadID = env->GetFieldID(eCls, "isDead", "Z"); }
    env->ExceptionClear();
    jfieldID heightID = env->GetFieldID(eCls, "o", "F");
    if (!heightID) { env->ExceptionClear(); heightID = env->GetFieldID(eCls, "height", "F"); }
    env->ExceptionClear();

    jclass elbCls = StrayCache::EntityLivingBase;
    if (!elbCls) { env->DeleteLocalRef(lCls); env->DeleteLocalRef(localPlayer); env->DeleteLocalRef(playerList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jfieldID limbSwingID = env->GetFieldID(elbCls, "R", "F");
    if (!limbSwingID) { env->ExceptionClear(); limbSwingID = env->GetFieldID(elbCls, "limbSwing", "F"); }
    env->ExceptionClear();
    jfieldID limbSwingAmountID = env->GetFieldID(elbCls, "S", "F");
    if (!limbSwingAmountID) { env->ExceptionClear(); limbSwingAmountID = env->GetFieldID(elbCls, "limbSwingAmount", "F"); }
    env->ExceptionClear();

    pX = env->GetDoubleField(localPlayer, posXID);
    pY = env->GetDoubleField(localPlayer, posYID);
    pZ = env->GetDoubleField(localPlayer, posZID);

    jclass playerCls = StrayCache::EntityPlayer;
    float range = ((NumberSetting*)FindSetting("Range"))->GetValue();
    std::vector<SkeletonData> newData;

    for (jint i = 0; i < eSize; i++)
    {
        jobject ent = env->CallObjectMethod(playerList, lGet, i);
        if (!ent) continue;

        if (env->IsSameObject(ent, localPlayer)) { env->DeleteLocalRef(ent); continue; }

        bool isPlayer = playerCls && env->IsInstanceOf(ent, playerCls);
        if (!isPlayer) { env->DeleteLocalRef(ent); continue; }

        bool dead = env->GetBooleanField(ent, isDeadID) == JNI_TRUE;
        if (dead) { env->DeleteLocalRef(ent); continue; }

        double eX = env->GetDoubleField(ent, posXID);
        double eY = env->GetDoubleField(ent, posYID);
        double eZ = env->GetDoubleField(ent, posZID);

        double dist = std::sqrt((eX - pX) * (eX - pX) + (eY - pY) * (eY - pY) + (eZ - pZ) * (eZ - pZ));
        if (dist > range) { env->DeleteLocalRef(ent); continue; }

        float height = heightID ? env->GetFloatField(ent, heightID) : 1.8f;
        float limbSwing = limbSwingID ? env->GetFloatField(ent, limbSwingID) : 0.0f;
        float limbSwingAmount = limbSwingAmountID ? env->GetFloatField(ent, limbSwingAmountID) : 0.0f;

        newData.push_back({ eX, eY, eZ, limbSwing, limbSwingAmount, height });
        env->DeleteLocalRef(ent);
    }

    env->DeleteLocalRef(lCls);
    env->DeleteLocalRef(playerList);
    env->DeleteLocalRef(wCls);
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
    float range = ((NumberSetting*)FindSetting("Range"))->GetValue();

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

        SkeletonBone head = { x, headY, z };
        SkeletonBone neck = { x, neckY, z };
        SkeletonBone shoulders = { x, shoulderY, z };
        SkeletonBone leftArm = { x - armHSpread - armSwing, shoulderY, z };
        SkeletonBone rightArm = { x + armHSpread + armSwing, shoulderY, z };
        SkeletonBone spine = { x, spineY, z };
        SkeletonBone pelvis = { x, pelvisY, z };
        SkeletonBone leftLeg = { x - legSpread + legSwing, footY, z };
        SkeletonBone rightLeg = { x + legSpread - legSwing, footY, z };

        SkeletonBone bones[] = { head, neck, shoulders, leftArm, rightArm, spine, pelvis, leftLeg, rightLeg };
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

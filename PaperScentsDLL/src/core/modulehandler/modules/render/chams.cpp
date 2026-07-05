#include "chams.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <imgui.h>
#include <cmath>
#include <cfloat>

bool ChamsModule::WorldToScreen(JNIEnv* env, double x, double y, double z, float& sx, float& sy)
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

ChamsModule::ChamsModule()
    : ModuleBase("Chams", "See entities through walls", Category::Render)
{
    AddSetting<ColorSetting>("Color", ImColor(0.0f, 1.0f, 0.0f, 0.3f));
    AddSetting<BooleanSetting>("Players", true, "Show chams on players");
    AddSetting<BooleanSetting>("Mobs", false, "Show chams on mobs");
}

void ChamsModule::OnEnable() { Logger::Log("Chams enabled"); }
void ChamsModule::OnDisable() { Logger::Log("Chams disabled"); }

void ChamsModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter % 3 != 0) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject world = GetWorldObject(env, mc);
    if (!world) { env->DeleteLocalRef(mc); return; }

    jfieldID entitiesField = env->GetFieldID(StrayCache::World, "g", "Ljava/util/List;");
    if (!entitiesField) { env->ExceptionClear(); entitiesField = env->GetFieldID(StrayCache::World, "loadedEntityList", "Ljava/util/List;"); }
    env->ExceptionClear();
    if (!entitiesField) { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jobject listObj = env->GetObjectField(world, entitiesField);
    if (!listObj) { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jclass listClass = env->FindClass("java/util/List");
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    if (!sizeMethod || !getMethod)
    {
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(listObj);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(mc);
        return;
    }

    jobject localPlayer = GetPlayerObject(env, mc);
    if (!localPlayer)
    {
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(listObj);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(mc);
        return;
    }

    bool showPlayers = ((BooleanSetting*)FindSetting("Players"))->GetValue();
    bool showMobs = ((BooleanSetting*)FindSetting("Mobs"))->GetValue();

    jfieldID posXID = env->GetFieldID(StrayCache::Entity, "r", "D");
    if (!posXID) { env->ExceptionClear(); posXID = env->GetFieldID(StrayCache::Entity, "posX", "D"); }
    env->ExceptionClear();
    jfieldID posYID = env->GetFieldID(StrayCache::Entity, "s", "D");
    if (!posYID) { env->ExceptionClear(); posYID = env->GetFieldID(StrayCache::Entity, "posY", "D"); }
    env->ExceptionClear();
    jfieldID posZID = env->GetFieldID(StrayCache::Entity, "t", "D");
    if (!posZID) { env->ExceptionClear(); posZID = env->GetFieldID(StrayCache::Entity, "posZ", "D"); }
    env->ExceptionClear();
    jfieldID widthID = env->GetFieldID(StrayCache::Entity, "n", "F");
    if (!widthID) { env->ExceptionClear(); widthID = env->GetFieldID(StrayCache::Entity, "width", "F"); }
    env->ExceptionClear();
    jfieldID heightID = env->GetFieldID(StrayCache::Entity, "o", "F");
    if (!heightID) { env->ExceptionClear(); heightID = env->GetFieldID(StrayCache::Entity, "height", "F"); }
    env->ExceptionClear();
    jfieldID isDeadID = env->GetFieldID(StrayCache::Entity, "J", "Z");
    if (!isDeadID) { env->ExceptionClear(); isDeadID = env->GetFieldID(StrayCache::Entity, "isDead", "Z"); }
    env->ExceptionClear();

    jmethodID getNameMid = env->GetMethodID(StrayCache::Entity, "c", "()Ljava/lang/String;");
    if (!getNameMid) { env->ExceptionClear(); getNameMid = env->GetMethodID(StrayCache::Entity, "getName", "()Ljava/lang/String;"); }
    env->ExceptionClear();

    jmethodID getEntityIdMid = env->GetMethodID(StrayCache::Entity, "q", "()I");
    if (!getEntityIdMid) { env->ExceptionClear(); getEntityIdMid = env->GetMethodID(StrayCache::Entity, "getEntityId", "()I"); }
    env->ExceptionClear();

    jmethodID canSeeMid = env->GetMethodID(StrayCache::Entity, "b", "(Lpk;)Z");
    if (!canSeeMid) { env->ExceptionClear(); canSeeMid = env->GetMethodID(StrayCache::Entity, "canEntityBeSeen", "(Lnet/minecraft/entity/Entity;)Z"); }
    env->ExceptionClear();

    jint eSize = env->CallIntMethod(listObj, sizeMethod);
    int localId = -1;
    if (getEntityIdMid) localId = env->CallIntMethod(localPlayer, getEntityIdMid);

    std::vector<ChamsData> newData;

    for (jint i = 0; i < eSize; i++)
    {
        jobject entity = env->CallObjectMethod(listObj, getMethod, i);
        if (!entity) continue;

        if (env->IsSameObject(entity, localPlayer)) { env->DeleteLocalRef(entity); continue; }

        bool isPlayer = env->IsInstanceOf(entity, StrayCache::EntityPlayer);
        bool isLivingBase = env->IsInstanceOf(entity, StrayCache::EntityLivingBase);

        if (isPlayer && !showPlayers) { env->DeleteLocalRef(entity); continue; }
        if (!isPlayer && isLivingBase && !showMobs) { env->DeleteLocalRef(entity); continue; }
        if (!isLivingBase) { env->DeleteLocalRef(entity); continue; }

        bool dead = isDeadID ? (env->GetBooleanField(entity, isDeadID) == JNI_TRUE) : true;
        if (dead) { env->DeleteLocalRef(entity); continue; }

        if (canSeeMid)
        {
            jboolean visible = env->CallBooleanMethod(localPlayer, canSeeMid, entity);
            if (visible) { env->DeleteLocalRef(entity); continue; }
        }

        double eX = posXID ? env->GetDoubleField(entity, posXID) : 0.0;
        double eY = posYID ? env->GetDoubleField(entity, posYID) : 0.0;
        double eZ = posZID ? env->GetDoubleField(entity, posZID) : 0.0;
        float h = heightID ? env->GetFloatField(entity, heightID) : 1.8f;
        float w = widthID ? env->GetFloatField(entity, widthID) : 0.6f;

        std::string name;
        if (getNameMid)
        {
            jobject nameObj = env->CallObjectMethod(entity, getNameMid);
            if (nameObj)
            {
                const char* str = env->GetStringUTFChars((jstring)nameObj, nullptr);
                if (str) { name = str; env->ReleaseStringUTFChars((jstring)nameObj, str); }
                env->DeleteLocalRef(nameObj);
            }
        }

        newData.push_back({ eX, eY, eZ, h, w, name });
        env->DeleteLocalRef(entity);
    }

    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(listObj);
    env->DeleteLocalRef(localPlayer);
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
                left = fminf(left, sx); top = fminf(top, sy);
                right = fmaxf(right, sx); bottom = fmaxf(bottom, sy);
            }
        }

        if (!anyValid || left >= right || top >= bottom) continue;

        draw->AddRectFilled(ImVec2(left, top), ImVec2(right, bottom), col);
        draw->AddRect(ImVec2(left, top), ImVec2(right, bottom), ImColor(0, 0, 0, 200), 0.0f, 0, 1.5f);
    }
}

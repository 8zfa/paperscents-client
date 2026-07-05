#include "itemesp.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <imgui.h>
#include <cmath>

bool ItemESPModule::WorldToScreen(JNIEnv* env, double x, double y, double z, float& sx, float& sy)
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

ItemESPModule::ItemESPModule()
    : ModuleBase("ItemESP", "Shows dropped items through walls", Category::Render)
{
    AddSetting<ColorSetting>("Color", ImColor(1.0f, 1.0f, 0.0f, 0.8f));
    AddSetting<BooleanSetting>("Nametags", true, "Show item name tags");
}

void ItemESPModule::OnEnable() { Logger::Log("ItemESP enabled"); }
void ItemESPModule::OnDisable() { Logger::Log("ItemESP disabled"); }

void ItemESPModule::OnUpdate()
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

    jclass entityItemCls = env->FindClass("sa");
    if (!entityItemCls) { env->ExceptionClear(); entityItemCls = env->FindClass("net/minecraft/entity/item/EntityItem"); }
    env->ExceptionClear();

    jfieldID posXID = env->GetFieldID(StrayCache::Entity, "r", "D");
    if (!posXID) { env->ExceptionClear(); posXID = env->GetFieldID(StrayCache::Entity, "posX", "D"); }
    env->ExceptionClear();
    jfieldID posYID = env->GetFieldID(StrayCache::Entity, "s", "D");
    if (!posYID) { env->ExceptionClear(); posYID = env->GetFieldID(StrayCache::Entity, "posY", "D"); }
    env->ExceptionClear();
    jfieldID posZID = env->GetFieldID(StrayCache::Entity, "t", "D");
    if (!posZID) { env->ExceptionClear(); posZID = env->GetFieldID(StrayCache::Entity, "posZ", "D"); }
    env->ExceptionClear();

    jmethodID getItemStackMid = nullptr;
    if (entityItemCls)
    {
        jmethodID gisMid = env->GetMethodID(entityItemCls, "d", "()Ljava/lang/Object;");
        if (!gisMid) { env->ExceptionClear(); gisMid = env->GetMethodID(entityItemCls, "getEntityItem", "()Lnet/minecraft/item/ItemStack;"); }
        env->ExceptionClear();
        getItemStackMid = gisMid;
    }

    jint eSize = env->CallIntMethod(listObj, sizeMethod);
    std::vector<ItemESPData> newData;

    for (jint i = 0; i < eSize; i++)
    {
        jobject entity = env->CallObjectMethod(listObj, getMethod, i);
        if (!entity) continue;

        bool isItem = entityItemCls && env->IsInstanceOf(entity, entityItemCls);
        if (!isItem) { env->DeleteLocalRef(entity); continue; }

        std::string displayName;
        if (getItemStackMid)
        {
            jobject itemStack = env->CallObjectMethod(entity, getItemStackMid);
            if (itemStack)
            {
                jclass stackCls = env->GetObjectClass(itemStack);
                if (stackCls)
                {
                    jmethodID displayMid = env->GetMethodID(stackCls, "c", "()Ljava/lang/String;");
                    if (!displayMid) { env->ExceptionClear(); displayMid = env->GetMethodID(stackCls, "getDisplayName", "()Ljava/lang/String;"); }
                    env->ExceptionClear();

                    if (displayMid)
                    {
                        jobject nameObj = env->CallObjectMethod(itemStack, displayMid);
                        if (nameObj)
                        {
                            const char* str = env->GetStringUTFChars((jstring)nameObj, nullptr);
                            if (str) { displayName = str; env->ReleaseStringUTFChars((jstring)nameObj, str); }
                            env->DeleteLocalRef(nameObj);
                        }
                    }
                    env->DeleteLocalRef(stackCls);
                }
                env->DeleteLocalRef(itemStack);
            }
        }

        if (displayName.empty()) { env->DeleteLocalRef(entity); continue; }

        double eX = posXID ? env->GetDoubleField(entity, posXID) : 0.0;
        double eY = posYID ? env->GetDoubleField(entity, posYID) : 0.0;
        double eZ = posZID ? env->GetDoubleField(entity, posZID) : 0.0;

        newData.push_back({ eX, eY, eZ, displayName });
        env->DeleteLocalRef(entity);
    }

    if (entityItemCls) env->DeleteLocalRef(entityItemCls);
    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(listObj);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(mc);

    m_RenderData = newData;
}

void ItemESPModule::OnRender()
{
    if (!IsEnabled() || m_RenderData.empty()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImColor color = ((ColorSetting*)FindSetting("Color"))->GetValue();
    ImU32 col = color;
    bool showNametags = ((BooleanSetting*)FindSetting("Nametags"))->GetValue();

    for (auto& data : m_RenderData)
    {
        float sx, sy;
        if (!WorldToScreen(env, data.x, data.y, data.z, sx, sy)) continue;

        float boxSize = 4.0f;
        draw->AddRectFilled(ImVec2(sx - boxSize, sy - boxSize), ImVec2(sx + boxSize, sy + boxSize), col);
        draw->AddRect(ImVec2(sx - boxSize - 1, sy - boxSize - 1), ImVec2(sx + boxSize + 1, sy + boxSize + 1), ImColor(0, 0, 0, 200), 0.0f, 0, 1.0f);

        if (showNametags)
        {
            ImVec2 textPos(sx + boxSize + 2, sy - 4);
            draw->AddText(textPos, ImColor(255, 255, 255, 255), data.displayName.c_str());
        }
    }
}

#include "itemesp.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <imgui.h>
#include <cmath>

ItemESPModule::ItemESPModule()
    : ModuleBase("ItemESP", "Shows dropped items through walls", Category::Render)
{
    AddSetting<ColorSetting>("Color", ImColor(1.0f, 1.0f, 0.0f, 0.8f));
    AddSetting<BooleanSetting>("Nametags", true, "Show item name tags");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void ItemESPModule::OnEnable() { Logger::Log("ItemESP enabled"); }
void ItemESPModule::OnDisable() { Logger::Log("ItemESP disabled"); }

void ItemESPModule::OnUpdate()
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

    jclass entityItemCls = env->FindClass("sa");
    if (!entityItemCls) { env->ExceptionClear(); entityItemCls = env->FindClass("net/minecraft/entity/item/EntityItem"); }
    env->ExceptionClear();

    jmethodID getItemStackMid = nullptr;
    if (entityItemCls)
    {
        jmethodID gisMid = env->GetMethodID(entityItemCls, "d", "()Ljava/lang/Object;");
        if (!gisMid) { env->ExceptionClear(); gisMid = env->GetMethodID(entityItemCls, "getEntityItem", "()Lnet/minecraft/item/ItemStack;"); }
        env->ExceptionClear();
        getItemStackMid = gisMid;
    }

    jint eSize = env->CallIntMethod(listObj, StrayCache::List_size);
    std::vector<ItemESPData> newData;

    for (jint i = 0; i < eSize; i++)
    {
        jobject entity = env->CallObjectMethod(listObj, StrayCache::List_get, i);
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

        double eX = GetEntityPosX(env, entity);
        double eY = GetEntityPosY(env, entity);
        double eZ = GetEntityPosZ(env, entity);

        newData.push_back({ eX, eY, eZ, displayName });
        env->DeleteLocalRef(entity);
    }

    if (entityItemCls) env->DeleteLocalRef(entityItemCls);
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

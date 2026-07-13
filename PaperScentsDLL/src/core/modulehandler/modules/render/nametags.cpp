#include "nametags.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <imgui.h>
#include <cmath>
#include <string>
#include <vector>
#include <cfloat>

std::string NametagsModule::StripColor(const std::string& input)
{
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); i++)
    {
        if (input[i] == '\xC2' && i + 1 < input.size() && input[i+1] == '\xA7') { i += 2; continue; }
        if (input[i] == '\xA7' && i + 1 < input.size()) { i++; continue; }
        out += input[i];
    }
    return out;
}

NametagsModule::NametagsModule()
    : ModuleBase("Nametags", "Show player names above heads", Category::Render)
{
    AddSetting<NumberSetting>("Scale", 1.0f, 0.5f, 3.0f, 0.1f);
    AddSetting<NumberSetting>("Offset", 0.0f, -20.0f, 20.0f, 1.0f);
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void NametagsModule::OnEnable() { Logger::Log("Nametags enabled"); }
void NametagsModule::OnDisable() { Logger::Log("Nametags disabled"); }

void NametagsModule::OnUpdate()
{
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

    jobject entityList = env->GetObjectField(world, StrayCache::World_playerEntities);
    if (!entityList) { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    double pX = GetEntityPosX(env, player);
    double pY = GetEntityPosY(env, player);
    double pZ = GetEntityPosZ(env, player);

    jint eSize = env->CallIntMethod(entityList, StrayCache::List_size);

    int localId = GetEntityId(env, player);
    float scale = ((NumberSetting*)FindSetting("Scale"))->GetValue();
    float offset = ((NumberSetting*)FindSetting("Offset"))->GetValue();
    std::vector<NameTagData> newTags;

    for (jint i = 0; i < eSize; i++)
    {
        jobject ent = env->CallObjectMethod(entityList, StrayCache::List_get, i);
        if (!ent) continue;

        int entId = GetEntityId(env, ent);
        if (entId == localId || entId < 0) { env->DeleteLocalRef(ent); continue; }
        if (IsEntityDead(env, ent)) { env->DeleteLocalRef(ent); continue; }

        std::string name = GetEntityName(env, ent);
        if (name.empty()) { env->DeleteLocalRef(ent); continue; }

        double eX = GetEntityPosX(env, ent);
        double eY = GetEntityPosY(env, ent);
        double eZ = GetEntityPosZ(env, ent);
        float height = GetEntityHeight(env, ent);

        double dist = std::sqrt((eX-pX)*(eX-pX) + (eY-pY)*(eY-pY) + (eZ-pZ)*(eZ-pZ));
        if (dist > 100.0) { env->DeleteLocalRef(ent); continue; }

        float sx, sy;
        if (WorldToScreen(env, eX, eY + height + 0.4f + offset, eZ, sx, sy))
        {
            float opacity = 1.0f;
            if (dist > 50.0f) opacity = 1.0f - (float)((dist - 50.0f) / 50.0f);
            newTags.push_back({ sx, sy, name, opacity });
        }
        env->DeleteLocalRef(ent);
    }

    env->DeleteLocalRef(entityList);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(mc);

    m_Tags = newTags;
}

void NametagsModule::OnRender()
{
    if (!IsEnabled() || m_Tags.empty()) return;

    float scale = ((NumberSetting*)FindSetting("Scale"))->GetValue();
    ImDrawList* draw = ImGui::GetForegroundDrawList();

    for (auto& t : m_Tags)
    {
        std::string clean = StripColor(t.text);
        ImVec2 textSize = ImGui::CalcTextSize(clean.c_str());
        float w = textSize.x * scale;
        float h = textSize.y * scale;
        ImVec2 pos(t.x - w * 0.5f, t.y - h);
        ImU32 bg = ImColor(0, 0, 0, (int)(100 * t.opacity));
        ImU32 fg = ImColor(255, 255, 255, (int)(255 * t.opacity));
        draw->AddRectFilled(ImVec2(pos.x - 2, pos.y - 2), ImVec2(pos.x + w + 2, pos.y + h + 2), bg, 2.0f);
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale, pos, fg, clean.c_str());
    }
}

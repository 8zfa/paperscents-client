#include "directionhud.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <imgui.h>
#include <cmath>
#include <string>

DirectionHUDModule::DirectionHUDModule()
    : ModuleBase("DirectionHUD", "Shows your current facing direction", Category::Render)
{
    AddSetting<NumberSetting>("X", 10.0f, -1000.0f, 2000.0f, 1.0f, "X position");
    AddSetting<NumberSetting>("Y", 10.0f, -1000.0f, 2000.0f, 1.0f, "Y position");
    AddSetting<ColorSetting>("Color", ImColor(1, 1, 1, 1));
}

void DirectionHUDModule::OnEnable() { Logger::Log("DirectionHUD enabled"); }
void DirectionHUDModule::OnDisable() { Logger::Log("DirectionHUD disabled"); }

void DirectionHUDModule::OnRender()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jfieldID yawID = env->GetFieldID(StrayCache::Entity, "v", "F");
    if (!yawID) { env->ExceptionClear(); yawID = env->GetFieldID(StrayCache::Entity, "rotationYaw", "F"); }
    env->ExceptionClear();

    float yaw = yawID ? env->GetFloatField(player, yawID) : 0.0f;

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);

    yaw = fmodf(yaw, 360.0f);
    if (yaw < 0) yaw += 360.0f;

    const char* direction;
    if (yaw >= 337.5f || yaw < 22.5f)
        direction = "South";
    else if (yaw >= 22.5f && yaw < 67.5f)
        direction = "SW";
    else if (yaw >= 67.5f && yaw < 112.5f)
        direction = "West";
    else if (yaw >= 112.5f && yaw < 157.5f)
        direction = "NW";
    else if (yaw >= 157.5f && yaw < 202.5f)
        direction = "North";
    else if (yaw >= 202.5f && yaw < 247.5f)
        direction = "NE";
    else if (yaw >= 247.5f && yaw < 292.5f)
        direction = "East";
    else
        direction = "SE";

    float x = ((NumberSetting*)FindSetting("X"))->GetValue();
    float y = ((NumberSetting*)FindSetting("Y"))->GetValue();
    ImColor color = ((ColorSetting*)FindSetting("Color"))->GetValue();
    ImU32 col = color;

    std::string text = std::string(direction) + " (" + std::to_string((int)yaw) + ")";
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    draw->AddText(ImVec2(x, y), col, text.c_str());
}

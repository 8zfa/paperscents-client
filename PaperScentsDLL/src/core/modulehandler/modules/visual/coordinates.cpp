#include "coordinates.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <imgui.h>
#include <cmath>

CoordinatesModule::CoordinatesModule()
    : ModuleBase("Coordinates", "Display position on screen", Category::Render)
{
    AddSetting<NumberSetting>("X", 4.0f, 0.0f, 2000.0f, 1.0f);
    AddSetting<NumberSetting>("Y", 60.0f, 0.0f, 2000.0f, 1.0f);
    AddSetting<BooleanSetting>("Nether", true, "Show nether coordinates");
    AddSetting<BooleanSetting>("Direction", true, "Show facing direction");
    AddSetting<ColorSetting>("Color", ImColor(1.0f, 1.0f, 1.0f, 1.0f));
}

void CoordinatesModule::OnEnable() { Logger::Log("Coordinates enabled"); }
void CoordinatesModule::OnDisable() { Logger::Log("Coordinates disabled"); }

static const char* GetDirection(float yaw)
{
    yaw = fmodf(yaw, 360.0f);
    if (yaw < 0) yaw += 360.0f;

    if (yaw >= 337.5f || yaw < 22.5f) return "S";
    if (yaw >= 22.5f && yaw < 67.5f) return "SW";
    if (yaw >= 67.5f && yaw < 112.5f) return "W";
    if (yaw >= 112.5f && yaw < 157.5f) return "NW";
    if (yaw >= 157.5f && yaw < 202.5f) return "N";
    if (yaw >= 202.5f && yaw < 247.5f) return "NE";
    if (yaw >= 247.5f && yaw < 292.5f) return "E";
    if (yaw >= 292.5f && yaw < 337.5f) return "SE";
    return "S";
}

void CoordinatesModule::OnRender()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    jclass mcClass = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) { env->ExceptionClear(); mcClass = env->FindClass("net/minecraft/client/Minecraft"); }
    if (!mcClass) return;

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "a", "()Lave;"); }
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "getMinecraft", "()Lave;"); }
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    env->ExceptionClear();

    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    jfieldID playerField = env->GetFieldID(mcClass, "h", "Lbew;");
    if (!playerField) { env->ExceptionClear(); playerField = env->GetFieldID(mcClass, "thePlayer", "Lbew;"); }
    env->ExceptionClear();

    jobject player = playerField ? env->GetObjectField(mc, playerField) : nullptr;
    if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass entityClass = Core::GetInstance().GetJava()->FindClass("pk", "net/minecraft/entity/Entity");
    if (!entityClass) { env->ExceptionClear(); entityClass = env->FindClass("net/minecraft/entity/Entity"); }

    jfieldID posXID = env->GetFieldID(entityClass, "r", "D");
    if (!posXID) { env->ExceptionClear(); posXID = env->GetFieldID(entityClass, "posX", "D"); }
    env->ExceptionClear();
    jfieldID posYID = env->GetFieldID(entityClass, "s", "D");
    if (!posYID) { env->ExceptionClear(); posYID = env->GetFieldID(entityClass, "posY", "D"); }
    env->ExceptionClear();
    jfieldID posZID = env->GetFieldID(entityClass, "t", "D");
    if (!posZID) { env->ExceptionClear(); posZID = env->GetFieldID(entityClass, "posZ", "D"); }
    env->ExceptionClear();
    jfieldID yawID = env->GetFieldID(entityClass, "N", "F");
    if (!yawID) { env->ExceptionClear(); yawID = env->GetFieldID(entityClass, "rotationYaw", "F"); }
    env->ExceptionClear();

    double px = posXID ? env->GetDoubleField(player, posXID) : 0.0;
    double py = posYID ? env->GetDoubleField(player, posYID) : 0.0;
    double pz = posZID ? env->GetDoubleField(player, posZID) : 0.0;
    float yaw = yawID ? env->GetFloatField(player, yawID) : 0.0f;

    if (entityClass) env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);

    float x = ((NumberSetting*)FindSetting("X"))->GetValue();
    float y = ((NumberSetting*)FindSetting("Y"))->GetValue();
    bool showNether = ((BooleanSetting*)FindSetting("Nether"))->GetValue();
    bool showDirection = ((BooleanSetting*)FindSetting("Direction"))->GetValue();
    ImColor color = ((ColorSetting*)FindSetting("Color"))->GetValue();
    ImU32 col = color;

    ImDrawList* draw = ImGui::GetForegroundDrawList();

    char coordText[256];
    snprintf(coordText, sizeof(coordText), "XYZ: %.1f / %.1f / %.1f", px, py, pz);
    draw->AddText(ImVec2(x, y), col, coordText);
    ImVec2 textSize = ImGui::CalcTextSize(coordText);
    float curY = y + textSize.y + 2.0f;

    if (showNether)
    {
        double nx = px / 8.0;
        double nz = pz / 8.0;
        char netherText[128];
        snprintf(netherText, sizeof(netherText), "Nether: %.1f / %.1f", nx, nz);
        draw->AddText(ImVec2(x, curY), col, netherText);
        curY += textSize.y + 2.0f;
    }

    if (showDirection)
    {
        float exactYaw = yaw;
        if (exactYaw < 0) exactYaw += 360.0f;
        const char* dir = GetDirection(yaw);
        char dirText[64];
        snprintf(dirText, sizeof(dirText), "Facing: %s (%.1f)", dir, exactYaw);
        draw->AddText(ImVec2(x, curY), col, dirText);
    }
}

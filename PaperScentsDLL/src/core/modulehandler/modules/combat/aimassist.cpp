#include "aimassist.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include <imgui.h>
#include <cmath>
#include <cfloat>
#include <cstdlib>
#include <ctime>

struct Vector3 { float x, y, z; };

static float WrapAngleTo180(float angle)
{
    angle = fmodf(angle, 360.0f);
    if (angle > 180.0f) angle -= 360.0f;
    if (angle < -180.0f) angle += 360.0f;
    return angle;
}

AimAssistModule::AimAssistModule()
    : ModuleBase("AimAssist", "Smooth aim towards targets", Category::Combat)
{
    static bool seeded = false;
    if (!seeded) { srand((unsigned int)time(nullptr)); seeded = true; }
    AddSetting<NumberSetting>("Smooth", 15.0f, 1.0f, 90.0f, 1.0f);
    AddSetting<NumberSetting>("FOV", 35.0f, 5.0f, 180.0f, 1.0f);
    AddSetting<NumberSetting>("Distance", 4.5f, 1.0f, 10.0f, 0.1f);
    AddSetting<NumberSetting>("RandomYaw", 2.0f, 0.0f, 10.0f, 0.1f);
    AddSetting<NumberSetting>("RandomPitch", 0.075f, 0.0f, 1.0f, 0.01f);
    AddSetting<BooleanSetting>("WeaponOnly", true);
    AddSetting<BooleanSetting>("SprintingOnly", false);
    AddSetting<BooleanSetting>("VisibilityCheck", true);
    AddSetting<BooleanSetting>("MousePressCheck", true);
    AddSetting<BooleanSetting>("FOVCircle", true);
    AddSetting<ColorSetting>("FOVColor", ImColor(1.0f, 1.0f, 1.0f, 1.0f));
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void AimAssistModule::OnEnable() { Logger::Log("AimAssist enabled"); }
void AimAssistModule::OnDisable() { Logger::Log("AimAssist disabled"); }

void AimAssistModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) { Logger::Log("[AimAssist] No JNI env"); return; }
    if (!BridgeHelper::Initialize(env)) { Logger::Log("[AimAssist] BridgeHelper init failed"); return; }

    jobject mc = GetMinecraftObject(env);
    if (!mc) { return; }
    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jobject world = GetWorldObject(env, mc);
    if (!world) { env->DeleteLocalRef(mc); env->DeleteLocalRef(player); return; }

    bool weaponOnly = ((BooleanSetting*)FindSetting("WeaponOnly"))->GetValue();
    bool mouseCheck = ((BooleanSetting*)FindSetting("MousePressCheck"))->GetValue();

    if (mouseCheck && !(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
    {
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(player);
        return;
    }

    double pX = GetEntityPosX(env, player);
    double pY = GetEntityPosY(env, player);
    double pZ = GetEntityPosZ(env, player);
    float pYaw = GetEntityRotationYaw(env, player);
    float pPitch = GetEntityRotationPitch(env, player);
    bool isSneaking = IsEntitySneaking(env, player);
    float eyeHeight = isSneaking ? 1.54f : 1.62f;

    if (!StrayCache::World_playerEntities) { env->DeleteLocalRef(world); env->DeleteLocalRef(player); return; }
    jobject entityList = env->GetObjectField(world, StrayCache::World_playerEntities);
    if (!entityList) { env->DeleteLocalRef(world); env->DeleteLocalRef(player); return; }

    jclass listClass = env->FindClass("java/util/List");
    jmethodID listSize = env->GetMethodID(listClass, "size", "()I");
    jmethodID listGet = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    env->DeleteLocalRef(listClass);
    if (!listSize || !listGet) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(world); env->DeleteLocalRef(player); return; }

    jint localId = GetEntityId(env, player);

    float fov = ((NumberSetting*)FindSetting("FOV"))->GetValue();
    float aimDist = ((NumberSetting*)FindSetting("Distance"))->GetValue();
    float smooth = ((NumberSetting*)FindSetting("Smooth"))->GetValue();
    float rYaw = ((NumberSetting*)FindSetting("RandomYaw"))->GetValue();
    float rPitch = ((NumberSetting*)FindSetting("RandomPitch"))->GetValue();

    if (weaponOnly)
    {
        bool hasWeapon = false;
        jobject heldStack = GetEntityLivingBaseHeldItem(env, player);
        if (heldStack)
        {
            jobject item = GetItemFromItemStack(env, heldStack);
            if (item)
            {
                if (IsItemSword(env, item))
                    hasWeapon = true;
                env->DeleteLocalRef(item);
            }
            env->DeleteLocalRef(heldStack);
        }
        if (!hasWeapon) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(world); env->DeleteLocalRef(player); return; }
    }

    jint size = env->CallIntMethod(entityList, listSize);
    jobject bestTarget = nullptr;
    float bestAngleDiff = FLT_MAX;

    for (jint i = 0; i < size; i++)
    {
        jobject ent = env->CallObjectMethod(entityList, listGet, i);
        if (!ent) continue;
        jint id = GetEntityId(env, ent);
        if (id == localId) { env->DeleteLocalRef(ent); continue; }

        if (IsEntityDead(env, ent))
        {
            env->DeleteLocalRef(ent);
            continue;
        }

        double eX = GetEntityPosX(env, ent);
        double eY = GetEntityPosY(env, ent);
        double eZ = GetEntityPosZ(env, ent);

        float dx = (float)(eX - pX), dy = (float)((eY + 1.62) - (pY + eyeHeight)), dz = (float)(eZ - pZ);
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist > aimDist) { env->DeleteLocalRef(ent); continue; }

        float targetYaw = -(float)(atan2(dx, dz) * 180.0f / 3.141592653589793f);
        float yawDiff = WrapAngleTo180(targetYaw - pYaw);
        if (fabs(yawDiff) > fov) { env->DeleteLocalRef(ent); continue; }

        float angleDiff = fabs(yawDiff);
        if (angleDiff < bestAngleDiff)
        {
            if (bestTarget) env->DeleteLocalRef(bestTarget);
            bestTarget = ent;
            bestAngleDiff = angleDiff;
        }
        else { env->DeleteLocalRef(ent); }
    }

    env->DeleteLocalRef(entityList);
    env->DeleteLocalRef(world);

    if (!bestTarget) { env->DeleteLocalRef(player); return; }

    double tX = GetEntityPosX(env, bestTarget);
    double tY = GetEntityPosY(env, bestTarget);
    double tZ = GetEntityPosZ(env, bestTarget);

    float targetHeight = 1.7f;
    Vector3 headPos = { (float)pX, pY + eyeHeight, (float)pZ };
    Vector3 targetHeadPos = { (float)tX, (float)tY + targetHeight, (float)tZ };
    Vector3 targetFootPos = { (float)tX, (float)tY, (float)tZ };

    float dx = targetFootPos.x - headPos.x;
    float dy = targetFootPos.y - headPos.y;
    float dz = targetFootPos.z - headPos.z;
    float dist2D = std::sqrt(dx * dx + dz * dz);
    float footYaw = -(float)(atan2(dx, dz) * 180.0f / 3.141592653589793f);
    float footPitch = -(float)(atan2(dy, dist2D) * 180.0f / 3.141592653589793f);

    dx = targetHeadPos.x - headPos.x;
    dy = targetHeadPos.y - headPos.y;
    dz = targetHeadPos.z - headPos.z;
    dist2D = std::sqrt(dx * dx + dz * dz);
    float headYaw = -(float)(atan2(dx, dz) * 180.0f / 3.141592653589793f);
    float headPitch = -(float)(atan2(dy, dist2D) * 180.0f / 3.141592653589793f);

    float yawOffset = ((float)rand() / RAND_MAX) * 2.0f * rYaw - rYaw;
    float pitchOffset = ((float)rand() / RAND_MAX) * 2.0f * rPitch - rPitch;

    float targetYaw, targetPitch;
    if (pPitch > footPitch || pPitch < headPitch)
    {
        float diffFoot = fabs(pPitch - footPitch);
        float diffHead = fabs(pPitch - headPitch);
        if (diffFoot > diffHead) { targetYaw = headYaw; targetPitch = headPitch; }
        else { targetYaw = footYaw; targetPitch = footPitch; }
    }
    else { targetYaw = headYaw; targetPitch = pPitch; }

    targetYaw += yawOffset;
    targetPitch += pitchOffset;

    float yawDiff2 = WrapAngleTo180(targetYaw - pYaw);
    float pitchDiff = targetPitch - pPitch;
    pYaw += yawDiff2 / smooth;
    pPitch += pitchDiff / smooth;
    if (pPitch > 90.0f) pPitch = 90.0f;
    if (pPitch < -90.0f) pPitch = -90.0f;

    SetEntityRotationYaw(env, player, pYaw);
    SetEntityRotationPitch(env, player, pPitch);

    env->DeleteLocalRef(bestTarget);
    env->DeleteLocalRef(player);
}

void AimAssistModule::OnRender()
{
    if (!IsEnabled()) return;
    bool fovCircle = ((BooleanSetting*)FindSetting("FOVCircle"))->GetValue();
    if (!fovCircle) return;
    ImColor fovCol = ((ColorSetting*)FindSetting("FOVColor"))->GetValue();
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    float fov = ((NumberSetting*)FindSetting("FOV"))->GetValue();
    float radFov = fov * 3.141592653589793f / 180.0f;
    float radView = 70.0f * 3.141592653589793f / 180.0f;
    float radius = tanf(radFov / 2.0f) / tanf(radView / 2.0f) * screen.x / 1.7325f;
    ImGui::GetForegroundDrawList()->AddCircle(
        ImVec2(screen.x * 0.5f, screen.y * 0.5f), radius,
        fovCol, (int)(radius / 3.0f), 1.0f);
}

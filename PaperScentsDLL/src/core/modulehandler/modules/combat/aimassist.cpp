#include "aimassist.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <imgui.h>
#include <cmath>
#include <cfloat>
#include <cstdlib>
#include <ctime>

static JNIEnv* GetEnv()
{
    auto* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return nullptr;
    return java->GetEnv();
}

static jobject GetMc()
{
    JNIEnv* env = GetEnv();
    if (!env) return nullptr;
    jclass cls = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!cls) { env->ExceptionClear(); cls = env->FindClass("net/minecraft/client/Minecraft"); }
    if (!cls) return nullptr;
    jmethodID get = env->GetStaticMethodID(cls, "A", "()Lave;");
    if (!get) { env->ExceptionClear(); get = env->GetStaticMethodID(cls, "getMinecraft", "()Lnet/minecraft/client/Minecraft;"); }
    if (!get) { env->DeleteLocalRef(cls); return nullptr; }
    env->ExceptionClear();
    jobject mc = env->CallStaticObjectMethod(cls, get);
    env->DeleteLocalRef(cls);
    return mc;
}

static double GetDouble(JNIEnv* env, jobject obj, const char* obf, const char* deobf)
{
    jclass cls = env->GetObjectClass(obj);
    if (!cls) return 0.0;
    jfieldID fid = env->GetFieldID(cls, obf, "D");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(cls, deobf, "D"); }
    double v = 0.0;
    if (fid) v = env->GetDoubleField(obj, fid);
    env->DeleteLocalRef(cls);
    return v;
}

static float GetFloat(JNIEnv* env, jobject obj, const char* obf, const char* deobf)
{
    jclass cls = env->GetObjectClass(obj);
    if (!cls) return 0.0f;
    jfieldID fid = env->GetFieldID(cls, obf, "F");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(cls, deobf, "F"); }
    float v = 0.0f;
    if (fid) v = env->GetFloatField(obj, fid);
    env->DeleteLocalRef(cls);
    return v;
}

static void SetFloat(JNIEnv* env, jobject obj, const char* obf, const char* deobf, float val)
{
    jclass cls = env->GetObjectClass(obj);
    if (!cls) return;
    jfieldID fid = env->GetFieldID(cls, obf, "F");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(cls, deobf, "F"); }
    if (fid) env->SetFloatField(obj, fid, val);
    env->DeleteLocalRef(cls);
}

static std::string GetName(JNIEnv* env, jobject entity)
{
    jclass cls = env->GetObjectClass(entity);
    if (!cls) return "";
    jmethodID mid = env->GetMethodID(cls, "c", "()Ljava/lang/String;");
    if (!mid) { env->ExceptionClear(); mid = env->GetMethodID(cls, "getName", "()Ljava/lang/String;"); }
    env->ExceptionClear();
    if (!mid) { env->DeleteLocalRef(cls); return ""; }
    jobject nameObj = env->CallObjectMethod(entity, mid);
    std::string result;
    if (nameObj)
    {
        const char* str = env->GetStringUTFChars((jstring)nameObj, nullptr);
        if (str) { result = str; env->ReleaseStringUTFChars((jstring)nameObj, str); }
        env->DeleteLocalRef(nameObj);
    }
    env->DeleteLocalRef(cls);
    return result;
}

static float GetHealth(JNIEnv* env, jobject entity)
{
    jclass cls = Core::GetInstance().GetJava()->FindClass("pr", "net/minecraft/entity/EntityLivingBase");
    if (!cls) { env->ExceptionClear(); cls = env->FindClass("net/minecraft/entity/EntityLivingBase"); }
    if (!cls) return 20.0f;
    jmethodID mid = env->GetMethodID(cls, "aM", "()F");
    if (!mid) { env->ExceptionClear(); mid = env->GetMethodID(cls, "getHealth", "()F"); }
    env->ExceptionClear();
    if (!mid) { env->DeleteLocalRef(cls); return 20.0f; }
    float h = env->CallFloatMethod(entity, mid);
    env->DeleteLocalRef(cls);
    return h;
}

static bool IsEntityDead(JNIEnv* env, jobject entity)
{
    jclass cls = env->GetObjectClass(entity);
    if (!cls) return true;
    jfieldID fid = env->GetFieldID(cls, "J", "Z");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(cls, "isDead", "Z"); }
    bool dead = true;
    if (fid) dead = env->GetBooleanField(entity, fid) == JNI_TRUE;
    env->DeleteLocalRef(cls);
    return dead;
}

// Math helpers
struct Vector3 { float x, y, z; };
static float DegToRad(float deg) { return deg * 3.141592653589793f / 180.0f; }
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
}

void AimAssistModule::OnEnable() { Logger::Log("AimAssist enabled"); }
void AimAssistModule::OnDisable() { Logger::Log("AimAssist disabled"); }

void AimAssistModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = GetEnv();
    if (!env) return;

    jobject mc = GetMc();
    if (!mc) return;

    jclass mcCls = env->GetObjectClass(mc);
    if (!mcCls) { env->DeleteLocalRef(mc); return; }

    // Check for GUI state
    jfieldID csFid = env->GetFieldID(mcCls, "m", "Lauh;");
    if (!csFid) { env->ExceptionClear(); csFid = env->GetFieldID(mcCls, "currentScreen", "Lnet/minecraft/client/gui/GuiScreen;"); }
    env->ExceptionClear();
    if (csFid)
    {
        jobject cs = env->GetObjectField(mc, csFid);
        if (cs) {
            env->DeleteLocalRef(cs);
            env->DeleteLocalRef(mcCls);
            env->DeleteLocalRef(mc);
            return;
        }
    }

    jfieldID pfid = env->GetFieldID(mcCls, "u", "Lbew;");
    if (!pfid) { env->ExceptionClear(); pfid = env->GetFieldID(mcCls, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;"); }
    jobject player = pfid ? env->GetObjectField(mc, pfid) : nullptr;
    if (!player) { env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    jfieldID wfid = env->GetFieldID(mcCls, "f", "Ladm;");
    if (!wfid) { env->ExceptionClear(); wfid = env->GetFieldID(mcCls, "theWorld", "Lnet/minecraft/world/World;"); }
    jobject world = wfid ? env->GetObjectField(mc, wfid) : nullptr;
    if (!world) { env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    bool weaponOnly = ((BooleanSetting*)FindSetting("WeaponOnly"))->GetValue();
    bool sprintOnly = ((BooleanSetting*)FindSetting("SprintingOnly"))->GetValue();
    bool visCheck = ((BooleanSetting*)FindSetting("VisibilityCheck"))->GetValue();
    bool mouseCheck = ((BooleanSetting*)FindSetting("MousePressCheck"))->GetValue();

    // Get player data
    float pX = GetDouble(env, player, "r", "posX");
    float pY = GetDouble(env, player, "s", "posY");
    float pZ = GetDouble(env, player, "t", "posZ");
    float pYaw = GetFloat(env, player, "y", "rotationYaw");
    float pPitch = GetFloat(env, player, "z", "rotationPitch");

    // Eye height: 1.62 (standing), 1.54 (sneaking)
    bool sneaking = false;
    jclass eCls = Core::GetInstance().GetJava()->FindClass("pk", "net/minecraft/entity/Entity");
    if (!eCls) { env->ExceptionClear(); eCls = env->FindClass("net/minecraft/entity/Entity"); }
    if (!eCls) { env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
    jmethodID sneakMid = env->GetMethodID(eCls, "aD", "()Z");
    if (!sneakMid) { env->ExceptionClear(); sneakMid = env->GetMethodID(eCls, "isSneaking", "()Z"); }
    env->ExceptionClear();
    if (sneakMid) sneaking = env->CallBooleanMethod(player, sneakMid);
    float eyeHeight = sneaking ? 1.54f : 1.62f;

    // Get entity list
    jclass wCls = env->GetObjectClass(world);
    jfieldID eflid = env->GetFieldID(wCls, "e", "Ljava/util/List;");
    if (!eflid) { env->ExceptionClear(); eflid = env->GetFieldID(wCls, "playerEntities", "Ljava/util/List;"); }
    jobject entityList = eflid ? env->GetObjectField(world, eflid) : nullptr;
    if (!entityList) { env->DeleteLocalRef(wCls); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    jclass lCls = env->FindClass("java/util/List");
    if (!lCls) { env->DeleteLocalRef(eCls); env->DeleteLocalRef(entityList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
    jmethodID lSize = env->GetMethodID(lCls, "size", "()I");
    jmethodID lGet = env->GetMethodID(lCls, "get", "(I)Ljava/lang/Object;");
    if (!lSize || !lGet) { env->DeleteLocalRef(lCls); env->DeleteLocalRef(eCls); env->DeleteLocalRef(entityList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
    jint eSize = env->CallIntMethod(entityList, lSize);

    jmethodID eidMid = env->GetMethodID(eCls, "q", "()I");
    if (!eidMid) { env->ExceptionClear(); eidMid = env->GetMethodID(eCls, "getEntityId", "()I"); }
    if (!eidMid) { env->DeleteLocalRef(lCls); env->DeleteLocalRef(eCls); env->DeleteLocalRef(entityList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(world); env->DeleteLocalRef(player); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
    jint localId = env->CallIntMethod(player, eidMid);

    float fov = ((NumberSetting*)FindSetting("FOV"))->GetValue();
    float aimDist = ((NumberSetting*)FindSetting("Distance"))->GetValue();
    float smooth = ((NumberSetting*)FindSetting("Smooth"))->GetValue();
    float rYaw = ((NumberSetting*)FindSetting("RandomYaw"))->GetValue();
    float rPitch = ((NumberSetting*)FindSetting("RandomPitch"))->GetValue();

    // Target finding
    jobject bestTarget = nullptr;
    float bestDist = FLT_MAX;
    float bestHealth = FLT_MAX;
    float bestAngleDiff = FLT_MAX;

    for (jint i = 0; i < eSize; i++)
    {
        jobject ent = env->CallObjectMethod(entityList, lGet, i);
        if (!ent) continue;
        jint id = env->CallIntMethod(ent, eidMid);
        if (id == localId) { env->DeleteLocalRef(ent); continue; }
        if (IsEntityDead(env, ent)) { env->DeleteLocalRef(ent); continue; }

        float eX = GetDouble(env, ent, "r", "posX");
        float eY = GetDouble(env, ent, "s", "posY");
        float eZ = GetDouble(env, ent, "t", "posZ");
        float eH = GetFloat(env, ent, "o", "height");
        (void)eH;

        float health = GetHealth(env, ent);
        float dx = eX - pX, dy = (eY + 1.62f) - (pY + eyeHeight), dz = eZ - pZ;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist > aimDist) { env->DeleteLocalRef(ent); continue; }

        // Calculate angle difference for FOV check
        float targetYaw = -(float)(atan2(dx, dz) * 180.0f / 3.141592653589793f);
        float yawDiff = WrapAngleTo180(targetYaw - pYaw);
        if (fabs(yawDiff) > fov) { env->DeleteLocalRef(ent); continue; }

        // Priority: closest to crosshair
        float angleDiff = fabs(yawDiff);
        if (angleDiff < bestAngleDiff)
        {
            if (bestTarget) env->DeleteLocalRef(bestTarget);
            bestTarget = ent;
            bestDist = dist;
            bestHealth = health;
            bestAngleDiff = angleDiff;
        }
        else
        {
            env->DeleteLocalRef(ent);
        }
    }

    env->DeleteLocalRef(eCls);
    env->DeleteLocalRef(lCls);
    env->DeleteLocalRef(entityList);
    env->DeleteLocalRef(wCls);
    env->DeleteLocalRef(world);

    if (!bestTarget)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mcCls);
        env->DeleteLocalRef(mc);
        return;
    }

    // Calculate aim angles
    float tX = GetDouble(env, bestTarget, "r", "posX");
    float tY = GetDouble(env, bestTarget, "s", "posY");
    float tZ = GetDouble(env, bestTarget, "t", "posZ");
    float tH = GetFloat(env, bestTarget, "o", "height");

    float targetHeight = tH - 0.1f;
    Vector3 headPos = { pX, pY + eyeHeight, pZ };
    Vector3 targetHeadPos = { tX, tY + targetHeight, tZ };
    Vector3 targetFootPos = { tX, tY, tZ };

    // Calculate angles
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

    // Random offset
    float yawOffset = ((float)rand() / RAND_MAX) * 2.0f * rYaw - rYaw;
    float pitchOffset = ((float)rand() / RAND_MAX) * 2.0f * rPitch - rPitch;

    // Choose targets based on pitch
    float targetYaw, targetPitch;
    if (pPitch > footPitch || pPitch < headPitch)
    {
        float diffFoot = fabs(pPitch - footPitch);
        float diffHead = fabs(pPitch - headPitch);
        if (diffFoot > diffHead)
        {
            targetYaw = headYaw;
            targetPitch = headPitch;
        }
        else
        {
            targetYaw = footYaw;
            targetPitch = footPitch;
        }
    }
    else
    {
        targetYaw = headYaw;
        targetPitch = pPitch;
    }

    targetYaw += yawOffset;
    targetPitch += pitchOffset;

    // Apply smoothing
    float yawDiff = WrapAngleTo180(targetYaw - pYaw);
    float pitchDiff = targetPitch - pPitch;

    pYaw += yawDiff / smooth;
    pPitch += pitchDiff / smooth;

    // Clamp pitch
    if (pPitch > 90.0f) pPitch = 90.0f;
    if (pPitch < -90.0f) pPitch = -90.0f;

    SetFloat(env, player, "y", "rotationYaw", pYaw);
    SetFloat(env, player, "z", "rotationPitch", pPitch);

    env->DeleteLocalRef(bestTarget);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mcCls);
    env->DeleteLocalRef(mc);
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
    float radView = 70.0f * 3.141592653589793f / 180.0f; // default MC FOV
    float radius = tanf(radFov / 2.0f) / tanf(radView / 2.0f) * screen.x / 1.7325f;

    ImGui::GetForegroundDrawList()->AddCircle(
        ImVec2(screen.x * 0.5f, screen.y * 0.5f), radius,
        fovCol, (int)(radius / 3.0f), 1.0f);
}

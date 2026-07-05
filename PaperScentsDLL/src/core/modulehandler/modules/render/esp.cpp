#include "esp.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <imgui.h>
#include <cmath>
#include <cfloat>
#include <vector>
#include <string>
#include <cstring>
#include <unordered_map>

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
    jmethodID get = env->GetStaticMethodID(cls, "A", "()Leave;");
    if (!get) { env->ExceptionClear(); get = env->GetStaticMethodID(cls, "getMinecraft", "()Lnet/minecraft/client/Minecraft;"); }
    if (!get) { env->DeleteLocalRef(cls); return nullptr; }
    env->ExceptionClear();
    jobject mc = env->CallStaticObjectMethod(cls, get);
    env->DeleteLocalRef(cls);
    return mc;
}

static double GetFieldDouble(JNIEnv* env, jobject obj, const char* obf, const char* deobf)
{
    jclass cls = env->GetObjectClass(obj);
    if (!cls) return 0.0;
    jfieldID fid = env->GetFieldID(cls, obf, "D");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(cls, deobf, "D"); }
    double val = 0.0;
    if (fid) val = env->GetDoubleField(obj, fid);
    env->DeleteLocalRef(cls);
    return val;
}

static float GetFieldFloat(JNIEnv* env, jobject obj, const char* obf, const char* deobf)
{
    jclass cls = env->GetObjectClass(obj);
    if (!cls) return 0.0f;
    jfieldID fid = env->GetFieldID(cls, obf, "F");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(cls, deobf, "F"); }
    float val = 0.0f;
    if (fid) val = env->GetFloatField(obj, fid);
    env->DeleteLocalRef(cls);
    return val;
}

static std::string GetEntityName(JNIEnv* env, jobject entity)
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

static bool IsEntityDead(JNIEnv* env, jobject entity)
{
    jclass cls = env->GetObjectClass(entity);
    if (!cls) return true;
    jfieldID fid = env->GetFieldID(cls, "J", "Z");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(cls, "isDead", "Z"); }
    bool dead = false;
    if (fid) dead = env->GetBooleanField(entity, fid) == JNI_TRUE;
    env->DeleteLocalRef(cls);
    return dead;
}

static float GetEntityHealth(JNIEnv* env, jobject entity)
{
    jclass cls = Core::GetInstance().GetJava()->FindClass("pr", "net/minecraft/entity/EntityLivingBase");
    if (!cls) { env->ExceptionClear(); cls = env->FindClass("net/minecraft/entity/EntityLivingBase"); }
    if (!cls) return 20.0f;
    jmethodID mid = env->GetMethodID(cls, "aM", "()F");
    if (!mid) { env->ExceptionClear(); mid = env->GetMethodID(cls, "getHealth", "()F"); }
    env->ExceptionClear();
    if (!mid) { env->DeleteLocalRef(cls); return 20.0f; }
    float health = env->CallFloatMethod(entity, mid);
    env->DeleteLocalRef(cls);
    return health;
}

static float GetEntityMaxHealth(JNIEnv* env, jobject entity)
{
    jclass cls = Core::GetInstance().GetJava()->FindClass("pr", "net/minecraft/entity/EntityLivingBase");
    if (!cls) { env->ExceptionClear(); cls = env->FindClass("net/minecraft/entity/EntityLivingBase"); }
    if (!cls) return 20.0f;
    jmethodID mid = env->GetMethodID(cls, "aA", "()F");
    if (!mid) { env->ExceptionClear(); mid = env->GetMethodID(cls, "getMaxHealth", "()F"); }
    env->ExceptionClear();
    if (!mid) { env->DeleteLocalRef(cls); return 20.0f; }
    float mh = env->CallFloatMethod(entity, mid);
    env->DeleteLocalRef(cls);
    return mh;
}

// Simple math types for WorldToScreen
struct Vector4 { float x, y, z, w; };

// Simple 4x4 matrix * 4-component vector for WorldToScreen
struct Matrix4
{
    float m[16];
    
    static Matrix4 FromFloatBuffer(JNIEnv* env, jobject fb)
    {
        Matrix4 mat = {};
        jclass fbCls = env->FindClass("java/nio/FloatBuffer");
        jmethodID get = env->GetMethodID(fbCls, "get", "(I)F");
        if (get)
        {
            for (int i = 0; i < 16; i++)
                mat.m[i] = env->CallFloatMethod(fb, get, i);
        }
        env->DeleteLocalRef(fbCls);
        return mat;
    }
};

static Vector4 MatMul(const Matrix4& mat, const Vector4& v)
{
    Vector4 r;
    r.x = mat.m[0] * v.x + mat.m[4] * v.y + mat.m[8] * v.z + mat.m[12] * v.w;
    r.y = mat.m[1] * v.x + mat.m[5] * v.y + mat.m[9] * v.z + mat.m[13] * v.w;
    r.z = mat.m[2] * v.x + mat.m[6] * v.y + mat.m[10] * v.z + mat.m[14] * v.w;
    r.w = mat.m[3] * v.x + mat.m[7] * v.y + mat.m[11] * v.z + mat.m[15] * v.w;
    return r;
}

static bool WorldToScreenJni(double x, double y, double z, float& sx, float& sy)
{
    JNIEnv* env = GetEnv();
    if (!env) return false;

    // Get ActiveRenderInfo matrices
    jclass ariCls = Core::GetInstance().GetJava()->FindClass("avq", "net/minecraft/client/renderer/ActiveRenderInfo");
    if (!ariCls) { env->ExceptionClear(); ariCls = env->FindClass("net/minecraft/client/renderer/ActiveRenderInfo"); }
    if (!ariCls) return false;

    jfieldID projFid = env->GetStaticFieldID(ariCls, "h", "Ljava/nio/FloatBuffer;");
    if (!projFid) { env->ExceptionClear(); projFid = env->GetStaticFieldID(ariCls, "PROJECTION", "Ljava/nio/FloatBuffer;"); }
    jfieldID mvFid = env->GetStaticFieldID(ariCls, "g", "Ljava/nio/FloatBuffer;");
    if (!mvFid) { env->ExceptionClear(); mvFid = env->GetStaticFieldID(ariCls, "MODELVIEW", "Ljava/nio/FloatBuffer;"); }
    env->ExceptionClear();

    if (!projFid || !mvFid) { env->DeleteLocalRef(ariCls); return false; }

    jobject projFB = env->GetStaticObjectField(ariCls, projFid);
    jobject mvFB = env->GetStaticObjectField(ariCls, mvFid);
    env->DeleteLocalRef(ariCls);

    if (!projFB || !mvFB) { if (projFB) env->DeleteLocalRef(projFB); if (mvFB) env->DeleteLocalRef(mvFB); return false; }

    Matrix4 projection = Matrix4::FromFloatBuffer(env, projFB);
    Matrix4 modelview = Matrix4::FromFloatBuffer(env, mvFB);
    env->DeleteLocalRef(projFB);
    env->DeleteLocalRef(mvFB);

    // Transform point
    Vector4 pos = { (float)x, (float)y, (float)z, 1.0f };
    Vector4 mvPos = MatMul(modelview, pos);
    Vector4 clipPos = MatMul(projection, mvPos);

    if (clipPos.w == 0.0f) return false;

    // Perspective division
    float ndx = clipPos.x / clipPos.w;
    float ndy = clipPos.y / clipPos.w;
    float ndz = clipPos.z / clipPos.w;

    if (ndz > 1.0f || ndz < -1.0f) return false;

    // Map to screen coordinates
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    sx = (ndx + 1.0f) * 0.5f * screen.x;
    sy = (1.0f - ndy) * 0.5f * screen.y;

    return true;
}

static bool IsFriend(const std::string& name)
{
    (void)name;
    return false;
}

ESPModule::ESPModule()
    : ModuleBase("ESP", "See entities through walls", Category::Render)
{
    AddSetting<BooleanSetting>("Enabled", true);
    AddSetting<NumberSetting>("FadeDistance", 3.0f, 0.0f, 10.0f, 0.1f);
    AddSetting<BooleanSetting>("HealthBar", true);
    AddSetting<BooleanSetting>("ShowFriends", true);
    AddSetting<BooleanSetting>("FilledBox", true);
    AddSetting<ColorSetting>("FilledColor", ImColor(0, 0, 0, 40));
    AddSetting<BooleanSetting>("Outline", true);
    AddSetting<ColorSetting>("OutlineColor", ImColor(0, 0, 0, 64));
    AddSetting<NumberSetting>("OutlineThickness", 1.5f, 0.5f, 5.0f, 0.1f);
}

void ESPModule::OnEnable() { Logger::Log("ESP enabled"); }
void ESPModule::OnDisable() { Logger::Log("ESP disabled"); }

void ESPModule::OnUpdate()
{
    JNIEnv* env = GetEnv();
    if (!env) return;

    jobject mc = GetMc();
    if (!mc) return;

    jclass mcCls = env->GetObjectClass(mc);
    if (!mcCls) { env->DeleteLocalRef(mc); return; }

    jfieldID wfid = env->GetFieldID(mcCls, "f", "Ladm;");
    if (!wfid) { env->ExceptionClear(); wfid = env->GetFieldID(mcCls, "theWorld", "Lnet/minecraft/world/World;"); }
    jobject world = wfid ? env->GetObjectField(mc, wfid) : nullptr;
    if (!world) { env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    jfieldID pfid = env->GetFieldID(mcCls, "u", "Lbew;");
    if (!pfid) { env->ExceptionClear(); pfid = env->GetFieldID(mcCls, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;"); }
    jobject player = pfid ? env->GetObjectField(mc, pfid) : nullptr;
    if (!player) { env->DeleteLocalRef(world); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    double pX = GetFieldDouble(env, player, "r", "posX");
    double pY = GetFieldDouble(env, player, "s", "posY");
    double pZ = GetFieldDouble(env, player, "t", "posZ");

    jclass wCls = env->GetObjectClass(world);
    jfieldID eflid = env->GetFieldID(wCls, "e", "Ljava/util/List;");
    if (!eflid) { env->ExceptionClear(); eflid = env->GetFieldID(wCls, "playerEntities", "Ljava/util/List;"); }
    jobject entityList = eflid ? env->GetObjectField(world, eflid) : nullptr;
    if (!entityList) { env->DeleteLocalRef(wCls); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    // Get local player entity ID
    jclass eCls = Core::GetInstance().GetJava()->FindClass("pk", "net/minecraft/entity/Entity");
    if (!eCls) { env->ExceptionClear(); eCls = env->FindClass("net/minecraft/entity/Entity"); }
    if (!eCls) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
    jmethodID eidMid = env->GetMethodID(eCls, "q", "()I");
    if (!eidMid) { env->ExceptionClear(); eidMid = env->GetMethodID(eCls, "getEntityId", "()I"); }
    if (!eidMid) { env->DeleteLocalRef(eCls); env->DeleteLocalRef(entityList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
    jint localId = env->CallIntMethod(player, eidMid);

    jclass lCls = env->FindClass("java/util/List");
    if (!lCls) { env->DeleteLocalRef(eCls); env->DeleteLocalRef(entityList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
    jmethodID lSize = env->GetMethodID(lCls, "size", "()I");
    jmethodID lGet = env->GetMethodID(lCls, "get", "(I)Ljava/lang/Object;");
    if (!lSize || !lGet) { env->DeleteLocalRef(lCls); env->DeleteLocalRef(eCls); env->DeleteLocalRef(entityList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
    jint eSize = env->CallIntMethod(entityList, lSize);

    float fadeDist = ((NumberSetting*)FindSetting("FadeDistance"))->GetValue();
    float outThick = ((NumberSetting*)FindSetting("OutlineThickness"))->GetValue();
    ImColor fillCol = ((ColorSetting*)FindSetting("FilledColor"))->GetValue();
    ImColor outCol = ((ColorSetting*)FindSetting("OutlineColor"))->GetValue();
    (void)outThick;

    std::vector<ESPData> newData;

    for (jint i = 0; i < eSize; i++)
    {
        jobject ent = env->CallObjectMethod(entityList, lGet, i);
        if (!ent) continue;

        jint id = env->CallIntMethod(ent, eidMid);
        if (id == localId) { env->DeleteLocalRef(ent); continue; }

        if (IsEntityDead(env, ent)) { env->DeleteLocalRef(ent); continue; }

        std::string name = GetEntityName(env, ent);
        if (name.empty()) { env->DeleteLocalRef(ent); continue; }

        double eX = GetFieldDouble(env, ent, "r", "posX");
        double eY = GetFieldDouble(env, ent, "s", "posY");
        double eZ = GetFieldDouble(env, ent, "t", "posZ");

        float height = GetFieldFloat(env, ent, "o", "height");
        float width = GetFieldFloat(env, ent, "n", "width");

        float health = GetEntityHealth(env, ent);
        float maxHealth = GetEntityMaxHealth(env, ent);

        float dist = (float)std::sqrt((eX - pX) * (eX - pX) + (eY - pY) * (eY - pY) + (eZ - pZ) * (eZ - pZ));
        if (dist > 300.0f) { env->DeleteLocalRef(ent); continue; }

        float opacityFade = 1.0f;
        if ((dist - 1.0f) <= fadeDist && fadeDist > 0.0f)
            opacityFade = (dist - 1.0f) / fadeDist;

        // Project 8 corners of the bounding box
        float hw = width * 0.5f;
        float hh = height;

        double corners[8][3] = {
            {eX - hw, eY,      eZ - hw},
            {eX + hw, eY,      eZ - hw},
            {eX + hw, eY + hh, eZ - hw},
            {eX - hw, eY + hh, eZ - hw},
            {eX - hw, eY,      eZ + hw},
            {eX + hw, eY,      eZ + hw},
            {eX + hw, eY + hh, eZ + hw},
            {eX - hw, eY + hh, eZ + hw},
        };

        float left = FLT_MAX, top_ = FLT_MAX, right = FLT_MIN, bottom = FLT_MIN;
        for (int c = 0; c < 8; c++)
        {
            float sx, sy;
            if (WorldToScreenJni(corners[c][0], corners[c][1], corners[c][2], sx, sy))
            {
                left = fmin(left, sx);
                top_ = fmin(top_, sy);
                right = fmax(right, sx);
                bottom = fmax(bottom, sy);
            }
        }

        if (left >= right || top_ >= bottom) { env->DeleteLocalRef(ent); continue; }

        newData.push_back({ left, top_, right, bottom, name, dist, health, maxHealth, opacityFade, false });
        env->DeleteLocalRef(ent);
    }

    env->DeleteLocalRef(eCls);
    env->DeleteLocalRef(lCls);
    env->DeleteLocalRef(entityList);
    env->DeleteLocalRef(wCls);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(mcCls);
    env->DeleteLocalRef(mc);

    m_RenderData = newData;
}

void ESPModule::OnRender()
{
    if (!IsEnabled() || m_RenderData.empty()) return;

    bool filled = ((BooleanSetting*)FindSetting("FilledBox"))->GetValue();
    bool outline = ((BooleanSetting*)FindSetting("Outline"))->GetValue();
    bool healthBar = ((BooleanSetting*)FindSetting("HealthBar"))->GetValue();
    float outThick = ((NumberSetting*)FindSetting("OutlineThickness"))->GetValue();
    ImColor fillCol = ((ColorSetting*)FindSetting("FilledColor"))->GetValue();
    ImColor outCol = ((ColorSetting*)FindSetting("OutlineColor"))->GetValue();

    ImDrawList* draw = ImGui::GetForegroundDrawList();

    for (auto& d : m_RenderData)
    {
        ImVec2 tl(d.boxLeft, d.boxTop);
        ImVec2 br(d.boxRight, d.boxBottom);
        float boxW = br.x - tl.x, boxH = br.y - tl.y;

        ImColor fCol = ImColor(fillCol.Value.x, fillCol.Value.y, fillCol.Value.z, fillCol.Value.w * d.opacityFade);
        ImColor oCol = ImColor(outCol.Value.x, outCol.Value.y, outCol.Value.z, outCol.Value.w * d.opacityFade);

        if (filled)
            draw->AddRectFilled(tl, br, fCol);

        if (outline)
        {
            draw->AddRect(tl, br, oCol, 0.0f, 0, outThick);
            draw->AddRect(ImVec2(tl.x - 1, tl.y - 1), ImVec2(br.x + 1, br.y + 1), oCol, 0.0f, 0, 0.5f);
            draw->AddRect(ImVec2(tl.x + 1, tl.y + 1), ImVec2(br.x - 1, br.y - 1), oCol, 0.0f, 0, 0.5f);
        }

        if (healthBar && d.maxHealth > 0.0f)
        {
            float hFrac = fmax(0.0f, fmin(1.0f, d.health / d.maxHealth));
            float barW = 2.5f;
            ImVec2 barTL(tl.x - barW - 1.5f, tl.y);
            ImVec2 barBR(tl.x - 1.0f, br.y);
            draw->AddRectFilled(barTL, barBR, ImColor(0, 0, 0, (int)(180 * d.opacityFade)));
            float barH = boxH * hFrac;
            ImVec2 healthTL(barTL.x, barBR.y - barH);
            int r = (int)(255 * (1.0f - hFrac));
            int g = (int)(255 * hFrac);
            draw->AddRectFilled(healthTL, barBR, ImColor(r, g, 0, (int)(255 * d.opacityFade)));
        }
    }
}

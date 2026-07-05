#include "esp.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include <imgui.h>
#include <cmath>
#include <cfloat>
#include <vector>
#include <string>

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

struct Vec4 { float x, y, z, w; };
struct Mat4 { float m[16]; };

static Mat4 ReadFloatBuffer(JNIEnv* env, jobject fb)
{
    Mat4 mat = {};
    if (!fb) return mat;
    jclass fbCls = env->FindClass("java/nio/FloatBuffer");
    if (!fbCls) return mat;
    jmethodID get = env->GetMethodID(fbCls, "get", "(I)F");
    if (get) for (int i = 0; i < 16; i++) mat.m[i] = env->CallFloatMethod(fb, get, i);
    env->DeleteLocalRef(fbCls);
    return mat;
}

static Vec4 Mul(const Mat4& m, const Vec4& v)
{
    return {
        m.m[0]*v.x + m.m[4]*v.y + m.m[8]*v.z + m.m[12]*v.w,
        m.m[1]*v.x + m.m[5]*v.y + m.m[9]*v.z + m.m[13]*v.w,
        m.m[2]*v.x + m.m[6]*v.y + m.m[10]*v.z + m.m[14]*v.w,
        m.m[3]*v.x + m.m[7]*v.y + m.m[11]*v.z + m.m[15]*v.w
    };
}

static bool WorldToScreen(JNIEnv* env, double x, double y, double z, float& sx, float& sy)
{
    jclass ari = env->FindClass("avq");
    if (!ari) { env->ExceptionClear(); ari = env->FindClass("net/minecraft/client/renderer/ActiveRenderInfo"); }
    if (!ari) return false;

    jfieldID pf = env->GetStaticFieldID(ari, "h", "Ljava/nio/FloatBuffer;");
    if (!pf) { env->ExceptionClear(); pf = env->GetStaticFieldID(ari, "PROJECTION", "Ljava/nio/FloatBuffer;"); }
    jfieldID mf = env->GetStaticFieldID(ari, "g", "Ljava/nio/FloatBuffer;");
    if (!mf) { env->ExceptionClear(); mf = env->GetStaticFieldID(ari, "MODELVIEW", "Ljava/nio/FloatBuffer;"); }
    env->ExceptionClear();

    if (!pf || !mf) { env->DeleteLocalRef(ari); return false; }

    jobject pfb = env->GetStaticObjectField(ari, pf);
    jobject mfb = env->GetStaticObjectField(ari, mf);
    env->DeleteLocalRef(ari);
    if (!pfb || !mfb) { if (pfb) env->DeleteLocalRef(pfb); if (mfb) env->DeleteLocalRef(mfb); return false; }

    Mat4 proj = ReadFloatBuffer(env, pfb);
    Mat4 mv = ReadFloatBuffer(env, mfb);
    env->DeleteLocalRef(pfb); env->DeleteLocalRef(mfb);

    Vec4 pos = { (float)x, (float)y, (float)z, 1.0f };
    Vec4 mvPos = Mul(mv, pos);
    Vec4 clip = Mul(proj, mvPos);
    if (clip.w == 0.0f) return false;

    float ndx = clip.x / clip.w, ndy = clip.y / clip.w, ndz = clip.z / clip.w;
    if (ndz > 1.0f || ndz < -1.0f) return false;

    ImVec2 screen = ImGui::GetIO().DisplaySize;
    sx = (ndx + 1.0f) * 0.5f * screen.x;
    sy = (1.0f - ndy) * 0.5f * screen.y;
    return true;
}

static double GetPos(JNIEnv* env, jobject obj, const char* obf, const char* deobf)
{
    jclass c = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(c, obf, "D");
    if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(c, deobf, "D"); }
    double v = 0.0;
    if (fid) v = env->GetDoubleField(obj, fid);
    env->DeleteLocalRef(c);
    return v;
}

static std::string GetEntName(JNIEnv* env, jobject entity)
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

static bool IsDead(JNIEnv* env, jobject entity)
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

static int GetEntityId(JNIEnv* env, jobject entity)
{
    jclass cls = env->GetObjectClass(entity);
    jmethodID mid = env->GetMethodID(cls, "q", "()I");
    if (!mid) { env->ExceptionClear(); mid = env->GetMethodID(cls, "getEntityId", "()I"); }
    int id = -1;
    if (mid) { env->ExceptionClear(); id = env->CallIntMethod(entity, mid); }
    env->DeleteLocalRef(cls);
    return id;
}

static void GetHealth(JNIEnv* env, jobject entity, float& health, float& maxHealth)
{
    jclass cls = env->GetObjectClass(entity);
    // Check if entity is EntityLivingBase
    if (!cls) return;
    jmethodID hMid = env->GetMethodID(cls, "aM", "()F");
    if (!hMid) { env->ExceptionClear(); hMid = env->GetMethodID(cls, "getHealth", "()F"); }
    if (hMid) { env->ExceptionClear(); health = env->CallFloatMethod(entity, hMid); }
    jmethodID mhMid = env->GetMethodID(cls, "aA", "()F");
    if (!mhMid) { env->ExceptionClear(); mhMid = env->GetMethodID(cls, "getMaxHealth", "()F"); }
    if (mhMid) { env->ExceptionClear(); maxHealth = env->CallFloatMethod(entity, mhMid); }
    env->DeleteLocalRef(cls);
}

ESPModule::ESPModule()
    : ModuleBase("ESP", "See entities through walls", Category::Render)
{
    AddSetting<NumberSetting>("FadeDistance", 3.0f, 0.0f, 10.0f, 0.1f);
    AddSetting<BooleanSetting>("HealthBar", true);
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
    if (++m_FrameCounter % 3 != 0) return;
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) { Logger::Log("ESP: no env"); return; }

    jobject mc = GetMinecraftObject(env);
    if (!mc) { Logger::Log("ESP: GetMinecraftObject failed"); return; }

    jobject world = GetWorldObject(env, mc);
    if (!world) { env->DeleteLocalRef(mc); Logger::Log("ESP: GetWorldObject failed"); return; }

    jclass wCls = env->GetObjectClass(world);
    if (!wCls) { env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    // Get player list
    jobject entityList = nullptr;
    jmethodID getPlayers = env->GetMethodID(wCls, "j", "()Ljava/util/List;");
    if (!getPlayers) { env->ExceptionClear(); getPlayers = env->GetMethodID(wCls, "getPlayerEntities", "()Ljava/util/List;"); }
    if (getPlayers) { env->ExceptionClear(); entityList = env->CallObjectMethod(world, getPlayers); }
    if (!entityList)
    {
        const char* eFields[] = { "e", "playerEntities", "field_73010_i" };
        jfieldID eflid = nullptr;
        for (auto f : eFields)
        {
            eflid = env->GetFieldID(wCls, f, "Ljava/util/List;"); if (eflid) break;
            env->ExceptionClear();
        }
        if (eflid) entityList = env->GetObjectField(world, eflid);
    }
    if (!entityList)
    {
        Logger::Log("ESP: failed to get player list");
        env->DeleteLocalRef(wCls); env->DeleteLocalRef(world); env->DeleteLocalRef(mc);
        return;
    }

    jobject player = GetPlayerObject(env, mc);
    if (!player) { Logger::Log("ESP: GetPlayerObject failed"); env->DeleteLocalRef(entityList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }

    double pX = GetPos(env, player, "r", "posX");
    double pY = GetPos(env, player, "s", "posY");
    double pZ = GetPos(env, player, "t", "posZ");

    // List helpers
    jclass lCls = env->FindClass("java/util/List");
    jmethodID lSize = env->GetMethodID(lCls, "size", "()I");
    jmethodID lGet = env->GetMethodID(lCls, "get", "(I)Ljava/lang/Object;");
    jint eSize = env->CallIntMethod(entityList, lSize);

    float fadeDist = ((NumberSetting*)FindSetting("FadeDistance"))->GetValue();
    int localId = GetEntityId(env, player);
    std::vector<ESPData> newData;

    for (jint i = 0; i < eSize; i++)
    {
        jobject ent = env->CallObjectMethod(entityList, lGet, i);
        if (!ent) continue;

        int entId = GetEntityId(env, ent);
        if (entId == localId || entId < 0) { env->DeleteLocalRef(ent); continue; }
        if (IsDead(env, ent)) { env->DeleteLocalRef(ent); continue; }

        std::string name = GetEntName(env, ent);
        if (name.empty()) { env->DeleteLocalRef(ent); continue; }

        double eX = GetPos(env, ent, "r", "posX");
        double eY = GetPos(env, ent, "s", "posY");
        double eZ = GetPos(env, ent, "t", "posZ");
        float height = GetFieldFloat(env, ent, "o", "height");
        float width = GetFieldFloat(env, ent, "n", "width");
        float health = 20.0f, maxHealth = 20.0f;
        GetHealth(env, ent, health, maxHealth);

        float dist = (float)std::sqrt((eX-pX)*(eX-pX) + (eY-pY)*(eY-pY) + (eZ-pZ)*(eZ-pZ));
        if (dist > 300.0f) { env->DeleteLocalRef(ent); continue; }

        float opacityFade = 1.0f;
        if ((dist - 1.0f) <= fadeDist && fadeDist > 0.0f) opacityFade = (dist - 1.0f) / fadeDist;

        float hw = width * 0.5f, hh = height;
        double corners[8][3] = {
            {eX - hw, eY,      eZ - hw}, {eX + hw, eY,      eZ - hw},
            {eX + hw, eY + hh, eZ - hw}, {eX - hw, eY + hh, eZ - hw},
            {eX - hw, eY,      eZ + hw}, {eX + hw, eY,      eZ + hw},
            {eX + hw, eY + hh, eZ + hw}, {eX - hw, eY + hh, eZ + hw},
        };

        float left = FLT_MAX, top_ = FLT_MAX, right = FLT_MIN, bottom = FLT_MIN;
        for (int c = 0; c < 8; c++)
        {
            float sx, sy;
            if (WorldToScreen(env, corners[c][0], corners[c][1], corners[c][2], sx, sy))
            { left = fmin(left, sx); top_ = fmin(top_, sy); right = fmax(right, sx); bottom = fmax(bottom, sy); }
        }

        if (left >= right || top_ >= bottom) { env->DeleteLocalRef(ent); continue; }

        newData.push_back({ left, top_, right, bottom, name, dist, health, maxHealth, opacityFade });
        env->DeleteLocalRef(ent);
    }

    env->DeleteLocalRef(lCls);
    env->DeleteLocalRef(entityList);
    env->DeleteLocalRef(wCls);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(world);
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

        ImColor fCol(fillCol.Value.x, fillCol.Value.y, fillCol.Value.z, fillCol.Value.w * d.opacityFade);
        ImColor oCol(outCol.Value.x, outCol.Value.y, outCol.Value.z, outCol.Value.w * d.opacityFade);

        if (filled) draw->AddRectFilled(tl, br, fCol);
        if (outline)
        {
            draw->AddRect(tl, br, oCol, 0.0f, 0, outThick);
            draw->AddRect(ImVec2(tl.x-1, tl.y-1), ImVec2(br.x+1, br.y+1), oCol, 0.0f, 0, 0.5f);
            draw->AddRect(ImVec2(tl.x+1, tl.y+1), ImVec2(br.x-1, br.y-1), oCol, 0.0f, 0, 0.5f);
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
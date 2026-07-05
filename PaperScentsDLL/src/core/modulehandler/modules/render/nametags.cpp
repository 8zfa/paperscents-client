#include "nametags.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <imgui.h>
#include <cmath>
#include <string>
#include <vector>
#include <cfloat>

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

struct Vec4 { float x, y, z, w; };
struct Mat4 { float m[16]; };

static Mat4 ReadFB(JNIEnv* env, jobject fb)
{
    Mat4 mat = {};
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

static bool WorldToScreen(double x, double y, double z, float& sx, float& sy)
{
    JNIEnv* env = GetEnv();
    if (!env) return false;
    jclass ari = Core::GetInstance().GetJava()->FindClass("avq", "net/minecraft/client/renderer/ActiveRenderInfo");
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
    Mat4 proj = ReadFB(env, pfb);
    Mat4 mv = ReadFB(env, mfb);
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
}

void NametagsModule::OnEnable() { Logger::Log("Nametags enabled"); }
void NametagsModule::OnDisable() { Logger::Log("Nametags disabled"); }

void NametagsModule::OnUpdate()
{
    JNIEnv* env = GetEnv();
    if (!env) return;
    env->ExceptionClear();

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

    double pX = GetDouble(env, player, "r", "posX");
    double pY = GetDouble(env, player, "s", "posY");
    double pZ = GetDouble(env, player, "t", "posZ");

    jclass wCls = env->GetObjectClass(world);
    jfieldID eflid = env->GetFieldID(wCls, "e", "Ljava/util/List;");
    if (!eflid) { env->ExceptionClear(); eflid = env->GetFieldID(wCls, "playerEntities", "Ljava/util/List;"); }
    jobject entityList = eflid ? env->GetObjectField(world, eflid) : nullptr;
    if (!entityList) { env->DeleteLocalRef(wCls); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }

    jclass lCls = env->FindClass("java/util/List");
    if (!lCls) { env->DeleteLocalRef(entityList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
    jmethodID lSize = env->GetMethodID(lCls, "size", "()I");
    jmethodID lGet = env->GetMethodID(lCls, "get", "(I)Ljava/lang/Object;");
    if (!lSize || !lGet) { env->DeleteLocalRef(lCls); env->DeleteLocalRef(entityList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
    jint eSize = env->CallIntMethod(entityList, lSize);

    jclass eCls = Core::GetInstance().GetJava()->FindClass("pk", "net/minecraft/entity/Entity");
    if (!eCls) { env->ExceptionClear(); eCls = env->FindClass("net/minecraft/entity/Entity"); }
    if (!eCls) { env->DeleteLocalRef(lCls); env->DeleteLocalRef(entityList); env->DeleteLocalRef(wCls); env->DeleteLocalRef(player); env->DeleteLocalRef(world); env->DeleteLocalRef(mcCls); env->DeleteLocalRef(mc); return; }
    jmethodID eidMid = env->GetMethodID(eCls, "q", "()I");
    if (!eidMid) { env->ExceptionClear(); eidMid = env->GetMethodID(eCls, "getEntityId", "()I"); }
    jint localId = 0;
    if (eidMid) localId = env->CallIntMethod(player, eidMid);

    float scale = ((NumberSetting*)FindSetting("Scale"))->GetValue();
    float offset = ((NumberSetting*)FindSetting("Offset"))->GetValue();

    std::vector<NameTagData> newTags;

    for (jint i = 0; i < eSize; i++)
    {
        jobject ent = env->CallObjectMethod(entityList, lGet, i);
        if (!ent) continue;

        jint id = 0;
        if (eidMid) id = env->CallIntMethod(ent, eidMid);
        if (id == localId) { env->DeleteLocalRef(ent); continue; }
        if (IsDead(env, ent)) { env->DeleteLocalRef(ent); continue; }

        std::string name = GetEntName(env, ent);
        if (name.empty()) { env->DeleteLocalRef(ent); continue; }

        double eX = GetDouble(env, ent, "r", "posX");
        double eY = GetDouble(env, ent, "s", "posY");
        double eZ = GetDouble(env, ent, "t", "posZ");
        float height = GetFloat(env, ent, "o", "height");

        double dist = std::sqrt((eX - pX)*(eX-pX) + (eY-pY)*(eY-pY) + (eZ-pZ)*(eZ-pZ));
        if (dist > 100.0) { env->DeleteLocalRef(ent); continue; }

        float sx, sy;
        if (WorldToScreen(eX, eY + height + 0.4f + offset * 0.01f, eZ, sx, sy))
        {
            float opacity = 1.0f;
            if (dist > 50.0f) opacity = 1.0f - (dist - 50.0f) / 50.0f;
            newTags.push_back({ sx, sy, name, opacity });
        }
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

#pragma once
#include <jni.h>
#include "../core.h"
#include "../sdk/strayCache.h"
#include <cmath>

inline jobject GetMinecraftObject(JNIEnv* env)
{
    if (!StrayCache::MinecraftInstance) return nullptr;
    return env->NewLocalRef(StrayCache::MinecraftInstance);
}

inline jobject GetPlayerObject(JNIEnv* env, jobject mc)
{
    if (!env || !mc || !StrayCache::Minecraft_thePlayer) return nullptr;
    jobject player = env->GetObjectField(mc, StrayCache::Minecraft_thePlayer);
    return player;
}

inline jobject GetWorldObject(JNIEnv* env, jobject mc)
{
    if (!env || !mc || !StrayCache::Minecraft_theWorld) return nullptr;
    jobject world = env->GetObjectField(mc, StrayCache::Minecraft_theWorld);
    return world;
}

inline double GetEntityPosX(JNIEnv* env, jobject entity)
{
    if (!StrayCache::Entity_posX) return 0.0;
    return env->GetDoubleField(entity, StrayCache::Entity_posX);
}

inline double GetEntityPosY(JNIEnv* env, jobject entity)
{
    if (!StrayCache::Entity_posY) return 0.0;
    return env->GetDoubleField(entity, StrayCache::Entity_posY);
}

inline double GetEntityPosZ(JNIEnv* env, jobject entity)
{
    if (!StrayCache::Entity_posZ) return 0.0;
    return env->GetDoubleField(entity, StrayCache::Entity_posZ);
}

inline bool IsEntityDead(JNIEnv* env, jobject entity)
{
    if (!StrayCache::Entity_isDead) return true;
    return env->GetBooleanField(entity, StrayCache::Entity_isDead) == JNI_TRUE;
}

inline int GetEntityId(JNIEnv* env, jobject entity)
{
    if (!StrayCache::Entity_getEntityId) return -1;
    return env->CallIntMethod(entity, StrayCache::Entity_getEntityId);
}

inline std::string GetEntityName(JNIEnv* env, jobject entity)
{
    if (!StrayCache::Entity_getName) return "";
    jobject nameObj = env->CallObjectMethod(entity, StrayCache::Entity_getName);
    std::string result;
    if (nameObj)
    {
        const char* str = env->GetStringUTFChars((jstring)nameObj, nullptr);
        if (str) { result = str; env->ReleaseStringUTFChars((jstring)nameObj, str); }
        env->DeleteLocalRef(nameObj);
    }
    return result;
}

inline float GetEntityWidth(JNIEnv* env, jobject entity)
{
    if (!StrayCache::Entity_width) return 0.6f;
    return env->GetFloatField(entity, StrayCache::Entity_width);
}

inline float GetEntityHeight(JNIEnv* env, jobject entity)
{
    if (!StrayCache::Entity_height) return 1.8f;
    return env->GetFloatField(entity, StrayCache::Entity_height);
}

inline bool ReadFloatBufferMat4(JNIEnv* env, jobject fb, float* out)
{
    if (!fb || !StrayCache::FloatBufferClass || !StrayCache::FloatBuffer_get) return false;
    for (int i = 0; i < 16; i++)
        out[i] = env->CallFloatMethod(fb, StrayCache::FloatBuffer_get, i);
    return true;
}

inline bool WorldToScreen(JNIEnv* env, double x, double y, double z, float& sx, float& sy)
{
    if (!StrayCache::ActiveRenderInfoClass || !StrayCache::ActiveRenderInfo_PROJECTION || !StrayCache::ActiveRenderInfo_MODELVIEW)
        return false;

    jobject pfb = env->GetStaticObjectField(StrayCache::ActiveRenderInfoClass, StrayCache::ActiveRenderInfo_PROJECTION);
    jobject mfb = env->GetStaticObjectField(StrayCache::ActiveRenderInfoClass, StrayCache::ActiveRenderInfo_MODELVIEW);
    if (!pfb || !mfb) { if (pfb) env->DeleteLocalRef(pfb); if (mfb) env->DeleteLocalRef(mfb); return false; }

    float proj[16], mv[16];
    bool ok1 = ReadFloatBufferMat4(env, pfb, proj);
    bool ok2 = ReadFloatBufferMat4(env, mfb, mv);
    env->DeleteLocalRef(pfb);
    env->DeleteLocalRef(mfb);
    if (!ok1 || !ok2) return false;

    float vx = mv[0]*x + mv[4]*y + mv[8]*z + mv[12];
    float vy = mv[1]*x + mv[5]*y + mv[9]*z + mv[13];
    float vz = mv[2]*x + mv[6]*y + mv[10]*z + mv[14];
    float vw = mv[3]*x + mv[7]*y + mv[11]*z + mv[15];

    float cx = proj[0]*vx + proj[4]*vy + proj[8]*vz + proj[12]*vw;
    float cy = proj[1]*vx + proj[5]*vy + proj[9]*vz + proj[13]*vw;
    float cz = proj[2]*vx + proj[6]*vy + proj[10]*vz + proj[14]*vw;
    float cw = proj[3]*vx + proj[7]*vy + proj[11]*vz + proj[15]*vw;

    if (cw == 0.0f) return false;
    float ndx = cx / cw;
    float ndy = cy / cw;
    float ndz = cz / cw;
    if (ndz > 1.0f || ndz < -1.0f) return false;

    ImVec2 screen = ImGui::GetIO().DisplaySize;
    sx = (ndx + 1.0f) * 0.5f * screen.x;
    sy = (1.0f - ndy) * 0.5f * screen.y;
    return true;
}

inline float GetEntityRotationYaw(JNIEnv* env, jobject entity)
{
    if (!StrayCache::Entity_rotationYaw) return 0.0f;
    return env->GetFloatField(entity, StrayCache::Entity_rotationYaw);
}

inline float GetEntityRotationPitch(JNIEnv* env, jobject entity)
{
    if (!StrayCache::Entity_rotationPitch) return 0.0f;
    return env->GetFloatField(entity, StrayCache::Entity_rotationPitch);
}

inline bool IsEntitySneaking(JNIEnv* env, jobject entity)
{
    if (!StrayCache::Entity_isSneaking) return false;
    return env->CallBooleanMethod(entity, StrayCache::Entity_isSneaking) == JNI_TRUE;
}

inline float GetEntityLivingBaseHealth(JNIEnv* env, jobject entity)
{
    if (!StrayCache::EntityLivingBase_getHealth) return 0.0f;
    return env->CallFloatMethod(entity, StrayCache::EntityLivingBase_getHealth);
}

inline float GetEntityLivingBaseMaxHealth(JNIEnv* env, jobject entity)
{
    if (!StrayCache::EntityLivingBase_getMaxHealth) return 0.0f;
    return env->CallFloatMethod(entity, StrayCache::EntityLivingBase_getMaxHealth);
}

inline jobject GetEntityLivingBaseHeldItem(JNIEnv* env, jobject entity)
{
    if (!StrayCache::EntityLivingBase_getHeldItem) return nullptr;
    return env->CallObjectMethod(entity, StrayCache::EntityLivingBase_getHeldItem);
}

inline jobject GetItemFromItemStack(JNIEnv* env, jobject itemStack)
{
    if (!StrayCache::ItemStack_getItem) return nullptr;
    return env->CallObjectMethod(itemStack, StrayCache::ItemStack_getItem);
}

inline void SetEntityRotationYaw(JNIEnv* env, jobject entity, float yaw)
{
    if (StrayCache::Entity_setRotationYaw)
        env->CallVoidMethod(entity, StrayCache::Entity_setRotationYaw, yaw);
}

inline void SetEntityRotationPitch(JNIEnv* env, jobject entity, float pitch)
{
    if (StrayCache::Entity_setRotationPitch)
        env->CallVoidMethod(entity, StrayCache::Entity_setRotationPitch, pitch);
}

inline bool IsItemSword(JNIEnv* env, jobject item)
{
    if (!StrayCache::ItemSwordClass) return false;
    return env->IsInstanceOf(item, StrayCache::ItemSwordClass) == JNI_TRUE;
}

#include "nohurtcam.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"

NoHurtCamModule::NoHurtCamModule()
    : ModuleBase("NoHurtCam", "Disables hurt camera effect", Category::Render)
{
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void NoHurtCamModule::OnEnable() { Logger::Log("NoHurtCam enabled"); }
void NoHurtCamModule::OnDisable() { Logger::Log("NoHurtCam disabled"); }

void NoHurtCamModule::OnUpdate()
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

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    if (StrayCache::Entity)
    {
        jfieldID hurtResistantTimeID = env->GetFieldID(StrayCache::Entity, "W", "I");
        if (!hurtResistantTimeID) { env->ExceptionClear(); hurtResistantTimeID = env->GetFieldID(StrayCache::Entity, "hurtResistantTime", "I"); }
        env->ExceptionClear();
        if (hurtResistantTimeID) env->SetIntField(player, hurtResistantTimeID, 0);
    }

    if (StrayCache::EntityLivingBase)
    {
        jfieldID hurtTimeID = env->GetFieldID(StrayCache::EntityLivingBase, "an", "I");
        if (!hurtTimeID) { env->ExceptionClear(); hurtTimeID = env->GetFieldID(StrayCache::EntityLivingBase, "hurtTime", "I"); }
        env->ExceptionClear();
        if (hurtTimeID) env->SetIntField(player, hurtTimeID, 0);

        jfieldID maxHurtTimeID = env->GetFieldID(StrayCache::EntityLivingBase, "ao", "I");
        if (!maxHurtTimeID) { env->ExceptionClear(); maxHurtTimeID = env->GetFieldID(StrayCache::EntityLivingBase, "maxHurtTime", "I"); }
        env->ExceptionClear();
        if (maxHurtTimeID) env->SetIntField(player, maxHurtTimeID, 0);

        jfieldID hurtTicksToFarID = env->GetFieldID(StrayCache::EntityLivingBase, "aV", "I");
        if (!hurtTicksToFarID) { env->ExceptionClear(); hurtTicksToFarID = env->GetFieldID(StrayCache::EntityLivingBase, "hurtTicksToFar", "I"); }
        env->ExceptionClear();
        if (hurtTicksToFarID) env->SetIntField(player, hurtTicksToFarID, 0);
    }

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

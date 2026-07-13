#include "hitbox.h"
#include "../../../java/java.h"
#include "../../../sdk/strayCache.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"

HitBoxModule::HitBoxModule()
    : ModuleBase("HitBox", "Expand entity hitboxes", Category::Render)
{
    AddSetting<NumberSetting>("Size", 0.3f, 0.1f, 1.0f, 0.05f, "Hitbox expansion size");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void HitBoxModule::OnEnable() { Logger::Log("HitBox enabled"); }
void HitBoxModule::OnDisable() { Logger::Log("HitBox disabled"); }

void HitBoxModule::OnUpdate()
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

    jobject localPlayer = GetPlayerObject(env, mc);
    if (!localPlayer) { env->DeleteLocalRef(mc); return; }

    jobject world = GetWorldObject(env, mc);
    if (!world) { env->DeleteLocalRef(localPlayer); env->DeleteLocalRef(mc); return; }

    if (!StrayCache::World || !StrayCache::World_loadedEntityList || !StrayCache::ListClass || !StrayCache::List_size || !StrayCache::List_get)
    { env->DeleteLocalRef(world); env->DeleteLocalRef(localPlayer); env->DeleteLocalRef(mc); return; }

    jobject listObj = env->GetObjectField(world, StrayCache::World_loadedEntityList);
    if (!listObj) { env->DeleteLocalRef(world); env->DeleteLocalRef(localPlayer); env->DeleteLocalRef(mc); return; }

    float size = ((NumberSetting*)FindSetting("Size"))->GetValue();

    if (!StrayCache::Entity || !StrayCache::Entity_width || !StrayCache::Entity_height)
    {
        env->DeleteLocalRef(listObj);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(mc);
        return;
    }

    int entSize = env->CallIntMethod(listObj, StrayCache::List_size);
    for (int i = 0; i < entSize; i++)
    {
        jobject entity = env->CallObjectMethod(listObj, StrayCache::List_get, i);
        if (!entity) continue;

        if (env->IsSameObject(entity, localPlayer))
        {
            env->DeleteLocalRef(entity);
            continue;
        }

        float baseW = env->GetFloatField(entity, StrayCache::Entity_width);
        float newW = baseW + size * 0.5f;
        if (newW < 0.1f) newW = 0.1f;
        env->SetFloatField(entity, StrayCache::Entity_width, newW);

        float baseH = env->GetFloatField(entity, StrayCache::Entity_height);
        float newH = baseH + size * 0.3f;
        if (newH < 0.1f) newH = 0.1f;
        env->SetFloatField(entity, StrayCache::Entity_height, newH);

        env->DeleteLocalRef(entity);
    }

    env->DeleteLocalRef(listObj);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);
}

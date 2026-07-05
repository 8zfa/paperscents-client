#include "hitbox.h"
#include "../../../java/java.h"
#include "../../../sdk/strayCache.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"

HitBoxModule::HitBoxModule()
    : ModuleBase("HitBox", "Expand entity hitboxes", Category::Render)
{
    AddSetting<NumberSetting>("Size", 0.3f, 0.1f, 1.0f, 0.05f, "Hitbox expansion size");
}

void HitBoxModule::OnEnable() { Logger::Log("HitBox enabled"); }
void HitBoxModule::OnDisable() { Logger::Log("HitBox disabled"); }

void HitBoxModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject localPlayer = GetPlayerObject(env, mc);
    if (!localPlayer) { env->DeleteLocalRef(mc); return; }

    jobject world = GetWorldObject(env, mc);
    if (!world) { env->DeleteLocalRef(localPlayer); env->DeleteLocalRef(mc); return; }

    jclass worldClass = StrayCache::World;
    if (!worldClass) { env->DeleteLocalRef(world); env->DeleteLocalRef(localPlayer); env->DeleteLocalRef(mc); return; }

    jfieldID entitiesField = env->GetFieldID(worldClass, "g", "Ljava/util/List;");
    if (!entitiesField) { env->ExceptionClear(); entitiesField = env->GetFieldID(worldClass, "loadedEntityList", "Ljava/util/List;"); }
    env->ExceptionClear();

    if (!entitiesField) { env->DeleteLocalRef(world); env->DeleteLocalRef(localPlayer); env->DeleteLocalRef(mc); return; }

    jobject listObj = env->GetObjectField(world, entitiesField);
    if (!listObj) { env->DeleteLocalRef(world); env->DeleteLocalRef(localPlayer); env->DeleteLocalRef(mc); return; }

    jclass listClass = env->FindClass("java/util/List");
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    if (!sizeMethod || !getMethod)
    {
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(listObj);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(mc);
        return;
    }

    float size = ((NumberSetting*)FindSetting("Size"))->GetValue();

    jclass entityClass = StrayCache::Entity;
    if (!entityClass)
    {
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(listObj);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(mc);
        return;
    }

    jfieldID widthID = env->GetFieldID(entityClass, "n", "F");
    if (!widthID) { env->ExceptionClear(); widthID = env->GetFieldID(entityClass, "width", "F"); }
    env->ExceptionClear();

    jfieldID heightID = env->GetFieldID(entityClass, "o", "F");
    if (!heightID) { env->ExceptionClear(); heightID = env->GetFieldID(entityClass, "height", "F"); }
    env->ExceptionClear();

    int entSize = env->CallIntMethod(listObj, sizeMethod);
    for (int i = 0; i < entSize; i++)
    {
        jobject entity = env->CallObjectMethod(listObj, getMethod, i);
        if (!entity) continue;

        if (env->IsSameObject(entity, localPlayer))
        {
            env->DeleteLocalRef(entity);
            continue;
        }

        if (widthID)
        {
            float baseW = env->GetFloatField(entity, widthID);
            float newW = baseW + size * 0.5f;
            if (newW < 0.1f) newW = 0.1f;
            env->SetFloatField(entity, widthID, newW);
        }
        if (heightID)
        {
            float baseH = env->GetFloatField(entity, heightID);
            float newH = baseH + size * 0.3f;
            if (newH < 0.1f) newH = 0.1f;
            env->SetFloatField(entity, heightID, newH);
        }

        env->DeleteLocalRef(entity);
    }

    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(listObj);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);
}

#include "noweb.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <jni.h>
#include <cmath>

NoWebModule::NoWebModule()
    : ModuleBase("NoWeb", "Move freely in webs", Category::Movement)
{
    AddSetting<NumberSetting>("Motion", 0.2f, 0.1f, 1.0f, 0.1f, "Movement speed while in webs");
}

void NoWebModule::OnEnable() { Logger::Log("NoWeb enabled"); }
void NoWebModule::OnDisable() { Logger::Log("NoWeb disabled"); }

void NoWebModule::OnUpdate()
{
    if (!IsEnabled()) return;
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    env->ExceptionClear();

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;
    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jclass entityClass = StrayCache::Entity;
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    float speed = ((NumberSetting*)FindSetting("Motion"))->GetValue();

    jfieldID isInWebField = env->GetFieldID(entityClass, "D", "Z");
    if (!isInWebField) { env->ExceptionClear(); isInWebField = env->GetFieldID(entityClass, "isInWeb", "Z"); }
    env->ExceptionClear();
    if (!isInWebField) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    bool inWeb = env->GetBooleanField(player, isInWebField);
    if (!inWeb) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jfieldID motionXField = env->GetFieldID(entityClass, "v", "D");
    if (!motionXField) { env->ExceptionClear(); motionXField = env->GetFieldID(entityClass, "motionX", "D"); }
    env->ExceptionClear();

    jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
    if (!motionYField) { env->ExceptionClear(); motionYField = env->GetFieldID(entityClass, "motionY", "D"); }
    env->ExceptionClear();

    jfieldID motionZField = env->GetFieldID(entityClass, "x", "D");
    if (!motionZField) { env->ExceptionClear(); motionZField = env->GetFieldID(entityClass, "motionZ", "D"); }
    env->ExceptionClear();

    if (!motionXField || !motionYField || !motionZField)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }

    double mx = env->GetDoubleField(player, motionXField);
    double my = env->GetDoubleField(player, motionYField);
    double mz = env->GetDoubleField(player, motionZField);

    double len = std::sqrt(mx * mx + my * my + mz * mz);
    if (len > 0.0)
    {
        env->SetDoubleField(player, motionXField, (mx / len) * speed);
        env->SetDoubleField(player, motionYField, (my / len) * speed);
        env->SetDoubleField(player, motionZField, (mz / len) * speed);
    }

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

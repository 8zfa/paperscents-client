#include "highjump.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <jni.h>

HighJumpModule::HighJumpModule()
    : ModuleBase("HighJump", "Jump higher", Category::Movement)
{
    AddSetting<NumberSetting>("Height", 2.0f, 1.0f, 10.0f, 0.5f, "Jump height multiplier");
}

void HighJumpModule::OnEnable() { Logger::Log("HighJump enabled"); m_OnGround = false; }
void HighJumpModule::OnDisable() { Logger::Log("HighJump disabled"); }

void HighJumpModule::OnUpdate()
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

    float height = ((NumberSetting*)FindSetting("Height"))->GetValue();

    jfieldID onGroundField = env->GetFieldID(entityClass, "C", "Z");
    if (!onGroundField) { env->ExceptionClear(); onGroundField = env->GetFieldID(entityClass, "onGround", "Z"); }
    env->ExceptionClear();
    if (!onGroundField) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
    if (!motionYField) { env->ExceptionClear(); motionYField = env->GetFieldID(entityClass, "motionY", "D"); }
    env->ExceptionClear();
    if (!motionYField) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    bool onGround = env->GetBooleanField(player, onGroundField);
    double motionY = env->GetDoubleField(player, motionYField);

    if (m_OnGround && !onGround && motionY > 0.0)
        env->SetDoubleField(player, motionYField, height * 0.42);

    m_OnGround = onGround;

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

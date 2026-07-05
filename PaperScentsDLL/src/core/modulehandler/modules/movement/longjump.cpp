#include "longjump.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <jni.h>
#include <Windows.h>

LongJumpModule::LongJumpModule()
    : ModuleBase("LongJump", "Jump further", Category::Movement)
{
    AddSetting<NumberSetting>("Boost", 1.5f, 1.0f, 5.0f, 0.1f, "Horizontal motion multiplier on jump");
    AddSetting<BooleanSetting>("AutoJump", true, "Automatically jump when holding space");
}

void LongJumpModule::OnEnable() { Logger::Log("LongJump enabled"); m_Jumping = false; }
void LongJumpModule::OnDisable() { Logger::Log("LongJump disabled"); m_Jumping = false; }

void LongJumpModule::OnUpdate()
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

    float boost = ((NumberSetting*)FindSetting("Boost"))->GetValue();
    bool autoJump = ((BooleanSetting*)FindSetting("AutoJump"))->GetValue();

    jfieldID onGroundField = env->GetFieldID(entityClass, "C", "Z");
    if (!onGroundField) { env->ExceptionClear(); onGroundField = env->GetFieldID(entityClass, "onGround", "Z"); }
    env->ExceptionClear();
    if (!onGroundField) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

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

    bool onGround = env->GetBooleanField(player, onGroundField);
    bool pressingSpace = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    if (onGround && (pressingSpace || autoJump))
    {
        env->SetDoubleField(player, motionYField, 0.42);
        m_Jumping = true;
    }

    if (m_Jumping && !onGround)
    {
        double mx = env->GetDoubleField(player, motionXField);
        double mz = env->GetDoubleField(player, motionZField);
        env->SetDoubleField(player, motionXField, mx * boost);
        env->SetDoubleField(player, motionZField, mz * boost);
        m_Jumping = false;
    }

    if (onGround)
        m_Jumping = false;

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

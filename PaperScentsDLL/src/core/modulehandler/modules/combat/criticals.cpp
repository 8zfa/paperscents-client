#include "criticals.h"
#include "../../../java/java.h"
#include "../../../sdk/strayCache.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"

CriticalsModule::CriticalsModule()
    : ModuleBase("Criticals", "Always land critical hits", Category::Combat)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Packet", "MiniJump", "Position"});
    m_LastCrit = std::chrono::steady_clock::now();
}

void CriticalsModule::OnEnable() { Logger::Log("Criticals enabled"); }
void CriticalsModule::OnDisable() { Logger::Log("Criticals disabled"); }

void CriticalsModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jclass entityClass = StrayCache::Entity;
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jfieldID onGroundField = env->GetFieldID(entityClass, "C", "Z");
    if (!onGroundField) { env->ExceptionClear(); onGroundField = env->GetFieldID(entityClass, "onGround", "Z"); }
    env->ExceptionClear();

    jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
    if (!motionYField) { env->ExceptionClear(); motionYField = env->GetFieldID(entityClass, "motionY", "D"); }
    env->ExceptionClear();

    jfieldID posYField = env->GetFieldID(entityClass, "t", "D");
    if (!posYField) { env->ExceptionClear(); posYField = env->GetFieldID(entityClass, "posY", "D"); }
    env->ExceptionClear();

    if (!onGroundField) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jboolean onGround = env->GetBooleanField(player, onGroundField);

    bool attacking = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    auto now = std::chrono::steady_clock::now();
    bool cooldown = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastCrit).count() < 200;

    if (onGround && attacking && !m_AppliedThisTick && !cooldown)
    {
        int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();

        if (mode == 0)
        {
            if (posYField && motionYField)
            {
                double currentY = env->GetDoubleField(player, posYField);
                env->SetDoubleField(player, posYField, currentY + 0.0625);
                env->SetBooleanField(player, onGroundField, JNI_FALSE);
                env->SetDoubleField(player, motionYField, -0.1);
            }
        }
        else if (mode == 1)
        {
            if (motionYField)
            {
                env->SetDoubleField(player, motionYField, 0.2);
                env->SetBooleanField(player, onGroundField, JNI_FALSE);
            }
        }
        else if (mode == 2)
        {
            if (posYField)
            {
                double currentY = env->GetDoubleField(player, posYField);
                env->SetDoubleField(player, posYField, currentY + 0.0625);
                env->SetBooleanField(player, onGroundField, JNI_FALSE);
            }
        }

        m_AppliedThisTick = true;
    }
    else if (!onGround && m_AppliedThisTick)
    {
        m_LastCrit = now;
        m_AppliedThisTick = false;
    }
    else if (onGround)
    {
        m_AppliedThisTick = false;
    }

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

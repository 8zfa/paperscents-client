#include "fastuse.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"

FastUseModule::FastUseModule()
    : ModuleBase("FastUse", "Use items faster", Category::Player)
{
    AddSetting<NumberSetting>("Speed", 1.5f, 1.0f, 3.0f, 0.1f, "Item use speed multiplier");
}

void FastUseModule::OnEnable()
{
    Logger::Log("FastUse enabled");
    m_Cached = false;
    m_TimerObj = nullptr;
    m_TimerField = nullptr;
    m_SpeedField = nullptr;
    m_OriginalSpeed = 1.0f;
}

void FastUseModule::OnDisable()
{
    Logger::Log("FastUse disabled");
    if (m_TimerObj && m_SpeedField)
    {
        JNIEnv* env = Java::GetThreadEnv();
        if (env)
        {
            env->SetFloatField(m_TimerObj, m_SpeedField, 1.0f);
            env->DeleteGlobalRef(m_TimerObj);
        }
        m_TimerObj = nullptr;
    }
}

void FastUseModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!StrayCache::Minecraft || !StrayCache::MinecraftInstance) return;

    if (!m_Cached)
    {
        m_TimerField = env->GetFieldID(StrayCache::Minecraft, "am", "Lavk;");
        if (!m_TimerField) { env->ExceptionClear(); m_TimerField = env->GetFieldID(StrayCache::Minecraft, "timer", "Lavk;"); }
        env->ExceptionClear();

        if (m_TimerField)
        {
            jobject timer = env->GetObjectField(StrayCache::MinecraftInstance, m_TimerField);
            if (timer)
            {
                jclass timerClass = env->GetObjectClass(timer);
                m_SpeedField = env->GetFieldID(timerClass, "a", "F");
                if (!m_SpeedField) { env->ExceptionClear(); m_SpeedField = env->GetFieldID(timerClass, "timerSpeed", "F"); }
                env->ExceptionClear();
                m_TimerObj = env->NewGlobalRef(timer);
                if (m_SpeedField) m_OriginalSpeed = env->GetFloatField(timer, m_SpeedField);
                env->DeleteLocalRef(timerClass);
                env->DeleteLocalRef(timer);
            }
        }
        m_Cached = true;
    }

    if (!m_TimerObj || !m_SpeedField) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jclass playerClass = env->GetObjectClass(player);
    jmethodID isUsingItem = env->GetMethodID(playerClass, "e", "()Z");
    if (!isUsingItem) { env->ExceptionClear(); isUsingItem = env->GetMethodID(playerClass, "isUsingItem", "()Z"); }
    env->ExceptionClear();

    bool usingItem = false;
    if (isUsingItem)
        usingItem = env->CallBooleanMethod(player, isUsingItem) == JNI_TRUE;

    if (usingItem)
    {
        float speed = ((NumberSetting*)FindSetting("Speed"))->GetValue();
        env->SetFloatField(m_TimerObj, m_SpeedField, speed);
    }
    else
    {
        env->SetFloatField(m_TimerObj, m_SpeedField, 1.0f);
    }

    env->DeleteLocalRef(playerClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

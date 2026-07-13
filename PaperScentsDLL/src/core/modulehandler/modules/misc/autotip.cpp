#include "autotip.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"

AutoTipModule::AutoTipModule()
    : ModuleBase("AutoTip", "Auto tip players", Category::Misc)
{
    AddSetting<NumberSetting>("Interval", 120.0f, 30.0f, 600.0f, 10.0f, "Interval between tips (seconds)");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void AutoTipModule::OnEnable()
{
    Logger::Log("AutoTip enabled");
    m_LastTip = std::chrono::steady_clock::now();
}

void AutoTipModule::OnDisable() { Logger::Log("AutoTip disabled"); }

void AutoTipModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_LastTip).count();
    float interval = ((NumberSetting*)FindSetting("Interval"))->GetValue();

    if (elapsed < interval) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jclass playerClass = env->GetObjectClass(player);
    jmethodID sendChat = env->GetMethodID(playerClass, "a", "(Ljava/lang/String;)V");
    if (!sendChat) { env->ExceptionClear(); sendChat = env->GetMethodID(playerClass, "sendChatMessage", "(Ljava/lang/String;)V"); }
    env->ExceptionClear();

    if (sendChat)
    {
        jstring msg = env->NewStringUTF("/tip all");
        env->CallVoidMethod(player, sendChat, msg);
        env->DeleteLocalRef(msg);
        m_LastTip = std::chrono::steady_clock::now();
    }

    env->DeleteLocalRef(playerClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

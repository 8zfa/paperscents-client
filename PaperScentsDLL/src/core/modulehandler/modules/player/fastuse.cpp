#include "fastuse.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"

FastUseModule::FastUseModule()
    : ModuleBase("FastUse", "Use items faster", Category::Player)
{
    AddSetting<NumberSetting>("Speed", 1.5f, 1.0f, 3.0f, 0.1f, "Item use speed multiplier");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
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
        if (env) { env->SetFloatField(m_TimerObj, m_SpeedField, 1.0f); env->DeleteGlobalRef(m_TimerObj); }
        m_TimerObj = nullptr;
    }
}

void FastUseModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    env->ExceptionClear();

    if (!m_Cached)
    {
        // Find Timer class via notch
        jobject mcInst = BridgeHelper::GetMinecraftInstance(env);
        if (!mcInst)
        {
            // Try notch
            Java* java = Core::GetInstance().GetJava();
            if (!java || !java->IsValid()) return;
            jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
            if (!mcClass) return;
            jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
            if (!getMc) { env->DeleteLocalRef(mcClass); return; }
            mcInst = env->CallStaticObjectMethod(mcClass, getMc);
            if (!mcInst) { env->DeleteLocalRef(mcClass); return; }

            jfieldID timerField = env->GetFieldID(mcClass, "am", "Lavk;");
            if (!timerField) { env->ExceptionClear(); timerField = env->GetFieldID(mcClass, "timer", "Lavk;"); }
            env->ExceptionClear();
            if (timerField)
            {
                jobject timer = env->GetObjectField(mcInst, timerField);
                if (timer)
                {
                    jclass timerClass = env->GetObjectClass(timer);
                    m_SpeedField = env->GetFieldID(timerClass, "a", "F");
                    if (!m_SpeedField) { env->ExceptionClear(); m_SpeedField = env->GetFieldID(timerClass, "timerSpeed", "F"); }
                    env->ExceptionClear();
                    if (m_SpeedField) m_OriginalSpeed = env->GetFloatField(timer, m_SpeedField);
                    m_TimerObj = env->NewGlobalRef(timer);
                    env->DeleteLocalRef(timerClass);
                    env->DeleteLocalRef(timer);
                }
            }
            env->DeleteLocalRef(mcClass);
        }
        else
        {
            // Try notch timer field on bridge-provided mc
            jclass mcClass = env->FindClass("net/minecraft/client/Minecraft");
            if (mcClass)
            {
                jfieldID timerField = env->GetFieldID(mcClass, "timer", "Lnet/minecraft/util/Timer;");
                if (timerField)
                {
                    jobject timer = env->GetObjectField(mcInst, timerField);
                    if (timer)
                    {
                        jclass timerClass = env->GetObjectClass(timer);
                        m_SpeedField = env->GetFieldID(timerClass, "timerSpeed", "F");
                        if (!m_SpeedField) { env->ExceptionClear(); }
                        if (m_SpeedField) { env->ExceptionClear(); m_OriginalSpeed = env->GetFloatField(timer, m_SpeedField); }
                        m_TimerObj = env->NewGlobalRef(timer);
                        env->DeleteLocalRef(timerClass);
                        env->DeleteLocalRef(timer);
                    }
                }
                env->DeleteLocalRef(mcClass);
            }
        }
        if (mcInst) env->DeleteLocalRef(mcInst);
        m_Cached = true;
    }

    if (!m_TimerObj || !m_SpeedField) return;

    // Check if player is using item via bridge or notch
    bool bridged = BridgeHelper::Initialize(env);
    jobject player = bridged ? BridgeHelper::GetPlayer(env) : nullptr;
    if (!player)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java || !java->IsValid()) return;
        jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
        if (!mcClass) return;
        jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
        if (!getMc) { env->DeleteLocalRef(mcClass); return; }
        jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
        if (!mc) { env->DeleteLocalRef(mcClass); return; }
        for (auto f : {"s", "t", "h", "thePlayer"})
        {
            jfieldID pf = env->GetFieldID(mcClass, f, "Lbew;");
            if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
            if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
        }
        if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    }

    bool usingItem = false;
    if (bridged && BridgeHelper::ELBridge_IsUsingItem)
        usingItem = env->CallBooleanMethod(player, BridgeHelper::ELBridge_IsUsingItem) != 0;
    else
    {
        jclass playerClass = env->GetObjectClass(player);
        jmethodID isUsingItemMeth = env->GetMethodID(playerClass, "e", "()Z");
        if (!isUsingItemMeth) { env->ExceptionClear(); isUsingItemMeth = env->GetMethodID(playerClass, "isUsingItem", "()Z"); }
        if (isUsingItemMeth) { env->ExceptionClear(); usingItem = env->CallBooleanMethod(player, isUsingItemMeth); }
        env->DeleteLocalRef(playerClass);
    }

    float speed = ((NumberSetting*)FindSetting("Speed"))->GetValue();
    env->SetFloatField(m_TimerObj, m_SpeedField, usingItem ? speed : 1.0f);

    env->DeleteLocalRef(player);
}

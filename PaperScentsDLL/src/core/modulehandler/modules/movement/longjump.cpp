#include "longjump.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <Windows.h>

LongJumpModule::LongJumpModule()
    : ModuleBase("LongJump", "Jump further", Category::Movement)
{
    AddSetting<NumberSetting>("Boost", 1.5f, 1.0f, 5.0f, 0.1f, "Horizontal motion multiplier on jump");
    AddSetting<BooleanSetting>("AutoJump", true, "Automatically jump when holding space");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void LongJumpModule::OnEnable() { Logger::Log("LongJump enabled"); m_Jumping = false; }
void LongJumpModule::OnDisable() { Logger::Log("LongJump disabled"); m_Jumping = false; }

void LongJumpModule::OnUpdate()
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

    float boost = ((NumberSetting*)FindSetting("Boost"))->GetValue();
    bool autoJump = ((BooleanSetting*)FindSetting("AutoJump"))->GetValue();

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

        jclass entityClass = java->FindClass("vg", "net/minecraft/entity/Entity");
        if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

        jfieldID onGroundField = env->GetFieldID(entityClass, "C", "Z");
        if (!onGroundField) { env->ExceptionClear(); onGroundField = env->GetFieldID(entityClass, "onGround", "Z"); }
        env->ExceptionClear();
        jfieldID motionXField = env->GetFieldID(entityClass, "v", "D");
        if (!motionXField) { env->ExceptionClear(); motionXField = env->GetFieldID(entityClass, "motionX", "D"); }
        env->ExceptionClear();
        jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
        if (!motionYField) { env->ExceptionClear(); motionYField = env->GetFieldID(entityClass, "motionY", "D"); }
        env->ExceptionClear();
        jfieldID motionZField = env->GetFieldID(entityClass, "x", "D");
        if (!motionZField) { env->ExceptionClear(); motionZField = env->GetFieldID(entityClass, "motionZ", "D"); }
        env->ExceptionClear();

        if (!onGroundField || !motionXField || !motionYField || !motionZField)
        {
            env->DeleteLocalRef(entityClass);
            env->DeleteLocalRef(player);
            env->DeleteLocalRef(mc);
            env->DeleteLocalRef(mcClass);
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

        if (onGround) m_Jumping = false;

        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
    }
    else
    {
        // Bridged path
        bool onGround = BridgeHelper::EntityBridge_IsOnGround
            ? env->CallBooleanMethod(player, BridgeHelper::EntityBridge_IsOnGround) != 0 : false;
        bool pressingSpace = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

        if (onGround && (pressingSpace || autoJump))
        {
            if (BridgeHelper::EntityBridge_SetMotionY)
                env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionY, 0.42);
            m_Jumping = true;
        }

        if (m_Jumping && !onGround)
        {
            double mx = BridgeHelper::EntityBridge_GetMotionX
                ? env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionX) : 0;
            double mz = BridgeHelper::EntityBridge_GetMotionZ
                ? env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionZ) : 0;
            if (BridgeHelper::EntityBridge_SetMotionX) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionX, mx * boost);
            if (BridgeHelper::EntityBridge_SetMotionZ) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionZ, mz * boost);
            m_Jumping = false;
        }

        if (onGround) m_Jumping = false;
    }

    if (player) env->DeleteLocalRef(player);
}

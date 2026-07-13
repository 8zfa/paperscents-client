#include "highjump.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"

HighJumpModule::HighJumpModule()
    : ModuleBase("HighJump", "Jump higher", Category::Movement)
{
    AddSetting<NumberSetting>("Height", 2.0f, 1.0f, 10.0f, 0.5f, "Jump height multiplier");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void HighJumpModule::OnEnable() { Logger::Log("HighJump enabled"); m_OnGround = false; }
void HighJumpModule::OnDisable() { Logger::Log("HighJump disabled"); }

void HighJumpModule::OnUpdate()
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

    float height = ((NumberSetting*)FindSetting("Height"))->GetValue();

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
        jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
        if (!motionYField) { env->ExceptionClear(); motionYField = env->GetFieldID(entityClass, "motionY", "D"); }
        env->ExceptionClear();

        if (!onGroundField || !motionYField)
        {
            env->DeleteLocalRef(entityClass);
            env->DeleteLocalRef(player);
            env->DeleteLocalRef(mc);
            env->DeleteLocalRef(mcClass);
            return;
        }

        bool onGround = env->GetBooleanField(player, onGroundField);
        double motionY = env->GetDoubleField(player, motionYField);

        if (m_OnGround && !onGround && motionY > 0.0)
            env->SetDoubleField(player, motionYField, height * 0.42);

        m_OnGround = onGround;

        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
    }
    else
    {
        // Bridged path
        bool onGround = BridgeHelper::EntityBridge_IsOnGround
            ? env->CallBooleanMethod(player, BridgeHelper::EntityBridge_IsOnGround) != 0 : false;
        double motionY = BridgeHelper::EntityBridge_GetMotionY
            ? env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionY) : 0;

        if (m_OnGround && !onGround && motionY > 0.0 && BridgeHelper::EntityBridge_SetMotionY)
            env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionY, height * 0.42);

        m_OnGround = onGround;
    }

    if (player) env->DeleteLocalRef(player);
}

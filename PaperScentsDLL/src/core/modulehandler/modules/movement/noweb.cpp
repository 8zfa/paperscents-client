#include "noweb.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <cmath>

NoWebModule::NoWebModule()
    : ModuleBase("NoWeb", "Move freely in webs", Category::Movement)
{
    AddSetting<NumberSetting>("Motion", 0.2f, 0.1f, 1.0f, 0.1f, "Movement speed while in webs");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void NoWebModule::OnEnable() { Logger::Log("NoWeb enabled"); }
void NoWebModule::OnDisable() { Logger::Log("NoWeb disabled"); }

void NoWebModule::OnUpdate()
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

    float speed = ((NumberSetting*)FindSetting("Motion"))->GetValue();

    // No bridge for isInWeb. Use notch fallback only (vanilla).
    // On Lunar, this module won't function.
    jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
    if (!entityClass) { env->ExceptionClear(); return; }

    jfieldID isInWebField = env->GetFieldID(entityClass, "isInWeb", "Z");
    if (!isInWebField) { env->ExceptionClear(); env->DeleteLocalRef(entityClass); return; }
    env->ExceptionClear();

    bool bridged = BridgeHelper::Initialize(env);
    jobject player = bridged ? BridgeHelper::GetPlayer(env) : nullptr;

    if (!player)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java || !java->IsValid()) { env->DeleteLocalRef(entityClass); return; }
        jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
        if (!mcClass) { env->DeleteLocalRef(entityClass); return; }
        jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
        if (!getMc) { env->DeleteLocalRef(mcClass); env->DeleteLocalRef(entityClass); return; }
        jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
        if (!mc) { env->DeleteLocalRef(mcClass); env->DeleteLocalRef(entityClass); return; }
        for (auto f : {"s", "t", "h", "thePlayer"})
        {
            jfieldID pf = env->GetFieldID(mcClass, f, "Lbew;");
            if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
            if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
        }
        if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); env->DeleteLocalRef(entityClass); return; }
        // Notch path
        bool inWeb = env->GetBooleanField(player, isInWebField);
        if (!inWeb) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); env->DeleteLocalRef(entityClass); return; }

        jfieldID motionXField = env->GetFieldID(entityClass, "v", "D");
        if (!motionXField) { env->ExceptionClear(); motionXField = env->GetFieldID(entityClass, "motionX", "D"); }
        env->ExceptionClear();
        jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
        if (!motionYField) { env->ExceptionClear(); motionYField = env->GetFieldID(entityClass, "motionY", "D"); }
        env->ExceptionClear();
        jfieldID motionZField = env->GetFieldID(entityClass, "x", "D");
        if (!motionZField) { env->ExceptionClear(); motionZField = env->GetFieldID(entityClass, "motionZ", "D"); }
        env->ExceptionClear();

        if (motionXField && motionYField && motionZField)
        {
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
        }

        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
    }
    else
    {
        // Bridged path — isInWeb via notch, motion via bridge if possible
        bool inWeb = env->GetBooleanField(player, isInWebField);
        if (!inWeb) { env->DeleteLocalRef(player); env->DeleteLocalRef(entityClass); return; }

        double mx = 0, my = 0, mz = 0;
        if (BridgeHelper::EntityBridge_GetMotionX) mx = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionX);
        if (BridgeHelper::EntityBridge_GetMotionY) my = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionY);
        if (BridgeHelper::EntityBridge_GetMotionZ) mz = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionZ);

        double len = std::sqrt(mx * mx + my * my + mz * mz);
        if (len > 0.0)
        {
            if (BridgeHelper::EntityBridge_SetMotionX) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionX, (mx / len) * speed);
            if (BridgeHelper::EntityBridge_SetMotionY) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionY, (my / len) * speed);
            if (BridgeHelper::EntityBridge_SetMotionZ) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionZ, (mz / len) * speed);
        }
    }

    if (player) env->DeleteLocalRef(player);
    env->DeleteLocalRef(entityClass);
}

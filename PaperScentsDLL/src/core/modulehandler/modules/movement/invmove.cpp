#include "invmove.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <cmath>

InvMoveModule::InvMoveModule()
    : ModuleBase("InvMove", "Move while in inventory", Category::Movement)
{
    AddSetting<BooleanSetting>("Sprint", true, "Allow sprinting in inventory");
    AddSetting<BooleanSetting>("Sneak", true, "Allow sneaking in inventory");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void InvMoveModule::OnEnable() { Logger::Log("InvMove enabled"); }
void InvMoveModule::OnDisable() { Logger::Log("InvMove disabled"); }

void InvMoveModule::OnUpdate()
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

    bool sprint = ((BooleanSetting*)FindSetting("Sprint"))->GetValue();
    bool sneak = ((BooleanSetting*)FindSetting("Sneak"))->GetValue();

    bool bridged = BridgeHelper::Initialize(env);
    jobject player = bridged ? BridgeHelper::GetPlayer(env) : nullptr;
    jobject mc = nullptr;
    jclass mcClass = nullptr;

    if (!player)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java || !java->IsValid()) return;
        mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
        if (!mcClass) return;
        jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
        if (!getMc) { env->DeleteLocalRef(mcClass); return; }
        mc = env->CallStaticObjectMethod(mcClass, getMc);
        if (!mc) { env->DeleteLocalRef(mcClass); return; }
        for (auto f : {"s", "t", "h", "thePlayer"})
        {
            jfieldID pf = env->GetFieldID(mcClass, f, "Lbew;");
            if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
            if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
        }
        if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    }
    else
    {
        mc = BridgeHelper::GetMinecraftInstance(env);
    }

    // Check if a screen is open
    jfieldID screenField = nullptr;
    jclass mcCls = mcClass ? mcClass : (bridged ? BridgeHelper::MinecraftBridge : nullptr);
    if (mcCls)
        screenField = env->GetFieldID(mcCls, "currentScreen", "Lnet/minecraft/client/gui/GuiScreen;");
    else if (mc)
    {
        // Try notch field names
        Java* java = Core::GetInstance().GetJava();
        if (java)
        {
            mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
            if (mcClass)
            {
                screenField = env->GetFieldID(mcClass, "m", "Lbdz;");
                if (!screenField) { env->ExceptionClear(); screenField = env->GetFieldID(mcClass, "currentScreen", "Lnet/minecraft/client/gui/GuiScreen;"); }
                if (!screenField) { env->ExceptionClear(); screenField = env->GetFieldID(mcClass, "m", "Ljava/lang/Object;"); }
            }
        }
    }

    jobject currentScreen = nullptr;
    if (screenField && mc)
    {
        env->ExceptionClear();
        currentScreen = env->GetObjectField(mc, screenField);
    }

    if (!currentScreen)
    {
        if (player) env->DeleteLocalRef(player);
        if (mc) env->DeleteLocalRef(mc);
        if (mcClass) env->DeleteLocalRef(mcClass);
        return;
    }

    bool forward = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool back = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool left = (GetAsyncKeyState('A') & 0x8000) != 0;
    bool right = (GetAsyncKeyState('D') & 0x8000) != 0;
    bool jump = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    int forwardV = (forward ? 1 : 0) - (back ? 1 : 0);
    int strafeV = (right ? 1 : 0) - (left ? 1 : 0);

    if (forwardV == 0 && strafeV == 0)
    {
        env->DeleteLocalRef(currentScreen);
        if (player) env->DeleteLocalRef(player);
        if (mc) env->DeleteLocalRef(mc);
        if (mcClass) env->DeleteLocalRef(mcClass);
        return;
    }

    // Motion + yaw via bridge or notch
    bool onGround = bridged && BridgeHelper::EntityBridge_IsOnGround
        ? env->CallBooleanMethod(player, BridgeHelper::EntityBridge_IsOnGround) != 0
        : false;
    float yaw = bridged && BridgeHelper::EntityBridge_GetRotationYaw
        ? (float)env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetRotationYaw)
        : 0.0f;

    if (!bridged)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java) { env->DeleteLocalRef(currentScreen); env->DeleteLocalRef(player); if (mc) env->DeleteLocalRef(mc); if (mcClass) env->DeleteLocalRef(mcClass); return; }
        jclass entityClass = java->FindClass("vg", "net/minecraft/entity/Entity");
        if (!entityClass) { env->DeleteLocalRef(currentScreen); env->DeleteLocalRef(player); if (mc) env->DeleteLocalRef(mc); if (mcClass) env->DeleteLocalRef(mcClass); return; }

        jfieldID onGroundField = env->GetFieldID(entityClass, "C", "Z");
        if (!onGroundField) { env->ExceptionClear(); onGroundField = env->GetFieldID(entityClass, "onGround", "Z"); }
        if (onGroundField) { env->ExceptionClear(); onGround = env->GetBooleanField(player, onGroundField); }

        jfieldID yawField = env->GetFieldID(entityClass, "y", "F");
        if (!yawField) { env->ExceptionClear(); yawField = env->GetFieldID(entityClass, "rotationYaw", "F"); }
        if (yawField) { env->ExceptionClear(); yaw = env->GetFloatField(player, yawField); }

        jfieldID motionXField = env->GetFieldID(entityClass, "v", "D");
        if (!motionXField) { env->ExceptionClear(); motionXField = env->GetFieldID(entityClass, "motionX", "D"); }
        jfieldID motionZField = env->GetFieldID(entityClass, "x", "D");
        if (!motionZField) { env->ExceptionClear(); motionZField = env->GetFieldID(entityClass, "motionZ", "D"); }

        if (motionXField && motionZField)
        {
            float rad = yaw * 0.017453292f;
            double sin = std::sin(rad);
            double cos = std::cos(rad);
            double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
            double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
            double len = std::sqrt(targetX * targetX + targetZ * targetZ);
            if (len > 0.0)
            {
                targetX /= len; targetZ /= len;
                double speed = onGround ? 0.28 : 0.15;
                env->SetDoubleField(player, motionXField, targetX * speed);
                env->SetDoubleField(player, motionZField, targetZ * speed);

                if (jump && onGround)
                {
                    jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
                    if (!motionYField) { env->ExceptionClear(); motionYField = env->GetFieldID(entityClass, "motionY", "D"); }
                    if (motionYField) { env->ExceptionClear(); env->SetDoubleField(player, motionYField, 0.42); }
                }

                if (sprint && forward)
                {
                    jmethodID setSprinting = nullptr;
                    for (auto m : {"b", "c", "a", "setSprinting", "d"})
                    {
                        setSprinting = env->GetMethodID(entityClass, m, "(Z)V");
                        if (setSprinting) break;
                        env->ExceptionClear();
                    }
                    if (setSprinting) env->CallVoidMethod(player, setSprinting, JNI_TRUE);
                }

                if (sneak)
                {
                    jmethodID setSneaking = nullptr;
                    for (auto m : {"a", "n", "setSneaking", "b"})
                    {
                        setSneaking = env->GetMethodID(entityClass, m, "(Z)V");
                        if (setSneaking) break;
                        env->ExceptionClear();
                    }
                    if (setSneaking) env->CallVoidMethod(player, setSneaking, JNI_TRUE);
                }
            }
        }

        env->DeleteLocalRef(entityClass);
    }
    else
    {
        // Bridged path
        float rad = yaw * 0.017453292f;
        double sin = std::sin(rad);
        double cos = std::cos(rad);
        double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
        double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
        double len = std::sqrt(targetX * targetX + targetZ * targetZ);
        if (len > 0.0)
        {
            targetX /= len; targetZ /= len;
            double moveSpeed = onGround ? 0.28 : 0.15;
            if (BridgeHelper::EntityBridge_SetMotionX) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionX, targetX * moveSpeed);
            if (BridgeHelper::EntityBridge_SetMotionZ) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionZ, targetZ * moveSpeed);

            if (jump && onGround && BridgeHelper::EntityBridge_SetMotionY)
                env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionY, 0.42);

            if (sprint && forward && BridgeHelper::EPSPBridge_SetSprinting)
                env->CallVoidMethod(player, BridgeHelper::EPSPBridge_SetSprinting, JNI_TRUE);

            // No bridge for setSneaking, skip on Lunar
        }
    }

    env->DeleteLocalRef(currentScreen);
    if (player) env->DeleteLocalRef(player);
    if (mc) env->DeleteLocalRef(mc);
    if (mcClass) env->DeleteLocalRef(mcClass);
}

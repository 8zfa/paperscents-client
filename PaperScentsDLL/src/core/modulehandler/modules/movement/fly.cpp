#include "fly.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <cmath>

FlyModule::FlyModule()
    : ModuleBase("Fly", "Allows you to fly", Category::Movement)
{
    AddSetting<NumberSetting>("Speed", 1.0f, 0.1f, 5.0f, 0.1f);
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Vanilla", "Creative", "Slide"});
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void FlyModule::OnEnable() { Logger::Log("Fly enabled"); }
void FlyModule::OnDisable() { Logger::Log("Fly disabled"); }

void FlyModule::OnUpdate()
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

    float speed = ((NumberSetting*)FindSetting("Speed"))->GetValue();
    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();

    // Get player via bridge or notch
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

        // Notch path
        jclass entityClass = java->FindClass("vg", "net/minecraft/entity/Entity");
        if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

        if (mode == 0 || mode == 1)
        {
            jclass capabilitiesClass = java->FindClass("vj", "net/minecraft/entity/player/PlayerCapabilities");
            if (!capabilitiesClass) capabilitiesClass = env->FindClass("net/minecraft/entity/player/PlayerCapabilities");

            for (auto f : {"a", "field_71075_bZ", "capabilities"})
            {
                jfieldID cf = env->GetFieldID(entityClass, f, "Lvj;");
                if (!cf) cf = env->GetFieldID(entityClass, f, "Lnet/minecraft/entity/player/PlayerCapabilities;");
                if (cf)
                {
                    jobject capabilities = env->GetObjectField(player, cf);
                    if (capabilities)
                    {
                        jfieldID isFlyingField = env->GetFieldID(capabilitiesClass, "d", "Z");
                        if (!isFlyingField) { env->ExceptionClear(); isFlyingField = env->GetFieldID(capabilitiesClass, "isFlying", "Z"); }
                        if (!isFlyingField) { env->ExceptionClear(); isFlyingField = env->GetFieldID(capabilitiesClass, "field_75100_b", "Z"); }
                        if (isFlyingField) { env->ExceptionClear(); env->SetBooleanField(capabilities, isFlyingField, JNI_TRUE); }

                        jfieldID flySpeedField = env->GetFieldID(capabilitiesClass, "e", "F");
                        if (!flySpeedField) { env->ExceptionClear(); flySpeedField = env->GetFieldID(capabilitiesClass, "flySpeed", "F"); }
                        if (!flySpeedField) { env->ExceptionClear(); flySpeedField = env->GetFieldID(capabilitiesClass, "field_75098_d", "F"); }
                        if (flySpeedField) { env->ExceptionClear(); env->SetFloatField(capabilities, flySpeedField, speed * 0.05f); }

                        if (mode == 1)
                        {
                            jfieldID allowFlyingField = env->GetFieldID(capabilitiesClass, "c", "Z");
                            if (!allowFlyingField) { env->ExceptionClear(); allowFlyingField = env->GetFieldID(capabilitiesClass, "allowFlying", "Z"); }
                            if (!allowFlyingField) { env->ExceptionClear(); allowFlyingField = env->GetFieldID(capabilitiesClass, "field_75101_c", "Z"); }
                            if (allowFlyingField) { env->ExceptionClear(); env->SetBooleanField(capabilities, allowFlyingField, JNI_TRUE); }
                        }

                        env->DeleteLocalRef(capabilities);
                    }
                    break;
                }
            }
            env->DeleteLocalRef(capabilitiesClass);
        }
        else if (mode == 2)
        {
            jfieldID noClipField = env->GetFieldID(entityClass, "j", "Z");
            if (!noClipField) { env->ExceptionClear(); noClipField = env->GetFieldID(entityClass, "noClip", "Z"); }
            if (noClipField) { env->ExceptionClear(); env->SetBooleanField(player, noClipField, JNI_TRUE); }

            jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
            if (!motionYField) { env->ExceptionClear(); motionYField = env->GetFieldID(entityClass, "motionY", "D"); }
            if (motionYField)
            {
                env->ExceptionClear();
                double currentMotionY = env->GetDoubleField(player, motionYField);
                env->SetDoubleField(player, motionYField, currentMotionY * 0.9 + 0.05);
            }

            jfieldID motionXField = env->GetFieldID(entityClass, "v", "D");
            if (!motionXField) { env->ExceptionClear(); motionXField = env->GetFieldID(entityClass, "motionX", "D"); }
            jfieldID motionZField = env->GetFieldID(entityClass, "x", "D");
            if (!motionZField) { env->ExceptionClear(); motionZField = env->GetFieldID(entityClass, "motionZ", "D"); }

            if (motionXField && motionZField)
            {
                double mx = env->GetDoubleField(player, motionXField);
                double mz = env->GetDoubleField(player, motionZField);
                double currentHoriz = std::sqrt(mx * mx + mz * mz);
                double maxSpeed = speed * 0.28;
                if (currentHoriz < maxSpeed)
                {
                    jclass livingClass = java->FindClass("sa", "net/minecraft/entity/EntityLivingBase");
                    if (livingClass)
                    {
                        jfieldID moveForwardField = env->GetFieldID(livingClass, "bb", "F");
                        if (!moveForwardField) { env->ExceptionClear(); moveForwardField = env->GetFieldID(livingClass, "moveForward", "F"); }
                        jfieldID moveStrafingField = env->GetFieldID(livingClass, "bc", "F");
                        if (!moveStrafingField) { env->ExceptionClear(); moveStrafingField = env->GetFieldID(livingClass, "moveStrafing", "F"); }
                        if (moveForwardField && moveStrafingField)
                        {
                            float forward = env->GetFloatField(player, moveForwardField);
                            float strafe = env->GetFloatField(player, moveStrafingField);
                            if (forward != 0.0f || strafe != 0.0f)
                            {
                                jfieldID rotYawField = env->GetFieldID(entityClass, "y", "F");
                                if (!rotYawField) { env->ExceptionClear(); rotYawField = env->GetFieldID(entityClass, "rotationYaw", "F"); }
                                if (rotYawField)
                                {
                                    float yaw = env->GetFloatField(player, rotYawField);
                                    float rad = yaw * 0.017453292f;
                                    double sin = std::sin(rad);
                                    double cos = std::cos(rad);
                                    double targetX = (double)(-forward) * sin + (double)(strafe) * cos;
                                    double targetZ = (double)(forward) * cos - (double)(strafe) * sin;
                                    double len = std::sqrt(targetX * targetX + targetZ * targetZ);
                                    if (len > 0.0)
                                    {
                                        targetX /= len; targetZ /= len;
                                        env->SetDoubleField(player, motionXField, targetX * maxSpeed);
                                        env->SetDoubleField(player, motionZField, targetZ * maxSpeed);
                                    }
                                }
                            }
                        }
                        env->DeleteLocalRef(livingClass);
                    }
                }
            }
        }

        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        env->DeleteLocalRef(player);
        return;
    }

    // --- Bridged path (Lunar) ---
    // No bridge for PlayerCapabilities. Vanilla and Creative modes can't work on Lunar.
    // Slide mode uses motion + noClip field via notch fallback.
    if (mode == 2)
    {
        jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
        if (entityClass)
        {
            jfieldID noClipField = env->GetFieldID(entityClass, "noClip", "Z");
            if (noClipField) { env->ExceptionClear(); env->SetBooleanField(player, noClipField, JNI_TRUE); }
            else env->ExceptionClear();

            jfieldID motionYField = env->GetFieldID(entityClass, "motionY", "D");
            if (motionYField)
            {
                env->ExceptionClear();
                double currentMotionY = env->GetDoubleField(player, motionYField);
                env->SetDoubleField(player, motionYField, currentMotionY * 0.9 + 0.05);
            }
            else env->ExceptionClear();

            if (BridgeHelper::EntityBridge_GetMotionX && BridgeHelper::EntityBridge_GetMotionZ)
            {
                double mx = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionX);
                double mz = env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionZ);
                double currentHoriz = std::sqrt(mx * mx + mz * mz);
                double maxSpeed = speed * 0.28;
                if (currentHoriz < maxSpeed)
                {
                    float yaw = (float)env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetRotationYaw);
                    float forward = (GetAsyncKeyState('W') & 0x8000) ? 1.0f : ((GetAsyncKeyState('S') & 0x8000) ? -1.0f : 0.0f);
                    float strafe = (GetAsyncKeyState('D') & 0x8000) ? 1.0f : ((GetAsyncKeyState('A') & 0x8000) ? -1.0f : 0.0f);
                    if (forward != 0.0f || strafe != 0.0f)
                    {
                        float rad = yaw * 0.017453292f;
                        double sin = std::sin(rad);
                        double cos = std::cos(rad);
                        double targetX = (double)(-forward) * sin + (double)(strafe) * cos;
                        double targetZ = (double)(forward) * cos - (double)(strafe) * sin;
                        double len = std::sqrt(targetX * targetX + targetZ * targetZ);
                        if (len > 0.0)
                        {
                            targetX /= len; targetZ /= len;
                            if (BridgeHelper::EntityBridge_SetMotionX) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionX, targetX * maxSpeed);
                            if (BridgeHelper::EntityBridge_SetMotionZ) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionZ, targetZ * maxSpeed);
                        }
                    }
                }
            }
            env->DeleteLocalRef(entityClass);
        }
    }

    env->DeleteLocalRef(player);
}

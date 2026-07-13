#include "liquidwalk.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <cmath>
#include <Windows.h>

LiquidWalkModule::LiquidWalkModule()
    : ModuleBase("LiquidWalk", "Walk on water and lava", Category::Movement)
{
    AddSetting<NumberSetting>("Speed", 0.3f, 0.1f, 1.0f, 0.1f, "Movement speed on liquids");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void LiquidWalkModule::OnEnable() { Logger::Log("LiquidWalk enabled"); }
void LiquidWalkModule::OnDisable() { Logger::Log("LiquidWalk disabled"); }

void LiquidWalkModule::OnUpdate()
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

    bool bridged = BridgeHelper::Initialize(env);

    // isInWater and isSneaking via bridge or notch; isInLava via notch only
    bool inWater = false, inLava = false, sneaking = false;

    jobject player = bridged ? BridgeHelper::GetPlayer(env) : nullptr;
    if (!player)
    {
        // Notch path (full vanilla)
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

        jmethodID isInWaterMeth = env->GetMethodID(entityClass, "H", "()Z");
        if (!isInWaterMeth) { env->ExceptionClear(); isInWaterMeth = env->GetMethodID(entityClass, "isInWater", "()Z"); }
        env->ExceptionClear();
        jmethodID isInLavaMeth = env->GetMethodID(entityClass, "I", "()Z");
        if (!isInLavaMeth) { env->ExceptionClear(); isInLavaMeth = env->GetMethodID(entityClass, "isInLava", "()Z"); }
        env->ExceptionClear();
        jmethodID isSneakingMeth = env->GetMethodID(entityClass, "n", "()Z");
        if (!isSneakingMeth) { env->ExceptionClear(); isSneakingMeth = env->GetMethodID(entityClass, "isSneaking", "()Z"); }
        env->ExceptionClear();

        inWater = isInWaterMeth ? env->CallBooleanMethod(player, isInWaterMeth) : false;
        inLava = isInLavaMeth ? env->CallBooleanMethod(player, isInLavaMeth) : false;
        sneaking = isSneakingMeth ? env->CallBooleanMethod(player, isSneakingMeth) : false;

        if ((!inWater && !inLava) || sneaking)
        {
            env->DeleteLocalRef(entityClass);
            env->DeleteLocalRef(player);
            env->DeleteLocalRef(mc);
            env->DeleteLocalRef(mcClass);
            return;
        }

        jfieldID motionXField = env->GetFieldID(entityClass, "v", "D");
        if (!motionXField) { env->ExceptionClear(); motionXField = env->GetFieldID(entityClass, "motionX", "D"); }
        env->ExceptionClear();
        jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
        if (!motionYField) { env->ExceptionClear(); motionYField = env->GetFieldID(entityClass, "motionY", "D"); }
        env->ExceptionClear();
        jfieldID motionZField = env->GetFieldID(entityClass, "x", "D");
        if (!motionZField) { env->ExceptionClear(); motionZField = env->GetFieldID(entityClass, "motionZ", "D"); }
        env->ExceptionClear();
        jfieldID onGroundField = env->GetFieldID(entityClass, "C", "Z");
        if (!onGroundField) { env->ExceptionClear(); onGroundField = env->GetFieldID(entityClass, "onGround", "Z"); }
        env->ExceptionClear();
        jfieldID yawField = env->GetFieldID(entityClass, "y", "F");
        if (!yawField) { env->ExceptionClear(); yawField = env->GetFieldID(entityClass, "rotationYaw", "F"); }
        env->ExceptionClear();

        if (!motionXField || !motionYField || !motionZField || !onGroundField)
        {
            env->DeleteLocalRef(entityClass);
            env->DeleteLocalRef(player);
            env->DeleteLocalRef(mc);
            env->DeleteLocalRef(mcClass);
            return;
        }

        bool forward = (GetAsyncKeyState('W') & 0x8000) != 0;
        bool backward = (GetAsyncKeyState('S') & 0x8000) != 0;
        bool left = (GetAsyncKeyState('A') & 0x8000) != 0;
        bool right = (GetAsyncKeyState('D') & 0x8000) != 0;
        bool jump = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

        int forwardV = (forward ? 1 : 0) - (backward ? 1 : 0);
        int strafeV = (right ? 1 : 0) - (left ? 1 : 0);

        if (forwardV != 0 || strafeV != 0)
        {
            float yaw = yawField ? env->GetFloatField(player, yawField) : 0.0f;
            float rad = yaw * 0.017453292f;
            double sin = std::sin(rad);
            double cos = std::cos(rad);
            double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
            double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
            double len = std::sqrt(targetX * targetX + targetZ * targetZ);
            if (len > 0.0)
            {
                targetX /= len; targetZ /= len;
                env->SetDoubleField(player, motionXField, targetX * speed);
                env->SetDoubleField(player, motionZField, targetZ * speed);
            }
        }

        if (jump)
            env->SetDoubleField(player, motionYField, speed);

        env->SetBooleanField(player, onGroundField, JNI_TRUE);

        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    // --- Bridged path ---
    inWater = BridgeHelper::ELBridge_IsInWater
        ? env->CallBooleanMethod(player, BridgeHelper::ELBridge_IsInWater) != 0 : false;
    // isInLava has no bridge — use notch
    if (!inWater)
    {
        jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
        if (entityClass)
        {
            jmethodID isInLavaMeth = env->GetMethodID(entityClass, "isInLava", "()Z");
            if (isInLavaMeth) { env->ExceptionClear(); inLava = env->CallBooleanMethod(player, isInLavaMeth); }
            else env->ExceptionClear();
            env->DeleteLocalRef(entityClass);
        }
    }
    sneaking = BridgeHelper::EntityBridge_IsSneaking
        ? env->CallBooleanMethod(player, BridgeHelper::EntityBridge_IsSneaking) != 0 : false;

    if ((!inWater && !inLava) || sneaking)
    {
        env->DeleteLocalRef(player);
        return;
    }

    float yaw = (float)env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetRotationYaw);
    bool forward = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool backward = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool left = (GetAsyncKeyState('A') & 0x8000) != 0;
    bool right = (GetAsyncKeyState('D') & 0x8000) != 0;
    bool jump = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    int forwardV = (forward ? 1 : 0) - (backward ? 1 : 0);
    int strafeV = (right ? 1 : 0) - (left ? 1 : 0);

    if (forwardV != 0 || strafeV != 0)
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
            if (BridgeHelper::EntityBridge_SetMotionX) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionX, targetX * speed);
            if (BridgeHelper::EntityBridge_SetMotionZ) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionZ, targetZ * speed);
        }
    }

    if (jump && BridgeHelper::EntityBridge_SetMotionY)
        env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionY, speed);

    // Set onGround via notch (no bridge setter)
    jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
    if (entityClass)
    {
        jfieldID onGroundField = env->GetFieldID(entityClass, "onGround", "Z");
        if (onGroundField) { env->ExceptionClear(); env->SetBooleanField(player, onGroundField, JNI_TRUE); }
        else env->ExceptionClear();
        env->DeleteLocalRef(entityClass);
    }

    env->DeleteLocalRef(player);
}

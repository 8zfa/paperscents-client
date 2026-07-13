#include "speed.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <cmath>
#include <Windows.h>

SpeedModule::SpeedModule()
    : ModuleBase("Speed", "Run faster", Category::Movement)
{
    AddSetting<NumberSetting>("Speed", 1.5f, 1.0f, 5.0f, 0.1f);
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"BHop", "Strafe", "Ground"});
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void SpeedModule::OnEnable() { Logger::Log("Speed enabled"); }
void SpeedModule::OnDisable() { Logger::Log("Speed disabled"); }

static bool GetKeyPressed(JNIEnv* env, jclass gsClass, jobject gameSettings, const char* field, const char* sig)
{
    jfieldID kf = env->GetFieldID(gsClass, field, sig);
    if (!kf) { env->ExceptionClear(); return false; }
    jobject keyBind = env->GetObjectField(gameSettings, kf);
    if (!keyBind) return false;
    jclass keyClass = env->GetObjectClass(keyBind);
    jfieldID pressedField = env->GetFieldID(keyClass, "e", "Z");
    if (!pressedField) { env->ExceptionClear(); pressedField = env->GetFieldID(keyClass, "pressed", "Z"); }
    bool pressed = false;
    if (pressedField) { env->ExceptionClear(); pressed = env->GetBooleanField(keyBind, pressedField); }
    env->DeleteLocalRef(keyClass);
    env->DeleteLocalRef(keyBind);
    return pressed;
}

void SpeedModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    bool bridged = BridgeHelper::Initialize(env);

    jobject player = bridged ? BridgeHelper::GetPlayer(env) : nullptr;
    if (!player)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java || !java->IsValid()) return;
        env->ExceptionClear();

        jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
        if (!mcClass) return;
        jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
        if (!getMc) { env->DeleteLocalRef(mcClass); return; }
        jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
        if (!mc) { env->DeleteLocalRef(mcClass); return; }

        const char* playerFields[] = { "s", "t", "h", "thePlayer" };
        jfieldID pf = nullptr;
        for (auto f : playerFields)
        {
            pf = env->GetFieldID(mcClass, f, "Lbew;");
            if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
            if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
        }
        if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
        // Continue with notch code below (will use player, mc, mcClass)
    }

    float speed = ((NumberSetting*)FindSetting("Speed"))->GetValue();
    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();

    bool onGround = bridged && BridgeHelper::EntityBridge_IsOnGround
        ? env->CallBooleanMethod(player, BridgeHelper::EntityBridge_IsOnGround) != 0 : false;
    double motionX = bridged && BridgeHelper::EntityBridge_GetMotionX
        ? env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionX) : 0;
    double motionZ = bridged && BridgeHelper::EntityBridge_GetMotionZ
        ? env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetMotionZ) : 0;
    float yaw = bridged && BridgeHelper::EntityBridge_GetRotationYaw
        ? (float)env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetRotationYaw) : 0;

    // If not bridged, read fields via notch
    if (!bridged)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java || !java->IsValid()) { env->DeleteLocalRef(player); return; }

        jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
        if (!mcClass) { env->DeleteLocalRef(player); return; }
        jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
        if (!getMc) { env->DeleteLocalRef(mcClass); env->DeleteLocalRef(player); return; }
        jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
        if (!mc) { env->DeleteLocalRef(mcClass); env->DeleteLocalRef(player); return; }

        jclass entityClass = java->FindClass("vg", "net/minecraft/entity/Entity");
        if (!entityClass) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); env->DeleteLocalRef(player); return; }

        jfieldID motionXField = env->GetFieldID(entityClass, "v", "D");
        if (!motionXField) { env->ExceptionClear(); motionXField = env->GetFieldID(entityClass, "motionX", "D"); }
        jfieldID motionZField = env->GetFieldID(entityClass, "x", "D");
        if (!motionZField) { env->ExceptionClear(); motionZField = env->GetFieldID(entityClass, "motionZ", "D"); }
        jfieldID onGroundField = env->GetFieldID(entityClass, "C", "Z");
        if (!onGroundField) { env->ExceptionClear(); onGroundField = env->GetFieldID(entityClass, "onGround", "Z"); }
        jfieldID yawField = env->GetFieldID(entityClass, "y", "F");
        if (!yawField) { env->ExceptionClear(); yawField = env->GetFieldID(entityClass, "rotationYaw", "F"); }

        if (!motionXField || !motionZField || !onGroundField)
        {
            env->DeleteLocalRef(entityClass);
            env->DeleteLocalRef(mc);
            env->DeleteLocalRef(mcClass);
            env->DeleteLocalRef(player);
            return;
        }

        onGround = env->GetBooleanField(player, onGroundField);
        motionX = env->GetDoubleField(player, motionXField);
        motionZ = env->GetDoubleField(player, motionZField);
        if (yawField) yaw = env->GetFloatField(player, yawField);

        // Read movement keys via notch
        jclass gsClass = java->FindClass("bdf", "net/minecraft/client/settings/GameSettings");
        if (!gsClass) gsClass = env->FindClass("net/minecraft/client/settings/GameSettings");

        const char* gsFields[] = { "t", "u", "gameSettings" };
        jobject gameSettings = nullptr;
        jfieldID gsf = nullptr;
        for (auto f : gsFields)
        {
            gsf = env->GetFieldID(mcClass, f, "Lbdf;");
            if (!gsf) gsf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/settings/GameSettings;");
            if (gsf) { gameSettings = env->GetObjectField(mc, gsf); if (gameSettings) break; }
        }

        bool forward = false, back = false, left = false, right = false, jump = false;
        if (gameSettings && gsClass)
        {
            forward = GetKeyPressed(env, gsClass, gameSettings, "c", "Lbca;") || GetKeyPressed(env, gsClass, gameSettings, "keyBindForward", "Lnet/minecraft/client/settings/KeyBinding;");
            back = GetKeyPressed(env, gsClass, gameSettings, "e", "Lbca;") || GetKeyPressed(env, gsClass, gameSettings, "keyBindBack", "Lnet/minecraft/client/settings/KeyBinding;");
            left = GetKeyPressed(env, gsClass, gameSettings, "d", "Lbca;") || GetKeyPressed(env, gsClass, gameSettings, "keyBindLeft", "Lnet/minecraft/client/settings/KeyBinding;");
            right = GetKeyPressed(env, gsClass, gameSettings, "f", "Lbca;") || GetKeyPressed(env, gsClass, gameSettings, "keyBindRight", "Lnet/minecraft/client/settings/KeyBinding;");
            jump = GetKeyPressed(env, gsClass, gameSettings, "g", "Lbca;") || GetKeyPressed(env, gsClass, gameSettings, "keyBindJump", "Lnet/minecraft/client/settings/KeyBinding;");
        }

        int forwardV = (forward ? 1 : 0) - (back ? 1 : 0);
        int strafeV = (right ? 1 : 0) - (left ? 1 : 0);

        if (forwardV == 0 && strafeV == 0)
        {
            if (gameSettings) env->DeleteLocalRef(gameSettings);
            if (gsClass) env->DeleteLocalRef(gsClass);
            env->DeleteLocalRef(entityClass);
            env->DeleteLocalRef(mc);
            env->DeleteLocalRef(mcClass);
            env->DeleteLocalRef(player);
            return;
        }

        switch (mode)
        {
        case 0:
        {
            if (onGround)
            {
                double currentSpeed = std::sqrt(motionX * motionX + motionZ * motionZ);
                double maxSpeed = speed * 0.28;
                double boost = 1.35;
                if (currentSpeed < maxSpeed)
                {
                    double newSpeed = currentSpeed > 0.0 ? maxSpeed * boost : maxSpeed;
                    float rad = yaw * 0.017453292f;
                    double sin = std::sin(rad);
                    double cos = std::cos(rad);
                    double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
                    double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
                    if (targetX != 0.0 || targetZ != 0.0)
                    {
                        double len = std::sqrt(targetX * targetX + targetZ * targetZ);
                        targetX /= len; targetZ /= len;
                        env->SetDoubleField(player, motionXField, targetX * newSpeed);
                        env->SetDoubleField(player, motionZField, targetZ * newSpeed);
                    }
                }
                if (jump)
                {
                    jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
                    if (!motionYField) { env->ExceptionClear(); motionYField = env->GetFieldID(entityClass, "motionY", "D"); }
                    if (motionYField) { env->ExceptionClear(); env->SetDoubleField(player, motionYField, 0.42); }
                }
            }
            break;
        }
        case 1:
        {
            float rad = yaw * 0.017453292f;
            double sin = std::sin(rad);
            double cos = std::cos(rad);
            double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
            double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
            if (targetX != 0.0 || targetZ != 0.0)
            {
                double len = std::sqrt(targetX * targetX + targetZ * targetZ);
                targetX /= len; targetZ /= len;
                env->SetDoubleField(player, motionXField, targetX * speed * 0.28);
                env->SetDoubleField(player, motionZField, targetZ * speed * 0.28);
            }
            break;
        }
        case 2:
        {
            if (onGround)
            {
                float rad = yaw * 0.017453292f;
                double sin = std::sin(rad);
                double cos = std::cos(rad);
                double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
                double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
                if (targetX != 0.0 || targetZ != 0.0)
                {
                    double len = std::sqrt(targetX * targetX + targetZ * targetZ);
                    targetX /= len; targetZ /= len;
                    env->SetDoubleField(player, motionXField, targetX * speed * 0.28);
                    env->SetDoubleField(player, motionZField, targetZ * speed * 0.28);
                }
            }
            break;
        }
        }

        if (gameSettings) env->DeleteLocalRef(gameSettings);
        if (gsClass) env->DeleteLocalRef(gsClass);
        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        env->DeleteLocalRef(player);
        return;
    }

    // --- Bridged path ---
    // Read movement keys via Windows (works on both vanilla and Lunar)
    bool forward = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool back = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool left = (GetAsyncKeyState('A') & 0x8000) != 0;
    bool right = (GetAsyncKeyState('D') & 0x8000) != 0;
    bool jump = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    int forwardV = (forward ? 1 : 0) - (back ? 1 : 0);
    int strafeV = (right ? 1 : 0) - (left ? 1 : 0);

    if (forwardV == 0 && strafeV == 0) { env->DeleteLocalRef(player); return; }

    switch (mode)
    {
    case 0:
    {
        if (onGround)
        {
            double currentSpeed = std::sqrt(motionX * motionX + motionZ * motionZ);
            double maxSpeed = speed * 0.28;
            double boost = 1.35;
            if (currentSpeed < maxSpeed)
            {
                double newSpeed = currentSpeed > 0.0 ? maxSpeed * boost : maxSpeed;
                float rad = yaw * 0.017453292f;
                double sin = std::sin(rad);
                double cos = std::cos(rad);
                double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
                double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
                if (targetX != 0.0 || targetZ != 0.0)
                {
                    double len = std::sqrt(targetX * targetX + targetZ * targetZ);
                    targetX /= len; targetZ /= len;
                    if (BridgeHelper::EntityBridge_SetMotionX) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionX, targetX * newSpeed);
                    if (BridgeHelper::EntityBridge_SetMotionZ) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionZ, targetZ * newSpeed);
                }
            }
            if (jump && BridgeHelper::EntityBridge_SetMotionY)
                env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionY, 0.42);
        }
        break;
    }
    case 1:
    {
        float rad = yaw * 0.017453292f;
        double sin = std::sin(rad);
        double cos = std::cos(rad);
        double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
        double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
        if (targetX != 0.0 || targetZ != 0.0)
        {
            double len = std::sqrt(targetX * targetX + targetZ * targetZ);
            targetX /= len; targetZ /= len;
            if (BridgeHelper::EntityBridge_SetMotionX) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionX, targetX * speed * 0.28);
            if (BridgeHelper::EntityBridge_SetMotionZ) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionZ, targetZ * speed * 0.28);
        }
        break;
    }
    case 2:
    {
        if (onGround)
        {
            float rad = yaw * 0.017453292f;
            double sin = std::sin(rad);
            double cos = std::cos(rad);
            double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
            double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
            if (targetX != 0.0 || targetZ != 0.0)
            {
                double len = std::sqrt(targetX * targetX + targetZ * targetZ);
                targetX /= len; targetZ /= len;
                if (BridgeHelper::EntityBridge_SetMotionX) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionX, targetX * speed * 0.28);
                if (BridgeHelper::EntityBridge_SetMotionZ) env->CallVoidMethod(player, BridgeHelper::EntityBridge_SetMotionZ, targetZ * speed * 0.28);
            }
        }
        break;
    }
    }

    env->DeleteLocalRef(player);
}

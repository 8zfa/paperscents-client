#include "fly.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <jni.h>

FlyModule::FlyModule()
    : ModuleBase("Fly", "Allows you to fly", Category::Movement)
{
    AddSetting<NumberSetting>("Speed", 1.0f, 0.1f, 5.0f, 0.1f);
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Vanilla", "Creative", "Slide"});
}

void FlyModule::OnEnable() { Logger::Log("Fly enabled"); }
void FlyModule::OnDisable() { Logger::Log("Fly disabled"); }

void FlyModule::OnUpdate()
{
    if (!IsEnabled()) return;
    Java* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return;
    JNIEnv* env = java->GetEnv();
    if (!env) return;

    float speed = ((NumberSetting*)FindSetting("Speed"))->GetValue();
    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();

    jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) return;
    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    const char* playerFields[] = { "s", "t", "h", "thePlayer" };
    jobject player = nullptr;
    jfieldID pf = nullptr;
    for (auto f : playerFields)
    {
        pf = env->GetFieldID(mcClass, f, "Lbew;");
        if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
        if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
    }
    if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass entityClass = java->FindClass("vg", "net/minecraft/entity/Entity");
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    // Get PlayerControllerMP
    const char* pcFields[] = { "v", "field_71442_b", "playerController" };
    jobject pc = nullptr;
    jfieldID pcf = nullptr;
    for (auto f : pcFields)
    {
        pcf = env->GetFieldID(mcClass, f, "Lbhl;");
        if (!pcf) pcf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/multiplayer/PlayerControllerMP;");
        if (pcf) { pc = env->GetObjectField(mc, pcf); if (pc) break; }
    }

    if (mode == 0) // Vanilla
    {
        // Get capabilities
                jclass capabilitiesClass = java->FindClass("vj", "net/minecraft/entity/player/PlayerCapabilities");
        if (!capabilitiesClass) capabilitiesClass = env->FindClass("net/minecraft/entity/player/PlayerCapabilities");

        const char* capFields[] = { "a", "field_71075_bZ", "capabilities" };
        jobject capabilities = nullptr;
        jfieldID cf = nullptr;
        for (auto f : capFields)
        {
            cf = env->GetFieldID(entityClass, f, "Lvj;");
            if (!cf) cf = env->GetFieldID(entityClass, f, "Lnet/minecraft/entity/player/PlayerCapabilities;");
            if (cf) { capabilities = env->GetObjectField(player, cf); if (capabilities) break; }
        }

        if (capabilities)
        {
            // Set isFlying = true
            jfieldID isFlyingField = env->GetFieldID(capabilitiesClass, "d", "Z");
            if (!isFlyingField) isFlyingField = env->GetFieldID(capabilitiesClass, "isFlying", "Z");
            if (!isFlyingField) isFlyingField = env->GetFieldID(capabilitiesClass, "field_75100_b", "Z");
            if (isFlyingField)
                env->SetBooleanField(capabilities, isFlyingField, JNI_TRUE);

            // Set fly speed
            jfieldID flySpeedField = env->GetFieldID(capabilitiesClass, "e", "F");
            if (!flySpeedField) flySpeedField = env->GetFieldID(capabilitiesClass, "flySpeed", "F");
            if (!flySpeedField) flySpeedField = env->GetFieldID(capabilitiesClass, "field_75098_d", "F");
            if (flySpeedField)
                env->SetFloatField(capabilities, flySpeedField, speed * 0.05f);

            env->DeleteLocalRef(capabilities);
        }
        env->DeleteLocalRef(capabilitiesClass);
    }
    else if (mode == 1) // Creative
    {
        // Enable creative fly via PlayerControllerMP
        if (pc)
        {
            jclass pcClass = env->GetObjectClass(pc);

            // Try calling setGameType or setFlyToggle
            jmethodID setFlyMethod = nullptr;
            const char* flyMethods[] = { "a", "b", "c", "enableFlight", "setFlying" };
            for (auto m : flyMethods)
            {
                setFlyMethod = env->GetMethodID(pcClass, m, "(Z)V");
                if (setFlyMethod) break;
                env->ExceptionClear();
            }

            // If no direct method, modify capabilities directly
            if (!setFlyMethod)
            {
        jclass capabilitiesClass = java->FindClass("vj", "net/minecraft/entity/player/PlayerCapabilities");
                if (!capabilitiesClass) capabilitiesClass = env->FindClass("net/minecraft/entity/player/PlayerCapabilities");

                const char* capFields[] = { "a", "field_71075_bZ", "capabilities" };
                jobject capabilities = nullptr;
                jfieldID cf = nullptr;
                for (auto f : capFields)
                {
                    cf = env->GetFieldID(entityClass, f, "Lvj;");
                    if (!cf) cf = env->GetFieldID(entityClass, f, "Lnet/minecraft/entity/player/PlayerCapabilities;");
                    if (cf) { capabilities = env->GetObjectField(player, cf); if (capabilities) break; }
                }

                if (capabilities)
                {
                    jfieldID allowFlyingField = env->GetFieldID(capabilitiesClass, "c", "Z");
                    if (!allowFlyingField) allowFlyingField = env->GetFieldID(capabilitiesClass, "allowFlying", "Z");
                    if (!allowFlyingField) allowFlyingField = env->GetFieldID(capabilitiesClass, "field_75101_c", "Z");
                    if (allowFlyingField)
                        env->SetBooleanField(capabilities, allowFlyingField, JNI_TRUE);

                    jfieldID isFlyingField = env->GetFieldID(capabilitiesClass, "d", "Z");
                    if (!isFlyingField) isFlyingField = env->GetFieldID(capabilitiesClass, "isFlying", "Z");
                    if (!isFlyingField) isFlyingField = env->GetFieldID(capabilitiesClass, "field_75100_b", "Z");
                    if (isFlyingField)
                        env->SetBooleanField(capabilities, isFlyingField, JNI_TRUE);

                    jfieldID flySpeedField = env->GetFieldID(capabilitiesClass, "e", "F");
                    if (!flySpeedField) flySpeedField = env->GetFieldID(capabilitiesClass, "flySpeed", "F");
                    if (!flySpeedField) flySpeedField = env->GetFieldID(capabilitiesClass, "field_75098_d", "F");
                    if (flySpeedField)
                        env->SetFloatField(capabilities, flySpeedField, speed * 0.05f);

                    env->DeleteLocalRef(capabilities);
                }
                env->DeleteLocalRef(capabilitiesClass);
            }

            env->DeleteLocalRef(pcClass);
        }
    }
    else if (mode == 2) // Slide
    {
        // Set noClip and modify motionY
        jfieldID noClipField = env->GetFieldID(entityClass, "j", "Z");
        if (!noClipField) noClipField = env->GetFieldID(entityClass, "noClip", "Z");
        if (noClipField)
            env->SetBooleanField(player, noClipField, JNI_TRUE);

        jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
        if (!motionYField) motionYField = env->GetFieldID(entityClass, "motionY", "D");
        if (motionYField)
        {
            double currentMotionY = env->GetDoubleField(player, motionYField);
            env->SetDoubleField(player, motionYField, currentMotionY * 0.9 + 0.05);
        }

        // Apply horizontal speed
        jfieldID motionXField = env->GetFieldID(entityClass, "v", "D");
        if (!motionXField) motionXField = env->GetFieldID(entityClass, "motionX", "D");
        jfieldID motionZField = env->GetFieldID(entityClass, "x", "D");
        if (!motionZField) motionZField = env->GetFieldID(entityClass, "motionZ", "D");

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
                    if (!moveForwardField) moveForwardField = env->GetFieldID(livingClass, "moveForward", "F");
                    jfieldID moveStrafingField = env->GetFieldID(livingClass, "bc", "F");
                    if (!moveStrafingField) moveStrafingField = env->GetFieldID(livingClass, "moveStrafing", "F");

                    if (moveForwardField && moveStrafingField)
                    {
                        float forward = env->GetFloatField(player, moveForwardField);
                        float strafe = env->GetFloatField(player, moveStrafingField);

                        if (forward != 0.0f || strafe != 0.0f)
                        {
                            jfieldID rotYawField = env->GetFieldID(entityClass, "y", "F");
                            if (!rotYawField) rotYawField = env->GetFieldID(entityClass, "rotationYaw", "F");
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
                                    targetX /= len;
                                    targetZ /= len;
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

    if (pc) env->DeleteLocalRef(pc);

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

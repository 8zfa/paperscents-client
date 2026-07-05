#include "invmove.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <jni.h>
#include <cmath>

InvMoveModule::InvMoveModule()
    : ModuleBase("InvMove", "Move while in inventory", Category::Movement)
{
    AddSetting<BooleanSetting>("Sprint", true, "Allow sprinting in inventory");
    AddSetting<BooleanSetting>("Sneak", true, "Allow sneaking in inventory");
}

void InvMoveModule::OnEnable() { Logger::Log("InvMove enabled"); }
void InvMoveModule::OnDisable() { Logger::Log("InvMove disabled"); }

void InvMoveModule::OnUpdate()
{
    if (!IsEnabled()) return;
    Java* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return;
    JNIEnv* env = java->GetEnv();
    if (!env) return;

    bool sprint = ((BooleanSetting*)FindSetting("Sprint"))->GetValue();
    bool sneak = ((BooleanSetting*)FindSetting("Sneak"))->GetValue();

    jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) return;
    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    // Check if a screen is open
    jfieldID screenField = env->GetFieldID(mcClass, "m", "Lbdz;");
    if (!screenField) screenField = env->GetFieldID(mcClass, "currentScreen", "Lnet/minecraft/client/gui/GuiScreen;");
    if (!screenField) screenField = env->GetFieldID(mcClass, "m", "Ljava/lang/Object;");

    jobject currentScreen = nullptr;
    if (screenField)
        currentScreen = env->GetObjectField(mc, screenField);

    if (!currentScreen)
    {
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return; // No screen open, nothing to do
    }

    // Screen is open - process movement keys regardless
    const char* playerFields[] = { "s", "t", "h", "thePlayer" };
    jobject player = nullptr;
    jfieldID pf = nullptr;
    for (auto f : playerFields)
    {
        pf = env->GetFieldID(mcClass, f, "Lbew;");
        if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
        if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
    }
    if (!player)
    {
        env->DeleteLocalRef(currentScreen);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    jclass entityClass = java->FindClass("vg", "net/minecraft/entity/Entity");
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(currentScreen); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    // Get game settings
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
    if (!gameSettings)
    {
        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(currentScreen);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    jclass keyClass = java->FindClass("bca", "net/minecraft/client/settings/KeyBinding");
    if (!keyClass) keyClass = env->FindClass("net/minecraft/client/settings/KeyBinding");

    auto isKeyPressed = [&](const char* gsFieldName, const char* gsFieldSig) -> bool
    {
        jfieldID kf = env->GetFieldID(gsClass, gsFieldName, gsFieldSig);
        if (!kf) return false;
        jobject keyBind = env->GetObjectField(gameSettings, kf);
        if (!keyBind) return false;
        jfieldID pressedField = env->GetFieldID(keyClass, "e", "Z");
        if (!pressedField) pressedField = env->GetFieldID(keyClass, "pressed", "Z");
        if (!pressedField) { env->DeleteLocalRef(keyBind); return false; }
        bool pressed = env->GetBooleanField(keyBind, pressedField);
        env->DeleteLocalRef(keyBind);
        return pressed;
    };

    bool forward = isKeyPressed("c", "Lbca;") || isKeyPressed("keyBindForward", "Lnet/minecraft/client/settings/KeyBinding;");
    bool back = isKeyPressed("e", "Lbca;") || isKeyPressed("keyBindBack", "Lnet/minecraft/client/settings/KeyBinding;");
    bool left = isKeyPressed("d", "Lbca;") || isKeyPressed("keyBindLeft", "Lnet/minecraft/client/settings/KeyBinding;");
    bool right = isKeyPressed("f", "Lbca;") || isKeyPressed("keyBindRight", "Lnet/minecraft/client/settings/KeyBinding;");
    bool jump = isKeyPressed("g", "Lbca;") || isKeyPressed("keyBindJump", "Lnet/minecraft/client/settings/KeyBinding;");

    int forwardV = (forward ? 1 : 0) - (back ? 1 : 0);
    int strafeV = (right ? 1 : 0) - (left ? 1 : 0);

    if (forwardV == 0 && strafeV == 0)
    {
        env->DeleteLocalRef(keyClass);
        env->DeleteLocalRef(gsClass);
        env->DeleteLocalRef(gameSettings);
        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(currentScreen);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    // Get motion fields
    jfieldID motionXField = env->GetFieldID(entityClass, "v", "D");
    if (!motionXField) motionXField = env->GetFieldID(entityClass, "motionX", "D");
    jfieldID motionZField = env->GetFieldID(entityClass, "x", "D");
    if (!motionZField) motionZField = env->GetFieldID(entityClass, "motionZ", "D");
    jfieldID onGroundField = env->GetFieldID(entityClass, "C", "Z");
    if (!onGroundField) onGroundField = env->GetFieldID(entityClass, "onGround", "Z");

    if (motionXField && motionZField && onGroundField)
    {
        bool onGround = env->GetBooleanField(player, onGroundField);
        jfieldID rotYawField = env->GetFieldID(entityClass, "y", "F");
        if (!rotYawField) rotYawField = env->GetFieldID(entityClass, "rotationYaw", "F");

        if (rotYawField)
        {
            float yaw = env->GetFloatField(player, rotYawField);
            float rad = yaw * 0.017453292f;
            double sin = std::sin(rad);
            double cos = std::cos(rad);
            double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
            double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
            double len = std::sqrt(targetX * targetX + targetZ * targetZ);

            if (len > 0.0)
            {
                targetX /= len;
                targetZ /= len;
                double speed = onGround ? 0.28 : 0.15;

                env->SetDoubleField(player, motionXField, targetX * speed);
                env->SetDoubleField(player, motionZField, targetZ * speed);

                // Handle jumping
                if (jump && onGround)
                {
                    jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
                    if (!motionYField) motionYField = env->GetFieldID(entityClass, "motionY", "D");
                    if (motionYField)
                        env->SetDoubleField(player, motionYField, 0.42);
                }

                // Handle sprinting
                if (sprint && forward)
                {
                    jmethodID setSprinting = nullptr;
                    const char* setSprintMethods[] = { "b", "c", "a", "setSprinting", "d" };
                    for (auto m : setSprintMethods)
                    {
                        setSprinting = env->GetMethodID(entityClass, m, "(Z)V");
                        if (setSprinting) break;
                        env->ExceptionClear();
                    }
                    if (setSprinting)
                        env->CallVoidMethod(player, setSprinting, JNI_TRUE);
                }

                // Handle sneaking
                if (sneak)
                {
                    jmethodID setSneaking = nullptr;
                    const char* setSneakMethods[] = { "a", "n", "setSneaking", "b" };
                    for (auto m : setSneakMethods)
                    {
                        setSneaking = env->GetMethodID(entityClass, m, "(Z)V");
                        if (setSneaking) break;
                        env->ExceptionClear();
                    }
                    if (setSneaking)
                        env->CallVoidMethod(player, setSneaking, sneak);
                }
            }
        }
    }

    env->DeleteLocalRef(keyClass);
    env->DeleteLocalRef(gsClass);
    env->DeleteLocalRef(gameSettings);
    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(currentScreen);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

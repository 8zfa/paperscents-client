#include "speed.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <jni.h>
#include <cmath>

SpeedModule::SpeedModule()
    : ModuleBase("Speed", "Run faster", Category::Movement)
{
    AddSetting<NumberSetting>("Speed", 1.5f, 1.0f, 5.0f, 0.1f);
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"BHop", "Strafe", "Ground"});
}

void SpeedModule::OnEnable() { Logger::Log("Speed enabled"); }
void SpeedModule::OnDisable() { Logger::Log("Speed disabled"); }

void SpeedModule::OnUpdate()
{
    if (!IsEnabled()) return;
    Java* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return;
    JNIEnv* env = java->GetEnv();
    if (!env) return;
    env->ExceptionClear();

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

    jclass gameSettingsClass = java->FindClass("bdf", "net/minecraft/client/settings/GameSettings");
    if (!gameSettingsClass) gameSettingsClass = env->FindClass("net/minecraft/client/settings/GameSettings");
    if (!gameSettingsClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    const char* gsFields[] = { "t", "u", "gameSettings" };
    jobject gameSettings = nullptr;
    jfieldID gsf = nullptr;
    for (auto f : gsFields)
    {
        gsf = env->GetFieldID(mcClass, f, "Lbdf;");
        if (!gsf) gsf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/settings/GameSettings;");
        if (gsf) { gameSettings = env->GetObjectField(mc, gsf); if (gameSettings) break; }
    }
    if (!gameSettings) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass entityClass = java->FindClass("vg", "net/minecraft/entity/Entity");
    if (!entityClass) { env->DeleteLocalRef(gameSettings); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    // Get motion fields
    jfieldID motionXField = env->GetFieldID(entityClass, "v", "D");
    if (!motionXField) motionXField = env->GetFieldID(entityClass, "motionX", "D");
    jfieldID motionZField = env->GetFieldID(entityClass, "x", "D");
    if (!motionZField) motionZField = env->GetFieldID(entityClass, "motionZ", "D");
    jfieldID onGroundField = env->GetFieldID(entityClass, "C", "Z");
    if (!onGroundField) onGroundField = env->GetFieldID(entityClass, "onGround", "Z");

    if (!motionXField || !motionZField || !onGroundField)
    {
        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(gameSettings);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    bool onGround = env->GetBooleanField(player, onGroundField);
    double motionX = env->GetDoubleField(player, motionXField);
    double motionZ = env->GetDoubleField(player, motionZField);

    // Read movement keys
    jclass keyClass = java->FindClass("bca", "net/minecraft/client/settings/KeyBinding");
    if (!keyClass) keyClass = env->FindClass("net/minecraft/client/settings/KeyBinding");

    auto getKeyPressed = [&](const char* gsFieldName, const char* gsFieldSig) -> bool
    {
        jfieldID kf = env->GetFieldID(gameSettingsClass, gsFieldName, gsFieldSig);
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

    // Key binding field names: c=forward, e=back, d=left, f=right, g=jump
    bool forward = getKeyPressed("c", "Lbca;") || getKeyPressed("keyBindForward", "Lnet/minecraft/client/settings/KeyBinding;");
    bool back = getKeyPressed("e", "Lbca;") || getKeyPressed("keyBindBack", "Lnet/minecraft/client/settings/KeyBinding;");
    bool left = getKeyPressed("d", "Lbca;") || getKeyPressed("keyBindLeft", "Lnet/minecraft/client/settings/KeyBinding;");
    bool right = getKeyPressed("f", "Lbca;") || getKeyPressed("keyBindRight", "Lnet/minecraft/client/settings/KeyBinding;");
    bool jump = getKeyPressed("g", "Lbca;") || getKeyPressed("keyBindJump", "Lnet/minecraft/client/settings/KeyBinding;");

    int forwardV = (forward ? 1 : 0) - (back ? 1 : 0);
    int strafeV = (right ? 1 : 0) - (left ? 1 : 0);

    if (forwardV == 0 && strafeV == 0)
    {
        env->DeleteLocalRef(keyClass);
        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(gameSettings);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    // Get yaw for direction
    jfieldID rotationYawField = env->GetFieldID(entityClass, "y", "F");
    if (!rotationYawField) rotationYawField = env->GetFieldID(entityClass, "rotationYaw", "F");
    float yaw = 0.0f;
    if (rotationYawField)
        yaw = env->GetFloatField(player, rotationYawField);

    switch (mode)
    {
    case 0: // BHop
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
                    targetX /= len;
                    targetZ /= len;
                    env->SetDoubleField(player, motionXField, targetX * newSpeed);
                    env->SetDoubleField(player, motionZField, targetZ * newSpeed);
                }
            }

            if (jump)
            {
                jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
                if (!motionYField) motionYField = env->GetFieldID(entityClass, "motionY", "D");
                if (motionYField)
                    env->SetDoubleField(player, motionYField, 0.42);
            }
        }
        break;
    }
    case 1: // Strafe
    {
        float rad = yaw * 0.017453292f;
        double sin = std::sin(rad);
        double cos = std::cos(rad);
        double targetX = (double)(-forwardV) * sin + (double)(strafeV) * cos;
        double targetZ = (double)(forwardV) * cos - (double)(strafeV) * sin;
        if (targetX != 0.0 || targetZ != 0.0)
        {
            double len = std::sqrt(targetX * targetX + targetZ * targetZ);
            targetX /= len;
            targetZ /= len;
            double moveSpeed = speed * 0.28;
            env->SetDoubleField(player, motionXField, targetX * moveSpeed);
            env->SetDoubleField(player, motionZField, targetZ * moveSpeed);
        }
        break;
    }
    case 2: // Ground
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
                targetX /= len;
                targetZ /= len;
                double moveSpeed = speed * 0.28;
                env->SetDoubleField(player, motionXField, targetX * moveSpeed);
                env->SetDoubleField(player, motionZField, targetZ * moveSpeed);
            }
        }
        break;
    }
    }

    env->DeleteLocalRef(keyClass);
    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(gameSettings);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

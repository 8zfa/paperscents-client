#include "noslow.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <jni.h>
#include <cmath>

NoSlowModule::NoSlowModule()
    : ModuleBase("NoSlow", "Cancel slowdown from items and blocks", Category::Movement)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Vanilla", "NCP", "Custom"});
    AddSetting<NumberSetting>("SlowPct", 50.0f, 0.0f, 100.0f, 5.0f);
}

void NoSlowModule::OnEnable() { Logger::Log("NoSlow enabled"); }
void NoSlowModule::OnDisable() { Logger::Log("NoSlow disabled"); }

void NoSlowModule::OnUpdate()
{
    if (!IsEnabled()) return;
    Java* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return;
    JNIEnv* env = java->GetEnv();
    if (!env) return;

    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();
    float slowPct = ((NumberSetting*)FindSetting("SlowPct"))->GetValue();

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

    jclass livingClass = java->FindClass("sa", "net/minecraft/entity/EntityLivingBase");
    if (!livingClass) livingClass = env->FindClass("net/minecraft/entity/EntityLivingBase");
    if (!livingClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass entityClass = java->FindClass("vg", "net/minecraft/entity/Entity");
    if (!entityClass) entityClass = env->FindClass("net/minecraft/entity/Entity");
    if (!entityClass) { env->DeleteLocalRef(livingClass); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    // Check if player is using an item
    jmethodID isUsingItem = nullptr;
    const char* useMethods[] = { "e", "g", "h", "isUsingItem" };
    for (auto m : useMethods)
    {
        isUsingItem = env->GetMethodID(livingClass, m, "()Z");
        if (isUsingItem) break;
        env->ExceptionClear();
    }

    bool usingItem = false;
    if (isUsingItem)
        usingItem = env->CallBooleanMethod(player, isUsingItem);

    // Also check getItemInUse()
    if (!usingItem)
    {
        jmethodID getItemInUse = nullptr;
        const char* itemMethods[] = { "f", "i", "j", "getItemInUse" };
        for (auto m : itemMethods)
        {
            getItemInUse = env->GetMethodID(livingClass, m, "()Lzv;");
            if (!getItemInUse) getItemInUse = env->GetMethodID(livingClass, m, "()Lnet/minecraft/item/ItemStack;");
            if (getItemInUse) break;
            env->ExceptionClear();
        }
        if (getItemInUse)
        {
            jobject itemStack = env->CallObjectMethod(player, getItemInUse);
            if (itemStack)
            {
                usingItem = true;
                env->DeleteLocalRef(itemStack);
            }
            env->ExceptionClear();
        }
    }

    if (!usingItem)
    {
        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(livingClass);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    // Get motion fields
    jfieldID motionXField = env->GetFieldID(entityClass, "v", "D");
    if (!motionXField) motionXField = env->GetFieldID(entityClass, "motionX", "D");
    jfieldID motionZField = env->GetFieldID(entityClass, "x", "D");
    if (!motionZField) motionZField = env->GetFieldID(entityClass, "motionZ", "D");

    if (!motionXField || !motionZField)
    {
        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(livingClass);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    // Get moveForward/moveStrafing for direction
    jfieldID moveForwardField = env->GetFieldID(livingClass, "bb", "F");
    if (!moveForwardField) moveForwardField = env->GetFieldID(livingClass, "moveForward", "F");
    jfieldID moveStrafingField = env->GetFieldID(livingClass, "bc", "F");
    if (!moveStrafingField) moveStrafingField = env->GetFieldID(livingClass, "moveStrafing", "F");

    if (mode == 0) // Vanilla
    {
        // Cancel slowdown by resetting motion based on input
        if (moveForwardField && moveStrafingField)
        {
            float forward = env->GetFloatField(player, moveForwardField);
            float strafe = env->GetFloatField(player, moveStrafingField);

            jfieldID rotYawField = env->GetFieldID(entityClass, "y", "F");
            if (!rotYawField) rotYawField = env->GetFieldID(entityClass, "rotationYaw", "F");

            if (rotYawField && (forward != 0.0f || strafe != 0.0f))
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
                    double currentX = env->GetDoubleField(player, motionXField);
                    double currentZ = env->GetDoubleField(player, motionZField);
                    double currentSpeed = std::sqrt(currentX * currentX + currentZ * currentZ);
                    double normalSpeed = 0.28;
                    if (currentSpeed < normalSpeed)
                    {
                        env->SetDoubleField(player, motionXField, targetX * normalSpeed);
                        env->SetDoubleField(player, motionZField, targetZ * normalSpeed);
                    }
                }
            }
        }
    }
    else if (mode == 1) // NCP
    {
        // NCP bypass: multiply motion to counteract slowdown
        double mx = env->GetDoubleField(player, motionXField);
        double mz = env->GetDoubleField(player, motionZField);
        double currentSpeed = std::sqrt(mx * mx + mz * mz);

        if (currentSpeed > 0.0 && currentSpeed < 0.22)
        {
            double factor = 0.22 / currentSpeed;
            env->SetDoubleField(player, motionXField, mx * factor);
            env->SetDoubleField(player, motionZField, mz * factor);
        }
    }
    else if (mode == 2) // Custom
    {
        // Apply percentage-based slowdown reduction
        double mx = env->GetDoubleField(player, motionXField);
        double mz = env->GetDoubleField(player, motionZField);
        double currentSpeed = std::sqrt(mx * mx + mz * mz);

        double normalSpeed = 0.28 * (slowPct / 100.0);
        if (currentSpeed < normalSpeed)
        {
            if (moveForwardField && moveStrafingField)
            {
                float forward = env->GetFloatField(player, moveForwardField);
                float strafe = env->GetFloatField(player, moveStrafingField);

                jfieldID rotYawField = env->GetFieldID(entityClass, "y", "F");
                if (!rotYawField) rotYawField = env->GetFieldID(entityClass, "rotationYaw", "F");

                if (rotYawField && (forward != 0.0f || strafe != 0.0f))
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
                        env->SetDoubleField(player, motionXField, targetX * normalSpeed);
                        env->SetDoubleField(player, motionZField, targetZ * normalSpeed);
                    }
                }
            }
        }
    }

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(livingClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

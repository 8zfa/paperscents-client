#include "phase.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <cmath>
#include <Windows.h>

PhaseModule::PhaseModule()
    : ModuleBase("Phase", "Walk through blocks", Category::Movement)
{
    AddSetting<BooleanSetting>("Clip", false, "Teleport forward when sneaking against a wall");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void PhaseModule::OnEnable()
{
    Logger::Log("Phase enabled");
    m_NoClipRestore = false;
}

void PhaseModule::OnDisable()
{
    Logger::Log("Phase disabled");
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    env->ExceptionClear();

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
        if (entityClass)
        {
            jfieldID noClipField = env->GetFieldID(entityClass, "z", "Z");
            if (!noClipField) { env->ExceptionClear(); noClipField = env->GetFieldID(entityClass, "noClip", "Z"); }
            if (noClipField) { env->ExceptionClear(); env->SetBooleanField(player, noClipField, m_NoClipRestore ? JNI_TRUE : JNI_FALSE); }
            env->DeleteLocalRef(entityClass);
        }
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
    }
    else
    {
        jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
        if (entityClass)
        {
            jfieldID noClipField = env->GetFieldID(entityClass, "noClip", "Z");
            if (noClipField) { env->ExceptionClear(); env->SetBooleanField(player, noClipField, m_NoClipRestore ? JNI_TRUE : JNI_FALSE); }
            env->DeleteLocalRef(entityClass);
        }
    }

    if (player) env->DeleteLocalRef(player);
}

void PhaseModule::OnUpdate()
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

    bool bridged = BridgeHelper::Initialize(env);
    jobject player = bridged ? BridgeHelper::GetPlayer(env) : nullptr;
    jclass entityClass = nullptr;

    if (!player)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java || !java->IsValid()) return;
        entityClass = java->FindClass("vg", "net/minecraft/entity/Entity");
        if (!entityClass) return;
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

        jfieldID noClipField = env->GetFieldID(entityClass, "z", "Z");
        if (!noClipField) { env->ExceptionClear(); noClipField = env->GetFieldID(entityClass, "noClip", "Z"); }
        if (noClipField) { env->ExceptionClear(); env->SetBooleanField(player, noClipField, JNI_TRUE); }

        bool clip = ((BooleanSetting*)FindSetting("Clip"))->GetValue();
        if (clip)
        {
            jmethodID isSneaking = env->GetMethodID(entityClass, "n", "()Z");
            if (!isSneaking) { env->ExceptionClear(); isSneaking = env->GetMethodID(entityClass, "isSneaking", "()Z"); }
            bool sneaking = isSneaking ? env->CallBooleanMethod(player, isSneaking) : false;

            if (sneaking)
            {
                jfieldID collidedHorizField = env->GetFieldID(entityClass, "aa", "Z");
                if (!collidedHorizField) { env->ExceptionClear(); collidedHorizField = env->GetFieldID(entityClass, "isCollidedHorizontally", "Z"); }
                if (collidedHorizField && env->GetBooleanField(player, collidedHorizField))
                {
                    jfieldID posXField = env->GetFieldID(entityClass, "r", "D");
                    if (!posXField) { env->ExceptionClear(); posXField = env->GetFieldID(entityClass, "posX", "D"); }
                    jfieldID posZField = env->GetFieldID(entityClass, "t", "D");
                    if (!posZField) { env->ExceptionClear(); posZField = env->GetFieldID(entityClass, "posZ", "D"); }
                    jfieldID yawField = env->GetFieldID(entityClass, "y", "F");
                    if (!yawField) { env->ExceptionClear(); yawField = env->GetFieldID(entityClass, "rotationYaw", "F"); }

                    if (posXField && posZField && yawField)
                    {
                        float yaw = env->GetFloatField(player, yawField);
                        float rad = yaw * 0.017453292f;
                        double px = env->GetDoubleField(player, posXField);
                        double pz = env->GetDoubleField(player, posZField);
                        env->SetDoubleField(player, posXField, px + (-std::sin(rad)) * 0.5);
                        env->SetDoubleField(player, posZField, pz + std::cos(rad) * 0.5);
                    }
                }
            }
        }

        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        env->DeleteLocalRef(entityClass);
    }
    else
    {
        // Bridged path — noClip + posX/Z + isSneaking + isCollidedHorizontally via notch
        entityClass = env->FindClass("net/minecraft/entity/Entity");
        if (!entityClass) { env->DeleteLocalRef(player); return; }

        jfieldID noClipField = env->GetFieldID(entityClass, "noClip", "Z");
        if (noClipField) { env->ExceptionClear(); env->SetBooleanField(player, noClipField, JNI_TRUE); }
        else env->ExceptionClear();

        bool clip = ((BooleanSetting*)FindSetting("Clip"))->GetValue();
        if (clip)
        {
            bool sneaking = BridgeHelper::EntityBridge_IsSneaking
                ? env->CallBooleanMethod(player, BridgeHelper::EntityBridge_IsSneaking) != 0 : false;

            if (sneaking)
            {
                jfieldID collidedHorizField = env->GetFieldID(entityClass, "isCollidedHorizontally", "Z");
                if (collidedHorizField && env->GetBooleanField(player, collidedHorizField))
                {
                    jfieldID posXField = env->GetFieldID(entityClass, "posX", "D");
                    jfieldID posZField = env->GetFieldID(entityClass, "posZ", "D");
                    jfieldID yawField = env->GetFieldID(entityClass, "rotationYaw", "F");
                    if (posXField && posZField && yawField)
                    {
                        float yaw = env->GetFloatField(player, yawField);
                        float rad = yaw * 0.017453292f;
                        double px = env->GetDoubleField(player, posXField);
                        double pz = env->GetDoubleField(player, posZField);
                        env->SetDoubleField(player, posXField, px + (-std::sin(rad)) * 0.5);
                        env->SetDoubleField(player, posZField, pz + std::cos(rad) * 0.5);
                    }
                }
            }
        }

        env->DeleteLocalRef(entityClass);
    }

    if (player) env->DeleteLocalRef(player);
}

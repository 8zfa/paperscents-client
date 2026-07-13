#include "eagle.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../sdk/strayCache.h"
#include "../../../utilities/logger.h"
#include <cstdlib>

EagleModule::EagleModule()
    : ModuleBase("Eagle", "Auto-sneak on block edges", Category::Movement)
{
    AddSetting<NumberSetting>("MinDelay", 2.0f, 0.0f, 10.0f, 1.0f);
    AddSetting<NumberSetting>("MaxDelay", 3.0f, 0.0f, 10.0f, 1.0f);
    AddSetting<BooleanSetting>("DirectionCheck", true, "Don't sneak when pressing forward");
    AddSetting<BooleanSetting>("JumpCheck", true, "Don't sneak when jumping");
    AddSetting<BooleanSetting>("PitchCheck", true, "Don't sneak when looking down");
    AddSetting<BooleanSetting>("BlocksOnly", true, "Only sneak when holding a block");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void EagleModule::OnEnable() { Logger::Log("Eagle enabled"); m_SneakDelay = 0; }
void EagleModule::OnDisable() { Logger::Log("Eagle disabled"); m_SneakDelay = 0; }

static bool IsHoldingBlock(JNIEnv* env, jobject player)
{
    if (!player) return false;
    jclass elClass = env->FindClass("net/minecraft/entity/EntityLivingBase");
    if (!elClass) { env->ExceptionClear(); return false; }
    jmethodID getHeldItem = env->GetMethodID(elClass, "getHeldItem", "()Lnet/minecraft/item/ItemStack;");
    if (!getHeldItem) { env->ExceptionClear(); env->DeleteLocalRef(elClass); return false; }
    env->ExceptionClear();
    jobject stack = env->CallObjectMethod(player, getHeldItem);
    env->DeleteLocalRef(elClass);
    if (!stack) return false;
    jclass stackClass = env->GetObjectClass(stack);
    jmethodID getItem = env->GetMethodID(stackClass, "getItem", "()Lnet/minecraft/item/Item;");
    if (!getItem) { env->ExceptionClear(); env->DeleteLocalRef(stackClass); env->DeleteLocalRef(stack); return false; }
    env->ExceptionClear();
    jobject item = env->CallObjectMethod(stack, getItem);
    env->DeleteLocalRef(stackClass);
    if (!item) { env->DeleteLocalRef(stack); return false; }
    jclass itemBlockClass = env->FindClass("net/minecraft/item/ItemBlock");
    bool result = itemBlockClass ? (env->IsInstanceOf(item, itemBlockClass) == JNI_TRUE) : false;
    if (itemBlockClass) env->DeleteLocalRef(itemBlockClass);
    env->DeleteLocalRef(item);
    env->DeleteLocalRef(stack);
    return result;
}

bool EagleModule::ShouldSneak(JNIEnv* env, jobject player)
{
    if (!player) return false;
    bool dirCheck = ((BooleanSetting*)FindSetting("DirectionCheck"))->GetValue();
    bool jumpCheck = ((BooleanSetting*)FindSetting("JumpCheck"))->GetValue();
    bool pitchCheck = ((BooleanSetting*)FindSetting("PitchCheck"))->GetValue();
    bool blocksOnly = ((BooleanSetting*)FindSetting("BlocksOnly"))->GetValue();

    bool bridged = BridgeHelper::Initialize(env);

    // Use Windows keys for direction/jump check (works on both)
    if (dirCheck && (GetAsyncKeyState('W') & 0x8000))
        return false;

    if (jumpCheck && (GetAsyncKeyState(VK_SPACE) & 0x8000))
        return false;

    // Check pitch
    if (pitchCheck)
    {
        float pitch = 0;
        if (bridged && BridgeHelper::EntityBridge_GetRotationPitch)
            pitch = (float)env->CallDoubleMethod(player, BridgeHelper::EntityBridge_GetRotationPitch);
        else
        {
            jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
            if (entityClass)
            {
                jfieldID pitchField = env->GetFieldID(entityClass, "rotationPitch", "F");
                if (pitchField) { env->ExceptionClear(); pitch = env->GetFloatField(player, pitchField); }
                else env->ExceptionClear();
                env->DeleteLocalRef(entityClass);
            }
        }
        if (pitch < 69.0f) return false;
    }

    if (blocksOnly && !IsHoldingBlock(env, player))
        return false;

    // Check onGround
    if (bridged && BridgeHelper::EntityBridge_IsOnGround)
    {
        if (!env->CallBooleanMethod(player, BridgeHelper::EntityBridge_IsOnGround))
            return false;
    }
    else
    {
        jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
        if (entityClass)
        {
            jfieldID onGroundField = env->GetFieldID(entityClass, "onGround", "Z");
            if (onGroundField) { env->ExceptionClear(); if (!env->GetBooleanField(player, onGroundField)) { env->DeleteLocalRef(entityClass); return false; } }
            else env->ExceptionClear();
            env->DeleteLocalRef(entityClass);
        }
    }

    return true;
}

void EagleModule::OnUpdate()
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
    }

    if (m_SneakDelay > 0)
        m_SneakDelay--;

    if (m_SneakDelay == 0)
    {
        int minD = (int)((NumberSetting*)FindSetting("MinDelay"))->GetValue();
        int maxD = (int)((NumberSetting*)FindSetting("MaxDelay"))->GetValue();
        if (maxD < minD) maxD = minD;
        m_SneakDelay = minD + (maxD > minD ? rand() % (maxD - minD + 1) : 0);
    }

    // Access movementInput to set sneak (no bridge — notch fallback)
    jclass playerClass = env->FindClass("net/minecraft/entity/player/EntityPlayer");
    if (!playerClass)
    {
        Java* java = Core::GetInstance().GetJava();
        if (java) playerClass = java->FindClass("qh", "net/minecraft/entity/player/EntityPlayer");
        if (!playerClass) { env->DeleteLocalRef(player); return; }
    }

    jfieldID movementInputField = env->GetFieldID(playerClass, "movementInput", "Lnet/minecraft/util/MovementInput;");
    if (!movementInputField) { env->ExceptionClear(); env->DeleteLocalRef(playerClass); env->DeleteLocalRef(player); return; }
    env->ExceptionClear();
    jobject movementInput = env->GetObjectField(player, movementInputField);
    if (!movementInput) { env->DeleteLocalRef(playerClass); env->DeleteLocalRef(player); return; }

    jclass miClass = env->GetObjectClass(movementInput);
    jfieldID sneakField = env->GetFieldID(miClass, "sneak", "Z");

    if (sneakField)
    {
        bool sneak = env->GetBooleanField(movementInput, sneakField);
        bool should = ShouldSneak(env, player);
        if (!sneak && should)
        {
            env->ExceptionClear();
            env->SetBooleanField(movementInput, sneakField, JNI_TRUE);
            jfieldID mfField = env->GetFieldID(miClass, "moveForward", "F");
            jfieldID msField = env->GetFieldID(miClass, "moveStrafe", "F");
            if (mfField && msField)
            {
                env->ExceptionClear();
                env->SetFloatField(movementInput, mfField, env->GetFloatField(movementInput, mfField) * 0.3f);
                env->SetFloatField(movementInput, msField, env->GetFloatField(movementInput, msField) * 0.3f);
            }
        }
    }

    env->DeleteLocalRef(miClass);
    env->DeleteLocalRef(movementInput);
    env->DeleteLocalRef(playerClass);
    env->DeleteLocalRef(player);
}

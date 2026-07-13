#include "noslow.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../sdk/strayCache.h"
#include "../../../sdk/bridgeHelper.h"

NoSlowModule::NoSlowModule()
    : ModuleBase("NoSlow", "Cancel slowdown from items and blocks", Category::Movement)
{
    AddSetting<EnumSetting>("SwordMode", 1, std::vector<std::string>{"None", "Vanilla"});
    AddSetting<NumberSetting>("SwordMotion", 100.0f, 0.0f, 100.0f, 5.0f);
    AddSetting<BooleanSetting>("SwordSprint", true);
    AddSetting<EnumSetting>("FoodMode", 0, std::vector<std::string>{"None", "Vanilla"});
    AddSetting<NumberSetting>("FoodMotion", 100.0f, 0.0f, 100.0f, 5.0f);
    AddSetting<BooleanSetting>("FoodSprint", true);
    AddSetting<EnumSetting>("BowMode", 0, std::vector<std::string>{"None", "Vanilla"});
    AddSetting<NumberSetting>("BowMotion", 100.0f, 0.0f, 100.0f, 5.0f);
    AddSetting<BooleanSetting>("BowSprint", true);
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void NoSlowModule::OnEnable() { Logger::Log("NoSlow enabled"); m_LastSlot = -1; }
void NoSlowModule::OnDisable() { Logger::Log("NoSlow disabled"); m_LastSlot = -1; }

static bool IsItemType(JNIEnv* env, jobject item, jclass bridgeClass)
{
    return bridgeClass && item && env->IsInstanceOf(item, bridgeClass) == JNI_TRUE;
}

void NoSlowModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!BridgeHelper::Initialize(env)) return;

    jobject player = BridgeHelper::GetPlayer(env);
    if (!player) return;

    bool isUsingItem = false;
    if (BridgeHelper::ELBridge_IsUsingItem)
        isUsingItem = env->CallBooleanMethod(player, BridgeHelper::ELBridge_IsUsingItem) == JNI_TRUE;

    if (!isUsingItem) { m_LastSlot = -1; env->DeleteLocalRef(player); return; }

    bool isSword = false, isBow = false, isFood = false;
    jobject heldStack = nullptr;
    if (BridgeHelper::ELBridge_GetHeldItem)
        heldStack = env->CallObjectMethod(player, BridgeHelper::ELBridge_GetHeldItem);

    if (heldStack && BridgeHelper::ItemStackBridge_GetItem)
    {
        jobject item = env->CallObjectMethod(heldStack, BridgeHelper::ItemStackBridge_GetItem);
        if (item)
        {
            isSword = IsItemType(env, item, BridgeHelper::ItemSwordBridge);
            isBow = IsItemType(env, item, BridgeHelper::ItemBowBridge);
            isFood = IsItemType(env, item, BridgeHelper::ItemFoodBridge);
            env->DeleteLocalRef(item);
        }
        env->DeleteLocalRef(heldStack);
    }

    int swordMode = ((EnumSetting*)FindSetting("SwordMode"))->GetValue();
    int foodMode = ((EnumSetting*)FindSetting("FoodMode"))->GetValue();
    int bowMode = ((EnumSetting*)FindSetting("BowMode"))->GetValue();

    bool active = false;
    float motionPct = 100.0f;
    bool canSprint = true;

    if (isSword && swordMode > 0) { active = true; motionPct = ((NumberSetting*)FindSetting("SwordMotion"))->GetValue(); canSprint = ((BooleanSetting*)FindSetting("SwordSprint"))->GetValue(); }
    else if (isFood && foodMode > 0) { active = true; motionPct = ((NumberSetting*)FindSetting("FoodMotion"))->GetValue(); canSprint = ((BooleanSetting*)FindSetting("FoodSprint"))->GetValue(); }
    else if (isBow && bowMode > 0) { active = true; motionPct = ((NumberSetting*)FindSetting("BowMotion"))->GetValue(); canSprint = ((BooleanSetting*)FindSetting("BowSprint"))->GetValue(); }

    if (!active) { env->DeleteLocalRef(player); return; }

    float multiplier = motionPct / 100.0f;
    if (StrayCache::Entity)
    {
        jfieldID moveForwardField = env->GetFieldID(StrayCache::Entity, "moveForward", "F");
        if (!moveForwardField) env->ExceptionClear();
        jfieldID moveStrafingField = env->GetFieldID(StrayCache::Entity, "moveStrafing", "F");
        if (!moveStrafingField) env->ExceptionClear();
        if (moveForwardField && moveStrafingField)
        {
            env->ExceptionClear();
            float forward = env->GetFloatField(player, moveForwardField);
            float strafe = env->GetFloatField(player, moveStrafingField);
            env->SetFloatField(player, moveForwardField, forward * multiplier);
            env->SetFloatField(player, moveStrafingField, strafe * multiplier);
        }
    }

    if (!canSprint && BridgeHelper::EPSPBridge_SetSprinting)
        env->CallVoidMethod(player, BridgeHelper::EPSPBridge_SetSprinting, JNI_FALSE);

    env->DeleteLocalRef(player);
}

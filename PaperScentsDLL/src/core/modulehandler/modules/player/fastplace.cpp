#include "fastplace.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"

FastPlaceModule::FastPlaceModule()
    : ModuleBase("FastPlace", "Place blocks faster", Category::Player)
{
    AddSetting<NumberSetting>("Delay", 1.0f, 1.0f, 3.0f, 1.0f, "Tick delay between placements");
    AddSetting<BooleanSetting>("BlocksOnly", true, "Only affect block placement");
    AddSetting<BooleanSetting>("SkipObsidian", true, "Don't fast-place obsidian");
    AddSetting<BooleanSetting>("SkipInteractable", true, "Don't fast-place interactable blocks");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void FastPlaceModule::OnEnable() { Logger::Log("FastPlace enabled"); m_DelayTicks = 0; }
void FastPlaceModule::OnDisable() { Logger::Log("FastPlace disabled"); m_DelayTicks = 0; }

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
    bool isBlock = itemBlockClass ? env->IsInstanceOf(item, itemBlockClass) == JNI_TRUE : false;
    if (itemBlockClass) env->DeleteLocalRef(itemBlockClass);
    env->DeleteLocalRef(item);
    env->DeleteLocalRef(stack);
    return isBlock;
}

static bool IsBlockObsidian(JNIEnv* env, jobject block)
{
    if (!block) return false;
    jclass obsidianClass = env->FindClass("net/minecraft/block/BlockObsidian");
    if (!obsidianClass) { env->ExceptionClear(); return false; }
    bool result = env->IsInstanceOf(block, obsidianClass) == JNI_TRUE;
    env->DeleteLocalRef(obsidianClass);
    return result;
}

static bool IsInteractableBlock(JNIEnv* env, jobject block)
{
    if (!block) return false;
    jclass blockClass = env->GetObjectClass(block);
    jmethodID isInteractable = env->GetMethodID(blockClass, "hasComparatorInputOverride", "()Z");
    if (!isInteractable) { env->ExceptionClear(); env->DeleteLocalRef(blockClass); return false; }
    env->ExceptionClear();
    jboolean result = env->CallBooleanMethod(block, isInteractable);
    env->DeleteLocalRef(blockClass);
    return result == JNI_TRUE;
}

static jobject GetBlockFromItem(JNIEnv* env, jobject itemStack)
{
    if (!itemStack) return nullptr;
    jclass stackClass = env->GetObjectClass(itemStack);
    jmethodID getItem = env->GetMethodID(stackClass, "getItem", "()Lnet/minecraft/item/Item;");
    if (!getItem) { env->ExceptionClear(); env->DeleteLocalRef(stackClass); return nullptr; }
    env->ExceptionClear();
    jobject item = env->CallObjectMethod(itemStack, getItem);
    env->DeleteLocalRef(stackClass);
    if (!item) return nullptr;
    jclass itemBlockClass = env->FindClass("net/minecraft/item/ItemBlock");
    if (!itemBlockClass) { env->ExceptionClear(); env->DeleteLocalRef(item); return nullptr; }
    bool isItemBlock = env->IsInstanceOf(item, itemBlockClass) == JNI_TRUE;
    if (!isItemBlock) { env->DeleteLocalRef(itemBlockClass); env->DeleteLocalRef(item); return nullptr; }
    jmethodID getBlock = env->GetMethodID(itemBlockClass, "getBlock", "()Lnet/minecraft/block/Block;");
    env->DeleteLocalRef(itemBlockClass);
    if (!getBlock) { env->ExceptionClear(); env->DeleteLocalRef(item); return nullptr; }
    env->ExceptionClear();
    jobject block = env->CallObjectMethod(item, getBlock);
    env->DeleteLocalRef(item);
    return block;
}

bool FastPlaceModule::CanPlace(JNIEnv* env, jobject player)
{
    if (!player) return false;
    bool blocksOnly = ((BooleanSetting*)FindSetting("BlocksOnly"))->GetValue();
    bool skipObsidian = ((BooleanSetting*)FindSetting("SkipObsidian"))->GetValue();
    bool skipInteractable = ((BooleanSetting*)FindSetting("SkipInteractable"))->GetValue();
    jclass elClass = env->FindClass("net/minecraft/entity/EntityLivingBase");
    if (!elClass) { env->ExceptionClear(); return !blocksOnly; }
    jmethodID getHeldItem = env->GetMethodID(elClass, "getHeldItem", "()Lnet/minecraft/item/ItemStack;");
    if (!getHeldItem) { env->ExceptionClear(); env->DeleteLocalRef(elClass); return !blocksOnly; }
    env->ExceptionClear();
    jobject stack = env->CallObjectMethod(player, getHeldItem);
    env->DeleteLocalRef(elClass);
    if (!stack) return false;
    if (blocksOnly && !IsHoldingBlock(env, player)) { env->DeleteLocalRef(stack); return false; }
    if (skipObsidian || skipInteractable)
    {
        jobject block = GetBlockFromItem(env, stack);
        if (block)
        {
            if ((skipObsidian && IsBlockObsidian(env, block)) || (skipInteractable && IsInteractableBlock(env, block)))
            {
                env->DeleteLocalRef(block);
                env->DeleteLocalRef(stack);
                return false;
            }
            env->DeleteLocalRef(block);
        }
    }
    env->DeleteLocalRef(stack);
    return true;
}

void FastPlaceModule::OnUpdate()
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

    int delay = (int)((NumberSetting*)FindSetting("Delay"))->GetValue();

    bool bridged = BridgeHelper::Initialize(env);
    jobject mc = bridged ? BridgeHelper::GetMinecraftInstance(env) : nullptr;
    jobject player = bridged ? BridgeHelper::GetPlayer(env) : nullptr;
    jclass mcClass = nullptr;

    if (!mc || !player)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java || !java->IsValid()) return;
        mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
        if (!mcClass) return;
        jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
        if (!getMc) { env->DeleteLocalRef(mcClass); return; }
        mc = env->CallStaticObjectMethod(mcClass, getMc);
        if (!mc) { env->DeleteLocalRef(mcClass); return; }
        for (auto f : {"s", "t", "h", "thePlayer"})
        {
            jfieldID pf = env->GetFieldID(mcClass, f, "Lbew;");
            if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
            if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
        }
        if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    }

    jfieldID timerField = nullptr;
    if (mcClass)
    {
        timerField = env->GetFieldID(mcClass, "e", "I");
        if (!timerField) { env->ExceptionClear(); timerField = env->GetFieldID(mcClass, "rightClickDelayTimer", "I"); }
    }
    else
    {
        jclass mcCls = env->FindClass("net/minecraft/client/Minecraft");
        if (mcCls)
        {
            timerField = env->GetFieldID(mcCls, "rightClickDelayTimer", "I");
            env->DeleteLocalRef(mcCls);
        }
    }
    if (!timerField) { env->ExceptionClear(); if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); if (mcClass) env->DeleteLocalRef(mcClass); } return; }
    env->ExceptionClear();

    int rightClickDelayTimer = env->GetIntField(mc, timerField);
    if (rightClickDelayTimer == 4) m_DelayTicks += delay;
    if (m_DelayTicks > 0) m_DelayTicks--;
    if (m_DelayTicks <= 0 && rightClickDelayTimer > 1 && CanPlace(env, player))
        env->SetIntField(mc, timerField, 0);

    if (!bridged) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); if (mcClass) env->DeleteLocalRef(mcClass); }
}

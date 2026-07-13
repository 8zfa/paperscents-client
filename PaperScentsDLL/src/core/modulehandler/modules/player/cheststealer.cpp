#include "cheststealer.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include "../../../sdk/strayCache.h"
#include <cstdlib>

ChestStealerModule::ChestStealerModule()
    : ModuleBase("ChestStealer", "Automatically steals from chests", Category::Player)
{
    AddSetting<NumberSetting>("MinDelay", 1.0f, 0.0f, 10.0f, 1.0f);
    AddSetting<NumberSetting>("MaxDelay", 2.0f, 0.0f, 10.0f, 1.0f);
    AddSetting<NumberSetting>("OpenDelay", 1.0f, 0.0f, 10.0f, 1.0f);
    AddSetting<BooleanSetting>("AutoClose", false);
    AddSetting<BooleanSetting>("NameCheck", true);
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void ChestStealerModule::OnEnable()
{
    Logger::Log("ChestStealer enabled");
    m_ClickDelay = 0;
    m_OpenDelay = 0;
    m_InChest = false;
    m_WarnedFull = false;
}

void ChestStealerModule::OnDisable()
{
    Logger::Log("ChestStealer disabled");
    m_ClickDelay = 0;
    m_OpenDelay = 0;
    m_InChest = false;
    m_WarnedFull = false;
}

void ChestStealerModule::ShiftClick(JNIEnv* env, jobject player, jobject controller, int windowId, int slotId)
{
    if (!controller) return;
    jclass ctrlClass = env->GetObjectClass(controller);
    jmethodID windowClick = env->GetMethodID(ctrlClass, "a", "(IIIILbew;)V");
    if (!windowClick) { env->ExceptionClear(); windowClick = env->GetMethodID(ctrlClass, "windowClick", "(IIIILnet/minecraft/entity/player/EntityPlayer;)V"); }
    if (windowClick) { env->ExceptionClear(); env->CallVoidMethod(controller, windowClick, windowId, slotId, 0, 1, player); }
    env->DeleteLocalRef(ctrlClass);
}

void ChestStealerModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }
    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;
    if (!StrayCache::MinecraftInstance || !StrayCache::Minecraft_thePlayer) return;

    if (m_ClickDelay > 0) m_ClickDelay--;
    if (m_OpenDelay > 0) m_OpenDelay--;

    // Get currentScreen
    jclass mcClass = StrayCache::Minecraft;
    if (!mcClass) return;
    jfieldID currentScreenField = env->GetFieldID(mcClass, "d", "Lbaw;");
    if (!currentScreenField) { env->ExceptionClear(); currentScreenField = env->GetFieldID(mcClass, "currentScreen", "Lnet/minecraft/client/gui/GuiScreen;"); }
    if (!currentScreenField) { env->ExceptionClear(); return; }
    env->ExceptionClear();
    jobject currentScreen = env->GetObjectField(StrayCache::MinecraftInstance, currentScreenField);

    if (!currentScreen)
    {
        m_InChest = false;
        m_WarnedFull = false;
        return;
    }

    // Check if instanceof GuiChest
    jclass guiChestClass = env->FindClass("bip");
    if (!guiChestClass) { env->ExceptionClear(); guiChestClass = env->FindClass("net/minecraft/client/gui/inventory/GuiChest"); }
    if (!guiChestClass) { env->ExceptionClear(); env->DeleteLocalRef(currentScreen); return; }
    bool isChest = env->IsInstanceOf(currentScreen, guiChestClass) == JNI_TRUE;
    env->DeleteLocalRef(guiChestClass);

    if (!isChest)
    {
        m_InChest = false;
        m_WarnedFull = false;
        env->DeleteLocalRef(currentScreen);
        return;
    }

    // Get inventorySlots field from GuiChest
    jclass guiChestClass2 = env->FindClass("bip");
    if (!guiChestClass2) { env->ExceptionClear(); guiChestClass2 = env->FindClass("net/minecraft/client/gui/inventory/GuiChest"); }
    if (!guiChestClass2) { env->DeleteLocalRef(currentScreen); return; }
    jfieldID invSlotsField = env->GetFieldID(guiChestClass2, "e", "Lbck;");
    if (!invSlotsField) { env->ExceptionClear(); invSlotsField = env->GetFieldID(guiChestClass2, "inventorySlots", "Lnet/minecraft/inventory/Container;"); }
    if (!invSlotsField) { env->ExceptionClear(); env->DeleteLocalRef(guiChestClass2); env->DeleteLocalRef(currentScreen); return; }
    env->ExceptionClear();
    jobject container = env->GetObjectField(currentScreen, invSlotsField);
    env->DeleteLocalRef(guiChestClass2);
    if (!container) { env->DeleteLocalRef(currentScreen); return; }

    // Check if instanceof ContainerChest
    jclass containerChestClass = env->FindClass("ams");
    if (!containerChestClass) { env->ExceptionClear(); containerChestClass = env->FindClass("net/minecraft/inventory/ContainerChest"); }
    bool isContainerChest = containerChestClass ? (env->IsInstanceOf(container, containerChestClass) == JNI_TRUE) : false;
    if (containerChestClass) env->DeleteLocalRef(containerChestClass);

    if (!isContainerChest)
    {
        m_InChest = false;
        env->DeleteLocalRef(container);
        env->DeleteLocalRef(currentScreen);
        return;
    }

    if (!m_InChest)
    {
        m_InChest = true;
        m_WarnedFull = false;
        m_OpenDelay = (int)((NumberSetting*)FindSetting("OpenDelay"))->GetValue() + 1;
    }

    if (m_OpenDelay > 0 || m_ClickDelay > 0)
    {
        env->DeleteLocalRef(container);
        env->DeleteLocalRef(currentScreen);
        return;
    }

    // Get lower chest inventory
    jclass contChestCls = env->FindClass("ams");
    if (!contChestCls) { env->ExceptionClear(); contChestCls = env->FindClass("net/minecraft/inventory/ContainerChest"); }
    if (!contChestCls) { env->DeleteLocalRef(container); env->DeleteLocalRef(currentScreen); return; }
    jmethodID getLowerChest = env->GetMethodID(contChestCls, "e", "()Lrf;");
    if (!getLowerChest) { env->ExceptionClear(); getLowerChest = env->GetMethodID(contChestCls, "getLowerChestInventory", "()Lnet/minecraft/inventory/IInventory;"); }
    if (!getLowerChest) { env->ExceptionClear(); env->DeleteLocalRef(contChestCls); env->DeleteLocalRef(container); env->DeleteLocalRef(currentScreen); return; }
    env->ExceptionClear();
    jobject inventory = env->CallObjectMethod(container, getLowerChest);
    env->DeleteLocalRef(contChestCls);
    if (!inventory) { env->DeleteLocalRef(container); env->DeleteLocalRef(currentScreen); return; }

    // Name check
    bool nameCheck = ((BooleanSetting*)FindSetting("NameCheck"))->GetValue();
    if (nameCheck)
    {
        jclass invClass = env->GetObjectClass(inventory);
        jmethodID getName = env->GetMethodID(invClass, "getName", "()Ljava/lang/String;");
        if (!getName) { env->ExceptionClear(); getName = env->GetMethodID(invClass, "k", "()Ljava/lang/String;"); }
        jstring chestName = nullptr;
        if (getName) { env->ExceptionClear(); chestName = (jstring)env->CallObjectMethod(inventory, getName); }
        env->DeleteLocalRef(invClass);
        if (chestName)
        {
            const char* nameStr = env->GetStringUTFChars(chestName, nullptr);
            bool valid = nameStr && (strcmp(nameStr, "container.chest") == 0 || strcmp(nameStr, "container.chestDouble") == 0);
            env->ReleaseStringUTFChars(chestName, nameStr);
            env->DeleteLocalRef(chestName);
            if (!valid)
            {
                env->DeleteLocalRef(inventory);
                env->DeleteLocalRef(container);
                env->DeleteLocalRef(currentScreen);
                return;
            }
        }
    }

    // Get player inventory
    jobject player = env->GetObjectField(StrayCache::MinecraftInstance, StrayCache::Minecraft_thePlayer);
    if (!player) { env->DeleteLocalRef(inventory); env->DeleteLocalRef(container); env->DeleteLocalRef(currentScreen); return; }

    // Get player controller
    jobject controller = nullptr;
    if (StrayCache::Minecraft_playerController)
        controller = env->GetObjectField(StrayCache::MinecraftInstance, StrayCache::Minecraft_playerController);

    // Check if inventory full
    jclass invPlayerClass = env->FindClass("qi");
    if (!invPlayerClass) { env->ExceptionClear(); invPlayerClass = env->FindClass("net/minecraft/entity/player/InventoryPlayer"); }
    if (invPlayerClass)
    {
        jmethodID getFirstEmpty = env->GetMethodID(invPlayerClass, "f", "()I");
        if (!getFirstEmpty) { env->ExceptionClear(); getFirstEmpty = env->GetMethodID(invPlayerClass, "getFirstEmptyStack", "()I"); }
        if (getFirstEmpty)
        {
            env->ExceptionClear();
            jfieldID invField = StrayCache::EntityPlayer_inventory;
            if (invField)
            {
                jobject inv = env->GetObjectField(player, invField);
                if (inv)
                {
                    int firstEmpty = env->CallIntMethod(inv, getFirstEmpty);
                    env->DeleteLocalRef(inv);
                    if (firstEmpty == -1)
                    {
                        if (!m_WarnedFull)
                        {
                            Logger::Log("ChestStealer: Inventory full!");
                            m_WarnedFull = true;
                        }
                        bool autoClose = ((BooleanSetting*)FindSetting("AutoClose"))->GetValue();
                        if (autoClose)
                        {
                            jmethodID closeScreen = env->GetMethodID(StrayCache::EntityPlayer, "o", "()V");
                            if (!closeScreen) { env->ExceptionClear(); closeScreen = env->GetMethodID(StrayCache::EntityPlayer, "closeScreen", "()V"); }
                            if (closeScreen) { env->ExceptionClear(); env->CallVoidMethod(player, closeScreen); }
                        }
                        env->DeleteLocalRef(invPlayerClass);
                        env->DeleteLocalRef(player);
                        if (controller) env->DeleteLocalRef(controller);
                        env->DeleteLocalRef(inventory);
                        env->DeleteLocalRef(container);
                        env->DeleteLocalRef(currentScreen);
                        return;
                    }
                }
            }
        }
    }

    // Get chest size and window ID
    jclass invClass2 = env->GetObjectClass(inventory);
    jmethodID getSize = env->GetMethodID(invClass2, "getSizeInventory", "()I");
    if (!getSize) { env->ExceptionClear(); getSize = env->GetMethodID(invClass2, "j", "()I"); }
    jint invSize = 0;
    if (getSize) { env->ExceptionClear(); invSize = env->CallIntMethod(inventory, getSize); }
    env->DeleteLocalRef(invClass2);
    jclass contClass = env->GetObjectClass(container);
    jfieldID windowIdField = env->GetFieldID(contClass, "g", "I");
    if (!windowIdField) { env->ExceptionClear(); windowIdField = env->GetFieldID(contClass, "windowId", "I"); }
    jint windowId = 0;
    if (windowIdField) { env->ExceptionClear(); windowId = env->GetIntField(container, windowIdField); }
    env->DeleteLocalRef(contClass);

    // Loop through chest slots
    jclass containerCls = env->GetObjectClass(container);
    jmethodID getSlot = env->GetMethodID(containerCls, "getSlot", "(I)Lbdd;");
    if (!getSlot) { env->ExceptionClear(); getSlot = env->GetMethodID(containerCls, "c", "(I)Lbdd;"); }
    jmethodID slotGetHasStack = nullptr;
    if (getSlot) slotGetHasStack = env->GetMethodID(env->FindClass("bdd"), "getHasStack", "()Z");
    if (!slotGetHasStack) { env->ExceptionClear(); slotGetHasStack = env->GetMethodID(env->FindClass("bdd"), "d", "()Z"); }
    env->DeleteLocalRef(containerCls);
    if (slotGetHasStack) env->ExceptionClear();

    bool didClick = false;
    if (getSlot && slotGetHasStack)
    {
        jclass slotClass = env->FindClass("bdd");
        if (!slotClass) { env->ExceptionClear(); slotClass = env->FindClass("net/minecraft/inventory/Slot"); }
        if (slotClass)
        {
            for (int i = 0; i < invSize; i++)
            {
                env->ExceptionClear();
                jobject slot = env->CallObjectMethod(container, getSlot, i);
                if (!slot) continue;
                bool hasStack = env->CallBooleanMethod(slot, slotGetHasStack) == JNI_TRUE;
                env->DeleteLocalRef(slot);
                if (hasStack)
                {
                    ShiftClick(env, player, controller, windowId, i);
                    didClick = true;
                    break;
                }
            }
            env->DeleteLocalRef(slotClass);
        }
    }

    if (!didClick)
    {
        bool autoClose = ((BooleanSetting*)FindSetting("AutoClose"))->GetValue();
        if (autoClose)
        {
            jmethodID closeScreen = env->GetMethodID(StrayCache::EntityPlayer, "o", "()V");
            if (!closeScreen) { env->ExceptionClear(); closeScreen = env->GetMethodID(StrayCache::EntityPlayer, "closeScreen", "()V"); }
            if (closeScreen) { env->ExceptionClear(); env->CallVoidMethod(player, closeScreen); }
        }
    }

    // Set click delay
    int minD = (int)((NumberSetting*)FindSetting("MinDelay"))->GetValue();
    int maxD = (int)((NumberSetting*)FindSetting("MaxDelay"))->GetValue();
    if (maxD < minD) maxD = minD;
    m_ClickDelay = minD + (maxD > minD ? rand() % (maxD - minD + 1) : 0);

    env->DeleteLocalRef(player);
    if (controller) env->DeleteLocalRef(controller);
    env->DeleteLocalRef(inventory);
    env->DeleteLocalRef(container);
    env->DeleteLocalRef(currentScreen);
}

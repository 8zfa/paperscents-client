#include "autotool.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"

AutoToolModule::AutoToolModule()
    : ModuleBase("AutoTool", "Auto-switch to best tool", Category::Player)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Silent", "Normal"});
    AddSetting<BooleanSetting>("Swords", true, "Switch to sword for attacking");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void AutoToolModule::OnEnable() { Logger::Log("AutoTool enabled"); }
void AutoToolModule::OnDisable() { Logger::Log("AutoTool disabled"); }

static bool IsPlayerMining(JNIEnv* env, jobject mc, jclass mcClass)
{
    jfieldID objField = env->GetFieldID(mcClass, "c", "Lbby;");
    if (!objField) { env->ExceptionClear(); objField = env->GetFieldID(mcClass, "objectMouseOver", "Lbby;"); }
    env->ExceptionClear();
    if (!objField) return false;

    jobject obj = env->GetObjectField(mc, objField);
    if (!obj) return false;

    jclass objClass = env->GetObjectClass(obj);
    jfieldID typeField = env->GetFieldID(objClass, "a", "Lbce;");
    if (!typeField) { env->ExceptionClear(); typeField = env->GetFieldID(objClass, "typeOfHit", "Lbce;"); }
    env->ExceptionClear();

    bool result = false;
    if (typeField)
    {
        jobject typeObj = env->GetObjectField(obj, typeField);
        if (typeObj)
        {
            jclass typeClass = env->GetObjectClass(typeObj);
            jmethodID ordinalMethod = env->GetMethodID(typeClass, "ordinal", "()I");
            if (ordinalMethod)
            {
                int ordinal = env->CallIntMethod(typeObj, ordinalMethod);
                result = (ordinal == 1);
            }
            env->DeleteLocalRef(typeClass);
            env->DeleteLocalRef(typeObj);
        }
    }

    env->DeleteLocalRef(objClass);
    env->DeleteLocalRef(obj);
    return result;
}

void AutoToolModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    jclass mcClass = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) { env->ExceptionClear(); mcClass = env->FindClass("net/minecraft/client/Minecraft"); }
    if (!mcClass) return;

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "a", "()Lave;"); }
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "getMinecraft", "()Lave;"); }
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    env->ExceptionClear();

    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    bool mining = IsPlayerMining(env, mc, mcClass);
    bool attacking = false;

    jfieldID lcField = env->GetFieldID(mcClass, "i", "Z");
    if (!lcField) { env->ExceptionClear(); lcField = env->GetFieldID(mcClass, "leftClickCounter", "Z"); }
    if (!lcField) { env->ExceptionClear(); lcField = env->GetFieldID(mcClass, "gameSettings", "Lavh;"); }
    env->ExceptionClear();

    if (!mining)
    {
        jclass gsClass = Core::GetInstance().GetJava()->FindClass("avh", "net/minecraft/client/settings/GameSettings");
        if (!gsClass) { env->ExceptionClear(); gsClass = env->FindClass("net/minecraft/client/settings/GameSettings"); }
        if (gsClass)
        {
            jfieldID keyBindAttack = env->GetFieldID(gsClass, "q", "Lave$c;");
            if (!keyBindAttack) { env->ExceptionClear(); keyBindAttack = env->GetFieldID(gsClass, "keyBindAttack", "Lave$c;"); }
            env->ExceptionClear();

            if (keyBindAttack)
            {
                jfieldID gsField = env->GetFieldID(mcClass, "t", "Lavh;");
                if (!gsField) { env->ExceptionClear(); gsField = env->GetFieldID(mcClass, "gameSettings", "Lavh;"); }
                env->ExceptionClear();

                if (gsField)
                {
                    jobject gs = env->GetObjectField(mc, gsField);
                    if (gs)
                    {
                        jobject keyObj = env->GetObjectField(gs, keyBindAttack);
                        if (keyObj)
                        {
                            jclass keyClass = env->GetObjectClass(keyObj);
                            jfieldID pressedField = env->GetFieldID(keyClass, "pressed", "Z");
                            if (!pressedField) { env->ExceptionClear(); pressedField = env->GetFieldID(keyClass, "e", "Z"); }
                            env->ExceptionClear();
                            if (pressedField)
                                attacking = env->GetBooleanField(keyObj, pressedField);
                            env->DeleteLocalRef(keyClass);
                            env->DeleteLocalRef(keyObj);
                        }
                        env->DeleteLocalRef(gs);
                    }
                }
            }
            env->DeleteLocalRef(gsClass);
        }
    }

    if (!mining && !attacking)
    {
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    jfieldID playerField = env->GetFieldID(mcClass, "h", "Lbew;");
    if (!playerField) { env->ExceptionClear(); playerField = env->GetFieldID(mcClass, "thePlayer", "Lbew;"); }
    env->ExceptionClear();

    jobject player = playerField ? env->GetObjectField(mc, playerField) : nullptr;
    if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass playerClass = env->GetObjectClass(player);
    jfieldID inventoryField = env->GetFieldID(playerClass, "bg", "Lavi;");
    if (!inventoryField) { env->ExceptionClear(); inventoryField = env->GetFieldID(playerClass, "inventory", "Lavi;"); }
    env->ExceptionClear();

    jobject inventory = inventoryField ? env->GetObjectField(player, inventoryField) : nullptr;
    if (!inventory) { env->DeleteLocalRef(playerClass); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass invClass = env->GetObjectClass(inventory);
    jfieldID mainInvField = env->GetFieldID(invClass, "a", "[Lbbr;");
    if (!mainInvField) { env->ExceptionClear(); mainInvField = env->GetFieldID(invClass, "mainInventory", "[Lbbr;"); }
    env->ExceptionClear();

    jobjectArray mainInv = mainInvField ? (jobjectArray)env->GetObjectField(inventory, mainInvField) : nullptr;

    jfieldID currentItemField = env->GetFieldID(invClass, "d", "I");
    if (!currentItemField) { env->ExceptionClear(); currentItemField = env->GetFieldID(invClass, "currentItem", "I"); }
    env->ExceptionClear();

    if (!mainInv || !currentItemField)
    {
        env->DeleteLocalRef(invClass);
        env->DeleteLocalRef(inventory);
        env->DeleteLocalRef(playerClass);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    int currentSlot = env->GetIntField(inventory, currentItemField);
    int bestSlot = -1;
    float bestValue = 0.0f;

    jclass itemStackClass = Core::GetInstance().GetJava()->FindClass("bbr", "net/minecraft/item/ItemStack");
    if (!itemStackClass) { env->ExceptionClear(); itemStackClass = env->FindClass("net/minecraft/item/ItemStack"); }
    jclass itemClass = Core::GetInstance().GetJava()->FindClass("bbe", "net/minecraft/item/Item");
    if (!itemClass) { env->ExceptionClear(); itemClass = env->FindClass("net/minecraft/item/Item"); }

    for (int i = 36; i < 45; i++)
    {
        jobject stack = env->GetObjectArrayElement(mainInv, i);
        if (!stack) continue;

        float value = 0.0f;

        if (itemStackClass)
        {
            jmethodID getItemMethod = env->GetMethodID(itemStackClass, "b", "()Lbbe;");
            if (!getItemMethod) { env->ExceptionClear(); getItemMethod = env->GetMethodID(itemStackClass, "getItem", "()Lbbe;"); }
            env->ExceptionClear();

            if (getItemMethod)
            {
                jobject item = env->CallObjectMethod(stack, getItemMethod);
                if (item && itemClass)
                {
                    jmethodID getToolEff = env->GetMethodID(itemClass, "a", "(Lbbr;)F");
                    if (!getToolEff) {
                        env->ExceptionClear();
                        jfieldID effField = env->GetFieldID(itemClass, "efficiencyOnProperMaterial", "F");
                        if (!effField) { env->ExceptionClear(); effField = env->GetFieldID(itemClass, "c", "F"); }
                        env->ExceptionClear();

                        if (effField)
                        {
                            float baseEff = env->GetFloatField(item, effField);
                            if (baseEff > 1.0f)
                                value = baseEff * 2.0f;
                        }
                    }
                    env->ExceptionClear();

                    jfieldID swordField = env->GetFieldID(itemClass, "attackDamage", "D");
                    if (!swordField) { env->ExceptionClear(); swordField = env->GetFieldID(itemClass, "h", "D"); }
                    env->ExceptionClear();

                    if (swordField && ((BooleanSetting*)FindSetting("Swords"))->GetValue())
                    {
                        double dmg = env->GetDoubleField(item, swordField);
                        if (dmg > value)
                            value = (float)dmg;
                    }

                    env->DeleteLocalRef(item);
                }
            }
        }

        if (value > bestValue)
        {
            bestValue = value;
            bestSlot = i - 36;
        }

        env->DeleteLocalRef(stack);
    }

    if (bestSlot >= 0 && bestSlot != currentSlot)
    {
        int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();

        if (mode == 1)
        {
            env->SetIntField(inventory, currentItemField, bestSlot);
        }
        else
        {
            jclass packetClass = Core::GetInstance().GetJava()->FindClass("hh", "net/minecraft/network/play/client/C08PacketPlayerBlockPlacement");
            if (!packetClass) { env->ExceptionClear(); packetClass = env->FindClass("net/minecraft/network/play/client/C08PacketPlayerBlockPlacement"); }
            if (!packetClass) { env->ExceptionClear(); packetClass = env->FindClass("net/minecraft/network/play/client/C09PacketHeldItemChange"); }
            env->ExceptionClear();

            jclass c09Class = Core::GetInstance().GetJava()->FindClass("hh", "net/minecraft/network/play/client/C09PacketHeldItemChange");
            if (!c09Class) { env->ExceptionClear(); c09Class = env->FindClass("net/minecraft/network/play/client/C09PacketHeldItemChange"); }
            if (!c09Class) { env->ExceptionClear(); c09Class = env->FindClass("net/minecraft/network/play/client/C09PacketHeldItemChange"); }
            env->ExceptionClear();

            if (c09Class)
            {
                jmethodID c09Ctor = env->GetMethodID(c09Class, "<init>", "(I)V");
                if (!c09Ctor) { env->ExceptionClear(); c09Ctor = env->GetMethodID(c09Class, "<init>", "(I)V"); }
                env->ExceptionClear();
                if (c09Ctor)
                {
                    jobject packet = env->NewObject(c09Class, c09Ctor, bestSlot);
                    if (packet)
                    {
                        jfieldID nhField = env->GetFieldID(mcClass, "q", "Leh;");
                        if (!nhField) { env->ExceptionClear(); nhField = env->GetFieldID(mcClass, "v", "Leh;"); }
                        if (!nhField) { env->ExceptionClear(); nhField = env->GetFieldID(mcClass, "getNetHandler", "Leh;"); }
                        env->ExceptionClear();

                        if (nhField)
                        {
                            jobject nh = env->GetObjectField(mc, nhField);
                            if (nh)
                            {
                                jclass nhClass = env->GetObjectClass(nh);
                                jmethodID sendMethod = env->GetMethodID(nhClass, "a", "(Lhb;)V");
                                if (!sendMethod) { env->ExceptionClear(); sendMethod = env->GetMethodID(nhClass, "addToSendQueue", "(Lnet/minecraft/network/Packet;)V"); }
                                env->ExceptionClear();

                                if (sendMethod)
                                    env->CallVoidMethod(nh, sendMethod, packet);

                                env->DeleteLocalRef(nhClass);
                                env->DeleteLocalRef(nh);
                            }
                        }
                        env->DeleteLocalRef(packet);
                    }
                }
                env->DeleteLocalRef(c09Class);
            }

            env->SetIntField(inventory, currentItemField, bestSlot);
        }
    }

    if (itemClass) env->DeleteLocalRef(itemClass);
    if (itemStackClass) env->DeleteLocalRef(itemStackClass);
    env->DeleteLocalRef(invClass);
    env->DeleteLocalRef(inventory);
    env->DeleteLocalRef(playerClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

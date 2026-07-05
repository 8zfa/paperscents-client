#include "armorhud.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <imgui.h>
#include <string>
#include <vector>

std::vector<ArmorPiece> ArmorHUDModule::GetArmor(JNIEnv* env, jobject player)
{
    std::vector<ArmorPiece> result;

    if (!player) return result;

    jmethodID getInvMid = env->GetMethodID(StrayCache::EntityPlayer, "d", "()Lbic;");
    if (!getInvMid) { env->ExceptionClear(); getInvMid = env->GetMethodID(StrayCache::EntityPlayer, "getInventory", "()Lnet/minecraft/entity/player/InventoryPlayer;"); }
    env->ExceptionClear();
    if (!getInvMid) return result;

    jobject inventory = env->CallObjectMethod(player, getInvMid);
    if (!inventory) return result;

    jclass invCls = env->GetObjectClass(inventory);
    if (!invCls) { env->DeleteLocalRef(inventory); return result; }

    jfieldID mainInvID = env->GetFieldID(invCls, "e", "[Ljava/lang/Object;");
    if (!mainInvID) { env->ExceptionClear(); mainInvID = env->GetFieldID(invCls, "mainInventory", "[Lnet/minecraft/item/ItemStack;"); }
    env->ExceptionClear();

    if (!mainInvID) { env->DeleteLocalRef(invCls); env->DeleteLocalRef(inventory); return result; }

    jobjectArray mainInv = (jobjectArray)env->GetObjectField(inventory, mainInvID);
    env->DeleteLocalRef(invCls);

    if (!mainInv) { env->DeleteLocalRef(inventory); return result; }

    jsize arrLen = env->GetArrayLength(mainInv);

    for (int i = 36; i <= 39 && i < arrLen; i++)
    {
        jobject itemStack = env->GetObjectArrayElement(mainInv, i);
        if (!itemStack) continue;

        ArmorPiece piece;
        piece.itemDamage = 0;

        jclass stackCls = env->GetObjectClass(itemStack);
        if (stackCls)
        {
            jmethodID displayMid = env->GetMethodID(stackCls, "c", "()Ljava/lang/String;");
            if (!displayMid) { env->ExceptionClear(); displayMid = env->GetMethodID(stackCls, "getDisplayName", "()Ljava/lang/String;"); }
            env->ExceptionClear();

            if (displayMid)
            {
                jobject nameObj = env->CallObjectMethod(itemStack, displayMid);
                if (nameObj)
                {
                    const char* str = env->GetStringUTFChars((jstring)nameObj, nullptr);
                    if (str) { piece.displayName = str; env->ReleaseStringUTFChars((jstring)nameObj, str); }
                    env->DeleteLocalRef(nameObj);
                }
            }

            if (piece.displayName.empty())
            {
                jmethodID itemMid = env->GetMethodID(stackCls, "b", "()Ljava/lang/Object;");
                if (!itemMid) { env->ExceptionClear(); itemMid = env->GetMethodID(stackCls, "getItem", "()Lnet/minecraft/item/Item;"); }
                env->ExceptionClear();

                if (itemMid)
                {
                    jobject item = env->CallObjectMethod(itemStack, itemMid);
                    if (item)
                    {
                        jclass itemCls = env->GetObjectClass(item);
                        if (itemCls)
                        {
                            jmethodID unameMid = env->GetMethodID(itemCls, "a", "()Ljava/lang/String;");
                            if (!unameMid) { env->ExceptionClear(); unameMid = env->GetMethodID(itemCls, "getUnlocalizedName", "()Ljava/lang/String;"); }
                            env->ExceptionClear();

                            if (unameMid)
                            {
                                jobject unameObj = env->CallObjectMethod(item, unameMid);
                                if (unameObj)
                                {
                                    const char* str = env->GetStringUTFChars((jstring)unameObj, nullptr);
                                    if (str) { piece.displayName = str; env->ReleaseStringUTFChars((jstring)unameObj, str); }
                                    env->DeleteLocalRef(unameObj);
                                }
                            }
                            env->DeleteLocalRef(itemCls);
                        }
                        env->DeleteLocalRef(item);
                    }
                }
            }

            jmethodID dmgMid = env->GetMethodID(stackCls, "e", "()I");
            if (!dmgMid) { env->ExceptionClear(); dmgMid = env->GetMethodID(stackCls, "getItemDamage", "()I"); }
            env->ExceptionClear();
            if (dmgMid) piece.itemDamage = env->CallIntMethod(itemStack, dmgMid);

            env->DeleteLocalRef(stackCls);
        }

        if (!piece.displayName.empty())
            result.push_back(piece);

        env->DeleteLocalRef(itemStack);
    }

    env->DeleteLocalRef(mainInv);
    env->DeleteLocalRef(inventory);

    return result;
}

ArmorHUDModule::ArmorHUDModule()
    : ModuleBase("ArmorHUD", "Shows equipped armor on HUD", Category::Render)
{
    AddSetting<BooleanSetting>("ShowSelf", true, "Show own armor");
    AddSetting<BooleanSetting>("ShowOthers", false, "Show other players armor");
    AddSetting<NumberSetting>("X", 0.0f, -1000.0f, 2000.0f, 1.0f, "X position");
    AddSetting<NumberSetting>("Y", 0.0f, -1000.0f, 2000.0f, 1.0f, "Y position");
}

void ArmorHUDModule::OnEnable() { Logger::Log("ArmorHUD enabled"); }
void ArmorHUDModule::OnDisable() { Logger::Log("ArmorHUD disabled"); }

void ArmorHUDModule::OnRender()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    bool showSelf = ((BooleanSetting*)FindSetting("ShowSelf"))->GetValue();
    float x = ((NumberSetting*)FindSetting("X"))->GetValue();
    float y = ((NumberSetting*)FindSetting("Y"))->GetValue();

    ImDrawList* draw = ImGui::GetForegroundDrawList();

    if (showSelf)
    {
        jobject player = GetPlayerObject(env, mc);
        if (player)
        {
            std::vector<ArmorPiece> armor = GetArmor(env, player);
            float curX = x;
            for (auto& piece : armor)
            {
                ImColor slotCol(0, 0, 0, 150);
                draw->AddRectFilled(ImVec2(curX, y), ImVec2(curX + 80, y + 20), slotCol);
                draw->AddRect(ImVec2(curX, y), ImVec2(curX + 80, y + 20), ImColor(255, 255, 255, 100), 0.0f, 0, 1.0f);

                std::string label = piece.displayName;
                if (piece.itemDamage > 0)
                    label += " [" + std::to_string(piece.itemDamage) + "]";

                draw->AddText(ImVec2(curX + 2, y + 2), ImColor(255, 255, 255, 255), label.c_str());
                curX += 82;
            }
            env->DeleteLocalRef(player);
        }
    }

    env->DeleteLocalRef(mc);
}

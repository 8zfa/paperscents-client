#include "autogg.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"

AutoGGModule::AutoGGModule()
    : ModuleBase("AutoGG", "Auto say gg on game end", Category::Misc)
{
    AddSetting<BooleanSetting>("GG", true, "Say 'gg' on game end");
    AddSetting<BooleanSetting>("GF", false, "Say 'gf' on game end");
}

void AutoGGModule::OnEnable()
{
    Logger::Log("AutoGG enabled");
    m_Triggered = false;
}

void AutoGGModule::OnDisable()
{
    Logger::Log("AutoGG disabled");
    m_Triggered = false;
}

void AutoGGModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (m_Triggered) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jclass mcClass = env->FindClass("net/minecraft/client/Minecraft");
    if (!mcClass) { env->ExceptionClear(); return; }

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "getMinecraft", "()Lnet/minecraft/client/Minecraft;"); }
    if (!getMc) { env->ExceptionClear(); env->DeleteLocalRef(mcClass); return; }
    env->ExceptionClear();

    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    jfieldID thePlayerField = env->GetFieldID(mcClass, "u", "Lbew;");
    if (!thePlayerField) { env->ExceptionClear(); thePlayerField = env->GetFieldID(mcClass, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;"); }
    if (!thePlayerField) { env->ExceptionClear(); thePlayerField = env->GetFieldID(mcClass, "h", "Lbew;"); }
    env->ExceptionClear();

    jobject player = thePlayerField ? env->GetObjectField(mc, thePlayerField) : nullptr;
    if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass playerClass = env->GetObjectClass(player);
    jmethodID sendChat = env->GetMethodID(playerClass, "a", "(Ljava/lang/String;)V");
    if (!sendChat) { env->ExceptionClear(); sendChat = env->GetMethodID(playerClass, "sendChatMessage", "(Ljava/lang/String;)V"); }
    env->ExceptionClear();

    if (!sendChat) { env->DeleteLocalRef(playerClass); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass worldClass = env->GetObjectClass(GetWorldObject(env, mc));
    if (!worldClass) { env->ExceptionClear(); env->DeleteLocalRef(playerClass); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jfieldID loadedEntityListField = env->GetFieldID(worldClass, "g", "Ljava/util/List;");
    if (!loadedEntityListField) { env->ExceptionClear(); loadedEntityListField = env->GetFieldID(worldClass, "loadedEntityList", "Ljava/util/List;"); }
    env->ExceptionClear();

    if (loadedEntityListField)
    {
        jobject list = env->GetObjectField(GetWorldObject(env, mc), loadedEntityListField);
        if (list)
        {
            jclass listClass = env->GetObjectClass(list);
            jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
            if (sizeMethod)
            {
                int size = env->CallIntMethod(list, sizeMethod);

                // Check scoreboard/chat for game end triggers
                // For this module we need to check chat messages
                // Since we don't have a chat hook, we check via the GuiNewChat
                jfieldID chatField = env->GetFieldID(mcClass, "o", "Lbic;");
                if (!chatField) { env->ExceptionClear(); chatField = env->GetFieldID(mcClass, "ingameGUI", "Lbic;"); }
                if (!chatField) { env->ExceptionClear(); chatField = env->GetFieldID(mcClass, "q", "Lbic;"); }
                env->ExceptionClear();

                if (chatField)
                {
                    jobject ingameGui = env->GetObjectField(mc, chatField);
                    if (ingameGui)
                    {
                        jclass guiClass = env->GetObjectClass(ingameGui);
                        jfieldID chatMessageField = env->GetFieldID(guiClass, "b", "Lbio;");
                        if (!chatMessageField) { env->ExceptionClear(); chatMessageField = env->GetFieldID(guiClass, "persistantChatGUI", "Lbio;"); }
                        env->ExceptionClear();

                        if (chatMessageField)
                        {
                            jobject chatGui = env->GetObjectField(ingameGui, chatMessageField);
                            if (chatGui)
                            {
                                jclass chatClass = env->GetObjectClass(chatGui);
                                jmethodID getLines = env->GetMethodID(chatClass, "a", "()Ljava/util/List;");
                                if (!getLines) { env->ExceptionClear(); getLines = env->GetMethodID(chatClass, "getChatMessages", "()Ljava/util/List;"); }
                                env->ExceptionClear();

                                if (getLines)
                                {
                                    jobject lines = env->CallObjectMethod(chatGui, getLines);
                                    if (lines)
                                    {
                                        jclass linesClass = env->GetObjectClass(lines);
                                        jmethodID linesSize = env->GetMethodID(linesClass, "size", "()I");
                                        jmethodID get = env->GetMethodID(linesClass, "get", "(I)Ljava/lang/Object;");
                                        if (linesSize && get)
                                        {
                                            int lineCount = env->CallIntMethod(lines, linesSize);
                                            for (int i = 0; i < lineCount && i < 20; i++)
                                            {
                                                jobject chatLine = env->CallObjectMethod(lines, get, i);
                                                if (!chatLine) continue;

                                                jclass chatLineClass = env->GetObjectClass(chatLine);
                                                jmethodID getChatComponent = env->GetMethodID(chatLineClass, "a", "()Lhh;");
                                                if (!getChatComponent) { env->ExceptionClear(); getChatComponent = env->GetMethodID(chatLineClass, "getChatComponent", "()Lnet/minecraft/util/IChatComponent;"); }
                                                env->ExceptionClear();

                                                if (getChatComponent)
                                                {
                                                    jobject component = env->CallObjectMethod(chatLine, getChatComponent);
                                                    if (component)
                                                    {
                                                        jclass compClass = env->GetObjectClass(component);
                                                        jmethodID getUnformatted = env->GetMethodID(compClass, "c", "()Ljava/lang/String;");
                                                        if (!getUnformatted) { env->ExceptionClear(); getUnformatted = env->GetMethodID(compClass, "getUnformattedText", "()Ljava/lang/String;"); }
                                                        env->ExceptionClear();

                                                        if (getUnformatted)
                                                        {
                                                            jstring text = (jstring)env->CallObjectMethod(component, getUnformatted);
                                                            if (text)
                                                            {
                                                                const char* textStr = env->GetStringUTFChars(text, nullptr);
                                                                if (textStr)
                                                                {
                                                                    if (strstr(textStr, "1st Killer") != nullptr ||
                                                                        strstr(textStr, "Winner") != nullptr ||
                                                                        strstr(textStr, "You died!") != nullptr ||
                                                                        strstr(textStr, "by ") != nullptr)
                                                                    {
                                                                        bool sayGG = ((BooleanSetting*)FindSetting("GG"))->GetValue();
                                                                        bool sayGF = ((BooleanSetting*)FindSetting("GF"))->GetValue();

                                                                        if (sayGG)
                                                                        {
                                                                            jstring msg = env->NewStringUTF("gg");
                                                                            env->CallVoidMethod(player, sendChat, msg);
                                                                            env->DeleteLocalRef(msg);
                                                                        }

                                                                        if (sayGF)
                                                                        {
                                                                            jstring msg = env->NewStringUTF("gf");
                                                                            env->CallVoidMethod(player, sendChat, msg);
                                                                            env->DeleteLocalRef(msg);
                                                                        }

                                                                        m_Triggered = true;
                                                                    }
                                                                    env->ReleaseStringUTFChars(text, textStr);
                                                                }
                                                                env->DeleteLocalRef(text);
                                                            }
                                                        }
                                                        env->DeleteLocalRef(compClass);
                                                        env->DeleteLocalRef(component);
                                                    }
                                                }
                                                env->DeleteLocalRef(chatLineClass);
                                                env->DeleteLocalRef(chatLine);
                                            }
                                        }
                                        env->DeleteLocalRef(linesClass);
                                        env->DeleteLocalRef(lines);
                                    }
                                }
                                env->DeleteLocalRef(chatClass);
                                env->DeleteLocalRef(chatGui);
                            }
                        }
                        env->DeleteLocalRef(guiClass);
                        env->DeleteLocalRef(ingameGui);
                    }
                }
            }
            env->DeleteLocalRef(listClass);
            env->DeleteLocalRef(list);
        }
    }

    env->DeleteLocalRef(worldClass);
    env->DeleteLocalRef(playerClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

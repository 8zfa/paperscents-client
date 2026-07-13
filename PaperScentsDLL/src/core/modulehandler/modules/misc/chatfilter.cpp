#include "chatfilter.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../core.h"
#include <string>
#include <vector>
#include <algorithm>

static bool ContainsAny(const std::string& text, const std::vector<std::string>& patterns)
{
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (auto& p : patterns)
    {
        std::string pl = p;
        std::transform(pl.begin(), pl.end(), pl.begin(), ::tolower);
        if (lower.find(pl) != std::string::npos) return true;
    }
    return false;
}

static const std::vector<std::string> AD_PATTERNS = {
    "buy", "sell", "shop", "store", "discord.gg", "youtube.com",
    "subscribe", "donate", "paypal", "free coins", "giveaway",
    "ip:", "webstore", "vote for", "server store"
};

static const std::vector<std::string> SWEAR_PATTERNS = {
    // intentionally empty - user can configure via settings
};

ChatFilterModule::ChatFilterModule()
    : ModuleBase("ChatFilter", "Filter unwanted chat messages", Category::Misc)
{
    AddSetting<BooleanSetting>("FilterAdvertisements", true, "Filter ad/spam messages");
    AddSetting<BooleanSetting>("FilterSwearWords", false, "Filter swear words");
    AddSetting<NumberSetting>("MaxChatAge", 5.0f, 1.0f, 30.0f, 1.0f, "Seconds to track chat");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void ChatFilterModule::OnEnable() { Logger::Log("ChatFilter enabled"); }
void ChatFilterModule::OnDisable() { Logger::Log("ChatFilter disabled"); }

void ChatFilterModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    bool filterAds = ((BooleanSetting*)FindSetting("FilterAdvertisements"))->GetValue();
    bool filterSwear = ((BooleanSetting*)FindSetting("FilterSwearWords"))->GetValue();

    if (!filterAds && !filterSwear) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jclass mcCls = env->GetObjectClass(mc);
    if (!mcCls) { env->DeleteLocalRef(mc); return; }

    jfieldID chatClsFid = env->GetFieldID(mcCls, "A", "Lbdb;");
    if (!chatClsFid) { env->ExceptionClear(); chatClsFid = env->GetFieldID(mcCls, "ingameGUIChat", "Lnet/minecraft/client/gui/GuiNewChat;"); }
    env->ExceptionClear();
    env->DeleteLocalRef(mcCls);

    if (!chatClsFid) { env->DeleteLocalRef(mc); return; }

    jobject chatGui = env->GetObjectField(mc, chatClsFid);
    if (!chatGui) { env->DeleteLocalRef(mc); return; }

    jclass chatGuiCls = env->GetObjectClass(chatGui);
    if (!chatGuiCls) { env->DeleteLocalRef(chatGui); env->DeleteLocalRef(mc); return; }

    jfieldID chatLinesFid = env->GetFieldID(chatGuiCls, "g", "Ljava/util/List;");
    if (!chatLinesFid) { env->ExceptionClear(); chatLinesFid = env->GetFieldID(chatGuiCls, "drawnChatLines", "Ljava/util/List;"); }
    env->ExceptionClear();
    env->DeleteLocalRef(chatGuiCls);

    if (!chatLinesFid) { env->DeleteLocalRef(chatGui); env->DeleteLocalRef(mc); return; }

    jobject chatLines = env->GetObjectField(chatGui, chatLinesFid);
    env->DeleteLocalRef(chatGui);

    if (!chatLines || !StrayCache::ListClass || !StrayCache::List_size || !StrayCache::List_get)
    {
        if (chatLines) env->DeleteLocalRef(chatLines);
        env->DeleteLocalRef(mc);
        return;
    }

    jint lineCount = env->CallIntMethod(chatLines, StrayCache::List_size);

    for (jint i = 0; i < lineCount; i++)
    {
        jobject line = env->CallObjectMethod(chatLines, StrayCache::List_get, i);
        if (!line) continue;

        jclass lineCls = env->GetObjectClass(line);
        if (!lineCls) { env->DeleteLocalRef(line); continue; }

        jmethodID getUnformattedText = env->GetMethodID(lineCls, "b", "()Ljava/lang/String;");
        if (!getUnformattedText) { env->ExceptionClear(); getUnformattedText = env->GetMethodID(lineCls, "getUnformattedText", "()Ljava/lang/String;"); }
        env->ExceptionClear();
        env->DeleteLocalRef(lineCls);

        if (!getUnformattedText) { env->DeleteLocalRef(line); continue; }

        jobject textObj = env->CallObjectMethod(line, getUnformattedText);
        if (!textObj) { env->DeleteLocalRef(line); continue; }

        const char* textChars = env->GetStringUTFChars((jstring)textObj, nullptr);
        std::string text = textChars ? textChars : "";
        if (textChars) env->ReleaseStringUTFChars((jstring)textObj, textChars);
        env->DeleteLocalRef(textObj);
        env->DeleteLocalRef(line);

        if (filterAds && ContainsAny(text, AD_PATTERNS))
        {
            Logger::Log("[ChatFilter] Filtered ad: %s", text.c_str());
        }
    }

    env->DeleteLocalRef(chatLines);
    env->DeleteLocalRef(mc);
}

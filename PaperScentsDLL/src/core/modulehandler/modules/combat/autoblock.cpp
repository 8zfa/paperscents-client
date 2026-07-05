#include "autoblock.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <jni.h>
#include <cstdlib>

AutoBlockModule::AutoBlockModule()
    : ModuleBase("AutoBlock", "Automatically block when attacking", Category::Combat)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Single", "Custom", "Always"});
    AddSetting<NumberSetting>("BlockChance", 80.0f, 0.0f, 100.0f, 5.0f);
    m_LastBlock = std::chrono::steady_clock::now();
}

void AutoBlockModule::OnEnable() { Logger::Log("AutoBlock enabled"); }
void AutoBlockModule::OnDisable() { Logger::Log("AutoBlock disabled"); }

void AutoBlockModule::OnUpdate()
{
    if (!IsEnabled()) return;

    Java* java = Core::GetInstance().GetJava();
    if (!java || !java->IsValid()) return;
    JNIEnv* env = java->GetEnv();
    if (!env) return;

    jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) { env->ExceptionClear(); return; }

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    const char* gsFields[] = { "t", "field_71474_y", "gameSettings" };
    jfieldID gsf = nullptr;
    for (auto f : gsFields)
    {
        gsf = env->GetFieldID(mcClass, f, "Lavh;");
        if (!gsf) gsf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/settings/GameSettings;");
        if (gsf) break;
    }
    if (!gsf) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    jobject gs = env->GetObjectField(mc, gsf);
    if (!gs) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass gsClass = env->GetObjectClass(gs);

    const char* useItemFields[] = { "l", "field_74318_l", "keyBindUseItem" };
    jfieldID uif = nullptr;
    for (auto f : useItemFields)
    {
        uif = env->GetFieldID(gsClass, f, "Lavk;");
        if (!uif) uif = env->GetFieldID(gsClass, f, "Lnet/minecraft/client/settings/KeyBinding;");
        if (uif) break;
    }
    if (!uif) { env->DeleteLocalRef(gsClass); env->DeleteLocalRef(gs); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    jobject keyBind = env->GetObjectField(gs, uif);
    if (!keyBind) { env->DeleteLocalRef(gsClass); env->DeleteLocalRef(gs); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass kbClass = env->GetObjectClass(keyBind);
    jfieldID pressedField = nullptr;
    const char* pressedAlt[] = { "e", "field_74513_e", "pressed" };
    for (auto f : pressedAlt)
    {
        pressedField = env->GetFieldID(kbClass, f, "Z");
        if (pressedField) break;
    }
    if (!pressedField) { env->DeleteLocalRef(kbClass); env->DeleteLocalRef(keyBind); env->DeleteLocalRef(gsClass); env->DeleteLocalRef(gs); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();
    float blockChance = ((NumberSetting*)FindSetting("BlockChance"))->GetValue();

    bool attacking = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool shouldBlock = false;

    if (mode == 0)
    {
        shouldBlock = attacking;
    }
    else if (mode == 1)
    {
        shouldBlock = attacking && ((::rand() % 100) < blockChance);
    }
    else if (mode == 2)
    {
        shouldBlock = true;
    }

    auto now = std::chrono::steady_clock::now();
    bool cooldown = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastBlock).count() < 50;

    if (shouldBlock && !cooldown)
    {
        env->SetBooleanField(keyBind, pressedField, JNI_TRUE);
        m_LastBlock = now;
        m_WasBlocking = true;
    }
    else if (!shouldBlock && m_WasBlocking)
    {
        env->SetBooleanField(keyBind, pressedField, JNI_FALSE);
        m_WasBlocking = false;
    }

    env->DeleteLocalRef(kbClass);
    env->DeleteLocalRef(keyBind);
    env->DeleteLocalRef(gsClass);
    env->DeleteLocalRef(gs);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

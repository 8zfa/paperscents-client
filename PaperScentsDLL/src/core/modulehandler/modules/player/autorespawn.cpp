#include "autorespawn.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"

AutoRespawnModule::AutoRespawnModule()
    : ModuleBase("AutoRespawn", "Auto respawn on death", Category::Player)
{
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void AutoRespawnModule::OnEnable() { Logger::Log("AutoRespawn enabled"); }
void AutoRespawnModule::OnDisable() { Logger::Log("AutoRespawn disabled"); }

void AutoRespawnModule::OnUpdate()
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
    jobject mc = nullptr;

    if (!player)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java || !java->IsValid()) return;
        jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
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
    else
    {
        mc = BridgeHelper::GetMinecraftInstance(env);
    }

    jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
    jfieldID isDeadField = nullptr;
    if (entityClass)
    {
        isDeadField = env->GetFieldID(entityClass, "isDead", "Z");
        if (!isDeadField) { env->ExceptionClear(); isDeadField = env->GetFieldID(entityClass, "J", "Z"); }
        env->ExceptionClear();
    }
    else
    {
        env->ExceptionClear();
        // Try notch name
        jclass entityClsNotch = env->FindClass("pk");
        if (entityClsNotch)
        {
            isDeadField = env->GetFieldID(entityClsNotch, "J", "Z");
            env->DeleteLocalRef(entityClsNotch);
        }
    }

    bool isDead = false;
    if (isDeadField)
        isDead = env->GetBooleanField(player, isDeadField) != 0;

    if (isDead && mc)
    {
        jclass mcCls = env->GetObjectClass(mc);
        jmethodID displayScreen = env->GetMethodID(mcCls, "displayGuiScreen", "(Lnet/minecraft/client/gui/GuiScreen;)V");
        if (!displayScreen) { env->ExceptionClear(); displayScreen = env->GetMethodID(mcCls, "a", "(Lbik;)V"); }
        env->ExceptionClear();
        if (displayScreen)
            env->CallVoidMethod(mc, displayScreen, nullptr);
        env->DeleteLocalRef(mcCls);
    }

    if (entityClass) env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    if (!bridged && mc) env->DeleteLocalRef(mc);
}

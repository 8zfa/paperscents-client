#include "criticals.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <jni.h>

CriticalsModule::CriticalsModule()
    : ModuleBase("Criticals", "Always land critical hits", Category::Combat)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Packet", "MiniJump", "Position"});
    m_LastCrit = std::chrono::steady_clock::now();
}

void CriticalsModule::OnEnable() { Logger::Log("Criticals enabled"); }
void CriticalsModule::OnDisable() { Logger::Log("Criticals disabled"); }

void CriticalsModule::OnUpdate()
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

    const char* playerFields[] = { "u", "field_71439_g", "thePlayer" };
    jfieldID pf = nullptr;
    for (auto f : playerFields)
    {
        pf = env->GetFieldID(mcClass, f, "Lbew;");
        if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
        if (pf) break;
    }
    if (!pf) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    jobject player = env->GetObjectField(mc, pf);
    if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass entityClass = java->FindClass("pk", "net/minecraft/entity/Entity");
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jfieldID onGroundField = env->GetFieldID(entityClass, "C", "Z");
    jfieldID motionYField = env->GetFieldID(entityClass, "w", "D");
    jfieldID posYField = env->GetFieldID(entityClass, "t", "D");

    if (!onGroundField || !motionYField || !posYField)
    {
        // Try alternative field names
        const char* ogAlt[] = { "E", "onGround" };
        for (auto a : ogAlt) { onGroundField = env->GetFieldID(entityClass, a, "Z"); if (onGroundField) break; }
        const char* myAlt[] = { "motionY" };
        for (auto a : myAlt) { motionYField = env->GetFieldID(entityClass, a, "D"); if (motionYField) break; }
        const char* pyAlt[] = { "posY" };
        for (auto a : pyAlt) { posYField = env->GetFieldID(entityClass, a, "D"); if (posYField) break; }
    }

    if (!onGroundField) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(player); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jboolean onGround = env->GetBooleanField(player, onGroundField);

    bool attacking = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    auto now = std::chrono::steady_clock::now();
    bool cooldown = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastCrit).count() < 200;

    if (onGround && attacking && !m_AppliedThisTick && !cooldown)
    {
        int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();

        if (mode == 0)
        {
            if (posYField && motionYField)
            {
                double currentY = env->GetDoubleField(player, posYField);
                env->SetDoubleField(player, posYField, currentY + 0.0625);
                env->SetBooleanField(player, onGroundField, JNI_FALSE);
                env->SetDoubleField(player, motionYField, -0.1);
            }
        }
        else if (mode == 1)
        {
            if (motionYField)
            {
                env->SetDoubleField(player, motionYField, 0.2);
                env->SetBooleanField(player, onGroundField, JNI_FALSE);
            }
        }
        else if (mode == 2)
        {
            if (posYField)
            {
                double currentY = env->GetDoubleField(player, posYField);
                env->SetDoubleField(player, posYField, currentY + 0.0625);
                env->SetBooleanField(player, onGroundField, JNI_FALSE);
            }
        }

        m_AppliedThisTick = true;
    }
    else if (!onGround && m_AppliedThisTick)
    {
        m_LastCrit = now;
        m_AppliedThisTick = false;
    }
    else if (onGround)
    {
        m_AppliedThisTick = false;
    }

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

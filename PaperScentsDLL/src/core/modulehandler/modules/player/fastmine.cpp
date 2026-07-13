#include "fastmine.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"

FastMineModule::FastMineModule()
    : ModuleBase("FastMine", "Break blocks faster", Category::Player)
{
    AddSetting<NumberSetting>("Speed", 1.5f, 1.0f, 3.0f, 0.1f, "Block break speed multiplier");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void FastMineModule::OnEnable() { Logger::Log("FastMine enabled"); }
void FastMineModule::OnDisable() { Logger::Log("FastMine disabled"); }

void FastMineModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

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

    jfieldID lcField = env->GetFieldID(mcClass, "i", "I");
    if (!lcField) { env->ExceptionClear(); lcField = env->GetFieldID(mcClass, "leftClickCounter", "I"); }
    env->ExceptionClear();

    if (lcField)
    {
        int counter = env->GetIntField(mc, lcField);
        float speed = ((NumberSetting*)FindSetting("Speed"))->GetValue();
        if (counter > 0 && speed > 0.0f)
        {
            env->SetIntField(mc, lcField, 0);
        }
    }

    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

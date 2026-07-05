#include "fastplace.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"

FastPlaceModule::FastPlaceModule()
    : ModuleBase("FastPlace", "Place blocks faster", Category::Player)
{
    AddSetting<NumberSetting>("Delay", 0.0f, 0.0f, 4.0f, 1.0f, "Tick delay between placements");
}

void FastPlaceModule::OnEnable()
{
    Logger::Log("FastPlace enabled");
    m_Cached = false;
}

void FastPlaceModule::OnDisable()
{
    Logger::Log("FastPlace disabled");
}

void FastPlaceModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    if (!m_Cached)
    {
        m_MinecraftClass = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
        if (!m_MinecraftClass)
        {
            env->ExceptionClear();
            m_MinecraftClass = env->FindClass("net/minecraft/client/Minecraft");
        }
        if (m_MinecraftClass)
        {
            m_MinecraftClass = (jclass)env->NewGlobalRef(m_MinecraftClass);
            m_GetMinecraft = env->GetStaticMethodID(m_MinecraftClass, "A", "()Lave;");
            if (!m_GetMinecraft)
            {
                env->ExceptionClear();
                m_GetMinecraft = env->GetStaticMethodID(m_MinecraftClass, "a", "()Lave;");
            }
            if (!m_GetMinecraft)
            {
                env->ExceptionClear();
                m_GetMinecraft = env->GetStaticMethodID(m_MinecraftClass, "getMinecraft", "()Lave;");
            }
            env->ExceptionClear();

            m_TimerField = env->GetFieldID(m_MinecraftClass, "e", "I");
            if (!m_TimerField)
            {
                env->ExceptionClear();
                m_TimerField = env->GetFieldID(m_MinecraftClass, "rightClickDelayTimer", "I");
            }
            env->ExceptionClear();
        }
        m_Cached = true;
    }

    if (!m_GetMinecraft || !m_TimerField) return;

    jobject mc = env->CallStaticObjectMethod(m_MinecraftClass, m_GetMinecraft);
    if (!mc) return;

    env->SetIntField(mc, m_TimerField, (int)((NumberSetting*)FindSetting("Delay"))->GetValue());
    env->DeleteLocalRef(mc);
}

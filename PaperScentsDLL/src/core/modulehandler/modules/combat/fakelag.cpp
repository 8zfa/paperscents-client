#include "fakelag.h"
#include "../../../utilities/logger.h"
#include "../../../java/java.h"
#include "../../../utilities/jni_helpers.h"
#include <Windows.h>
#include <algorithm>

FakeLagModule::FakeLagModule()
    : ModuleBase("FakeLag", "Artificially delays packets to create lag effect", Category::Combat), m_Lagging(false)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Toggle", "Hold"}, "Lag mode");
    AddSetting<NumberSetting>("Delay", 5.0f, 1.0f, 20.0f, 1.0f, "Ticks between lag cycles");
    m_LagTimer = std::chrono::steady_clock::now();
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void FakeLagModule::OnEnable() { Logger::Log("FakeLag enabled"); m_Lagging = false; m_LagTimer = std::chrono::steady_clock::now(); }
void FakeLagModule::OnDisable() { Logger::Log("FakeLag disabled"); m_Lagging = false; }

void FakeLagModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    auto now = std::chrono::steady_clock::now();
    int delay = (int)((NumberSetting*)FindSetting("Delay"))->GetValue();
    int delayMs = (std::max)(50, delay * 50);

    EnumSetting* mode = (EnumSetting*)FindSetting("Mode");
    bool isHold = (mode && mode->GetSelected() == "Hold");

    bool shouldLag = false;
    if (isHold)
    {
        shouldLag = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) || (GetAsyncKeyState(VK_RBUTTON) & 0x8000);
    }
    else
    {
        shouldLag = true;
    }

    if (shouldLag)
    {
        if (!m_Lagging)
        {
            m_Lagging = true;
            m_LagTimer = now;
        }

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LagTimer).count() < delayMs)
        {
            jclass entityClass = env->GetObjectClass(player);
            if (entityClass)
            {
                jfieldID motionX = env->GetFieldID(entityClass, "v", "D");
                if (!motionX) { env->ExceptionClear(); motionX = env->GetFieldID(entityClass, "motionX", "D"); env->ExceptionClear(); }
                jfieldID motionY = env->GetFieldID(entityClass, "w", "D");
                if (!motionY) { env->ExceptionClear(); motionY = env->GetFieldID(entityClass, "motionY", "D"); env->ExceptionClear(); }
                jfieldID motionZ = env->GetFieldID(entityClass, "x", "D");
                if (!motionZ) { env->ExceptionClear(); motionZ = env->GetFieldID(entityClass, "motionZ", "D"); env->ExceptionClear(); }

                if (motionX && motionY && motionZ)
                {
                    env->SetDoubleField(player, motionX, 0.0);
                    env->SetDoubleField(player, motionY, 0.0);
                    env->SetDoubleField(player, motionZ, 0.0);
                }
                env->DeleteLocalRef(entityClass);
            }
        }
        else
        {
            m_Lagging = false;
        }
    }
    else
    {
        m_Lagging = false;
    }

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

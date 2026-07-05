#include "zoom.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <Windows.h>

ZoomModule::ZoomModule()
    : ModuleBase("Zoom", "Zoom in with keybind", Category::Misc)
{
    AddSetting<NumberSetting>("Factor", 3.0f, 1.0f, 10.0f, 0.5f, "Zoom multiplier");
}

void ZoomModule::OnEnable()
{
    Logger::Log("Zoom enabled");
    m_Cached = false;
    m_WasZooming = false;
}

void ZoomModule::OnDisable()
{
    Logger::Log("Zoom disabled");
    if (m_WasZooming)
    {
        JNIEnv* env = Java::GetThreadEnv();
        if (env && StrayCache::MinecraftInstance)
        {
            jclass mcCls = env->GetObjectClass(StrayCache::MinecraftInstance);
            if (mcCls)
            {
                jfieldID gsField = env->GetFieldID(mcCls, "t", "Lavh;");
                if (!gsField) { env->ExceptionClear(); gsField = env->GetFieldID(mcCls, "gameSettings", "Lnet/minecraft/client/settings/GameSettings;"); }
                env->ExceptionClear();
                if (gsField)
                {
                    jobject gameSettings = env->GetObjectField(StrayCache::MinecraftInstance, gsField);
                    if (gameSettings)
                    {
                        jclass gsCls = env->GetObjectClass(gameSettings);
                        jfieldID fovField = env->GetFieldID(gsCls, "v", "F");
                        if (!fovField) { env->ExceptionClear(); fovField = env->GetFieldID(gsCls, "fovSetting", "F"); }
                        env->ExceptionClear();
                        if (fovField)
                            env->SetFloatField(gameSettings, fovField, m_OriginalFov);
                        env->DeleteLocalRef(gsCls);
                        env->DeleteLocalRef(gameSettings);
                    }
                }
                env->DeleteLocalRef(mcCls);
            }
        }
        m_WasZooming = false;
    }
}

void ZoomModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jclass mcClass = env->GetObjectClass(mc);
    jfieldID gsField = env->GetFieldID(mcClass, "t", "Lavh;");
    if (!gsField) { env->ExceptionClear(); gsField = env->GetFieldID(mcClass, "gameSettings", "Lnet/minecraft/client/settings/GameSettings;"); }
    env->ExceptionClear();

    if (!gsField) { env->DeleteLocalRef(mcClass); env->DeleteLocalRef(mc); return; }

    jobject gameSettings = env->GetObjectField(mc, gsField);
    if (!gameSettings) { env->DeleteLocalRef(mcClass); env->DeleteLocalRef(mc); return; }

    jclass gsClass = env->GetObjectClass(gameSettings);
    jfieldID fovField = env->GetFieldID(gsClass, "v", "F");
    if (!fovField) { env->ExceptionClear(); fovField = env->GetFieldID(gsClass, "fovSetting", "F"); }
    env->ExceptionClear();

    if (!fovField) { env->DeleteLocalRef(gsClass); env->DeleteLocalRef(gameSettings); env->DeleteLocalRef(mcClass); env->DeleteLocalRef(mc); return; }

    bool keyDown = (GetAsyncKeyState(GetKeybind()) & 0x8000) != 0;
    if (GetKeybind() == 0) keyDown = false;

    if (keyDown)
    {
        if (!m_Cached)
        {
            m_OriginalFov = env->GetFloatField(gameSettings, fovField);
            m_Cached = true;
        }

        float factor = ((NumberSetting*)FindSetting("Factor"))->GetValue();
        if (factor > 0.0f)
            env->SetFloatField(gameSettings, fovField, m_OriginalFov / factor);
        m_WasZooming = true;
    }
    else if (m_WasZooming)
    {
        env->SetFloatField(gameSettings, fovField, m_OriginalFov);
        m_WasZooming = false;
        m_Cached = false;
    }

    env->DeleteLocalRef(gsClass);
    env->DeleteLocalRef(gameSettings);
    env->DeleteLocalRef(mcClass);
    env->DeleteLocalRef(mc);
}

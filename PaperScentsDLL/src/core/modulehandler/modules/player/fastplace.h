#pragma once
#include "../../modulebase.h"
#include <jni.h>

class FastPlaceModule : public ModuleBase
{
public:
    FastPlaceModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    jfieldID m_CachedTimerID = nullptr;
    jclass m_MinecraftClass = nullptr;
    jmethodID m_GetMinecraft = nullptr;
    jfieldID m_TimerField = nullptr;
    bool m_Cached = false;
};

#pragma once
#include "../../modulebase.h"
#include <jni.h>

class ChestStealerModule : public ModuleBase
{
public:
    ChestStealerModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    void ShiftClick(JNIEnv* env, jobject player, jobject controller, int windowId, int slotId);
    int m_ClickDelay = 0;
    int m_OpenDelay = 0;
    bool m_InChest = false;
    bool m_WarnedFull = false;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

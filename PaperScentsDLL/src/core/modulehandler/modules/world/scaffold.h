#pragma once
#include "../../modulebase.h"
#include <jni.h>
#include <chrono>

class ScaffoldModule : public ModuleBase
{
public:
    ScaffoldModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
    int FindBlockSlot(JNIEnv* env, jobject inventory);
    bool PlaceBlock(JNIEnv* env, jobject mc, jobject player, jobject world, int blockX, int blockY, int blockZ);
    std::chrono::steady_clock::time_point m_LastPlace;
    int m_PlaceTicks = 0;
    int m_LastBlockSlot = -1;
    bool m_Towering = false;
    int m_TowerTicks = 0;
};

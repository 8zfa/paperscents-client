#pragma once
#include "../../modulebase.h"
#include <jni.h>
#include <chrono>

class KillAuraModule : public ModuleBase
{
public:
    KillAuraModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    bool IsValidEntity(JNIEnv* env, jobject entity, jobject player, float range, int fov, bool throughWalls);
    void Attack(JNIEnv* env, jobject entity);
    void Block(JNIEnv* env);
    void Unblock();
    std::chrono::steady_clock::time_point m_LastAttack;
    bool m_Blocking = false;
    int m_CurrentTargetId = -1;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

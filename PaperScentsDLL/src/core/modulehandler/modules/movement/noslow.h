#pragma once
#include "../../modulebase.h"
#include <jni.h>

class NoSlowModule : public ModuleBase
{
public:
    NoSlowModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    bool IsHoldingSword(JNIEnv* env, jobject player);
    bool IsEating(JNIEnv* env, jobject player);
    bool IsUsingBow(JNIEnv* env, jobject player);
    bool IsFishingRod(JNIEnv* env, jobject player);
    bool IsUsingItem(JNIEnv* env, jobject player);
    static jobject GetHeldItem(JNIEnv* env, jobject player);
    int m_LastSlot = -1;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};

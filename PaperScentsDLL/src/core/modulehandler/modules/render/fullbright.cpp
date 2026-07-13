#include "fullbright.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../sdk/bridgeHelper.h"
#include <cmath>

FullBrightModule::FullBrightModule()
    : ModuleBase("FullBright", "Full brightness", Category::Render)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Gamma", "Effect"});
    AddSetting<NumberSetting>("Gamma", 100.0f, 1.0f, 100.0f, 1.0f);
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void FullBrightModule::OnEnable()
{
    Logger::Log("FullBright enabled");
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();
    if (mode == 0)
    {
        SetGamma();
    }
    else
    {
        jobject player = BridgeHelper::GetPlayer(env);
        if (player)
        {
            ApplyNightVision(env, player);
            env->DeleteLocalRef(player);
        }
    }
}

void FullBrightModule::OnDisable()
{
    Logger::Log("FullBright disabled");
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();
    if (mode == 0)
    {
        if (!std::isnan(m_PrevGamma) && BridgeHelper::Initialize(env) && BridgeHelper::GSBridge_SetGamma)
        {
            jobject gs = BridgeHelper::GetGameSettings(env);
            if (gs) { env->CallVoidMethod(gs, BridgeHelper::GSBridge_SetGamma, m_PrevGamma); env->DeleteLocalRef(gs); }
            m_PrevGamma = NAN;
        }
    }
    else
    {
        if (m_AppliedNightVision)
        {
            jobject player = BridgeHelper::GetPlayer(env);
            if (player)
            {
                RemoveNightVision(env, player);
                env->DeleteLocalRef(player);
            }
            m_AppliedNightVision = false;
        }
    }
}

void FullBrightModule::SetGamma()
{
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!BridgeHelper::Initialize(env) || !BridgeHelper::GSBridge_SetGamma) return;

    float val = ((NumberSetting*)FindSetting("Gamma"))->GetValue();
    jobject gs = BridgeHelper::GetGameSettings(env);
    if (!gs) return;
    if (std::isnan(m_PrevGamma)) m_PrevGamma = val;
    env->CallVoidMethod(gs, BridgeHelper::GSBridge_SetGamma, val);
    env->DeleteLocalRef(gs);
}

void FullBrightModule::ApplyNightVision(JNIEnv* env, jobject player)
{
    if (!player) return;
    jclass potionClass = env->FindClass("net/minecraft/potion/Potion");
    if (!potionClass) { env->ExceptionClear(); return; }

    jfieldID nvField = env->GetStaticFieldID(potionClass, "nightVision", "Lnet/minecraft/potion/Potion;");
    if (!nvField) { env->ExceptionClear(); env->DeleteLocalRef(potionClass); return; }
    env->ExceptionClear();
    jobject nightVision = env->GetStaticObjectField(potionClass, nvField);
    if (!nightVision) { env->DeleteLocalRef(potionClass); return; }

    jmethodID potionGetId = env->GetMethodID(potionClass, "getId", "()I");
    jint nvId = 0;
    if (potionGetId) { env->ExceptionClear(); nvId = env->CallIntMethod(nightVision, potionGetId); }

    jclass potionEffectClass = env->FindClass("net/minecraft/potion/PotionEffect");
    if (potionEffectClass)
    {
        jmethodID peInit = env->GetMethodID(potionEffectClass, "<init>", "(III)V");
        if (peInit)
        {
            env->ExceptionClear();
            jobject effect = env->NewObject(potionEffectClass, peInit, nvId, 25940, 0);
            if (effect)
            {
                jclass elClass = env->FindClass("net/minecraft/entity/EntityLivingBase");
                if (elClass)
                {
                    jmethodID addEffect = env->GetMethodID(elClass, "addPotionEffect", "(Lnet/minecraft/potion/PotionEffect;)V");
                    if (addEffect)
                    {
                        env->ExceptionClear();
                        env->CallVoidMethod(player, addEffect, effect);
                        m_AppliedNightVision = true;
                    }
                    env->DeleteLocalRef(elClass);
                }
                env->DeleteLocalRef(effect);
            }
        }
        env->DeleteLocalRef(potionEffectClass);
    }

    env->DeleteLocalRef(nightVision);
    env->DeleteLocalRef(potionClass);
}

void FullBrightModule::RemoveNightVision(JNIEnv* env, jobject player)
{
    if (!player) return;
    jclass elClass = env->FindClass("net/minecraft/entity/EntityLivingBase");
    if (!elClass) { env->ExceptionClear(); return; }
    jmethodID removeEffect = env->GetMethodID(elClass, "removePotionEffectClient", "(I)V");
    if (removeEffect)
    {
        env->ExceptionClear();
        env->CallVoidMethod(player, removeEffect, 16);
    }
    env->DeleteLocalRef(elClass);
}

void FullBrightModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();
    if (mode == 0) SetGamma();
}

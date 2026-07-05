#pragma once
#include <jni.h>
#include "../core.h"

inline jobject GetMinecraftObject(JNIEnv* env)
{
    if (!env) return nullptr;
    Java* java = Core::GetInstance().GetJava();
    if (!java) return nullptr;
    jclass cls = java->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!cls) return nullptr;
    jmethodID getMc = env->GetStaticMethodID(cls, "A", "()Lave;");
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(cls, "getMinecraft", "()Lnet/minecraft/client/Minecraft;"); }
    if (!getMc) { env->DeleteLocalRef(cls); return nullptr; }
    env->ExceptionClear();
    jobject mc = env->CallStaticObjectMethod(cls, getMc);
    env->DeleteLocalRef(cls);
    return mc;
}

inline jobject GetPlayerObject(JNIEnv* env, jobject mc)
{
    if (!env || !mc) return nullptr;
    jclass mcCls = env->GetObjectClass(mc);
    if (!mcCls) return nullptr;
    const char* playerFields[] = { "u", "thePlayer", "h" };
    jfieldID pf = nullptr;
    for (auto f : playerFields)
    {
        pf = env->GetFieldID(mcCls, f, "Lbew;");
        if (!pf) { env->ExceptionClear(); pf = env->GetFieldID(mcCls, f, "Lnet/minecraft/client/entity/EntityPlayerSP;"); }
        if (pf) break;
        env->ExceptionClear();
    }
    jobject player = pf ? env->GetObjectField(mc, pf) : nullptr;
    env->DeleteLocalRef(mcCls);
    return player;
}

inline jobject GetWorldObject(JNIEnv* env, jobject mc)
{
    if (!env || !mc) return nullptr;
    jclass mcCls = env->GetObjectClass(mc);
    if (!mcCls) return nullptr;
    const char* worldFields[] = { "f", "theWorld", "field_71441_e" };
    jfieldID wf = nullptr;
    for (auto f : worldFields)
    {
        wf = env->GetFieldID(mcCls, f, "Ladm;");
        if (!wf) { env->ExceptionClear(); wf = env->GetFieldID(mcCls, f, "Lnet/minecraft/world/World;"); }
        if (wf) break;
        env->ExceptionClear();
    }
    jobject world = wf ? env->GetObjectField(mc, wf) : nullptr;
    env->DeleteLocalRef(mcCls);
    return world;
}
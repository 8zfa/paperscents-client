#pragma once
#include <jni.h>
#include "net/minecraft/client/Minecraft.h"
#include "net/minecraft/client/entity/EntityPlayerSP.h"
#include "net/minecraft/entity/Entity.h"
#include "net/minecraft/entity/EntityLivingBase.h"
#include "net/minecraft/entity/player/EntityPlayer.h"
#include "net/minecraft/world/World.h"
#include "net/minecraft/client/settings/GameSettings.h"
#include "net/minecraft/util/Vec3.h"
#include "java/lang/String.h"
#include "java/util/List.h"

class SDK
{
public:
    static SDK& GetInstance();

    bool Init(JNIEnv* env);
    void Shutdown();

    JNIEnv* GetEnv() { return m_Env; }
    Minecraft* GetMinecraft() { return m_Minecraft; }
    EntityPlayerSP* GetPlayer() { return m_Player; }

    jclass FindClass(const char* className);

private:
    SDK() = default;
    ~SDK() = default;
    SDK(const SDK&) = delete;
    SDK& operator=(const SDK&) = delete;

    JNIEnv* m_Env = nullptr;
    Minecraft* m_Minecraft = nullptr;
    EntityPlayerSP* m_Player = nullptr;
};

#pragma once
#include <jni.h>

struct BridgeHelper
{
    static jclass MinecraftBridge;
    static jclass EntityBridge;
    static jclass EntityLivingBaseBridge;
    static jclass EntityPlayerSPBridge;
    static jclass EntityMobBridge;
    static jclass EntityAnimalBridge;
    static jclass WorldBridge;
    static jclass WorldClientBridge;
    static jclass GameSettingsBridge;
    static jclass PlayerControllerBridge;
    static jclass NetHandlerPlayClientBridge;

    static jmethodID McBridge_GetPlayer;
    static jmethodID McBridge_GetPlayerController;
    static jmethodID McBridge_GetGameSettings;
    static jmethodID McBridge_GetObjectMouseOver;
    static jmethodID McBridge_GetPointedEntity;
    static jmethodID McBridge_GetRenderGlobal;

    static jmethodID EntityBridge_GetPosX;
    static jmethodID EntityBridge_GetPosY;
    static jmethodID EntityBridge_GetPosZ;
    static jmethodID EntityBridge_GetMotionX;
    static jmethodID EntityBridge_GetMotionY;
    static jmethodID EntityBridge_GetMotionZ;
    static jmethodID EntityBridge_GetRotationYaw;
    static jmethodID EntityBridge_GetRotationPitch;
    static jmethodID EntityBridge_GetEntityId;
    static jmethodID EntityBridge_GetWorld;
    static jmethodID EntityBridge_IsOnGround;
    static jmethodID EntityBridge_IsSneaking;
    static jmethodID EntityBridge_IsInvisible;
    static jmethodID EntityBridge_IsDead;
    static jmethodID EntityBridge_GetWidth;
    static jmethodID EntityBridge_GetEyeHeight;
    static jmethodID EntityBridge_GetFallDistance;
    static jmethodID EntityBridge_GetBoundingBox;
    static jmethodID EntityBridge_SetPosX;
    static jmethodID EntityBridge_SetPosY;
    static jmethodID EntityBridge_SetPosZ;
    static jmethodID EntityBridge_SetMotionX;
    static jmethodID EntityBridge_SetMotionY;
    static jmethodID EntityBridge_SetMotionZ;
    static jmethodID EntityBridge_SetRotationYaw;
    static jmethodID EntityBridge_SetRotationPitch;
    static jmethodID EntityBridge_GetDistanceToEntity;
    static jmethodID EntityBridge_GetDistanceSqToEntity;

    static jmethodID ItemStackBridge_GetItem;

    static jmethodID ELBridge_GetHealth;
    static jmethodID ELBridge_GetHurtTime;
    static jmethodID ELBridge_GetDeathTime;
    static jmethodID ELBridge_GetMaxHealth;
    static jmethodID ELBridge_IsOnLadder;
    static jmethodID ELBridge_IsInWater;
    static jmethodID ELBridge_GetLastAttackedMillis;
    static jmethodID ELBridge_GetLastHurtMillis;
    static jmethodID ELBridge_GetActivePotionEffects;
    static jmethodID ELBridge_GetHeldItem;
    static jmethodID ELBridge_IsUsingItem;

    static jclass ItemSwordBridge;
    static jclass ItemBowBridge;
    static jclass ItemFoodBridge;
    static jclass ItemStackBridge;

    static jmethodID EPSPBridge_SetSprinting;
    static jmethodID EPSPBridge_GetSprintingTicksLeft;
    static jmethodID EPSPBridge_SetMovementInput;
    static jmethodID EPSPBridge_GetMovementInput;
    static jmethodID EPSPBridge_OnCriticalHit;
    static jmethodID EPSPBridge_SendChatMessage;

    static jmethodID WorldBridge_GetEntities;
    static jmethodID WorldBridge_GetPlayerEntities;
    static jmethodID WorldBridge_IsRemote;

    static jmethodID GSBridge_KeyBindAttack;
    static jmethodID GSBridge_KeyBindUseItem;
    static jmethodID GSBridge_KeyBindForward;
    static jmethodID GSBridge_KeyBindBack;
    static jmethodID GSBridge_KeyBindSneak;
    static jmethodID GSBridge_KeyBindJump;
    static jmethodID GSBridge_KeyBindSprint;
    static jmethodID GSBridge_KeyBindLeft;
    static jmethodID GSBridge_KeyBindRight;
    static jmethodID GSBridge_SetKeyBindState;
    static jmethodID GSBridge_SetGamma;
    static jmethodID GSBridge_UnpressAllKeys;

    static jmethodID NHPBridge_AddToSendQueue;

    static jmethodID PCBridge_IsSpectator;

    // --- Core initialization ---
    static bool Initialize(JNIEnv* env);

    // --- Minecraft singleton (works on any obfuscation via reflection) ---
    static jobject GetMinecraftInstance(JNIEnv* env);

    // --- Self-contained convenience helpers (no mc param needed) ---
    static jobject GetPlayer(JNIEnv* env);
    static jobject GetPlayerController(JNIEnv* env);
    static jobject GetGameSettings(JNIEnv* env);
    static jobject GetWorldFromEntity(JNIEnv* env, jobject entity);

    // --- Keybind helper: simulate attack via GameSettings keybind ---
    static void SimulateAttack(JNIEnv* env);
};

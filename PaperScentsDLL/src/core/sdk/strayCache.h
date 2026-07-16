#pragma once
#include <jni.h>
#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>

class StrayCache
{
public:
    static StrayCache& GetInstance();

    void Initialize(JNIEnv* env);

    // Cached global refs for key game classes
    static jclass Minecraft;
    static jobject MinecraftInstance;
    static jclass World;
    static jclass Entity;
    static jclass EntityLivingBase;
    static jclass EntityPlayer;
    static jclass EntityPlayerSP;

    static jclass ListClass;
    static jclass FloatBufferClass;
    static jclass ActiveRenderInfoClass;
    static jclass RenderManagerClass;
    static jclass GameSettingsClass;
    static jclass PlayerControllerMP;

    // Cached field IDs - Entity
    static jfieldID Entity_posX;
    static jfieldID Entity_posY;
    static jfieldID Entity_posZ;
    static jfieldID Entity_width;
    static jfieldID Entity_height;
    static jfieldID Entity_isDead;
    static jfieldID Entity_motionX;
    static jfieldID Entity_motionY;
    static jfieldID Entity_motionZ;
    static jfieldID Entity_rotationYaw;
    static jfieldID Entity_rotationPitch;
    static jfieldID Entity_onGround;
    static jfieldID Entity_fallDistance;

    // Cached method IDs - Entity
    static jmethodID Entity_getEntityId;
    static jmethodID Entity_getName;
    static jmethodID Entity_setSprinting;
    static jmethodID Entity_isSprinting;
    static jmethodID Entity_isSneaking;
    static jmethodID Entity_getDistanceToEntity;
    static jmethodID Entity_setRotationYaw;
    static jmethodID Entity_setRotationPitch;

    // Cached field IDs - PlayerControllerMP
    static jfieldID PC_blockReachDistance;

    // Cached method IDs - List
    static jmethodID List_size;
    static jmethodID List_get;

    // Cached method IDs - FloatBuffer
    static jmethodID FloatBuffer_get;

    // Cached field IDs - ActiveRenderInfo
    static jfieldID ActiveRenderInfo_PROJECTION;
    static jfieldID ActiveRenderInfo_MODELVIEW;

    // Cached field IDs - World
    static jfieldID World_loadedEntityList;
    static jfieldID World_playerEntities;

    // Cached field IDs - Minecraft
    static jfieldID Minecraft_thePlayer;
    static jfieldID Minecraft_theWorld;
    static jfieldID Minecraft_renderManager;
    static jfieldID Minecraft_playerController;
    static jfieldID Minecraft_objectMouseOver;
    static jfieldID Minecraft_gameSettings;
    static jfieldID Minecraft_rightClickDelayTimer;

    // Cached field IDs - RenderManager
    static jfieldID RenderManager_viewerPosX;
    static jfieldID RenderManager_viewerPosY;
    static jfieldID RenderManager_viewerPosZ;

    // Cached method IDs - EntityLivingBase
    static jmethodID EntityLivingBase_getHealth;
    static jmethodID EntityLivingBase_getMaxHealth;
    static jfieldID EntityLivingBase_hurtTime;
    static jfieldID EntityLivingBase_deathTime;
    static jmethodID EntityLivingBase_isUsingItem;
    static jmethodID EntityLivingBase_getHeldItem;

    // Cached method IDs - ItemStack
    static jmethodID ItemStack_getItem;

    // Cached class IDs - Item
    static jclass ItemSwordClass;

    // Cached field IDs - EntityPlayer
    static jfieldID EntityPlayer_inventory;
    static jfieldID EntityPlayer_movementInput;

    // Cached field IDs - GameSettings
    static jfieldID GameSettings_gammaSetting;
    static jfieldID GameSettings_keyBindForward;
    static jfieldID GameSettings_keyBindJump;
    static jfieldID GameSettings_keyBindSprint;
    static jfieldID GameSettings_keyBindSneak;

private:
    StrayCache() = default;
    ~StrayCache() = default;
    StrayCache(const StrayCache&) = delete;
    StrayCache& operator=(const StrayCache&) = delete;

    JNIEnv* m_Env = nullptr;
    std::unordered_map<std::string, jclass> m_Classes;

    // Mapping tables for lunar mappings
    static std::unordered_map<std::string, std::string> s_classMap;
    static std::unordered_map<std::string, std::string> s_fieldMap;
    static std::unordered_map<std::string, std::string> s_methodMap;
    // Reverse mappings (human-readable -> obfuscated)
    static std::unordered_map<std::string, std::string> s_classMapRev;
    static std::unordered_map<std::string, std::string> s_fieldMapRev;
    static std::unordered_map<std::string, std::string> s_methodMapRev;

    static bool LoadLunarMappings(const std::string& filename);
    static std::string GetObfuscatedClassName(const std::string& className);
    static std::string GetObfuscatedFieldName(const std::string& classDescriptor,
                                             const std::string& fieldName,
                                             const std::string& fieldDescriptor);
    static std::string GetObfuscatedMethodName(const std::string& classDescriptor,
                                              const std::string& methodName,
                                              const std::string& methodDescriptor);

    jclass GetClass(const std::string& name);
    jclass GetClassFromHuman(const std::string& humanClassName);
    jfieldID GetFieldID(const std::string& humanClassName, const std::string& fieldName, const std::string& fieldDescriptor);
    jfieldID GetStaticFieldID(const std::string& humanClassName, const std::string& fieldName, const std::string& fieldDescriptor);
    jmethodID GetMethodID(const std::string& humanClassName, const std::string& methodName, const std::string& methodDescriptor);
    jmethodID GetStaticMethodID(const std::string& humanClassName, const std::string& methodName, const std::string& methodDescriptor);

    static void TryField(JNIEnv* env, jclass cls, jfieldID& out, const char* obf, const char* deobf, const char* sig);
    static void TryField2(JNIEnv* env, jclass cls, jfieldID& out, const char* obf, const char* deobf, const char* obfSig, const char* deobfSig);
    static void TryMethod(JNIEnv* env, jclass cls, jmethodID& out, const char* obf, const char* deobf, const char* sig);
    static void TryMethod2(JNIEnv* env, jclass cls, jmethodID& out, const char* obf, const char* deobf, const char* obfSig, const char* deobfSig);
};
#include "strayCache.h"
#include "../core.h"
#include "../utilities/logger.h"

jclass StrayCache::Minecraft = nullptr;
jobject StrayCache::MinecraftInstance = nullptr;
jclass StrayCache::World = nullptr;
jclass StrayCache::Entity = nullptr;
jclass StrayCache::EntityLivingBase = nullptr;
jclass StrayCache::EntityPlayer = nullptr;
jclass StrayCache::EntityPlayerSP = nullptr;

jclass StrayCache::ListClass = nullptr;
jclass StrayCache::FloatBufferClass = nullptr;
jclass StrayCache::ActiveRenderInfoClass = nullptr;
jclass StrayCache::RenderManagerClass = nullptr;
jclass StrayCache::GameSettingsClass = nullptr;
jclass StrayCache::PlayerControllerMP = nullptr;

jfieldID StrayCache::Entity_posX = nullptr;
jfieldID StrayCache::Entity_posY = nullptr;
jfieldID StrayCache::Entity_posZ = nullptr;
jfieldID StrayCache::Entity_width = nullptr;
jfieldID StrayCache::Entity_height = nullptr;
jfieldID StrayCache::Entity_isDead = nullptr;
jfieldID StrayCache::Entity_motionX = nullptr;
jfieldID StrayCache::Entity_motionY = nullptr;
jfieldID StrayCache::Entity_motionZ = nullptr;
jfieldID StrayCache::Entity_rotationYaw = nullptr;
jfieldID StrayCache::Entity_rotationPitch = nullptr;
jfieldID StrayCache::Entity_onGround = nullptr;
jfieldID StrayCache::Entity_fallDistance = nullptr;

jmethodID StrayCache::Entity_getEntityId = nullptr;
jmethodID StrayCache::Entity_getName = nullptr;
jmethodID StrayCache::Entity_setSprinting = nullptr;
jmethodID StrayCache::Entity_isSprinting = nullptr;
jmethodID StrayCache::Entity_isSneaking = nullptr;
jmethodID StrayCache::Entity_getDistanceToEntity = nullptr;

jfieldID StrayCache::PC_blockReachDistance = nullptr;

jmethodID StrayCache::List_size = nullptr;
jmethodID StrayCache::List_get = nullptr;

jmethodID StrayCache::FloatBuffer_get = nullptr;

jfieldID StrayCache::ActiveRenderInfo_PROJECTION = nullptr;
jfieldID StrayCache::ActiveRenderInfo_MODELVIEW = nullptr;

jfieldID StrayCache::World_loadedEntityList = nullptr;
jfieldID StrayCache::World_playerEntities = nullptr;

jfieldID StrayCache::Minecraft_thePlayer = nullptr;
jfieldID StrayCache::Minecraft_theWorld = nullptr;
jfieldID StrayCache::Minecraft_renderManager = nullptr;
jfieldID StrayCache::Minecraft_playerController = nullptr;
jfieldID StrayCache::Minecraft_objectMouseOver = nullptr;
jfieldID StrayCache::Minecraft_gameSettings = nullptr;
jfieldID StrayCache::Minecraft_rightClickDelayTimer = nullptr;

jfieldID StrayCache::RenderManager_viewerPosX = nullptr;
jfieldID StrayCache::RenderManager_viewerPosY = nullptr;
jfieldID StrayCache::RenderManager_viewerPosZ = nullptr;

jmethodID StrayCache::EntityLivingBase_getHealth = nullptr;
jmethodID StrayCache::EntityLivingBase_getMaxHealth = nullptr;
jfieldID StrayCache::EntityLivingBase_hurtTime = nullptr;
jfieldID StrayCache::EntityLivingBase_deathTime = nullptr;
jmethodID StrayCache::EntityLivingBase_isUsingItem = nullptr;
jmethodID StrayCache::EntityLivingBase_getHeldItem = nullptr;

jfieldID StrayCache::EntityPlayer_inventory = nullptr;
jfieldID StrayCache::EntityPlayer_movementInput = nullptr;

jfieldID StrayCache::GameSettings_gammaSetting = nullptr;
jfieldID StrayCache::GameSettings_keyBindForward = nullptr;
jfieldID StrayCache::GameSettings_keyBindJump = nullptr;
jfieldID StrayCache::GameSettings_keyBindSprint = nullptr;
jfieldID StrayCache::GameSettings_keyBindSneak = nullptr;

StrayCache& StrayCache::GetInstance()
{
    static StrayCache instance;
    return instance;
}

void StrayCache::TryField(JNIEnv* env, jclass cls, jfieldID& out, const char* obf, const char* deobf, const char* sig)
{
    out = env->GetFieldID(cls, obf, sig);
    if (!out) { env->ExceptionClear(); out = env->GetFieldID(cls, deobf, sig); }
    if (out) env->ExceptionClear();
}

void StrayCache::TryField2(JNIEnv* env, jclass cls, jfieldID& out, const char* obf, const char* deobf, const char* obfSig, const char* deobfSig)
{
    out = env->GetFieldID(cls, obf, obfSig);
    if (!out) { env->ExceptionClear(); out = env->GetFieldID(cls, deobf, deobfSig); }
    if (out) env->ExceptionClear();
}

void StrayCache::TryMethod(JNIEnv* env, jclass cls, jmethodID& out, const char* obf, const char* deobf, const char* sig)
{
    out = env->GetMethodID(cls, obf, sig);
    if (!out) { env->ExceptionClear(); out = env->GetMethodID(cls, deobf, sig); }
    if (out) env->ExceptionClear();
}

void StrayCache::TryMethod2(JNIEnv* env, jclass cls, jmethodID& out, const char* obf, const char* deobf, const char* obfSig, const char* deobfSig)
{
    out = env->GetMethodID(cls, obf, obfSig);
    if (!out) { env->ExceptionClear(); out = env->GetMethodID(cls, deobf, deobfSig); }
    if (out) env->ExceptionClear();
}

void StrayCache::Initialize(JNIEnv* env)
{
    m_Env = env;
    Logger::Log("StrayCache initializing...");

    Java* java = Core::GetInstance().GetJava();
    if (!java) { Logger::Log("StrayCache: Java instance not available"); return; }

    auto cache = [&](const char* obf, const char* deobf, const char* label, jclass& out)
    {
        jclass cls = java->FindClass(obf, deobf);
        if (cls) { out = (jclass)env->NewGlobalRef(cls); env->DeleteLocalRef(cls); Logger::Log("Cached %s", label); }
        else Logger::Log("Failed to cache %s", label);
    };

    cache("ave", "net/minecraft/client/Minecraft", "Minecraft", Minecraft);
    cache("adm", "net/minecraft/world/World", "World", World);
    cache("pk", "net/minecraft/entity/Entity", "Entity", Entity);
    cache("pr", "net/minecraft/entity/EntityLivingBase", "EntityLivingBase", EntityLivingBase);
    cache("qh", "net/minecraft/entity/player/EntityPlayer", "EntityPlayer", EntityPlayer);
    cache("bew", "net/minecraft/client/entity/EntityPlayerSP", "EntityPlayerSP", EntityPlayerSP);
    cache("java/util/List", "java/util/List", "List", ListClass);
    cache("java/nio/FloatBuffer", "java/nio/FloatBuffer", "FloatBuffer", FloatBufferClass);
    cache("avq", "net/minecraft/client/renderer/ActiveRenderInfo", "ActiveRenderInfo", ActiveRenderInfoClass);
    cache("cgs", "net/minecraft/client/renderer/entity/RenderManager", "RenderManager", RenderManagerClass);
    cache("bhl", "net/minecraft/client/multiplayer/PlayerControllerMP", "PlayerControllerMP", PlayerControllerMP);

    if (PlayerControllerMP)
    {
        TryField(env, PlayerControllerMP, PC_blockReachDistance, "g", "blockReachDistance", "F");
    }

    if (Minecraft)
    {
        jmethodID getMc = env->GetStaticMethodID(Minecraft, "A", "()Lave;");
        if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(Minecraft, "getMinecraft", "()Lnet/minecraft/client/Minecraft;"); }
        if (getMc)
        {
            env->ExceptionClear();
            jobject mc = env->CallStaticObjectMethod(Minecraft, getMc);
            if (mc) { MinecraftInstance = env->NewGlobalRef(mc); env->DeleteLocalRef(mc); }
        }
        env->ExceptionClear();

        TryField2(env, Minecraft, Minecraft_thePlayer, "u", "thePlayer", "Lbew;", "Lnet/minecraft/client/entity/EntityPlayerSP;");
        TryField2(env, Minecraft, Minecraft_theWorld, "f", "theWorld", "Ladm;", "Lnet/minecraft/world/World;");
        TryField2(env, Minecraft, Minecraft_renderManager, "ad", "renderManager", "Lcgs;", "Lnet/minecraft/client/renderer/entity/RenderManager;");
        TryField2(env, Minecraft, Minecraft_playerController, "b", "playerController", "Lbhl;", "Lnet/minecraft/client/multiplayer/PlayerControllerMP;");
        TryField2(env, Minecraft, Minecraft_objectMouseOver, "z", "objectMouseOver", "Lazu;", "Lnet/minecraft/util/MovingObjectPosition;");
        TryField2(env, Minecraft, Minecraft_gameSettings, "t", "gameSettings", "Lavh;", "Lnet/minecraft/client/settings/GameSettings;");
        TryField(env, Minecraft, Minecraft_rightClickDelayTimer, "e", "rightClickDelayTimer", "I");

        // Cache GameSettings fields
        if (MinecraftInstance && Minecraft_gameSettings)
        {
            jobject gs = env->GetObjectField(MinecraftInstance, Minecraft_gameSettings);
            if (gs)
            {
                GameSettingsClass = (jclass)env->NewGlobalRef(env->GetObjectClass(gs));
                TryField(env, GameSettingsClass, GameSettings_gammaSetting, "v", "gammaSetting", "F");
                TryField2(env, GameSettingsClass, GameSettings_keyBindForward, "d", "keyBindForward", "Lavc;", "Lnet/minecraft/client/settings/KeyBinding;");
                TryField2(env, GameSettingsClass, GameSettings_keyBindJump, "c", "keyBindJump", "Lavc;", "Lnet/minecraft/client/settings/KeyBinding;");
                TryField2(env, GameSettingsClass, GameSettings_keyBindSprint, "e", "keyBindSprint", "Lavc;", "Lnet/minecraft/client/settings/KeyBinding;");
                TryField2(env, GameSettingsClass, GameSettings_keyBindSneak, "g", "keyBindSneak", "Lavc;", "Lnet/minecraft/client/settings/KeyBinding;");
                env->DeleteLocalRef(gs);
            }
        }
    }

    if (Entity)
    {
        TryField(env, Entity, Entity_posX, "r", "posX", "D");
        TryField(env, Entity, Entity_posY, "s", "posY", "D");
        TryField(env, Entity, Entity_posZ, "t", "posZ", "D");
        TryField(env, Entity, Entity_width, "n", "width", "F");
        TryField(env, Entity, Entity_height, "o", "height", "F");
        TryField(env, Entity, Entity_isDead, "J", "isDead", "Z");
        TryField(env, Entity, Entity_motionX, "v", "motionX", "D");
        TryField(env, Entity, Entity_motionY, "w", "motionY", "D");
        TryField(env, Entity, Entity_motionZ, "x", "motionZ", "D");
        TryField(env, Entity, Entity_rotationYaw, "y", "rotationYaw", "F");
        TryField(env, Entity, Entity_rotationPitch, "z", "rotationPitch", "F");
        TryField(env, Entity, Entity_onGround, "C", "onGround", "Z");
        TryField(env, Entity, Entity_fallDistance, "k", "fallDistance", "F");
        TryMethod(env, Entity, Entity_getEntityId, "q", "getEntityId", "()I");
        TryMethod(env, Entity, Entity_getName, "c", "getName", "()Ljava/lang/String;");
        TryMethod(env, Entity, Entity_setSprinting, "b", "setSprinting", "(Z)V");
        TryMethod(env, Entity, Entity_isSprinting, "f", "isSprinting", "()Z");
        TryMethod(env, Entity, Entity_isSneaking, "n", "isSneaking", "()Z");
        TryMethod2(env, Entity, Entity_getDistanceToEntity, "a", "getDistanceToEntity", "(Lpk;)D", "(Lnet/minecraft/entity/Entity;)D");
    }

    if (ListClass)
    {
        TryMethod(env, ListClass, List_size, "size", "size", "()I");
        TryMethod(env, ListClass, List_get, "get", "get", "(I)Ljava/lang/Object;");
    }

    if (FloatBufferClass)
    {
        TryMethod(env, FloatBufferClass, FloatBuffer_get, "get", "get", "(I)F");
    }

    if (ActiveRenderInfoClass)
    {
        TryField(env, ActiveRenderInfoClass, ActiveRenderInfo_PROJECTION, "h", "PROJECTION", "Ljava/nio/FloatBuffer;");
        TryField(env, ActiveRenderInfoClass, ActiveRenderInfo_MODELVIEW, "g", "MODELVIEW", "Ljava/nio/FloatBuffer;");
    }

    if (World)
    {
        TryField(env, World, World_loadedEntityList, "g", "loadedEntityList", "Ljava/util/List;");
        TryField(env, World, World_playerEntities, "e", "playerEntities", "Ljava/util/List;");
    }

    if (RenderManagerClass)
    {
        TryField(env, RenderManagerClass, RenderManager_viewerPosX, "c", "viewerPosX", "D");
        TryField(env, RenderManagerClass, RenderManager_viewerPosY, "d", "viewerPosY", "D");
        TryField(env, RenderManagerClass, RenderManager_viewerPosZ, "e", "viewerPosZ", "D");
    }

    if (EntityLivingBase)
    {
        TryMethod(env, EntityLivingBase, EntityLivingBase_getHealth, "aM", "getHealth", "()F");
        TryMethod(env, EntityLivingBase, EntityLivingBase_getMaxHealth, "aA", "getMaxHealth", "()F");
        TryField(env, EntityLivingBase, EntityLivingBase_hurtTime, "ae", "hurtTime", "I");
        TryField(env, EntityLivingBase, EntityLivingBase_deathTime, "af", "deathTime", "I");
        TryMethod(env, EntityLivingBase, EntityLivingBase_isUsingItem, "e", "isUsingItem", "()Z");
        TryMethod2(env, EntityLivingBase, EntityLivingBase_getHeldItem, "dd", "getHeldItem", "()Lzv;", "()Lnet/minecraft/item/ItemStack;");
    }

    if (EntityPlayer)
    {
        TryField2(env, EntityPlayer, EntityPlayer_inventory, "a", "inventory", "Lqi;", "Lnet/minecraft/entity/player/InventoryPlayer;");
        TryField2(env, EntityPlayer, EntityPlayer_movementInput, "aY", "movementInput", "Lbdm;", "Lnet/minecraft/util/MovementInput;");
    }

    Logger::Log("StrayCache initialized with cached JNI IDs");
}

jclass StrayCache::GetClass(const std::string& name)
{
    auto it = m_Classes.find(name);
    if (it != m_Classes.end())
        return it->second;

    Java* java = Core::GetInstance().GetJava();
    jclass clazz = java ? java->FindClass(name) : nullptr;
    if (clazz && m_Env)
    {
        m_Classes[name] = (jclass)m_Env->NewGlobalRef(clazz);
        m_Env->DeleteLocalRef(clazz);
    }
    return clazz;
}

jmethodID StrayCache::GetMethodID(jclass clazz, const std::string& name, const std::string& sig)
{
    return m_Env ? m_Env->GetMethodID(clazz, name.c_str(), sig.c_str()) : nullptr;
}

jfieldID StrayCache::GetFieldID(jclass clazz, const std::string& name, const std::string& sig)
{
    return m_Env ? m_Env->GetFieldID(clazz, name.c_str(), sig.c_str()) : nullptr;
}

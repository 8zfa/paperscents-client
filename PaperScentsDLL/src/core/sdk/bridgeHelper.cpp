#include "bridgeHelper.h"
#include "../core.h"
#include "../utilities/logger.h"
#include <vector>

jclass BridgeHelper::MinecraftBridge = nullptr;
jclass BridgeHelper::EntityBridge = nullptr;
jclass BridgeHelper::EntityLivingBaseBridge = nullptr;
jclass BridgeHelper::EntityPlayerSPBridge = nullptr;
jclass BridgeHelper::EntityMobBridge = nullptr;
jclass BridgeHelper::EntityAnimalBridge = nullptr;
jclass BridgeHelper::WorldBridge = nullptr;
jclass BridgeHelper::WorldClientBridge = nullptr;
jclass BridgeHelper::GameSettingsBridge = nullptr;
jclass BridgeHelper::PlayerControllerBridge = nullptr;
jclass BridgeHelper::NetHandlerPlayClientBridge = nullptr;

jmethodID BridgeHelper::McBridge_GetPlayer = nullptr;
jmethodID BridgeHelper::McBridge_GetPlayerController = nullptr;
jmethodID BridgeHelper::McBridge_GetGameSettings = nullptr;
jmethodID BridgeHelper::McBridge_GetObjectMouseOver = nullptr;
jmethodID BridgeHelper::McBridge_GetPointedEntity = nullptr;
jmethodID BridgeHelper::McBridge_GetRenderGlobal = nullptr;

jmethodID BridgeHelper::EntityBridge_GetPosX = nullptr;
jmethodID BridgeHelper::EntityBridge_GetPosY = nullptr;
jmethodID BridgeHelper::EntityBridge_GetPosZ = nullptr;
jmethodID BridgeHelper::EntityBridge_GetMotionX = nullptr;
jmethodID BridgeHelper::EntityBridge_GetMotionY = nullptr;
jmethodID BridgeHelper::EntityBridge_GetMotionZ = nullptr;
jmethodID BridgeHelper::EntityBridge_GetRotationYaw = nullptr;
jmethodID BridgeHelper::EntityBridge_GetRotationPitch = nullptr;
jmethodID BridgeHelper::EntityBridge_GetEntityId = nullptr;
jmethodID BridgeHelper::EntityBridge_GetWorld = nullptr;
jmethodID BridgeHelper::EntityBridge_IsOnGround = nullptr;
jmethodID BridgeHelper::EntityBridge_IsSneaking = nullptr;
jmethodID BridgeHelper::EntityBridge_IsInvisible = nullptr;
jmethodID BridgeHelper::EntityBridge_IsDead = nullptr;
jmethodID BridgeHelper::EntityBridge_GetWidth = nullptr;
jmethodID BridgeHelper::EntityBridge_GetEyeHeight = nullptr;
jmethodID BridgeHelper::EntityBridge_GetFallDistance = nullptr;
jmethodID BridgeHelper::EntityBridge_GetBoundingBox = nullptr;
jmethodID BridgeHelper::EntityBridge_SetPosX = nullptr;
jmethodID BridgeHelper::EntityBridge_SetPosY = nullptr;
jmethodID BridgeHelper::EntityBridge_SetPosZ = nullptr;
jmethodID BridgeHelper::EntityBridge_SetMotionX = nullptr;
jmethodID BridgeHelper::EntityBridge_SetMotionY = nullptr;
jmethodID BridgeHelper::EntityBridge_SetMotionZ = nullptr;
jmethodID BridgeHelper::EntityBridge_SetRotationYaw = nullptr;
jmethodID BridgeHelper::EntityBridge_SetRotationPitch = nullptr;
jmethodID BridgeHelper::EntityBridge_GetDistanceToEntity = nullptr;
jmethodID BridgeHelper::EntityBridge_GetDistanceSqToEntity = nullptr;

jmethodID BridgeHelper::ELBridge_GetHealth = nullptr;
jmethodID BridgeHelper::ELBridge_GetHurtTime = nullptr;
jmethodID BridgeHelper::ELBridge_GetDeathTime = nullptr;
jmethodID BridgeHelper::ELBridge_GetMaxHealth = nullptr;
jmethodID BridgeHelper::ELBridge_IsOnLadder = nullptr;
jmethodID BridgeHelper::ELBridge_IsInWater = nullptr;
jmethodID BridgeHelper::ELBridge_GetLastAttackedMillis = nullptr;
jmethodID BridgeHelper::ELBridge_GetLastHurtMillis = nullptr;
jmethodID BridgeHelper::ELBridge_GetActivePotionEffects = nullptr;
jmethodID BridgeHelper::ELBridge_GetHeldItem = nullptr;
jmethodID BridgeHelper::ELBridge_IsUsingItem = nullptr;

jclass BridgeHelper::ItemSwordBridge = nullptr;
jclass BridgeHelper::ItemBowBridge = nullptr;
jclass BridgeHelper::ItemFoodBridge = nullptr;
jclass BridgeHelper::ItemStackBridge = nullptr;
jmethodID BridgeHelper::ItemStackBridge_GetItem = nullptr;

jmethodID BridgeHelper::EPSPBridge_SetSprinting = nullptr;
jmethodID BridgeHelper::EPSPBridge_GetSprintingTicksLeft = nullptr;
jmethodID BridgeHelper::EPSPBridge_SetMovementInput = nullptr;
jmethodID BridgeHelper::EPSPBridge_GetMovementInput = nullptr;
jmethodID BridgeHelper::EPSPBridge_OnCriticalHit = nullptr;
jmethodID BridgeHelper::EPSPBridge_SendChatMessage = nullptr;

jmethodID BridgeHelper::WorldBridge_GetEntities = nullptr;
jmethodID BridgeHelper::WorldBridge_GetPlayerEntities = nullptr;
jmethodID BridgeHelper::WorldBridge_IsRemote = nullptr;

jmethodID BridgeHelper::GSBridge_KeyBindAttack = nullptr;
jmethodID BridgeHelper::GSBridge_KeyBindUseItem = nullptr;
jmethodID BridgeHelper::GSBridge_KeyBindForward = nullptr;
jmethodID BridgeHelper::GSBridge_KeyBindBack = nullptr;
jmethodID BridgeHelper::GSBridge_KeyBindSneak = nullptr;
jmethodID BridgeHelper::GSBridge_KeyBindJump = nullptr;
jmethodID BridgeHelper::GSBridge_KeyBindSprint = nullptr;
jmethodID BridgeHelper::GSBridge_KeyBindLeft = nullptr;
jmethodID BridgeHelper::GSBridge_KeyBindRight = nullptr;
jmethodID BridgeHelper::GSBridge_SetKeyBindState = nullptr;
jmethodID BridgeHelper::GSBridge_SetGamma = nullptr;
jmethodID BridgeHelper::GSBridge_UnpressAllKeys = nullptr;

jmethodID BridgeHelper::NHPBridge_AddToSendQueue = nullptr;

jmethodID BridgeHelper::PCBridge_IsSpectator = nullptr;

static jobject g_MinecraftInstance = nullptr;
static bool g_McReflectionTried = false;

static std::string GetJavaClassName(JNIEnv* env, jclass clazz)
{
    jclass cc = env->FindClass("java/lang/Class");
    jmethodID gn = env->GetMethodID(cc, "getName", "()Ljava/lang/String;");
    jstring jn = (jstring)env->CallObjectMethod(clazz, gn);
    if (!jn) { env->ExceptionClear(); env->DeleteLocalRef(cc); return ""; }
    const char* s = env->GetStringUTFChars(jn, nullptr);
    std::string r(s ? s : "");
    if (s) env->ReleaseStringUTFChars(jn, s);
    env->DeleteLocalRef(jn);
    env->DeleteLocalRef(cc);
    return r;
}

jobject BridgeHelper::GetMinecraftInstance(JNIEnv* env)
{
    if (g_MinecraftInstance) return g_MinecraftInstance;
    if (g_McReflectionTried) return nullptr;

    Java* java = Core::GetInstance().GetJava();
    if (!java) return nullptr;

    jclass mcClass = java->FindClass("net.minecraft.client.Minecraft", "net.minecraft.client.Minecraft");
    if (!mcClass) { g_McReflectionTried = true; return nullptr; }

    jclass classClass = env->FindClass("java/lang/Class");
    if (!classClass) { env->ExceptionClear(); g_McReflectionTried = true; return nullptr; }

    jmethodID getDeclaredFields = env->GetMethodID(classClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
    jmethodID getDeclaredMethods = env->GetMethodID(classClass, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");

    // Method 1: Find static field of Minecraft type
    if (getDeclaredFields)
    {
        jclass fieldClass = env->FindClass("java/lang/reflect/Field");
        if (fieldClass)
        {
            jmethodID fieldGetType = env->GetMethodID(fieldClass, "getType", "()Ljava/lang/Class;");
            jmethodID fieldGetMods = env->GetMethodID(fieldClass, "getModifiers", "()I");
            jmethodID fieldSetAcc = env->GetMethodID(fieldClass, "setAccessible", "(Z)V");
            jmethodID fieldGet = env->GetMethodID(fieldClass, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");

            if (fieldGetType && fieldGetMods && fieldSetAcc && fieldGet)
            {
                jobjectArray fields = (jobjectArray)env->CallObjectMethod(mcClass, getDeclaredFields);
                if (fields)
                {
                    jint fc = env->GetArrayLength(fields);
                    for (jint i = 0; i < fc; i++)
                    {
                        jobject f = env->GetObjectArrayElement(fields, i);
                        if (!f) continue;
                        jint mods = env->CallIntMethod(f, fieldGetMods);
                        if ((mods & 8) == 0) { env->DeleteLocalRef(f); continue; }
                        jclass ft = (jclass)env->CallObjectMethod(f, fieldGetType);
                        if (ft && env->IsAssignableFrom(mcClass, ft))
                        {
                            env->CallVoidMethod(f, fieldSetAcc, JNI_TRUE);
                            jobject inst = env->CallObjectMethod(f, fieldGet, nullptr);
                            if (inst)
                            {
                                g_MinecraftInstance = env->NewGlobalRef(inst);
                                env->DeleteLocalRef(inst); env->DeleteLocalRef(ft);
                                env->DeleteLocalRef(f); env->DeleteLocalRef(fields);
                                Logger::Log("BH: Got MC instance via reflection field");
                                return g_MinecraftInstance;
                            }
                        }
                        if (ft) env->DeleteLocalRef(ft);
                        env->DeleteLocalRef(f);
                    }
                    env->DeleteLocalRef(fields);
                }
            }
            env->DeleteLocalRef(fieldClass);
        }
    }

    // Method 2: Find static method with no params returning Minecraft
    if (getDeclaredMethods)
    {
        jclass methodClass = env->FindClass("java/lang/reflect/Method");
        if (methodClass)
        {
            jmethodID methodGetRetType = env->GetMethodID(methodClass, "getReturnType", "()Ljava/lang/Class;");
            jmethodID methodGetParamTypes = env->GetMethodID(methodClass, "getParameterTypes", "()[Ljava/lang/Class;");
            jmethodID methodSetAcc = env->GetMethodID(methodClass, "setAccessible", "(Z)V");
            jmethodID methodInvoke = env->GetMethodID(methodClass, "invoke", "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");

            if (methodGetRetType && methodGetParamTypes && methodSetAcc && methodInvoke)
            {
                jobjectArray methods = (jobjectArray)env->CallObjectMethod(mcClass, getDeclaredMethods);
                if (methods)
                {
                    jint mc = env->GetArrayLength(methods);
                    for (jint i = 0; i < mc; i++)
                    {
                        jobject m = env->GetObjectArrayElement(methods, i);
                        if (!m) continue;
                        jclass rt = (jclass)env->CallObjectMethod(m, methodGetRetType);
                        jobjectArray pt = (jobjectArray)env->CallObjectMethod(m, methodGetParamTypes);
                        jint pc = pt ? env->GetArrayLength(pt) : -1;
                        if (pt) env->DeleteLocalRef(pt);
                        if (rt && env->IsAssignableFrom(mcClass, rt) && pc == 0)
                        {
                            env->CallVoidMethod(m, methodSetAcc, JNI_TRUE);
                            jobject inst = env->CallObjectMethod(m, methodInvoke, nullptr, nullptr);
                            if (inst)
                            {
                                g_MinecraftInstance = env->NewGlobalRef(inst);
                                env->DeleteLocalRef(inst); env->DeleteLocalRef(rt);
                                env->DeleteLocalRef(m); env->DeleteLocalRef(methods);
                                Logger::Log("BH: Got MC instance via reflection method");
                                return g_MinecraftInstance;
                            }
                        }
                        if (rt) env->DeleteLocalRef(rt);
                        env->DeleteLocalRef(m);
                    }
                    env->DeleteLocalRef(methods);
                }
            }
            env->DeleteLocalRef(methodClass);
        }
    }

    g_McReflectionTried = true;
    Logger::Log("BH: Failed to get Minecraft instance via reflection");
    return nullptr;
}

bool BridgeHelper::Initialize(JNIEnv* env)
{
    if (MinecraftBridge) return true;

    Java* java = Core::GetInstance().GetJava();
    if (!java) { Logger::Log("BH: Java not available"); return false; }

    // Step 1: Discover MinecraftBridge
    jclass mcBridgeLocal = nullptr;
    {
        jclass known = java->GetBridgeMinecraft();
        if (known) {
            mcBridgeLocal = known;
            Logger::Log("BH: Got MinecraftBridge from Java discovery");
        }
    }
    if (!mcBridgeLocal)
    {
        Logger::Log("BH: Scanning Minecraft interfaces for Lunar bridge...");
        jclass mcClass = java->FindClass("net.minecraft.client.Minecraft", "net.minecraft.client.Minecraft");
        if (mcClass) {
            jclass cc = env->FindClass("java/lang/Class");
            jmethodID gi = env->GetMethodID(cc, "getInterfaces", "()[Ljava/lang/Class;");
            jmethodID gn = env->GetMethodID(cc, "getName", "()Ljava/lang/String;");
            if (gi && gn) {
                jobjectArray ifaces = (jobjectArray)env->CallObjectMethod(mcClass, gi);
                if (ifaces) {
                    jint cnt = env->GetArrayLength(ifaces);
                    for (jint i = 0; i < cnt; i++) {
                        jclass ifc = (jclass)env->GetObjectArrayElement(ifaces, i);
                        jstring jn = (jstring)env->CallObjectMethod(ifc, gn);
                        const char* ns = env->GetStringUTFChars(jn, nullptr);
                        if (ns && strstr(ns, "com.moonsworth.lunar.") == ns) {
                            mcBridgeLocal = ifc;
                            Logger::Log("BH: Found MinecraftBridge: %s", ns);
                            env->ReleaseStringUTFChars(jn, ns);
                            env->DeleteLocalRef(jn);
                            env->DeleteLocalRef(ifc);
                            env->DeleteLocalRef(ifaces);
                            env->DeleteLocalRef(mcClass);
                            env->DeleteLocalRef(cc);
                            break;
                        }
                        if (ns) env->ReleaseStringUTFChars(jn, ns);
                        env->DeleteLocalRef(jn);
                        env->DeleteLocalRef(ifc);
                    }
                    if (!mcBridgeLocal) env->DeleteLocalRef(ifaces);
                }
            }
            if (!mcBridgeLocal) { env->DeleteLocalRef(mcClass); env->DeleteLocalRef(cc); }
        }
    }
    if (!mcBridgeLocal) { Logger::Log("BH: Failed to find MinecraftBridge"); return false; }

    // Pre-cache reflection method IDs
    jclass classClass = env->FindClass("java/lang/Class");
    jclass methodClass = env->FindClass("java/lang/reflect/Method");

    jmethodID gnMid = env->GetMethodID(classClass, "getName", "()Ljava/lang/String;");
    jmethodID gdmMid = env->GetMethodID(classClass, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
    jmethodID giMid = env->GetMethodID(classClass, "getInterfaces", "()[Ljava/lang/Class;");
    jmethodID mgnMid = env->GetMethodID(methodClass, "getName", "()Ljava/lang/String;");
    jmethodID mrtMid = env->GetMethodID(methodClass, "getReturnType", "()Ljava/lang/Class;");

    if (!gnMid || !gdmMid || !giMid || !mgnMid || !mrtMid) {
        Logger::Log("BH: Failed to acquire reflection method IDs");
        env->ExceptionClear();
        return false;
    }

    // Struct for discovered bridge interfaces
    struct FoundBridge {
        jclass clazz;    // global ref
        jclass* slot;    // nullptr = not yet categorized
    };
    std::vector<FoundBridge> found;

    // Add with dedup and Lunar-interface check
    auto addIfNew = [&](jclass cl) -> bool {
        if (!cl) return false;
        std::string nm = GetJavaClassName(env, cl);
        if (nm.find("com.moonsworth.lunar.") != 0) return false;
        for (auto& fb : found) {
            if (env->IsSameObject(fb.clazz, cl)) return false;
        }
        found.push_back({(jclass)env->NewGlobalRef(cl), nullptr});
        return true;
    };

    // Bootstrap with MinecraftBridge
    addIfNew(mcBridgeLocal);

    // Recursively scan all discovered interfaces:
    // - method return types
    // - parent interfaces
    for (size_t i = 0; i < found.size(); i++) {
        jclass cl = found[i].clazz;

        // Scan declared methods for Lunar return types
        jobjectArray methods = (jobjectArray)env->CallObjectMethod(cl, gdmMid);
        if (methods) {
            jint mcnt = env->GetArrayLength(methods);
            for (jint j = 0; j < mcnt; j++) {
                jobject m = env->GetObjectArrayElement(methods, j);
                if (!m) continue;
                jstring mname = (jstring)env->CallObjectMethod(m, mgnMid);
                const char* mn = env->GetStringUTFChars(mname, nullptr);
                bool isBridge = (mn && strstr(mn, "bridge$") == mn);
                env->ReleaseStringUTFChars(mname, mn);
                env->DeleteLocalRef(mname);
                if (!isBridge) { env->DeleteLocalRef(m); continue; }
                jclass rt = (jclass)env->CallObjectMethod(m, mrtMid);
                if (rt) {
                    addIfNew(rt);
                    env->DeleteLocalRef(rt);
                }
                env->DeleteLocalRef(m);
            }
            env->DeleteLocalRef(methods);
        }

        // Scan parent interfaces
        jobjectArray ifaces = (jobjectArray)env->CallObjectMethod(cl, giMid);
        if (ifaces) {
            jint icnt = env->GetArrayLength(ifaces);
            for (jint j = 0; j < icnt; j++) {
                jclass ifc = (jclass)env->GetObjectArrayElement(ifaces, j);
                if (ifc) {
                    addIfNew(ifc);
                    env->DeleteLocalRef(ifc);
                }
            }
            env->DeleteLocalRef(ifaces);
        }
    }

    // Step 3: Categorize each discovered interface by scanning its methods
    for (auto& fb : found) {
        std::string nm = GetJavaClassName(env, fb.clazz);
        bool hasPosX = false, hasHealth = false, hasGetMaxHealth = false;
        bool hasSetSprinting = false, hasGetEntities = false;
        bool hasKeyBindAttack = false, hasIsSpectator = false;
        bool hasGetItem = false, hasAddToSendQueue = false;
        bool hasGetPlayer = false;

        jobjectArray methods = (jobjectArray)env->CallObjectMethod(fb.clazz, gdmMid);
        if (methods) {
            jint mcnt = env->GetArrayLength(methods);
            for (jint j = 0; j < mcnt; j++) {
                jobject m = env->GetObjectArrayElement(methods, j);
                if (!m) continue;
                jstring mname = (jstring)env->CallObjectMethod(m, mgnMid);
                const char* s = env->GetStringUTFChars(mname, nullptr);
                if (s) {
                    if (strcmp(s, "bridge$getPosX") == 0) hasPosX = true;
                    else if (strcmp(s, "bridge$getHealth") == 0) hasHealth = true;
                    else if (strcmp(s, "bridge$getMaxHealth") == 0) hasGetMaxHealth = true;
                    else if (strcmp(s, "bridge$setSprinting") == 0) hasSetSprinting = true;
                    else if (strcmp(s, "bridge$getEntities") == 0) hasGetEntities = true;
                    else if (strcmp(s, "bridge$keyBindAttack") == 0) hasKeyBindAttack = true;
                    else if (strcmp(s, "bridge$isSpectator") == 0) hasIsSpectator = true;
                    else if (strcmp(s, "bridge$getItem") == 0) hasGetItem = true;
                    else if (strcmp(s, "bridge$addToSendQueue") == 0) hasAddToSendQueue = true;
                    else if (strcmp(s, "bridge$getPlayer") == 0) hasGetPlayer = true;
                }
                env->ReleaseStringUTFChars(mname, s);
                env->DeleteLocalRef(mname);
                env->DeleteLocalRef(m);
            }
            env->DeleteLocalRef(methods);
        }

        if (hasGetPlayer) {
            fb.slot = &MinecraftBridge;
            Logger::Log("BH: Categorized -> MinecraftBridge");
        } else if (hasSetSprinting) {
            fb.slot = &EntityPlayerSPBridge;
        } else if (hasHealth || hasGetMaxHealth) {
            fb.slot = &EntityLivingBaseBridge;
        } else if (hasPosX) {
            fb.slot = &EntityBridge;
        } else if (hasKeyBindAttack) {
            fb.slot = &GameSettingsBridge;
        } else if (hasGetEntities) {
            fb.slot = &WorldBridge;
        } else if (hasIsSpectator) {
            fb.slot = &PlayerControllerBridge;
        } else if (hasGetItem) {
            fb.slot = &ItemStackBridge;
        } else if (hasAddToSendQueue) {
            fb.slot = &NetHandlerPlayClientBridge;
        }
    }

    // Assign slots and clean up uncategorized interfaces
    for (auto& fb : found) {
        if (fb.slot) {
            *fb.slot = fb.clazz;
        } else {
            env->DeleteGlobalRef(fb.clazz);
        }
    }

    // Step 4: Resolve bridge$ methods for each bridge via FromReflectedMethod
    auto resolveMethods = [&](jclass clazz, const char* label) {
        if (!clazz) return;
        jobjectArray methods = (jobjectArray)env->CallObjectMethod(clazz, gdmMid);
        if (!methods) { env->ExceptionClear(); return; }
        jint cnt = env->GetArrayLength(methods);
        int res = 0;
        for (jint i = 0; i < cnt; i++) {
            jobject m = env->GetObjectArrayElement(methods, i);
            if (!m) continue;
            jstring mname = (jstring)env->CallObjectMethod(m, mgnMid);
            const char* s = env->GetStringUTFChars(mname, nullptr);
            if (!s || strstr(s, "bridge$") != s) {
                if (s) env->ReleaseStringUTFChars(mname, s);
                env->DeleteLocalRef(mname);
                env->DeleteLocalRef(m);
                continue;
            }

            jmethodID mid = env->FromReflectedMethod(m);

            // EntityBridge
            if (strcmp(s, "bridge$getPosX") == 0) EntityBridge_GetPosX = mid;
            else if (strcmp(s, "bridge$getPosY") == 0) EntityBridge_GetPosY = mid;
            else if (strcmp(s, "bridge$getPosZ") == 0) EntityBridge_GetPosZ = mid;
            else if (strcmp(s, "bridge$getMotionX") == 0) EntityBridge_GetMotionX = mid;
            else if (strcmp(s, "bridge$getMotionY") == 0) EntityBridge_GetMotionY = mid;
            else if (strcmp(s, "bridge$getMotionZ") == 0) EntityBridge_GetMotionZ = mid;
            else if (strcmp(s, "bridge$getRotationYaw") == 0) EntityBridge_GetRotationYaw = mid;
            else if (strcmp(s, "bridge$getRotationPitch") == 0) EntityBridge_GetRotationPitch = mid;
            else if (strcmp(s, "bridge$getEntityId") == 0) EntityBridge_GetEntityId = mid;
            else if (strcmp(s, "bridge$getWorld") == 0) EntityBridge_GetWorld = mid;
            else if (strcmp(s, "bridge$isOnGround") == 0) EntityBridge_IsOnGround = mid;
            else if (strcmp(s, "bridge$isSneaking") == 0) EntityBridge_IsSneaking = mid;
            else if (strcmp(s, "bridge$isInvisible") == 0) EntityBridge_IsInvisible = mid;
            else if (strcmp(s, "bridge$isDead") == 0) EntityBridge_IsDead = mid;
            else if (strcmp(s, "bridge$getWidth") == 0) EntityBridge_GetWidth = mid;
            else if (strcmp(s, "bridge$getEyeHeight") == 0) EntityBridge_GetEyeHeight = mid;
            else if (strcmp(s, "bridge$getFallDistance") == 0) EntityBridge_GetFallDistance = mid;
            else if (strcmp(s, "bridge$getBoundingBox") == 0) EntityBridge_GetBoundingBox = mid;
            else if (strcmp(s, "bridge$setPosX") == 0) EntityBridge_SetPosX = mid;
            else if (strcmp(s, "bridge$setPosY") == 0) EntityBridge_SetPosY = mid;
            else if (strcmp(s, "bridge$setPosZ") == 0) EntityBridge_SetPosZ = mid;
            else if (strcmp(s, "bridge$setMotionX") == 0) EntityBridge_SetMotionX = mid;
            else if (strcmp(s, "bridge$setMotionY") == 0) EntityBridge_SetMotionY = mid;
            else if (strcmp(s, "bridge$setMotionZ") == 0) EntityBridge_SetMotionZ = mid;
            else if (strcmp(s, "bridge$setRotationYaw") == 0) EntityBridge_SetRotationYaw = mid;
            else if (strcmp(s, "bridge$setRotationPitch") == 0) EntityBridge_SetRotationPitch = mid;
            else if (strcmp(s, "bridge$getDistanceToEntity") == 0) EntityBridge_GetDistanceToEntity = mid;
            else if (strcmp(s, "bridge$getDistanceSqToEntity") == 0) EntityBridge_GetDistanceSqToEntity = mid;
            // EntityLivingBaseBridge
            else if (strcmp(s, "bridge$getHealth") == 0) ELBridge_GetHealth = mid;
            else if (strcmp(s, "bridge$getMaxHealth") == 0) ELBridge_GetMaxHealth = mid;
            else if (strcmp(s, "bridge$getHurtTime") == 0) ELBridge_GetHurtTime = mid;
            else if (strcmp(s, "bridge$getDeathTime") == 0) ELBridge_GetDeathTime = mid;
            else if (strcmp(s, "bridge$isOnLadder") == 0) ELBridge_IsOnLadder = mid;
            else if (strcmp(s, "bridge$isInWater") == 0) ELBridge_IsInWater = mid;
            else if (strcmp(s, "bridge$getLastAttackedMillis") == 0) ELBridge_GetLastAttackedMillis = mid;
            else if (strcmp(s, "bridge$getLastHurtMillis") == 0) ELBridge_GetLastHurtMillis = mid;
            else if (strcmp(s, "bridge$getActivePotionEffects") == 0) ELBridge_GetActivePotionEffects = mid;
            else if (strcmp(s, "bridge$getHeldItem") == 0) ELBridge_GetHeldItem = mid;
            else if (strcmp(s, "bridge$isUsingItem") == 0) ELBridge_IsUsingItem = mid;
            // EntityPlayerSPBridge
            else if (strcmp(s, "bridge$setSprinting") == 0) EPSPBridge_SetSprinting = mid;
            else if (strcmp(s, "bridge$getSprintingTicksLeft") == 0) EPSPBridge_GetSprintingTicksLeft = mid;
            else if (strcmp(s, "bridge$setMovementInput") == 0) EPSPBridge_SetMovementInput = mid;
            else if (strcmp(s, "bridge$getMovementInput") == 0) EPSPBridge_GetMovementInput = mid;
            else if (strcmp(s, "bridge$onCriticalHit") == 0) EPSPBridge_OnCriticalHit = mid;
            else if (strcmp(s, "bridge$sendChatMessage") == 0) EPSPBridge_SendChatMessage = mid;
            // WorldBridge
            else if (strcmp(s, "bridge$getEntities") == 0) WorldBridge_GetEntities = mid;
            else if (strcmp(s, "bridge$getPlayerEntities") == 0) WorldBridge_GetPlayerEntities = mid;
            else if (strcmp(s, "bridge$isRemote") == 0) WorldBridge_IsRemote = mid;
            // GameSettingsBridge
            else if (strcmp(s, "bridge$keyBindAttack") == 0) GSBridge_KeyBindAttack = mid;
            else if (strcmp(s, "bridge$keyBindUseItem") == 0) GSBridge_KeyBindUseItem = mid;
            else if (strcmp(s, "bridge$keyBindForward") == 0) GSBridge_KeyBindForward = mid;
            else if (strcmp(s, "bridge$keyBindBack") == 0) GSBridge_KeyBindBack = mid;
            else if (strcmp(s, "bridge$keyBindSneak") == 0) GSBridge_KeyBindSneak = mid;
            else if (strcmp(s, "bridge$keyBindJump") == 0) GSBridge_KeyBindJump = mid;
            else if (strcmp(s, "bridge$keyBindSprint") == 0) GSBridge_KeyBindSprint = mid;
            else if (strcmp(s, "bridge$keyBindLeft") == 0) GSBridge_KeyBindLeft = mid;
            else if (strcmp(s, "bridge$keyBindRight") == 0) GSBridge_KeyBindRight = mid;
            else if (strcmp(s, "bridge$setKeyBindState") == 0) GSBridge_SetKeyBindState = mid;
            else if (strcmp(s, "bridge$setGamma") == 0) GSBridge_SetGamma = mid;
            else if (strcmp(s, "bridge$unpressAllKeys") == 0) GSBridge_UnpressAllKeys = mid;
            // PlayerControllerBridge
            else if (strcmp(s, "bridge$isSpectator") == 0) PCBridge_IsSpectator = mid;
            // NetHandlerPlayClientBridge
            else if (strcmp(s, "bridge$addToSendQueue") == 0) NHPBridge_AddToSendQueue = mid;
            // ItemStackBridge
            else if (strcmp(s, "bridge$getItem") == 0) ItemStackBridge_GetItem = mid;
            // MinecraftBridge
            else if (strcmp(s, "bridge$getPlayer") == 0) McBridge_GetPlayer = mid;
            else if (strcmp(s, "bridge$getPlayerController") == 0) McBridge_GetPlayerController = mid;
            else if (strcmp(s, "bridge$getGameSettings") == 0) McBridge_GetGameSettings = mid;
            else if (strcmp(s, "bridge$getObjectMouseOver") == 0) McBridge_GetObjectMouseOver = mid;
            else if (strcmp(s, "bridge$getPointedEntity") == 0) McBridge_GetPointedEntity = mid;
            else if (strcmp(s, "bridge$getRenderGlobal") == 0) McBridge_GetRenderGlobal = mid;

            env->ReleaseStringUTFChars(mname, s);
            env->DeleteLocalRef(mname);
            env->DeleteLocalRef(m);
            res++;
        }
        env->DeleteLocalRef(methods);
        Logger::Log("BH: Resolved %d methods for %s", res, label);
    };

    resolveMethods(MinecraftBridge, "MinecraftBridge");
    resolveMethods(EntityBridge, "EntityBridge");
    resolveMethods(EntityLivingBaseBridge, "EntityLivingBaseBridge");
    resolveMethods(EntityPlayerSPBridge, "EntityPlayerSPBridge");
    resolveMethods(WorldBridge, "WorldBridge");
    resolveMethods(GameSettingsBridge, "GameSettingsBridge");
    resolveMethods(PlayerControllerBridge, "PlayerControllerBridge");
    resolveMethods(ItemStackBridge, "ItemStackBridge");
    resolveMethods(NetHandlerPlayClientBridge, "NetHandlerPlayClientBridge");

    env->DeleteLocalRef(classClass);
    env->DeleteLocalRef(methodClass);

    Logger::Log("BridgeHelper: initialized");
    return true;
}

jobject BridgeHelper::GetPlayer(JNIEnv* env)
{
    jobject mc = GetMinecraftInstance(env);
    if (!mc || !McBridge_GetPlayer) return nullptr;
    return env->CallObjectMethod(mc, McBridge_GetPlayer);
}

jobject BridgeHelper::GetPlayerController(JNIEnv* env)
{
    jobject mc = GetMinecraftInstance(env);
    if (!mc || !McBridge_GetPlayerController) return nullptr;
    return env->CallObjectMethod(mc, McBridge_GetPlayerController);
}

jobject BridgeHelper::GetGameSettings(JNIEnv* env)
{
    jobject mc = GetMinecraftInstance(env);
    if (!mc || !McBridge_GetGameSettings) return nullptr;
    return env->CallObjectMethod(mc, McBridge_GetGameSettings);
}

jobject BridgeHelper::GetWorldFromEntity(JNIEnv* env, jobject entity)
{
    if (!entity || !EntityBridge_GetWorld) return nullptr;
    return env->CallObjectMethod(entity, EntityBridge_GetWorld);
}

void BridgeHelper::SimulateAttack(JNIEnv* env)
{
    jobject gs = GetGameSettings(env);
    if (!gs || !GSBridge_KeyBindAttack || !GSBridge_SetKeyBindState) { if (gs) env->DeleteLocalRef(gs); return; }
    jobject kb = env->CallObjectMethod(gs, GSBridge_KeyBindAttack);
    if (kb)
    {
        env->CallVoidMethod(gs, GSBridge_SetKeyBindState, kb, JNI_TRUE);
        env->CallVoidMethod(gs, GSBridge_SetKeyBindState, kb, JNI_FALSE);
        env->DeleteLocalRef(kb);
    }
    env->DeleteLocalRef(gs);
}

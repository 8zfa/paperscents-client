#include "strayCache.h"
#include "../utilities/logger.h"
#include <sstream>

// Static member definitions
jclass StrayCache::Minecraft = nullptr;
jobject StrayCache::MinecraftInstance = nullptr;
jclass StrayCache::World = nullptr;
jclass StrayCache::Entity = nullptr;
jclass StrayCache::EntityLivingBase = nullptr;
jclass StrayCache::EntityPlayer = nullptr;
jclass StrayCache::EntityPlayerSP = nullptr;
jclass StrayCache::ItemSwordClass = nullptr;

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
jmethodID StrayCache::Entity_isSneaking = nullptr;
jmethodID StrayCache::Entity_getDistanceToEntity = nullptr;
jmethodID StrayCache::Entity_setRotationYaw = nullptr;
jmethodID StrayCache::Entity_setRotationPitch = nullptr;

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

jfieldID StrayCache::EntityLivingBase_hurtTime = nullptr;
jfieldID StrayCache::EntityLivingBase_deathTime = nullptr;
jmethodID StrayCache::EntityLivingBase_getHealth = nullptr;
jmethodID StrayCache::EntityLivingBase_getMaxHealth = nullptr;
jmethodID StrayCache::EntityLivingBase_isUsingItem = nullptr;
jmethodID StrayCache::EntityLivingBase_getHeldItem = nullptr;

jmethodID StrayCache::ItemStack_getItem = nullptr;

jfieldID StrayCache::EntityPlayer_inventory = nullptr;
jfieldID StrayCache::EntityPlayer_movementInput = nullptr;

jfieldID StrayCache::GameSettings_gammaSetting = nullptr;
jfieldID StrayCache::GameSettings_keyBindForward = nullptr;
jfieldID StrayCache::GameSettings_keyBindJump = nullptr;
jfieldID StrayCache::GameSettings_keyBindSprint = nullptr;
jfieldID StrayCache::GameSettings_keyBindSneak = nullptr;

std::unordered_map<std::string, std::string> StrayCache::s_classMap;
std::unordered_map<std::string, std::string> StrayCache::s_fieldMap;
std::unordered_map<std::string, std::string> StrayCache::s_methodMap;
std::unordered_map<std::string, std::string> StrayCache::s_classMapRev;
std::unordered_map<std::string, std::string> StrayCache::s_fieldMapRev;
std::unordered_map<std::string, std::string> StrayCache::s_methodMapRev;

StrayCache& StrayCache::GetInstance()
{
    static StrayCache instance;
    return instance;
}

bool StrayCache::LoadLunarMappings(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::Log("Failed to open mappings file: %s", filename.c_str());
        return false;
    }

    std::string line;
    bool firstLine = true;

    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) continue;

        // Skip the header line (starts with "tiny")
        if (firstLine) {
            firstLine = false;
            continue;
        }

        // Parse the line based on type
        std::istringstream iss(line);
        std::string type;
        if (!(iss >> type)) continue;

        if (type == "c") { // Class mapping
            std::string obfuscated, humanReadable;
            if (iss >> obfuscated >> humanReadable) {
                s_classMap[obfuscated] = humanReadable;
                s_classMapRev[humanReadable] = obfuscated;
                Logger::Log("Mappings: %s -> %s", obfuscated.c_str(), humanReadable.c_str());
            }
        }
        else if (type == "f") { // Field mapping
            std::string descriptor, obfuscatedName, humanReadableName;
            if (iss >> descriptor >> obfuscatedName >> humanReadableName) {
                std::string key = descriptor + "|" + obfuscatedName;
                s_fieldMap[key] = humanReadableName;
                s_fieldMapRev[humanReadableName + "|" + descriptor] = obfuscatedName;
                // Also store reverse for lookup by field name only (fallback)
                s_fieldMapRev[humanReadableName] = obfuscatedName;
                Logger::Log("Mapped field: %s|%s -> %s", descriptor.c_str(), obfuscatedName.c_str(), humanReadableName.c_str());
            }
        }
        else if (type == "m") { // Method mapping
            std::string descriptor, obfuscatedName, humanReadableName;
            if (iss >> descriptor >> obfuscatedName >> humanReadableName) {
                std::string key = descriptor + "|" + obfuscatedName;
                s_methodMap[key] = humanReadableName;
                s_methodMapRev[humanReadableName + "|" + descriptor] = obfuscatedName;
                // Also store reverse for lookup by method name only (fallback)
                s_methodMapRev[humanReadableName] = obfuscatedName;
                Logger::Log("Mapped method: %s|%s -> %s", descriptor.c_str(), obfuscatedName.c_str(), humanReadableName.c_str());
            }

            // Handle parameter mappings (lines starting with spaces followed by 'p')
            std::string paramLine;
            while (std::getline(file, paramLine)) {
                // Check if this is a parameter line (starts with tab or spaces then 'p')
                if (paramLine.empty() || (paramLine[0] != '\t' && paramLine[0] != ' ')) {
                    // Put the line back for the next iteration
                    file.seekg(-static_cast<int>(paramLine.size()), std::ios_base::cur);
                    break;
                }

                // Skip leading whitespace
                size_t startPos = 0;
                while (startPos < paramLine.size() && (paramLine[startPos] == '\t' || paramLine[startPos] == ' ')) {
                    startPos++;
                }

                if (startPos >= paramLine.size()) continue;

                std::istringstream paramIss(paramLine.substr(startPos));
                std::string pType;
                if (!(paramIss >> pType) || pType != "p") continue;

                int paramIndex;
                std::string paramName;
                if (paramIss >> paramIndex >> paramName) {
                    // Store parameter mapping with method context
                    std::string paramKey = descriptor + "|" + obfuscatedName + "|" + std::to_string(paramIndex);
                    // We could store parameter mappings separately if needed
                    // For now, we'll just log them
                    Logger::Log("Mapped parameter: %s|%s|%d -> %s", descriptor.c_str(), obfuscatedName.c_str(), paramIndex, paramName.c_str());
                }
            }
        }
    }

    file.close();
    Logger::Log("Loaded mappings from %s", filename.c_str());
    return true;
}

std::string StrayCache::GetObfuscatedClassName(const std::string& className)
{
    // Look up human-readable class name to get obfuscated name
    auto it = s_classMapRev.find(className);
    if (it != s_classMapRev.end()) return it->second;
    // Fallback: return the name as-is (assumes already obfuscated)
    return className;
}

std::string StrayCache::GetObfuscatedFieldName(const std::string& classDescriptor,
                                              const std::string& fieldName,
                                              const std::string& fieldDescriptor)
{
    // First try to look up by human field name with descriptor
    std::string key = fieldName + "|" + fieldDescriptor;
    auto it = s_fieldMapRev.find(key);
    if (it != s_fieldMapRev.end()) return it->second;
    // Fallback: try to look up by human field name only
    it = s_fieldMapRev.find(fieldName);
    if (it != s_fieldMapRev.end()) return it->second;
    // Last resort: return fieldName as-is (assumes already obfuscated)
    return fieldName;
}

std::string StrayCache::GetObfuscatedMethodName(const std::string& classDescriptor,
                                               const std::string& methodName,
                                               const std::string& methodDescriptor)
{
    // First try to look up by human method name with descriptor
    std::string key = methodName + "|" + methodDescriptor;
    auto it = s_methodMapRev.find(key);
    if (it != s_methodMapRev.end()) return it->second;
    // Fallback: try to look up by human method name only
    it = s_methodMapRev.find(methodName);
    if (it != s_methodMapRev.end()) return it->second;
    // Last resort: return methodName as-is (assumes already obfuscated)
    return methodName;
}

jclass StrayCache::GetClass(const std::string& name)
{
    auto it = m_Classes.find(name);
    if (it != m_Classes.end()) return it->second;
    if (!m_Env) return nullptr;
    jclass c = m_Env->FindClass(name.c_str());
    if (c) {
        m_Classes[name] = (jclass)m_Env->NewGlobalRef(c);
        m_Env->DeleteLocalRef(c);
        return m_Classes[name];
    }
    return nullptr;
}

// New helper to get class from human name using mapping
jclass StrayCache::GetClassFromHuman(const std::string& humanClassName)
{
    std::string obfuscated = GetObfuscatedClassName(humanClassName);
    return GetClass(obfuscated);
}

// New helper to get field ID from human names
jfieldID StrayCache::GetFieldID(const std::string& humanClassName,
                               const std::string& fieldName,
                               const std::string& fieldDescriptor)
{
    if (!m_Env) return nullptr;
    std::string classDescriptor = "L" + humanClassName + ";";
    std::string obfuscatedField = GetObfuscatedFieldName(classDescriptor, fieldName, fieldDescriptor);
    // Get class using human name (will apply mapping internally)
    jclass cls = GetClassFromHuman(humanClassName);
    if (!cls) return nullptr;
    jfieldID fid = m_Env->GetFieldID(cls, obfuscatedField.c_str(), fieldDescriptor.c_str());
    // Cache the result? We'll rely on the caller to store it.
    return fid;
}

// New helper to get static field ID from human names
jfieldID StrayCache::GetStaticFieldID(const std::string& humanClassName,
                                     const std::string& fieldName,
                                     const std::string& fieldDescriptor)
{
    if (!m_Env) return nullptr;
    std::string classDescriptor = "L" + humanClassName + ";";
    std::string obfuscatedField = GetObfuscatedFieldName(classDescriptor, fieldName, fieldDescriptor);
    jclass cls = GetClassFromHuman(humanClassName);
    if (!cls) return nullptr;
    jfieldID fid = m_Env->GetStaticFieldID(cls, obfuscatedField.c_str(), fieldDescriptor.c_str());
    return fid;
}

// New helper to get method ID from human names
jmethodID StrayCache::GetMethodID(const std::string& humanClassName,
                                 const std::string& methodName,
                                 const std::string& methodDescriptor)
{
    if (!m_Env) return nullptr;
    std::string classDescriptor = "L" + humanClassName + ";";
    std::string obfuscatedMethod = GetObfuscatedMethodName(classDescriptor, methodName, methodDescriptor);
    jclass cls = GetClassFromHuman(humanClassName);
    if (!cls) return nullptr;
    jmethodID mid = m_Env->GetMethodID(cls, obfuscatedMethod.c_str(), methodDescriptor.c_str());
    return mid;
}

// New helper to get static method ID from human names
jmethodID StrayCache::GetStaticMethodID(const std::string& humanClassName,
                                       const std::string& methodName,
                                       const std::string& methodDescriptor)
{
    if (!m_Env) return nullptr;
    std::string classDescriptor = "L" + humanClassName + ";";
    std::string obfuscatedMethod = GetObfuscatedMethodName(classDescriptor, methodName, methodDescriptor);
    jclass cls = GetClassFromHuman(humanClassName);
    if (!cls) return nullptr;
    jmethodID mid = m_Env->GetStaticMethodID(cls, obfuscatedMethod.c_str(), methodDescriptor.c_str());
    return mid;
}

void StrayCache::Initialize(JNIEnv* env)
{
    m_Env = env;
    Logger::Log("StrayCache initializing");

    // Load Lunar client mappings from desktop
    std::string mappingsPath = "C:\\Users\\raw\\Desktop\\mappings\\mappings.txt";
    if (!LoadLunarMappings(mappingsPath)) {
        Logger::Log("Failed to load Lunar mappings, continuing with minimal initialization");
    }

    // Resolve and cache class references
    Minecraft = GetClassFromHuman("net/minecraft/client/Minecraft");
    ListClass = GetClassFromHuman("java/util/List");
    FloatBufferClass = GetClassFromHuman("java/nio/FloatBuffer");
    ActiveRenderInfoClass = GetClassFromHuman("net/minecraft/client/renderer/ActiveRenderInfo");

    // Resolve and cache field and method IDs using mappings
    // Entity fields
    Entity_posX = GetFieldID("net/minecraft/entity/Entity", "posX", "D");
    Entity_posY = GetFieldID("net/minecraft/entity/Entity", "posY", "D");
    Entity_posZ = GetFieldID("net/minecraft/entity/Entity", "posZ", "D");
    Entity_width = GetFieldID("net/minecraft/entity/Entity", "width", "F");
    Entity_height = GetFieldID("net/minecraft/entity/Entity", "height", "F");
    Entity_isDead = GetFieldID("net/minecraft/entity/Entity", "dead", "Z");
    Entity_motionX = GetFieldID("net/minecraft/entity/Entity", "motionX", "D");
    Entity_motionY = GetFieldID("net/minecraft/entity/Entity", "motionY", "D");
    Entity_motionZ = GetFieldID("net/minecraft/entity/Entity", "motionZ", "D");
    Entity_rotationYaw = GetFieldID("net/minecraft/entity/Entity", "rotationYaw", "F");
    Entity_rotationPitch = GetFieldID("net/minecraft/entity/Entity", "rotationPitch", "F");
    Entity_onGround = GetFieldID("net/minecraft/entity/Entity", "onGround", "Z");
    Entity_fallDistance = GetFieldID("net/minecraft/entity/Entity", "fallDistance", "F");

    // Entity methods
    Entity_getEntityId = GetMethodID("net/minecraft/entity/Entity", "getEntityId", "()I");
    Entity_getName = GetMethodID("net/minecraft/entity/Entity", "getName", "()Ljava/lang/String;");
    Entity_setSprinting = GetMethodID("net/minecraft/entity/Entity", "setSprinting", "(Z)V");
    Entity_isSneaking = GetMethodID("net/minecraft/entity/Entity", "isSneaking", "()Z");
    Entity_getDistanceToEntity = GetMethodID("net/minecraft/entity/Entity", "getDistanceToEntity", "(Lnet/minecraft/entity/Entity;)D");
    Entity_setRotationYaw = GetMethodID("net/minecraft/entity/Entity", "setRotationYaw", "(F)V");
    Entity_setRotationPitch = GetMethodID("net/minecraft/entity/Entity", "setRotationPitch", "(F)V");

    // PlayerControllerMP field
    PC_blockReachDistance = GetFieldID("net/minecraft/client/multiplayer/PlayerControllerMP", "blockReachDistance", "F");

    // List methods
    List_size = GetMethodID("java/util/List", "size", "()I");
    List_get = GetMethodID("java/util/List", "get", "(I)Ljava/lang/Object;"); // Fixed descriptor

    // FloatBuffer method
    FloatBuffer_get = GetMethodID("java/nio/FloatBuffer", "get", "(I)F"); // Fixed descriptor

    // ActiveRenderInfo fields (static)
    ActiveRenderInfo_PROJECTION = GetStaticFieldID("net/minecraft/client/renderer/ActiveRenderInfo", "projection", "Ljava/nio/FloatBuffer;");
    ActiveRenderInfo_MODELVIEW = GetStaticFieldID("net/minecraft/client/renderer/ActiveRenderInfo", "modelview", "Ljava/nio/FloatBuffer;");

    // World fields
    World_loadedEntityList = GetFieldID("net/minecraft/world/World", "loadedEntityList", "Ljava/util/List;");
    World_playerEntities = GetFieldID("net/minecraft/world/World", "playerEntities", "Ljava/util/List;");

    // Minecraft fields
    Minecraft_thePlayer = GetFieldID("net/minecraft/client/Minecraft", "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;");
    Minecraft_theWorld = GetFieldID("net/minecraft/client/Minecraft", "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;");
    Minecraft_renderManager = GetFieldID("net/minecraft/client/Minecraft", "renderManager", "Lnet/minecraft/client/renderer/EntityRenderer;");
    Minecraft_playerController = GetFieldID("net/minecraft/client/Minecraft", "playerController", "Lnet/minecraft/client/multiplayer/PlayerControllerMP;");
    Minecraft_objectMouseOver = GetFieldID("net/minecraft/client/Minecraft", "objectMouseOver", "Lnet/minecraft/util/MovingObjectPosition;");
    Minecraft_gameSettings = GetFieldID("net/minecraft/client/Minecraft", "gameSettings", "Lnet/minecraft/client/settings/GameSettings;");
    Minecraft_rightClickDelayTimer = GetFieldID("net/minecraft/client/Minecraft", "rightClickDelayTimer", "I");

    // Minecraft instance (singleton)
    if (Minecraft) {
        jfieldID mcField = GetStaticFieldID("net/minecraft/client/Minecraft", "mc", "Lnet/minecraft/client/Minecraft;");
        if (mcField) {
            jobject mcInstance = m_Env->GetStaticObjectField(Minecraft, mcField);
            if (mcInstance) {
                MinecraftInstance = m_Env->NewGlobalRef(mcInstance);
                m_Env->DeleteLocalRef(mcInstance);
            }
        }
    }

    // RenderManager fields
    RenderManager_viewerPosX = GetFieldID("net/minecraft/client/renderer/EntityRenderer", "viewerPosX", "D");
    RenderManager_viewerPosY = GetFieldID("net/minecraft/client/renderer/EntityRenderer", "viewerPosY", "D");
    RenderManager_viewerPosZ = GetFieldID("net/minecraft/client/renderer/EntityRenderer", "viewerPosZ", "D");

    // EntityLivingBase fields
    EntityLivingBase_hurtTime = GetFieldID("net/minecraft/entity/EntityLivingBase", "hurtTime", "I");
    EntityLivingBase_deathTime = GetFieldID("net/minecraft/entity/EntityLivingBase", "deathTime", "I");

    // EntityLivingBase methods
    EntityLivingBase_getHealth = GetMethodID("net/minecraft/entity/EntityLivingBase", "getHealth", "()F");
    EntityLivingBase_getMaxHealth = GetMethodID("net/minecraft/entity/EntityLivingBase", "getMaxHealth", "()F");
    EntityLivingBase_isUsingItem = GetMethodID("net/minecraft/entity/EntityLivingBase", "isUsingItem", "()Z");
    EntityLivingBase_getHeldItem = GetMethodID("net/minecraft/entity/EntityLivingBase", "getHeldItem", "()Lnet/minecraft/item/ItemStack;");

    // ItemStack method
    ItemStack_getItem = GetMethodID("net/minecraft/item/ItemStack", "getItem", "()Lnet/minecraft/item/Item;");

    // EntityPlayer fields
    EntityPlayer_inventory = GetFieldID("net/minecraft/entity/EntityPlayer", "inventory", "Lnet/minecraft/inventory/InventoryPlayer;");
    EntityPlayer_movementInput = GetFieldID("net/minecraft/entity/EntityPlayer", "movementInput", "Lnet/minecraft/client/settings/MovementInputFromOptions;");

    // GameSettings fields
    GameSettings_gammaSetting = GetFieldID("net/minecraft/client/settings/GameSettings", "gammaSetting", "F");
    GameSettings_keyBindForward = GetFieldID("net/minecraft/client/settings/GameSettings", "keyBindForward", "Lnet/minecraft/client/settings/KeyBinding;");
    GameSettings_keyBindJump = GetFieldID("net/minecraft/client/settings/GameSettings", "keyBindJump", "Lnet/minecraft/client/settings/KeyBinding;");
    GameSettings_keyBindSprint = GetFieldID("net/minecraft/client/settings/GameSettings", "keyBindSprint", "Lnet/minecraft/client/settings/KeyBinding;");
    GameSettings_keyBindSneak = GetFieldID("net/minecraft/client/settings/GameSettings", "keyBindSneak", "Lnet/minecraft/client/settings/KeyBinding;");

    Logger::Log("StrayCache initialization complete");
}

void StrayCache::TryField(JNIEnv* /*env*/, jclass /*cls*/, jfieldID& /*out*/, const char* /*obf*/, const char* /*deobf*/, const char* /*sig*/)
{
    // Intentionally empty - method left as placeholder for future mapping logic
}

void StrayCache::TryField2(JNIEnv* /*env*/, jclass /*cls*/, jfieldID& /*out*/, const char* /*obf*/, const char* /*deobf*/, const char* /*obfSig*/, const char* /*deobfSig*/)
{
}

void StrayCache::TryMethod(JNIEnv* /*env*/, jclass /*cls*/, jmethodID& /*out*/, const char* /*obf*/, const char* /*deobf*/, const char* /*sig*/)
{
}

void StrayCache::TryMethod2(JNIEnv* /*env*/, jclass /*cls*/, jmethodID& /*out*/, const char* /*obf*/, const char* /*deobf*/, const char* /*obfSig*/, const char* /*deobfSig*/)
{
}
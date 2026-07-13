#include "antibot.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"
#include <unordered_set>
#include <string>
#include <cstring>

static std::unordered_set<jint> s_BotIds;

static bool IsBotName(const std::string& name)
{
    if (name.empty()) return true;
    if (name.find("[NPC]") != std::string::npos) return true;
    if (name.find("NPC_") != std::string::npos) return true;
    if (name.find("Bot") != std::string::npos || name.find("bot") != std::string::npos) return true;
    if (name.length() > 16) return true;
    for (char c : name)
    {
        if (!isprint((unsigned char)c)) return true;
    }
    return false;
}

AntiBotModule::AntiBotModule()
    : ModuleBase("AntiBot", "Hide bots from view", Category::Combat)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Remove", "Invisible"});
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void AntiBotModule::OnEnable()
{
    Logger::Log("AntiBot enabled");
    s_BotIds.clear();
}

void AntiBotModule::OnDisable()
{
    Logger::Log("AntiBot disabled");
    s_BotIds.clear();

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    env->ExceptionClear();

    bool bridged = BridgeHelper::Initialize(env);
    jobject player = bridged ? BridgeHelper::GetPlayer(env) : nullptr;
    jobject world = bridged && player ? BridgeHelper::GetWorldFromEntity(env, player) : nullptr;
    jobject mc = nullptr;

    if (!world)
    {
        if (!bridged)
        {
            Java* java = Core::GetInstance().GetJava();
            if (!java || !java->IsValid()) return;
            jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
            if (!mcClass) return;
            jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
            if (!getMc) { env->DeleteLocalRef(mcClass); return; }
            mc = env->CallStaticObjectMethod(mcClass, getMc);
            if (!mc) { env->DeleteLocalRef(mcClass); return; }
            jfieldID wfId = env->GetFieldID(mcClass, "f", "Ladm;");
            if (!wfId) { env->ExceptionClear(); wfId = env->GetFieldID(mcClass, "theWorld", "Lnet/minecraft/world/World;"); }
            if (wfId) { env->ExceptionClear(); world = env->GetObjectField(mc, wfId); }
            env->DeleteLocalRef(mcClass);
        }
    }

    if (world)
    {
        jclass worldClass = env->GetObjectClass(world);
        jfieldID entitiesField = env->GetFieldID(worldClass, "loadedEntityList", "Ljava/util/List;");
        if (!entitiesField) { env->ExceptionClear(); entitiesField = env->GetFieldID(worldClass, "g", "Ljava/util/List;"); }
        env->ExceptionClear();

        if (entitiesField)
        {
            jobject listObj = env->GetObjectField(world, entitiesField);
            if (listObj)
            {
                jclass listClass = env->FindClass("java/util/List");
                jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
                jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
                if (sizeMethod && getMethod)
                {
                    int size = env->CallIntMethod(listObj, sizeMethod);
                    for (int i = 0; i < size; i++)
                    {
                        jobject entity = env->CallObjectMethod(listObj, getMethod, i);
                        if (!entity) continue;
                        if (player && env->IsSameObject(entity, player)) { env->DeleteLocalRef(entity); continue; }

                        jint eid = 0;
                        jclass eCls = env->FindClass("net/minecraft/entity/Entity");
                        jmethodID getEID = eCls ? env->GetMethodID(eCls, "getEntityId", "()I") : nullptr;
                        if (!getEID) { env->ExceptionClear(); }
                        if (getEID) { env->ExceptionClear(); eid = env->CallIntMethod(entity, getEID); }
                        if (eCls) env->DeleteLocalRef(eCls);

                        if (s_BotIds.count(eid))
                        {
                            jclass entCls = env->GetObjectClass(entity);
                            jfieldID invisField = env->GetFieldID(entCls, "isInvisible", "Z");
                            if (!invisField) { env->ExceptionClear(); }
                            if (invisField) { env->ExceptionClear(); env->SetBooleanField(entity, invisField, JNI_FALSE); }
                            env->DeleteLocalRef(entCls);
                        }
                        env->DeleteLocalRef(entity);
                    }
                }
                env->DeleteLocalRef(listClass);
                env->DeleteLocalRef(listObj);
            }
        }
        env->DeleteLocalRef(worldClass);
    }

    if (player) env->DeleteLocalRef(player);
    if (mc) env->DeleteLocalRef(mc);
}

void AntiBotModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    env->ExceptionClear();

    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();

    bool bridged = BridgeHelper::Initialize(env);
    jobject player = bridged ? BridgeHelper::GetPlayer(env) : nullptr;
    jobject world = bridged && player ? BridgeHelper::GetWorldFromEntity(env, player) : nullptr;
    jobject mc = nullptr;

    if (!world)
    {
        if (!bridged)
        {
            Java* java = Core::GetInstance().GetJava();
            if (!java || !java->IsValid()) return;
            jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
            if (!mcClass) return;
            jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
            if (!getMc) { env->DeleteLocalRef(mcClass); return; }
            mc = env->CallStaticObjectMethod(mcClass, getMc);
            if (!mc) { env->DeleteLocalRef(mcClass); return; }
            jfieldID wfId = env->GetFieldID(mcClass, "f", "Ladm;");
            if (!wfId) { env->ExceptionClear(); wfId = env->GetFieldID(mcClass, "theWorld", "Lnet/minecraft/world/World;"); }
            if (wfId) { env->ExceptionClear(); world = env->GetObjectField(mc, wfId); }
            env->DeleteLocalRef(mcClass);
        }
    }

    if (!world) { if (player) env->DeleteLocalRef(player); return; }

    jclass worldClass = env->GetObjectClass(world);
    jfieldID entitiesField = env->GetFieldID(worldClass, "loadedEntityList", "Ljava/util/List;");
    if (!entitiesField) { env->ExceptionClear(); entitiesField = env->GetFieldID(worldClass, "g", "Ljava/util/List;"); }
    env->ExceptionClear();

    if (!entitiesField) { env->DeleteLocalRef(worldClass); env->DeleteLocalRef(world); if (player) env->DeleteLocalRef(player); return; }

    jobject listObj = env->GetObjectField(world, entitiesField);
    if (!listObj) { env->DeleteLocalRef(worldClass); env->DeleteLocalRef(world); if (player) env->DeleteLocalRef(player); return; }

    jclass listClass = env->FindClass("java/util/List");
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    if (!sizeMethod || !getMethod)
    {
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(listObj);
        env->DeleteLocalRef(worldClass);
        env->DeleteLocalRef(world);
        if (player) env->DeleteLocalRef(player);
        return;
    }

    int size = env->CallIntMethod(listObj, sizeMethod);

    jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
    if (!entityClass) { env->ExceptionClear(); entityClass = env->FindClass("pk"); }

    jmethodID getNameMethod = entityClass ? env->GetMethodID(entityClass, "getName", "()Ljava/lang/String;") : nullptr;
    if (!getNameMethod && entityClass) { env->ExceptionClear(); getNameMethod = env->GetMethodID(entityClass, "c", "()Ljava/lang/String;"); }
    if (getNameMethod) env->ExceptionClear();

    jmethodID getEID = entityClass ? env->GetMethodID(entityClass, "getEntityId", "()I") : nullptr;
    if (!getEID && entityClass) { env->ExceptionClear(); getEID = env->GetMethodID(entityClass, "F", "()I"); }
    if (getEID) env->ExceptionClear();

    for (int i = 0; i < size; i++)
    {
        jobject entity = env->CallObjectMethod(listObj, getMethod, i);
        if (!entity) continue;
        if (player && env->IsSameObject(entity, player)) { env->DeleteLocalRef(entity); continue; }

        jint eid = getEID ? env->CallIntMethod(entity, getEID) : 0;
        std::string entName;
        if (getNameMethod)
        {
            jobject jname = env->CallObjectMethod(entity, getNameMethod);
            if (jname)
            {
                const char* chars = env->GetStringUTFChars((jstring)jname, nullptr);
                if (chars) { entName = chars; env->ReleaseStringUTFChars((jstring)jname, chars); }
                env->DeleteLocalRef(jname);
            }
        }

        bool isBot = IsBotName(entName);
        if (isBot)
        {
            s_BotIds.insert(eid);
            if (mode == 1)
            {
                jclass entCls = env->GetObjectClass(entity);
                jfieldID invisField = env->GetFieldID(entCls, "isInvisible", "Z");
                if (!invisField) { env->ExceptionClear(); }
                if (invisField) { env->ExceptionClear(); env->SetBooleanField(entity, invisField, JNI_TRUE); }
                env->DeleteLocalRef(entCls);
            }
        }
        env->DeleteLocalRef(entity);
    }

    if (entityClass) env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(listObj);
    env->DeleteLocalRef(worldClass);
    env->DeleteLocalRef(world);
    if (player) env->DeleteLocalRef(player);
    if (mc) env->DeleteLocalRef(mc);
}

#include "antibot.h"
#include "../../../java/java.h"
#include "../../../sdk/strayCache.h"
#include "../../../utilities/jni_helpers.h"
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
    if (!StrayCache::Minecraft || !StrayCache::MinecraftInstance) return;

    jobject mc = StrayCache::MinecraftInstance;
    jobject localPlayer = GetPlayerObject(env, mc);
    jobject world = GetWorldObject(env, mc);

    if (world)
    {
        jfieldID entitiesField = env->GetFieldID(StrayCache::World, "g", "Ljava/util/List;");
        if (!entitiesField) { env->ExceptionClear(); entitiesField = env->GetFieldID(StrayCache::World, "loadedEntityList", "Ljava/util/List;"); }
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

                    jfieldID invisField = nullptr;
                    if (StrayCache::Entity)
                    {
                        invisField = env->GetFieldID(StrayCache::Entity, "au", "Z");
                        if (!invisField) { env->ExceptionClear(); invisField = env->GetFieldID(StrayCache::Entity, "isInvisible", "Z"); }
                        env->ExceptionClear();
                    }

                    for (int i = 0; i < size; i++)
                    {
                        jobject entity = env->CallObjectMethod(listObj, getMethod, i);
                        if (!entity) continue;
                        if (localPlayer && env->IsSameObject(entity, localPlayer)) { env->DeleteLocalRef(entity); continue; }

                        jint eid = 0;
                        jmethodID getEID = env->GetMethodID(StrayCache::Entity, "F", "()I");
                        if (!getEID) { env->ExceptionClear(); getEID = env->GetMethodID(StrayCache::Entity, "getEntityId", "()I"); }
                        env->ExceptionClear();
                        if (getEID) eid = env->CallIntMethod(entity, getEID);

                        if (s_BotIds.count(eid))
                        {
                            if (invisField)
                                env->SetBooleanField(entity, invisField, JNI_FALSE);
                        }

                        env->DeleteLocalRef(entity);
                    }
                }
                env->DeleteLocalRef(listClass);
                env->DeleteLocalRef(listObj);
            }
        }
    }

    if (localPlayer) env->DeleteLocalRef(localPlayer);
    if (world) env->DeleteLocalRef(world);
}

void AntiBotModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!StrayCache::Minecraft || !StrayCache::MinecraftInstance || !StrayCache::World || !StrayCache::Entity || !StrayCache::EntityPlayer) return;

    jobject mc = StrayCache::MinecraftInstance;
    jobject localPlayer = GetPlayerObject(env, mc);
    jobject world = GetWorldObject(env, mc);

    if (!world) { if (localPlayer) env->DeleteLocalRef(localPlayer); return; }

    jfieldID entitiesField = env->GetFieldID(StrayCache::World, "g", "Ljava/util/List;");
    if (!entitiesField) { env->ExceptionClear(); entitiesField = env->GetFieldID(StrayCache::World, "loadedEntityList", "Ljava/util/List;"); }
    env->ExceptionClear();

    if (!entitiesField) { env->DeleteLocalRef(world); if (localPlayer) env->DeleteLocalRef(localPlayer); return; }

    jobject listObj = env->GetObjectField(world, entitiesField);
    if (!listObj) { env->DeleteLocalRef(world); if (localPlayer) env->DeleteLocalRef(localPlayer); return; }

    jclass listClass = env->FindClass("java/util/List");
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    if (!sizeMethod || !getMethod)
    {
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(listObj);
        env->DeleteLocalRef(world);
        if (localPlayer) env->DeleteLocalRef(localPlayer);
        return;
    }

    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();
    int size = env->CallIntMethod(listObj, sizeMethod);

    jfieldID invisField = env->GetFieldID(StrayCache::Entity, "au", "Z");
    if (!invisField) { env->ExceptionClear(); invisField = env->GetFieldID(StrayCache::Entity, "isInvisible", "Z"); }
    env->ExceptionClear();

    jmethodID getNameMethod = env->GetMethodID(StrayCache::Entity, "c", "()Ljava/lang/String;");
    if (!getNameMethod) { env->ExceptionClear(); getNameMethod = env->GetMethodID(StrayCache::Entity, "getName", "()Ljava/lang/String;"); }
    env->ExceptionClear();

    jmethodID getEID = env->GetMethodID(StrayCache::Entity, "F", "()I");
    if (!getEID) { env->ExceptionClear(); getEID = env->GetMethodID(StrayCache::Entity, "getEntityId", "()I"); }
    env->ExceptionClear();

    for (int i = 0; i < size; i++)
    {
        jobject entity = env->CallObjectMethod(listObj, getMethod, i);
        if (!entity) continue;
        if (localPlayer && env->IsSameObject(entity, localPlayer)) { env->DeleteLocalRef(entity); continue; }

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

            if (mode == 1 && invisField)
                env->SetBooleanField(entity, invisField, JNI_TRUE);
        }

        env->DeleteLocalRef(entity);
    }

    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(listObj);
    env->DeleteLocalRef(world);
    if (localPlayer) env->DeleteLocalRef(localPlayer);
}

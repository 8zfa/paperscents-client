#include "antibot.h"
#include "../../../core.h"
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
    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    jclass mcClass = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) { env->ExceptionClear(); mcClass = env->FindClass("net/minecraft/client/Minecraft"); }
    if (!mcClass) return;

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "a", "()Lave;"); }
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "getMinecraft", "()Lave;"); }
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    env->ExceptionClear();

    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    jfieldID playerField = env->GetFieldID(mcClass, "h", "Lbew;");
    if (!playerField) { env->ExceptionClear(); playerField = env->GetFieldID(mcClass, "thePlayer", "Lbew;"); }
    env->ExceptionClear();
    jobject localPlayer = playerField ? env->GetObjectField(mc, playerField) : nullptr;

    jfieldID worldField = env->GetFieldID(mcClass, "f", "Ladm;");
    if (!worldField) { env->ExceptionClear(); worldField = env->GetFieldID(mcClass, "theWorld", "Ladm;"); }
    env->ExceptionClear();
    jobject world = worldField ? env->GetObjectField(mc, worldField) : nullptr;

    if (world)
    {
        jclass worldClass = env->GetObjectClass(world);
        jfieldID entitiesField = env->GetFieldID(worldClass, "g", "Ljava/util/List;");
        if (!entitiesField) { env->ExceptionClear(); entitiesField = env->GetFieldID(worldClass, "loadedEntityList", "Ljava/util/List;"); }
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
                    jclass entityClass = Core::GetInstance().GetJava()->FindClass("pk", "net/minecraft/entity/Entity");
                    if (!entityClass) { env->ExceptionClear(); entityClass = env->FindClass("net/minecraft/entity/Entity"); }
                    jfieldID invisField = nullptr;
                    if (entityClass)
                    {
                        invisField = env->GetFieldID(entityClass, "au", "Z");
                        if (!invisField) { env->ExceptionClear(); invisField = env->GetFieldID(entityClass, "isInvisible", "Z"); }
                        env->ExceptionClear();
                    }

                    for (int i = 0; i < size; i++)
                    {
                        jobject entity = env->CallObjectMethod(listObj, getMethod, i);
                        if (!entity) continue;
                        if (localPlayer && env->IsSameObject(entity, localPlayer)) { env->DeleteLocalRef(entity); continue; }

                        jint eid = 0;
                        jmethodID getEID = env->GetMethodID(entityClass, "F", "()I");
                        if (!getEID) { env->ExceptionClear(); getEID = env->GetMethodID(entityClass, "getEntityId", "()I"); }
                        env->ExceptionClear();
                        if (getEID) eid = env->CallIntMethod(entity, getEID);

                        if (s_BotIds.count(eid))
                        {
                            if (invisField)
                                env->SetBooleanField(entity, invisField, JNI_FALSE);
                        }

                        env->DeleteLocalRef(entity);
                    }

                    if (entityClass) env->DeleteLocalRef(entityClass);
                }
                env->DeleteLocalRef(listClass);
                env->DeleteLocalRef(listObj);
            }
        }
        env->DeleteLocalRef(worldClass);
    }

    if (localPlayer) env->DeleteLocalRef(localPlayer);
    if (world) env->DeleteLocalRef(world);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

void AntiBotModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    jclass mcClass = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass) { env->ExceptionClear(); mcClass = env->FindClass("net/minecraft/client/Minecraft"); }
    if (!mcClass) return;

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "a", "()Lave;"); }
    if (!getMc) { env->ExceptionClear(); getMc = env->GetStaticMethodID(mcClass, "getMinecraft", "()Lave;"); }
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    env->ExceptionClear();

    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    jfieldID playerField = env->GetFieldID(mcClass, "h", "Lbew;");
    if (!playerField) { env->ExceptionClear(); playerField = env->GetFieldID(mcClass, "thePlayer", "Lbew;"); }
    env->ExceptionClear();
    jobject localPlayer = playerField ? env->GetObjectField(mc, playerField) : nullptr;

    jfieldID worldField = env->GetFieldID(mcClass, "f", "Ladm;");
    if (!worldField) { env->ExceptionClear(); worldField = env->GetFieldID(mcClass, "theWorld", "Ladm;"); }
    env->ExceptionClear();
    jobject world = worldField ? env->GetObjectField(mc, worldField) : nullptr;

    if (!world) { if (localPlayer) env->DeleteLocalRef(localPlayer); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass worldClass = env->GetObjectClass(world);
    jfieldID entitiesField = env->GetFieldID(worldClass, "g", "Ljava/util/List;");
    if (!entitiesField) { env->ExceptionClear(); entitiesField = env->GetFieldID(worldClass, "loadedEntityList", "Ljava/util/List;"); }
    env->ExceptionClear();

    if (!entitiesField)
    {
        env->DeleteLocalRef(worldClass);
        env->DeleteLocalRef(world);
        if (localPlayer) env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    jobject listObj = env->GetObjectField(world, entitiesField);
    if (!listObj)
    {
        env->DeleteLocalRef(worldClass);
        env->DeleteLocalRef(world);
        if (localPlayer) env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    jclass listClass = env->FindClass("java/util/List");
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    if (!sizeMethod || !getMethod)
    {
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(listObj);
        env->DeleteLocalRef(worldClass);
        env->DeleteLocalRef(world);
        if (localPlayer) env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();
    int size = env->CallIntMethod(listObj, sizeMethod);

    jclass entityClass = Core::GetInstance().GetJava()->FindClass("pk", "net/minecraft/entity/Entity");
    if (!entityClass) { env->ExceptionClear(); entityClass = env->FindClass("net/minecraft/entity/Entity"); }
    if (!entityClass)
    {
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(listObj);
        env->DeleteLocalRef(worldClass);
        env->DeleteLocalRef(world);
        if (localPlayer) env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    jfieldID invisField = env->GetFieldID(entityClass, "au", "Z");
    if (!invisField) { env->ExceptionClear(); invisField = env->GetFieldID(entityClass, "isInvisible", "Z"); }
    env->ExceptionClear();

    jmethodID getNameMethod = env->GetMethodID(entityClass, "c", "()Ljava/lang/String;");
    if (!getNameMethod) { env->ExceptionClear(); getNameMethod = env->GetMethodID(entityClass, "getName", "()Ljava/lang/String;"); }
    env->ExceptionClear();

    jmethodID getEID = env->GetMethodID(entityClass, "F", "()I");
    if (!getEID) { env->ExceptionClear(); getEID = env->GetMethodID(entityClass, "getEntityId", "()I"); }
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

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(listObj);
    env->DeleteLocalRef(worldClass);
    env->DeleteLocalRef(world);
    if (localPlayer) env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

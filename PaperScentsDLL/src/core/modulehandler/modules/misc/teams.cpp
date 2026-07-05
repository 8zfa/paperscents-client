#include "teams.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"

TeamsModule::TeamsModule()
    : ModuleBase("Teams", "Respect team colors in attacks", Category::Misc)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Color", "Scoreboard", "Both"});
}

void TeamsModule::OnEnable() { Logger::Log("Teams enabled"); }
void TeamsModule::OnDisable() { Logger::Log("Teams disabled"); }

void TeamsModule::OnUpdate()
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
    if (!localPlayer) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jfieldID worldField = env->GetFieldID(mcClass, "f", "Ladm;");
    if (!worldField) { env->ExceptionClear(); worldField = env->GetFieldID(mcClass, "theWorld", "Ladm;"); }
    env->ExceptionClear();

    jobject world = worldField ? env->GetObjectField(mc, worldField) : nullptr;
    if (!world) { env->DeleteLocalRef(localPlayer); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass worldClass = env->GetObjectClass(world);
    jfieldID scoreboardField = env->GetFieldID(worldClass, "a", "Lavz;");
    if (!scoreboardField) { env->ExceptionClear(); scoreboardField = env->GetFieldID(worldClass, "scoreboard", "Lavz;"); }
    env->ExceptionClear();

    jobject scoreboard = scoreboardField ? env->GetObjectField(world, scoreboardField) : nullptr;

    jfieldID entitiesField = env->GetFieldID(worldClass, "g", "Ljava/util/List;");
    if (!entitiesField) { env->ExceptionClear(); entitiesField = env->GetFieldID(worldClass, "loadedEntityList", "Ljava/util/List;"); }
    env->ExceptionClear();

    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();

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
                jclass playerClass = Core::GetInstance().GetJava()->FindClass("wn", "net/minecraft/entity/player/EntityPlayer");
                if (!playerClass) { env->ExceptionClear(); playerClass = env->FindClass("net/minecraft/entity/player/EntityPlayer"); }

                jmethodID getNameMethod = nullptr;
                jmethodID getDisplayNameMethod = nullptr;
                jmethodID getTeamMethod = nullptr;

                if (mode == 0 || mode == 2)
                {
                    getNameMethod = env->GetMethodID(entityClass, "c", "()Ljava/lang/String;");
                    if (!getNameMethod) { env->ExceptionClear(); getNameMethod = env->GetMethodID(entityClass, "getName", "()Ljava/lang/String;"); }
                    env->ExceptionClear();

                    getDisplayNameMethod = env->GetMethodID(entityClass, "d", "()Ljava/lang/String;");
                    if (!getDisplayNameMethod) { env->ExceptionClear(); getDisplayNameMethod = env->GetMethodID(entityClass, "getDisplayName", "()Ljava/lang/String;"); }
                    env->ExceptionClear();
                }

                if (mode == 1 || mode == 2)
                {
                    getTeamMethod = env->GetMethodID(entityClass, "ae", "()Lavz$b;");
                    if (!getTeamMethod) { env->ExceptionClear(); getTeamMethod = env->GetMethodID(entityClass, "getTeam", "()Lnet/minecraft/scoreboard/ScorePlayerTeam;"); }
                    env->ExceptionClear();
                }

                jclass scoreboardClass = scoreboard ? env->GetObjectClass(scoreboard) : nullptr;
                jmethodID getPlayersTeam = nullptr;
                if (scoreboardClass)
                {
                    getPlayersTeam = env->GetMethodID(scoreboardClass, "c", "(Ljava/lang/String;)Lavz$b;");
                    if (!getPlayersTeam) { env->ExceptionClear(); getPlayersTeam = env->GetMethodID(scoreboardClass, "getPlayersTeam", "(Ljava/lang/String;)Lnet/minecraft/scoreboard/ScorePlayerTeam;"); }
                    env->ExceptionClear();
                }

                for (int i = 0; i < size; i++)
                {
                    jobject entity = env->CallObjectMethod(listObj, getMethod, i);
                    if (!entity) continue;
                    if (env->IsSameObject(entity, localPlayer)) { env->DeleteLocalRef(entity); continue; }
                    if (!playerClass || !env->IsInstanceOf(entity, playerClass)) { env->DeleteLocalRef(entity); continue; }

                    std::string entName;
                    std::string displayName;

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

                    if (getDisplayNameMethod)
                    {
                        jobject jdn = env->CallObjectMethod(entity, getDisplayNameMethod);
                        if (jdn)
                        {
                            const char* chars = env->GetStringUTFChars((jstring)jdn, nullptr);
                            if (chars) { displayName = chars; env->ReleaseStringUTFChars((jstring)jdn, chars); }
                            env->DeleteLocalRef(jdn);
                        }
                    }

                    bool sameTeam = false;

                    if ((mode == 0 || mode == 2) && !displayName.empty())
                    {
                        if (displayName.find('\u00a7') != std::string::npos)
                        {
                            size_t pos = displayName.find('\u00a7');
                            if (pos + 1 < displayName.length())
                            {
                                char colorCode = displayName[pos + 1];
                                std::string colorStr = "\u00a7";
                                colorStr += colorCode;

                                if (!entName.empty())
                                {
                                    std::string localName;
                                    if (getNameMethod)
                                    {
                                        jobject jln = env->CallObjectMethod(localPlayer, getNameMethod);
                                        if (jln)
                                        {
                                            const char* lchars = env->GetStringUTFChars((jstring)jln, nullptr);
                                            if (lchars) { localName = lchars; env->ReleaseStringUTFChars((jstring)jln, lchars); }
                                            env->DeleteLocalRef(jln);
                                        }
                                    }

                                    if (!localName.empty())
                                    {
                                        jclass localPlayerClass = env->GetObjectClass(localPlayer);
                                        jmethodID localGetDisplayName = env->GetMethodID(localPlayerClass, "d", "()Ljava/lang/String;");
                                        if (!localGetDisplayName) { env->ExceptionClear(); localGetDisplayName = env->GetMethodID(localPlayerClass, "getDisplayName", "()Ljava/lang/String;"); }
                                        env->ExceptionClear();

                                        std::string localDisplayName;
                                        if (localGetDisplayName)
                                        {
                                            jobject jldn = env->CallObjectMethod(localPlayer, localGetDisplayName);
                                            if (jldn)
                                            {
                                                const char* lchars = env->GetStringUTFChars((jstring)jldn, nullptr);
                                                if (lchars) { localDisplayName = lchars; env->ReleaseStringUTFChars((jstring)jldn, lchars); }
                                                env->DeleteLocalRef(jldn);
                                            }
                                        }
                                        env->DeleteLocalRef(localPlayerClass);

                                        if (localDisplayName.find(colorStr) != std::string::npos)
                                            sameTeam = true;
                                    }
                                }
                            }
                        }
                    }

                    if ((mode == 1 || mode == 2) && scoreboard && getPlayersTeam)
                    {
                        if (!entName.empty())
                        {
                            std::string localName;
                            jmethodID localGetName = env->GetMethodID(entityClass, "c", "()Ljava/lang/String;");
                            if (!localGetName) { env->ExceptionClear(); localGetName = env->GetMethodID(entityClass, "getName", "()Ljava/lang/String;"); }
                            env->ExceptionClear();
                            if (localGetName)
                            {
                                jobject jln = env->CallObjectMethod(localPlayer, localGetName);
                                if (jln)
                                {
                                    const char* lchars = env->GetStringUTFChars((jstring)jln, nullptr);
                                    if (lchars) { localName = lchars; env->ReleaseStringUTFChars((jstring)jln, lchars); }
                                    env->DeleteLocalRef(jln);
                                }
                            }

                            if (!localName.empty())
                            {
                                jobject localTeamObj = env->CallObjectMethod(scoreboard, getPlayersTeam, env->NewStringUTF(localName.c_str()));
                                jobject entTeamObj = env->CallObjectMethod(scoreboard, getPlayersTeam, env->NewStringUTF(entName.c_str()));

                                if (localTeamObj && entTeamObj)
                                {
                                    jclass teamClass = env->GetObjectClass(localTeamObj);
                                    jmethodID getRegisteredName = env->GetMethodID(teamClass, "a", "()Ljava/lang/String;");
                                    if (!getRegisteredName) { env->ExceptionClear(); getRegisteredName = env->GetMethodID(teamClass, "getRegisteredName", "()Ljava/lang/String;"); }
                                    env->ExceptionClear();

                                    if (getRegisteredName)
                                    {
                                        jobject localRegName = env->CallObjectMethod(localTeamObj, getRegisteredName);
                                        jobject entRegName = env->CallObjectMethod(entTeamObj, getRegisteredName);
                                        if (localRegName && entRegName)
                                        {
                                            jstring jlrn = (jstring)localRegName;
                                            jstring jern = (jstring)entRegName;
                                            if (env->IsSameObject(jlrn, jern))
                                                sameTeam = true;
                                        }
                                        if (localRegName) env->DeleteLocalRef(localRegName);
                                        if (entRegName) env->DeleteLocalRef(entRegName);
                                    }
                                    env->DeleteLocalRef(teamClass);
                                }
                                if (localTeamObj) env->DeleteLocalRef(localTeamObj);
                                if (entTeamObj) env->DeleteLocalRef(entTeamObj);
                            }
                        }
                    }

                    env->DeleteLocalRef(entity);
                }

                if (entityClass) env->DeleteLocalRef(entityClass);
                if (playerClass) env->DeleteLocalRef(playerClass);
                if (scoreboardClass) env->DeleteLocalRef(scoreboardClass);
            }
            env->DeleteLocalRef(listClass);
            env->DeleteLocalRef(listObj);
        }
    }

    if (scoreboard) env->DeleteLocalRef(scoreboard);
    env->DeleteLocalRef(worldClass);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

#include "antivoid.h"
#include "../../../java/java.h"
#include "../../../sdk/strayCache.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../utilities/logger.h"

AntiVoidModule::AntiVoidModule()
    : ModuleBase("AntiVoid", "Prevent falling into the void", Category::Player)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Packet", "Motion"});
    AddSetting<NumberSetting>("FallDist", 5.0f, 3.0f, 20.0f, 1.0f, "Fall distance threshold");
}

void AntiVoidModule::OnEnable() { Logger::Log("AntiVoid enabled"); }
void AntiVoidModule::OnDisable() { Logger::Log("AntiVoid disabled"); }

void AntiVoidModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!StrayCache::Minecraft || !StrayCache::MinecraftInstance || !StrayCache::Entity) return;

    jobject mc = StrayCache::MinecraftInstance;
    jobject player = GetPlayerObject(env, mc);
    if (!player) return;

    jfieldID posYID = env->GetFieldID(StrayCache::Entity, "s", "D");
    if (!posYID) { env->ExceptionClear(); posYID = env->GetFieldID(StrayCache::Entity, "posY", "D"); }
    env->ExceptionClear();

    jfieldID motionYID = env->GetFieldID(StrayCache::Entity, "w", "D");
    if (!motionYID) { env->ExceptionClear(); motionYID = env->GetFieldID(StrayCache::Entity, "motionY", "D"); }
    env->ExceptionClear();

    jfieldID onGroundID = env->GetFieldID(StrayCache::Entity, "I", "Z");
    if (!onGroundID) { env->ExceptionClear(); onGroundID = env->GetFieldID(StrayCache::Entity, "onGround", "Z"); }
    env->ExceptionClear();

    jfieldID posXID = env->GetFieldID(StrayCache::Entity, "r", "D");
    if (!posXID) { env->ExceptionClear(); posXID = env->GetFieldID(StrayCache::Entity, "posX", "D"); }
    env->ExceptionClear();

    jfieldID posZID = env->GetFieldID(StrayCache::Entity, "t", "D");
    if (!posZID) { env->ExceptionClear(); posZID = env->GetFieldID(StrayCache::Entity, "posZ", "D"); }
    env->ExceptionClear();

    if (!posYID || !motionYID) { env->DeleteLocalRef(player); return; }

    double posY = env->GetDoubleField(player, posYID);
    double motionY = env->GetDoubleField(player, motionYID);
    bool onGround = onGroundID && env->GetBooleanField(player, onGroundID);
    float threshold = ((NumberSetting*)FindSetting("FallDist"))->GetValue();
    int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();

    static double lastGroundX = 0.0, lastGroundY = 0.0, lastGroundZ = 0.0;
    static bool hasLastGround = false;

    if (onGround)
    {
        if (posXID && posZID)
        {
            lastGroundX = env->GetDoubleField(player, posXID);
            lastGroundY = posY;
            lastGroundZ = env->GetDoubleField(player, posZID);
        }
        hasLastGround = true;
    }

    if (posY < (double)threshold && motionY < 0.0 && !onGround && hasLastGround)
    {
        if (mode == 0)
        {
            jfieldID nethandlerField = env->GetFieldID(StrayCache::Minecraft, "q", "Leh;");
            if (!nethandlerField) { env->ExceptionClear(); nethandlerField = env->GetFieldID(StrayCache::Minecraft, "v", "Leh;"); }
            if (!nethandlerField) { env->ExceptionClear(); nethandlerField = env->GetFieldID(StrayCache::Minecraft, "getNetHandler", "Leh;"); }
            env->ExceptionClear();

            if (nethandlerField)
            {
                jobject nh = env->GetObjectField(mc, nethandlerField);
                if (nh)
                {
                    jclass nhClass = env->GetObjectClass(nh);
                    jmethodID sendMethod = env->GetMethodID(nhClass, "a", "(Lhb;)V");
                    if (!sendMethod) { env->ExceptionClear(); sendMethod = env->GetMethodID(nhClass, "addToSendQueue", "(Lnet/minecraft/network/Packet;)V"); }
                    env->ExceptionClear();

                    if (sendMethod)
                    {
                        jclass c03Class = env->FindClass("hg");
                        if (!c03Class) { env->ExceptionClear(); c03Class = env->FindClass("net/minecraft/network/play/client/C03PacketPlayer"); }
                        if (c03Class)
                        {
                            jmethodID c03Ctor = env->GetMethodID(c03Class, "<init>", "(DDDZ)V");
                            if (!c03Ctor) { env->ExceptionClear(); c03Ctor = env->GetMethodID(c03Class, "<init>", "()V"); }
                            env->ExceptionClear();
                            if (c03Ctor)
                            {
                                jobject packet = env->NewObject(c03Class, c03Ctor, lastGroundX, lastGroundY + 0.1, lastGroundZ, JNI_TRUE);
                                if (packet)
                                {
                                    env->CallVoidMethod(nh, sendMethod, packet);
                                    env->DeleteLocalRef(packet);
                                }
                            }
                            env->DeleteLocalRef(c03Class);
                        }
                    }
                    env->DeleteLocalRef(nh);
                    env->DeleteLocalRef(nhClass);
                }
            }
        }
        else
        {
            env->SetDoubleField(player, motionYID, 0.0);
        }
    }

    env->DeleteLocalRef(player);
}

#include "antivoid.h"
#include "../../../core.h"
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

    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    jclass mcClass = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (!mcClass)
    {
        env->ExceptionClear();
        mcClass = env->FindClass("net/minecraft/client/Minecraft");
    }
    if (!mcClass) return;

    jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMc)
    {
        env->ExceptionClear();
        getMc = env->GetStaticMethodID(mcClass, "a", "()Lave;");
    }
    if (!getMc)
    {
        env->ExceptionClear();
        getMc = env->GetStaticMethodID(mcClass, "getMinecraft", "()Lave;");
    }
    if (!getMc) { env->DeleteLocalRef(mcClass); return; }
    env->ExceptionClear();

    jobject mc = env->CallStaticObjectMethod(mcClass, getMc);
    if (!mc) { env->DeleteLocalRef(mcClass); return; }

    jfieldID playerField = env->GetFieldID(mcClass, "h", "Lbew;");
    if (!playerField)
    {
        env->ExceptionClear();
        playerField = env->GetFieldID(mcClass, "thePlayer", "Lbew;");
    }
    env->ExceptionClear();

    jobject player = playerField ? env->GetObjectField(mc, playerField) : nullptr;
    if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass entityClass = Core::GetInstance().GetJava()->FindClass("pk", "net/minecraft/entity/Entity");
    if (!entityClass)
    {
        env->ExceptionClear();
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    jfieldID posYID = env->GetFieldID(entityClass, "s", "D");
    if (!posYID)
    {
        env->ExceptionClear();
        posYID = env->GetFieldID(entityClass, "posY", "D");
    }
    env->ExceptionClear();

    jfieldID motionYID = env->GetFieldID(entityClass, "w", "D");
    if (!motionYID)
    {
        env->ExceptionClear();
        motionYID = env->GetFieldID(entityClass, "motionY", "D");
    }
    env->ExceptionClear();

    jfieldID onGroundID = env->GetFieldID(entityClass, "I", "Z");
    if (!onGroundID)
    {
        env->ExceptionClear();
        onGroundID = env->GetFieldID(entityClass, "onGround", "Z");
    }
    env->ExceptionClear();

    jfieldID posXID = env->GetFieldID(entityClass, "r", "D");
    if (!posXID)
    {
        env->ExceptionClear();
        posXID = env->GetFieldID(entityClass, "posX", "D");
    }
    env->ExceptionClear();

    jfieldID posZID = env->GetFieldID(entityClass, "t", "D");
    if (!posZID)
    {
        env->ExceptionClear();
        posZID = env->GetFieldID(entityClass, "posZ", "D");
    }
    env->ExceptionClear();

    if (!posYID || !motionYID)
    {
        env->DeleteLocalRef(entityClass);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

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
            jfieldID nethandlerField = env->GetFieldID(mcClass, "q", "Leh;");
            if (!nethandlerField)
            {
                env->ExceptionClear();
                nethandlerField = env->GetFieldID(mcClass, "v", "Leh;");
            }
            if (!nethandlerField)
            {
                env->ExceptionClear();
                nethandlerField = env->GetFieldID(mcClass, "getNetHandler", "Leh;");
            }
            env->ExceptionClear();

            if (nethandlerField)
            {
                jobject nh = env->GetObjectField(mc, nethandlerField);
                if (nh)
                {
                    jclass nhClass = env->GetObjectClass(nh);
                    jmethodID sendMethod = env->GetMethodID(nhClass, "a", "(Lhb;)V");
                    if (!sendMethod)
                    {
                        env->ExceptionClear();
                        sendMethod = env->GetMethodID(nhClass, "addToSendQueue", "(Lnet/minecraft/network/Packet;)V");
                    }
                    env->ExceptionClear();

                    if (sendMethod)
                    {
                        jclass c03Class = Core::GetInstance().GetJava()->FindClass("hg", "net/minecraft/network/play/client/C03PacketPlayer");
                        if (!c03Class)
                        {
                            env->ExceptionClear();
                            c03Class = env->FindClass("net/minecraft/network/play/client/C03PacketPlayer");
                        }
                        if (c03Class)
                        {
                            jmethodID c03Ctor = env->GetMethodID(c03Class, "<init>", "(DDDZ)V");
                            if (!c03Ctor)
                            {
                                env->ExceptionClear();
                                c03Ctor = env->GetMethodID(c03Class, "<init>", "()V");
                            }
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

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

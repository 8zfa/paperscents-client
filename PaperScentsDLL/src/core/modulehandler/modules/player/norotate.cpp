#include "norotate.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"

NoRotateModule::NoRotateModule()
    : ModuleBase("NoRotate", "Block server head rotation", Category::Player)
{
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void NoRotateModule::OnEnable() { Logger::Log("NoRotate enabled"); }
void NoRotateModule::OnDisable() { Logger::Log("NoRotate disabled"); }

void NoRotateModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

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

    jfieldID yawID = env->GetFieldID(entityClass, "N", "F");
    if (!yawID)
    {
        env->ExceptionClear();
        yawID = env->GetFieldID(entityClass, "rotationYaw", "F");
    }
    env->ExceptionClear();

    jfieldID pitchID = env->GetFieldID(entityClass, "O", "F");
    if (!pitchID)
    {
        env->ExceptionClear();
        pitchID = env->GetFieldID(entityClass, "rotationPitch", "F");
    }
    env->ExceptionClear();

    jfieldID prevYawID = env->GetFieldID(entityClass, "P", "F");
    if (!prevYawID)
    {
        env->ExceptionClear();
        prevYawID = env->GetFieldID(entityClass, "prevRotationYaw", "F");
    }
    env->ExceptionClear();

    jfieldID prevPitchID = env->GetFieldID(entityClass, "Q", "F");
    if (!prevPitchID)
    {
        env->ExceptionClear();
        prevPitchID = env->GetFieldID(entityClass, "prevRotationPitch", "F");
    }
    env->ExceptionClear();

    jfieldID lastYawID = env->GetFieldID(entityClass, "R", "F");
    if (!lastYawID)
    {
        env->ExceptionClear();
        lastYawID = env->GetFieldID(entityClass, "lastReportedYaw", "F");
    }
    env->ExceptionClear();

    jfieldID lastPitchID = env->GetFieldID(entityClass, "S", "F");
    if (!lastPitchID)
    {
        env->ExceptionClear();
        lastPitchID = env->GetFieldID(entityClass, "lastReportedPitch", "F");
    }
    env->ExceptionClear();

    jfieldID serverYawID = env->GetFieldID(entityClass, "T", "F");
    if (!serverYawID)
    {
        env->ExceptionClear();
        serverYawID = env->GetFieldID(entityClass, "serverRotationYaw", "F");
    }
    env->ExceptionClear();

    jfieldID serverPitchID = env->GetFieldID(entityClass, "U", "F");
    if (!serverPitchID)
    {
        env->ExceptionClear();
        serverPitchID = env->GetFieldID(entityClass, "serverRotationPitch", "F");
    }
    env->ExceptionClear();

    float playerYaw = yawID ? env->GetFloatField(player, yawID) : 0.0f;
    float playerPitch = pitchID ? env->GetFloatField(player, pitchID) : 0.0f;

    if (prevYawID)
        env->SetFloatField(player, prevYawID, playerYaw);
    if (prevPitchID)
        env->SetFloatField(player, prevPitchID, playerPitch);
    if (lastYawID)
        env->SetFloatField(player, lastYawID, playerYaw);
    if (lastPitchID)
        env->SetFloatField(player, lastPitchID, playerPitch);
    if (serverYawID)
        env->SetFloatField(player, serverYawID, playerYaw);
    if (serverPitchID)
        env->SetFloatField(player, serverPitchID, playerPitch);

    jfieldID onGroundID = env->GetFieldID(entityClass, "I", "Z");
    if (!onGroundID)
    {
        env->ExceptionClear();
        onGroundID = env->GetFieldID(entityClass, "onGround", "Z");
    }
    env->ExceptionClear();

    if (onGroundID)
    {
        jboolean og = env->GetBooleanField(player, onGroundID);

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
                jclass c03Class = Core::GetInstance().GetJava()->FindClass("hg", "net/minecraft/network/play/client/C03PacketPlayer");
                if (!c03Class)
                {
                    env->ExceptionClear();
                    c03Class = env->FindClass("net/minecraft/network/play/client/C03PacketPlayer");
                }
                if (c03Class)
                {
                    jmethodID c03Ctor = env->GetMethodID(c03Class, "<init>", "(Z)V");
                    if (!c03Ctor)
                    {
                        env->ExceptionClear();
                        c03Ctor = env->GetMethodID(c03Class, "<init>", "(FZF)V");
                    }
                    if (!c03Ctor)
                    {
                        env->ExceptionClear();
                        c03Ctor = env->GetMethodID(c03Class, "<init>", "()V");
                    }
                    env->ExceptionClear();

                    jobject packet = nullptr;
                    if (c03Ctor)
                    {
                        jclass nhClass = env->GetObjectClass(nh);
                        jmethodID sendMethod = env->GetMethodID(nhClass, "a", "(Lhb;)V");
                        if (!sendMethod)
                        {
                            env->ExceptionClear();
                            sendMethod = env->GetMethodID(nhClass, "addToSendQueue", "(Lnet/minecraft/network/Packet;)V");
                        }
                        env->ExceptionClear();

                        packet = env->NewObject(c03Class, c03Ctor);
                        if (packet)
                        {
                            jfieldID yawF = env->GetFieldID(c03Class, "yaw", "F");
                            jfieldID pitchF = env->GetFieldID(c03Class, "pitch", "F");
                            jfieldID ogF = env->GetFieldID(c03Class, "onGround", "Z");
                            if (!yawF)
                            {
                                env->ExceptionClear();
                                yawF = env->GetFieldID(c03Class, "b", "F");
                            }
                            if (!pitchF)
                            {
                                env->ExceptionClear();
                                pitchF = env->GetFieldID(c03Class, "c", "F");
                            }
                            if (!ogF)
                            {
                                env->ExceptionClear();
                                ogF = env->GetFieldID(c03Class, "d", "Z");
                            }
                            env->ExceptionClear();

                            if (yawF) env->SetFloatField(packet, yawF, playerYaw);
                            if (pitchF) env->SetFloatField(packet, pitchF, playerPitch);
                            if (ogF) env->SetBooleanField(packet, ogF, og);

                            if (sendMethod)
                                env->CallVoidMethod(nh, sendMethod, packet);
                        }
                        env->DeleteLocalRef(nhClass);
                    }
                    if (packet) env->DeleteLocalRef(packet);
                    env->DeleteLocalRef(c03Class);
                }
                env->DeleteLocalRef(nh);
            }
        }
    }

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

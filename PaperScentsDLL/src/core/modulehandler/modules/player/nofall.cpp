#include "nofall.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"

NoFallModule::NoFallModule()
    : ModuleBase("NoFall", "Negate fall damage", Category::Player)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Packet", "Ground", "Void"});
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void NoFallModule::OnEnable() { Logger::Log("NoFall enabled"); m_HasLastSafe = false; }
void NoFallModule::OnDisable() { Logger::Log("NoFall disabled"); }

void NoFallModule::OnUpdate()
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

    bool bridged = BridgeHelper::Initialize(env);
    jobject player = bridged ? BridgeHelper::GetPlayer(env) : nullptr;
    jobject mc = nullptr;
    jclass mcClass = nullptr;

    if (!player)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java || !java->IsValid()) return;
        mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
        if (!mcClass) return;
        jmethodID getMc = env->GetStaticMethodID(mcClass, "A", "()Lave;");
        if (!getMc) { env->DeleteLocalRef(mcClass); return; }
        mc = env->CallStaticObjectMethod(mcClass, getMc);
        if (!mc) { env->DeleteLocalRef(mcClass); return; }
        for (auto f : {"s", "t", "h", "thePlayer"})
        {
            jfieldID pf = env->GetFieldID(mcClass, f, "Lbew;");
            if (!pf) pf = env->GetFieldID(mcClass, f, "Lnet/minecraft/client/entity/EntityPlayerSP;");
            if (pf) { player = env->GetObjectField(mc, pf); if (player) break; }
        }
        if (!player) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }
    }
    else
    {
        mc = BridgeHelper::GetMinecraftInstance(env);
    }

    // Read fallDistance, onGround, posX/Y/Z via notch fields
    jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
    if (!entityClass) { env->ExceptionClear(); env->DeleteLocalRef(player); if (mc) env->DeleteLocalRef(mc); if (mcClass) env->DeleteLocalRef(mcClass); return; }

    jfieldID fallDistID = env->GetFieldID(entityClass, "fallDistance", "F");
    if (!fallDistID) { env->ExceptionClear(); fallDistID = env->GetFieldID(env->FindClass("pk"), "k", "F"); }
    if (!fallDistID) { env->ExceptionClear(); env->DeleteLocalRef(entityClass); env->DeleteLocalRef(player); if (mc) env->DeleteLocalRef(mc); if (mcClass) env->DeleteLocalRef(mcClass); return; }
    env->ExceptionClear();

    jfieldID onGroundID = env->GetFieldID(entityClass, "onGround", "Z");
    if (!onGroundID) { env->ExceptionClear(); onGroundID = env->GetFieldID(entityClass, "I", "Z"); }
    env->ExceptionClear();

    jfieldID posXID = env->GetFieldID(entityClass, "posX", "D");
    if (!posXID) { env->ExceptionClear(); posXID = env->GetFieldID(entityClass, "r", "D"); }
    env->ExceptionClear();

    jfieldID posYID = env->GetFieldID(entityClass, "posY", "D");
    if (!posYID) { env->ExceptionClear(); posYID = env->GetFieldID(entityClass, "s", "D"); }
    env->ExceptionClear();

    jfieldID posZID = env->GetFieldID(entityClass, "posZ", "D");
    if (!posZID) { env->ExceptionClear(); posZID = env->GetFieldID(entityClass, "t", "D"); }
    env->ExceptionClear();

    if (!fallDistID) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(player); if (mc) env->DeleteLocalRef(mc); if (mcClass) env->DeleteLocalRef(mcClass); return; }

    float fallDist = env->GetFloatField(player, fallDistID);
    bool onGround = onGroundID ? env->GetBooleanField(player, onGroundID) : false;
    double px = posXID ? env->GetDoubleField(player, posXID) : 0;
    double py = posYID ? env->GetDoubleField(player, posYID) : 0;
    double pz = posZID ? env->GetDoubleField(player, posZID) : 0;

    if (onGround)
    {
        m_LastSafeX = px; m_LastSafeY = py; m_LastSafeZ = pz; m_HasLastSafe = true;
    }

    if (fallDist > 3.0f)
    {
        int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();
        switch (mode)
        {
        case 0:
        case 2:
        {
            double tx = (mode == 2 && m_HasLastSafe) ? m_LastSafeX : px;
            double ty = (mode == 2 && m_HasLastSafe) ? m_LastSafeY : py;
            double tz = (mode == 2 && m_HasLastSafe) ? m_LastSafeZ : pz;

            // Find netHandler and send C03
            jfieldID nhField = nullptr;
            if (mcClass)
            {
                nhField = env->GetFieldID(mcClass, "q", "Leh;");
                if (!nhField) { env->ExceptionClear(); nhField = env->GetFieldID(mcClass, "v", "Leh;"); }
                if (!nhField) { env->ExceptionClear(); nhField = env->GetFieldID(mcClass, "getNetHandler", "Leh;"); }
            }
            else if (bridged && BridgeHelper::MinecraftBridge)
            {
                nhField = env->GetFieldID(BridgeHelper::MinecraftBridge, "bridge$getNetHandler", "()Lcom/moonsworth/lunar/bridge/client/network/NetHandlerPlayClientBridge;");
            }
            env->ExceptionClear();

            if (nhField && mc)
            {
                jobject nh = env->GetObjectField(mc, nhField);
                if (nh)
                {
                    jclass c03Class = env->FindClass("net/minecraft/network/play/client/C03PacketPlayer");
                    if (!c03Class) { env->ExceptionClear(); }
                    if (c03Class)
                    {
                        jmethodID c03Ctor = env->GetMethodID(c03Class, "<init>", "(DDDZ)V");
                        if (!c03Ctor) { env->ExceptionClear(); c03Ctor = env->GetMethodID(c03Class, "<init>", "()V"); }
                        env->ExceptionClear();
                        if (c03Ctor)
                        {
                            jobject packet = env->NewObject(c03Class, c03Ctor, tx, ty, tz, JNI_TRUE);
                            if (packet)
                            {
                                jclass nhClass = env->GetObjectClass(nh);
                                jmethodID send = env->GetMethodID(nhClass, "a", "(Lhb;)V");
                                if (!send) { env->ExceptionClear(); send = env->GetMethodID(nhClass, "addToSendQueue", "(Lnet/minecraft/network/Packet;)V"); }
                                env->ExceptionClear();
                                if (send) env->CallVoidMethod(nh, send, packet);
                                env->DeleteLocalRef(nhClass);
                                env->DeleteLocalRef(packet);
                            }
                        }
                        env->DeleteLocalRef(c03Class);
                    }
                    env->DeleteLocalRef(nh);
                }
            }
            break;
        }
        case 1:
            if (onGroundID) env->SetBooleanField(player, onGroundID, JNI_TRUE);
            break;
        }
    }

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    if (mc) env->DeleteLocalRef(mc);
    if (mcClass) env->DeleteLocalRef(mcClass);
}

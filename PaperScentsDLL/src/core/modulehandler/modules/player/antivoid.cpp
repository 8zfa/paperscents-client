#include "antivoid.h"
#include "../../../core.h"
#include "../../../java/java.h"
#include "../../../sdk/bridgeHelper.h"
#include "../../../utilities/logger.h"

AntiVoidModule::AntiVoidModule()
    : ModuleBase("AntiVoid", "Prevent falling into the void", Category::Player)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Packet", "Motion"});
    AddSetting<NumberSetting>("FallDist", 5.0f, 3.0f, 20.0f, 1.0f, "Fall distance threshold");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void AntiVoidModule::OnEnable() { Logger::Log("AntiVoid enabled"); }
void AntiVoidModule::OnDisable() { Logger::Log("AntiVoid disabled"); }

void AntiVoidModule::OnUpdate()
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

    if (!player)
    {
        Java* java = Core::GetInstance().GetJava();
        if (!java || !java->IsValid()) return;

        jclass mcClass = java->FindClass("ave", "net/minecraft/client/Minecraft");
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

    // Read entity fields via notch
    jclass entityClass = env->FindClass("net/minecraft/entity/Entity");
    if (!entityClass) { env->ExceptionClear(); entityClass = env->FindClass("pk"); }
    if (!entityClass) { env->ExceptionClear(); env->DeleteLocalRef(player); if (!bridged && mc) env->DeleteLocalRef(mc); return; }

    jfieldID posYID = env->GetFieldID(entityClass, "posY", "D");
    if (!posYID) { env->ExceptionClear(); posYID = env->GetFieldID(entityClass, "s", "D"); }
    env->ExceptionClear();

    jfieldID motionYID = env->GetFieldID(entityClass, "motionY", "D");
    if (!motionYID) { env->ExceptionClear(); motionYID = env->GetFieldID(entityClass, "w", "D"); }
    env->ExceptionClear();

    jfieldID onGroundID = env->GetFieldID(entityClass, "onGround", "Z");
    if (!onGroundID) { env->ExceptionClear(); onGroundID = env->GetFieldID(entityClass, "I", "Z"); }
    env->ExceptionClear();

    jfieldID posXID = env->GetFieldID(entityClass, "posX", "D");
    if (!posXID) { env->ExceptionClear(); posXID = env->GetFieldID(entityClass, "r", "D"); }
    env->ExceptionClear();

    jfieldID posZID = env->GetFieldID(entityClass, "posZ", "D");
    if (!posZID) { env->ExceptionClear(); posZID = env->GetFieldID(entityClass, "t", "D"); }
    env->ExceptionClear();

    if (!posYID || !motionYID) { env->DeleteLocalRef(entityClass); env->DeleteLocalRef(player); if (!bridged && mc) env->DeleteLocalRef(mc); return; }

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
            // Send C03 packet via notch
            jclass mcCls = bridged ? nullptr : env->GetObjectClass(mc);
            jfieldID nhField = nullptr;
            if (mcCls)
            {
                nhField = env->GetFieldID(mcCls, "q", "Leh;");
                if (!nhField) { env->ExceptionClear(); nhField = env->GetFieldID(mcCls, "v", "Leh;"); }
                env->ExceptionClear();
            }
            if (!nhField)
            {
                jclass mcNotch = env->FindClass("net/minecraft/client/Minecraft");
                if (mcNotch)
                {
                    nhField = env->GetFieldID(mcNotch, "v", "Leh;");
                    if (!nhField) { env->ExceptionClear(); nhField = env->GetFieldID(mcNotch, "getNetHandler", "Leh;"); }
                    env->ExceptionClear();
                    env->DeleteLocalRef(mcNotch);
                }
            }

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
                            jobject packet = env->NewObject(c03Class, c03Ctor, lastGroundX, lastGroundY + 0.1, lastGroundZ, JNI_TRUE);
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
            if (mcCls && !bridged) env->DeleteLocalRef(mcCls);
        }
        else
        {
            env->SetDoubleField(player, motionYID, 0.0);
        }
    }

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    if (!bridged && mc) env->DeleteLocalRef(mc);
}

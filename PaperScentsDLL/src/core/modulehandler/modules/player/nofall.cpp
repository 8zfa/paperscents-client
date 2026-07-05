#include "nofall.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"

static jclass g_EntityClass = nullptr;
static jclass g_MinecraftClass = nullptr;
static jclass g_NetHandlerClass = nullptr;
static jclass g_PacketClass = nullptr;
static jfieldID g_fallDistID = nullptr;
static jfieldID g_onGroundID = nullptr;
static jfieldID g_posXID = nullptr;
static jfieldID g_posYID = nullptr;
static jfieldID g_posZID = nullptr;
static jfieldID g_sendQueueID = nullptr;
static jmethodID g_getMinecraft = nullptr;
static jfieldID g_getPlayer = nullptr;
static jfieldID g_getNetHandler = nullptr;
static jmethodID g_packetCtor = nullptr;
static bool g_Cached = false;

static void CacheFields(JNIEnv* env)
{
    if (g_Cached) return;

    g_MinecraftClass = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
    if (g_MinecraftClass)
    {
        g_MinecraftClass = (jclass)env->NewGlobalRef(g_MinecraftClass);
        g_getMinecraft = env->GetStaticMethodID(g_MinecraftClass, "A", "()Lave;");
        if (!g_getMinecraft)
        {
            env->ExceptionClear();
            g_getMinecraft = env->GetStaticMethodID(g_MinecraftClass, "a", "()Lave;");
        }
        if (!g_getMinecraft)
        {
            env->ExceptionClear();
            g_getMinecraft = env->GetStaticMethodID(g_MinecraftClass, "getMinecraft", "()Lave;");
        }
        env->ExceptionClear();

        g_getPlayer = env->GetFieldID(g_MinecraftClass, "h", "Lbew;");
        if (!g_getPlayer)
        {
            env->ExceptionClear();
            g_getPlayer = env->GetFieldID(g_MinecraftClass, "thePlayer", "Lbew;");
        }
        env->ExceptionClear();

        g_getNetHandler = env->GetFieldID(g_MinecraftClass, "q", "Leh;");
        if (!g_getNetHandler)
        {
            env->ExceptionClear();
            g_getNetHandler = env->GetFieldID(g_MinecraftClass, "getNetHandler", "Leh;");
        }
        if (!g_getNetHandler)
        {
            env->ExceptionClear();
            g_getNetHandler = env->GetFieldID(g_MinecraftClass, "v", "Leh;");
        }
        env->ExceptionClear();
    }

    g_EntityClass = Core::GetInstance().GetJava()->FindClass("pk", "net/minecraft/entity/Entity");
    if (g_EntityClass)
    {
        g_EntityClass = (jclass)env->NewGlobalRef(g_EntityClass);
        g_fallDistID = env->GetFieldID(g_EntityClass, "k", "F");
        if (!g_fallDistID)
        {
            env->ExceptionClear();
            g_fallDistID = env->GetFieldID(g_EntityClass, "fallDistance", "F");
        }
        env->ExceptionClear();

        g_onGroundID = env->GetFieldID(g_EntityClass, "I", "Z");
        if (!g_onGroundID)
        {
            env->ExceptionClear();
            g_onGroundID = env->GetFieldID(g_EntityClass, "onGround", "Z");
        }
        env->ExceptionClear();

        g_posXID = env->GetFieldID(g_EntityClass, "r", "D");
        if (!g_posXID)
        {
            env->ExceptionClear();
            g_posXID = env->GetFieldID(g_EntityClass, "posX", "D");
        }
        env->ExceptionClear();

        g_posYID = env->GetFieldID(g_EntityClass, "s", "D");
        if (!g_posYID)
        {
            env->ExceptionClear();
            g_posYID = env->GetFieldID(g_EntityClass, "posY", "D");
        }
        env->ExceptionClear();

        g_posZID = env->GetFieldID(g_EntityClass, "t", "D");
        if (!g_posZID)
        {
            env->ExceptionClear();
            g_posZID = env->GetFieldID(g_EntityClass, "posZ", "D");
        }
        env->ExceptionClear();
    }

    g_NetHandlerClass = Core::GetInstance().GetJava()->FindClass("eh", "net/minecraft/network/NetHandlerPlayServer");
    if (g_NetHandlerClass)
    {
        g_NetHandlerClass = (jclass)env->NewGlobalRef(g_NetHandlerClass);
    }

    g_PacketClass = Core::GetInstance().GetJava()->FindClass("hb", "net/minecraft/util/MovementInput");
    if (g_PacketClass)
    {
        g_PacketClass = (jclass)env->NewGlobalRef(g_PacketClass);
    }

    g_Cached = true;
}

static void SendPositionPacket(JNIEnv* env, jobject mc, double x, double y, double z, bool onGround)
{
    jclass c03Class = Core::GetInstance().GetJava()->FindClass("hg", "net/minecraft/network/play/client/C03PacketPlayer");
    if (!c03Class)
    {
        env->ExceptionClear();
        c03Class = env->FindClass("net/minecraft/network/play/client/C03PacketPlayer");
    }
    if (!c03Class) return;

    jmethodID ctor = env->GetMethodID(c03Class, "<init>", "(DDDZ)V");
    if (!ctor)
    {
        env->ExceptionClear();
        ctor = env->GetMethodID(c03Class, "<init>", "(DDDZF)V");
    }
    if (!ctor)
    {
        env->ExceptionClear();
        ctor = env->GetMethodID(c03Class, "<init>", "(DDDZZ)V");
    }
    if (!ctor)
    {
        env->ExceptionClear();
        ctor = env->GetMethodID(c03Class, "<init>", "()V");
    }
    env->ExceptionClear();

    jobject packet = nullptr;
    if (ctor)
    {
        jstring sig = env->NewStringUTF("(DDDZ)V");
        ctor = env->GetMethodID(c03Class, "<init>", "(DDDZ)V");
        env->DeleteLocalRef(sig);
        env->ExceptionClear();
        packet = env->NewObject(c03Class, ctor, x, y, z, onGround);
    }

    if (!packet)
    {
        ctor = env->GetMethodID(c03Class, "<init>", "()V");
        env->ExceptionClear();
        if (ctor)
        {
            packet = env->NewObject(c03Class, ctor);
            if (packet)
            {
                jfieldID xID = env->GetFieldID(c03Class, "x", "D");
                jfieldID yID = env->GetFieldID(c03Class, "y", "D");
                jfieldID zID = env->GetFieldID(c03Class, "z", "D");
                jfieldID ogID = env->GetFieldID(c03Class, "onGround", "Z");
                if (!xID)
                {
                    env->ExceptionClear();
                    xID = env->GetFieldID(c03Class, "a", "D");
                }
                if (!yID)
                {
                    env->ExceptionClear();
                    yID = env->GetFieldID(c03Class, "b", "D");
                }
                if (!zID)
                {
                    env->ExceptionClear();
                    zID = env->GetFieldID(c03Class, "c", "D");
                }
                if (!ogID)
                {
                    env->ExceptionClear();
                    ogID = env->GetFieldID(c03Class, "d", "Z");
                }
                env->ExceptionClear();
                if (xID) env->SetDoubleField(packet, xID, x);
                if (yID) env->SetDoubleField(packet, yID, y);
                if (zID) env->SetDoubleField(packet, zID, z);
                if (ogID) env->SetBooleanField(packet, ogID, onGround);
            }
        }
    }

    if (packet && g_getNetHandler)
    {
        jobject nethandler = env->GetObjectField(mc, g_getNetHandler);
        if (nethandler)
        {
            jclass nhClass = env->GetObjectClass(nethandler);
            jmethodID addMethod = env->GetMethodID(nhClass, "a", "(Lhb;)V");
            if (!addMethod)
            {
                env->ExceptionClear();
                addMethod = env->GetMethodID(nhClass, "addToSendQueue", "(Lnet/minecraft/network/Packet;)V");
            }
            env->ExceptionClear();
            if (addMethod)
                env->CallVoidMethod(nethandler, addMethod, packet);
            env->DeleteLocalRef(nethandler);
        }
        env->DeleteLocalRef(packet);
    }
    env->DeleteLocalRef(c03Class);
}

NoFallModule::NoFallModule()
    : ModuleBase("NoFall", "Negate fall damage", Category::Player)
{
    AddSetting<EnumSetting>("Mode", 0, std::vector<std::string>{"Packet", "Ground", "Void"});
}

void NoFallModule::OnEnable()
{
    Logger::Log("NoFall enabled");
    g_Cached = false;
    m_HasLastSafe = false;
}

void NoFallModule::OnDisable()
{
    Logger::Log("NoFall disabled");
}

void NoFallModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    CacheFields(env);

    if (!g_MinecraftClass || !g_getMinecraft) return;
    jobject mc = env->CallStaticObjectMethod(g_MinecraftClass, g_getMinecraft);
    if (!mc) return;

    if (!g_getPlayer) { env->DeleteLocalRef(mc); return; }
    jobject player = env->GetObjectField(mc, g_getPlayer);
    if (!player)
    {
        env->DeleteLocalRef(mc);
        return;
    }

    if (!g_fallDistID || !g_onGroundID || !g_posXID || !g_posYID || !g_posZID)
    {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(mc);
        return;
    }
    float fallDist = env->GetFloatField(player, g_fallDistID);
    bool onGround = env->GetBooleanField(player, g_onGroundID);
    double px = env->GetDoubleField(player, g_posXID);
    double py = env->GetDoubleField(player, g_posYID);
    double pz = env->GetDoubleField(player, g_posZID);

    if (onGround)
    {
        m_LastSafeX = px;
        m_LastSafeY = py;
        m_LastSafeZ = pz;
        m_HasLastSafe = true;
    }

    if (fallDist > 3.0f)
    {
        int mode = ((EnumSetting*)FindSetting("Mode"))->GetValue();

        switch (mode)
        {
        case 0:
            SendPositionPacket(env, mc, px, py, pz, true);
            break;
        case 1:
            env->SetBooleanField(player, g_onGroundID, JNI_TRUE);
            break;
        case 2:
            if (m_HasLastSafe)
                SendPositionPacket(env, mc, m_LastSafeX, m_LastSafeY, m_LastSafeZ, true);
            break;
        }
    }

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

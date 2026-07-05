#include "freecam.h"
#include "../../../java/java.h"
#include "../../../utilities/logger.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include <cmath>

FreeCamModule::FreeCamModule()
    : ModuleBase("FreeCam", "Detach camera from body", Category::Render)
{
    AddSetting<NumberSetting>("Speed", 1.0f, 0.1f, 5.0f, 0.1f);
}

void FreeCamModule::OnEnable()
{
    Logger::Log("FreeCam enabled");

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jclass eCls = StrayCache::Entity;
    if (!eCls) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jfieldID posXID = env->GetFieldID(eCls, "r", "D");
    if (!posXID) { env->ExceptionClear(); posXID = env->GetFieldID(eCls, "posX", "D"); }
    env->ExceptionClear();
    jfieldID posYID = env->GetFieldID(eCls, "s", "D");
    if (!posYID) { env->ExceptionClear(); posYID = env->GetFieldID(eCls, "posY", "D"); }
    env->ExceptionClear();
    jfieldID posZID = env->GetFieldID(eCls, "t", "D");
    if (!posZID) { env->ExceptionClear(); posZID = env->GetFieldID(eCls, "posZ", "D"); }
    env->ExceptionClear();
    jfieldID yawID = env->GetFieldID(eCls, "v", "F");
    if (!yawID) { env->ExceptionClear(); yawID = env->GetFieldID(eCls, "rotationYaw", "F"); }
    env->ExceptionClear();
    jfieldID pitchID = env->GetFieldID(eCls, "w", "F");
    if (!pitchID) { env->ExceptionClear(); pitchID = env->GetFieldID(eCls, "rotationPitch", "F"); }
    env->ExceptionClear();

    m_OriginalX = env->GetDoubleField(player, posXID);
    m_OriginalY = env->GetDoubleField(player, posYID);
    m_OriginalZ = env->GetDoubleField(player, posZID);
    m_OriginalYaw = env->GetFloatField(player, yawID);
    m_OriginalPitch = env->GetFloatField(player, pitchID);

    jfieldID noClipID = env->GetFieldID(eCls, "z", "Z");
    if (!noClipID) { env->ExceptionClear(); noClipID = env->GetFieldID(eCls, "noClip", "Z"); }
    env->ExceptionClear();
    if (noClipID) env->SetBooleanField(player, noClipID, JNI_TRUE);

    GetCursorPos(&m_LastCursor);
    m_HasStoredPosition = true;

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

void FreeCamModule::OnDisable()
{
    Logger::Log("FreeCam disabled");

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jclass eCls = StrayCache::Entity;
    if (!eCls) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jfieldID posXID = env->GetFieldID(eCls, "r", "D");
    if (!posXID) { env->ExceptionClear(); posXID = env->GetFieldID(eCls, "posX", "D"); }
    env->ExceptionClear();
    jfieldID posYID = env->GetFieldID(eCls, "s", "D");
    if (!posYID) { env->ExceptionClear(); posYID = env->GetFieldID(eCls, "posY", "D"); }
    env->ExceptionClear();
    jfieldID posZID = env->GetFieldID(eCls, "t", "D");
    if (!posZID) { env->ExceptionClear(); posZID = env->GetFieldID(eCls, "posZ", "D"); }
    env->ExceptionClear();
    jfieldID yawID = env->GetFieldID(eCls, "v", "F");
    if (!yawID) { env->ExceptionClear(); yawID = env->GetFieldID(eCls, "rotationYaw", "F"); }
    env->ExceptionClear();
    jfieldID pitchID = env->GetFieldID(eCls, "w", "F");
    if (!pitchID) { env->ExceptionClear(); pitchID = env->GetFieldID(eCls, "rotationPitch", "F"); }
    env->ExceptionClear();
    jfieldID onGroundID = env->GetFieldID(eCls, "C", "Z");
    if (!onGroundID) { env->ExceptionClear(); onGroundID = env->GetFieldID(eCls, "onGround", "Z"); }
    env->ExceptionClear();
    jfieldID noClipID = env->GetFieldID(eCls, "z", "Z");
    if (!noClipID) { env->ExceptionClear(); noClipID = env->GetFieldID(eCls, "noClip", "Z"); }
    env->ExceptionClear();

    env->SetDoubleField(player, posXID, m_OriginalX);
    env->SetDoubleField(player, posYID, m_OriginalY);
    env->SetDoubleField(player, posZID, m_OriginalZ);
    env->SetFloatField(player, yawID, m_OriginalYaw);
    env->SetFloatField(player, pitchID, m_OriginalPitch);
    if (onGroundID) env->SetBooleanField(player, onGroundID, JNI_TRUE);
    if (noClipID) env->SetBooleanField(player, noClipID, JNI_FALSE);

    m_HasStoredPosition = false;

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

void FreeCamModule::OnUpdate()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jclass eCls = StrayCache::Entity;
    if (!eCls) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jfieldID posXID = env->GetFieldID(eCls, "r", "D");
    if (!posXID) { env->ExceptionClear(); posXID = env->GetFieldID(eCls, "posX", "D"); }
    env->ExceptionClear();
    jfieldID posYID = env->GetFieldID(eCls, "s", "D");
    if (!posYID) { env->ExceptionClear(); posYID = env->GetFieldID(eCls, "posY", "D"); }
    env->ExceptionClear();
    jfieldID posZID = env->GetFieldID(eCls, "t", "D");
    if (!posZID) { env->ExceptionClear(); posZID = env->GetFieldID(eCls, "posZ", "D"); }
    env->ExceptionClear();
    jfieldID yawID = env->GetFieldID(eCls, "v", "F");
    if (!yawID) { env->ExceptionClear(); yawID = env->GetFieldID(eCls, "rotationYaw", "F"); }
    env->ExceptionClear();
    jfieldID pitchID = env->GetFieldID(eCls, "w", "F");
    if (!pitchID) { env->ExceptionClear(); pitchID = env->GetFieldID(eCls, "rotationPitch", "F"); }
    env->ExceptionClear();
    jfieldID noClipID = env->GetFieldID(eCls, "z", "Z");
    if (!noClipID) { env->ExceptionClear(); noClipID = env->GetFieldID(eCls, "noClip", "Z"); }
    env->ExceptionClear();
    jfieldID onGroundID = env->GetFieldID(eCls, "C", "Z");
    if (!onGroundID) { env->ExceptionClear(); onGroundID = env->GetFieldID(eCls, "onGround", "Z"); }
    env->ExceptionClear();

    float speed = ((NumberSetting*)FindSetting("Speed"))->GetValue();
    float yaw = env->GetFloatField(player, yawID);
    float pitch = env->GetFloatField(player, pitchID);

    POINT curCursor;
    GetCursorPos(&curCursor);
    if (m_HasStoredPosition)
    {
        float deltaX = (float)(curCursor.x - m_LastCursor.x);
        float deltaY = (float)(curCursor.y - m_LastCursor.y);

        float newYaw = yaw + deltaX * 0.3f;
        float newPitch = pitch - deltaY * 0.3f;
        if (newPitch > 90.0f) newPitch = 90.0f;
        if (newPitch < -90.0f) newPitch = -90.0f;

        env->SetFloatField(player, yawID, newYaw);
        env->SetFloatField(player, pitchID, newPitch);
        yaw = newYaw;
    }
    m_LastCursor = curCursor;
    m_HasStoredPosition = true;

    double dx = 0.0, dy = 0.0, dz = 0.0;
    if (GetAsyncKeyState('W') & 0x8000)
    {
        dx -= sin(yaw * 3.141592653589793f / 180.0f) * speed * 0.5;
        dz += cos(yaw * 3.141592653589793f / 180.0f) * speed * 0.5;
    }
    if (GetAsyncKeyState('S') & 0x8000)
    {
        dx += sin(yaw * 3.141592653589793f / 180.0f) * speed * 0.5;
        dz -= cos(yaw * 3.141592653589793f / 180.0f) * speed * 0.5;
    }
    if (GetAsyncKeyState('A') & 0x8000)
    {
        dx -= sin((yaw - 90.0f) * 3.141592653589793f / 180.0f) * speed * 0.5;
        dz += cos((yaw - 90.0f) * 3.141592653589793f / 180.0f) * speed * 0.5;
    }
    if (GetAsyncKeyState('D') & 0x8000)
    {
        dx -= sin((yaw + 90.0f) * 3.141592653589793f / 180.0f) * speed * 0.5;
        dz += cos((yaw + 90.0f) * 3.141592653589793f / 180.0f) * speed * 0.5;
    }
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) dy += speed * 0.5;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) dy -= speed * 0.5;

    double px = env->GetDoubleField(player, posXID) + dx;
    double py = env->GetDoubleField(player, posYID) + dy;
    double pz = env->GetDoubleField(player, posZID) + dz;

    env->SetDoubleField(player, posXID, px);
    env->SetDoubleField(player, posYID, py);
    env->SetDoubleField(player, posZID, pz);
    if (noClipID) env->SetBooleanField(player, noClipID, JNI_TRUE);
    if (onGroundID) env->SetBooleanField(player, onGroundID, JNI_TRUE);

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

#include "backtrack.h"
#include "../../../utilities/logger.h"
#include "../../../java/java.h"
#include "../../../utilities/jni_helpers.h"
#include <algorithm>

BackTrackModule::BackTrackModule()
    : ModuleBase("BackTrack", "Stores past player positions for backtracking", Category::Combat), m_TickCounter(0)
{
    AddSetting<NumberSetting>("Ticks", 5.0f, 1.0f, 20.0f, 1.0f, "Number of ticks to track back");
    AddSetting<BooleanSetting>("Players", true, "Track players");
    AddSetting<BooleanSetting>("Visualize", true, "Visualize backtrack positions");
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
}

void BackTrackModule::OnEnable() { Logger::Log("BackTrack enabled"); m_Positions.clear(); m_TickCounter = 0; }
void BackTrackModule::OnDisable() { Logger::Log("BackTrack disabled"); m_Positions.clear(); }

void BackTrackModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }

    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;

    jobject mc = GetMinecraftObject(env);
    if (!mc) return;

    jobject player = GetPlayerObject(env, mc);
    if (!player) { env->DeleteLocalRef(mc); return; }

    jclass entityClass = env->GetObjectClass(player);
    if (!entityClass) { env->DeleteLocalRef(player); env->DeleteLocalRef(mc); return; }

    jfieldID posXField = env->GetFieldID(entityClass, "r", "D");
    if (!posXField) { env->ExceptionClear(); posXField = env->GetFieldID(entityClass, "posX", "D"); env->ExceptionClear(); }
    jfieldID posYField = env->GetFieldID(entityClass, "s", "D");
    if (!posYField) { env->ExceptionClear(); posYField = env->GetFieldID(entityClass, "posY", "D"); env->ExceptionClear(); }
    jfieldID posZField = env->GetFieldID(entityClass, "t", "D");
    if (!posZField) { env->ExceptionClear(); posZField = env->GetFieldID(entityClass, "posZ", "D"); env->ExceptionClear(); }

    if (posXField && posYField && posZField)
    {
        double x = env->GetDoubleField(player, posXField);
        double y = env->GetDoubleField(player, posYField);
        double z = env->GetDoubleField(player, posZField);

        m_Positions.push_back({ x, y, z, m_TickCounter });

        int maxTicks = (int)((NumberSetting*)FindSetting("Ticks"))->GetValue();
        while ((int)m_Positions.size() > maxTicks)
            m_Positions.erase(m_Positions.begin());
    }

    m_TickCounter++;
    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(mc);
}

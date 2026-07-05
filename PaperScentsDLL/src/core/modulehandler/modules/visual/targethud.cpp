#include "targethud.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"
#include <imgui.h>
#include "../../modulehandler.h"

TargetHUDModule::TargetHUDModule()
    : ModuleBase("TargetHUD", "Display target info on screen", Category::Render)
{
    AddSetting<NumberSetting>("X", 4.0f, 0.0f, 2000.0f, 1.0f);
    AddSetting<NumberSetting>("Y", 200.0f, 0.0f, 2000.0f, 1.0f);
    AddSetting<BooleanSetting>("ShowHealth", true);
    AddSetting<BooleanSetting>("ShowDistance", true);
    AddSetting<ColorSetting>("Color", ImColor(0.42f, 0.39f, 1.0f, 1.0f));
}

void TargetHUDModule::OnEnable() { Logger::Log("TargetHUD enabled"); }
void TargetHUDModule::OnDisable() { Logger::Log("TargetHUD disabled"); }

void TargetHUDModule::OnRender()
{
    if (!IsEnabled()) return;

    JNIEnv* env = Core::GetInstance().GetJava()->GetEnv();
    if (!env) return;

    ModuleBase* kaMod = ModuleHandler::GetInstance().GetModule("KillAura");
    if (!kaMod || !kaMod->IsEnabled()) return;

    double targetX = 0.0, targetY = 0.0, targetZ = 0.0;
    std::string targetName;
    float targetHealth = 0.0f;
    float targetMaxHealth = 20.0f;
    bool hasTarget = false;

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
                jclass playerClass = Core::GetInstance().GetJava()->FindClass("wn", "net/minecraft/entity/player/EntityPlayer");
                if (!playerClass) { env->ExceptionClear(); playerClass = env->FindClass("net/minecraft/entity/player/EntityPlayer"); }

                jfieldID posXID = env->GetFieldID(entityClass, "r", "D");
                if (!posXID) { env->ExceptionClear(); posXID = env->GetFieldID(entityClass, "posX", "D"); }
                env->ExceptionClear();
                jfieldID posYID = env->GetFieldID(entityClass, "s", "D");
                if (!posYID) { env->ExceptionClear(); posYID = env->GetFieldID(entityClass, "posY", "D"); }
                env->ExceptionClear();
                jfieldID posZID = env->GetFieldID(entityClass, "t", "D");
                if (!posZID) { env->ExceptionClear(); posZID = env->GetFieldID(entityClass, "posZ", "D"); }
                env->ExceptionClear();

                jmethodID getNameMethod = env->GetMethodID(entityClass, "c", "()Ljava/lang/String;");
                if (!getNameMethod) { env->ExceptionClear(); getNameMethod = env->GetMethodID(entityClass, "getName", "()Ljava/lang/String;"); }
                env->ExceptionClear();

                jclass entityLivingClass = Core::GetInstance().GetJava()->FindClass("pr", "net/minecraft/entity/EntityLivingBase");
                if (!entityLivingClass) { env->ExceptionClear(); entityLivingClass = env->FindClass("net/minecraft/entity/EntityLivingBase"); }
                jmethodID getHealthMethod = nullptr;
                if (entityLivingClass)
                {
                    getHealthMethod = env->GetMethodID(entityLivingClass, "aB", "()F");
                    if (!getHealthMethod) { env->ExceptionClear(); getHealthMethod = env->GetMethodID(entityLivingClass, "getHealth", "()F"); }
                    env->ExceptionClear();
                }
                jmethodID getMaxHealthMethod = nullptr;
                if (entityLivingClass)
                {
                    getMaxHealthMethod = env->GetMethodID(entityLivingClass, "aC", "()F");
                    if (!getMaxHealthMethod) { env->ExceptionClear(); getMaxHealthMethod = env->GetMethodID(entityLivingClass, "getMaxHealth", "()F"); }
                    env->ExceptionClear();
                }

                double nearestDist = 1000.0;

                for (int i = 0; i < size; i++)
                {
                    jobject entity = env->CallObjectMethod(listObj, getMethod, i);
                    if (!entity) continue;
                    if (env->IsSameObject(entity, localPlayer)) { env->DeleteLocalRef(entity); continue; }

                    bool isPlayer = playerClass && env->IsInstanceOf(entity, playerClass);
                    if (!isPlayer) { env->DeleteLocalRef(entity); continue; }

                    double ex = posXID ? env->GetDoubleField(entity, posXID) : 0.0;
                    double ey = posYID ? env->GetDoubleField(entity, posYID) : 0.0;
                    double ez = posZID ? env->GetDoubleField(entity, posZID) : 0.0;

                    jclass lpc = env->GetObjectClass(localPlayer);
                    jfieldID lpxID = env->GetFieldID(lpc, "r", "D");
                    if (!lpxID) { env->ExceptionClear(); lpxID = env->GetFieldID(lpc, "posX", "D"); }
                    env->ExceptionClear();
                    jfieldID lpyID = env->GetFieldID(lpc, "s", "D");
                    if (!lpyID) { env->ExceptionClear(); lpyID = env->GetFieldID(lpc, "posY", "D"); }
                    env->ExceptionClear();
                    jfieldID lpzID = env->GetFieldID(lpc, "t", "D");
                    if (!lpzID) { env->ExceptionClear(); lpzID = env->GetFieldID(lpc, "posZ", "D"); }
                    env->ExceptionClear();

                    double lx = env->GetDoubleField(localPlayer, lpxID);
                    double ly = env->GetDoubleField(localPlayer, lpyID);
                    double lz = env->GetDoubleField(localPlayer, lpzID);
                    env->DeleteLocalRef(lpc);

                    double dx = ex - lx;
                    double dy = ey - ly;
                    double dz = ez - lz;
                    double dist = sqrt(dx * dx + dy * dy + dz * dz);

                    if (dist < nearestDist && dist < 6.0)
                    {
                        nearestDist = dist;
                        targetX = ex;
                        targetY = ey;
                        targetZ = ez;

                        if (getNameMethod)
                        {
                            jobject jname = env->CallObjectMethod(entity, getNameMethod);
                            if (jname)
                            {
                                const char* chars = env->GetStringUTFChars((jstring)jname, nullptr);
                                if (chars) { targetName = chars; env->ReleaseStringUTFChars((jstring)jname, chars); }
                                env->DeleteLocalRef(jname);
                            }
                        }

                        if (getHealthMethod)
                            targetHealth = env->CallFloatMethod(entity, getHealthMethod);
                        if (getMaxHealthMethod)
                            targetMaxHealth = env->CallFloatMethod(entity, getMaxHealthMethod);

                        hasTarget = true;
                    }

                    env->DeleteLocalRef(entity);
                }

                if (entityLivingClass) env->DeleteLocalRef(entityLivingClass);
                if (entityClass) env->DeleteLocalRef(entityClass);
                if (playerClass) env->DeleteLocalRef(playerClass);
            }
            env->DeleteLocalRef(listClass);
            env->DeleteLocalRef(listObj);
        }
    }

    env->DeleteLocalRef(worldClass);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);

    if (!hasTarget) return;

    float x = ((NumberSetting*)FindSetting("X"))->GetValue();
    float y = ((NumberSetting*)FindSetting("Y"))->GetValue();
    bool showHealth = ((BooleanSetting*)FindSetting("ShowHealth"))->GetValue();
    bool showDistance = ((BooleanSetting*)FindSetting("ShowDistance"))->GetValue();
    ImColor color = ((ColorSetting*)FindSetting("Color"))->GetValue();
    ImU32 col = color;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImVec2 textSize = ImGui::CalcTextSize(targetName.c_str());
    float w = textSize.x + 20.0f;
    if (w < 120.0f) w = 120.0f;
    float h = 40.0f;

    draw->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), IM_COL32(0, 0, 0, 160), 4.0f);
    draw->AddText(ImVec2(x + 8.0f, y + 4.0f), col, targetName.c_str());

    float curY = y + textSize.y + 8.0f;

    if (showHealth && targetMaxHealth > 0.0f)
    {
        float hpPct = targetHealth / targetMaxHealth;
        if (hpPct < 0.0f) hpPct = 0.0f;
        if (hpPct > 1.0f) hpPct = 1.0f;

        ImU32 hpColor = IM_COL32(50, 200, 50, 255);
        if (hpPct < 0.3f) hpColor = IM_COL32(200, 50, 50, 255);
        else if (hpPct < 0.6f) hpColor = IM_COL32(200, 200, 50, 255);

        draw->AddRectFilled(ImVec2(x + 8.0f, curY), ImVec2(x + w - 8.0f, curY + 4.0f), IM_COL32(60, 60, 60, 200), 2.0f);
        draw->AddRectFilled(ImVec2(x + 8.0f, curY), ImVec2(x + 8.0f + (w - 16.0f) * hpPct, curY + 4.0f), hpColor, 2.0f);

        char hpText[32];
        snprintf(hpText, sizeof(hpText), "%.1f / %.1f", targetHealth, targetMaxHealth);
        draw->AddText(ImVec2(x + 8.0f, curY + 6.0f), IM_COL32(220, 220, 220, 255), hpText);
        curY += 20.0f;
    }

    if (showDistance)
    {
        jclass mcClass2 = Core::GetInstance().GetJava()->FindClass("ave", "net/minecraft/client/Minecraft");
        if (!mcClass2) { env->ExceptionClear(); mcClass2 = env->FindClass("net/minecraft/client/Minecraft"); }
        if (mcClass2)
        {
            jmethodID getMc2 = env->GetStaticMethodID(mcClass2, "A", "()Lave;");
            if (!getMc2) { env->ExceptionClear(); getMc2 = env->GetStaticMethodID(mcClass2, "a", "()Lave;"); }
            if (!getMc2) { env->ExceptionClear(); getMc2 = env->GetStaticMethodID(mcClass2, "getMinecraft", "()Lave;"); }
            env->ExceptionClear();
            if (getMc2)
            {
                jobject mc2 = env->CallStaticObjectMethod(mcClass2, getMc2);
                if (mc2)
                {
                    jfieldID playerField2 = env->GetFieldID(mcClass2, "h", "Lbew;");
                    if (!playerField2) { env->ExceptionClear(); playerField2 = env->GetFieldID(mcClass2, "thePlayer", "Lbew;"); }
                    env->ExceptionClear();
                    if (playerField2)
                    {
                        jobject lp2 = env->GetObjectField(mc2, playerField2);
                        if (lp2)
                        {
                            jclass lpClass = env->GetObjectClass(lp2);
                            jfieldID lpxID2 = env->GetFieldID(lpClass, "r", "D");
                            if (!lpxID2) { env->ExceptionClear(); lpxID2 = env->GetFieldID(lpClass, "posX", "D"); }
                            env->ExceptionClear();
                            jfieldID lpyID2 = env->GetFieldID(lpClass, "s", "D");
                            if (!lpyID2) { env->ExceptionClear(); lpyID2 = env->GetFieldID(lpClass, "posY", "D"); }
                            env->ExceptionClear();
                            jfieldID lpzID2 = env->GetFieldID(lpClass, "t", "D");
                            if (!lpzID2) { env->ExceptionClear(); lpzID2 = env->GetFieldID(lpClass, "posZ", "D"); }
                            env->ExceptionClear();

                            double lx = env->GetDoubleField(lp2, lpxID2);
                            double ly = env->GetDoubleField(lp2, lpyID2);
                            double lz = env->GetDoubleField(lp2, lpzID2);

                            double dx = targetX - lx;
                            double dy = targetY - ly;
                            double dz = targetZ - lz;
                            double dist = sqrt(dx * dx + dy * dy + dz * dz);

                            char distText[32];
                            snprintf(distText, sizeof(distText), "%.1fm", dist);
                            draw->AddText(ImVec2(x + 8.0f, curY), IM_COL32(180, 180, 180, 255), distText);

                            env->DeleteLocalRef(lpClass);
                            env->DeleteLocalRef(lp2);
                        }
                    }
                    env->DeleteLocalRef(mc2);
                }
            }
            env->DeleteLocalRef(mcClass2);
        }
    }
}

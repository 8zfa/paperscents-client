#include "hitbox.h"
#include "../../../core.h"
#include "../../../utilities/logger.h"

HitBoxModule::HitBoxModule()
    : ModuleBase("HitBox", "Expand entity hitboxes", Category::Render)
{
    AddSetting<NumberSetting>("Size", 0.3f, 0.1f, 1.0f, 0.05f, "Hitbox expansion size");
}

void HitBoxModule::OnEnable() { Logger::Log("HitBox enabled"); }
void HitBoxModule::OnDisable() { Logger::Log("HitBox disabled"); }

void HitBoxModule::OnUpdate()
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

    jobject localPlayer = playerField ? env->GetObjectField(mc, playerField) : nullptr;
    if (!localPlayer) { env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jfieldID worldField = env->GetFieldID(mcClass, "f", "Ladm;");
    if (!worldField)
    {
        env->ExceptionClear();
        worldField = env->GetFieldID(mcClass, "theWorld", "Ladm;");
    }
    env->ExceptionClear();

    jobject world = worldField ? env->GetObjectField(mc, worldField) : nullptr;
    if (!world) { env->DeleteLocalRef(localPlayer); env->DeleteLocalRef(mc); env->DeleteLocalRef(mcClass); return; }

    jclass worldClass = env->GetObjectClass(world);
    jfieldID entitiesField = env->GetFieldID(worldClass, "g", "Ljava/util/List;");
    if (!entitiesField)
    {
        env->ExceptionClear();
        entitiesField = env->GetFieldID(worldClass, "loadedEntityList", "Ljava/util/List;");
    }
    env->ExceptionClear();

    if (!entitiesField)
    {
        env->DeleteLocalRef(worldClass);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    jobject listObj = env->GetObjectField(world, entitiesField);
    if (!listObj)
    {
        env->DeleteLocalRef(worldClass);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    jclass listClass = env->FindClass("java/util/List");
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    if (!sizeMethod || !getMethod)
    {
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(listObj);
        env->DeleteLocalRef(worldClass);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    float size = ((NumberSetting*)FindSetting("Size"))->GetValue();

    jclass entityClass = Core::GetInstance().GetJava()->FindClass("pk", "net/minecraft/entity/Entity");
    if (!entityClass)
    {
        env->ExceptionClear();
        entityClass = env->FindClass("net/minecraft/entity/Entity");
    }
    if (!entityClass)
    {
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(listObj);
        env->DeleteLocalRef(worldClass);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(mc);
        env->DeleteLocalRef(mcClass);
        return;
    }

    jfieldID widthID = env->GetFieldID(entityClass, "n", "F");
    if (!widthID)
    {
        env->ExceptionClear();
        widthID = env->GetFieldID(entityClass, "width", "F");
    }
    env->ExceptionClear();
    jfieldID heightID = env->GetFieldID(entityClass, "o", "F");
    if (!heightID)
    {
        env->ExceptionClear();
        heightID = env->GetFieldID(entityClass, "height", "F");
    }
    env->ExceptionClear();

    int entSize = env->CallIntMethod(listObj, sizeMethod);
    for (int i = 0; i < entSize; i++)
    {
        jobject entity = env->CallObjectMethod(listObj, getMethod, i);
        if (!entity) continue;

        if (env->IsSameObject(entity, localPlayer))
        {
            env->DeleteLocalRef(entity);
            continue;
        }

        if (widthID)
        {
            float baseW = env->GetFloatField(entity, widthID);
            float newW = baseW + size * 0.5f;
            if (newW < 0.1f) newW = 0.1f;
            env->SetFloatField(entity, widthID, newW);
        }
        if (heightID)
        {
            float baseH = env->GetFloatField(entity, heightID);
            float newH = baseH + size * 0.3f;
            if (newH < 0.1f) newH = 0.1f;
            env->SetFloatField(entity, heightID, newH);
        }

        env->DeleteLocalRef(entity);
    }

    env->DeleteLocalRef(entityClass);
    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(listObj);
    env->DeleteLocalRef(worldClass);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);
    env->DeleteLocalRef(mcClass);
}

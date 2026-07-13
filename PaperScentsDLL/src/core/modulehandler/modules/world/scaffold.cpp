#include "scaffold.h"
#include "../../../core.h"
#include "../../../utilities/jni_helpers.h"
#include "../../../sdk/strayCache.h"
#include "../../../utilities/logger.h"
#include "../../../rendering/menu/menu.h"
#include <cmath>
#include <cstdlib>

ScaffoldModule::ScaffoldModule()
    : ModuleBase("Scaffold", "Auto-place blocks under you", Category::Player)
{
    AddSetting<BooleanSetting>("Tower", true);
    AddSetting<NumberSetting>("Delay", 0.0f, 0.0f, 5.0f, 1.0f);
    AddSetting<BooleanSetting>("Swing", true);
    AddSetting<NumberSetting>("Update Interval", 3, 1, 10, 1, "Frames between updates");
    m_LastPlace = std::chrono::steady_clock::now();
}

void ScaffoldModule::OnEnable() { Logger::Log("Scaffold enabled"); m_TowerTicks = 0; }
void ScaffoldModule::OnDisable() { Logger::Log("Scaffold disabled"); m_TowerTicks = 0; }

int ScaffoldModule::FindBlockSlot(JNIEnv* env, jobject inventory)
{
    if (!inventory) return -1;
    jclass invClass = env->GetObjectClass(inventory);
    jmethodID getStack = env->GetMethodID(invClass, "a", "(I)Lzx;");
    if (!getStack) { env->ExceptionClear(); getStack = env->GetMethodID(invClass, "getStackInSlot", "(I)Lnet/minecraft/item/ItemStack;"); }
    if (!getStack) { env->ExceptionClear(); env->DeleteLocalRef(invClass); return -1; }
    env->ExceptionClear();
    jclass itemStackCls = env->FindClass("zx");
    if (!itemStackCls) { env->ExceptionClear(); itemStackCls = env->FindClass("net/minecraft/item/ItemStack"); }
    if (!itemStackCls) { env->DeleteLocalRef(invClass); return -1; }
    jmethodID getItem = env->GetMethodID(itemStackCls, "a", "()Ladq;");
    if (!getItem) { env->ExceptionClear(); getItem = env->GetMethodID(itemStackCls, "getItem", "()Lnet/minecraft/item/Item;"); }
    jclass itemBlockCls = env->FindClass("adz");
    if (!itemBlockCls) { env->ExceptionClear(); itemBlockCls = env->FindClass("net/minecraft/item/ItemBlock"); }
    env->ExceptionClear();
    for (int i = 36; i <= 44; i++)
    {
        jobject stack = env->CallObjectMethod(inventory, getStack, i);
        if (stack)
        {
            if (getItem && itemBlockCls)
            {
                jobject item = env->CallObjectMethod(stack, getItem);
                if (item)
                {
                    bool isBlock = env->IsInstanceOf(item, itemBlockCls) == JNI_TRUE;
                    env->DeleteLocalRef(item);
                    if (isBlock) { env->DeleteLocalRef(stack); env->DeleteLocalRef(itemBlockCls); env->DeleteLocalRef(itemStackCls); env->DeleteLocalRef(invClass); return i - 36; }
                }
            }
            env->DeleteLocalRef(stack);
        }
    }
    if (itemBlockCls) env->DeleteLocalRef(itemBlockCls);
    if (itemStackCls) env->DeleteLocalRef(itemStackCls);
    env->DeleteLocalRef(invClass);
    return -1;
}

static bool IsReplaceable(JNIEnv* env, jobject world, int x, int y, int z)
{
    if (!world || !StrayCache::World) return true;
    jclass worldClass = StrayCache::World;
    jmethodID isAir = env->GetMethodID(worldClass, "d", "(III)Z");
    if (!isAir) { env->ExceptionClear(); isAir = env->GetMethodID(worldClass, "isAirBlock", "(III)Z"); }
    if (!isAir) { env->ExceptionClear(); return true; }
    env->ExceptionClear();
    return env->CallBooleanMethod(world, isAir, x, y, z) == JNI_TRUE;
}

bool ScaffoldModule::PlaceBlock(JNIEnv* env, jobject mc, jobject player, jobject world, int blockX, int blockY, int blockZ)
{
    if (!mc || !player || !world) return false;
    // Check if target position is air
    if (!IsReplaceable(env, world, blockX, blockY, blockZ)) return false;
    // Find adjacent solid block to place against
    int adjX = blockX, adjY = blockY - 1, adjZ = blockZ;
    int side = 0;
    float hx = 0.5f, hy = 1.0f, hz = 0.5f;
    if (!IsReplaceable(env, world, adjX, adjY, adjZ))
    {
        side = 0; // UP (place on block below)
        hy = 1.0f;
    }
    else
    {
        adjX = blockX + 1; adjY = blockY; adjZ = blockZ;
        if (!IsReplaceable(env, world, adjX, adjY, adjZ)) { side = 4; hx = 0.0f; hy = 0.5f; hz = 0.5f; }
        else {
            adjX = blockX - 1;
            if (!IsReplaceable(env, world, adjX, adjY, adjZ)) { side = 5; hx = 1.0f; hy = 0.5f; hz = 0.5f; }
            else {
                adjX = blockX; adjZ = blockZ + 1;
                if (!IsReplaceable(env, world, adjX, adjY, adjZ)) { side = 2; hx = 0.5f; hy = 0.5f; hz = 0.0f; }
                else {
                    adjX = blockX; adjZ = blockZ - 1;
                    if (!IsReplaceable(env, world, adjX, adjY, adjZ)) { side = 3; hx = 0.5f; hy = 0.5f; hz = 1.0f; }
                    else return false;
                }
            }
        }
    }
    // Set rotation to face placement
    jclass entityClass = StrayCache::Entity;
    jfieldID yawField = StrayCache::Entity_rotationYaw;
    jfieldID pitchField = StrayCache::Entity_rotationPitch;
    if (yawField && pitchField)
    {
        double dx = (double)adjX + 0.5 - GetEntityPosX(env, player);
        double dy = (double)adjY + 0.5 - (GetEntityPosY(env, player) + 1.62);
        double dz = (double)adjZ + 0.5 - GetEntityPosZ(env, player);
        double dist = std::sqrt(dx * dx + dz * dz);
        float yaw = (float)(std::atan2(dz, dx) * 180.0 / 3.14159265) - 90.0f;
        float pitch = (float)(-(std::atan2(dy, dist) * 180.0 / 3.14159265));
        env->SetFloatField(player, yawField, yaw);
        env->SetFloatField(player, pitchField, pitch);
    }
    // Switch to block slot
    jobject inv = nullptr;
    if (StrayCache::EntityPlayer_inventory)
        inv = env->GetObjectField(player, StrayCache::EntityPlayer_inventory);
    if (!inv) return false;
    int slot = FindBlockSlot(env, inv);
    if (slot == -1) { env->DeleteLocalRef(inv); return false; }
    jclass invClass = env->GetObjectClass(inv);
    jfieldID ciField = env->GetFieldID(invClass, "d", "I");
    if (!ciField) { env->ExceptionClear(); ciField = env->GetFieldID(invClass, "currentItem", "I"); }
    if (ciField) env->SetIntField(inv, ciField, slot);
    env->DeleteLocalRef(invClass);
    // Get held item stack
    jmethodID getHeldItem = StrayCache::EntityLivingBase_getHeldItem;
    if (!getHeldItem) { getHeldItem = env->GetMethodID(StrayCache::EntityLivingBase, "dd", "()Lzv;"); if (!getHeldItem) { env->ExceptionClear(); env->DeleteLocalRef(inv); return false; } env->ExceptionClear(); }
    jobject stack = env->CallObjectMethod(player, getHeldItem);
    if (!stack) { env->DeleteLocalRef(inv); return false; }
    // Build BlockPos and EnumFacing
    jclass blockPosClass = env->FindClass("aza");
    if (!blockPosClass) { env->ExceptionClear(); blockPosClass = env->FindClass("net/minecraft/util/BlockPos"); }
    if (!blockPosClass) { env->DeleteLocalRef(stack); env->DeleteLocalRef(inv); return false; }
    jmethodID bpInit = env->GetMethodID(blockPosClass, "<init>", "(III)V");
    if (!bpInit) { env->ExceptionClear(); env->DeleteLocalRef(blockPosClass); env->DeleteLocalRef(stack); env->DeleteLocalRef(inv); return false; }
    env->ExceptionClear();
    jobject bp = env->NewObject(blockPosClass, bpInit, adjX, adjY, adjZ);
    env->DeleteLocalRef(blockPosClass);
    if (!bp) { env->DeleteLocalRef(stack); env->DeleteLocalRef(inv); return false; }
    jclass enumFacingClass = env->FindClass("bev");
    if (!enumFacingClass) { env->ExceptionClear(); enumFacingClass = env->FindClass("net/minecraft/util/EnumFacing"); }
    jfieldID sideField = nullptr;
    if (enumFacingClass) { sideField = env->GetStaticFieldID(enumFacingClass, "a", "Lbev;"); if (!sideField) { env->ExceptionClear(); sideField = env->GetStaticFieldID(enumFacingClass, "DOWN", "Lbev;"); } }
    if (!sideField) { env->ExceptionClear(); env->DeleteLocalRef(bp); env->DeleteLocalRef(stack); env->DeleteLocalRef(inv); return false; }
    env->ExceptionClear();
    jobject face = env->GetStaticObjectField(enumFacingClass, sideField);
    // Get the actual facing based on side
    jmethodID valuesMethod = env->GetStaticMethodID(enumFacingClass, "values", "()[Lbev;");
    if (valuesMethod)
    {
        env->ExceptionClear();
        jobjectArray facings = (jobjectArray)env->CallStaticObjectMethod(enumFacingClass, valuesMethod);
        if (facings && side >= 0 && side < 6)
        {
            jobject f = env->GetObjectArrayElement(facings, side);
            if (f) { if (face) env->DeleteLocalRef(face); face = f; }
            env->DeleteLocalRef(facings);
        }
    }
    if (enumFacingClass) env->DeleteLocalRef(enumFacingClass);
    if (!face) { env->DeleteLocalRef(bp); env->DeleteLocalRef(stack); env->DeleteLocalRef(inv); return false; }
    // Build Vec3 hit vector
    jclass vec3Class = env->FindClass("azu");
    if (!vec3Class) { env->ExceptionClear(); vec3Class = env->FindClass("net/minecraft/util/Vec3"); }
    jmethodID vec3Init = nullptr;
    if (vec3Class) { vec3Init = env->GetMethodID(vec3Class, "<init>", "(DDD)V"); if (!vec3Init) { env->ExceptionClear(); } }
    jobject hitVec = nullptr;
    if (vec3Init) { env->ExceptionClear(); hitVec = env->NewObject(vec3Class, vec3Init, (double)(adjX + hx), (double)(adjY + hy), (double)(adjZ + hz)); }
    if (vec3Class) env->DeleteLocalRef(vec3Class);
    // Call playerController.onPlayerRightClick
    jobject pc = StrayCache::Minecraft_playerController ? env->GetObjectField(mc, StrayCache::Minecraft_playerController) : nullptr;
    bool placed = false;
    if (pc)
    {
        jclass pcClass = env->GetObjectClass(pc);
        jmethodID rightClick = env->GetMethodID(pcClass, "a", "(Lbew;Ladm;Lzx;Laza;Lbev;Lazu;)Z");
        if (!rightClick) { env->ExceptionClear(); rightClick = env->GetMethodID(pcClass, "onPlayerRightClick", "(Lnet/minecraft/client/entity/EntityPlayerSP;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;Lnet/minecraft/util/BlockPos;Lnet/minecraft/util/EnumFacing;Lnet/minecraft/util/Vec3;)Z"); }
        if (rightClick)
        {
            env->ExceptionClear();
            placed = env->CallBooleanMethod(pc, rightClick, player, world, stack, bp, face, hitVec ? hitVec : bp) == JNI_TRUE;
        }
        env->DeleteLocalRef(pcClass);
        env->DeleteLocalRef(pc);
    }
    // Swing
    if (placed && ((BooleanSetting*)FindSetting("Swing"))->GetValue())
    {
        jclass spClass = StrayCache::EntityPlayerSP;
        if (!spClass) { spClass = env->FindClass("bew"); if (!spClass) { env->ExceptionClear(); spClass = env->FindClass("net/minecraft/client/entity/EntityPlayerSP"); } }
        if (spClass)
        {
            jmethodID swing = env->GetMethodID(spClass, "v", "()V");
            if (!swing) { env->ExceptionClear(); swing = env->GetMethodID(spClass, "swingItem", "()V"); }
            if (swing) { env->ExceptionClear(); env->CallVoidMethod(player, swing); }
            if (spClass != StrayCache::EntityPlayerSP) env->DeleteLocalRef(spClass);
        }
    }
    if (hitVec) env->DeleteLocalRef(hitVec);
    if (face) env->DeleteLocalRef(face);
    if (bp) env->DeleteLocalRef(bp);
    if (stack) env->DeleteLocalRef(stack);
    env->DeleteLocalRef(inv);
    return placed;
}

void ScaffoldModule::OnUpdate()
{
    if (!IsEnabled()) return;
    if (++m_FrameCounter >= m_UpdateInterval) {
        m_FrameCounter = 0;
    } else {
        return;
    }
    if (Menu::GetInstance().IsOpen()) return;
    JNIEnv* env = Java::GetThreadEnv();
    if (!env) return;
    if (!StrayCache::MinecraftInstance || !StrayCache::Minecraft_thePlayer || !StrayCache::Minecraft_theWorld) return;
    env->ExceptionClear();
    jobject mc = StrayCache::MinecraftInstance;
    jobject player = env->GetObjectField(mc, StrayCache::Minecraft_thePlayer);
    if (!player) return;
    jobject world = env->GetObjectField(mc, StrayCache::Minecraft_theWorld);
    if (!world) { env->DeleteLocalRef(player); return; }
    double pX = GetEntityPosX(env, player);
    double pY = GetEntityPosY(env, player);
    double pZ = GetEntityPosZ(env, player);
    // Tower check
    bool tower = ((BooleanSetting*)FindSetting("Tower"))->GetValue();
    bool jumpDown = false;
    if (tower && StrayCache::GameSettings_keyBindJump)
    {
        jobject gs = env->GetObjectField(mc, StrayCache::Minecraft_gameSettings);
        if (gs)
        {
            jobject keyJump = env->GetObjectField(gs, StrayCache::GameSettings_keyBindJump);
            if (keyJump)
            {
                jclass kbClass = env->GetObjectClass(keyJump);
                jfieldID pressedField = env->GetFieldID(kbClass, "e", "Z");
                if (!pressedField) { env->ExceptionClear(); pressedField = env->GetFieldID(kbClass, "pressed", "Z"); }
                if (pressedField) { env->ExceptionClear(); jumpDown = env->GetBooleanField(keyJump, pressedField) == JNI_TRUE; }
                env->DeleteLocalRef(kbClass);
                env->DeleteLocalRef(keyJump);
            }
            env->DeleteLocalRef(gs);
        }
    }
    m_Towering = tower && jumpDown;
    // Calculate target block position
    int bX = (int)std::floor(pX);
    int bY = (int)std::floor(m_Towering ? pY : pY - 0.5);
    int bZ = (int)std::floor(pZ);
    double fracX = pX - std::floor(pX);
    double fracZ = pZ - std::floor(pZ);
    if (fracX < 0.3) bX--;
    if (fracX > 0.7) bX++;
    if (fracZ < 0.3) bZ--;
    if (fracZ > 0.7) bZ++;
    // Delay
    int delay = (int)((NumberSetting*)FindSetting("Delay"))->GetValue();
    m_PlaceTicks++;
    if (m_PlaceTicks <= delay) { env->DeleteLocalRef(world); env->DeleteLocalRef(player); return; }
    // Try to place
    bool placed = PlaceBlock(env, mc, player, world, bX, bY, bZ);
    if (placed) m_PlaceTicks = 0;
    // Tower motion
    if (m_Towering && pY - std::floor(pY) < 0.7)
    {
        jfieldID motionYField = StrayCache::Entity_motionY;
        if (motionYField)
            env->SetDoubleField(player, motionYField, 0.42);
    }
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(player);
}

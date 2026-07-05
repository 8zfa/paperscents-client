#include "stub_modules.h"
#include <json.hpp>

// Combat stubs
KeepSprintModule::KeepSprintModule() : ModuleBase("KeepSprint", "Don't lose sprint on attack", Category::Combat) {}
void KeepSprintModule::OnEnable() {}
void KeepSprintModule::OnDisable() {}

BackTrackModule::BackTrackModule() : ModuleBase("BackTrack", "Attack players where they were", Category::Combat) {}
void BackTrackModule::OnEnable() {}
void BackTrackModule::OnDisable() {}

FakeLagModule::FakeLagModule() : ModuleBase("FakeLag", "Delay packets to appear laggy", Category::Combat) {}
void FakeLagModule::OnEnable() {}
void FakeLagModule::OnDisable() {}

TriggerBotModule::TriggerBotModule() : ModuleBase("TriggerBot", "Auto attack when crosshair on entity", Category::Combat) {}
void TriggerBotModule::OnEnable() {}
void TriggerBotModule::OnDisable() {}

BowAimModule::BowAimModule() : ModuleBase("BowAim", "Auto aim bow shots", Category::Combat) {}
void BowAimModule::OnEnable() {}
void BowAimModule::OnDisable() {}

RodAuraModule::RodAuraModule() : ModuleBase("RodAura", "Auto use fishing rod", Category::Combat) {}
void RodAuraModule::OnEnable() {}
void RodAuraModule::OnDisable() {}

// Movement stubs
NoWebModule::NoWebModule() : ModuleBase("NoWeb", "Move freely in webs", Category::Movement) {}
void NoWebModule::OnEnable() {}
void NoWebModule::OnDisable() {}

LongJumpModule::LongJumpModule() : ModuleBase("LongJump", "Jump further", Category::Movement) {}
void LongJumpModule::OnEnable() {}
void LongJumpModule::OnDisable() {}

HighJumpModule::HighJumpModule() : ModuleBase("HighJump", "Jump higher", Category::Movement) {}
void HighJumpModule::OnEnable() {}
void HighJumpModule::OnDisable() {}

LiquidWalkModule::LiquidWalkModule() : ModuleBase("LiquidWalk", "Walk on water/lava", Category::Movement) {}
void LiquidWalkModule::OnEnable() {}
void LiquidWalkModule::OnDisable() {}

PhaseModule::PhaseModule() : ModuleBase("Phase", "Walk through blocks", Category::Movement) {}
void PhaseModule::OnEnable() {}
void PhaseModule::OnDisable() {}

StrafeModule::StrafeModule() : ModuleBase("Strafe", "Perfect movement strafing", Category::Movement) {}
void StrafeModule::OnEnable() {}
void StrafeModule::OnDisable() {}

BHopModule::BHopModule() : ModuleBase("BHop", "Bunny hop for speed", Category::Movement) {}
void BHopModule::OnEnable() {}
void BHopModule::OnDisable() {}

// Render stubs
ChamsModule::ChamsModule() : ModuleBase("Chams", "See entities through walls with color", Category::Render) {}
void ChamsModule::OnEnable() {}
void ChamsModule::OnDisable() {}

FreeCamModule::FreeCamModule() : ModuleBase("FreeCam", "Detach camera from body", Category::Render) {}
void FreeCamModule::OnEnable() {}
void FreeCamModule::OnDisable() {}

NoHurtCamModule::NoHurtCamModule() : ModuleBase("NoHurtCam", "Disable hurt camera effect", Category::Render) {}
void NoHurtCamModule::OnEnable() {}
void NoHurtCamModule::OnDisable() {}

ItemESPModule::ItemESPModule() : ModuleBase("ItemESP", "See items through walls", Category::Render) {}
void ItemESPModule::OnEnable() {}
void ItemESPModule::OnDisable() {}

SkeletonESPModule::SkeletonESPModule() : ModuleBase("SkeletonESP", "Draw skeletons on players", Category::Render) {}
void SkeletonESPModule::OnEnable() {}
void SkeletonESPModule::OnDisable() {}

ArmorHUDModule::ArmorHUDModule() : ModuleBase("ArmorHUD", "Display player armor on screen", Category::Render) {}
void ArmorHUDModule::OnEnable() {}
void ArmorHUDModule::OnDisable() {}

DirectionHUDModule::DirectionHUDModule() : ModuleBase("DirectionHUD", "Show facing direction", Category::Render) {}
void DirectionHUDModule::OnEnable() {}
void DirectionHUDModule::OnDisable() {}

// Player stubs
FastMineModule::FastMineModule() : ModuleBase("FastMine", "Break blocks faster", Category::Player) {}
void FastMineModule::OnEnable() {}
void FastMineModule::OnDisable() {}

AutoEatModule::AutoEatModule() : ModuleBase("AutoEat", "Auto eat when hungry", Category::Player) {}
void AutoEatModule::OnEnable() {}
void AutoEatModule::OnDisable() {}

AutoRespawnModule::AutoRespawnModule() : ModuleBase("AutoRespawn", "Auto respawn on death", Category::Player) {}
void AutoRespawnModule::OnEnable() {}
void AutoRespawnModule::OnDisable() {}

AutoSoupModule::AutoSoupModule() : ModuleBase("AutoSoup", "Auto eat soup on low health", Category::Player) {}
void AutoSoupModule::OnEnable() {}
void AutoSoupModule::OnDisable() {}

AutoPearlModule::AutoPearlModule() : ModuleBase("AutoPearl", "Auto throw pearl at position", Category::Player) {}
void AutoPearlModule::OnEnable() {}
void AutoPearlModule::OnDisable() {}

FastUseModule::FastUseModule() : ModuleBase("FastUse", "Use items faster", Category::Player) {}
void FastUseModule::OnEnable() {}
void FastUseModule::OnDisable() {}

// Misc stubs
AutoGGModule::AutoGGModule() : ModuleBase("AutoGG", "Auto say gg on game end", Category::Misc) {}
void AutoGGModule::OnEnable() {}
void AutoGGModule::OnDisable() {}

AutoTipModule::AutoTipModule() : ModuleBase("AutoTip", "Auto tip players", Category::Misc) {}
void AutoTipModule::OnEnable() {}
void AutoTipModule::OnDisable() {}

MiddleClickPearlModule::MiddleClickPearlModule() : ModuleBase("MiddleClickPearl", "Throw pearl on middle click", Category::Misc) {}
void MiddleClickPearlModule::OnEnable() {}
void MiddleClickPearlModule::OnDisable() {}

TimeChangerModule::TimeChangerModule() : ModuleBase("TimeChanger", "Change in-game time", Category::Misc) {}
void TimeChangerModule::OnEnable() {}
void TimeChangerModule::OnDisable() {}

ZoomModule::ZoomModule() : ModuleBase("Zoom", "Zoom in with keybind", Category::Misc) {}
void ZoomModule::OnEnable() {}
void ZoomModule::OnDisable() {}

ChatFilterModule::ChatFilterModule() : ModuleBase("ChatFilter", "Filter chat messages", Category::Misc) {}
void ChatFilterModule::OnEnable() {}
void ChatFilterModule::OnDisable() {}

FriendsModule::FriendsModule() : ModuleBase("Friends", "Manage friends list", Category::Misc) {}
void FriendsModule::OnEnable() {}
void FriendsModule::OnDisable() {}

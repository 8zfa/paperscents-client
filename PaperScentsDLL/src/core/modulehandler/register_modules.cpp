#include "modulehandler.h"
#include "modules/combat/ka.h"
#include "modules/combat/velocity.h"
#include "modules/combat/autoclicker.h"
#include "modules/combat/reach.h"
#include "modules/combat/criticals.h"
#include "modules/combat/autoblock.h"
#include "modules/combat/aimassist.h"
#include "modules/combat/wtap.h"
#include "modules/combat/keepsprint.h"
#include "modules/combat/bowaim.h"
#include "modules/combat/backtrack.h"
#include "modules/combat/fakelag.h"
#include "modules/combat/triggerbot.h"
#include "modules/combat/rodaura.h"
#include "modules/misc/antibot.h"
#include "modules/misc/teams.h"
#include "modules/misc/timer.h"
#include "modules/misc/autotool.h"
#include "modules/movement/sprint.h"
#include "modules/movement/speed.h"
#include "modules/movement/fly.h"
#include "modules/movement/noslow.h"
#include "modules/movement/step.h"
#include "modules/movement/invmove.h"
#include "modules/movement/noweb.h"
#include "modules/movement/longjump.h"
#include "modules/movement/highjump.h"
#include "modules/movement/liquidwalk.h"
#include "modules/movement/phase.h"
#include "modules/movement/strafe.h"
#include "modules/movement/bhop.h"
#include "modules/render/chams.h"
#include "modules/render/itemesp.h"
#include "modules/render/armorhud.h"
#include "modules/render/directionhud.h"
#include "modules/render/esp.h"
#include "modules/render/fullbright.h"
#include "modules/render/tracers.h"
#include "modules/render/nametags.h"
#include "modules/render/hitbox.h"
#include "modules/render/skeletonesp.h"
#include "modules/render/freecam.h"
#include "modules/render/nohurtcam.h"
#include "modules/player/nofall.h"
#include "modules/player/legitscaffold.h"
#include "modules/player/fastplace.h"
#include "modules/player/antivoid.h"
#include "modules/player/norotate.h"
#include "modules/player/autoeat.h"
#include "modules/world/scaffold.h"
#include "modules/exploit/disabler.h"
#include "modules/visual/arraylist.h"
#include "modules/visual/clickgui.h"
#include "modules/visual/watermark.h"
#include "modules/visual/targethud.h"
#include "modules/visual/keystrokes.h"
#include "modules/visual/coordinates.h"
#include "modules/player/fastmine.h"
#include "modules/player/autorespawn.h"
#include "modules/player/autosoup.h"
#include "modules/player/autopearl.h"
#include "modules/player/fastuse.h"
#include "modules/misc/autogg.h"
#include "modules/misc/autotip.h"
#include "modules/misc/middleclickpearl.h"
#include "modules/misc/timechanger.h"
#include "modules/misc/zoom.h"
#include "modules/misc/chatfilter.h"
#include "modules/misc/friends.h"
void RegisterModules()
{
    ModuleHandler& handler = ModuleHandler::GetInstance();

    // Combat
    handler.RegisterModule(new KillAuraModule());
    handler.RegisterModule(new AimAssistModule());
    handler.RegisterModule(new VelocityModule());
    handler.RegisterModule(new ReachModule());
    handler.RegisterModule(new HitBoxModule());
    handler.RegisterModule(new AutoClickerModule());
    handler.RegisterModule(new CriticalsModule());
    handler.RegisterModule(new KeepSprintModule());
    handler.RegisterModule(new AntiBotModule());
    handler.RegisterModule(new BackTrackModule());
    handler.RegisterModule(new FakeLagModule());
    handler.RegisterModule(new TriggerBotModule());
    handler.RegisterModule(new WTapModule());
    handler.RegisterModule(new BowAimModule());
    handler.RegisterModule(new RodAuraModule());

    // Movement
    handler.RegisterModule(new SpeedModule());
    handler.RegisterModule(new FlyModule());
    handler.RegisterModule(new NoSlowModule());
    handler.RegisterModule(new NoWebModule());
    handler.RegisterModule(new LongJumpModule());
    handler.RegisterModule(new HighJumpModule());
    handler.RegisterModule(new LiquidWalkModule());
    handler.RegisterModule(new PhaseModule());
    handler.RegisterModule(new StepModule());
    handler.RegisterModule(new SprintModule());
    handler.RegisterModule(new TimerModule());
    handler.RegisterModule(new StrafeModule());
    handler.RegisterModule(new BHopModule());

    // Render
    handler.RegisterModule(new ClickGUIModule());
    handler.RegisterModule(new ESPModule());
    handler.RegisterModule(new ChamsModule());
    handler.RegisterModule(new NametagsModule());
    handler.RegisterModule(new FullBrightModule());
    handler.RegisterModule(new FreeCamModule());
    handler.RegisterModule(new TracersModule());
    handler.RegisterModule(new NoHurtCamModule());
    handler.RegisterModule(new ItemESPModule());
    handler.RegisterModule(new SkeletonESPModule());
    handler.RegisterModule(new ArmorHUDModule());
    handler.RegisterModule(new DirectionHUDModule());
    handler.RegisterModule(new CoordinatesModule());
    handler.RegisterModule(new KeyStrokesModule());
    handler.RegisterModule(new ArrayListModule());
    handler.RegisterModule(new WatermarkModule());
    handler.RegisterModule(new TargetHUDModule());

    // Player
    handler.RegisterModule(new AutoToolModule());
    handler.RegisterModule(new FastPlaceModule());
    handler.RegisterModule(new FastMineModule());
    handler.RegisterModule(new LegitScaffoldModule());
    handler.RegisterModule(new ScaffoldModule());
    handler.RegisterModule(new AutoEatModule());
    handler.RegisterModule(new AntiVoidModule());
    handler.RegisterModule(new NoFallModule());
    handler.RegisterModule(new AutoRespawnModule());
    handler.RegisterModule(new AutoSoupModule());
    handler.RegisterModule(new AutoPearlModule());
    handler.RegisterModule(new FastUseModule());

    // Misc
    handler.RegisterModule(new AutoGGModule());
    handler.RegisterModule(new AutoTipModule());
    handler.RegisterModule(new MiddleClickPearlModule());
    handler.RegisterModule(new TimeChangerModule());
    handler.RegisterModule(new ZoomModule());
    handler.RegisterModule(new ChatFilterModule());
    handler.RegisterModule(new NoRotateModule());
    handler.RegisterModule(new FriendsModule());
    handler.RegisterModule(new TeamsModule());
    handler.RegisterModule(new DisablerModule());

    // Auto-enable defaults
    auto* arrayList = handler.GetModule("ArrayList");
    if (arrayList) arrayList->SetEnabled(true);

    auto* sprint = handler.GetModule("Sprint");
    if (sprint) sprint->SetEnabled(true);

    auto* watermark = handler.GetModule("Watermark");
    if (watermark) watermark->SetEnabled(true);
}

/* Compile the original BattleShip VS Results scene translation unit. */
#include <string.h>

#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <lb/transition.h>
#include <mn/menu.h>
#include <nds/nds_controller.h>
#include <nds/nds_platform.h>
#include <nds/nds_renderer.h>
#include <nds/nds_scene_manager.h>
#include <nds/nds_startup.h>
#include <nds/nds_task37_profile.h>
#include <nds/timers.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/controller.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>

#include "../../decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.h"

extern sb32 (*dLBCommonFuncMatrixList[])(void);
extern void efManagerInitEffects(void);
extern void *efManagerConfettiMakeEffect(Vec3f *pos,
                                         sb32 is_genlink_mask);
extern u32 sGCCommonsActiveNum;
extern u32 sGCSpritesActiveNum;
extern void ndsFighterManagerRegisterDisplayFighter(GObj *fighter_gobj,
                                                     u32 slot);

#define NDS_VS_RESULTS_PASS 0x56535231u

volatile u32 gNdsVSResultsResult;
volatile u32 gNdsVSResultsMask;
volatile u32 gNdsVSResultsStartCount;
volatile u32 gNdsVSResultsTickCount;
volatile u32 gNdsVSResultsLoadedFileCount;
volatile u32 gNdsVSResultsFighterCount;
volatile u32 gNdsVSResultsGObjCount;
volatile u32 gNdsVSResultsSObjCount;
volatile u32 gNdsVSResultsKind;
volatile u32 gNdsVSResultsCameraProcCount;
volatile u32 gNdsVSResultsFighterDisplayCount;
volatile u32 gNdsVSResultsFighterSubmitCount;
/* P2-2: Results itself has always been source N-player. Keep the observation
 * surface the same width so a four-way result can be inspected without
 * pretending slots 2/3 do not exist. P0/P1 readers remain ABI-compatible. */
volatile u32 gNdsVSResultsFighterPlace[GMCOMMON_PLAYERS_MAX];
volatile u32 gNdsVSResultsFighterStatus[GMCOMMON_PLAYERS_MAX];
volatile s32 gNdsVSResultsFighterMotion[GMCOMMON_PLAYERS_MAX];
/* R2-07 R1. The Battle -> Results hand-off is ~30 s of dead air with the last
 * battle frame still on screen, and the board's first step is to split it rather
 * than assume the loader owns it. These three price the scene's task-start:
 * `FuncStart` is the whole of `mnVSResultsFuncStart`, `SetupFiles` is the fighter
 * asset half inside it, and the difference is `lbRelocLoadFilesListed` plus scene
 * construction. Ticks, not VBlanks, so nothing is floored (standing rule 11);
 * `cpuGetTiming` is 32-bit at 33.514 MHz and wraps at ~128 s, comfortably clear
 * of a 30 s span. Cost is four timer reads per Results entry. */
volatile u32 gNdsVSResultsFuncStartTicks;
volatile u32 gNdsVSResultsSetupFilesTicks;
volatile u32 gNdsVSResultsSetupFilesCalls;
/* ...and the enclosing span, because the first measurement refuted the framing.
 * `FuncStart` came in at 21,851,904 ticks (0.65 s) against a hand-off the board
 * had recorded as ~30 s, so the load is NOT where the dead air lives and a
 * subtraction against `sVBlankCount` is too coarse to say where it is. These
 * two bracket battle-taskman-exit to the first Results tick directly. Written
 * once each per transition. */
volatile u32 gNdsVSResultsTransitionStartTick;
volatile u32 gNdsVSResultsTransitionTicks;
/* Time to the reveal, which is what the owner actually perceives as dead air.
 * The source holds the wallpaper until Results tic 80 and the result panels
 * until tic 120 (mnvsresults.c:2843-2844), so the last battle frame stays on
 * screen until this scene has rendered eighty of its own frames. That makes the
 * "GAME SET dead air" a function of the scene's PER-FRAME cost, not of any load,
 * and this pair measures it end to end from the first Results tick. */
volatile u32 gNdsVSResultsFirstTickStamp;
volatile u32 gNdsVSResultsToWallpaperTicks;
volatile u32 gNdsVSResultsToResultsTicks;
/* Owner requirement, switch plan R2-07: "Pressing start in Results screen should
 * restart match (P1 specific)". Counts the redirects so a soak can prove match
 * two happened rather than inferring it from a screenshot. */
volatile u32 gNdsVSResultsRematchCount;
/* 1..9, wrapping. Starts at 1 so the demo opens easy. */
volatile u32 gNdsDemoFoxCpuLevel = 1u;
volatile u32 gNdsVSResultsInputPollCount;
volatile u32 gNdsVSResultsInputSeenMask;
volatile u32 gNdsVSResultsInputTapMask;
volatile u32 gNdsVSResultsPadMask;

/* R2-07. The presented-frame VBlank-interval histogram for THIS scene.
 *
 * Acceptance is defined on the 2/3/4/5+ interval histogram and the maximum
 * interval, never on min FPS or a half-second average, and until now only the
 * battle loop produced one -- the Results loop reaches `ndsPlatformEndFrame`
 * with no interval recorder at all, so the scene's cadence could only be
 * inferred from breakpoint censuses.
 *
 * This is an OBSERVATION, deliberately not a second scheduler. The battle
 * histogram is welded to `ndsPlatformSchedulePresentAtVBlank` and its phase
 * bookkeeping; reusing those globals here would corrupt the battle numbers with
 * Results frames, and duplicating the scheduler is exactly the "competing
 * pacing state" the R2-07 research plan forbids. All this does is difference the
 * VBlank counter across consecutive Results iterations and bucket the result.
 *
 * Index n counts intervals of exactly n VBlanks; the last index is the
 * n-or-more bin. Index 0/1 exist so a sub-2 reading has somewhere to land and
 * shows up as the anomaly it would be rather than being folded into the 2
 * bucket.
 *
 * SIXTEEN BINS, not the battle path's six. The owner set P95 as the acceptance
 * metric for this scene, and a percentile cannot be read out of a distribution
 * whose tail is a single catch-all: the first run of this instrument returned
 * 117 intervals at 2 and 210 at "5+" with a max of 9, which pins P95 no more
 * precisely than "somewhere in 5..9". The battle path can afford a 5+ bin
 * because its scheduler holds it at 2 and anything above is an excursion to be
 * counted, not measured. Results is nowhere near its gate yet, so its tail is
 * the part that matters.
 *
 * NOTE the sample is taken once per Results iteration, which is the scene's own
 * presented frame -- this loop has no separate logic/present split to confuse
 * it, unlike the battle path. */
#define NDS_VSRESULTS_INTERVAL_BUCKETS 16u
volatile u32 gNdsVSResultsPresentIntervalBucket[NDS_VSRESULTS_INTERVAL_BUCKETS];
volatile u32 gNdsVSResultsPresentIntervalMax;
volatile u32 gNdsVSResultsPresentIntervalSamples;
static u32 sNdsVSResultsLastPresentVBlank;

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);
extern void ndsDevSceneHarnessApply(void);
extern u16 ndsControllerLiveButtons(void);

void ndsMNVSResultsManagerFuncUpdate(SYTaskmanSetup *setup);
void ndsMNVSResultsSetupFilesKind(s32 fkind);
void ndsMNVSResultsSetLoadScene(void);

#define mnVSResultsStartScene ndsBaseMNVSResultsStartScene
#define scManagerFuncUpdate ndsMNVSResultsManagerFuncUpdate
#define syTaskmanSetLoadScene ndsMNVSResultsSetLoadScene
#define ftManagerSetupFilesAllKind ndsMNVSResultsSetupFilesKind

void ndsBaseMNVSResultsStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c"

#undef mnVSResultsStartScene
#undef scManagerFuncUpdate
#undef ftManagerSetupFilesAllKind
#undef syTaskmanSetLoadScene

/* Owner requirement, switch plan R2-07: "Pressing start in Results screen should
 * restart match (P1 specific)".
 *
 * `mnVSResultsFuncRun` polls `mnVSResultsCheckExit`, which returns TRUE on a
 * START tap once `sMNVSResultsTotalTimeTics >= sMNVSResultsAllowExitWait` (410
 * ticks for a normal result, 370 for a tie, 200 for No Contest -- mnvsresults.c
 * :2820-2835), then picks a destination and calls `syTaskmanSetLoadScene`. Both
 * of its destinations are menus this milestone does not have: `nSCKindPlayersVS`
 * normally, or `nSCKindMessage` when an unlock fires. There is exactly ONE call
 * site (mnvsresults.c:3316), so redefining the symbol over the included source
 * is unambiguous, and it keeps `decomp/` read-only -- the same mechanism this
 * translation unit already uses for `scManagerFuncUpdate` and
 * `ftManagerSetupFilesAllKind`.
 *
 * Re-seeding through `ndsDevSceneHarnessApply` rather than hand-clearing state:
 * it restores `dSCManagerDefaultBattleState` into `gSCManagerTransferBattleState`
 * and re-declares the whole canonical configuration, so a rematch starts from
 * byte-identical state to the boot match instead of from whatever the finished
 * match left behind (damage, stocks, scores, time_remain). It writes only
 * globals -- no allocation, no scene teardown -- so it is safe to re-run here.
 * It seeds `dSCManagerDefaultSceneData`, not the live scene, which is why
 * `gSCManagerSceneData` is set explicitly afterwards.
 *
 * `scene_prev` is `nSCKindMaps` to match what the harness declares for a fresh
 * boot, not `nSCKindVSResults`: the battle path is entered exactly as it was for
 * match one, so nothing downstream can distinguish a rematch from a cold start.
 *
 * REBOOT ALTERNATIVE TRIED AND MEASURED DEAD (2026-07-30). The owner asked for
 * the cheap way out of the second-entry geometry corruption: "can't you just
 * reload the game itself for now?" It is the right instinct -- this ROM boots
 * straight into the canonical match, so a cold restart IS the rematch, correct
 * by construction. It does not work on this toolchain, and the reason is
 * specific rather than a tuning problem:
 *
 *   - libnds' `swiSoftReset` does not exist under calico. Calico declares
 *     `svcSoftReset` but links no ARM9 implementation, and its header cannot be
 *     included alongside libnds' `<nds/bios.h>` (three redefinitions).
 *   - Issuing BIOS SWI 0x00 by hand builds and runs, and does nothing. Measured:
 *     a 3.5-minute soak pressing START on Results left `gNdsVSResultsRematchCount`
 *     at 1. A real ARM9 restart re-zeroes .bss, so that counter would read 0.
 *     The program fell through into the guard spin instead, display already off.
 *     (The soak's NO-FREEZE verdict was not evidence of survival -- only ~4 of
 *     the 8 identical-frame polls it needs fit between the press and the end of
 *     the run.)
 *   - Calico's real ARM9 entry is `crt0Startup(r0, r1, r2)`: it copies 40 bytes
 *     from r0 and branches on r1. Those are loader-supplied boot arguments,
 *     gone by the time Results runs, so it cannot simply be called.
 *
 * Reviving this needs the boot arguments captured at first entry, which is a
 * crt0 change, not a scene change. Until then the redirect stays: visually
 * wrong on the second entry, but it restarts the match and it runs at 28.9 FPS.
 * Do not retry bare `swi #0` -- it is measured, not suspected. */
void ndsMNVSResultsSetLoadScene(void)
{
#if NDS_DEMO_FOX_CPU_LADDER
    /* THE DEMO LADDER (owner, 2026-08-17): Fox opens at level 1 and gains a
     * level every time Mario wins and START starts the next match, wrapping
     * 9 -> 1. A Mario loss repeats the level rather than punishing twice.
     *
     * BEFORE ndsDevSceneHarnessApply(), not after: that call re-seeds
     * gSCManagerTransferBattleState from the harness defaults, and the harness
     * reads this variable. Advancing afterwards would show the new level on the
     * HUD one match late.
     *
     * PLACE 0 IS THE WINNER, NOT 1. sMNVSResultsPlaces is 0-based: in a
     * two-player match decomp mnvsresults.c:901-909 sets the WIN status on
     * case 0 and the LOSE status on case 1, and :422/:445/:494 all test == 0
     * for the winner. The on-screen "Place 1" is a display value, and the
     * `place == 1` in the winner-sprite helper at :1558 is a different,
     * display-side parameter -- reading either of those instead of the array's
     * own writers is what made the first version of this line test for SECOND
     * place, so it advanced on a loss and never on a win (owner, 2026-08-17:
     * "it does not increment").
     *
     * Index 0 is Mario, populated every Results frame by
     * ndsMNVSResultsRecordFrame from sMNVSResultsPlaces and cleared at scene
     * start -- so it is final and valid at the moment START is pressed. */
    if (gNdsVSResultsFighterPlace[0] == 0u)
    {
        gNdsDemoFoxCpuLevel = (gNdsDemoFoxCpuLevel >= 9u) ?
            1u : (gNdsDemoFoxCpuLevel + 1u);
    }
#endif
#if NDS_P2_MENU_SHELL
    /* P2-1f: THE SOURCE'S OWN DESTINATION, once there is a menu to return to.
     * `mnVSResultsFuncRun`'s exit block sets `scene_curr = nSCKindPlayersVS`
     * (mnvsresults.c:3312) whenever no unlock message is pending -- the
     * character select, not a rematch and not the VS menu. Everything above
     * this line existed because this ROM booted straight into one match and
     * had nowhere else to send the player.
     *
     * ndsDevSceneHarnessApply() IS DELIBERATELY NOT CALLED HERE. It re-runs
     * `ndsMatchConfigLoadMarioFoxDreamLand` and overwrites the whole
     * descriptor from the mode-163 preset (scene_harness.c:560,189), which
     * would throw away the fighters the character select just chose and the
     * stage the stage select just chose -- the defect P2-1e handed forward
     * against the day Results routed anywhere the descriptor survives. It is
     * not needed either: its job is to hand match two a clean base state, and
     * the character select's own commit does exactly that through
     * `ndsMatchConfigApply`, whose first act is
     * `gSCManagerTransferBattleState = dSCManagerDefaultBattleState` -- the
     * pre-match snapshot, taken by that same function's tail and written by
     * nothing else in this build.
     *
     * The demo ladder above still runs: it is a build flag, it advances
     * `gNdsDemoFoxCpuLevel` only, and the descriptor is what the character
     * select re-reads on entry. */
    /* P2-1k (g), and the probe is what found it: `mnVSResultsFuncRun`'s exit
     * block calls `syAudioStopBGMAll()` immediately before
     * `syTaskmanSetLoadScene()` (mnvsresults.c:3315), and this branch did not.
     * The measured symptom was `MSBGM playing=1 track=22` still true at the
     * character select's own entry stop, so the Results theme played on under
     * the CSS until `ndsMenuShellCssPlayBgm` started BGM 10 over the top of it
     * instead of after it. The source's order is stop, then the next scene's
     * own start. */
    syAudioStopBGMAll();
    gNdsVSResultsRematchCount++;
    /* The source already selected PlayersVS or Message above. Preserve the
     * latter so earned Item Switch/Inishie unlocks reach mnMessageApplyUnlock. */
#if NDS_P2_1P_GAME
    ndsSceneManagerRequest((u32)gSCManagerSceneData.scene_curr,
                           (u32)nSCKindVSResults);
#else
    /* The flag-off configuration has no Message scene yet. */
    ndsSceneManagerRequest((u32)nSCKindPlayersVS, (u32)nSCKindVSResults);
#endif
#else
    ndsDevSceneHarnessApply();
    /* P2-1b: the destination goes through the scene registry rather than being
     * two assignments and a taskman call written out here. Same bytes into the
     * same two fields -- `nSCKindMaps` as scene_prev for the reason above -- but
     * a kind this build does not have is now refused and counted instead of
     * handed to scManagerRunLoop's switch, and the hop lands in the transition
     * ring so the flow is read rather than inferred. */
    gNdsVSResultsRematchCount++;
    ndsSceneManagerRequest((u32)nSCKindVSBattle, (u32)nSCKindMaps);
#endif
}

static void (*sNdsMNVSResultsFuncStart)(void);

/* Claimed here because this is the only seam that brackets mnVSResultsFuncStart,
 * which calls efParticleInitAll (mnvsresults.c:3339) and would otherwise take
 * the battle's sizing. Pointless without the confetti rate raise in
 * battleship_lbparticle.c -- measured: with the source rate, the bigger pool
 * went entirely unused. Cleared after, so no other scene inherits it. */
extern volatile u32 gNdsParticlePoolStructsWanted;
extern volatile u32 gNdsParticlePoolGeneratorsWanted;
extern volatile u32 gNdsParticlePoolTransformsWanted;

static void ndsMNVSResultsFuncStartTimed(void)
{
    u32 start = cpuGetTiming();

    /* 112/24/16 SATURATED -- the 2026-08-02 soak read StructsLive 112 of
     * StructsMax 112, which is why the owner still saw confetti that did not
     * cover the scene. Raised with NDS_R2_CONFETTI_UPDATE_RATE, never alone.
     * Memory is not the constraint here (45,888 bytes against ~300 KB free);
     * the Results present interval is, and the owner has taken that trade
     * explicitly -- see NDS_R2_CONFETTI_UPDATE_RATE for the measured table. */
    /* 384/48/24 UNTIL 2026-08-03, and the block above is why it grew. It is
     * back to the source's own sizing with the rate raise it existed to feed
     * (see NDS_R2_CONFETTI_UPDATE_RATE in battleship_lbparticle.c): the two
     * always had to move together, so neutralizing one and leaving the other
     * would just be a 384-struct pool sitting unused with 0 rejects, which is
     * exactly the state the first raise was made to escape. The override stays
     * as a named seam because the Results scene genuinely is the one scene that
     * may want different sizing from the battle. */
    gNdsParticlePoolStructsWanted = 0u;
    gNdsParticlePoolGeneratorsWanted = 0u;
    gNdsParticlePoolTransformsWanted = 0u;
    if (sNdsMNVSResultsFuncStart != NULL)
    {
        sNdsMNVSResultsFuncStart();
    }
#if NDS_R2_PARTICLE_DRAW
    /* BattleShip creates the two source confetti generators at Results tic 120
     * (mnvsresults.c:3248-3251), but unlike VSBattle this scene had no DS
     * particle-atlas residency seam.  The particle interpreter therefore ran
     * correctly while ndsRendererHardwareParticleAtlasName() stayed zero and
     * every Results particle failed closed before atlas lookup.  Prepare the
     * same generated DS atlas once after the source scene has finished all
     * setup and before its first frame.  No source particle state, rate, size,
     * position, colour or bytecode is changed; this is only the DS equivalent
     * of making the source texture bank resident before it is drawn. */
    (void)ndsRendererHardwarePrepareParticleAtlas();
#endif
    gNdsParticlePoolStructsWanted = 0u;
    gNdsParticlePoolGeneratorsWanted = 0u;
    gNdsParticlePoolTransformsWanted = 0u;
    gNdsVSResultsFuncStartTicks = cpuGetTiming() - start;
}

void ndsMNVSResultsManagerFuncUpdate(SYTaskmanSetup *setup)
{
    SYTaskmanSetup ds_setup = *setup;

    ds_setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    ds_setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    /* R2-07 R1 measurement. The setup struct is already copied here, so the task
     * start function can be timed by substitution -- no seam in `decomp/`, and
     * the scene still runs the original. `mnVSResultsFuncStart` is where both
     * halves of the hand-off live (mnvsresults.c:3336 loads the scene file list,
     * :3343 loops the fighter kinds), so bracketing it bounds the whole load. */
    sNdsMNVSResultsFuncStart = ds_setup.func_start;
    ds_setup.func_start = ndsMNVSResultsFuncStartTimed;
    gNdsVSResultsFuncStartTicks = 0u;
    gNdsVSResultsSetupFilesTicks = 0u;
    gNdsVSResultsSetupFilesCalls = 0u;
    scManagerFuncUpdate(&ds_setup);
}

void ndsMNVSResultsSetupFilesKind(s32 fkind)
{
    /* The source loops every playable kind (mnvsresults.c:3343); only the
     * fighters this build can put in a match are wanted, which is still most
     * kinds of loading the port does not do.
     *
     * P2-3r3 (2026-08-23): this list was Mario/Fox only, so a match with Luigi
     * or Donkey Kong ended in `mnVSResultsMakeFighter` building a fighter whose
     * files were never loaded -- the ARM9 wandered to pc=0xfffffffc on the
     * FIRST Results entry, which the board had filed as a battle-setup crash.
     * `scripts/probe-trace-symbols.ps1` placed it: 213 status changes of a
     * complete match, then ftManagerAllocFighter from
     * ndsMNVSResultsSetupFilesKind's caller, then the wander. The rule is now
     * the kinds that were in the match, read from the transfer state the
     * Results scene itself is built from, so every admitted fighter -- and
     * every future one -- loads exactly when it is on the podium. */
    s32 i;

    for (i = 0; i < (s32)ARRAY_COUNT(gSCManagerTransferBattleState.players); i++)
    {
        if ((gSCManagerTransferBattleState.players[i].pkind !=
             nFTPlayerKindNot) &&
            (gSCManagerTransferBattleState.players[i].fkind == fkind))
        {
            u32 start = cpuGetTiming();

            ftManagerSetupFilesAllKind(fkind);
            gNdsVSResultsSetupFilesTicks += cpuGetTiming() - start;
            gNdsVSResultsSetupFilesCalls++;
            return;
        }
    }
}

/* The port never wires the real keypad into `gSYControllerDevices` for imported
 * scenes: each menu synthesises the single button it needs -- `mnplayersvs.c:341`
 * injects START, `mnmaps.c:256` injects A. Results was left with no injection at
 * all, so `mnVSResultsCheckExit` (decomp mnvsresults.c:266) polled a permanently
 * zero `button_tap` and the scene could never be left. Measured: a real 500 ms
 * held START at Results tic ~1,000 left `gNdsVSResultsRematchCount` at 0 with the
 * scene still ticking, which is what sent this hunt to the input layer rather
 * than to the redirect.
 *
 * Publish the live pad instead of synthesising one, so the owner's START reaches
 * the source's own exit test unmodified. Only device 0 is written -- claiming
 * four controllers pressed START would satisfy the test by lying about hardware
 * that is not there. `button_tap` is the rising edge, which is exactly what
 * `mnVSResultsCheckExit` samples. */
static void ndsMNVSResultsObserveInput(void)
{
    /* OBSERVE ONLY -- deliberately no writes to `gSYControllerDevices`.
     *
     * An earlier attempt published a synthetic pad from here and it could not
     * work, for a reason worth keeping: `taskman_seam.c:6987-6995` runs
     * `syControllerReadDeviceData` + `syControllerUpdateGlobalData` BEFORE
     * `task_update` (which is `gcRunAll` -> `mnVSResultsFuncRun` ->
     * `mnVSResultsCheckExit`), and calls this function AFTER it. So anything
     * written here lands after the only reader has run and is overwritten by the
     * source pipeline before the next one. The source's controller path already
     * runs every update -- `NDS_HARNESS_FAST_LOGIC` is 0 in every configuration
     * -- so the fix belongs in that path, not in a second one bolted alongside.
     *
     * These three masks are sticky (a held button is long gone by the time a
     * soak reads globals) and split the failure three ways:
     *   PadMask  0x1000, SeenMask 0        -> the source pipeline is not seeing
     *                                         the keypad the port can already read
     *   SeenMask 0x1000, TapMask  0        -> hold arrives, edge is lost
     *   both 0x1000, RematchCount 0        -> the exit test itself is refusing */
    gNdsVSResultsInputPollCount++;
    gNdsVSResultsPadMask |= ndsControllerLiveButtons();
    gNdsVSResultsInputSeenMask |= gSYControllerDevices[0].button_hold;
    gNdsVSResultsInputTapMask |= gSYControllerDevices[0].button_tap;
}

void ndsMNVSResultsRecordFrame(void)
{
    u32 file_count = 0;
    u32 fighter_count = 0;
    u32 i;

    /* R2-07 R0h. The profiler's window is driven from here in
     * NDS_TASK37_PROFILE_RESULTS builds and compiles to nothing everywhere else.
     * The battle site keys off presented frames, which this loop never
     * increments, so a battle-keyed window would open and dump during the match
     * and describe nothing about this scene. Costs one compare per Results
     * iteration in profile builds only. */
    NDS_TASK37_PROFILE_RESULTS_TICK(sMNVSResultsTotalTimeTics);

    /* Sample this scene's presented-frame cadence. The first iteration only
     * seeds the reference -- there is no previous frame to difference against,
     * and counting that gap would report the whole scene-entry load as one
     * enormous interval. */
    {
        u32 now = ndsPlatformVBlankCount();

        if (sNdsVSResultsLastPresentVBlank != 0u)
        {
            u32 interval = now - sNdsVSResultsLastPresentVBlank;
            u32 bucket = (interval < (NDS_VSRESULTS_INTERVAL_BUCKETS - 1u))
                ? interval
                : (NDS_VSRESULTS_INTERVAL_BUCKETS - 1u);

            gNdsVSResultsPresentIntervalBucket[bucket]++;
            gNdsVSResultsPresentIntervalSamples++;
            if (interval > gNdsVSResultsPresentIntervalMax)
            {
                gNdsVSResultsPresentIntervalMax = interval;
            }
        }
        sNdsVSResultsLastPresentVBlank = now;
    }

    ndsMNVSResultsObserveInput();

    /* Close the transition bracket on the first tick only. The start stamp is
     * taken at battle taskman exit, so this is the whole hand-off as the player
     * experiences it: last battle frame on screen to first Results frame. */
    if ((gNdsVSResultsTransitionTicks == 0u) &&
        (gNdsVSResultsTransitionStartTick != 0u))
    {
        gNdsVSResultsTransitionTicks =
            cpuGetTiming() - gNdsVSResultsTransitionStartTick;
        gNdsVSResultsFirstTickStamp = cpuGetTiming();
        /* Scope the controller edge counters to this scene. A soak presses START
         * on a wall-clock schedule, so some presses land while the battle is
         * still running and a run-global mask reports those as success. */
        ndsControllerEdgeTelemetryReset();
    }
    if (gNdsVSResultsFirstTickStamp != 0u)
    {
        u32 elapsed = cpuGetTiming() - gNdsVSResultsFirstTickStamp;

        if ((gNdsVSResultsToWallpaperTicks == 0u) &&
            (sMNVSResultsTotalTimeTics >= sMNVSResultsDrawWallpaperTic))
        {
            gNdsVSResultsToWallpaperTicks = elapsed;
        }
        if ((gNdsVSResultsToResultsTicks == 0u) &&
            (sMNVSResultsTotalTimeTics >= sMNVSResultsMakeResultsTic))
        {
            gNdsVSResultsToResultsTicks = elapsed;
        }
    }

    for (i = 0; i < ARRAY_COUNT(sMNVSResultsFiles); i++)
    {
        if (sMNVSResultsFiles[i] != NULL)
        {
            file_count++;
        }
    }
    for (i = 0; i < ARRAY_COUNT(sMNVSResultsFighterGObjs); i++)
    {
        if (sMNVSResultsFighterGObjs[i] != NULL)
        {
            FTStruct *fp = ftGetStruct(sMNVSResultsFighterGObjs[i]);

            fighter_count++;
            /* BattleShip ftdef.h uses fighter DL link 9, which is one of the
             * source Results fighter camera's captured links. */
            if ((fp != NULL) && (fp->dl_link != 9))
            {
                ftParamMoveDLLink(sMNVSResultsFighterGObjs[i], 9);
            }
            if (fp != NULL)
            {
                /* Results is N-player in BattleShip: all four entries of
                 * sMNVSResultsFighterGObjs are source-owned and the source
                 * fighter camera renders all present entries. Register every
                 * live Results fighter with the DS renderer and publish the
                 * same four entries for later P2-2 verification. */
                ndsFighterManagerRegisterDisplayFighter(
                    sMNVSResultsFighterGObjs[i], i);
                gNdsVSResultsFighterPlace[i] =
                    (u32)sMNVSResultsPlaces[i];
                gNdsVSResultsFighterStatus[i] = fp->status_id;
                gNdsVSResultsFighterMotion[i] = fp->motion_id;
            }
        }
    }

    gNdsVSResultsTickCount = (u32)sMNVSResultsTotalTimeTics;
    gNdsVSResultsLoadedFileCount = file_count;
    gNdsVSResultsFighterCount = fighter_count;
    gNdsVSResultsGObjCount = sGCCommonsActiveNum;
    gNdsVSResultsSObjCount = sGCSpritesActiveNum;
    gNdsVSResultsKind = (u32)sMNVSResultsKind;
    if (file_count == ARRAY_COUNT(sMNVSResultsFiles))
    {
        gNdsVSResultsMask |= 1u << 0;
    }
    if ((u32)sMNVSResultsTotalTimeTics >=
        (u32)sMNVSResultsDrawWallpaperTic)
    {
        gNdsVSResultsMask |= 1u << 1;
    }
    if ((u32)sMNVSResultsTotalTimeTics >=
        (u32)sMNVSResultsMakeResultsTic)
    {
        gNdsVSResultsMask |= 1u << 2;
    }
    if (fighter_count >= 2u)
    {
        gNdsVSResultsMask |= 1u << 3;
    }
    if (sGCSpritesActiveNum != 0u)
    {
        gNdsVSResultsMask |= 1u << 4;
    }
    if ((gNdsVSResultsMask & 0x1fu) == 0x1fu)
    {
        gNdsVSResultsResult = NDS_VS_RESULTS_PASS;
    }
}

void mnVSResultsStartScene(void)
{
    u32 i;

    gNdsVSResultsResult = 0;
    gNdsVSResultsMask = 0;
    gNdsVSResultsTickCount = 0;
    gNdsVSResultsLoadedFileCount = 0;
    gNdsVSResultsFighterCount = 0;
    gNdsVSResultsGObjCount = 0;
    gNdsVSResultsSObjCount = 0;
    gNdsVSResultsKind = 0;
    gNdsVSResultsCameraProcCount = 0;
    gNdsVSResultsFighterDisplayCount = 0;
    gNdsVSResultsFighterSubmitCount = 0;
    memset((void *)gNdsVSResultsFighterPlace, 0,
           sizeof(gNdsVSResultsFighterPlace));
    memset((void *)gNdsVSResultsFighterStatus, 0,
           sizeof(gNdsVSResultsFighterStatus));
    memset((void *)gNdsVSResultsFighterMotion, 0,
           sizeof(gNdsVSResultsFighterMotion));
    /* AND THE ONE NON-TELEMETRY LINE: drop the previous entry's fighter GObjs.
     *
     * This is the second-entry crash the owner reported as "2nd match froze at
     * 00:00, no results screen" (2026-07-31). `sMNVSResultsFighterGObjs` is
     * populated by `ftManagerMakeFighter` during the scene (decomp
     * mnvsresults.c:991) and is never cleared at teardown, while the taskman
     * arena IS rewound between scenes -- so on a second entry it holds four
     * pointers into memory the new scene has already re-allocated. The source
     * survives that because every one of its own reads is gated on
     * `sMNVSResultsIsPresent` (mnvsresults.c:1870, 2788), not on the pointer
     * being non-NULL. `ndsMNVSResultsRecordFrame` above gates on `!= NULL`, so on
     * the second entry's FIRST tick it called `ftGetStruct` on a dead GObj and
     * handed the result to `gcMoveGObjDL`, which walked a corrupt display list.
     *
     * Captured shape: ARM9 in ABORT mode (`cpsr 0x400000b7`) with
     * `lr_abt = 0xe9b` -- a wild branch to low memory -- and `lr_usr` at
     * `ftParamMoveDLLink+18`, which is exactly the return address of that
     * `gcMoveGObjDL` call. `gNdsVSResultsTickCount` still 0 because the crash
     * beat the write at the end of the same function.
     *
     * Clearing here is safe and source-faithful: this runs before
     * `mnVSResultsFuncStart`, and the source recreates every fighter it wants.
     * `sMNVSResultsFiles` deliberately stays untouched -- it is reloaded in
     * func_start (mnvsresults.c:3336) before any tick, and it is only ever
     * NULL-tested here, never dereferenced.
     *
     * Third instance of one law, after the prepared-run cache and the
     * display-list heads: state that outlives a scene boundary must be
     * re-derived from something the boundary moves, never trusted because it
     * still looks like a pointer. */
    /* The source recreates its full four-entry Results fighter array on every
     * scene entry.  Mirror that lifetime in the DS live registry before the
     * arena is reused.  Without this, a 4-way Results -> 2-way rematch could
     * leave slots 2/3 pointing into the previous Results arena, and the now
     * four-slot battle renderer would treat them as current fighters. */
    for (i = 0u; i < GMCOMMON_PLAYERS_MAX; i++)
    {
        ndsFighterManagerRegisterDisplayFighter(NULL, i);
    }
    memset(sMNVSResultsFighterGObjs, 0, sizeof(sMNVSResultsFighterGObjs));
    gNdsVSResultsStartCount++;
    ndsBaseMNVSResultsStartScene();
}

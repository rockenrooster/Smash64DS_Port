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
volatile u32 gNdsVSResultsFighterPlace[2];
volatile u32 gNdsVSResultsFighterStatus[2];
volatile s32 gNdsVSResultsFighterMotion[2];
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
volatile u32 gNdsVSResultsInputPollCount;
volatile u32 gNdsVSResultsInputSeenMask;
volatile u32 gNdsVSResultsInputTapMask;
volatile u32 gNdsVSResultsPadMask;

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
 * match one, so nothing downstream can distinguish a rematch from a cold start. */
void ndsMNVSResultsSetLoadScene(void)
{
    ndsDevSceneHarnessApply();
    gSCManagerSceneData.scene_prev = nSCKindMaps;
    gSCManagerSceneData.scene_curr = nSCKindVSBattle;
    gNdsVSResultsRematchCount++;
    syTaskmanSetLoadScene();
}

static void (*sNdsMNVSResultsFuncStart)(void);

static void ndsMNVSResultsFuncStartTimed(void)
{
    u32 start = cpuGetTiming();

    if (sNdsMNVSResultsFuncStart != NULL)
    {
        sNdsMNVSResultsFuncStart();
    }
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
    /* The source loops every playable kind (mnvsresults.c:3343); only the two
     * fighters this milestone builds are wanted, which is already ten kinds of
     * loading the port does not do. */
    if ((fkind == nFTKindMario) || (fkind == nFTKindFox))
    {
        u32 start = cpuGetTiming();

        ftManagerSetupFilesAllKind(fkind);
        gNdsVSResultsSetupFilesTicks += cpuGetTiming() - start;
        gNdsVSResultsSetupFilesCalls++;
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
            if ((i < 2u) && (fp != NULL))
            {
                gNdsVSResultsFighterPlace[i] =
                    (u32)sMNVSResultsPlaces[i];
                gNdsVSResultsFighterStatus[i] = fp->status_id;
                gNdsVSResultsFighterMotion[i] = fp->motion_id;
                ndsFighterManagerRegisterDisplayFighter(
                    sMNVSResultsFighterGObjs[i], i);
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
    gNdsVSResultsStartCount++;
    ndsBaseMNVSResultsStartScene();
}

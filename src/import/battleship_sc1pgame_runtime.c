/* P2 campaign runtime: import the source setup, spawn, waves and scoring.
 * The manager owns the persistent 1P battle state, including remaining stocks
 * and ally selections. The DS entry below preserves that state and boots the
 * original sc1PGameFuncStart, which calls sc1PGameSetupStageAll once. Platform
 * differences are confined to scene routing, video and taskman setup.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <ssb_types.h>
#include <ft/fighter.h>
#include <ft/ftcomputer.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <mn/menu.h>
#include <sc/scene.h>
#include <sys/dma.h>
#include <sys/video.h>
#include <reloc_data.h>

/* Same local extern as every other import TU; no port header publishes it. */
s32 syUtilsRandIntRange(s32 range);
/* The fight boot below (sc1pgame.c:2898-2920 on DS): the 3D layer reclaim
 * (nds_platform.h), the DS taskman arena (diagnostics_taskman_heap.c), the
 * battle re-budget (battleship_scvsbattle.c) and the rumble init the source
 * tail calls (gm/gmrumble.c, imported in battleship_ifcommon.c). */
#include <nds/nds_platform.h>
extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);
extern void ndsBattleRebudgetSceneSetup(SYTaskmanSetup *setup);
extern void gmRumbleInitPlayers(void);

/* decomp ft/ftdef.h:5 verbatim (port include/ft/fighter.h lacks it; the
 * included TU uses it at sc1pgame.c:1385 setup path). */
#ifndef FTCOMMON_HANDICAP_DEFAULT
#define FTCOMMON_HANDICAP_DEFAULT 9
#endif

/* The source task setup references the overlay end; the DS entry replaces
 * its arena with the shared taskman arena before starting the scene. */
extern uintptr_t ovl65_BSS_END;

/* Owned by the step-1 pair battleship_sc1pmanager.c (originals there). */
extern s32 gSC1PManagerLevelDrop;
extern u8 gSC1PManagerKirbyTeamFinalCopy;

/* Keep the source runtime and replace only its platform entry. */
#define sc1PGameStartScene ndsExcludedSC1PGameStartScene

/* sc1pgame.c:1771 seeds each enemy-team stock SObj with the sprite at offset
 * llStagePupupuFile2FileID inside gGMCommonFiles[4] (IFCommonDigits): the
 * upstream symbol is a mis-named link constant (StagePupupuFile2 is file
 * 0x68, a file id, not an offset), and 0x68 inside IFCommonDigits IS
 * llIFCommonDigits0Sprite (tools/reloc_data_symbols.us.txt:2966). The
 * display proc (:1690-1707) replaces the sprite per stage before anything is
 * drawn, so the seed only has to be a valid Sprite; the port names the same
 * bytes the ROM read. */
#define llStagePupupuFile2FileID llIFCommonDigits0Sprite
/* The overlay copy of the source (scripts/import-overlays/battleship/
 * src_sc_sc1pmode_sc1pgame.patch): identical except that the N64
 * title-signature check in sc1PGameFuncStart -- a DMA read of the cartridge
 * header plus a CALL into relocData file 200's MIPS code -- is compiled out
 * under SSB64_TARGET_NDS, since no include-side seam can skip a call through
 * a data pointer inside the function. */
#include <battleship_overlay/src/sc/sc1pmode/sc1pgame.c>

#undef sc1PGameStartScene

/* Bridge diagnostics: what was asked, what booted, what was refused and why.
 * RefusedStage is indexed by spgame_stage (CommonStart..ChallengerEnd). */
volatile u32 gNdsSC1PGameBridgeStageRequested;
volatile u32 gNdsSC1PGameBridgeAppliedCount;
volatile u32 gNdsSC1PGameBridgeRefusedCount;
volatile u32 gNdsSC1PGameBridgeRefusedStage[nSC1PGameStageChallengerEnd + 1];

static void ndsSC1PGameBridgeRefuse(u8 stage)
{
    gNdsSC1PGameBridgeRefusedCount++;
    if (stage <= (u8)nSC1PGameStageChallengerEnd)
    {
        gNdsSC1PGameBridgeRefusedStage[stage]++;
    }
}

/* Preserve the manager's live state; the imported scene owns stage setup. */
void sc1PGameStartScene(void)
{
    u8 stage = gSCManagerSceneData.spgame_stage;
    SC1PGameStage *stagesetup;
    s32 i;

    gNdsSC1PGameBridgeStageRequested = stage;

    if (stage > (u8)nSC1PGameStageChallengerEnd)
    {
        ndsSC1PGameBridgeRefuse(stage);
        return;
    }
    if (stage == (u8)nSC1PGameStageBoss)
    {
        ndsSC1PGameBridgeRefuse(stage);
        return;
    }
    stagesetup = &dSC1PGameStageDesc[stage];

    /* Every ladder venue has a wired native packet since P2-4n1 (the five 1P
     * arenas PupupuSmall, YosterSmall, Metal, Zako and Last beside the eight
     * VS stages) and, since 2026-09-05, the 25 bonus boards and Race. The
     * boss stage is refused above until Master Hand's owner export lands.
     * Variant admission (file doc): Giant DK and Metal Mario are
     * admitted; the polygon kinds and Boss wait on their admissions. Base
     * kinds ride their NDS_P2_* build flags. */
    for (i = 0; i < 2; i++)
    {
        if ((stagesetup->fkind[i] == (u8)nFTKindBoss) ||
            ((stagesetup->fkind[i] >= (u8)nFTKindNStart) &&
             (stagesetup->fkind[i] <= (u8)nFTKindNEnd)))
        {
            ndsSC1PGameBridgeRefuse(stage);
            return;
        }
    }

    gSCManagerBattleState = &gSCManager1PGameBattleState;
    gSCManagerBattleState->game_type = nSCBattleGameType1PGame;

    gNdsSC1PGameBridgeAppliedCount++;

    /* THE FIGHT ITSELF (sc1pgame.c:2898-2920), once the stage is
     * admitted. The source boots the battle task with dSC1PGameTaskmanSetup
     * and sc1PGameFuncStart through scManagerFuncUpdate and, when the task
     * returns, resets the bonus tallies, silences the BGM and re-arms the
     * rumble players. The DS differences are the ones every battle scene
     * carries: the 3D layer is reclaimed from the native menus
     * (battleship_scvsbattle.c), the setup is re-budgeted to the DS display
     * list, graphics heap and RDP sizes (ndsBattleRebudgetSceneSetup, the
     * same numbers the VS match uses; the N64 arena words are overridden at
     * syTaskmanStartTask for every registered kind), and the scene is marked
     * nSCKind1PGame for the seam: sc1pmanager.c:364-385 and :512-523 leave
     * scene_curr at the intro or the challenger when they call this, and the
     * seam routes the battle runner by scene_curr (taskman_seam_harness.c).
     * The manager writes the next scene_curr itself when the task returns
     * (:399, :469), so nothing is restored here. */
    ndsPlatformSet3DLayerEnabled(TRUE);
    gSCManagerSceneData.scene_prev = gSCManagerSceneData.scene_curr;
    gSCManagerSceneData.scene_curr = (u8)nSCKind1PGame;

    dSC1PGameVideoSetup.zbuffer = SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dSC1PGameVideoSetup);

    ndsBattleRebudgetSceneSetup(&dSC1PGameTaskmanSetup);
    dSC1PGameTaskmanSetup.func_start = sc1PGameFuncStart;
    {
        SYTaskmanSetup setup = dSC1PGameTaskmanSetup;

        setup.scene_setup.arena_start = ndsTaskmanArenaStart();
        setup.scene_setup.arena_size = ndsTaskmanArenaSize();
        scManagerFuncUpdate(&setup);
    }

    sc1PGameInitBonusStats();
    syAudioStopBGMAll();
    /* sc1pgame.c:2913-2916 spins until the BGM player reports stopped; the
     * DS mixer clears its playing flag inside StopAll (nds_audio_bgm.c), so
     * the spin exits at once and cannot outlive a stopped stream. */
    while (syAudioCheckBGMPlaying(0) != FALSE)
    {
        continue;
    }
    syAudioSetBGMVolume(0, 0x7800);
    func_800266A0_272A0();
    gmRumbleInitPlayers();
}

#endif /* NDS_P2_1P_GAME */

/* Compile the original BattleShip PlayersVS scene translation unit.
 *
 * The DS entry remains bounded: it runs original character-select setup,
 * records proof of the original object/file graph, and parks before the
 * continuous interactive loop unless a harness explicitly drives the original
 * ready/start transition.
 */
#include "nds_build_config.h"

#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <mn/menu.h>
#include <nds/nds_audio_bgm.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_renderer.h>
#include <nds/generated/nds_native_fighter_image.generated.h>
#include <nds/nds_menu_shell.h>
#include <nds/nds_platform.h>
#include <nds/nds_startup.h>
#include <sc/scene.h>
#include <sys/controller.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);
extern s32 gcGetGObjsActiveNum(void);
extern u32 sGCCamerasActiveNum;
extern u32 sGCSpritesActiveNum;
extern s32 sSYTaskmanStatus;
extern sb32 (*dLBCommonFuncMatrixList[])(void);
extern void efManagerInitEffects(void);
extern void ndsFighterManagerRegisterDisplayFighter(GObj *fighter_gobj,
                                                     u32 slot);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
extern void ndsFighterRendererInvalidateMaterialCachesForSlot(u32 slot);
#endif

extern void mnPlayersVSFuncLights(Gfx **dls);
void mnPlayersVSFuncStart(void);
void mnPlayersVSFuncRun(GObj *gobj);
s32 mnPlayersVSRandFighterKind(GObj *gobj);
sb32 mnPlayersVSCheckReady(void);
void mnPlayersVSSetSceneData(void);
void mnPlayersVSPauseSlotProcesses(void);
s32 mnPlayersVSGetNextTimeValue(s32 current_value);
s32 mnPlayersVSGetPrevTimeValue(s32 current_value);
sb32 mnPlayersVSCheckCostumeUsed(s32 fkind, s32 player, s32 costume);
s32 mnPlayersVSUpdateCursorPlacementPriorities(s32 player, s32 puck);
void mnPlayersVSUpdateCursor(GObj *gobj, s32 player, s32 cursor_status);
void mnPlayersVSAnnounceFighter(s32 player, s32 slot);
void mnPlayersVSMakePortraitFlash(s32 player);
void mnPlayersVSUpdateNameAndEmblem(s32 player);
void mnPlayersVSMakeHandicapLevel(s32 player);
void mnPlayersVSMakeHandicapLevelValue(s32 player);
void mnPlayersVSUpdateHandicapLevel(s32 player);
sb32 mnPlayersVSCheckHandicap(void);
sb32 mnPlayersVSCheckHandicapOn(void);
sb32 mnPlayersVSCheckHandicapAuto(void);
sb32 mnPlayersVSCheckHandicapArrowRInRange(GObj *gobj, s32 player);
sb32 mnPlayersVSCheckHandicapArrowLInRange(GObj *gobj, s32 player);
void mnPlayersVSUpdateCursorGrabPriorities(s32 player, s32 puck);
void mnPlayersVSUpdatePuck(GObj *gobj, s32 puck);
void lbCommonSetSpriteScissor(s32 xmin, s32 xmax, s32 ymin, s32 ymax);

#define mnPlayersVSFuncStart ndsBaseMNPlayersVSFuncStart
#define mnPlayersVSStartScene ndsBaseMNPlayersVSStartScene

void ndsBaseMNPlayersVSFuncStart(void);
void ndsBaseMNPlayersVSStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayersvs.c"

#undef mnPlayersVSFuncStart
#undef mnPlayersVSStartScene

static GObj *sNdsPlayersVSMainGObj;
static sb32 sNdsPlayersVSPreviewActive;
static sb32 sNdsPlayersVSPreviewRulesReady;
/* P2-3r12: the per-kind "this rebuild needs no storage" mask is GONE, not
 * merely unused. It licensed skipping the BGM fence, and the claim was false
 * -- a prepared kind still ran +1,054 payload reads on rebuild. The masks
 * below stay: they are telemetry for what the warming pass achieved, which is
 * a real effect, and probes read them. */
volatile u32 gNdsPlayersVSPreviewResidentPrepareMask;
volatile u32 gNdsPlayersVSPreviewResidentReadyMask;
volatile u32 gNdsPlayersVSPreviewResidentMainFailMask;
volatile u32 gNdsPlayersVSPreviewResidentSubmotionFailMask;
volatile u32 gNdsPlayersVSPreviewResidentAnimFailMask;
volatile u32 gNdsPlayersVSPreviewResidentOwnerFailMask;
/* Genuine source preview rebuilds only. These counters deliberately bracket
 * mnPlayersVSUpdateFighter itself so unrelated CSS/menu NitroFS traffic cannot
 * be mistaken for a rebuild dependency again. */
volatile u32 gNdsPlayersVSPreviewRebuildCount;
volatile u32 gNdsPlayersVSPreviewRebuildPayloadReadCount;
volatile u32 gNdsPlayersVSPreviewRebuildPayloadReadMax;
static u32 sNdsPlayersVSPreviewDrawPhase;
volatile u32 gNdsPlayersVSPreviewFrameCount;
volatile u32 gNdsPlayersVSPreviewDrawCount;
volatile f32 gNdsPlayersVSPreviewRotationY[GMCOMMON_PLAYERS_MAX];
volatile s32 gNdsPlayersVSPreviewStatus[GMCOMMON_PLAYERS_MAX];
volatile s32 gNdsPlayersVSPreviewMotion[GMCOMMON_PLAYERS_MAX];
volatile u32 gNdsPlayersVSPreviewFreeRotateFrames[GMCOMMON_PLAYERS_MAX];
volatile f32 gNdsPlayersVSPreviewLastFreeRotationY[GMCOMMON_PLAYERS_MAX];
volatile s32 gNdsPlayersVSPreviewLastFreeStatus[GMCOMMON_PLAYERS_MAX];
volatile s32 gNdsPlayersVSPreviewLastFreeMotion[GMCOMMON_PLAYERS_MAX];
volatile u32 gNdsPlayersVSPreviewSelectedMask;
volatile u32 gNdsPlayersVSPreviewVisibleMask;
volatile u32 gNdsPlayersVSPreviewExitCount;
volatile u32 gNdsPlayersVSPreviewCostumeChangeCount;
volatile u32 gNdsPlayersVSPreviewSelectedKindMask;
volatile u32
    gNdsPlayersVSPreviewSelectedKindFrames[NDS_MENU_SHELL_FIGHTER_KINDS];
volatile s32
    gNdsPlayersVSPreviewSelectedKindStatus[NDS_MENU_SHELL_FIGHTER_KINDS];
volatile s32
    gNdsPlayersVSPreviewSelectedKindMotion[NDS_MENU_SHELL_FIGHTER_KINDS];

_Static_assert((nFTKindPlayableEnd + 1) == NDS_MENU_SHELL_FIGHTER_KINDS,
               "PlayersVS production telemetry must cover every playable kind");
_Static_assert(nFTKindPlayableEnd < 32,
               "PlayersVS resident-kind mask must cover every playable kind");

static sb32 ndsMNPlayersVSPreviewPrepareResidentKind(s32 fkind)
{
    FTData *data;
    const void *initial_anim_file;
    u32 kind_bit;

    if ((fkind < nFTKindPlayableStart) || (fkind > nFTKindPlayableEnd))
    {
        return FALSE;
    }
    kind_bit = 1u << fkind;
    gNdsPlayersVSPreviewResidentPrepareMask |= kind_bit;
    data = dFTManagerDataFiles[fkind];
    /* This is the source manager's own residency invariant:
     * ftManagerSetupFilesAllKind only tests p_file_main, and when it is absent
     * loads main plus the model/motion/special status-buffer closure together. */
    if ((data == NULL) || (data->p_file_main == NULL) ||
        (*data->p_file_main == NULL))
    {
        gNdsPlayersVSPreviewResidentMainFailMask |= kind_bit;
        return FALSE;
    }

    /* ftManagerMakeFighter gives demo fighters nFTDemoStatusNull before the CSS
     * applies its Selected status. BattleShip maps that to Opening2/submotion 0.
     * Mario/Fox already hit resident animation infrastructure there; P2-3
     * Luigi/Donkey previously read Anim000 synchronously while BGM was live.
     * Warm the exact source submotion-0 token for every admitted kind so this
     * predicate proves fighter creation itself is storage-free. */
    if (data->submotion == NULL)
    {
        gNdsPlayersVSPreviewResidentSubmotionFailMask |= kind_bit;
        return FALSE;
    }
    initial_anim_file = (const void *)(uintptr_t)
        data->submotion->motion_desc[0].anim_file_id;
    if ((initial_anim_file == NULL) ||
        (ndsR2AnimCachePreloadFighterFile(initial_anim_file) == FALSE))
    {
        gNdsPlayersVSPreviewResidentAnimFailMask |= kind_bit;
        return FALSE;
    }

#if NDS_P2_LUIGI
    if (fkind == nFTKindLuigi)
    {
        if ((ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_LUIGI, 0u) == FALSE) ||
            (ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_LUIGI, 1u) == FALSE))
        {
            gNdsPlayersVSPreviewResidentOwnerFailMask |= kind_bit;
            return FALSE;
        }
        gNdsPlayersVSPreviewResidentReadyMask |= kind_bit;
        return TRUE;
    }
#endif
#if NDS_P2_DONKEY
    if (fkind == nFTKindDonkey)
    {
        if ((ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_DONKEY, 0u) == FALSE) ||
            (ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_DONKEY, 1u) == FALSE))
        {
            gNdsPlayersVSPreviewResidentOwnerFailMask |= kind_bit;
            return FALSE;
        }
        gNdsPlayersVSPreviewResidentReadyMask |= kind_bit;
        return TRUE;
    }
#endif
#if NDS_P2_CAPTAIN
    if (fkind == nFTKindCaptain)
    {
        if ((ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_CAPTAIN, 0u) == FALSE) ||
            (ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_CAPTAIN, 1u) == FALSE))
        {
            gNdsPlayersVSPreviewResidentOwnerFailMask |= kind_bit;
            return FALSE;
        }
        gNdsPlayersVSPreviewResidentReadyMask |= kind_bit;
        return TRUE;
    }
#endif
#if NDS_P2_SAMUS
    if (fkind == nFTKindSamus)
    {
        if ((ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_SAMUS, 0u) == FALSE) ||
            (ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_SAMUS, 1u) == FALSE))
        {
            gNdsPlayersVSPreviewResidentOwnerFailMask |= kind_bit;
            return FALSE;
        }
        gNdsPlayersVSPreviewResidentReadyMask |= kind_bit;
        return TRUE;
    }
#endif
#if NDS_P2_LINK
    if (fkind == nFTKindLink)
    {
        if ((ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_LINK, 0u) == FALSE) ||
            (ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_LINK, 1u) == FALSE))
        {
            gNdsPlayersVSPreviewResidentOwnerFailMask |= kind_bit;
            return FALSE;
        }
        gNdsPlayersVSPreviewResidentReadyMask |= kind_bit;
        return TRUE;
    }
#endif
#if NDS_P2_PIKACHU
    if (fkind == nFTKindPikachu)
    {
        if ((ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_PIKACHU, 0u) == FALSE) ||
            (ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_PIKACHU, 1u) == FALSE))
        {
            gNdsPlayersVSPreviewResidentOwnerFailMask |= kind_bit;
            return FALSE;
        }
        gNdsPlayersVSPreviewResidentReadyMask |= kind_bit;
        return TRUE;
    }
#endif
#if NDS_P2_YOSHI
    if (fkind == nFTKindYoshi)
    {
        if ((ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_YOSHI, 0u) == FALSE) ||
            (ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_YOSHI, 1u) == FALSE))
        {
            gNdsPlayersVSPreviewResidentOwnerFailMask |= kind_bit;
            return FALSE;
        }
        gNdsPlayersVSPreviewResidentReadyMask |= kind_bit;
        return TRUE;
    }
#endif
    gNdsPlayersVSPreviewResidentReadyMask |= kind_bit;
    return TRUE;
}

static void ndsMNPlayersVSPreviewPrepareResidentKinds(void)
{
    gNdsPlayersVSPreviewResidentPrepareMask = 0u;
    gNdsPlayersVSPreviewResidentReadyMask = 0u;
    gNdsPlayersVSPreviewResidentMainFailMask = 0u;
    gNdsPlayersVSPreviewResidentSubmotionFailMask = 0u;
    gNdsPlayersVSPreviewResidentAnimFailMask = 0u;
    gNdsPlayersVSPreviewResidentOwnerFailMask = 0u;
    gNdsPlayersVSPreviewRebuildCount = 0u;
    gNdsPlayersVSPreviewRebuildPayloadReadCount = 0u;
    gNdsPlayersVSPreviewRebuildPayloadReadMax = 0u;

    (void)ndsMNPlayersVSPreviewPrepareResidentKind(nFTKindMario);
    (void)ndsMNPlayersVSPreviewPrepareResidentKind(nFTKindFox);
#if NDS_P2_LUIGI
    (void)ndsMNPlayersVSPreviewPrepareResidentKind(nFTKindLuigi);
#endif
#if NDS_P2_DONKEY
    (void)ndsMNPlayersVSPreviewPrepareResidentKind(nFTKindDonkey);
#endif
#if NDS_P2_CAPTAIN
    (void)ndsMNPlayersVSPreviewPrepareResidentKind(nFTKindCaptain);
#endif
#if NDS_P2_SAMUS
    (void)ndsMNPlayersVSPreviewPrepareResidentKind(nFTKindSamus);
#endif
#if NDS_P2_LINK
    (void)ndsMNPlayersVSPreviewPrepareResidentKind(nFTKindLink);
#endif
#if NDS_P2_PIKACHU
    (void)ndsMNPlayersVSPreviewPrepareResidentKind(nFTKindPikachu);
#endif
#if NDS_P2_YOSHI
    (void)ndsMNPlayersVSPreviewPrepareResidentKind(nFTKindYoshi);
#endif
}

/* The corrected source viewport maps BattleShip's 840-world-unit fighter-slot
 * pitch to exactly 64 DS pixels, so the source roots land at x=32/96/160/224.
 * That is ideal for the ordinary roster, but Donkey's wider silhouette can
 * cross the physical screen edge from the two 64-pixel outer panels. Keep the
 * inner pair source-exact and inset only 1P/4P by 80 world units (~6.1 DS px).
 *
 * This is presentation-only and deliberately lives after the source rebuild;
 * gameplay, the source CSS implementation, fighter scale, and joint transforms
 * remain untouched. Set the exact derived root position instead of accumulating
 * an offset so repeated player-kind / grab rebuilds are idempotent. */
#define NDS_CSS_OUTER_PREVIEW_INSET 40.0F
static void ndsMNPlayersVSPreviewApplyOuterSlotInset(u32 slot,
                                                      GObj *fighter_gobj)
{
    DObj *root;
    f32 x;

    if ((fighter_gobj == NULL) || (slot >= GMCOMMON_PLAYERS_MAX))
    {
        return;
    }
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        return;
    }

    x = ((f32)slot * 840.0F) - 1250.0F;
    if (slot == 0u)
    {
        x += NDS_CSS_OUTER_PREVIEW_INSET;
    }
    else if (slot == (GMCOMMON_PLAYERS_MAX - 1u))
    {
        x -= NDS_CSS_OUTER_PREVIEW_INSET;
    }
    root->translate.vec.f.x = x;
}

/* P2-1N Stage D: the native shell owns the 2D CSS, but its fighter previews
 * remain the source's real fighter objects. This is the smallest faithful
 * subset of mnPlayersVSFuncStart: the same reloc context, fighter manager,
 * figatree heaps, fighter camera, light, mnPlayersVSMakeFighter and preview
 * process. The source portrait/cursor/gate GObjs are intentionally omitted
 * because the UI kit already owns those layers. */
void ndsMNPlayersVSPreviewInit(void)
{
    LBRelocSetup rl_setup;
    s32 i;

    if (sNdsPlayersVSPreviewActive != FALSE)
    {
        return;
    }

    /* P2-3r16. THE WHOLE OF THIS FUNCTION IS A BLOCKING LOAD BURST, and on a
     * SECOND character-select entry it runs with menu BGM already playing.
     *
     * The note further down used to say "this function runs before
     * ndsMenuShellCssPlayBgm", which is true of the CSS's own track and false of
     * the one still playing when the player backs out of the stage select. This
     * body calls `ftManagerSetupFilesAllKind` for every admitted kind -- four of
     * them on the landed roster -- and then warms the preview owners, and none
     * of it was bracketed. MEASURED 2026-08-25 on `build-p2-shell`
     * (`artifacts/verification/2026-08-25_p2-3r14-bgmseam2.txt`): the shell
     * walk's SECOND `ndsMenuShellRunCharSelect` entered with
     * `gNdsAudioBgmSeamMissCount` 0 and `gNdsRelocAssetPayloadReadCount` 1,637,
     * and `ndsAudioBgmFailPlayback` fired at 1,810 reads with the miss count at
     * 1 -- before the first `mnPlayersVSUpdateFighter` bracket below was
     * reached. The music was then gone for the rest of the screen.
     *
     * Same remedy as that bracket, for the same reason: the stall is legitimate
     * and known in advance, so top the stream up and disarm the timer instead of
     * letting the seam call it an underrun. `ndsAudioBgmSuspendForBlockingLoad`
     * returns immediately when nothing is playing, so a FIRST entry is
     * unaffected and the pairing still counts.
     *
     * It reads as a flaky arm because it is marginal, not because it is random:
     * whether one CSS-entry frame overruns one 8,196-byte packet moves with code
     * placement, so an unrelated few-instruction change can flip it either way.
     * That is what exposed it. */
    ndsAudioBgmSuspendForBlockingLoad();

    gNdsPlayersVSPreviewFrameCount = 0u;
    gNdsPlayersVSPreviewDrawCount = 0u;
    gNdsPlayersVSPreviewSelectedMask = 0u;
    gNdsPlayersVSPreviewVisibleMask = 0u;
    gNdsPlayersVSPreviewSelectedKindMask = 0u;
    for (i = 0; i < ARRAY_COUNT(sMNPlayersVSSlots); i++)
    {
        gNdsPlayersVSPreviewRotationY[i] = 0.0F;
        gNdsPlayersVSPreviewStatus[i] = -1;
        gNdsPlayersVSPreviewMotion[i] = -1;
        gNdsPlayersVSPreviewFreeRotateFrames[i] = 0u;
        gNdsPlayersVSPreviewLastFreeRotationY[i] = 0.0F;
        gNdsPlayersVSPreviewLastFreeStatus[i] = -1;
        gNdsPlayersVSPreviewLastFreeMotion[i] = -1;
    }
    for (i = 0; i <= nFTKindPlayableEnd; i++)
    {
        gNdsPlayersVSPreviewSelectedKindFrames[i] = 0u;
        gNdsPlayersVSPreviewSelectedKindStatus[i] = -1;
        gNdsPlayersVSPreviewSelectedKindMotion[i] = -1;
    }

    rl_setup.table_addr = (uintptr_t)&lLBRelocTableAddr;
    rl_setup.table_files_num = (u32)&llRelocFileCount;
    rl_setup.file_heap = NULL;
    rl_setup.file_heap_size = 0;
    rl_setup.status_buffer = sMNPlayersVSStatusBuffer;
    rl_setup.status_buffer_size = ARRAY_COUNT(sMNPlayersVSStatusBuffer);
    rl_setup.force_status_buffer = sMNPlayersVSForceStatusBuffer;
    rl_setup.force_status_buffer_size = ARRAY_COUNT(sMNPlayersVSForceStatusBuffer);
    lbRelocInitSetup(&rl_setup);

    ftManagerAllocFighter(FTDATA_FLAG_SUBMOTION, 4);
    /* P2-2 intentionally stress-tests four fighter INSTANCES independently of
     * fighter KIND. Keep the shipping Mario/Fox preload unchanged, then admit
     * each P2-3 production kind only with the same build flag that stages its
     * generated reloc closure. BattleShip's PlayersVS source calls
     * ftManagerSetupFilesAllKind for every fighter allowed by its fighter mask;
     * this is the DS equivalent without paying RAM/I/O for unproduced kinds. */
    ftManagerSetupFilesAllKind(nFTKindMario);
    ftManagerSetupFilesAllKind(nFTKindFox);
#if NDS_P2_LUIGI
    ftManagerSetupFilesAllKind(nFTKindLuigi);
#endif
#if NDS_P2_DONKEY
    ftManagerSetupFilesAllKind(nFTKindDonkey);
#endif
#if NDS_P2_CAPTAIN
    ftManagerSetupFilesAllKind(nFTKindCaptain);
#endif
#if NDS_P2_SAMUS
    ftManagerSetupFilesAllKind(nFTKindSamus);
#endif
#if NDS_P2_LINK
    ftManagerSetupFilesAllKind(nFTKindLink);
#endif
#if NDS_P2_PIKACHU
    ftManagerSetupFilesAllKind(nFTKindPikachu);
#endif
#if NDS_P2_YOSHI
    ftManagerSetupFilesAllKind(nFTKindYoshi);
#endif

    for (i = 0; i < ARRAY_COUNT(sMNPlayersVSSlots); i++)
    {
        sMNPlayersVSSlots[i].player = NULL;
        sMNPlayersVSSlots[i].figatree_heap =
            syTaskmanMalloc(gFTManagerFigatreeHeapSize, 0x10);
        sMNPlayersVSSlots[i].pkind = nFTPlayerKindNot;
        sMNPlayersVSSlots[i].fkind = nFTKindNull;
        sMNPlayersVSSlots[i].costume = 0;
        sMNPlayersVSSlots[i].shade = 0;
        sMNPlayersVSSlots[i].is_selected = FALSE;
        sMNPlayersVSSlots[i].is_fighter_selected = FALSE;
        sMNPlayersVSSlots[i].is_status_selected = FALSE;
    }
    /* P2-3r16 CORRECTION: this runs before `ndsMenuShellCssPlayBgm` only on a
     * FIRST entry. A second entry, from the stage select's back-out, still has
     * menu BGM playing -- which is why the whole function is now bracketed at
     * the top. Load every owner image
     * admitted by this exact roster here, after the four figatree heaps so the
     * established CSS allocation order stays stable. Fighter creation keeps its
     * ensure call as a fail-safe, but a healthy CSS never reaches disk there. */
    ndsMNPlayersVSPreviewPrepareResidentKinds();
    /* The native shell seeds the actual rule/team values immediately after its
     * descriptor is loaded.  Start from a deterministic neutral state so a
     * second CSS entry cannot inherit this file-global from the prior scene. */
    sMNPlayersVSIsTeamBattle = FALSE;
    sNdsPlayersVSPreviewRulesReady = FALSE;
    mnPlayersVSMakeFighterCamera();
    scSubsysFighterSetLightParams(45.0F, 45.0F, 0xFF, 0xFF, 0xFF, 0xFF);
    sNdsPlayersVSPreviewDrawPhase = 0u;
    sNdsPlayersVSPreviewActive = TRUE;
    ndsAudioBgmResumeAfterBlockingLoad();
}

/* P2-2a: the shell owns the 2D controls, but costume/shade behavior stays in
 * BattleShip.  This mirrors mnPlayersVSUpdateGameMode + mnPlayersVSUpdateGateAll
 * and mnPlayersVSCheckTeamSelectInRangeAll rather than inventing DS-side team
 * appearance rules.  In particular, entering Team Battle first parks every
 * live shade at 4, then recomputes each fighter in slot order so duplicate
 * same-character teammates get the same shade allocation as the source. */
void ndsMNPlayersVSPreviewSyncRules(sb32 is_team_battle, const u8 *teams,
                                    u32 team_count)
{
    sb32 new_team_battle;
    sb32 mode_changed;
    u32 changed_mask = 0u;
    u32 i;

    if (sNdsPlayersVSPreviewActive == FALSE)
    {
        return;
    }
    new_team_battle = (is_team_battle != FALSE) ? TRUE : FALSE;
    mode_changed = ((sNdsPlayersVSPreviewRulesReady != FALSE) &&
                    (sMNPlayersVSIsTeamBattle != new_team_battle)) ? TRUE :
                                                                       FALSE;

    for (i = 0u; i < ARRAY_COUNT(sMNPlayersVSSlots); i++)
    {
        s32 team = (i < team_count && teams != NULL) ? (s32)teams[i] :
                                                       nSCBattleTeamIDRed;

        if ((team < nSCBattleTeamIDRed) || (team > nSCBattleTeamIDGreen))
        {
            team = nSCBattleTeamIDRed;
        }
        if ((sNdsPlayersVSPreviewRulesReady != FALSE) &&
            (sMNPlayersVSSlots[i].team != team))
        {
            changed_mask |= 1u << i;
        }
        sMNPlayersVSSlots[i].team = team;
    }

    if ((mode_changed != FALSE) && (new_team_battle != FALSE))
    {
        /* mnPlayersVSUpdateGameMode:1940-1946. Shade 4 is a temporary sentinel
         * so UpdateGateAll's sequential mnPlayersVSGetShade calls do not see a
         * stale 0..3 shade from the previous FFA arrangement. */
        for (i = 0u; i < ARRAY_COUNT(sMNPlayersVSSlots); i++)
        {
            if (sMNPlayersVSSlots[i].fkind != nFTKindNull)
            {
                sMNPlayersVSSlots[i].shade = 4;
            }
        }
    }
    sMNPlayersVSIsTeamBattle = new_team_battle;

    if (sNdsPlayersVSPreviewRulesReady != FALSE)
    {
        for (i = 0u; i < ARRAY_COUNT(sMNPlayersVSSlots); i++)
        {
            GObj *fighter_gobj = sMNPlayersVSSlots[i].player;

            if ((sMNPlayersVSSlots[i].fkind == nFTKindNull) ||
                (fighter_gobj == NULL) ||
                ((mode_changed == FALSE) &&
                 (((changed_mask >> i) & 1u) == 0u)))
            {
                continue;
            }
            sMNPlayersVSSlots[i].costume = mnPlayersVSGetFreeCostume(
                sMNPlayersVSSlots[i].fkind, (s32)i);
            sMNPlayersVSSlots[i].shade = mnPlayersVSGetShade((s32)i);
            ftParamInitAllParts(fighter_gobj,
                                sMNPlayersVSSlots[i].costume,
                                sMNPlayersVSSlots[i].shade);
        }
    }
    sNdsPlayersVSPreviewRulesReady = TRUE;
}

void ndsMNPlayersVSPreviewSync(u32 slot, s32 pkind, s32 fkind,
                               sb32 is_selected)
{
    s32 old_pkind;
    s32 old_fkind;
    sb32 old_selected;
    GObj *fighter_gobj;
    sb32 update_fighter;

    /* BattleShip's PlayersVS state is four independent player slots. P2-2's
     * renderer now separates that INSTANCE slot from the generated Mario/Fox
     * owner kind, so Mario/Fox mirrors in slots 2/3 use exactly the same source
     * preview path rather than disappearing at a DS-only two-slot guard. */
    if ((sNdsPlayersVSPreviewActive == FALSE) ||
        (slot >= ARRAY_COUNT(sMNPlayersVSSlots)))
    {
        return;
    }
    if ((fkind != nFTKindMario) && (fkind != nFTKindFox)
#if NDS_P2_LUIGI
        && (fkind != nFTKindLuigi)
#endif
#if NDS_P2_DONKEY
        && (fkind != nFTKindDonkey)
#endif
#if NDS_P2_CAPTAIN
        && (fkind != nFTKindCaptain)
#endif
#if NDS_P2_SAMUS
        && (fkind != nFTKindSamus)
#endif
#if NDS_P2_PIKACHU
        && (fkind != nFTKindPikachu)
#endif
#if NDS_P2_YOSHI
        && (fkind != nFTKindYoshi)
#endif
    )
    {
        fkind = nFTKindNull;
    }
    if (pkind == nFTPlayerKindNot)
    {
        fkind = nFTKindNull;
        is_selected = FALSE;
    }

    old_pkind = sMNPlayersVSSlots[slot].pkind;
    old_fkind = sMNPlayersVSSlots[slot].fkind;
    old_selected = sMNPlayersVSSlots[slot].is_fighter_selected;
    fighter_gobj = sMNPlayersVSSlots[slot].player;

    sMNPlayersVSSlots[slot].pkind = pkind;
    sMNPlayersVSSlots[slot].fkind = fkind;
    sMNPlayersVSSlots[slot].is_selected = is_selected;
    sMNPlayersVSSlots[slot].is_fighter_selected = is_selected;

    /* Mirror exactly the source events that call mnPlayersVSUpdateFighter:
     * - a player-kind change (mnPlayersVSCheckPlayerKindSelect),
     * - a puck crossing onto another fighter (mnPlayersVSPuckProcUpdate),
     * - grabbing a selected puck (mnPlayersVSSetCursorGrab), and
     * - initial creation when no preview object exists.
     *
     * This matters even when the fighter KIND did not change. The source
     * rebuild on grab returns a selected-pose fighter to its ordinary preview
     * status while preserving Y rotation; it also recomputes the source's free
     * costume/shade and lets mnPlayersVSMakeFighter apply the CPU-player color
     * animation. Merely flipping is_fighter_selected leaves the wrong pose.
     * A FALSE->TRUE selection is intentionally absent: the source drop path
     * only flips is_fighter_selected and lets mnPlayersVSFighterProcUpdate turn
     * into the selected pose.
     *
     * `fkind != nFTKindNull` ON THE CREATION TERM, and it is source-exact
     * rather than an optimisation. `mnPlayersVSMakeFighter` (mnplayersvs.c:
     * 1624) wraps its ENTIRE body in `if (fkind != nFTKindNull)`, and
     * `mnPlayersVSUpdateFighter`'s hide-and-skip branch needs a non-NULL
     * fighter_gobj, so for an empty slot -- pkind NA, fkind Null, no object --
     * the source updater provably does nothing at all.
     *
     * The port calls this sync for ALL FOUR slots on EVERY character-select
     * tic (ndsMenuShellCssSyncPreviews), so without this term the two N/A
     * slots take the rebuild path forever: `fighter_gobj == NULL` is true, the
     * updater makes nothing, and the slot is still NULL next tic. Measured on
     * 2026-08-25 that was 2,209 no-op rebuilds in ONE character-select visit
     * -- two per tic -- each one paying a BGM blocking-load fence whose
     * resume re-primes the stream from the current cursor. That is the whole
     * of the "audible song lurch" this file's residency work was written to
     * remove; it was never the fence being expensive, it was the fence
     * bracketing a call the source would not have made. */
    update_fighter = (((fighter_gobj == NULL) && (fkind != nFTKindNull)) ||
                      (old_pkind != pkind) ||
                      (old_fkind != fkind) ||
                      ((old_selected != FALSE) &&
                       (is_selected == FALSE))) ? TRUE : FALSE;

    if (update_fighter != FALSE)
    {
        /* The source updater hides an existing object for NA/null and otherwise
         * calls mnPlayersVSMakeFighter, which destroys/replaces it, preserves Y
         * rotation, chooses a free costume and recomputes shade. Clear the DS
         * registration around that operation so neither branch can expose a
         * stale taskman-arena fighter pointer. */
        ndsFighterManagerRegisterDisplayFighter(NULL, slot);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        /* Unconditional (2026-08-21): mnPlayersVSUpdateFighter may destroy and
         * rebuild this slot's fighter even when the slot was previously EMPTY,
         * and the replacement MObjs can reuse addresses freed by ANOTHER
         * slot's earlier rebuild. The old fighter_gobj != NULL guard left that
         * reuse able to inherit another fighter's converted costume colors --
         * the owner's "mixed colors on the second same-kind fighter". The
         * clear is a 32x4 row wipe at menu-action rate; always pay it. */
        ndsFighterRendererInvalidateMaterialCachesForSlot(slot);
#endif
        /* 2026-08-30: this rebuild is now storage-free by measurement, so do
         * NOT stop/restart Battle Select BGM around it.  The old fence was the
         * audible "partially muted / instruments drop" report: suspend kills
         * the hardware channel and resume re-primes it from the current ADPCM
         * cursor.  The actual blocking work was lazy native-owner image load;
         * ndsMNPlayersVSPreviewPrepareResidentKinds now completes that work at
         * scene entry.  Shipping-cadence proof on build-bugs-css-io measured
         * 15 genuine rebuilds, payload total/max = 0/0, ready mask 0x9f and no
         * owner/anim/arena failures.  Keep the payload counters below as the
         * regression guard: a future rebuild dependency must become visible
         * here instead of being hidden by an audio fence. */
        {
            u32 payload_before = gNdsRelocAssetPayloadReadCount;
            u32 payload_delta;

            mnPlayersVSUpdateFighter((s32)slot);
            payload_delta = gNdsRelocAssetPayloadReadCount - payload_before;
            gNdsPlayersVSPreviewRebuildCount++;
            gNdsPlayersVSPreviewRebuildPayloadReadCount += payload_delta;
            if (payload_delta > gNdsPlayersVSPreviewRebuildPayloadReadMax)
            {
                gNdsPlayersVSPreviewRebuildPayloadReadMax = payload_delta;
            }
        }
        fighter_gobj = sMNPlayersVSSlots[slot].player;
        ndsMNPlayersVSPreviewApplyOuterSlotInset(slot, fighter_gobj);
        if ((fighter_gobj != NULL) &&
            ((fighter_gobj->flags & GOBJ_FLAG_HIDDEN) == 0u))
        {
            ndsFighterManagerRegisterDisplayFighter(fighter_gobj, slot);
        }
    }
}

/* P2-3 (owner, 2026-08-23: "should be able to change skins by selecting the 3d
 * preview").  THE SOURCE MECHANISM IS UNCHANGED -- this is only a different
 * button reaching it.  mnPlayersVSFuncRun gives each of the four C-buttons one
 * costume id through `ftParamGetCostumeCommonID(fkind, button)`, refuses a
 * costume another player already holds with `mnPlayersVSCheckCostumeUsed`, and
 * otherwise assigns costume+shade and re-inits the preview's parts
 * (mnplayersvs.c:3369-3406/:3287).  The DS pad has no C-buttons, so the shell
 * asks for the NEXT free id in that same four-entry cycle instead of a
 * specific one; every id, the used test, the shade and the re-init are the
 * source's own.
 *
 * Returns the new costume id, or -1 when the slot cannot change (no selected
 * fighter, Team Battle -- where the source takes the costume from the team --
 * or all four ids already held). */
s32 ndsMNPlayersVSPreviewCycleCostume(u32 slot)
{
    s32 fkind;
    s32 current;
    s32 i;

    if ((sNdsPlayersVSPreviewActive == FALSE) ||
        (slot >= ARRAY_COUNT(sMNPlayersVSSlots)))
    {
        return -1;
    }
    if (sMNPlayersVSIsTeamBattle != FALSE)
    {
        return -1;
    }
    if ((sMNPlayersVSSlots[slot].player == NULL) ||
        (sMNPlayersVSSlots[slot].is_fighter_selected == FALSE))
    {
        return -1;
    }
    fkind = sMNPlayersVSSlots[slot].fkind;
    if (fkind == nFTKindNull)
    {
        return -1;
    }
    current = sMNPlayersVSSlots[slot].costume;

    /* Start one past whichever button id currently matches, so repeated
     * presses walk the source's own 0,1,2,3 order rather than restarting. */
    {
        s32 start = 0;

        for (i = 0; i < 4; i++)
        {
            if (ftParamGetCostumeCommonID(fkind, i) == current)
            {
                start = i + 1;
                break;
            }
        }
        for (i = 0; i < 4; i++)
        {
            s32 button = (start + i) & 3;
            s32 costume = ftParamGetCostumeCommonID(fkind, button);

            if (costume == current)
            {
                continue;
            }
            if (mnPlayersVSCheckCostumeUsed(fkind, (s32)slot, costume) !=
                FALSE)
            {
                continue;
            }
            sMNPlayersVSSlots[slot].costume = costume;
            sMNPlayersVSSlots[slot].shade = mnPlayersVSGetShade((s32)slot);
            ftParamInitAllParts(sMNPlayersVSSlots[slot].player, costume,
                                sMNPlayersVSSlots[slot].shade);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
            /* Same reason the preview rebuild clears them: the converted
             * material rows are keyed by MObj address and this changes what
             * those addresses mean. */
            ndsFighterRendererInvalidateMaterialCachesForSlot(slot);
#endif
            gNdsPlayersVSPreviewCostumeChangeCount++;
            return costume;
        }
    }
    return -1;
}

u32 ndsMNPlayersVSPreviewGetAppearance(u32 slot)
{
    if ((sNdsPlayersVSPreviewActive == FALSE) ||
        (slot >= ARRAY_COUNT(sMNPlayersVSSlots)))
    {
        return 0u;
    }
    return ((u32)sMNPlayersVSSlots[slot].costume & 0xffu) |
           (((u32)sMNPlayersVSSlots[slot].shade & 0xffu) << 8);
}

void ndsMNPlayersVSPreviewFrame(void)
{
    u32 slot;

    if (sNdsPlayersVSPreviewActive == FALSE)
    {
        return;
    }
    /* This is the renderer/cache SOURCE-FRAME epoch, not a count of GX
     * presents. State below advances every 60 Hz tic even when this tic's 3D
     * image is retained, so per-frame caches must see a fresh epoch too. */
    gNdsRendererProfileFrameCount++;
    /* With the shell's func_start NULL, these are only the fighter camera and
     * fighter GObjs created above. Thus the source object manager can run and
     * draw the preview without resurrecting the source CSS input/UI graph. */
    gcRunAll();

    gNdsPlayersVSPreviewFrameCount++;
    gNdsPlayersVSPreviewSelectedMask = 0u;
    gNdsPlayersVSPreviewVisibleMask = 0u;
    for (slot = 0u; slot < ARRAY_COUNT(sMNPlayersVSSlots); slot++)
    {
        GObj *fighter_gobj = sMNPlayersVSSlots[slot].player;

        if (sMNPlayersVSSlots[slot].is_fighter_selected != FALSE)
        {
            gNdsPlayersVSPreviewSelectedMask |= 1u << slot;
        }
        if (fighter_gobj != NULL)
        {
            FTStruct *fp = ftGetStruct(fighter_gobj);

            gNdsPlayersVSPreviewRotationY[slot] =
                DObjGetStruct(fighter_gobj)->rotate.vec.f.y;
            if (fp != NULL)
            {
                gNdsPlayersVSPreviewStatus[slot] = fp->status_id;
                gNdsPlayersVSPreviewMotion[slot] = fp->motion_id;
                if ((sMNPlayersVSSlots[slot].is_fighter_selected != FALSE) &&
                    (sMNPlayersVSSlots[slot].fkind >= nFTKindPlayableStart) &&
                    (sMNPlayersVSSlots[slot].fkind <= nFTKindPlayableEnd))
                {
                    u32 fkind = (u32)sMNPlayersVSSlots[slot].fkind;

                    gNdsPlayersVSPreviewSelectedKindMask |= 1u << fkind;
                    gNdsPlayersVSPreviewSelectedKindFrames[fkind]++;
                    gNdsPlayersVSPreviewSelectedKindStatus[fkind] = fp->status_id;
                    gNdsPlayersVSPreviewSelectedKindMotion[fkind] = fp->motion_id;
                }
                if (sMNPlayersVSSlots[slot].is_fighter_selected == FALSE)
                {
                    gNdsPlayersVSPreviewFreeRotateFrames[slot]++;
                    gNdsPlayersVSPreviewLastFreeRotationY[slot] =
                        gNdsPlayersVSPreviewRotationY[slot];
                    gNdsPlayersVSPreviewLastFreeStatus[slot] = fp->status_id;
                    gNdsPlayersVSPreviewLastFreeMotion[slot] = fp->motion_id;
                }
            }
            if ((fighter_gobj->flags & GOBJ_FLAG_HIDDEN) == 0u)
            {
                gNdsPlayersVSPreviewVisibleMask |= 1u << slot;
            }
        }
    }
    /* BattleShip hides an NA/null fighter object immediately. The DS 3D layer
     * is retained, however, so when that was the last visible preview the
     * previously completed geometry frame stayed on BG0 even though the source
     * GObj was correctly hidden. Make BG0 visibility follow the source live
     * fighter set; another visible fighter will naturally replace the old frame
     * on the next 30 Hz preview draw. */
    ndsPlatformSet3DLayerEnabled(
        (gNdsPlayersVSPreviewVisibleMask != 0u) ? TRUE : FALSE);
    /* Source state remains a 60 Hz process: rotation, figatree evaluation and
     * the selected-status transition above all advance every CSS tic. The
     * owner-ratified DS presentation is a 30 Hz 3D pass under a 60 Hz 2D menu,
     * so submit GX on alternating tics and let retained BG0 hold the preceding
     * 3D image between them. The renderer dedup token advances only when a real
     * source draw occurs, exactly as it does at the battle/Results draw seam. */
    if (sNdsPlayersVSPreviewDrawPhase == 0u)
    {
        /* mnPlayersVSMakeFighterCamera uses (10,10)-(310,230) inside the
         * source's 320x240 frame. Using the whole DS viewport stretched that
         * 300x220 window to 320x240 equivalent, which moved the outer 1P/4P
         * previews away from their source panel centres. */
        ndsPlatformSet3DViewportSource(10, 10, 310, 230);
        gcDrawAll();
        ndsPlatformReset3DViewport();
        gNdsPlayersVSPreviewDrawCount++;
    }
    sNdsPlayersVSPreviewDrawPhase ^= 1u;
}

void ndsMNPlayersVSPreviewExit(void)
{
    u32 slot;

    if (sNdsPlayersVSPreviewActive == FALSE)
    {
        return;
    }
    for (slot = 0u; slot < ARRAY_COUNT(sMNPlayersVSSlots); slot++)
    {
        GObj *fighter_gobj = sMNPlayersVSSlots[slot].player;

        ndsFighterManagerRegisterDisplayFighter(NULL, slot);
        if (fighter_gobj != NULL)
        {
            ftManagerDestroyFighter(fighter_gobj);
        }
        sMNPlayersVSSlots[slot].player = NULL;
    }
    sNdsPlayersVSPreviewActive = FALSE;
    gNdsPlayersVSPreviewExitCount++;
}

/* Only mnPlayersVSStartScene below uses this, and that is compiled out with the
 * P2-1e menu shell on -- marked rather than bracketed so the guard stays one
 * block around the scene it belongs to. */
static SYTaskmanSetup ndsMNPlayersVSMakeTaskmanSetup(void)
    __attribute__((unused));

static SYTaskmanSetup ndsMNPlayersVSMakeTaskmanSetup(void)
{
    SYTaskmanSetup setup = dMNPlayersVSTaskmanSetup;

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = mnPlayersVSFuncStart;
    return setup;
}

static void ndsMNPlayersVSClearControllerState(void)
{
    s32 i;

    gSYControllerConnectedNum = 2;
    for (i = 0; i < MAXCONTROLLERS; i++)
    {
        gSYControllerDeviceStatuses[i] = -1;
        gSYControllerDevices[i].button_tap = 0;
        gSYControllerDevices[i].button_hold = 0;
        gSYControllerDevices[i].button_update = 0;
        gSYControllerDevices[i].button_release = 0;
        gSYControllerDevices[i].stick_range.x = 0;
        gSYControllerDevices[i].stick_range.y = 0;
    }
    gSYControllerDeviceStatuses[0] = 0;
    gSYControllerDeviceStatuses[1] = 1;
}

static void ndsMNPlayersVSRecordSlotProof(void)
{
    s32 i;
    u32 selected_mask = 0;
    u32 kind_mask = 0;
    u32 cursors = 0;
    u32 pucks = 0;
    u32 gates = 0;
    u32 heaps = 0;
    u32 controller_mask = 0;

    for (i = 0; i < ARRAY_COUNT(sMNPlayersVSSlots); i++)
    {
        if (sMNPlayersVSSlots[i].pkind != nFTPlayerKindNot)
        {
            kind_mask |= 1u << i;
        }
        if (sMNPlayersVSSlots[i].is_fighter_selected != FALSE)
        {
            selected_mask |= 1u << i;
        }
        if (sMNPlayersVSSlots[i].cursor != NULL)
        {
            cursors++;
        }
        if (sMNPlayersVSSlots[i].puck != NULL)
        {
            pucks++;
        }
        if (sMNPlayersVSSlots[i].panel != NULL)
        {
            gates++;
        }
        if (sMNPlayersVSSlots[i].figatree_heap != NULL)
        {
            heaps++;
        }
        if (sMNPlayersVSControllerOrders[i] != -1)
        {
            controller_mask |= 1u << i;
        }
    }

    gNdsPlayersVSOriginalControllerOrderMask = controller_mask;
    gNdsPlayersVSOriginalSlotKindMask = kind_mask;
    gNdsPlayersVSOriginalSlotSelectedMask = selected_mask;
    gNdsPlayersVSOriginalCursorCount = cursors;
    gNdsPlayersVSOriginalPuckCount = pucks;
    gNdsPlayersVSOriginalGateCount = gates;
    gNdsPlayersVSOriginalFigatreeHeapCount = heaps;
}

void mnPlayersVSFuncStart(void)
{
    GObj *main_gobj;
    LBRelocSetup rl_setup;
    s32 i;

    gNdsPlayersVSOriginalFuncStartResult =
        NDS_PLAYERS_VS_ORIGINAL_FUNC_START_PASS;

    rl_setup.table_addr = (uintptr_t)&lLBRelocTableAddr;
    rl_setup.table_files_num = (u32)&llRelocFileCount;
    rl_setup.file_heap = NULL;
    rl_setup.file_heap_size = 0;
    rl_setup.status_buffer = sMNPlayersVSStatusBuffer;
    rl_setup.status_buffer_size = ARRAY_COUNT(sMNPlayersVSStatusBuffer);
    rl_setup.force_status_buffer = sMNPlayersVSForceStatusBuffer;
    rl_setup.force_status_buffer_size =
        ARRAY_COUNT(sMNPlayersVSForceStatusBuffer);

    lbRelocInitSetup(&rl_setup);
    lbRelocLoadFilesListed(dMNPlayersVSFileIDs, sMNPlayersVSFiles);
    if (gNdsPlayersVSOriginalLoadedFileCount == 7u)
    {
        gNdsPlayersVSOriginalSetupMask |= (1u << 0);
    }

    main_gobj = gcMakeGObjSPAfter(nGCCommonKindPlayerSelect,
                                  mnPlayersVSFuncRun,
                                  15,
                                  GOBJ_PRIORITY_DEFAULT);
    sNdsPlayersVSMainGObj = main_gobj;
    if (main_gobj != NULL)
    {
        gNdsPlayersVSOriginalMainGObjID = main_gobj->id;
        gNdsPlayersVSOriginalSetupMask |= (1u << 1);
    }

    gcMakeDefaultCameraGObj(16, GOBJ_PRIORITY_DEFAULT, 100,
                            COBJ_FLAG_ZBUFFER,
                            GPACK_RGBA8888(0x00, 0x00, 0x00, 0x00));
    if (sGCCamerasActiveNum >= 1u)
    {
        gNdsPlayersVSOriginalCameraCount = sGCCamerasActiveNum;
        gNdsPlayersVSOriginalSetupMask |= (1u << 2);
    }

    efParticleInitAll();
    efManagerInitEffects();
    ndsMNPlayersVSClearControllerState();
    mnPlayersVSUpdateControllerOrders();
    mnPlayersVSInitVars();
    ftManagerAllocFighter(FTDATA_FLAG_SUBMOTION, 4);

    for (i = nFTKindPlayableStart; i <= nFTKindPlayableEnd; i++)
    {
        ftManagerSetupFilesAllKind(i);
    }
    for (i = 0; i < ARRAY_COUNT(sMNPlayersVSSlots); i++)
    {
        sMNPlayersVSSlots[i].figatree_heap =
            syTaskmanMalloc(gFTManagerFigatreeHeapSize, 0x10);
    }
    gNdsPlayersVSOriginalSetupMask |= (1u << 3);

    mnPlayersVSMakePortraitCamera();
    mnPlayersVSMakeCursorCamera();
    mnPlayersVSMakePuckCamera();
    mnPlayersVSMakePlayerKindCamera();
    mnPlayersVSMakeGateCamera();
    mnPlayersVSMakePlayerKindSelectCamera();
    mnPlayersVSMakeFighterCamera();
    mnPlayersVSMakeTeamSelectCamera();
    mnPlayersVSMakeHandicapLevelCamera();
    mnPlayersVSMakePortraitWallpaperCamera();
    mnPlayersVSMakePortraitFlashCamera();
    mnPlayersVSMakeReadyCamera();
    gNdsPlayersVSOriginalSetupMask |= (1u << 4);

    mnPlayersVSMakeWallpaper();
    mnPlayersVSMakePortraitAll();
    mnPlayersVSInitSlotAll();
    mnPlayersVSMakeLabels();
    mnPlayersVSMakePuckAdjust();
    mnPlayersVSMakePuckGlow();
    mnPlayersVSMakeCostumeSync();
    mnPlayersVSMakeSpotlight();
    mnPlayersVSMakeReady();
    scSubsysFighterSetLightParams(45.0F, 45.0F, 0xFF, 0xFF, 0xFF, 0xFF);
    gNdsPlayersVSOriginalSetupMask |= (1u << 5);

    ndsMNPlayersVSRecordSlotProof();

    gNdsPlayersVSOriginalGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsPlayersVSOriginalSObjCount = sGCSpritesActiveNum;
    gNdsPlayersVSOriginalPortraitCount =
        (sGCSpritesActiveNum >= GMCOMMON_FIGHTERS_PLAYABLE_NUM) ?
        GMCOMMON_FIGHTERS_PLAYABLE_NUM : sGCSpritesActiveNum;
    gNdsPlayersVSOriginalTime = (u32)sMNPlayersVSTimeValue;
    gNdsPlayersVSOriginalStock = (u32)sMNPlayersVSStockValue;
    gNdsPlayersVSOriginalGameRule = (u32)sMNPlayersVSGameRule;
    gNdsPlayersVSOriginalIsTeam = (u32)sMNPlayersVSIsTeamBattle;
    gNdsPlayersVSOriginalIsStageSelect =
        (u32)gSCManagerTransferBattleState.is_stage_select;
    gNdsPlayersVSOriginalSetupMask |= (1u << 6);
    gNdsPlayersVSOriginalSetupMask |= (1u << 7);
    gNdsPlayersVSOriginalDeferredMask =
        (1u << 0) | /* continuous character-select input */
        (1u << 1) | /* fighter object import/display */
        (1u << 2);  /* continuous menu rendering/audio */
    gNdsPlayersVSOriginalSetupResult =
        NDS_PLAYERS_VS_ORIGINAL_SETUP_PASS;
}

static void ndsMNPlayersVSSeedReadyPlayers(void)
{
    s32 i;

    ndsMNPlayersVSClearControllerState();
    sMNPlayersVSTotalTimeTics = I_SEC_TO_TICS(1) + 1;
    sMNPlayersVSIsStart = FALSE;
    sMNPlayersVSStartProceedWait = 0;
    gSCManagerTransferBattleState.is_stage_select = TRUE;

    for (i = 0; i < ARRAY_COUNT(sMNPlayersVSSlots); i++)
    {
        sMNPlayersVSSlots[i].cursor_status = nMNPlayersCursorStatusPointer;
        sMNPlayersVSSlots[i].is_fighter_selected = FALSE;
        sMNPlayersVSSlots[i].is_selected = FALSE;
        sMNPlayersVSSlots[i].pkind = nFTPlayerKindNot;
        sMNPlayersVSSlots[i].fkind = nFTKindNull;
        sMNPlayersVSSlots[i].team = 0;
        sMNPlayersVSSlots[i].costume = 0;
        sMNPlayersVSSlots[i].shade = 0;
        sMNPlayersVSSlots[i].cpu_level = 1;
        sMNPlayersVSSlots[i].handicap = 0;
    }

    sMNPlayersVSSlots[0].pkind = nFTPlayerKindMan;
    sMNPlayersVSSlots[0].fkind = nFTKindMario;
    sMNPlayersVSSlots[0].is_fighter_selected = TRUE;
    sMNPlayersVSSlots[0].is_selected = TRUE;
    sMNPlayersVSSlots[0].holder_player = 0;
    sMNPlayersVSSlots[0].held_player = -1;

    sMNPlayersVSSlots[1].pkind = nFTPlayerKindMan;
    sMNPlayersVSSlots[1].fkind = nFTKindFox;
    sMNPlayersVSSlots[1].is_fighter_selected = TRUE;
    sMNPlayersVSSlots[1].is_selected = TRUE;
    sMNPlayersVSSlots[1].holder_player = 1;
    sMNPlayersVSSlots[1].held_player = -1;
}

static void ndsMNPlayersVSRunMainTick(u32 *updates)
{
    mnPlayersVSFuncRun(sNdsPlayersVSMainGObj);
    (*updates)++;
    gNdsPlayersVSReadyTransitionUpdateCount = *updates;
}

void ndsMNPlayersVSRunReadyTransitionProbe(void)
{
    u32 updates = 0;
    u32 mask = 0;
    u32 i;

    gNdsPlayersVSReadyTransitionResult =
        NDS_PLAYERS_VS_READY_TRANSITION_FAIL;
    gNdsPlayersVSReadyTransitionMask = 0;
    gNdsPlayersVSReadyTransitionScenePrevBefore =
        gSCManagerSceneData.scene_prev;
    gNdsPlayersVSReadyTransitionSceneCurrBefore =
        gSCManagerSceneData.scene_curr;

    if (((gNdsPlayersVSOriginalSetupMask & 0xffu) == 0xffu) &&
        (sNdsPlayersVSMainGObj != NULL))
    {
        mask |= (1u << 0);
    }

    ndsMNPlayersVSSeedReadyPlayers();
    if ((mnPlayersVSCheckReady() != FALSE) &&
        (gSCManagerTransferBattleState.is_stage_select != FALSE))
    {
        mask |= (1u << 1);
    }

    gNdsPlayersVSReadyTransitionInputMask = START_BUTTON;
    gSYControllerDevices[0].button_tap = START_BUTTON;
    gSYControllerDevices[0].button_hold = START_BUTTON;
    ndsMNPlayersVSRunMainTick(&updates);

    if (sMNPlayersVSIsStart != FALSE)
    {
        mask |= (1u << 2);
    }
    if (gNdsPlayersVSReadyTransitionInputMask == START_BUTTON)
    {
        mask |= (1u << 3);
    }

    for (i = 0; i < 31u; i++)
    {
        ndsMNPlayersVSClearControllerState();
        ndsMNPlayersVSRunMainTick(&updates);
        if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
        {
            break;
        }
    }

    if ((gSCManagerTransferBattleState.pl_count == 2u) &&
        (gSCManagerTransferBattleState.cp_count == 0u))
    {
        mask |= (1u << 4);
    }
    if ((gSCManagerSceneData.scene_prev == nSCKindPlayersVS) &&
        (gSCManagerSceneData.scene_curr == nSCKindMaps))
    {
        mask |= (1u << 5);
    }

    gNdsPlayersVSReadyTransitionTaskmanStatus = (u32)sSYTaskmanStatus;
    if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
    {
        mask |= (1u << 6);
    }

    gcEjectAll();
    gNdsPlayersVSReadyTransitionCleanupCount++;
    mask |= (1u << 7);

    gNdsPlayersVSReadyTransitionPlayerCount =
        gSCManagerTransferBattleState.pl_count;
    gNdsPlayersVSReadyTransitionCpuCount =
        gSCManagerTransferBattleState.cp_count;
    gNdsPlayersVSReadyTransitionP0FKind =
        gSCManagerTransferBattleState.players[0].fkind;
    gNdsPlayersVSReadyTransitionP1FKind =
        gSCManagerTransferBattleState.players[1].fkind;
    gNdsPlayersVSReadyTransitionStageSelect =
        gSCManagerTransferBattleState.is_stage_select;
    gNdsPlayersVSReadyTransitionScenePrevFinal =
        gSCManagerSceneData.scene_prev;
    gNdsPlayersVSReadyTransitionSceneCurrFinal =
        gSCManagerSceneData.scene_curr;
    gNdsPlayersVSReadyTransitionMask = mask;
    if (mask == 0xffu)
    {
        gNdsPlayersVSReadyTransitionResult =
            NDS_PLAYERS_VS_READY_TRANSITION_PASS;
    }
}

#if !NDS_P2_MENU_SHELL
/* P2-1e defines the real character-select scene in src/nds/nds_menu_shell.c.
 * This one runs the ORIGINAL mnPlayersVSFuncStart, which loads seven menu
 * files, sets up all twelve fighters and allocates four figatree heaps out of
 * the scene arena -- the bounded proof path, not a playable screen. */
void mnPlayersVSStartScene(void)
{
    dMNPlayersVSVideoSetup.zbuffer =
        SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNPlayersVSVideoSetup);

    gNdsPlayersVSOriginalStartResult =
        NDS_PLAYERS_VS_ORIGINAL_START_PASS;

    {
        SYTaskmanSetup setup = ndsMNPlayersVSMakeTaskmanSetup();
        scManagerFuncUpdate(&setup);
    }
}
#endif

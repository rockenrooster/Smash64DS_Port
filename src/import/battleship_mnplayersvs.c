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
#include <nds/nds_preview_pack.h>
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
extern void *ndsBattleShipLoadCSSSelectedFigatree(const void *file_id,
                                                   void *heap);
extern void ftManagerSetupFilesKind(s32 fkind);
extern void ndsFTManagerRestoreKirbyPreviewMainMotion(void);
extern void *ndsRelocExternTreeSliceBegin(const void *file_id);
extern s32 ndsRelocExternTreeSliceStep(void *cursor, u32 byte_budget,
                                       u32 node_budget, void **out_root,
                                       u32 *out_payload_bytes);
extern void ndsRelocExternTreeSliceCancel(void *cursor);
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

/* VS preview residency. Native production reuses 1P CSS's compact Main/Model
 * FPC1 packs. Each compact block also owns that preview's two native owner
 * images, so browsing a new fighter cannot permanently raise the scene heap's
 * high-water. Link is the largest current combination at < 70 KiB on disk;
 * 80 KiB leaves bounded loader/alignment headroom. Source/oracle profiles still
 * consume full Gfx/Vtx closures and keep the measured 156 KiB raw-tree slot. */
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
#define NDS_PLAYERS_VS_COMPACT_PREVIEW 1
#else
#define NDS_PLAYERS_VS_COMPACT_PREVIEW 0
#endif
/* The shared closure is the fixed tree list in sNdsPlayersVSSharedResidentAssetIDs,
 * so its arena is sized to what those trees measure, not to a round number. The
 * compact-preview list (seven trees) pins 15,232 B, identically on the VS-only
 * shell and the all-content free-play ROM (2026-09-23, at VS CSS exit). The raw-
 * tree profiles also pin YoshiModel and keep 64 KiB.
 *
 * Every byte here comes out of the character select's scene heap BEFORE
 * ndsR2AnimCacheReserveCSSWorkingSet, which must also leave
 * NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE behind its reservation. With 64 KiB the
 * all-content configuration (the published smash64ds and its free-play twin)
 * reached that reservation with 163,792 B free against 183,072 B required, the
 * reservation declined, and every VS preview was switched off -- the published
 * 2026-09-22 ROM had cleared it by 5,296 B. Too small a value cannot hide the
 * same way: a shared tree that does not fit halts in ndsSyMallocOverflowHalt
 * naming this arena ('CSS\0'). */
#if NDS_PLAYERS_VS_COMPACT_PREVIEW
#define NDS_PLAYERS_VS_SHARED_RESIDENT_BYTES (24u * 1024u)
#else
#define NDS_PLAYERS_VS_SHARED_RESIDENT_BYTES (64u * 1024u)
#endif
#define NDS_PLAYERS_VS_RESIDENT_BLOCKS GMCOMMON_PLAYERS_MAX
#define NDS_PLAYERS_VS_LOAD_CHUNK_BYTES (8u * 1024u)
#define NDS_PLAYERS_VS_LOAD_CHUNK_NODES 4u
/* M03. ONE aggregate read/decode span per CSS update, shared by all four
 * slots -- not a budget each. The compact transaction spends it in units of
 * NDS_PLAYERS_VS_LOAD_STEP_BYTES, so no single work unit inside the loader can
 * hold the main thread (and therefore the filesystem mutex the BGM refill
 * needs) for longer than one such unit. The shell loop's own per-frame
 * ndsAudioBgmUpdate is what services the stream between units; the preview
 * path no longer suspends the track at all.
 *
 * Sizing: the largest shipped preview pack is Link's at 32,032 data bytes, so
 * 8 KiB per update finishes its read in four updates while leaving the frame's
 * own work room. Both values are deliberately named so a measurement can move
 * the aggregate without changing the unit that bounds the audio gap. */
#define NDS_PLAYERS_VS_PREVIEW_SERVICE_BYTES (8u * 1024u)
#define NDS_PLAYERS_VS_LOAD_STEP_BYTES (2u * 1024u)
/* M04. The 13-tic dwell existed only to stop a full-speed roster pass from
 * starting a BLOCKING closure it would immediately regret: a 45-pixel portrait
 * cell at 4 pixels per tic is at most 12 consecutive tics of one kind. With the
 * load sliced and cancellable, an abandoned request now costs at most the units
 * already spent, so stabilization only has to reject genuine cursor transit.
 * Two stable updates is the candidate value to measure; a kind that is already
 * resident pays one, because reusing a warm block moves no bytes at all. */
#define NDS_PLAYERS_VS_PREVIEW_DWELL_TICKS 2u
#define NDS_PLAYERS_VS_PREVIEW_DWELL_WARM_TICKS 1u

/* Stages of the compact transaction AFTER its pack has been read, decoded and
 * relocated. Each is one aggregate action, so two of them never land on the
 * same update. READY is not a stage: it is the commit the caller performs. */
enum {
    NDS_PLAYERS_VS_PREPARE_COMMIT = 0, /* owner boundary: publish + capacity */
    NDS_PLAYERS_VS_PREPARE_ANIM_IDLE,
    NDS_PLAYERS_VS_PREPARE_ANIM_SELECTED,
    NDS_PLAYERS_VS_PREPARE_OWNER_HIGH,
    NDS_PLAYERS_VS_PREPARE_OWNER_LOW,
    NDS_PLAYERS_VS_PREPARE_READY
};

typedef enum NDSPlayersVSResidentAcquireResult {
    nNDSPlayersVSResidentAcquireFail = 0,
    nNDSPlayersVSResidentAcquireReady = 1,
    nNDSPlayersVSResidentAcquireRetry = 2
} NDSPlayersVSResidentAcquireResult;

typedef enum NDSPlayersVSResidentRetireReason {
    nNDSPlayersVSResidentRetireReuse = 0,
    nNDSPlayersVSResidentRetireExit = 1
} NDSPlayersVSResidentRetireReason;

enum {
    NDS_RELOC_EXTERN_TREE_SLICE_FAIL = 0,
    NDS_RELOC_EXTERN_TREE_SLICE_IN_PROGRESS = 1,
    NDS_RELOC_EXTERN_TREE_SLICE_DONE = 2
};

typedef struct NDSPlayersVSResidentBlock {
    SYMallocRegion arena;
    void *base;
    s32 fkind;
    u32 refs;
    void *load_cursor;
    void *load_root;
    s32 loading_fkind;
    sb32 load_tree_done;
    sb32 cancel_requested;
    /* Transaction identity. `load_stage` walks the post-closure preparation
     * one action at a time; `load_generation` is the taskman resource
     * generation the transaction was opened against, so a completion that
     * outlived a scene rewind is rejected instead of published. */
    u32 load_stage;
    u32 load_generation;
} NDSPlayersVSResidentBlock;

typedef struct NDSPlayersVSPreviewPending {
    s32 fkind;
    u32 stable_tics;
    sb32 seen_request;
    sb32 acquire_pending;
    /* M04 measures selection-change -> first correct visible preview, which is
     * stabilization plus load and must be reported per warm/cold class. Count
     * the source updates the request waited and which class it started in. */
    u32 request_updates;
    sb32 request_warm;
} NDSPlayersVSPreviewPending;

/* Upper bounds, in source updates, of buckets 0..6; bucket 7 is everything
 * above. A capture reads the two rows and computes P50/P95 directly. */
#define NDS_PLAYERS_VS_PREVIEW_LATENCY_BUCKETS 8u

static const u32 sNdsPlayersVSSharedResidentAssetIDs[] = {
    0x0c9u, 0x129u, 0x12au, 0x12bu,
    0x13bu, 0x146u,
#if !NDS_PLAYERS_VS_COMPACT_PREVIEW
    /* Raw extern-tree residency shares YoshiModel with Kirby. Compact CSS
     * gives asset 0x152 to Yoshi's FPC1 Model section instead; pinning the
     * raw file here would occupy that unique registry key before Yoshi loads. */
    0x152u,
#endif
    0x164u
};
static SYMallocRegion sNdsPlayersVSSharedResidentArena;
static void *sNdsPlayersVSSharedResidentBase;
static NDSPlayersVSResidentBlock
    sNdsPlayersVSResidentBlocks[NDS_PLAYERS_VS_RESIDENT_BLOCKS];
static u32 sNdsPlayersVSResidentPoolGeneration;
static sb32 sNdsPlayersVSResidentPoolsReady;
static f32 sNdsPlayersVSReleasedRotationY[GMCOMMON_PLAYERS_MAX];
static NDSPlayersVSPreviewPending
    sNdsPlayersVSPreviewPending[GMCOMMON_PLAYERS_MAX];
/* SyncRules is the once-per-CSS-tic entry immediately before the four slot
 * syncs. After the entry sync, permit at most one physical residency action
 * (one zero-ref retirement OR one closure load) across all four slots. */
static u32 sNdsPlayersVSPreviewResidencyActionBudget;
/* The aggregate read/decode span the whole screen may still spend this update.
 * Stepping an in-flight closure spends bytes; structural actions (begin,
 * retire, cancel, one preparation stage) spend the action budget above. */
static u32 sNdsPlayersVSPreviewServiceByteBudget;
/* Entry construction runs before CSS BGM starts and is a declared load frame,
 * so the four initial previews may still be driven to completion there. Every
 * live browsing update afterwards is bounded. */
static sb32 sNdsPlayersVSPreviewEntryDrain;
static sb32 sNdsPlayersVSPreviewEntrySyncPending;
/* A completed closure transaction can discover a deterministic resource
 * failure (required animation/data/owner image). Repeating the same full load
 * cannot change that result while the taskman resource generation is unchanged.
 * Latch it per fighter so a stationary/selected request remains an explicit
 * failed preview without turning into a NitroFS reload loop. */
static u32 sNdsPlayersVSPreviewPermanentFailGeneration;
static u32 sNdsPlayersVSPreviewPermanentFailMask;
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
/* Boot-lifetime witness: a compact preview is admitted only when its already-
 * loaded FPC1 body leaves enough of the fixed resident block for both native
 * owner images. Do not clear this at CSS re-entry; LOOPDONE must see a failure
 * from any lap in the run. */
volatile u32 gNdsPlayersVSPreviewResidentCapacityFailCount;
/* Boot-lifetime witness, same contract: a nonzero count is a character select
 * that opened with every VS preview switched off, because its pools could not
 * be built. The stage names the refusal: 1 shared arena, 2 a closure block,
 * 3 a shared tree, 4 the animation working set (the scene heap could not also
 * keep NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE behind it). Without it the only trace
 * of that failure was an empty panel. */
volatile u32 gNdsPlayersVSPreviewPoolFailCount;
volatile u32 gNdsPlayersVSPreviewPoolFailStage;
/* Genuine source preview rebuilds only. These counters deliberately bracket
 * mnPlayersVSUpdateFighter itself so unrelated CSS/menu NitroFS traffic cannot
 * be mistaken for a rebuild dependency again. */
volatile u32 gNdsPlayersVSPreviewRebuildCount;
volatile u32 gNdsPlayersVSPreviewRebuildPayloadReadCount;
volatile u32 gNdsPlayersVSPreviewRebuildPayloadReadMax;
/* d99a89f moved the fighter closure load ahead of the rebuild bracket above,
 * so those counters can truthfully read zero while a roster move still does
 * blocking NitroFS work. Keep the residency operation itself visible: a hit is
 * refcount-only; a load is the path that runs ftManagerSetupFilesAllKind plus
 * the animation/owner preparation before fighter creation can proceed. */
volatile u32 gNdsPlayersVSPreviewAcquireCount;
volatile u32 gNdsPlayersVSPreviewAcquireHitCount;
volatile u32 gNdsPlayersVSPreviewAcquireCachedHitCount;
/* LoadCount is the begin edge and LoadFinishCount the end edge of the entire
 * blocking closure transaction, including prepare and failure cleanup. */
volatile u32 gNdsPlayersVSPreviewAcquireLoadCount;
volatile u32 gNdsPlayersVSPreviewAcquireLoadFinishCount;
volatile u32 gNdsPlayersVSPreviewAcquirePayloadReadCount;
volatile u32 gNdsPlayersVSPreviewAcquirePayloadReadMax;
/* Main-tree payload bytes moved by the sliced loader. The older PayloadRead
 * counters above count read calls, not bytes. */
volatile u32 gNdsPlayersVSPreviewAcquireTreePayloadByteCount;
volatile u32 gNdsPlayersVSPreviewAcquireTreePayloadByteMax;
volatile u32 gNdsPlayersVSPreviewAcquireRetryCount;
volatile u32 gNdsPlayersVSPreviewAcquireFailCount;
volatile u32 gNdsPlayersVSPreviewReleaseCount;
volatile u32 gNdsPlayersVSPreviewReleaseLastRefCount;
/* RetireBeginCount brackets the scan-heavy release path with RetireCount. */
volatile u32 gNdsPlayersVSPreviewReleaseRetireBeginCount;
volatile u32 gNdsPlayersVSPreviewReleaseRetireCount;
volatile u32 gNdsPlayersVSPreviewReleaseReuseRetireCount;
volatile u32 gNdsPlayersVSPreviewReleaseExitRetireCount;
volatile u32 gNdsPlayersVSPreviewReleaseExitResidualRefCount;
volatile u32 gNdsPlayersVSPreviewDwellRequestCount;
volatile u32 gNdsPlayersVSPreviewDwellSkipCount;
volatile u32 gNdsPlayersVSPreviewDwellHoldTicCount;
volatile u32 gNdsPlayersVSPreviewDwellCommitCount;
/* M03/M04 transaction telemetry. ServiceStepCount is how many bounded units
 * the screen ran; ServiceByteMax is the largest span any single unit moved and
 * is the number that bounds the audio gap. StaleCommitRejectCount must stay at
 * zero unless a scene rewind raced a completion. WarmCommitCount separates a
 * cache reuse from a cold load in the latency measurement. */
volatile u32 gNdsPlayersVSPreviewServiceStepCount;
volatile u32 gNdsPlayersVSPreviewServiceByteMax;
volatile u32 gNdsPlayersVSPreviewServiceUpdateByteMax;
volatile u32 gNdsPlayersVSPreviewStaleCommitRejectCount;
volatile u32 gNdsPlayersVSPreviewWarmCommitCount;
volatile u32 gNdsPlayersVSPreviewPrepareStageCount;
volatile u32 gNdsPlayersVSPreviewLatencyWarmBuckets
    [NDS_PLAYERS_VS_PREVIEW_LATENCY_BUCKETS];
volatile u32 gNdsPlayersVSPreviewLatencyColdBuckets
    [NDS_PLAYERS_VS_PREVIEW_LATENCY_BUCKETS];
volatile u32 gNdsPlayersVSPreviewLatencyWarmMax;
volatile u32 gNdsPlayersVSPreviewLatencyColdMax;
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

static void ndsMNPlayersVSPreviewValidatePermanentFailures(void)
{
    if (sNdsPlayersVSPreviewPermanentFailGeneration !=
        gNdsTaskmanHeapGeneration)
    {
        sNdsPlayersVSPreviewPermanentFailGeneration =
            gNdsTaskmanHeapGeneration;
        sNdsPlayersVSPreviewPermanentFailMask = 0u;
    }
}

static sb32 ndsMNPlayersVSPreviewIsPermanentFailure(s32 fkind)
{
    ndsMNPlayersVSPreviewValidatePermanentFailures();
    return ((fkind >= nFTKindPlayableStart) &&
            (fkind <= nFTKindPlayableEnd) &&
            ((sNdsPlayersVSPreviewPermanentFailMask & (1u << fkind)) != 0u)) ?
        TRUE : FALSE;
}

static void ndsMNPlayersVSPreviewMarkPermanentFailure(s32 fkind)
{
    ndsMNPlayersVSPreviewValidatePermanentFailures();
    if ((fkind >= nFTKindPlayableStart) && (fkind <= nFTKindPlayableEnd))
    {
        sNdsPlayersVSPreviewPermanentFailMask |= 1u << fkind;
    }
}

/* fkind -> generated native owner-image slot. Mario and Fox draw from their
 * compiled-in tables and own no loadable image, so they answer FALSE and the
 * two owner stages become no-ops for them -- the same outcome the per-kind
 * ladder this replaces produced by falling through to its tail. */
static sb32 ndsMNPlayersVSPreviewOwnerImageSlot(s32 fkind, u32 *out_slot)
{
#define NDS_CSS_OWNER_IMAGE_SLOT(kind_, slot_)                                 \
    if (fkind == (kind_))                                                      \
    {                                                                          \
        *out_slot = (slot_);                                                   \
        return TRUE;                                                           \
    }
#if NDS_P2_LUIGI
    NDS_CSS_OWNER_IMAGE_SLOT(nFTKindLuigi, NDS_NATIVE_IMAGE_SLOT_LUIGI)
#endif
#if NDS_P2_DONKEY
    NDS_CSS_OWNER_IMAGE_SLOT(nFTKindDonkey, NDS_NATIVE_IMAGE_SLOT_DONKEY)
#endif
#if NDS_P2_CAPTAIN
    NDS_CSS_OWNER_IMAGE_SLOT(nFTKindCaptain, NDS_NATIVE_IMAGE_SLOT_CAPTAIN)
#endif
#if NDS_P2_SAMUS
    NDS_CSS_OWNER_IMAGE_SLOT(nFTKindSamus, NDS_NATIVE_IMAGE_SLOT_SAMUS)
#endif
#if NDS_P2_LINK
    NDS_CSS_OWNER_IMAGE_SLOT(nFTKindLink, NDS_NATIVE_IMAGE_SLOT_LINK)
#endif
#if NDS_P2_PIKACHU
    NDS_CSS_OWNER_IMAGE_SLOT(nFTKindPikachu, NDS_NATIVE_IMAGE_SLOT_PIKACHU)
#endif
#if NDS_P2_YOSHI
    NDS_CSS_OWNER_IMAGE_SLOT(nFTKindYoshi, NDS_NATIVE_IMAGE_SLOT_YOSHI)
#endif
#if NDS_P2_NESS
    NDS_CSS_OWNER_IMAGE_SLOT(nFTKindNess, NDS_NATIVE_IMAGE_SLOT_NESS)
#endif
#if NDS_P2_PURIN
    NDS_CSS_OWNER_IMAGE_SLOT(nFTKindPurin, NDS_NATIVE_IMAGE_SLOT_PURIN)
#endif
#if NDS_P2_KIRBY
    NDS_CSS_OWNER_IMAGE_SLOT(nFTKindKirby, NDS_NATIVE_IMAGE_SLOT_KIRBY)
#endif
#undef NDS_CSS_OWNER_IMAGE_SLOT
    (void)out_slot;
    return FALSE;
}

/* One owner image is one preparation stage: it is the largest single read the
 * transaction still makes (30,160 bytes for Kirby's high-detail image), so the
 * two details never share an update. */
static sb32 ndsMNPlayersVSPreviewEnsureOwnerImage(s32 fkind, u32 detail)
{
    u32 slot;

    if (ndsMNPlayersVSPreviewOwnerImageSlot(fkind, &slot) == FALSE)
    {
        return TRUE;
    }
    if (ndsRendererNativeEnsureOwnerImage(slot, detail) == FALSE)
    {
        gNdsPlayersVSPreviewResidentOwnerFailMask |= 1u << fkind;
        return FALSE;
    }
    return TRUE;
}

static sb32 ndsMNPlayersVSPreviewPreloadAnimFile(
    const void *file_id, SYMallocRegion *anim_region)
{
    SYMallocRegion *resident_region;
    sb32 loaded;

    /* The animation cache is scene-generation storage. PrepareResidentKind is
     * intentionally called with the fighter's resettable block selected so its
     * owner images land there; never let the cache's bare syTaskmanMalloc inherit
     * that shorter lifetime. */
    resident_region = ndsTaskmanSwapMallocRegion(anim_region);
    loaded = ndsR2AnimCachePreloadFighterFile(file_id);
    ndsTaskmanSwapMallocRegion(resident_region);
    return loaded;
}

/* One preparation stage. Every stage is independently resumable: it reads only
 * FTData and the caches, and either succeeds outright or records its own
 * failure mask, so a transaction may yield between any two of them. */
static sb32 ndsMNPlayersVSPreviewPrepareResidentStage(
    s32 fkind, SYMallocRegion *anim_region, u32 stage)
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
    if (stage == NDS_PLAYERS_VS_PREPARE_OWNER_HIGH)
    {
        return ndsMNPlayersVSPreviewEnsureOwnerImage(fkind, 0u);
    }
    if (stage == NDS_PLAYERS_VS_PREPARE_OWNER_LOW)
    {
        if (ndsMNPlayersVSPreviewEnsureOwnerImage(fkind, 1u) == FALSE)
        {
            return FALSE;
        }
        gNdsPlayersVSPreviewResidentReadyMask |= kind_bit;
        return TRUE;
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
    if (stage == NDS_PLAYERS_VS_PREPARE_ANIM_IDLE)
    {
        initial_anim_file = (const void *)(uintptr_t)
            data->submotion->motion_desc[0].anim_file_id;
        if ((initial_anim_file == NULL) ||
            (ndsMNPlayersVSPreviewPreloadAnimFile(initial_anim_file,
                                                  anim_region) == FALSE))
        {
            gNdsPlayersVSPreviewResidentAnimFailMask |= kind_bit;
            return FALSE;
        }
        return TRUE;
    }
    /* mnPlayersVSGetStatusSelected chooses the demo pose scSubsysFighterSetStatus
     * applies once the puck lands (mnplayersvs.c:1553-1593): Win1..Win4 select
     * submotion rows 1..4 (DemoNull is row 0, warmed above). Yoshi/Purin/Ness
     * take Win2, so their Selected clip (Yoshi file 444) is first touched at
     * selection time unless it is resident now. The Selected clips live in the
     * compiled-in tables answered by ndsBattleShipLoadCSSSelectedFigatree, the
     * same path the force loader takes before any NitroFS/fallback accounting,
     * so a hit here keeps ndsRelocForceFighterAnimFallbackCount at zero on
     * selection. Heap NULL probes residency without copying; a non-resident
     * row falls back to the same cache preload as row 0. */
    {
        s32 selected_status = mnPlayersVSGetStatusSelected(fkind);
        s32 selected_row = selected_status - nFTDemoStatusNull;
        const void *selected_anim_file;

        if ((selected_row <= 0) || (selected_row > 4))
        {
            gNdsPlayersVSPreviewResidentAnimFailMask |= kind_bit;
            return FALSE;
        }
        selected_anim_file = (const void *)(uintptr_t)
            data->submotion->motion_desc[selected_row].anim_file_id;
        if (selected_anim_file == NULL)
        {
            gNdsPlayersVSPreviewResidentAnimFailMask |= kind_bit;
            return FALSE;
        }
        if (ndsBattleShipLoadCSSSelectedFigatree(selected_anim_file, NULL) ==
            NULL)
        {
            if (ndsMNPlayersVSPreviewPreloadAnimFile(
                    selected_anim_file, anim_region) == FALSE)
            {
                gNdsPlayersVSPreviewResidentAnimFailMask |= kind_bit;
                return FALSE;
            }
        }
    }

    return TRUE;
}

#if !NDS_PLAYERS_VS_COMPACT_PREVIEW
/* The source/oracle profiles still want the whole preparation in one call; the
 * stages are the same stages and run in the same order. */
static sb32 ndsMNPlayersVSPreviewPrepareResidentKind(
    s32 fkind, SYMallocRegion *anim_region)
{
    u32 stage;

    for (stage = NDS_PLAYERS_VS_PREPARE_ANIM_IDLE;
         stage < NDS_PLAYERS_VS_PREPARE_READY; stage++)
    {
        if (ndsMNPlayersVSPreviewPrepareResidentStage(fkind, anim_region,
                                                       stage) == FALSE)
        {
            return FALSE;
        }
    }
    return TRUE;
}
#endif

#if NDS_PLAYERS_VS_COMPACT_PREVIEW
static u32 ndsMNPlayersVSPreviewOwnerImageBytes(s32 fkind, u32 use_low_detail)
{
#if NDS_P2_LUIGI
    if (fkind == nFTKindLuigi)
    {
        return (use_low_detail != 0u) ? (u32)sizeof(NDSNativeLuigiLowImage) :
                                        (u32)sizeof(NDSNativeLuigiHighImage);
    }
#endif
#if NDS_P2_DONKEY
    if (fkind == nFTKindDonkey)
    {
        return (use_low_detail != 0u) ? (u32)sizeof(NDSNativeDonkeyLowImage) :
                                        (u32)sizeof(NDSNativeDonkeyHighImage);
    }
#endif
#if NDS_P2_CAPTAIN
    if (fkind == nFTKindCaptain)
    {
        return (use_low_detail != 0u) ? (u32)sizeof(NDSNativeCaptainLowImage) :
                                        (u32)sizeof(NDSNativeCaptainHighImage);
    }
#endif
#if NDS_P2_SAMUS
    if (fkind == nFTKindSamus)
    {
        return (use_low_detail != 0u) ? (u32)sizeof(NDSNativeSamusLowImage) :
                                        (u32)sizeof(NDSNativeSamusHighImage);
    }
#endif
#if NDS_P2_LINK
    if (fkind == nFTKindLink)
    {
        return (use_low_detail != 0u) ? (u32)sizeof(NDSNativeLinkLowImage) :
                                        (u32)sizeof(NDSNativeLinkHighImage);
    }
#endif
#if NDS_P2_PIKACHU
    if (fkind == nFTKindPikachu)
    {
        return (use_low_detail != 0u) ? (u32)sizeof(NDSNativePikachuLowImage) :
                                        (u32)sizeof(NDSNativePikachuHighImage);
    }
#endif
#if NDS_P2_YOSHI
    if (fkind == nFTKindYoshi)
    {
        return (use_low_detail != 0u) ? (u32)sizeof(NDSNativeYoshiLowImage) :
                                        (u32)sizeof(NDSNativeYoshiHighImage);
    }
#endif
#if NDS_P2_NESS
    if (fkind == nFTKindNess)
    {
        return (use_low_detail != 0u) ? (u32)sizeof(NDSNativeNessLowImage) :
                                        (u32)sizeof(NDSNativeNessHighImage);
    }
#endif
#if NDS_P2_PURIN
    if (fkind == nFTKindPurin)
    {
        return (use_low_detail != 0u) ? (u32)sizeof(NDSNativePurinLowImage) :
                                        (u32)sizeof(NDSNativePurinHighImage);
    }
#endif
#if NDS_P2_KIRBY
    if (fkind == nFTKindKirby)
    {
        return (use_low_detail != 0u) ? (u32)sizeof(NDSNativeKirbyLowImage) :
                                        (u32)sizeof(NDSNativeKirbyHighImage);
    }
#endif
    (void)fkind;
    (void)use_low_detail;
    return 0u;
}

static sb32 ndsMNPlayersVSPreviewOwnerImagesFit(
    const NDSPlayersVSResidentBlock *block, s32 fkind)
{
    uintptr_t cursor;
    uintptr_t end;
    u32 detail;

    if ((block == NULL) || (block->arena.ptr == NULL) ||
        (block->arena.end == NULL))
    {
        return FALSE;
    }
    cursor = (uintptr_t)block->arena.ptr;
    end = (uintptr_t)block->arena.end;
    for (detail = 0u; detail < 2u; detail++)
    {
        u32 bytes = ndsMNPlayersVSPreviewOwnerImageBytes(fkind, detail);
        uintptr_t aligned;

        if (bytes == 0u)
        {
            continue;
        }
        aligned = (cursor + 15u) & ~(uintptr_t)15u;
        if ((aligned < cursor) || (aligned > end) ||
            ((uintptr_t)bytes > end - aligned))
        {
            return FALSE;
        }
        cursor = aligned + (uintptr_t)bytes;
    }
    return TRUE;
}
#endif

static void ndsMNPlayersVSPreviewPrepareResidentKinds(void)
{
    u32 i;

    gNdsPlayersVSPreviewResidentPrepareMask = 0u;
    gNdsPlayersVSPreviewResidentReadyMask = 0u;
    gNdsPlayersVSPreviewResidentMainFailMask = 0u;
    gNdsPlayersVSPreviewResidentSubmotionFailMask = 0u;
    gNdsPlayersVSPreviewResidentAnimFailMask = 0u;
    gNdsPlayersVSPreviewResidentOwnerFailMask = 0u;
    gNdsPlayersVSPreviewRebuildCount = 0u;
    gNdsPlayersVSPreviewRebuildPayloadReadCount = 0u;
    gNdsPlayersVSPreviewRebuildPayloadReadMax = 0u;
    gNdsPlayersVSPreviewAcquireCount = 0u;
    gNdsPlayersVSPreviewAcquireHitCount = 0u;
    gNdsPlayersVSPreviewAcquireCachedHitCount = 0u;
    gNdsPlayersVSPreviewAcquireLoadCount = 0u;
    gNdsPlayersVSPreviewAcquireLoadFinishCount = 0u;
    gNdsPlayersVSPreviewAcquirePayloadReadCount = 0u;
    gNdsPlayersVSPreviewAcquirePayloadReadMax = 0u;
    gNdsPlayersVSPreviewAcquireTreePayloadByteCount = 0u;
    gNdsPlayersVSPreviewAcquireTreePayloadByteMax = 0u;
    gNdsPlayersVSPreviewAcquireRetryCount = 0u;
    gNdsPlayersVSPreviewAcquireFailCount = 0u;
    gNdsPlayersVSPreviewReleaseCount = 0u;
    gNdsPlayersVSPreviewReleaseLastRefCount = 0u;
    gNdsPlayersVSPreviewReleaseRetireBeginCount = 0u;
    gNdsPlayersVSPreviewReleaseRetireCount = 0u;
    gNdsPlayersVSPreviewReleaseReuseRetireCount = 0u;
    gNdsPlayersVSPreviewReleaseExitRetireCount = 0u;
    gNdsPlayersVSPreviewReleaseExitResidualRefCount = 0u;
    gNdsPlayersVSPreviewDwellRequestCount = 0u;
    gNdsPlayersVSPreviewDwellSkipCount = 0u;
    gNdsPlayersVSPreviewDwellHoldTicCount = 0u;
    gNdsPlayersVSPreviewDwellCommitCount = 0u;
    gNdsPlayersVSPreviewServiceStepCount = 0u;
    gNdsPlayersVSPreviewServiceByteMax = 0u;
    gNdsPlayersVSPreviewServiceUpdateByteMax = 0u;
    gNdsPlayersVSPreviewStaleCommitRejectCount = 0u;
    gNdsPlayersVSPreviewWarmCommitCount = 0u;
    gNdsPlayersVSPreviewPrepareStageCount = 0u;
    gNdsPlayersVSPreviewLatencyWarmMax = 0u;
    gNdsPlayersVSPreviewLatencyColdMax = 0u;
    for (i = 0u; i < NDS_PLAYERS_VS_PREVIEW_LATENCY_BUCKETS; i++)
    {
        gNdsPlayersVSPreviewLatencyWarmBuckets[i] = 0u;
        gNdsPlayersVSPreviewLatencyColdBuckets[i] = 0u;
    }
}

void ndsMNPlayersClearPreviewFighterFiles(s32 fkind)
{
    FTData *data;

    if ((fkind < nFTKindPlayableStart) || (fkind > nFTKindPlayableEnd))
    {
        return;
    }
    data = dFTManagerDataFiles[fkind];
    if (data == NULL)
    {
        return;
    }
#define NDS_CSS_CLEAR_FILE(field_)                                             \
    do                                                                         \
    {                                                                          \
        if (data->field_ != NULL)                                              \
        {                                                                      \
            *data->field_ = NULL;                                              \
        }                                                                      \
    } while (0)
    NDS_CSS_CLEAR_FILE(p_file_main);
    NDS_CSS_CLEAR_FILE(p_file_mainmotion);
    NDS_CSS_CLEAR_FILE(p_file_submotion);
    NDS_CSS_CLEAR_FILE(p_file_model);
    NDS_CSS_CLEAR_FILE(p_file_special1);
    NDS_CSS_CLEAR_FILE(p_file_special2);
    NDS_CSS_CLEAR_FILE(p_file_special3);
    NDS_CSS_CLEAR_FILE(p_file_special4);
#undef NDS_CSS_CLEAR_FILE
    /* BattleShip stores the shield-pose file itself in this field at setup
     * time rather than dereferencing the pointer holder used by the siblings. */
    data->p_file_shieldpose = NULL;
}

static sb32 ndsMNPlayersVSPreviewPoolFail(u32 stage)
{
    gNdsPlayersVSPreviewPoolFailCount++;
    gNdsPlayersVSPreviewPoolFailStage = stage;
    return FALSE;
}

static sb32 ndsMNPlayersVSPreviewInitResidentPools(void)
{
    SYMallocRegion *previous;
    u32 i;

    if ((sNdsPlayersVSResidentPoolGeneration != gNdsTaskmanHeapGeneration) ||
        (sNdsPlayersVSSharedResidentBase == NULL))
    {
        sNdsPlayersVSSharedResidentBase =
            syTaskmanMalloc(NDS_PLAYERS_VS_SHARED_RESIDENT_BYTES, 0x10u);
        for (i = 0u; i < NDS_PLAYERS_VS_RESIDENT_BLOCKS; i++)
        {
            sNdsPlayersVSResidentBlocks[i].base =
                syTaskmanMalloc(NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES, 0x10u);
        }
        sNdsPlayersVSResidentPoolGeneration = gNdsTaskmanHeapGeneration;
    }
    if (sNdsPlayersVSSharedResidentBase == NULL)
    {
        return ndsMNPlayersVSPreviewPoolFail(1u);
    }

    syMallocInit(&sNdsPlayersVSSharedResidentArena, 0x43535300u,
                 sNdsPlayersVSSharedResidentBase,
                 NDS_PLAYERS_VS_SHARED_RESIDENT_BYTES);
    for (i = 0u; i < NDS_PLAYERS_VS_RESIDENT_BLOCKS; i++)
    {
        if (sNdsPlayersVSResidentBlocks[i].base == NULL)
        {
            return ndsMNPlayersVSPreviewPoolFail(2u);
        }
        syMallocInit(&sNdsPlayersVSResidentBlocks[i].arena, 0x43535310u + i,
                     sNdsPlayersVSResidentBlocks[i].base,
                     NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES);
        sNdsPlayersVSResidentBlocks[i].fkind = nFTKindNull;
        sNdsPlayersVSResidentBlocks[i].refs = 0u;
        sNdsPlayersVSResidentBlocks[i].load_cursor = NULL;
        sNdsPlayersVSResidentBlocks[i].load_root = NULL;
        sNdsPlayersVSResidentBlocks[i].loading_fkind = nFTKindNull;
        sNdsPlayersVSResidentBlocks[i].load_tree_done = FALSE;
        sNdsPlayersVSResidentBlocks[i].cancel_requested = FALSE;
        sNdsPlayersVSResidentBlocks[i].load_stage =
            NDS_PLAYERS_VS_PREPARE_COMMIT;
        sNdsPlayersVSResidentBlocks[i].load_generation = 0u;
    }

    previous = ndsTaskmanSwapMallocRegion(&sNdsPlayersVSSharedResidentArena);
    for (i = 0u; i < ARRAY_COUNT(sNdsPlayersVSSharedResidentAssetIDs); i++)
    {
        if (ndsRelocLoadExternTreeAssetID(
                sNdsPlayersVSSharedResidentAssetIDs[i]) == FALSE)
        {
            ndsTaskmanSwapMallocRegion(previous);
            ndsRelocReleaseHeapRange(sNdsPlayersVSSharedResidentBase,
                                     NDS_PLAYERS_VS_SHARED_RESIDENT_BYTES);
            syMallocReset(&sNdsPlayersVSSharedResidentArena);
            return ndsMNPlayersVSPreviewPoolFail(3u);
        }
    }
    ndsTaskmanSwapMallocRegion(previous);
    /* The four closure blocks now have their fixed space and the shared tree is
     * pinned. Reserve every CSS row-0 animation before the first lazy per-kind
     * acquisition can run; the cache sizer uses immutable source descriptors
     * and the same payload provider as the actual warm loader. */
    if (ndsR2AnimCacheReserveCSSWorkingSet() == FALSE)
    {
        return ndsMNPlayersVSPreviewPoolFail(4u);
    }
    return TRUE;
}

static void ndsMNPlayersVSPreviewResetResidentLoadState(
    NDSPlayersVSResidentBlock *block)
{
    if (block == NULL)
    {
        return;
    }
    block->load_cursor = NULL;
    block->load_root = NULL;
    block->loading_fkind = nFTKindNull;
    block->load_tree_done = FALSE;
    block->cancel_requested = FALSE;
    block->load_stage = NDS_PLAYERS_VS_PREPARE_COMMIT;
    block->load_generation = 0u;
}

static sb32 ndsMNPlayersVSPreviewCancelResidentLoad(
    NDSPlayersVSResidentBlock *block)
{
    s32 fkind;

    if ((block == NULL) || (block->load_cursor == NULL))
    {
        return FALSE;
    }
    fkind = block->loading_fkind;
    /* Drop the transaction's own state FIRST: the compact form owns an open
     * file handle and a staged completion, and both must be gone before the
     * arena underneath them is reset. The raw-tree form owns only its cursor.
     * Everything after this line -- temporary registrations, native owner
     * staging, the heap range and the block arena -- is released identically
     * for a cancelled load and a retired one. */
#if NDS_PLAYERS_VS_COMPACT_PREVIEW
    ndsRelocPreviewFighterLoadCancel(block->load_cursor, fkind);
#else
    ndsRelocExternTreeSliceCancel(block->load_cursor);
#endif
    ndsMNPlayersClearPreviewFighterFiles(fkind);
    ndsRelocReleasePreviewFighter(fkind);
    ndsRendererNativeReleaseOwnerImagesInRange(
        block->base, NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES);
    /* The owner-image release above clears one alias into these bytes; the
     * hardware texture cache is the other, and it was being left behind. Its
     * keys are image POINTERS compared by memcmp, so an entry built from this
     * preview survives the arena reset, keeps holding VRAM, and can be taken
     * as a HIT by whichever fighter's pack is allocated at the same address
     * next -- which draws the retired fighter's texels. Releasing here also
     * stops retired previews from occupying slots that the character select,
     * the only screen drawing four fighters with no pinned corpus, needs a
     * takeable victim among. Every site that frees this range does it. */
    (void)ndsRendererHardwareReleaseTexturesInRange(
        block->base, NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES);
    ndsRelocReleaseHeapRange(block->base, NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES);
    syMallocReset(&block->arena);
    ndsMNPlayersVSPreviewResetResidentLoadState(block);
    gNdsPlayersVSPreviewAcquireLoadFinishCount++;
    return TRUE;
}

static void ndsMNPlayersVSPreviewRequestLoadCancel(s32 fkind,
                                                    u32 changing_slot)
{
    u32 i;

    /* One in-flight closure can satisfy multiple slots selecting the same
     * kind. A slot abandoning it is only a cancellation when nobody else is
     * still parked on that acquire. */
    for (i = 0u; i < ARRAY_COUNT(sNdsPlayersVSPreviewPending); i++)
    {
        if ((i != changing_slot) &&
            (sNdsPlayersVSPreviewPending[i].acquire_pending != FALSE) &&
            (sNdsPlayersVSPreviewPending[i].fkind == fkind))
        {
            return;
        }
    }
    for (i = 0u; i < NDS_PLAYERS_VS_RESIDENT_BLOCKS; i++)
    {
        NDSPlayersVSResidentBlock *block = &sNdsPlayersVSResidentBlocks[i];

        if ((block->load_cursor != NULL) && (block->loading_fkind == fkind))
        {
            block->cancel_requested = TRUE;
            return;
        }
    }
}

static void ndsMNPlayersVSPreviewServiceLoadCancel(void)
{
    u32 i;
    u32 pending_index;

    if (sNdsPlayersVSPreviewResidencyActionBudget == 0u)
    {
        return;
    }
    for (i = 0u; i < NDS_PLAYERS_VS_RESIDENT_BLOCKS; i++)
    {
        NDSPlayersVSResidentBlock *block = &sNdsPlayersVSResidentBlocks[i];

        if ((block->load_cursor != NULL) &&
            (block->cancel_requested != FALSE))
        {
            for (pending_index = 0u;
                 pending_index < ARRAY_COUNT(sNdsPlayersVSPreviewPending);
                 pending_index++)
            {
                if ((sNdsPlayersVSPreviewPending[pending_index].acquire_pending !=
                     FALSE) &&
                    (sNdsPlayersVSPreviewPending[pending_index].fkind ==
                     block->loading_fkind))
                {
                    block->cancel_requested = FALSE;
                    break;
                }
            }
            if (block->cancel_requested == FALSE)
            {
                continue;
            }
            sNdsPlayersVSPreviewResidencyActionBudget--;
            (void)ndsMNPlayersVSPreviewCancelResidentLoad(block);
            return;
        }
    }
}

#if !NDS_PLAYERS_VS_COMPACT_PREVIEW
static sb32 ndsMNPlayersVSPreviewBindResidentKindMain(s32 fkind,
                                                       void *main_file)
{
    FTData *data;

    if ((fkind < nFTKindPlayableStart) || (fkind > nFTKindPlayableEnd) ||
        (main_file == NULL))
    {
        return FALSE;
    }
    data = dFTManagerDataFiles[fkind];
    if ((data == NULL) || (data->p_file_main == NULL))
    {
        return FALSE;
    }

    /* This is the source MainKind tail after lbRelocGetExternHeapFile. The
     * sliced backend already produced that root and its complete extern tree,
     * so publishing it here avoids allocating/loading the closure a second
     * time while preserving the particle-bank and Kind bindings. */
    *data->p_file_main = main_file;
    if (data->particles_script_lo != 0x0)
    {
        *data->p_particle = efParticleGetLoadBankID(
            data->particles_script_lo, data->particles_script_hi,
            data->particles_texture_lo, data->particles_texture_hi);
    }
    ftManagerSetupFilesKind(fkind);

    /* Preserve the DS wrapper's post-setup work. Its BattleShip base sees the
     * now-published Main pointer and therefore does not perform another load. */
    ftManagerSetupFilesAllKind(fkind);
    return (*data->p_file_main != NULL) ? TRUE : FALSE;
}
#endif

static sb32 ndsMNPlayersVSPreviewRetireResidentBlock(
    NDSPlayersVSResidentBlock *block, NDSPlayersVSResidentRetireReason reason)
{
    s32 fkind;

    if ((block == NULL) || (block->refs != 0u) ||
        (block->fkind < nFTKindPlayableStart) ||
        (block->fkind > nFTKindPlayableEnd))
    {
        return FALSE;
    }
    fkind = block->fkind;
    gNdsPlayersVSPreviewReleaseRetireBeginCount++;
    ndsMNPlayersClearPreviewFighterFiles(fkind);
    ndsRelocReleasePreviewFighter(fkind);
    ndsRendererNativeReleaseOwnerImagesInRange(
        block->base, NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES);
    /* The owner-image release above clears one alias into these bytes; the
     * hardware texture cache is the other, and it was being left behind. Its
     * keys are image POINTERS compared by memcmp, so an entry built from this
     * preview survives the arena reset, keeps holding VRAM, and can be taken
     * as a HIT by whichever fighter's pack is allocated at the same address
     * next -- which draws the retired fighter's texels. Releasing here also
     * stops retired previews from occupying slots that the character select,
     * the only screen drawing four fighters with no pinned corpus, needs a
     * takeable victim among. Every site that frees this range does it. */
    (void)ndsRendererHardwareReleaseTexturesInRange(
        block->base, NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES);
    ndsRelocReleaseHeapRange(block->base, NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES);
    syMallocReset(&block->arena);
    block->fkind = nFTKindNull;
    gNdsPlayersVSPreviewResidentReadyMask &= ~(1u << fkind);
    gNdsPlayersVSPreviewReleaseRetireCount++;
    if (reason == nNDSPlayersVSResidentRetireExit)
    {
        gNdsPlayersVSPreviewReleaseExitRetireCount++;
    }
    else
    {
        gNdsPlayersVSPreviewReleaseReuseRetireCount++;
    }
    return TRUE;
}

#if NDS_PLAYERS_VS_COMPACT_PREVIEW
/* Release everything a partially built transaction owns. This is the same
 * sequence the successful path never runs and the cancel path always runs; it
 * is written out here rather than shared with CancelResidentLoad because that
 * helper is keyed on `load_cursor`, which COMMIT has already consumed. */
static void ndsMNPlayersVSPreviewAbandonCompactLoad(
    NDSPlayersVSResidentBlock *block, s32 fkind)
{
    ndsMNPlayersClearPreviewFighterFiles(fkind);
    ndsRelocReleasePreviewFighter(fkind);
    ndsRendererNativeReleaseOwnerImagesInRange(
        block->base, NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES);
    /* The owner-image release above clears one alias into these bytes; the
     * hardware texture cache is the other, and it was being left behind. Its
     * keys are image POINTERS compared by memcmp, so an entry built from this
     * preview survives the arena reset, keeps holding VRAM, and can be taken
     * as a HIT by whichever fighter's pack is allocated at the same address
     * next -- which draws the retired fighter's texels. Releasing here also
     * stops retired previews from occupying slots that the character select,
     * the only screen drawing four fighters with no pinned corpus, needs a
     * takeable victim among. Every site that frees this range does it. */
    (void)ndsRendererHardwareReleaseTexturesInRange(
        block->base, NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES);
    ndsRelocReleaseHeapRange(block->base, NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES);
    syMallocReset(&block->arena);
    ndsMNPlayersVSPreviewResetResidentLoadState(block);
}

/* The compact transaction: REQUEST -> READ/DECODE/RELOCATE -> COMMIT ->
 * PREPARE -> READY.
 *
 * Every return point has already restored the caller's malloc region, and
 * nothing this builds is reachable as an authoritative global fighter-file
 * pointer until COMMIT runs ftManagerSetupFilesAllKind, which publishes the
 * whole closure at the source's own owner boundary. The BGM fence the
 * single-call form wrapped around all of this is gone: no unit below can hold
 * the main thread -- and therefore the filesystem mutex a stream refill needs
 * -- for longer than one bounded span. */
static NDSPlayersVSResidentAcquireResult
ndsMNPlayersVSPreviewServiceCompactLoad(NDSPlayersVSResidentBlock *block,
                                        s32 fkind)
{
    FTData *data;
    SYMallocRegion *previous;
    u32 payload_before;
    u32 payload_delta;
    u32 step_bytes;
    u32 unit_bytes = 0u;
    u32 budget;
    s32 step;
    sb32 drain = sNdsPlayersVSPreviewEntryDrain;
    sb32 ok;

    if (block->load_cursor == NULL)
    {
        /* REQUEST. Opening the transaction reads the pack header and section
         * table -- under 200 bytes -- and moves no payload. A cursor that is
         * abandoned on the next update therefore costs nothing but the open. */
        if (sNdsPlayersVSPreviewResidencyActionBudget == 0u)
        {
            gNdsPlayersVSPreviewAcquireRetryCount++;
            return nNDSPlayersVSResidentAcquireRetry;
        }
        sNdsPlayersVSPreviewResidencyActionBudget--;
        data = dFTManagerDataFiles[fkind];
        gNdsPlayersVSPreviewAcquireLoadCount++;
        syMallocReset(&block->arena);
        previous = ndsTaskmanSwapMallocRegion(&block->arena);
        block->load_cursor = (data != NULL) ?
            ndsRelocPreviewFighterLoadBegin(fkind) : NULL;
        ndsTaskmanSwapMallocRegion(previous);
        if (block->load_cursor == NULL)
        {
            ndsRelocReleaseHeapRange(block->base,
                                     NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES);
            syMallocReset(&block->arena);
            ndsMNPlayersVSPreviewResetResidentLoadState(block);
            ndsMNPlayersVSPreviewMarkPermanentFailure(fkind);
            gNdsPlayersVSPreviewAcquireFailCount++;
            gNdsPlayersVSPreviewAcquireLoadFinishCount++;
            return nNDSPlayersVSResidentAcquireFail;
        }
        block->loading_fkind = fkind;
        block->load_root = NULL;
        block->load_tree_done = FALSE;
        block->cancel_requested = FALSE;
        block->load_stage = NDS_PLAYERS_VS_PREPARE_COMMIT;
        block->load_generation = gNdsTaskmanHeapGeneration;
        gNdsPlayersVSPreviewAcquireRetryCount++;
        return nNDSPlayersVSResidentAcquireRetry;
    }

    if (block->load_generation != gNdsTaskmanHeapGeneration)
    {
        /* A scene rewind ran under an in-flight transaction: its arena and
         * every pointer it built belong to the previous generation, so this
         * completion is stale and must never reach a slot. */
        gNdsPlayersVSPreviewStaleCommitRejectCount++;
        (void)ndsMNPlayersVSPreviewCancelResidentLoad(block);
        gNdsPlayersVSPreviewAcquireFailCount++;
        return nNDSPlayersVSResidentAcquireFail;
    }

    if (block->load_tree_done == FALSE)
    {
        budget = NDS_PLAYERS_VS_LOAD_STEP_BYTES;
        if (drain == FALSE)
        {
            if (sNdsPlayersVSPreviewServiceByteBudget == 0u)
            {
                gNdsPlayersVSPreviewAcquireRetryCount++;
                return nNDSPlayersVSResidentAcquireRetry;
            }
            if (budget > sNdsPlayersVSPreviewServiceByteBudget)
            {
                budget = sNdsPlayersVSPreviewServiceByteBudget;
            }
        }
        payload_before = gNdsRelocAssetPayloadReadCount;
        previous = ndsTaskmanSwapMallocRegion(&block->arena);
        do
        {
            step_bytes = 0u;
            step = ndsRelocPreviewFighterLoadStep(block->load_cursor,
                                                  (drain != FALSE) ? 0u : budget,
                                                  &step_bytes);
            gNdsPlayersVSPreviewServiceStepCount++;
            unit_bytes += step_bytes;
            if (step_bytes > gNdsPlayersVSPreviewServiceByteMax)
            {
                gNdsPlayersVSPreviewServiceByteMax = step_bytes;
            }
            if (drain == FALSE)
            {
                /* SPEND THE AGGREGATE BUDGET, WHICH IS WHAT IT IS FOR.
                 *
                 * The loop's continuation used to be gated on `drain`, which
                 * is entry-only, so a browsing cursor got exactly ONE
                 * NDS_PLAYERS_VS_LOAD_STEP_BYTES unit per CSS update and left
                 * three quarters of sNdsPlayersVSPreviewServiceByteBudget
                 * unspent every tic. That is the owner's hover delay: the
                 * aggregate span is a property of the UPDATE, not of a unit.
                 *
                 * Each unit is still capped at NDS_PLAYERS_VS_LOAD_STEP_BYTES,
                 * so the audio-gap bound that constant exists to enforce is
                 * unchanged; only the number of units inside one update moves.
                 * A unit that moved nothing cannot extend the loop, because
                 * such a step can only be followed by DONE. */
                u32 remaining =
                    (sNdsPlayersVSPreviewServiceByteBudget > unit_bytes) ?
                        (sNdsPlayersVSPreviewServiceByteBudget - unit_bytes) :
                        0u;

                budget = (remaining > NDS_PLAYERS_VS_LOAD_STEP_BYTES) ?
                    (u32)NDS_PLAYERS_VS_LOAD_STEP_BYTES : remaining;
                if (budget == 0u)
                {
                    break;
                }
            }
        } while (step == NDS_PREVIEW_PACK_STEP_IN_PROGRESS);
        ndsTaskmanSwapMallocRegion(previous);
        if (drain == FALSE)
        {
            sNdsPlayersVSPreviewServiceByteBudget =
                (sNdsPlayersVSPreviewServiceByteBudget > unit_bytes) ?
                    (sNdsPlayersVSPreviewServiceByteBudget - unit_bytes) : 0u;
        }
        payload_delta = gNdsRelocAssetPayloadReadCount - payload_before;
        gNdsPlayersVSPreviewAcquirePayloadReadCount += payload_delta;
        if (payload_delta > gNdsPlayersVSPreviewAcquirePayloadReadMax)
        {
            gNdsPlayersVSPreviewAcquirePayloadReadMax = payload_delta;
        }
        gNdsPlayersVSPreviewAcquireTreePayloadByteCount += unit_bytes;
        if (unit_bytes > gNdsPlayersVSPreviewAcquireTreePayloadByteMax)
        {
            gNdsPlayersVSPreviewAcquireTreePayloadByteMax = unit_bytes;
        }
        if (step == NDS_PREVIEW_PACK_STEP_FAIL)
        {
            (void)ndsMNPlayersVSPreviewCancelResidentLoad(block);
            ndsMNPlayersVSPreviewMarkPermanentFailure(fkind);
            gNdsPlayersVSPreviewAcquireFailCount++;
            return nNDSPlayersVSResidentAcquireFail;
        }
        if (step != NDS_PREVIEW_PACK_STEP_DONE)
        {
            gNdsPlayersVSPreviewAcquireRetryCount++;
            return nNDSPlayersVSResidentAcquireRetry;
        }
        block->load_tree_done = TRUE;
        if (drain == FALSE)
        {
            gNdsPlayersVSPreviewAcquireRetryCount++;
            return nNDSPlayersVSResidentAcquireRetry;
        }
    }

    while (block->load_stage != NDS_PLAYERS_VS_PREPARE_READY)
    {
        if ((drain == FALSE) &&
            (sNdsPlayersVSPreviewResidencyActionBudget == 0u))
        {
            gNdsPlayersVSPreviewAcquireRetryCount++;
            return nNDSPlayersVSResidentAcquireRetry;
        }
        if (drain == FALSE)
        {
            sNdsPlayersVSPreviewResidencyActionBudget--;
        }
        gNdsPlayersVSPreviewPrepareStageCount++;
        payload_before = gNdsRelocAssetPayloadReadCount;
        previous = ndsTaskmanSwapMallocRegion(&block->arena);
        if (block->load_stage == NDS_PLAYERS_VS_PREPARE_COMMIT)
        {
            /* The owner boundary, run exactly once and never in a loop: the
             * staged closure is published here, the particle bank is created
             * by the source's own guarded path, and the block is proven to
             * still have room for both native owner images. */
            data = dFTManagerDataFiles[fkind];
            if (data != NULL)
            {
                ftManagerSetupFilesAllKind(fkind);
            }
            ok = ((data != NULL) && (data->p_file_main != NULL) &&
                  (*data->p_file_main != NULL) &&
                  (data->p_file_model != NULL) &&
                  (*data->p_file_model != NULL)) ? TRUE : FALSE;
            if ((ok != FALSE) &&
                (ndsMNPlayersVSPreviewOwnerImagesFit(block, fkind) == FALSE))
            {
                gNdsPlayersVSPreviewResidentCapacityFailCount++;
                ok = FALSE;
            }
        }
        else
        {
            ok = ndsMNPlayersVSPreviewPrepareResidentStage(fkind, previous,
                                                            block->load_stage);
        }
        ndsTaskmanSwapMallocRegion(previous);
        payload_delta = gNdsRelocAssetPayloadReadCount - payload_before;
        gNdsPlayersVSPreviewAcquirePayloadReadCount += payload_delta;
        if (payload_delta > gNdsPlayersVSPreviewAcquirePayloadReadMax)
        {
            gNdsPlayersVSPreviewAcquirePayloadReadMax = payload_delta;
        }
        if (ok == FALSE)
        {
            ndsMNPlayersVSPreviewAbandonCompactLoad(block, fkind);
            ndsMNPlayersVSPreviewMarkPermanentFailure(fkind);
            gNdsPlayersVSPreviewAcquireFailCount++;
            gNdsPlayersVSPreviewAcquireLoadFinishCount++;
            return nNDSPlayersVSResidentAcquireFail;
        }
        block->load_stage++;
        if ((drain == FALSE) &&
            (block->load_stage != NDS_PLAYERS_VS_PREPARE_READY))
        {
            gNdsPlayersVSPreviewAcquireRetryCount++;
            return nNDSPlayersVSResidentAcquireRetry;
        }
    }

    /* READY. Asset closure, native owner, textures, first pose and reference
     * ownership are all in place before the block names its kind, so no slot
     * can ever draw a half-relocated preview. */
    block->fkind = fkind;
    block->refs = 1u;
    ndsMNPlayersVSPreviewResetResidentLoadState(block);
    gNdsPlayersVSPreviewAcquireLoadFinishCount++;
    return nNDSPlayersVSResidentAcquireReady;
}
#endif

static NDSPlayersVSResidentAcquireResult
ndsMNPlayersVSPreviewAcquireResidentKind(s32 fkind)
{
    NDSPlayersVSResidentBlock *block = NULL;
    NDSPlayersVSResidentBlock *victim = NULL;
#if !NDS_PLAYERS_VS_COMPACT_PREVIEW
    FTData *data;
    SYMallocRegion *previous;
    u32 payload_before;
    u32 payload_delta;
    u32 payload_bytes = 0u;
    s32 slice_result;
    sb32 prepared;
#endif
    u32 i;

    if ((fkind < nFTKindPlayableStart) || (fkind > nFTKindPlayableEnd) ||
        (sNdsPlayersVSResidentPoolsReady == FALSE))
    {
        gNdsPlayersVSPreviewAcquireFailCount++;
        return nNDSPlayersVSResidentAcquireFail;
    }
    if (ndsMNPlayersVSPreviewIsPermanentFailure(fkind) != FALSE)
    {
        return nNDSPlayersVSResidentAcquireFail;
    }
    gNdsPlayersVSPreviewAcquireCount++;
    for (i = 0u; i < NDS_PLAYERS_VS_RESIDENT_BLOCKS; i++)
    {
        NDSPlayersVSResidentBlock *candidate =
            &sNdsPlayersVSResidentBlocks[i];

        /* Zero-ref closures remain valid cache entries until their fixed block
         * is actually needed for another kind. Re-selecting one is therefore
         * the same cheap refcount hit as sharing an already-live kind. */
        if (candidate->fkind == fkind)
        {
            gNdsPlayersVSPreviewAcquireHitCount++;
            if (candidate->refs == 0u)
            {
                gNdsPlayersVSPreviewAcquireCachedHitCount++;
            }
            candidate->refs++;
            /* M04's warm case: a ready block is reused in this same update,
             * with no byte moved and no action spent. */
            gNdsPlayersVSPreviewWarmCommitCount++;
            return nNDSPlayersVSResidentAcquireReady;
        }
        if (candidate->load_cursor != NULL)
        {
            if (candidate->loading_fkind == fkind)
            {
                /* This acquire is current demand for the transaction even if
                 * an earlier slot move queued its cancellation. */
                candidate->cancel_requested = FALSE;
                block = candidate;
            }
            continue;
        }
        if (candidate->refs != 0u)
        {
            continue;
        }
        if ((candidate->fkind == nFTKindNull) && (block == NULL))
        {
            block = candidate;
        }
        else if ((candidate->fkind != nFTKindNull) && (victim == NULL))
        {
            victim = candidate;
        }
    }
    if (block == NULL)
    {
        /* All four bytes ranges are preallocated forever; a zero-ref cached
         * occupant is the only thing that can make a free range look busy.
         * Retire at most one such occupant this tic, then let the next tic do
         * the replacement load so scan-heavy retirement and NitroFS never
         * stack on the same successful browsing frame. */
        if (victim != NULL)
        {
            if (sNdsPlayersVSPreviewResidencyActionBudget == 0u)
            {
                gNdsPlayersVSPreviewAcquireRetryCount++;
                return nNDSPlayersVSResidentAcquireRetry;
            }
            sNdsPlayersVSPreviewResidencyActionBudget--;
            if (ndsMNPlayersVSPreviewRetireResidentBlock(
                    victim, nNDSPlayersVSResidentRetireReuse) == FALSE)
            {
                gNdsPlayersVSPreviewAcquireRetryCount++;
                return nNDSPlayersVSResidentAcquireRetry;
            }
            gNdsPlayersVSPreviewAcquireRetryCount++;
            return nNDSPlayersVSResidentAcquireRetry;
        }
        /* All four closures are referenced by live slots. Ownership can change
         * on a later CSS tic, so this is resource contention, not a permanent
         * fighter failure. */
        gNdsPlayersVSPreviewAcquireRetryCount++;
        return nNDSPlayersVSResidentAcquireRetry;
    }
#if NDS_PLAYERS_VS_COMPACT_PREVIEW
    /* Native CSS only needs Main/Model plus the row-0/Selected animations that
     * the preparation stages warm explicitly. Keep the compact pack and its
     * owner-image pair in this resettable block; the animation cache has its
     * own pre-reserved arena. Retirement invalidates those image bindings before
     * the block is reused, so browsing history cannot accumulate arena bytes. */
    return ndsMNPlayersVSPreviewServiceCompactLoad(block, fkind);
#else
    if (sNdsPlayersVSPreviewResidencyActionBudget == 0u)
    {
        gNdsPlayersVSPreviewAcquireRetryCount++;
        return nNDSPlayersVSResidentAcquireRetry;
    }

    sNdsPlayersVSPreviewResidencyActionBudget--;
    if (block->load_cursor == NULL)
    {
        /* Starting a miss is deliberately a cursor-only action. The first
         * NitroFS payload byte cannot move until a later CSS tic. */
        data = dFTManagerDataFiles[fkind];
        gNdsPlayersVSPreviewAcquireLoadCount++;
        syMallocReset(&block->arena);
        previous = ndsTaskmanSwapMallocRegion(&block->arena);
        block->load_cursor = ((data != NULL) && (data->file_main_id != 0u)) ?
            ndsRelocExternTreeSliceBegin(
                (const void *)(uintptr_t)data->file_main_id) : NULL;
        ndsTaskmanSwapMallocRegion(previous);
        if (block->load_cursor == NULL)
        {
            ndsRelocReleaseHeapRange(block->base,
                                     NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES);
            syMallocReset(&block->arena);
            ndsMNPlayersVSPreviewResetResidentLoadState(block);
            ndsMNPlayersVSPreviewMarkPermanentFailure(fkind);
            gNdsPlayersVSPreviewAcquireFailCount++;
            gNdsPlayersVSPreviewAcquireLoadFinishCount++;
            return nNDSPlayersVSResidentAcquireFail;
        }
        block->loading_fkind = fkind;
        block->load_root = NULL;
        block->load_tree_done = FALSE;
        block->cancel_requested = FALSE;
        gNdsPlayersVSPreviewAcquireRetryCount++;
        return nNDSPlayersVSResidentAcquireRetry;
    }

    if (block->load_tree_done == FALSE)
    {
        payload_before = gNdsRelocAssetPayloadReadCount;
        ndsAudioBgmSuspendForBlockingLoad();
        previous = ndsTaskmanSwapMallocRegion(&block->arena);
        slice_result = ndsRelocExternTreeSliceStep(
            block->load_cursor, NDS_PLAYERS_VS_LOAD_CHUNK_BYTES,
            NDS_PLAYERS_VS_LOAD_CHUNK_NODES, &block->load_root,
            &payload_bytes);
        ndsTaskmanSwapMallocRegion(previous);
        ndsAudioBgmResumeAfterBlockingLoad();
        payload_delta = gNdsRelocAssetPayloadReadCount - payload_before;
        gNdsPlayersVSPreviewAcquirePayloadReadCount += payload_delta;
        if (payload_delta > gNdsPlayersVSPreviewAcquirePayloadReadMax)
        {
            gNdsPlayersVSPreviewAcquirePayloadReadMax = payload_delta;
        }
        gNdsPlayersVSPreviewAcquireTreePayloadByteCount += payload_bytes;
        if (payload_bytes > gNdsPlayersVSPreviewAcquireTreePayloadByteMax)
        {
            gNdsPlayersVSPreviewAcquireTreePayloadByteMax = payload_bytes;
        }
        if (slice_result == NDS_RELOC_EXTERN_TREE_SLICE_FAIL)
        {
            (void)ndsMNPlayersVSPreviewCancelResidentLoad(block);
            ndsMNPlayersVSPreviewMarkPermanentFailure(fkind);
            gNdsPlayersVSPreviewAcquireFailCount++;
            return nNDSPlayersVSResidentAcquireFail;
        }
        if (slice_result == NDS_RELOC_EXTERN_TREE_SLICE_DONE)
        {
            if (block->load_root == NULL)
            {
                (void)ndsMNPlayersVSPreviewCancelResidentLoad(block);
                ndsMNPlayersVSPreviewMarkPermanentFailure(fkind);
                gNdsPlayersVSPreviewAcquireFailCount++;
                return nNDSPlayersVSResidentAcquireFail;
            }
            block->load_tree_done = TRUE;
        }
        gNdsPlayersVSPreviewAcquireRetryCount++;
        return nNDSPlayersVSResidentAcquireRetry;
    }

    /* The completed tree becomes visible to BattleShip only on its own final
     * residency action. This keeps Main/Kind binding + prepare off the last
     * payload tic and preserves the source setup side effects. */
    payload_before = gNdsRelocAssetPayloadReadCount;
    ndsAudioBgmSuspendForBlockingLoad();
    previous = ndsTaskmanSwapMallocRegion(&block->arena);
    prepared = ndsMNPlayersVSPreviewBindResidentKindMain(fkind,
                                                         block->load_root);
    if (prepared != FALSE)
    {
        prepared = ndsMNPlayersVSPreviewPrepareResidentKind(fkind, previous);
    }
    ndsTaskmanSwapMallocRegion(previous);
    ndsAudioBgmResumeAfterBlockingLoad();
    payload_delta = gNdsRelocAssetPayloadReadCount - payload_before;
    gNdsPlayersVSPreviewAcquirePayloadReadCount += payload_delta;
    if (payload_delta > gNdsPlayersVSPreviewAcquirePayloadReadMax)
    {
        gNdsPlayersVSPreviewAcquirePayloadReadMax = payload_delta;
    }
    if (prepared == FALSE)
    {
        (void)ndsMNPlayersVSPreviewCancelResidentLoad(block);
        ndsMNPlayersVSPreviewMarkPermanentFailure(fkind);
        gNdsPlayersVSPreviewAcquireFailCount++;
        return nNDSPlayersVSResidentAcquireFail;
    }

    block->fkind = fkind;
    block->refs = 1u;
    ndsMNPlayersVSPreviewResetResidentLoadState(block);
    gNdsPlayersVSPreviewAcquireLoadFinishCount++;
    return nNDSPlayersVSResidentAcquireReady;
#endif
}

/* M04: "warm" means this kind commits without moving a byte -- a block already
 * holds its closure, referenced or merely cached. A warm request must not pay
 * the cold-load stabilization interval, because there is no cold load to
 * protect the screen from. */
static sb32 ndsMNPlayersVSPreviewKindIsWarm(s32 fkind)
{
    u32 i;

    if ((fkind < nFTKindPlayableStart) || (fkind > nFTKindPlayableEnd))
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_PLAYERS_VS_RESIDENT_BLOCKS; i++)
    {
        if (sNdsPlayersVSResidentBlocks[i].fkind == fkind)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* Close one request's latency measurement. Bucket bounds are update counts, so
 * a capture reads the two rows and computes warm/cold P50/P95 without needing
 * a per-event stream. */
static void ndsMNPlayersVSPreviewRecordLatency(
    NDSPlayersVSPreviewPending *pending)
{
    static const u32 bounds[NDS_PLAYERS_VS_PREVIEW_LATENCY_BUCKETS - 1u] = {
        1u, 2u, 3u, 4u, 6u, 8u, 12u
    };
    volatile u32 *row;
    u32 i;

    if (pending->request_warm != FALSE)
    {
        row = gNdsPlayersVSPreviewLatencyWarmBuckets;
        if (pending->request_updates > gNdsPlayersVSPreviewLatencyWarmMax)
        {
            gNdsPlayersVSPreviewLatencyWarmMax = pending->request_updates;
        }
    }
    else
    {
        row = gNdsPlayersVSPreviewLatencyColdBuckets;
        if (pending->request_updates > gNdsPlayersVSPreviewLatencyColdMax)
        {
            gNdsPlayersVSPreviewLatencyColdMax = pending->request_updates;
        }
    }
    for (i = 0u; i < (NDS_PLAYERS_VS_PREVIEW_LATENCY_BUCKETS - 1u); i++)
    {
        if (pending->request_updates <= bounds[i])
        {
            row[i]++;
            pending->request_updates = 0u;
            return;
        }
    }
    row[NDS_PLAYERS_VS_PREVIEW_LATENCY_BUCKETS - 1u]++;
    pending->request_updates = 0u;
}

static sb32 ndsMNPlayersVSPreviewReleaseResidentKind(s32 fkind)
{
    u32 i;

    if ((fkind < nFTKindPlayableStart) || (fkind > nFTKindPlayableEnd))
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_PLAYERS_VS_RESIDENT_BLOCKS; i++)
    {
        NDSPlayersVSResidentBlock *block = &sNdsPlayersVSResidentBlocks[i];

        if ((block->refs == 0u) || (block->fkind != fkind))
        {
            continue;
        }
        block->refs--;
        /* Keep a zero-ref closure intact. The block is already part of the
         * fixed four-slot allocation, so this cannot grow memory usage; it only
         * postpones the relocation-table retirement until a real miss needs
         * this exact range for a different kind. */
        return (block->refs == 0u) ? TRUE : FALSE;
    }
    return FALSE;
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

    /* Shared reloc dependencies are the only entry-time file burst now. A
     * second CSS entry can still inherit menu BGM, so keep that bounded burst
     * under the established blocking-load fence. Per-fighter closures and
     * native owner images are acquired later by the slot that needs them. */
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

    /* BattleShip initializes the particle/effect owners before fighter setup
     * (mnPlayersVSFuncStart:4753-4757). The native-shell subset used to skip
     * both, so a CSS re-entry after Results inherited bank-0 pointers into the
     * Results taskman arena. That arena is rewound on PlayersVS entry; once a
     * preview spawned common script 91, lbParticleMakeChildScriptID dereferenced
     * the stale script-table slot and took a data abort. Recreate the source
     * scene-lifetime owners here before any preview fighter can run. */
    efParticleInitAll();
    efManagerInitEffects();
    ftManagerAllocFighter(FTDATA_FLAG_SUBMOTION, 4);
    ndsMNPlayersVSPreviewPrepareResidentKinds();
    sNdsPlayersVSResidentPoolsReady =
        ndsMNPlayersVSPreviewInitResidentPools();
    if (sNdsPlayersVSResidentPoolsReady == FALSE)
    {
        ndsAudioBgmResumeAfterBlockingLoad();
        return;
    }

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
        sNdsPlayersVSReleasedRotationY[i] = 0.0F;
        sNdsPlayersVSPreviewPending[i].fkind = nFTKindNull;
        sNdsPlayersVSPreviewPending[i].stable_tics = 0u;
        sNdsPlayersVSPreviewPending[i].seen_request = FALSE;
        sNdsPlayersVSPreviewPending[i].acquire_pending = FALSE;
        sNdsPlayersVSPreviewPending[i].request_updates = 0u;
        sNdsPlayersVSPreviewPending[i].request_warm = FALSE;
    }
    sNdsPlayersVSPreviewResidencyActionBudget = 0u;
    sNdsPlayersVSPreviewServiceByteBudget = 0u;
    sNdsPlayersVSPreviewEntryDrain = FALSE;
    sNdsPlayersVSPreviewEntrySyncPending = TRUE;
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
    if (sNdsPlayersVSPreviewEntrySyncPending != FALSE)
    {
        /* The router calls the first sync before CSS BGM starts. Preserve that
         * load-boundary behavior and allow all four initial previews to become
         * resident there. Every live CSS tic after it gets one action. */
        sNdsPlayersVSPreviewResidencyActionBudget =
            NDS_PLAYERS_VS_RESIDENT_BLOCKS;
        sNdsPlayersVSPreviewEntryDrain = TRUE;
        sNdsPlayersVSPreviewEntrySyncPending = FALSE;
    }
    else
    {
        sNdsPlayersVSPreviewResidencyActionBudget = 1u;
        sNdsPlayersVSPreviewEntryDrain = FALSE;
    }
    if ((NDS_PLAYERS_VS_PREVIEW_SERVICE_BYTES -
         sNdsPlayersVSPreviewServiceByteBudget) >
        gNdsPlayersVSPreviewServiceUpdateByteMax)
    {
        gNdsPlayersVSPreviewServiceUpdateByteMax =
            NDS_PLAYERS_VS_PREVIEW_SERVICE_BYTES -
            sNdsPlayersVSPreviewServiceByteBudget;
    }
    /* ONE aggregate span budget for the whole screen, refreshed here and then
     * spent by whichever slots have work. Four slots do not get four budgets:
     * the point of the bound is the wall-clock length of one update, and that
     * is a property of the screen, not of a slot. */
    sNdsPlayersVSPreviewServiceByteBudget =
        NDS_PLAYERS_VS_PREVIEW_SERVICE_BYTES;
    ndsMNPlayersVSPreviewServiceLoadCancel();
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

static void ndsMNPlayersVSPreviewRebuildChangedKind(u32 slot, s32 pkind,
                                                     s32 fkind,
                                                     sb32 is_selected)
{
    GObj *fighter_gobj;
    u32 payload_before;
    u32 payload_delta;

    sMNPlayersVSSlots[slot].pkind = pkind;
    sMNPlayersVSSlots[slot].fkind = fkind;
    sMNPlayersVSSlots[slot].is_selected = is_selected;
    sMNPlayersVSSlots[slot].is_fighter_selected = is_selected;

    payload_before = gNdsRelocAssetPayloadReadCount;
    mnPlayersVSUpdateFighter((s32)slot);
    payload_delta = gNdsRelocAssetPayloadReadCount - payload_before;
    gNdsPlayersVSPreviewRebuildCount++;
    gNdsPlayersVSPreviewRebuildPayloadReadCount += payload_delta;
    if (payload_delta > gNdsPlayersVSPreviewRebuildPayloadReadMax)
    {
        gNdsPlayersVSPreviewRebuildPayloadReadMax = payload_delta;
    }

    fighter_gobj = sMNPlayersVSSlots[slot].player;
    if (fighter_gobj != NULL)
    {
        DObj *root = DObjGetStruct(fighter_gobj);

        if (root != NULL)
        {
            root->rotate.vec.f.y = sNdsPlayersVSReleasedRotationY[slot];
        }
    }
    ndsMNPlayersVSPreviewApplyOuterSlotInset(slot, fighter_gobj);
    if ((fighter_gobj != NULL) &&
        ((fighter_gobj->flags & GOBJ_FLAG_HIDDEN) == 0u))
    {
        ndsFighterManagerRegisterDisplayFighter(fighter_gobj, slot);
    }
}

void ndsMNPlayersVSPreviewSync(u32 slot, s32 pkind, s32 fkind,
                               sb32 is_selected)
{
    s32 old_pkind;
    s32 old_fkind;
    sb32 old_selected;
    GObj *fighter_gobj;
    NDSPlayersVSPreviewPending *pending;
    NDSPlayersVSResidentAcquireResult acquire_result;
    sb32 initial_request = FALSE;
    sb32 update_fighter;
    u32 dwell;

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
/* Link was the one landed fighter this filter never listed, so his slot fell
 * to nFTKindNull and the character select drew him no 3D preview -- the whole
 * chain behind it was already there and reachable (setup :510-512, resident
 * prepare :354-356, his native owner slot and image row, ftmanager residency
 * at battleship_ftmanager.c:135-140). Owner playtest, 2026-09-04. */
#if NDS_P2_LINK
        && (fkind != nFTKindLink)
#endif
#if NDS_P2_PIKACHU
        && (fkind != nFTKindPikachu)
#endif
#if NDS_P2_YOSHI
        && (fkind != nFTKindYoshi)
#endif
#if NDS_P2_NESS
        && (fkind != nFTKindNess)
#endif
#if NDS_P2_PURIN
        && (fkind != nFTKindPurin)
#endif
#if NDS_P2_KIRBY
        && (fkind != nFTKindKirby)
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
    pending = &sNdsPlayersVSPreviewPending[slot];
    /* M04's clock. It starts on the update the requested kind stops matching
     * what the slot is showing -- input stabilization included, because that is
     * part of what the player waits through -- and stops at the commit below. */
    if (fkind != old_fkind)
    {
        if (pending->request_updates == 0u)
        {
            pending->request_warm = ndsMNPlayersVSPreviewKindIsWarm(fkind);
        }
        pending->request_updates++;
    }

    /* After one completed deterministic failure the panel is deliberately
     * empty and the failure masks/counter remain visible. Keep that same
     * request parked until either the requested kind changes or a new taskman
     * resource generation clears the latch. If an old fighter is still live,
     * let the ordinary commit path below release it first. */
    if ((fkind != nFTKindNull) &&
        (old_fkind == nFTKindNull) && (fighter_gobj == NULL) &&
        (ndsMNPlayersVSPreviewIsPermanentFailure(fkind) != FALSE))
    {
        pending->seen_request = TRUE;
        pending->fkind = fkind;
        pending->stable_tics = 0u;
        pending->acquire_pending = FALSE;
        pending->request_updates = 0u;
        sMNPlayersVSSlots[slot].pkind = pkind;
        sMNPlayersVSSlots[slot].fkind = nFTKindNull;
        sMNPlayersVSSlots[slot].is_selected = FALSE;
        sMNPlayersVSSlots[slot].is_fighter_selected = FALSE;
        return;
    }

    /* The router's entry sync happens before CSS BGM starts. Do not make that
     * load boundary wait 13 tics: the dwell exists for interactive roster
     * browsing, not for initial screen construction. */
    if (pending->seen_request == FALSE)
    {
        pending->seen_request = TRUE;
        pending->fkind = fkind;
        pending->stable_tics = NDS_PLAYERS_VS_PREVIEW_DWELL_TICKS;
        initial_request = TRUE;
    }

    /* A committed replacement owns no old fighter anymore. If the cursor keeps
     * the same request, resume its acquire stage. If it moved again before that
     * acquire ran, abandon the target with no residency leak and start a fresh
     * dwell from the now-empty preview panel. */
    if (pending->acquire_pending != FALSE)
    {
        if (fkind == pending->fkind)
        {
            acquire_result = ndsMNPlayersVSPreviewAcquireResidentKind(fkind);
            if (acquire_result == nNDSPlayersVSResidentAcquireRetry)
            {
                return;
            }
            pending->acquire_pending = FALSE;
            pending->stable_tics = 0u;
            if (acquire_result != nNDSPlayersVSResidentAcquireReady)
            {
                sMNPlayersVSSlots[slot].pkind = pkind;
                sMNPlayersVSSlots[slot].fkind = nFTKindNull;
                sMNPlayersVSSlots[slot].is_selected = FALSE;
                sMNPlayersVSSlots[slot].is_fighter_selected = FALSE;
                return;
            }
            ndsMNPlayersVSPreviewRecordLatency(pending);
            ndsMNPlayersVSPreviewRebuildChangedKind(slot, pkind, fkind,
                                                     is_selected);
            return;
        }
        gNdsPlayersVSPreviewDwellSkipCount++;
        ndsMNPlayersVSPreviewRequestLoadCancel(pending->fkind, slot);
        pending->acquire_pending = FALSE;
        pending->fkind = fkind;
        /* The ordinary dwell path below accounts for this same tic. Seed zero
         * here so a target changed during an acquire wait cannot count twice. */
        pending->stable_tics = 0u;
        gNdsPlayersVSPreviewDwellRequestCount++;
    }

    /* Returning to the kind that is already displayed cancels any uncommitted
     * dwell and keeps the source's same-kind player/grab events immediate. */
    if (fkind == old_fkind)
    {
        if ((pending->stable_tics != 0u) && (pending->fkind != fkind))
        {
            gNdsPlayersVSPreviewDwellSkipCount++;
        }
        if (pending->request_updates != 0u)
        {
            /* The cursor came back to what is already on screen: that IS the
             * request, resolved without a load. */
            ndsMNPlayersVSPreviewRecordLatency(pending);
        }
        pending->fkind = fkind;
        pending->stable_tics = 0u;

        sMNPlayersVSSlots[slot].pkind = pkind;
        sMNPlayersVSSlots[slot].fkind = fkind;
        sMNPlayersVSSlots[slot].is_selected = is_selected;
        sMNPlayersVSSlots[slot].is_fighter_selected = is_selected;

        update_fighter = (((fighter_gobj == NULL) &&
                           (fkind != nFTKindNull)) ||
                          (old_pkind != pkind) ||
                          ((old_selected != FALSE) &&
                           (is_selected == FALSE))) ? TRUE : FALSE;

        if (update_fighter != FALSE)
        {
            /* The source updater hides an existing object for NA/null and
             * otherwise destroys/replaces it, preserving Y rotation, costume
             * and shade rules. Clear DS registration around that operation so
             * no stale taskman-arena pointer reaches the native renderer. */
            ndsFighterManagerRegisterDisplayFighter(NULL, slot);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
            ndsFighterRendererInvalidateMaterialCachesForSlot(slot);
#endif
            {
                u32 payload_before = gNdsRelocAssetPayloadReadCount;
                u32 payload_delta;

                mnPlayersVSUpdateFighter((s32)slot);
                payload_delta =
                    gNdsRelocAssetPayloadReadCount - payload_before;
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
        return;
    }

    /* Interactive kind changes stabilize before they own residency. The old
     * 13-tic value was sized so a full-speed roster pass could never START the
     * blocking closure; now that the closure is sliced and cancellable, an
     * abandoned request costs only the units already spent, so stabilization
     * only has to reject genuine cursor transit. A kind that is already
     * resident costs nothing to show and takes the shorter warm interval.
     * A completed selection and closing an NA slot are explicit user decisions
     * and may commit sooner; even then the acquire is staged to a later tic.
     * During this interval the previous 3D fighter keeps rendering. */
    if (initial_request == FALSE)
    {
        dwell = (ndsMNPlayersVSPreviewKindIsWarm(fkind) != FALSE) ?
            NDS_PLAYERS_VS_PREVIEW_DWELL_WARM_TICKS :
            NDS_PLAYERS_VS_PREVIEW_DWELL_TICKS;
        if (pending->fkind != fkind)
        {
            if (pending->stable_tics != 0u)
            {
                gNdsPlayersVSPreviewDwellSkipCount++;
            }
            pending->fkind = fkind;
            pending->stable_tics = 1u;
            gNdsPlayersVSPreviewDwellRequestCount++;
        }
        else if (pending->stable_tics < dwell)
        {
            pending->stable_tics++;
        }
        if ((pending->stable_tics < dwell) &&
            (is_selected == FALSE) && (pkind != nFTPlayerKindNot))
        {
            gNdsPlayersVSPreviewDwellHoldTicCount++;
            return;
        }
        gNdsPlayersVSPreviewDwellCommitCount++;
    }

    /* Commit is deliberately only the ownership half of the change. Destroy
     * the old source fighter, drop its reference, and return with an empty
     * preview. The replacement acquire starts on a later tic, which guarantees
     * release-before-acquire while preventing both operations from stacking. */
    ndsFighterManagerRegisterDisplayFighter(NULL, slot);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    ndsFighterRendererInvalidateMaterialCachesForSlot(slot);
#endif
    if (fighter_gobj != NULL)
    {
        DObj *root = DObjGetStruct(fighter_gobj);

        if (root != NULL)
        {
            sNdsPlayersVSReleasedRotationY[slot] = root->rotate.vec.f.y;
        }
        ftManagerDestroyFighter(fighter_gobj);
        sMNPlayersVSSlots[slot].player = NULL;
    }
    if ((old_fkind >= nFTKindPlayableStart) &&
        (old_fkind <= nFTKindPlayableEnd))
    {
        gNdsPlayersVSPreviewReleaseCount++;
        if (ndsMNPlayersVSPreviewReleaseResidentKind(old_fkind) != FALSE)
        {
            gNdsPlayersVSPreviewReleaseLastRefCount++;
        }
    }

    sMNPlayersVSSlots[slot].pkind = pkind;
    sMNPlayersVSSlots[slot].fkind = nFTKindNull;
    sMNPlayersVSSlots[slot].is_selected = FALSE;
    sMNPlayersVSSlots[slot].is_fighter_selected = FALSE;
    pending->fkind = fkind;
    pending->stable_tics = 0u;

    if (fkind == nFTKindNull)
    {
        pending->acquire_pending = FALSE;
        /* An emptied slot has no preview to wait for, so it is not a latency
         * sample; dropping it keeps the warm/cold rows honest. */
        pending->request_updates = 0u;
        return;
    }

    /* Initial CSS construction is already a load frame and precedes BGM. Keep
     * its previous behavior; live kind changes always take the staged return
     * above and resume here on a later tic. */
    if (initial_request != FALSE)
    {
        acquire_result = ndsMNPlayersVSPreviewAcquireResidentKind(fkind);
        if (acquire_result == nNDSPlayersVSResidentAcquireReady)
        {
            ndsMNPlayersVSPreviewRecordLatency(pending);
            ndsMNPlayersVSPreviewRebuildChangedKind(slot, pkind, fkind,
                                                     is_selected);
            return;
        }
        if (acquire_result == nNDSPlayersVSResidentAcquireFail)
        {
            pending->acquire_pending = FALSE;
            return;
        }
    }
    pending->acquire_pending = TRUE;
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
        /* taskman.c:1093-1100 rewinds the graphics heap and the four DL heads
         * before EVERY source scene draw; this loop drew without doing either.
         * The preview camera's own capture proc emits about seventeen Gfx
         * words a draw, nothing on DS executes or rewinds them, and the heads
         * walked up the arena until they reached live GObjs: after a VS match
         * the CSS aborted about twenty seconds in (gGCCommonLinks[0] held DL
         * words -- 0xE2001E01 where a process list belonged). From VS Mode the
         * same leak ran into memory nothing owned, which is why it hid. */
        syTaskmanResetGraphicsHeap();
        func_80004AB0();
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
        s32 fkind = sMNPlayersVSSlots[slot].fkind;

        ndsFighterManagerRegisterDisplayFighter(NULL, slot);
        if (fighter_gobj != NULL)
        {
            ftManagerDestroyFighter(fighter_gobj);
        }
        sMNPlayersVSSlots[slot].player = NULL;
        sMNPlayersVSSlots[slot].fkind = nFTKindNull;
        ndsMNPlayersVSPreviewReleaseResidentKind(fkind);
    }
    /* Zero-ref closures deliberately survive live browsing. The scene boundary
     * is where all four fixed blocks must finally retire so no relocation
     * metadata points into the taskman arena after CSS exits. A nonzero ref at
     * this point is an ownership witness failure; count it, then force cleanup
     * because every fighter object above has already been destroyed. */
    for (slot = 0u; slot < NDS_PLAYERS_VS_RESIDENT_BLOCKS; slot++)
    {
        NDSPlayersVSResidentBlock *block = &sNdsPlayersVSResidentBlocks[slot];

        if (block->load_cursor != NULL)
        {
            (void)ndsMNPlayersVSPreviewCancelResidentLoad(block);
        }
        if (block->refs != 0u)
        {
            gNdsPlayersVSPreviewReleaseExitResidualRefCount += block->refs;
            block->refs = 0u;
        }
        (void)ndsMNPlayersVSPreviewRetireResidentBlock(
            block, nNDSPlayersVSResidentRetireExit);
    }
    if ((sNdsPlayersVSResidentPoolsReady != FALSE) &&
        (sNdsPlayersVSSharedResidentBase != NULL))
    {
        ndsRelocReleaseHeapRange(sNdsPlayersVSSharedResidentBase,
                                 NDS_PLAYERS_VS_SHARED_RESIDENT_BYTES);
        syMallocReset(&sNdsPlayersVSSharedResidentArena);
    }
    sNdsPlayersVSResidentPoolsReady = FALSE;
    ndsFTManagerRestoreKirbyPreviewMainMotion();
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

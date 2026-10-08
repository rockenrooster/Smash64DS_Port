/*
 * Whole BattleShip ft/ftmain.c import.
 *
 * Compatibility declarations live in include/. Keep gameplay behavior in the
 * original TU instead of extending the old DS-side ftMain seam copies.
 */
#include <PR/ultratypes.h>

typedef struct alSoundEffect {
    u8 filler_0x00[0x26];
    u16 sfx_id;
} alSoundEffect;

#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <nds/nds_reloc_assets.h>

void lbCommonAddFighterPartsFigatree(DObj *root_dobj, void *figatree,
                                     f32 anim_frame);
void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);
void ftParamSetAnimLocks(FTStruct *fp);
void ftParamClearAnimLocks(FTStruct *fp);
void mpCommonUpdateFighterSlopeContour(GObj *fighter_gobj);
void func_80026738_27338(alSoundEffect *sfx);
alSoundEffect *lbCommonMakePositionFGM(u16 fgm, f32 pos);
void ndsDiagnosticsRecordImportedFTMainAnimEvents(GObj *fighter_gobj);
void ndsDiagnosticsRecordImportedFTMainSetStatus(GObj *fighter_gobj,
                                                 s32 status_id,
                                                 f32 frame_begin,
                                                 f32 anim_speed, u32 flags);
void ndsFTParamsInvalidateFlatWalkCacheForFighter(GObj *fighter_gobj);
void ndsFighterRendererInvalidateStatusCachesOnSetStatus(GObj *fighter_gobj);
sb32 ndsDiagnosticsHandleImportedFTMainSetStatusBefore(GObj *fighter_gobj,
                                                       s32 status_id,
                                                       f32 frame_begin,
                                                       f32 anim_speed,
                                                       u32 flags);
volatile u32 gNdsFoxLaserColAnimSuppressCount;
volatile u32 gNdsFoxLaserColAnimPassCount;
volatile u32 gNdsKirbyCopyFoxColAnimSuppressCount;

/* BUGS.md owner override: BattleShip's Fox LASER / LASER AERIAL neutral-B
 * scripts (208_FoxMainMotion.c:1442-1466) each issue
 * SetColAnim(nGMColAnimFighterFoxSpecialHiStart, 0).  The constant's name is
 * misleading: these events belong to nFTFoxStatusSpecialN/SpecialAirN, not to
 * Fire Fox.  On DS that cosmetic flash presents as the reported laser strobe.
 * Keep the source neutral-B timing, projectile flags, SFX, sparkle/dust events,
 * and every other color animation intact; refuse only this exact event while a
 * Fox/NFox is in the two source laser statuses.  ftmain.c advances the motion
 * event unconditionally after this call, so suppression cannot stall or alter
 * the source script.
 *
 * Kirby carries his own copy of those two scripts:
 * 228_KirbyMainMotion.c dKirbyMainMotion_LaserGround and _LaserAir issue the
 * same SetColAnim(nGMColAnimFighterFoxSpecialHiStart, 0) under
 * nFTKirbyStatusCopyFoxSpecialN / ...AirN, where fkind is Kirby.  The Fox-only
 * gate above therefore let it through, which is the white-and-gold flash the
 * owner reports over Kirby's body when the copied blaster fires -- the same
 * cosmetic event, on a fighter the override never covered.  Extend the override
 * to exactly those two statuses.  Kirby's copied Samus and Donkey scripts reuse
 * this colanim id under their own statuses; scoping by status leaves them
 * alone. */
static sb32 ndsFTMainCheckSetFighterColAnimID(GObj *fighter_gobj,
                                              s32 colanim_id, s32 length)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp != NULL) &&
        ((fp->fkind == nFTKindFox) || (fp->fkind == nFTKindNFox)) &&
        ((fp->status_id == nFTFoxStatusSpecialN) ||
         (fp->status_id == nFTFoxStatusSpecialAirN)) &&
        (colanim_id == nGMColAnimFighterFoxSpecialHiStart))
    {
        gNdsFoxLaserColAnimSuppressCount++;
        return FALSE;
    }
    /* Deliberately not fenced behind NDS_P2_KIRBY. Both status constants and
     * both fighter kinds come from the unconditional enums in ft/fighter.h, so
     * the guard would buy nothing and could only silently delete the clause in
     * a configuration where the macro is undefined -- exactly the failure this
     * repair is fixing. Rosters without Kirby never produce these fkinds. */
    if ((fp != NULL) &&
        ((fp->fkind == nFTKindKirby) || (fp->fkind == nFTKindNKirby)) &&
        ((fp->status_id == nFTKirbyStatusCopyFoxSpecialN) ||
         (fp->status_id == nFTKirbyStatusCopyFoxSpecialAirN)) &&
        (colanim_id == nGMColAnimFighterFoxSpecialHiStart))
    {
        NDS_DIAG(gNdsKirbyCopyFoxColAnimSuppressCount++);
        return FALSE;
    }
    if ((fp != NULL) &&
        ((fp->fkind == nFTKindFox) || (fp->fkind == nFTKindNFox)))
    {
        gNdsFoxLaserColAnimPassCount++;
    }
    return ftParamCheckSetFighterColAnimID(fighter_gobj, colanim_id, length);
}
#if NDS_TASK108_SITR_CALLBACK_CENSUS
void ndsTask108SitrRefreshCallbacks(GObj *fighter_gobj);
#endif

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifdef lbRelocGetFileData
#undef lbRelocGetFileData
#endif
#define lbRelocGetFileData(type, file, offset) \
    ((type)((uintptr_t)(file) + (intptr_t)(offset)))

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP && \
    NDS_TICK_HUD && !NDS_TICK_HUD_SRC_SPLIT
/* LAB: price the parts of a status change, in columns the SRC split leaves
 * unused without it. SHDT = the motion fetch and SPRM = the figatree install,
 * each only as called from the setter below; SCPU = the whole setter. MCAP =
 * the new clip's first play (anim keys + part transforms) and MPRO = the
 * setter's part/effect/hit-status resets, both only while a setter runs; these
 * two are the cumulative MISC counters, so MCAM reads low in these rows. */
#define NDS_LAB_STATUS_PRICE 1
extern u32 cpuGetTiming(void);
extern volatile u32 gNdsTickHudSrcHitDetectTicks;
extern volatile u32 gNdsTickHudSrcParamsTicks;
extern volatile u32 gNdsTickHudSrcComputerTicks;
extern volatile u32 gNdsMiscCaptureTicks;
extern volatile u32 gNdsMiscProcDisplayTicks;
static u32 sNdsLabStatusDepth;

#define NDS_LAB_TIMED_IN_SETTER(counter_, call_)                              \
    do                                                                         \
    {                                                                          \
        if (sNdsLabStatusDepth != 0u)                                          \
        {                                                                      \
            u32 nds_lab_start_ = cpuGetTiming();                               \
                                                                               \
            call_;                                                             \
            (counter_) += cpuGetTiming() - nds_lab_start_;                     \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            call_;                                                             \
        }                                                                      \
    } while (0)

static void ndsLabTimedUpdateAnimKeys(GObj *fighter_gobj)
{
    NDS_LAB_TIMED_IN_SETTER(gNdsMiscCaptureTicks,
                            ftParamUpdateAnimKeys(fighter_gobj));
}

static void ndsLabTimedPartsTransform(DObj *joint)
{
    NDS_LAB_TIMED_IN_SETTER(gNdsMiscCaptureTicks,
                            ftParamsUpdateFighterPartsTransform(joint));
}

#define NDS_LAB_RESET_WRAP1(name_, type_)                                      \
    static void ndsLabTimed_##name_(type_ a)                                   \
    {                                                                          \
        NDS_LAB_TIMED_IN_SETTER(gNdsMiscProcDisplayTicks, name_(a));           \
    }
#define NDS_LAB_RESET_WRAP2(name_, type_, type2_)                              \
    static void ndsLabTimed_##name_(type_ a, type2_ b)                         \
    {                                                                          \
        NDS_LAB_TIMED_IN_SETTER(gNdsMiscProcDisplayTicks, name_(a, b));        \
    }
NDS_LAB_RESET_WRAP2(ftParamSetModelPartDetailAll, GObj *, u8)
NDS_LAB_RESET_WRAP1(ftParamClearAttackCollAll, GObj *)
NDS_LAB_RESET_WRAP1(ftParamResetModelPartAll, GObj *)
NDS_LAB_RESET_WRAP1(ftParamResetTexturePartAll, GObj *)
NDS_LAB_RESET_WRAP1(ftParamProcStopEffect, GObj *)
NDS_LAB_RESET_WRAP1(ftParamStopLoopSFX, FTStruct *)
NDS_LAB_RESET_WRAP1(ftParamResetStatUpdateColAnim, GObj *)
NDS_LAB_RESET_WRAP2(ftParamSetHitStatusPartAll, GObj *, s32)
NDS_LAB_RESET_WRAP2(ftParamSetHitStatusAll, GObj *, s32)
NDS_LAB_RESET_WRAP1(ftParamResetFighterDamageCollsAll, GObj *)
NDS_LAB_RESET_WRAP2(ftParamMoveDLLink, GObj *, u8)
#define ftParamUpdateAnimKeys ndsLabTimedUpdateAnimKeys
#define ftParamsUpdateFighterPartsTransform ndsLabTimedPartsTransform
#define ftParamSetModelPartDetailAll ndsLabTimed_ftParamSetModelPartDetailAll
#define ftParamClearAttackCollAll ndsLabTimed_ftParamClearAttackCollAll
#define ftParamResetModelPartAll ndsLabTimed_ftParamResetModelPartAll
#define ftParamResetTexturePartAll ndsLabTimed_ftParamResetTexturePartAll
#define ftParamProcStopEffect ndsLabTimed_ftParamProcStopEffect
#define ftParamStopLoopSFX ndsLabTimed_ftParamStopLoopSFX
#define ftParamResetStatUpdateColAnim ndsLabTimed_ftParamResetStatUpdateColAnim
#define ftParamSetHitStatusPartAll ndsLabTimed_ftParamSetHitStatusPartAll
#define ftParamSetHitStatusAll ndsLabTimed_ftParamSetHitStatusAll
#define ftParamResetFighterDamageCollsAll ndsLabTimed_ftParamResetFighterDamageCollsAll
#define ftParamMoveDLLink ndsLabTimed_ftParamMoveDLLink

static void *ndsLabTimedForceExtern(const void *file_id, void *heap)
{
    u32 start = cpuGetTiming();
    void *file = lbRelocGetForceExternHeapFile(file_id, heap);

    gNdsTickHudSrcHitDetectTicks += cpuGetTiming() - start;
    return file;
}

static void ndsLabTimedAddFigatree(DObj *root_dobj, void *figatree,
                                   f32 anim_frame)
{
    u32 start = cpuGetTiming();

    lbCommonAddFighterPartsFigatree(root_dobj, figatree, anim_frame);
    gNdsTickHudSrcParamsTicks += cpuGetTiming() - start;
}
#define lbRelocGetForceExternHeapFile ndsLabTimedForceExtern
#define lbCommonAddFighterPartsFigatree ndsLabTimedAddFigatree
#else
#define NDS_LAB_STATUS_PRICE 0
#endif

#define ftMainCheckGetUpdateDamage battleship_ftMainCheckGetUpdateDamage
#define ftMainPlayHitSFX battleship_ftMainPlayHitSFX
#define ftMainUpdateDamageStatFighter battleship_ftMainUpdateDamageStatFighter
#define ftMainSetHitInteractStats battleship_ftMainSetHitInteractStats
#define ftMainSetHitRebound battleship_ftMainSetHitRebound
#define ftMainUpdateAttackStatFighter battleship_ftMainUpdateAttackStatFighter
#define ftMainUpdateShieldStatFighter battleship_ftMainUpdateShieldStatFighter
#define ftMainUpdateCatchStatFighter battleship_ftMainUpdateCatchStatFighter
#define ftMainProcessHitCollisionStatsMain battleship_ftMainProcessHitCollisionStatsMain
#define ftMainCheckAddGroundObstacle battleship_ftMainCheckAddGroundObstacle
#define ftMainClearGroundObstacle battleship_ftMainClearGroundObstacle
#define ftMainSetHitHazard battleship_ftMainSetHitHazard
#define ftMainSearchHitHazard battleship_ftMainSearchHitHazard
#define ftMainSearchHitFighter battleship_ftMainSearchHitFighter
#define ftMainSearchFighterCatch battleship_ftMainSearchFighterCatch
#define ftMainProcSearchCatch battleship_ftMainProcSearchCatch
#define ftMainSearchHitItem battleship_ftMainSearchHitItem
#define ftMainSearchHitWeapon battleship_ftMainSearchHitWeapon
#define ftMainSearchGroundHit battleship_ftMainSearchGroundHit
#define ftMainProcSearchHitAll battleship_ftMainProcSearchHitAll
#if NDS_P4
/* P4 wraps the source proc (Remix sword trails, below the include). */
#define ftMainProcParams ndsBaseFTMainProcParams
#else
#define ftMainProcParams battleship_ftMainProcParams
#endif
#define ftMainRunUpdateColAnim battleship_ftMainRunUpdateColAnim
#define ftMainPlayAnimEventsAll battleship_ftMainPlayAnimEventsAll
#define ftMainSetStatus battleship_ftMainSetStatus
#define ftParamCheckSetFighterColAnimID ndsFTMainCheckSetFighterColAnimID
/* Cycle 92 SGCO split. The last three of the six per-fighter procs registered
 * at decomp ft/ftmanager.c:858-863; the other three are renamed above. Each has
 * exactly ONE call site -- that gcAddGObjProcess registration, in a different TU
 * that never sees these renames -- so the registration binds the port wrapper in
 * src/port/reloc_backend_diagnostic_recorders.c and the bracket measures every
 * invocation. linker/nds_hot_text.ld carries the matching ITCM pins under these
 * new names, edited in place so the hot list keeps its order. */
#define ftMainProcUpdateInterrupt battleship_ftMainProcUpdateInterrupt
#define ftMainProcPhysicsMapDefault battleship_ftMainProcPhysicsMapDefault
#define ftMainProcPhysicsMapCapture battleship_ftMainProcPhysicsMapCapture
#if NDS_P4
/* P4: Remix's custom motion commands (first byte 0xD0+, src/Command.asm) are
 * executed where Remix executes them -- at the event-kind read of each motion
 * loop (load_command_ / load_command_2_). In this TU every read of a command
 * word goes through ftMotionEventCast with FTMotionEventDefault or
 * FTMotionEventMakeEffect1; every other cast reads an operand word, which may
 * legitimately begin with 0xD0+ and must stay a plain cast. The two
 * fast-forward loops are the only enclosing functions whose names are 32 and
 * 38 bytes long, which is how the cursor knows to use Remix's second table. */
#include <nds/nds_p4.h>
#undef ftMotionEventCast
#define ftMotionEventCast(event, type) NDS_P4_EVCAST_##type(event, type)
#define NDS_P4_EVCAST_CURSOR(event, type)                                    \
    ((type *)ndsP4MotionEventCursor(fighter_gobj, (event),                  \
                                    (sizeof(__func__) == 32u) ||            \
                                    (sizeof(__func__) == 38u)))
#define NDS_P4_EVCAST_PLAIN(event, type) ((type *)(event)->p_script)
#define NDS_P4_EVCAST_FTMotionEventDefault NDS_P4_EVCAST_CURSOR
#define NDS_P4_EVCAST_FTMotionEventMakeEffect1 NDS_P4_EVCAST_CURSOR
#define NDS_P4_EVCAST_FTMotionEventGoto2 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventMakeAttack1 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventMakeAttack2 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventMakeAttack3 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventMakeAttack4 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventMakeAttack5 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventMakeEffect2 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventMakeEffect3 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventMakeEffect4 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventMakeRumble NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventParallel2 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetAfterImage NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetAttackCollDamage NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetAttackCollSize NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetAttackCollSound NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetAttackOffset1 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetAttackOffset2 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetColAnimID NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetDamageCollPartID1 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetDamageCollPartID2 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetDamageCollPartID3 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetDamageCollPartID4 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetDamageThrown2 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetHitStatusPartID NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetModelPartID NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetTexturePartID NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSetThrow2 NDS_P4_EVCAST_PLAIN
#define NDS_P4_EVCAST_FTMotionEventSubroutine2 NDS_P4_EVCAST_PLAIN
/* P4 S14: the source's per-kind special status table, renamed so the
 * wrapper below can lend a P4 fighter's row its content's table for one
 * ftMainSetStatus call. ftMainSetStatus is its only reader. */
#define dFTMainSpecialStatusDescs gNdsFTMainSpecialStatusDescsView
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftmain.c"
#if NDS_P4
#undef ftMotionEventCast
#define ftMotionEventCast(event, type) ((type *)(event)->p_script)
#undef dFTMainSpecialStatusDescs
#endif
#undef ftMainCheckGetUpdateDamage
#undef ftMainPlayHitSFX
#undef ftMainUpdateDamageStatFighter
#undef ftMainSetHitInteractStats
#undef ftMainSetHitRebound
#undef ftMainUpdateAttackStatFighter
#undef ftMainUpdateShieldStatFighter
#undef ftMainUpdateCatchStatFighter
#undef ftMainProcessHitCollisionStatsMain
#undef ftMainCheckAddGroundObstacle
#undef ftMainClearGroundObstacle
#undef ftMainSetHitHazard
#undef ftMainSearchHitHazard
#undef ftMainSearchHitFighter
#undef ftMainSearchFighterCatch
#undef ftMainProcSearchCatch
#undef ftMainSearchHitItem
#undef ftMainSearchHitWeapon
#undef ftMainSearchGroundHit
#undef ftMainProcSearchHitAll
#undef ftMainProcParams
#undef ftMainRunUpdateColAnim
#undef ftMainPlayAnimEventsAll
#undef ftMainSetStatus
#undef ftParamCheckSetFighterColAnimID
#undef ftMainProcUpdateInterrupt
#undef ftMainProcPhysicsMapDefault
#undef ftMainProcPhysicsMapCapture
#if NDS_LAB_STATUS_PRICE
#undef lbRelocGetForceExternHeapFile
#undef lbCommonAddFighterPartsFigatree
#undef ftParamUpdateAnimKeys
#undef ftParamsUpdateFighterPartsTransform
#undef ftParamSetModelPartDetailAll
#undef ftParamClearAttackCollAll
#undef ftParamResetModelPartAll
#undef ftParamResetTexturePartAll
#undef ftParamProcStopEffect
#undef ftParamStopLoopSFX
#undef ftParamResetStatUpdateColAnim
#undef ftParamSetHitStatusPartAll
#undef ftParamSetHitStatusAll
#undef ftParamResetFighterDamageCollsAll
#undef ftParamMoveDLLink
#endif

#if NDS_P4
/* Remix SwordTrail.asm: SET AFTERIMAGE with is_itemswing >= 2 picks a Remix
 * trail row. The source's afterimage switch, the last statement of
 * ftMainProcParams, has cases for the vanilla 0 and 1 only; Remix's
 * initial_setup_ sends a row matching the fighter through the Link-sword
 * step on the row's joint and axis. Same gate: no hitlag when the proc
 * began, drawstatus not -1. */
void battleship_ftMainProcParams(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 hitlag_tics = fp->hitlag_tics;

    ndsBaseFTMainProcParams(fighter_gobj);
    if ((hitlag_tics == 0u) && (fp->afterimage.drawstatus != -1) &&
        (fp->afterimage.is_itemswing >= 2))
    {
        ndsP4UpdateSwordTrail(fp);
    }
}

/* Remix's define_character gives a fighter its own action array: the
 * parent's, with the statuses it changes and appends. The source reads the
 * special statuses (0xDC on) from the view's fkind row, which a P4 fighter
 * shares with its parent, so for one ftMainSetStatus call the row holds the
 * fighter's table -- its content's, or the source table when an original
 * fighter's status is set inside a P4 fighter's call -- and is put back
 * after. Outside the setter the view is the source table; the first call
 * keeps a copy of it, which is what an inherited slot reads. */
static FTStatusDesc *sNdsFTMainSourceSpecialStatusDescs[
    ARRAY_COUNT(gNdsFTMainSpecialStatusDescsView)];
static sb32 sNdsFTMainSourceSpecialStatusKept;

static FTStatusDesc **ndsFTMainLendSpecialStatusDescs(FTStruct *fp,
                                                      FTStatusDesc **saved)
{
    FTStatusDesc **row;
    FTStatusDesc *table;
    u32 i;

    if (sNdsFTMainSourceSpecialStatusKept == FALSE)
    {
        for (i = 0u; i < ARRAY_COUNT(gNdsFTMainSpecialStatusDescsView); i++)
        {
            sNdsFTMainSourceSpecialStatusDescs[i] =
                gNdsFTMainSpecialStatusDescsView[i];
        }
        sNdsFTMainSourceSpecialStatusKept = TRUE;
    }
    if ((u32)fp->fkind >= ARRAY_COUNT(gNdsFTMainSpecialStatusDescsView))
    {
        return NULL;
    }
    row = &gNdsFTMainSpecialStatusDescsView[fp->fkind];
    table = ndsP4SpecialStatusDescs(fp,
                                    sNdsFTMainSourceSpecialStatusDescs[fp->fkind]);
    if (table == NULL)
    {
        table = sNdsFTMainSourceSpecialStatusDescs[fp->fkind];
    }
    if (*row == table)
    {
        return NULL;
    }
    *saved = *row;
    *row = table;
    return row;
}
#endif

void ftMainPlayAnimEventsAll(GObj *fighter_gobj)
{
    battleship_ftMainPlayAnimEventsAll(fighter_gobj);
#if NDS_SHIP_TELEMETRY
    ndsDiagnosticsRecordImportedFTMainAnimEvents(fighter_gobj);
#endif
}

#if NDS_P2_SAMUS_ATTACK_TOUR
void ndsSamusAttackTourRecordStatusTransition(GObj *fighter_gobj,
                                               s32 status_id);
#endif

/* See the flattened-walk block in ftMainSetStatus below. */
__attribute__((used)) volatile u32 gNdsFtStatusFlatKept;
/* The renderer half of the same test (2026-10-02). */
__attribute__((used)) volatile u32 gNdsFtStatusRenderKept;

void ftMainSetStatus(GObj *fighter_gobj, s32 status_id,
                     f32 frame_begin, f32 anim_speed, u32 flags)
{
#if NDS_LAB_STATUS_PRICE
    /* A status proc may set a status from inside a setter; only the outer
     * call is timed so the column is not counted twice. */
    u32 lab_start = cpuGetTiming();

    sNdsLabStatusDepth++;
#endif
    FTStruct *nds_topology_fp;
    u32 nds_topology_word;
#if NDS_P4
    FTStatusDesc **nds_p4_row = NULL;
    FTStatusDesc *nds_p4_saved = NULL;
#endif

    if (ndsDiagnosticsHandleImportedFTMainSetStatusBefore(fighter_gobj,
            status_id, frame_begin, anim_speed, flags) != FALSE)
    {
#if NDS_LAB_STATUS_PRICE
        sNdsLabStatusDepth--;
#endif
        return;
    }
    nds_topology_fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    nds_topology_word = (nds_topology_fp != NULL) ?
        nds_topology_fp->anim_desc.word : 0xffffffffu;
#if NDS_P4
    if (nds_topology_fp != NULL)
    {
        ndsP4OnSetStatus(fighter_gobj);
        nds_p4_row = ndsFTMainLendSpecialStatusDescs(nds_topology_fp,
                                                     &nds_p4_saved);
    }
#endif
    battleship_ftMainSetStatus(fighter_gobj, status_id, frame_begin,
                               anim_speed, flags);
#if NDS_P4
    if (nds_p4_row != NULL)
    {
        *nds_p4_row = nds_p4_saved;
    }
#endif
    if (nds_topology_fp != NULL)
    {
        nds_topology_word |= nds_topology_fp->anim_desc.word;
#if NDS_P4
        if (nds_topology_fp->nds_p4_content != 0u)
        {
            ndsP4AfterSetStatus(fighter_gobj, status_id);
        }
#endif
    }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP && \
    NDS_TICK_HUD && !NDS_TICK_HUD_SRC_SPLIT
    /* LAB: status changes per frame, in the SPHC column (unused without the
     * SRC split). */
    {
        extern volatile u32 gNdsTickHudSrcPhysicsCaptureTicks;

        gNdsTickHudSrcPhysicsCaptureTicks++;
    }
#endif
#if NDS_P2_SAMUS_ATTACK_TOUR
    /* Read-only P2-3 acceptance observer. BattleShip has already selected and
     * installed the status; this only records transient states that can begin
     * and end inside one gcRunAll before the once-per-update sampler runs. */
    ndsSamusAttackTourRecordStatusTransition(fighter_gobj, status_id);
#endif
    /* BattleShip's status setter owns the dynamic hidden-part topology used by
     * grab/throw animations. It can allocate/eject/re-parent a DObj without
     * changing either the fighter root pointer or the taskman heap generation.
     * Invalidate BOTH consumers that key on those stable identities: the
     * flattened per-frame transform-invalidation walk and the renderer caches.
     * The former is gameplay-critical: ftCommonCapturePulledRotateScale reads
     * the dynamically enabled item-heavy joint's world matrix.
     *
     * P2-2p8 (2026-09-29): the setter touches the topology only through the
     * hidden-part loop (ftmain.c:4630-4651) and the TransN re-link
     * (:4710-4722), and both are driven by the anim_desc bits above the five
     * flag bits: the loop runs an add, update or eject for every such bit set
     * in the OLD or the NEW word, and TransN's is one of them. With none set
     * in either word no DObj was added, ejected or re-linked, the flattened
     * walk still lists the same FTParts in the same order, and re-walking it
     * is ~3.6K ticks a change for nothing. */
    if ((nds_topology_fp == NULL) ||
        ((nds_topology_word & ~0x1Fu) != 0u))
    {
        ndsFTParamsInvalidateFlatWalkCacheForFighter(fighter_gobj);
    }
    else
    {
        NDS_DIAG(gNdsFtStatusFlatKept++);
    }
    /* The renderer caches (display-contract memo, draw plan, lean instance)
     * key on the drawn DObjs and their DLs. A status change moves those only
     * through the topology above or through the model-part writers, which
     * invalidate on their own when they change a part
     * (ndsFTParamInvalidateModelPartRenderer); texture parts reach the lean
     * path through its writer serial. Re-deriving them after every change
     * cost the next draw the source DL walk, the material hashes and the
     * lean re-tuple and re-proof, which found the same answer. */
    if ((nds_topology_fp == NULL) ||
        ((nds_topology_word & ~0x1Fu) != 0u))
    {
        ndsFighterRendererInvalidateStatusCachesOnSetStatus(fighter_gobj);
    }
    else
    {
        NDS_DIAG(gNdsFtStatusRenderKept++);
    }
#if NDS_P2_HURTBOX_REJECT
    /* The same topology change retires the hurtbox reject's cached worlds
     * (src/port/nds_p2_hurtbox_reject.c keys them on the DObj pointer). */
    {
        extern volatile u32 gNdsP2HurtboxLatchEpoch;

        gNdsP2HurtboxLatchEpoch++;
    }
#endif
#if NDS_TASK108_SITR_CALLBACK_CENSUS
    /* A status change can happen between the census's outer-proc entry and the
     * later proc_update/proc_interrupt calls. Re-wrap the newly installed
     * source callbacks only while that lab census is active. */
    ndsTask108SitrRefreshCallbacks(fighter_gobj);
#endif
#if NDS_R2_BATTLEPACK
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        if (fp != NULL)
        {
            fp->figatree = ndsRelocResolveAuthoritativeForceFile(fp->figatree);
        }
    }
#endif
#if NDS_SHIP_TELEMETRY
    ndsDiagnosticsRecordImportedFTMainSetStatus(fighter_gobj, status_id,
                                                 frame_begin, anim_speed,
                                                 flags);
#endif
#if NDS_LAB_STATUS_PRICE
    if (--sNdsLabStatusDepth == 0u)
    {
        gNdsTickHudSrcComputerTicks += cpuGetTiming() - lab_start;
    }
#endif
}

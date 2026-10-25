/* Diagnostic words, timing and the renderer: the tick HUD's buckets, the
 * replay digest, effects, the painter, renderer, item and stage words.
 * Included in place by nds_startup.h (inside its guard); not a header of
 * its own. */

#if NDS_TICK_HUD
enum NDSTickHudBucket {
    nNDSTickHudBucketAll,
    nNDSTickHudBucketFighters,
    nNDSTickHudBucketStage,
    nNDSTickHudBucketBackground,
    nNDSTickHudBucketAudio,
    nNDSTickHudBucketHud,
    nNDSTickHudBucketSourceUpdate,
    nNDSTickHudBucketMiscDraw,
    nNDSTickHudBucketOther,
    /* Task 66. Appended after OTHR so every bucket that existed before keeps
     * its index and every measurement already on record stays comparable.
     *
     * ALL is wall time for the presented iteration and therefore quantized to
     * whole VBlank periods (Task 64), which makes it the right acceptance gate
     * and a useless search gate: a change that removes real work but less than
     * one 560,190-tick period cannot move it. WAIT is the idle span that
     * absorbs the difference, and WORK = ALL - WAIT is the quantity a search
     * actually needs.
     *
     * WORK is sampled as its own series rather than derived from the printed
     * percentiles, because the P95 of a difference is not the difference of
     * two P95s. PROJECT_GOAL.md gates on P95 <= 1.12M, so WORK's own P95 is
     * the number that decides the milestone. */
    nNDSTickHudBucketVBlankWait,
    nNDSTickHudBucketWork,
    /* Cycle 85. SRC sub-owners, appended after WORK for the same reason Task 66
     * appended after OTHR: every index that already existed keeps its value, so
     * every banked measurement stays comparable and the named/OTHR/WORK identity
     * is byte-identical.
     *
     * These are SUB-buckets of SRC, not new work. SRC brackets
     * ndsFxEffectsUpdate + scVSBattleFuncUpdate (taskman_seam.c), and both
     * spans below are nested strictly inside it, so they are deliberately NOT
     * added to `named` -- doing so would double-count them against ALL and
     * corrupt OTHR. The third sub-owner (the decomp sim path, "SBAS") is the
     * residual SRC - SHDT - SWRM and is derived by the analyzer rather than
     * ringed: it costs no bytes, and its non-negativity on every frame is the
     * proof that these two spans really are nested inside SRC. */
    nNDSTickHudBucketSrcHitDetect,
    nNDSTickHudBucketSrcAnimWarm,
    /* Cycle 86 SBAS split, appended after the cycle-85 pair for the same reason
     * they were appended after WORK: every index that already existed keeps its
     * value, so every banked measurement stays comparable.
     *
     * The composition was VERIFIED before these were chosen, not assumed. SRC
     * brackets ndsFxEffectsUpdate + scVSBattleFuncUpdate; the decomp scene
     * update is a one-liner (scvsbattle.c:75) calling
     * ifCommonBattleUpdateInterfaceAll, whose game_status switch reaches
     * ifCommonBattleGoUpdateInterface, which ends in gcRunAll (ifcommon.c:2970).
     * gcRunAll is therefore the SOLE gateway to the entire simulation inside
     * SRC, and it already has a port wrapper -- so no decomp/ edit is needed.
     *
     * The fighter proc chain is decomp ft/ftmanager.c:858-863, six procs per
     * fighter in priority order 5..0: UpdateInterrupt, PhysicsMapDefault,
     * PhysicsMapCapture, SearchCatch, SearchHitAll (= SHDT), Params. Four of the
     * six sub-owners below are bracketed on EXISTING port wrappers; the
     * interrupt and the two physics procs are deliberately NOT wrapped, because
     * linker/nds_hot_text.ld:207-209 pins them into ITCM by exact symbol name
     * under a size ASSERT, and renaming them would move code out of hot text and
     * change the very cost being measured.
     *
     * Strictly hierarchical and disjoint: SCPU nests inside the interrupt proc
     * (ftmain.c:1269, case nFTPlayerKindCom); SCAT, SHDT and SPRM are separate
     * sequential procs; all four nest inside GCRA. Two residuals are DERIVED by
     * the analyzer at zero byte cost -- SOUT = SBAS - GCRA (work outside the sim:
     * Fx sprites, the interface dispatch, port glue) and SGCO = GCRA - SCPU
     * - SCAT - SHDT - SPRM (inside the sim, unattributed: the interrupt proc less
     * its AI, both physics procs, camera, effects, items, weapons, interface
     * GObjs). Their non-negativity on every frame is the proof the nesting is
     * real -- the same falsifier the cycle-85 pair used. Like SHDT/SWRM these are
     * sub-spans of SRC and are deliberately NOT added to `named`. */
    nNDSTickHudBucketSrcRunAll,
    nNDSTickHudBucketSrcComputer,
    nNDSTickHudBucketSrcCatch,
    nNDSTickHudBucketSrcParams,
    /* Cycle 92 SGCO split, appended after the cycle-86 four on the same terms:
     * every index that already existed keeps its value, so every banked
     * measurement stays comparable and named/OTHR/WORK stays byte-identical.
     *
     * SGCO carried 153,291 of the gate arm's WORK-H excursion (52.0%, 73.6% of
     * SRC) and 81.3% of Boundary's SRC excursion -- arm-independent, therefore
     * structural. It is a residual, and a residual cannot be optimised, so it is
     * split in turn. These three close the fighter half of it by bracketing the
     * remaining three of the six procs at ftmanager.c:858-860:
     *
     *   SINT  ftMainProcUpdateInterrupt   priority 5, SCPU nested inside it
     *   SPHD  ftMainProcPhysicsMapDefault priority 4, the uncaptured physics arm
     *   SPHC  ftMainProcPhysicsMapCapture priority 3, the captured physics arm
     *
     * The cycle-86 note above said these three could not be wrapped without
     * moving them out of ITCM. That is answered rather than ignored: they are
     * renamed with the same #define-before-#include pattern the other three
     * procs already use (src/import/battleship_ftmain.c), and
     * linker/nds_hot_text.ld is edited IN PLACE in the same commit so the pins
     * follow the new symbol names at their existing positions. Editing in place
     * matters -- the Task 94 note on that file records that this list is a
     * curated 8 KiB working set whose members re-address each other when the
     * order changes, so a rename must not become a move.
     *
     * Verified before instrumenting, not assumed. Each of the three has exactly
     * ONE call site -- the gcAddGObjProcess registration in decomp ftmanager.c,
     * a different TU that does not see the renames -- so the registration takes
     * the address of the port wrapper. This is the identical mechanism that
     * already carries SCAT/SHDT/SPRM, which register in the same if-block at
     * ftmanager.c:861-863 and already measure non-zero; that is what proves the
     * three below are live before a run is spent on them.
     *
     * PhysicsMapDefault and PhysicsMapCapture are MUTUALLY EXCLUSIVE arms of the
     * same test (ftmain.c:1918-1937): each calls the shared ftMainProcPhysicsMap
     * only when its capture predicate holds. The shared worker keeps its own name
     * and its own ITCM pin, so bracketing the two outer procs is disjoint and
     * exhaustive over it.
     *
     * Two more residuals are DERIVED by the analyzer at zero byte cost:
     *   SITR = SINT - SCPU    the interrupt proc less its AI
     *   SOBJ = GCRA - SINT - SPHD - SPHC - SCAT - SHDT - SPRM
     *                         the NON-FIGHTER GObjs: camera, effects, items,
     *                         weapons, interface, plus gcRunAll's own two loops
     * so that SGCO = SITR + SPHD + SPHC + SOBJ exactly, and the banked SGCO
     * stays derivable as a regression check. Non-negativity of both on every
     * frame is the falsifier, the same one the cycle-85 and cycle-86 residuals
     * use. Like every SRC sub-bucket these are NOT added to `named`.
     *
     * NOT bracketed, deliberately: mpProcessUpdateMain. It is the one seam here
     * that already has a port wrapper (battleship_mpprocess_live_bridge.c:150),
     * but it is called from ~20 sites across fighter, ITEM and WEAPON map
     * (mpcommon.c, itmap.c, wpmap.c), so it is an overlay spanning SPHD/SPHC and
     * SOBJ rather than a disjoint owner, and its call frequency is an order of
     * magnitude above every existing bracket. The cycle-90/91 phase instrument
     * already showed a high-frequency bracket moving WORK-H P95 +15,808 and
     * +45,568, far outside the +/-5,376 floor. Measure it with a call counter or
     * on the fighter entry points only, never with a timer on the shared leaf. */
    nNDSTickHudBucketSrcInterrupt,
    nNDSTickHudBucketSrcPhysicsDefault,
    nNDSTickHudBucketSrcPhysicsCapture,
    /* P2-2p8 Phase 0 (docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md section 2).
     * MISC is a residual (DRAW minus the named draw owners) and ~200K of it
     * matched no named symbol. These four are the per-frame deltas of the
     * existing cumulative sub-path counters (weapon, effect and particle draw,
     * texture upload), so the residual can be sized per frame. They are
     * sub-spans of MISC and, like the SRC sub-buckets, NOT part of `named`. */
    nNDSTickHudBucketMiscWeapon,
    nNDSTickHudBucketMiscItem,
    nNDSTickHudBucketMiscEffect,
    nNDSTickHudBucketMiscActor,
    nNDSTickHudBucketMiscParticle,
    nNDSTickHudBucketMiscTexUpload,
    nNDSTickHudBucketMiscCapture,
    nNDSTickHudBucketMiscProc,
    nNDSTickHudBucketMiscCamera,
    nNDSTickHudBucketMiscFlush,
    /* Not ticks: the geometry engine's polygon and vertex list RAM usage of
     * this frame, read just before glFlush (2,048 / 6,144 are the hardware
     * limits). Ringed so every percentile row carries its own GX load. */
    nNDSTickHudBucketGxPolygons,
    nNDSTickHudBucketGxVertices,
    /* Not ticks: the gameplay replay digest (src/port/nds_replay_digest.c)
     * after the undrawn and the drawn logic tick of this presented frame. A
     * candidate and its control must agree on both for a whole match
     * (scripts/compare-replay-digest.py). */
    nNDSTickHudBucketDigestA,
    nNDSTickHudBucketDigestB,
    /* This frame's synchronous storage waits (gNdsAudioStorageWaitTicks64's
     * delta, cpuGetTiming ticks): every card read the frame waited for, the
     * motion clips a status change missed in the cache among them. Owner
     * 2026-10-20 replaced "0 motion reads after GO" with "a motion read never
     * pushes a frame past two VBlanks": a presented frame past two VBlanks
     * whose busy time less this column would have fit counts against it
     * (scripts/p7/check_motion_read_push.py). A sub-span, out of every total. */
    nNDSTickHudBucketStorageWait,
    nNDSTickHudBucketCount,
    /* The on-screen table stops at OTHR: the console is 24 rows and rows 20-23
     * already carry the legend, the VBlank histogram and the build stamp. WAIT
     * and WORK are still ringed, still percentiled, and still readable over GDB
     * by scripts/sample-tick-hud-buckets.ps1; WORK gets the one row that fits. */
    nNDSTickHudBucketDisplayCount = nNDSTickHudBucketVBlankWait
};
extern volatile u32 gNdsTickHudBuckets[nNDSTickHudBucketCount];
u32 ndsReplayDigestTick(void);
extern volatile u32 gNdsTickHudVBlankWaitTicks;
/* P2-2p8 (2026-10-04, owner ruling D12a): the tick HUD's per-span clock
 * reads -- ~120 a presented frame, inside WORK -- are the instrument's cost,
 * not the game's. gNdsTickHudSpans 0 (the four-CPU gate ROM's default,
 * NDS_TICK_HUD_SPANS_DEFAULT) skips them at the high-frequency span sites:
 * their buckets read 0 while WORK stays exact. 1 (every other ROM's default;
 * poke it on a gate ROM) keeps the per-bucket breakdown for analysis. */
#if NDS_TICK_HUD
extern volatile u32 gNdsTickHudSpans;
#define NDS_TICK_HUD_SPAN_CLOCK() \
    ((gNdsTickHudSpans != 0u) ? cpuGetTiming() : 0u)
#else
#define NDS_TICK_HUD_SPAN_CLOCK() cpuGetTiming()
#endif

/* The four points at which the fighter draw can give up on the native
 * production owner and fall through to the generic display-list interpreter,
 * in the order they are reached. Ordering matters: INPUTS and BEGIN reject
 * before any GX traffic, CONTRACT rejects after preparation but before GX, and
 * POST_GX is the expensive one because the frame has already paid for a
 * partial native submission before restarting generically. */
#define NDS_TICK_HUD_NATIVE_OWNER_MARK(reason) ((void)0)
#define NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(reason) ((void)0)

extern volatile u32 gNdsTickHudFighterTicks;
extern volatile u32 gNdsTickHudStageTicks;
extern volatile u32 gNdsTickHudBackgroundTicks;
extern volatile u32 gNdsTickHudForegroundTicks;
extern volatile u32 gNdsTickHudAudioTicks;
extern volatile u32 gNdsTickHudSourceTicks;
/* Cycle 85 SRC split. Per-frame like gNdsTickHudSourceTicks (reset alongside it,
 * not cumulative), because a hot-vs-clean excursion needs the per-frame series
 * the ring carries and not a pair of differenced totals. */
extern volatile u32 gNdsTickHudSrcHitDetectTicks;
extern volatile u32 gNdsTickHudSrcAnimWarmTicks;
/* Cycle 86 SBAS split. Per-frame, reset alongside gNdsTickHudSourceTicks. */
extern volatile u32 gNdsTickHudSrcRunAllTicks;
extern volatile u32 gNdsTickHudSrcComputerTicks;
extern volatile u32 gNdsTickHudSrcCatchTicks;
extern volatile u32 gNdsTickHudSrcParamsTicks;
/* Cycle 92 SGCO split. Per-frame, reset alongside gNdsTickHudSourceTicks. */
extern volatile u32 gNdsTickHudSrcInterruptTicks;
extern volatile u32 gNdsTickHudSrcPhysicsDefaultTicks;
extern volatile u32 gNdsTickHudSrcPhysicsCaptureTicks;
extern volatile u32 gNdsTickHudFlushTicks;
/* R2-07 MISC split. Cumulative, never reset per frame -- difference them
 * across two ring stops with -PerStopGlobals. See diagnostics.c. */
extern volatile u32 gNdsMiscWeaponDrawTicks;
extern volatile u32 gNdsMiscItemDrawTicks;
extern volatile u32 gNdsMiscEffectDrawTicks;
extern volatile u32 gNdsMiscActorDrawTicks;
extern volatile u32 gNdsMiscParticleDrawTicks;
extern volatile u32 gNdsMiscSplitAccountedTicks;
enum
{
    NDS_MISC_PROC_KIND_WEAPON = 0,
    NDS_MISC_PROC_KIND_ITEM,
    NDS_MISC_PROC_KIND_EFFECT,
    NDS_MISC_PROC_KIND_GROUND,
    NDS_MISC_PROC_KIND_INTERFACE,
    NDS_MISC_PROC_KIND_PARTICLE,
    NDS_MISC_PROC_KIND_OTHER,
    NDS_MISC_PROC_KIND_COUNT
};
extern volatile u32 gNdsMiscCaptureTicks;
extern volatile u32 gNdsMiscProcDisplayTicks;
extern volatile u32 gNdsMiscProcDisplayKindTicks[NDS_MISC_PROC_KIND_COUNT];
extern volatile u32 gNdsMiscProcDisplayKindCount[NDS_MISC_PROC_KIND_COUNT];
/* R2-07 effect-cost probe, all cumulative. See diagnostics.c. */
extern volatile u32 gNdsMiscTexUploadTicks;
extern volatile u32 gNdsMiscTexUploadCount;
extern volatile u32 gNdsMiscTexUploadBytes;
/* R2-08 effect-submit phase split, all cumulative and NESTED. See diagnostics.c
 * for which span contains which -- they subtract, they do not sum. */
extern volatile u32 gNdsEffectPhaseColorTicks;
extern volatile u32 gNdsEffectPhaseTreeTicks;
extern volatile u32 gNdsEffectPhaseDLTicks;
extern volatile u32 gNdsEffectPhaseFindTicks;
extern volatile u32 gNdsEffectPhaseMaterialTicks;
extern volatile u32 gNdsEffectPhaseMatrixTicks;
extern volatile u32 gNdsEffectPhaseExecTicks;
extern volatile u32 gNdsEffectPhaseTexTicks;
extern volatile u32 gNdsEffectPhaseDLCount;
extern volatile u32 gNdsEffectPhaseNodeCount;
extern volatile u32 gNdsEffectPhaseActive;
/* R2-08 cap-versus-end, cumulative. See diagnostics.c. */
extern volatile u32 gNdsEffectDLCommandTotal;
extern volatile u32 gNdsEffectDLTermCapCount;
extern volatile u32 gNdsEffectDLTermEndCount;
extern volatile u32 gNdsEffectDLTermOtherCount;
extern volatile u32 gNdsEffectDLTermOtherMask;
/* G3 step 0: the UNIQUE-template census. Cumulative; see the recorder in
 * reloc_backend_renderer_dl.c. Every prior G3 figure was an INSTANCE count, and
 * an arena sized from instances is sized wrong by the reuse factor. */
extern volatile u32 gNdsEffectDLCensusUnique;
extern volatile u32 gNdsEffectDLCensusOverflow;
extern volatile u32 gNdsEffectDLCensusStateVariants;
extern volatile u32 gNdsEffectDLCensusCommandVariants;
extern volatile u32 gNdsEffectDLCensusCommandMax;
extern volatile u32 gNdsEffectDLCensusUniqueCommandTotal;
extern volatile u32 gNdsEffectDLTriangleTotal;
extern volatile u32 gNdsEffectDLVertexTotal;
/* The ARENA CONSTANT, measured rather than inferred: the sum over unique
 * templates of the largest geometry each was ever seen to submit. GeomVariants
 * is its confidence -- 0 means culling never moved a template's output, so the
 * maxima are exact. */
extern volatile u32 gNdsEffectDLCensusGeomVariants;
extern volatile u32 gNdsEffectDLCensusTrisMaxTotal;
extern volatile u32 gNdsEffectDLCensusVertsMaxTotal;
/* G3 step 1: the effect GX stream capture. Defined in nds_renderer.c beside the
 * GX record funnel it hooks; the comment there states what it is for and why
 * geometry, colour and matrix words are hashed apart. Arrays are declared
 * unsized so this header does not have to see NDS_GX_CLASS_COUNT, which
 * is itself behind a renderer-config guard. */
extern volatile u32 gNdsEffectPacketWords[];
extern volatile u32 gNdsEffectPacketClassCommands[];
extern volatile u32 gNdsEffectPacketClassWords[];
extern volatile u32 gNdsEffectPacketGeomHash;
extern volatile u32 gNdsEffectPacketColorHash;
extern volatile u32 gNdsEffectPacketMatrixHash;
extern volatile u32 gNdsEffectPacketGeomWords;
extern volatile u32 gNdsEffectPacketColorWords;
extern volatile u32 gNdsEffectPacketMatrixWords;
extern volatile u32 gNdsEffectPacketCaptureCount;
extern volatile u32 gNdsEffectPacketDroppedWords;
extern volatile u32 gNdsEffectPacketFaultCount;
extern volatile u32 gNdsEffectPacketLastWordCount;
extern volatile u32 gNdsEffectPacketTotalGeomWords;
extern volatile u32 gNdsEffectPacketTotalColorWords;
extern volatile u32 gNdsEffectPacketTotalMatrixWords;
extern volatile u32 gNdsEffectPacketTotalMatrixCommands;
extern volatile u32 gNdsEffectPacketTotalVertexCommands;
/* G3 step 2: which predicate in ndsRendererHardwareClassifySubmit refuses the
 * raw path for effect triangles. Binned at the return sites, so index == the
 * deciding predicate; see the table beside the definition. */
extern volatile u32 gNdsEffectSubmitNoZ;
extern volatile u32 gNdsEffectSubmitDecal;
extern volatile u32 gNdsEffectSubmitPrimDepth;
extern volatile u32 gNdsEffectSubmitRangeReject;
extern volatile u32 gNdsEffectSubmitMatrixReject;
extern volatile u32 gNdsEffectSubmitRawCurrent;
extern volatile u32 gNdsEffectSubmitRawSnapshot;
extern volatile u32 gNdsEffectSubmitCrossMatrix;
extern volatile u32 gNdsEffectSubmitTotal;
void ndsEffectPacketCaptureBegin(void);
void ndsEffectPacketCaptureEnd(void);
/* The per-template verdict, accumulated in reloc_backend_renderer_dl.c beside
 * the unique-template census whose key table it reuses. Match and Variant are
 * reported as a PAIR for each of the three word classes: a variant count alone
 * cannot distinguish "invariant" from "never compared". */
extern volatile u32 gNdsEffectPacketTemplates;
extern volatile u32 gNdsEffectPacketTableOverflow;
extern volatile u32 gNdsEffectPacketGeomMatchCount;
extern volatile u32 gNdsEffectPacketGeomVariantCount;
extern volatile u32 gNdsEffectPacketColorMatchCount;
extern volatile u32 gNdsEffectPacketColorVariantCount;
extern volatile u32 gNdsEffectPacketMatrixMatchCount;
extern volatile u32 gNdsEffectPacketMatrixVariantCount;
extern volatile u32 gNdsEffectPacketGeomWordVariantCount;
/* G3 step 3: the traversal-versus-projection split inside Exec. Traversal is
 * derived, never counted -- Exec - TexInExec - Vtx - Tri. */
extern volatile u32 gNdsEffectPhaseVtxTicks;
extern volatile u32 gNdsEffectPhaseVtxCount;
extern volatile u32 gNdsEffectPhaseTriTicks;
extern volatile u32 gNdsEffectPhaseTriCount;
extern volatile u32 gNdsEffectPhaseTexInExecTicks;
/* G3 step 5: the painter depth-slot census. The reserved no-Z bands are 128 v16
 * depths at EACH endpoint -- see NDS_RENDERER_HW_SOURCE_DEPTH_MIN/MAX and the
 * comment on ndsRendererHardwareSourceDepthToV16 -- not 4,096. The 0x1000 in
 * BACKGROUND_START is the v16 representation of clip-space 1.0, not a slot
 * count, and reading it as one overstates the budget 32x. Past 128 a background
 * primitive descends into the source-Z range and starts ordering against stage
 * geometry; past 128 a foreground primitive passes v16 -4096 and leaves the
 * |z| <= w clip volume outright. Derived from the depth counter itself rather
 * than counted at the accessor, because the M3 replay path decrements it in
 * bulk (triangle_count * STEP) without calling the accessor at all. */
extern volatile u32 gNdsPainterSlotFrames;
extern volatile u32 gNdsPainterSlotBgMax;
extern volatile u32 gNdsPainterSlotFgMax;
extern volatile u32 gNdsPainterSlotTotalMax;
extern volatile u32 gNdsPainterSlotBgSum;
extern volatile u32 gNdsPainterSlotFgSum;
extern volatile u32 gNdsPainterSlotBgOverBand;
extern volatile u32 gNdsPainterSlotFgOverBand;
/* CYCLE 98 -- the FTR pre-submission census. The 172,092 ticks/frame between
 * the DObj walk and the first submitted triangle (board, cycle 96/97) split
 * over five seams, and four of them are only worth deleting if their inputs
 * actually change between frames. Nothing here times anything: ITCM is 99.1%
 * full (cycle 95) so no timer-based partition of the draw half can link. These
 * are pure counters answering one question -- did this work reproduce a value
 * it had already computed?
 *
 * The stage settled exactly this question for free with
 * gNdsR2StagePrepareReuseCount/BuildCount at 99.9% reuse (cycle 93). The
 * fighter owner validate had no such pair, so a cache hit and a cache miss were
 * indistinguishable and any deletion here would have been a guess. */
extern volatile u32 gNdsFtrPreValidateReuse;
extern volatile u32 gNdsFtrPreValidateBuild;
extern volatile u32 gNdsFtrPreValidateReject;
/* Walk: the DObj collection's identity, hashed per slot and compared with the
 * previous frame's. Same == the topology the walk rebuilds every frame is the
 * one it built last frame. */
extern volatile u32 gNdsFtrPreWalkSame;
extern volatile u32 gNdsFtrPreWalkVariant;
extern volatile u32 gNdsFtrPreWalkFirst;
/* Material: BuildNativeMaterialSnapshot has no cache at all, so the question is
 * not hit-versus-miss but whether the snapshot it produces equals the one it
 * produced last time for that MObj. Same == the build re-derived a constant.
 * New and Evict separate a genuinely first-seen MObj from table thrash, because
 * a thrashing table reports every call as new and would read as "everything
 * varies". */
extern volatile u32 gNdsFtrPreMatCalls;
extern volatile u32 gNdsFtrPreMatSame;
extern volatile u32 gNdsFtrPreMatVariant;
extern volatile u32 gNdsFtrPreMatNew;
extern volatile u32 gNdsFtrPreMatEvict;
/* Reset: Transient is the bzero-carrying one (70% of the fighter draw's memset
 * traffic per the note at reloc_backend_renderer_dl.c:13474); Runtime is the
 * eleven-store alternative. Both call sites choose between them on
 * detailed_output, and the native owner production path is itself gated on
 * detailed_output == FALSE -- so Transient is PREDICTED 0 on the gate arm.
 * Runtime is its negative control: a zero meaning "already dead" and a zero
 * meaning "the counter never linked" are the same zero, and Runtime is what
 * tells them apart. */
extern volatile u32 gNdsFtrPreResetTransient;
extern volatile u32 gNdsFtrPreResetRuntime;
#endif

/* Cycle 99 -- the baked fighter draw plan (reloc_backend_renderer_dl.c). Hit
 * and Build are its engagement proof: Hit cannot be zero once the plan is
 * baked, and Build is the per-scene bake count, so it also says whether the
 * plan is being thrown away and re-derived. */
extern volatile u32 gNdsFtrPlanHit;
extern volatile u32 gNdsFtrPlanBuild;
extern volatile u32 gNdsRendererProfileTextureUploads;
extern volatile u32 gNdsRendererProfileTextureUploadBytes;
extern volatile u32 gNdsRendererProfileTextureCi4DirectPixels;
extern volatile u32 gNdsRendererProfileTextureBinds;
extern volatile u32 gNdsRendererProfileTextureCacheAliasAvoidCount;
extern volatile u32 gNdsRendererProfileTextureLookupCallCount;
extern volatile u32 gNdsRendererProfileTextureLookupProbeCount;
extern volatile u32 gNdsRendererProfileTextureLookupActiveHitCount;
extern volatile u32 gNdsRendererProfileTextureLookupTableHitCount;
extern volatile u32 gNdsRendererProfileTextureLookupMissCount;
extern volatile u32 gNdsRendererProfileTexel1CompositeCount;
extern volatile u32 gNdsRendererProfileTexel1LoadMatchCount;
extern volatile u32 gNdsRendererProfileTexel1RejectCount;
extern volatile u32 gNdsRendererProfileTexel1FractionRefreshCount;
extern volatile u32 gNdsRendererProfileTextureCacheEvictCount;
extern volatile s32 gNdsRendererProfileTextureCoordMinS;
extern volatile s32 gNdsRendererProfileTextureCoordMaxS;
extern volatile s32 gNdsRendererProfileTextureCoordMinT;
extern volatile s32 gNdsRendererProfileTextureCoordMaxT;
extern volatile u32 gNdsRendererProfileProjectedSubmitFallbackCount;
extern volatile u32 gNdsRendererProfileHardwareVertices;
extern volatile u32 gNdsRendererProfileHardwareTriangles;
extern volatile u32 gNdsRendererProfileHardwareBatchBeginCount;
extern volatile u32 gNdsRendererProfileHardwareBatchReuseCount;
extern volatile u32 gNdsRendererProfileHardwareBatchEndCount;
extern volatile u32 gNdsRendererProfileTexturePrepareCount;
extern volatile u32 gNdsRendererProfileTexturePrepareReuseCount;
extern volatile u32 gNdsRendererProfileCi4IndexCacheBuildCount;
extern volatile u32 gNdsRendererProfileCi4IndexCacheReuseCount;
extern volatile u32 gNdsRendererProfileCi4RepresentativePixelCount;
extern volatile u32 gNdsRendererProfileCi4ReusePixelCount;
extern volatile u32 gNdsRendererProfileHardwareOverLimit;
extern volatile u32 gNdsRendererProfileOracleSamples;
extern volatile u32 gNdsRendererProfileOracleMismatches;
extern volatile u32 gNdsRendererProfileOracleMaxDelta;
extern volatile u32 gNdsRendererProfileMatrixLoadCount;
extern volatile u32 gNdsRendererProfileRawCurrentCandidateCount;
extern volatile u32 gNdsRendererProfileRawCurrentRangeRejectCount;
extern volatile u32 gNdsRendererProfileRawCrossMatrixCount;
extern volatile u32 gNdsRendererProfileSubmitRawCurrentCount;
extern volatile u32 gNdsRendererProfileSubmitRawSnapshotCount;
extern volatile u32 gNdsRendererProfileSubmitProjectedCrossCount;
extern volatile u32 gNdsRendererProfileSubmitProjectedNoZCount;
extern volatile u32 gNdsRendererProfileSubmitProjectedDecalCount;
extern volatile u32 gNdsRendererProfileSubmitProjectedPrimDepthCount;
extern volatile u32 gNdsRendererProfileSubmitProjectedRangeOrMatrixCount;
extern volatile u32 gNdsRendererProfileSubmitRejectCount;
/* Profile-2-only actual divide/clamp counts and error flags; logical demand is
 * derived from the submitted class totals by the renderer verifier. */
extern volatile u32 gNdsRendererProfileHardwareDivideSummary;
extern volatile u32 gNdsRendererProfileSourceVertexLoadCount;
extern volatile u32 gNdsRendererProfileCPUTransformCount;
extern volatile u32 gNdsRendererProfileTransformCacheHitCount;
extern volatile u32 gNdsRendererProfileMatrixSnapshotCreateCount;
extern volatile u32 gNdsRendererProfileMatrixSnapshotReuseCount;
extern volatile u32 gNdsRendererProfileMatrixSnapshotOverflowCount;
extern volatile u32 gNdsRendererProfileMatrixPosTestSamples;
extern volatile u32 gNdsRendererProfileMatrixPosTestMismatches;
extern volatile u32 gNdsRendererProfileMatrixPosTestMaxError;
extern volatile u32 gNdsRendererProfileMatrixPosTestWSignMismatches;
extern volatile u32 gNdsRendererProfileMatrixPosTestClipMismatches;
extern volatile u32 gNdsRendererProfileMatrixPosTestMatrixWordSamples;
extern volatile u32 gNdsRendererProfileMatrixPosTestDropped;
extern volatile u32 gNdsRendererProfileHWVertexSaturateCount;
extern volatile u32 gNdsRendererProfileNearPlaneTriangleRejectCount;
extern volatile u32 gNdsRendererDepthStageSamples;
extern volatile s32 gNdsRendererDepthStageMin;
extern volatile s32 gNdsRendererDepthStageMax;
extern volatile s32 gNdsRendererDepthStageWMin;
extern volatile s32 gNdsRendererDepthStageWMax;
extern volatile u32 gNdsRendererDepthFighterP0Samples;
extern volatile s32 gNdsRendererDepthFighterP0Min;
extern volatile s32 gNdsRendererDepthFighterP0Max;
extern volatile s32 gNdsRendererDepthFighterP0WMin;
extern volatile s32 gNdsRendererDepthFighterP0WMax;
extern volatile u32 gNdsRendererDepthFighterP1Samples;
extern volatile s32 gNdsRendererDepthFighterP1Min;
extern volatile s32 gNdsRendererDepthFighterP1Max;
extern volatile s32 gNdsRendererDepthFighterP1WMin;
extern volatile s32 gNdsRendererDepthFighterP1WMax;
extern volatile u32 gNdsIFCommonHUDRecordCount;
extern volatile u32 gNdsIFCommonHUDObjectMask;
extern volatile u32 gNdsIFCommonHUDP0DamageCurrent;
extern volatile u32 gNdsIFCommonHUDP1DamageCurrent;
extern volatile u32 gNdsIFCommonHUDP2DamageCurrent;
extern volatile u32 gNdsIFCommonHUDP3DamageCurrent;
extern volatile u32 gNdsIFCommonHUDP0DamageMax;
extern volatile u32 gNdsIFCommonHUDP1DamageMax;
extern volatile u32 gNdsIFCommonHUDP2DamageMax;
extern volatile u32 gNdsIFCommonHUDP3DamageMax;
extern volatile u32 gNdsIFCommonHUDP0DigitCount;
extern volatile u32 gNdsIFCommonHUDP1DigitCount;
extern volatile u32 gNdsIFCommonHUDP2DigitCount;
extern volatile u32 gNdsIFCommonHUDP3DigitCount;
extern volatile u32 gNdsIFCommonHUDP0Digits;
extern volatile u32 gNdsIFCommonHUDP1Digits;
extern volatile u32 gNdsIFCommonHUDP2Digits;
extern volatile u32 gNdsIFCommonHUDP3Digits;
extern volatile u32 gNdsIFCommonHUDP0StockCurrent;
extern volatile u32 gNdsIFCommonHUDP1StockCurrent;
extern volatile u32 gNdsIFCommonHUDP2StockCurrent;
extern volatile u32 gNdsIFCommonHUDP3StockCurrent;
extern volatile u32 gNdsIFCommonHUDP0StockMin;
extern volatile u32 gNdsIFCommonHUDP1StockMin;
extern volatile u32 gNdsIFCommonHUDP2StockMin;
extern volatile u32 gNdsIFCommonHUDP3StockMin;
extern volatile u32 gNdsIFCommonHUDP0StockMax;
extern volatile u32 gNdsIFCommonHUDP1StockMax;
extern volatile u32 gNdsIFCommonHUDP2StockMax;
extern volatile u32 gNdsIFCommonHUDP3StockMax;
extern volatile u32 gNdsIFCommonHUDActivePlayerMask;
extern volatile u32 gNdsIFCommonHUDShowDamageMask;
extern volatile u32 gNdsIFCommonHUDDamageFlashMask;
extern volatile u32 gNdsIFCommonHUDSingleStockMask;
extern volatile u32 gNdsIFCommonHUDCPUPlayerMask;
extern volatile u32 gNdsIFCommonHUDP0FighterKind;
extern volatile u32 gNdsIFCommonHUDP1FighterKind;
extern volatile u32 gNdsIFCommonHUDP2FighterKind;
extern volatile u32 gNdsIFCommonHUDP3FighterKind;
extern volatile u32 gNdsIFCommonHUDP0Level;
extern volatile u32 gNdsIFCommonHUDP1Level;
extern volatile u32 gNdsIFCommonHUDP2Level;
extern volatile u32 gNdsIFCommonHUDP3Level;
extern volatile u32 gNdsIFCommonHUDP0Costume;
extern volatile u32 gNdsIFCommonHUDP1Costume;
extern volatile u32 gNdsIFCommonHUDP2Costume;
extern volatile u32 gNdsIFCommonHUDP3Costume;
extern volatile u32 gNdsIFCommonHUDP0LowerStock;
extern volatile u32 gNdsIFCommonHUDP1LowerStock;
extern volatile u32 gNdsIFCommonHUDP2LowerStock;
extern volatile u32 gNdsIFCommonHUDP3LowerStock;
extern volatile u32 gNdsIFCommonHUDTimeRemain;
extern volatile u32 gNdsIFCommonHUDTimerLimit;
extern volatile u32 gNdsIFCommonHUDTimerStarted;
extern volatile u32 gNdsIFCommonHUDTimerVisible;
extern volatile u32 gNdsIFCommonHUDGameStatus;
extern volatile u32 gNdsIFCommonHUDLowerRouteMask;
extern volatile u32 gNdsIFCommonHUDLowerTextMode;
extern volatile u32 gNdsIFCommonHUDLowerRouteCount;
extern volatile u32 gNdsIFCommonHUDLowerTimerRouteCount;
extern volatile u32 gNdsIFCommonHUDLowerDamageRouteCount;
extern volatile u32 gNdsIFCommonHUDTopGenericPassCount;
void ndsFighterMarioFoxNaturalMotionPrepare(void);
s32 ndsFighterMarioFoxNaturalMotionUpdateEnabled(void);
void ndsFighterMarioFoxNaturalMotionRunVSBattleUpdate(void);
extern volatile u32 gNdsFighterMarioFoxGCDrawAllLoopResult;
extern volatile u32 gNdsFighterGCDrawAllLoopPrepared;
extern volatile u32 gNdsFighterGCDrawAllLoopPreviewWidth;
extern volatile u32 gNdsFighterGCDrawAllLoopPreviewHeight;
extern volatile u32 gNdsFighterGCDrawAllLoopPreviewPitch;
extern volatile u32 gNdsFighterGCDrawAllLoopPreviewReady;
extern volatile u32 gNdsFighterGCDrawAllLoopPreviewCommitBefore;
extern volatile u32 gNdsFighterGCDrawAllLoopPreviewCommitAfter;
extern volatile u32 gNdsFighterGCDrawAllLoopPreviewCommitDelta;
extern volatile u32 gNdsFighterGCDrawAllLoopDrawFrameCount;
extern volatile u32 gNdsFighterGCDrawAllLoopDisplayCallbackCount;
extern volatile u32 gNdsFighterGCDrawAllLoopP0PixelCount;
extern volatile u32 gNdsFighterGCDrawAllLoopP1PixelCount;
extern volatile u32 gNdsFighterGCDrawAllLoopTotalPixelCount;
extern volatile s32 gNdsFighterGCDrawAllLoopP0ScreenYMin;
extern volatile s32 gNdsFighterGCDrawAllLoopP1ScreenYMin;
extern volatile u32 gNdsFighterMarioFoxStageGCDrawAllLoopResult;
extern volatile u32 gNdsFighterMarioFoxStageGCDrawAllLoopSafeResult;
extern volatile u32 gNdsFighterMarioFoxStageGCDrawAllLoopMask;
extern volatile u32 gNdsFighterMarioFoxStageGCDrawAllLoopDeferredMask;
extern volatile u32 gNdsFighterMarioFoxStageGCDrawAllLoopCount;
extern volatile u32 gNdsStageGCDrawAllLoopPrepared;
extern volatile u32 gNdsStageGCDrawAllLoopBaseResultSeen;
extern volatile u32 gNdsStageGCDrawAllLoopDrawAllCount;
extern volatile u32 gNdsStageGCDrawAllLoopCameraCallbackCount;
extern volatile u32 gNdsStageGCDrawAllLoopCapturedDisplayCount;
extern volatile u32 gNdsStageGCDrawAllLoopLayerCaptureMask;
extern volatile u32 gNdsStageGCDrawAllLoopMapCaptureMask;
extern volatile u32 gNdsStageGCDrawAllLoopDObjDrawCallbackCount;
extern volatile u32 gNdsStageGCDrawAllLoopDObjDrawKindMask;
extern volatile u32 gNdsStageGCDrawAllLoopLayerDObjMask;
extern volatile u32 gNdsStageGCDrawAllLoopMapDObjMask;
extern volatile u32 gNdsStageGCDrawAllLoopLayerDLReadyMask;
extern volatile u32 gNdsStageGCDrawAllLoopMapDLReadyMask;
extern volatile u32 gNdsStageGCDrawAllLoopLayerMObjMask;
extern volatile u32 gNdsStageGCDrawAllLoopMapMObjMask;
extern volatile u32 gNdsStageGCDrawAllLoopNonStageCaptureCount;
extern volatile u32 gNdsStageGCDrawAllLoopFighterDisplayCallbackCount;
extern volatile u32 gNdsStageGCDrawAllLoopUnexpectedSceneCount;
extern volatile u32 gNdsStageGCDrawAllLoopManualDisplayCallCount;
extern volatile u32 gNdsStageGCDrawAllLoopGObjCountBefore;
extern volatile u32 gNdsStageGCDrawAllLoopGObjCountAfter;
extern volatile u32 gNdsStageGCDrawAllLoopGObjCountDelta;
extern volatile u32 gNdsStageGCDrawAllLoopPreviewCommitDelta;
extern volatile u32 gNdsStageGCDrawAllLoopTotalPixelCount;
extern volatile u32 gNdsStageGCDrawAllLoopCompatMask;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareSubmitCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareTriangleCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareTextureBindCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareTextureUploadCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareTextureReadyCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareTextureRejectCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareTextureFormatMask;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareTextureMaxWidth;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareTextureMaxHeight;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareFighterSubmitCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareFighterTriangleCount;
extern volatile u32 gNdsWeaponRendererCaptureCount;
extern volatile u32 gNdsWeaponRendererDObjDrawCount;
extern volatile u32 gNdsWeaponRendererSubmitCount;
extern volatile u32 gNdsWeaponRendererVisibleDrawCount;
extern volatile u32 gNdsWeaponRendererTriangleCount;
extern volatile u32 gNdsWeaponRendererTextureReadyCount;
extern volatile u32 gNdsWeaponRendererTextureRejectCount;
extern volatile u32 gNdsWeaponRendererKindMask;
extern volatile u32 gNdsWeaponRendererCallbackKind;
extern volatile u32 gNdsWeaponRendererNoZCount;
extern volatile u32 gNdsWeaponRendererMovingDrawCount;
extern volatile u32 gNdsWeaponRendererLastXBits;
extern volatile u32 gNdsWeaponRendererLastYBits;
extern volatile u32 gNdsWeaponRendererFireballSubmitCount;
extern volatile u32 gNdsWeaponRendererFireballTriangleCount;
extern volatile u32 gNdsWeaponRendererFireballVisibleDrawCount;
extern volatile u32 gNdsWeaponRendererRejectedDrawCount;
extern volatile u32 gNdsWeaponRendererFireballCustom47AppliedCount;
extern volatile u32 gNdsWeaponRendererFireballFirstXBits;
extern volatile u32 gNdsWeaponRendererFireballFirstYBits;
extern volatile u32 gNdsWeaponRendererFireballLastXBits;
extern volatile u32 gNdsWeaponRendererFireballLastYBits;
extern volatile u32 gNdsWeaponRendererBlasterSubmitCount;
extern volatile u32 gNdsWeaponRendererBlasterTriangleCount;
extern volatile u32 gNdsWeaponRendererBlasterVisibleDrawCount;
extern volatile u32 gNdsItemRendererCaptureCount;
extern volatile u32 gNdsItemRendererDObjDrawCount;
extern volatile u32 gNdsItemRendererSubmitCount;
extern volatile u32 gNdsItemRendererVisibleDrawCount;
extern volatile u32 gNdsItemRendererTriangleCount;
extern volatile u32 gNdsItemRendererTextureReadyCount;
extern volatile u32 gNdsItemRendererTextureRejectCount;
extern volatile u32 gNdsItemRendererKindMask;
extern volatile u32 gNdsItemRendererRejectedDrawCount;
extern volatile u32 gNdsItemRendererLastHead;
extern volatile u32 gNdsItemRendererLastColorMask;
extern volatile u32 gNdsItemRendererLastEnvColor;
extern volatile u32 gNdsItemRendererLastOtherModeL;
extern volatile u32 gNdsItemRendererLastOtherModeH;
extern volatile u32 gNdsItemRendererAttach52BuildCount;
extern volatile u32 gNdsFoxBlasterQuadDrawCount;
extern volatile u32 gNdsFoxBlasterQuadFallbackCount;
extern volatile u32 gNdsFoxBlasterGlowAOTSpawnCount;
extern volatile u32 gNdsFoxBlasterGlowAOTDrawCount;
extern volatile u32 gNdsFoxBlasterGlowAOTFallbackCount;
extern volatile u32 gNdsFoxBlasterGlowAOTMissCount;
extern volatile u32 gNdsRendererFoxBlasterGlowPrepareCount;
extern volatile u32 gNdsRendererFoxBlasterGlowFailCount;
extern volatile u32 gNdsRendererFoxBlasterGlowBytes;
/* ndsRendererAdapterPrepareInitialMatrices' verdict for an EFFECT submit.
 * Mask bits: 1 camera projection valid, 2 camera modelview valid, 4 DObj world
 * valid, 8/16 projection/modelview pointer non-NULL before the 0x47 rewrite,
 * 32/64 the same two after it. */
extern volatile u32 gNdsRendererAdapterEffectPrepMask;
extern volatile u32 gNdsRendererAdapterCustom47AppliedCount;
extern volatile u32 gNdsRendererAdapterCustom47RejectCount;
extern volatile u32 gNdsRendererAdapterCustom47TranslationMismatchCount;
extern volatile u32 gNdsRendererAdapterCustom47LastKinds;
extern volatile u32 gNdsRendererAdapterCustom47LastRotateXBits;
extern volatile u32 gNdsRendererAdapterCustom47LastRotateYBits;
extern volatile u32 gNdsRendererAdapterCustom47LastTranslateX20p12;
extern volatile u32 gNdsRendererAdapterCustom47LastTranslateY20p12;
extern volatile u32 gNdsRendererAdapterCustom47LastTranslateZ20p12;
/* How many of the applications above took the kind-44 branch
 * (nGCMatrixKindRecalcRotRpyRSca, the shield's billboard) rather than the
 * original 0x47 one. The two share the rewrite but not the orientation, and
 * this is the engagement control that says whether the P1 scene contains any
 * kind-44 DObj other than the source effect trees. */
extern volatile u32 gNdsFighterDisplayContractSelectedCount;
extern volatile u32 gNdsFighterDisplayContractSubmittedCount;
extern volatile u32 gNdsFighterDisplayContractGeometryMode;
extern volatile u32 gNdsFighterDisplayContractCycleType;
extern volatile u32 gNdsFighterDisplayContractRenderMode;
extern volatile u32 gNdsFighterDisplayContractMaterialLightSeedCount;
extern volatile u32 gNdsFighterDisplayContractMaterialLight1;
extern volatile u32 gNdsFighterDisplayContractMaterialLight2;
/* Fighter draw-contract memo. Route is poke-able (.data, own cache line);
 * the rest are engagement counters. See src/port/diagnostics.c. It is a
 * 32-byte cell rather than a bare u32 so nothing else can share its line and
 * stamp a poke back; poke `gNdsFtrDrawMemoRoute.route`. */
typedef struct NDSFtrDrawMemoRouteCell {
    volatile u32 route;
    u32 line_pad[7];
} NDSFtrDrawMemoRouteCell;
extern NDSFtrDrawMemoRouteCell gNdsFtrDrawMemoRoute;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareCarrySeedCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareCarryCaptureCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareCarryTextureSeedCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareCarryTileSeedCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareCarryShortTextureSeedCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareCarryShortTileSeedCount;
extern volatile u32 gNdsStageGCDrawAllLoopHardwareCarrySegmentSeedCount;
void ndsFighterMarioFoxStageGCDrawAllLoopPrepare(void);
void ndsFighterMarioFoxStageGCDrawAllLoopSubmitHardwareFrame(void);
void ndsFighterMarioFoxStageGCDrawAllLoopPresentHardwareFrame(void);
s32 ndsFighterMarioFoxStageGCDrawAllLoopProofActive(void);
void ndsStageGCDrawAllLoopRecordCameraCallback(void);
s32 ndsStageGCDrawAllLoopRecordCapturedDisplay(void *camera_gobj,
                                               void *display_gobj,
                                               s32 link_id);
void ndsStageGCDrawAllLoopRecordDObjDraw(void *gobj, u32 kind);
void ndsRendererAdapterBeginStageTraversal(void);
void ndsRendererAdapterEndStageTraversal(void);
void ndsRendererAdapterSubmitStageDObj(void *dobj, u32 kind,
                                       void *camera_gobj,
                                       u32 initial_geometry_mode);
/* Sector Z's Arwing tree (TREE_DLLINKS) in two passes: every root's matrices,
 * then every list in source order. Returns FALSE, having drawn nothing, when
 * the walk cannot hold the tree; *submitted_dobjs is the scan's DObj count. */
s32 ndsRendererAdapterSubmitArwingTwoPass(void *root_dobj, void *camera_gobj,
                                          u32 initial_geometry_mode,
                                          u32 *submitted_dobjs);
/* Seat the current camera's cached world->clip matrices for a direct
 * world-space quad. Used by the Fox blaster lab to avoid rebuilding the
 * BattleShip CObj matrices once per laser. */
s32 ndsRendererAdapterSetWorldQuadCamera(void *camera_gobj);
s32 ndsRendererAdapterBeginParticleViewPass(void *camera_gobj);
/* Same cached camera seat, plus the source-equivalent normalized billboard
 * right/up axes in Q20.12. Only cameras whose native builder publishes that
 * basis return TRUE; callers retain their source path for every other shape. */
s32 ndsRendererAdapterSetWorldQuadCameraBasisQ12(void *camera_gobj,
                                                  s32 *right_q12,
                                                  s32 *up_q12);
/* Same submit, but it walks the DObj tree (child + sibling chain) the way
 * objdisplay.c's gcDrawDObjTree does. Effect-only on purpose: doing it on the
 * stage entry above cost a whole VBlank on measured frames. */
/* Bracket a source proc_display so the effect submit can recover the prim/env
 * it emitted: Mark before the proc runs, Capture before the tree is submitted.
 * Both are no-ops outside NDS_RENDERER_HW_TRIANGLES. */
void ndsRendererAdapterMarkDisplayProcHeads(void);
void ndsRendererAdapterCaptureDisplayProcColors(void);
void ndsRendererAdapterCaptureItemDisplayProcState(void);
void ndsRendererAdapterSubmitEffectDObjTree(void *dobj, u32 kind,
                                            void *camera_gobj,
                                            u32 initial_geometry_mode);
void ndsRendererAdapterSubmitWeaponDObjTree(void *dobj, u32 kind,
                                            void *camera_gobj,
                                            u32 initial_geometry_mode);
void ndsRendererAdapterSubmitItemDObjTree(void *dobj, u32 kind,
                                          void *camera_gobj,
                                          u32 initial_geometry_mode);
void ndsRendererAdapterSubmitItemDObjTreeReplay(void *dobj, u32 kind,
                                                void *camera_gobj,
                                                u32 initial_geometry_mode,
                                                u32 item_kind);
s32 ndsRendererAdapterPrepareNativeStageOwner(void *camera_gobj);
s32 ndsRendererAdapterCommitNativeStageDisplay(void *display_gobj,
                                                s32 link_id);
void ndsRendererAdapterFinishNativeStageOwner(void);
extern volatile u32 gNdsStageCollisionLoopPrepared;
extern volatile u32 gNdsStageCollisionLoopGeometryReady;
extern volatile u32 gNdsStageCollisionLoopGroundDataReady;
extern volatile u32 gNdsStageCollisionLoopYakumonoCount;
extern volatile u32 gNdsStageCollisionLoopMapObjCount;
extern volatile u32 gNdsStageCollisionLoopFloorLineCount;
extern volatile u32 gNdsStageCollisionLoopTotalLineCount;
extern volatile u32 gNdsStageCollisionLoopGeometryProjectCallCount;
extern volatile u32 gNdsStageCollisionLoopBadVertexCount;
extern volatile u32 gNdsStageCollisionLoopP0HitCount;
extern volatile s32 gNdsStageCollisionLoopP0FloorLineID;
extern volatile s32 gNdsStageCollisionLoopP1FloorLineID;
extern volatile u32 gNdsStageCollisionLoopP0FloorKind;
extern volatile u32 gNdsStageCollisionLoopP1FloorKind;
extern volatile s32 gNdsStageCollisionLoopFloorGroupID;
extern volatile u32 gNdsStageCollisionLoopFloorGroupCount;
extern volatile s32 gNdsStageCollisionLoopFloorLineMin;
extern volatile s32 gNdsStageCollisionLoopFloorLineMaxExclusive;
extern volatile u32 gNdsStageCollisionLoopNonFloorCandidateCount;
extern volatile u32 gNdsStageCollisionLoopUnsafeFallbackAfterPrepareCount;
void ndsStageCollisionLoopPrepareRuntime(void);
extern volatile s32 gNdsStageFloorFollowLoopP0FloorLineID;
extern volatile s32 gNdsStageFloorFollowLoopP1FloorLineID;
extern volatile u32 gNdsStageFloorFollowLoopP0FloorKind;
extern volatile u32 gNdsStageFloorFollowLoopP1FloorKind;
extern volatile u32 gNdsStageFloorFollowLoopP0StatusFinal;
extern volatile u32 gNdsStageFloorFollowLoopP1StatusFinal;
extern volatile u32 gNdsStageFloorFollowLoopP0GAFinal;
extern volatile u32 gNdsStageFloorFollowLoopP1GAFinal;
extern volatile s32 gNdsStageFloorEdgeLoopSelectedLineID;
extern volatile u32 gNdsStageFloorEdgeLoopSelectedLineKind;
extern volatile s32 gNdsStageFloorEdgeLoopP0MinDistMilli;
extern volatile s32 gNdsStageFloorEdgeLoopP1MinDistMilli;
extern volatile s32 gNdsStageMPProcessFloorLoopP0FinalLineID;
extern volatile s32 gNdsStageMPProcessFloorLoopP1FinalLineID;
extern volatile u32 gNdsStageMPUpdateFloorLoopCheckCliffEdgeCallCount;
extern volatile s32 gNdsStageMPUpdateFloorLoopP0FinalLineID;
extern volatile s32 gNdsStageMPUpdateFloorLoopP1FinalLineID;
extern volatile s32 gNdsStageMPSweepFloorLoopProbeLineID;
extern volatile s32 gNdsStageMPSweepFloorLoopAltLineID;
extern volatile s32 gNdsStageMPSweepFloorLoopP0FinalLineID;
extern volatile s32 gNdsStageMPSweepFloorLoopP1FinalLineID;
extern volatile u32 gNdsStageMPPlatformSpeedFloorLoopYakumonoID;
extern volatile u32 gNdsStageMPPlatformSpeedFloorLoopAnimPlayCount;
extern volatile u32 gNdsStageMPPlatformSpeedFloorLoopAnimStatusAfter;
extern volatile s32 gNdsStageMPPlatformSpeedFloorLoopAnimSpeedXMilli;
extern volatile s32 gNdsStageMPPlatformSpeedFloorLoopAnimSpeedYMilli;
extern volatile s32 gNdsStageMPPlatformSpeedFloorLoopAnimSpeedZMilli;
extern volatile u32 gNdsStageInishieScaleLoopUnsafeCount;
extern volatile u32 gNdsStageInishieScaleLoopSourceSetupStep;
extern volatile u32 gNdsStageInishieScaleLoopSourceSetupGObjReadyMask;
extern volatile u32 gNdsStageInishieScaleLoopSourceDisplayMask;
extern volatile u32 gNdsStageInishieScaleLoopSourceDisplayCount;
extern volatile u32 gNdsStageInishieScaleLoopSourceDisplayCommands;
extern volatile u32 gNdsStageInishieScaleLoopSourceDisplayTriangles;
extern volatile u32 gNdsStageInishieScaleLoopSourceDisplayBlocker;
extern volatile u32 gNdsStageInishieScaleLoopSourceTextureMask;
extern volatile u32 gNdsStageInishieScaleLoopSourceTextureCommands;
extern volatile u32 gNdsStageInishieScaleLoopSourceTextureImage;
extern volatile u32 gNdsStageInishieScaleLoopSourceTextureSize;
extern volatile u32 gNdsStageInishieScaleLoopSourcePreviewMask;
extern volatile u32 gNdsStageInishieScaleLoopSourcePreviewDObjCount;
extern volatile u32 gNdsStageInishieScaleLoopSourcePreviewVertexCount;
extern volatile u32 gNdsStageInishieScaleLoopSourcePreviewTriangleCount;
extern volatile u32 gNdsStageInishieScaleLoopSourcePreviewValidTriangleCount;
extern volatile u32 gNdsStageInishieScaleLoopSourcePreviewPixelCount;
extern volatile u32 gNdsStageInishieScaleLoopSourcePreviewCommitDelta;
extern volatile u32 gNdsStageInishieScaleLoopSourcePreviewBlocker;
extern volatile u32 gNdsStageMPCliffWaitDamageLoopPassiveStandGroundSetCount;
extern volatile u32 gNdsStageMPCliffWaitDamageLoopPassiveGroundSetCount;
extern volatile s32 gNdsStageMPLiveHitDamageLoopShieldPlayer;
extern volatile u32 gNdsStageMPLiveHitDamageLoopShieldEffectCount;
extern volatile s32 gNdsStageMPLiveHitDamageLoopShieldEffectSize;
s32 ndsFighterMarioFoxLivePreviewUpdateEnabled(void);
extern volatile u32 gNdsSCVSBattleStageResult;
extern volatile u32 gNdsSCVSBattleStageMask;
extern volatile u32 gNdsSCVSBattleStageGKind;
extern volatile u32 gNdsSCVSBattleStageGroundDataReady;
extern volatile u32 gNdsSCVSBattleStageGeometryReady;
extern volatile u32 gNdsSCVSBattleStageMapNodesReady;
extern volatile u32 gNdsSCVSBattleStageBGM;
extern volatile u32 gNdsSCVSBattleStageLightAngleXBits;
extern volatile u32 gNdsSCVSBattleStageLightAngleYBits;
extern volatile u32 gNdsSCVSBattleStageDeferredMask;
extern volatile u32 gNdsPupupuGroundSetupResult;
extern volatile u32 gNdsPupupuGroundDisplayResult;
extern volatile u32 gNdsPupupuGroundGObjResult;
extern volatile u32 gNdsPupupuGroundSetupMask;
extern volatile u32 gNdsPupupuGroundDeferredMask;
extern volatile u32 gNdsPupupuGroundLayerGObjCount;
extern volatile u32 gNdsPupupuGroundLayerGObjMask;
extern volatile u32 gNdsPupupuGroundLayerDObjMask;
extern volatile u32 gNdsPupupuGroundLayerMObjMask;
extern volatile u32 gNdsPupupuGroundLayerAnimMask;
extern volatile u32 gNdsPupupuGroundMapGObjCount;
extern volatile u32 gNdsPupupuGroundMapGObjMask;
extern volatile u32 gNdsPupupuGroundMapHeadReady;
extern volatile u32 gNdsPupupuGroundMapHeadOffset;
extern volatile u32 gNdsPupupuGroundRootGObjID;
extern volatile u32 gNdsPupupuGroundWhispyEyesGObjID;
extern volatile u32 gNdsPupupuGroundWhispyMouthGObjID;
extern volatile u32 gNdsPupupuGroundFlowersBackGObjID;
extern volatile u32 gNdsPupupuGroundFlowersFrontGObjID;
extern volatile u32 gNdsPupupuGroundParticleBankID;
extern volatile u32 gNdsPupupuGroundProcessAttachCount;
extern volatile u32 gNdsPupupuGroundGObjCountBefore;
extern volatile u32 gNdsPupupuGroundGObjCountAfter;
extern volatile u32 gNdsPupupuGroundDObjCountAfter;
extern volatile u32 gNdsPupupuGroundMObjCountAfter;
extern volatile u32 gNdsPupupuGroundAObjCountAfter;
extern volatile u32 gNdsPupupuWaterStillFreezeCount;
extern volatile u32 gNdsPupupuWaterStillFreezeFailCount;
extern volatile u32 gNdsPupupuWaterStillFreezeResult;
extern volatile u32 gNdsPupupuUpdateResult;
extern volatile u32 gNdsPupupuUpdateMask;
extern volatile u32 gNdsPupupuUpdateTickCount;
extern volatile u32 gNdsPupupuUpdateGameStatusBefore;
extern volatile u32 gNdsPupupuUpdateGameStatusAfter;
extern volatile u32 gNdsPupupuUpdateWhispyStatusBefore;
extern volatile u32 gNdsPupupuUpdateWhispyStatusAfterFirst;
extern volatile u32 gNdsPupupuUpdateWhispyStatusAfterFinal;
extern volatile u32 gNdsPupupuUpdateWindWaitBefore;
extern volatile u32 gNdsPupupuUpdateWindWaitAfterFirst;
extern volatile u32 gNdsPupupuUpdateWindWaitAfterFinal;
extern volatile u32 gNdsPupupuUpdateBlinkWaitBefore;
extern volatile u32 gNdsPupupuUpdateBlinkWaitAfterFinal;
extern volatile u32 gNdsPupupuUpdateFlowersBackStatusBefore;
extern volatile u32 gNdsPupupuUpdateFlowersBackStatusAfterFinal;
extern volatile u32 gNdsPupupuUpdateFlowersFrontStatusBefore;
extern volatile u32 gNdsPupupuUpdateFlowersFrontStatusAfterFinal;
extern volatile u32 gNdsPupupuUpdateMapGObjMaskBefore;
extern volatile u32 gNdsPupupuUpdateMapGObjMaskAfter;
extern volatile u32 gNdsPupupuUpdateGObjCountBefore;
extern volatile u32 gNdsPupupuUpdateGObjCountAfter;
extern volatile u32 gNdsPupupuUpdateVelPushCount;
extern volatile u32 gNdsPupupuUpdateQuakeCount;
extern volatile u32 gNdsPupupuUpdateParticleScriptCount;
extern volatile u32 gNdsPupupuUpdateWindFGMCount;
extern volatile u32 gNdsOpeningRoomMaterialDLProbeResult;
extern volatile u32 gNdsOpeningRoomMaterialDLProbeBlocker;
extern volatile u32 gNdsOpeningRoomMaterialDLProbeFirstDL;
extern volatile u32 gNdsOpeningRoomMaterialDLExpandResult;
extern volatile u32 gNdsOpeningRoomMaterialDLExpandBlocker;
extern volatile u32 gNdsOpeningRoomMaterialDLExpandFirstDL;
extern volatile u32 gNdsOpeningRoomOutsideAssetMask;
extern volatile u32 gNdsOpeningRoomOutsideDisplayListOffset;
extern volatile u32 gNdsOpeningRoomOutsideCreateResult;
extern volatile u32 gNdsOpeningRoomOutsideCreateMask;
extern volatile u32 gNdsOpeningRoomOutsideCreateGObjCount;
extern volatile u32 gNdsOpeningRoomOutsideGObjDelta;
extern volatile u32 gNdsOpeningRoomOutsideDObjDelta;
extern volatile u32 gNdsOpeningRoomOutsideXObjDelta;
extern volatile u32 gNdsOpeningRoomOutsideDisplaySet;
extern volatile u32 gNdsOpeningRoomHazeAssetMask;
extern volatile u32 gNdsOpeningRoomHazeDisplayListOffset;
extern volatile u32 gNdsOpeningRoomHazeCreateResult;
extern volatile u32 gNdsOpeningRoomHazeCreateMask;
extern volatile u32 gNdsOpeningRoomHazeCreateGObjCount;
extern volatile u32 gNdsOpeningRoomHazeGObjDelta;
extern volatile u32 gNdsOpeningRoomHazeDObjDelta;
extern volatile u32 gNdsOpeningRoomHazeXObjDelta;
extern volatile u32 gNdsOpeningRoomHazeDisplaySet;
extern volatile u32 gNdsOpeningRoomSunlightAssetMask;
extern volatile u32 gNdsOpeningRoomSunlightDisplayListOffset;
extern volatile u32 gNdsOpeningRoomSunlightCreateResult;
extern volatile u32 gNdsOpeningRoomSunlightCreateMask;
extern volatile u32 gNdsOpeningRoomSunlightCreateGObjCount;
extern volatile u32 gNdsOpeningRoomSunlightGObjDelta;
extern volatile u32 gNdsOpeningRoomSunlightDObjDelta;
extern volatile u32 gNdsOpeningRoomSunlightXObjDelta;
extern volatile u32 gNdsOpeningRoomSunlightDisplaySet;
extern volatile u32 gNdsOpeningRoomDeskAssetMask;
extern volatile u32 gNdsOpeningRoomDeskDObjOffset;
extern volatile u32 gNdsOpeningRoomDeskCreateResult;
extern volatile u32 gNdsOpeningRoomDeskCreateMask;
extern volatile u32 gNdsOpeningRoomDeskCreateGObjCount;
extern volatile u32 gNdsOpeningRoomDeskGObjDelta;
extern volatile u32 gNdsOpeningRoomDeskDObjDelta;
extern volatile u32 gNdsOpeningRoomDeskXObjDelta;
extern volatile u32 gNdsOpeningRoomDeskDisplaySet;
extern volatile u32 gNdsOpeningRoomSunlightEjectResult;
extern volatile u32 gNdsOpeningRoomSunlightEjectBeforeGObjCount;
extern volatile u32 gNdsOpeningRoomSunlightEjectAfterGObjCount;
extern volatile u32 gNdsOpeningRoomSunlightEjectUnlinkedMask;
extern volatile u32 gNdsOpeningRoomOverlayCreateResult;
extern volatile u32 gNdsOpeningRoomOverlayDisplaySet;
extern volatile u32 gNdsOpeningRoomOverlayAlphaInit;
extern volatile u32 gNdsOpeningRoomOverlayCreateGObjCount;
extern volatile u32 gNdsOpeningRoomOverlayEjectResult;
extern volatile u32 gNdsOpeningRoomOverlayEjectBeforeGObjCount;
extern volatile u32 gNdsOpeningRoomOverlayEjectAfterGObjCount;
extern volatile u32 gNdsOpeningRoomOverlayEjectUnlinkedMask;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayCreateResult;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayCreateMask;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayCreateTick;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayCreateGObjCount;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayGObjDelta;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayDisplaySet;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayAlphaInit;
extern volatile u32 gNdsOpeningRoomSpotlightAssetMask;
extern volatile u32 gNdsOpeningRoomSpotlightDisplayListOffset;
extern volatile u32 gNdsOpeningRoomSpotlightMObjOffset;
extern volatile u32 gNdsOpeningRoomSpotlightMatAnimOffset;
extern volatile u32 gNdsOpeningRoomSpotlightCreateResult;
extern volatile u32 gNdsOpeningRoomSpotlightCreateMask;
extern volatile u32 gNdsOpeningRoomSpotlightCreateTick;
extern volatile u32 gNdsOpeningRoomSpotlightCreateGObjCount;
extern volatile u32 gNdsOpeningRoomSpotlightGObjDelta;
extern volatile u32 gNdsOpeningRoomSpotlightDObjDelta;
extern volatile u32 gNdsOpeningRoomSpotlightXObjDelta;
extern volatile u32 gNdsOpeningRoomSpotlightMObjDelta;
extern volatile u32 gNdsOpeningRoomSpotlightAObjDelta;
extern volatile u32 gNdsOpeningRoomSpotlightDisplaySet;
extern volatile u32 gNdsOpeningRoomSpotlightProcessSet;
extern volatile u32 gNdsOpeningRoomSpotlightMObjSet;
extern volatile u32 gNdsOpeningRoomSpotlightMatAnimSet;
extern volatile u32 gNdsOpeningRoomSpotlightPositionSet;
extern volatile u32 gNdsOpeningRoomScene1CameraCreateResult;
extern volatile u32 gNdsOpeningRoomScene1CameraCreateMask;
extern volatile u32 gNdsOpeningRoomScene1CameraCreateGObjCount;
extern volatile u32 gNdsOpeningRoomScene1CameraGObjDelta;
extern volatile u32 gNdsOpeningRoomScene1CameraCObjDelta;
extern volatile u32 gNdsOpeningRoomScene1CameraXObjDelta;
extern volatile u32 gNdsOpeningRoomScene1CameraAObjDelta;
extern volatile u32 gNdsOpeningRoomScene1CameraDisplaySet;
extern volatile u32 gNdsOpeningRoomScene1CameraProcessSet;
extern volatile u32 gNdsOpeningRoomScene1CameraAnimSet;
extern volatile u32 gNdsOpeningRoomScene1CameraViewportSet;
extern volatile u32 gNdsOpeningRoomScene1CameraDLBufferSet;
extern volatile u32 gNdsOpeningRoomScene2CameraAssetMask;
extern volatile u32 gNdsOpeningRoomScene2CameraAnimOffset;
extern volatile u32 gNdsOpeningRoomScene2CameraEjectResult;
extern volatile u32 gNdsOpeningRoomScene2CameraEjectMask;
extern volatile u32 gNdsOpeningRoomScene2CameraEjectBeforeGObjCount;
extern volatile u32 gNdsOpeningRoomScene2CameraEjectAfterGObjCount;
extern volatile u32 gNdsOpeningRoomScene2CameraEjectBeforeCameraCount;
extern volatile u32 gNdsOpeningRoomScene2CameraEjectAfterCameraCount;
extern volatile u32 gNdsOpeningRoomScene2CameraCreateResult;
extern volatile u32 gNdsOpeningRoomScene2CameraCreateMask;
extern volatile u32 gNdsOpeningRoomScene2CameraCreateGObjCount;
extern volatile u32 gNdsOpeningRoomScene2CameraGObjDelta;
extern volatile u32 gNdsOpeningRoomScene2CameraCObjDelta;
extern volatile u32 gNdsOpeningRoomScene2CameraXObjDelta;
extern volatile u32 gNdsOpeningRoomScene2CameraAObjDelta;
extern volatile u32 gNdsOpeningRoomScene2CameraDisplaySet;
extern volatile u32 gNdsOpeningRoomScene2CameraProcessSet;
extern volatile u32 gNdsOpeningRoomScene2CameraAnimSet;
extern volatile u32 gNdsOpeningRoomScene2CameraViewportSet;
extern volatile u32 gNdsOpeningRoomScene2CameraDLBufferSet;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayCameraCreateResult;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayCameraCreateMask;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayCameraCreateGObjCount;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayCameraGObjDelta;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayCameraCObjDelta;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayCameraXObjDelta;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayCameraDisplaySet;
extern volatile u32 gNdsOpeningRoomCloseUpOverlayCameraViewportSet;
extern volatile u32 gNdsOpeningRoomWallpaperCameraCreateResult;
extern volatile u32 gNdsOpeningRoomWallpaperCameraCreateMask;
extern volatile u32 gNdsOpeningRoomWallpaperCameraCreateGObjCount;
extern volatile u32 gNdsOpeningRoomWallpaperCameraGObjDelta;
extern volatile u32 gNdsOpeningRoomWallpaperCameraCObjDelta;
extern volatile u32 gNdsOpeningRoomWallpaperCameraXObjDelta;
extern volatile u32 gNdsOpeningRoomWallpaperCameraDisplaySet;
extern volatile u32 gNdsOpeningRoomWallpaperCameraViewportSet;
extern volatile u32 gNdsOpeningRoomLogoCameraAssetMask;
extern volatile u32 gNdsOpeningRoomLogoCameraAnimOffset;
extern volatile u32 gNdsOpeningRoomLogoCameraCreateResult;
extern volatile u32 gNdsOpeningRoomLogoCameraCreateMask;
extern volatile u32 gNdsOpeningRoomLogoCameraCreateGObjCount;
extern volatile u32 gNdsOpeningRoomLogoCameraGObjDelta;
extern volatile u32 gNdsOpeningRoomLogoCameraCObjDelta;
extern volatile u32 gNdsOpeningRoomLogoCameraXObjDelta;
extern volatile u32 gNdsOpeningRoomLogoCameraAObjDelta;
extern volatile u32 gNdsOpeningRoomLogoCameraDisplaySet;
extern volatile u32 gNdsOpeningRoomLogoCameraProcessSet;
extern volatile u32 gNdsOpeningRoomLogoCameraAnimSet;
extern volatile u32 gNdsOpeningRoomLogoCameraViewportSet;
extern volatile u32 gNdsOpeningRoomLogoAssetMask;
extern volatile u32 gNdsOpeningRoomLogoDObjOffset;
extern volatile u32 gNdsOpeningRoomLogoMObjOffset;
extern volatile u32 gNdsOpeningRoomLogoMatAnimOffset;
extern volatile u32 gNdsOpeningRoomLogoCreateResult;
extern volatile u32 gNdsOpeningRoomLogoCreateMask;
extern volatile u32 gNdsOpeningRoomLogoCreateGObjCount;
extern volatile u32 gNdsOpeningRoomLogoGObjDelta;
extern volatile u32 gNdsOpeningRoomLogoDObjDelta;
extern volatile u32 gNdsOpeningRoomLogoXObjDelta;
extern volatile u32 gNdsOpeningRoomLogoMObjDelta;
extern volatile u32 gNdsOpeningRoomLogoAObjDelta;
extern volatile u32 gNdsOpeningRoomLogoDisplaySet;
extern volatile u32 gNdsOpeningRoomLogoMObjSet;
extern volatile u32 gNdsOpeningRoomLogoMatAnimSet;
extern volatile u32 gNdsOpeningRoomLogoEjectResult;
extern volatile u32 gNdsOpeningRoomLogoEjectBeforeGObjCount;
extern volatile u32 gNdsOpeningRoomLogoEjectAfterGObjCount;
extern volatile u32 gNdsOpeningRoomLogoEjectUnlinkedMask;
extern volatile u32 gNdsOpeningRoomBossShadowAssetMask;
extern volatile u32 gNdsOpeningRoomBossShadowDisplayListOffset;
extern volatile u32 gNdsOpeningRoomBossShadowAnimOffset;
extern volatile u32 gNdsOpeningRoomBossShadowCreateResult;
extern volatile u32 gNdsOpeningRoomBossShadowCreateMask;
extern volatile u32 gNdsOpeningRoomBossShadowCreateGObjCount;
extern volatile u32 gNdsOpeningRoomBossShadowGObjDelta;
extern volatile u32 gNdsOpeningRoomBossShadowDObjDelta;
extern volatile u32 gNdsOpeningRoomBossShadowXObjDelta;
extern volatile u32 gNdsOpeningRoomBossShadowAObjDelta;
extern volatile u32 gNdsOpeningRoomBossShadowProcessSet;
extern volatile u32 gNdsOpeningRoomBossShadowDisplaySet;
extern volatile u32 gNdsOpeningRoomBossShadowAnimSet;
extern volatile u32 gNdsOpeningRoomBossShadowEjectResult;
extern volatile u32 gNdsOpeningRoomBossShadowEjectBeforeGObjCount;
extern volatile u32 gNdsOpeningRoomBossShadowEjectAfterGObjCount;
extern volatile u32 gNdsOpeningRoomBossShadowEjectUnlinkedMask;
extern volatile u32 gNdsOpeningRoomPencilsCreateResult;
extern volatile u32 gNdsOpeningRoomPencilsCreateMask;
extern volatile u32 gNdsOpeningRoomPencilsGObjDelta;
extern volatile u32 gNdsOpeningRoomPencilsDObjDelta;
extern volatile u32 gNdsOpeningRoomPencilsXObjDelta;
extern volatile u32 gNdsOpeningRoomPencilsAObjDelta;
extern volatile u32 gNdsOpeningRoomPencilsProcessSet;
extern volatile u32 gNdsOpeningRoomPencilsDisplaySet;
extern volatile u32 gNdsOpeningRoomPencilsDObjTreeCount;
extern volatile u32 gNdsOpeningRoomPencilsAnimRootCount;
extern volatile u32 gNdsOpeningRoomControllerCheckCount;
extern volatile u32 gNdsOpeningRoomPulledFighterKind;
extern volatile u32 gNdsOpeningRoomDroppedFighterKind;
extern volatile u32 gNdsOpeningRoomSkipToTitleCount;
extern volatile u32 gNdsOpeningRoomRelocResult;
extern volatile u32 gNdsOpeningRoomRelocInitCount;
extern volatile u32 gNdsOpeningRoomRelocLoadCount;
extern volatile u32 gNdsOpeningRoomRelocFileMask;
extern volatile u32 gNdsOpeningRoomRelocHeaderMask;
extern volatile u32 gNdsOpeningRoomRelocPayloadMask;
extern volatile u32 gNdsOpeningRoomRelocContentReady;
extern volatile u32 gNdsOpeningRoomRelocFixupReady;
extern volatile u32 gNdsOpeningRoomRelocBytesLoaded;
extern volatile u32 gNdsOpeningRoomRelocLastFileID;
extern volatile u32 gNdsOpeningRoomRelocLastSize;
extern volatile u32 gNdsOpeningRoomRelocWordSwapMask;
extern volatile u32 gNdsOpeningRoomRelocWordSwapCount;
extern volatile u32 gNdsOpeningRoomRelocWordSwapFailCount;
extern volatile u32 gNdsOpeningRoomRelocPointerFixupMask;
extern volatile u32 gNdsOpeningRoomRelocPointerFixupCount;
extern volatile u32 gNdsOpeningRoomRelocPointerFixupFailCount;
extern volatile u32 gNdsOpeningRoomRelocSymbolResolveCount;
extern volatile u32 gNdsOpeningRoomRelocSymbolResolveFailCount;
extern volatile u32 gNdsOpeningRoomRelocSymbolProbeMask;
extern volatile u32 gNdsOpeningRoomRelocLastSymbolOffset;
extern volatile u32 gNdsOpeningRoomRelocMObjSubNormalizeCount;
extern volatile u32 gNdsOpeningRoomRelocMObjSubFirstFlags;
extern volatile u32 gNdsOpeningRoomRelocMObjSubSourceResult;
extern volatile u32 gNdsOpeningRoomRelocMObjSubFirstTextureOffset;
extern volatile u32 gNdsOpeningRoomRelocMObjSubFirstTextureFlags;

#ifndef NDS_RENDERER_FIGHTER_LEAN_H
#define NDS_RENDERER_FIGHTER_LEAN_H

/* P2-2p8 Phase 1 -- the lean fighter path (docs/p2/
 * FOUR_FIGHTER_30FPS_ARCHITECTURE.md A1; spec: phase1-spec.md sections 2-4).
 *
 * Slice 1 drew Samus LOW program 0; slice 3 the four stress kinds -- Donkey,
 * Samus, Link, Kirby -- from packets the old path RECORDED and the lean path
 * adopted. Slice 4 (spec ORDER steps 5-6): the lean path no longer depends on
 * recorded packets. A fighter's list is MATERIALIZED from the generated owner
 * tables (the owner generator's image: roots, epochs, runs, Task 56 strips,
 * dense normals, packed VTX16 positions, dense UVs, state spans) and the live
 * material state, in the LOAD4x3 + P' layout, into the slot's existing region:
 *
 *   head   LIGHT_COLOR, MTX_MODE(0), LOAD4x4 P' (the projection with row 3
 *          scaled 2^-8, loaded once), MTX_MODE(2), IDENTITY, LIGHT_VECTOR
 *          (the light under an identity vector matrix)
 *   root   one 12-word LOAD4x3 (the binding's world, row 3 across the
 *          world-unit seam), STORE when the root owns a cross slot, then the
 *          root's epochs and runs exactly as the production execute emits them
 *
 * The host twin of that list (scripts/fighters/fighter_list_emitter.py, with
 * its proof scripts/fighters/fighter_list_proof.py) is word-compared against
 * runtime-recorded packets by scripts/fighters/check_nds_native_owner_packet.py.
 * A materialized list is keyed on everything its words depend on (program,
 * detail, root vector, material state, the key[3] preamble fields, the tables),
 * two entries live per battle slot, and the event path (first draw, status /
 * program / model-part / material change) selects an entry or materializes
 * one -- no old-path draw, no recording. The per-frame path is the tuple, the
 * kernel (12-word LOAD4x3 straight into the list), the replay's patches (P',
 * tint tiles, shade re-derive, light, Link's texgen words) and one DMA0.
 *
 *   gNdsFtrLeanRoute  0  off -- today's behaviour (default)
 *                     1  lean path draws the four kinds; both halves of the
 *                        slot's region are lean entries, and the recorder
 *                        never arms for a slot whose lower half the lean
 *                        path owns (a declined draw runs production direct)
 *                     2  oracle-exact: the old path draws (recorder in the
 *                        lower half) with the Q43.20 source compose forced;
 *                        the lean path materializes/patches its entry in the
 *                        upper half and TryReplay (or the re-record) compares
 *                        it SEMANTICALLY: every non-matrix command exactly,
 *                        P' and each LOAD4x3 against the recorded projection
 *                        and split modelview word for word, and the composed
 *                        clip matrices (rows 0-2 exact, row 3 within 1 LSB)
 *                     3  oracle-shipped: as 2 with the old path unchanged
 *   gNdsFtrLeanAdmit  0 / 1 / 2: fighter texture admission, see the slice
 *                        2b block at the end of this header
 *
 * Both words live in DTCM: it is uncached, so a GDB poke at the first
 * frame-complete marker is seen by the next read (a main-RAM word that shares
 * a dirty D-cache line gets stamped back -- see gNdsFtrPlanRoute's note in
 * diagnostics_collision_runtime.c). They are whole u32 words (never poke one
 * byte). */

#include <nds/nds_renderer.h>

/* Mirrors NDS_FIGHTER_PACKET_LIVE (src/nds/nds_renderer_preamble.c): the lean
 * lists live in the packet arena and reuse its patch helpers. */
#if defined(NDS_R2_FIGHTER_PACKET) && NDS_R2_FIGHTER_PACKET && \
    defined(NDS_RENDERER_HW_TRIANGLES) && NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_PROFILE_LEVEL < 2) && \
    defined(NDS_R2_FIGHTER_GX_COMPOSE) && NDS_R2_FIGHTER_GX_COMPOSE && \
    defined(NDS_R2_FIGHTER_HW_MTX) && NDS_R2_FIGHTER_HW_MTX && \
    defined(NDS_R2_FIGHTER_HW_LIGHT) && NDS_R2_FIGHTER_HW_LIGHT
#define NDS_FTR_LEAN_LIVE 1
#else
#define NDS_FTR_LEAN_LIVE 0
#endif

/* Slice 2b BSS diet: the lean counters (engagement, kernel, oracle, census,
 * texture witnesses: gNdsFtrLean below) are the lab instrument. A build
 * without NDS_TICK_HUD has no gNdsFtrLean at all; NDS_FTR_LEAN_CTR() compiles
 * to nothing there, and the oracle / census bodies are compiled out (routes 2
 * and 3 still patch and disarm, they just record nothing). */
#if defined(NDS_TICK_HUD) && NDS_TICK_HUD
#define NDS_FTR_LEAN_LAB 1
#define NDS_FTR_LEAN_CTR(...) do { __VA_ARGS__; } while (0)
#else
#define NDS_FTR_LEAN_LAB 0
#define NDS_FTR_LEAN_CTR(...) ((void)0)
#endif

#define NDS_FTR_LEAN_ROUTE_OFF 0u
#define NDS_FTR_LEAN_ROUTE_DRAW 1u
#define NDS_FTR_LEAN_ROUTE_ORACLE_EXACT 2u
#define NDS_FTR_LEAN_ROUTE_ORACLE_SHIPPED 3u

extern volatile u32 gNdsFtrLeanRoute;
extern volatile u32 gNdsFtrLeanAdmit;
/* Slice 3 cost A/B (DTCM runtime word, default 0 = every slice 3 cut on).
 * Each bit puts one part back to its slice 1 form on the same ROM, so the
 * before/after per-draw costs are one poke apart:
 *   1  kernel: scale != 1 and warm-cache joints take the source builder
 *   2  guard: preambles hashed every draw (not once per memo fill), texture
 *      residency proved and touched every draw, live link hash every draw
 *   4  patch/submit: light word re-derived every draw, whole list cleaned */
extern volatile u32 gNdsFtrLeanSlow;
#define NDS_FTR_LEAN_SLOW_KERNEL 1u
#define NDS_FTR_LEAN_SLOW_GUARD 2u
#define NDS_FTR_LEAN_SLOW_SUBMIT 4u
/* Lab attribution, not a cost form: time the kernel's phases per joint.
 * Compiled only into a tick-HUD build made with NDS_FTR_LEAN_KTIME=1 (Makefile,
 * own build dir); elsewhere the bit is ignored and the kernel carries no
 * timing code. */
#define NDS_FTR_LEAN_SLOW_KTIME 8u
/* Lab check, not a cost form: every event that re-selects a held list (an
 * entry, or a variant record of one) also materializes the key into the other
 * entry and compares the two (verify_*). Route 1 only (it needs both halves). */
#define NDS_FTR_LEAN_SLOW_VERIFY 16u
#ifndef NDS_FTR_LEAN_KTIME
#define NDS_FTR_LEAN_KTIME 0
#endif

/* Decline reasons (gNdsFtrLean.decline[], per kind k_decline[][]): the draws
 * the lean path cannot take yet, each one named. A declined draw is the old
 * path's (route 1: production direct, never a recording). */
enum
{
    nNDSFtrLeanDeclineKind = 0,        /* no hitlag fold in this build */
    nNDSFtrLeanDeclineSkeleton,        /* electric skeleton: no native owner
                                        * for these kinds (R7) */
    nNDSFtrLeanDeclineCamera,          /* no projection / camera modelview seed */
    nNDSFtrLeanDeclinePlan,            /* the draw-plan resolver refused the
                                        * display contract (stage 1-3) */
    nNDSFtrLeanDeclineValidate,        /* the owner validator refused the root
                                        * vector (no generated program) */
    nNDSFtrLeanDeclineKirbyHead,       /* live trio body, unknown joint-6 part */
    nNDSFtrLeanDeclineRoots,           /* more roots than the lean arrays hold */
    nNDSFtrLeanDeclineMaterial,        /* material rows not buildable */
    nNDSFtrLeanDeclineTables,          /* runtime tables / root preflight */
    nNDSFtrLeanDeclinePolicy,          /* a run the production prepare rejects */
    nNDSFtrLeanDeclineTexture,         /* the texture bind path refused a run */
    nNDSFtrLeanDeclineCapacity,        /* the list does not fit an entry */
    nNDSFtrLeanDeclineTopology,        /* joint table not buildable */
    nNDSFtrLeanDeclineKernel,          /* compose declined (source would too) */
    nNDSFtrLeanDeclineTexgen,          /* Link's texgen patch declined */
    nNDSFtrLeanDeclineTint,            /* shade re-derive refused */
    nNDSFtrLeanDeclineStale,           /* a fresh list still failed its guard */
    nNDSFtrLeanDeclineInputs,          /* production inputs not buildable */
    nNDSFtrLeanDeclineReserved18,
    nNDSFtrLeanDeclineReserved19,
    nNDSFtrLeanDeclineCount = 20
};

/* Why the event path ran (gNdsFtrLean.event[], per kind k_event[][]). An
 * event is not a decline: it ends in an entry switch or a materialization and
 * the draw stays lean. */
enum
{
    nNDSFtrLeanEventNone = 0,          /* the active entry holds this draw */
    nNDSFtrLeanEventFirst,             /* no active entry (first draw) */
    nNDSFtrLeanEventTuple,             /* detail / root / heap / file moved */
    nNDSFtrLeanEventStatus,            /* status generation moved */
    nNDSFtrLeanEventRebind,            /* material/parts graph rebuilt */
    nNDSFtrLeanEventMaterial,          /* material identity moved */
    nNDSFtrLeanEventPreamble,          /* key[3] preamble fields moved */
    nNDSFtrLeanEventTintSet,           /* tint-tile set generation moved */
    nNDSFtrLeanEventTintTile,          /* a tinted prim went white / lost its tile */
    nNDSFtrLeanEventFence,             /* texture fence / residency moved */
    nNDSFtrLeanEventKernel,            /* a kept joint re-parented */
    nNDSFtrLeanEventCount = 11
};

/* Slice 3: the four stress kinds, as the counters' kind index. */
#define NDS_FTR_LEAN_KINDS 4u
#define NDS_FTR_LEAN_KIND_NONE 0xffu
#define NDS_FTR_LEAN_OWNER_KIND(owner)                                      \
    (((owner) == NDS_RENDERER_NATIVE_FIGHTER_OWNER_DONKEY) ? 0u :           \
     ((owner) == NDS_RENDERER_NATIVE_FIGHTER_OWNER_SAMUS) ? 1u :            \
     ((owner) == NDS_RENDERER_NATIVE_FIGHTER_OWNER_LINK) ? 2u :             \
     ((owner) == NDS_RENDERER_NATIVE_FIGHTER_OWNER_KIRBY) ? 3u :            \
     NDS_FTR_LEAN_KIND_NONE)
/* Kernel slow-joint classes (k_kernel_class[][]): which joints still take
 * the adapter's own source builder, and why. 0 is the fast path. */
enum
{
    nNDSFtrLeanJointFast = 0,
    nNDSFtrLeanJointNoLocal,           /* no FighterParts XObj (identity) */
    nNDSFtrLeanJointScale,             /* scale != 1: exact integer RSca */
    nNDSFtrLeanJointWarm,              /* warm gameplay cache: exact F2L */
    nNDSFtrLeanJointLock,              /* animation locks: source builder */
    nNDSFtrLeanJointConvert,           /* angle/translate not exact: float */
    nNDSFtrLeanJointXObj,              /* other XObj kind / parts count */
    nNDSFtrLeanJointNoGObj,            /* no parent GObj */
    nNDSFtrLeanJointClassCount = 8
};

/* Oracle word classes (oracle_mismatch[], k_oracle_mismatch[][]). */
enum
{
    nNDSFtrLeanOracleStructure = 0,    /* command sequence / counts differ */
    nNDSFtrLeanOracleProjection,       /* P' != the recorded projection with
                                        * row 3 >> 8 */
    nNDSFtrLeanOracleModelview,        /* LOAD4x3 != the recorded split
                                        * modelview (or its m33 != 16) */
    nNDSFtrLeanOracleShade,            /* DIF_AMB word */
    nNDSFtrLeanOracleLight,            /* LIGHT_VECTOR word */
    nNDSFtrLeanOracleOther,            /* any other command parameter */
    nNDSFtrLeanOracleTint,             /* tint-tile TEXIMAGE / PLTT word */
    nNDSFtrLeanOracleTexgen,           /* texgen TEXCOORD word */
    nNDSFtrLeanOracleClassCount = 8
};

/* One flushed block of counters, read with
 * -ExtraGlobals gNdsFtrLean.<field>. Published to main RAM once per frame at
 * the end of the fighter submit loop (a stop reads RAM, not the D-cache). */
typedef struct NDSFtrLeanCounters
{
    /* engagement */
    u32 draws;                  /* route 1: lean path drew */
    u32 shadow_runs;            /* routes 2/3: lean patched its entry */
    u32 attempts;
    u32 decline[20];
    u32 event[11];              /* nNDSFtrLeanEvent* */
    u32 entry_hits;             /* events that found a materialized entry */
    u32 entry_switches;         /* ... that was not the active one */
    u32 materializations;
    u32 materialize_ticks;      /* whole event paths that materialized */
    u32 materialize_list_ticks; /* ... of which the list walk */
    u32 materialize_words;      /* words of the last materialization */
    u32 materialize_words_max;
    u32 materialize_roots;      /* roots of the last materialization */
    u32 materialize_textures;   /* cache textures bound by the last one */
    u32 materialize_tint_binds; /* tint tiles bound by the last one */
    u32 materialize_tint_pending; /* tint epochs that folded (tile queued) */
    u32 event_ticks;            /* event paths that did not materialize */
    u32 materialize_part_ticks[8]; /* 0 preflight + head, 1 root binds,
                                      2 spans + materials, 3 shade, 4 run
                                      prepares (validate, words), 5 corners,
                                      6 texture resolve + bind, 7 tint bind +
                                      texture words readback */
    u32 materialize_key_miss[6];   /* a materialization's key word that
                                      differs from each other valid entry */
    u32 key_events;             /* event paths that reached the entry key */
    u32 key_seen_before;        /* ... whose key the slot had selected before */
    u32 key_distinct;           /* ... whose key was new to the slot */
    u32 materialize_why[4];     /* per usable entry at a materialization:
                                   0 invalid, 1 same key but tint set moved,
                                   2 same key but fence moved, 3 key differs */
    /* variants (ndsFtrLeanLearnVariant) */
    u32 variants_learned;       /* new lists kept as a record of the other */
    u32 variant_switches;       /* an entry's words moved to another record */
    u32 variant_evictions;      /* a full tail replaced a record */
    u32 variant_reject[4];      /* 0 no current other list, 1 shape, 2 no
                                   room, 3 more words than a record holds */
    u32 variant_diff_words;     /* words of the learned records */
    u32 variant_diff_max;
    u32 tint_repatches;         /* fold-free lists re-pointed after the
                                   tint-tile set moved */
    u32 rerecord_resets;        /* held lists re-derived where the old path
                                   re-records without a new list */
    /* lab verify (NDS_FTR_LEAN_SLOW_VERIFY) */
    u32 verify_runs;            /* re-selected lists compared */
    u32 verify_variant_runs;    /* ... of an entry holding records */
    u32 verify_mismatch[4];     /* 0 shape, 1 words, 2 texture table,
                                   3 fences */
    u32 verify_first[4];        /* first word mismatch: slot | entry << 4 |
                                   record << 8 | records << 16, index,
                                   held word, fresh word */
    u32 region_takes;           /* route 1: lower half taken from recorder */
    /* kernel */
    u32 kernel_joints;
    u32 kernel_slow_joints;
    u32 kernel_fail;
    /* FTR sub-phases (ticks, lean-engaged draws only) */
    u32 head_ticks;             /* capture/head of every draw */
    u32 guard_ticks;
    u32 kernel_ticks;
    u32 patch_ticks;
    u32 submit_ticks;
    u32 dma_wait_ticks;         /* lean submit's own DMA0 busy wait */
    u32 dma_wait_spins;
    /* oracle */
    u32 oracle_runs;
    u32 oracle_words;           /* lean words compared */
    u32 oracle_mismatch[8];     /* nNDSFtrLeanOracle* */
    u32 oracle_clip_max[2];     /* max |clip delta| rows 0-2, row 3 */
    u32 oracle_clip_roots;      /* roots whose clip matrices were composed */
    u32 oracle_donor_memo;      /* texture words of a donor-table root that
                                 * differ: production's run texture memo
                                 * keys on (run index, player), so a donor
                                 * root replays the owner root's texture */
    u32 oracle_key_moved;       /* compared entry's key != this draw's */
    u32 oracle_record_under_hit[2];   /* same, differs (outside sites) */
    u32 oracle_record_diff[8];  /* record-under-hit differences by class */
    u32 oracle_unconsumed;      /* shadow armed, old path never replayed */
    u32 oracle_source_miss;     /* route 2: forced source compose failed */
    u32 oracle_first[8];        /* first mismatch: frame, slot<<24|class<<16
                                   |root, lean cmd, lean param index, lean
                                   word, recorded word, lean command ordinal,
                                   recorded command ordinal */
    /* Phase 0 leftovers */
    u32 ge_busy_samples;        /* GXSTAT sampled at the end of each fighter */
    u32 ge_busy_hits;           /* ... with bit 27 (GE busy) set */
    u32 packet_dma_waits;       /* next-writer waits that spun (any fighter) */
    u32 packet_dma_wait_ticks;
    u32 tint_rerecords[4];      /* per battle slot: tinted prim moved */
    /* 2.6 tile-word patch */
    u32 tint_patch_binds;       /* tile binds patched (every lean patch) */
    u32 tint_patch_moved;       /* ... whose colour differs from the list */
    u32 tint_patch_miss;        /* colour has no resident tile: event */
    u32 tint_patch_white;       /* colour went white (untinted): event */
    u32 tint_patch_shape;       /* palette word presence differs: event */
    u32 fighter_uploads;        /* texture uploads under a fighter owner */
    u32 fighter_uploads_after_go;
    u32 fighter_upload_bytes_after_go;
    u32 go_frame;               /* gNdsRendererProfileFrameCount at GO */
    /* admission / texture census */
    u32 admit_runs;
    u32 admit_pinned;           /* fighter textures pinned */
    u32 admit_pinned_bytes;
    u32 union_textures[4];      /* distinct cache entries bound per slot */
    u32 union_bytes[4];
    u32 census_live;            /* texture cache at the census point */
    u32 census_free;
    u32 census_pinned;
    u32 census_static;
    u32 census_live_bytes;
    u32 census_fighter_bytes;
    u32 reject_count;           /* fighter-owner texture rejects */
    u32 reject_mask;            /* NDS_RENDERER_HW_TEXREJECT_* seen */
    u32 reject_first[12];       /* reason, format, size, owner, frame,
                                   go, live, free, pinned, thisframe,
                                   evictable, live_bytes */
    /* ---- per kind (0 Donkey, 1 Samus, 2 Link, 3 Kirby) ---- */
    u32 k_attempts[4];
    u32 k_draws[4];             /* route 1 draws; routes 2/3 shadow runs */
    u32 k_materializations[4];
    u32 k_decline[4][20];
    u32 k_event[4][11];
    u32 k_head_ticks[4];        /* capture/head of every draw of the kind */
    u32 k_guard_ticks[4];
    u32 k_kernel_ticks[4];
    u32 k_patch_ticks[4];
    u32 k_submit_ticks[4];
    u32 k_dma_wait_ticks[4];
    u32 k_event_ticks[4];       /* event paths (switch or materialize) */
    u32 k_materialize_ticks[4];
    u32 k_words_max[4];         /* largest list materialized */
    u32 k_kernel_joints[4];
    u32 k_kernel_class[4][8];   /* nNDSFtrLeanJoint* */
    u32 k_program_draws[4][16]; /* draws by root program (15 = >= 15) */
    u32 k_high_draws[4];        /* draws at HIGH detail */
    u32 k_oracle_runs[4];
    u32 k_oracle_mismatch[4][8];
    u32 k_oracle_donor_memo[4];
    u32 k_key_events[4];        /* lab census, per kind */
    u32 k_key_seen_before[4];
    u32 k_key_miss[4][6];
    u32 k_why[4][4];
    u32 k_variants_learned[4];
    u32 k_variant_switches[4];
    /* guard / patch / submit sub-phases (all kinds; lean-engaged draws) */
    u32 guard_part_ticks[8];    /* 0 tuple, 1 topology(lab), 2 camera,
                                   3 identity, 4 preamble, 5 packet guard,
                                   6 shuffle, 7 event path */
    u32 patch_part_ticks[6];    /* 0 tint tiles, 1 apply tint, 2 matrices,
                                   3 light, 4 texgen, 5 texture sites */
    u32 submit_part_ticks[4];   /* 0 flush, 1 gx state + dma wait, 2 dma
                                   start + invalidate, 3 stats */
    u32 flush_bytes;            /* bytes cleaned by the lean submit */
    u32 light_patches;          /* light word recomputed (moved) */
    u32 texgen_patches;         /* lean draws that patched texgen words */
    u32 pinned_residency_miss;  /* all-pinned list failed residency (2/3) */
    u32 pre_checks;             /* preamble fields hashed and compared */
    u32 pre_skips;              /* ... skipped: memo content already proven */
    u32 book_ticks;             /* route-1 draw bookkeeping around submit */
    u32 ident_watch_miss;       /* whole identity moved, watch saw nothing */
    u32 projection_patches;     /* draws whose projection moved */
    u32 retuples;               /* status changes kept on the same entry */
    u32 kernel_rebuilds;        /* joint tables re-collected after a refusal */
    u32 kernel_part_ticks[4];   /* fast local, compose, output, slow local
                                   (gNdsFtrLeanSlow bit 8 only) */
    u32 kernel_part_joints;     /* joints those ticks cover */
} NDSFtrLeanCounters;

#if NDS_FTR_LEAN_LAB
extern NDSFtrLeanCounters gNdsFtrLean;
#endif

/* Route-2 handshake: set by the old path's matrix prep when the forced
 * Q43.20 source compose produced this draw's matrices. */
extern volatile u32 gNdsFtrLeanOracleSourceOk;

/* ---- the joint kernel (src/nds/nds_ftr_lean_kernel.c) ---------------- */

typedef struct DObj DObj;
struct FTParts;

/* NDSFtrLeanJoint.local_kind: the XObj class of the joint, which is fixed for
 * a DObj's life (gcAddDObj3TransformsKind), classified exactly in
 * ndsRendererAdapterBuildSourceFighterLocalMtx's order. */
#define NDS_FTR_LEAN_LOCAL_NONE 0u      /* no FighterParts XObj: no local */
#define NDS_FTR_LEAN_LOCAL_PARTS 1u     /* one FighterParts XObj, parts set */
#define NDS_FTR_LEAN_LOCAL_XOBJ 2u      /* anything else: the source builder */
#define NDS_FTR_LEAN_LOCAL_NO_GOBJ 3u   /* no parent GObj: the source builder */

typedef struct NDSFtrLeanJoint
{
    DObj *dobj;
    const struct FTParts *parts; /* LOCAL_PARTS: ftGetParts(dobj) */
    u8 parent;                  /* joint index, 0xff = topology root */
    u8 binding;                 /* production root index, 0xff = none */
    u8 local_kind;              /* NDS_FTR_LEAN_LOCAL_* */
    u8 depth;                   /* 0 at a topology root */
} NDSFtrLeanJoint;
/* The kernel keeps one world per depth (preorder): a deeper kept tree is not
 * taken (nNDSFtrLeanDeclineTopology). */
#define NDS_FTR_LEAN_DEPTH_MAX 20u

/* Slow local builder (TU of the adapter): the exact
 * ndsRendererAdapterBuildSourceFighterLocalMtx result decoded to s32 16.16
 * cells in row-major 4x3 order (rows 0-2 basis, row 3 translation).
 * `accum` is the joint's animation-lock scale accumulator, read and updated
 * in place exactly as ndsRendererAdapterComposeOwnerWorldsSource threads it
 * (x, y, z floats; only the lock branches read or write it).
 * Returns FALSE where the source compose would decline. */
typedef s32 (*NDSFtrLeanSlowLocalFn)(DObj *dobj, f32 *accum, s32 *cells,
                                     u32 *has_local);

/* `flags`: NDS_FTR_LEAN_KERNEL_LOCKS when fp->is_use_animlocks is set for
 * this draw -- every FighterParts joint then takes the source builder with a
 * per-joint accumulator, as the source compose does; NDS_FTR_LEAN_KERNEL_NO_FAST
 * sends scale != 1 and warm-cache joints to the builder too (the cost A/B's
 * slice 1 form; counted under their own class). `class_counts` (NULL outside
 * the lab): per-class joint counts, indexed by nNDSFtrLeanJoint*.
 * `part_ticks` (NULL unless the lab asks, gNdsFtrLeanSlow bit 8): [0] fast
 * local, [1] compose, [2] output, [3] source-builder local. */
#define NDS_FTR_LEAN_KERNEL_LOCKS 1u
#define NDS_FTR_LEAN_KERNEL_NO_FAST 2u
/* Outputs per binding: `mv_sites[b]` (when non-NULL) receives the 12
 * LOAD4x3 parameters of the binding's world -- rows 0-2 of the Q20.12 basis
 * and row 3 across the world-unit seam (RoundShiftS32Signed(t, 8)), exactly
 * the first three columns of ndsFighterPacketStoreSplitModelview's words --
 * straight into the list; `binding_worlds` receives the Q20.12 world itself
 * for the bindings in `world_mask` (Link's texgen reads them). */
s32 ndsFtrLeanKernelCompose(const NDSFtrLeanJoint *joints, u32 joint_count,
                            u32 *const *mv_sites,
                            NDSRendererMatrix20p12 *binding_worlds,
                            u32 world_mask,
                            u32 binding_count,
                            s32 shuffle_x, s32 shuffle_y,
                            u32 flags,
                            NDSFtrLeanSlowLocalFn slow,
                            u32 *class_counts,
                            u32 *part_ticks);

/* ---- list side (src/nds/nds_renderer_native_common.c) ----------------- */

/* The entry key: every input a materialized list's words depend on that no
 * per-frame patch covers. [0] the drawn roots' material rows (their content,
 * pointer-free), [1] owner, detail, battle slot, costume and shade, [2] the
 * owner file and heap generation, [3] the roots' key[3] preamble fields,
 * [4] program | root count << 8, [5] Kirby's trio head key. Content keys make
 * a list outlive the DObj/MObj graph it was drawn from: a model-part swap or
 * a rebuilt MObj re-selects by key (ndsRendererFighterPacketInvalidateSlot
 * keeps the lists). */
#define NDS_FTR_LEAN_KEY_WORDS 6u
#define NDS_FTR_LEAN_ENTRY_NONE 0xffu

/* The list of `battle_slot` for `key` (still current: tint-tile set, texture
 * fence) as an entry code for ndsFtrLeanEntryActivate -- an entry whose words
 * hold that state, or a variant record of one -- or NDS_FTR_LEAN_ENTRY_NONE. */
u32 ndsFtrLeanEntryFind(u32 battle_slot, const u32 *key);
/* The entry a new list goes into: an empty or stale one, else the one that
 * is not active (route 1); the upper half (routes 2/3). */
u32 ndsFtrLeanEntryVictim(u32 battle_slot);
/* The active entry, or NDS_FTR_LEAN_ENTRY_NONE. */
u32 ndsFtrLeanEntryActiveIndex(u32 battle_slot);
/* The active list cannot serve its key any more (its tint folds under a moved
 * set, a moved fence, a lost texture): drop that entry alone. */
void ndsFtrLeanEntryDropActive(u32 battle_slot);
/* After `fresh` was materialized: when the other entry's list has the same
 * shape and differs from it in at most a few words outside the patch sites,
 * keep the new state as a variant record of that list instead (its words
 * switched to the new state; `fresh` stays a valid copy). Returns the entry
 * code to activate, or NDS_FTR_LEAN_ENTRY_NONE (activate `fresh`). */
u32 ndsFtrLeanLearnVariant(u32 battle_slot, u32 fresh);
/* Lab (NDS_FTR_LEAN_SLOW_VERIFY): `held` against `fresh`, a materialization
 * of the same key (verify_*). */
void ndsFtrLeanVerifyEntries(u32 battle_slot, u32 held, u32 fresh);
/* Lab: which key words of a materializing draw differ from each valid entry
 * (materialize_key_miss[]). */
void ndsFtrLeanNoteKeyMiss(u32 battle_slot, const u32 *key);
/* Make `entry` the list the per-frame path patches and submits; every
 * patch site is re-derived before its first submit. TRUE when it was not the
 * active entry. */
u32 ndsFtrLeanEntryActivate(u32 battle_slot, u32 code);
/* An entry switch stands for the re-record the old path makes when its
 * packet's key moves: re-derive the active list's shade words as that record
 * would (the execute's modulate, 0, over this draw's prims) and name the live
 * modulate, so the per-frame ApplyTint evolves them as the replay's. */
/* The old path's packet key (ndsFighterPacketBuildKey) over the lean inputs,
 * less the tint-tile set generation: when it moves, or the packet was
 * invalidated, the old path re-records and a held list takes the record's
 * shade derivation (ndsFtrLeanEntryResetShade). */
u32 ndsFtrLeanRerecordKey(u32 ident, const u32 *key,
                          const NDSRendererNativeFighterRoot *inputs,
                          u32 input_count);
void ndsFtrLeanEntryResetShade(u32 battle_slot,
                               const NDSRendererNativeFighterRoot *inputs,
                               u32 input_count);
/* Materialize the list for `key` into `entry` from the generated tables of
 * the owner (selected by owner_slot / use_low_detail and the root program
 * the caller set) and the production inputs `inputs` (preambles, configs,
 * material rows, root offsets, asset bases). `stats` is the draw's initial
 * renderer state, seeded exactly as the old path seeds its own. Returns 0 or
 * a decline reason (nNDSFtrLeanDecline*). No old-path draw, no recording:
 * the only GX writes are the texture binds the production prepare makes
 * (after the previous fighter's DMA). */
u32 ndsFtrLeanMaterialize(u32 battle_slot, u32 entry, const u32 *key,
                          const NDSRendererNativeFighterRoot *inputs,
                          u32 input_count, u32 owner_slot,
                          u32 use_low_detail, const void *asset_base,
                          NDSRendererStats *stats);
void ndsFtrLeanPacketDrop(u32 battle_slot);
/* The slot's DObj/MObj graph was rebuilt (a model part, a costume): the
 * recorder's packet is gone; the lean lists stay (content keys). */
void ndsFtrLeanPacketRebind(u32 battle_slot);
/* The active list's per-root LOAD4x3 parameter sites, for the kernel to
 * write (their cache lines are marked for the submit). Returns the mask of
 * roots whose Q20.12 world the patch also reads (Link's texgen group roots),
 * or NDS_FTR_LEAN_SITES_NONE. */
#define NDS_FTR_LEAN_SITES_NONE 0xffffffffu
u32 ndsFtrLeanPacketModelviewSites(u32 battle_slot, u32 **sites,
                                   u32 root_count);
/* The active list's validity half: tint-tile set generation (the tile words
 * it bound), the texture fence for a list that could not name a texture,
 * and residency (skipped for an all-admitted list in route 1). Returns 0 or
 * an event reason (nNDSFtrLeanEvent*): the list must be re-materialized. */
u32 ndsFtrLeanPacketGuard(u32 battle_slot, u32 touch);
/* owner_slot / use_low_detail select the native runtime tables Link's texgen
 * words are derived from (the owner's root program must already be set).
 * pre_same: the roots' preambles are byte-identical to the last patch's (the
 * same memo fill), so the prim-derived tint tiles and the light word cannot
 * have moved and the shade re-derive depends on the colour modulate alone.
 * Returns 0, a decline reason (nNDSFtrLeanDecline*), or NDS_FTR_LEAN_PATCH_
 * REMATERIALIZE when a tinted prim went white or lost its tile (the list's
 * structure is the live draw's no longer). */
#define NDS_FTR_LEAN_PATCH_REMATERIALIZE 0x100u
u32 ndsFtrLeanPacketPatch(u32 battle_slot,
                          const NDSRendererNativeFighterRoot *inputs,
                          u32 input_count, u32 owner_slot,
                          u32 use_low_detail, u32 pre_same);
void ndsFtrLeanPacketSubmit(u32 battle_slot, NDSRendererStats *stats);
/* Kind index for the counters (lab) of the lean list in a battle slot,
 * published by the adapter at every attempt. */
void ndsFtrLeanPacketNoteKind(u32 battle_slot, u32 kind);
void ndsFtrLeanShadowArm(u32 battle_slot, u32 armed);
u32 ndsFtrLeanShadowArmed(u32 battle_slot);
void ndsFtrLeanCountersPublish(void);
void ndsFtrLeanTextureCensus(void);
void ndsFtrLeanNoteGo(u32 frame);
void ndsFtrLeanNoteTextureReject(u32 reason, u32 format, u32 size);
void ndsFtrLeanNoteTextureUpload(u32 bytes);
/* nds_renderer_preamble.c: every packet replay hit. A hit draw runs no
 * material preparation, so it cannot have written the fighter's MObjs. */
extern volatile u32 gNdsFighterPacketHits;
/* nds_renderer_textures_effects.c: the resident tint tile for a colour, as
 * the TEXIMAGE_PARAM / PLTT_BASE words a packet bind records (pltt is
 * 0xffffffff when the tile has no palette). FALSE when not resident. */
s32 ndsFtrLeanTintTileWords(u32 rgb, u32 touch, u32 *teximage, u32 *pltt);

/* ---- adapter side (src/port/renderer_fighter_lean.c) ------------------- */
void ndsFtrLeanNoteRebind(u32 player_slot);
/* A texture-part shim wrote a fighter MObj's texture id (battle slot). */
void ndsFtrLeanNoteTexturePart(u32 slot);

/* ---- P2-2p8 Phase 1 slice 2a: VRAM census (lab only) --------------------
 * Measurement, not behaviour: every GL texture name is tagged with the site
 * (caller PC) that created/uploaded it, the fighter draw slot active at the
 * upload and the runtime profile owner; gNdsVramCensusEnable = 1 (runtime
 * word, default 0) walks libnds's texture/palette tables and block lists at
 * every frame end and latches snapshots (GO, first fighter texture reject,
 * the worst largest-free frame of the P0 episode and after it). Tagging runs
 * whenever the build has NDS_TICK_HUD so names created before the poke are
 * attributed too. */
#if defined(NDS_TICK_HUD) && NDS_TICK_HUD
#define NDS_VRAM_CENSUS_LIVE 1
#else
#define NDS_VRAM_CENSUS_LIVE 0
#endif
#if NDS_VRAM_CENSUS_LIVE
extern volatile u32 gNdsVramCensusEnable;          /* TU B, DTCM word */
extern volatile u32 gNdsVramCensusDrawSlotPlus1;   /* TU A; RAF sets it */
void ndsVramCensusFrame(void);
void ndsVramCensusCaptureBurst(void);
#endif

/* ---- P2-2p8 Phase 1 slice 2b: fighter texture admission ------------------
 * gNdsFtrLeanAdmit (DTCM runtime word):
 *   0  today: fighter textures are uploaded on demand by the draw
 *   1  admit + pin in A+B: every texture of the admission table
 *      (scripts/fighters/generate_nds_fighter_admission.py -> NitroFS
 *      fighters/admission.bin) for each fighter's kind x detail x costume,
 *      plus Kirby's hats of the kinds present, is replayed through the
 *      renderer's own texture state recorders and the cache's own
 *      resolve/convert/upload path, and the resulting entries are exempt
 *      from eviction/refresh for the battle
 *   2  the plan: as 1, but first, when the battle leaves BG3 empty (no
 *      opaque BG3 pixel), bank D becomes texture slot 3 and BG3 is hidden
 *      and withheld. Admission allocates first-fit, so the admitted set
 *      packs into what A+B still has free and only the rest lands in D; at
 *      the first frame end after the admission (setup done) A+B are locked
 *      (glLockVRAMBank), so D is the only region that allocates and frees
 *      for the rest of the battle. Battle exit releases every cache entry in
 *      D, locks D, unlocks A+B, and hands D back to BG3 once the next scene's
 *      first 3D frame is the displayed one (or at its first BG3 request,
 *      if that comes first), so the final battle frame keeps its texels.
 * Admission runs at fighter creation when the word is already set, else at
 * the first frame end that sees it set (the sampler pokes after setup). */
#define NDS_FTR_LEAN_ADMIT_OFF 0u
#define NDS_FTR_LEAN_ADMIT_PIN 1u
#define NDS_FTR_LEAN_ADMIT_REGIONS 2u
#define NDS_FTR_LEAN_ADMIT_FIGHTERS 4u

/* Failure latch (shipping): count, and the first failure's
 * [0] fkind << 16 | detail << 8 | reason, [1] record index, [2] image
 * asset << 20 | source offset, [3] the cache's reject reason mask. */
enum
{
    nNDSFtrLeanAdmitFailNone = 0,
    nNDSFtrLeanAdmitFailOpen,       /* admission.bin missing / unreadable */
    nNDSFtrLeanAdmitFailFormat,     /* magic / version / size mismatch */
    nNDSFtrLeanAdmitFailRead,       /* short read of a record chunk */
    nNDSFtrLeanAdmitFailAsset,      /* image or TLUT file not loaded */
    nNDSFtrLeanAdmitFailResolve,    /* the cache refused the texture */
    nNDSFtrLeanAdmitFailLibc,       /* libc top chunk at the floor: stopped */
    nNDSFtrLeanAdmitFailSlots       /* cache slot reserve reached: stopped */
};
extern volatile u32 gNdsFtrLeanAdmitFail;
extern volatile u32 gNdsFtrLeanAdmitFailFirst[4];

/* TU B (renderer_fighter_lean.c): creation seam and frame pass. */
void ndsFtrLeanAdmitNoteFighter(u32 player, u32 fkind, u32 costume,
                                u32 detail);
/* Slice 2c: the creation-time admission, run last by
 * ndsBattlePrepareSceneTextures (after the scene's texture VRAM reset). */
void ndsFtrLeanAdmitSceneTexturesReady(void);
/* TU A (nds_renderer_textures_effects.c). base_asset / base_data: the
 * loaded files of the fighters present (asset id -> loaded data), which the
 * adapter reads from each kind's FTData file pointers; a record's image and
 * TLUT resolve through them (ndsRelocNativeAssetAddress), falling back to
 * ndsRelocGetLoadedAssetView. */
u32 ndsFtrLeanAdmitRun(NDSRendererStats *scratch, const u32 *fkind,
                       const u32 *costume, const u32 *detail,
                       const u32 *player, u32 count,
                       const u32 *base_asset, const void *const *base_data,
                       u32 base_count, u32 word);
/* Lock A+B once the battle's setup uploads are done (word 2; idempotent). */
void ndsFtrLeanAdmitLockRegions(void);
void ndsFtrLeanAdmitBattleExit(void);

#endif

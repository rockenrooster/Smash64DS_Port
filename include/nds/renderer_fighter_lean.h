#ifndef NDS_RENDERER_FIGHTER_LEAN_H
#define NDS_RENDERER_FIGHTER_LEAN_H

/* P2-2p8 Phase 1 -- the lean fighter path (docs/p2/
 * FOUR_FIGHTER_30FPS_ARCHITECTURE.md A1; spec: phase1-spec.md sections 2 and 6).
 *
 * Slice 1 drew Samus LOW program 0. Slice 3 (spec ORDER step 4): the four
 * stress kinds -- Donkey, Samus, Link, Kirby -- in every program and detail
 * the old path records, one instance per battle slot. The list is the packet
 * the existing recorder produced for that state, ADOPTED into the upper half
 * of the slot's framebuffer region; the per-frame work is a state-tuple
 * compare, the ARM joint kernel (Q43.20 composition from the source 16.16
 * locals, bit-exact with ndsRendererAdapterComposeOwnerWorldsSource), the
 * replay patches (plus Link's texgen words, the replay's own PatchTexgen) and
 * one DMA0. Every other state keeps today's path.
 *
 *   gNdsFtrLeanRoute  0  off -- today's behaviour (default)
 *                     1  lean path draws the four kinds when eligible
 *                     2  oracle-exact: the old path draws with the Q43.20
 *                        source compose forced for the four kinds, the lean
 *                        path patches its private copy, and the TryReplay hit
 *                        compares every patched word (expect 0 mismatches)
 *                     3  oracle-shipped: as 2 with the old path unchanged
 *                        (flat Q20.12 compose); reports LSB deltas
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
 * path adopts recorded packets, so it exists exactly where they do. */
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
#ifndef NDS_FTR_LEAN_KTIME
#define NDS_FTR_LEAN_KTIME 0
#endif

/* Decline reasons (gNdsFtrLean.decline[], per kind k_decline[][]). */
enum
{
    nNDSFtrLeanDeclineKind = 0,        /* no hitlag fold in this build */
    nNDSFtrLeanDeclineAdoptPending,    /* no adopted list for this tuple yet */
    nNDSFtrLeanDeclineTuple,           /* status/heap generation, root, detail */
    nNDSFtrLeanDeclineAnimLock,        /* (slice 3: animlocks are kernel work) */
    nNDSFtrLeanDeclineCamera,          /* no projection / camera modelview seed */
    nNDSFtrLeanDeclineMaterial,        /* material identity != adopted key[0] */
    nNDSFtrLeanDeclinePreamble,        /* key[3] preamble inputs moved */
    nNDSFtrLeanDeclineTintSet,         /* tint-tile set moved under a tinted list */
    nNDSFtrLeanDeclineResidency,       /* a bound texture is no longer resident */
    nNDSFtrLeanDeclineFence,           /* needs_fence list and the fence moved */
    nNDSFtrLeanDeclineKernel,          /* compose declined (source would too) */
    nNDSFtrLeanDeclineTint,            /* tinted site prim moved (tile rebind) */
    nNDSFtrLeanDeclineTopology,        /* live tree differs from the adopted one */
    nNDSFtrLeanDeclineSkeleton,        /* electric skeleton: no native owner */
    nNDSFtrLeanDeclineRebind,          /* material/parts rebuilt on this slot */
    nNDSFtrLeanDeclineEvents,          /* display contract event list moved */
    nNDSFtrLeanDeclineKirbyHead,       /* Kirby's joint-6 head key moved */
    nNDSFtrLeanDeclineTexgen,          /* Link's texgen patch declined */
    nNDSFtrLeanDeclineUncacheable,     /* live plan not bakeable (mixed file) */
    nNDSFtrLeanDeclineCount = 20
};

/* Adoption refusals (gNdsFtrLean.adopt_refuse[]). */
enum
{
    nNDSFtrLeanAdoptOk = 0,
    nNDSFtrLeanAdoptInvalid,           /* packet not valid / wrong root count */
    nNDSFtrLeanAdoptShape,             /* not the split-matrix layout */
    nNDSFtrLeanAdoptTexgen,            /* texgen tables over the replay's caps */
    nNDSFtrLeanAdoptTinted,            /* binds a tint tile (slice 2) */
    nNDSFtrLeanAdoptCapacity,          /* words overlap / exceed the lean half */
    nNDSFtrLeanAdoptTopology,
    nNDSFtrLeanAdoptProjectionIndex,
    nNDSFtrLeanAdoptPlan,              /* no baked plan (mixed file / miss) */
    nNDSFtrLeanAdoptRoots,             /* more roots than the lean arrays hold */
    nNDSFtrLeanAdoptRefuseCount = 10
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

/* One flushed block of counters, read with
 * -ExtraGlobals gNdsFtrLean.<field>. Published to main RAM once per frame at
 * the end of the fighter submit loop (a stop reads RAM, not the D-cache). */
typedef struct NDSFtrLeanCounters
{
    /* engagement */
    u32 draws;                  /* route 1: lean path drew */
    u32 shadow_runs;            /* routes 2/3: lean patched its private copy */
    u32 attempts;
    u32 adopts;
    u32 decline[20];
    u32 adopt_refuse[10];
    u32 adopt_tinted_sites;     /* tinted shade sites in the last adoption */
    u32 adopt_needs_fence;
    u32 adopt_words;
    u32 adopt_roots;
    u32 adopt_textures;
    /* kernel */
    u32 kernel_joints;
    u32 kernel_slow_joints;
    u32 kernel_fail;
    /* FTR sub-phases (ticks, lean-engaged draws only) */
    u32 head_ticks;             /* capture/head of every Samus draw */
    u32 guard_ticks;
    u32 kernel_ticks;
    u32 patch_ticks;
    u32 submit_ticks;
    u32 dma_wait_ticks;         /* lean submit's own DMA0 busy wait */
    u32 dma_wait_spins;
    /* oracle */
    u32 oracle_runs;
    u32 oracle_words;
    u32 oracle_mismatch[8];     /* proj, basis, row3, shade, light, other,
                                   tint tile, texgen */
    u32 oracle_max_lsb[3];      /* proj, basis, row3 (route 3) */
    u32 oracle_key_moved[8];    /* key[0..5], root count, word count */
    u32 oracle_record_under_hit[2];   /* same, differs */
    u32 oracle_unconsumed;      /* shadow armed, old path never replayed */
    u32 oracle_source_miss;     /* route 2: forced source compose failed */
    /* Record-under-hit words by class (6.4): 0 projection, 1 basis, 2 row 3,
     * 3 shade, 4 light, 5 OUTSIDE every patch site (a classification hole),
     * 6 tint tile word, 7 texgen word, 8 structure (root or word count). */
    u32 oracle_record_diff[9];
    u32 oracle_fence_rekey;     /* key[5] moved under a tint-only fence copy */
    /* First four record-under-hit shade (class 3) differences: frame serial,
     * slot<<24|root<<16|word, live word, lean word, lean site material,
     * lean site light1, lean site light2, flags (use_material | prim_from_root
     * <<8 | tinted<<16 | modulate-equal<<24 | prim-hash-equal<<25), live site
     * material, live site light1. */
    u32 oracle_shade_witness_count;
    u32 oracle_shade_witness[4][12];   /* ... [10] live site light2,
                                        * [11] live tint_modulate */
    /* Shade self-check (routes != 0): every recorded DIF_AMB word against
     * the re-derivation of its own recorded inputs at the packet's
     * tint_modulate (what ndsFighterPacketApplyTint would write). */
    u32 record_shade_checked;
    u32 record_shade_inconsistent;
    u32 record_shade_inconsistent_packets;
    u32 record_shade_witness_count;
    /* slot<<24|word, recorded, derived, light1, light2, material,
     * use|pfr<<8|tinted<<16, tint_modulate, frame serial */
    u32 record_shade_witness[2][9];
    /* Phase 0 leftovers */
    u32 ge_busy_samples;        /* GXSTAT sampled at the end of each fighter */
    u32 ge_busy_hits;           /* ... with bit 27 (GE busy) set */
    u32 packet_dma_waits;       /* next-writer waits that spun (any fighter) */
    u32 packet_dma_wait_ticks;
    u32 tint_rerecords[4];      /* per battle slot: tinted prim moved */
    /* 2.6 tile-word patch (lean copy of a tinted packet) */
    u32 tint_patch_binds;       /* tile binds patched (every lean patch) */
    u32 tint_patch_moved;       /* ... whose colour differs from the record */
    u32 tint_patch_miss;        /* colour has no resident tile: declined */
    u32 tint_patch_white;       /* colour went white (untinted): declined */
    u32 tint_patch_shape;       /* palette word presence differs: declined */
    u32 adopt_tint_binds;       /* tint binds of the last adopted packet */
    u32 adopt_fence_other;      /* non-tint fence causes of the last adopt */
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
    /* ---- slice 3: per kind (0 Donkey, 1 Samus, 2 Link, 3 Kirby) ---- */
    u32 k_attempts[4];
    u32 k_draws[4];             /* route 1 draws; routes 2/3 shadow runs */
    u32 k_adopts[4];
    u32 k_decline[4][20];
    u32 k_adopt_refuse[4][10];
    u32 k_head_ticks[4];        /* capture/head of every draw of the kind */
    u32 k_guard_ticks[4];
    u32 k_kernel_ticks[4];
    u32 k_patch_ticks[4];
    u32 k_submit_ticks[4];
    u32 k_dma_wait_ticks[4];
    u32 k_kernel_joints[4];
    u32 k_kernel_class[4][8];   /* nNDSFtrLeanJoint* */
    u32 k_program_draws[4][16]; /* draws by root program (15 = >= 15) */
    u32 k_high_draws[4];        /* draws at HIGH detail */
    u32 k_oracle_runs[4];
    u32 k_oracle_mismatch[4][8];
    /* guard / patch / submit sub-phases (all kinds; lean-engaged draws) */
    u32 guard_part_ticks[8];    /* 0 tuple, 1 topology(lab), 2 camera,
                                   3 identity, 4 preamble, 5 packet guard,
                                   6 shuffle, 7 texgen select */
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
    /* Last tinted-refusal witness per kind: tinted sites, tint binds,
     * binds seen, shade sites, fence_other, needs_fence, word count,
     * root count. */
    u32 k_tint_witness[4][8];
    u32 ident_watch_miss;       /* whole identity moved, watch saw nothing */
    u32 adopt_watch;            /* watched MObjs of the last adoption */
    u32 adopt_all_pinned;       /* adoptions whose textures are all admitted */
    u32 retuples;               /* status changes kept lean (plan re-keyed) */
    u32 rekeys;                 /* recorder packets re-keyed (route 1) */
    u32 adopt_ticks;            /* whole adoptions (joint table .. hashes) */
    u32 adopt_copy_ticks;       /* ... of which the packet copy + clean */
    u32 adopt_kept;             /* adoptions whose copy already held it */
    u32 projection_patches;     /* draws whose projection moved */
    u32 variant_learns;         /* material variants learned */
    u32 variant_words;          /* ... their words, summed */
    u32 variant_switches;       /* identity moves drawn lean via a variant */
    u32 variant_learn_fail;     /* recordings not learnable (re-adopted) */
    u32 variant_learn_ticks;    /* learn attempts, identity included */
    u32 retuple_ticks;          /* status-change re-tuples (walk + compare) */
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
 * adopted (nNDSFtrLeanAdoptTopology). */
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
/* Outputs per binding: `mv_sites[b]` (when non-NULL) receives the 16
 * LOAD4x4 modelview parameters ndsFighterPacketStoreSplitModelview would
 * write for the binding's world -- straight into the list; `binding_worlds`
 * receives the Q20.12 world itself for the bindings in `world_mask` (Link's
 * texgen reads them). */
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

/* ---- packet side (src/nds/nds_renderer_native_common.c) --------------- */

typedef struct NDSFtrLeanAdoptInfo
{
    u32 key[6];
    u32 word_count;
    u32 root_count;
    u32 texture_count;
    u32 site_count;
    u32 tinted_sites;
    u32 needs_fence;
} NDSFtrLeanAdoptInfo;

u32 ndsFtrLeanPacketUseSerial(u32 battle_slot);
/* nds_renderer_preamble.c: every packet replay hit. A hit draw runs no
 * material preparation, so it cannot have written the fighter's MObjs. */
extern volatile u32 gNdsFighterPacketHits;
/* `identity`: the material identity the recorder's packet was drawn with;
 * the copy's base (its learned variants are dropped with a new copy). */
u32 ndsFtrLeanPacketAdopt(u32 battle_slot, u32 root_count, u32 identity,
                          NDSFtrLeanAdoptInfo *info);
/* Route 1: key the recorder's packet on the material identity its words
 * were recorded with (see the adoption in renderer_fighter_lean.c). */
void ndsFtrLeanPacketRekeyIdentity(u32 battle_slot, u32 identity);
/* Slice 3 material variants (the list's second entry, see the NRC block):
 * switch the copy to the variant drawn for `identity` (the base included);
 * learn the recorder's just-drawn packet as a new variant. Both FALSE when
 * they cannot (the caller declines / re-adopts). */
u32 ndsFtrLeanPacketSelectVariant(u32 battle_slot, u32 identity);
u32 ndsFtrLeanPacketLearnVariant(u32 battle_slot, u32 identity);
void ndsFtrLeanPacketDrop(u32 battle_slot);
u32 ndsFtrLeanPacketGuard(u32 battle_slot, u32 touch);
/* The lean list's per-root LOAD4x4 modelview parameter sites, for the kernel
 * to write (their cache lines are marked for the submit). Returns the mask of
 * roots whose Q20.12 world the patch also reads (Link's texgen group roots),
 * or NDS_FTR_LEAN_SITES_NONE. */
#define NDS_FTR_LEAN_SITES_NONE 0xffffffffu
u32 ndsFtrLeanPacketModelviewSites(u32 battle_slot, u32 **sites,
                                   u32 root_count);
/* owner_slot / use_low_detail select the native runtime tables Link's texgen
 * words are derived from (the owner's root program must already be set).
 * pre_same: the roots' preambles are byte-identical to the last patch's (the
 * same memo fill), so the prim-derived tint tiles and the light word cannot
 * have moved and the shade re-derive depends on the colour modulate alone.
 * Returns 0 or a decline reason (nNDSFtrLeanDecline*). */
u32 ndsFtrLeanPacketPatch(u32 battle_slot,
                          const NDSRendererNativeFighterRoot *inputs,
                          u32 input_count, u32 owner_slot,
                          u32 use_low_detail, u32 pre_same);
void ndsFtrLeanPacketSubmit(u32 battle_slot, NDSRendererStats *stats);
/* Kind index for the counters (lab) of the lean packet in a battle slot,
 * published by the adapter at every attempt. */
void ndsFtrLeanPacketNoteKind(u32 battle_slot, u32 kind);
void ndsFtrLeanShadowArm(u32 battle_slot, u32 armed);
u32 ndsFtrLeanShadowArmed(u32 battle_slot);
void ndsFtrLeanCountersPublish(void);
void ndsFtrLeanTextureCensus(void);
void ndsFtrLeanNoteGo(u32 frame);
void ndsFtrLeanNoteTextureReject(u32 reason, u32 format, u32 size);
void ndsFtrLeanNoteTextureUpload(u32 bytes);
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

#ifndef NDS_RENDERER_FIGHTER_LEAN_H
#define NDS_RENDERER_FIGHTER_LEAN_H

/* P2-2p8 Phase 1 slice 1 -- the lean fighter path (docs/p2/
 * FOUR_FIGHTER_30FPS_ARCHITECTURE.md A1; spec: phase1-spec.md section 6).
 *
 * Scope of this slice: Samus, LOW detail, root program 0, no animation locks.
 * The list is the packet the existing recorder produced for that state,
 * ADOPTED once into the upper half of the slot's framebuffer region; the
 * per-frame work is a state-tuple compare, the ARM joint kernel (Q43.20
 * composition from the source 16.16 locals, bit-exact with
 * ndsRendererAdapterComposeOwnerWorldsSource), the replay patches and one
 * DMA0. Everything else keeps today's path.
 *
 *   gNdsFtrLeanRoute  0  off -- today's behaviour (default)
 *                     1  lean path draws Samus when eligible
 *                     2  oracle-exact: the old path draws with the Q43.20
 *                        source compose forced for Samus, the lean path
 *                        patches its private copy, and the TryReplay hit
 *                        compares every patched word (expect 0 mismatches)
 *                     3  oracle-shipped: as 2 with the old path unchanged
 *                        (flat Q20.12 compose); reports LSB deltas
 *   gNdsFtrLeanAdmit  0  today's load (default); 1 pins every texture a
 *                        fighter binds from its first use and prints the
 *                        admission census (section 2.8 of the spec)
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

#define NDS_FTR_LEAN_ROUTE_OFF 0u
#define NDS_FTR_LEAN_ROUTE_DRAW 1u
#define NDS_FTR_LEAN_ROUTE_ORACLE_EXACT 2u
#define NDS_FTR_LEAN_ROUTE_ORACLE_SHIPPED 3u

extern volatile u32 gNdsFtrLeanRoute;
extern volatile u32 gNdsFtrLeanAdmit;

/* Decline reasons (gNdsFtrLean.decline[]). */
enum
{
    nNDSFtrLeanDeclineKind = 0,        /* not Samus / not LOW / not program 0 */
    nNDSFtrLeanDeclineAdoptPending,    /* no adopted list for this tuple yet */
    nNDSFtrLeanDeclineTuple,           /* status/heap generation, root moved */
    nNDSFtrLeanDeclineAnimLock,
    nNDSFtrLeanDeclineCamera,          /* no projection / camera modelview seed */
    nNDSFtrLeanDeclineMaterial,        /* material identity != adopted key[0] */
    nNDSFtrLeanDeclinePreamble,        /* key[3] preamble inputs moved */
    nNDSFtrLeanDeclineTintSet,         /* tint-tile set moved under a tinted list */
    nNDSFtrLeanDeclineResidency,       /* a bound texture is no longer resident */
    nNDSFtrLeanDeclineFence,           /* needs_fence list and the fence moved */
    nNDSFtrLeanDeclineKernel,          /* compose declined (source would too) */
    nNDSFtrLeanDeclineTint,            /* tinted site prim moved (tile rebind) */
    nNDSFtrLeanDeclineTopology,        /* live tree differs from the adopted one */
    nNDSFtrLeanDeclineSkeleton,
    nNDSFtrLeanDeclineRebind,          /* material/parts rebuilt on this slot */
    nNDSFtrLeanDeclineCount = 16
};

/* Adoption refusals (gNdsFtrLean.adopt_refuse[]). */
enum
{
    nNDSFtrLeanAdoptOk = 0,
    nNDSFtrLeanAdoptInvalid,           /* packet not valid / wrong root count */
    nNDSFtrLeanAdoptShape,             /* not the split-matrix layout */
    nNDSFtrLeanAdoptTexgen,
    nNDSFtrLeanAdoptTinted,            /* binds a tint tile (slice 2) */
    nNDSFtrLeanAdoptCapacity,          /* words overlap / exceed the lean half */
    nNDSFtrLeanAdoptTopology,
    nNDSFtrLeanAdoptProjectionIndex,
    nNDSFtrLeanAdoptRefuseCount = 8
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
    u32 decline[16];
    u32 adopt_refuse[8];
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
    u32 oracle_mismatch[7];     /* proj, basis, row3, shade, light, other, tint tile */
    u32 oracle_max_lsb[3];      /* proj, basis, row3 (route 3) */
    u32 oracle_key_moved[8];    /* key[0..5], root count, word count */
    u32 oracle_record_under_hit[2];   /* same, differs */
    u32 oracle_unconsumed;      /* shadow armed, old path never replayed */
    u32 oracle_source_miss;     /* route 2: forced source compose failed */
    /* Record-under-hit words by class (6.4): 0 projection, 1 basis, 2 row 3,
     * 3 shade, 4 light, 5 OUTSIDE every patch site (a classification hole),
     * 6 tint tile word, 7 structure (root or word count). */
    u32 oracle_record_diff[8];
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
} NDSFtrLeanCounters;

extern NDSFtrLeanCounters gNdsFtrLean;

/* Route-2 handshake: set by the old path's matrix prep when the forced
 * Q43.20 source compose produced this draw's matrices. */
extern volatile u32 gNdsFtrLeanOracleSourceOk;

/* ---- the joint kernel (src/nds/nds_ftr_lean_kernel.c) ---------------- */

typedef struct DObj DObj;

typedef struct NDSFtrLeanJoint
{
    DObj *dobj;
    u8 parent;                  /* joint index, 0xff = topology root */
    u8 binding;                 /* production root index, 0xff = none */
    u8 pad[2];
} NDSFtrLeanJoint;

/* Slow local builder (TU of the adapter): the exact
 * ndsRendererAdapterBuildSourceFighterLocalMtx result decoded to s32 16.16
 * cells in row-major 4x3 order (rows 0-2 basis, row 3 translation).
 * Returns FALSE where the source compose would decline. */
typedef s32 (*NDSFtrLeanSlowLocalFn)(DObj *dobj, s32 *cells, u32 *has_local);

s32 ndsFtrLeanKernelCompose(const NDSFtrLeanJoint *joints, u32 joint_count,
                            NDSRendererMatrix20p12 *binding_worlds,
                            u32 binding_count,
                            s32 shuffle_x, s32 shuffle_y,
                            NDSFtrLeanSlowLocalFn slow);

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
u32 ndsFtrLeanPacketAdopt(u32 battle_slot, u32 root_count,
                          NDSFtrLeanAdoptInfo *info);
void ndsFtrLeanPacketDrop(u32 battle_slot);
u32 ndsFtrLeanPacketGuard(u32 battle_slot, u32 touch);
s32 ndsFtrLeanPacketPatch(u32 battle_slot,
                          const NDSRendererNativeFighterRoot *inputs,
                          u32 input_count);
void ndsFtrLeanPacketSubmit(u32 battle_slot, NDSRendererStats *stats);
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

#endif

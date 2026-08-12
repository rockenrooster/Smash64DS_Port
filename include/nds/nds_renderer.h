#ifndef SSB64_NDS_RENDERER_H
#define SSB64_NDS_RENDERER_H

#include <stddef.h>
#include <PR/gbi.h>

#ifndef NDS_RENDERER_PROFILE_LEVEL
#define NDS_RENDERER_PROFILE_LEVEL 2
#endif

#if (NDS_RENDERER_PROFILE_LEVEL < 0) || (NDS_RENDERER_PROFILE_LEVEL > 2)
#error "NDS_RENDERER_PROFILE_LEVEL must be 0, 1, or 2"
#endif

#ifndef NDS_SHIP_TELEMETRY
#define NDS_SHIP_TELEMETRY 1
#endif

#ifndef NDS_TICK_HUD
#define NDS_TICK_HUD 0
#endif

#if ((NDS_SHIP_TELEMETRY != 0) && (NDS_SHIP_TELEMETRY != 1)) || \
    ((NDS_TICK_HUD != 0) && (NDS_TICK_HUD != 1))
#error "NDS_SHIP_TELEMETRY and NDS_TICK_HUD must be 0 or 1"
#endif

/* Per-call frame-summary counters (matrix loads, batch begin/reuse/end,
 * texture prepare/reuse). At PROFILE_LEVEL 0 each is a read-modify-write on
 * sNdsRendererRuntimeFrameSummary on every hardware batch and every matrix
 * load, published once a frame into the gNdsRendererProfile* globals. In the
 * fighter path alone: matrix load 564, texture prepare 583 + 92 reuse, batch
 * begin 68 ticks/frame.
 *
 * DEFAULT 1, and it must stay 1 unless the gate moves with it. Cycle 110 tried
 * 0 on the reading that "nothing in the Latest or Boundary registry reads
 * them" -- verify-all.ps1 -Profile Boundary runs more than its registry rows,
 * and verify-battle-mariofox-gcrunall-loop-harness.ps1 asserts exact batch and
 * texture-prepare accounting off precisely these globals. It failed with
 * "Canonical realtime HW build drifted from exact source-weapon-aware batch
 * and texture-prepare accounting", which is the assertion doing its job: these
 * are verification evidence, not leftover instrumentation. ~1,300 ticks/frame
 * is what that evidence costs. probe-task56-fighter-path.ps1 reads them too.
 *
 * The behaviour-bearing counters (hardware_triangles, hardware_vertices,
 * hardware_over_limit) are NOT gated by this and never should be. */
#ifndef NDS_RENDERER_FRAME_SUMMARY_COUNTERS
#define NDS_RENDERER_FRAME_SUMMARY_COUNTERS 1
#endif

#ifndef NDS_RENDERER_M2_DETAILED_LEDGER
#define NDS_RENDERER_M2_DETAILED_LEDGER 0
#endif

#if (NDS_RENDERER_M2_DETAILED_LEDGER != 0) && \
    (NDS_RENDERER_M2_DETAILED_LEDGER != 1)
#error "NDS_RENDERER_M2_DETAILED_LEDGER must be 0 or 1"
#endif

#if NDS_RENDERER_M2_DETAILED_LEDGER && \
    (NDS_RENDERER_PROFILE_LEVEL != 1)
#error "NDS_RENDERER_M2_DETAILED_LEDGER requires profile level 1"
#endif

#ifndef NDS_RENDERER_M3_PHASE0_PROFILE
#define NDS_RENDERER_M3_PHASE0_PROFILE 0
#endif

#if (NDS_RENDERER_M3_PHASE0_PROFILE != 0) && \
    (NDS_RENDERER_M3_PHASE0_PROFILE != 1)
#error "NDS_RENDERER_M3_PHASE0_PROFILE must be 0 or 1"
#endif

#if NDS_RENDERER_M3_PHASE0_PROFILE && \
    (NDS_RENDERER_PROFILE_LEVEL != 1)
#error "NDS_RENDERER_M3_PHASE0_PROFILE requires profile level 1"
#endif

/* Task 26 is compile-selected so disabled control builds retain the exact
 * existing stage executor and linked layout. */
#ifndef NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
#define NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE 0
#endif

#if (NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE != 0) && \
    (NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE != 1)
#error "NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE must be 0 or 1"
#endif

/* Task 29's no-behavior GX census is a profile-1 lab surface only.  Shipping
 * builds retain no counters, wrappers, or state. */
#ifndef NDS_TASK29_GX_CENSUS
#define NDS_TASK29_GX_CENSUS 0
#endif

#if (NDS_TASK29_GX_CENSUS != 0) && (NDS_TASK29_GX_CENSUS != 1)
#error "NDS_TASK29_GX_CENSUS must be 0 or 1"
#endif

#if NDS_TASK29_GX_CENSUS && (NDS_RENDERER_PROFILE_LEVEL != 1)
#error "NDS_TASK29_GX_CENSUS requires profile level 1"
#endif

/* Task 34 E1 records the exact native-stage GX stream without changing it. */
#ifndef NDS_TASK34_STAGE_STREAM_CENSUS
#define NDS_TASK34_STAGE_STREAM_CENSUS 0
#endif

#if (NDS_TASK34_STAGE_STREAM_CENSUS != 0) && \
    (NDS_TASK34_STAGE_STREAM_CENSUS != 1)
#error "NDS_TASK34_STAGE_STREAM_CENSUS must be 0 or 1"
#endif

#ifndef NDS_TASK36_HW_COMPOSE
#define NDS_TASK36_HW_COMPOSE 0
#endif

#if (NDS_TASK36_HW_COMPOSE < 0) || (NDS_TASK36_HW_COMPOSE > 2)
#error "NDS_TASK36_HW_COMPOSE must be 0, 1, or 2"
#endif

#if NDS_TASK36_HW_COMPOSE
/* Bindings 0-19 (layer0), 30-32 (layer2) and 39-41 (layer3).
 *
 * R2-02 E4 tried to widen this to the two flower segments (25-28, 33-38) on the
 * strength of their world matrices being constant. The worlds were fine -- the
 * runtime rigid-constancy check accepted them and task36_runtime_rigid_mask held
 * the widened value all run -- and STG fell to 173,120, under R2-02's 180,000
 * gate. The flower beds were gone.
 *
 * The rigid emit path is single-binding by construction:
 * ndsRendererNativeStageEmitNoZTriangle drops a triangle whose corners are not
 * all bound to the run's own binding, before it ever reaches EnsureWorld. The
 * flower beds are the only cross-matrix geometry on Dream Land -- 10 of their 15
 * triangles, which is exactly the cross_matrix_triangles=10 that
 * M3_NATIVE_STAGE_CHECK_OK prints on every Boundary run.
 *
 * So this mask cannot take a binding whose runs carry cross-matrix triangles,
 * however constant its world is. De-cross them in the generator first (duplicate
 * each foreign corner into the run's binding space at build time), gate that on
 * the Task 49 Tier-2 differ, and crop the changed segments against the control
 * arm. See docs/optimization/ClaudeOpus5_R202_E4_ActorSegmentsRefuted_20260728.md
 * section 8.
 *
 * Overridable from nds_build_config.h for BUGS.md #9 only. A zero mask routes
 * every binding through the CPU-composed submit path -- the one binding 29
 * already uses -- which is the A/B that decides whether the pause-orbit floor
 * seam belongs to the rigid path or to the shared no-Z path. It must be paired
 * with NDS_TASK36_HW_COMPOSE=1, never 2: the replay segment set below is fixed
 * at 0/5/7 on the contract that every binding in them is rigid, and replaying a
 * dynamic binding's per-triangle LOAD4x4 stream pins that geometry to the
 * capture frame's camera. The default value is unchanged, so a build that does
 * not set this is byte-identical to one compiled before the guard existed. */
#ifndef NDS_RENDERER_TASK36_RIGID_BINDING_MASK
#define NDS_RENDERER_TASK36_RIGID_BINDING_MASK 0x00000381c00fffffULL
#endif
#endif

/* Task 53: default-off re-activation guard for the Task 36 replay arena
 * admission path. The robust downward-stepping arena allocator at
 * src/port/diagnostics.c:7368 secures anywhere from the 0x130000 floor
 * (after stepping down from NDS_TASKMAN_ARENA_SIZE) up to the full arena;
 * the captured words live in a STATIC BSS buffer and have no
 * arena-layout dependency, so the legacy exact-arena guard was a stale
 * "pristine environment only" check. When this flag is 1 the BeginFrame
 * and StartCapture admission guards admit any arena of >= 0x130000 bytes
 * regardless of alloc-fail-count, so 0x14C000 (tick-HUD) and 0x14E000
 * (published) become eligible. When this flag is 0 the guards are kept
 * byte-identical to master so the published ROM stays 1818AA77-sh equivalent. */
#ifndef NDS_TASK53_REPLAY_ARENA_FIX
#define NDS_TASK53_REPLAY_ARENA_FIX 0
#endif

#if (NDS_TASK53_REPLAY_ARENA_FIX != 0) && \
    (NDS_TASK53_REPLAY_ARENA_FIX != 1)
#error "NDS_TASK53_REPLAY_ARENA_FIX must be 0 or 1"
#endif

#if NDS_TASK53_REPLAY_ARENA_FIX && (NDS_TASK36_HW_COMPOSE != 2)
#error "NDS_TASK53_REPLAY_ARENA_FIX=1 requires NDS_TASK36_HW_COMPOSE=2"
#endif

#ifndef NDS_TASK55_STAGE_GEOM
#define NDS_TASK55_STAGE_GEOM 0
#endif

#if (NDS_TASK55_STAGE_GEOM != 0) && (NDS_TASK55_STAGE_GEOM != 1)
#error "NDS_TASK55_STAGE_GEOM must be 0 or 1"
#endif

#if NDS_TASK55_STAGE_GEOM && (NDS_TASK36_HW_COMPOSE != 2)
#error "NDS_TASK55_STAGE_GEOM=1 requires NDS_TASK36_HW_COMPOSE=2"
#endif

#if NDS_TASK55_STAGE_GEOM && !NDS_TASK53_REPLAY_ARENA_FIX
#error "NDS_TASK55_STAGE_GEOM=1 requires NDS_TASK53_REPLAY_ARENA_FIX=1 (capture path must be live)"
#endif

#ifndef NDS_TASK56_FIGHTER_PRIMITIVES
#define NDS_TASK56_FIGHTER_PRIMITIVES 0
#endif

#if (NDS_TASK56_FIGHTER_PRIMITIVES != 0) && (NDS_TASK56_FIGHTER_PRIMITIVES != 1) && \
    (NDS_TASK56_FIGHTER_PRIMITIVES != 2)
#error "NDS_TASK56_FIGHTER_PRIMITIVES must be 0, 1, or 2"
#endif

/* Task 56 emits fighter GL_TRIANGLE_STRIP/QUAD primitive streams; the native
 * fighter production path is compiled only when HW triangles are on and the
 * instrumentation level is below the forensic oracle. */
#if NDS_TASK56_FIGHTER_PRIMITIVES && !NDS_RENDERER_HW_TRIANGLES
#error "NDS_TASK56_FIGHTER_PRIMITIVES requires NDS_RENDERER_HW_TRIANGLES (native fighter path)"
#endif
#if NDS_TASK56_FIGHTER_PRIMITIVES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
#error "NDS_TASK56_FIGHTER_PRIMITIVES requires NDS_RENDERER_PROFILE_LEVEL < 2"
#endif

/* Task 62: generated Dream Land DS-native static 3D mesh (candidate c120). The
 * runtime draw path bypasses the segment0 program and emits the baked blob
 * directly via GFX_FIFO. Default off; the published ROM is byte-identical at 0. */
#ifndef NDS_DREAMLAND_DS_MESH
#define NDS_DREAMLAND_DS_MESH 0
#endif

#if (NDS_DREAMLAND_DS_MESH != 0) && (NDS_DREAMLAND_DS_MESH != 1)
#error "NDS_DREAMLAND_DS_MESH must be 0 or 1"
#endif

#if NDS_DREAMLAND_DS_MESH && !NDS_RENDERER_HW_TRIANGLES
#error "NDS_DREAMLAND_DS_MESH requires NDS_RENDERER_HW_TRIANGLES (native stage path)"
#endif
#if NDS_DREAMLAND_DS_MESH && (NDS_RENDERER_PROFILE_LEVEL >= 2)
#error "NDS_DREAMLAND_DS_MESH requires NDS_RENDERER_PROFILE_LEVEL < 2"
#endif

#if (NDS_TASK36_HW_COMPOSE == 2) && \
    (NDS_TASK29_GX_CENSUS || NDS_TASK34_STAGE_STREAM_CENSUS)
#error "Task 36 replay cannot be combined with the Task 29/34 stream census"
#endif

#ifndef NDS_RENDERER_SCREEN_SPACE_CENSUS
#define NDS_RENDERER_SCREEN_SPACE_CENSUS 0
#endif

#if (NDS_RENDERER_SCREEN_SPACE_CENSUS != 0) && \
    (NDS_RENDERER_SCREEN_SPACE_CENSUS != 1)
#error "NDS_RENDERER_SCREEN_SPACE_CENSUS must be 0 or 1"
#endif

#if NDS_RENDERER_SCREEN_SPACE_CENSUS && \
    (NDS_RENDERER_PROFILE_LEVEL != 1)
#error "NDS_RENDERER_SCREEN_SPACE_CENSUS requires profile level 1"
#endif

#ifndef NDS_RENDER_ECONOMY
#define NDS_RENDER_ECONOMY 0
#endif

#ifndef NDS_RENDER_ECONOMY_OWNER_MASK
#define NDS_RENDER_ECONOMY_OWNER_MASK 32
#endif

#if (NDS_RENDER_ECONOMY != 0) && (NDS_RENDER_ECONOMY != 1)
#error "NDS_RENDER_ECONOMY must be 0 or 1"
#endif

#if (NDS_RENDER_ECONOMY_OWNER_MASK < 0) || \
    (NDS_RENDER_ECONOMY_OWNER_MASK > 255)
#error "NDS_RENDER_ECONOMY_OWNER_MASK must fit the eight Dream Land owners"
#endif

#define NDS_RENDERER_BENCHMARK_NONE 0
#define NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP 1
#define NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX 2
#define NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD 4

#define NDS_RENDERER_FAST_RUN_GENERIC 0u
#define NDS_RENDERER_FAST_RUN_MARIO_ONLY 1u
#define NDS_RENDERER_FAST_RUN_FIGHTERS 2u
#define NDS_RENDERER_FAST_RUN_ALL_RAW_CURRENT 3u
#define NDS_RENDERER_FAST_RUN_STAGE_TEXTURE_SITES 4u
#define NDS_RENDERER_FAST_RUN_NATIVE_MARIO 5u
#define NDS_RENDERER_FAST_RUN_NATIVE_FOX 6u
#define NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS 7u
#define NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION 8u
#define NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE 9u

/* G1 -- the stage texture-site memo's runtime route and its census.
 *
 * ndsRendererProfileSetOwner enables sNdsRendererStageTextureSites only for
 * fast-run modes 4/7/8, and every measured ROM builds mode 9, so the memo has
 * never executed in battle. Route 0 is that behaviour byte for byte; route 1
 * adds mode 9. One binary carries both arms because this ROM's pacing is
 * placement-sensitive and separately-linked A/B ROMs have confused two
 * comparisons (board standing rule 7).
 *
 * The counters are deliberately NOT guarded by NDS_RENDERER_PROFILE_LEVEL: the
 * tick-HUD measuring target builds at profile 0, and a proof-scoped counter
 * reads 0 there indistinguishably from a clean one.
 *
 * Overwrites is the load-bearing number, not the hit rate. The refuted
 * (dl-pointer, bind-ordinal) memo took 471 hits on 10,336 consults with 7,517
 * evictions of 7,525 fills against a working set estimated at ~175 keys; this
 * table holds 128. If the working set still overflows, Remember's round-robin
 * fallback thrashes and the memo pays a probe walk to miss. */
extern volatile u32 gNdsG1SiteCacheRoute;
extern volatile u32 gNdsG1SiteConsults;
extern volatile u32 gNdsG1SiteHits;
extern volatile u32 gNdsG1SiteRemembers;
extern volatile u32 gNdsG1SiteOverwrites;
extern volatile u32 gNdsG1SiteOccupancy;

#ifndef NDS_RENDERER_BENCHMARK_MODE
#define NDS_RENDERER_BENCHMARK_MODE NDS_RENDERER_BENCHMARK_NONE
#endif

#if NDS_TASK29_GX_CENSUS && \
    (NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE)
#error "NDS_TASK29_GX_CENSUS requires real GX emission"
#endif

#if (NDS_RENDERER_BENCHMARK_MODE < NDS_RENDERER_BENCHMARK_NONE) || \
    (NDS_RENDERER_BENCHMARK_MODE > NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD)
#error "NDS_RENDERER_BENCHMARK_MODE must be 0 through 4"
#endif

#define NDS_RENDERER_BLOCKER_NONE 0u
#define NDS_RENDERER_BLOCKER_BAD_BRANCH 1u
#define NDS_RENDERER_BLOCKER_TOO_DEEP 2u
#define NDS_RENDERER_BLOCKER_BUDGET 3u
#define NDS_RENDERER_BLOCKER_UNSUPPORTED 4u
#define NDS_RENDERER_BLOCKER_NO_VERTICES 5u
#define NDS_RENDERER_BLOCKER_NO_TRIANGLES 6u
#define NDS_RENDERER_BLOCKER_NO_END 7u

#define NDS_RENDERER_RESOLVE_NONE 0u
#define NDS_RENDERER_RESOLVE_SEGMENT 1u

#define NDS_RENDERER_TEXTURE_SETCOMBINE (1u << 0)
#define NDS_RENDERER_TEXTURE_SETTILE (1u << 1)
#define NDS_RENDERER_TEXTURE_TEXTURE (1u << 2)
#define NDS_RENDERER_TEXTURE_SETTILESIZE (1u << 3)
#define NDS_RENDERER_TEXTURE_SETTIMG (1u << 4)
#define NDS_RENDERER_TEXTURE_LOADBLOCK (1u << 5)
#define NDS_RENDERER_TEXTURE_LOADTILE (1u << 6)

#define NDS_RENDERER_TEXTURE_STATE_SEEN (1u << 0)
#define NDS_RENDERER_TEXTURE_STATE_ON (1u << 1)
#define NDS_RENDERER_TEXTURE_STATE_SCALE_S (1u << 2)
#define NDS_RENDERER_TEXTURE_STATE_SCALE_T (1u << 3)

#define NDS_RENDERER_TILE_RENDER_SEEN (1u << 0)
#define NDS_RENDERER_TILE_LOAD_SEEN (1u << 1)
#define NDS_RENDERER_TILE_S_CLAMP (1u << 2)
#define NDS_RENDERER_TILE_S_MIRROR (1u << 3)
#define NDS_RENDERER_TILE_S_MASKED (1u << 4)
#define NDS_RENDERER_TILE_T_CLAMP (1u << 5)
#define NDS_RENDERER_TILE_T_MIRROR (1u << 6)
#define NDS_RENDERER_TILE_T_MASKED (1u << 7)

#define NDS_RENDERER_VERTEX_CACHE_SIZE 32u
#define NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY 64u
#define NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX 27u
/* Slice 43. Not a palette index: the DS position stack is 31 deep, so 0..30 are
 * real slots and 31 is the same "no slot" sentinel the generated cross-slot
 * tables already use. */
#define NDS_RENDERER_FIGHTER_GX_SLOT_NONE 31u
/* Slice 43. NOT the joint count: a binding whose baked parent is 0xff walks all
 * the way to the DObj root, and Mario has THREE such bindings, so their shared
 * ancestors are captured once per root chain. Sized at JOINT_MAX 27 the capture
 * declined on every Mario owner -- 535 of 535 -- after reaching 26. */
#define NDS_RENDERER_FIGHTER_GX_LOCAL_MAX 48u
#define NDS_RENDERER_TILE_COUNT 8u
#define NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT 2u
#define NDS_RENDERER_SEMANTIC_TRACE_CAPACITY 832u

#define NDS_RENDERER_GEOM_ZBUFFER 0x00000001u
#define NDS_RENDERER_GEOM_SHADE 0x00000004u
#define NDS_RENDERER_GEOM_CULL_FRONT 0x00000200u
#define NDS_RENDERER_GEOM_CULL_BACK 0x00000400u
#define NDS_RENDERER_GEOM_FOG 0x00010000u
#define NDS_RENDERER_GEOM_LIGHTING 0x00020000u
#define NDS_RENDERER_GEOM_TEXTURE_GEN 0x00040000u
#define NDS_RENDERER_GEOM_TEXTURE_GEN_LINEAR 0x00080000u
#define NDS_RENDERER_GEOM_SHADING_SMOOTH 0x00200000u
#define NDS_RENDERER_GEOM_RESET_MODE \
    (NDS_RENDERER_GEOM_ZBUFFER | NDS_RENDERER_GEOM_SHADE | \
     NDS_RENDERER_GEOM_CULL_BACK | NDS_RENDERER_GEOM_SHADING_SMOOTH)

typedef s32 (*NDSRendererValidateRange)(const Gfx *dl, size_t bytes,
                                        void *user);
typedef size_t (*NDSRendererImmutableCommandSpan)(const Gfx *dl, void *user);
typedef const Gfx *(*NDSRendererResolveBranch)(const Gfx *dl,
                                               u32 *resolve_kind,
                                               void *user);
typedef const void *(*NDSRendererResolveData)(const void *ptr, size_t bytes,
                                              void *user);

typedef struct NDSRendererMatrix20p12
{
    s32 m[4][4];
} NDSRendererMatrix20p12;

/* The particle pass draws in world space and therefore needs the scene camera,
 * which it used to lack entirely -- quads inherited whichever object's matrix
 * was last loaded, so effects rendered at the eye or inside a stage segment.
 * The game supplies it here because gGMCameraMatrix and gGCMatrixPerspF live
 * behind headers the renderer does not include. Pass NULLs to invalidate. */
void ndsRendererSetParticleCamera(const NDSRendererMatrix20p12 *projection,
                                  const NDSRendererMatrix20p12 *modelview);

/* Task 86. `*dst = *src` on this struct is 64 bytes, and GCC answers that with
 * `bl memcpy` rather than inline loads: on ARMv5 it cannot assume the pointers
 * are aligned, and 16 words is past the size it will open-code blind. Task 85
 * measured what that costs -- a memcpy call is ~70 ticks of call machinery
 * whatever it carries -- and objdump shows these copies are the largest
 * remaining group of them in the frame.
 *
 * Sixteen explicit element assignments instead. Straight-line rather than a
 * loop deliberately: -ftree-loop-distribute-patterns rewrites a word-copy loop
 * straight back into the memcpy this exists to avoid. Indexing `m` rather than
 * casting to u32* keeps it free of aliasing games, and the compiler pairs the
 * accesses into LDM/STM by itself. */
static inline void ndsRendererMatrixCopy20p12(
    NDSRendererMatrix20p12 *dst, const NDSRendererMatrix20p12 *src)
{
    dst->m[0][0] = src->m[0][0];
    dst->m[0][1] = src->m[0][1];
    dst->m[0][2] = src->m[0][2];
    dst->m[0][3] = src->m[0][3];
    dst->m[1][0] = src->m[1][0];
    dst->m[1][1] = src->m[1][1];
    dst->m[1][2] = src->m[1][2];
    dst->m[1][3] = src->m[1][3];
    dst->m[2][0] = src->m[2][0];
    dst->m[2][1] = src->m[2][1];
    dst->m[2][2] = src->m[2][2];
    dst->m[2][3] = src->m[2][3];
    dst->m[3][0] = src->m[3][0];
    dst->m[3][1] = src->m[3][1];
    dst->m[3][2] = src->m[3][2];
    dst->m[3][3] = src->m[3][3];
}

/* R2-03 E69. The clear half of the same problem. `memset(out, 0, 64)` followed by
 * four diagonal stores is a library call plus a loop, to write twelve zeros;
 * `ndsRendererAdapterMtxIdentity20p12` and `ndsRendererMtxIdentity20p12` were both
 * written that way and E68b found them in the memset attribution. Straight-line
 * for the same reason the copy above is: a zeroing loop gets rewritten back into
 * `memset` by -ftree-loop-distribute-patterns.
 *
 * `one` is the caller's fixed-point scale, because the adapter and the renderer
 * disagree on the macro name for it (NDS_RENDERER_ADAPTER_MTX_FRAC_BITS vs
 * NDS_RENDERER_DS_MTX_FRAC_BITS) while agreeing on the value. */
static inline void ndsRendererMatrixIdentity20p12(
    NDSRendererMatrix20p12 *dst, s32 one)
{
    dst->m[0][0] = one;
    dst->m[0][1] = 0;
    dst->m[0][2] = 0;
    dst->m[0][3] = 0;
    dst->m[1][0] = 0;
    dst->m[1][1] = one;
    dst->m[1][2] = 0;
    dst->m[1][3] = 0;
    dst->m[2][0] = 0;
    dst->m[2][1] = 0;
    dst->m[2][2] = one;
    dst->m[2][3] = 0;
    dst->m[3][0] = 0;
    dst->m[3][1] = 0;
    dst->m[3][2] = 0;
    dst->m[3][3] = one;
}

typedef struct NDSRendererMatrixSnapshot
{
    NDSRendererMatrix20p12 matrix;
    u32 generation;
    u32 signature;
} NDSRendererMatrixSnapshot;

typedef struct NDSRendererInputVertex
{
    s16 x;
    s16 y;
    s16 z;
    s16 s;
    s16 t;
    u8 r;
    u8 g;
    u8 b;
    u8 a;
} NDSRendererInputVertex;

typedef struct NDSRendererClipVertex20p12
{
    s32 x;
    s32 y;
    s32 z;
    s32 w;
} NDSRendererClipVertex20p12;

typedef struct NDSRendererVertexCache
{
    NDSRendererInputVertex input_vertices[NDS_RENDERER_VERTEX_CACHE_SIZE];
    NDSRendererClipVertex20p12
        transformed_vertices[NDS_RENDERER_VERTEX_CACHE_SIZE];
    u32 vertex_colors[NDS_RENDERER_VERTEX_CACHE_SIZE];
    NDSRendererMatrixSnapshot
        matrix_snapshots[NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY];
    u8 vertex_matrix_snapshot[NDS_RENDERER_VERTEX_CACHE_SIZE];
    u8 vertex_clip_snapshot[NDS_RENDERER_VERTEX_CACHE_SIZE];
    u32 input_valid_mask;
    u32 transformed_valid_mask;
    u32 vertex_color_valid_mask;
    u32 matrix_snapshot_count;
    u32 raw_vertex_fit_mask;
} NDSRendererVertexCache;

typedef enum NDSRendererProfileOwner
{
    NDS_RENDERER_PROFILE_OWNER_STAGE = 0,
    NDS_RENDERER_PROFILE_OWNER_MARIO,
    NDS_RENDERER_PROFILE_OWNER_FOX,
    NDS_RENDERER_PROFILE_OWNER_COUNT,
    NDS_RENDERER_PROFILE_OWNER_NONE = NDS_RENDERER_PROFILE_OWNER_COUNT
} NDSRendererProfileOwner;

#if NDS_TASK29_GX_CENSUS || NDS_TASK34_STAGE_STREAM_CENSUS || \
    (NDS_TASK36_HW_COMPOSE == 2) || NDS_TASK49_GX_DIFFER
typedef enum NDSRendererTask29GXClass
{
    NDS_TASK29_GX_CONTROL = 0,
    NDS_TASK29_GX_ALPHA_TEST,
    NDS_TASK29_GX_FOG_TABLE,
    NDS_TASK29_GX_FOG_OFFSET,
    NDS_TASK29_GX_FOG_COLOR,
    NDS_TASK29_GX_TEXTURE_PARAM,
    NDS_TASK29_GX_TEXTURE_BIND,
    NDS_TASK29_GX_MATRIX_MODE,
    NDS_TASK29_GX_MATRIX_IDENTITY,
    NDS_TASK29_GX_MATRIX_LOAD4X4,
    NDS_TASK29_GX_MATRIX_MULT4X4,
    NDS_TASK29_GX_MATRIX_PUSH,
    NDS_TASK29_GX_MATRIX_POP,
    NDS_TASK29_GX_MATRIX_STORE,
    NDS_TASK29_GX_MATRIX_RESTORE,
    NDS_TASK29_GX_POLY_FORMAT,
    NDS_TASK29_GX_BEGIN,
    NDS_TASK29_GX_END,
    NDS_TASK29_GX_COLOR,
    NDS_TASK29_GX_TEX_COORD,
    NDS_TASK29_GX_VERTEX16,
    NDS_TASK29_GX_FLUSH,
    NDS_TASK29_GX_MATRIX_MULT4x3,  /* Task 51: 12-word model matrix under a once-loaded view. Appended so existing enum values (and the masks that index them) stay byte-stable at default. */
    NDS_TASK29_GX_CLASS_COUNT
} NDSRendererTask29GXClass;
#endif

#if NDS_TASK29_GX_CENSUS
#define NDS_TASK29_GX_OWNER_COUNT \
    (NDS_RENDERER_PROFILE_OWNER_COUNT + 1u)

extern volatile u32 gNdsTask29GXFrame;
extern volatile u32 gNdsTask29GXCommandCount[NDS_TASK29_GX_CLASS_COUNT];
extern volatile u32 gNdsTask29GXWordCount[NDS_TASK29_GX_CLASS_COUNT];
extern volatile u32 gNdsTask29GXRepeatCount[NDS_TASK29_GX_CLASS_COUNT];
extern volatile u32 gNdsTask29GXOwnerCommandCount
    [NDS_TASK29_GX_OWNER_COUNT][NDS_TASK29_GX_CLASS_COUNT];
extern volatile u32 gNdsTask29GXOwnerWordCount
    [NDS_TASK29_GX_OWNER_COUNT][NDS_TASK29_GX_CLASS_COUNT];
extern volatile u32 gNdsTask29GXOwnerRepeatCount
    [NDS_TASK29_GX_OWNER_COUNT][NDS_TASK29_GX_CLASS_COUNT];
extern volatile u32 gNdsTask29GXTotalCommandCount;
extern volatile u32 gNdsTask29GXTotalWordCount;
extern volatile u32 gNdsTask29GXTotalRepeatCount;
extern volatile u32 gNdsTask29GXStreamHashA;
extern volatile u32 gNdsTask29GXStreamHashB;
extern volatile u32 gNdsTask29GXOwnerHashA[NDS_TASK29_GX_OWNER_COUNT];
extern volatile u32 gNdsTask29GXOwnerHashB[NDS_TASK29_GX_OWNER_COUNT];
extern volatile u32 gNdsTask29GXBoundaryHashA;
extern volatile u32 gNdsTask29GXBoundaryHashB;
extern volatile u32 gNdsTask29GXBoundaryCount;
extern volatile u32 gNdsTask29GXFaultCount;
extern volatile u32 gNdsTask29GXNeverSuppressMask;
#endif

#if NDS_TASK34_STAGE_STREAM_CENSUS
#define NDS_TASK34_STAGE_STREAM_ENTRY_CAPACITY 2800u
#define NDS_TASK34_STAGE_STREAM_WORD_CAPACITY 7000u
#define NDS_TASK34_STAGE_STREAM_DOBJ_NONE 0xffffu

typedef struct NDSRendererTask34StageStreamEntry
{
    u16 word_offset;
    u16 dobj_index;
    u8 command_class;
    u8 word_count;
    u8 segment_index;
    u8 reserved;
} NDSRendererTask34StageStreamEntry;

extern volatile u32 gNdsTask34StageStreamFrame;
extern volatile u32 gNdsTask34StageStreamCaptureEnabled;
extern volatile u32 gNdsTask34StageStreamEntryCount;
extern volatile u32 gNdsTask34StageStreamWordCount;
extern volatile u32 gNdsTask34StageStreamOverflowCount;
extern volatile u32 gNdsTask34StageStreamFaultCount;
extern volatile NDSRendererTask34StageStreamEntry
    gNdsTask34StageStreamEntries[NDS_TASK34_STAGE_STREAM_ENTRY_CAPACITY];
extern volatile u32
    gNdsTask34StageStreamWords[NDS_TASK34_STAGE_STREAM_WORD_CAPACITY];

void ndsRendererTask34StageStreamBeginSegment(u32 segment_index);
void ndsRendererTask34StageStreamSetDObj(u32 dobj_index);
void ndsRendererTask34StageStreamEndSegment(void);
#endif

#if NDS_TASK29_GX_CENSUS
void ndsRendererTask29GXRecordFlush(u32 mode);
void ndsRendererTask29GXSetOwner(NDSRendererProfileOwner owner);
void ndsRendererTask29GXPublishFrame(void);
#endif

#if NDS_RENDERER_SCREEN_SPACE_CENSUS
#define NDS_RENDERER_SCREEN_SPACE_CENSUS_PART_COUNT 42u
#define NDS_RENDERER_SCREEN_SPACE_CENSUS_STAGE_OWNER_COUNT 8u

typedef struct NDSRendererScreenSpaceCensusRow
{
    u32 identity;
    u32 triangle_count;
    u32 area_lt_1px_count;
    u32 area_lt_4px_count;
    u32 invalid_count;
    u64 area2_q8_sum;
} NDSRendererScreenSpaceCensusRow;
#endif

/* Diagnostic-only owner census.  Profile 0 never allocates or touches this
 * ledger; profiles 1/2 publish it once per synchronized frame. */
typedef struct NDSRendererOwnerProfile
{
    u32 exclusive_ticks;
    u32 selected_count;
    u32 source_command_count;
    u32 vertex_command_count;
    u32 source_vertex_count;
    u32 triangle_command_count;
    u32 triangle_count;
    u32 submit_class_count[8];
    u32 material_operation_count;
    u32 matrix_change_count;
    u32 texture_change_count;
    u32 run_count;
    u32 entry_state_hash;
    u32 exit_state_hash;
    u32 entry_vertex_cache_hash;
    u32 exit_vertex_cache_hash;
    u32 entry_resolver_hash;
    u32 exit_resolver_hash;
    u32 entry_global_hash;
    u32 exit_global_hash;
    u32 topology_signature;
    u32 selected_event_signature;
    u32 camera_signature;
    u32 dobj_matrix_signature;
    u32 material_signature;
    u32 light_signature;
    u32 texture_signature;
    u32 semantic_output_hash;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    /* M2 decision ledger.  These fields are deliberately absent from the
     * shipping profile-0 owner and from the independent profile-2 oracle. */
    u32 m2_contract_capture_ticks;
    u32 m2_collection_ticks;
    u32 m2_owner_validation_ticks;
    u32 m2_census_ticks;
    u32 m2_camera_fetch_ticks;
    u32 m2_hash_parent_lookup_ticks;
    u32 m2_local_matrix_ticks;
    u32 m2_world_affine_ticks;
    u32 m2_world_camera_ticks;
    u32 m2_final_compose_ticks;
    u32 m2_material_ticks;
    u32 m2_production_total_ticks;
    u32 m2_production_preflight_state_ticks;
    u32 m2_lighting_shading_ticks;
    u32 m2_root_gx_ticks;
    u32 m2_run_prepare_ticks;
    u32 m2_corner_emit_account_ticks;
    u32 m2_owner_residual_ticks;
    u32 m2_production_success_count;
    u32 m2_production_failure_count;
    u32 m2_production_phase_overlap_count;
    u32 m2_owner_phase_overlap_count;
    u32 m2_schedule_joint_count;
    u32 m2_schedule_match_count;
    u32 m2_binding_count;
    u32 m2_binding_match_count;
    u32 m2_xobj_count;
    u32 m2_xobj_kind_4b_count;
    u32 m2_xobj_kind_2_count;
    u32 m2_xobj_other_count;
    u32 m2_xobj_null_count;
    u32 m2_parts_count;
    u32 m2_parts_matrix_mode0_count;
    u32 m2_parts_matrix_mode1_count;
    u32 m2_parts_matrix_mode3_count;
    u32 m2_parts_matrix_other_count;
    u32 m2_animlock_active;
    u32 m2_camera_fetch_count;
    u32 m2_world_matrix_request_count;
    u32 m2_world_matrix_cache_hit_count;
    u32 m2_local_matrix_build_count;
    u32 m2_world_affine_count;
    u32 m2_world_camera_count;
    u32 m2_final_compose_count;
    u32 m2_root_gx_count;
    u32 m2_lighting_epoch_count;
    u32 m2_run_prepare_count;
    u32 m2_corner_emit_run_count;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 semantic_output_hash2;
    u32 semantic_event_count;
    u32 semantic_overflow_count;
    u32 semantic_occurrence_count;
    u32 semantic_first_owner_occurrence;
    u32 semantic_first_list_ordinal;
    u32 semantic_first_branch_path;
    u32 semantic_first_command_index;
    u32 semantic_first_tri2_half;
    u32 semantic_first_outcome;
#endif
} NDSRendererOwnerProfile;

#if NDS_RENDERER_PROFILE_LEVEL >= 1
extern volatile NDSRendererOwnerProfile
    gNdsRendererProfileOwners[NDS_RENDERER_PROFILE_OWNER_COUNT];
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
extern volatile u32 gNdsRendererSemanticOutputHash;
extern volatile u32 gNdsRendererSemanticOutputHash2;
extern volatile u32 gNdsRendererSemanticEventCount;
extern volatile u32 gNdsRendererSemanticOverflowCount;
extern volatile u32 gNdsRendererSemanticPrefixHash[
    NDS_RENDERER_SEMANTIC_TRACE_CAPACITY];
extern volatile u32 gNdsRendererSemanticPrefixHash2[
    NDS_RENDERER_SEMANTIC_TRACE_CAPACITY];

#define NDS_RENDERER_STAGE_DEPTH_TRACE_CAPACITY 202u
typedef struct NDSRendererStageDepthTrace
{
    u32 owner_occurrence;
    u32 list_ordinal;
    u32 branch_path;
    u32 command_index;
    s32 projected_z[3];
    s16 submitted_z[3];
    u8 submit_class;
    u8 source_zbuffered;
    u8 no_z_phase;
    u8 tri2_half;
} NDSRendererStageDepthTrace;

extern volatile NDSRendererStageDepthTrace gNdsRendererStageDepthTrace[
    NDS_RENDERER_STAGE_DEPTH_TRACE_CAPACITY];
extern volatile u32 gNdsRendererStageDepthTraceCount;
extern volatile u32 gNdsRendererStageDepthTraceOverflowCount;
extern volatile u32 gNdsRendererStageDepthTraceHash;
extern volatile u32 gNdsRendererStageDepthTraceClassCount[8];
extern volatile u32 gNdsRendererStageDepthTraceNoZCollisionCount;
extern volatile u32 gNdsRendererStageDepthTraceBackgroundCount;
extern volatile s32 gNdsRendererStageDepthTraceBackgroundMin;
extern volatile s32 gNdsRendererStageDepthTraceBackgroundMax;
extern volatile u32 gNdsRendererStageDepthTraceForegroundCount;
extern volatile s32 gNdsRendererStageDepthTraceForegroundMin;
extern volatile s32 gNdsRendererStageDepthTraceForegroundMax;
#endif

typedef struct NDSRendererCommand
{
    const Gfx *dl;
    u32 w0;
    u32 w1;
    u32 op;
    u32 depth;
    u32 list_index;
    const Gfx *raw_branch_dl;
    const Gfx *resolved_branch_dl;
    u32 branch_resolve_kind;
    u32 branch_is_jump;
    const NDSRendererClipVertex20p12 *transformed_vertices;
    u32 transformed_vertex_valid_mask;
    u32 matrix_valid;
} NDSRendererCommand;

typedef struct NDSRendererTileState
{
    u32 set_seen;
    u32 size_seen;
    u32 format;
    u32 size;
    u32 line;
    u32 tmem;
    u32 palette;
    u32 cms;
    u32 cmt;
    u32 masks;
    u32 maskt;
    u32 shifts;
    u32 shiftt;
    u32 uls;
    u32 ult;
    u32 lrs;
    u32 lrt;
    u32 width;
    u32 height;
    u32 flags;
} NDSRendererTileState;

typedef struct NDSRendererTextureLoadState
{
    u32 image;
    u32 sequence;
    u16 image_width;
    u16 load_uls;
    u16 load_ult;
    u16 load_lrs;
    u16 load_dxt;
    u16 load_texels;
    u16 load_tmem;
    u8 valid;
    u8 image_format;
    u8 image_size;
    u8 load_kind;
    u8 load_tile;
} NDSRendererTextureLoadState;

typedef s32 (*NDSRendererCommandCallback)(const NDSRendererCommand *command,
                                          void *user);

typedef enum NDSRendererTextureDataLayout
{
    NDS_RENDERER_TEXTURE_DATA_NATIVE = 0,
    NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED = 1
} NDSRendererTextureDataLayout;

typedef struct NDSRendererConfig
{
    u32 max_depth;
    u32 max_commands;
    u32 max_list_commands;
    const NDSRendererMatrix20p12 *initial_projection;
    const NDSRendererMatrix20p12 *initial_modelview;
    u32 initial_geometry_mode;
    u32 color_modulate;
    NDSRendererTextureDataLayout texture_data_layout;
    NDSRendererValidateRange validate_range;
    NDSRendererImmutableCommandSpan immutable_command_span;
    NDSRendererResolveBranch resolve_branch;
    NDSRendererResolveData resolve_data;
    void *user;
} NDSRendererConfig;

#define NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE (1u << 0)
#define NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT (1u << 1)
#define NDS_RENDERER_NATIVE_MATERIAL_LIGHT1 (1u << 2)
#define NDS_RENDERER_NATIVE_MATERIAL_LIGHT2 (1u << 3)
#define NDS_RENDERER_NATIVE_MATERIAL_PRIM (1u << 4)
#define NDS_RENDERER_NATIVE_MATERIAL_ENV (1u << 5)
#define NDS_RENDERER_NATIVE_MATERIAL_BLEND (1u << 6)
#define NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE (1u << 7)
#define NDS_RENDERER_NATIVE_MATERIAL_LOAD_BLOCK (1u << 8)
#define NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE (1u << 9)
#define NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE (1u << 10)
#define NDS_RENDERER_NATIVE_MATERIAL_SCROLL_TILE_SIZE (1u << 11)
#define NDS_RENDERER_NATIVE_MATERIAL_TEXTURE (1u << 12)

/* Typed BattleShip material effect. The adapter derives this directly from
 * the live MObj; the native owner never builds or executes a segment-E Gfx
 * mini display list. */
typedef struct NDSRendererNativeMaterial
{
    u32 effects;
    u16 command_count;
    u16 sync_count;
    u32 palette_image_w0;
    u32 palette_image;
    u32 palette_tile_w0;
    u32 palette_tile_w1;
    u32 palette_tlut_w1;
    u32 light1;
    u32 light2;
    u32 prim_w0;
    u32 prim_w1;
    u32 env_color;
    u32 blend_color;
    u32 block_image_w0;
    u32 block_image;
    u32 load_block_w0;
    u32 load_block_w1;
    u32 current_image_w0;
    u32 current_image;
    u32 render_tile_size_w0;
    u32 render_tile_size_w1;
    u32 scroll_tile_size_w0;
    u32 scroll_tile_size_w1;
    u32 texture_w0;
    u32 texture_w1;
} NDSRendererNativeMaterial;

#define NDS_RENDERER_NATIVE_PREAMBLE_VALID (1u << 0)
#define NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID (1u << 1)

typedef struct NDSRendererNativeFighterPreamble
{
    u32 geometry_mode;
    u32 cycle_type;
    u32 render_mode;
    u32 prim_color;
    u32 env_color;
    s8 light_dir_x;
    s8 light_dir_y;
    s8 light_dir_z;
    u8 flags;
} NDSRendererNativeFighterPreamble;

/* One preflighted root row for the production native fighter owner.  The
 * adapter retains ownership of every pointer for the duration of the call.
 * composed_matrix is the exact CPU-composed matrix used by the generic
 * renderer; modelview_matrix is retained separately for source lighting. */
typedef struct NDSRendererNativeFighterRoot
{
    u32 root_offset;
    u32 material_count;
    const NDSRendererMatrix20p12 *composed_matrix;
    const NDSRendererMatrix20p12 *modelview_matrix;
#if NDS_R2_FIGHTER_HW_MTX
    /* R2-03 E16b. With the split matrix load the hardware performs the
     * modelview x projection multiply, so the projection has to reach the
     * backend instead of being folded in by the adapter. */
    const NDSRendererMatrix20p12 *projection_matrix;
#endif
#if NDS_R2_FIGHTER_GX_COMPOSE
    /* Slice 43. E17 gave the geometry engine the modelview x projection
     * multiply; this gives it the joint chain as well, so `modelview_matrix`
     * above is not composed at all. `gx_locals` is this binding's chain from its
     * baked binding-parent down to itself, ALREADY in multiply order and already
     * carrying the WORLD_UNIT_SHIFT on row 3, so the backend only issues
     * MTX_MULT_4x4 in order. `gx_parent_slot` is the palette slot holding the
     * parent's finished world, or NDS_RENDERER_FIGHTER_GX_SLOT_NONE for a root
     * binding, which seeds from `gx_seed` instead. `gx_store_slot` is where this
     * binding's world must be left for a later binding or a cross-run corner. */
    const NDSRendererMatrix20p12 *gx_locals;
    const NDSRendererMatrix20p12 *gx_seed;
    u8 gx_local_count;
    u8 gx_parent_slot;
    u8 gx_store_slot;
    u8 gx_seed_is_identity;
    /* The capture is all-or-nothing per owner and it is allowed to decline, so
     * the descriptors above are only meaningful when this is set. Without it a
     * declining owner emits the PREVIOUS owner's chains: the first run of this
     * slice had Mario declining every frame and drawing Fox's joint chains,
     * with 32.06 roots/frame and only 18 of them described. */
    u8 gx_valid;
#endif
    const NDSRendererNativeMaterial *materials;
    const NDSRendererConfig *config;
#if NDS_RENDERER_M2_DETAILED_LEDGER
    u32 owner_generation;
#endif
    /* By reference, not by value. The preamble is built once per contract event
     * during the record pass and is immutable for the whole playback pass, so
     * copying its 24 bytes into every root every frame was re-materialising data
     * that had not moved: a 39.5-per-frame ldmia/stmia pair the c115 per-PC
     * census prices at 144 cycles a copy, 3,250 ticks/frame, because the source
     * index is random and the destination line is write-allocated. Every
     * consumer already took its address or read one field, so the pointer costs
     * nothing to read. Never NULL -- the no-event paths point at the adapter's
     * shared zero row rather than leaving it unset, which keeps the reads
     * branch-free. */
    const NDSRendererNativeFighterPreamble *preamble;
} NDSRendererNativeFighterRoot;

/* Mode-7 laboratory candidate.  The adapter supplies exact BattleShip local
 * matrices and live topology; the DS backend validates the complete owner
 * before it mutates GX, then owns the hierarchy and direct corner stream. */
typedef struct NDSRendererNativeFighterHierarchy
{
    const NDSRendererMatrix20p12 *projection;
    const NDSRendererMatrix20p12 *camera_modelview;
    const NDSRendererMatrix20p12 *joint_locals;
    const u8 *joint_parents;
    const u8 *joint_bindings;
    const NDSRendererNativeFighterRoot *roots;
    const NDSRendererConfig *config;
    u32 joint_count;
    u32 root_count;
} NDSRendererNativeFighterHierarchy;

#define NDS_RENDERER_NATIVE_STAGE_ASSET_COUNT 4u
#define NDS_RENDERER_NATIVE_STAGE_BINDING_COUNT 42u
#define NDS_RENDERER_NATIVE_STAGE_MATERIAL_COUNT 4u
#define NDS_RENDERER_NATIVE_STAGE_DOBJ_COUNT 57u

typedef struct NDSRendererNativeStageDObj
{
    const void *identity;
    u16 parent_index;
    u16 binding_index;
    u16 transform_flags;
    u8 owner;
    u8 depth;
} NDSRendererNativeStageDObj;

/* Mode-9 Dream Land owner input. Preflight consumes all fields synchronously,
 * but successful execution retains binding_composed through the eight later
 * display-link commits; the adapter workspace must remain live until finish. */
typedef struct NDSRendererNativeStageFrame
{
    const void *asset_bases[NDS_RENDERER_NATIVE_STAGE_ASSET_COUNT];
    const NDSRendererNativeStageDObj *dobjs;
    const void *const *binding_display_lists;
    const NDSRendererMatrix20p12 *projection;
    const NDSRendererMatrix20p12 *camera_modelview;
    const NDSRendererMatrix20p12 *binding_world;
    const NDSRendererMatrix20p12 *binding_composed;
    const NDSRendererNativeMaterial *materials;
    const NDSRendererConfig *config;
    u32 topology_generation;
    u32 topology_stamp;
    u64 rigid_binding_mask;
} NDSRendererNativeStageFrame;

typedef struct NDSRendererStats
{
    u32 blocker;
    u32 command_count;
    u32 vertex_count;
    u32 triangle_count;
    u32 first_opcode;
    u32 unsupported_opcode;
    u32 vertex_command_count;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 source_vertex_count;
#endif
    u32 triangle_command_count;
    u32 matrix_command_count;
    u32 matrix_load_count;
    u32 matrix_mul_count;
    u32 matrix_projection_count;
    u32 matrix_modelview_count;
    u32 matrix_push_count;
    u32 matrix_pop_count;
    u32 matrix_transform_count;
    u32 matrix_mvp_recalc_count;
    u32 matrix_move_word_count;
    u32 transformed_vertex_count;
    u32 transformed_triangle_count;
    u32 hardware_vertex_count;
    u32 hardware_triangle_count;
    u32 hardware_zbuffer_triangle_count;
    u32 hardware_projected_depth_triangle_count;
    u32 hardware_projected_depth_sample_count;
    s32 hardware_projected_depth_min;
    s32 hardware_projected_depth_max;
    s32 hardware_projected_w_min;
    s32 hardware_projected_w_max;
    u32 hardware_decal_depth_triangle_count;
    u32 hardware_prim_depth_triangle_count;
    u32 hardware_oracle_triangle_count;
    u32 hardware_oracle_reject_count;
    u32 hardware_matrix_seed_count;
    u32 hardware_texture_bind_count;
    u32 hardware_texture_upload_count;
    u32 hardware_texture_ready_count;
    u32 hardware_texture_reject_count;
    u32 hardware_texture_format;
    u32 hardware_texture_width;
    u32 hardware_texture_height;
    u32 first_transformed_tri_v0;
    u32 first_transformed_tri_v1;
    u32 first_transformed_tri_v2;
    u32 matrix_flags;
    s32 first_transformed_x;
    s32 first_transformed_y;
    s32 first_transformed_z;
    s32 first_transformed_w;
    u32 sync_command_count;
    u32 end_command_count;
    u32 branch_command_count;
    u32 branch_call_count;
    u32 branch_jump_count;
    u32 segment_resolve_count;
    u32 othermode_command_count;
    u32 color_command_count;
    u32 light_color_command_count;
    u32 light_direction_command_count;
    u32 light_fallback_count;
    u32 unsupported_command_count;
    u32 state_command_count;
    u32 skip_command_count;
    u32 render_command_count;
    u32 max_depth_seen;
    u32 cull_command_count;
    u32 ignored_state_command_count;
    u32 first_othermode_opcode;
    u32 first_othermode_w0;
    u32 first_othermode_w1;
    u32 othermode_h;
    u32 othermode_l;
    u32 first_cull_w0;
    u32 first_cull_w1;
    const Gfx *first_branch_dl;
    const Gfx *first_resolved_branch_dl;
    u32 geometry_mode;
    u32 geometry_clear_mask;
    u32 geometry_set_mask;
    u32 geometry_command_count;
    u32 texture_mask;
    u32 texture_load_kind;
    u32 texture_command_count;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 texture_level;
    u32 texture_tile;
    u32 texture_on;
    u32 texture_xparam;
    u32 texture_state_flags;
    u32 texture_image;
    u32 texture_format;
    u32 texture_size;
    u32 texture_image_width;
    u32 texture_set_tile_count;
    u32 texture_tlut_image;
    u32 texture_tlut_count;
    u32 texture_tlut_tile;
    u32 texture_render_tile;
    u32 texture_render_tile_format;
    u32 texture_render_tile_size;
    u32 texture_render_tile_line;
    u32 texture_render_tile_tmem;
    u32 texture_render_tile_palette;
    u32 texture_render_tile_cms;
    u32 texture_render_tile_cmt;
    u32 texture_render_tile_masks;
    u32 texture_render_tile_maskt;
    u32 texture_render_tile_shifts;
    u32 texture_render_tile_shiftt;
    u32 texture_render_tile_flags;
    u32 texture_load_tile;
    u32 texture_load_block_uls;
    u32 texture_load_block_ult;
    u32 texture_load_block_lrs;
    u32 texture_load_block_dxt;
    u32 texture_load_texels;
    u32 texture_tile_size_tile;
    u32 texture_tile_size_uls;
    u32 texture_tile_size_ult;
    u32 texture_tile_size_lrs;
    u32 texture_tile_size_lrt;
    u32 texture_tile_width;
    u32 texture_tile_height;
    NDSRendererTileState texture_tiles[NDS_RENDERER_TILE_COUNT];
    u32 texture_load_sequence;
    NDSRendererTextureLoadState
        texture_loads[NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT];
    u32 texture_combine_w0;
    u32 texture_combine_w1;
    u32 texture_combine_count;
    u32 prim_color;
    u32 prim_min_level;
    u32 prim_lod_fraction;
    u32 env_color;
    u32 blend_color;
    u32 light_color_1;
    u32 light_color_2;
    u32 light_color_mask;
    s32 light_dir_x;
    s32 light_dir_y;
    s32 light_dir_z;
    u32 light_dir_mask;
    u32 prim_depth;
    u32 prim_depth_delta;
    u32 prim_depth_command_count;
    u32 fog_color;
    s32 fog_min;
    s32 fog_max;
    u32 fog_status;
    u32 texture_source_hash1;
    u32 texture_source_hash2;
} NDSRendererStats;

/* DS-native fixed-mesh effect submit. ImpactWave is the first owner: source
 * animation and live MObj state are retained, while immutable geometry is AOT
 * and the texture is a preconverted DS PAL16 asset selected by source variant.
 * source_setup remains only as the fixed state oracle for non-texture commands;
 * no N64 texture/TLUT decode is performed by the native draw. */
s32 ndsRendererSubmitNativeImpactWave(
    const NDSRendererInputVertex *vertices, u32 vertex_count,
    const u8 *triangle_indices, u32 triangle_count,
    const Gfx *source_setup,
    const NDSRendererNativeMaterial *material,
    u32 variant,
    const NDSRendererConfig *config,
    NDSRendererStats *stats);

/* Scene-entry upload of the five 16x32 PAL16 ImpactWave colour variants. The
 * texel indices and RGB555 palettes are already in DS format in ROM; this does
 * allocation/upload only, never source conversion. */
s32 ndsRendererHardwarePrepareImpactWaveTextures(void);

/* AOT EFCommonEffects3 RebirthHalo owner. root_offset is one of the three
 * immutable source wrapper offsets (main, beam, leaves); live DObj matrices
 * still come from BattleShip's source attachment/animation owner. */
s32 ndsRendererSubmitNativeRebirthHalo(
    u32 root_offset,
    const NDSRendererConfig *config,
    NDSRendererStats *stats);
s32 ndsRendererHardwarePrepareRebirthHaloTextures(void);

s32 ndsRendererMtxCellS16p16(const Mtx *mtx, u32 row, u32 col);
void ndsRendererMtxLoadN64ToDS20p12(const Mtx *src,
                                    NDSRendererMatrix20p12 *dst);
void ndsRendererMtxMul20p12(const NDSRendererMatrix20p12 *lhs,
                            const NDSRendererMatrix20p12 *rhs,
                            NDSRendererMatrix20p12 *out);
void ndsRendererMtxMulAffine20p12(const NDSRendererMatrix20p12 *lhs,
                                  const NDSRendererMatrix20p12 *rhs,
                                  NDSRendererMatrix20p12 *out);
void ndsRendererTransformVertex20p12(const NDSRendererMatrix20p12 *mtx,
                                     const NDSRendererInputVertex *vtx,
                                     NDSRendererClipVertex20p12 *out);
void ndsRendererInitStats(NDSRendererStats *stats);
void ndsRendererInitVertexCache(NDSRendererVertexCache *vertex_cache);
void ndsRendererScanDisplayList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats);
void ndsRendererExecuteDisplayList(const Gfx *dl,
                                   const NDSRendererConfig *config,
                                   NDSRendererCommandCallback callback,
                                   void *callback_user,
                                   NDSRendererStats *stats);
void ndsRendererExecuteDisplayListWithVertexCache(
    const Gfx *dl,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache);
s32 ndsRendererExecuteNativeFighterRoot(
    u32 slot,
    u32 root_ordinal,
    const void *asset_base,
    u32 root_offset,
    const NDSRendererNativeMaterial *materials,
    u32 material_count,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache);
s32 ndsRendererExecuteNativeFighterOwnerProduction(
    u32 slot,
    const void *asset_base,
    const NDSRendererNativeFighterRoot *roots,
    u32 root_count,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    u32 *out_hardware_started);
s32 ndsRendererExecuteNativeFighterOwnerHierarchy(
    u32 slot,
    const void *asset_base,
    const NDSRendererNativeFighterHierarchy *hierarchy,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    u32 *out_hardware_started);
/* Baked nearest-bound-ancestor index per matrix binding; NULL for an unknown
 * slot. Lets the owner adapter compose world matrices in one forward pass
 * instead of walking every binding to the root through the DObj world hash. */
const u8 *ndsRendererNativeFighterBindingParents(u32 slot, u32 *count);
#if NDS_R2_FIGHTER_GX_COMPOSE
const u8 *ndsRendererNativeFighterCrossPaletteSlots(u32 slot, u32 *count);
#endif
s32 ndsRendererPrepareNativeStageOwner(
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats);
s32 ndsRendererCommitNativeStageSegment(u32 segment_index);
void ndsRendererFinishNativeStageOwner(void);
void ndsRendererResetNativeStageValidationCache(void);
s32 ndsRendererBeginNativeFighterOwner(
    u32 slot,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache);
s32 ndsRendererEndNativeFighterOwner(
    u32 slot,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache);
void ndsRendererAbortNativeFighterOwner(void);
s32 ndsRendererValidateNativeFighterOwner(
    u32 slot,
    u32 asset_data_size,
    u32 root_count,
    const u32 *root_offsets,
    const u32 *material_counts);
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER && NDS_RENDERER_HW_TRIANGLES
void ndsRendererProfileCensusNativeFighterSchedule(
    u32 slot,
    const u8 *joint_parents,
    const u8 *joint_bindings,
    u32 joint_count,
    u32 binding_count,
    u32 *schedule_match_count,
    u32 *binding_match_count);
#endif
void ndsRendererHardwareResetSourceCaches(void);
void ndsRendererHardwareDiscardTextureCache(void);
/* Scene-owned texture-VRAM lifecycle (R2-07 E3/E4 root cause; see the comment on
 * the definition). Every entry into the battle scene resets libnds's texture and
 * palette allocators, so entry N allocates identically to entry 1. The caller
 * releases its own software texture owners first -- glResetTextures invalidates
 * every name. `Enable` is a runtime control arm for same-binary A/B, default on;
 * `Count` is the permanent regression guard (one per battle-scene entry). */
void ndsRendererHardwareResetSceneTextureVram(void);
extern volatile u32 gNdsRendererSceneTextureVramResetEnable;
extern volatile u32 gNdsRendererSceneTextureVramResetCount;
s32 ndsRendererHardwarePrepareBattleStaticTextures(void);

#if NDS_R2_PARTICLE_RUNTIME
/* R2-07 particle draw path. One RGB555+A1 atlas, one GL name, one bind for
 * every particle in the frame -- see the definition for why an atlas rather
 * than a texture per frame. Prepared once per battle beside the static set and
 * pinned; the name is 0 until then, and the draw seam declines on 0 rather
 * than binding something else. */
s32 ndsRendererHardwarePrepareParticleAtlas(void);
/* The quad sheet is NDS_PARTICLE_QUAD_ATLAS_SHEETS separate 8,192-byte
 * allocations rather than one larger block -- see the note on
 * QUAD_ATLAS_SHEETS_MAX in generate_nds_particle_banks.py. Each frame row
 * carries the sheet it was packed into; the no-argument form is sheet 0 and
 * exists for the callers that predate the split. */
u32 ndsRendererHardwareParticleAtlasNameForSheet(u32 sheet);
u32 ndsRendererHardwareParticleAtlasName(void);
/* Exact EFCommon texture 27: PAL16 16x8 with hardware T mirror, so UV T 0..16
 * reconstructs the source MASKT 16x16 flash without duplicated texels. */
u32 ndsRendererHardwareFoxBlasterGlowName(void);
u32 ndsRendererHardwareWhispyNativeName(u32 texture_id);
void ndsRendererHardwareDiscardParticleAtlas(void);
/* One camera-facing quad in world space. The caller supplies the camera basis
 * because it reads the CObj the source's own draw reads, and because deriving
 * it is per-frame work that must not repeat per particle. The batch opens on
 * the first quad and closes in ndsRendererEndParticleQuads, so a whole
 * particle pass is one glBegin and one bind. */
/* `color` is BGR555 (the hardware vertex colour, which has no alpha channel);
 * `alpha` is the particle's own 8-bit source alpha and becomes POLYGON_ATTR's
 * 5-bit polygon alpha. They are separate because the DS splits them, not
 * because the caller has two colours. */
s32 ndsRendererSubmitParticleQuad(u32 atlas_name, const Vec3f *pos, f32 size,
                                  u32 color, u8 alpha,
                                  const Vec3f *right, const Vec3f *up,
                                  u32 atlas_x, u32 atlas_y,
                                  u32 atlas_w, u32 atlas_h);
/* Exact-script Whispy native seam. The camera basis is quantized once per pass;
 * each native full-texture quad then reaches GX as fixed coordinates without
 * rebuilding four float corners. Submit returns -1 when the closed fixed-point
 * contract is absent so the caller can use the generic source draw. */
void ndsRendererSetWhispyNativeBasis(const Vec3f *right, const Vec3f *up);
/* One-time spawn conversion for the Fox glow AOT pool. Keeping the centre in
 * Q12 lets every visible source state avoid soft-float conversion. */
s32 ndsRendererParticlePositionToQ12(const Vec3f *pos,
                                     s32 fixed_center_q12[3]);
/* Convert one rigid Whispy root + local particle position to the renderer's
 * Q12 center without ARM9 software-float matrix arithmetic. `affine == NULL`
 * is the identity transform. The optional oracle executes the source float
 * expression and records its Q12 delta; it is for the route-3 lab arm only. */
s32 ndsRendererSubmitWhispyNativeQuad(u32 texture_name,
                                      u32 texture_slot,
                                      const Vec3f *pos, f32 size,
                                      const s32 fixed_center_q12[3],
                                      s32 fixed_size_q8,
                                      u32 color, u8 alpha,
                                      u32 mirror_mask,
                                      u32 texture_w, u32 texture_h,
                                      u32 submit_route);
/* Fox's source blaster display is an untextured four-vertex quad from
 * relocData 316. This lab submit converts the live source translation/scale
 * directly to fixed GX coordinates; facing is +1 or -1. It returns FALSE
 * outside the exact horizontal source contract so the display-list path can
 * remain the correctness fallback. */
s32 ndsRendererSubmitFoxBlasterQuad(const Vec3f *translate,
                                    f32 scale_x, f32 scale_y,
                                    s32 facing);
#if NDS_R2_FOX_GUN_OVERLAY
/* BUGS.md "Fox's pistol model is missing". Submits the blaster's 22 baked
 * triangles at joint 17's WORLD matrix, immediately after the fighter's own
 * production run, so it inherits that draw's camera and projection. `world` is
 * the joint's world matrix -- the caller owns building it, because the refresh
 * it needs (func_ovl2_800EDBA4 then parts->mtx_translate) is source-side and
 * lives in the port adapter, not here.
 *
 * The fighter body is not touched. Source swaps the joint's own display list;
 * the DS cannot, because that list belongs to reloc asset 0x13b and
 * ndsFighterDrawPlanResolve would reject the whole collection and push the
 * entire fighter off the native path for one small part. */
s32 ndsRendererSubmitFoxGun(const NDSRendererMatrix20p12 *composed);
#endif
void ndsRendererEndParticleQuads(void);
/* DEBUG-ONLY. Draws a world-space collision-diamond outline inside an open
 * particle quad batch (see src/nds/nds_renderer.c). For tuning the fireball's
 * stage-collision box; no-op if no batch is open. */
void ndsRendererSubmitDebugDiamond(f32 cx, f32 cy, f32 cz,
                                   f32 top, f32 center,
                                   f32 bottom, f32 width);
extern volatile u32 gNdsRendererParticleAtlasPrepareCount;
extern volatile u32 gNdsRendererParticleAtlasFailCount;
extern volatile u32 gNdsRendererParticleAtlasBytes;
extern volatile u32 gNdsRendererWhispyNativePrepareCount;
extern volatile u32 gNdsRendererWhispyNativeFailCount;
extern volatile u32 gNdsRendererWhispyNativeBytes;
extern volatile u32 gNdsRendererFoxBlasterGlowPrepareCount;
extern volatile u32 gNdsRendererFoxBlasterGlowFailCount;
extern volatile u32 gNdsRendererFoxBlasterGlowBytes;
/* The v16 rail, counted where it happens. Clamp must be 0: a quad that reaches
 * it is drawn flattened onto the rail instead of where it belongs, which is
 * BUGS.md's "VFX get x flattened around stage edges". ScaleEscalations and
 * ScaleShiftMax say how much range the pass had to buy to keep it at 0 --
 * 0/0 on an ordinary frame, non-zero on a Star KO, whose sparkle follows the
 * dying fighter out to z = -14,999. */
extern volatile u32 gNdsParticleWorldClampCount;
extern volatile u32 gNdsParticleScaleEscalations;
extern volatile u32 gNdsParticleScaleShiftMax;
#endif
void ndsRendererHardwareArmBattleStaticTextures(void);
/* R2-03 E48 lab probe. Latches the generic colour path's per-frame branch counts
 * at two named frames and clears the running ones. Declared unconditionally so
 * this header does not depend on the generated config defines being visible
 * first; defined only under NDS_R2_FLASH_PROBE, and its one call site is guarded
 * by the same flag. Delete with the experiment. */
void ndsRendererR2FlashProbeFrameEnd(u32 presented_frame);
void ndsRendererHardwareDiscardBattleStaticTextures(void);
void ndsRendererHardwareAbortBattleStaticTextures(void);
s32 ndsRendererHardwareUploadSceneMipCache(const u16 *mip0,
                                             const u16 *mip1,
                                             const u16 *mip2);
s32 ndsRendererHardwareDrawSceneMipCache(u32 mip_index,
                                           const s32 *tex_s_q4,
                                           const s32 *tex_t_q4,
                                           u32 columns,
                                           u32 rows);
typedef s32 (*NDSRendererTextureFillCallback)(u8 *pixels, u32 bytes,
                                               void *user_data);
s32 ndsRendererHardwarePrepareIFCommonCloudAtlas(
    u32 width, u32 height, const u16 palette[8],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name);
/* The A3I5 sibling: 32 palette entries against 3 alpha bits, where the Cloud
 * entry point is 8 entries against 5. Same upload and the same release. */
s32 ndsRendererHardwarePrepareIFCommonA3I5Atlas(
    u32 width, u32 height, const u16 palette[32],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name);
/* The lossless sibling, for a source CI4 asset whose TLUT already carries the
 * transparency in entry 0: 16 palette entries, no alpha bits spent, and the
 * fill writes PACKED nibbles (two texels per byte) rather than one byte per
 * texel. Same upload and the same release as the two above. */
s32 ndsRendererHardwarePrepareIFCommonPal16Atlas(
    u32 width, u32 height, const u16 palette[16],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name);
void ndsRendererHardwareReleaseIFCommonCloudAtlas(u32 *texture_name);
s32 ndsRendererHardwareDrawIFCommonCloudAtlas(
    u32 texture_name, s32 x_q16, s32 y_q16,
    s32 width_q16, s32 height_q16,
    u32 texture_x, u32 texture_y, u32 texture_width,
    u32 texture_height, u32 poly_id);
void ndsRendererHardwareSetNoOracle(u32 enabled);
u32 ndsRendererHardwareNoOracleEnabled(void);
u32 ndsRendererHardwareConsumeSubmittedFrame(void);
u32 ndsRendererHardwareCommitPendingTextureRefreshes(void);
void ndsRendererProfileSetOwner(NDSRendererProfileOwner owner);
void ndsRendererProfileSetSourceProvenance(u32 owner_occurrence,
                                           u32 list_ordinal,
                                           u32 root_branch_path);
void ndsRendererProfileRecordFrameBoundaryGXState(u32 status, u32 control);
void ndsRendererProfileRecordMaterialOperations(u32 count);
u32 ndsRendererProfileGlobalStateHash(void);
#if NDS_RENDER_ECONOMY
void ndsRendererProfileFrameBegin(u32 render_economy_allowed);
#else
void ndsRendererProfileFrameBegin(void);
#endif
void ndsRendererProfileFramePublish(void);
extern volatile u32 gNdsRendererFastRunMode;
extern volatile u32 gNdsRendererFastRunCount;
extern volatile u32 gNdsRendererFastTriangleCount;
extern volatile u32 gNdsRendererFastOwnerTriangleCount[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
extern volatile u32 gNdsRendererFastFallbackCount[3];
#if NDS_RENDERER_SCREEN_SPACE_CENSUS
extern volatile NDSRendererScreenSpaceCensusRow
    gNdsRendererScreenSpaceCensus[NDS_RENDERER_PROFILE_OWNER_COUNT]
                                   [NDS_RENDERER_SCREEN_SPACE_CENSUS_PART_COUNT];
extern volatile u64 gNdsRendererScreenSpaceStageOwnerTicks[
    NDS_RENDERER_SCREEN_SPACE_CENSUS_STAGE_OWNER_COUNT];
extern volatile u32 gNdsRendererScreenSpaceCensusArmed;
extern volatile u32 gNdsRendererScreenSpaceCensusResetRequested;
extern volatile u32 gNdsRendererScreenSpaceCensusFrameCount;
extern volatile u32 gNdsRendererScreenSpaceCensusOverflowCount;
#endif
#if NDS_RENDER_ECONOMY
extern volatile u32 gNdsRendererEconomyConfiguredOwnerMask;
extern volatile u32 gNdsRendererEconomyActiveOwnerMask;
extern volatile u32 gNdsRendererEconomyAppliedOwnerMask;
extern volatile u32 gNdsRendererEconomySkippedRunCount;
extern volatile u32 gNdsRendererEconomySkippedTriangleCount;
#endif
/* BUGS ROW 6, AND DELIBERATELY OUTSIDE EVERY GUARD BELOW.
 * ndsRendererAdapterPrepareNativeStageOwner's reject path aborts the pinned
 * static-texture corpus, which discards the whole hardware texture cache and
 * leaves it unable to re-arm for the rest of the scene. The existing records of
 * that event (gNdsRendererM3PostArmFailureCount,
 * gNdsRendererTask36AdapterRejectReason) sit inside
 * `#if NDS_RENDERER_PROFILE_LEVEL == 1` and are nm-confirmed absent from the
 * shipping ELF, so the shipping build could destroy its own texture cache
 * silently. Putting these under the same guard would reproduce that bug
 * exactly; they are unconditional on purpose. See nds_renderer.c for the
 * reason-code table. */
extern volatile u32 gNdsRendererStageOwnerRejectCount;
extern volatile u32 gNdsRendererStageOwnerLastRejectReason;
extern volatile u32 gNdsRendererStageOwnerFirstRejectReason;
extern volatile u32 gNdsRendererStageOwnerAbortCount;
extern volatile u32 gNdsRendererStageOwnerPostArmRejectCount;
extern volatile u32 gNdsRendererStaticTexturePreparedNow;

#if NDS_RENDERER_PROFILE_LEVEL == 1
extern volatile u32 gNdsRendererM3PreflightAttemptCount;
extern volatile u32 gNdsRendererM3PreflightSuccessCount;
extern volatile u32 gNdsRendererM3PreflightFallbackCount;
extern volatile u32 gNdsRendererM3SegmentCount;
extern volatile u32 gNdsRendererM3SegmentMask;
extern volatile u32 gNdsRendererM3PostArmFailureCount;
extern volatile u32 gNdsRendererM3DObjCount;
extern volatile u32 gNdsRendererM3BindingCount;
extern volatile u32 gNdsRendererM3RunCount;
extern volatile u32 gNdsRendererM3TriangleCount;
extern volatile u32 gNdsRendererM3ResidentEpochCount;
extern volatile u32 gNdsRendererM3MaterialShadowCount;
extern volatile u32 gNdsRendererM3MaterialCommitCount;
extern volatile u32 gNdsRendererM3CrossRunCount;
extern volatile u32 gNdsRendererM3CrossTriangleCount;
extern volatile u32 gNdsRendererM3CrossForeignCornerCount;
extern volatile u32 gNdsRendererM3TopologyFullValidationCount;
extern volatile u32 gNdsRendererM3TopologyCacheHitCount;
extern volatile u32 gNdsRendererM3TopologyStampMismatchCount;
#if NDS_TASK36_HW_COMPOSE
extern volatile u32 gNdsRendererTask36HardwareComposedDObjCount;
extern volatile u32 gNdsRendererTask36CameraLoadCount;
extern volatile u32 gNdsRendererTask36WorldMultCount;
extern volatile u32 gNdsRendererTask36AdapterRejectReason;
extern volatile u32 gNdsRendererTask36RendererRejectReason;
extern volatile u32 gNdsRendererTask36PrepareRunRejectReason;
extern volatile u32 gNdsRendererTask36RigidConstancyMismatchCount;
extern volatile u32 gNdsRendererTask36ObservedDynamicMaskLo;
extern volatile u32 gNdsRendererTask36ObservedDynamicMaskHi;
#if NDS_TASK44_STAGE_STEADY
/* Task 44 item 3 accounting. Admit = generation compare passed and the full
 * four-asset/topology validation was skipped. Revalidate = the generation moved
 * or the cheap segment guard failed, so the full path ran. Their sum is the
 * preparation count; a revalidation that never fires across a stage-asset
 * invalidation event is the failure this pair exists to catch. */
extern volatile u32 gNdsRendererTask44SteadyAdmitCount;
extern volatile u32 gNdsRendererTask44RevalidateCount;
extern volatile u32 gNdsRendererTask44AdmissionGeneration;
#endif
#if NDS_TASK36_HW_COMPOSE == 2
extern volatile u32 gNdsRendererTask36ReplayState;
extern volatile u32 gNdsRendererTask36BakeAttemptCount;
extern volatile u32 gNdsRendererTask36BakeSuccessCount;
extern volatile u32 gNdsRendererTask36BakeFailureCount;
extern volatile u32 gNdsRendererTask36ReplayFrameCount;
extern volatile u32 gNdsRendererTask36ReplaySegmentCount;
extern volatile u32 gNdsRendererTask36ReplayRunCount;
extern volatile u32 gNdsRendererTask36ReplayWordCount;
extern volatile u32 gNdsRendererTask36ReplayFallbackCount;
extern volatile u32 gNdsRendererTask36ReplayArenaRejectCount;
extern volatile u32 gNdsRendererTask36ReplayMaterialRejectCount;
extern volatile u32 gNdsRendererTask36ReplayProjectionRejectCount;
extern volatile u32 gNdsRendererTask36ReplayCaptureWordCount;
#endif
#endif
/* Task 53: declared outside the profile-1 block above so it is visible at
 * profile-0 too (mirrors the definition in nds_renderer.c) -- the staleness
 * detector is a regression catch, not a profiling instrument. */
#if NDS_TASK36_HW_COMPOSE == 2 && NDS_TASK53_REPLAY_ARENA_FIX
extern volatile u32 gNdsRendererTask36ReplayArenaStaleCount;
#endif
#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
extern volatile u32 gNdsRendererM3GeneratedSegment0AttemptCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0SuccessCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0PreGxFallbackCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0RunCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0TriangleCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0EpochCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0MaterialCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0CertificateValidationCount;
#if NDS_RENDERER_M3_PHASE0_PROFILE
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowDenseCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowStateEntryCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowSyncCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowFieldComparisonCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowMismatchCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowFaultInjectedCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowFaultRejectedCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowLiveFaultInjectedCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowLiveFaultRejectedCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowLiveFaultRevalidatedCount;
#endif
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
extern volatile u32 gNdsRendererM3TopologyFaultInjectionCount;
extern volatile u32 gNdsRendererM3TopologyFaultRevalidationCount;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
extern volatile u32 gNdsRendererM3Phase0PreflightTicks;
extern volatile u32 gNdsRendererM3Phase0PrepareRunTicks;
extern volatile u32 gNdsRendererM3Phase0VertexPrepareTicks;
extern volatile u32 gNdsRendererM3Phase0NearTransformTicks;
extern volatile u32 gNdsRendererM3Phase0RunTransitionTicks;
extern volatile u32 gNdsRendererM3Phase0RawEmitTicks;
extern volatile u32 gNdsRendererM3Phase0RangeEmitTicks;
extern volatile u32 gNdsRendererM3Phase0NoZEmitTicks;
extern volatile u32 gNdsRendererM3Phase0NoZMatrixTicks;
extern volatile u32 gNdsRendererM3Phase0AccountingTicks;
extern volatile u32 gNdsRendererM3Phase0CommitTicks;
extern volatile u32 gNdsRendererM3Phase0TimerReadCount;
extern volatile u32 gNdsRendererM3Phase0TimerSpanCount;
extern volatile u32 gNdsRendererM3Phase0CalibrationTicks;
extern volatile u32 gNdsRendererM3Phase0CalibrationIntervals;
extern volatile u32 gNdsRendererM3Phase0PreparedDenseCount;
extern volatile u32 gNdsRendererM3Phase0NearTransformCount;
extern volatile u32 gNdsRendererM3Phase0NoZMatrixCount;
extern volatile u32 gNdsRendererM3ResidualPrepareTicks;
extern volatile u32 gNdsRendererM3ResidualVertexTicks;
extern volatile u32 gNdsRendererM3ResidualNearTicks;
extern volatile u32 gNdsRendererM3ResidualKeyTicks;
extern volatile u32 gNdsRendererM3ResidualKeyHitCount;
extern volatile u32 gNdsRendererM3ResidualKeyMissCount;
extern volatile u32 gNdsRendererM3ResidualKeyByteCount;
extern volatile u32 gNdsRendererM3ResidualRunCount;
extern volatile u32 gNdsRendererM3ResidualDenseCount;
extern volatile u32 gNdsRendererM3ResidualNearCount;
extern volatile u32 gNdsRendererM3G2TextureParamWriteCount;
extern volatile u32 gNdsRendererM3G2TextureParamSkipCount;
extern volatile u32 gNdsRendererM3G2MatrixModeWriteCount;
extern volatile u32 gNdsRendererM3G2MatrixModeSkipCount;
extern volatile u32 gNdsRendererM3G2PolyFmtWriteCount;
extern volatile u32 gNdsRendererM3G2PolyFmtSkipCount;
extern volatile u32 gNdsRendererPhase05WallpaperSetupTicks;
extern volatile u32 gNdsRendererPhase05WallpaperXMapTicks;
extern volatile u32 gNdsRendererPhase05WallpaperYMapTicks;
extern volatile u32 gNdsRendererPhase05WallpaperWriteTicks;
extern volatile u32 gNdsRendererPhase05WallpaperCommitTicks;
extern volatile u32 gNdsRendererPhase05PresentHardwareTicks;
extern volatile u32 gNdsRendererPhase05GCDrawAllTicks;
extern volatile u32 gNdsRendererPhase05StageTransitionTicks;
extern volatile u32 gNdsRendererPhase05FighterWrapperTicks;
extern volatile u32 gNdsRendererPhase05FrameResetTicks;
extern volatile u32 gNdsRendererPhase05PresentTailTicks;
extern volatile u32 gNdsRendererPhase05ProfileBookkeepingTicks;
extern volatile u32 gNdsRendererPhase05ProfilePublishTicks;
extern volatile u32 gNdsRendererPhase05FlushPrepTicks;
extern volatile u32 gNdsRendererPhase05TimerReadCount;
extern volatile u32 gNdsRendererPhase05TimerSpanCount;
extern volatile u32 gNdsRendererPhase05CalibrationTicks;
extern volatile u32 gNdsRendererPhase05CalibrationIntervals;
extern volatile u32 gNdsRendererPhase05WallpaperRowCount;
extern volatile u32 gNdsRendererPhase05WallpaperPixelWriteCount;
extern volatile u32 gNdsRendererPhase05WallpaperFullRowCount;
extern volatile u32 gNdsRendererPhase05WallpaperIncrementalRowCount;
extern volatile u32 gNdsRendererPhase05WallpaperChangedXCount;
extern volatile u32 gNdsRendererPhase05WallpaperChangedRunCount;
extern volatile u32 gNdsRendererPhase05WallpaperLongestChangedRun;
extern volatile u32 gNdsRendererPhase05WallpaperRunGE2Count;
extern volatile u32 gNdsRendererPhase05WallpaperRunGE2Pixels;
extern volatile u32 gNdsRendererPhase05WallpaperRunGE4Count;
extern volatile u32 gNdsRendererPhase05WallpaperRunGE4Pixels;
extern volatile u32 gNdsRendererPhase05WallpaperRunGE8Count;
extern volatile u32 gNdsRendererPhase05WallpaperRunGE8Pixels;
extern volatile u32 gNdsRendererPhase05WallpaperScalarStoreCount;
extern volatile u32 gNdsRendererPhase05WallpaperPackedStoreCount;
extern volatile u32 gNdsRendererPhase05WallpaperDmaPixelCount;
extern volatile u32 gNdsRendererPhase05WallpaperCopyPixelCount;
#endif
#endif
extern volatile u32 gNdsRendererBattleStaticTextureEnabled;
extern volatile u32 gNdsRendererBattleStaticTexturePrepareCount;
extern volatile u32 gNdsRendererBattleStaticTexturePrepareFailCount;
extern volatile u32 gNdsRendererBattleStaticTexturePreparedCount;
extern volatile u32 gNdsRendererBattleStaticTexturePreparedBytes;
extern volatile u32 gNdsRendererBattleStaticTextureArmCount;
extern volatile u32 gNdsRendererBattleStaticTexturePinnedHitCount;
extern volatile u32 gNdsRendererBattleStaticTextureSeenMask;
extern volatile u32 gNdsRendererBattleStaticTextureOwnerMask;
extern volatile u32 gNdsRendererBattleStaticTextureViolationCount;
extern volatile u32 gNdsRendererBattleStaticTextureTeardownCount;
extern volatile u32 gNdsRendererBattleStaticTextureFirstAddress;
extern volatile u32 gNdsRendererBattleStaticTextureEndAddress;
extern volatile u32 gNdsRendererBattleStaticTextureAllocationSpanBytes;
extern volatile u32 gNdsRendererBattleStaticTextureBankMask;
typedef enum NDSRendererBattleTextureFenceClass
{
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_CONVERT = 0,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_PALETTE_DECODE,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_ALLOC,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_FILE_IO,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_GL_CREATE,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_GL_UPLOAD,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_GL_DELETE,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_EVICT,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_REPLACE_REFRESH,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_MANIFEST_FALLBACK,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_COUNT
} NDSRendererBattleTextureFenceClass;
extern volatile u32 gNdsRendererBattleTextureFenceCounts[
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_COUNT];
extern volatile u32 gNdsRendererBattleTextureFenceFirstClassPlus1;
extern volatile u32 gNdsRendererBattleTextureFenceFirstFrame;
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
extern volatile u32 gNdsRendererBenchmarkSinkHashA;
extern volatile u32 gNdsRendererBenchmarkSinkHashB;
extern volatile u32 gNdsRendererBenchmarkSegment0SinkWords;
extern volatile u32 gNdsRendererBenchmarkSegment0SinkHashA;
extern volatile u32 gNdsRendererBenchmarkSegment0SinkHashB;
extern volatile u32 gNdsRendererBenchmarkSegment0SinkArmFaults;
extern u32 gNdsRendererBenchmarkSegment0Trace[3072];
extern volatile u32 gNdsRendererBenchmarkSegment0RunWords[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunHashA[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunHashB[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunRawTextureName[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureEpochPlus1[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureImage[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureTlut[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureKeyHashA[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureKeyHashB[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureDescriptor[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureParams[26];
void ndsRendererBenchmarkSinkEndOwner(NDSRendererProfileOwner owner);
#endif
#endif

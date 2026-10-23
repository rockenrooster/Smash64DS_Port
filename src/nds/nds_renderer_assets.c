/* Native renderer, the generated assets: the traversal's vertex storage, the
 * generated owner, stage and entry-effect packets it includes, and the
 * fighters' high and low detail tables. A slice of nds_renderer.c; its
 * slices follow it there: owner_tables, owner_runtime, native_pools. */
typedef struct NDSRendererTraversalVertexStorage
{
    NDSRendererClipVertex20p12 vertices[NDS_RENDERER_MAX_VTX];
    NDSRendererInputVertex input_vertices[NDS_RENDERER_MAX_VTX];
    u32 vertex_colors[NDS_RENDERER_MAX_VTX];
    u8 vertex_matrix_snapshot[NDS_RENDERER_MAX_VTX];
    u8 vertex_clip_snapshot[NDS_RENDERER_MAX_VTX];
} NDSRendererTraversalVertexStorage;

typedef struct NDSRendererTraversalState
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    NDSRendererMatrix20p12 matrix;
    Mtx matrix_word_raw;
    NDSRendererMatrix20p12 modelview_stack[NDS_RENDERER_MODELVIEW_STACK_SIZE];
    NDSRendererClipVertex20p12 *vertices;
    NDSRendererInputVertex *input_vertices;
    u32 *vertex_colors;
    NDSRendererMatrixSnapshot *matrix_snapshots;
    u8 *vertex_matrix_snapshot;
    u8 *vertex_clip_snapshot;
    const Gfx *source_command_site;
    u32 modelview_valid_stack[NDS_RENDERER_MODELVIEW_STACK_SIZE];
    u32 modelview_stack_depth;
    u32 vertex_valid_mask;
    u32 input_vertex_valid_mask;
    u32 vertex_color_valid_mask;
    u32 current_transform_vertex_mask;
    u32 matrix_generation;
    u32 matrix_snapshot_count;
    u32 current_matrix_snapshot;
    u32 color_modulate;
    NDSRendererHardwareLightDirection prepared_light_direction;
    u32 prepared_light_direction_valid;
    u32 texture_prepare_valid;
    u32 texture_prepare_enabled;
    u32 texture_prepare_name;
    u32 texture_prepare_material_color;
    u32 texture_prepare_scale_s;
    u32 texture_prepare_scale_t;
    u32 texture_prepare_origin_s;
    u32 texture_prepare_origin_t;
    s32 texture_prepare_offset;
    u32 texture_prepare_vertex_flags;
    u32 texture_prepare_source_zbuffered;
    u32 texture_prepare_decal_depth;
    u32 texture_prepare_prim_depth;
    u32 texture_prepare_alpha_constant;
    u32 texture_prepare_poly_alpha;
    u32 texture_prepare_poly_fmt;
    u16 prepared_vertex_colors[NDS_RENDERER_MAX_VTX];
    s16 prepared_texcoord_s[NDS_RENDERER_MAX_VTX];
    s16 prepared_texcoord_t[NDS_RENDERER_MAX_VTX];
    v16 prepared_projected_x[NDS_RENDERER_MAX_VTX];
    v16 prepared_projected_y[NDS_RENDERER_MAX_VTX];
    v16 prepared_projected_source_z[NDS_RENDERER_MAX_VTX];
    u32 prepared_vertex_color_valid_mask;
    u32 prepared_texcoord_valid_mask;
    u32 prepared_projected_xy_valid_mask;
    u32 prepared_projected_source_z_valid_mask;
    u32 projection_valid;
    u32 modelview_valid;
    u32 matrix_valid;
    u32 matrix_word_valid;
    u32 raw_vertex_fit_mask;
} NDSRendererTraversalState;

#define NDS_NATIVE_STATE_OTHERMODE 2u
#define NDS_NATIVE_STATE_COMBINE 3u
#define NDS_NATIVE_STATE_TEXTURE 4u
#define NDS_NATIVE_STATE_GEOMETRY 5u
#define NDS_NATIVE_STATE_IMAGE 6u
#define NDS_NATIVE_STATE_TILE 7u
#define NDS_NATIVE_STATE_LOAD_TLUT 8u
#define NDS_NATIVE_STATE_LOAD_BLOCK 9u
#define NDS_NATIVE_STATE_TILE_SIZE 10u
#define NDS_NATIVE_STATE_PRIM 11u
#define NDS_NATIVE_STATE_BLEND 12u
#define NDS_NATIVE_STATE_MATERIAL 13u
#define NDS_NATIVE_STATE_LIGHT_COLOR 14u
/* Stage-generator ids past the shared range (generate_nds_native_stage.py
 * STATE_EFFECT_*): the stage span applier translates them before the shared
 * applier sees them, because the fighter generator already spends 14 on
 * LIGHT_COLOR (a G_MOVEWORD). ENVCOLOR has no DS combiner input and the
 * generic walk ignores G_SETENVCOLOR too; MOVEWORD is the same applier as
 * LIGHT_COLOR; LOADTILE records the load exactly as the DL core does. */
#define NDS_NATIVE_STATE_STAGE_ENVCOLOR 14u
#define NDS_NATIVE_STATE_STAGE_MOVEWORD 16u
#define NDS_NATIVE_STATE_LOAD_TILE 20u
#define NDS_NATIVE_STATE_NONE 0xffffu
#define NDS_NATIVE_MATERIAL_NONE 0xffu
#define NDS_NATIVE_VERTEX_BLOCK 0u
#define NDS_NATIVE_MODIFY_ST 1u
#define NDS_NATIVE_RUN_RAW_CURRENT 0u
#define NDS_NATIVE_RUN_CROSS_MATRIX 1u
#define NDS_NATIVE_ROOT_BINDING_COUNT 32u
#define NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK 0x07u
#define NDS_NATIVE_DIRECT_POLICY_TEXTURED_LIT 0u
#define NDS_NATIVE_DIRECT_POLICY_LIT_ONLY 2u
#define NDS_NATIVE_DIRECT_POLICY_TEXTURED_PASS2 4u
#define NDS_NATIVE_DIRECT_POLICY_LIT_PASS2 5u
#define NDS_NATIVE_DIRECT_POLICY_CULL_NONE 0x80u
#define NDS_NATIVE_DENSE_ID_MASK 0x07ffu
#define NDS_NATIVE_PACKED_CORNER_MATRIX_SHIFT 11u
/* An owner with 2048 dense vertices or more (a P4 donor carrying every model
 * part its motions draw: Banjo, Crash) has wide dense ids: a span's u16 is its
 * whole first id and its count is the action's own (a vertex block's count,
 * else one), and a raw run's corner is a whole id. A cross run's corner keeps
 * the 11-bit id beside its GX slot; the generator keeps those ids below 2048
 * (generate_nds_native_owners.py build_direct_dense_tables). */
#define NDS_NATIVE_DENSE_WIDE(tables_)                                         \
    ((tables_)->dense_count > NDS_NATIVE_DENSE_ID_MASK)
#define NDS_NATIVE_SPAN_FIRST(wide_, span_)                                    \
    ((wide_) ? (u32)(span_) : ((u32)(span_) & NDS_NATIVE_DENSE_ID_MASK))
/* P2-3 image ABI tag: first u32 of every native-owner image, checked on load
 * before any member is bound. v6 gives NDSNativeRun.submit_class upper bits
 * the source-unlit vertex-colour flag plus per-run polygon alpha while keeping
 * the row at 8 bytes; dense_normals stores FIFO_COLOR words for those flagged
 * runs. v5 adds foreign IMAGE asset provenance. v4 adds
 * derived run-first-corner addressing and
 * conditionally omits the source-order packed-corner stream when the Task56
 * primitive stream is the only production consumer.
 * v2 11-bit packed-corner ABI (dense 0..10, GX
 * slot 11..15; spans first 0..10, count 11..15). The untagged v1 (10-bit)
 * images are also one word shorter, so stale payloads already fail the exact
 * size read; the tag is the explicit second lock, and the only discriminator
 * for same-size payloads. Top byte 0x36 is outside BattleShip's RDP opcode
 * space, so no v1 state word can alias it. The generated image header
 * re-emits the same value once images are regenerated; the guard keeps both
 * orders compiling. */
#ifndef NDS_NATIVE_OWNER_IMAGE_ABI_TAG
#define NDS_NATIVE_OWNER_IMAGE_ABI_TAG 0x364f444eu
#endif
#define NDS_NATIVE_GX_MATRIX_CURRENT 31u
#define NDS_NATIVE_GX_MATRIX_SLOT_MAX 30u
#define NDS_NATIVE_SOURCE_GEOM_CULL_FRONT 0x00001000u
#define NDS_NATIVE_SOURCE_GEOM_CULL_BACK 0x00002000u
/* P2-3r4: the ungated table ELEMENT types moved to a shared header so the
 * generated owner images and this renderer cannot disagree about their
 * layout. `NDSNativePreparedDenseVertex` stays defined below: it is
 * build-gated draw scratch, and the generated image header carries a
 * byte-identical fallback copy for standalone image TUs. The define below
 * tells the header this translation unit provides its own, so the two can
 * never collide here. */
#include <nds/nds_native_fighter_tables.h>

/* The arena the image buffers come from; the renderer does not otherwise
 * allocate, so the declaration arrives with the feature that needs it. */
extern void *syTaskmanMalloc(size_t size, u32 align);
#define NDS_NATIVE_PREPARED_DENSE_DEFINED_BY_RENDERER 1
/* The image types AND the owner-slot ids: one generated ABI, so the
 * renderer and the fighter manager cannot number owners differently. */


/* R2-03 E29. Under NDS_R2_FIGHTER_HW_LIGHT the geometry engine lights the
 * fighter: all four emit paths write GFX_NORMAL from
 * sNdsNativeFighterDenseNormals and never read packed_color, and every epoch is
 * lit (gNdsR2ShadeLitEpochs == gNdsR2ExecEpochCalls == 22,296 over 480 frames),
 * so the per-dense-vertex loop that writes these two fields never runs. They are
 * dead weight in the middle of the struct the hot emit loop reads, and this
 * table is randomly indexed by 1,878 corners a frame against a 4 KB dcache --
 * 16 bytes x 541 is 8,656, more than twice the cache. The hardware-lit layout
 * also omits alignment padding: GCC emits halfword loads for its ten-byte
 * stride, preserving the two full-width GX position writes. */
typedef struct NDSNativePreparedDenseVertex
{
    u32 gx_xy;
    u16 gx_z;
    s16 s;
    s16 t;
} __attribute__((packed, aligned(2))) NDSNativePreparedDenseVertex;


typedef struct NDSNativeRoot
{
    u32 root_offset;
    u16 first_epoch;
    u16 tail_state_first;
    u16 source_command_count;
    u8 epoch_count;
    u8 tail_state_count;
    u8 tail_sync_count;
    u8 light_preamble;
} NDSNativeRoot;

/* Image members embed PreparedDense, so its complete type must precede this
 * header. Standalone image TUs use the generator's matching definition. */
#include <nds/generated/nds_native_fighter_image.generated.h>
#include <nds/nds_p4_native.h>

/* Passive fighter model parts replace one live DObj display list while
 * keeping that joint's matrix binding.  Generated variants therefore carry
 * the logical canonical binding next to a complete native root descriptor;
 * their executable epochs/runs/vertices live in the same owner table image. */
typedef struct NDSNativeRootVariant
{
    u32 binding;
    NDSNativeRoot root;
} NDSNativeRootVariant;

typedef struct NDSNativeDirectPolicy
{
    u32 combine_w0;
    u32 combine_w1;
    u8 vertex_flags;
    u8 textured;
    u8 reserved[2];
} NDSNativeDirectPolicy;

_Static_assert(sizeof(NDSNativeStateDelta) == 12u,
               "native state effect ABI must stay compact");
_Static_assert(sizeof(NDSNativeDenseVertex) == 12u,
               "native dense vertex ABI must stay cache-line friendly");
/* R2-03 E29. 16 bytes bought exactly two per 32-byte cache line and no
 * straddling; 12 bytes buys a smaller table (6,492 against 8,656, both over the
 * 4 KB dcache) at the cost of one access in three spanning two lines. Which wins
 * is measured, not assumed -- see the E29 write-up. */
_Static_assert(sizeof(NDSNativePreparedDenseVertex) == 10u,
               "hardware-lit prepared vertices must use ten bytes");
_Static_assert(sizeof(NDSNativeRun) == 8u,
               "native run ABI must stay compact");
_Static_assert(sizeof(NDSNativeEpoch) == 16u,
               "native epoch ABI must stay compact");
_Static_assert(sizeof(NDSNativeRoot) == 16u,
               "native root ABI must stay compact");
_Static_assert(sizeof(NDSNativeRootVariant) == 20u,
               "native root variant ABI must stay compact");
_Static_assert(sizeof(NDSNativeDirectPolicy) == 12u,
               "native direct policy ABI must stay compact");
#include "nds_native_fighter_owner.generated.inc"
#include "nds_p4_native_owner.generated.inc"
/* P4 S8: Remix's Kirby hats (scripts/p4/p4_kirby_hats.py): their image types,
 * and their trio programs with Kirby's head list regenerated around them. */
#include "kirby/hats/nds_p4_kirby_hats.generated.h"
#include "kirby/hats/nds_p4_kirby_hats.generated.inc"
#if !defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
#error "Kirby owner images require the generated Kirby root programs"
#endif
#include "nds_native_stage_owner.generated.inc"
/* Blob residency: a stage whose packet loads from NitroFS links only its
 * registry row, not these tables. Each default is 0 (blob) except Dream
 * Land, which has no LINKED macro and stays linked; pass
 * -DNDS_NATIVE_STAGE_LINKED_<STAGE>=1 to keep a stage linked. */
#ifndef NDS_NATIVE_STAGE_LINKED_YOSTER
#define NDS_NATIVE_STAGE_LINKED_YOSTER 0
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_JUNGLE
#define NDS_NATIVE_STAGE_LINKED_JUNGLE 0
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_CASTLE
#define NDS_NATIVE_STAGE_LINKED_CASTLE 0
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_SECTOR
#define NDS_NATIVE_STAGE_LINKED_SECTOR 0
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_HYRULE
#define NDS_NATIVE_STAGE_LINKED_HYRULE 0
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_INISHIE
#define NDS_NATIVE_STAGE_LINKED_INISHIE 0
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_ZEBES
#define NDS_NATIVE_STAGE_LINKED_ZEBES 0
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_YAMABUKI
#define NDS_NATIVE_STAGE_LINKED_YAMABUKI 0
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_PUPUPUSMALL
#define NDS_NATIVE_STAGE_LINKED_PUPUPUSMALL 0
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_YOSTERSMALL
#define NDS_NATIVE_STAGE_LINKED_YOSTERSMALL 0
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_METAL
#define NDS_NATIVE_STAGE_LINKED_METAL 0
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_ZAKO
#define NDS_NATIVE_STAGE_LINKED_ZAKO 0
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_LAST
#define NDS_NATIVE_STAGE_LINKED_LAST 0
#endif
#if defined(NDS_P2_STAGE_YOSTER) && (NDS_P2_STAGE_YOSTER == 1) && (NDS_NATIVE_STAGE_LINKED_YOSTER == 1)
#include "nds_native_stage_yoster.generated.inc"
#endif
#if defined(NDS_P2_STAGE_JUNGLE) && (NDS_P2_STAGE_JUNGLE == 1) && (NDS_NATIVE_STAGE_LINKED_JUNGLE == 1)
#include "nds_native_stage_jungle.generated.inc"
#endif
#if defined(NDS_P2_STAGE_CASTLE) && (NDS_P2_STAGE_CASTLE == 1) && (NDS_NATIVE_STAGE_LINKED_CASTLE == 1)
#include "nds_native_stage_castle.generated.inc"
#endif
#if defined(NDS_P2_STAGE_SECTOR) && (NDS_P2_STAGE_SECTOR == 1) && (NDS_NATIVE_STAGE_LINKED_SECTOR == 1)
#include "nds_native_stage_sector.generated.inc"
#endif
#if defined(NDS_P2_STAGE_HYRULE) && (NDS_P2_STAGE_HYRULE == 1) && (NDS_NATIVE_STAGE_LINKED_HYRULE == 1)
#include "nds_native_stage_hyrule.generated.inc"
#endif
#if defined(NDS_P2_STAGE_INISHIE) && (NDS_P2_STAGE_INISHIE == 1) && (NDS_NATIVE_STAGE_LINKED_INISHIE == 1)
#include "nds_native_stage_inishie.generated.inc"
#endif
#if defined(NDS_P2_STAGE_ZEBES) && (NDS_P2_STAGE_ZEBES == 1) && (NDS_NATIVE_STAGE_LINKED_ZEBES == 1)
#include "nds_native_stage_zebes.generated.inc"
#endif
#if defined(NDS_P2_STAGE_YAMABUKI) && (NDS_P2_STAGE_YAMABUKI == 1) && (NDS_NATIVE_STAGE_LINKED_YAMABUKI == 1)
#include "nds_native_stage_yamabuki.generated.inc"
#endif
#if defined(NDS_P2_STAGE_PUPUPUSMALL) && (NDS_P2_STAGE_PUPUPUSMALL == 1) && (NDS_NATIVE_STAGE_LINKED_PUPUPUSMALL == 1)
#include "nds_native_stage_pupupusmall.generated.inc"
#endif
#if defined(NDS_P2_STAGE_YOSTERSMALL) && (NDS_P2_STAGE_YOSTERSMALL == 1) && (NDS_NATIVE_STAGE_LINKED_YOSTERSMALL == 1)
#include "nds_native_stage_yostersmall.generated.inc"
#endif
#if defined(NDS_P2_STAGE_METAL) && (NDS_P2_STAGE_METAL == 1) && (NDS_NATIVE_STAGE_LINKED_METAL == 1)
#include "nds_native_stage_metal.generated.inc"
#endif
#if defined(NDS_P2_STAGE_ZAKO) && (NDS_P2_STAGE_ZAKO == 1) && (NDS_NATIVE_STAGE_LINKED_ZAKO == 1)
#include "nds_native_stage_zako.generated.inc"
#endif
#if defined(NDS_P2_STAGE_LAST) && (NDS_P2_STAGE_LAST == 1) && (NDS_NATIVE_STAGE_LINKED_LAST == 1)
#include "nds_native_stage_last.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS3
#define NDS_NATIVE_STAGE_LINKED_BONUS3 0
#endif
#if defined(NDS_P2_STAGE_BONUS3) && (NDS_P2_STAGE_BONUS3 == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS3 == 1)
#include "nds_native_stage_bonus3.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS1_MARIO
#define NDS_NATIVE_STAGE_LINKED_BONUS1_MARIO 0
#endif
#if defined(NDS_P2_STAGE_BONUS1_MARIO) && (NDS_P2_STAGE_BONUS1_MARIO == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS1_MARIO == 1)
#include "nds_native_stage_bonus1_mario.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS1_FOX
#define NDS_NATIVE_STAGE_LINKED_BONUS1_FOX 0
#endif
#if defined(NDS_P2_STAGE_BONUS1_FOX) && (NDS_P2_STAGE_BONUS1_FOX == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS1_FOX == 1)
#include "nds_native_stage_bonus1_fox.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS1_DONKEY
#define NDS_NATIVE_STAGE_LINKED_BONUS1_DONKEY 0
#endif
#if defined(NDS_P2_STAGE_BONUS1_DONKEY) && (NDS_P2_STAGE_BONUS1_DONKEY == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS1_DONKEY == 1)
#include "nds_native_stage_bonus1_donkey.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS1_SAMUS
#define NDS_NATIVE_STAGE_LINKED_BONUS1_SAMUS 0
#endif
#if defined(NDS_P2_STAGE_BONUS1_SAMUS) && (NDS_P2_STAGE_BONUS1_SAMUS == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS1_SAMUS == 1)
#include "nds_native_stage_bonus1_samus.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS1_LUIGI
#define NDS_NATIVE_STAGE_LINKED_BONUS1_LUIGI 0
#endif
#if defined(NDS_P2_STAGE_BONUS1_LUIGI) && (NDS_P2_STAGE_BONUS1_LUIGI == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS1_LUIGI == 1)
#include "nds_native_stage_bonus1_luigi.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS1_LINK
#define NDS_NATIVE_STAGE_LINKED_BONUS1_LINK 0
#endif
#if defined(NDS_P2_STAGE_BONUS1_LINK) && (NDS_P2_STAGE_BONUS1_LINK == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS1_LINK == 1)
#include "nds_native_stage_bonus1_link.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS1_YOSHI
#define NDS_NATIVE_STAGE_LINKED_BONUS1_YOSHI 0
#endif
#if defined(NDS_P2_STAGE_BONUS1_YOSHI) && (NDS_P2_STAGE_BONUS1_YOSHI == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS1_YOSHI == 1)
#include "nds_native_stage_bonus1_yoshi.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS1_CAPTAIN
#define NDS_NATIVE_STAGE_LINKED_BONUS1_CAPTAIN 0
#endif
#if defined(NDS_P2_STAGE_BONUS1_CAPTAIN) && (NDS_P2_STAGE_BONUS1_CAPTAIN == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS1_CAPTAIN == 1)
#include "nds_native_stage_bonus1_captain.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS1_KIRBY
#define NDS_NATIVE_STAGE_LINKED_BONUS1_KIRBY 0
#endif
#if defined(NDS_P2_STAGE_BONUS1_KIRBY) && (NDS_P2_STAGE_BONUS1_KIRBY == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS1_KIRBY == 1)
#include "nds_native_stage_bonus1_kirby.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS1_PIKACHU
#define NDS_NATIVE_STAGE_LINKED_BONUS1_PIKACHU 0
#endif
#if defined(NDS_P2_STAGE_BONUS1_PIKACHU) && (NDS_P2_STAGE_BONUS1_PIKACHU == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS1_PIKACHU == 1)
#include "nds_native_stage_bonus1_pikachu.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS1_PURIN
#define NDS_NATIVE_STAGE_LINKED_BONUS1_PURIN 0
#endif
#if defined(NDS_P2_STAGE_BONUS1_PURIN) && (NDS_P2_STAGE_BONUS1_PURIN == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS1_PURIN == 1)
#include "nds_native_stage_bonus1_purin.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS1_NESS
#define NDS_NATIVE_STAGE_LINKED_BONUS1_NESS 0
#endif
#if defined(NDS_P2_STAGE_BONUS1_NESS) && (NDS_P2_STAGE_BONUS1_NESS == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS1_NESS == 1)
#include "nds_native_stage_bonus1_ness.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS2_MARIO
#define NDS_NATIVE_STAGE_LINKED_BONUS2_MARIO 0
#endif
#if defined(NDS_P2_STAGE_BONUS2_MARIO) && (NDS_P2_STAGE_BONUS2_MARIO == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS2_MARIO == 1)
#include "nds_native_stage_bonus2_mario.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS2_FOX
#define NDS_NATIVE_STAGE_LINKED_BONUS2_FOX 0
#endif
#if defined(NDS_P2_STAGE_BONUS2_FOX) && (NDS_P2_STAGE_BONUS2_FOX == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS2_FOX == 1)
#include "nds_native_stage_bonus2_fox.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS2_DONKEY
#define NDS_NATIVE_STAGE_LINKED_BONUS2_DONKEY 0
#endif
#if defined(NDS_P2_STAGE_BONUS2_DONKEY) && (NDS_P2_STAGE_BONUS2_DONKEY == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS2_DONKEY == 1)
#include "nds_native_stage_bonus2_donkey.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS2_SAMUS
#define NDS_NATIVE_STAGE_LINKED_BONUS2_SAMUS 0
#endif
#if defined(NDS_P2_STAGE_BONUS2_SAMUS) && (NDS_P2_STAGE_BONUS2_SAMUS == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS2_SAMUS == 1)
#include "nds_native_stage_bonus2_samus.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS2_LUIGI
#define NDS_NATIVE_STAGE_LINKED_BONUS2_LUIGI 0
#endif
#if defined(NDS_P2_STAGE_BONUS2_LUIGI) && (NDS_P2_STAGE_BONUS2_LUIGI == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS2_LUIGI == 1)
#include "nds_native_stage_bonus2_luigi.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS2_LINK
#define NDS_NATIVE_STAGE_LINKED_BONUS2_LINK 0
#endif
#if defined(NDS_P2_STAGE_BONUS2_LINK) && (NDS_P2_STAGE_BONUS2_LINK == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS2_LINK == 1)
#include "nds_native_stage_bonus2_link.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS2_YOSHI
#define NDS_NATIVE_STAGE_LINKED_BONUS2_YOSHI 0
#endif
#if defined(NDS_P2_STAGE_BONUS2_YOSHI) && (NDS_P2_STAGE_BONUS2_YOSHI == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS2_YOSHI == 1)
#include "nds_native_stage_bonus2_yoshi.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS2_CAPTAIN
#define NDS_NATIVE_STAGE_LINKED_BONUS2_CAPTAIN 0
#endif
#if defined(NDS_P2_STAGE_BONUS2_CAPTAIN) && (NDS_P2_STAGE_BONUS2_CAPTAIN == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS2_CAPTAIN == 1)
#include "nds_native_stage_bonus2_captain.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS2_KIRBY
#define NDS_NATIVE_STAGE_LINKED_BONUS2_KIRBY 0
#endif
#if defined(NDS_P2_STAGE_BONUS2_KIRBY) && (NDS_P2_STAGE_BONUS2_KIRBY == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS2_KIRBY == 1)
#include "nds_native_stage_bonus2_kirby.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS2_PIKACHU
#define NDS_NATIVE_STAGE_LINKED_BONUS2_PIKACHU 0
#endif
#if defined(NDS_P2_STAGE_BONUS2_PIKACHU) && (NDS_P2_STAGE_BONUS2_PIKACHU == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS2_PIKACHU == 1)
#include "nds_native_stage_bonus2_pikachu.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS2_PURIN
#define NDS_NATIVE_STAGE_LINKED_BONUS2_PURIN 0
#endif
#if defined(NDS_P2_STAGE_BONUS2_PURIN) && (NDS_P2_STAGE_BONUS2_PURIN == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS2_PURIN == 1)
#include "nds_native_stage_bonus2_purin.generated.inc"
#endif
#ifndef NDS_NATIVE_STAGE_LINKED_BONUS2_NESS
#define NDS_NATIVE_STAGE_LINKED_BONUS2_NESS 0
#endif
#if defined(NDS_P2_STAGE_BONUS2_NESS) && (NDS_P2_STAGE_BONUS2_NESS == 1) && (NDS_NATIVE_STAGE_LINKED_BONUS2_NESS == 1)
#include "nds_native_stage_bonus2_ness.generated.inc"
#endif
/* Generated blob maxima (scripts/stages/emit_native_stage_runtime_rows.py
 * --maxima, staged as generated/nds_native_stage_blob_maxima.generated.h).
 * Absent
 * until the first blob registration stages it; the select.inc floor treats
 * absence as "no blob stages yet". */
#if defined(__has_include)
#if __has_include("generated/nds_native_stage_blob_maxima.generated.h")
#include "generated/nds_native_stage_blob_maxima.generated.h"
#endif
#endif
#if NDS_P5
/* P5: the Remix stages' packets (scripts/p5/stage_generate.py) raise the
 * blob maxima the workspaces below are sized by, as a vanilla stage's do. */
#include <nds/nds_p5.h>
#include "nds_p5_native_stages.generated.h"
#endif
/* Must follow every generated packet: it names their tables. */
#include "nds_native_stage_select.inc"
#include "dreamland_ds_mesh.generated.inc"

/* P2-2: BattleShip switches every VS fighter to the Low JointTree when three
 * or more fighters are present (scvsbattle.c:188/:460).  The generated owner
 * therefore has two immutable programs with the same ABI but different roots
 * and cardinalities.  Keep one file-scope view selected once per owner execute;
 * draw callbacks are serialized, so the hot inner loops can consume the view
 * without carrying a detail argument through every call.
 *
 * This is deliberately a view over generated constants, not a second runtime
 * representation.  High-detail remains the default for the legacy/root and
 * hierarchy entry points; mode 9 selects the view from FTStruct::detail_curr. */
typedef struct NDSNativeFighterRuntimeTables
{
    const NDSNativeStateDelta *state_deltas;
    u32 state_delta_count;
    const u8 *state_sequence;
    u32 state_sequence_count;
    const NDSNativeVertexAction *vertex_actions;
    u32 vertex_action_count;
    const u8 *epoch_direct_policy;
    const NDSNativeDenseVertex *dense_vertices;
    u32 dense_count;
    /* P2-3f49: precomputed GFX_NORMAL words, one per dense vertex. Imaged
     * owners bind this into the loaded NitroFS image (resident before first
     * draw via EnsureOwnerImage); Mario/Fox keep their baked static arrays. */
    const u32 *dense_normals;
    NDSNativePreparedDenseVertex *prepared_dense;
    const u16 *action_dense_spans;
    const u16 *packed_corners;
    u32 packed_corner_count;
    const u16 *run_first_corner;
    u32 run_first_corner_count;
    const u16 *run_first_unique;
    const u8 *run_unique_count;
    const u16 *run_unique_dense;
    const u16 *triangles;
    u32 triangle_count;
    const NDSNativeRun *runs;
    u32 run_count;
    const u16 *primitive_group_first;
    const u8 *primitive_group_count;
    const u8 *primitive_group_type;
    const u16 *primitive_group_first_vertex;
    const u8 *primitive_group_vertex_count;
    const u16 *primitive_vertices;
    const NDSNativeEpoch *epochs;
    u32 epoch_count;
} NDSNativeFighterRuntimeTables;

/* Mario's pipe and Fox's Arwing are short-lived source DObj trees, but their
 * mesh/display-list/texture payloads are immutable.  Keep BattleShip's live
 * DObj animation (that is what makes Mario rise from the pipe and Fox leave the
 * ship) and replace only the expensive presentation half with an AOT packet.
 * The generated vertices already contain final DS t16 texcoords and every
 * texture is already PAL16/A5I3/RGBA, so this owner never decodes an N64 display list
 * or converts an N64 texture at runtime.
 *
 * geometry_mode/geometry_clear are the bits the list set and cleared; a bit
 * in neither is inherited from the battle display's state at submit. Both
 * models inherit G_LIGHTING that way, so a SHADE-combined group's vertex
 * r,g,b are unit normals. light_color_1/2 (valid per light_mask bit 0/1)
 * are the lists' own gSPLightColor words; the direction is the battle's. */
typedef struct NDSEntryEffectGroup
{
    u16 first_vertex;
    u16 triangle_count;
    /* Sparse RSP load-time-matrix provenance.  Almost every corner was loaded
     * under this group's root; only Mario's pipe body reuses earlier cache
     * slots.  These two fields occupy padding that the old 56-byte row already
     * paid, so source-correct matrix lifetime no longer needs one byte/corner. */
    u16 matrix_override_first;
    u8 texture_slot;
    u8 root_index;
    u8 geometry_state;
    u8 combine_state;
    u8 othermode_state;
    u8 prim_color_index;
    u8 env_color_index;
    u8 light_state;
    u8 cms;
    u8 cmt;
    u8 masks;
    u8 maskt;
    u8 matrix_override_count;
    /* Source segment-0xE material branch selected immediately before this
     * geometry group. 0xFF means the immutable list had no dynamic MObj
     * branch. Link's grounded Spin weapon is the first owner to use this: its
     * LinkModel root selects nine live PRIM-only MObjs by 8-byte branch slot. */
    u8 material_slot;
} NDSEntryEffectGroup;

typedef struct NDSEntryEffectPosition
{
    s16 x;
    s16 y;
    s16 z;
} NDSEntryEffectPosition;

typedef struct NDSEntryEffectColor
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
} NDSEntryEffectColor;

typedef struct NDSEntryEffectPairState
{
    u32 a;
    u32 b;
} NDSEntryEffectPairState;

typedef struct NDSEntryEffectRoot
{
    u32 source_offset;
    u16 first_group;
    u8 group_count;
    u8 flags; /* NDS_ENTRY_ROOT_* */
} NDSEntryEffectRoot;

/* The root draws with a polygon ID of its own rather than the seeded 1: it
 * sits inside another translucent root (Link's entry wave in his beam), and
 * the DS draws no translucent pixel over one of the same ID. */
#define NDS_ENTRY_ROOT_OWN_POLY_ID 0x01u
#define NDS_ENTRY_ROOT_OWN_POLY_ID_VALUE 60u

typedef struct NDSEntryEffectTexture
{
    const u8 *texels;
    /* Stored byte count. For LZ10 this is the encoded count; decoded size
     * is derived exactly from width/height/ds_format by the setup-time fill. */
    u32 texel_bytes;
    const u16 *palette;
    u16 palette_entries;
    u16 width;
    u16 height;
    u8 ds_format;
    /* DS PAL16 has no per-palette-entry alpha.  Index 0 becomes transparent
     * only when TEXIMAGE_PARAM.COLOR0 is enabled, so this is part of the
     * converted asset, not a property of the format.  The generator derives it
     * from the canonical RGBA5551 image: transparent sources are normalized to
     * palette[0] == 0, while opaque images may use index 0 as a real colour. */
    u8 color0_transparent;
    u8 compression;
} NDSEntryEffectTexture;

#define NDS_ENTRY_EFFECT_TEXTURE_PAL16 0u
#define NDS_ENTRY_EFFECT_TEXTURE_A5I3 1u
#define NDS_ENTRY_EFFECT_TEXTURE_RGBA 2u
/* Appended, never renumbered: the generated rows already encode 0/1/2.  Only
 * the shield uses this -- 32 palette entries against A5I3's 8, because its
 * banding channel is the colour ramp and its source alpha is flat. */
#define NDS_ENTRY_EFFECT_TEXTURE_A3I5 3u
#define NDS_ENTRY_EFFECT_TEXTURE_NONE 0xffu
#define NDS_ENTRY_EFFECT_COMPRESSION_RAW 0u
#define NDS_ENTRY_EFFECT_COMPRESSION_LZ10 1u
/* P4 builds: a P4 article texture's raw texels in nitro:/p4/entry_texels.bin;
 * texels holds 1 + the byte offset (generate_nds_entry_effects.py --p4). */
#define NDS_ENTRY_EFFECT_COMPRESSION_FILE 2u

/* P4 builds: the same packet with every enabled content's article roots
 * appended (scripts/p4/p4_articles.py, generated into the build's p4
 * directory by generate_nds_entry_effects.py --p4). */
#include "nds_p4_entry_effects.generated.inc"
/* The texcoord dictionaries' corner index: u8 unless a packet outgrew it. */
#ifndef NDS_ENTRY_EFFECT_UV_INDEX_T
#define NDS_ENTRY_EFFECT_UV_INDEX_T u8
#endif

/* The source RSP vertex cache is transformed at gSPVertex load time, not at
 * triangle emission time. Mario's pipe body reuses six cache slots loaded by
 * the preceding rim DObj and changes only their ST with gSPModifyVertex; 18
 * emitted body corners therefore belong to the RIM matrix while the other
 * corners belong to the BODY matrix. The AOT generator records that load-root
 * per corner. Keep each root's live modelview/composed matrix so mixed-cache
 * groups can use the generic renderer's CPU-projected cross-matrix contract. */
_Static_assert(NDS_ENTRY_EFFECT_CROSS_MATRIX_CORNER_COUNT != 0u,
               "entry-effect matrix provenance unexpectedly became trivial");

static u32 sNdsRendererEntryEffectTextureName[NDS_ENTRY_EFFECT_TEXTURE_COUNT];
/* efmanager.c:dEFManagerShieldColors. Each preconverted texture has its own
 * palette so four simultaneous shields never mutate a queued draw's colors. */
static const u32 sNdsEntryShieldEnvironment[5] = {
    0xff0000c0u, 0x00ff00c0u, 0x0000ffc0u, 0x000000c0u, 0xc0c0c0c0u
};
static u32 sNdsEntryShieldTextureName[5];
/* KO MatAnimJoint changes alpha only. RGB endpoints are source constants:
 * EFCommonEffects2 player scripts and efmanager.c's child/sibling env tables. */
static const u32 sNdsEntryKoRootOffsets[3] = {0x5218u, 0x52b0u, 0x5310u};
static const u32 sNdsEntryKoPrimRgb[3][4] = {
    {0xffffbb00u, 0xc3ffcb00u, 0xcdffff00u, 0xffd6ff00u},
    {0xffffff00u, 0xffffff00u, 0xffffff00u, 0xffffff00u},
    {0xffb43d00u, 0xa16dff00u, 0x60ff0000u, 0x4bf0ff00u}
};
static const u32 sNdsEntryKoEnvRgb[3][4] = {
    {0xa6622100u, 0x1fffa100u, 0x3e6dff00u, 0xfb66c700u},
    {0xfcf69000u, 0xfcf69000u, 0xfcf69000u, 0xfcf69000u},
    {0xff624b00u, 0x007eff00u, 0xffff0000u, 0x00ff0000u}
};
static u32 sNdsEntryKoPaletteName[3][4];
static u16 sNdsEntryKoPaletteScratch[8] __attribute__((aligned(32)));
/* A root's modelview and composed matrices, by matrix slot
 * (ndsRendererEntryEffectMatrixSlot): slot 0 is the root being drawn, slots
 * 1.. the override sources, whose matrices later roots' corners read and so
 * keep theirs across draws. Every root kept its own (205 roots with the P4
 * contents' articles: 26 KB of .bss) though only the sources' are read after
 * their draw. */
#define NDS_ENTRY_EFFECT_MATRIX_SLOTS 16u
static NDSRendererMatrix20p12
    sNdsRendererEntryEffectModelview[NDS_ENTRY_EFFECT_MATRIX_SLOTS];
static NDSRendererMatrix20p12
    sNdsRendererEntryEffectComposed[NDS_ENTRY_EFFECT_MATRIX_SLOTS];
#define NDS_ENTRY_EFFECT_MASK_WORDS ((NDS_ENTRY_EFFECT_ROOT_COUNT + 31u) / 32u)
_Static_assert(NDS_ENTRY_EFFECT_ROOT_COUNT <= 256u,
               "entry root indices must fit their generated u8 fields");
static u32 sNdsRendererEntryEffectModelviewValidMask[NDS_ENTRY_EFFECT_MASK_WORDS];
volatile u32 gNdsEntryEffectNativeDrawCount;
volatile u32 gNdsEntryEffectNativeFallbackCount;
volatile u32 gNdsFalconKickNativeSubmitCount;
volatile u32 gNdsFalconPunchNativeSubmitCount;
/* Poke Ball entry rays, plus the shared skipped-draw witness. Defined
 * unconditionally like every other owner row so a gc-sections pass cannot drop
 * a witness the harness reads. */
volatile u32 gNdsEntryEffectNativeAlphaSkipCount;
volatile u32 gNdsMBallRaysCandidateCount;
volatile u32 gNdsMBallRaysMaterialRejectCount;
volatile u32 gNdsEntryEffectNativeTexturePrepareCount;
volatile u32 gNdsEntryEffectNativeShieldPrepareCount;
volatile u32 gNdsEntryEffectNativeTextureBindCount;
/* P2-3 (owner: "the Mario intro green tube still doesn't render the full pipe,
 * I just see the rim"). The aggregate draw count cannot tell a rim from a
 * body: Mario's pipe is TWO roots -- 0x03c0 is the 12-triangle ring and
 * 0x04c0 the 32-triangle, 120-tall barrel -- and Fox's Arwing is eight more.
 * Count per root so a missing piece names itself. */
volatile u32 gNdsEntryEffectNativeRootDraws[NDS_ENTRY_EFFECT_ROOT_COUNT];

typedef struct NDSNativeFighterOwnerRuntime
{
    const NDSNativeFighterRuntimeTables *tables;
    const NDSNativeRoot *roots;
    u32 root_count;
    const u8 *cross_palette_slots;
    const u32 (*root_light_preambles)[2];
    u32 root_light_preamble_count;
    u32 asset_data_size;
} NDSNativeFighterOwnerRuntime;

#define NDS_NATIVE_FIGHTER_OWNER_COUNT \
    NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT

#define NDS_FTR_COUNT(a) ((u32)(sizeof(a) / sizeof((a)[0])))

static const NDSNativeFighterRuntimeTables sNdsNativeFighterHighTables =
{
    sNdsNativeFighterStateDeltas, NDS_FTR_COUNT(sNdsNativeFighterStateDeltas),
    sNdsNativeFighterStateSequence, NDS_FTR_COUNT(sNdsNativeFighterStateSequence),
    sNdsNativeFighterVertexActions, NDS_FTR_COUNT(sNdsNativeFighterVertexActions),
    sNdsNativeFighterEpochDirectPolicy,
    sNdsNativeFighterDenseVertices, NDS_FTR_COUNT(sNdsNativeFighterDenseVertices),
    /* P2-3f49: NULL for Mario/Fox on purpose. This field exists so an imaged
     * owner can hand the emit a table that arrived inside its NitroFS image;
     * Mario/Fox have no image, still bake at load, and reach their words
     * through sNdsNativeFighterActiveDenseNormals, which the selector points
     * at the static arrays. Those arrays are defined in
     * nds_renderer_native_common.c, which this file precedes in the
     * translation unit, so naming them here does not even compile. */
    NULL,
    sNdsNativeFighterPreparedDense,
    sNdsNativeFighterActionDenseSpans,
    /* R1 retired: no raw-path reader is compiled, so these stay unlinked. */
    NULL, 0u, NULL, 0u,
    sNdsNativeFighterRunFirstUnique,
    sNdsNativeFighterRunUniqueCount,
    sNdsNativeFighterRunUniqueDense,
    sNdsNativeFighterTriangles, NDS_FTR_COUNT(sNdsNativeFighterTriangles),
    sNdsNativeFighterRuns, NDS_FTR_COUNT(sNdsNativeFighterRuns),
    sNdsNativeFighterPrimitiveGroupFirst,
    sNdsNativeFighterPrimitiveGroupCount,
    sNdsNativeFighterPrimitiveGroupType,
    sNdsNativeFighterPrimitiveGroupFirstVertex,
    sNdsNativeFighterPrimitiveGroupVertexCount,
    sNdsNativeFighterPrimitiveVertices,
    sNdsNativeFighterEpochs, NDS_FTR_COUNT(sNdsNativeFighterEpochs)
};

static const NDSNativeFighterRuntimeTables sNdsNativeFighterLowTables =
{
    sNdsNativeFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeFighterStateDeltasLow),
    sNdsNativeFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeFighterStateSequenceLow),
    sNdsNativeFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeFighterVertexActionsLow),
    sNdsNativeFighterEpochDirectPolicyLow,
    sNdsNativeFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeFighterDenseVerticesLow),
    /* NULL for the same reason as the high table above. */
    NULL,
    sNdsNativeFighterPreparedDenseLow,
    sNdsNativeFighterActionDenseSpansLow,
    NULL, 0u, NULL, 0u,
    sNdsNativeFighterRunFirstUniqueLow,
    sNdsNativeFighterRunUniqueCountLow,
    sNdsNativeFighterRunUniqueDenseLow,
    sNdsNativeFighterTrianglesLow, NDS_FTR_COUNT(sNdsNativeFighterTrianglesLow),
    sNdsNativeFighterRunsLow, NDS_FTR_COUNT(sNdsNativeFighterRunsLow),
    sNdsNativeFighterPrimitiveGroupFirstLow,
    sNdsNativeFighterPrimitiveGroupCountLow,
    sNdsNativeFighterPrimitiveGroupTypeLow,
    sNdsNativeFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeFighterPrimitiveGroupVertexCountLow,
    sNdsNativeFighterPrimitiveVerticesLow,
    sNdsNativeFighterEpochsLow, NDS_FTR_COUNT(sNdsNativeFighterEpochsLow)
};

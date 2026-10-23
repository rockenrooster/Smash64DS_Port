/* Native renderer, the fighter owners at run time: the active owner and its
 * programs, owner images and their residency, Kirby's hats, and the tables
 * behind a resolved root. A slice of nds_renderer.c. */

static const NDSNativeFighterRuntimeTables *sNdsNativeFighterActiveTables =
    &sNdsNativeFighterHighTables;
static const NDSNativeFighterOwnerRuntime *sNdsNativeFighterActiveOwner =
    &sNdsNativeMarioHighOwner;
static const u32 (*sNdsNativeFighterActiveRootLightPreambles)[2];
static u32 sNdsNativeFighterActiveRootLightPreambleCount;
static u8 sNdsNativeFighterRootPrograms[NDS_NATIVE_FIGHTER_OWNER_COUNT];
/* The slot's current program's row; NULL for canonical or another slot. */
static const NDSP4RootProgram *ndsP4CurrentRootProgram(u32 slot)
{
    u32 count;
    const NDSP4RootProgram *programs = ndsP4RootPrograms(slot, &count);
    u32 program = (slot < NDS_NATIVE_FIGHTER_OWNER_COUNT) ?
        (u32)sNdsNativeFighterRootPrograms[slot] : 0u;

    return ((programs != NULL) && (program >= 1u) && (program <= count)) ?
        &programs[program - 1u] : NULL;
}
/* The complete 9/10-root hidden-part programs are keyed not only by their
 * ordered source DL vector but by the live joint-6 head that produced the
 * context-baked body cache.  The adapter republishes this key every Kirby
 * draw before root-program selection. */
static u32 sNdsKirbyTrioHeadMp = 0u;
void ndsRendererNativeKirbyTrioSetHeadKey(u32 head_mp)
{
    sNdsKirbyTrioHeadMp = head_mp;
}

/* Which live joint-6 modelparts have a program to draw. The adapter asks here
 * rather than carrying its own literal set: heads 1 and 14 were hard-coded
 * beside head 10 while ten copy hats had no program at all, and a rejected
 * root draws nothing, so the adapter's set and the baked contexts must be one
 * fact. Head 10 is CopyLink's mixed-file program, not a trio context. */
s32 ndsRendererNativeKirbyTrioHeadSupported(u32 head_mp)
{
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT) &&     defined(NDS_NATIVE_KIRBY_TRIO_BODY_PRESENT)
    u32 index;

    for (index = 0u; index < NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT; index++)
    {
        if ((u32)sNdsKirbyTrioProgramHead[index] == head_mp)
        {
            return TRUE;
        }
    }
#endif
    return (head_mp == 10u) ? TRUE : FALSE;
}

/* The Mario/Fox-only mirror stress build has no image-backed owner block, but
 * the production preflight still resolves every selected root through these
 * two helpers. Keep the base-owner identity available even when that entire
 * block compiles out; otherwise the documented NDS_P2_FOUR_CPU_ROSTER=0
 * control fails at compile time before it can validate renderer changes. */

/* --- P2-3r4: image-backed owner tables ------------------------------------
 *
 * A P2-3 owner's generated geometry ships as a NitroFS image rather than as
 * arrays in the ARM9 binary, because on this hardware the binary costs the
 * taskman arena one byte for one byte and the roster is going to grow by ten
 * more fighters. The image is one struct, so the layout is the compiler's and
 * these offsets cannot drift from the bytes.
 *
 * The buffer comes from the taskman arena, which the scene manager rewinds
 * between scenes -- hence the heap generation stamp: a pointer from the
 * previous scene is not reloaded, it is re-read. Loading happens where a
 * fighter is CREATED, never inside a draw: a NitroFS read mid-frame is exactly
 * the stall that cost the BGM its seam on the character select. */
#define NDS_NATIVE_IMAGE_DETAILS 2u

typedef struct NDSNativeOwnerImageSlot
{
    const void *base;
    u32 heap_generation;
    u32 bytes;
} NDSNativeOwnerImageSlot;

static NDSNativeOwnerImageSlot
    sNdsNativeOwnerImage[NDS_NATIVE_IMAGE_OWNER_SLOTS_ALL][NDS_NATIVE_IMAGE_DETAILS];

__attribute__((used)) volatile u32 gNdsNativeOwnerImageLoadCount;
__attribute__((used)) volatile u32 gNdsNativeOwnerImageFailCount;
/* Second-detail binds served by the first detail's image (same bytes). */
__attribute__((used)) volatile u32 gNdsNativeOwnerImageSharedCount;
__attribute__((used)) volatile u32 gNdsNativeOwnerImageBytes;
/* Of gNdsNativeOwnerImageBytes, how many came from the battle scratch in idle
 * fighter-packet ports rather than the taskman arena. */
__attribute__((used)) volatile u32 gNdsNativeOwnerImageScratchBytes;
/* battleship_sys_objman.c: bump allocator over idle fighter-packet ports,
 * non-NULL only inside a VS battle that has them. */
void *ndsBattleIdleScratchAlloc(size_t size, u32 alignment);
__attribute__((used)) volatile u32 gNdsNativeOwnerImageMatchCount;
__attribute__((used)) volatile u32 gNdsNativeOwnerImageMismatchCount;

__attribute__((used)) volatile u32 gNdsNativeKirbyHatSuppressedCount;
/* A table identity owns its mutable prepared-dense data. VS admits one image
 * per reachable hat/detail and shares that same table between Kirby instances,
 * exactly as mirror fighters share their ordinary owner image. Non-VS display
 * scenes retain bounded, per-slot working images for their changing previews. */
typedef struct NDSNativeKirbyHatImageSlot
{
    void *base;
    u32 heap_generation;
    u32 bytes;
    u8 copy_modelpart_id;
    u8 use_low_detail;
    u8 valid;
    u8 p4_hat;       /* P4 S8: the Remix hat this image draws, 0: the source hat */
    NDSNativeFighterRuntimeTables tables;
    NDSNativeRoot root;
    const u32 (*light_preambles)[2];
    u32 light_preamble_count;
} NDSNativeKirbyHatImageSlot;

/* Native hat residency is keyed by the same four live fighter slots as the
 * game layer. Keep the renderer-local capacity explicit and prove it cannot
 * drift from the source player bound. */
static NDSNativeKirbyHatImageSlot
    sNdsNativeKirbyHatWorkingImages[NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS]
                              [NDS_NATIVE_IMAGE_DETAILS];
static NDSNativeKirbyHatImageSlot
    *sNdsNativeKirbyHatActive[NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS]
                             [NDS_NATIVE_IMAGE_DETAILS];
static u32 sNdsNativeKirbyHatBindingGeneration;
static u32 sNdsNativeKirbyHatMatchGeneration = 0xffffffffu;
static u32 sNdsNativeKirbyHatMatchReady;
static u32 sNdsNativeKirbyHatMatchAttempted;
static u32 sNdsNativeKirbyHatMatchMasks[NDS_NATIVE_IMAGE_DETAILS];
static u32 sNdsNativeKirbyHatMatchCount;
static NDSNativeKirbyHatImageSlot *sNdsNativeKirbyHatMatchImages;

static NDSNativeKirbyHatImageSlot *ndsKirbyHatSlot(u32 slot, u32 detail)
{
    if ((sNdsNativeKirbyHatBindingGeneration == gNdsTaskmanHeapGeneration) &&
        (sNdsNativeKirbyHatActive[slot][detail] != NULL))
    {
        return sNdsNativeKirbyHatActive[slot][detail];
    }
    return &sNdsNativeKirbyHatWorkingImages[slot][detail];
}

volatile u32 gNdsNativeKirbyHatMatchRequiredBytes;
volatile u32 gNdsNativeKirbyHatMatchReadyCount;
volatile u32 gNdsNativeKirbyHatMatchRejectCount;
volatile u32 gNdsNativeKirbyHatResidentBindCount;

void ndsRendererNativeBeginKirbyHatMatch(void)
{
    sNdsNativeKirbyHatBindingGeneration = gNdsTaskmanHeapGeneration;
    sNdsNativeKirbyHatMatchGeneration = gNdsTaskmanHeapGeneration;
    sNdsNativeKirbyHatMatchImages = NULL;
    sNdsNativeKirbyHatMatchCount = 0u;
    sNdsNativeKirbyHatMatchReady = FALSE;
    sNdsNativeKirbyHatMatchAttempted = FALSE;
    gNdsNativeKirbyHatMatchRequiredBytes = 0u;
    memset(sNdsNativeKirbyHatActive, 0, sizeof(sNdsNativeKirbyHatActive));
}

__attribute__((used)) volatile u32 gNdsNativeKirbyHatLoadCount;
__attribute__((used)) volatile u32 gNdsNativeKirbyHatFailCount;
/* [0] high-detail hat draws, [1] low. See the read site below: [1] == 0
 * across a natural two-player copy match is what makes deferring the
 * low-detail load safe, and it is worth 7,636 bytes of general heap. */
__attribute__((used)) volatile u32 gNdsNativeKirbyHatTableHits[2];
/* Same index. The draw asked for this detail's root and the image was not
 * resident, so the hat did not draw. [1] is the low-detail deferral's refutation
 * counter; see the read site. */
__attribute__((used)) volatile u32 gNdsNativeKirbyHatTableMisses[2];
__attribute__((used)) volatile u32 gNdsNativeKirbyHatBytes;
/* Copies whose power the match had not admitted (see the late load in
 * ndsRendererNativeEnsureKirbyCopyHat). */
__attribute__((used)) volatile u32 gNdsNativeKirbyHatLateLoadCount;

static const char *ndsRendererNativeOwnerImagePath(u32 owner_slot,
                                                   u32 use_low_detail)
{
    /* One row per owner from the image generator, replacing a per-fighter if
     * ladder that restated the generator's own path identity by hand. */
#define NDS_NATIVE_OWNER_IMAGE_PATH_ROW(slot, high_path, low_path, high_t, low_t)     if (owner_slot == (slot))                                                         {                                                                                     return (use_low_detail != 0u) ? (low_path) : (high_path);                      }
    NDS_NATIVE_OWNER_IMAGE_ROWS(NDS_NATIVE_OWNER_IMAGE_PATH_ROW)
    NDS_P4_NATIVE_OWNER_IMAGE_ROWS(NDS_NATIVE_OWNER_IMAGE_PATH_ROW)
#undef NDS_NATIVE_OWNER_IMAGE_PATH_ROW
    (void)use_low_detail;
    return NULL;
}

NDS_RENDERER_COLD static u32 ndsRendererNativeOwnerImageBytes(u32 owner_slot, u32 use_low_detail)
{
    /* Same generated rows as the path lookup above; the byte count is the
     * sizeof of the image struct the generator emitted for that owner. */
#define NDS_NATIVE_OWNER_IMAGE_BYTES_ROW(slot, high_path, low_path, high_t, low_t)     if (owner_slot == (slot))                                                          {                                                                                      return (use_low_detail != 0u) ? (u32)sizeof(low_t) : (u32)sizeof(high_t);       }
    NDS_NATIVE_OWNER_IMAGE_ROWS(NDS_NATIVE_OWNER_IMAGE_BYTES_ROW)
    NDS_P4_NATIVE_OWNER_IMAGE_ROWS(NDS_NATIVE_OWNER_IMAGE_BYTES_ROW)
#undef NDS_NATIVE_OWNER_IMAGE_BYTES_ROW
    (void)use_low_detail;
    return 0u;
}

NDS_RENDERER_COLD u32 ndsRendererNativeOwnerImageSize(u32 owner_slot, u32 use_low_detail)
{
    return ndsRendererNativeOwnerImageBytes(owner_slot, use_low_detail);
}

/* --- P2-3r4: binding an owner's tables to its loaded image ----------------
 *
 * The runtime tables struct is a set of pointers plus counts. Binding is
 * therefore assigning each pointer into the loaded image and each count from
 * the generated `_COUNT` macro that produced that array. Both come out of
 * `nds_native_fighter_image.generated.h`, so a table this ROM reads and the
 * bytes this ROM loads are described by one file.
 *
 * `prepared_dense` binds to the image's own resident member, not to a static
 * array: the member carries the exact generated initial bytes, and the image
 * buffer is scene-owned writable arena (16-aligned, member offsets from the
 * compiler's own layout), so the draw path's per-frame writes land in the
 * loaded copy exactly as they landed in the static one. Mario/Fox have no
 * image and keep their static arrays; every other owner reaches its tables
 * only after ndsRendererNativeEnsureOwnerImage has bound this struct. */
#define NDS_IMG_PRIM_OFF(type_, member_) offsetof(type_, member_##_m2)

/* One owner image's members as offsets into it, in the runtime tables'
 * order, with the generated counts: the binding is a constant per owner and
 * detail. It used to be the field-by-field assignments themselves, inlined at
 * every bind site, about 30 KB of resident code across the original cast's
 * and the P4 fighters' images. */
typedef struct NDSImgBindDesc
{
    u32 state_deltas;
    u32 state_sequence;
    u32 vertex_actions;
    u32 epoch_direct_policy;
    u32 dense_vertices;
    u32 dense_normals;
    u32 prepared_dense;
    u32 action_dense_spans;
#if NDS_NATIVE_FIGHTER_IMAGE_HAS_PACKED_CORNERS
    u32 packed_corners;
#endif
    u32 run_first_unique;
    u32 run_unique_count;
    u32 run_unique_dense;
    u32 triangles;
    u32 runs;
    u32 primitive_group_first;
    u32 primitive_group_count;
    u32 primitive_group_type;
    u32 primitive_group_first_vertex;
    u32 primitive_group_vertex_count;
    u32 primitive_vertices;
    u32 epochs;
    u32 state_delta_count;
    u32 state_sequence_count;
    u32 vertex_action_count;
    u32 dense_count;
#if NDS_NATIVE_FIGHTER_IMAGE_HAS_PACKED_CORNERS
    u32 packed_corner_count;
#endif
    u32 triangle_count;
    u32 run_count;
    u32 epoch_count;
    /* 0 when a lean build images the owner without dense_vertices
     * (generate_nds_native_owner_images.py LEAN_TRIM): bound NULL. */
    u32 dense_vertices_imaged;
} NDSImgBindDesc;

#define NDS_IMG_DESC_COLOR(type_)
#if NDS_NATIVE_FIGHTER_IMAGE_HAS_PACKED_CORNERS
#define NDS_IMG_DESC_CORNERS(type_) offsetof(type_, packed_corners),
#define NDS_IMG_DESC_CORNER_COUNT(prefix_) prefix_##_PACKED_CORNERS_COUNT,
#else
#define NDS_IMG_DESC_CORNERS(type_)
#define NDS_IMG_DESC_CORNER_COUNT(prefix_)
#endif
#define NDS_IMG_DESC_PRIMITIVES(type_)                                         \
    NDS_IMG_PRIM_OFF(type_, primitive_group_first),                            \
    NDS_IMG_PRIM_OFF(type_, primitive_group_count),                            \
    NDS_IMG_PRIM_OFF(type_, primitive_group_type),                             \
    NDS_IMG_PRIM_OFF(type_, primitive_group_first_vertex),                     \
    NDS_IMG_PRIM_OFF(type_, primitive_group_vertex_count),                     \
    NDS_IMG_PRIM_OFF(type_, primitive_vertices),
#define NDS_IMG_DESC(type_, prefix_)                                           \
    {                                                                          \
        offsetof(type_, state_deltas), offsetof(type_, state_sequence),        \
        offsetof(type_, vertex_actions), offsetof(type_, epoch_direct_policy), \
        offsetof(type_, dense_vertices), offsetof(type_, dense_normals),       \
        offsetof(type_, prepared_dense), offsetof(type_, action_dense_spans),  \
        NDS_IMG_DESC_COLOR(type_)                                              \
        NDS_IMG_DESC_CORNERS(type_)                                            \
        offsetof(type_, run_first_unique), offsetof(type_, run_unique_count),  \
        offsetof(type_, run_unique_dense), offsetof(type_, triangles),         \
        offsetof(type_, runs),                                                 \
        NDS_IMG_DESC_PRIMITIVES(type_)                                         \
        offsetof(type_, epochs),                                               \
        prefix_##_STATE_DELTAS_COUNT, prefix_##_STATE_SEQUENCE_COUNT,          \
        prefix_##_VERTEX_ACTIONS_COUNT, prefix_##_DENSE_VERTICES_COUNT,        \
        NDS_IMG_DESC_CORNER_COUNT(prefix_)                                     \
        prefix_##_TRIANGLES_COUNT, prefix_##_RUNS_COUNT,                       \
        prefix_##_EPOCHS_COUNT, prefix_##_DENSE_VERTICES_IMAGED                \
    }

/* Every bind and unbind of a tables struct replaces the image bytes its
 * prepared_dense points at, so the per-run UV memo keyed on that struct must
 * forget it; see ndsRendererNativeForgetFighterRunUvTables for the CSS face,
 * eye and gray-Link failures this closes. Defined beside the memo in
 * nds_renderer_native_common.c, later in this same unity translation unit, and
 * only where that memo exists. */
static void ndsRendererNativeForgetFighterRunUvTables(const void *tables);
#define NDS_IMG_FORGET_RUN_UV(tables_)                                         \
    ndsRendererNativeForgetFighterRunUvTables((const void *)&(tables_))

/* Binds a runtime tables struct to a loaded image (NULL unbinds it). */
NDS_RENDERER_COLD static void __attribute__((noinline)) ndsRendererNativeBindImageTables(
    NDSNativeFighterRuntimeTables *tables, const NDSImgBindDesc *desc,
    const void *base)
{
    const u8 *img = (const u8 *)base;

    NDS_IMG_FORGET_RUN_UV(*tables);
    if (img == NULL)
    {
        memset(tables, 0, sizeof(*tables));
        return;
    }
    tables->state_deltas = (const NDSNativeStateDelta *)(img + desc->state_deltas);
    tables->state_delta_count = desc->state_delta_count;
    tables->state_sequence = img + desc->state_sequence;
    tables->state_sequence_count = desc->state_sequence_count;
    tables->vertex_actions = (const NDSNativeVertexAction *)(img + desc->vertex_actions);
    tables->vertex_action_count = desc->vertex_action_count;
    tables->epoch_direct_policy = img + desc->epoch_direct_policy;
    tables->dense_vertices = (desc->dense_vertices_imaged != 0u) ?
        (const NDSNativeDenseVertex *)(img + desc->dense_vertices) : NULL;
    tables->dense_count = desc->dense_count;
    tables->dense_normals = (const u32 *)(img + desc->dense_normals);
    /* Image RAM owns mutable prepared-dense scratch. */
    tables->prepared_dense = (NDSNativePreparedDenseVertex *)(img + desc->prepared_dense);
    tables->action_dense_spans = (const u16 *)(img + desc->action_dense_spans);
#if NDS_NATIVE_FIGHTER_IMAGE_HAS_PACKED_CORNERS
    tables->packed_corners = (const u16 *)(img + desc->packed_corners);
    tables->packed_corner_count = desc->packed_corner_count;
#else
    tables->packed_corners = NULL;
    tables->packed_corner_count = 0u;
#endif
    tables->run_first_corner = NULL;
    tables->run_first_corner_count = 0u;
    tables->run_first_unique = (const u16 *)(img + desc->run_first_unique);
    tables->run_unique_count = img + desc->run_unique_count;
    tables->run_unique_dense = (const u16 *)(img + desc->run_unique_dense);
    tables->triangles = (const u16 *)(img + desc->triangles);
    tables->triangle_count = desc->triangle_count;
    tables->runs = (const NDSNativeRun *)(img + desc->runs);
    tables->run_count = desc->run_count;
    tables->primitive_group_first = (const u16 *)(img + desc->primitive_group_first);
    tables->primitive_group_count = img + desc->primitive_group_count;
    tables->primitive_group_type = img + desc->primitive_group_type;
    tables->primitive_group_first_vertex =
        (const u16 *)(img + desc->primitive_group_first_vertex);
    tables->primitive_group_vertex_count = img + desc->primitive_group_vertex_count;
    tables->primitive_vertices = (const u16 *)(img + desc->primitive_vertices);
    tables->epochs = (const NDSNativeEpoch *)(img + desc->epochs);
    tables->epoch_count = desc->epoch_count;
}

#define NDS_IMG_BIND(tables_, type_, base_, prefix_)                            \
    do                                                                         \
    {                                                                          \
        static const NDSImgBindDesc desc_ = NDS_IMG_DESC(type_, prefix_);      \
        ndsRendererNativeBindImageTables(&(tables_), &desc_, (base_));         \
    } while (0)

NDS_RENDERER_COLD static const char *ndsRendererNativeKirbyHatImagePath(
    u32 copy_modelpart_id, u32 use_low_detail)
{
#define NDS_KIRBY_HAT_PATH_CASE(id_, detail_, type_, prefix_, path_)            \
    if ((copy_modelpart_id == (id_)) && (use_low_detail == (detail_)))          \
    {                                                                            \
        return path_;                                                            \
    }
    NDS_NATIVE_KIRBY_HAT_IMAGES(NDS_KIRBY_HAT_PATH_CASE)
#undef NDS_KIRBY_HAT_PATH_CASE
    return NULL;
}

NDS_RENDERER_COLD static u32 ndsRendererNativeKirbyHatImageBytes(
    u32 copy_modelpart_id, u32 use_low_detail)
{
#define NDS_KIRBY_HAT_BYTES_CASE(id_, detail_, type_, prefix_, path_)           \
    if ((copy_modelpart_id == (id_)) && (use_low_detail == (detail_)))          \
    {                                                                            \
        return (u32)sizeof(type_);                                                \
    }
    NDS_NATIVE_KIRBY_HAT_IMAGES(NDS_KIRBY_HAT_BYTES_CASE)
#undef NDS_KIRBY_HAT_BYTES_CASE
    return 0u;
}

NDS_RENDERER_COLD static s32 ndsRendererNativeBindKirbyHatImage(
    NDSNativeKirbyHatImageSlot *entry, u32 copy_modelpart_id, u32 use_low_detail,
    const void *base)
{
    if ((entry == NULL) ||
        (use_low_detail >= NDS_NATIVE_IMAGE_DETAILS) || (base == NULL))
    {
        return FALSE;
    }
#define NDS_KIRBY_HAT_BIND_CASE(id_, detail_, type_, prefix_, path_)            \
    if ((copy_modelpart_id == (id_)) && (use_low_detail == (detail_)))          \
    {                                                                            \
        const type_ *hat_ = (const type_ *)(base);                               \
        if ((hat_->copy_modelpart_id[0] != (id_)) ||                             \
            (hat_->root_offset[0] == 0u) ||                                      \
            ((prefix_##_LIGHT_PREAMBLE_WORDS_COUNT & 1u) != 0u))                 \
        {                                                                        \
            return FALSE;                                                        \
        }                                                                        \
        NDS_IMG_BIND(entry->tables, type_, base, prefix_);                        \
        entry->root.root_offset = hat_->root_offset[0];                          \
        entry->root.first_epoch = hat_->root_first_epoch[0];                     \
        entry->root.tail_state_first = hat_->root_tail_state_first[0];           \
        entry->root.source_command_count = hat_->root_source_command_count[0];   \
        entry->root.epoch_count = hat_->root_epoch_count[0];                     \
        entry->root.tail_state_count = hat_->root_tail_state_count[0];           \
        entry->root.tail_sync_count = hat_->root_tail_sync_count[0];             \
        entry->root.light_preamble = hat_->root_light_preamble[0];               \
        entry->light_preambles = (const u32 (*)[2])hat_->light_preamble_words;    \
        entry->light_preamble_count = prefix_##_LIGHT_PREAMBLE_WORDS_COUNT / 2u; \
        return TRUE;                                                             \
    }
    NDS_NATIVE_KIRBY_HAT_IMAGES(NDS_KIRBY_HAT_BIND_CASE)
#undef NDS_KIRBY_HAT_BIND_CASE
    return FALSE;
}

#if defined(NDS_P4_KIRBY_HATS_PRESENT)
NDS_RENDERER_COLD static const char *ndsRendererNativeP4KirbyHatImagePath(u32 hat, u32 use_low_detail)
{
#define NDS_P4_KIRBY_HAT_PATH_CASE(id_, base_, detail_, type_, prefix_, path_)   \
    if ((hat == (id_)) && (use_low_detail == (detail_)))                        \
    {                                                                            \
        return path_;                                                            \
    }
    NDS_P4_KIRBY_HAT_IMAGES(NDS_P4_KIRBY_HAT_PATH_CASE)
#undef NDS_P4_KIRBY_HAT_PATH_CASE
    return NULL;
}

NDS_RENDERER_COLD static u32 ndsRendererNativeP4KirbyHatImageBytes(u32 hat, u32 use_low_detail)
{
#define NDS_P4_KIRBY_HAT_BYTES_CASE(id_, base_, detail_, type_, prefix_, path_)  \
    if ((hat == (id_)) && (use_low_detail == (detail_)))                        \
    {                                                                            \
        return (u32)sizeof(type_);                                                \
    }
    NDS_P4_KIRBY_HAT_IMAGES(NDS_P4_KIRBY_HAT_BYTES_CASE)
#undef NDS_P4_KIRBY_HAT_BYTES_CASE
    return 0u;
}

/* A Remix hat's image binds as its source hat's does: the image names the
 * source hat (copy_modelpart_id) whose live part it draws in place of. */
NDS_RENDERER_COLD static s32 ndsRendererNativeBindP4KirbyHatImage(
    NDSNativeKirbyHatImageSlot *entry, u32 hat, u32 use_low_detail, const void *base)
{
    if ((entry == NULL) || (use_low_detail >= NDS_NATIVE_IMAGE_DETAILS) || (base == NULL))
    {
        return FALSE;
    }
#define NDS_P4_KIRBY_HAT_BIND_CASE(id_, base_, detail_, type_, prefix_, path_)   \
    if ((hat == (id_)) && (use_low_detail == (detail_)))                        \
    {                                                                            \
        const type_ *hat_ = (const type_ *)(base);                               \
        if ((hat_->copy_modelpart_id[0] != (base_)) ||                           \
            (hat_->root_offset[0] == 0u) ||                                      \
            ((prefix_##_LIGHT_PREAMBLE_WORDS_COUNT & 1u) != 0u))                 \
        {                                                                        \
            return FALSE;                                                        \
        }                                                                        \
        NDS_IMG_BIND(entry->tables, type_, base, prefix_);                        \
        entry->root.root_offset = hat_->root_offset[0];                          \
        entry->root.first_epoch = hat_->root_first_epoch[0];                     \
        entry->root.tail_state_first = hat_->root_tail_state_first[0];           \
        entry->root.source_command_count = hat_->root_source_command_count[0];   \
        entry->root.epoch_count = hat_->root_epoch_count[0];                     \
        entry->root.tail_state_count = hat_->root_tail_state_count[0];           \
        entry->root.tail_sync_count = hat_->root_tail_sync_count[0];             \
        entry->root.light_preamble = hat_->root_light_preamble[0];               \
        entry->light_preambles = (const u32 (*)[2])hat_->light_preamble_words;    \
        entry->light_preamble_count = prefix_##_LIGHT_PREAMBLE_WORDS_COUNT / 2u; \
        entry->copy_modelpart_id = (u8)(base_);                                  \
        return TRUE;                                                             \
    }
    NDS_P4_KIRBY_HAT_IMAGES(NDS_P4_KIRBY_HAT_BIND_CASE)
#undef NDS_P4_KIRBY_HAT_BIND_CASE
    return FALSE;
}
#endif

/* P4 S8: the Remix hats the next admission adds (ndsRendererNativeKirbyHatSetP4Hats). */
static u8 sNdsNativeKirbyP4Hats[8];
static u32 sNdsNativeKirbyP4HatCount;
static u32 sNdsNativeKirbyP4HatWithLow;
/* P4 S15: a battle whose arena cannot hold every reachable Remix hat keeps
 * one buffer per Kirby instead, sized for the largest of them, and a Kirby's
 * hat loads into its own buffer when it takes the power
 * (ndsRendererNativeEnsureKirbyP4Hat): a Kirby wears one hat at a time. */
static NDSNativeKirbyHatImageSlot
    sNdsNativeKirbyP4HatWorking[NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS];
static u8 sNdsNativeKirbyP4DeferredHats[8];
static u32 sNdsNativeKirbyP4DeferredCount;
static u32 sNdsNativeKirbyP4DeferredDetail;
static u32 sNdsNativeKirbyP4DeferredBytes;
/* The battle slots a Kirby plays (nds_kirby_hat_residency.c). */
static u32 sNdsNativeKirbyP4KirbySlots;

void ndsRendererNativeKirbyHatSetKirbySlots(u32 slot_mask)
{
    sNdsNativeKirbyP4KirbySlots = slot_mask;
}
__attribute__((used)) volatile u32 gNdsNativeKirbyP4HatDeferCount;
__attribute__((used)) volatile u32 gNdsNativeKirbyP4HatLateLoadCount;

void ndsRendererNativeKirbyHatSetP4Hats(const u8 *hats, u32 count, u32 use_low_detail)
{
    u32 i;

    if (count > ARRAY_COUNT(sNdsNativeKirbyP4Hats))
    {
        count = ARRAY_COUNT(sNdsNativeKirbyP4Hats);
    }
    for (i = 0u; i < count; i++)
    {
        sNdsNativeKirbyP4Hats[i] = hats[i];
    }
    sNdsNativeKirbyP4HatCount = count;
    sNdsNativeKirbyP4HatWithLow = use_low_detail;
}

NDS_RENDERER_COLD s32 ndsRendererNativePrepareKirbyHatMatch(u32 high_mask, u32 low_mask)
{
    const u32 valid_mask = 0x3ff8u; /* source modelparts 3..13 */
    u32 masks[2] = { high_mask, low_mask };
    u32 count = 0u;
    u32 image_bytes = 0u;
    u32 part, detail, index;
    u32 table_bytes;
    u8 *storage;
    u32 p4_count = sNdsNativeKirbyP4HatCount;
    u32 p4_with_low = sNdsNativeKirbyP4HatWithLow;
    u32 p4_images = 0u;
    u32 p4_bytes = 0u;
    u32 p4_max = 0u;
    u32 p4_deferred = FALSE;

    sNdsNativeKirbyP4HatCount = 0u;
    sNdsNativeKirbyP4DeferredCount = 0u;
    sNdsNativeKirbyP4DeferredBytes = 0u;
    memset(sNdsNativeKirbyP4HatWorking, 0, sizeof(sNdsNativeKirbyP4HatWorking));

    /* A low bit outside the high mask is a source hat only a content's power
     * shows, in a battle of three or four where such a Kirby stays low
     * (nds_kirby_hat_residency.c). */
    if ((sNdsNativeKirbyHatMatchGeneration != gNdsTaskmanHeapGeneration) ||
        ((high_mask | low_mask) & ~valid_mask))
    {
        gNdsNativeKirbyHatMatchRejectCount++;
        return FALSE;
    }
    if (sNdsNativeKirbyHatMatchAttempted != FALSE)
    {
        if (sNdsNativeKirbyHatMatchReady &&
            (high_mask == sNdsNativeKirbyHatMatchMasks[0]) &&
            (low_mask == sNdsNativeKirbyHatMatchMasks[1]))
        {
            return TRUE;
        }
        gNdsNativeKirbyHatMatchRejectCount++;
        return FALSE;
    }
    sNdsNativeKirbyHatMatchAttempted = TRUE;
    sNdsNativeKirbyHatMatchMasks[0] = high_mask;
    sNdsNativeKirbyHatMatchMasks[1] = low_mask;
    for (part = 3u; part <= 13u; part++)
    {
        for (detail = 0u; detail < 2u; detail++)
        {
            if ((masks[detail] & (1u << part)) != 0u)
            {
                u32 bytes = ndsRendererNativeKirbyHatImageBytes(part, detail);
                if (bytes == 0u)
                {
                    gNdsNativeKirbyHatMatchRejectCount++;
                    return FALSE;
                }
                image_bytes += (bytes + 15u) & ~15u;
                count++;
            }
        }
    }
#if defined(NDS_P4_KIRBY_HATS_PRESENT)
    for (part = 0u; part < p4_count; part++)
    {
        for (detail = p4_with_low ? 1u : 0u; detail < (p4_with_low ? 2u : 1u); detail++)
        {
            u32 bytes = ndsRendererNativeP4KirbyHatImageBytes(sNdsNativeKirbyP4Hats[part], detail);

            if (bytes == 0u)
            {
                gNdsNativeKirbyHatMatchRejectCount++;
                return FALSE;
            }
            bytes = (bytes + 15u) & ~15u;
            image_bytes += bytes;
            p4_bytes += bytes;
            p4_max = (bytes > p4_max) ? bytes : p4_max;
            p4_images++;
            count++;
        }
    }
#else
    (void)p4_count;
    (void)p4_with_low;
#endif
    table_bytes = (count * sizeof(NDSNativeKirbyHatImageSlot) + 15u) & ~15u;
    gNdsNativeKirbyHatMatchRequiredBytes = table_bytes + image_bytes;
    if (count == 0u)
    {
        sNdsNativeKirbyHatMatchReady = TRUE;
        gNdsNativeKirbyHatMatchReadyCount++;
        return TRUE;
    }
    storage = ndsSceneAssetTryAlloc(table_bytes + image_bytes, 16u, 25600u);
#if defined(NDS_P4_KIRBY_HATS_PRESENT)
    if ((storage == NULL) && (p4_images != 0u) && (sNdsNativeKirbyP4KirbySlots != 0u))
    {
        u32 kirbys = 0u;
        u32 player;

        for (player = 0u; player < NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS; player++)
        {
            if ((sNdsNativeKirbyP4KirbySlots & (1u << player)) != 0u)
            {
                kirbys++;
            }
        }
        count -= p4_images;
        image_bytes -= p4_bytes;
        table_bytes = (count * sizeof(NDSNativeKirbyHatImageSlot) + 15u) & ~15u;
        gNdsNativeKirbyHatMatchRequiredBytes =
            table_bytes + image_bytes + (kirbys * p4_max);
        storage = ndsSceneAssetTryAlloc(table_bytes + image_bytes +
                                            (kirbys * p4_max), 16u, 25600u);
        p4_deferred = TRUE;
    }
#endif
    if (storage == NULL)
    {
        gNdsNativeKirbyHatMatchRejectCount++;
        return FALSE;
    }
    sNdsNativeKirbyHatMatchImages = (NDSNativeKirbyHatImageSlot *)storage;
    memset(storage, 0, table_bytes);
    storage += table_bytes;
    index = 0u;
    for (part = 3u; part <= 13u; part++)
    {
        for (detail = 0u; detail < 2u; detail++)
        {
            NDSNativeKirbyHatImageSlot *entry;
            NdsRelocAssetStream stream;
            u32 bytes;
            if ((masks[detail] & (1u << part)) == 0u) { continue; }
            entry = &sNdsNativeKirbyHatMatchImages[index++];
            bytes = ndsRendererNativeKirbyHatImageBytes(part, detail);
            entry->base = storage;
            if (ndsRelocAssetStreamOpen(&stream,
                    ndsRendererNativeKirbyHatImagePath(part, detail)) == FALSE)
            {
                gNdsNativeKirbyHatMatchRejectCount++;
                return FALSE;
            }
            if (ndsRelocAssetStreamRead(&stream, 0u, storage, bytes) == FALSE)
            {
                ndsRelocAssetStreamClose(&stream);
                gNdsNativeKirbyHatMatchRejectCount++;
                return FALSE;
            }
            ndsRelocAssetStreamClose(&stream);
            if ((*(const u32 *)storage != (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG) ||
                (ndsRendererNativeBindKirbyHatImage(entry, part, detail, storage) == FALSE))
            {
                gNdsNativeKirbyHatMatchRejectCount++;
                return FALSE;
            }
            entry->heap_generation = gNdsTaskmanHeapGeneration;
            entry->bytes = bytes;
            entry->copy_modelpart_id = (u8)part;
            entry->use_low_detail = (u8)detail;
            entry->valid = 1u;
            storage += (bytes + 15u) & ~15u;
            gNdsNativeKirbyHatLoadCount++;
            NDS_DIAG(gNdsNativeKirbyHatBytes += bytes);
        }
    }
#if defined(NDS_P4_KIRBY_HATS_PRESENT)
    if (p4_deferred != FALSE)
    {
        u32 player;

        for (part = 0u; part < p4_count; part++)
        {
            sNdsNativeKirbyP4DeferredHats[part] = sNdsNativeKirbyP4Hats[part];
        }
        sNdsNativeKirbyP4DeferredCount = p4_count;
        sNdsNativeKirbyP4DeferredDetail = p4_with_low ? 1u : 0u;
        sNdsNativeKirbyP4DeferredBytes = p4_max;
        for (player = 0u; player < NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS; player++)
        {
            if ((sNdsNativeKirbyP4KirbySlots & (1u << player)) != 0u)
            {
                sNdsNativeKirbyP4HatWorking[player].base = storage;
                sNdsNativeKirbyP4HatWorking[player].heap_generation =
                    gNdsTaskmanHeapGeneration;
                storage += p4_max;
            }
        }
        gNdsNativeKirbyP4HatDeferCount++;
        p4_count = 0u;
    }
    for (part = 0u; part < p4_count; part++)
    {
        for (detail = p4_with_low ? 1u : 0u; detail < (p4_with_low ? 2u : 1u); detail++)
        {
            u32 hat = sNdsNativeKirbyP4Hats[part];
            NDSNativeKirbyHatImageSlot *entry = &sNdsNativeKirbyHatMatchImages[index++];
            NdsRelocAssetStream stream;
            u32 bytes = ndsRendererNativeP4KirbyHatImageBytes(hat, detail);

            entry->base = storage;
            if (ndsRelocAssetStreamOpen(&stream,
                    ndsRendererNativeP4KirbyHatImagePath(hat, detail)) == FALSE)
            {
                gNdsNativeKirbyHatMatchRejectCount++;
                return FALSE;
            }
            if (ndsRelocAssetStreamRead(&stream, 0u, storage, bytes) == FALSE)
            {
                ndsRelocAssetStreamClose(&stream);
                gNdsNativeKirbyHatMatchRejectCount++;
                return FALSE;
            }
            ndsRelocAssetStreamClose(&stream);
            if ((*(const u32 *)storage != (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG) ||
                (ndsRendererNativeBindP4KirbyHatImage(entry, hat, detail, storage) == FALSE))
            {
                gNdsNativeKirbyHatMatchRejectCount++;
                return FALSE;
            }
            entry->heap_generation = gNdsTaskmanHeapGeneration;
            entry->bytes = bytes;
            entry->use_low_detail = (u8)detail;
            entry->p4_hat = (u8)hat;
            entry->valid = 1u;
            storage += (bytes + 15u) & ~15u;
            gNdsNativeKirbyHatLoadCount++;
            NDS_DIAG(gNdsNativeKirbyHatBytes += bytes);
        }
    }
#endif
    sNdsNativeKirbyHatMatchCount = count;
    sNdsNativeKirbyHatMatchReady = TRUE;
    gNdsNativeKirbyHatMatchReadyCount++;
    return TRUE;
}

/* TRUE when this scene admitted its hats up front and holds `part` at
 * `use_low_detail`: binding it is then a pointer copy, never a load. */
s32 ndsRendererNativeKirbyHatMatchHasPart(u32 part, u32 use_low_detail)
{
    u32 i;

    if ((sNdsNativeKirbyHatMatchGeneration != gNdsTaskmanHeapGeneration) ||
        (sNdsNativeKirbyHatMatchReady == FALSE))
    {
        return FALSE;
    }
    for (i = 0u; i < sNdsNativeKirbyHatMatchCount; i++)
    {
        const NDSNativeKirbyHatImageSlot *slot =
            &sNdsNativeKirbyHatMatchImages[i];

        if ((slot->copy_modelpart_id == part) && (slot->p4_hat == 0u) &&
            (slot->use_low_detail == use_low_detail) && slot->valid)
        {
            return TRUE;
        }
    }
    return FALSE;
}

NDS_RENDERER_COLD s32 ndsRendererNativeEnsureKirbyP4Hat(u32 battle_slot, u32 hat, u32 use_low_detail)
{
    u32 i;

    if ((battle_slot >= NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS) ||
        (use_low_detail >= NDS_NATIVE_IMAGE_DETAILS) || (hat == 0u) ||
        (sNdsNativeKirbyHatMatchGeneration != gNdsTaskmanHeapGeneration) ||
        (sNdsNativeKirbyHatMatchReady == FALSE))
    {
        return FALSE;
    }
    for (i = 0u; i < sNdsNativeKirbyHatMatchCount; i++)
    {
        NDSNativeKirbyHatImageSlot *slot = &sNdsNativeKirbyHatMatchImages[i];

        if ((slot->p4_hat == hat) && (slot->use_low_detail == use_low_detail) && slot->valid)
        {
            sNdsNativeKirbyHatBindingGeneration = gNdsTaskmanHeapGeneration;
            sNdsNativeKirbyHatActive[battle_slot][use_low_detail] = slot;
            gNdsNativeKirbyHatResidentBindCount++;
            return TRUE;
        }
    }
#if defined(NDS_P4_KIRBY_HATS_PRESENT)
    if ((sNdsNativeKirbyP4DeferredCount != 0u) &&
        (use_low_detail == sNdsNativeKirbyP4DeferredDetail))
    {
        NDSNativeKirbyHatImageSlot *slot = &sNdsNativeKirbyP4HatWorking[battle_slot];
        NdsRelocAssetStream stream;
        u32 bytes = ndsRendererNativeP4KirbyHatImageBytes(hat, use_low_detail);
        u32 listed = FALSE;

        for (i = 0u; i < sNdsNativeKirbyP4DeferredCount; i++)
        {
            listed |= (sNdsNativeKirbyP4DeferredHats[i] == hat) ? TRUE : FALSE;
        }
        if ((listed == FALSE) || (slot->base == NULL) ||
            (slot->heap_generation != gNdsTaskmanHeapGeneration) ||
            (bytes == 0u) || (bytes > sNdsNativeKirbyP4DeferredBytes))
        {
            return FALSE;
        }
        if ((slot->valid == 0u) || (slot->p4_hat != hat))
        {
            /* Invalidate before overwrite: a failed read cannot leave the
             * previous hat claiming these bytes. */
            slot->valid = 0u;
            slot->p4_hat = 0u;
            if (ndsRelocAssetStreamOpen(&stream,
                    ndsRendererNativeP4KirbyHatImagePath(hat, use_low_detail)) == FALSE)
            {
                return FALSE;
            }
            if (ndsRelocAssetStreamRead(&stream, 0u, slot->base, bytes) == FALSE)
            {
                ndsRelocAssetStreamClose(&stream);
                return FALSE;
            }
            ndsRelocAssetStreamClose(&stream);
            if ((*(const u32 *)slot->base != (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG) ||
                (ndsRendererNativeBindP4KirbyHatImage(slot, hat, use_low_detail,
                                                      slot->base) == FALSE))
            {
                return FALSE;
            }
            slot->bytes = bytes;
            slot->use_low_detail = (u8)use_low_detail;
            slot->p4_hat = (u8)hat;
            slot->valid = 1u;
            gNdsNativeKirbyP4HatLateLoadCount++;
            NDS_DIAG(gNdsNativeKirbyHatBytes += bytes);
        }
        sNdsNativeKirbyHatBindingGeneration = gNdsTaskmanHeapGeneration;
        sNdsNativeKirbyHatActive[battle_slot][use_low_detail] = slot;
        return TRUE;
    }
#endif
    return FALSE;
}

s32 ndsRendererNativeEnsureKirbyCopyHat(
    u32 battle_slot, u32 copy_modelpart_id, u32 use_low_detail)
{
    NdsRelocAssetStream stream;
    NDSNativeKirbyHatImageSlot *slot;
    const char *path;
    u32 bytes;
    u32 max_bytes;
    u32 in_match = FALSE;

    /* If a regenerate ever collapses the two per-detail unions back onto the
     * combined one, the allocation below silently returns to charging every low
     * slot the largest high image, the copy drops the heap under the GObj latch
     * again, and nothing fails to show it. Assert the split is real. */
    _Static_assert(NDS_NATIVE_KIRBY_HAT_MAX_LOW_BYTES <
                       NDS_NATIVE_KIRBY_HAT_MAX_BYTES,
                   "Kirby low-detail hat slots must be sized by the low union");
    _Static_assert(NDS_NATIVE_KIRBY_HAT_MAX_HIGH_BYTES <=
                       NDS_NATIVE_KIRBY_HAT_MAX_BYTES,
                   "Kirby high-detail hat slots must fit the combined union");

    if ((copy_modelpart_id < NDS_NATIVE_KIRBY_HAT_MIN_MODELPART) ||
        (copy_modelpart_id > NDS_NATIVE_KIRBY_HAT_MAX_MODELPART) ||
        (use_low_detail >= NDS_NATIVE_IMAGE_DETAILS) ||
        (battle_slot >= NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS))
    {
        gNdsNativeKirbyHatFailCount++;
        return FALSE;
    }
    if (sNdsNativeKirbyHatMatchGeneration == gNdsTaskmanHeapGeneration)
    {
        u32 i;
        if (sNdsNativeKirbyHatMatchReady != FALSE)
        {
            for (i = 0u; i < sNdsNativeKirbyHatMatchCount; i++)
            {
                slot = &sNdsNativeKirbyHatMatchImages[i];
                if ((slot->copy_modelpart_id == copy_modelpart_id) && (slot->p4_hat == 0u) &&
                    (slot->use_low_detail == use_low_detail) && slot->valid)
                {
                    sNdsNativeKirbyHatActive[battle_slot][use_low_detail] = slot;
                    gNdsNativeKirbyHatResidentBindCount++;
                    return TRUE;
                }
            }
        }
        /* P2-6 (2026-10-01): A POWER THE MATCH DID NOT ADMIT LOADS INTO THE
         * SLOT'S WORKING BUFFER. The 1P ladder admits the powers of the
         * fighters present at its start (and the Kirby Team's table); the
         * Polygon Team's waves bring polygon kinds that set was never asked
         * for (it admitted none at all, measured 2026-10-01), so the first
         * swallowed polygon failed here and halted the game for good (owner
         * playtest, Kirby, two thirds into the match).
         * The working buffer is allocated only from what the arena can spare
         * above the GObj floor (25,600); when it cannot, the caller keeps the
         * power and the hat does not draw. */
        in_match = TRUE;
        sNdsNativeKirbyHatActive[battle_slot][use_low_detail] = NULL;
    }
    /* Display scenes may change preview kinds. Their bounded working buffers
     * retain the previous scene policy; VS never reaches this storage reader. */
    slot = &sNdsNativeKirbyHatWorkingImages[battle_slot][use_low_detail];
    if ((slot->valid != 0u) &&
        (slot->heap_generation == gNdsTaskmanHeapGeneration) &&
        ((u32)slot->copy_modelpart_id == copy_modelpart_id) &&
        ((u32)slot->use_low_detail == use_low_detail))
    {
        if (in_match != FALSE)
        {
            sNdsNativeKirbyHatActive[battle_slot][use_low_detail] = slot;
        }
        return TRUE;
    }
    if (in_match != FALSE)
    {
        /* A stale power's hat must not draw in place of the new one. */
        slot->valid = 0u;
        slot->copy_modelpart_id = 0u;
        NDS_DIAG(gNdsNativeKirbyHatLateLoadCount++);
    }
    path = ndsRendererNativeKirbyHatImagePath(
        copy_modelpart_id, use_low_detail);
    bytes = ndsRendererNativeKirbyHatImageBytes(
        copy_modelpart_id, use_low_detail);
    /* SIZE THE SLOT BY ITS DETAIL, NOT BY THE UNION OF BOTH.
     *
     * sNdsNativeKirbyHatWorkingImages is indexed [battle_slot][use_low_detail], so a
     * buffer only ever holds images of its own detail -- the low slot can never
     * be handed a high image. Charging it NDS_NATIVE_KIRBY_HAT_MAX_BYTES, the
     * union over BOTH details, therefore bought nothing and cost the whole
     * difference on every copy.
     *
     * That difference is not academic. Measured 2026-09-21 (Kirby vs Fox, Dream
     * Land, natural inhale and swallow): the general heap sits at 41,684 bytes
     * free, both details load at the copy for about 8,556 bytes each, and free
     * drops to 23,204 and never recovers for the rest of the match. 25,600 is
     * where ifCommonSetMaxNumGObj freezes the GObj cap at the live count, and it
     * never unlatches -- so from the copy onward no further effect object can be
     * created. That is the shape of the owner's intermittent shield and a
     * candidate for the invisible spit-out star: whether either exists depends
     * on whether it was alive when the latch fired.
     *
     * Reuse across copies is preserved exactly: any hat of this detail still
     * fits, so the allocate-once path is unchanged. */
    max_bytes = (use_low_detail != 0u) ?
        (u32)NDS_NATIVE_KIRBY_HAT_MAX_LOW_BYTES :
        (u32)NDS_NATIVE_KIRBY_HAT_MAX_HIGH_BYTES;
    if ((path == NULL) || (bytes == 0u) || (bytes > max_bytes))
    {
        gNdsNativeKirbyHatFailCount++;
        return FALSE;
    }
    if ((slot->base == NULL) ||
        (slot->heap_generation != gNdsTaskmanHeapGeneration))
    {
        /* Scratch first, for the reason ndsRendererNativeEnsureOwnerImage
         * gives: a copy hat is battle-lifetime, and the 8,552 bytes a swallow
         * asks for mid-match are exactly what Sector Z and Zebes did not have
         * left in the arena. */
        slot->base = ndsBattleIdleScratchAlloc(max_bytes, 0x10u);
        if (slot->base == NULL)
        {
            slot->base = (in_match != FALSE) ?
                ndsSceneAssetTryAlloc(max_bytes, 0x10u, 25600u) :
                syTaskmanMalloc(max_bytes, 0x10u);
        }
        if (slot->base == NULL)
        {
            slot->valid = 0u;
            gNdsNativeKirbyHatFailCount++;
            return FALSE;
        }
        slot->heap_generation = gNdsTaskmanHeapGeneration;
    }

    /* Invalidate before overwrite. A short/missing/corrupt replacement cannot
     * leave the previous hat claiming residency over partially changed bytes. */
    slot->valid = 0u;
    slot->copy_modelpart_id = 0u;
    if (ndsRelocAssetStreamOpen(&stream, path) == FALSE)
    {
        gNdsNativeKirbyHatFailCount++;
        return FALSE;
    }
    if (ndsRelocAssetStreamRead(&stream, 0u, slot->base, bytes) == FALSE)
    {
        ndsRelocAssetStreamClose(&stream);
        gNdsNativeKirbyHatFailCount++;
        return FALSE;
    }
    ndsRelocAssetStreamClose(&stream);
    if (*(const u32 *)slot->base != (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG)
    {
        gNdsNativeKirbyHatFailCount++;
        return FALSE;
    }
    if (ndsRendererNativeBindKirbyHatImage(
            slot, copy_modelpart_id, use_low_detail, slot->base) == FALSE)
    {
        gNdsNativeKirbyHatFailCount++;
        return FALSE;
    }
    slot->bytes = bytes;
    slot->copy_modelpart_id = (u8)copy_modelpart_id;
    slot->use_low_detail = (u8)use_low_detail;
    slot->valid = 1u;
    if (in_match != FALSE)
    {
        sNdsNativeKirbyHatActive[battle_slot][use_low_detail] = slot;
    }
    gNdsNativeKirbyHatLoadCount++;
    NDS_DIAG(gNdsNativeKirbyHatBytes += bytes);
    return TRUE;
}

#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT) &&     defined(NDS_NATIVE_KIRBY_TRIO_BODY_PRESENT)
/* A trio program whose head is a deferred COPY HAT is mixed-file: its root 0
 * lives in the per-slot hat image, exactly like CopyLink's. Return that
 * program's SourceOwners table, or NULL for a face head (whose program is
 * single-file) and for any owner that is not a trio program at all -- both
 * fall through to the owner's own tables. */
static const u8 *ndsKirbyTrioProgramSourceOwnersScan(
    const NDSNativeFighterOwnerRuntime *owner, u32 *count, u32 *detail)
{
    u32 index;

    *count = 0u;
    for (index = 0u; index < NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT; index++)
    {
        if (owner == sNdsKirbyTrioHighOwners[index])
        {
            *detail = 0u;
        }
        else if (owner == sNdsKirbyTrioLowOwners[index])
        {
            *detail = 1u;
        }
        else
        {
            continue;
        }
        *count = (u32)sNdsKirbyTrioSourceOwnerCount[index];
        return sNdsKirbyTrioSourceOwners[index];
    }
    if ((owner == &sNdsNativeKirbyCopyTransitionHighOwner) ||
        (owner == &sNdsNativeKirbyCopyTransitionLowOwner))
    {
        *detail = (owner == &sNdsNativeKirbyCopyTransitionLowOwner) ? 1u : 0u;
        *count = NDS_FTR_COUNT(sNdsNativeKirbyCopyTransitionSourceOwners);
        return sNdsNativeKirbyCopyTransitionSourceOwners;
    }
    return NULL;
}

/* The scan's answer is a pure function of the owner (every table is const),
 * and a materialization asks it for every root of one owner: the last
 * owner's answer is kept (P2-2p8, 2026-10-06). *detail is written only when
 * the scan writes it. */
static const u8 *ndsKirbyTrioProgramSourceOwners(
    const NDSNativeFighterOwnerRuntime *owner, u32 *count, u32 *detail)
{
    static const NDSNativeFighterOwnerRuntime *s_owner;
    static const u8 *s_owners;
    static u32 s_count;
    static u32 s_detail;
    static u32 s_valid;

    if ((s_valid == 0u) || (owner != s_owner))
    {
        s_detail = 0u;
        s_owners = ndsKirbyTrioProgramSourceOwnersScan(owner, &s_count,
                                                       &s_detail);
        s_owner = owner;
        s_valid = 1u;
    }
    *count = s_count;
    if (s_owners != NULL)
    {
        *detail = s_detail;
    }
    return s_owners;
}
#endif

static const NDSNativeFighterRuntimeTables *
ndsRendererNativeFighterTablesForResolvedRoot(
    const NDSNativeRoot *root,
    const NDSNativeFighterOwnerRuntime *owner,
    u32 battle_slot,
    u32 binding)
{
    u32 detail;

    if ((battle_slot < NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS) &&
        ((owner == &sNdsNativeKirbyHighOwner) ||
         (owner == &sNdsNativeKirbyLowOwner)
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
         || (owner == &sNdsNativeKirbyCopyLinkHighOwner) ||
         (owner == &sNdsNativeKirbyCopyLinkLowOwner)
#endif
        ))
    {
        for (detail = 0u; detail < NDS_NATIVE_IMAGE_DETAILS; detail++)
        {
            NDSNativeKirbyHatImageSlot *hat =
                ndsKirbyHatSlot(battle_slot, detail);

            if (root != &ndsKirbyHatSlot(battle_slot, detail)->root)
            {
                continue;
            }
            if ((hat->valid == 0u) ||
                (hat->heap_generation != gNdsTaskmanHeapGeneration))
            {
                /* THE ROOT NAMED THIS DETAIL AND THE IMAGE IS NOT RESIDENT.
                 *
                 * Before the low-detail deferral this meant a failed load or a
                 * heap generation change, and falling through was right. It
                 * now also covers a wrong predicate: the copy site
                 * (reloc_backend_compat_shims.c) skips the low image when the
                 * fighter's base detail is High, on the argument that nothing
                 * can then select Low. If that argument is ever wrong the hat
                 * stops drawing, which is SILENT -- so count it here. A
                 * non-zero [1] is the deferral refuted, and it is what to read
                 * beside gNdsNativeKirbyHatTableHits rather than a
                 * screenshot. */
                gNdsNativeKirbyHatTableMisses[detail]++;
                continue;
            }
            {
                /* THE COUNTER THAT PRICES THE LOW-DETAIL HAT LEVER.
                 *
                 * Both hat details load eagerly at the copy
                 * (reloc_backend_compat_shims.c:12522-12525), about 8,556
                 * bytes each, and the general heap never recovers: measured
                 * 41,684 free before the copy, 23,204 after, against the
                 * 25,600 floor where ifCommonSetMaxNumGObj freezes the GObj
                 * cap for the rest of the match. Deferring the low image would
                 * return 7,636 bytes and clear that floor outright -- but only
                 * if nothing ever draws it.
                 *
                 * This is the draw-time selection: the renderer asked for a
                 * specific root and the root names the detail. So
                 * [1] staying ZERO across a natural two-player copy match is
                 * the proof that the low image is dead weight and the deferral
                 * is safe; any non-zero reading refutes the lever outright.
                 * Do not guess this one -- it was explicitly left unmeasured
                 * rather than assumed. */
                gNdsNativeKirbyHatTableHits[detail]++;
                return &ndsKirbyHatSlot(battle_slot, detail)->tables;
            }
        }
    }
#if NDS_P2_LINK && defined(NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT)
    if ((owner == &sNdsNativeLinkSpecialNHighOwner) ||
        (owner == &sNdsNativeLinkSpecialNLowOwner) ||
        (owner == &sNdsNativeLinkSpecialNItemHighOwner) ||
        (owner == &sNdsNativeLinkSpecialNItemLowOwner))
    {
        const sb32 item = ((owner == &sNdsNativeLinkSpecialNItemHighOwner) ||
                           (owner == &sNdsNativeLinkSpecialNItemLowOwner)) ?
            TRUE : FALSE;
        const u8 *source_owners = (item != FALSE) ?
            sNdsNativeLinkSpecialNItemSourceOwners :
            sNdsNativeLinkSpecialNSourceOwners;
        u32 source_owner;

        if (binding >= NDS_FTR_COUNT(sNdsNativeLinkSpecialNSourceOwners))
        {
            return NULL;
        }
        source_owner = source_owners[binding];
        if (source_owner == 2u)
        {
            return ((owner == &sNdsNativeLinkSpecialNLowOwner) ||
                    (owner == &sNdsNativeLinkSpecialNItemLowOwner)) ?
                &sNdsNativeLinkBoomerangFighterLowTables :
                &sNdsNativeLinkBoomerangFighterHighTables;
        }
        if (source_owner != 0u)
        {
            return NULL;
        }
    }
#endif
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT) &&     defined(NDS_NATIVE_KIRBY_TRIO_BODY_PRESENT)
    {
        u32 trio_count;
        u32 trio_detail = 0u;
        const u8 *trio_owners =
            ndsKirbyTrioProgramSourceOwners(owner, &trio_count, &trio_detail);

        if (trio_owners != NULL)
        {
            if (binding >= trio_count)
            {
                return NULL;
            }
            if (trio_owners[binding] == 1u)
            {
                if (battle_slot >= NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS)
                {
                    return NULL;
                }
                return ((ndsKirbyHatSlot(battle_slot, trio_detail)->valid != 0u) &&
                        (ndsKirbyHatSlot(battle_slot, trio_detail)->heap_generation ==
                         gNdsTaskmanHeapGeneration)) ?
                    &ndsKirbyHatSlot(battle_slot, trio_detail)->tables :
                    NULL;
            }
        }
    }
#endif
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
    if ((owner == &sNdsNativeKirbyCopyLinkHighOwner) ||
        (owner == &sNdsNativeKirbyCopyLinkLowOwner))
    {
        if (binding >= NDS_FTR_COUNT(sNdsNativeKirbyCopyLinkSourceOwners))
        {
            return NULL;
        }
        if (sNdsNativeKirbyCopyLinkSourceOwners[binding] == 1u)
        {
            detail = (owner == &sNdsNativeKirbyCopyLinkLowOwner) ? 1u : 0u;
            if (battle_slot >= NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS)
            {
                return NULL;
            }
            return ((ndsKirbyHatSlot(battle_slot, detail)->valid != 0u) &&
                    (ndsKirbyHatSlot(battle_slot, detail)->heap_generation ==
                     gNdsTaskmanHeapGeneration)) ?
                &ndsKirbyHatSlot(battle_slot, detail)->tables :
                NULL;
        }
        if (sNdsNativeKirbyCopyLinkSourceOwners[binding] == 2u)
        {
            return (owner == &sNdsNativeKirbyCopyLinkLowOwner) ?
                &sNdsNativeLinkBoomerangFighterLowTables :
                &sNdsNativeLinkBoomerangFighterHighTables;
        }
    }
#endif
    return (owner != NULL) ? owner->tables : NULL;
}

static const u32 (*ndsRendererNativeFighterLightPreamblesForResolvedRoot(
    const NDSNativeRoot *root,
    const NDSNativeFighterOwnerRuntime *owner,
    u32 battle_slot,
    u32 binding,
    u32 *count))[2]
{
    u32 detail;

    if ((battle_slot < NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS) &&
        ((owner == &sNdsNativeKirbyHighOwner) ||
         (owner == &sNdsNativeKirbyLowOwner)
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
         || (owner == &sNdsNativeKirbyCopyLinkHighOwner) ||
         (owner == &sNdsNativeKirbyCopyLinkLowOwner)
#endif
        ))
    {
        for (detail = 0u; detail < NDS_NATIVE_IMAGE_DETAILS; detail++)
        {
            NDSNativeKirbyHatImageSlot *hat =
                ndsKirbyHatSlot(battle_slot, detail);

            if ((root == &ndsKirbyHatSlot(battle_slot, detail)->root) &&
                (hat->valid != 0u) &&
                (hat->heap_generation == gNdsTaskmanHeapGeneration))
            {
                *count = ndsKirbyHatSlot(battle_slot, detail)->light_preamble_count;
                return ndsKirbyHatSlot(battle_slot, detail)->light_preambles;
            }
        }
    }
#if NDS_P2_LINK && defined(NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT)
    if ((owner == &sNdsNativeLinkSpecialNHighOwner) ||
        (owner == &sNdsNativeLinkSpecialNLowOwner) ||
        (owner == &sNdsNativeLinkSpecialNItemHighOwner) ||
        (owner == &sNdsNativeLinkSpecialNItemLowOwner))
    {
        const u8 *source_owners =
            ((owner == &sNdsNativeLinkSpecialNItemHighOwner) ||
             (owner == &sNdsNativeLinkSpecialNItemLowOwner)) ?
            sNdsNativeLinkSpecialNItemSourceOwners :
            sNdsNativeLinkSpecialNSourceOwners;
        u32 source_owner;

        if (binding >= NDS_FTR_COUNT(sNdsNativeLinkSpecialNSourceOwners))
        {
            *count = 0u;
            return NULL;
        }
        source_owner = source_owners[binding];
        if (source_owner == 2u)
        {
            *count = NDS_FTR_COUNT(sNdsNativeLinkBoomerangRootLightPreambles);
            return sNdsNativeLinkBoomerangRootLightPreambles;
        }
        if (source_owner != 0u)
        {
            *count = 0u;
            return NULL;
        }
    }
#endif
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT) &&     defined(NDS_NATIVE_KIRBY_TRIO_BODY_PRESENT)
    {
        u32 trio_count;
        u32 trio_detail = 0u;
        const u8 *trio_owners =
            ndsKirbyTrioProgramSourceOwners(owner, &trio_count, &trio_detail);

        if (trio_owners != NULL)
        {
            if (binding >= trio_count)
            {
                *count = 0u;
                return NULL;
            }
            if (trio_owners[binding] == 1u)
            {
                if ((battle_slot >= NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS) ||
                    (ndsKirbyHatSlot(battle_slot, trio_detail)->valid == 0u) ||
                    (ndsKirbyHatSlot(battle_slot, trio_detail)->heap_generation !=
                     gNdsTaskmanHeapGeneration))
                {
                    *count = 0u;
                    return NULL;
                }
                *count = ndsKirbyHatSlot(battle_slot, trio_detail)->light_preamble_count;
                return ndsKirbyHatSlot(battle_slot, trio_detail)->light_preambles;
            }
        }
    }
#endif
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
    if ((owner == &sNdsNativeKirbyCopyLinkHighOwner) ||
        (owner == &sNdsNativeKirbyCopyLinkLowOwner))
    {
        if (binding >= NDS_FTR_COUNT(sNdsNativeKirbyCopyLinkSourceOwners))
        {
            *count = 0u;
            return NULL;
        }
        if (sNdsNativeKirbyCopyLinkSourceOwners[binding] == 1u)
        {
            detail = (owner == &sNdsNativeKirbyCopyLinkLowOwner) ? 1u : 0u;
            if ((battle_slot >= NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS) ||
                (ndsKirbyHatSlot(battle_slot, detail)->valid == 0u) ||
                (ndsKirbyHatSlot(battle_slot, detail)->heap_generation !=
                 gNdsTaskmanHeapGeneration))
            {
                *count = 0u;
                return NULL;
            }
            *count = ndsKirbyHatSlot(battle_slot, detail)->light_preamble_count;
            return ndsKirbyHatSlot(battle_slot, detail)->light_preambles;
        }
        if (sNdsNativeKirbyCopyLinkSourceOwners[binding] == 2u)
        {
            *count = NDS_FTR_COUNT(sNdsNativeLinkBoomerangRootLightPreambles);
            return sNdsNativeLinkBoomerangRootLightPreambles;
        }
    }
#endif
    if (owner == NULL)
    {
        *count = 0u;
        return NULL;
    }
    *count = owner->root_light_preamble_count;
    return owner->root_light_preambles;
}

/* Bind every owner whose tables this build takes from an image. Called once
 * per successful load, with the buffer this scene's arena generation owns. */
static const u8 sNdsSkeletonBindingParents[32] = { [0 ... 31] = 0xffu };
static const u8 sNdsSkeletonCrossSlots[32] = { [0 ... 31] = 31u };
#include "generated/nds_native_skeletons.generated.inc"

/* A P4 content's root tables (owner_words) are in each of its images: the one
 * being bound, else -- it is being unbound -- the other detail's while that
 * one stays loaded; NULL when neither is. */
static const void *ndsP4OwnerTablesInImage(u32 owner_slot, u32 use_low_detail,
                                           const void *base, u32 offset)
{
    const NDSNativeOwnerImageSlot *self =
        &sNdsNativeOwnerImage[owner_slot][use_low_detail];
    const NDSNativeOwnerImageSlot *other =
        &sNdsNativeOwnerImage[owner_slot][use_low_detail ^ 1u];

    if (base != NULL)
    {
        return (const u8 *)base + offset;
    }
    if ((other->base != NULL) && (other->base != self->base))
    {
        return (const u8 *)other->base + offset;
    }
    return NULL;
}

NDS_RENDERER_COLD static void ndsRendererNativeBindOwnerImage(u32 owner_slot, u32 use_low_detail,
                                            const void *base)
{
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_MARIO_SKELETON1)
    {
        NDS_IMG_BIND(sNdsMario1SkeletonTables, NDSNativeMario_Skeleton1HighImage,
                     base, NDS_NATIVE_IMAGE_MARIO_SKELETON1_HIGH);
        return;
    }
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_FOX_SKELETON1)
    {
        NDS_IMG_BIND(sNdsFox1SkeletonTables, NDSNativeFox_Skeleton1HighImage,
                     base, NDS_NATIVE_IMAGE_FOX_SKELETON1_HIGH);
        return;
    }
#if NDS_NATIVE_OWNER_IMAGE_LUIGI
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_LUIGI)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeLuigiFighterLowTables,
                         NDSNativeLuigiLowImage, base,
                         NDS_NATIVE_IMAGE_LUIGI_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeLuigiFighterHighTables,
                         NDSNativeLuigiHighImage, base,
                         NDS_NATIVE_IMAGE_LUIGI_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_DONKEY
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_DONKEY)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeDonkeyFighterLowTables,
                         NDSNativeDonkeyLowImage, base,
                         NDS_NATIVE_IMAGE_DONKEY_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeDonkeyFighterHighTables,
                         NDSNativeDonkeyHighImage, base,
                         NDS_NATIVE_IMAGE_DONKEY_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_CAPTAIN
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_CAPTAIN)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeCaptainFighterLowTables,
                         NDSNativeCaptainLowImage, base,
                         NDS_NATIVE_IMAGE_CAPTAIN_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeCaptainFighterHighTables,
                         NDSNativeCaptainHighImage, base,
                         NDS_NATIVE_IMAGE_CAPTAIN_HIGH);
        }
        return;
    }
#endif
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_SAMUS)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeSamusFighterLowTables,
                         NDSNativeSamusLowImage, base,
                         NDS_NATIVE_IMAGE_SAMUS_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeSamusFighterHighTables,
                         NDSNativeSamusHighImage, base,
                         NDS_NATIVE_IMAGE_SAMUS_HIGH);
        }
        return;
    }
#if NDS_NATIVE_OWNER_IMAGE_LINK
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_LINK)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeLinkFighterLowTables,
                         NDSNativeLinkLowImage, base,
                         NDS_NATIVE_IMAGE_LINK_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeLinkFighterHighTables,
                         NDSNativeLinkHighImage, base,
                         NDS_NATIVE_IMAGE_LINK_HIGH);
        }
        return;
    }
#endif
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_PIKACHU)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativePikachuFighterLowTables,
                         NDSNativePikachuLowImage, base,
                         NDS_NATIVE_IMAGE_PIKACHU_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativePikachuFighterHighTables,
                         NDSNativePikachuHighImage, base,
                         NDS_NATIVE_IMAGE_PIKACHU_HIGH);
        }
        return;
    }
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_YOSHI)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeYoshiFighterLowTables,
                         NDSNativeYoshiLowImage, base,
                         NDS_NATIVE_IMAGE_YOSHI_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeYoshiFighterHighTables,
                         NDSNativeYoshiHighImage, base,
                         NDS_NATIVE_IMAGE_YOSHI_HIGH);
        }
        return;
    }
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NESS)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNessFighterLowTables,
                         NDSNativeNessLowImage, base,
                         NDS_NATIVE_IMAGE_NESS_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNessFighterHighTables,
                         NDSNativeNessHighImage, base,
                         NDS_NATIVE_IMAGE_NESS_HIGH);
        }
        return;
    }
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_PURIN)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativePurinFighterLowTables,
                         NDSNativePurinLowImage, base,
                         NDS_NATIVE_IMAGE_PURIN_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativePurinFighterHighTables,
                         NDSNativePurinHighImage, base,
                         NDS_NATIVE_IMAGE_PURIN_HIGH);
        }
        return;
    }
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_KIRBY)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeKirbyFighterLowTables,
                         NDSNativeKirbyLowImage, base,
                         NDS_NATIVE_IMAGE_KIRBY_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeKirbyFighterHighTables,
                         NDSNativeKirbyHighImage, base,
                         NDS_NATIVE_IMAGE_KIRBY_HIGH);
        }
        return;
    }
#if NDS_NATIVE_OWNER_IMAGE_MMARIO
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_MMARIO)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeMMarioFighterLowTables,
                         NDSNativeMMarioLowImage, base,
                         NDS_NATIVE_IMAGE_MMARIO_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeMMarioFighterHighTables,
                         NDSNativeMMarioHighImage, base,
                         NDS_NATIVE_IMAGE_MMARIO_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_NMARIO
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NMARIO)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNMarioFighterLowTables,
                         NDSNativeNMarioLowImage, base,
                         NDS_NATIVE_IMAGE_NMARIO_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNMarioFighterHighTables,
                         NDSNativeNMarioHighImage, base,
                         NDS_NATIVE_IMAGE_NMARIO_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_NFOX
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NFOX)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNFoxFighterLowTables,
                         NDSNativeNFoxLowImage, base,
                         NDS_NATIVE_IMAGE_NFOX_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNFoxFighterHighTables,
                         NDSNativeNFoxHighImage, base,
                         NDS_NATIVE_IMAGE_NFOX_HIGH);
        }
        return;
    }
#endif
#define NDS_P4_OWNER_BIND(id_, T_, N_, n_, parent_, model_, main_)             \
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_##N_)                               \
    {                                                                           \
        _Static_assert(offsetof(NDSNative##T_##LowImage, owner_words) ==        \
                       offsetof(NDSNative##T_##HighImage, owner_words),         \
                       "P4 owner tables differ in place between details");      \
        if (use_low_detail != 0u)                                               \
        {                                                                       \
            NDS_IMG_BIND(sNdsNative##T_##FighterLowTables,                      \
                         NDSNative##T_##LowImage, base,                         \
                         NDS_NATIVE_IMAGE_##N_##_LOW);                          \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            NDS_IMG_BIND(sNdsNative##T_##FighterHighTables,                     \
                         NDSNative##T_##HighImage, base,                        \
                         NDS_NATIVE_IMAGE_##N_##_HIGH);                         \
        }                                                                       \
        ndsP4Bind##T_##OwnerTables(ndsP4OwnerTablesInImage(                     \
            owner_slot, use_low_detail, base,                                   \
            (u32)offsetof(NDSNative##T_##LowImage, owner_words)));              \
        return;                                                                 \
    }
    NDS_P4_CONTENT_ROWS(NDS_P4_OWNER_BIND)
#undef NDS_P4_OWNER_BIND
#if NDS_NATIVE_OWNER_IMAGE_NDONKEY
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NDONKEY)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNDonkeyFighterLowTables,
                         NDSNativeNDonkeyLowImage, base,
                         NDS_NATIVE_IMAGE_NDONKEY_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNDonkeyFighterHighTables,
                         NDSNativeNDonkeyHighImage, base,
                         NDS_NATIVE_IMAGE_NDONKEY_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_NSAMUS
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NSAMUS)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNSamusFighterLowTables,
                         NDSNativeNSamusLowImage, base,
                         NDS_NATIVE_IMAGE_NSAMUS_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNSamusFighterHighTables,
                         NDSNativeNSamusHighImage, base,
                         NDS_NATIVE_IMAGE_NSAMUS_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_NLINK
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NLINK)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNLinkFighterLowTables,
                         NDSNativeNLinkLowImage, base,
                         NDS_NATIVE_IMAGE_NLINK_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNLinkFighterHighTables,
                         NDSNativeNLinkHighImage, base,
                         NDS_NATIVE_IMAGE_NLINK_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_NYOSHI
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NYOSHI)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNYoshiFighterLowTables,
                         NDSNativeNYoshiLowImage, base,
                         NDS_NATIVE_IMAGE_NYOSHI_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNYoshiFighterHighTables,
                         NDSNativeNYoshiHighImage, base,
                         NDS_NATIVE_IMAGE_NYOSHI_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_NCAPTAIN
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NCAPTAIN)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNCaptainFighterLowTables,
                         NDSNativeNCaptainLowImage, base,
                         NDS_NATIVE_IMAGE_NCAPTAIN_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNCaptainFighterHighTables,
                         NDSNativeNCaptainHighImage, base,
                         NDS_NATIVE_IMAGE_NCAPTAIN_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_NKIRBY
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NKIRBY)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNKirbyFighterLowTables,
                         NDSNativeNKirbyLowImage, base,
                         NDS_NATIVE_IMAGE_NKIRBY_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNKirbyFighterHighTables,
                         NDSNativeNKirbyHighImage, base,
                         NDS_NATIVE_IMAGE_NKIRBY_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_NPIKACHU
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NPIKACHU)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNPikachuFighterLowTables,
                         NDSNativeNPikachuLowImage, base,
                         NDS_NATIVE_IMAGE_NPIKACHU_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNPikachuFighterHighTables,
                         NDSNativeNPikachuHighImage, base,
                         NDS_NATIVE_IMAGE_NPIKACHU_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_NPURIN
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NPURIN)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNPurinFighterLowTables,
                         NDSNativeNPurinLowImage, base,
                         NDS_NATIVE_IMAGE_NPURIN_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNPurinFighterHighTables,
                         NDSNativeNPurinHighImage, base,
                         NDS_NATIVE_IMAGE_NPURIN_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_NNESS
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NNESS)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNNessFighterLowTables,
                         NDSNativeNNessLowImage, base,
                         NDS_NATIVE_IMAGE_NNESS_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNNessFighterHighTables,
                         NDSNativeNNessHighImage, base,
                         NDS_NATIVE_IMAGE_NNESS_HIGH);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_BOSS
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_BOSS)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeBossFighterLowTables,
                         NDSNativeBossLowImage, base,
                         NDS_NATIVE_IMAGE_BOSS_LOW);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeBossFighterHighTables,
                         NDSNativeBossHighImage, base,
                         NDS_NATIVE_IMAGE_BOSS_HIGH);
        }
        return;
    }
#endif
    (void)owner_slot;
    (void)use_low_detail;
    (void)base;
}

/* Load one owner image for this scene, or report that it is already resident.
 * Called from fighter creation; never from a draw. */
/* TRUE only while this scene's heap still holds the bound image. */
s32 ndsRendererNativeOwnerImageResident(u32 owner_slot, u32 use_low_detail)
{
    return ((owner_slot < NDS_NATIVE_IMAGE_OWNER_SLOTS_ALL) &&
            (use_low_detail < NDS_NATIVE_IMAGE_DETAILS) &&
            (sNdsNativeOwnerImage[owner_slot][use_low_detail].base != NULL) &&
            (sNdsNativeOwnerImage[owner_slot][use_low_detail].heap_generation ==
                 gNdsTaskmanHeapGeneration)) ? TRUE : FALSE;
}

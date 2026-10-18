/* P2-2 compact shield-pose runtime.
 *
 * BattleShip guard selects one of eight 45-degree Event32 tables, evaluates it
 * once at the continuous local angle, blends against ShieldPose DObjDesc base
 * transforms by stick magnitude, then clears the animation.  The DS package
 * preserves that behavior but stores source data as one compact NitroFS blob
 * per selected fighter, so a four-kind match never pays for the roster.
 */
#include "nds_scene_harness_config.h"

#include <nds/nds_shield_pose.h>

#include <nds/nds_anim_fixed.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_frontend_overlay.h>
#include <nds/nds_startup.h>
#include <sys/obj.h>
#include <sys/taskman.h>
#include <nds/nds_p4_contents.h>
#if NDS_P4
#include <nds/nds_p4.h>
#endif

__attribute__((used)) volatile u32 gNdsShieldPoseNativeFixupCount;
__attribute__((used)) volatile u32 gNdsShieldPoseNativeFixupRejectCount;
__attribute__((used)) volatile u32 gNdsShieldPoseSingleApplyCount;
__attribute__((used)) volatile u32 gNdsShieldPoseAllApplyCount;
__attribute__((used)) volatile u32 gNdsShieldPoseBatchPlayCount;
__attribute__((used)) volatile u32 gNdsShieldPoseDecodeFailCount;
__attribute__((used)) volatile u32 gNdsShieldPoseLoadCount;
__attribute__((used)) volatile u32 gNdsShieldPoseLoadFailCount;
__attribute__((used)) volatile u32 gNdsShieldPoseResidentBytes;

#if NDS_P2_COMPACT_BATTLE_FIGHTERS || NDS_P2_DONKEY || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_KIRBY || \
    NDS_P2_CAPTAIN || NDS_P2_PIKACHU || NDS_P2_PURIN || NDS_P2_NESS

#include <nds/generated/nds_shield_pose_assets.generated.h>

#define NDS_SHIELD_POSE_CODE __attribute__((noinline, optimize("Os")))
/* The per-tick decode (P2-2p8, 2026-09-28). A shielding fighter decodes
 * every joint's script each tick, ~250-500 scratch words, and the Os Thumb
 * decode made an out-of-line value read and Q->f32 call per word. ARM,
 * O2, the helpers inline; the output words are unchanged (replay
 * identical). Saffron P95 -5.4K, Dream Land P95 -2.8K. */
#define NDS_SHIELD_POSE_HOT_CODE \
    __attribute__((noinline, optimize("O2"), target("arm")))
#define NDS_SHIELD_POSE_HOT_INLINE \
    static inline __attribute__((always_inline, target("arm")))

typedef struct NDSShieldPoseBlobHeader
{
    u32 magic;
    u16 version;
    u16 header_bytes;
    u16 total_bytes;
    u16 joint_count;
    u16 base_count;
    u16 scratch_words;
    u16 command_count;
    u16 op_count;
    u16 flag_count;
    u16 duration_count;
    u16 template_count;
    u16 template_token_count;
    u16 value_small_count;
    u16 value_large_count;
    u16 handle_count;
    u16 base_offset_count;
    u16 script_data_bytes;
    u16 base_data_bytes;
    u16 command_keys_offset;
    u16 command_ops_offset;
    u16 command_flags_offset;
    u16 command_durations_offset;
    u16 template_first_offset;
    u16 template_tokens_offset;
    u16 value_small_offset;
    u16 value_large_offset;
    u16 script_data_offset;
    u16 handles_offset;
    u16 base_offsets_offset;
    u16 base_data_offset;
} NDSShieldPoseBlobHeader;

_Static_assert(sizeof(NDSShieldPoseBlobHeader) == NDS_SHIELD_POSE_BLOB_HEADER_BYTES,
               "ShieldPose blob header ABI drifted");

typedef struct NDSShieldPoseAssetDesc
{
    s16 fkind;
    u16 main_asset;
    u16 shield_asset;
    u16 blob_bytes;
    u16 dobj_offset;
    u16 table_offsets[8];
    u16 main_fixup_slots[9];
} NDSShieldPoseAssetDesc;

#define NDS_SHIELD_POSE_ROW(fkind_, main_, shield_, bytes_, dobj_, \
                            t0_, t1_, t2_, t3_, t4_, t5_, t6_, t7_, \
                            s0_, s1_, s2_, s3_, s4_, s5_, s6_, s7_, s8_) \
    { (s16)(fkind_), (u16)(main_), (u16)(shield_), (u16)(bytes_), \
      (u16)(dobj_), { (u16)(t0_), (u16)(t1_), (u16)(t2_), (u16)(t3_), \
                       (u16)(t4_), (u16)(t5_), (u16)(t6_), (u16)(t7_) }, \
      { (u16)(s0_), (u16)(s1_), (u16)(s2_), (u16)(s3_), (u16)(s4_), \
        (u16)(s5_), (u16)(s6_), (u16)(s7_), (u16)(s8_) } },
static const NDSShieldPoseAssetDesc sNdsShieldPoseAssets[] = {
    NDS_SHIELD_POSE_ASSET_ROWS(NDS_SHIELD_POSE_ROW)
};
#undef NDS_SHIELD_POSE_ROW

_Static_assert(ARRAY_COUNT(sNdsShieldPoseAssets) == NDS_SHIELD_POSE_ASSET_COUNT,
               "ShieldPose descriptor count drifted");

#if NDS_P4
/* P4 S15: a content's own package (scripts/p4/p4_shield_pose.py) is package
 * NDS_SHIELD_POSE_ASSET_COUNT + content - 1, its descriptor built from the
 * content's NDSP4ShieldPose row. Its limits are the content packages' (a
 * Remix guard script can need more words than the stack scratch holds: it
 * then decodes into a scratch of its own, allocated with the package). */
#define NDS_SHIELD_POSE_P4_PACKAGES ((u32)NDS_P4_CONTENT_LIMIT - 1u)
static NDSShieldPoseAssetDesc sNdsShieldPoseP4Descs[NDS_P4_CONTENT_LIMIT];
static AObjEvent32 *sNdsShieldPoseP4Scratch[NDS_P4_CONTENT_LIMIT];
/* A content package's base rows (its dobj_lookup), decoded once at load into
 * the heap beside the blob: the shared scratch below lives in DTCM, sized for
 * the original cast (a content has up to NDS_P4_SHIELD_POSE_MAX_BASE_COUNT). */
static DObjDesc *sNdsShieldPoseP4Base[NDS_P4_CONTENT_LIMIT];
#else
#define NDS_SHIELD_POSE_P4_PACKAGES 0u
#endif
#define NDS_SHIELD_POSE_BASE_LIMIT NDS_SHIELD_POSE_MAX_BASE_COUNT
#define NDS_SHIELD_POSE_PACKAGES (NDS_SHIELD_POSE_ASSET_COUNT + NDS_SHIELD_POSE_P4_PACKAGES)
/* scripts/p4/p4_shield_pose.py: the value token past a package's dictionary
 * that carries an s32, and the longest guard script it accepts. */
#define NDS_SHIELD_POSE_VALUE_WIDE_ESCAPE 253u
#define NDS_SHIELD_POSE_MAX_COMMANDS 256u

/* The package's descriptor, or NULL. */
static const NDSShieldPoseAssetDesc *ndsShieldPoseDesc(u32 package)
{
    if (package < NDS_SHIELD_POSE_ASSET_COUNT)
    {
        return &sNdsShieldPoseAssets[package];
    }
#if NDS_P4
    if (package < NDS_SHIELD_POSE_PACKAGES)
    {
        u32 content = package - NDS_SHIELD_POSE_ASSET_COUNT + 1u;
        const NDSP4Fighter *f = ndsP4Fighter(content);
        NDSShieldPoseAssetDesc *d = &sNdsShieldPoseP4Descs[content];
        const NDSP4ShieldPose *row;
        u32 i;

        if (f == NULL)
        {
            return NULL;
        }
        row = f->shield_pose;
        if (row->blob_bytes == 0u)
        {
            return NULL;
        }
        d->fkind = (s16)(NDS_P4_SEL_BASE + content);
        d->main_asset = row->main_asset;
        d->shield_asset = row->shield_asset;
        d->blob_bytes = row->blob_bytes;
        d->dobj_offset = row->dobj_offset;
        for (i = 0u; i < 8u; i++)
        {
            d->table_offsets[i] = row->table_offsets[i];
        }
        for (i = 0u; i < 9u; i++)
        {
            d->main_fixup_slots[i] = row->main_fixup_slots[i];
        }
        return d;
    }
#endif
    return NULL;
}

typedef struct NDSShieldPoseView
{
    const NDSShieldPoseBlobHeader *h;
    /* The package's own decode scratch (P4), or NULL for the stack's. */
    AObjEvent32 *scratch;
    /* Its dobj_lookup rows: the shared scratch, or a content package's own
     * (decoded once at load). */
    DObjDesc *base;
    const u16 *command_keys;
    const u8 *command_ops;
    const u16 *command_flags;
    const u16 *command_durations;
    const u16 *template_first;
    const u8 *template_tokens;
    const s8 *value_small;
    const s16 *value_large;
    const u8 *script_data;
    const u16 *handles;
    const u16 *base_offsets;
    const u8 *base_data;
} NDSShieldPoseView;

static s32 NDS_SHIELD_POSE_HOT_CODE ndsShieldPoseDecodeBaseRow(
    const NDSShieldPoseView *view, u32 row, DObjDesc *dst);
extern DObj *lbCommonGetTreeDObjNextFromRoot(DObj *dobj, DObj *root);
extern void ftMainPlayAnimEventsAll(GObj *fighter_gobj);
extern void lbCommonPlayTranslateScaledDObjAnim(DObj *dobj, Vec3f *scale);
extern void ndsBaseGcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                                      f32 anim_frame);

/* Source guard only reads translate/rotate through dobj_lookup.  This 32-row
 * maximum is shared because guard evaluation is synchronous; script scratch is
 * stack-local so it never consumes the taskman arena or static BSS. */
static DObjDesc sNdsShieldPoseDObjScratch[NDS_SHIELD_POSE_BASE_LIMIT]
    __attribute__((section(".sbss.shield_pose")));
static void *sNdsShieldPoseLoaded[NDS_SHIELD_POSE_PACKAGES];
static u32 sNdsShieldPoseLoadedGeneration;

static GObj *sNdsShieldPoseBatchGObj;
static DObj *sNdsShieldPoseBatchRoot;
static u8 sNdsShieldPoseBatchPackage;
static u8 sNdsShieldPoseBatchSector;

static s32 NDS_SHIELD_POSE_CODE ndsShieldPoseSpanFits(
    const NDSShieldPoseBlobHeader *h, u32 offset, u32 count, u32 element_size)
{
    if ((h == NULL) || (element_size == 0u) ||
        (offset < h->header_bytes) || (offset > h->total_bytes))
    {
        return FALSE;
    }
    return count <= (((u32)h->total_bytes - offset) / element_size);
}

static void ndsShieldPoseResetGeneration(void)
{
    u32 i;

    if (sNdsShieldPoseLoadedGeneration == gNdsTaskmanHeapGeneration)
    {
        return;
    }
    for (i = 0u; i < NDS_SHIELD_POSE_PACKAGES; i++)
    {
        sNdsShieldPoseLoaded[i] = NULL;
    }
#if NDS_P4
    for (i = 0u; i < (u32)NDS_P4_CONTENT_LIMIT; i++)
    {
        sNdsShieldPoseP4Scratch[i] = NULL;
        sNdsShieldPoseP4Base[i] = NULL;
    }
#endif
    sNdsShieldPoseLoadedGeneration = gNdsTaskmanHeapGeneration;
    gNdsShieldPoseResidentBytes = 0u;
}

static s32 NDS_SHIELD_POSE_CODE ndsShieldPoseValidateBlob(
    u32 package, const void *data, NDSShieldPoseView *view)
{
    const NDSShieldPoseAssetDesc *desc;
    const NDSShieldPoseBlobHeader *h;
    const u8 *base = data;

    desc = ndsShieldPoseDesc(package);
    if ((desc == NULL) || (data == NULL) || (view == NULL))
    {
        return FALSE;
    }
    h = (const NDSShieldPoseBlobHeader *)data;
    view->scratch = NULL;
    view->base = sNdsShieldPoseDObjScratch;
#if NDS_P4
    if (package >= NDS_SHIELD_POSE_ASSET_COUNT)
    {
        u32 content = package - NDS_SHIELD_POSE_ASSET_COUNT + 1u;

        view->scratch = sNdsShieldPoseP4Scratch[content];
        view->base = sNdsShieldPoseP4Base[content];
        if (((h->scratch_words > NDS_SHIELD_POSE_MAX_SCRATCH_WORDS) &&
             ((view->scratch == NULL) ||
              (h->scratch_words > NDS_P4_SHIELD_POSE_MAX_SCRATCH_WORDS))) ||
            (view->base == NULL) ||
            (h->base_count > NDS_P4_SHIELD_POSE_MAX_BASE_COUNT) ||
            (h->base_count != ndsP4Fighter(content)->shield_pose->base_count))
        {
            return FALSE;
        }
    }
    else
#endif
    if ((h->scratch_words > NDS_SHIELD_POSE_MAX_SCRATCH_WORDS) ||
        (h->base_count > NDS_SHIELD_POSE_MAX_BASE_COUNT))
    {
        return FALSE;
    }
    if ((h->magic != NDS_SHIELD_POSE_BLOB_MAGIC) ||
        (h->version != NDS_SHIELD_POSE_BLOB_VERSION) ||
        (h->header_bytes != NDS_SHIELD_POSE_BLOB_HEADER_BYTES) ||
        (h->total_bytes != desc->blob_bytes) ||
        (h->handle_count != (u16)(h->joint_count * 8u)) ||
        (h->base_offset_count != h->base_count) ||
        (ndsShieldPoseSpanFits(h, h->command_keys_offset,
                               h->command_count, sizeof(u16)) == FALSE) ||
        (ndsShieldPoseSpanFits(h, h->command_ops_offset,
                               h->op_count, sizeof(u8)) == FALSE) ||
        (ndsShieldPoseSpanFits(h, h->command_flags_offset,
                               h->flag_count, sizeof(u16)) == FALSE) ||
        (ndsShieldPoseSpanFits(h, h->command_durations_offset,
                               h->duration_count, sizeof(u16)) == FALSE) ||
        (ndsShieldPoseSpanFits(h, h->template_first_offset,
                               h->template_count, sizeof(u16)) == FALSE) ||
        (ndsShieldPoseSpanFits(h, h->template_tokens_offset,
                               h->template_token_count, sizeof(u8)) == FALSE) ||
        (ndsShieldPoseSpanFits(h, h->value_small_offset,
                               h->value_small_count, sizeof(s8)) == FALSE) ||
        (ndsShieldPoseSpanFits(h, h->value_large_offset,
                               h->value_large_count, sizeof(s16)) == FALSE) ||
        (ndsShieldPoseSpanFits(h, h->script_data_offset,
                               h->script_data_bytes, sizeof(u8)) == FALSE) ||
        (ndsShieldPoseSpanFits(h, h->handles_offset,
                               h->handle_count, sizeof(u16)) == FALSE) ||
        (ndsShieldPoseSpanFits(h, h->base_offsets_offset,
                               h->base_offset_count, sizeof(u16)) == FALSE) ||
        (ndsShieldPoseSpanFits(h, h->base_data_offset,
                               h->base_data_bytes, sizeof(u8)) == FALSE))
    {
        return FALSE;
    }
    view->h = h;
    view->command_keys = (const u16 *)(base + h->command_keys_offset);
    view->command_ops = base + h->command_ops_offset;
    view->command_flags = (const u16 *)(base + h->command_flags_offset);
    view->command_durations = (const u16 *)(base + h->command_durations_offset);
    view->template_first = (const u16 *)(base + h->template_first_offset);
    view->template_tokens = base + h->template_tokens_offset;
    view->value_small = (const s8 *)(base + h->value_small_offset);
    view->value_large = (const s16 *)(base + h->value_large_offset);
    view->script_data = base + h->script_data_offset;
    view->handles = (const u16 *)(base + h->handles_offset);
    view->base_offsets = (const u16 *)(base + h->base_offsets_offset);
    view->base_data = base + h->base_data_offset;
    return TRUE;
}

/* A pose package lives as long as the battle: a VS battle's idle overlay
 * tail first (ndsVSBattleOverlayTryAlloc), the scene heap otherwise. */
static void *ndsShieldPoseAlloc(size_t bytes)
{
    void *data = ndsVSBattleOverlayTryAlloc(bytes, 4u);

    return (data != NULL) ? data : syTaskmanMalloc(bytes, 4u);
}

static void *NDS_SHIELD_POSE_CODE ndsShieldPoseLoad(u32 package)
{
    const NDSShieldPoseAssetDesc *desc;
    NDSShieldPoseView view;
    char path[] = "nitro:/fighters/shield_pose/00.bin";
    void *data;
    const u32 digit_at = sizeof("nitro:/fighters/shield_pose/") - 1u;

    desc = ndsShieldPoseDesc(package);
    if (desc == NULL)
    {
        return NULL;
    }
    ndsShieldPoseResetGeneration();
    if (sNdsShieldPoseLoaded[package] != NULL)
    {
        return sNdsShieldPoseLoaded[package];
    }
#if NDS_P4
    if (package >= NDS_SHIELD_POSE_ASSET_COUNT)
    {
        u32 content = package - NDS_SHIELD_POSE_ASSET_COUNT + 1u;
        u32 words = ndsP4Fighter(content)->shield_pose->scratch_words;

        if ((words > NDS_SHIELD_POSE_MAX_SCRATCH_WORDS) &&
            (sNdsShieldPoseP4Scratch[content] == NULL))
        {
            sNdsShieldPoseP4Scratch[content] =
                ndsShieldPoseAlloc(words * sizeof(AObjEvent32));
        }
        if (sNdsShieldPoseP4Base[content] == NULL)
        {
            sNdsShieldPoseP4Base[content] = ndsShieldPoseAlloc(
                ndsP4Fighter(content)->shield_pose->base_count * sizeof(DObjDesc));
        }
    }
#endif
    data = ndsShieldPoseAlloc(desc->blob_bytes);
    if (data == NULL)
    {
        gNdsShieldPoseLoadFailCount++;
        return NULL;
    }
    path[digit_at] = (char)('0' + ((u32)desc->fkind / 10u));
    path[digit_at + 1u] = (char)('0' + ((u32)desc->fkind % 10u));
    if ((ndsRelocAssetReadRawRange(path, 0u, data, desc->blob_bytes) == FALSE) ||
        (ndsShieldPoseValidateBlob(package, data, &view) == FALSE))
    {
        gNdsShieldPoseLoadFailCount++;
        return NULL;
    }
    if (view.base != sNdsShieldPoseDObjScratch)
    {
        u32 row;

        for (row = 0u; row < view.h->base_count; row++)
        {
            if ((view.base_offsets[row] >= view.h->base_data_bytes) ||
                (ndsShieldPoseDecodeBaseRow(&view, row, &view.base[row]) == FALSE))
            {
                gNdsShieldPoseLoadFailCount++;
                return NULL;
            }
        }
    }
    sNdsShieldPoseLoaded[package] = data;
    gNdsShieldPoseLoadCount++;
    gNdsShieldPoseResidentBytes += desc->blob_bytes;
    return data;
}

static s32 NDS_SHIELD_POSE_CODE ndsShieldPoseGetView(
    u32 package, NDSShieldPoseView *view)
{
    void *data = ndsShieldPoseLoad(package);

    return (data != NULL) ? ndsShieldPoseValidateBlob(package, data, view)
                          : FALSE;
}

/* The Polygon Team's fighters guard with their base fighter's ShieldPose file
 * (dFTN<Name>Data names it), and each polygon Main points at the same nine
 * targets in it as the base Main does -- the generator proves both and that
 * neither Main has translate_scales. The base package therefore serves the
 * polygon too: twelve kinds load in that one stage, and the raw files were
 * 79 KB it could not spare. */
typedef struct NDSShieldPosePolygonDesc
{
    s16 fkind;
    s16 base_fkind;
    u16 main_asset;
} NDSShieldPosePolygonDesc;

#define NDS_SHIELD_POSE_POLYGON_ROW(fkind_, base_, main_)                     \
    { (s16)(fkind_), (s16)(base_), (u16)(main_) },
static const NDSShieldPosePolygonDesc sNdsShieldPosePolygons[] = {
    NDS_SHIELD_POSE_POLYGON_ROWS(NDS_SHIELD_POSE_POLYGON_ROW)
};
#undef NDS_SHIELD_POSE_POLYGON_ROW

static s32 ndsShieldPosePackageForBaseFKind(s32 fkind)
{
    u32 i;

    for (i = 0u; i < NDS_SHIELD_POSE_ASSET_COUNT; i++)
    {
        if (sNdsShieldPoseAssets[i].fkind == fkind)
        {
            return (s32)i;
        }
    }
    return -1;
}

/* The package a fighter of this kind guards with: its own, or for a polygon
 * its base fighter's. */
static s32 ndsShieldPosePackageForFKind(s32 fkind)
{
    u32 i;

    for (i = 0u; i < ARRAY_COUNT(sNdsShieldPosePolygons); i++)
    {
        if (sNdsShieldPosePolygons[i].fkind == fkind)
        {
            return ndsShieldPosePackageForBaseFKind(
                sNdsShieldPosePolygons[i].base_fkind);
        }
    }
    return ndsShieldPosePackageForBaseFKind(fkind);
}

static s32 ndsShieldPosePolygonMainPackage(u32 owner_asset, u32 dep_asset)
{
    u32 i;

    for (i = 0u; i < ARRAY_COUNT(sNdsShieldPosePolygons); i++)
    {
        if (sNdsShieldPosePolygons[i].main_asset == owner_asset)
        {
            s32 package = ndsShieldPosePackageForBaseFKind(
                sNdsShieldPosePolygons[i].base_fkind);

            return ((package >= 0) &&
                    (sNdsShieldPoseAssets[package].shield_asset == dep_asset))
                       ? package : -1;
        }
    }
    return -1;
}

#if NDS_P4
/* A content's package: its own, or the parent's it aliases; -1 keeps the
 * raw file. */
static s32 ndsShieldPosePackageForContent(u32 content)
{
    const NDSP4Fighter *f = ndsP4Fighter(content);

    if (f == NULL)
    {
        return -1;
    }
    if (f->shield_pose->blob_bytes != 0u)
    {
        return (s32)(NDS_SHIELD_POSE_ASSET_COUNT + content - 1u);
    }
    return (f->shield_pose->alias_fkind >= 0) ?
        ndsShieldPosePackageForBaseFKind(f->shield_pose->alias_fkind) : -1;
}
#endif

/* The package a fighter guards with (a content's by its content, not its
 * parent's kind). */
static s32 ndsShieldPosePackageForFighter(const FTStruct *fp)
{
    if (fp == NULL)
    {
        return -1;
    }
#if NDS_P4
    if (fp->nds_p4_content != 0u)
    {
        return ndsShieldPosePackageForContent(fp->nds_p4_content);
    }
#endif
    return ndsShieldPosePackageForFKind(fp->fkind);
}

static s32 ndsShieldPosePackageForAssets(u32 owner_asset, u32 dep_asset)
{
    u32 i;

    for (i = 0u; i < NDS_SHIELD_POSE_ASSET_COUNT; i++)
    {
        if ((sNdsShieldPoseAssets[i].main_asset == owner_asset) &&
            (sNdsShieldPoseAssets[i].shield_asset == dep_asset))
        {
            return (s32)i;
        }
    }
#if NDS_P4
    for (i = 1u; i < (u32)NDS_P4_CONTENT_LIMIT; i++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(i);

        if ((f != NULL) && (f->shield_pose->main_asset == owner_asset) &&
            (f->shield_pose->shield_asset == dep_asset))
        {
            return ndsShieldPosePackageForContent(i);
        }
    }
#endif
    return ndsShieldPosePolygonMainPackage(owner_asset, dep_asset);
}

s32 ndsShieldPoseReplacesSourceFile(s32 fkind)
{
    return (ndsShieldPosePackageForFKind(fkind) >= 0) ? TRUE : FALSE;
}

s32 ndsShieldPoseNamesDependency(u32 owner_asset, u32 dep_asset)
{
    return (ndsShieldPosePackageForAssets(owner_asset, dep_asset) >= 0) ? TRUE : FALSE;
}

s32 ndsShieldPoseServesDependency(u32 owner_asset, u32 dep_asset)
{
    s32 package = ndsShieldPosePackageForAssets(owner_asset, dep_asset);
    NDSShieldPoseView view;

    return ((package >= 0) &&
            (ndsShieldPoseGetView((u32)package, &view) != FALSE)) ? TRUE
                                                                   : FALSE;
}

s32 NDS_SHIELD_POSE_CODE ndsShieldPosePatchCompactMain(
    s32 fkind, void *main_data, u32 main_bytes)
{
    s32 package = ndsShieldPosePackageForFKind(fkind);
    const u16 *slots;
    NDSShieldPoseView view;
    void *resolved[9];
    u8 *base = main_data;
    u32 i;

#if NDS_P4
    /* A content's pack kind (NDS_P4_SEL_BASE + content): its package, and
     * the guard slots of its own Main (an aliased package's descriptor
     * names the parent's). */
    if ((u32)fkind > NDS_P4_SEL_BASE)
    {
        const NDSP4Fighter *f = ndsP4Fighter((u32)fkind - NDS_P4_SEL_BASE);

        if (f == NULL)
        {
            return 0;
        }
        package = ndsShieldPosePackageForContent((u32)fkind - NDS_P4_SEL_BASE);
        slots = f->shield_pose->main_fixup_slots;
    }
    else
#endif
    {
        slots = (package >= 0) ? ndsShieldPoseDesc((u32)package)->main_fixup_slots : NULL;
    }
    if (package < 0)
    {
        return 0;
    }
    if ((main_data == NULL) ||
        (ndsShieldPoseGetView((u32)package, &view) == FALSE))
    {
        gNdsShieldPoseNativeFixupRejectCount++;
        return -1;
    }
    resolved[0] = view.base;
    for (i = 1u; i < ARRAY_COUNT(resolved); i++)
    {
        resolved[i] = (void *)&view.handles[(i - 1u) * view.h->joint_count];
    }

    /* FPC Main keeps source Main byte offsets but deliberately NULLs external
     * dependencies outside its own compact sections.  The generator proves
     * these are exactly the nine source Main->ShieldPose slots.  Validate the
     * NULL sentinel contract before publishing any native pointer so a stale
     * FPC/header pair fails closed rather than becoming a mixed representation. */
    for (i = 0u; i < ARRAY_COUNT(resolved); i++)
    {
        u32 slot = slots[i];

        if ((slot > main_bytes) || (sizeof(void *) > (main_bytes - slot)) ||
            (*(void **)(base + slot) != NULL))
        {
            gNdsShieldPoseNativeFixupRejectCount++;
            return -1;
        }
    }
    for (i = 0u; i < ARRAY_COUNT(resolved); i++)
    {
        *(void **)(base + slots[i]) = resolved[i];
    }
    gNdsShieldPoseNativeFixupCount += ARRAY_COUNT(resolved);
    return 1;
}

static void ndsShieldPoseMarkBatchFailure(GObj *fighter_gobj)
{
    sNdsShieldPoseBatchGObj = fighter_gobj;
    sNdsShieldPoseBatchRoot = NULL;
}

static s32 NDS_SHIELD_POSE_CODE ndsShieldPoseNativeSelected(
    const FTStruct *fp, u32 package, u32 sector, NDSShieldPoseView *view)
{
    const void *expected;

    if ((fp == NULL) || (fp->attr == NULL) || (sector >= 8u) ||
        (ndsShieldPoseGetView(package, view) == FALSE))
    {
        return FALSE;
    }
    expected = (const void *)&view->handles[sector * view->h->joint_count];
    return (((const void *)fp->attr->shield_anim_joints[sector] == expected) &&
            ((const void *)fp->attr->dobj_lookup ==
             (const void *)view->base)) ? TRUE : FALSE;
}

NDS_SHIELD_POSE_HOT_INLINE s32 ndsShieldPoseReadValue(
    const NDSShieldPoseView *view, const u8 **cursor, const u8 *end, s32 *out)
{
    u32 token;
    u32 index;

    if (*cursor >= end)
    {
        return FALSE;
    }
    token = *(*cursor)++;
    if (token < NDS_SHIELD_POSE_VALUE_ESCAPE)
    {
        if (token < view->h->value_small_count)
        {
            *out = (s32)view->value_small[token];
            return TRUE;
        }
        index = token - view->h->value_small_count;
        if (index < view->h->value_large_count)
        {
            *out = (s32)view->value_large[index];
            return TRUE;
        }
        /* Past the dictionary: a content package's value beyond Q6's s16
         * range (Marth's and Roy's guard poses) follows as an s32. The
         * original cast's packages never emit a token here. */
        if ((token != NDS_SHIELD_POSE_VALUE_WIDE_ESCAPE) ||
            ((size_t)(end - *cursor) < 4u))
        {
            return FALSE;
        }
        *out = (s32)((u32)(*cursor)[0] | ((u32)(*cursor)[1] << 8) |
                     ((u32)(*cursor)[2] << 16) | ((u32)(*cursor)[3] << 24));
        *cursor += 4;
        return TRUE;
    }
    if ((token != NDS_SHIELD_POSE_VALUE_ESCAPE) ||
        ((size_t)(end - *cursor) < 2u))
    {
        return FALSE;
    }
    *out = (s32)(s16)((u16)(*cursor)[0] | ((u16)(*cursor)[1] << 8));
    *cursor += 2;
    return TRUE;
}

/* Q`frac` -> f32 for the blob's s16-range values: the bits
 * ndsR2FixedToF32(value << (NDS_R2_AQ_VF - frac), NDS_R2_AQ_VF) returns,
 * built directly -- |value| < 2^16 fits the significand, so no rounding
 * (checked for every s16 at fracs 6, 11 and 12). */
NDS_SHIELD_POSE_HOT_INLINE f32 ndsShieldPoseQToF32(s32 value, u32 frac)
{
    u32 m;
    u32 msb;
    u32 bits;
    f32 f;

    if (value == 0)
    {
        return 0.0F;
    }
    m = (value < 0) ? (0u - (u32)value) : (u32)value;
    msb = 31u - (u32)__builtin_clz(m);
    bits = ((value < 0) ? 0x80000000u : 0u) |
           ((msb + 127u - frac) << 23) |
           ((m << (23u - msb)) & 0x7fffffu);
    __builtin_memcpy(&f, &bits, sizeof(f));
    return f;
}

static s32 NDS_SHIELD_POSE_HOT_CODE ndsShieldPoseUnpackScript(
    const NDSShieldPoseView *view, u16 handle, AObjEvent32 *scratch)
{
    const u8 *cursor;
    const u8 *end = view->script_data + view->h->script_data_bytes;
    u32 template_id;
    u32 command_at;
    u32 scratch_at = 0u;
    u32 guard;

    if ((handle == NDS_SHIELD_POSE_NULL_HANDLE) ||
        (handle >= view->h->script_data_bytes))
    {
        return FALSE;
    }
    cursor = view->script_data + handle;
    template_id = *cursor++;
    if (template_id == NDS_SHIELD_POSE_TEMPLATE_ESCAPE)
    {
        if ((size_t)(end - cursor) < 2u)
        {
            return FALSE;
        }
        template_id = (u32)cursor[0] | ((u32)cursor[1] << 8);
        cursor += 2;
    }
    if (template_id >= view->h->template_count)
    {
        return FALSE;
    }
    command_at = view->template_first[template_id];
    for (guard = 0u; guard < NDS_SHIELD_POSE_MAX_COMMANDS; guard++, command_at++)
    {
        u32 command_index;
        u32 key;
        u32 opcode_index;
        u32 flag_index;
        u32 duration_index;
        u32 opcode;
        u32 flags;
        u32 duration;
        u32 bit;

        if (command_at >= view->h->template_token_count)
        {
            return FALSE;
        }
        command_index = view->template_tokens[command_at];
        if (command_index >= view->h->command_count)
        {
            return FALSE;
        }
        key = view->command_keys[command_index];
        opcode_index = key & 0x0fu;
        flag_index = (key >> 4) & 0x3fu;
        duration_index = (key >> 10) & 0x3fu;
        if ((opcode_index >= view->h->op_count) ||
            (flag_index >= view->h->flag_count) ||
            (duration_index >= view->h->duration_count) ||
            (scratch_at >= view->h->scratch_words))
        {
            return FALSE;
        }
        opcode = view->command_ops[opcode_index];
        flags = view->command_flags[flag_index];
        duration = view->command_durations[duration_index];
        scratch[scratch_at++].u = opcode | (flags << 7) | (duration << 17);
        if (opcode == 0u)
        {
            return TRUE;
        }
        if ((opcode == 2u) || (opcode == 12u))
        {
            continue;
        }
        if ((opcode != 3u) && (opcode != 4u) && (opcode != 5u) &&
            (opcode != 6u) && (opcode != 7u) && (opcode != 8u) &&
            (opcode != 9u) && (opcode != 10u) && (opcode != 11u))
        {
            return FALSE;
        }
        for (bit = 0u; bit < 10u; bit++)
        {
            s32 value;

            if ((flags & (1u << bit)) == 0u)
            {
                continue;
            }
            /* TraI carries an SYInterpDesc pointer, not a scalar.  The host
             * generator refuses it from this compact guard corpus. */
            if ((bit == 3u) ||
                (ndsShieldPoseReadValue(view, &cursor, end, &value) == FALSE) ||
                (scratch_at >= view->h->scratch_words))
            {
                return FALSE;
            }
            scratch[scratch_at++].f =
                ndsShieldPoseQToF32(value, NDS_SHIELD_POSE_VALUE_FRAC);
            if ((opcode == 5u) || (opcode == 6u))
            {
                if ((ndsShieldPoseReadValue(view, &cursor, end, &value) == FALSE) ||
                    (scratch_at >= view->h->scratch_words))
                {
                    return FALSE;
                }
                scratch[scratch_at++].f =
                    ndsShieldPoseQToF32(value, NDS_SHIELD_POSE_RATE_FRAC);
            }
        }
    }
    return FALSE;
}

/* A posed joint's AObjs go back to the pool. The guard never plays them again
 * (anim_wait is AOBJ_ANIM_NULL until the next apply parses its script from
 * the start), every script is checked independent of stale AObj state
 * (generate_nds_shield_pose_pack.py, p4_shield_pose.py), and the pose engine
 * keeps fighter motions in its own tracks. Left attached they stayed on every
 * joint after a fighter's first shield: four Sonics held 511 AObjs, 18 KB of
 * the heap's per-AObj fallback once the pool ran dry (2026-10-11). Kirby keeps
 * them: his copy of Donkey Kong's charge is an animjoint motion, which the
 * generic player parses over whatever his joints hold. */
static sb32 ndsShieldPoseReturnsAObjs(const DObj *dobj)
{
    const FTStruct *fp = ftGetStruct(dobj->parent_gobj);

#if NDS_P4
    if (fp->nds_p4_content != 0u)
    {
        return TRUE;
    }
#endif
    return ((fp->fkind != nFTKindKirby) && (fp->fkind != nFTKindNKirby)) ?
               TRUE : FALSE;
}

/* The tracks a decoded script sets (its value commands' flags, bit b the
 * track nGCAnimTrackJointStart + b): what the play writes, and all it writes
 * of the joint's transform. `words` is the package's scratch: a P4 content's
 * scripts run past the original cast's 58 words (Sonic's two longest, of 148,
 * read as unbounded, so his shield memo never filled). */
static u32 ndsShieldPoseScriptTracks(const AObjEvent32 *scratch, u32 words)
{
    u32 mask = 0u;
    u32 at = 0u;
    u32 guard;

    for (guard = 0u; guard < words; guard++)
    {
        u32 word;
        u32 opcode;
        u32 flags;

        if (at >= words)
        {
            break;
        }
        word = scratch[at++].u;
        opcode = word & 0x7fu;
        flags = (word >> 7) & 0x3ffu;
        if (opcode == 0u)
        {
            return mask;
        }
        if ((opcode == 2u) || (opcode == 12u))
        {
            continue;
        }
        mask |= flags;
        at += (u32)__builtin_popcount(flags) *
            (((opcode == 5u) || (opcode == 6u)) ? 2u : 1u);
    }
    return 0xffffffffu;
}

/* The translate scale the source's guard plays a joint with: the row of
 * translate_scales at the joint's fp->joints index (ftParamUpdateAnimKeys
 * under ftCommonGuardInitJoints; YRotN's row in ftCommonGuardUpdateJoints,
 * whose joint is YRotN), NULL for a fighter without them. The tree walk
 * meets the joints in their array order, so the search starts at `hint`. */
static Vec3f *ndsShieldPoseJointScale(const FTStruct *fp, const DObj *dobj,
                                      u32 *hint)
{
    const u32 count = ARRAY_COUNT(fp->joints);
    u32 i = *hint;
    u32 k;

    if ((fp->is_have_translate_scale == FALSE) ||
        (fp->attr->translate_scales == NULL))
    {
        return NULL;
    }
    for (k = 0u; k < count; k++, i++)
    {
        if (i >= count)
        {
            i = 0u;
        }
        if (fp->joints[i] == dobj)
        {
            *hint = i + 1u;
            return &fp->attr->translate_scales[i];
        }
    }
    return NULL;
}

static s32 NDS_SHIELD_POSE_CODE ndsShieldPoseApplyScript(
    const NDSShieldPoseView *view, DObj *dobj, u16 handle, f32 angle,
    u32 *out_tracks, Vec3f *scale)
{
    AObjEvent32 stack_scratch[NDS_SHIELD_POSE_MAX_SCRATCH_WORDS];
    AObjEvent32 *scratch = (view->scratch != NULL) ? view->scratch : stack_scratch;

    if ((dobj == NULL) ||
        (ndsShieldPoseUnpackScript(view, handle, scratch) == FALSE))
    {
        return FALSE;
    }
    if (out_tracks != NULL)
    {
        *out_tracks = ndsShieldPoseScriptTracks(scratch,
                                                view->h->scratch_words);
    }
    ndsBaseGcAddDObjAnimJoint(dobj, scratch, angle);
    gcParseDObjAnimJoint(dobj);
    /* A content on its own proportions (translate_scales, a P6 look) plays
     * its guard as the source does, through the scaled player
     * (2026-10-18: Palutena's raw ShieldPose file was 22 KB of a four-heavy
     * match's heap). */
    if (scale != NULL)
    {
        lbCommonPlayTranslateScaledDObjAnim(dobj, scale);
    }
    else
    {
        gcPlayDObjAnimJoint(dobj);
    }
    dobj->anim_joint.event32 = NULL;
    if (ndsShieldPoseReturnsAObjs(dobj) != FALSE)
    {
        gcRemoveAObjFromDObj(dobj);
    }
    dobj->anim_wait = AOBJ_ANIM_NULL;
    return TRUE;
}

/* Which blob each scratch row was last decoded from (P2-2p8). The scratch is
 * shared by every guarding fighter, so each apply decodes its base rows again
 * (~3.8K ticks an all-joint apply); a row that already holds this blob's
 * values, decoded in this heap generation, is left as it is. Only
 * ndsShieldPoseRefreshBaseRow writes the scratch. */
static const void *sNdsShieldPoseScratchOwner[NDS_SHIELD_POSE_BASE_LIMIT];
static u32 sNdsShieldPoseScratchGeneration;
__attribute__((used)) volatile u32 gNdsShieldPoseBaseMemoHits;

static s32 NDS_SHIELD_POSE_HOT_CODE ndsShieldPoseRefreshBaseRow(
    const NDSShieldPoseView *view, u32 row)
{
    u32 i;

    if ((row >= view->h->base_count) ||
        (view->base_offsets[row] >= view->h->base_data_bytes))
    {
        return FALSE;
    }
    if (view->base != sNdsShieldPoseDObjScratch)
    {
        /* A content package's own rows, decoded once when it loaded. */
        return TRUE;
    }
    if (sNdsShieldPoseScratchGeneration != gNdsTaskmanHeapGeneration)
    {
        for (i = 0u; i < NDS_SHIELD_POSE_BASE_LIMIT; i++)
        {
            sNdsShieldPoseScratchOwner[i] = NULL;
        }
        sNdsShieldPoseScratchGeneration = gNdsTaskmanHeapGeneration;
    }
    if (sNdsShieldPoseScratchOwner[row] == (const void *)view->h)
    {
        NDS_DIAG(gNdsShieldPoseBaseMemoHits++);
        return TRUE;
    }
    sNdsShieldPoseScratchOwner[row] = NULL;
    if (ndsShieldPoseDecodeBaseRow(view, row, &sNdsShieldPoseDObjScratch[row]) == FALSE)
    {
        return FALSE;
    }
    sNdsShieldPoseScratchOwner[row] = (const void *)view->h;
    return TRUE;
}

/* One dobj_lookup row: its translate and rotate from the package, unit
 * scale, no list. */
static s32 NDS_SHIELD_POSE_HOT_CODE ndsShieldPoseDecodeBaseRow(
    const NDSShieldPoseView *view, u32 row, DObjDesc *dst)
{
    const u8 *src;
    const u8 *end = view->base_data + view->h->base_data_bytes;
    f32 *out;
    u32 mask;
    u32 i;

    src = view->base_data + view->base_offsets[row];
    if (src >= end)
    {
        return FALSE;
    }
    out = &dst->translate.x;
    mask = *src++;
    dst->id = (s32)row;
    dst->dl = NULL;
    for (i = 0u; i < 6u; i++)
    {
        s32 value = 0;

        if ((mask & (1u << i)) != 0u)
        {
            if ((size_t)(end - src) < 2u)
            {
                return FALSE;
            }
            value = (s32)(s16)((u16)src[0] | ((u16)src[1] << 8));
            src += 2;
        }
        out[i] = ndsShieldPoseQToF32(
            value, (i < 3u) ? NDS_SHIELD_POSE_BASE_TRA_FRAC
                             : NDS_SHIELD_POSE_BASE_ROT_FRAC);
    }
    dst->scale.x = dst->scale.y = dst->scale.z = 1.0F;
    return TRUE;
}

static s32 NDS_SHIELD_POSE_CODE ndsShieldPoseRefreshBaseAll(
    const NDSShieldPoseView *view)
{
    u32 i;

    for (i = 0u; i < view->h->base_count; i++)
    {
        if (ndsShieldPoseRefreshBaseRow(view, i) == FALSE)
        {
            return FALSE;
        }
    }
    return TRUE;
}

s32 ndsShieldPoseResolveExternalFixup(u32 owner_asset, u32 dep_asset,
                                      u32 target_offset, void **resolved)
{
    s32 package = ndsShieldPosePackageForAssets(owner_asset, dep_asset);
    NDSShieldPoseView view;
    const NDSShieldPoseAssetDesc *desc;
    u32 sector;

    if (package < 0)
    {
        return 0;
    }
    if ((resolved == NULL) ||
        (ndsShieldPoseGetView((u32)package, &view) == FALSE))
    {
        gNdsShieldPoseNativeFixupRejectCount++;
        /* A polygon Main whose base package is not staged in this build
         * keeps the raw file: every one of its nine fixups misses the same
         * way, and ndsShieldPoseNativeSelected then sees source pointers. */
        return (ndsShieldPosePolygonMainPackage(owner_asset, dep_asset) >= 0)
                   ? 0 : -1;
    }
    desc = ndsShieldPoseDesc((u32)package);
    if (target_offset == desc->dobj_offset)
    {
        *resolved = view.base;
        gNdsShieldPoseNativeFixupCount++;
        return 1;
    }
    for (sector = 0u; sector < 8u; sector++)
    {
        if (target_offset == desc->table_offsets[sector])
        {
            *resolved = (void *)&view.handles[sector * view.h->joint_count];
            gNdsShieldPoseNativeFixupCount++;
            return 1;
        }
    }
    gNdsShieldPoseNativeFixupRejectCount++;
    return -1;
}

/* A joint past the package's table: the source reads the table's NULL
 * terminator there (every guard table ends with one), so the joint gets no
 * script -- a content whose skeleton has more joints than its guard file
 * covers (Palutena: Peach's 29-entry tables, 30 joints from XRotN). Its old
 * script must not stay behind for the source's parser. */
static void ndsShieldPoseApplyNone(DObj *dobj)
{
    AObj *aobj;

    for (aobj = dobj->aobj; aobj != NULL; aobj = aobj->next)
    {
        aobj->kind = nGCAnimKindNone;
    }
    dobj->anim_joint.event32 = NULL;
    dobj->anim_wait = AOBJ_ANIM_NULL;
    if ((dobj->aobj != NULL) && (ndsShieldPoseReturnsAObjs(dobj) != FALSE))
    {
        gcRemoveAObjFromDObj(dobj);
    }
}

s32 ndsShieldPoseTryApplySingle(DObj *dobj, f32 angle)
{
    FTStruct *fp;
    NDSShieldPoseView view;
    s32 package;
    u32 sector;
    s32 i;
    s32 joint_num = 0;
    u16 handle;
    u32 hint = nFTPartsJointYRotN;

    if ((dobj == NULL) || (dobj->parent_gobj == NULL))
    {
        return 0;
    }
    fp = ftGetStruct(dobj->parent_gobj);
    package = ndsShieldPosePackageForFighter(fp);
    if (package < 0)
    {
        return 0;
    }
    sector = (u32)fp->status_vars.common.guard.angle_i;
    if (ndsShieldPoseNativeSelected(fp, (u32)package, sector, &view) == FALSE)
    {
        return 0;
    }
    for (i = nFTPartsJointXRotN; i < ARRAY_COUNT(fp->joints); i++)
    {
        if ((fp->joints[i] != NULL)
#if NDS_P4
            /* EXTRA's static parts are left out of the count
             * (StaticPart.asm fix_ftCommonGuardUpdateJoints_). */
            && (ndsP4StaticPart(fp, i) == FALSE)
#endif
            )
        {
            joint_num++;
        }
    }
    joint_num--;
    if ((joint_num < 0) || ((u32)joint_num >= view.h->base_count))
    {
        dobj->anim_joint.event32 = NULL;
        dobj->anim_wait = AOBJ_ANIM_NULL;
        gNdsShieldPoseDecodeFailCount++;
        return -1;
    }
    if ((u32)joint_num >= view.h->joint_count)
    {
        ndsShieldPoseApplyNone(dobj);
        if (ndsShieldPoseRefreshBaseRow(&view, (u32)joint_num) == FALSE)
        {
            gNdsShieldPoseDecodeFailCount++;
            return -1;
        }
        gNdsShieldPoseSingleApplyCount++;
        return 1;
    }
    handle = view.handles[(sector * view.h->joint_count) + (u32)joint_num];
    if ((handle == NDS_SHIELD_POSE_NULL_HANDLE) ||
        (ndsShieldPoseApplyScript(&view, dobj, handle, angle, NULL,
                                  ndsShieldPoseJointScale(fp, dobj, &hint)) ==
         FALSE) ||
        (ndsShieldPoseRefreshBaseRow(&view, (u32)joint_num) == FALSE))
    {
        dobj->anim_joint.event32 = NULL;
        dobj->anim_wait = AOBJ_ANIM_NULL;
        gNdsShieldPoseDecodeFailCount++;
        return -1;
    }
    gNdsShieldPoseSingleApplyCount++;
    return 1;
}

/* THE LAST POSE OF EACH SHIELD, KEPT (P4, 2026-10-12).
 *
 * The source re-applies the guard pose to every joint every tick a fighter
 * shields (ftCommonGuardInitJoints): here a script decode, an AObj add, the
 * source parse and play and the AObjs' return, per joint -- the largest
 * simulation premium of a P4 four-CPU tail (four Banjos: decode, parse, play
 * and AObj adds ~125K cycles a frame over the gate, 2026-10-12 census). The
 * pose is a function of the fighter's package and sector, the stick angle
 * and the joints' speed alone (every script is checked independent of stale
 * AObj state, generate_nds_shield_pose_pack.py), and a held shield repeats
 * them tick after tick: each battle slot keeps the tracks its last apply
 * wrote, bit for bit, and a repeat writes those instead. The rows live in the
 * framebuffer's tail past the fighter packets (ndsFighterPacketArenaTail):
 * no memory of their own. Not for Kirby, whose AObjs stay on his joints. */
extern u8 *ndsFighterPacketArenaTail(u32 *out_bytes);
#define NDS_SHIELD_POSE_MEMO_SLOTS 4u
#define NDS_SHIELD_POSE_MEMO_BYTES 1600u
#define NDS_SHIELD_POSE_MEMO_ROWS 48u
#define NDS_SHIELD_POSE_MEMO_POSED 0x8000u
typedef struct NDSShieldPoseMemo
{
    const GObj *gobj;
    u32 heap_generation;
    u32 angle_bits;
    u32 speed_bits;
    f32 parent_anim_frame;
    u16 rows;           /* DObjs the apply visits, static parts included */
    u8 package;
    u8 sector;
    u8 valid;
    u8 reserved[3];
} NDSShieldPoseMemo;
/* After the head: u16 tracks[rows] (POSED | the track bits the play wrote),
 * f32 anim_frame[posed rows], f32 values[] (each posed row's tracks in track
 * order). */
#define NDS_SHIELD_POSE_MEMO_HEAD_BYTES ((sizeof(NDSShieldPoseMemo) + 3u) & ~3u)
__attribute__((used)) volatile u32 gNdsShieldPoseMemoHits;
__attribute__((used)) volatile u32 gNdsShieldPoseMemoFills;
__attribute__((used)) volatile u32 gNdsShieldPoseMemo = 1u; /* A/B word */

static NDSShieldPoseMemo *ndsShieldPoseMemoFor(const FTStruct *fp)
{
    u32 bytes = 0u;
    u8 *tail;

    if ((gNdsShieldPoseMemo == 0u) ||
        ((u32)fp->player >= NDS_SHIELD_POSE_MEMO_SLOTS) ||
        (fp->fkind == nFTKindKirby) || (fp->fkind == nFTKindNKirby))
    {
        return NULL;
    }
    tail = ndsFighterPacketArenaTail(&bytes);
    if ((tail == NULL) ||
        (bytes < (NDS_SHIELD_POSE_MEMO_SLOTS * NDS_SHIELD_POSE_MEMO_BYTES)))
    {
        return NULL;
    }
    return (NDSShieldPoseMemo *)(void *)(tail +
        ((u32)fp->player * NDS_SHIELD_POSE_MEMO_BYTES));
}

static u32 ndsShieldPoseF32Bits(f32 value)
{
    u32 bits;

    __builtin_memcpy(&bits, &value, sizeof(bits));
    return bits;
}

/* The DObj fields of the joint tracks, by track bit (TraI never: the decode
 * refuses it). */
static f32 *ndsShieldPoseTrackField(DObj *dobj, u32 bit)
{
    switch (bit)
    {
    case 0u: return &dobj->rotate.vec.f.x;
    case 1u: return &dobj->rotate.vec.f.y;
    case 2u: return &dobj->rotate.vec.f.z;
    case 4u: return &dobj->translate.vec.f.x;
    case 5u: return &dobj->translate.vec.f.y;
    case 6u: return &dobj->translate.vec.f.z;
    case 7u: return &dobj->scale.vec.f.x;
    case 8u: return &dobj->scale.vec.f.y;
    case 9u: return &dobj->scale.vec.f.z;
    default: return NULL;
    }
}

static sb32 ndsShieldPoseMemoMatches(const NDSShieldPoseMemo *memo,
                                     DObj *root_dobj, u32 package,
                                     u32 sector, f32 angle)
{
    const GObj *gobj = root_dobj->parent_gobj;

    return ((memo->valid != 0u) && (memo->gobj == gobj) &&
            (memo->heap_generation == gNdsTaskmanHeapGeneration) &&
            (memo->package == (u8)package) && (memo->sector == (u8)sector) &&
            (memo->angle_bits == ndsShieldPoseF32Bits(angle)) &&
            (memo->speed_bits == ndsShieldPoseF32Bits(root_dobj->anim_speed)) &&
            ((gobj->flags & GOBJ_FLAG_NOANIM) == 0u)) ? TRUE : FALSE;
}

/* The apply's effects from the memo: FALSE (nothing written) when the tree
 * no longer has the memo's shape. */
static sb32 ndsShieldPoseMemoApply(const NDSShieldPoseMemo *memo,
                                   DObj *root_dobj)
{
    const u16 *tracks = (const u16 *)(const void *)
        ((const u8 *)memo + NDS_SHIELD_POSE_MEMO_HEAD_BYTES);
    const f32 *frames = (const f32 *)(const void *)
        ((const u8 *)tracks + ((((u32)memo->rows * sizeof(u16)) + 3u) & ~3u));
    const f32 *values;
    DObj *current;
    u32 rows = 0u;
    u32 posed = 0u;
    u32 row;

    for (current = root_dobj; current != NULL;
         current = lbCommonGetTreeDObjNextFromRoot(current, root_dobj))
    {
        if (rows >= memo->rows)
        {
            return FALSE;
        }
        if ((tracks[rows] & NDS_SHIELD_POSE_MEMO_POSED) != 0u)
        {
            posed++;
        }
        rows++;
    }
    if (rows != memo->rows)
    {
        return FALSE;
    }
    values = frames + posed;
    row = 0u;
    for (current = root_dobj; current != NULL;
         current = lbCommonGetTreeDObjNextFromRoot(current, root_dobj), row++)
    {
        u32 mask = tracks[row];
        u32 bit;

        if ((mask & NDS_SHIELD_POSE_MEMO_POSED) == 0u)
        {
            current->anim_wait = AOBJ_ANIM_NULL;
            continue;
        }
        for (bit = 0u; bit < 10u; bit++)
        {
            if ((mask & (1u << bit)) != 0u)
            {
                *ndsShieldPoseTrackField(current, bit) = *values++;
            }
        }
        current->anim_frame = *frames++;
        current->anim_joint.event32 = NULL;
        if (current->aobj != NULL)
        {
            gcRemoveAObjFromDObj(current);
        }
        current->anim_wait = AOBJ_ANIM_NULL;
    }
    root_dobj->parent_gobj->anim_frame = memo->parent_anim_frame;
    return TRUE;
}

/* After a full apply: what it wrote, for the next tick of the same pose. */
static void ndsShieldPoseMemoFill(NDSShieldPoseMemo *memo, DObj *root_dobj,
                                  const u16 *row_tracks, u32 rows,
                                  u32 package, u32 sector, f32 angle)
{
    u16 *tracks = (u16 *)(void *)((u8 *)memo + NDS_SHIELD_POSE_MEMO_HEAD_BYTES);
    f32 *frames = (f32 *)(void *)
        ((u8 *)tracks + (((rows * sizeof(u16)) + 3u) & ~3u));
    f32 *values;
    f32 *end = (f32 *)(void *)((u8 *)memo + NDS_SHIELD_POSE_MEMO_BYTES);
    DObj *current;
    u32 posed = 0u;
    u32 row;

    memo->valid = 0u;
    if ((rows == 0u) || (rows > NDS_SHIELD_POSE_MEMO_ROWS) ||
        ((root_dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM) != 0u))
    {
        return;
    }
    for (row = 0u; row < rows; row++)
    {
        if ((row_tracks[row] == 0xffffu) || ((row_tracks[row] & 0x8u) != 0u))
        {
            return;     /* unread script, or a TraI track (never decoded) */
        }
        tracks[row] = row_tracks[row];
        if ((row_tracks[row] & NDS_SHIELD_POSE_MEMO_POSED) != 0u)
        {
            posed++;
        }
    }
    values = frames + posed;
    if (values > end)
    {
        return;
    }
    row = 0u;
    for (current = root_dobj; (current != NULL) && (row < rows);
         current = lbCommonGetTreeDObjNextFromRoot(current, root_dobj), row++)
    {
        u32 mask = tracks[row];
        u32 bit;

        if ((mask & NDS_SHIELD_POSE_MEMO_POSED) == 0u)
        {
            continue;
        }
        for (bit = 0u; bit < 10u; bit++)
        {
            if ((mask & (1u << bit)) != 0u)
            {
                if (values >= end)
                {
                    return;
                }
                *values++ = *ndsShieldPoseTrackField(current, bit);
            }
        }
        *frames++ = current->anim_frame;
    }
    memo->gobj = root_dobj->parent_gobj;
    memo->heap_generation = gNdsTaskmanHeapGeneration;
    memo->angle_bits = ndsShieldPoseF32Bits(angle);
    memo->speed_bits = ndsShieldPoseF32Bits(root_dobj->anim_speed);
    memo->parent_anim_frame = root_dobj->parent_gobj->anim_frame;
    memo->rows = (u16)rows;
    memo->package = (u8)package;
    memo->sector = (u8)sector;
    memo->valid = 1u;
    gNdsShieldPoseMemoFills++;
}

s32 ndsShieldPoseTryApplyAll(DObj *root_dobj, f32 angle)
{
    FTStruct *fp;
    NDSShieldPoseMemo *memo;
    u16 row_tracks[NDS_SHIELD_POSE_MEMO_ROWS];
    u32 rows = 0u;
    NDSShieldPoseView view;
    s32 package;
    u32 sector;
    u32 joint = 0u;
    u32 hint = 0u;
    DObj *current;

    if ((root_dobj == NULL) || (root_dobj->parent_gobj == NULL))
    {
        return 0;
    }
    fp = ftGetStruct(root_dobj->parent_gobj);
    package = ndsShieldPosePackageForFighter(fp);
    if (package < 0)
    {
        return 0;
    }
    sector = (u32)fp->status_vars.common.guard.angle_i;
    if (ndsShieldPoseNativeSelected(fp, (u32)package, sector, &view) == FALSE)
    {
        return 0;
    }
    if (ndsShieldPoseRefreshBaseAll(&view) == FALSE)
    {
        gNdsShieldPoseDecodeFailCount++;
        return -1;
    }
    memo = ndsShieldPoseMemoFor(fp);
    if ((memo != NULL) &&
        (ndsShieldPoseMemoMatches(memo, root_dobj, (u32)package, sector,
                                  angle) != FALSE) &&
        (ndsShieldPoseMemoApply(memo, root_dobj) != FALSE))
    {
        gNdsShieldPoseMemoHits++;
        sNdsShieldPoseBatchGObj = root_dobj->parent_gobj;
        sNdsShieldPoseBatchRoot = root_dobj;
        sNdsShieldPoseBatchPackage = (u8)package;
        sNdsShieldPoseBatchSector = (u8)sector;
        gNdsShieldPoseAllApplyCount++;
        return 1;
    }
    if (memo != NULL)
    {
        memo->valid = 0u;
    }
    root_dobj->parent_gobj->anim_frame = angle;
    for (current = root_dobj; current != NULL;
         current = lbCommonGetTreeDObjNextFromRoot(current, root_dobj), joint++)
    {
        u16 handle;
        u32 tracks = 0u;

        if (rows < NDS_SHIELD_POSE_MEMO_ROWS)
        {
            row_tracks[rows] = 0u;
        }
        rows++;

#if NDS_P4
        /* A static part takes no guard row and no animation
         * (lbCommonAddDObjAnimJointAll_extended_). */
        if (ndsP4StaticPartDObj(current) != FALSE)
        {
            current->anim_wait = AOBJ_ANIM_NULL;
            joint--;
            continue;
        }
#endif
        if (joint >= view.h->joint_count)
        {
            /* Past the table: its NULL terminator (ndsShieldPoseApplyNone),
             * as lbCommonAddDObjAnimJointAll plays a NULL entry. */
            current->anim_wait = AOBJ_ANIM_NULL;
            continue;
        }
        handle = view.handles[(sector * view.h->joint_count) + joint];
        if (handle == NDS_SHIELD_POSE_NULL_HANDLE)
        {
            current->anim_wait = AOBJ_ANIM_NULL;
            continue;
        }
        if (ndsShieldPoseApplyScript(&view, current, handle, angle,
                                     &tracks,
                                     ndsShieldPoseJointScale(fp, current,
                                                             &hint)) == FALSE)
        {
            current->anim_joint.event32 = NULL;
            current->anim_wait = AOBJ_ANIM_NULL;
            gNdsShieldPoseDecodeFailCount++;
            ndsShieldPoseMarkBatchFailure(root_dobj->parent_gobj);
            return -1;
        }
        if (rows <= NDS_SHIELD_POSE_MEMO_ROWS)
        {
            row_tracks[rows - 1u] = (tracks > 0x3ffu) ? 0xffffu :
                (u16)(NDS_SHIELD_POSE_MEMO_POSED | tracks);
        }
    }
    if (joint < view.h->joint_count)
    {
        gNdsShieldPoseDecodeFailCount++;
        ndsShieldPoseMarkBatchFailure(root_dobj->parent_gobj);
        return -1;
    }
    if ((memo != NULL) && (rows <= NDS_SHIELD_POSE_MEMO_ROWS))
    {
        ndsShieldPoseMemoFill(memo, root_dobj, row_tracks, rows,
                              (u32)package, sector, angle);
    }
    sNdsShieldPoseBatchGObj = root_dobj->parent_gobj;
    sNdsShieldPoseBatchRoot = root_dobj;
    sNdsShieldPoseBatchPackage = (u8)package;
    sNdsShieldPoseBatchSector = (u8)sector;
    gNdsShieldPoseAllApplyCount++;
    return 1;
}

s32 ndsShieldPoseTryPlayBatch(GObj *fighter_gobj)
{
    NDSShieldPoseView view;
    DObj *current;
    u32 joint = 0u;

    if ((fighter_gobj == NULL) || (fighter_gobj != sNdsShieldPoseBatchGObj))
    {
        return 0;
    }
    if ((sNdsShieldPoseBatchRoot == NULL) ||
        (ndsShieldPoseGetView(sNdsShieldPoseBatchPackage, &view) == FALSE))
    {
        sNdsShieldPoseBatchGObj = NULL;
        return -1;
    }
    sNdsShieldPoseBatchGObj = NULL;
    ftMainPlayAnimEventsAll(fighter_gobj);
    for (current = sNdsShieldPoseBatchRoot; current != NULL;
         current = lbCommonGetTreeDObjNextFromRoot(current,
                                                   sNdsShieldPoseBatchRoot),
         joint++)
    {
        u16 handle;

#if NDS_P4
        if (ndsP4StaticPartDObj(current) != FALSE)
        {
            current->anim_wait = AOBJ_ANIM_NULL;
            joint--;
            continue;
        }
#endif
        if (joint >= view.h->joint_count)
        {
            current->anim_wait = AOBJ_ANIM_NULL;
            continue;
        }
        handle = view.handles[
            ((u32)sNdsShieldPoseBatchSector * view.h->joint_count) + joint];
        current->anim_wait = (handle == NDS_SHIELD_POSE_NULL_HANDLE) ?
            AOBJ_ANIM_NULL : AOBJ_ANIM_END;
    }
    sNdsShieldPoseBatchRoot = NULL;
    if (joint < view.h->joint_count)
    {
        gNdsShieldPoseDecodeFailCount++;
        return -1;
    }
    gNdsShieldPoseBatchPlayCount++;
    return 1;
}

#else

s32 ndsShieldPoseReplacesSourceFile(s32 fkind)
{
    (void)fkind;
    return FALSE;
}

s32 ndsShieldPoseServesDependency(u32 owner_asset, u32 dep_asset)
{
    (void)owner_asset; (void)dep_asset;
    return FALSE;
}

s32 ndsShieldPoseNamesDependency(u32 owner_asset, u32 dep_asset)
{
    (void)owner_asset; (void)dep_asset;
    return FALSE;
}

s32 ndsShieldPosePatchCompactMain(s32 fkind, void *main_data, u32 main_bytes)
{
    (void)fkind; (void)main_data; (void)main_bytes;
    return 0;
}

s32 ndsShieldPoseResolveExternalFixup(u32 owner_asset, u32 dep_asset,
                                      u32 target_offset, void **resolved)
{
    (void)owner_asset; (void)dep_asset; (void)target_offset; (void)resolved;
    return 0;
}

s32 ndsShieldPoseTryApplySingle(DObj *dobj, f32 angle)
{
    (void)dobj; (void)angle; return 0;
}

s32 ndsShieldPoseTryApplyAll(DObj *root_dobj, f32 angle)
{
    (void)root_dobj; (void)angle; return 0;
}

s32 ndsShieldPoseTryPlayBatch(GObj *fighter_gobj)
{
    (void)fighter_gobj; return 0;
}

#endif

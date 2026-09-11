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
#include <nds/nds_startup.h>
#include <sys/obj.h>
#include <sys/taskman.h>

__attribute__((used)) volatile u32 gNdsShieldPoseNativeFixupCount;
__attribute__((used)) volatile u32 gNdsShieldPoseNativeFixupRejectCount;
__attribute__((used)) volatile u32 gNdsShieldPoseSingleApplyCount;
__attribute__((used)) volatile u32 gNdsShieldPoseAllApplyCount;
__attribute__((used)) volatile u32 gNdsShieldPoseBatchPlayCount;
__attribute__((used)) volatile u32 gNdsShieldPoseDecodeFailCount;
__attribute__((used)) volatile u32 gNdsShieldPoseLoadCount;
__attribute__((used)) volatile u32 gNdsShieldPoseLoadFailCount;
__attribute__((used)) volatile u32 gNdsShieldPoseResidentBytes;

#if NDS_P2_DONKEY || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_KIRBY || \
    NDS_P2_CAPTAIN || NDS_P2_PIKACHU || NDS_P2_PURIN

#include <nds/generated/nds_shield_pose_assets.generated.h>

#define NDS_SHIELD_POSE_CODE __attribute__((noinline, optimize("Os")))

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
} NDSShieldPoseAssetDesc;

#define NDS_SHIELD_POSE_ROW(fkind_, main_, shield_, bytes_, dobj_, \
                            t0_, t1_, t2_, t3_, t4_, t5_, t6_, t7_) \
    { (s16)(fkind_), (u16)(main_), (u16)(shield_), (u16)(bytes_), \
      (u16)(dobj_), { (u16)(t0_), (u16)(t1_), (u16)(t2_), (u16)(t3_), \
                       (u16)(t4_), (u16)(t5_), (u16)(t6_), (u16)(t7_) } },
static const NDSShieldPoseAssetDesc sNdsShieldPoseAssets[] = {
    NDS_SHIELD_POSE_ASSET_ROWS(NDS_SHIELD_POSE_ROW)
};
#undef NDS_SHIELD_POSE_ROW

_Static_assert(ARRAY_COUNT(sNdsShieldPoseAssets) == NDS_SHIELD_POSE_ASSET_COUNT,
               "ShieldPose descriptor count drifted");

typedef struct NDSShieldPoseView
{
    const NDSShieldPoseBlobHeader *h;
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

extern DObj *lbCommonGetTreeDObjNextFromRoot(DObj *dobj, DObj *root);
extern void ftMainPlayAnimEventsAll(GObj *fighter_gobj);
extern void ndsBaseGcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                                      f32 anim_frame);

/* Source guard only reads translate/rotate through dobj_lookup.  This 32-row
 * maximum is shared because guard evaluation is synchronous; script scratch is
 * stack-local so it never consumes the taskman arena or static BSS. */
static DObjDesc sNdsShieldPoseDObjScratch[NDS_SHIELD_POSE_MAX_BASE_COUNT]
    __attribute__((section(".sbss.shield_pose")));
static void *sNdsShieldPoseLoaded[NDS_SHIELD_POSE_ASSET_COUNT];
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
    for (i = 0u; i < NDS_SHIELD_POSE_ASSET_COUNT; i++)
    {
        sNdsShieldPoseLoaded[i] = NULL;
    }
    sNdsShieldPoseLoadedGeneration = gNdsTaskmanHeapGeneration;
    gNdsShieldPoseResidentBytes = 0u;
}

static s32 NDS_SHIELD_POSE_CODE ndsShieldPoseValidateBlob(
    u32 package, const void *data, NDSShieldPoseView *view)
{
    const NDSShieldPoseAssetDesc *desc;
    const NDSShieldPoseBlobHeader *h;
    const u8 *base = data;

    if ((package >= NDS_SHIELD_POSE_ASSET_COUNT) ||
        (data == NULL) || (view == NULL))
    {
        return FALSE;
    }
    desc = &sNdsShieldPoseAssets[package];
    h = (const NDSShieldPoseBlobHeader *)data;
    if ((h->magic != NDS_SHIELD_POSE_BLOB_MAGIC) ||
        (h->version != NDS_SHIELD_POSE_BLOB_VERSION) ||
        (h->header_bytes != NDS_SHIELD_POSE_BLOB_HEADER_BYTES) ||
        (h->total_bytes != desc->blob_bytes) ||
        (h->base_count > NDS_SHIELD_POSE_MAX_BASE_COUNT) ||
        (h->scratch_words > NDS_SHIELD_POSE_MAX_SCRATCH_WORDS) ||
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

static void *NDS_SHIELD_POSE_CODE ndsShieldPoseLoad(u32 package)
{
    const NDSShieldPoseAssetDesc *desc;
    NDSShieldPoseView view;
    char path[] = "nitro:/fighters/shield_pose/00.bin";
    void *data;
    const u32 digit_at = sizeof("nitro:/fighters/shield_pose/") - 1u;

    if (package >= NDS_SHIELD_POSE_ASSET_COUNT)
    {
        return NULL;
    }
    ndsShieldPoseResetGeneration();
    if (sNdsShieldPoseLoaded[package] != NULL)
    {
        return sNdsShieldPoseLoaded[package];
    }
    desc = &sNdsShieldPoseAssets[package];
    data = syTaskmanMalloc(desc->blob_bytes, 4u);
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

static s32 ndsShieldPosePackageForFKind(s32 fkind)
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
    return -1;
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
        (fp->is_have_translate_scale != FALSE) ||
        (ndsShieldPoseGetView(package, view) == FALSE))
    {
        return FALSE;
    }
    expected = (const void *)&view->handles[sector * view->h->joint_count];
    return (((const void *)fp->attr->shield_anim_joints[sector] == expected) &&
            ((const void *)fp->attr->dobj_lookup ==
             (const void *)sNdsShieldPoseDObjScratch)) ? TRUE : FALSE;
}

static s32 NDS_SHIELD_POSE_CODE ndsShieldPoseReadValue(
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
        return FALSE;
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

static __attribute__((noinline, optimize("Os"))) f32
ndsShieldPoseQToF32(s32 value, u32 frac)
{
    return ndsR2FixedToF32(value << (NDS_R2_AQ_VF - frac), NDS_R2_AQ_VF);
}

static s32 NDS_SHIELD_POSE_CODE ndsShieldPoseUnpackScript(
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
    for (guard = 0u; guard < 64u; guard++, command_at++)
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

static s32 NDS_SHIELD_POSE_CODE ndsShieldPoseApplyScript(
    const NDSShieldPoseView *view, DObj *dobj, u16 handle, f32 angle)
{
    AObjEvent32 scratch[NDS_SHIELD_POSE_MAX_SCRATCH_WORDS];

    if ((dobj == NULL) ||
        (ndsShieldPoseUnpackScript(view, handle, scratch) == FALSE))
    {
        return FALSE;
    }
    ndsBaseGcAddDObjAnimJoint(dobj, scratch, angle);
    gcParseDObjAnimJoint(dobj);
    gcPlayDObjAnimJoint(dobj);
    dobj->anim_joint.event32 = NULL;
    dobj->anim_wait = AOBJ_ANIM_NULL;
    return TRUE;
}

static s32 NDS_SHIELD_POSE_CODE ndsShieldPoseRefreshBaseRow(
    const NDSShieldPoseView *view, u32 row)
{
    const u8 *src;
    const u8 *end = view->base_data + view->h->base_data_bytes;
    DObjDesc *dst;
    f32 *out;
    u32 mask;
    u32 i;

    if ((row >= view->h->base_count) ||
        (view->base_offsets[row] >= view->h->base_data_bytes))
    {
        return FALSE;
    }
    src = view->base_data + view->base_offsets[row];
    if (src >= end)
    {
        return FALSE;
    }
    dst = &sNdsShieldPoseDObjScratch[row];
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
        return -1;
    }
    desc = &sNdsShieldPoseAssets[package];
    if (target_offset == desc->dobj_offset)
    {
        *resolved = sNdsShieldPoseDObjScratch;
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

s32 ndsShieldPoseTryApplySingle(DObj *dobj, f32 angle)
{
    FTStruct *fp;
    NDSShieldPoseView view;
    s32 package;
    u32 sector;
    s32 i;
    s32 joint_num = 0;
    u16 handle;

    if ((dobj == NULL) || (dobj->parent_gobj == NULL))
    {
        return 0;
    }
    fp = ftGetStruct(dobj->parent_gobj);
    package = (fp != NULL) ? ndsShieldPosePackageForFKind(fp->fkind) : -1;
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
        if (fp->joints[i] != NULL)
        {
            joint_num++;
        }
    }
    joint_num--;
    if ((joint_num < 0) || ((u32)joint_num >= view.h->joint_count) ||
        ((u32)joint_num >= view.h->base_count))
    {
        gNdsShieldPoseDecodeFailCount++;
        return -1;
    }
    handle = view.handles[(sector * view.h->joint_count) + (u32)joint_num];
    if ((handle == NDS_SHIELD_POSE_NULL_HANDLE) ||
        (ndsShieldPoseApplyScript(&view, dobj, handle, angle) == FALSE) ||
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

s32 ndsShieldPoseTryApplyAll(DObj *root_dobj, f32 angle)
{
    FTStruct *fp;
    NDSShieldPoseView view;
    s32 package;
    u32 sector;
    u32 joint = 0u;
    DObj *current;

    if ((root_dobj == NULL) || (root_dobj->parent_gobj == NULL))
    {
        return 0;
    }
    fp = ftGetStruct(root_dobj->parent_gobj);
    package = (fp != NULL) ? ndsShieldPosePackageForFKind(fp->fkind) : -1;
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
    root_dobj->parent_gobj->anim_frame = angle;
    for (current = root_dobj; current != NULL;
         current = lbCommonGetTreeDObjNextFromRoot(current, root_dobj), joint++)
    {
        u16 handle;

        if (joint >= view.h->joint_count)
        {
            gNdsShieldPoseDecodeFailCount++;
            ndsShieldPoseMarkBatchFailure(root_dobj->parent_gobj);
            return -1;
        }
        handle = view.handles[(sector * view.h->joint_count) + joint];
        if (handle == NDS_SHIELD_POSE_NULL_HANDLE)
        {
            current->anim_wait = AOBJ_ANIM_NULL;
            continue;
        }
        if (ndsShieldPoseApplyScript(&view, current, handle, angle) == FALSE)
        {
            current->anim_joint.event32 = NULL;
            current->anim_wait = AOBJ_ANIM_NULL;
            gNdsShieldPoseDecodeFailCount++;
            ndsShieldPoseMarkBatchFailure(root_dobj->parent_gobj);
            return -1;
        }
    }
    if (joint != view.h->joint_count)
    {
        gNdsShieldPoseDecodeFailCount++;
        ndsShieldPoseMarkBatchFailure(root_dobj->parent_gobj);
        return -1;
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

        if (joint >= view.h->joint_count)
        {
            gNdsShieldPoseDecodeFailCount++;
            sNdsShieldPoseBatchRoot = NULL;
            return -1;
        }
        handle = view.handles[
            ((u32)sNdsShieldPoseBatchSector * view.h->joint_count) + joint];
        current->anim_wait = (handle == NDS_SHIELD_POSE_NULL_HANDLE) ?
            AOBJ_ANIM_NULL : AOBJ_ANIM_END;
    }
    sNdsShieldPoseBatchRoot = NULL;
    if (joint != view.h->joint_count)
    {
        gNdsShieldPoseDecodeFailCount++;
        return -1;
    }
    gNdsShieldPoseBatchPlayCount++;
    return 1;
}

#else

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

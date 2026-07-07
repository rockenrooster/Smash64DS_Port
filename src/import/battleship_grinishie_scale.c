/* Bounded original Inishie / Mushroom Kingdom scale update import.
 *
 * Source reference:
 * decomp/BattleShip-main/decomp/src/gr/grcommon/grinishie.c
 *
 * This file imports only the moving-scale update path needed by the current
 * proof. Full Inishie ground setup, Pakkun, Power Block, items, and stage
 * model loading remain deferred until the StageInishie model assets are
 * staged.
 */
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <macros.h>
#include <nds/nds_renderer.h>
#include "nds_build_config.h"
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/objanim.h>
#include <sys/objman.h>
#include <stdio.h>

#ifndef NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
#define NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP 0
#endif

#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llGRInishieMapScaleDObjDesc NDS_RELOC_LVALUE(0x380u)
#define llGRInishieMapMapHead NDS_RELOC_LVALUE(0x5f0u)
#define llGRInishieMapScaleRetractAnimJoint NDS_RELOC_LVALUE(0x734u)

#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
void gcDrawDObjDLHead0(GObj *gobj);
void gcDrawDObjTreeForGObj(GObj *gobj);
void gcPlayAnimAll(GObj *gobj);
extern s32 sGCCommonsActiveNum;
extern u32 sGCDrawsActiveNum;
extern volatile u32 gNdsStageInishieScaleLoopSourceSetupStep;
extern volatile u32 gNdsStageInishieScaleLoopSourceSetupGObjCountBefore;
extern volatile u32 gNdsStageInishieScaleLoopSourceSetupDObjCountBefore;
extern volatile u32 gNdsStageInishieScaleLoopSourceSetupGObjReadyMask;
#endif

u16 dGRInishieScaleMapObjKinds[/* */] = { nMPMapObjKindScaleL,
                                          nMPMapObjKindScaleR };

u8 dGRInishieScaleLineGroups[/* */] = { 0x01, 0x02 };

#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
DObjTransformTypes dGRInishieScaleTransformKinds[/* */] = {
    { nGCMatrixKindTra, nGCMatrixKindNull, 0x01 },
    { nGCMatrixKindTra, nGCMatrixKindNull, 0x01 },
    { nGCMatrixKindTra, nGCMatrixKindNull, 0x00 },
    { nGCMatrixKindTra, nGCMatrixKindNull, 0x01 },
    { nGCMatrixKindTra, nGCMatrixKindNull, 0x00 },
};

#define NDS_GRINISHIE_SCALE_DOBJ_OFFSET 0x380u
#define NDS_GRINISHIE_SCALE_MAP_HEAD_OFFSET 0x5f0u
#define NDS_GRINISHIE_SCALE_RETRACT_OFFSET 0x734u
#define NDS_GRINISHIE_SCALE_RAW_DOBJ_STRIDE 0x2cu
#define NDS_GRINISHIE_SCALE_RAW_SIZE 5136u
#define NDS_GRINISHIE_SCALE_RAW_PATH "nitro:/relocdata/us/155.vpk0.bin"
#define NDS_GRINISHIE_SCALE_DL_01C8_COUNT 18u
#define NDS_GRINISHIE_SCALE_DL_0258_COUNT 18u
#define NDS_GRINISHIE_SCALE_DL_02E8_COUNT 3u
#define NDS_GRINISHIE_SCALE_DL_0300_COUNT 5u
#define NDS_GRINISHIE_SCALE_DL_0328_COUNT 3u
#define NDS_GRINISHIE_SCALE_DL_0340_COUNT 8u
#define NDS_GRINISHIE_SCALE_MAP_HEAD_DL_COUNT 39u
#define NDS_GRINISHIE_G_ENDDL 0xdf000000u
#define NDS_GRINISHIE_SOURCE_MASK_SAFE_ENDDL (1u << 8)
#define NDS_GRINISHIE_SOURCE_MASK_TYPED_ANIM (1u << 9)
#define NDS_GRINISHIE_SOURCE_MASK_RAW_LOADED (1u << 10)
#define NDS_GRINISHIE_SOURCE_MASK_RAW_DOBJ (1u << 11)
#define NDS_GRINISHIE_SOURCE_MASK_RAW_DL (1u << 12)
#define NDS_GRINISHIE_SOURCE_MASK_RAW_ANIM (1u << 13)
#define NDS_GRINISHIE_SOURCE_MASK_NATIVE_DL (1u << 14)
#define NDS_GRINISHIE_SOURCE_MASK_NATIVE_DOBJ_DL (1u << 15)
#define NDS_GRINISHIE_SOURCE_MASK_NATIVE_MAP_DL (1u << 16)

typedef struct NDSGRInishieScaleSourceAsset
{
    u8 pad_0x0000[NDS_GRINISHIE_SCALE_DOBJ_OFFSET];
    DObjDesc dobj_descs[6];
    u8 pad_0x0488[NDS_GRINISHIE_SCALE_MAP_HEAD_OFFSET -
                  NDS_GRINISHIE_SCALE_DOBJ_OFFSET -
                  (sizeof(DObjDesc) * 6u)];
    Gfx map_head[NDS_GRINISHIE_SCALE_MAP_HEAD_DL_COUNT];
    u8 pad_0x05f8[NDS_GRINISHIE_SCALE_RETRACT_OFFSET -
                  NDS_GRINISHIE_SCALE_MAP_HEAD_OFFSET -
                  (sizeof(Gfx) * NDS_GRINISHIE_SCALE_MAP_HEAD_DL_COUNT)];
    u32 retract_anim[4];
} NDSGRInishieScaleSourceAsset;

static NDSGRInishieScaleSourceAsset sNdsGRInishieScaleSourceAsset
    __attribute__((aligned(16)));
static u8 sNdsGRInishieScaleRawAsset[NDS_GRINISHIE_SCALE_RAW_SIZE]
    __attribute__((aligned(16)));
static u32 sNdsGRInishieScaleVtx0048[8u * 4u] __attribute__((aligned(16)));
static u32 sNdsGRInishieScaleVtx00C8[4u * 4u] __attribute__((aligned(16)));
static u32 sNdsGRInishieScaleVtx0148[2u * 4u] __attribute__((aligned(16)));
static u32 sNdsGRInishieScaleVtx0168[4u * 4u] __attribute__((aligned(16)));
static u32 sNdsGRInishieScaleVtx01A8[2u * 4u] __attribute__((aligned(16)));
static u32 sNdsGRInishieScaleVtx04F0[12u * 4u] __attribute__((aligned(16)));
static u32 sNdsGRInishieScaleVtx05B0[4u * 4u] __attribute__((aligned(16)));
static Gfx sNdsGRInishieScaleDL01C8[NDS_GRINISHIE_SCALE_DL_01C8_COUNT]
    __attribute__((aligned(16)));
static Gfx sNdsGRInishieScaleDL0258[NDS_GRINISHIE_SCALE_DL_0258_COUNT]
    __attribute__((aligned(16)));
static Gfx sNdsGRInishieScaleDL02E8[NDS_GRINISHIE_SCALE_DL_02E8_COUNT]
    __attribute__((aligned(16)));
static Gfx sNdsGRInishieScaleDL0300[NDS_GRINISHIE_SCALE_DL_0300_COUNT]
    __attribute__((aligned(16)));
static Gfx sNdsGRInishieScaleDL0328[NDS_GRINISHIE_SCALE_DL_0328_COUNT]
    __attribute__((aligned(16)));
static Gfx sNdsGRInishieScaleDL0340[NDS_GRINISHIE_SCALE_DL_0340_COUNT]
    __attribute__((aligned(16)));
static sb32 sNdsGRInishieScaleSourceAssetReady;
static sb32 sNdsGRInishieScaleRawAssetReady;

static u32 ndsGRInishieScaleReadRawBE32(u32 offset)
{
    const u8 *bytes = &sNdsGRInishieScaleRawAsset[offset];

    return ((u32)bytes[0] << 24) | ((u32)bytes[1] << 16) |
           ((u32)bytes[2] << 8) | (u32)bytes[3];
}

static f32 ndsGRInishieScaleReadRawBEF32(u32 offset)
{
    union
    {
        u32 word;
        f32 value;
    } raw;

    raw.word = ndsGRInishieScaleReadRawBE32(offset);
    return raw.value;
}

static sb32 ndsGRInishieScaleLoadRawAsset(void)
{
    FILE *file;
    size_t read_size;

    if (sNdsGRInishieScaleRawAssetReady != FALSE)
    {
        return TRUE;
    }

    file = fopen(NDS_GRINISHIE_SCALE_RAW_PATH, "rb");
    if (file == NULL)
    {
        return FALSE;
    }

    read_size =
        fread(sNdsGRInishieScaleRawAsset, 1, NDS_GRINISHIE_SCALE_RAW_SIZE, file);
    fclose(file);

    if (read_size != NDS_GRINISHIE_SCALE_RAW_SIZE)
    {
        return FALSE;
    }

    sNdsGRInishieScaleRawAssetReady = TRUE;
    gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
        NDS_GRINISHIE_SOURCE_MASK_RAW_LOADED;
    return TRUE;
}

static void ndsGRInishieScaleSetVec3f(Vec3f *dst, f32 x, f32 y, f32 z)
{
    dst->x = x;
    dst->y = y;
    dst->z = z;
}

static void ndsGRInishieScaleSetDObjDesc(DObjDesc *desc, s32 id, void *dl,
                                         f32 tx, f32 ty, f32 tz)
{
    desc->id = id;
    desc->dl = dl;
    ndsGRInishieScaleSetVec3f(&desc->translate, tx, ty, tz);
    ndsGRInishieScaleSetVec3f(&desc->rotate, 0.0F, 0.0F, 0.0F);
    ndsGRInishieScaleSetVec3f(&desc->scale, 1.0F, 1.0F, 1.0F);
}

static void ndsGRInishieScaleSetDObjDescFromRaw(DObjDesc *desc, u32 offset,
                                                void *dl)
{
    desc->id = (s32)ndsGRInishieScaleReadRawBE32(offset + 0x00u);
    desc->dl = dl;
    desc->translate.x = ndsGRInishieScaleReadRawBEF32(offset + 0x08u);
    desc->translate.y = ndsGRInishieScaleReadRawBEF32(offset + 0x0cu);
    desc->translate.z = ndsGRInishieScaleReadRawBEF32(offset + 0x10u);
    desc->rotate.x = ndsGRInishieScaleReadRawBEF32(offset + 0x14u);
    desc->rotate.y = ndsGRInishieScaleReadRawBEF32(offset + 0x18u);
    desc->rotate.z = ndsGRInishieScaleReadRawBEF32(offset + 0x1cu);
    desc->scale.x = ndsGRInishieScaleReadRawBEF32(offset + 0x20u);
    desc->scale.y = ndsGRInishieScaleReadRawBEF32(offset + 0x24u);
    desc->scale.z = ndsGRInishieScaleReadRawBEF32(offset + 0x28u);
}

static sb32 ndsGRInishieScaleValidateRawDObjDesc(void)
{
    static const s32 expected_ids[6] = { 0, 1, 2, 1, 2, DOBJ_ARRAY_MAX };
    s32 i;

    for (i = 0; i < (s32)ARRAY_COUNT(expected_ids); i++)
    {
        u32 offset = NDS_GRINISHIE_SCALE_DOBJ_OFFSET +
                     ((u32)i * NDS_GRINISHIE_SCALE_RAW_DOBJ_STRIDE);

        if ((s32)ndsGRInishieScaleReadRawBE32(offset) != expected_ids[i])
        {
            return FALSE;
        }
    }

    if (ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_DOBJ_OFFSET + 0x0cu) !=
        0x44fb4000u)
    {
        return FALSE;
    }
    if (ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_DOBJ_OFFSET + 0x20u) !=
        0x3f800000u)
    {
        return FALSE;
    }
    if (ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_DOBJ_OFFSET +
                                     (5u * NDS_GRINISHIE_SCALE_RAW_DOBJ_STRIDE) +
                                     0x20u) != 0u)
    {
        return FALSE;
    }

    gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
        NDS_GRINISHIE_SOURCE_MASK_RAW_DOBJ;
    return TRUE;
}

static sb32 ndsGRInishieScaleValidateRawMapHead(void)
{
    if (ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_MAP_HEAD_OFFSET) !=
        0xe7000000u)
    {
        return FALSE;
    }
    if (ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_MAP_HEAD_OFFSET + 0x08u) !=
        0xd9fdffffu)
    {
        return FALSE;
    }

    gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
        NDS_GRINISHIE_SOURCE_MASK_RAW_DL;
    return TRUE;
}

static sb32 ndsGRInishieScaleValidateRawRetractAnim(void)
{
    if (ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_RETRACT_OFFSET + 0x00u) !=
        0x1e010002u)
    {
        return FALSE;
    }
    if (ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_RETRACT_OFFSET + 0x04u) !=
        0x1e000002u)
    {
        return FALSE;
    }
    if (ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_RETRACT_OFFSET + 0x08u) !=
        0x1c000000u)
    {
        return FALSE;
    }

    gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
        NDS_GRINISHIE_SOURCE_MASK_RAW_ANIM;
    return TRUE;
}

static void ndsGRInishieScaleCopyRawWords(u32 *dst, u32 offset, u32 word_count)
{
    u32 i;

    for (i = 0; i < word_count; i++)
    {
        dst[i] = ndsGRInishieScaleReadRawBE32(offset + (i * sizeof(u32)));
    }
}

static void ndsGRInishieScaleCopyRawDL(Gfx *dst, u32 offset, u32 count)
{
    u32 i;

    for (i = 0; i < count; i++)
    {
        dst[i].words.w0 = ndsGRInishieScaleReadRawBE32(offset + (i * 8u));
        dst[i].words.w1 = ndsGRInishieScaleReadRawBE32(offset + (i * 8u) + 4u);
    }
}

static void ndsGRInishieScalePatchDLPointer(Gfx *dl, u32 index, const void *ptr)
{
    dl[index].words.w1 = (u32)(uintptr_t)ptr;
}

static sb32 ndsGRInishieScaleScanNativeDL(const Gfx *dl)
{
    NDSRendererConfig config;
    NDSRendererStats stats;

    config.max_depth = 2u;
    config.max_commands = 96u;
    config.max_list_commands = 64u;
    config.initial_projection = NULL;
    config.initial_modelview = NULL;
    config.initial_geometry_mode = 0u;
    config.validate_range = NULL;
    config.resolve_branch = NULL;
    config.resolve_data = NULL;
    config.user = NULL;

    ndsRendererInitStats(&stats);
    ndsRendererScanDisplayList(dl, &config, &stats);

    return ((stats.blocker == NDS_RENDERER_BLOCKER_NONE) &&
            (stats.triangle_count != 0u)) ? TRUE : FALSE;
}

static sb32 ndsGRInishieScaleSetupNativeDLs(void)
{
    Gfx *map_head = sNdsGRInishieScaleSourceAsset.map_head;

    ndsGRInishieScaleCopyRawWords(sNdsGRInishieScaleVtx0048, 0x0048u,
                                  ARRAY_COUNT(sNdsGRInishieScaleVtx0048));
    ndsGRInishieScaleCopyRawWords(sNdsGRInishieScaleVtx00C8, 0x00c8u,
                                  ARRAY_COUNT(sNdsGRInishieScaleVtx00C8));
    ndsGRInishieScaleCopyRawWords(sNdsGRInishieScaleVtx0148, 0x0148u,
                                  ARRAY_COUNT(sNdsGRInishieScaleVtx0148));
    ndsGRInishieScaleCopyRawWords(sNdsGRInishieScaleVtx0168, 0x0168u,
                                  ARRAY_COUNT(sNdsGRInishieScaleVtx0168));
    ndsGRInishieScaleCopyRawWords(sNdsGRInishieScaleVtx01A8, 0x01a8u,
                                  ARRAY_COUNT(sNdsGRInishieScaleVtx01A8));
    ndsGRInishieScaleCopyRawWords(sNdsGRInishieScaleVtx04F0, 0x04f0u,
                                  ARRAY_COUNT(sNdsGRInishieScaleVtx04F0));
    ndsGRInishieScaleCopyRawWords(sNdsGRInishieScaleVtx05B0, 0x05b0u,
                                  ARRAY_COUNT(sNdsGRInishieScaleVtx05B0));

    ndsGRInishieScaleCopyRawDL(sNdsGRInishieScaleDL01C8, 0x01c8u,
                               ARRAY_COUNT(sNdsGRInishieScaleDL01C8));
    ndsGRInishieScaleCopyRawDL(sNdsGRInishieScaleDL0258, 0x0258u,
                               ARRAY_COUNT(sNdsGRInishieScaleDL0258));
    ndsGRInishieScaleCopyRawDL(sNdsGRInishieScaleDL02E8, 0x02e8u,
                               ARRAY_COUNT(sNdsGRInishieScaleDL02E8));
    ndsGRInishieScaleCopyRawDL(sNdsGRInishieScaleDL0300, 0x0300u,
                               ARRAY_COUNT(sNdsGRInishieScaleDL0300));
    ndsGRInishieScaleCopyRawDL(sNdsGRInishieScaleDL0328, 0x0328u,
                               ARRAY_COUNT(sNdsGRInishieScaleDL0328));
    ndsGRInishieScaleCopyRawDL(sNdsGRInishieScaleDL0340, 0x0340u,
                               ARRAY_COUNT(sNdsGRInishieScaleDL0340));
    ndsGRInishieScaleCopyRawDL(map_head, NDS_GRINISHIE_SCALE_MAP_HEAD_OFFSET,
                               NDS_GRINISHIE_SCALE_MAP_HEAD_DL_COUNT);

    ndsGRInishieScalePatchDLPointer(sNdsGRInishieScaleDL01C8, 16u,
                                    sNdsGRInishieScaleDL0258);
    ndsGRInishieScalePatchDLPointer(sNdsGRInishieScaleDL0258, 6u,
                                    sNdsGRInishieScaleVtx0048);
    ndsGRInishieScalePatchDLPointer(sNdsGRInishieScaleDL0258, 15u,
                                    sNdsGRInishieScaleVtx00C8);
    ndsGRInishieScalePatchDLPointer(sNdsGRInishieScaleDL02E8, 0u,
                                    sNdsGRInishieScaleVtx00C8);
    ndsGRInishieScalePatchDLPointer(sNdsGRInishieScaleDL0300, 2u,
                                    sNdsGRInishieScaleVtx0148);
    ndsGRInishieScalePatchDLPointer(sNdsGRInishieScaleDL0328, 0u,
                                    sNdsGRInishieScaleVtx0168);
    ndsGRInishieScalePatchDLPointer(sNdsGRInishieScaleDL0340, 2u,
                                    sNdsGRInishieScaleVtx01A8);
    ndsGRInishieScalePatchDLPointer(map_head, 18u,
                                    sNdsGRInishieScaleVtx04F0);
    ndsGRInishieScalePatchDLPointer(map_head, 32u,
                                    sNdsGRInishieScaleVtx05B0);

    if ((ndsGRInishieScaleScanNativeDL(sNdsGRInishieScaleDL01C8) == FALSE) ||
        (ndsGRInishieScaleScanNativeDL(sNdsGRInishieScaleDL02E8) == FALSE) ||
        (ndsGRInishieScaleScanNativeDL(sNdsGRInishieScaleDL0300) == FALSE) ||
        (ndsGRInishieScaleScanNativeDL(sNdsGRInishieScaleDL0328) == FALSE) ||
        (ndsGRInishieScaleScanNativeDL(sNdsGRInishieScaleDL0340) == FALSE) ||
        (ndsGRInishieScaleScanNativeDL(map_head) == FALSE))
    {
        return FALSE;
    }

    gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
        NDS_GRINISHIE_SOURCE_MASK_NATIVE_DL;
    return TRUE;
}

static sb32 ndsGRInishieScaleTryUseRawSourceAsset(DObjDesc *descs)
{
    s32 i;
    static Gfx *const dobj_dls[5] = {
        sNdsGRInishieScaleDL01C8,
        sNdsGRInishieScaleDL02E8,
        sNdsGRInishieScaleDL0300,
        sNdsGRInishieScaleDL0328,
        sNdsGRInishieScaleDL0340,
    };

    if (ndsGRInishieScaleLoadRawAsset() == FALSE)
    {
        return FALSE;
    }
    if (ndsGRInishieScaleValidateRawDObjDesc() == FALSE)
    {
        return FALSE;
    }
    if (ndsGRInishieScaleValidateRawMapHead() == FALSE)
    {
        return FALSE;
    }
    if (ndsGRInishieScaleValidateRawRetractAnim() == FALSE)
    {
        return FALSE;
    }
    if (ndsGRInishieScaleSetupNativeDLs() == FALSE)
    {
        return FALSE;
    }

    for (i = 0; i < 5; i++)
    {
        ndsGRInishieScaleSetDObjDescFromRaw(
            &descs[i],
            NDS_GRINISHIE_SCALE_DOBJ_OFFSET +
                ((u32)i * NDS_GRINISHIE_SCALE_RAW_DOBJ_STRIDE),
            dobj_dls[i]);
    }
    gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
        NDS_GRINISHIE_SOURCE_MASK_NATIVE_DOBJ_DL;
    ndsGRInishieScaleSetDObjDescFromRaw(
        &descs[5],
        NDS_GRINISHIE_SCALE_DOBJ_OFFSET +
            (5u * NDS_GRINISHIE_SCALE_RAW_DOBJ_STRIDE),
        NULL);

    sNdsGRInishieScaleSourceAsset.retract_anim[0] =
        ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_RETRACT_OFFSET + 0x00u);
    sNdsGRInishieScaleSourceAsset.retract_anim[1] =
        ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_RETRACT_OFFSET + 0x04u);
    sNdsGRInishieScaleSourceAsset.retract_anim[2] =
        ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_RETRACT_OFFSET + 0x08u);
    sNdsGRInishieScaleSourceAsset.retract_anim[3] =
        (u32)(uintptr_t)sNdsGRInishieScaleSourceAsset.retract_anim;
    gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
        NDS_GRINISHIE_SOURCE_MASK_TYPED_ANIM;
    if (sNdsGRInishieScaleSourceAsset.map_head[0].words.w0 !=
        NDS_GRINISHIE_G_ENDDL)
    {
        gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
            NDS_GRINISHIE_SOURCE_MASK_NATIVE_MAP_DL;
    }
    return TRUE;
}

void *ndsGRInishieScaleGetSourceSetupMapHead(void)
{
    DObjDesc *descs = sNdsGRInishieScaleSourceAsset.dobj_descs;
    Gfx *empty_dl = sNdsGRInishieScaleSourceAsset.map_head;

    gNdsStageInishieScaleLoopSourceSetupStep = 20u;
    if (sNdsGRInishieScaleSourceAssetReady == FALSE)
    {
        /* Minimal StageInishieFile3-compatible offsets for grInishieMakeScale.
         * Full generated display-list/texture payloads are absent upstream. */
        empty_dl[0].words.w0 = NDS_GRINISHIE_G_ENDDL;
        empty_dl[0].words.w1 = 0;
        if (empty_dl[0].words.w0 == NDS_GRINISHIE_G_ENDDL)
        {
            gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
                NDS_GRINISHIE_SOURCE_MASK_SAFE_ENDDL;
        }
        gNdsStageInishieScaleLoopSourceSetupStep = 21u;

        if (ndsGRInishieScaleTryUseRawSourceAsset(descs) == FALSE)
        {
            ndsGRInishieScaleSetDObjDesc(&descs[0], 0, empty_dl, 0.0F,
                                         2010.0F, 0.0F);
            ndsGRInishieScaleSetDObjDesc(&descs[1], 1, empty_dl, 420.0F,
                                         -57.751007080078125F, 0.0F);
            ndsGRInishieScaleSetDObjDesc(&descs[2], 2, empty_dl, 0.0F,
                                         -1590.0F, 0.0F);
            ndsGRInishieScaleSetDObjDesc(&descs[3], 1, empty_dl, -420.0F,
                                         -57.750091552734375F, 0.0F);
            ndsGRInishieScaleSetDObjDesc(&descs[4], 2, empty_dl, 0.0F,
                                         -1590.0F, 0.0F);
            descs[5].id = DOBJ_ARRAY_MAX;
            descs[5].dl = NULL;
            ndsGRInishieScaleSetVec3f(&descs[5].translate, 0.0F, 0.0F, 0.0F);
            ndsGRInishieScaleSetVec3f(&descs[5].rotate, 0.0F, 0.0F, 0.0F);
            ndsGRInishieScaleSetVec3f(&descs[5].scale, 0.0F, 0.0F, 0.0F);
            sNdsGRInishieScaleSourceAsset.retract_anim[0] = 0x1e010002u;
            sNdsGRInishieScaleSourceAsset.retract_anim[1] = 0x1e000002u;
            sNdsGRInishieScaleSourceAsset.retract_anim[2] = 0x1c000000u;
            sNdsGRInishieScaleSourceAsset.retract_anim[3] =
                (u32)(uintptr_t)sNdsGRInishieScaleSourceAsset.retract_anim;
            gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
                NDS_GRINISHIE_SOURCE_MASK_TYPED_ANIM;
        }
        gNdsStageInishieScaleLoopSourceSetupStep = 22u;

        sNdsGRInishieScaleSourceAssetReady = TRUE;
    }
    gNdsStageInishieScaleLoopSourceSetupStep = 23u;
    return &sNdsGRInishieScaleSourceAsset;
}
#else
void *ndsGRInishieScaleGetSourceSetupMapHead(void)
{
    return NULL;
}
#endif

enum grInishieScaleStatus
{
    nGRInishieScaleStatusWait,
    nGRInishieScaleStatusFall,
    nGRInishieScaleStatusSleep,
    nGRInishieScaleStatusRetract
};

void grInishieScaleUpdateFighterStatsGA(void)
{
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        s32 player = fp->player;

        if (fp->ga == nMPKineticsGround)
        {
            if (gGRCommonStruct.inishie.players_ga[player] !=
                nMPKineticsGround)
            {
                gGRCommonStruct.inishie.players_tt[player] = 1;
            }
            else if (gGRCommonStruct.inishie.players_tt[player] != 0)
            {
                gGRCommonStruct.inishie.players_tt[player]--;
            }
        }
        else
        {
            gGRCommonStruct.inishie.players_tt[player] = 0;
        }

        gGRCommonStruct.inishie.players_ga[player] = fp->ga;

        fighter_gobj = fighter_gobj->link_next;
    }
}

f32 grInishieScaleGetPressure(s32 line_id)
{
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    f32 pressure = 0.0F;

    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if (fp->ga == nMPKineticsGround)
        {
            if ((fp->coll_data.floor_line_id != -2) &&
                (mpCollisionSetDObjNoID(fp->coll_data.floor_line_id) ==
                    line_id))
            {
                f32 weight = (1.0F - fp->attr->weight) + 1.4F;

                if (gGRCommonStruct.inishie.players_tt[fp->player] != 0)
                {
                    pressure += (weight * 8.0F);
                }
                else
                {
                    pressure += weight;
                }
            }
        }
        fighter_gobj = fighter_gobj->link_next;
    }
    return pressure;
}

void grInishieScaleUpdateWait(void)
{
    DObj *l_dobj;
    DObj *r_dobj;
    f32 l_weight;
    f32 r_weight;
    f32 alt;
    sb32 ud;

    grInishieScaleUpdateFighterStatsGA();

    l_weight = grInishieScaleGetPressure(dGRInishieScaleLineGroups[0]);
    r_weight = grInishieScaleGetPressure(dGRInishieScaleLineGroups[1]);

    if ((l_weight == 0.0F) && (r_weight == 0.0F))
    {
        if (gGRCommonStruct.inishie.splat_alt != 0.0F)
        {
            if (gGRCommonStruct.inishie.splat_alt < 0.0F)
            {
                gGRCommonStruct.inishie.splat_alt += 8.0F;

                if (gGRCommonStruct.inishie.splat_alt > 0.0F)
                {
                    gGRCommonStruct.inishie.splat_alt = 0.0F;
                }
            }
            else
            {
                gGRCommonStruct.inishie.splat_alt -= 8.0F;

                if (gGRCommonStruct.inishie.splat_alt < 0.0F)
                {
                    gGRCommonStruct.inishie.splat_alt = 0.0F;
                }
            }
        }
        gGRCommonStruct.inishie.splat_accelerate = 0.0F;
    }
    else
    {
        gGRCommonStruct.inishie.splat_accelerate += (r_weight - l_weight);

        if ((l_weight != 0.0F) && (r_weight != 0.0F) &&
            (gGRCommonStruct.inishie.splat_accelerate != 0.0F))
        {
            gGRCommonStruct.inishie.splat_accelerate *= 0.93F;
        }
        else if (gGRCommonStruct.inishie.splat_accelerate > 0.0F)
        {
            gGRCommonStruct.inishie.splat_accelerate -= 0.9F;

            if (gGRCommonStruct.inishie.splat_accelerate < 0.0F)
            {
                gGRCommonStruct.inishie.splat_accelerate = 0.0F;
            }
        }
        else if (gGRCommonStruct.inishie.splat_accelerate < 0.0F)
        {
            gGRCommonStruct.inishie.splat_accelerate += 0.9F;

            if (gGRCommonStruct.inishie.splat_accelerate > 0.0F)
            {
                gGRCommonStruct.inishie.splat_accelerate = 0.0F;
            }
        }
        gGRCommonStruct.inishie.splat_alt +=
            gGRCommonStruct.inishie.splat_accelerate;
    }
    alt = ABSF(gGRCommonStruct.inishie.splat_alt);

    l_dobj = gGRCommonStruct.inishie.scale[0].platform_dobj;
    r_dobj = gGRCommonStruct.inishie.scale[1].platform_dobj;

    if (alt > 1100.0F)
    {
        ud = 0;

        if (gGRCommonStruct.inishie.splat_alt < 0.0F)
        {
            ud = 1;

            if (gGRCommonStruct.inishie.splat_accelerate != 0.0F)
            {
            }
        }
        gGRCommonStruct.inishie.splat_accelerate = 0.0F;

        if (ud != 0)
        {
            gGRCommonStruct.inishie.splat_alt = -1100.0F;
        }
        else
        {
            gGRCommonStruct.inishie.splat_alt = 1100.0F;
        }

        gGRCommonStruct.inishie.splat_status = nGRInishieScaleStatusFall;

        efManagerSparkleWhiteScaleMakeEffect(&l_dobj->translate.vec.f, 1.0F);
        efManagerSparkleWhiteScaleMakeEffect(&r_dobj->translate.vec.f, 1.0F);
    }
    l_dobj->translate.vec.f.y =
        gGRCommonStruct.inishie.scale[0].platform_base_y +
        gGRCommonStruct.inishie.splat_alt;
    r_dobj->translate.vec.f.y =
        gGRCommonStruct.inishie.scale[1].platform_base_y -
        gGRCommonStruct.inishie.splat_alt;

    gGRCommonStruct.inishie.scale[0].string_dobj->translate.vec.f.y =
        l_dobj->translate.vec.f.y -
        gGRCommonStruct.inishie.scale[0].string_length;
    gGRCommonStruct.inishie.scale[1].string_dobj->translate.vec.f.y =
        r_dobj->translate.vec.f.y -
        gGRCommonStruct.inishie.scale[1].string_length;
}

void grInishieScaleUpdateFall(void)
{
    f32 deadzone;

    gGRCommonStruct.inishie.splat_accelerate += 3.0F;

    if (gGRCommonStruct.inishie.splat_accelerate > 70.0F)
    {
        gGRCommonStruct.inishie.splat_accelerate = 70.0F;
    }
    gGRCommonStruct.inishie.scale[0].platform_dobj->translate.vec.f.y -=
        gGRCommonStruct.inishie.splat_accelerate;
    gGRCommonStruct.inishie.scale[1].platform_dobj->translate.vec.f.y -=
        gGRCommonStruct.inishie.splat_accelerate;

    deadzone = gMPCollisionGroundData->map_bound_bottom + (-1000.0F);

    if ((gGRCommonStruct.inishie.scale[0].platform_dobj->translate.vec.f.y <
            deadzone) &&
        (gGRCommonStruct.inishie.scale[1].platform_dobj->translate.vec.f.y <
            deadzone))
    {
        gGRCommonStruct.inishie.splat_status = nGRInishieScaleStatusSleep;
        gGRCommonStruct.inishie.splat_accelerate = 0.0F;

        mpCollisionSetYakumonoOffID(dGRInishieScaleLineGroups[0]);
        mpCollisionSetYakumonoOffID(dGRInishieScaleLineGroups[1]);

        gGRCommonStruct.inishie.splat_wait = 180;
    }
}

void grInishieScaleUpdateStep(void)
{
    gGRCommonStruct.inishie.splat_wait--;

    if (gGRCommonStruct.inishie.splat_wait == 0)
    {
        gGRCommonStruct.inishie.splat_status =
            nGRInishieScaleStatusRetract;

        gcAddDObjAnimJoint(
            gGRCommonStruct.inishie.scale[0].platform_dobj,
            (AObjEvent32 *)((intptr_t)&llGRInishieMapScaleRetractAnimJoint +
                            (uintptr_t)gGRCommonStruct.inishie.map_head),
            0.0F);
        gcAddDObjAnimJoint(
            gGRCommonStruct.inishie.scale[1].platform_dobj,
            (AObjEvent32 *)((intptr_t)&llGRInishieMapScaleRetractAnimJoint +
                            (uintptr_t)gGRCommonStruct.inishie.map_head),
            0.0F);
    }
}

void grInishieScaleUpdateRetract(void)
{
    DObj *l_dobj;
    DObj *r_dobj;
    sb32 is_complete = FALSE;

    if (gGRCommonStruct.inishie.splat_alt != 0.0F)
    {
        if (gGRCommonStruct.inishie.splat_alt < 0.0F)
        {
            gGRCommonStruct.inishie.splat_alt += 10.0F;

            if (gGRCommonStruct.inishie.splat_alt >= 0.0F)
            {
                is_complete = TRUE;
            }
        }
        else
        {
            gGRCommonStruct.inishie.splat_alt -= 10.0F;

            if (gGRCommonStruct.inishie.splat_alt <= 0.0F)
            {
                is_complete = TRUE;
            }
        }
    }
    l_dobj = gGRCommonStruct.inishie.scale[0].platform_dobj;
    r_dobj = gGRCommonStruct.inishie.scale[1].platform_dobj;

    if (is_complete != FALSE)
    {
        gGRCommonStruct.inishie.splat_alt = 0.0F;

        l_dobj->anim_wait = AOBJ_ANIM_NULL;
        l_dobj->flags = DOBJ_FLAG_NONE;

        mpCollisionSetYakumonoOnID(dGRInishieScaleLineGroups[0]);

        r_dobj->anim_wait = AOBJ_ANIM_NULL;
        r_dobj->flags = DOBJ_FLAG_NONE;

        mpCollisionSetYakumonoOnID(dGRInishieScaleLineGroups[1]);

        gGRCommonStruct.inishie.splat_status = nGRInishieScaleStatusWait;
    }
    l_dobj->translate.vec.f.y =
        gGRCommonStruct.inishie.scale[0].platform_base_y +
        gGRCommonStruct.inishie.splat_alt;
    r_dobj->translate.vec.f.y =
        gGRCommonStruct.inishie.scale[1].platform_base_y -
        gGRCommonStruct.inishie.splat_alt;

    gGRCommonStruct.inishie.scale[0].string_dobj->translate.vec.f.y =
        l_dobj->translate.vec.f.y -
        gGRCommonStruct.inishie.scale[0].string_length;
    gGRCommonStruct.inishie.scale[1].string_dobj->translate.vec.f.y =
        r_dobj->translate.vec.f.y -
        gGRCommonStruct.inishie.scale[1].string_length;
}

void grInishieScaleProcUpdate(GObj *ground_gobj)
{
    (void)ground_gobj;

    switch (gGRCommonStruct.inishie.splat_status)
    {
    case nGRInishieScaleStatusWait:
        grInishieScaleUpdateWait();
        break;

    case nGRInishieScaleStatusFall:
        grInishieScaleUpdateFall();
        break;

    case nGRInishieScaleStatusSleep:
        grInishieScaleUpdateStep();
        break;

    case nGRInishieScaleStatusRetract:
        grInishieScaleUpdateRetract();
        break;
    }
    mpCollisionSetYakumonoPosID(
        dGRInishieScaleLineGroups[0],
        &gGRCommonStruct.inishie.scale[0].platform_dobj->translate.vec.f);
    mpCollisionSetYakumonoPosID(
        dGRInishieScaleLineGroups[1],
        &gGRCommonStruct.inishie.scale[1].platform_dobj->translate.vec.f);
}

#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
void grInishieMakeScale(void)
{
    void *map_head;
    GObj *ground_gobj;
    DObj *map_dobjs[5];
    DObj *platform_dobj;
    s32 i;
    s32 mapobj;
    Vec3f yakumono_pos;

    gNdsStageInishieScaleLoopSourceSetupStep = 3u;
    map_head = gGRCommonStruct.inishie.map_head;
    ground_gobj =
        gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround,
                          GOBJ_PRIORITY_DEFAULT);
    if (ground_gobj != NULL)
    {
        gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |= 1u << 0;
    }
    gNdsStageInishieScaleLoopSourceSetupStep = 4u;

    gcAddGObjDisplay(ground_gobj, gcDrawDObjTreeForGObj, 6,
                     GOBJ_PRIORITY_DEFAULT, ~0);
    gNdsStageInishieScaleLoopSourceSetupStep = 5u;
    grModelSetupGroundDObjs(
        ground_gobj,
        (DObjDesc *)((uintptr_t)map_head +
                     (intptr_t)&llGRInishieMapScaleDObjDesc),
        map_dobjs,
        dGRInishieScaleTransformKinds);
    gNdsStageInishieScaleLoopSourceSetupStep = 6u;

    gGRCommonStruct.inishie.scale[0].string_dobj = map_dobjs[4];
    gGRCommonStruct.inishie.scale[0].string_length =
        map_dobjs[0]->translate.vec.f.y + map_dobjs[3]->translate.vec.f.y;

    gGRCommonStruct.inishie.scale[1].string_dobj = map_dobjs[2];
    gGRCommonStruct.inishie.scale[1].string_length =
        map_dobjs[0]->translate.vec.f.y + map_dobjs[1]->translate.vec.f.y;
    gNdsStageInishieScaleLoopSourceSetupStep = 7u;

    for (i = 0; i < ARRAY_COUNT(gGRCommonStruct.inishie.scale); i++)
    {
        gNdsStageInishieScaleLoopSourceSetupStep = 8u + (u32)i;
        ground_gobj =
            gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround,
                              GOBJ_PRIORITY_DEFAULT);
        if (ground_gobj != NULL)
        {
            gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
                1u << (1 + i);
        }
        gcAddGObjDisplay(ground_gobj, gcDrawDObjDLHead0, 6,
                         GOBJ_PRIORITY_DEFAULT, ~0);

        platform_dobj = gcAddDObjForGObj(
            ground_gobj,
            (void *)((uintptr_t)map_head +
                     (intptr_t)&llGRInishieMapMapHead));
        gGRCommonStruct.inishie.scale[i].platform_dobj = platform_dobj;

        gcAddXObjForDObjFixed(platform_dobj, nGCMatrixKindTra, 0);
        gcAddGObjProcess(ground_gobj, gcPlayAnimAll, nGCProcessKindFunc, 5);

        mpCollisionGetMapObjIDsKind(dGRInishieScaleMapObjKinds[i], &mapobj);
        mpCollisionGetMapObjPositionID(mapobj, &yakumono_pos);

        platform_dobj->translate.vec.f = yakumono_pos;

        gGRCommonStruct.inishie.scale[i].platform_base_y = yakumono_pos.y;

        mpCollisionSetYakumonoOnID(dGRInishieScaleLineGroups[i]);
    }
    gNdsStageInishieScaleLoopSourceSetupStep = 10u;
    gcAddGObjProcess(ground_gobj, grInishieScaleProcUpdate, nGCProcessKindFunc,
                     4);
    gNdsStageInishieScaleLoopSourceSetupStep = 11u;

    gGRCommonStruct.inishie.splat_status = nGRInishieScaleStatusWait;
    gGRCommonStruct.inishie.splat_alt = 0.0F;
    gGRCommonStruct.inishie.splat_accelerate = 0.0F;

    for (i = 0;
         i < (ARRAY_COUNT(gGRCommonStruct.inishie.players_ga) +
              ARRAY_COUNT(gGRCommonStruct.inishie.players_tt)) /
                 2;
         i++)
    {
        gGRCommonStruct.inishie.players_tt[i] = 0;
        gGRCommonStruct.inishie.players_ga[i] = 0;
    }
    gNdsStageInishieScaleLoopSourceSetupStep = 12u;
}
#endif

static DObj *ndsGRInishieScaleMakeProofDObj(f32 x, f32 y)
{
    GObj *gobj =
        gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround,
                          GOBJ_PRIORITY_DEFAULT);
    DObj *dobj;

    if (gobj == NULL)
    {
        return NULL;
    }

    dobj = gcAddDObjForGObj(gobj, NULL);
    if (dobj == NULL)
    {
        return NULL;
    }

    gcAddXObjForDObjFixed(dobj, nGCMatrixKindTra, 0);
    dobj->translate.vec.f.x = x;
    dobj->translate.vec.f.y = y;
    dobj->translate.vec.f.z = 0.0F;
    return dobj;
}

void ndsGRInishieScaleMakeProofShell(void)
{
    static const Vec3f positions[2] = {
        { -417.0F, 363.0F, 0.0F },
        { 420.0F, 362.0F, 0.0F },
    };
    s32 i;

#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
    if (gGRCommonStruct.inishie.map_head != NULL)
    {
        gNdsStageInishieScaleLoopSourceSetupStep = 1u;
        gNdsStageInishieScaleLoopSourceSetupGObjCountBefore =
            (u32)sGCCommonsActiveNum;
        gNdsStageInishieScaleLoopSourceSetupDObjCountBefore =
            sGCDrawsActiveNum;
        grInishieMakeScale();
        gNdsStageInishieScaleLoopSourceSetupStep = 13u;
        gGRCommonStruct.inishie.splat_alt = 80.0F;
        return;
    }
#endif

    gGRCommonStruct.inishie.item_head = NULL;
    gGRCommonStruct.inishie.splat_status = nGRInishieScaleStatusWait;
    gGRCommonStruct.inishie.splat_alt = 80.0F;
    gGRCommonStruct.inishie.splat_accelerate = 0.0F;
    gGRCommonStruct.inishie.splat_wait = 0;

    for (i = 0; i < 4; i++)
    {
        gGRCommonStruct.inishie.players_tt[i] = 0;
        gGRCommonStruct.inishie.players_ga[i] = 0;
    }

    for (i = 0; i < 2; i++)
    {
        gGRCommonStruct.inishie.scale[i].platform_dobj =
            ndsGRInishieScaleMakeProofDObj(positions[i].x, positions[i].y);
        gGRCommonStruct.inishie.scale[i].string_dobj =
            ndsGRInishieScaleMakeProofDObj(positions[i].x,
                                           positions[i].y - 240.0F);
        gGRCommonStruct.inishie.scale[i].string_length = 240.0F;
        gGRCommonStruct.inishie.scale[i].platform_base_y = positions[i].y;
    }
}

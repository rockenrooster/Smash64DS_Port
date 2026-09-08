/* Host-only retired Inishie source-scale preview pipeline.
 *
 * BattleShip source contract (decomp/BattleShip-main/decomp/src/gr/grcommon/
 * grinishie.c): the Mushroom Kingdom seesaw scale is gameplay state --
 * grInishieScaleUpdateFighterStatsGA, grInishieScaleGetPressure,
 * grInishieScaleUpdateWait/Fall/Step/Retract, grInishieScaleProcUpdate and
 * grInishieMakeScale -- driven through map-object positions and moving
 * yakumono collision. The DS port kept a source-scale diagnostic preview
 * beside that gameplay: raw-asset decode of nitro:/relocdata/us/155.vpk0.bin,
 * native display-list reconstruction with generic display-list scans, and a
 * software execute-and-rasterize thumbnail committed through the platform
 * original-DL preview. That preview is retired from every ROM: the ROM side
 * keeps only live gameplay/collision/scale behavior, DObj/material setup,
 * and explicit ndsRendererRecordNativeFailure reporting. This file is the
 * host-only home of the retired implementation so harnesses can still
 * exercise it off-device.
 *
 * Nothing here may enter an ARM9/ARM7/NDS link. The ROM header
 * include/nds/nds_renderer.h poisons the generic interpreter entry points
 * and the software rasterizer in ROM translation units, and the packaging
 * gate rejects any host/graphics_reference input in ROM compiler
 * dependencies.
 */
#if defined(ARM9) || defined(ARM7) || defined(__NDS__)
#error Software Inishie scale preview is host-only
#endif

#include <stddef.h>
#include <stdio.h>

/* Environment (Gfx/DObj/GObj/MObj types, reloc range checks, platform
 * preview staging, renderer reference, fighter DL draw types, scene globals)
 * is provided by the host harness including this file, mirroring
 * src/host/graphics_reference/opening_preview_reference.c and
 * sprite_reference.c. The bodies below are moved verbatim from the retired
 * ROM side; the ROM side now contains only retired stubs that record a
 * native failure and never claim PASS or success for empty rendering.
 */

/* ---- Moved from src/import/battleship_grinishie_scale.c ---- */

void gcDrawDObjDLHead0(GObj *gobj);
void gcDrawDObjTreeForGObj(GObj *gobj);
void gcPlayAnimAll(GObj *gobj);
extern s32 sGCCommonsActiveNum;
extern u32 sGCDrawsActiveNum;
extern volatile u32 gNdsStageInishieScaleLoopSourceSetupStep;
extern volatile u32 gNdsStageInishieScaleLoopSourceSetupGObjCountBefore;
extern volatile u32 gNdsStageInishieScaleLoopSourceSetupDObjCountBefore;
extern volatile u32 gNdsStageInishieScaleLoopSourceSetupGObjReadyMask;

#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llGRInishieMapScaleDObjDesc NDS_RELOC_LVALUE(0x380u)
#define llGRInishieMapMapHead NDS_RELOC_LVALUE(0x5f0u)
#define llGRInishieMapScaleRetractAnimJoint NDS_RELOC_LVALUE(0x734u)

DObjTransformTypes dGRInishieScaleTransformKindsHost[/* */] = {
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

typedef struct NDSGRInishieScaleSourceAssetHost
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
} NDSGRInishieScaleSourceAssetHost;

static NDSGRInishieScaleSourceAssetHost sNdsGRInishieScaleSourceAssetHost
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

/* Retired: native display-list validation scanned generic display lists
 * through the poisoned interpreter. Moved here so the ROM never scans
 * generic display lists. */
static sb32 ndsGRInishieScaleScanNativeDL(const Gfx *dl)
{
    NDSRendererConfig config = {0};
    NDSRendererStats stats;

    config.max_depth = 2u;
    config.max_commands = 96u;
    config.max_list_commands = 64u;
    config.initial_projection = NULL;
    config.initial_modelview = NULL;
    config.initial_geometry_mode = 0u;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_NATIVE;
    config.validate_range = NULL;
    config.immutable_command_span = NULL;
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
    Gfx *map_head = sNdsGRInishieScaleSourceAssetHost.map_head;

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

    sNdsGRInishieScaleSourceAssetHost.retract_anim[0] =
        ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_RETRACT_OFFSET + 0x00u);
    sNdsGRInishieScaleSourceAssetHost.retract_anim[1] =
        ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_RETRACT_OFFSET + 0x04u);
    sNdsGRInishieScaleSourceAssetHost.retract_anim[2] =
        ndsGRInishieScaleReadRawBE32(NDS_GRINISHIE_SCALE_RETRACT_OFFSET + 0x08u);
    sNdsGRInishieScaleSourceAssetHost.retract_anim[3] =
        (u32)(uintptr_t)sNdsGRInishieScaleSourceAssetHost.retract_anim;
    gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
        NDS_GRINISHIE_SOURCE_MASK_TYPED_ANIM;
    if (sNdsGRInishieScaleSourceAssetHost.map_head[0].words.w0 !=
        NDS_GRINISHIE_G_ENDDL)
    {
        gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
            NDS_GRINISHIE_SOURCE_MASK_NATIVE_MAP_DL;
    }
    return TRUE;
}

/* Retired source-scale setup asset provider. Moved here so the ROM stub can
 * report the retired presentation path as a native failure. Host harnesses
 * replay the retired setup below. */
void *ndsGRInishieScaleGetSourceSetupMapHeadHost(void)
{
    DObjDesc *descs = sNdsGRInishieScaleSourceAssetHost.dobj_descs;
    Gfx *empty_dl = sNdsGRInishieScaleSourceAssetHost.map_head;

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
            sNdsGRInishieScaleSourceAssetHost.retract_anim[0] = 0x1e010002u;
            sNdsGRInishieScaleSourceAssetHost.retract_anim[1] = 0x1e000002u;
            sNdsGRInishieScaleSourceAssetHost.retract_anim[2] = 0x1c000000u;
            sNdsGRInishieScaleSourceAssetHost.retract_anim[3] =
                (u32)(uintptr_t)sNdsGRInishieScaleSourceAssetHost.retract_anim;
            gNdsStageInishieScaleLoopSourceSetupGObjReadyMask |=
                NDS_GRINISHIE_SOURCE_MASK_TYPED_ANIM;
        }
        gNdsStageInishieScaleLoopSourceSetupStep = 22u;

        sNdsGRInishieScaleSourceAssetReady = TRUE;
    }
    gNdsStageInishieScaleLoopSourceSetupStep = 23u;
    return &sNdsGRInishieScaleSourceAssetHost;
}

/* Retired source-scale ground setup (BattleShip grinishie.c:345 contract).
 * Moved here so the ROM stub can report the retired presentation path as a
 * native failure. Host harnesses replay the retired setup below. */
void grInishieMakeScaleHost(void)
{
    void *map_head;
    GObj *ground_gobj;
    DObj *map_dobjs[5];
    DObj *platform_dobj;
    s32 i;
    s32 mapobj;
    Vec3f yakumono_pos;
    extern u16 dGRInishieScaleMapObjKinds[];
    extern u8 dGRInishieScaleLineGroups[];

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
        dGRInishieScaleTransformKindsHost);
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

/* ---- Moved from src/port/reloc_backend_mp_collision.c ---- */

/* Retired: source display-list scan probed each scale DObj through the
 * poisoned interpreter. Moved here so the ROM never scans generic display
 * lists. */
static void ndsStageInishieScaleLoopScanSourceDObj(
    DObj *dobj,
    u32 callback_bit,
    u32 scan_bit,
    void (*proc_display)(GObj *))
{
    NDSRendererConfig config = {0};
    NDSRendererStats stats;

    if ((dobj != NULL) && (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->proc_display == proc_display))
    {
        gNdsStageInishieScaleLoopSourceDisplayMask |= 1u << callback_bit;
    }

    if ((dobj == NULL) || (dobj->dl == NULL))
    {
        if (gNdsStageInishieScaleLoopSourceDisplayBlocker == 0u)
        {
            gNdsStageInishieScaleLoopSourceDisplayBlocker =
                NDS_RENDERER_BLOCKER_BAD_BRANCH;
        }
        return;
    }

    config.max_depth = 4u;
    config.max_commands = 256u;
    config.max_list_commands = 128u;
    config.initial_projection = NULL;
    config.initial_modelview = NULL;
    config.initial_geometry_mode = 0u;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = NULL;
    config.immutable_command_span = NULL;
    config.resolve_branch = NULL;
    config.resolve_data = NULL;
    config.user = NULL;
    ndsRendererInitStats(&stats);
    ndsRendererScanDisplayList(dobj->dl, &config, &stats);

    gNdsStageInishieScaleLoopSourceDisplayCount++;
    gNdsStageInishieScaleLoopSourceDisplayCommands += stats.command_count;
    gNdsStageInishieScaleLoopSourceDisplayTriangles += stats.triangle_count;
    gNdsStageInishieScaleLoopSourceTextureMask |= stats.texture_mask;
    gNdsStageInishieScaleLoopSourceTextureCommands +=
        stats.texture_command_count;
    if ((gNdsStageInishieScaleLoopSourceTextureImage == 0u) &&
        (stats.texture_image != 0u))
    {
        gNdsStageInishieScaleLoopSourceTextureImage = stats.texture_image;
    }
    if ((gNdsStageInishieScaleLoopSourceTextureSize == 0u) &&
        ((stats.texture_tile_width != 0u) ||
         (stats.texture_tile_height != 0u)))
    {
        gNdsStageInishieScaleLoopSourceTextureSize =
            (stats.texture_tile_width << 16) | stats.texture_tile_height;
    }

    if (stats.blocker == NDS_RENDERER_BLOCKER_NONE)
    {
        gNdsStageInishieScaleLoopSourceDisplayMask |= 1u << scan_bit;
    }
    else if (gNdsStageInishieScaleLoopSourceDisplayBlocker == 0u)
    {
        gNdsStageInishieScaleLoopSourceDisplayBlocker = stats.blocker;
    }
}

static s32 ndsStageInishieScaleLoopVisitSourcePreviewCommand(
    const NDSRendererCommand *command, void *user)
{
    NDSFighterDLDrawState *state = user;
    u32 op;

    if ((command == NULL) || (state == NULL))
    {
        return FALSE;
    }

    op = command->op;
    switch (op)
    {
    case NDS_FIGHTER_DL_OP_NOOP:
    case NDS_FIGHTER_DL_OP_MODIFYVTX:
        return TRUE;

    case NDS_FIGHTER_DL_OP_VTX:
    {
        u32 v0;
        u32 count;
        const u8 *src;
        u32 i;

        if (ndsGBIDecodeF3DEX2Vtx(command->w0, NDS_FIGHTER_DL_DRAW_MAX_VTX,
                                  &v0, &count) == FALSE)
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        src = (const u8 *)(uintptr_t)command->w1;
        if ((src == NULL) ||
            (((uintptr_t)src & (sizeof(u32) - 1u)) != 0u))
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        for (i = 0u; i < count; i++)
        {
            ndsFighterDLDrawDecodeVtx(state, v0 + i, src + (i * 16u));
        }
        return TRUE;
    }

    case NDS_FIGHTER_DL_OP_TRI1:
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri1(command->w0));
        return TRUE;

    case NDS_FIGHTER_DL_OP_TRI2:
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri2First(
                                           command->w0));
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri2Second(
                                           command->w1));
        return TRUE;

    case NDS_FIGHTER_DL_OP_CULLDL:
    case NDS_FIGHTER_DL_OP_TEXTURE:
    case NDS_FIGHTER_DL_OP_POPMTX:
    case NDS_FIGHTER_DL_OP_MTX:
    case NDS_FIGHTER_DL_OP_GEOMETRYMODE:
    case NDS_FIGHTER_DL_OP_MOVEWORD:
    case NDS_FIGHTER_DL_OP_SPECIAL_1:
    case NDS_FIGHTER_DL_OP_DL:
    case NDS_FIGHTER_DL_OP_ENDDL:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_H:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_L:
    case NDS_FIGHTER_DL_OP_SETSCISSOR:
    case NDS_FIGHTER_DL_OP_SETCOMBINE:
    case NDS_FIGHTER_DL_OP_SETCIMG:
    case NDS_FIGHTER_DL_OP_SETFOGCOLOR:
    case NDS_FIGHTER_DL_OP_SETBLENDCOLOR:
    case NDS_FIGHTER_DL_OP_SETENVCOLOR:
    case NDS_FIGHTER_DL_OP_SETPRIMCOLOR:
    case NDS_FIGHTER_DL_OP_SETTIMG:
    case NDS_FIGHTER_DL_OP_SETTILE:
    case NDS_FIGHTER_DL_OP_LOADBLOCK:
    case NDS_FIGHTER_DL_OP_LOADTLUT:
    case NDS_FIGHTER_DL_OP_SETTILESIZE:
    case NDS_FIGHTER_DL_OP_RDPSETOTHERMODE:
    case NDS_FIGHTER_DL_OP_RDPPIPESYNC:
    case NDS_FIGHTER_DL_OP_RDPLOADSYNC:
    case NDS_FIGHTER_DL_OP_RDPTILESYNC:
    case NDS_FIGHTER_DL_OP_RDPFULLSYNC:
        return TRUE;

    default:
        if (state->unsupported_opcode == 0u)
        {
            state->unsupported_opcode = op;
        }
        state->unsupported_command_count++;
        return FALSE;
    }
}

static void ndsStageInishieScaleLoopPlotPreviewPoint(u16 *pixels, u32 pitch,
                                                     s32 x, s32 y,
                                                     u16 color,
                                                     u32 *pixel_count)
{
    if ((pixels == NULL) || (pixel_count == NULL) ||
        (x < 0) || (y < 0) ||
        (x >= (s32)NDS_FIGHTER_DL_DRAW_WIDTH) ||
        (y >= (s32)NDS_FIGHTER_DL_DRAW_HEIGHT))
    {
        return;
    }
    pixels[((u32)y * pitch) + (u32)x] = color;
    (*pixel_count)++;
}

static void ndsStageInishieScaleLoopPlotPreviewMarker(u16 *pixels, u32 pitch,
                                                      s32 x, s32 y,
                                                      u16 color,
                                                      u32 *pixel_count)
{
    s32 dx;
    s32 dy;

    for (dy = -1; dy <= 1; dy++)
    {
        for (dx = -1; dx <= 1; dx++)
        {
            ndsStageInishieScaleLoopPlotPreviewPoint(
                pixels, pitch, x + dx, y + dy, color, pixel_count);
        }
    }
}

static void ndsStageInishieScaleLoopRasterizeSourcePreview(
    NDSFighterDLDrawState *states, const u8 *clean, u32 count,
    u16 *pixels, u32 pitch)
{
    s32 min_x = 0;
    s32 max_x = 0;
    s32 min_y = 0;
    s32 max_y = 0;
    u32 bounds_valid = 0u;
    u32 i;
    u32 pixel_count = 0u;

    if ((states == NULL) || (clean == NULL) || (pixels == NULL))
    {
        return;
    }

    for (i = 0u; i < count; i++)
    {
        u32 tri_index;

        if (clean[i] == FALSE)
        {
            continue;
        }
        for (tri_index = 0u; tri_index < states[i].triangle_count;
             tri_index++)
        {
            const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
            if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
            {
                continue;
            }
            ndsFighterDLDrawRecordAxisPoint(
                &states[i].vertices[tri->v0], 0u, &bounds_valid,
                &min_x, &max_x, &min_y, &max_y);
            ndsFighterDLDrawRecordAxisPoint(
                &states[i].vertices[tri->v1], 0u, &bounds_valid,
                &min_x, &max_x, &min_y, &max_y);
            ndsFighterDLDrawRecordAxisPoint(
                &states[i].vertices[tri->v2], 0u, &bounds_valid,
                &min_x, &max_x, &min_y, &max_y);
        }
    }

    if ((bounds_valid == 0u) ||
        ((min_x == max_x) && (min_y == max_y)))
    {
        if (gNdsStageInishieScaleLoopSourcePreviewBlocker == 0u)
        {
            gNdsStageInishieScaleLoopSourcePreviewBlocker =
                NDS_RENDERER_BLOCKER_NO_TRIANGLES;
        }
        return;
    }

    for (i = 0u; i < count; i++)
    {
        u32 tri_index;

        if (clean[i] == FALSE)
        {
            continue;
        }
        for (tri_index = 0u; tri_index < states[i].triangle_count;
             tri_index++)
        {
            const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
            const NDSFighterDLDrawVtx *v0;
            const NDSFighterDLDrawVtx *v1;
            const NDSFighterDLDrawVtx *v2;
            s32 x0;
            s32 y0;
            s32 x1;
            s32 y1;
            s32 x2;
            s32 y2;
            u16 fill;
            u16 edge;

            if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
            {
                continue;
            }
            v0 = &states[i].vertices[tri->v0];
            v1 = &states[i].vertices[tri->v1];
            v2 = &states[i].vertices[tri->v2];
            if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                (v2->valid == FALSE))
            {
                continue;
            }

            x0 = ndsFighterDLDrawMapCoord(v0->x, min_x, max_x, 4, 91);
            y0 = ndsFighterDLDrawMapCoord(v0->y, min_y, max_y, 67, 4);
            x1 = ndsFighterDLDrawMapCoord(v1->x, min_x, max_x, 4, 91);
            y1 = ndsFighterDLDrawMapCoord(v1->y, min_y, max_y, 67, 4);
            x2 = ndsFighterDLDrawMapCoord(v2->x, min_x, max_x, 4, 91);
            y2 = ndsFighterDLDrawMapCoord(v2->y, min_y, max_y, 67, 4);
            fill = ndsFighterDLDrawTriangleColor(&states[i], tri);
            edge = ndsFighterDLDrawRGB15(255, 255, 255);

            ndsFighterDLDrawTriangle(pixels, pitch, x0, y0, x1, y1, x2, y2,
                                     fill, edge, &pixel_count);
        }
    }

    if (pixel_count == 0u)
    {
        for (i = 0u; i < count; i++)
        {
            u32 tri_index;

            if (clean[i] == FALSE)
            {
                continue;
            }
            for (tri_index = 0u; tri_index < states[i].triangle_count;
                 tri_index++)
            {
                const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
                const NDSFighterDLDrawVtx *v0;
                const NDSFighterDLDrawVtx *v1;
                const NDSFighterDLDrawVtx *v2;
                s32 x0;
                s32 y0;
                s32 x1;
                s32 y1;
                s32 x2;
                s32 y2;
                u16 fill;

                if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
                {
                    continue;
                }
                v0 = &states[i].vertices[tri->v0];
                v1 = &states[i].vertices[tri->v1];
                v2 = &states[i].vertices[tri->v2];
                if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                    (v2->valid == FALSE))
                {
                    continue;
                }

                x0 = ndsFighterDLDrawMapCoord(v0->x, min_x, max_x, 4, 91);
                y0 = ndsFighterDLDrawMapCoord(v0->y, min_y, max_y, 67, 4);
                x1 = ndsFighterDLDrawMapCoord(v1->x, min_x, max_x, 4, 91);
                y1 = ndsFighterDLDrawMapCoord(v1->y, min_y, max_y, 67, 4);
                x2 = ndsFighterDLDrawMapCoord(v2->x, min_x, max_x, 4, 91);
                y2 = ndsFighterDLDrawMapCoord(v2->y, min_y, max_y, 67, 4);
                fill = ndsFighterDLDrawTriangleColor(&states[i], tri);
                ndsStageInishieScaleLoopPlotPreviewMarker(
                    pixels, pitch, x0, y0, fill, &pixel_count);
                ndsStageInishieScaleLoopPlotPreviewMarker(
                    pixels, pitch, x1, y1, fill, &pixel_count);
                ndsStageInishieScaleLoopPlotPreviewMarker(
                    pixels, pitch, x2, y2, fill, &pixel_count);
            }
        }
    }

    gNdsStageInishieScaleLoopSourcePreviewPixelCount = pixel_count;
}

static void ndsStageInishieScaleLoopPreviewSourceDObj(
    DObj *dobj, NDSFighterDLDrawState *state, NDSRendererStats *stats,
    u8 *clean)
{
    NDSRendererConfig config = {0};

    if ((state == NULL) || (stats == NULL) || (clean == NULL))
    {
        return;
    }
    if ((dobj == NULL) || (dobj->dl == NULL))
    {
        if (gNdsStageInishieScaleLoopSourcePreviewBlocker == 0u)
        {
            gNdsStageInishieScaleLoopSourcePreviewBlocker =
                NDS_RENDERER_BLOCKER_BAD_BRANCH;
        }
        return;
    }

    config.max_depth = 4u;
    config.max_commands = 256u;
    config.max_list_commands = 128u;
    config.initial_projection = NULL;
    config.initial_modelview = NULL;
    config.initial_geometry_mode = 0u;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = NULL;
    config.immutable_command_span = NULL;
    config.resolve_branch = NULL;
    config.resolve_data = NULL;
    config.user = state;

    ndsRendererInitStats(stats);
    ndsRendererExecuteDisplayList(dobj->dl, &config,
                                  ndsStageInishieScaleLoopVisitSourcePreviewCommand,
                                  state, stats);

    gNdsStageInishieScaleLoopSourcePreviewDObjCount++;
    gNdsStageInishieScaleLoopSourcePreviewVertexCount +=
        state->vertex_decoded_count;
    gNdsStageInishieScaleLoopSourcePreviewTriangleCount +=
        state->triangle_count;
    gNdsStageInishieScaleLoopSourcePreviewValidTriangleCount +=
        state->triangle_valid_count;
    gNdsStageInishieScaleLoopSourceTextureMask |= stats->texture_mask;
    gNdsStageInishieScaleLoopSourceTextureCommands +=
        stats->texture_command_count;

    if ((stats->blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (stats->unsupported_command_count == 0u) &&
        (state->unsupported_command_count == 0u) &&
        (state->vertex_range_reject_count == 0u) &&
        (state->vertex_decoded_count > 0u) &&
        (state->triangle_count > 0u))
    {
        *clean = TRUE;
    }
    else if (gNdsStageInishieScaleLoopSourcePreviewBlocker == 0u)
    {
        gNdsStageInishieScaleLoopSourcePreviewBlocker =
            (stats->blocker != NDS_RENDERER_BLOCKER_NONE) ?
                stats->blocker : NDS_RENDERER_BLOCKER_UNSUPPORTED;
    }
}

static void ndsStageInishieScaleLoopPreviewSourceDisplay(void)
{
    DObj *dobjs[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    NDSFighterDLDrawState states[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    NDSRendererStats stats;
    u8 clean[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 pitch = 0u;
    u16 *pixels;
    u32 commit_before;
    u32 i;

    dobjs[0] = gGRCommonStruct.inishie.scale[0].platform_dobj;
    dobjs[1] = gGRCommonStruct.inishie.scale[1].platform_dobj;
    dobjs[2] = gGRCommonStruct.inishie.scale[0].string_dobj;
    dobjs[3] = gGRCommonStruct.inishie.scale[1].string_dobj;

    bzero(states, sizeof(states));
    bzero(&stats, sizeof(stats));
    bzero(clean, sizeof(clean));

    commit_before = gNdsOriginalDLPreviewCommitCount;
    pixels = ndsPlatformBeginOriginalDLPreview(NDS_FIGHTER_DL_DRAW_WIDTH,
                                               NDS_FIGHTER_DL_DRAW_HEIGHT,
                                               &pitch);
    if (pixels == NULL)
    {
        gNdsStageInishieScaleLoopSourcePreviewBlocker =
            NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }
    gNdsStageInishieScaleLoopSourcePreviewMask |= 1u << 0;
    ndsFighterPreviewLoopClear(pixels, pitch);

    for (i = 0u; i < NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED; i++)
    {
        states[i].slot = i & 1u;
        ndsStageInishieScaleLoopPreviewSourceDObj(dobjs[i], &states[i],
                                                  &stats, &clean[i]);
    }

    if ((clean[0] != FALSE) && (clean[1] != FALSE) &&
        (clean[2] != FALSE) && (clean[3] != FALSE))
    {
        gNdsStageInishieScaleLoopSourcePreviewMask |= 1u << 1;
    }
    if (gNdsStageInishieScaleLoopSourceTextureCommands > 0u)
    {
        gNdsStageInishieScaleLoopSourcePreviewMask |= 1u << 2;
    }
    if ((gNdsStageInishieScaleLoopSourcePreviewVertexCount > 0u) &&
        (gNdsStageInishieScaleLoopSourcePreviewTriangleCount > 0u) &&
        (gNdsStageInishieScaleLoopSourcePreviewValidTriangleCount > 0u))
    {
        gNdsStageInishieScaleLoopSourcePreviewMask |= 1u << 3;
    }

    ndsStageInishieScaleLoopRasterizeSourcePreview(
        states, clean, NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED, pixels, pitch);
    if (gNdsStageInishieScaleLoopSourcePreviewPixelCount > 0u)
    {
        gNdsStageInishieScaleLoopSourcePreviewMask |= 1u << 4;
        ndsPlatformCommitOriginalDLPreview();
        gNdsStageInishieScaleLoopSourcePreviewCommitDelta =
            gNdsOriginalDLPreviewCommitCount - commit_before;
    }
    if ((gNdsOriginalDLPreviewReady != 0u) &&
        (gNdsStageInishieScaleLoopSourcePreviewCommitDelta == 1u))
    {
        gNdsStageInishieScaleLoopSourcePreviewMask |= 1u << 5;
    }
}

static void ndsStageInishieScaleLoopScanSourceDisplay(void)
{
    ndsStageInishieScaleLoopScanSourceDObj(
        gGRCommonStruct.inishie.scale[0].platform_dobj, 0u, 4u,
        gcDrawDObjDLHead0);
    ndsStageInishieScaleLoopScanSourceDObj(
        gGRCommonStruct.inishie.scale[1].platform_dobj, 1u, 5u,
        gcDrawDObjDLHead0);
    ndsStageInishieScaleLoopScanSourceDObj(
        gGRCommonStruct.inishie.scale[0].string_dobj, 2u, 6u,
        gcDrawDObjTreeForGObj);
    ndsStageInishieScaleLoopScanSourceDObj(
        gGRCommonStruct.inishie.scale[1].string_dobj, 3u, 7u,
        gcDrawDObjTreeForGObj);
    ndsStageInishieScaleLoopPreviewSourceDisplay();

    if (((gNdsStageInishieScaleLoopSourceDisplayMask & 0xffu) != 0xffu) ||
        (gNdsStageInishieScaleLoopSourceDisplayCount != 4u) ||
        (gNdsStageInishieScaleLoopSourceDisplayCommands == 0u) ||
        (gNdsStageInishieScaleLoopSourceDisplayTriangles == 0u) ||
        (gNdsStageInishieScaleLoopSourceDisplayBlocker != 0u) ||
        ((gNdsStageInishieScaleLoopSourcePreviewMask & 0x3du) != 0x3du) ||
        (gNdsStageInishieScaleLoopSourcePreviewDObjCount != 4u) ||
        (gNdsStageInishieScaleLoopSourcePreviewBlocker != 0u))
    {
        gNdsStageInishieScaleLoopUnsafeCount++;
    }
}

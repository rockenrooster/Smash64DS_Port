#include <sys/objdef.h>

#define NDS_GBI_OP_DL 0xdeu
#define NDS_GBI_OP_ENDDL 0xdfu
#define NDS_GBI_OP_SETENVCOLOR 0xfbu
#define NDS_GBI_OP_SETPRIMCOLOR 0xfau
#define NDS_GBI_OP_SETTIMG 0xfdu

/* Retired opening-room software DL preview.
 *
 * BattleShip source contract (decomp/BattleShip-main/decomp/src/mv/mvopening/
 * mvopeningroom.c): opening-room objects draw through gcDrawDObjTreeDLLinks
 * display callbacks with RDP wallpaper procs. The DS software thumbnail
 * pipeline (word decode, generic list scan/execute, texture sampling,
 * triangle raster, camera projection fallback, platform preview staging) is
 * removed. The original is retained in git at a5f2223179d. This ROM unit
 * keeps only source CPU material
 * inspection, DObj/display diagnostics, live display dispatch, and explicit
 * ndsRendererRecordNativeFailure reporting. It never claims PASS or success
 * for empty rendering and never calls poisoned interpreter/rasterizer APIs.
 * Source CPU matrix math and native converted images remain allowed. */

typedef struct NDSOpeningRoomMaterialProbe
{
    u32 count;
    u32 flags;
    u32 effective_flags;
    u32 mask;
    u32 texture_curr;
    u32 texture_next;
    u32 palette_index;
    s32 lfrac100;
    u32 format;
    u32 size;
    u32 block_format;
    u32 block_size;
    u32 tile_width;
    u32 tile_height;
    u32 scroll_width;
    u32 scroll_height;
    s32 scale_s100;
    s32 scale_t100;
    s32 translate_s100;
    s32 translate_t100;
    u32 sprite_array;
    u32 palette_array;
    u32 sprite_curr;
    u32 sprite_next;
    u32 palette_ptr;
} NDSOpeningRoomMaterialProbe;










static s32 ndsPreviewFloatToS32(f32 value)
{
    return (s32)((value >= 0.0F) ? (value + 0.5F) : (value - 0.5F));
}

static s32 ndsPreviewFloatToCenti(f32 value)
{
    return ndsPreviewFloatToS32(value * 100.0F);
}


static void ndsOpeningRoomInitMaterialProbe(
    NDSOpeningRoomMaterialProbe *probe)
{
    if (probe == NULL)
    {
        return;
    }

    probe->count = 0;
    probe->flags = 0;
    probe->effective_flags = 0;
    probe->mask = 0;
    probe->texture_curr = 0xffffffffu;
    probe->texture_next = 0xffffffffu;
    probe->palette_index = 0xffffffffu;
    probe->lfrac100 = 0;
    probe->format = 0xffffffffu;
    probe->size = 0xffffffffu;
    probe->block_format = 0xffffffffu;
    probe->block_size = 0xffffffffu;
    probe->tile_width = 0;
    probe->tile_height = 0;
    probe->scroll_width = 0;
    probe->scroll_height = 0;
    probe->scale_s100 = 0;
    probe->scale_t100 = 0;
    probe->translate_s100 = 0;
    probe->translate_t100 = 0;
    probe->sprite_array = 0;
    probe->palette_array = 0;
    probe->sprite_curr = 0;
    probe->sprite_next = 0;
    probe->palette_ptr = 0;
}

static u32 ndsOpeningRoomGetEffectiveMObjFlags(const MObj *mobj)
{
    u32 flags;

    if (mobj == NULL)
    {
        return MOBJ_FLAG_NONE;
    }

    flags = mobj->sub.flags;
    if (flags == MOBJ_FLAG_NONE)
    {
        flags = MOBJ_FLAG_TEXTURE | 0x20u | MOBJ_FLAG_ALPHA;
    }
    return flags;
}

static s32 ndsOpeningRoomMaterialTrunc(f32 value)
{
    return (s32)value;
}

static u32 ndsOpeningRoomMaterialPositiveOrOne(s32 value)
{
    return (value <= 0) ? 1u : (u32)value;
}

static void ndsOpeningRoomComputeMaterialLoadBlock(
    const MObj *mobj,
    u32 *texels,
    u32 *dxt)
{
    s32 load_texels = 0;
    u32 divisor = 1;

    if ((mobj == NULL) || (texels == NULL) || (dxt == NULL))
    {
        return;
    }

    switch (mobj->sub.block_siz)
    {
    case G_IM_SIZ_4b:
        load_texels =
            ((((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) + 3) >> 2) -
            1;
        divisor = ndsOpeningRoomMaterialPositiveOrOne(
            (s32)mobj->sub.block_dxt / 16);
        break;

    case G_IM_SIZ_8b:
        load_texels =
            ((((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) + 1) >> 1) -
            1;
        divisor = ndsOpeningRoomMaterialPositiveOrOne(
            (s32)mobj->sub.block_dxt / 8);
        break;

    case G_IM_SIZ_16b:
        load_texels =
            ((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) - 1;
        divisor = ndsOpeningRoomMaterialPositiveOrOne(
            ((s32)mobj->sub.block_dxt * 2) / 8);
        break;

    case G_IM_SIZ_32b:
        load_texels =
            ((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) - 1;
        divisor = ndsOpeningRoomMaterialPositiveOrOne(
            ((s32)mobj->sub.block_dxt * 4) / 8);
        break;

    default:
        break;
    }

    *texels = (load_texels > 0) ? (u32)load_texels : 0u;
    *dxt = (divisor + 0x7ffu) / divisor;
}

static void ndsOpeningRoomMaterialTextureState(
    const MObj *mobj,
    u32 flags,
    f32 *scau,
    f32 *scav,
    f32 *trau,
    f32 *trav,
    f32 *scrollu,
    f32 *scrollv,
    u32 *mask)
{
    if ((mobj == NULL) || (scau == NULL) || (scav == NULL) ||
        (trau == NULL) || (trav == NULL) || (scrollu == NULL) ||
        (scrollv == NULL))
    {
        return;
    }

    if ((flags & (MOBJ_FLAG_TEXTURE | 0x40u | 0x20u)) == 0)
    {
        return;
    }

    *scau = mobj->sub.scau;
    *scav = mobj->sub.scav;
    *trau = mobj->sub.trau;
    *trav = mobj->sub.trav;
    *scrollu = mobj->sub.scrollu;
    *scrollv = mobj->sub.scrollv;

    if (mobj->sub.unk10 == 1)
    {
        *scau *= 0.5F;
        *trau =
            ((*trau - mobj->sub.unk24) + 1.0F -
             (mobj->sub.unk28 * 0.5F)) *
            0.5F;
        *scrollu =
            ((*scrollu - mobj->sub.unk44) + 1.0F -
             (mobj->sub.unk28 * 0.5F)) *
            0.5F;
        if (mask != NULL)
        {
            *mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_ADJUSTED_ST;
        }
    }
}

static void ndsOpeningRoomRecordMaterialBranch(DObj *dobj)
{
    MObj *mobj;
    u32 count = 0;
    u32 table_commands = 0;
    u32 generated_commands = 0;

    if ((dobj == NULL) || (dobj->mobj == NULL))
    {
        return;
    }

    for (mobj = dobj->mobj; (mobj != NULL) && (count < 64u);
         mobj = mobj->next)
    {
        f32 scau = 0.0F;
        f32 scav = 0.0F;
        f32 trau = 0.0F;
        f32 trav = 0.0F;
        f32 scrollu = 0.0F;
        f32 scrollv = 0.0F;
        u32 flags = ndsOpeningRoomGetEffectiveMObjFlags(mobj);
        u32 mask = NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_TABLE;
        u32 mobj_generated = 0;
        s32 uls;
        s32 ult;
        s32 s;
        s32 t;

        count++;
        table_commands++;
        if (count == 1u)
        {
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_SEGMENT;
        }

        ndsOpeningRoomMaterialTextureState(
            mobj,
            flags,
            &scau,
            &scav,
            &trau,
            &trav,
            &scrollu,
            &scrollv,
            &mask);

        if ((flags & MOBJ_FLAG_PALETTE) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PALETTE_IMAGE;
            if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
            {
                mobj_generated += 5u;
                mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PALETTE_TLUT;
            }
        }
        if ((flags & MOBJ_FLAG_LIGHT1) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_LIGHT;
        }
        if ((flags & MOBJ_FLAG_LIGHT2) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_LIGHT;
        }
        if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PRIMCOLOR;
        }
        if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_ENVCOLOR;
        }
        if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_BLENDCOLOR;
        }
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_NEXT_IMAGE;
            if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
            {
                u32 texels = 0;
                u32 dxt = 0;

                mobj_generated += 3u;
                mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_LOADBLOCK;
                ndsOpeningRoomComputeMaterialLoadBlock(mobj, &texels, &dxt);
                if (count == 1u)
                {
                    gNdsOpeningRoomDrawMaterialBranchFirstLoadBlockTexels =
                        texels;
                    gNdsOpeningRoomDrawMaterialBranchFirstLoadBlockDxt = dxt;
                }
            }
        }
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_CURRENT_IMAGE;
        }
        if ((flags & 0x20u) != 0)
        {
            if (mobj->sub.unk10 == 2)
            {
                uls = (fabsf(scau) > (1.0F / 65535.0F)) ?
                    ndsOpeningRoomMaterialTrunc(
                        (((f32)mobj->sub.unk0C * trau) / scau) * 4.0F) :
                    0;
                ult = (fabsf(scav) > (1.0F / 65535.0F)) ?
                    ndsOpeningRoomMaterialTrunc(
                        (((f32)mobj->sub.unk0E * trav) / scav) * 4.0F) :
                    0;
                if (uls < 0)
                {
                    uls = 0;
                }
                if (ult < 0)
                {
                    ult = 0;
                }
            }
            else
            {
                uls = (fabsf(scau) > (1.0F / 65535.0F)) ?
                    ndsOpeningRoomMaterialTrunc(
                        ((((f32)mobj->sub.unk0C * trau) +
                          (f32)mobj->sub.unk0A) /
                         scau) *
                        4.0F) :
                    0;
                ult = (fabsf(scav) > (1.0F / 65535.0F)) ?
                    ndsOpeningRoomMaterialTrunc(
                        (((((1.0F - scav) - trav) *
                           (f32)mobj->sub.unk0E) +
                          (f32)mobj->sub.unk0A) /
                         scav) *
                        4.0F) :
                    0;
            }
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_TILESIZE;
            if (count == 1u)
            {
                gNdsOpeningRoomDrawMaterialBranchFirstTileUls = uls;
                gNdsOpeningRoomDrawMaterialBranchFirstTileUlt = ult;
                gNdsOpeningRoomDrawMaterialBranchFirstTileLrs =
                    (((s32)mobj->sub.unk0C - 1) << 2) + uls;
                gNdsOpeningRoomDrawMaterialBranchFirstTileLrt =
                    (((s32)mobj->sub.unk0E - 1) << 2) + ult;
            }
        }
        if ((flags & 0x40u) != 0)
        {
            uls = (fabsf(scau) > (1.0F / 65535.0F)) ?
                ndsOpeningRoomMaterialTrunc(
                    ((((f32)mobj->sub.unk38 * scrollu) +
                      (f32)mobj->sub.unk0A) /
                     scau) *
                    4.0F) :
                0;
            ult = (fabsf(scav) > (1.0F / 65535.0F)) ?
                ndsOpeningRoomMaterialTrunc(
                    (((((1.0F - scav) - scrollv) *
                       (f32)mobj->sub.unk3A) +
                      (f32)mobj->sub.unk0A) /
                     scav) *
                    4.0F) :
                0;
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_SCROLL_TILESIZE;
            if (count == 1u)
            {
                gNdsOpeningRoomDrawMaterialBranchFirstScrollUls = uls;
                gNdsOpeningRoomDrawMaterialBranchFirstScrollUlt = ult;
                gNdsOpeningRoomDrawMaterialBranchFirstScrollLrs =
                    (((s32)mobj->sub.unk38 - 1) << 2) + uls;
                gNdsOpeningRoomDrawMaterialBranchFirstScrollLrt =
                    (((s32)mobj->sub.unk3A - 1) << 2) + ult;
            }
        }
        if ((flags & MOBJ_FLAG_TEXTURE) != 0)
        {
            if (mobj->sub.unk10 == 2)
            {
                s = (fabsf(scau) > (1.0F / 65535.0F)) ?
                    ndsOpeningRoomMaterialTrunc(
                        ((f32)mobj->sub.unk0C * 64.0F) / scau) :
                    0;
                t = (fabsf(scav) > (1.0F / 65535.0F)) ?
                    ndsOpeningRoomMaterialTrunc(
                        ((f32)mobj->sub.unk0E * 64.0F) / scav) :
                    0;
            }
            else
            {
                s = ((mobj->sub.unk08 != 0) &&
                     (fabsf(scau) > (1.0F / 65535.0F))) ?
                    ndsOpeningRoomMaterialTrunc(
                        (2097152.0F / (f32)mobj->sub.unk08) / scau) :
                    0;
                t = ((mobj->sub.unk08 != 0) &&
                     (fabsf(scav) > (1.0F / 65535.0F))) ?
                    ndsOpeningRoomMaterialTrunc(
                        (2097152.0F / (f32)mobj->sub.unk08) / scav) :
                    0;
            }
            if (s > 0xffff)
            {
                s = 0xffff;
            }
            if (t > 0xffff)
            {
                t = 0xffff;
            }
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_TEXTURE;
            if (count == 1u)
            {
                gNdsOpeningRoomDrawMaterialBranchFirstTextureScaleS = (u32)s;
                gNdsOpeningRoomDrawMaterialBranchFirstTextureScaleT = (u32)t;
            }
        }

        mobj_generated++;
        mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_END;
        generated_commands += mobj_generated;

        if (count == 1u)
        {
            gNdsOpeningRoomDrawMaterialBranchFirstMask = mask;
            gNdsOpeningRoomDrawMaterialBranchFirstGeneratedCommands =
                mobj_generated;
        }
    }

    if (count != 0)
    {
        gNdsOpeningRoomDrawMaterialBranchResult =
            NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PASS;
        gNdsOpeningRoomDrawMaterialBranchMObjCount = count;
        gNdsOpeningRoomDrawMaterialBranchSegmentCommands = 1;
        gNdsOpeningRoomDrawMaterialBranchTableCommands = table_commands;
        gNdsOpeningRoomDrawMaterialBranchGeneratedCommands =
            generated_commands;
    }
}

static void ndsOpeningRoomEmitBranchTableCommand(Gfx *cmd, const Gfx *branch)
{
    cmd->words.w0 = NDS_GBI_OP_DL << 24;
    cmd->words.w1 = (u32)(uintptr_t)branch;
}

static void ndsOpeningRoomEmitEndDL(Gfx *cmd)
{
    cmd->words.w0 = NDS_GBI_OP_ENDDL << 24;
    cmd->words.w1 = 0;
}

static void ndsOpeningRoomEmitPrimColor(Gfx *cmd, const MObj *mobj)
{
    u32 lfrac;

    lfrac = (u32)ndsPreviewFloatToS32(mobj->lfrac * 255.0F);
    if (lfrac > 0xffu)
    {
        lfrac = 0xffu;
    }

    cmd->words.w0 =
        (NDS_GBI_OP_SETPRIMCOLOR << 24) |
        (((u32)mobj->sub.prim_m & 0xffu) << 8) |
        (lfrac & 0xffu);
    cmd->words.w1 =
        ((u32)mobj->sub.primcolor.s.r << 24) |
        ((u32)mobj->sub.primcolor.s.g << 16) |
        ((u32)mobj->sub.primcolor.s.b << 8) |
        (u32)mobj->sub.primcolor.s.a;
}

static void ndsOpeningRoomEmitEnvColor(Gfx *cmd, const MObj *mobj)
{
    cmd->words.w0 = NDS_GBI_OP_SETENVCOLOR << 24;
    cmd->words.w1 =
        ((u32)mobj->sub.envcolor.s.r << 24) |
        ((u32)mobj->sub.envcolor.s.g << 16) |
        ((u32)mobj->sub.envcolor.s.b << 8) |
        (u32)mobj->sub.envcolor.s.a;
}

static void ndsOpeningRoomEmitTextureImage(Gfx *cmd, const MObj *mobj)
{
    cmd->words.w0 =
        (NDS_GBI_OP_SETTIMG << 24) |
        (((u32)mobj->sub.fmt & 0x7u) << 21) |
        (((u32)mobj->sub.siz & 0x3u) << 19);
    cmd->words.w1 =
        (mobj->sub.sprites != NULL) ?
            (u32)(uintptr_t)mobj->sub.sprites[mobj->texture_id_curr] :
            0u;
}

static u32 ndsOpeningRoomMaterialEmitUnsupportedMask(u32 flags)
{
    u32 unsupported = 0;

    if ((flags & MOBJ_FLAG_PALETTE) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PALETTE_IMAGE;
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
        {
            unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PALETTE_TLUT;
        }
    }
    if ((flags & (MOBJ_FLAG_LIGHT1 | MOBJ_FLAG_LIGHT2)) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_LIGHT;
    }
    if ((flags & (MOBJ_FLAG_FRAC | 0x8u)) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PRIMCOLOR;
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_BLENDCOLOR;
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_NEXT_IMAGE;
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_LOADBLOCK;
        }
    }
    if ((flags & 0x20u) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_TILESIZE;
    }
    if ((flags & 0x40u) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_SCROLL_TILESIZE;
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_TEXTURE;
    }
    return unsupported;
}

static void ndsOpeningRoomCaptureFirstEmittedBranch(const Gfx *branch,
                                                    u32 commands)
{
    if ((branch == NULL) || (commands == 0))
    {
        return;
    }
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_0 = branch[0].words.w0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_0 = branch[0].words.w1;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchOp0 =
        branch[0].words.w0 >> 24;
    if (commands < 2u)
    {
        return;
    }
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_1 = branch[1].words.w0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_1 = branch[1].words.w1;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchOp1 =
        branch[1].words.w0 >> 24;
    if (commands < 3u)
    {
        return;
    }
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_2 = branch[2].words.w0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_2 = branch[2].words.w1;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchOp2 =
        branch[2].words.w0 >> 24;
}


/* Retired manual list-shape probe. New ROMs never interpret
 * generic lists here; this stub publishes the native failure and leaves the
 * existing failure result unset. */
static void ndsOpeningRoomProbeMaterialDLShape(void)
{
    const Gfx *dl = sNdsOpeningRoomMaterialPreviewDL;

    if (gNdsOpeningRoomMaterialDLProbeResult != 0)
    {
        return;
    }

    gNdsOpeningRoomMaterialDLProbeBlocker =
        NDS_OPENING_ROOM_MATERIAL_DL_PROBE_BLOCKER_UNSUPPORTED;
    gNdsOpeningRoomMaterialDLProbeFirstDL = (u32)(uintptr_t)dl;
    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_STAGE,
        (u32)gSCManagerSceneData.scene_curr, 0xffffffffu, 0u,
        (u32)(uintptr_t)dl, 0u, NDS_NATIVE_FAILURE_NO_PROGRAM);
    return;
}


static s32 ndsOpeningRoomPointerRangeInLoadedFiles(const void *ptr,
                                                   size_t size)
{
    u32 i;

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocPointerRangeInLoadedFile(
                &sNdsRelocLoadedFiles[i], ptr, size) != FALSE)
        {
            return TRUE;
        }
    }
    return FALSE;
}


static void ndsOpeningRoomProbeMaterialDLExpandedShape(void)
{
    const Gfx *dl = sNdsOpeningRoomMaterialPreviewDL;

    if (gNdsOpeningRoomMaterialDLExpandResult != 0)
    {
        return;
    }

    gNdsOpeningRoomMaterialDLExpandBlocker =
        NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_UNSUPPORTED;
    gNdsOpeningRoomMaterialDLExpandFirstDL = (u32)(uintptr_t)dl;
    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_STAGE,
        (u32)gSCManagerSceneData.scene_curr, 0xffffffffu, 0u,
        (u32)(uintptr_t)dl, 0u, NDS_NATIVE_FAILURE_NO_PROGRAM);
    return;
}

static void ndsOpeningRoomEmitMaterialBranch(DObj *dobj)
{
    MObj *mobj;
    Gfx *new_dl;
    Gfx *branch_dl;
    Gfx *first_branch;
    uintptr_t heap_start;
    uintptr_t heap_end;
    uintptr_t heap_ptr;
    size_t heap_bytes;
    u32 count = 0;
    u32 generated = 0;
    u32 first_generated = 0;
    u32 unsupported = 0;

    if ((dobj == NULL) || (dobj->mobj == NULL))
    {
        gNdsOpeningRoomDrawMaterialEmitBlocker =
            NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_NO_MOBJ;
        return;
    }

    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        u32 flags;
        u32 mobj_generated = 0;

        count++;
        if (count > 64u)
        {
            gNdsOpeningRoomDrawMaterialEmitBlocker =
                NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_TOO_MANY_MOBJS;
            return;
        }

        flags = ndsOpeningRoomGetEffectiveMObjFlags(mobj);
        unsupported |= ndsOpeningRoomMaterialEmitUnsupportedMask(flags);
        if ((flags & MOBJ_FLAG_PRIMCOLOR) != 0)
        {
            mobj_generated++;
        }
        if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
        {
            mobj_generated++;
        }
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            mobj_generated++;
        }
        mobj_generated++;
        generated += mobj_generated;
        if (count == 1u)
        {
            first_generated = mobj_generated;
        }
    }

    if (unsupported != 0)
    {
        gNdsOpeningRoomDrawMaterialEmitUnsupportedMask = unsupported;
        gNdsOpeningRoomDrawMaterialEmitBlocker =
            NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_UNSUPPORTED_FAMILY;
        return;
    }
    if ((gSYTaskmanGraphicsHeap.ptr == NULL) ||
        (gSYTaskmanGraphicsHeap.start == NULL) ||
        (gSYTaskmanGraphicsHeap.end == NULL))
    {
        gNdsOpeningRoomDrawMaterialEmitBlocker =
            NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_NO_HEAP;
        return;
    }

    heap_start = (uintptr_t)gSYTaskmanGraphicsHeap.start;
    heap_end = (uintptr_t)gSYTaskmanGraphicsHeap.end;
    heap_ptr = (uintptr_t)gSYTaskmanGraphicsHeap.ptr;
    heap_bytes = (size_t)(count + generated) * sizeof(Gfx);

    if ((heap_ptr < heap_start) || (heap_ptr > heap_end) ||
        (heap_bytes > (size_t)(heap_end - heap_ptr)))
    {
        gNdsOpeningRoomDrawMaterialEmitBlocker =
            NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_HEAP_RANGE;
        return;
    }

    new_dl = (Gfx *)gSYTaskmanGraphicsHeap.ptr;
    branch_dl = new_dl + count;
    first_branch = branch_dl;

    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        u32 flags = ndsOpeningRoomGetEffectiveMObjFlags(mobj);

        ndsOpeningRoomEmitBranchTableCommand(new_dl++, branch_dl);
        if ((flags & MOBJ_FLAG_PRIMCOLOR) != 0)
        {
            ndsOpeningRoomEmitPrimColor(branch_dl++, mobj);
        }
        if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
        {
            ndsOpeningRoomEmitEnvColor(branch_dl++, mobj);
        }
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            ndsOpeningRoomEmitTextureImage(branch_dl++, mobj);
        }
        ndsOpeningRoomEmitEndDL(branch_dl++);
    }

    gSYTaskmanGraphicsHeap.ptr = branch_dl;
    gNdsOpeningRoomDrawMaterialEmitResult =
        NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_PASS;
    gNdsOpeningRoomDrawMaterialEmitBlocker =
        NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_NONE;
    gNdsOpeningRoomDrawMaterialEmitMObjCount = count;
    gNdsOpeningRoomDrawMaterialEmitTableCommands = count;
    gNdsOpeningRoomDrawMaterialEmitGeneratedCommands = generated;
    gNdsOpeningRoomDrawMaterialEmitHeapStart = (u32)heap_ptr;
    gNdsOpeningRoomDrawMaterialEmitBranchStart =
        (u32)(uintptr_t)(new_dl);
    gNdsOpeningRoomDrawMaterialEmitHeapAfter = (u32)(uintptr_t)branch_dl;
    gNdsOpeningRoomDrawMaterialEmitHeapBytes =
        (u32)((uintptr_t)branch_dl - heap_ptr);
    gNdsOpeningRoomDrawMaterialEmitFirstTableOp =
        ((Gfx *)heap_ptr)[0].words.w0 >> 24;
    ndsOpeningRoomCaptureFirstEmittedBranch(first_branch, first_generated);
}

static u32 ndsOpeningRoomReadPointerArrayEntry(void **array, u32 index)
{
    if ((array == NULL) || (index >= 64u) ||
        (ndsOpeningRoomPointerRangeInLoadedFiles(
             array, ((size_t)index + 1u) * sizeof(*array)) == FALSE))
    {
        return 0;
    }
    return (u32)(uintptr_t)array[index];
}

static u32 ndsOpeningRoomMakeTextureMObjMask(const MObj *mobj,
                                             u32 effective_flags,
                                             u32 *out_sprite_curr,
                                             u32 *out_sprite_next)
{
    u32 mask = NDS_OPENING_ROOM_DL_MATERIAL_HAS_MOBJ;
    u32 sprite_curr = 0;
    u32 sprite_next = 0;

    if (mobj == NULL)
    {
        return 0;
    }
    if (mobj->sub.flags == MOBJ_FLAG_NONE)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_DEFAULT_FLAGS;
    }
    if ((effective_flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_TEXTURE;
    }
    if ((effective_flags & MOBJ_FLAG_ALPHA) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_ALPHA;
    }
    if ((effective_flags & 0x20u) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_TILESIZE;
    }
    if ((effective_flags & 0x40u) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_SCROLL_TILESIZE;
    }
    if ((effective_flags & MOBJ_FLAG_PALETTE) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_PALETTE;
    }
    if ((effective_flags & MOBJ_FLAG_FRAC) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_FRAC;
    }
    if ((effective_flags & MOBJ_FLAG_SPLIT) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_SPLIT;
    }
    if ((effective_flags &
         (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_ENVCOLOR |
          MOBJ_FLAG_BLENDCOLOR)) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_COLOR;
    }
    if ((effective_flags & (MOBJ_FLAG_LIGHT1 | MOBJ_FLAG_LIGHT2)) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_LIGHT;
    }
    if (mobj->sub.sprites != NULL)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_SPRITE_ARRAY;
        if ((effective_flags & (MOBJ_FLAG_ALPHA | MOBJ_FLAG_FRAC)) != 0)
        {
            sprite_curr = ndsOpeningRoomReadPointerArrayEntry(
                mobj->sub.sprites, mobj->texture_id_curr);
            if (sprite_curr != 0)
            {
                mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_CURR_SPRITE;
            }
        }
        if ((effective_flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
        {
            sprite_next = ndsOpeningRoomReadPointerArrayEntry(
                mobj->sub.sprites, mobj->texture_id_next);
            if (sprite_next != 0)
            {
                mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_NEXT_SPRITE;
            }
        }
    }

    if (out_sprite_curr != NULL)
    {
        *out_sprite_curr = sprite_curr;
    }
    if (out_sprite_next != NULL)
    {
        *out_sprite_next = sprite_next;
    }
    return mask;
}

static void ndsOpeningRoomProbeDObjMaterial(
    DObj *dobj,
    NDSOpeningRoomMaterialProbe *probe)
{
    MObj *mobj;
    MObj *first_mobj = NULL;

    ndsOpeningRoomInitMaterialProbe(probe);
    if ((dobj == NULL) || (probe == NULL))
    {
        return;
    }

    probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_DOBJ_SEEN;

    for (mobj = dobj->mobj; (mobj != NULL) && (probe->count < 64u);
         mobj = mobj->next)
    {
        if (first_mobj == NULL)
        {
            first_mobj = mobj;
        }
        probe->count++;
    }

    if (first_mobj == NULL)
    {
        return;
    }

    probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_MOBJ;
    probe->flags = first_mobj->sub.flags;
    probe->effective_flags = ndsOpeningRoomGetEffectiveMObjFlags(first_mobj);
    if (probe->flags == MOBJ_FLAG_NONE)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_DEFAULT_FLAGS;
    }
    if ((probe->effective_flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_TEXTURE;
    }
    if ((probe->effective_flags & MOBJ_FLAG_ALPHA) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_ALPHA;
    }
    if ((probe->effective_flags & 0x20u) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_TILESIZE;
    }
    if ((probe->effective_flags & 0x40u) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_SCROLL_TILESIZE;
    }
    if ((probe->effective_flags & MOBJ_FLAG_PALETTE) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_PALETTE;
    }
    if ((probe->effective_flags & MOBJ_FLAG_FRAC) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_FRAC;
    }
    if ((probe->effective_flags & MOBJ_FLAG_SPLIT) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_SPLIT;
    }
    if ((probe->effective_flags &
         (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_ENVCOLOR |
          MOBJ_FLAG_BLENDCOLOR)) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_COLOR;
    }
    if ((probe->effective_flags & (MOBJ_FLAG_LIGHT1 | MOBJ_FLAG_LIGHT2)) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_LIGHT;
    }

    probe->texture_curr = first_mobj->texture_id_curr;
    probe->texture_next = first_mobj->texture_id_next;
    if (first_mobj->palette_id >= 0.0F)
    {
        probe->palette_index = (u32)ndsPreviewFloatToS32(first_mobj->palette_id);
    }
    probe->lfrac100 = ndsPreviewFloatToCenti(first_mobj->lfrac);
    probe->format = first_mobj->sub.fmt;
    probe->size = first_mobj->sub.siz;
    probe->block_format = first_mobj->sub.block_fmt;
    probe->block_size = first_mobj->sub.block_siz;
    probe->tile_width = first_mobj->sub.unk0C;
    probe->tile_height = first_mobj->sub.unk0E;
    probe->scroll_width = first_mobj->sub.unk38;
    probe->scroll_height = first_mobj->sub.unk3A;
    probe->scale_s100 = ndsPreviewFloatToCenti(first_mobj->sub.scau);
    probe->scale_t100 = ndsPreviewFloatToCenti(first_mobj->sub.scav);
    probe->translate_s100 = ndsPreviewFloatToCenti(first_mobj->sub.trau);
    probe->translate_t100 = ndsPreviewFloatToCenti(first_mobj->sub.trav);
    probe->sprite_array = (u32)(uintptr_t)first_mobj->sub.sprites;
    probe->palette_array = (u32)(uintptr_t)first_mobj->sub.palettes;

    if (first_mobj->sub.sprites != NULL)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_SPRITE_ARRAY;
        if ((probe->effective_flags &
             (MOBJ_FLAG_ALPHA | MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
        {
            probe->sprite_curr =
                (u32)(uintptr_t)
                    first_mobj->sub.sprites[first_mobj->texture_id_curr];
            if (probe->sprite_curr != 0)
            {
                probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_CURR_SPRITE;
            }
        }
        if ((probe->effective_flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
        {
            probe->sprite_next =
                (u32)(uintptr_t)
                    first_mobj->sub.sprites[first_mobj->texture_id_next];
            if (probe->sprite_next != 0)
            {
                probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_NEXT_SPRITE;
            }
        }
    }
    if (first_mobj->sub.palettes != NULL)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_PALETTE_ARRAY;
        if (((probe->effective_flags & MOBJ_FLAG_PALETTE) != 0) &&
            (probe->palette_index != 0xffffffffu))
        {
            probe->palette_ptr =
                (u32)(uintptr_t)
                    first_mobj->sub.palettes[probe->palette_index];
            if (probe->palette_ptr != 0)
            {
                probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_PALETTE_PTR;
            }
        }
    }
}

static void ndsOpeningRoomStoreDLPreviewMaterial(
    const NDSOpeningRoomMaterialProbe *probe)
{
    if (probe == NULL)
    {
        return;
    }

    gNdsOpeningRoomDLPreviewMaterialCount = probe->count;
    gNdsOpeningRoomDLPreviewMaterialFlags = probe->flags;
    gNdsOpeningRoomDLPreviewMaterialEffectiveFlags = probe->effective_flags;
    gNdsOpeningRoomDLPreviewMaterialMask = probe->mask;
    gNdsOpeningRoomDLPreviewMaterialTextureCurr = probe->texture_curr;
    gNdsOpeningRoomDLPreviewMaterialTextureNext = probe->texture_next;
    gNdsOpeningRoomDLPreviewMaterialPaletteIndex = probe->palette_index;
    gNdsOpeningRoomDLPreviewMaterialLfrac100 = probe->lfrac100;
    gNdsOpeningRoomDLPreviewMaterialFormat = probe->format;
    gNdsOpeningRoomDLPreviewMaterialSize = probe->size;
    gNdsOpeningRoomDLPreviewMaterialBlockFormat = probe->block_format;
    gNdsOpeningRoomDLPreviewMaterialBlockSize = probe->block_size;
    gNdsOpeningRoomDLPreviewMaterialTileWidth = probe->tile_width;
    gNdsOpeningRoomDLPreviewMaterialTileHeight = probe->tile_height;
    gNdsOpeningRoomDLPreviewMaterialScrollWidth = probe->scroll_width;
    gNdsOpeningRoomDLPreviewMaterialScrollHeight = probe->scroll_height;
    gNdsOpeningRoomDLPreviewMaterialScaleS100 = probe->scale_s100;
    gNdsOpeningRoomDLPreviewMaterialScaleT100 = probe->scale_t100;
    gNdsOpeningRoomDLPreviewMaterialTranslateS100 = probe->translate_s100;
    gNdsOpeningRoomDLPreviewMaterialTranslateT100 = probe->translate_t100;
    gNdsOpeningRoomDLPreviewMaterialSpriteCurr = probe->sprite_curr;
    gNdsOpeningRoomDLPreviewMaterialSpriteNext = probe->sprite_next;
    gNdsOpeningRoomDLPreviewMaterialPalettePtr = probe->palette_ptr;
}

static void ndsOpeningRoomStoreCandidateMaterial(
    const NDSOpeningRoomMaterialProbe *probe)
{
    if (probe == NULL)
    {
        return;
    }

    gNdsOpeningRoomDrawMaterialCandidateMObjCount = probe->count;
    gNdsOpeningRoomDrawMaterialCandidateMObjFlags = probe->flags;
    gNdsOpeningRoomDrawMaterialCandidateMObjEffectiveFlags =
        probe->effective_flags;
    gNdsOpeningRoomDrawMaterialCandidateMObjMask = probe->mask;
    gNdsOpeningRoomDrawMaterialCandidateMObjTextureCurr = probe->texture_curr;
    gNdsOpeningRoomDrawMaterialCandidateMObjTextureNext = probe->texture_next;
    gNdsOpeningRoomDrawMaterialCandidateMObjPaletteIndex =
        probe->palette_index;
    gNdsOpeningRoomDrawMaterialCandidateMObjLfrac100 = probe->lfrac100;
    gNdsOpeningRoomDrawMaterialCandidateMObjFormat = probe->format;
    gNdsOpeningRoomDrawMaterialCandidateMObjSize = probe->size;
    gNdsOpeningRoomDrawMaterialCandidateMObjBlockFormat =
        probe->block_format;
    gNdsOpeningRoomDrawMaterialCandidateMObjBlockSize = probe->block_size;
    gNdsOpeningRoomDrawMaterialCandidateMObjTileWidth = probe->tile_width;
    gNdsOpeningRoomDrawMaterialCandidateMObjTileHeight = probe->tile_height;
    gNdsOpeningRoomDrawMaterialCandidateMObjScrollWidth =
        probe->scroll_width;
    gNdsOpeningRoomDrawMaterialCandidateMObjScrollHeight =
        probe->scroll_height;
    gNdsOpeningRoomDrawMaterialCandidateMObjScaleS100 = probe->scale_s100;
    gNdsOpeningRoomDrawMaterialCandidateMObjScaleT100 = probe->scale_t100;
    gNdsOpeningRoomDrawMaterialCandidateMObjTranslateS100 =
        probe->translate_s100;
    gNdsOpeningRoomDrawMaterialCandidateMObjTranslateT100 =
        probe->translate_t100;
    gNdsOpeningRoomDrawMaterialCandidateMObjSpriteArray =
        probe->sprite_array;
    gNdsOpeningRoomDrawMaterialCandidateMObjPaletteArray =
        probe->palette_array;
    gNdsOpeningRoomDrawMaterialCandidateMObjSpriteCurr = probe->sprite_curr;
    gNdsOpeningRoomDrawMaterialCandidateMObjSpriteNext = probe->sprite_next;
    gNdsOpeningRoomDrawMaterialCandidateMObjPalettePtr = probe->palette_ptr;
}

static void ndsOpeningRoomRecordDLPreviewMaterial(DObj *dobj)
{
    NDSOpeningRoomMaterialProbe probe;

    ndsOpeningRoomProbeDObjMaterial(dobj, &probe);
    ndsOpeningRoomStoreDLPreviewMaterial(&probe);
}





/* Retired DL preview. New ROMs never execute generic lists
 * or rasterize here; this stub publishes the native failure and leaves the
 * existing failure result unset. */
static void ndsOpeningRoomRenderDLPreview(DObj *dobj, Gfx *dl)
{
    if (gNdsOpeningRoomDLPreviewResult != 0)
    {
        return;
    }

    gNdsOpeningRoomDLPreviewFirstDL = (u32)(uintptr_t)dl;
    gNdsOpeningRoomDLPreviewBlocker =
        NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_UNSUPPORTED_CMD;
    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_STAGE,
        (u32)gSCManagerSceneData.scene_curr, 0xffffffffu, 0u,
        (u32)(uintptr_t)dl, (dobj != NULL) ? (u32)(uintptr_t)dobj->mobj : 0u,
        NDS_NATIVE_FAILURE_NO_PROGRAM);
    return;
}

static Gfx *ndsOpeningRoomGetFirstDObjDLLink(DObj *dobj)
{
    DObjDLLink *dl_link;
    u32 i;

    if (dobj == NULL)
    {
        return NULL;
    }

    dl_link = dobj->dl_link;
    if (dl_link == NULL)
    {
        return NULL;
    }

    for (i = 0; i < 4u; i++, dl_link++)
    {
        if (dl_link->list_id == 4)
        {
            break;
        }
        if (dl_link->dl != NULL)
        {
            return dl_link->dl;
        }
    }
    return NULL;
}

static Gfx *ndsOpeningRoomGetDObjDLForCallback(DObj *dobj,
                                               u32 callback_marker)
{
    if (dobj == NULL)
    {
        return NULL;
    }

    switch (callback_marker)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
        return dobj->dl;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
    default:
        return ndsOpeningRoomGetFirstDObjDLLink(dobj);
    }
}

static ub8 ndsOpeningRoomDObjDrawableForCallback(DObj *dobj,
                                                 u32 callback_marker)
{
    if (dobj == NULL)
    {
        return FALSE;
    }
    if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0)
    {
        return FALSE;
    }

    switch (callback_marker)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
        return ((dobj->flags & DOBJ_FLAG_NOTEXTURE) == 0) ? TRUE : FALSE;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
    default:
        return (dobj->flags == DOBJ_FLAG_NONE) ? TRUE : FALSE;
    }
}

static u32 ndsOpeningRoomMakeDObjMeta(DObj *dobj, Gfx *dl)
{
    u32 meta = 0;

    if (dobj == NULL)
    {
        return 0;
    }
    if (dl != NULL)
    {
        meta |= NDS_OPENING_ROOM_DRAW_DOBJ_HAS_DL;
    }
    if (dobj->child != NULL)
    {
        meta |= NDS_OPENING_ROOM_DRAW_DOBJ_HAS_CHILD;
    }
    if ((dobj->sib_next != NULL) || (dobj->sib_prev != NULL))
    {
        meta |= NDS_OPENING_ROOM_DRAW_DOBJ_HAS_SIBLING;
    }
    if (dobj->mobj != NULL)
    {
        meta |= NDS_OPENING_ROOM_DRAW_DOBJ_HAS_MOBJ;
    }
    if (dobj->xobjs_num != 0)
    {
        meta |= NDS_OPENING_ROOM_DRAW_DOBJ_HAS_XOBJ;
    }
    return meta;
}

static ub8 ndsOpeningRoomCallbackWalksDObjTree(u32 callback_marker)
{
    return ((callback_marker == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE) ||
            (callback_marker ==
             NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS)) ?
        TRUE : FALSE;
}

static DObj *ndsOpeningRoomFindDObjCandidateRecurse(DObj *dobj,
                                                    u32 callback_marker,
                                                    ub8 require_mobj,
                                                    Gfx **out_dl,
                                                    u32 *visited)
{
    while ((dobj != NULL) && (*visited < 64u))
    {
        Gfx *dl;

        (*visited)++;
        dl = ndsOpeningRoomGetDObjDLForCallback(dobj, callback_marker);
        if ((dl != NULL) &&
            (ndsOpeningRoomDObjDrawableForCallback(dobj, callback_marker) !=
             FALSE) &&
            ((require_mobj == FALSE) || (dobj->mobj != NULL)))
        {
            if (out_dl != NULL)
            {
                *out_dl = dl;
            }
            return dobj;
        }

        if ((ndsOpeningRoomCallbackWalksDObjTree(callback_marker) != FALSE) &&
            (dobj->child != NULL))
        {
            DObj *found = ndsOpeningRoomFindDObjCandidateRecurse(
                dobj->child,
                callback_marker,
                require_mobj,
                out_dl,
                visited);

            if (found != NULL)
            {
                return found;
            }
        }

        if (ndsOpeningRoomCallbackWalksDObjTree(callback_marker) == FALSE)
        {
            break;
        }
        dobj = dobj->sib_next;
    }
    return NULL;
}

static DObj *ndsOpeningRoomFindDObjDrawCandidate(DObj *dobj,
                                                 u32 callback_marker,
                                                 ub8 require_mobj,
                                                 Gfx **out_dl)
{
    u32 visited = 0;

    if (out_dl != NULL)
    {
        *out_dl = NULL;
    }
    return ndsOpeningRoomFindDObjCandidateRecurse(
        dobj,
        callback_marker,
        require_mobj,
        out_dl,
        &visited);
}

static void ndsOpeningRoomRecordCapturedDisplay(GObj *camera_gobj,
                                                GObj *display_gobj,
                                                s32 link_id)
{
    (void)camera_gobj;

    if (gSCManagerSceneData.scene_curr != nSCKindOpeningRoom)
    {
        return;
    }
    gNdsOpeningRoomDrawDisplayCallbackCount++;

    if ((display_gobj != NULL) &&
        (gNdsOpeningRoomDrawFirstObjectDLLink == 0xffffffffu))
    {
        gNdsOpeningRoomDrawFirstObjectDLLink = (u32)link_id;
        gNdsOpeningRoomDrawFirstObjectID = display_gobj->id;
        gNdsOpeningRoomDrawFirstObjectKind = display_gobj->obj_kind;
    }
}

static void ndsOpeningRoomStoreTextureMaterialCandidate(GObj *gobj,
                                                        DObj *dobj,
                                                        Gfx *dl,
                                                        u32 callback_marker,
                                                        const MObj *mobj,
                                                        u32 mobj_count,
                                                        u32 effective_flags)
{
    u32 sprite_curr = 0;
    u32 sprite_next = 0;
    u32 mask;

    if ((dobj == NULL) || (mobj == NULL) ||
        (gNdsOpeningRoomDrawTextureMaterialResult ==
         NDS_OPENING_ROOM_DRAW_TEXTURE_MATERIAL_PASS))
    {
        return;
    }

    mask = ndsOpeningRoomMakeTextureMObjMask(
        mobj, effective_flags, &sprite_curr, &sprite_next);

    gNdsOpeningRoomDrawTextureMaterialResult =
        NDS_OPENING_ROOM_DRAW_TEXTURE_MATERIAL_PASS;
    gNdsOpeningRoomDrawTextureMaterialObjectDLLink =
        (gobj != NULL) ? gobj->dl_link_id : 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialObjectID =
        (gobj != NULL) ? gobj->id : 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialObjectKind =
        (gobj != NULL) ? gobj->obj_kind : 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialCallback = callback_marker;
    gNdsOpeningRoomDrawTextureMaterialDObjDL = (u32)(uintptr_t)dl;
    gNdsOpeningRoomDrawTextureMaterialDObjMeta =
        ndsOpeningRoomMakeDObjMeta(dobj, dl);
    gNdsOpeningRoomDrawTextureMaterialMObjFlags = mobj->sub.flags;
    gNdsOpeningRoomDrawTextureMaterialMObjEffectiveFlags = effective_flags;
    gNdsOpeningRoomDrawTextureMaterialMObjMask = mask;
    gNdsOpeningRoomDrawTextureMaterialMObjTextureCurr =
        mobj->texture_id_curr;
    gNdsOpeningRoomDrawTextureMaterialMObjTextureNext =
        mobj->texture_id_next;
    gNdsOpeningRoomDrawTextureMaterialMObjSpriteArray =
        (u32)(uintptr_t)mobj->sub.sprites;
    gNdsOpeningRoomDrawTextureMaterialMObjSpriteCurr = sprite_curr;
    gNdsOpeningRoomDrawTextureMaterialMObjSpriteNext = sprite_next;
    (void)mobj_count;
}

static void ndsOpeningRoomRecordTextureMaterialDObj(GObj *gobj,
                                                    DObj *dobj,
                                                    Gfx *dl,
                                                    u32 callback_marker)
{
    MObj *mobj;
    u32 mobj_count = 0;
    ub8 counted_dobj = FALSE;

    if (dobj == NULL)
    {
        return;
    }

    for (mobj = dobj->mobj; (mobj != NULL) && (mobj_count < 64u);
         mobj = mobj->next)
    {
        mobj_count++;
    }

    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        u32 effective_flags = ndsOpeningRoomGetEffectiveMObjFlags(mobj);
        u32 sprite_curr = 0;
        u32 sprite_next = 0;
        u32 mask;

        if ((effective_flags & MOBJ_FLAG_TEXTURE) == 0)
        {
            continue;
        }

        if (counted_dobj == FALSE)
        {
            gNdsOpeningRoomDrawTextureMaterialCandidateCount++;
            counted_dobj = TRUE;
        }
        gNdsOpeningRoomDrawTextureMaterialMObjCount++;

        mask = ndsOpeningRoomMakeTextureMObjMask(
            mobj, effective_flags, &sprite_curr, &sprite_next);
        if ((mask & NDS_OPENING_ROOM_DL_MATERIAL_HAS_SPRITE_ARRAY) != 0)
        {
            gNdsOpeningRoomDrawTextureMaterialSpriteArrayCount++;
        }
        if ((mask & NDS_OPENING_ROOM_DL_MATERIAL_HAS_CURR_SPRITE) != 0)
        {
            gNdsOpeningRoomDrawTextureMaterialSpriteCurrCount++;
        }
        if ((mask & NDS_OPENING_ROOM_DL_MATERIAL_HAS_NEXT_SPRITE) != 0)
        {
            gNdsOpeningRoomDrawTextureMaterialSpriteNextCount++;
        }

        ndsOpeningRoomStoreTextureMaterialCandidate(
            gobj, dobj, dl, callback_marker, mobj, mobj_count,
            effective_flags);
    }
}

static void ndsOpeningRoomRecordTextureMaterialRecurse(GObj *gobj,
                                                       DObj *dobj,
                                                       u32 callback_marker,
                                                       u32 *visited)
{
    while ((dobj != NULL) && (*visited < 64u))
    {
        Gfx *dl;

        (*visited)++;
        dl = ndsOpeningRoomGetDObjDLForCallback(dobj, callback_marker);
        if ((dl != NULL) &&
            (ndsOpeningRoomDObjDrawableForCallback(dobj, callback_marker) !=
             FALSE) &&
            (dobj->mobj != NULL))
        {
            ndsOpeningRoomRecordTextureMaterialDObj(
                gobj, dobj, dl, callback_marker);
        }

        if ((ndsOpeningRoomCallbackWalksDObjTree(callback_marker) != FALSE) &&
            (dobj->child != NULL))
        {
            ndsOpeningRoomRecordTextureMaterialRecurse(
                gobj, dobj->child, callback_marker, visited);
        }

        if (ndsOpeningRoomCallbackWalksDObjTree(callback_marker) == FALSE)
        {
            break;
        }
        dobj = dobj->sib_next;
    }
}

static void ndsOpeningRoomRecordTextureMaterialCandidates(GObj *gobj,
                                                         DObj *dobj,
                                                         u32 callback_marker)
{
    u32 visited = 0;

    ndsOpeningRoomRecordTextureMaterialRecurse(
        gobj, dobj, callback_marker, &visited);
}

static void ndsOpeningRoomRecordDObjDraw(GObj *gobj, u32 callback_marker)
{
    DObj *dobj = (gobj != NULL) ? DObjGetStruct(gobj) : NULL;
    Gfx *first_dl = NULL;
    Gfx *material_dl = NULL;
    DObj *first_dobj;
    DObj *material_dobj;

    if (gSCManagerSceneData.scene_curr != nSCKindOpeningRoom)
    {
        return;
    }
    gNdsOpeningRoomDrawDObjCallbackCount++;
    gNdsOpeningRoomDrawBlocker =
        NDS_OPENING_ROOM_DRAW_BLOCKER_DOBJ_DISPLAY_LIST;

    first_dobj = ndsOpeningRoomFindDObjDrawCandidate(
        dobj,
        callback_marker,
        FALSE,
        &first_dl);
    material_dobj = ndsOpeningRoomFindDObjDrawCandidate(
        dobj,
        callback_marker,
        TRUE,
        &material_dl);
    ndsOpeningRoomRecordTextureMaterialCandidates(
        gobj,
        dobj,
        callback_marker);

    if ((first_dobj != NULL) &&
        (sNdsOpeningRoomFallbackPreviewDObj == NULL))
    {
        sNdsOpeningRoomFallbackPreviewCameraGObj =
            sNdsOpeningRoomCurrentDrawCameraGObj;
        sNdsOpeningRoomFallbackPreviewGObj = gobj;
        sNdsOpeningRoomFallbackPreviewDObj = first_dobj;
        sNdsOpeningRoomFallbackPreviewDL = first_dl;
    }

    if (material_dobj != NULL)
    {
        gNdsOpeningRoomDrawMaterialCandidateCount++;
        if (sNdsOpeningRoomMaterialPreviewDObj == NULL)
        {
            sNdsOpeningRoomMaterialPreviewCameraGObj =
                sNdsOpeningRoomCurrentDrawCameraGObj;
            sNdsOpeningRoomMaterialPreviewGObj = gobj;
            sNdsOpeningRoomMaterialPreviewDObj = material_dobj;
            sNdsOpeningRoomMaterialPreviewDL = material_dl;
            gNdsOpeningRoomDrawMaterialCandidateResult =
                NDS_OPENING_ROOM_DRAW_MATERIAL_CANDIDATE_PASS;
            gNdsOpeningRoomDrawMaterialCandidateCameraMaskLow =
                (sNdsOpeningRoomCurrentDrawCameraGObj != NULL) ?
                    (u32)(sNdsOpeningRoomCurrentDrawCameraGObj->camera_mask &
                          0xffffffffu) :
                    0u;
            gNdsOpeningRoomDrawMaterialCandidateCameraPriority =
                (sNdsOpeningRoomCurrentDrawCameraGObj != NULL) ?
                    sNdsOpeningRoomCurrentDrawCameraGObj->dl_link_priority :
                    0xffffffffu;
            gNdsOpeningRoomDrawMaterialCandidateObjectDLLink =
                (gobj != NULL) ? gobj->dl_link_id : 0xffffffffu;
            gNdsOpeningRoomDrawMaterialCandidateObjectID =
                (gobj != NULL) ? gobj->id : 0xffffffffu;
            gNdsOpeningRoomDrawMaterialCandidateObjectKind =
                (gobj != NULL) ? gobj->obj_kind : 0xffffffffu;
            gNdsOpeningRoomDrawMaterialCandidateCallback = callback_marker;
            gNdsOpeningRoomDrawMaterialCandidateDObjDL =
                (u32)(uintptr_t)material_dl;
            gNdsOpeningRoomDrawMaterialCandidateDObjMeta =
                ndsOpeningRoomMakeDObjMeta(material_dobj, material_dl);
            {
                NDSOpeningRoomMaterialProbe probe;

                ndsOpeningRoomProbeDObjMaterial(material_dobj, &probe);
                ndsOpeningRoomStoreCandidateMaterial(&probe);
                ndsOpeningRoomRecordMaterialBranch(material_dobj);
                ndsOpeningRoomEmitMaterialBranch(material_dobj);
            }
        }
    }

    if (gNdsOpeningRoomDrawFirstCallback != 0)
    {
        return;
    }

    gNdsOpeningRoomDrawFirstCallback = callback_marker;
    if (gNdsOpeningRoomDrawFirstObjectDLLink == 0xffffffffu)
    {
        gNdsOpeningRoomDrawFirstObjectDLLink =
            (gobj != NULL) ? gobj->dl_link_id : 0xffffffffu;
        gNdsOpeningRoomDrawFirstObjectID =
            (gobj != NULL) ? gobj->id : 0xffffffffu;
        gNdsOpeningRoomDrawFirstObjectKind =
            (gobj != NULL) ? gobj->obj_kind : 0xffffffffu;
    }
    if (first_dobj != NULL)
    {
        gNdsOpeningRoomDrawFirstDObjDL = (u32)(uintptr_t)first_dl;
        gNdsOpeningRoomDrawFirstDObjMeta =
            ndsOpeningRoomMakeDObjMeta(first_dobj, first_dl);
    }
}

/* Retired selected preview. Probes run as retired stubs that
 * publish native failures; no list is executed or rasterized here. */
static void ndsOpeningRoomRenderSelectedDLPreview(void)
{
    ndsOpeningRoomProbeMaterialDLShape();
    ndsOpeningRoomProbeMaterialDLExpandedShape();
    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_STAGE,
        (u32)gSCManagerSceneData.scene_curr, 0xffffffffu, 0u, 0u, 0u,
        NDS_NATIVE_FAILURE_NO_PROGRAM);
}

void __attribute__((section(".itcm")))
gcCaptureCameraGObj(GObj *camera_gobj, sb32 is_tag_mask_or_id)
{
    u64 camera_mask;
    s32 link_id = 0;

    if (camera_gobj == NULL)
    {
        return;
    }

    camera_mask = camera_gobj->camera_mask;
    while (camera_mask != 0)
    {
        if ((camera_mask & 1u) != 0)
        {
            GObj *current_gobj = gGCCommonDLLinks[link_id];

            while (current_gobj != NULL)
            {
                if (((current_gobj->flags & GOBJ_FLAG_HIDDEN) == 0) &&
                    (current_gobj->proc_display != NULL) &&
                    (((is_tag_mask_or_id == FALSE) &&
                      ((camera_gobj->camera_tag &
                        current_gobj->camera_tag) != 0)) ||
                     ((is_tag_mask_or_id != FALSE) &&
                      (camera_gobj->camera_tag ==
                       current_gobj->camera_tag))))
                {
                    GObj *prev_camera_gobj =
                        sNdsOpeningRoomCurrentDrawCameraGObj;
                    sb32 native_stage_handled;

                    dGCCurrentStatus = nGCStatusDisplaying;
                    gGCCurrentDisplay = current_gobj;

                    native_stage_handled =
                        ndsStageGCDrawAllLoopRecordCapturedDisplay(
                            camera_gobj, current_gobj, link_id);
                    ndsOpeningRoomRecordCapturedDisplay(camera_gobj,
                                                        current_gobj,
                                                        link_id);
                    sNdsOpeningRoomCurrentDrawCameraGObj = camera_gobj;
                    if (native_stage_handled == FALSE)
                    {
                        current_gobj->proc_display(current_gobj);
                    }
                    sNdsOpeningRoomCurrentDrawCameraGObj = prev_camera_gobj;

                    dGCCurrentStatus = nGCStatusCapturing;
                    current_gobj->frame_draw_last = dSYTaskmanFrameCount;
                }
                current_gobj = current_gobj->dl_link_next;
            }
        }
        camera_mask >>= 1;
        link_id++;
    }
}

static void ndsOpeningRoomRecordDrawCamera(GObj *gobj, CObj *cobj)
{
    if ((gobj == NULL) || (cobj == NULL))
    {
        return;
    }
    if (gNdsOpeningRoomDrawFirstCameraPriority != 0xffffffffu)
    {
        return;
    }

    gNdsOpeningRoomDrawFirstCameraMaskLow =
        (u32)(gobj->camera_mask & 0xffffffffu);
    gNdsOpeningRoomDrawFirstCameraPriority = gobj->dl_link_priority;
    gNdsOpeningRoomDrawFirstCameraFlags = cobj->flags;
    gNdsOpeningRoomDrawFirstCameraXObjCount = cobj->xobjs_num;
    gNdsOpeningRoomDrawFirstCameraXObjKind0 =
        ((cobj->xobjs_num > 0) && (cobj->xobjs[0] != NULL)) ?
            cobj->xobjs[0]->kind : 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraXObjKind1 =
        ((cobj->xobjs_num > 1) && (cobj->xobjs[1] != NULL)) ?
            cobj->xobjs[1]->kind : 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraViewportScaleX =
        cobj->viewport.vp.vscale[0];
    gNdsOpeningRoomDrawFirstCameraViewportScaleY =
        cobj->viewport.vp.vscale[1];
    gNdsOpeningRoomDrawFirstCameraViewportTransX =
        cobj->viewport.vp.vtrans[0];
    gNdsOpeningRoomDrawFirstCameraViewportTransY =
        cobj->viewport.vp.vtrans[1];
    gNdsOpeningRoomDrawFirstCameraNear100 =
        ndsPreviewFloatToCenti(cobj->projection.persp.near);
    gNdsOpeningRoomDrawFirstCameraFar100 =
        ndsPreviewFloatToCenti(cobj->projection.persp.far);
    gNdsOpeningRoomDrawFirstCameraFovY100 =
        ndsPreviewFloatToCenti(cobj->projection.persp.fovy);
    gNdsOpeningRoomDrawFirstCameraEyeX100 =
        ndsPreviewFloatToCenti(cobj->vec.eye.x);
    gNdsOpeningRoomDrawFirstCameraEyeY100 =
        ndsPreviewFloatToCenti(cobj->vec.eye.y);
    gNdsOpeningRoomDrawFirstCameraEyeZ100 =
        ndsPreviewFloatToCenti(cobj->vec.eye.z);
    gNdsOpeningRoomDrawFirstCameraAtX100 =
        ndsPreviewFloatToCenti(cobj->vec.at.x);
    gNdsOpeningRoomDrawFirstCameraAtY100 =
        ndsPreviewFloatToCenti(cobj->vec.at.y);
    gNdsOpeningRoomDrawFirstCameraAtZ100 =
        ndsPreviewFloatToCenti(cobj->vec.at.z);
}

void func_80017DBC(GObj *gobj)
{
#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
    extern volatile u32 gNdsVSResultsCameraProcCount;
#endif
    CObj *cobj;

    if (gobj == NULL)
    {
        return;
    }
    cobj = CObjGetStruct(gobj);
    if (cobj == NULL)
    {
        return;
    }
#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
    if (gSCManagerSceneData.scene_curr == nSCKindVSResults)
    {
        gNdsVSResultsCameraProcCount++;
    }
#endif
    gcCaptureCameraGObj(
        gobj, (cobj->flags & COBJ_FLAG_IDENTIFIER) ? TRUE : FALSE);
}

void func_80017EC0(GObj *gobj)
{
    CObj *cobj;

    if (gobj == NULL)
    {
        return;
    }

    cobj = CObjGetStruct(gobj);
    if (cobj == NULL)
    {
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindOpeningRoom)
    {
        gNdsOpeningRoomDrawCameraCallbackCount++;
        ndsOpeningRoomRecordDrawCamera(gobj, cobj);
        if (gNdsOpeningRoomDrawBlocker == NDS_OPENING_ROOM_DRAW_BLOCKER_NONE)
        {
            gNdsOpeningRoomDrawBlocker =
                NDS_OPENING_ROOM_DRAW_BLOCKER_CAMERA_BACKEND;
        }
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gNdsFighterGCDrawAllLoopPrepared != 0u) &&
        (gNdsFighterMarioFoxGCDrawAllLoopResult == 0u))
    {
        gNdsFighterGCDrawAllLoopCameraCallbackCount++;
    }
    ndsStageGCDrawAllLoopRecordCameraCallback();

    gcCaptureCameraGObj(gobj,
                        (cobj->flags & COBJ_FLAG_IDENTIFIER) ? TRUE : FALSE);
}

void gcDrawDObjTreeForGObj(GObj *gobj)
{
    ndsStageGCDrawAllLoopRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE);
    ndsOpeningRoomRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE);
}

void gcDrawDObjTreeDLLinksForGObj(GObj *gobj)
{
    ndsStageGCDrawAllLoopRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS);
    ndsOpeningRoomRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS);
}

void gcDrawDObjDLLinksForGObj(GObj *gobj)
{
    ndsStageGCDrawAllLoopRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS);
    ndsOpeningRoomRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS);
}

void gcDrawDObjDLHead0(GObj *gobj)
{
    ndsStageGCDrawAllLoopRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0);
    ndsOpeningRoomRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0);
}

void gcDrawDObjDLHead1(GObj *gobj)
{
    ndsStageGCDrawAllLoopRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1);
    ndsOpeningRoomRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1);
}

void scManagerRunPrintGObjStatus(void)
{
}

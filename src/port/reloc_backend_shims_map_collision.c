/* Source-function shims, the map: player control locks, the map collision
 * (ground data, yakumono lines and bounds, map objects), rumble and the RDP
 * lights. A slice of reloc_backend.c. */

void ftParamLockPlayerControl(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->input.pl.button_hold = fp->input.pl.button_tap =
        fp->input.cp.button_inputs = 0;
    fp->input.pl.stick_range.x = fp->input.pl.stick_range.y =
        fp->input.pl.stick_prev.x = fp->input.pl.stick_prev.y =
        fp->input.cp.stick_range.x = fp->input.cp.stick_range.y = 0;
    fp->tap_stick_x = fp->tap_stick_y =
        fp->hold_stick_x = fp->hold_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->is_control_disable = TRUE;
}

void ftParamUnlockPlayerControl(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->is_control_disable = FALSE;
}


void battleship_ftKeyProcessKeyEvents(GObj *fighter_gobj);

void ftKeyProcessKeyEvents(GObj *fighter_gobj)
{
    battleship_ftKeyProcessKeyEvents(fighter_gobj);
}

void ftParamTryUpdateItemMusic(void)
{
    u32 bgm_play = gMPCollisionBGMDefault;
    s32 length = ftParamGetItemMusicLength(bgm_play);
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        u32 bgm_id = gMPCollisionBGMDefault;
        s32 length_new;

        if ((fp->item_gobj != NULL) && (itGetStruct(fp->item_gobj)->kind == nITKindHammer))
        {
            bgm_id = nSYAudioBGMHammer;
        }
        if (fp->star_invincible_tics > ITSTAR_WARN_BEGIN_FRAME)
        {
            bgm_id = nSYAudioBGMStar;
        }
        length_new = ftParamGetItemMusicLength(bgm_id);

        if (length < length_new)
        {
            length = length_new;
            bgm_play = bgm_id;
        }
        fighter_gobj = fighter_gobj->link_next;
    }
    if (bgm_play != gMPCollisionBGMCurrent)
    {
        syAudioPlayBGM(0, bgm_play);
        gMPCollisionBGMCurrent = bgm_play;
    }
}



GObj *ftShadowMakeShadow(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    return NULL;
}

/* Skipped-particle shims. NDS_R2_PARTICLE_RUNTIME=1 compiles the original
 * lb/lbparticle.c in place (src/import/battleship_lbparticle.c), which owns
 * every symbol below. */

/* The source quake is the only driver of camera shake: efManagerQuakeProcUpdate
 * feeds the effect's animated translate into gmCameraSetVelAt, which
 * gmCameraApplyVel adds to the look-at.  Recording the request and returning
 * NULL therefore silenced every shake in the game -- Whispy's wind rumble
 * (grpupupu.c:313), the KO burst, and wall damage.  Run the original whenever
 * the pieces it needs are compiled in: battleship_efmanager.c supplies the
 * effect and battleship_gmcamera.c supplies gGMCameraGObj, which only
 * NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE builds have. */
GObj *ndsBaseEFManagerQuakeMakeEffect(s32 magnitude);

GObj *efManagerQuakeMakeEffect(s32 id)
{
    NDS_DIAG(gNdsPupupuUpdateQuakeCount++);
    return ndsBaseEFManagerQuakeMakeEffect(id);
}


/* renderer_adapter_matrix.c, included later in this translation unit. */
void ndsRendererAdapterNativeStagePreloadAssets(void);

/* THE STAGE'S DEAD BYTES (P7, 2026-10-21). A native stage's geometry file
 * still carries the display lists and vertex arrays its packet was generated
 * from, and once the packet loads nothing reads them: the owner draws the
 * packet, and a layer's DL pointer is only compared by the binding check.
 * Hyrule Temple's are 40,224 B of the 59,856 B file. A VS battle lends them as
 * allocation space (syTaskmanMalloc takes them after the loan), and the reloc
 * registry stops calling an address inside them the file's. A layer's source
 * draw, run only on a frame the owner declines (the heaviest mixes never
 * showed one, 2026-10-21), meets a lent list in the stage body, which rejects
 * it without reading it (renderer_adapter_stage.c, counted in
 * gNdsNativeStageLentLayerSkips). The rows are
 * scripts/p5/stage_generate.py's dead_spans and, for the vanilla VS stages
 * (8.6-14.6 KB each), scripts/stages/generate_stage_dead_spans.py's. */
void ndsBattleDeadSpanAdd(void *base, u32 bytes);
__attribute__((used)) volatile u32 gNdsNativeStageLentLayerSkips;

#if __has_include("nds_stage_dead_spans.generated.h")
/* The VS stages' rows (scripts/stages/generate_stage_dead_spans.py). */
#include "nds_stage_dead_spans.generated.h"
#endif
#if !defined(NDS_STAGE_DEAD_SPAN_ROWS)
#define NDS_STAGE_DEAD_SPAN_ROWS(X)
#endif
#if !(NDS_P5 && defined(NDS_P5_DEAD_SPAN_ROWS))
#undef NDS_P5_DEAD_SPAN_ROWS
#define NDS_P5_DEAD_SPAN_ROWS(X)
#endif

void ndsNativeStageLendDeadSpans(u32 gkind)
{
    static const struct
    {
        u32 gkind;
        u32 file;
        u32 file_bytes;
        u32 offset;
        u32 bytes;
    } rows[] = {
#define NDS_DEAD_SPAN_ROW(g, file, file_bytes, offset, bytes) \
        { (g), (file), (file_bytes), (offset), (bytes) },
        NDS_STAGE_DEAD_SPAN_ROWS(NDS_DEAD_SPAN_ROW)
        NDS_P5_DEAD_SPAN_ROWS(NDS_DEAD_SPAN_ROW)
#undef NDS_DEAD_SPAN_ROW
        { 0xffffffffu, 0u, 0u, 0u, 0u }
    };
    u32 lo[4];
    u32 hi[4];
    u32 file = 0u;
    u32 file_bytes = 0u;
    u32 count = 0u;
    u32 i;
    u8 *data;

    if ((gSCManagerSceneData.scene_curr != nSCKindVSBattle) ||
        (ndsNativeStageBlobPacket() == NULL))
    {
        return;
    }
    for (i = 0u; i < ARRAY_COUNT(rows); i++)
    {
        if ((rows[i].gkind != gkind) || (count >= ARRAY_COUNT(lo)) ||
            ((count != 0u) && (rows[i].file != file)))
        {
            continue;
        }
        file = rows[i].file;
        file_bytes = rows[i].file_bytes;
        lo[count] = rows[i].offset;
        hi[count] = rows[i].offset + rows[i].bytes;
        count++;
    }
    if (count == 0u)
    {
        return;
    }
    data = (u8 *)ndsRelocLendDeadSpans(file, file_bytes, lo, hi, count);
    if (data == NULL)
    {
        return;
    }
    for (i = 0u; i < count; i++)
    {
        ndsBattleDeadSpanAdd(data + lo[i], hi[i] - lo[i]);
    }
}

void mpCollisionInitGroundData(void)
{
    ndsMPCollisionInvalidateTopology();
    gMPCollisionGroundData = NULL;
    ndsMPCollisionSetGeometry(NULL);
    gMPCollisionMapObjs = NULL;
    if (gMPCollisionYakumonoDObjs != NULL)
    {
        memset(gMPCollisionYakumonoDObjs->dobjs, 0,
               sizeof(gMPCollisionYakumonoDObjs->dobjs));
    }
    if (gMPCollisionSpeeds != NULL)
    {
        memset(gMPCollisionSpeeds, 0,
               NDS_MP_YAKUMONO_DOBJ_SLOTS * sizeof(*gMPCollisionSpeeds));
    }
    /* EVERY STAGE, OUT OF ONE TABLE.
     *
     * This was one fifty-line arm per stage, copied five times, and the four
     * stages that never got an arm -- Hyrule Castle, Saffron City, Mushroom
     * Kingdom and Sector Z -- reached their battle with gMPCollisionGroundData
     * still NULL and aborted on the first dereference of it, inside
     * grWallpaperMakeCommon. The owner heard the same four stages die on entry.
     *
     * The arms only ever differed in three values -- the ground kind, the map
     * file, and the header offset within it -- so three values is what they are
     * now, and a stage landing without an entry here is a missing ROW rather
     * than fifty missing lines. */
    {
        /* decomp mp/mpcollision.c:26-67 verbatim, gkind-indexed exactly as
         * mpCollisionInitGroundData indexes it there (nGRKindCastle 0 ..
         * nGRKindBonus2Ness 40): every ground the game has, in one table,
         * so a stage lands by staging its files, never by adding a row. A
         * kind whose file is not staged reads size 0 below and leaves the
         * ground data NULL, the same failure the missing-row case had,
         * but named by the file rather than by a missing arm. The row shape
         * is the source's GRFileInfo (gr/ground.h:122) spelled out, since
         * this TU does not include the ground header. */
        static const struct {
            uintptr_t *file_id;
            uintptr_t *offset;
        } dMPCollisionGroundFileInfos[] = {
            { &llGRCastleMapFileID, &llGRCastleMapMapHeader }, /*  0 nGRKindCastle */
            { &llGRSectorMapFileID, &llGRSectorMapMapHeader }, /*  1 nGRKindSector */
            { &llGRJungleMapFileID, &llGRJungleMapMapHeader }, /*  2 nGRKindJungle */
            { &llGRZebesMapFileID, &llGRZebesMapMapHeader }, /*  3 nGRKindZebes */
            { &llGRHyruleMapFileID, &llGRHyruleMapMapHeader }, /*  4 nGRKindHyrule */
            { &llGRYosterMapFileID, &llGRYosterMapMapHeader }, /*  5 nGRKindYoster */
            { &llGRPupupuMapFileID, &llGRPupupuMapMapHeader }, /*  6 nGRKindPupupu */
            { &llGRYamabukiMapFileID, &llGRYamabukiMapMapHeader }, /*  7 nGRKindYamabuki */
            { &llGRInishieMapFileID, &llGRInishieMapMapHeader }, /*  8 nGRKindInishie */
            { &llGRPupupuSmallMapFileID, &llGRPupupuSmallMapMapHeader }, /*  9 nGRKindPupupuSmall */
            { &llGRPupupuTestMapFileID, &llGRPupupuTestMapMapHeader }, /* 10 nGRKindPupupuNew */
            { &llGRExplainMapFileID, &llGRExplainMapMapHeader }, /* 11 nGRKindExplain */
            { &llGRYosterSmallMapFileID, &llGRYosterSmallMapMapHeader }, /* 12 nGRKindYosterSmall */
            { &llGRMetalMapFileID, &llGRMetalMapMapHeader }, /* 13 nGRKindMetal */
            { &llGRZakoMapFileID, &llGRZakoMapMapHeader }, /* 14 nGRKindZako */
            { &llGRBonus3MapFileID, &llGRBonus3MapMapHeader }, /* 15 nGRKindBonus3 */
            { &llGRLastMapFileID, &llGRLastMapMapHeader }, /* 16 nGRKindLast */
            { &llGRBonus1MarioMapFileID, &llGRBonus1MarioMapMapHeader }, /* 17 nGRKindBonus1Mario */
            { &llGRBonus1FoxMapFileID, &llGRBonus1FoxMapMapHeader }, /* 18 nGRKindBonus1Fox */
            { &llGRBonus1DonkeyMapFileID, &llGRBonus1DonkeyMapMapHeader }, /* 19 nGRKindBonus1Donkey */
            { &llGRBonus1SamusMapFileID, &llGRBonus1SamusMapMapHeader }, /* 20 nGRKindBonus1Samus */
            { &llGRBonus1LuigiMapFileID, &llGRBonus1LuigiMapMapHeader }, /* 21 nGRKindBonus1Luigi */
            { &llGRBonus1LinkMapFileID, &llGRBonus1LinkMapMapHeader }, /* 22 nGRKindBonus1Link */
            { &llGRBonus1YoshiMapFileID, &llGRBonus1YoshiMapMapHeader }, /* 23 nGRKindBonus1Yoshi */
            { &llGRBonus1CaptainMapFileID, &llGRBonus1CaptainMapMapHeader }, /* 24 nGRKindBonus1Captain */
            { &llGRBonus1KirbyMapFileID, &llGRBonus1KirbyMapMapHeader }, /* 25 nGRKindBonus1Kirby */
            { &llGRBonus1PikachuMapFileID, &llGRBonus1PikachuMapMapHeader }, /* 26 nGRKindBonus1Pikachu */
            { &llGRBonus1PurinMapFileID, &llGRBonus1PurinMapMapHeader }, /* 27 nGRKindBonus1Purin */
            { &llGRBonus1NessMapFileID, &llGRBonus1NessMapMapHeader }, /* 28 nGRKindBonus1Ness */
            { &llGRBonus2MarioMapFileID, &llGRBonus2MarioMapMapHeader }, /* 29 nGRKindBonus2Mario */
            { &llGRBonus2FoxMapFileID, &llGRBonus2FoxMapMapHeader }, /* 30 nGRKindBonus2Fox */
            { &llGRBonus2DonkeyMapFileID, &llGRBonus2DonkeyMapMapHeader }, /* 31 nGRKindBonus2Donkey */
            { &llGRBonus2SamusMapFileID, &llGRBonus2SamusMapMapHeader }, /* 32 nGRKindBonus2Samus */
            { &llGRBonus2LuigiMapFileID, &llGRBonus2LuigiMapMapHeader }, /* 33 nGRKindBonus2Luigi */
            { &llGRBonus2LinkMapFileID, &llGRBonus2LinkMapMapHeader }, /* 34 nGRKindBonus2Link */
            { &llGRBonus2YoshiMapFileID, &llGRBonus2YoshiMapMapHeader }, /* 35 nGRKindBonus2Yoshi */
            { &llGRBonus2CaptainMapFileID, &llGRBonus2CaptainMapMapHeader }, /* 36 nGRKindBonus2Captain */
            { &llGRBonus2KirbyMapFileID, &llGRBonus2KirbyMapMapHeader }, /* 37 nGRKindBonus2Kirby */
            { &llGRBonus2PikachuMapFileID, &llGRBonus2PikachuMapMapHeader }, /* 38 nGRKindBonus2Pikachu */
            { &llGRBonus2PurinMapFileID, &llGRBonus2PurinMapMapHeader }, /* 39 nGRKindBonus2Purin */
            { &llGRBonus2NessMapFileID, &llGRBonus2NessMapMapHeader }, /* 40 nGRKindBonus2Ness */
        };
        u32 i;
#if NDS_P5
        /* P5: the Remix stages' kinds follow the source's last ground, their
         * map file and header offset from scripts/p5/stage_export.py. */
        const u32 kinds = NDS_P5_GKIND_MAX + 1u;
#else
        const u32 kinds = ARRAY_COUNT(dMPCollisionGroundFileInfos);
#endif

        for (i = 0u; i < kinds; i++)
        {
            void *file;
            MPGroundData *ground_data;
            size_t bytes;
            uintptr_t *file_id;
            uintptr_t *header;

            if ((gSCManagerBattleState == NULL) ||
                (gSCManagerBattleState->gkind != (s32)i))
            {
                continue;
            }
            if (i < ARRAY_COUNT(dMPCollisionGroundFileInfos))
            {
                file_id = dMPCollisionGroundFileInfos[i].file_id;
                header = dMPCollisionGroundFileInfos[i].offset;
            }
            else
            {
                file_id = NULL;
                header = NULL;
#if NDS_P5
#define NDS_P5_GROUND_ROW(g, cname, name, map_file, map_offset) \
                if (i == (g)) \
                { \
                    file_id = (uintptr_t *)(uintptr_t)(map_file); \
                    header = (uintptr_t *)(uintptr_t)(map_offset); \
                }
                NDS_P5_NATIVE_STAGE_ROWS(NDS_P5_GROUND_ROW)
#undef NDS_P5_GROUND_ROW
#endif
                if (file_id == NULL)
                {
                    break;
                }
            }
            bytes = lbRelocGetFileSize(file_id);
            if (bytes == 0u)
            {
                break;
            }
            file = lbRelocGetExternHeapFile(file_id, syTaskmanMalloc(bytes, 0x10));
            ground_data = lbRelocGetFileData(MPGroundData*, file, header);
            if (ground_data == NULL)
            {
                break;
            }
            gMPCollisionGroundData = ground_data;
            ndsMPCollisionSetGeometry(ground_data->map_geometry);
            gMPCollisionMapObjs = (gMPCollisionGeometry != NULL) ?
                gMPCollisionGeometry->mapobjs : NULL;
            gMPCollisionLightAngleX = ground_data->light_angle.x;
            gMPCollisionLightAngleY = ground_data->light_angle.y;
#if NDS_P5
            /* Remix BGM.asm: a stage's entry in Stages.default_music_track_table
             * replaces its header's song as it loads (Hyrule Temple's header
             * names Hyrule Castle's track). */
#define NDS_P5_STAGE_BGM_ROW(g, bgm) \
            if (i == (u32)(g)) \
            { \
                ground_data->bgm_id = (bgm); \
            }
            NDS_P5_STAGE_BGM_ROWS(NDS_P5_STAGE_BGM_ROW)
#undef NDS_P5_STAGE_BGM_ROW
#endif
            gMPCollisionBGMDefault = ground_data->bgm_id;
            gMPCollisionBGMCurrent = ground_data->bgm_id;

            gNdsSCVSBattleStageResult = NDS_STAGE_PUPUPU_BATTLE_PASS;
            gNdsSCVSBattleStageGKind = (s32)i;
            gNdsSCVSBattleStageGroundDataReady = 1;
            /* Blob-resident stage packets load here, beside the ground they
             * draw over: Dream Land's packet stays linked and skips the
             * load, every other kind reads its relocatable blob into the
             * scene arena now, while load time is cheap. A missing or
             * corrupt blob leaves the blob packet NULL and the stage
             * declines through the existing unresolved-kind path. */
            if (i != NDS_NATIVE_STAGE_BLOB_GKIND_DREAMLAND)
            {
                ndsNativeStageBlobLoad(i);
            }
            /* And every asset that packet references (defined later in this
             * translation unit, renderer_adapter_matrix.c). */
            ndsRendererAdapterNativeStagePreloadAssets();
            ndsNativeStageLendDeadSpans(i);
            gNdsSCVSBattleStageMask |= (1u << 0);
            gNdsSCVSBattleStageMask |= (1u << 1);
            gNdsSCVSBattleStageMask |= (1u << 2);

            if (ground_data->map_geometry != NULL)
            {
                gNdsSCVSBattleStageGeometryReady = 1;
                gNdsSCVSBattleStageMask |= (1u << 3);
            }
            if ((ground_data->map_nodes != NULL) ||
                ((ground_data->map_geometry != NULL) &&
                 (ground_data->map_geometry->mapobjs != NULL)))
            {
                gNdsSCVSBattleStageMapNodesReady = 1;
                gNdsSCVSBattleStageMask |= (1u << 4);
            }
            gNdsSCVSBattleStageLightAngleXBits =
                ndsFloatBits(ground_data->light_angle.x);
            gNdsSCVSBattleStageLightAngleYBits =
                ndsFloatBits(ground_data->light_angle.y);
            gNdsSCVSBattleStageMask |= (1u << 5);

            gNdsSCVSBattleStageBGM = ground_data->bgm_id;
            gNdsSCVSBattleStageMask |= (1u << 6);
            gNdsSCVSBattleStageDeferredMask |= (1u << 0);
            gNdsSCVSBattleStageDeferredMask |= (1u << 1);
            gNdsSCVSBattleStageMask |= (1u << 7);
            break;
        }
    }

    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_GROUND_COLLISION;
}

extern GObj *gGMCameraGObj;

static GObj *ndsBattleCompatMainCameraGObj(void)
{
    return gGMCameraGObj;
}

void ndsGRCompatibilityNonPupupuSetup(void)
{
    if ((gSCManagerBattleState != NULL) &&
        (gSCManagerBattleState->gkind == nGRKindPupupu) &&
        (gNdsSCVSBattleStageGroundDataReady != 0u))
    {
        gNdsSCVSBattleStageMask |= (1u << 7);
        gNdsPupupuGroundDeferredMask |= 1u << 0;
    }
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_GROUND_COLLISION;
}

void mpCollisionClearYakumonoAll(void)
{
    s32 count;
    s32 i;

    if ((gMPCollisionYakumonoDObjs == NULL) || (gMPCollisionSpeeds == NULL))
    {
        gNdsPupupuGroundDeferredMask |= 1u << 5;
        return;
    }

    count = gMPCollisionYakumonosNum;
    if (count > NDS_MP_YAKUMONO_DOBJ_SLOTS)
    {
        count = NDS_MP_YAKUMONO_DOBJ_SLOTS;
    }
    for (i = 0; i < count; i++)
    {
        if (gMPCollisionYakumonoDObjs->dobjs[i] != NULL)
        {
            gMPCollisionYakumonoDObjs->dobjs[i]->user_data.s =
                nMPYakumonoStatusNone;
        }
        gMPCollisionSpeeds[i].x = 0.0F;
        gMPCollisionSpeeds[i].y = 0.0F;
        gMPCollisionSpeeds[i].z = 0.0F;
    }
    gMPCollisionUpdateTic = 0;
}

static void ndsMPBoundsInitEmpty(MPBounds *bounds)
{
    bounds->top = -65536.0F;
    bounds->right = -65536.0F;
    bounds->bottom = 65536.0F;
    bounds->left = 65536.0F;
}

static void ndsMPBoundsInclude(MPBounds *bounds, f32 vx, f32 vy,
                               sb32 current_style)
{
    if (current_style != FALSE)
    {
        if (bounds->top < vy)
        {
            bounds->top = vy;
        }
        else if (bounds->bottom > vy)
        {
            bounds->bottom = vy;
        }
        if (bounds->right < vx)
        {
            bounds->right = vx;
        }
        else if (bounds->left > vx)
        {
            bounds->left = vx;
        }
        return;
    }

    if (bounds->top < vy)
    {
        bounds->top = vy;
    }
    if (bounds->bottom > vy)
    {
        bounds->bottom = vy;
    }
    if (bounds->right < vx)
    {
        bounds->right = vx;
    }
    if (bounds->left > vx)
    {
        bounds->left = vx;
    }
}

static void ndsMPBoundsIncludeYakumonoLines(
    NDSMPO2RHalfwordView line_info, DObj *dobj, MPBounds *bounds,
    sb32 use_translate, sb32 current_style)
{
    MPVertexLinks *links = gMPCollisionGeometry->vertex_links;
    MPVertexArray *ids = gMPCollisionGeometry->vertex_id;
    MPVertexPosContainer *verts = gMPCollisionGeometry->vertex_data;
    f32 tx = (use_translate != FALSE) ? dobj->translate.vec.f.x : 0.0F;
    f32 ty = (use_translate != FALSE) ? dobj->translate.vec.f.y : 0.0F;
    u32 kind;

    for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
    {
        u32 line_id = ndsMPLineInfoGroupID(line_info, kind);
        u32 line_count = ndsMPLineInfoLineCount(line_info, kind);
        u32 i;

        if (line_count > 4096u)
        {
            line_count = 4096u;
        }
        for (i = 0u; i < line_count; i++, line_id++)
        {
            u32 first_vertex = ndsMPVertexLinkFirst(links, line_id);
            u32 vertex_count = ndsMPVertexLinkCount(links, line_id);
            u32 vertex_index;

            if ((vertex_count == 0u) || (vertex_count > 128u))
            {
                continue;
            }
            for (vertex_index = first_vertex;
                 vertex_index < (first_vertex + vertex_count);
                 vertex_index++)
            {
                u32 vertex_id = ndsMPVertexID(ids, vertex_index);
                f32 vx = (f32)ndsMPVertexX(verts, vertex_id) + tx;
                f32 vy = (f32)ndsMPVertexY(verts, vertex_id) + ty;

                ndsMPBoundsInclude(bounds, vx, vy, current_style);
            }
        }
    }
}

/* P2-6 (2026-10-03): ndsMPBoundsIncludeYakumonoLines with current_style
 * TRUE, over a valid box (top >= bottom, right >= left). There a vertex can
 * raise top or lower bottom but never both, so the else-if chain is a plain
 * max/min, and the box stays valid. Rounding is monotonic, so the largest
 * (f32)v + t over a line set is (f32)max(v) + t: the integer extremes are
 * found once (they depend only on the geometry) and four float adds replace
 * two adds and four float compares per vertex. Same bounds, bit for bit.
 * Board the Platforms runs this over every moving platform each tick: 28K
 * ticks a tick on Purin's board. */
typedef struct NDSMPYakumonoExtent
{
    s32 min_x;
    s32 max_x;
    s32 min_y;
    s32 max_y;
} NDSMPYakumonoExtent;

/* The integer extents depend only on the geometry, so they are found once per
 * line_info entry and kept while the geometry and the heap generation hold. */
static NDSMPYakumonoExtent sNdsMPYakumonoExtents[NDS_MP_YAKUMONO_DOBJ_SLOTS];
static u64 sNdsMPYakumonoExtentKnown;
static u64 sNdsMPYakumonoExtentAny;
static const void *sNdsMPYakumonoExtentGeometry;
static u32 sNdsMPYakumonoExtentGeneration;

static void ndsMPYakumonoExtentFind(NDSMPO2RHalfwordView line_info,
                                    NDSMPYakumonoExtent *extent, sb32 *any)
{
    MPVertexLinks *links = gMPCollisionGeometry->vertex_links;
    MPVertexArray *ids = gMPCollisionGeometry->vertex_id;
    MPVertexPosContainer *verts = gMPCollisionGeometry->vertex_data;
    s32 min_x = 0x7fffffff;
    s32 max_x = -0x7fffffff - 1;
    s32 min_y = 0x7fffffff;
    s32 max_y = -0x7fffffff - 1;
    sb32 found = FALSE;
    u32 kind;

    for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
    {
        u32 line_id = ndsMPLineInfoGroupID(line_info, kind);
        u32 line_count = ndsMPLineInfoLineCount(line_info, kind);
        u32 i;

        if (line_count > 4096u)
        {
            line_count = 4096u;
        }
        for (i = 0u; i < line_count; i++, line_id++)
        {
            u32 first_vertex = ndsMPVertexLinkFirst(links, line_id);
            u32 vertex_count = ndsMPVertexLinkCount(links, line_id);
            u32 vertex_index;

            if ((vertex_count == 0u) || (vertex_count > 128u))
            {
                continue;
            }
            for (vertex_index = first_vertex;
                 vertex_index < (first_vertex + vertex_count);
                 vertex_index++)
            {
                u32 vertex_id = ndsMPVertexID(ids, vertex_index);
                s32 x = ndsMPVertexX(verts, vertex_id);
                s32 y = ndsMPVertexY(verts, vertex_id);

                min_x = (x < min_x) ? x : min_x;
                max_x = (x > max_x) ? x : max_x;
                min_y = (y < min_y) ? y : min_y;
                max_y = (y > max_y) ? y : max_y;
                found = TRUE;
            }
        }
    }
    extent->min_x = min_x;
    extent->max_x = max_x;
    extent->min_y = min_y;
    extent->max_y = max_y;
    *any = found;
}

static void ndsMPBoundsIncludeYakumonoExtent(
    NDSMPO2RHalfwordView line_info, u32 info_index, DObj *dobj,
    MPBounds *bounds)
{
    const u64 bit = (u64)1u << info_index;
    const NDSMPYakumonoExtent *extent =
        &sNdsMPYakumonoExtents[info_index];
    f32 v;

    if ((sNdsMPYakumonoExtentGeometry != gMPCollisionGeometry) ||
        (sNdsMPYakumonoExtentGeneration != gNdsTaskmanHeapGeneration))
    {
        sNdsMPYakumonoExtentGeometry = gMPCollisionGeometry;
        sNdsMPYakumonoExtentGeneration = gNdsTaskmanHeapGeneration;
        sNdsMPYakumonoExtentKnown = 0u;
        sNdsMPYakumonoExtentAny = 0u;
    }
    if ((sNdsMPYakumonoExtentKnown & bit) == 0u)
    {
        sb32 any;

        ndsMPYakumonoExtentFind(line_info, &sNdsMPYakumonoExtents[info_index],
                                &any);
        sNdsMPYakumonoExtentKnown |= bit;
        if (any != FALSE)
        {
            sNdsMPYakumonoExtentAny |= bit;
        }
    }
    if ((sNdsMPYakumonoExtentAny & bit) == 0u)
    {
        return;
    }
    v = (f32)extent->max_y + dobj->translate.vec.f.y;
    if (bounds->top < v)
    {
        bounds->top = v;
    }
    v = (f32)extent->min_y + dobj->translate.vec.f.y;
    if (bounds->bottom > v)
    {
        bounds->bottom = v;
    }
    v = (f32)extent->max_x + dobj->translate.vec.f.x;
    if (bounds->right < v)
    {
        bounds->right = v;
    }
    v = (f32)extent->min_x + dobj->translate.vec.f.x;
    if (bounds->left > v)
    {
        bounds->left = v;
    }
}

void mpCollisionInitYakumonoAll(void)
{
    MPBounds bounds_moved;
    MPBounds bounds_static;
    MPLineInfo *line_info;
    u32 yakumono_count;
    u32 i;

    if ((ndsStageCollisionLoopGeometryReady() == FALSE) ||
        (gMPCollisionYakumonoDObjs == NULL))
    {
        gNdsPupupuGroundDeferredMask |= 1u << 5;
        return;
    }

    ndsMPBoundsInitEmpty(&bounds_moved);
    ndsMPBoundsInitEmpty(&bounds_static);
    line_info = gMPCollisionGeometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(gMPCollisionGeometry);
    if (yakumono_count > NDS_MP_YAKUMONO_DOBJ_SLOTS)
    {
        yakumono_count = NDS_MP_YAKUMONO_DOBJ_SLOTS;
    }

    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(line_info, i);
        u32 yakumono_id = ndsMPLineInfoYakumonoID(info);
        DObj *yakumono_dobj;

        if (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS)
        {
            continue;
        }
        yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
        if ((yakumono_dobj == NULL) ||
            (yakumono_dobj->user_data.s == nMPYakumonoStatusOn) ||
            (yakumono_dobj->user_data.s == nMPYakumonoStatusOff))
        {
            continue;
        }
        if (yakumono_dobj->anim_joint.event32 == NULL)
        {
            ndsMPBoundsIncludeYakumonoLines(info, yakumono_dobj,
                                            &bounds_static, FALSE, FALSE);
        }
        ndsMPBoundsIncludeYakumonoLines(info, yakumono_dobj, &bounds_moved,
                                        (yakumono_dobj->anim_joint.event32 !=
                                         NULL) ? TRUE : FALSE,
                                        FALSE);
    }

    gMPCollisionBounds.start = bounds_moved;
    gMPCollisionBounds.stop = bounds_static;
    gMPCollisionBounds.current = bounds_moved;
    gMPCollisionBounds.diff.top = 0.0F;
    gMPCollisionBounds.diff.bottom = 0.0F;
    gMPCollisionBounds.diff.right = 0.0F;
    gMPCollisionBounds.diff.left = 0.0F;
}

void mpCollisionUpdateBoundsCurrent(void)
{
    MPBounds bounds;
    MPLineInfo *line_info;
    u32 yakumono_count;
    sb32 extent;
    u32 i;

    if ((ndsStageCollisionLoopGeometryReady() == FALSE) ||
        (gMPCollisionYakumonoDObjs == NULL))
    {
        gNdsPupupuGroundDeferredMask |= 1u << 5;
        return;
    }

    bounds = gMPCollisionBounds.stop;
    /* An empty start box keeps the per-vertex else-if chain; see
     * ndsMPBoundsIncludeYakumonoExtent. */
    extent = ((bounds.top >= bounds.bottom) &&
              (bounds.right >= bounds.left)) ? TRUE : FALSE;
    line_info = gMPCollisionGeometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(gMPCollisionGeometry);
    if (yakumono_count > NDS_MP_YAKUMONO_DOBJ_SLOTS)
    {
        yakumono_count = NDS_MP_YAKUMONO_DOBJ_SLOTS;
    }

    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(line_info, i);
        u32 yakumono_id = ndsMPLineInfoYakumonoID(info);
        DObj *yakumono_dobj;

        if (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS)
        {
            continue;
        }
        yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
        if ((yakumono_dobj == NULL) ||
            ((yakumono_dobj->anim_joint.event32 == NULL) &&
             ((yakumono_dobj->user_data.s >= nMPYakumonoStatusOff) ||
              (yakumono_dobj->user_data.s == nMPYakumonoStatusNone))))
        {
            continue;
        }
        if (extent != FALSE)
        {
            ndsMPBoundsIncludeYakumonoExtent(info, i, yakumono_dobj, &bounds);
        }
        else
        {
            ndsMPBoundsIncludeYakumonoLines(info, yakumono_dobj, &bounds, TRUE,
                                            TRUE);
        }
    }
    gMPCollisionBounds.current = bounds;
}

void mpCollisionUpdateBoundsDiff(void)
{
    gMPCollisionBounds.diff.top =
        gMPCollisionBounds.current.top - gMPCollisionBounds.start.top;
    gMPCollisionBounds.diff.bottom =
        gMPCollisionBounds.current.bottom - gMPCollisionBounds.start.bottom;
    gMPCollisionBounds.diff.right =
        gMPCollisionBounds.current.right - gMPCollisionBounds.start.right;
    gMPCollisionBounds.diff.left =
        gMPCollisionBounds.current.left - gMPCollisionBounds.start.left;
}

void mpCollisionAdvanceUpdateTic(GObj *ground_gobj)
{
    (void)ground_gobj;
    NDS_DIAG(gNdsPupupuGroundDeferredMask |= 1u << 5);
}

extern void gcParseDObjAnimJoint(DObj *dobj);
extern void gcPlayDObjAnimJoint(DObj *dobj);
extern void gcParseMObjMatAnimJoint(MObj *mobj);
extern void gcPlayMObjMatAnim(MObj *mobj);
extern u16 gMPCollisionUpdateTic;

/* P2-6 (2026-10-03): mpCollisionPlayYakumonoAnim asked
 * ndsMPFindYakumonoDObjIndex for every DObj of the ground tree, and most of
 * them (Board the Platforms' platform parts) are no yakumono, so each scanned
 * all 64 slots: 15K ticks a tick on Purin's board. One open-addressed table
 * of slot + 1 per call answers the same question, first slot first. */
#define NDS_MP_YAKUMONO_INDEX_SLOTS (2u * NDS_MP_YAKUMONO_DOBJ_SLOTS)
_Static_assert(((NDS_MP_YAKUMONO_INDEX_SLOTS &
                 (NDS_MP_YAKUMONO_INDEX_SLOTS - 1u)) == 0u) &&
               (NDS_MP_YAKUMONO_DOBJ_SLOTS < 255u),
               "the yakumono index stores slot + 1 in a byte, power-of-two table");

static inline u32 ndsMPYakumonoIndexHome(const DObj *dobj)
{
    return ((u32)(uintptr_t)dobj * 2654435761u) >> 25;
}

static void ndsMPYakumonoIndexBuild(u8 *table)
{
    s32 count = gMPCollisionYakumonosNum;
    s32 i;

    memset(table, 0, NDS_MP_YAKUMONO_INDEX_SLOTS);
    if (count > NDS_MP_YAKUMONO_DOBJ_SLOTS)
    {
        count = NDS_MP_YAKUMONO_DOBJ_SLOTS;
    }
    for (i = 0; i < count; i++)
    {
        const DObj *d = gMPCollisionYakumonoDObjs->dobjs[i];
        u32 slot;

        if (d == NULL)
        {
            continue;
        }
        slot = ndsMPYakumonoIndexHome(d) & (NDS_MP_YAKUMONO_INDEX_SLOTS - 1u);
        while ((table[slot] != 0u) &&
               (gMPCollisionYakumonoDObjs->dobjs[table[slot] - 1u] != d))
        {
            slot = (slot + 1u) & (NDS_MP_YAKUMONO_INDEX_SLOTS - 1u);
        }
        if (table[slot] == 0u)
        {
            table[slot] = (u8)(i + 1);
        }
    }
}

static s32 ndsMPYakumonoIndexFind(const u8 *table, const DObj *dobj)
{
    u32 slot;

    if (dobj == NULL)
    {
        return -1;
    }
    slot = ndsMPYakumonoIndexHome(dobj) & (NDS_MP_YAKUMONO_INDEX_SLOTS - 1u);
    while (table[slot] != 0u)
    {
        if (gMPCollisionYakumonoDObjs->dobjs[table[slot] - 1u] == dobj)
        {
            return (s32)table[slot] - 1;
        }
        slot = (slot + 1u) & (NDS_MP_YAKUMONO_INDEX_SLOTS - 1u);
    }
    return -1;
}

static __attribute__((unused)) s32 ndsMPFindYakumonoDObjIndex(DObj *dobj)
{
    s32 count;
    s32 i;

    if ((dobj == NULL) || (gMPCollisionYakumonoDObjs == NULL))
    {
        return -1;
    }
    count = gMPCollisionYakumonosNum;
    if (count > NDS_MP_YAKUMONO_DOBJ_SLOTS)
    {
        count = NDS_MP_YAKUMONO_DOBJ_SLOTS;
    }
    for (i = 0; i < count; i++)
    {
        if (gMPCollisionYakumonoDObjs->dobjs[i] == dobj)
        {
            return i;
        }
    }
    return -1;
}

s32 mpCollisionSetDObjNoID(s32 line_id)
{
    u32 yakumono_id = 0u;

    if ((line_id < 0) || (ndsMPFindLineYakumonoID(line_id, &yakumono_id) ==
            FALSE))
    {
        return -1;
    }
    return (s32)yakumono_id;
}

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB ONLY: [0] total, [1] yakumono index search, [2] DObj joint anims,
 * [3] MObj material anims, [4] bounds update, [5] DObjs walked, [6] MObjs,
 * [7] yakumono count (last), [8] DObjs with a joint script, [9] calls. */
extern u32 cpuGetTiming(void);
__attribute__((used)) volatile u32 gNdsLabYakuAcc[10];
#define NDS_LAB_YAKU_MARK(v) ((v) = cpuGetTiming())
#define NDS_LAB_YAKU_ADD(i, v) (gNdsLabYakuAcc[(i)] += cpuGetTiming() - (v))
#define NDS_LAB_YAKU_INC(i, n) (gNdsLabYakuAcc[(i)] += (n))
#else
#define NDS_LAB_YAKU_MARK(v) ((void)0)
#define NDS_LAB_YAKU_ADD(i, v) ((void)0)
#define NDS_LAB_YAKU_INC(i, n) ((void)0)
#endif

void mpCollisionPlayYakumonoAnim(GObj *ground_gobj)
{
    DObj *dobj;
    u8 yakumono_index[NDS_MP_YAKUMONO_INDEX_SLOTS];
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    u32 lab_total = cpuGetTiming();
    u32 lab_mark = 0u;
#endif

    /* THE PROOF PREDICATE WAS DISABLING THE WHOLE FUNCTION.
     *
     * NDS_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP_HARNESS is defined
     * nowhere -- one `#if` in reloc_backend_fighter_model.c:1060 and no
     * Makefile row, no -D, no config header -- so the predicate is FALSE in
     * every configuration ever built. At its four other call sites that only
     * costs a counter: the work around them (`*vel = gMPCollisionSpeeds[..]`,
     * mpCollisionUpdateBoundsCurrent) runs regardless. Here it gated a
     * `return`, so this function has never run outside a harness that does
     * not exist.
     *
     * What it does is not diagnostic. It walks the ground's DObj tree,
     * parses and plays every joint animation and material animation, and
     * derives each yakumono platform's speed from its translation delta --
     * which is what makes a moving platform move and carry a fighter. Dream
     * Land never showed the loss because its animation comes from its own
     * native stage path; every opt-in stage relies on this one, which is why
     * the owner's 2026-09-04 playtest reports Congo Jungle's platforms
     * standing still (docs/BUGS.md).
     *
     * The null guards below are real and stay, and so does the deferral
     * witness for them. Only the predicate leaves the condition. */
    if ((ground_gobj == NULL) || (gMPCollisionYakumonoDObjs == NULL) ||
        (gMPCollisionSpeeds == NULL))
    {
        gNdsPupupuGroundDeferredMask |= 1u << 5;
        return;
    }

    dobj = DObjGetStruct(ground_gobj);
    ndsMPYakumonoIndexBuild(yakumono_index);
    NDS_LAB_YAKU_INC(9, 1u);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    gNdsLabYakuAcc[7] = (u32)gMPCollisionYakumonosNum;
#endif
    while (dobj != NULL)
    {
        MObj *mobj;
        s32 yakumono_id;

        NDS_LAB_YAKU_MARK(lab_mark);
        yakumono_id = ndsMPYakumonoIndexFind(yakumono_index, dobj);
        NDS_LAB_YAKU_ADD(1, lab_mark);
        NDS_LAB_YAKU_INC(5, 1u);
        NDS_LAB_YAKU_INC(8, (dobj->anim_joint.event32 != NULL) ? 1u : 0u);
        NDS_LAB_YAKU_MARK(lab_mark);
        if (yakumono_id >= 0)
        {
            if ((dobj->user_data.s != nMPYakumonoStatusOn) &&
                (dobj->user_data.s != nMPYakumonoStatusOff))
            {
                u8 flags = dobj->flags;
                Vec3f translate;

                gcParseDObjAnimJoint(dobj);
                translate = dobj->translate.vec.f;
                gcPlayDObjAnimJoint(dobj);

                gMPCollisionSpeeds[yakumono_id].x =
                    dobj->translate.vec.f.x - translate.x;
                gMPCollisionSpeeds[yakumono_id].y =
                    dobj->translate.vec.f.y - translate.y;
                gMPCollisionSpeeds[yakumono_id].z =
                    dobj->translate.vec.f.z - translate.z;

                if (flags == DOBJ_FLAG_NONE)
                {
                    if (dobj->flags != DOBJ_FLAG_NONE)
                    {
                        dobj->user_data.s = nMPYakumonoStatusHidden;
                    }
                }
                else if (dobj->flags == DOBJ_FLAG_NONE)
                {
                    dobj->user_data.s = nMPYakumonoStatusShow;
                }

                if ((u32)yakumono_id ==
                    gNdsStageMPPlatformSpeedFloorLoopYakumonoID)
                {
                    gNdsStageMPPlatformSpeedFloorLoopAnimPlayCount++;
                    gNdsStageMPPlatformSpeedFloorLoopAnimSpeedXMilli =
                        ndsFloatToMilliSigned(
                            gMPCollisionSpeeds[yakumono_id].x);
                    gNdsStageMPPlatformSpeedFloorLoopAnimSpeedYMilli =
                        ndsFloatToMilliSigned(
                            gMPCollisionSpeeds[yakumono_id].y);
                    gNdsStageMPPlatformSpeedFloorLoopAnimSpeedZMilli =
                        ndsFloatToMilliSigned(
                            gMPCollisionSpeeds[yakumono_id].z);
                    gNdsStageMPPlatformSpeedFloorLoopAnimStatusAfter =
                        (u32)dobj->user_data.s;
                }
            }
        }
        else
        {
            gcParseDObjAnimJoint(dobj);
            gcPlayDObjAnimJoint(dobj);
        }
        NDS_LAB_YAKU_ADD(2, lab_mark);
        NDS_LAB_YAKU_MARK(lab_mark);

        mobj = dobj->mobj;
        while (mobj != NULL)
        {
            gcParseMObjMatAnimJoint(mobj);
            gcPlayMObjMatAnim(mobj);
            mobj = mobj->next;
            NDS_LAB_YAKU_INC(6, 1u);
        }
        NDS_LAB_YAKU_ADD(3, lab_mark);

        if (dobj->child != NULL)
        {
            dobj = dobj->child;
        }
        else if (dobj->sib_next != NULL)
        {
            dobj = dobj->sib_next;
        }
        else
        {
            while (TRUE)
            {
                if (dobj->parent == DOBJ_PARENT_NULL)
                {
                    dobj = NULL;
                    break;
                }
                if (dobj->parent->sib_next != NULL)
                {
                    dobj = dobj->parent->sib_next;
                    break;
                }
                dobj = dobj->parent;
            }
        }
    }

    NDS_LAB_YAKU_MARK(lab_mark);
    mpCollisionUpdateBoundsCurrent();
    mpCollisionUpdateBoundsDiff();
    NDS_LAB_YAKU_ADD(4, lab_mark);
    gMPCollisionUpdateTic++;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    NDS_DIAG(gNdsLabYakuAcc[0] += cpuGetTiming() - lab_total);
#endif
}

void mpCollisionSetYakumonoPosID(s32 line_id, Vec3f *yakumono_pos)
{
    DObj *yakumono_dobj;

    if ((line_id < 0) || (line_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS) ||
        (yakumono_pos == NULL) || (gMPCollisionYakumonoDObjs == NULL) ||
        (gMPCollisionSpeeds == NULL) ||
        (gMPCollisionYakumonoDObjs->dobjs[line_id] == NULL))
    {
        return;
    }

    yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[line_id];
    gMPCollisionSpeeds[line_id].x =
        yakumono_pos->x - yakumono_dobj->translate.vec.f.x;
    gMPCollisionSpeeds[line_id].y =
        yakumono_pos->y - yakumono_dobj->translate.vec.f.y;
    gMPCollisionSpeeds[line_id].z =
        yakumono_pos->z - yakumono_dobj->translate.vec.f.z;
    yakumono_dobj->translate.vec.f.x = yakumono_pos->x;
    yakumono_dobj->translate.vec.f.y = yakumono_pos->y;
    yakumono_dobj->translate.vec.f.z = yakumono_pos->z;

}

void mpCollisionSetYakumonoOnID(s32 line_id)
{
    if ((line_id < 0) || (line_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS) ||
        (gMPCollisionYakumonoDObjs == NULL) ||
        (gMPCollisionYakumonoDObjs->dobjs[line_id] == NULL))
    {
        return;
    }
    gMPCollisionYakumonoDObjs->dobjs[line_id]->user_data.s =
        nMPYakumonoStatusOn;
}

void mpCollisionSetYakumonoOffID(s32 line_id)
{
    if ((line_id < 0) || (line_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS) ||
        (gMPCollisionYakumonoDObjs == NULL) ||
        (gMPCollisionYakumonoDObjs->dobjs[line_id] == NULL))
    {
        return;
    }
    gMPCollisionYakumonoDObjs->dobjs[line_id]->user_data.s =
        nMPYakumonoStatusOff;
}

/* P2-5: with the item core compiled in, itManagerMakeAppearActor is the real
 * spawn-law actor in battleship_item_link_core.c and returns the actor GObj,
 * as the source does. Without it, grCommonSetupInitAll still calls the name
 * (grcommonsetup.c:33), so the witness stub stays for exactly that case. */

/* efGroundMakeAppearActor is the real one since 2026-09-05:
 * src/import/battleship_efground.c carries ef/efground.c whole. The witness
 * stub that used to sit here set gNdsPupupuGroundDeferredMask bit 7. */

s32 mpCollisionGetMapObjCountKind(s32 kind)
{
    u32 i;
    u32 count = 0u;
    u32 mapobj_count;

#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
    if (sNdsStageInishieScaleSourceSetupActive != FALSE)
    {
        return ((kind == nMPMapObjKindScaleL) ||
                (kind == nMPMapObjKindScaleR)) ? 1 : 0;
    }
#endif

    mapobj_count = ndsMPGeometryMapObjCount(gMPCollisionGeometry);
    for (i = 0; i < mapobj_count; i++)
    {
        u16 mapobj_kind;

        if ((ndsMPReadMapObj((s32)i, &mapobj_kind, NULL, NULL) != FALSE) &&
            ((s32)mapobj_kind == kind))
        {
            count++;
        }
    }
    return (s32)count;
}

void mpCollisionGetMapObjIDsKind(s32 kind, s32 *ids)
{
    u32 i;
    u32 count = 0u;
    u32 mapobj_count;

    if (ids != NULL)
    {
#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
        if (sNdsStageInishieScaleSourceSetupActive != FALSE)
        {
            if (kind == nMPMapObjKindScaleL)
            {
                ids[0] = 0;
            }
            else if (kind == nMPMapObjKindScaleR)
            {
                ids[0] = 1;
            }
            else
            {
                ids[0] = -1;
            }
            return;
        }
#endif

        mapobj_count = ndsMPGeometryMapObjCount(gMPCollisionGeometry);
        for (i = 0; i < mapobj_count; i++)
        {
            u16 mapobj_kind;

            if ((ndsMPReadMapObj((s32)i, &mapobj_kind, NULL, NULL) != FALSE) &&
                ((s32)mapobj_kind == kind))
            {
                ids[count] = (s32)i;
                count++;
            }
        }
    }
}

void mpCollisionGetMapObjPositionID(s32 id, Vec3f *pos)
{
    if (pos != NULL)
    {
#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
        if (sNdsStageInishieScaleSourceSetupActive != FALSE)
        {
            if (id == 0)
            {
                pos->x = -417.0F;
                pos->y = 363.0F;
                pos->z = 0.0F;
                return;
            }
            if (id == 1)
            {
                pos->x = 420.0F;
                pos->y = 362.0F;
                pos->z = 0.0F;
                return;
            }
        }
#endif

        {
            s16 x;
            s16 y;

            if (ndsMPReadMapObj(id, NULL, &x, &y) != FALSE)
            {
                pos->x = (f32)x;
                pos->y = (f32)y;
                pos->z = 0.0F;
                return;
            }
        }

        pos->x = 0.0F;
        pos->y = 0.0F;
        pos->z = 0.0F;
    }
}

void mpCollisionGetPlayerMapObjPosition(s32 player, Vec3f *pos)
{
    if (pos != NULL)
    {
        u32 i;
        u32 mapobj_count = ndsMPGeometryMapObjCount(gMPCollisionGeometry);

        /* BattleShip mpcollision.c:3405. The source zeros the output only when
         * the stage has NO map objects. If a malformed/non-VS stage has map
         * objects but no spawn kind matching this player, it leaves the caller's
         * existing FTDesc position untouched. Preserve that distinction instead
         * of silently moving an unmatched P3/P4 spawn to the world origin. */
        if (mapobj_count == 0u)
        {
            pos->x = 0.0F;
            pos->y = 0.0F;
            pos->z = 0.0F;
        }
        for (i = 0; i < mapobj_count; i++)
        {
            u16 kind;
            s16 x;
            s16 y;

            if ((ndsMPReadMapObj((s32)i, &kind, &x, &y) != FALSE) &&
                ((s32)kind == player))
            {
                pos->x = (f32)x;
                pos->y = (f32)y;
                pos->z = 0.0F;
                break;
            }
        }
    }
    gNdsSCVSBattleCompatSpawnMask |= 1u << (player & 3);
}

sb32 mpCollisionCheckProjectFloor(Vec3f *pos, s32 *floor_line_id,
                                  f32 *floor_dist, u32 *floor_flags,
                                  Vec3f *floor_angle)
{
    sb32 is_floor = FALSE;


    if (
        /* ANY stage whose geometry loaded, which is what the call below
         * already checks. This was a gkind whitelist -- Dream Land, later Dream
         * Land or Yoshi's Island -- and it is the second half of the same hole
         * as the line-group gate: a stage off the list got no floor query even
         * once it had line groups, so its fighters found nothing under them.
         * The y=0 flat fabrication further down stays Dream Land-only on
         * purpose; it is a fallback for one layout and no other stage may
         * inherit it. */
        (ndsStageCollisionLoopGeometryReady() != FALSE))
    {
        Vec3f angle = { 0.0F, 1.0F, 0.0F };
        s32 line_id = -1;
        f32 dist = 0.0F;
        u32 flags = 0u;

        is_floor = ndsMPProjectFloorGeometry(pos, &line_id, &dist,
                                             &flags, &angle);
        if (floor_line_id != NULL)
        {
            *floor_line_id = (is_floor != FALSE) ? line_id : -1;
        }
        if (floor_dist != NULL)
        {
            *floor_dist = (is_floor != FALSE) ? dist : 0.0F;
        }
        if (floor_flags != NULL)
        {
            *floor_flags = (is_floor != FALSE) ? flags : 0u;
        }
        if (floor_angle != NULL)
        {
            *floor_angle = angle;
        }
        return is_floor;
    }

    if ((gSCManagerSceneData.gkind == nGRKindPupupu) &&
        (pos != NULL) &&
        (pos->y > -1.0F) &&
        (pos->y < 1.0F))
    {
        is_floor = TRUE;
    }

    if (floor_line_id != NULL)
    {
        *floor_line_id = (is_floor != FALSE) ? 0 : -1;
    }
    if (floor_dist != NULL)
    {
        *floor_dist = 0.0F;
    }
    if (floor_flags != NULL)
    {
        *floor_flags = 0u;
    }
    if (floor_angle != NULL)
    {
        floor_angle->x = 0.0F;
        floor_angle->y = 1.0F;
        floor_angle->z = 0.0F;
    }
    return is_floor;
}

/* decomp mpcollision.c:2857-2957. RWall twin of mpCollisionCheckProjectFloor
 * above: ftCommonDokanWaitSetStatus (ftcommondokan.c:222) projects the
 * pipe-exit target out of the DokanWall with it. Each RWall line is queried
 * through the public per-line helper (include/mp/map.h:165), which carries
 * the source's own pieces: yakumono-relative query
 * (ndsMPLinePositionWorldToLocal, cf. source :2882-2889), y-overlap gate
 * (cf. source :2902) and x-at-y interpolation (cf. source :2920). The outer
 * scan keeps the source's selection: wall at or left of the point
 * (fpos <= vpdist_x, :2924), nearest wins (:2926). */
sb32 mpCollisionGetLRCommonRWall(s32 line_id, Vec3f *object_pos,
                                 f32 *dist, u32 *flags, Vec3f *angle);

sb32 mpCollisionCheckProjectRWall(Vec3f *position, s32 *project_line_id,
                                  f32 *ga_dist, u32 *stand_coll_flags,
                                  Vec3f *angle)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    f32 line_project_pos = F32_MAX;
    s32 best_line = -1;
    f32 best_dist = 0.0F;
    u32 best_flags = 0u;
    Vec3f best_angle = { 1.0F, 0.0F, 0.0F };
    u32 yakumono_count;
    u32 i;

    if ((position == NULL) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(line_info, i);
        s32 line_first =
            (s32)ndsMPLineInfoGroupID(info, nMPLineKindRWall);
        s32 line_count =
            (s32)ndsMPLineInfoLineCount(info, nMPLineKindRWall);
        s32 line_id;
        s32 line_end;

        line_end = line_first + line_count;
        if ((line_end - line_first) > 4096)
        {
            line_end = line_first + 4096;
        }
        for (line_id = line_first; line_id < line_end; line_id++)
        {
            f32 dist = 0.0F;
            u32 flags = 0u;
            Vec3f wall_angle = { 1.0F, 0.0F, 0.0F };

            if (mpCollisionGetLRCommonRWall(line_id, position, &dist,
                                            &flags, &wall_angle) == FALSE)
            {
                continue;
            }
            if ((dist <= 0.0F) && (ABSF(dist) < line_project_pos))
            {
                best_line = line_id;
                best_dist = dist;
                best_flags = flags;
                best_angle = wall_angle;
                line_project_pos = ABSF(dist);
            }
        }
    }
    if (best_line < 0)
    {
        return FALSE;
    }
    if (project_line_id != NULL)
    {
        *project_line_id = best_line;
    }
    if (ga_dist != NULL)
    {
        *ga_dist = best_dist;
    }
    if (stand_coll_flags != NULL)
    {
        *stand_coll_flags = best_flags;
    }
    if (angle != NULL)
    {
        *angle = best_angle;
    }
    return TRUE;
}

void mpCollisionSetPlayBGM(void)
{
    if (gMPCollisionGroundData != NULL)
    {
        gMPCollisionBGMDefault = gMPCollisionGroundData->bgm_id;
        gMPCollisionBGMCurrent = gMPCollisionGroundData->bgm_id;
        gNdsSCVSBattleStageBGM = gMPCollisionGroundData->bgm_id;
        gNdsStagePupupuBGM = gMPCollisionGroundData->bgm_id;
        syAudioPlayBGM(0, gMPCollisionBGMDefault);
    }
    gNdsSCVSBattleCompatAudioMask |= 1u << 4;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_AUDIO;
}

void gmRumbleMakeActor(void)
{
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_RUMBLE;
}

void gmRumbleInitPlayers(void)
{
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_RUMBLE;
}

__attribute__((weak)) void itManagerInitItems(void)
{
    gNdsSCVSBattleCompatManagerMask |= 1u << 8;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_ITEM_WEAPON_MANAGER;
}

void syDebugSetFuncPrint(void (*function)(void))
{
    (void)function;
}

void syDebugStartRmonThread5Hang(void)
{
}

void scManagerFuncPrint(void)
{
}

void syRdpSetViewport(Vp *viewport, f32 ulx, f32 uly, f32 lrx, f32 lry)
{
    f32 h;
    f32 v;

    if (viewport == NULL)
    {
        return;
    }

    h = (ulx + lrx) / 2.0F;
    v = (uly + lry) / 2.0F;

    viewport->vp.vscale[0] = (s16)(((s32)((lrx - h) * 4.0F)) & 0xFFFF);
    viewport->vp.vscale[1] = (s16)(((s32)((lry - v) * 4.0F)) & 0xFFFF);
    viewport->vp.vtrans[0] = (s16)(((s32)(h * 4.0F)) & 0xFFFF);
    viewport->vp.vtrans[1] = (s16)(((s32)(v * 4.0F)) & 0xFFFF);
    viewport->vp.vscale[2] = (s16)(0x03FF / 2);
    viewport->vp.vtrans[2] = (s16)(0x03FF / 2);
}

void (*dSYRdpFuncLights)(Gfx **);

void syRdpSetFuncLights(void (*func_lights)(Gfx **))
{
    dSYRdpFuncLights = func_lights;
    ndsFighterDisplayContractResetSceneLight();
}

void syRdpResetSettings(Gfx **dl)
{
    /* BattleShip sys/rdp.c:112-115 applies the scene light callback before
     * scene drawing. The DS backend consumes its GBI light state directly. */
    if ((dl != NULL) && (dSYRdpFuncLights != NULL))
    {
        dSYRdpFuncLights(dl);
    }
}

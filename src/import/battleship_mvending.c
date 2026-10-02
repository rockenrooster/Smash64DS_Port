/* P2-6 step 8 tail. 1P ending movie, source import: textual include of
 * decomp/BattleShip-main/decomp/src/mv/mvending/mvending.c whole (560 lines:
 * dMVEndingFileIDs :87, lights, room/fighter/camera builders, mvEndingFuncRun
 * :468, mvEndingFuncStart :508, mvEndingStartScene :551), following
 * src/import/battleship_sc1pbonusstage.c / battleship_mnoption.c (scene TU
 * with the scene entry imported as ndsBase* and re-exported under its source
 * name, so a later measured DS arena rebudget has a seam and the diff stays
 * reviewable). The adapter is a verbatim pass-through; no behaviour invented
 * here.
 *
 * Unified-owner rule (stated in battleship_sc1pgame_runtime.c, followed
 * here): the include OWNS every symbol it defines under its source name --
 * no renamed private copies. The only rename is the scene entry, imported as
 * ndsBase* and re-exported under its source name. Gated on NDS_P2_1P_GAME
 * like the rest of the P2-6 step 8 tail.
 *
 * Reloc files (dMVEndingFileIDs mvending.c:87): MVCommon + MVEnding 0x4c.
 * Both staged 2026-09-04 by scripts/menus/stage_reloc_file.py; manifests in
 * include/reloc_data.h (NDS_MV_ENDING_RELOC_SYMBOLS: one camera AnimJoint
 * llMVEndingOperatorCamAnimJoint); definitions in
 * src/port/diagnostics_mp_taskman_state.c (llMVCommonFileID :64,
 * llMVEndingFileID = 0x4c :444). No file this TU loads directly is unstaged.
 * Per-fighter payload is indirect: mvEndingFuncStart :530 calls
 * ftManagerSetupFilesAllKind(fkind), which pulls that fighter's FTData
 * closure from decomp ft/ftdata.c (dFTMarioData :348 pattern): Main,
 * MainMotion, Model, ShieldPose, Special1/2/3 (+Special4 where the kind has
 * one), plus that fighter's ~100 llFT<Name>Anim*FileID motion rows. Those
 * model/animation files are NOT sprite records, so the sprite tool cannot
 * normalize them; the orchestrator stages them per fighter with
 * python scripts/menus/stage_reloc_file.py --file NAME --list
 * NDS_1P_RELOC_FILES (12 playable closures: Mario, Fox, Donkey, Samus,
 * Luigi, Link, Yoshi, Captain, Kirby, Pikachu, Purin, Ness).
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - No local shims. BGM/FGM the TU needs already exist in port
 *   include/gm/gmsound.h (nSYAudioBGMEnding = 38, nSYAudioFGMDoorClose = 20);
 *   nSCKindStaffroll exists in include/sc/scene.h; ovl54_BSS_END + ovl1_VRAM
 *   are covered by DECLARE_OVL in include/sc/scene.h; func_80017EC0 is in
 *   include/sys/objhelper.h:92; nFTDemoStatusFigureDropped is in
 *   include/ft/fighter.h:176.
 * - Left unresolved at link (never stubbed): func_800269C0_275C0
 *   (called :495; port include/sys/audio.h:92 declares it per the
 *   stage-clear precedent, definition lives in the audio seam), and the
 *   per-fighter model/animation file definitions above until the
 *   orchestrator stages them. Everything else the TU calls is
 *   port-provided: gc, lbReloc, sy, ef, ftManager, scSubsys, syAudioPlayBGM.
 * - Collisions needing reported gating (not renamed away, behaviour must
 *   win): mvEndingStartScene (adapter below) vs
 *   src/port/title_backend.c:463 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <mn/menu.h>
#include <mv/movie.h>
#include <nds/nds_obj_anim.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/controller.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>

#define mvEndingStartScene ndsBaseMVEndingStartScene
void ndsBaseMVEndingStartScene(void);

/* Exact source headers: decomp mv/mvending/mvending.h:8,27 (setup :99-140
 * references both before :150, :508); decomp lb/lbcommon.h:11 (matrix list
 * at :129), same extern form as battleship_mnplayersvs.c:38. */
extern void mvEndingFuncLights(Gfx **dls);
extern void mvEndingFuncStart(void);
extern sb32 (*dLBCommonFuncMatrixList[])(void);

/* Landed precedent externs (battleship_mvopeningmario.c:21,34); called at
 * :526, :528, :495 via sys/audio.h decl. */
extern void efParticleInitAll(void);
extern void efManagerInitEffects(void);

#include <nds/nds_native_baked_types.h>
#include <nds/nds_video.h>

/* The room's fade-in (black, mvending.c:261-288) and closing light (white,
 * :327-347) are full-viewport PRIM rectangles drawn over the 3D, which the
 * native renderer never sees. They are the scene's only gDPSetPrimColor and
 * gDPFillRectangle, so both are taken here: the command is still written
 * where the source writes it, and each rectangle's alpha becomes the matching
 * MASTER_BRIGHT fade (black down, white up; ndsVideoSetSceneFade). */
static u32 sNdsMVEndingPrimRGBA;
static u32 sNdsMVEndingFadeDown;
static u32 sNdsMVEndingFadeUp;

static void ndsMVEndingWritePrimColor(Gfx *pkt, u32 m, u32 l, u32 r, u32 g,
                                      u32 b, u32 a)
{
    gDPSetPrimColor(pkt, m, l, r, g, b, a);
}

static void ndsMVEndingWriteFillRect(Gfx *pkt, s32 ulx, s32 uly, s32 lrx,
                                     s32 lry)
{
    gDPFillRectangle(pkt, ulx, uly, lrx, lry);
}

static void ndsMVEndingPrimColor(Gfx *pkt, u32 m, u32 l, u32 r, u32 g, u32 b,
                                 u32 a)
{
    ndsMVEndingWritePrimColor(pkt, m, l, r, g, b, a);
    sNdsMVEndingPrimRGBA = ((r & 0xffu) << 24) | ((g & 0xffu) << 16) |
                           ((b & 0xffu) << 8) | (a & 0xffu);
}

static void ndsMVEndingFillRect(Gfx *pkt, s32 ulx, s32 uly, s32 lrx, s32 lry)
{
    u32 level = (((sNdsMVEndingPrimRGBA & 0xffu) * 16u) + 127u) / 255u;

    ndsMVEndingWriteFillRect(pkt, ulx, uly, lrx, lry);
    if ((sNdsMVEndingPrimRGBA >> 8) == 0u)
    {
        sNdsMVEndingFadeDown = level;
    }
    else
    {
        sNdsMVEndingFadeUp = level;
    }
    ndsVideoSetSceneFade(sNdsMVEndingFadeDown, sNdsMVEndingFadeUp);
}

#define gDPSetPrimColor(pkt, m, l, r, g, b, a)     ndsMVEndingPrimColor((pkt), (m), (l), (r), (g), (b), (a))
#define gDPFillRectangle(pkt, ulx, uly, lrx, lry)     ndsMVEndingFillRect((pkt), (ulx), (uly), (lrx), (lry))

#include "../../decomp/BattleShip-main/decomp/src/mv/mvending/mvending.c"

#undef gDPSetPrimColor
#undef gDPFillRectangle
#undef mvEndingStartScene

#include <nds/nds_startup.h>
#include <nds/nds_platform.h>
#include <nds/nds_reloc_assets.h>

/* The room's baked roots (nitro:/movies/room_baked.bin, written by
 * scripts/stages/generate_nds_native_item_baked.py) live in this scene's own
 * heap for its run: read once at the start, the groups' data offsets rebased
 * to pointers, and dropped when the scene ends. No static image carries
 * them. A missing or mismatched file leaves the table NULL and the room
 * undrawn. */
#define NDS_MV_ENDING_ROOM_MAX_BYTES 0x8000u
#define NDS_MV_ENDING_ROOM_HEAP_MARGIN 0x4000u

static u32 ndsMVEndingRoomHash(const u8 *bytes, u32 count)
{
    u32 hash = 2166136261u;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        hash = (hash ^ bytes[i]) * 16777619u;
    }
    return hash;
}

static void ndsMVEndingDropRoomTable(void)
{
    gNdsNativeBakedRoomRoots = NULL;
    gNdsNativeBakedRoomOps = NULL;
    gNdsNativeBakedRoomGroups = NULL;
    gNdsNativeBakedRoomRootCount = 0u;
}

static void ndsMVEndingLoadRoomTable(void)
{
    NdsRelocAssetStream stream = { NULL };
    u32 header[8];
    const NDSNativeBakedRoot *roots;
    const NDSNativeBakedOp *ops;
    NDSNativeBakedGroup *groups;
    const u8 *data;
    u32 root_count;
    u32 op_count;
    u32 group_count;
    u32 data_bytes;
    u32 body_bytes;
    u32 table_bytes;
    u32 free_bytes;
    u32 i;
    u8 *body;

    ndsMVEndingDropRoomTable();
    if ((ndsRelocAssetStreamOpen(&stream, "nitro:/movies/room_baked.bin") ==
         FALSE) ||
        (ndsRelocAssetStreamRead(&stream, 0u, header, sizeof(header)) ==
         FALSE))
    {
        goto done;
    }
    root_count = header[2];
    op_count = header[3];
    group_count = header[4];
    data_bytes = header[5];
    body_bytes = header[7];
    table_bytes = (root_count * (u32)sizeof(NDSNativeBakedRoot)) +
                  (op_count * (u32)sizeof(NDSNativeBakedOp)) +
                  (group_count * (u32)sizeof(NDSNativeBakedGroup));
    free_bytes = (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
                       (uintptr_t)gSYTaskmanGeneralHeap.ptr);
    if ((header[0] != NDS_NATIVE_BAKED_ROOM_MAGIC) ||
        (header[1] != NDS_NATIVE_BAKED_ROOM_VERSION) || (root_count == 0u) ||
        (root_count > 0x100u) || (op_count > 0x1000u) ||
        (group_count > 0x400u) || (body_bytes > NDS_MV_ENDING_ROOM_MAX_BYTES) ||
        (table_bytes + data_bytes != body_bytes) ||
        (free_bytes < body_bytes + NDS_MV_ENDING_ROOM_HEAP_MARGIN))
    {
        goto done;
    }
    body = syTaskmanMalloc(body_bytes, 0x4u);
    if ((body == NULL) ||
        (ndsRelocAssetStreamRead(&stream, (u32)sizeof(header), body,
                                 body_bytes) == FALSE) ||
        (ndsMVEndingRoomHash(body, body_bytes) != header[6]))
    {
        goto done;
    }
    roots = (const NDSNativeBakedRoot *)(void *)body;
    ops = (const NDSNativeBakedOp *)(void *)(roots + root_count);
    groups = (NDSNativeBakedGroup *)(void *)(ops + op_count);
    data = body + table_bytes;
    for (i = 0u; i < group_count; i++)
    {
        u32 verts = (u32)(uintptr_t)groups[i].verts;
        u32 colors = (u32)(uintptr_t)groups[i].colors;
        u32 indices = (u32)(uintptr_t)groups[i].indices;
        u32 vertex_count = groups[i].vertex_count;
        u32 corner_count = (u32)groups[i].triangle_count * 3u;

        if ((verts > data_bytes) || (colors > data_bytes) ||
            (indices > data_bytes) ||
            ((data_bytes - verts) < vertex_count * 10u) ||
            ((data_bytes - colors) < vertex_count * 4u) ||
            ((data_bytes - indices) < corner_count * 2u) || ((colors & 3u) != 0u) ||
            ((verts & 1u) != 0u) || ((indices & 1u) != 0u))
        {
            goto done;
        }
        groups[i].verts = (const s16 *)(const void *)(data + verts);
        groups[i].colors = (const u32 *)(const void *)(data + colors);
        groups[i].indices = (const u16 *)(const void *)(data + indices);
    }
    for (i = 0u; i < root_count; i++)
    {
        if (((u32)roots[i].first_op + roots[i].op_count) > op_count)
        {
            goto done;
        }
    }
    for (i = 0u; i < op_count; i++)
    {
        if ((ops[i].op == NDS_NATIVE_BAKED_OP_EMIT) &&
            (ops[i].arg >= group_count))
        {
            goto done;
        }
    }
    gNdsNativeBakedRoomOps = ops;
    gNdsNativeBakedRoomGroups = groups;
    gNdsNativeBakedRoomRootCount = root_count;
    gNdsNativeBakedRoomRoots = roots;
done:
    ndsRelocAssetStreamClose(&stream);
}

/* The room's source display procs (gcDrawDObjTree*ForGObj, gcDrawDObjDLHead0)
 * are recorders on the DS: only a battle's stage loop turns what they record
 * into geometry, so the ending drew nothing but its backdrop. Here each room
 * GObj submits its own tree through the stage DL path with the room camera
 * (mvending.c:410-430), where the baked MVCommon roots
 * (nds_native_room_baked.c) draw every list. Geometry starts as the room
 * camera's RSP reset leaves it: G_ZBUFFER | G_CULL_BACK. */
#define NDS_MV_ENDING_ROOM_GEOMETRY (0x00000001u | 0x00000400u)

static void ndsMVEndingSubmitRoom(GObj *gobj, u32 kind)
{
    DObj *root = DObjGetStruct(gobj);

    if ((root == NULL) || (sMVEndingRoomCameraGObj == NULL))
    {
        return;
    }
    /* The scene's 3D is the cameras' (10,10)-(310,230) window, as the
     * staff roll and the 1P intro present theirs; BG0 shows it. */
    ndsPlatformSet3DLayerEnabled(TRUE);
    ndsPlatformSet3DViewportSource(10, 10, 310, 230);
    ndsRendererAdapterBeginStageTraversal();
    if (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0)
    {
        ndsRendererAdapterSubmitStageDObj(root, kind, sMVEndingRoomCameraGObj,
                                          NDS_MV_ENDING_ROOM_GEOMETRY);
    }
    else
    {
        ndsRendererAdapterSubmitWeaponDObjTree(root, kind,
                                               sMVEndingRoomCameraGObj,
                                               NDS_MV_ENDING_ROOM_GEOMETRY);
    }
    ndsRendererAdapterEndStageTraversal();
}

static void ndsMVEndingDrawRoomTree(GObj *gobj)
{
    ndsMVEndingSubmitRoom(gobj, NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE);
}

static void ndsMVEndingDrawRoomTreeDLLinks(GObj *gobj)
{
    ndsMVEndingSubmitRoom(gobj,
                          NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS);
}

static void ndsMVEndingDrawRoomDLHead0(GObj *gobj)
{
    ndsMVEndingSubmitRoom(gobj, NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0);
}

static void ndsMVEndingSetRoomDisplay(GObj *gobj, void (*proc)(GObj *))
{
    if (gobj != NULL)
    {
        gobj->proc_display = proc;
    }
}

/* mvEndingFuncStart, then the room GObjs it made draw through the DS submit
 * above. Their source procs stay what they were in the source; only the DS
 * display hook changes. */
static void ndsMVEndingFuncStart(void)
{
    mvEndingFuncStart();
    ndsMVEndingLoadRoomTable();
    ndsMVEndingSetRoomDisplay(sMVEndingRoomBackgroundGObj,
                              ndsMVEndingDrawRoomTreeDLLinks);
    ndsMVEndingSetRoomDisplay(sMVEndingRoomDeskGObj, ndsMVEndingDrawRoomTree);
    ndsMVEndingSetRoomDisplay(sMVEndingRoomBooksGObj, ndsMVEndingDrawRoomTree);
    ndsMVEndingSetRoomDisplay(sMVEndingRoomPencilsGObj,
                              ndsMVEndingDrawRoomTree);
    ndsMVEndingSetRoomDisplay(sMVEndingRoomLampGObj, ndsMVEndingDrawRoomTree);
    ndsMVEndingSetRoomDisplay(sMVEndingRoomTissuesGObj,
                              ndsMVEndingDrawRoomDLHead0);
}

void mvEndingStartScene(void)
{
    /* The room draws from the baked roots ndsMVEndingFuncStart loads into
     * this scene's heap; the table goes with the heap. The staff roll that
     * follows clears the scene fade at its first draw. */
    sNdsMVEndingPrimRGBA = 0u;
    sNdsMVEndingFadeDown = 0u;
    sNdsMVEndingFadeUp = 0u;
    dMVEndingTaskmanSetup.func_start = ndsMVEndingFuncStart;
    ndsBaseMVEndingStartScene();
    ndsMVEndingDropRoomTable();
}

#endif /* NDS_P2_1P_GAME */

/* P2-6 step 8 tail. Staff-roll credits, source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sccommon/scstaffroll.c whole
 * (2339 lines: credit tables :16-326, dSCStaffrollFileIDs :329,
 * dSCStaffrollNameAndJobSpriteInfo :332, dSCStaffrollTextBoxSpriteInfo :395,
 * scStaffrollJobProcDisplay :1499, scStaffrollNameProcDisplay :1513,
 * scStaffrollInitNameAndJobDisplayLists :2053, scStaffrollFuncStart :2186,
 * scStaffrollStartScene :2311), following
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
 * Reloc files (dSCStaffrollFileIDs scstaffroll.c:329): SCStaffroll 0xc3
 * only, staged 2026-09-04 by scripts/menus/stage_reloc_file.py; manifest in
 * include/reloc_data.h (NDS_SC_STAFFROLL_RELOC_SYMBOLS: 59 raw Image blocks
 * llSCStaffrollNameAndJob*Image + 77 Sprite records + llSCStaffrollCrosshair
 * / brackets / Interpolation / AnimJoint / DObjDesc); definitions in
 * src/port/diagnostics_mp_taskman_state.c (llSCStaffrollFileID = 0xc3 :449).
 * No file this TU loads is unstaged. llSCStaffrollFileID global confirmed
 * under src/; the 59 Image + 77 Sprite rows are manifest externs in
 * include/reloc_data.h with definitions generated from the same manifest.
 *
 * Names are NOT Sprite records. scStaffrollInitNameAndJobDisplayLists
 * (:2053-2102) builds one DL per glyph with gDPLoadTextureBlock_4b from the
 * raw Image pointer (lbRelocGetFileData(Sprite*, file, offset) :2085 --
 * really an I/4b bitmap, not a Sprite struct), G_IM_FMT_I, width padded to
 * 16, then gSPVertex + gSP2Triangles over a 4-vert quad; the DObj children
 * made in scStaffrollMakeJobDObjs (:1540) / scStaffrollMakeNameGObjAndDObjs
 * (:1712) carry those DLs, and scStaffrollJobProcDisplay (:1499) /
 * scStaffrollNameProcDisplay (:1513) set PRIMITIVE combine
 * (gDPSetCombineLERP TEXEL0/PRIMITIVE) + XLU render mode before
 * gcDrawDObjTreeForGObj. Port sprite path (src/port/sprite_preview_backend.c)
 * cannot render them: it only accepts Sprite* (lbCommonMakeSObjForGObj :128,
 * SObj preview shape tests, wallpaper/decode caches) -- there is no
 * Image-bitmap-to-quad seam (no gDPLoadTextureBlock_4b / raw-I4-glyph path).
 * Seam (landed 2026-09-05, zero source lines touched): object-like rename
 * gcDrawDObjTreeForGObj -> ndsPortGcDrawDObjTreeForGObj above rebinds the
 * two call sites (:1509, :1523) to a wrapper that keeps the recorder call
 * and appends ndsStaffrollDrawGObjGlyphs (below): each DObj->dl is matched
 * against sSCStaffrollNameAndJobDisplayLists, dims/offset come from
 * dSCStaffrollNameAndJobSpriteInfo, the Image via
 * lbRelocGetFileData(sSCStaffrollFiles[0]), and the DS pixels via the
 * decode-once cache + PRIMITIVE-tinted XLU blit in
 * src/port/sprite_preview_backend.c. Init (:2053-2102), attach (:1570,
 * :1758), kerning, timing, and prim constants are preserved as-is.
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - No local shims and no local enum definitions. nSYAudioBGMStaffroll = 39
 *   already exists in port include/gm/gmsound.h; nSCKindStartup exists in
 *   include/sc/scene.h; ovl59_BSS_END is covered by DECLARE_OVL in
 *   include/sc/scene.h; func_80017EC0 is in include/sys/objhelper.h:92.
 * - SCStaffroll types/enums (SCStaffrollText/Sprite/Job/Name/Setup/Matrix/
 *   Projection, nSCStaffrollCompany*, GMSTAFFROLL_* font-index macros): in
 *   include/sc/scene.h:590+ and include/gm/generic.h since 2026-09-05.
 * - Left unresolved at link (never stubbed): func_800269C0_275C0
 *   if reached; everything else the TU calls is port-provided (gc*/lbReloc*/
 *   sy*/syAudioStopBGMAll/syAudioPlayBGM/lbCommonDrawSprite).
 * - Collisions needing reported gating (not renamed away, behaviour must
 *   win): scStaffrollStartScene (adapter below) vs
 *   src/port/title_backend.c:485 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <gm/gmsound.h>
#include <reloc_data.h>
#include <gm/generic.h> /* GMSTAFFROLL_* font indices (gmdef.h:18-35, restated) */
#include <sc/scene.h>
#include <sys/controller.h>
#include <sys/interp.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>

#define scStaffrollStartScene ndsBaseSCStaffrollStartScene
void ndsBaseSCStaffrollStartScene(void);

/* Port glyph seam (P2-6 step 8 tail): the decode/blit primitives live in
 * src/port/sprite_preview_backend.c (linked via scene_backend.o). The
 * object-like rename below rebinds the gcDrawDObjTreeForGObj CALL sites
 * (:1509, :1523) -- and only those, since :738/:1970 are bare references
 * carrying no paren -- to the wrapper defined after the include, which keeps
 * the recorder call and appends the glyph draw. An object-like rename only
 * swaps an identifier, so prototypes stay valid; every source line, including
 * the DL builder (:2053-2102) and DObj attach (:1570, :1758), is untouched. */
extern s32 ndsStaffrollGlyphEnsure(const void *image, u32 width, u32 height,
                                   u32 *out_slot);
extern void ndsStaffrollGlyphBlit(u32 slot, s32 org_x, s32 org_y,
                                  u8 prim_r, u8 prim_g, u8 prim_b,
                                  u16 *preview, u32 preview_pitch,
                                  u32 preview_width, u32 preview_height);
extern s32 ndsStaffrollFrameBegin(u16 **out_preview, u32 *out_pitch);
extern void ndsStaffrollFrameCommit(void);
extern void ndsStaffrollGlyphCacheInvalidate(void);
extern void ndsStaffrollDrawGObjGlyphs(struct GObj *gobj);
extern void ndsPortGcDrawDObjTreeForGObj(struct GObj *gobj);
#define gcDrawDObjTreeForGObj ndsPortGcDrawDObjTreeForGObj

#include "../../decomp/BattleShip-main/decomp/src/sc/sccommon/scstaffroll.c"

#undef scStaffrollStartScene
#undef gcDrawDObjTreeForGObj

/* Per-scene invalidation: the cache keys on Image pointers resolved from
 * sSCStaffrollFiles[0]; a changed file pointer means a new scene load, so
 * the previous scene's entries (and pool) are dropped before reuse. */
static const void *sNdsStaffrollGlyphFile = NULL;

static void ndsStaffrollWalkDObjs(DObj *dobj, f32 base_x, f32 base_y,
                                  u8 prim_r, u8 prim_g, u8 prim_b,
                                  u16 *preview, u32 preview_pitch)
{
    while (dobj != NULL)
    {
        f32 world_x = base_x + dobj->translate.vec.f.x;
        f32 world_y = base_y + dobj->translate.vec.f.y;
        u32 i;

        /* Only DObjs carrying a glyph DL (attached at :1570, :1758 from
         * sSCStaffrollNameAndJobDisplayLists) draw; every other DObj (scroll
         * parents, text-box roots) only contributes its offset. */
        for (i = 0u; i < ARRAY_COUNT(sSCStaffrollNameAndJobDisplayLists); i++)
        {
            if (dobj->dl == sSCStaffrollNameAndJobDisplayLists[i])
            {
                const void *image = lbRelocGetFileData(
                    const void *, sSCStaffrollFiles[0],
                    (const void *)dSCStaffrollNameAndJobSpriteInfo[i].offset);
                u32 slot;

                if (ndsStaffrollGlyphEnsure(
                        image,
                        (u32)dSCStaffrollNameAndJobSpriteInfo[i].width,
                        (u32)dSCStaffrollNameAndJobSpriteInfo[i].height,
                        &slot) != FALSE)
                {
                    ndsStaffrollGlyphBlit(
                        slot, (s32)world_x, (s32)world_y,
                        prim_r, prim_g, prim_b,
                        preview, preview_pitch, 320u, 240u);
                }
                break;
            }
        }
        if (dobj->child != NULL)
        {
            ndsStaffrollWalkDObjs(dobj->child, world_x, world_y,
                                  prim_r, prim_g, prim_b, preview,
                                  preview_pitch);
        }
        dobj = dobj->sib_next;
    }
}

void ndsStaffrollDrawGObjGlyphs(struct GObj *gobj)
{
    u8 prim_r;
    u8 prim_g;
    u8 prim_b;
    u16 *preview;
    u32 preview_pitch;
    DObj *root;

    if (gobj == NULL)
    {
        return;
    }
    /* Tint is the PRIMITIVE each display proc sets: job :1506
     * (0x7F, 0x7F, 0x89), name :1520 (0x88, 0x93, 0xFF). Any other DObj GObj
     * (:738, :1970) keeps the recorder call only. */
    if (gobj->proc_display == scStaffrollJobProcDisplay)
    {
        prim_r = 0x7Fu;
        prim_g = 0x7Fu;
        prim_b = 0x89u;
    }
    else if (gobj->proc_display == scStaffrollNameProcDisplay)
    {
        prim_r = 0x88u;
        prim_g = 0x93u;
        prim_b = 0xFFu;
    }
    else
    {
        return;
    }
    if ((gobj->obj_kind != 1u) || (gobj->obj == NULL))
    {
        return;
    }
    if (sNdsStaffrollGlyphFile != sSCStaffrollFiles[0])
    {
        ndsStaffrollGlyphCacheInvalidate();
        sNdsStaffrollGlyphFile = sSCStaffrollFiles[0];
    }
    root = (DObj *)gobj->obj;
    if (ndsStaffrollFrameBegin(&preview, &preview_pitch) == FALSE)
    {
        return;
    }
    ndsStaffrollWalkDObjs(root, 0.0F, 0.0F, prim_r, prim_g, prim_b,
                          preview, preview_pitch);
    ndsStaffrollFrameCommit();
}

void ndsPortGcDrawDObjTreeForGObj(struct GObj *gobj)
{
    gcDrawDObjTreeForGObj(gobj);
    ndsStaffrollDrawGObjGlyphs(gobj);
}

void scStaffrollStartScene(void)
{
    ndsBaseSCStaffrollStartScene();
}

#endif /* NDS_P2_1P_GAME */

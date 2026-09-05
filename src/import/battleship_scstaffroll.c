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
 * Missing seam: a staffroll glyph path that turns an SCStaffrollSprite
 * {width,height,Image offset} row into a textured quad under the
 * PRIMITIVE-tinted XLU combine, or a pre-bake of the 59 I4 glyphs into
 * Sprite records the SObj path accepts.
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - No local shims and no local enum definitions. nSYAudioBGMStaffroll = 39
 *   already exists in port include/gm/gmsound.h; nSCKindStartup exists in
 *   include/sc/scene.h; ovl59_BSS_END is covered by DECLARE_OVL in
 *   include/sc/scene.h; func_80017EC0 is in include/sys/objhelper.h:92.
 * - SCStaffroll types/enums from decomp sc/scdef.h + sc/sctypes.h
 *   (SCStaffrollText/Sprite/Job/Name/Setup/Matrix/Projection,
 *   nSCStaffrollCompanyHAL/Null/NINTENDO, GMSTAFFROLL_* font-index macros)
 *   are NOT defined in port include/sc/scene.h: listed, not defined here --
 *   a separate task widens the headers (same policy as mndef.h/gmsound.h
 *   enumerators in the brief).
 * - Left unresolved at link (never stubbed): the SCStaffroll type/enum rows
 *   above (block compile until the header task lands); func_800269C0_275C0
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

#include "../../decomp/BattleShip-main/decomp/src/sc/sccommon/scstaffroll.c"

#undef scStaffrollStartScene

void scStaffrollStartScene(void)
{
    ndsBaseSCStaffrollStartScene();
}

#endif /* NDS_P2_1P_GAME */

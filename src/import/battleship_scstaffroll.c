/* P2-6 step 8 tail. Staff-roll credits, source import: overlay copy of
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
 * here. The one overlay delta (scripts/import-overlays/battleship/
 * src_sc_sccommon_scstaffroll.patch) stores the five credit character-ID
 * tables as s8 instead of s32: every measured value fits (-55..73) with
 * exact C promotion, so all readers compare and index identically. The four
 * generated tables keep source-generated values via narrowed overlay copies
 * (credits/*.narrow, emitted by scripts/
 * generate-battleship-import-overlay.ps1); CompanyIDs keeps its source enum
 * initializers. This saves 13863 B of battle-resident RAM; see
 * builds/resume-20260905/staffroll-data-width.md.
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
 *   if reached; everything else the TU calls is port-provided (gc, lbReloc,
 *   sy, syAudioStopBGMAll, syAudioPlayBGM, lbCommonDrawSprite).
 * - Collisions needing reported gating (not renamed away, behaviour must
 *   win): scStaffrollStartScene (adapter below) vs
 *   src/port/title_backend.c:485 NDS_SCENE_STUB.
 *
 * DS platform entry (1P build): source scStaffrollStartScene :2311-2339
 * brackets syVideoInit/syTaskmanStartTask with N64 framebuffer clear loops to
 * 0x80400000 (:2326-2328, :2336-2338). Those loops NEVER run here -- mapping
 * the address macro to a DS buffer and running them would overwrite RAM. The
 * wrapper below replaces only the platform start: the three video slots alias
 * &gSYFramebufferSets[0] (like mnTitleStartScene) with the DS z-buffer
 * extent, syVideoInit runs, and the task starts on ndsTaskmanArenaStart/Size
 * with the ORIGINAL scStaffrollFuncStart and scStaffrollFuncDraw. Blackout
 * needs no work here: source scStaffrollFuncDraw :2245 latches
 * SYVIDEO_FLAG_BLACKOUT once (-1 -> -2) and the shared video seam
 * (battleship_sys_video.c) mirrors that onto the DS brightness latch, while
 * syVideoInit clears it so the Startup/OpeningRoom entry after RollEndWait
 * recovers. Credit tables, DL builder, attach, timing, and exit gameflow
 * (BLACKOUT to Startup/OpeningRoom) are untouched; user-facing credit
 * rendering is still owed (source-derived, no invented bitmap).
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <gm/gmsound.h>
#include <mn/menu.h>
#include <reloc_data.h>
#include <gm/generic.h> /* GMSTAFFROLL_* font indices (gmdef.h:18-35, restated) */
#include <nds/nds_obj_anim.h> /* gcDrawDObjTreeForGObj, decomp sys/objdisplay.h:46 */
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/controller.h>
#include <sys/interp.h>
#include <sys/matrix.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>
#include <string.h>
#include <nds/arm9/video.h>
#include <nds/nds_effects.h>
#include <nds/nds_platform.h>
#include <nds/nds_renderer.h>

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);

#define scStaffrollStartScene ndsBaseSCStaffrollStartScene
void ndsBaseSCStaffrollStartScene(void);

/* Port draw seam (P2-6 step 8 tail). The object-like rename below rebinds
 * the gcDrawDObjTreeForGObj CALL sites (:1509, :1523) -- and only those, since
 * :738/:1970 are bare references carrying no paren -- to the wrapper defined
 * after the include, which keeps the recorder call and draws the glyphs on the
 * 3D engine. An object-like rename only swaps an identifier, so prototypes
 * stay valid; every source line, including the DL builder (:2053-2102) and
 * DObj attach (:1570, :1758), is untouched. */
extern void ndsPortGcDrawDObjTreeForGObj(struct GObj *gobj);
#define gcDrawDObjTreeForGObj ndsPortGcDrawDObjTreeForGObj

/* Exact source header decomp sc/sccommon/scstaffroll.h:44-45 (used at
 * :1393-1394 before their definitions). */
extern void scStaffrollMakeTextBoxBracketSObjs(void);
extern void scStaffrollMakeTextBoxGObj(void);

#include <battleship_overlay/src/sc/sccommon/scstaffroll.c>

#undef scStaffrollStartScene
#undef gcDrawDObjTreeForGObj

/* THE NAMES AND JOBS, natively.
 *
 * Each glyph is the quad scStaffrollInitNameAndJobDisplayLists (:2053-2102)
 * builds: object corners (+-width, +-height, 0), the I/4b image's
 * (0,0)-(width,height) texels across it, drawn under the root DObj's
 * TraRotRpyRSca matrix (the cubic path and the AnimJoint) and the glyph DObj's
 * Tra offset, by the 3D camera, with TEXEL0 x PRIMITIVE colour and TEXEL0
 * alpha over an XLU blend (:1499-1525). On the DS that is one textured
 * parallelogram per glyph through the particle quad path, whose texture is
 * A3I5: alpha I*7/15, a grey ramp index I*31/15, tinted by the PRIMITIVE vertex
 * colour. One texture per glyph, made on first use; the scene's textures are
 * released when a new staff roll loads its file. */
static u32 sNdsStaffrollGlyphNames[ARRAY_COUNT(dSCStaffrollNameAndJobSpriteInfo)];
static const void *sNdsStaffrollGlyphFile = NULL;
volatile u32 gNdsStaffrollGlyphDraws;
volatile u32 gNdsStaffrollGlyphFailures;

typedef struct NDSStaffrollGlyphFill
{
    const u8 *image;
    u32 width;
    u32 height;
    u32 texture_width;
} NDSStaffrollGlyphFill;

static u32 ndsStaffrollPow2(u32 value)
{
    u32 size = 8u;

    while (size < value)
    {
        size <<= 1;
    }
    return size;
}

static s32 ndsStaffrollGlyphFillTexels(u8 *pixels, u32 bytes, void *user_data)
{
    const NDSStaffrollGlyphFill *fill = (const NDSStaffrollGlyphFill *)user_data;
    u32 row_bytes;
    u32 x;
    u32 y;

    if ((pixels == NULL) || (fill == NULL) || (fill->image == NULL))
    {
        return FALSE;
    }
    memset(pixels, 0, bytes);
    /* The DL loads the image `width` rounded up to 16 texels per row
     * (:2088); rows are packed 4 bits a texel, high nibble first, in a file
     * whose 32-bit words the relocator byte-swapped (hence ^3). */
    row_bytes = ((fill->width + 15u) / 16u) * 8u;
    for (y = 0u; y < fill->height; y++)
    {
        for (x = 0u; x < fill->width; x++)
        {
            u32 index = (y * row_bytes) + (x >> 1);
            u8 packed = fill->image[index ^ 3u];
            u32 level = ((x & 1u) == 0u) ? (u32)(packed >> 4) :
                                           (u32)(packed & 0x0fu);
            u32 texel = (y * fill->texture_width) + x;

            if (texel < bytes)
            {
                pixels[texel] = (u8)((((level * 7u) + 7u) / 15u) << 5) |
                                (u8)(((level * 31u) + 7u) / 15u);
            }
        }
    }
    return TRUE;
}

static void ndsStaffrollReleaseGlyphs(void)
{
    u32 i;

    for (i = 0u; i < ARRAY_COUNT(sNdsStaffrollGlyphNames); i++)
    {
        if (sNdsStaffrollGlyphNames[i] != 0u)
        {
            ndsRendererHardwareReleaseIFCommonCloudAtlas(
                &sNdsStaffrollGlyphNames[i]);
            sNdsStaffrollGlyphNames[i] = 0u;
        }
    }
}

static u32 ndsStaffrollGlyphTexture(u32 glyph)
{
    static u16 ramp[32];
    NDSStaffrollGlyphFill fill;
    u32 i;

    if (sNdsStaffrollGlyphNames[glyph] != 0u)
    {
        return sNdsStaffrollGlyphNames[glyph];
    }
    if (ramp[31] == 0u)
    {
        for (i = 0u; i < 32u; i++)
        {
            ramp[i] = (u16)(i | (i << 5) | (i << 10));
        }
    }
    fill.image = lbRelocGetFileData(
        const u8 *, sSCStaffrollFiles[0],
        (const void *)dSCStaffrollNameAndJobSpriteInfo[glyph].offset);
    fill.width = (u32)dSCStaffrollNameAndJobSpriteInfo[glyph].width;
    fill.height = (u32)dSCStaffrollNameAndJobSpriteInfo[glyph].height;
    fill.texture_width = ndsStaffrollPow2(fill.width);
    if (ndsRendererHardwarePrepareIFCommonA3I5Atlas(
            fill.texture_width, ndsStaffrollPow2(fill.height), ramp,
            ndsStaffrollGlyphFillTexels, &fill,
            &sNdsStaffrollGlyphNames[glyph]) == FALSE)
    {
        sNdsStaffrollGlyphNames[glyph] = 0u;
    }
    return sNdsStaffrollGlyphNames[glyph];
}

static void ndsStaffrollDrawGObjGlyphs(struct GObj *gobj)
{
    u32 color;
    DObj *root;
    DObj *glyph_dobj;
    Mtx44f mf;

    /* Tint is the PRIMITIVE each display proc sets: job :1506
     * (0x7F, 0x7F, 0x89), name :1520 (0x88, 0x93, 0xFF). Any other DObj GObj
     * (:738, :1970) keeps the recorder call only. */
    if (gobj->proc_display == scStaffrollJobProcDisplay)
    {
        color = RGB15(0x7F >> 3, 0x7F >> 3, 0x89 >> 3);
    }
    else if (gobj->proc_display == scStaffrollNameProcDisplay)
    {
        color = RGB15(0x88 >> 3, 0x93 >> 3, 0xFF >> 3);
    }
    else
    {
        return;
    }
    if ((gobj->obj_kind != nGCCommonAppendDObj) || (gobj->obj == NULL))
    {
        return;
    }
    if (sNdsStaffrollGlyphFile != sSCStaffrollFiles[0])
    {
        ndsStaffrollReleaseGlyphs();
        sNdsStaffrollGlyphFile = sSCStaffrollFiles[0];
    }
    root = (DObj *)gobj->obj;
    if ((root->flags & DOBJ_FLAG_HIDDEN) != 0u)
    {
        return;
    }
    syMatrixTraRotRpyRScaF(&mf, root->translate.vec.f.x,
                           root->translate.vec.f.y, root->translate.vec.f.z,
                           root->rotate.vec.f.x, root->rotate.vec.f.y,
                           root->rotate.vec.f.z, root->scale.vec.f.x,
                           root->scale.vec.f.y, root->scale.vec.f.z);
    for (glyph_dobj = root->child; glyph_dobj != NULL;
         glyph_dobj = glyph_dobj->sib_next)
    {
        u32 glyph;

        if ((glyph_dobj->flags & DOBJ_FLAG_HIDDEN) != 0u)
        {
            continue;
        }
        for (glyph = 0u; glyph < ARRAY_COUNT(sSCStaffrollNameAndJobDisplayLists);
             glyph++)
        {
            if (glyph_dobj->dl == sSCStaffrollNameAndJobDisplayLists[glyph])
            {
                break;
            }
        }
        if (glyph >= ARRAY_COUNT(sSCStaffrollNameAndJobDisplayLists))
        {
            continue;
        }
        {
            /* Row vectors, as the source composes: p * Tra(child) * root. */
            const Vec3f *t = &glyph_dobj->translate.vec.f;
            f32 w = (f32)dSCStaffrollNameAndJobSpriteInfo[glyph].width;
            f32 h = (f32)dSCStaffrollNameAndJobSpriteInfo[glyph].height;
            Vec3f centre;
            Vec3f right;
            Vec3f up;
            u32 name = ndsStaffrollGlyphTexture(glyph);

            centre.x = (t->x * mf[0][0]) + (t->y * mf[1][0]) +
                       (t->z * mf[2][0]) + mf[3][0];
            centre.y = (t->x * mf[0][1]) + (t->y * mf[1][1]) +
                       (t->z * mf[2][1]) + mf[3][1];
            centre.z = (t->x * mf[0][2]) + (t->y * mf[1][2]) +
                       (t->z * mf[2][2]) + mf[3][2];
            right.x = w * mf[0][0];
            right.y = w * mf[0][1];
            right.z = w * mf[0][2];
            up.x = h * mf[1][0];
            up.y = h * mf[1][1];
            up.z = h * mf[1][2];
            if ((name != 0u) &&
                (ndsParticleDrawOwnTextureParallelogram(
                     name, (u32)w, (u32)h, &centre, &right, &up, color,
                     0xFFu) != FALSE))
            {
                gNdsStaffrollGlyphDraws++;
            }
            else
            {
                gNdsStaffrollGlyphFailures++;
            }
        }
    }
}

void ndsPortGcDrawDObjTreeForGObj(struct GObj *gobj)
{
    gcDrawDObjTreeForGObj(gobj);
    if (gobj != NULL)
    {
        ndsStaffrollDrawGObjGlyphs(gobj);
    }
}

/* THE FILL RECTANGLES: the text box frame (dSCStaffrollTextBoxDisplayList,
 * :476-487, while its GObj lives) and the lock-on highlight
 * (scStaffrollHighlightProcDisplay, :785-826), both G_CYC_FILL rectangles in
 * the 640x480 frame, drawn by the 3D camera over the names. On the DS they are
 * painted into the sprite overlay layer, mapped like the scene's sprites
 * ((20,20)-(620,460) onto 256x192), and erased the next frame. */
#define NDS_STAFFROLL_RECT_MAX 8u

typedef struct NDSStaffrollRect
{
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
} NDSStaffrollRect;

static NDSStaffrollRect sNdsStaffrollRects[NDS_STAFFROLL_RECT_MAX];
static u32 sNdsStaffrollRectCount;
static sb32 sNdsStaffrollBackdropSet;

static s32 ndsStaffrollMapX(s32 x)
{
    s32 mapped = ((x - 20) * 256) / 600;

    return (mapped < 0) ? 0 : ((mapped > 256) ? 256 : mapped);
}

static s32 ndsStaffrollMapY(s32 y)
{
    s32 mapped = ((y - 20) * 192) / 440;

    return (mapped < 0) ? 0 : ((mapped > 192) ? 192 : mapped);
}

static void ndsStaffrollFillRect(u16 *layer, u32 pitch, const NDSStaffrollRect *r,
                                 u16 value)
{
    s32 x;
    s32 y;

    for (y = r->y0; y < r->y1; y++)
    {
        for (x = r->x0; x < r->x1; x++)
        {
            layer[((u32)y * pitch) + (u32)x] = value;
        }
    }
}

static void ndsStaffrollAddRect(NDSStaffrollRect *rects, u32 *count,
                                s32 ulx, s32 uly, s32 lrx, s32 lry)
{
    NDSStaffrollRect *r;

    if (*count >= NDS_STAFFROLL_RECT_MAX)
    {
        return;
    }
    r = &rects[(*count)++];
    r->x0 = (s16)ndsStaffrollMapX(ulx);
    r->y0 = (s16)ndsStaffrollMapY(uly);
    /* A 2-pixel hi-res edge stays at least one DS pixel. */
    r->x1 = (s16)ndsStaffrollMapX(lrx);
    r->y1 = (s16)ndsStaffrollMapY(lry);
    if (r->x1 <= r->x0) { r->x1 = (s16)(r->x0 + 1); }
    if (r->y1 <= r->y0) { r->y1 = (s16)(r->y0 + 1); }
}

static void ndsStaffrollDrawRects(void)
{
    NDSStaffrollRect rects[NDS_STAFFROLL_RECT_MAX];
    u32 count = 0u;
    u32 frame_count = 0u;
    u32 box_count;
    u32 pitch = 0u;
    u16 *layer;
    GObj *gobj;
    u32 i;

    layer = ndsPlatformGetOriginalSpriteOverlayLayer(TRUE, &pitch, NULL, NULL,
                                                     NULL);
    if ((layer == NULL) || (pitch == 0u))
    {
        return;
    }
    for (gobj = gGCCommonLinks[7]; gobj != NULL; gobj = gobj->link_next)
    {
        if ((gobj->obj != NULL) &&
            (DObjGetStruct(gobj)->dl == dSCStaffrollTextBoxDisplayList))
        {
            ndsStaffrollAddRect(rects, &count, 346, 35, 348, 164);
            ndsStaffrollAddRect(rects, &count, 346, 35, 584, 37);
            ndsStaffrollAddRect(rects, &count, 582, 35, 584, 164);
            ndsStaffrollAddRect(rects, &count, 346, 162, 584, 164);
            break;
        }
    }
    box_count = count;
    if (gGCCommonLinks[nGCCommonLinkIDHighlight] != NULL)
    {
        s32 size = sSCStaffrollHighlightSize;
        f32 x = sSCStaffrollHighlightPositionX;
        f32 y = sSCStaffrollHighlightPositionY;

        ndsStaffrollAddRect(rects, &count,
            scStaffrollGetLockOnPositionX((size * -30) + x),
            scStaffrollGetLockOnPositionY((size * -25) + y),
            scStaffrollGetLockOnPositionX(((size * -30) + 2) + x),
            scStaffrollGetLockOnPositionY(((size * 45) + 2) + y));
        ndsStaffrollAddRect(rects, &count,
            scStaffrollGetLockOnPositionX((size * -30) + x),
            scStaffrollGetLockOnPositionY((size * -25) + y),
            scStaffrollGetLockOnPositionX(((size * 65) + 2) + x),
            scStaffrollGetLockOnPositionY(((size * -25) + 2) + y));
        ndsStaffrollAddRect(rects, &count,
            scStaffrollGetLockOnPositionX((size * -30) + x),
            scStaffrollGetLockOnPositionY((size * 45) + y),
            scStaffrollGetLockOnPositionX(((size * 65) + 2) + x),
            scStaffrollGetLockOnPositionY(((size * 45) + 2) + y));
        ndsStaffrollAddRect(rects, &count,
            scStaffrollGetLockOnPositionX((size * 65) + x),
            scStaffrollGetLockOnPositionY((size * -25) + y),
            scStaffrollGetLockOnPositionX(((size * 65) + 2) + x),
            scStaffrollGetLockOnPositionY(((size * 45) + 2) + y));
    }
    for (i = 0u; i < sNdsStaffrollRectCount; i++)
    {
        ndsStaffrollFillRect(layer, pitch, &sNdsStaffrollRects[i], 0u);
    }
    for (i = 0u; i < count; i++)
    {
        ndsStaffrollFillRect(layer, pitch, &rects[i],
                             (i < box_count) ?
                                 (u16)(0x8000u | RGB15(0x42 >> 3, 0x3A >> 3,
                                                       0x31 >> 3)) :
                                 (u16)(0x8000u | RGB15(0x80 >> 3, 0, 0)));
        sNdsStaffrollRects[i] = rects[i];
    }
    frame_count = count;
    sNdsStaffrollRectCount = frame_count;
}

/* scStaffrollFuncDraw with the scene's DS frame around it: the 3D camera's
 * (20,20)-(620,460) viewport is the 640x480 form of the 320x240 window every
 * other source scene presents, and the fill rectangles follow the frame. */
static void ndsStaffrollFuncDraw(void)
{
    if (sNdsStaffrollBackdropSet == FALSE)
    {
        /* The scene's default camera fills black (scstaffroll.c:2189); the
         * backdrop otherwise keeps the previous menu's colour. At the first
         * draw, so a held transition frame stays until the scene draws. */
        ndsPlatformSetBackdropColor(RGB15(0, 0, 0));
        sNdsStaffrollBackdropSet = TRUE;
    }
    ndsPlatformSet3DLayerEnabled(TRUE);
    ndsPlatformSet3DViewportSource(10, 10, 310, 230);
    scStaffrollFuncDraw();
    ndsPlatformReset3DViewport();
    ndsStaffrollDrawRects();
}

void scStaffrollStartScene(void)
{
    SYTaskmanSetup setup;

    dSCStaffrollVideoSetup.framebuffers[0] = &gSYFramebufferSets[0];
    dSCStaffrollVideoSetup.framebuffers[1] = &gSYFramebufferSets[0];
    dSCStaffrollVideoSetup.framebuffers[2] = &gSYFramebufferSets[0];
    dSCStaffrollVideoSetup.zbuffer = SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dSCStaffrollVideoSetup);

    setup = dSCStaffrollTaskmanSetup;
    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.scene_setup.func_draw = ndsStaffrollFuncDraw;
    setup.func_start = scStaffrollFuncStart;
    sNdsStaffrollRectCount = 0u;
    sNdsStaffrollBackdropSet = FALSE;
    syTaskmanStartTask(&setup);
}

#endif /* NDS_P2_1P_GAME */

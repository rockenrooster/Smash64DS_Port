/* P2-7 item 2. Newcomer-unlock message scene, source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mncommon/mnmessage.c whole,
 * following src/import/battleship_sc1pbonusstage.c (scene TU with the scene
 * entry imported as ndsBase* and re-exported under its source name, so a
 * later measured DS arena rebudget has a seam and the diff stays reviewable).
 * The adapter is a verbatim pass-through; no behaviour invented here.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md item 2):
 * - Bits are decomp lb/lbdef.h:134-140, applied at mnmessage.c:284-301:
 *   unlock_mask |= (1 << id), fighter_mask |= (1 << fkind) plus
 *   characters_fkind = fkind for the four newcomers (Luigi/Ness/Captain/
 *   Purin via fkinds[] :287), then lbBackupWrite() :301.
 * - Challenger-approaches vs unlocked screens are the same TU: wallpaper
 *   (collage :68-80), tint (:83-101), exclaim (:104-119), per-unlock message
 *   sprite (:122-171, 7 offsets :128-137 + US positions :142-149), four
 *   cameras (:174-271), queue drain in mnMessageStartScene (:409-462, one
 *   task per queued unlock_message, VSResults exit routes to PlayersVS
 *   :448-452 else Startup (US) :456-458).
 * - mnMessageFuncRun :305-325: 120-tic hold, stick-centred reset of the
 *   dead-up-star wait, A/B/START applies the unlock and loads the scene.
 *
 * There is no battleship_mnmessagefiles.c: decomp mn/mncommon/ holds only
 * mnmessage.c + mnmessage.h (no mnmessagefiles.c exists there), and the
 * single TU already carries its file list (dMNMessageFileIDs :15), status
 * buffer (:50) and setup (FuncStart :328-358). Unlike the bonus stage,
 * nothing splits off.
 *
 * Gated on NDS_P2_1P_GAME: the Makefile has no NDS_P2_MODES_META flag
 * (verified 2026-09-05, same gate as battleship_mnoption.c /
 * battleship_mnbackupclear.c / battleship_mnsoundtest.c), so this rides the
 * campaign flag until P2-7 mints its own gate.
 *
 * Shell status: same as the Options/SoundTest imports -- the native shell
 * cannot reach nSCKindMessage today and item 9 (Menu completion) wires it,
 * not this slice. Stops at the import by design.
 *
 * Shims vs unresolved, see handoff report:
 * - sc1PManagerCheckUnlockSoundTest (called :429, REGION_JP arm only):
 *   shimmed below as a local extern; PROVIDED by the sibling TU
 *   battleship_sc1pmanager.c behind the same gate (whole-TU include of
 *   sc1pmanager.c, which defines it at :189). On REGION_US the call
 *   compiles out and the extern is harmless.
 * - nSYAudioBGMMessage (played :356): in include/gm/gmsound.h since the
 *   2026-09-05 widening. nSYAudioFGMDeadUpStar (:357) IS carried
 *   (port gmsound.h:60) and func_800269C0_275C0 rides include/sys/audio.h:92.
 * - ll* rows: NONE unresolved. dMNMessageFileIDs needs llMNCommonFileID +
 *   llMNMessageFileID (both declared: reloc_data.h:156 + :1207); the
 *   wallpaper needs llMNCommonSmashBrosCollageSprite (:178); the exclaim +
 *   7 unlock sprites are the staged NDS_MN_MESSAGE_RELOC_SYMBOLS block
 *   (:1209-1221, file 0x9 reloc_menus). No manifest staging outstanding.
 * - Resolved port-side, no action: gcMakeGObjSPAfter / gcAddGObjDisplay /
 *   gcRunAll / gcDrawAll (sys/objman.h), gcMakeCameraGObj /
 *   gcMakeDefaultCameraGObj (sys/objhelper.h), lbCommonDrawSObjAttr /
 *   lbCommonMakeSObjForGObj / lbCommonClearExternSpriteParams /
 *   lbCommonDrawSprite (mn/menu.h, owned by battleship_lb_common.c),
 *   lbRelocGetFileData / lbRelocInitSetup / lbRelocLoadFilesListed +
 *   lLBRelocTableAddr / llRelocFileCount (reloc_data.h),
 *   syTaskmanStartTask / syTaskmanSetLoadScene (sys/taskman.h), syVideoInit
 *   + SYVIDEO_SETUP_DEFAULT (sys/video.h), syRdpSetViewport (sys/rdp.h),
 *   syControllerFuncRead (sys/controller.h), syAudioPlayBGM (sys/audio.h),
 *   scSubsysControllerGetPlayerTapButtons /
 *   scSubsysControllerGetPlayerStickInRangeLR/UD (sc/scene.h:561-567),
 *   lbBackupWrite (mn/menu.h + battleship_lbbackup.c), gSCManagerSceneData
 *   / gSCManagerBackupData + nLBBackupUnlock* + SCKind + ovl1_VRAM /
 *   ovl22_BSS_END (sc/scene.h), nFTKindLuigi/Ness/Captain/Purin (ft/
 *   fighter.h).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnMessageStartScene (adapter below) vs
 *   src/port/title_backend.c:419 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <nds/nds_backup.h>
#include <nds/nds_p7.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_source2d.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <gm/gmsound.h>
#include <mn/menu.h>
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

/* decomp sc/sc1pmode/sc1pmanager.c:189. Defined by the sibling TU
 * battleship_sc1pmanager.c behind this same gate; only called here on the
 * REGION_JP arm (mnmessage.c:429), so on REGION_US this extern binds
 * nothing and costs nothing. */
sb32 sc1PManagerCheckUnlockSoundTest(void);

#define mnMessageStartScene ndsBaseMNMessageStartScene
void ndsBaseMNMessageStartScene(void);

/* The tint (mnMessageTintProcDisplay, :83-96): its fill rectangle -- prim
 * (0x00, 0x00, 0xFF, 0x3F) under G_RM_AA_XLU_SURF -- is the source 2D
 * presenter's translucent fill over the wallpaper; the state words around it
 * are consumed as every tenant's are. It was the screen's one native
 * failure a frame (SPRITE / NO_PROGRAM at the menu fill drain). */
#pragma push_macro("gDPFillRectangle")
#undef gDPFillRectangle
#define gDPFillRectangle(pkt, ulx, uly, lrx, lry) \
    ndsSource2DFillTranslucent((ulx), (uly), (lrx), (lry), 0x0000FF3Fu)

#include "../../decomp/BattleShip-main/decomp/src/mn/mncommon/mnmessage.c"

#pragma pop_macro("gDPFillRectangle")
#undef mnMessageStartScene

void mnMessageStartScene(void)
{
    ndsBaseMNMessageStartScene();
}

/* P7 U6: a P7 unlock's message. The source's scene -- the collage
 * wallpaper, the tint, the exclaim, the four cameras, the message BGM and
 * cue, the 120-tic hold and A/B/START -- built from its own makers, with the
 * row's lines (scripts/p7/generate_p7_messages.py, the source's I4 style:
 * NitroFS p7/messages.bin, read into this scene's heap) in place of the
 * source sprite, and no source unlock applied: the 1P manager's seam
 * delivered the row before its message shows. */
typedef struct NDSP7Message
{
    u8 row;
    u8 width;
    u8 height;
    s16 x;
    s16 y;
    u32 offset;
} NDSP7Message;
#include "../nds/nds_p7_messages.generated.inc"

static Sprite sNdsP7MessageSprite;
static const NDSP7Message *sNdsP7MessageCurrent;

/* The row's Bitmap and texels in one block of this scene's heap, the texels
 * read from the file and the block registered as the scene's loaded file
 * (the sprite presenter draws only what a file holds). NULL when it cannot
 * be read: the screen shows the wallpaper and the exclaim alone. */
static Bitmap *ndsP7MessageLoad(const NDSP7Message *m)
{
    u32 header[2];
    u32 bytes = ((u32)m->width / 2u) * (u32)m->height;
    u32 head = (sizeof(Bitmap) + 7u) & ~7u;
    u8 *block = syTaskmanMalloc(head + bytes, 8);
    Bitmap *bitmap = (Bitmap *)block;
    FILE *file;
    sb32 ok = FALSE;

    if (block == NULL)
    {
        return NULL;
    }
    ndsFsLock();
    file = fopen("nitro:/p7/messages.bin", "rb");
    if (file != NULL)
    {
        ok = ((fread(header, sizeof(header), 1u, file) == 1u) &&
              (header[0] == NDS_P7_MESSAGE_MAGIC) && (header[1] == NDS_P7_MESSAGE_BYTES) &&
              (fseek(file, (long)(sizeof(header) + m->offset), SEEK_SET) == 0) &&
              (fread(block + head, 1u, bytes, file) == bytes)) ? TRUE : FALSE;
        fclose(file);
    }
    ndsFsUnlock();
    if (ok == FALSE)
    {
        return NULL;
    }
    memset(bitmap, 0, sizeof(*bitmap));
    bitmap->width = m->width;
    bitmap->width_img = m->width;
    bitmap->buf = block + head;
    bitmap->actualHeight = m->height;
    if (ndsRelocRegisterSyntheticFile(NDS_RELOC_SYNTHETIC_ASSET_BASE + m->row, block,
                                      head + bytes) == FALSE)
    {
        return NULL;
    }
    return bitmap;
}

static void ndsP7MessageMakeLines(const NDSP7Message *m)
{
    GObj *gobj;
    SObj *sobj;
    Bitmap *bitmap = ndsP7MessageLoad(m);

    if (bitmap == NULL)
    {
        return;
    }
    memset(&sNdsP7MessageSprite, 0, sizeof(sNdsP7MessageSprite));
    sNdsP7MessageSprite.width = m->width;
    sNdsP7MessageSprite.height = m->height;
    sNdsP7MessageSprite.scalex = 1.0F;
    sNdsP7MessageSprite.scaley = 1.0F;
    sNdsP7MessageSprite.red = 0xFF;
    sNdsP7MessageSprite.green = 0xFF;
    sNdsP7MessageSprite.blue = 0xFF;
    sNdsP7MessageSprite.alpha = 0xFF;
    sNdsP7MessageSprite.nbitmaps = 1;
    sNdsP7MessageSprite.bmheight = m->height;
    sNdsP7MessageSprite.bmHreal = m->height;
    sNdsP7MessageSprite.bmfmt = G_IM_FMT_I;
    sNdsP7MessageSprite.bmsiz = G_IM_SIZ_4b;
    sNdsP7MessageSprite.bitmap = bitmap;

    gobj = gcMakeGObjSPAfter(0, NULL, 3, GOBJ_PRIORITY_DEFAULT);
    gcAddGObjDisplay(gobj, lbCommonDrawSObjAttr, 1, GOBJ_PRIORITY_DEFAULT, ~0);
    sobj = lbCommonMakeSObjForGObj(gobj, &sNdsP7MessageSprite);
    sobj->sprite.attr &= ~SP_FASTCOPY;
    sobj->sprite.attr |= SP_TRANSPARENT;
    sobj->pos.x = m->x;
    sobj->pos.y = m->y;
}

/* mnMessageFuncRun (:305-325) without the source's unlock. */
static void ndsP7MessageFuncRun(GObj *gobj)
{
    (void)gobj;
    sMNMessageTotalTimeTics++;
    if (sMNMessageTotalTimeTics >= 120)
    {
        if (sMNMessageUnk0x80132660 != 0)
        {
            sMNMessageUnk0x80132660--;
        }
        if ((scSubsysControllerGetPlayerStickInRangeLR(-30, 30) != FALSE) &&
            (scSubsysControllerGetPlayerStickInRangeUD(-30, 30) != FALSE))
        {
            sMNMessageUnk0x80132660 = 0;
        }
        if (scSubsysControllerGetPlayerTapButtons(A_BUTTON | B_BUTTON | START_BUTTON) != FALSE)
        {
            syTaskmanSetLoadScene();
        }
    }
}

/* mnMessageFuncStart (:328-358) with the row's lines. */
static void ndsP7MessageFuncStart(void)
{
    LBRelocSetup rl_setup;

    rl_setup.table_addr = (uintptr_t)&lLBRelocTableAddr;
    rl_setup.table_files_num = (u32)&llRelocFileCount;
    rl_setup.file_heap = NULL;
    rl_setup.file_heap_size = 0;
    rl_setup.status_buffer = sMNMessageStatusBuffer;
    rl_setup.status_buffer_size = ARRAY_COUNT(sMNMessageStatusBuffer);
    rl_setup.force_status_buffer = NULL;
    rl_setup.force_status_buffer_size = 0;

    lbRelocInitSetup(&rl_setup);
    lbRelocLoadFilesListed(dMNMessageFileIDs, sMNMessageFiles);
    gcMakeGObjSPAfter(0, ndsP7MessageFuncRun, 0, GOBJ_PRIORITY_DEFAULT);
    gcMakeDefaultCameraGObj(0, GOBJ_PRIORITY_DEFAULT, 100, 0, GPACK_RGBA8888(0x00, 0x00, 0x00, 0x00));

    sMNMessageUnk0x80132660 = 0;
    sMNMessageTotalTimeTics = 0;
    mnMessageMakeWallpaperCamera();
    mnMessageMakeMessageCamera();
    mnMessageMakeTintCamera();
    mnMessageMakeExclaimCamera();
    mnMessageMakeWallpaper();
    mnMessageMakeTint();
    mnMessageMakeExclaim();
    ndsP7MessageMakeLines(sNdsP7MessageCurrent);

    syAudioPlayBGM(0, nSYAudioBGMMessage);
    func_800269C0_275C0(nSYAudioFGMDeadUpStar);
}

void ndsP7ShowMessages(const u8 *rows, u32 count)
{
    SYTaskmanSetup setup;
    u32 i;
    u32 k;

    for (i = 0u; (rows != NULL) && (i < count); i++)
    {
        sNdsP7MessageCurrent = NULL;
        for (k = 0u; k < NDS_P7_MESSAGE_COUNT; k++)
        {
            if (sNdsP7Messages[k].row == rows[i])
            {
                sNdsP7MessageCurrent = &sNdsP7Messages[k];
                break;
            }
        }
        if (sNdsP7MessageCurrent == NULL)
        {
            continue;
        }
        dMNMessageVideoSetup.zbuffer = SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
        syVideoInit(&dMNMessageVideoSetup);
        setup = dMNMessageTaskmanSetup;
        setup.func_start = ndsP7MessageFuncStart;
        setup.scene_setup.arena_size =
            (size_t)((uintptr_t)&ovl1_VRAM - (uintptr_t)&ovl22_BSS_END);
        syTaskmanStartTask(&setup);
    }
}

#endif /* NDS_P2_1P_GAME */

/* P2-6 step 8. Continue screen (score halves at decomp :1075).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mn1pmode/mn1pcontinue.c whole,
 * following src/import/battleship_mnoption.c (scene TU with the scene
 * entry imported as ndsBase* and re-exported under its source name, so a
 * later measured DS arena rebudget has a seam and the diff stays reviewable).
 * The adapter is a verbatim pass-through; no behaviour invented here.
 *
 * Score pin: mnPlayers1PGameContinueFuncRun Yes arm does
 * gSCManagerSceneData.spgame_score *= 0.5F (decomp :1075), then rebuilds
 * the score display and plays nSYAudioFGM1PGameContinue (:1078,:1084).
 * No-path (:1087+) makes the room fade + Game Over text, BGM
 * nSYAudioBGM1PGameOver + AnnounceGameOver voice (P2-6 pin sheet rows).
 *
 * The include OWNS every symbol it defines under its source name
 * (unified-owner rule, see src/import/battleship_sc1pgame_runtime.c file doc).
 * No shims, no stubs in this TU.
 *
 * Reloc: dMN1PContinueFileIDs (:37-44) loads llMN1PContinueFileID,
 * llSC1PStageClear2FileID, llIFCommonAnnounceCommonFileID,
 * llIFCommonPlayerDamageFileID, llSC1PStageClear1FileID. All five globals
 * exist under src/ + reloc_data.h externs (MN1PContinue staged 2026-09-04;
 * SC1PStageClear1/2 staged with the tally tables; IFCommon* carried since
 * P2-1). No unstaged file.
 *
 * Shims vs unresolved, see handoff report:
 * - nMN1PContinueOptionYes/No lives in source sc/scdef.h:383-389 and is
 *   absent from port include/sc/scene.h (main-owned need, reported);
 *   nFTDemoStatusFigureDropped/FigureStand (:348,:1079):
 *   include/ft/fighter.h:176. Both checked 2026-09-05.
 * - nSYAudioFGM1PGameContinue / nSYAudioBGM1PGameOver / AnnounceGameOver
 *   ordinals: in include/gm/gmsound.h since the 2026-09-05 widening.
 * - func_800269C0_275C0 voice helper: NOT shimmed or stubbed; resolves via
 *   include/sys/audio.h like the mnmessage TU (link reveals).
 * - ftManagerSetupFilesAllKind (:1210), scSubsysFighterSetStatus,
 *   ovl setup/BSS refs: left unresolved (link reveals).
 * - All other externs (gc/lb/sy/sc/ft/if/reloc): left unresolved, no shims,
 *   no stubs.
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnPlayers1PGameContinueStartScene (adapter below) vs
 *   src/port/title_backend.c:435 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
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

#define mnPlayers1PGameContinueStartScene ndsBaseMNPlayers1PGameContinueStartScene
void ndsBaseMNPlayers1PGameContinueStartScene(void);

/* Exact source header decomp lb/lbcommon.h:11 (matrix list at :1264), same
 * extern form as battleship_mnplayersvs.c:38. The nMN1PContinueOptionYes/No
 * enum lives in source sc/scdef.h:383-389 and is absent from the port (see
 * outstanding-needs report); no enum is added here. */
extern sb32 (*dLBCommonFuncMatrixList[])(void);

/* Landed precedent extern (battleship_mntraining.c:90); called at :1208. */
extern void efManagerInitEffects(void);

/* THE SCREEN ON THE DS (2026-10-04). Until now the Continue screen drew a
 * navy backdrop, an opaque grey cylinder for the spotlight, no fighter, and
 * one NO_PROGRAM failure a frame for its three fade rectangles. The source's
 * order (camera priorities, :771-:960) is: room 90, room fade-in 80,
 * spotlight + light pool 70, spotlight fade 60, the fighter (3D) 50, room
 * fade-out 40, then the text. On the DS the room is the source 2D tenant's BG2
 * background, the fighter is BG0, the spotlight and light pool are
 * semi-transparent OBJs between them (both are I4 discs of coverage 3/15 in
 * the prim colour, nds_source2d.c), and the three fades are the 2D blend:
 * brightness-down on the room (and on the fighter for the fade-out) plus the
 * lights' own BLDALPHA, from the very alphas the source procs step. */
static void ndsMN1PContinueFuncDraw(void);
#define scManagerFuncDraw ndsMN1PContinueFuncDraw

void mnPlayers1PGameContinueRoomFadeOutProcDisplay(GObj *gobj);
void mnPlayers1PGameContinueRoomFadeInProcDisplay(GObj *gobj);
void mnPlayers1PGameContinueSpotlightFadeProcDisplay(GObj *gobj);
void ndsBaseMN1PContinueRoomFadeOutProcDisplay(GObj *gobj);
void ndsBaseMN1PContinueRoomFadeInProcDisplay(GObj *gobj);
void ndsBaseMN1PContinueSpotlightFadeProcDisplay(GObj *gobj);
/* Function-like, so the DEFINITIONS are renamed and the gcAddGObjDisplay
 * references (no parentheses) bind to the wrappers below. */
#define mnPlayers1PGameContinueRoomFadeOutProcDisplay(gobj) \
    ndsBaseMN1PContinueRoomFadeOutProcDisplay(gobj)
#define mnPlayers1PGameContinueRoomFadeInProcDisplay(gobj) \
    ndsBaseMN1PContinueRoomFadeInProcDisplay(gobj)
#define mnPlayers1PGameContinueSpotlightFadeProcDisplay(gobj) \
    ndsBaseMN1PContinueSpotlightFadeProcDisplay(gobj)

#include "../../decomp/BattleShip-main/decomp/src/mn/mn1pmode/mn1pcontinue.c"

#undef mnPlayers1PGameContinueSpotlightFadeProcDisplay
#undef mnPlayers1PGameContinueRoomFadeInProcDisplay
#undef mnPlayers1PGameContinueRoomFadeOutProcDisplay
#undef scManagerFuncDraw
#undef mnPlayers1PGameContinueStartScene

#include <nds/arm9/video.h>
#include <nds/nds_platform.h>
#include <nds/nds_source2d.h>

/* The spotlight and light pool are I4 at 3/15 almost everywhere (26,206 of
 * 26,880 spotlight texels, 4,580 of 5,920 pool texels): the blend's EVA. */
#define NDS_MN1P_CONTINUE_LIGHT_COVERAGE 51u

static u32 sNdsMN1PContinueFadeAlpha[3];
static sb32 sNdsMN1PContinueBackdropSet;

/* Each fade proc runs the source body (its alpha step) and then takes back
 * the fill rectangle it wrote: the blend below presents that alpha. */
#define NDS_MN1P_CONTINUE_FADE_WRAPPER(name, base, slot, alpha)              \
    void name(GObj *gobj)                                                     \
    {                                                                         \
        Gfx *head = gSYTaskmanDLHeads[0];                                     \
                                                                              \
        base(gobj);                                                           \
        gSYTaskmanDLHeads[0] = head;                                          \
        sNdsMN1PContinueFadeAlpha[slot] = (u32)(alpha);                       \
    }
NDS_MN1P_CONTINUE_FADE_WRAPPER(mnPlayers1PGameContinueRoomFadeInProcDisplay,
                               ndsBaseMN1PContinueRoomFadeInProcDisplay, 0,
                               sMN1PContinueRoomFadeInAlpha)
NDS_MN1P_CONTINUE_FADE_WRAPPER(mnPlayers1PGameContinueSpotlightFadeProcDisplay,
                               ndsBaseMN1PContinueSpotlightFadeProcDisplay, 1,
                               sMN1PContinueSpotlightFadeAlpha)
NDS_MN1P_CONTINUE_FADE_WRAPPER(mnPlayers1PGameContinueRoomFadeOutProcDisplay,
                               ndsBaseMN1PContinueRoomFadeOutProcDisplay, 2,
                               sMN1PContinueRoomFadeOutAlpha)
#undef NDS_MN1P_CONTINUE_FADE_WRAPPER

/* The frame draw (dMN1PContinueTaskmanSetup's scManagerFuncDraw): a black
 * backdrop (the default camera's fill, :1205), the 3D fighter through the
 * cameras' (10,10)-(310,230) window, then this frame's blend. */
static void ndsMN1PContinueFuncDraw(void)
{
    u32 room_in;
    u32 spot;
    u32 room_out;
    u32 keep_room;
    u32 keep_light;

    if (sNdsMN1PContinueBackdropSet == FALSE)
    {
        ndsPlatformSetBackdropColor(RGB15(0, 0, 0));
        sNdsMN1PContinueBackdropSet = TRUE;
    }
    if (sMN1PContinueFiles[0] != NULL)
    {
        ndsSource2DSetTranslucentBitmaps(
            lbRelocGetFileData(Sprite*, sMN1PContinueFiles[0],
                               &llMN1PContinueSpotlightSprite)->bitmap,
            lbRelocGetFileData(Sprite*, sMN1PContinueFiles[0],
                               &llMN1PContinueShadowSprite)->bitmap);
    }
    sNdsMN1PContinueFadeAlpha[0] = 0u;
    sNdsMN1PContinueFadeAlpha[1] = 0u;
    sNdsMN1PContinueFadeAlpha[2] = 0u;
    ndsPlatformSet3DLayerEnabled(TRUE);
    ndsPlatformSet3DViewportSource(10, 10, 310, 230);
    scManagerFuncDraw();
    ndsPlatformReset3DViewport();

    /* Source order: the room under all three fades, the lights under the
     * spotlight fade and the fade-out, the fighter under the fade-out only,
     * the text under none. */
    room_in = sNdsMN1PContinueFadeAlpha[0];
    spot = sNdsMN1PContinueFadeAlpha[1];
    room_out = sNdsMN1PContinueFadeAlpha[2];
    keep_light = ((255u - spot) * (255u - room_out) + 127u) / 255u;
    keep_room = ((255u - room_in) * keep_light + 127u) / 255u;
    ndsSource2DSetBlend(
        (u16)(BLEND_FADE_BLACK | BLEND_SRC_BG2 |
              ((room_out != 0u) ? BLEND_SRC_BG0 : 0u) |
              BLEND_DST_BG0 | BLEND_DST_BG2 | BLEND_DST_BACKDROP),
        (u16)((((16u * NDS_MN1P_CONTINUE_LIGHT_COVERAGE * keep_light) +
                (255u * 255u / 2u)) / (255u * 255u)) |
              ((((16u * (255u - NDS_MN1P_CONTINUE_LIGHT_COVERAGE) *
                  keep_room) + (255u * 255u / 2u)) / (255u * 255u)) << 8)),
        (u16)((16u * (255u - keep_room) + 127u) / 255u));
}

void mnPlayers1PGameContinueStartScene(void)
{
    /* The source manager calls Continue directly after a lost battle without
     * changing scene_curr. Select its menu task before the DS dispatcher runs;
     * the source exit and manager retain ownership of the following scene. */
    gSCManagerSceneData.scene_curr = nSCKind1PContinue;

    /* N64 reloads this overlay's BSS for each loss; the DS keeps the TU live.
     * InitVars resets the other decision state, but leaves these fields cold.
     * A retained Yes deadline would otherwise select Yes on the next visit. */
    sMN1PContinueOptionYesRetryTic = 0;
    sMN1PContinueOptionChangeWait = 0;
    sMN1PContinueIsSelectContinue = FALSE;
    sNdsMN1PContinueBackdropSet = FALSE;
    ndsBaseMNPlayers1PGameContinueStartScene();
}

#endif /* NDS_P2_1P_GAME */

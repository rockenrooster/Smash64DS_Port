/* P2-6 step 8. 1P Game VS-style stage intro (portraits + camera animations).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pintro.c whole
 * (2044 lines: dSC1PIntroFileIDs :18, sky/banners/VS decal/labels/figures/
 * stage info, player/ally/VS fighter makers, fighter + stage camera makers,
 * announce, fighter-file setup, FuncStart, taskman setup, sc1PIntroStartScene
 * :2037), following battleship_sc1pstageclear.c (scene TU with its
 * taskman/video setup and start/update/draw functions), NOT a data-only
 * transcription.
 *
 * Unified-owner rule (stated in battleship_sc1pgame_runtime.c, followed
 * here): the include OWNS every symbol it defines under its source name --
 * no renamed private copies, no ndsExcluded* duplicates. The only rename is
 * the scene entry, imported as ndsBase* and re-exported under its source
 * name (the battleship_sc1pbonusstage.c / battleship_scvsbattle.c seam),
 * so a later measured DS arena rebudget has a seam and the diff stays
 * reviewable. The adapter is a verbatim pass-through; no behaviour invented
 * here. Gated on NDS_P2_1P_GAME like the ladder tables and the runtime.
 *
 * Reloc files (dSC1PIntroFileIDs :18):
 * - SC1PIntro 0xb: STAGED (llSC1PIntroFileID in
 *   src/port/diagnostics_mp_taskman_state.c:474, manifest rows in
 *   include/reloc_data.h:1316-1388, asset in src/port/reloc_backend_assets.c,
 *   NitroFS in NDS_1P_RELOC_FILES). Covers the 47 sprites plus all 23
 *   per-fighter/stage camera AnimJoints.
 * - CharacterNames 0xc, BonusPicture 0xd, BonusPicturePlatform 0xe: UNSTAGED.
 *   No llCharacterNamesFileID / llBonusPictureFileID /
 *   llBonusPicturePlatformFileID global exists under src/, and no
 *   CharacterNames/BonusPicture rows exist under include/. Orchestrator:
 *     python scripts/menus/stage_reloc_file.py --file CharacterNames --list NDS_1P_RELOC_FILES
 *     python scripts/menus/stage_reloc_file.py --file BonusPicture --list NDS_1P_RELOC_FILES
 *     python scripts/menus/stage_reloc_file.py --file BonusPicturePlatform --list NDS_1P_RELOC_FILES
 *   Until then the name-sprite table (:527-538), the VS-label table
 *   (:459-472, Link/Pikachu rows), and sc1PIntroMakeBonusPicture (:1045,
 *   :1055, :1065) stay honestly open at link; offsets invented here would be
 *   fabricated data.
 *
 * Fighter-side symbols the intro resolves (all port-provided, no action):
 * ftManagerMakeFighter (battleship_ftmanager.c:94), ftManagerSetupFilesAllKind
 * (battleship_ftmanager.c:80), ftManagerAllocFighter + gFTManagerFigatreeHeapSize
 * (reloc_backend_compat_shims.c:1639), dFTManagerDefaultFighterDesc
 * (include/ft/fighter.h:4158), ftParamGetCostumeCommonID
 * (reloc_backend_compat_shims.c:16271), ftParamInitAllParts
 * (reloc_backend_compat_shims.c:1807), ftParamSetModelPartID
 * (reloc_backend_compat_shims.c:8323), scSubsysFighterSetStatus
 * (reloc_backend_compat_shims.c:16316), ftMainSetStatus
 * (reloc_backend_ftmain_status_compat.c:4488), ftDisplayMainProcDisplay
 * (reloc_backend_fighter_display_seam.c:1), dSCSubsysFighterScales +
 * scSubsysFighterSetLightParams (reloc_backend_fighter_display_seam.c:93,110),
 * gSC1PManagerKirbyTeamModelPartID (owned by the sc1pmanager.c whole-TU include
 * in battleship_sc1pmanager.c under the same flag). Fighter production data
 * already carries every kind the intro instantiates: Mario/Fox/Donkey/Samus/
 * Luigi/Link/Yoshi/Captain/Kirby/Pikachu/Purin/Ness plus the Yoshi/Kirby teams,
 * the Mario Bros pair, Giant DK, Metal Mario, Boss, and the nFTKindNStart..NEnd
 * polygon range (ftchar_data_slots.c slots + reloc_backend_ftdata_symbols.c
 * manifests per the P2-6 inventory); SetupFilesAllKind/AllocFighter size from
 * that manifest, so no new fighter data is staged here.
 *
 * Camera AnimJoints are not sprites: the source pulls each joint with
 * lbRelocGetFileData(AObjEvent32*, sSC1PIntroFiles[0], &llSC1PIntro*CamAnimJoint)
 * (:1178, :1558) and runs the AObj camera path gcAddCObjCamAnimJoint(cobj, joint,
 * 0.0F) + gcPlayCamAnim(gobj). The port already runs source camera AnimJoints:
 * gcAddCObjCamAnimJoint is the whole-TU import in
 * src/import/battleship_sys_objanim.c:20,52,1972, gcPlayCamAnim is externed in
 * src/port/diagnostics_mp_taskman_state.c:546 and observed by
 * src/port/taskman_seam_scene_capture.c:824,993,1233, and the opening movie
 * already drives the same seam (llMVOpeningRoomScene1/2CamAnimJoint,
 * llMVOpeningCommon*CamAnimJoint probed in reloc_backend_assets.c:8466-8501,
 * 8850-8857). No seam is missing; the intro's 23 joints ride it once staged
 * (they are, inside SC1PIntro).
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - FTDemoDesc { fkind; costume; shade }: shimmed below, verbatim from decomp
 *   ft/fttypes.h:717-722 (port include/ft/fighter.h carries FTDesc but not
 *   FTDemoDesc; the intro keeps three FTDemoDesc globals :68-80 and passes them
 *   by value :757). Guarded so a later header promotion collides loudly.
 * - BGM/voice enumerators the TU names but the port headers lack (separate task
 *   widens them -- listed, NOT defined here): nSYAudioBGM1PIntro (:1983),
 *   nSYAudioBGMBossStage (:1981), nSYAudioVoiceAnnounce{Versus,Mario,Fox,Donkey,
 *   Samus,Luigi,Link,Yoshi,Captain,Kirby,Pikachu,Purin,Ness,Link,YoshiTeam,Fox,
 *   MarioBros,Pikachu,GDonkey,KirbyTeam,Samus,MMario,Zako,BreakTheTargets,
 *   BoardThePlatforms,RaceToTheFinish} (:1709-1741, :1787-1795). Their numeric
 *   values can only come from the gm/gmsound.h promotion.
 * - Everything else the TU calls is port-provided, no action:
 *   lbRelocGetFileData / lbRelocInitSetup / lbRelocLoadFilesListed,
 *   gcMakeGObjSPAfter / gcAddGObjDisplay / gcAddGObjProcess / gcMoveGObjDL /
 *   gcMakeCameraGObj / gcMakeDefaultCameraGObj / gcAddXObjForCamera /
 *   gcRunAll / gcPlayAnimAll / gcEndProcessAll, lbCommonMakeSObjForGObj /
 *   lbCommonDrawSObjAttr / lbCommonDrawSprite / lbCommonClearExternSpriteParams,
 *   scSubsysControllerGetPlayerTapButtons / GetPlayerStickInRangeLR/UD
 *   (include/sc/scene.h), syTaskmanMalloc / syTaskmanSetLoadScene /
 *   scManagerFuncUpdate / scManagerFuncDraw, syVideoInit, syRdpSetViewport,
 *   syControllerFuncRead, sySchedulerGetTicCount / sySchedulerSetTicCount,
 *   syAudioPlayBGM (include/sys/audio.h), func_800269C0_275C0
 *   (include/sys/audio.h:92), func_800266A0_272A0 (same basis as the
 *   battleship_mvopening*.c imports), func_80017EC0
 *   (opening_movie_backend.c:4479), efParticleInitAll / efManagerInitEffects,
 *   gSCManagerSceneData / gSCManager1PGameBattleState / gSYTaskmanDLHeads,
 *   lLBRelocTableAddr / llRelocFileCount, dLBCommonFuncMatrixList,
 *   ovl24_BSS_END + ovl1_VRAM (DECLARE_OVL in include/sc/scene.h covers both).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   sc1PIntroStartScene (adapter below) vs
 *   src/port/title_backend.c:471 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <it/item.h>
#include <mn/menu.h>
#include <nds/nds_platform.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_task37_profile.h>
/* LAB: a profile ROM built with NDS_TASK37_PROFILE_START=65000 (a battle
 * frame no run reaches) profiles the 1P intro's first draw instead. */
#if NDS_TASK37_PROFILE && (NDS_TASK37_PROFILE_START == 65000u)
#define NDS_TASK37_PROFILE_INTRO_DRAW 1
#endif
#if NDS_1P_INTRO_BAKE
#include <nds/nds_renderer.h>
#endif
#include <reloc_data.h>
#include <stdio.h>
#if NDS_1P_INTRO_BAKE
#include <nds/arm9/video.h>
#include <nds/arm9/videoGL.h>
#endif
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/controller.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>

#define sc1PIntroStartScene ndsBaseSC1PIntroStartScene
void ndsBaseSC1PIntroStartScene(void);

extern sb32 (*dLBCommonFuncMatrixList[])(void);
extern u8 gSC1PManagerKirbyTeamModelPartID;
void efManagerInitEffects(void);
void sySchedulerSetTicCount(u32 tics);

/* Transient native seam (src/port/renderer_adapter_fighter.c): clears the
 * single scratch binding so a previous Intro visit's arena pointers can
 * never match a new visit's Demo actors. */
extern void ndsFighterIntroTransientReset(void);
static void ndsSC1PIntroDraw(void);

/* The source scene draws through scManagerFuncDraw (its taskman setup's
 * frame draw function). Route that one reference through the DS draw
 * wrapper so the Intro keeps its source cameras, viewports, pose updates,
 * and display order while the 3D layer follows the source live fighter
 * set and the fighter viewport matches the source rect, exactly as the
 * 1P CSS bridge does for its single preview. */
#define scManagerFuncDraw ndsSC1PIntroDraw
#define sc1PIntroFuncStart ndsBaseSC1PIntroFuncStart
#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pintro.c"
#undef sc1PIntroFuncStart
#undef scManagerFuncDraw

#undef sc1PIntroStartScene

/* Pre-stage intros show their fighters as static pictures (owner, 2026-09-24:
 * "pre-stage intros static images"). 0 restores the source's live cards. */
#ifndef NDS_1P_INTRO_BAKE
#define NDS_1P_INTRO_BAKE 0
#endif
#if NDS_1P_INTRO_BAKE
/* LAB: the still bake renders the source's live cards. */
#undef NDS_1P_INTRO_STATIC
#define NDS_1P_INTRO_STATIC 0
#endif
#ifndef NDS_1P_INTRO_STATIC
#define NDS_1P_INTRO_STATIC 1
#endif

#if NDS_1P_INTRO_STATIC
/* sc1pintro.c:1928-2035 (sc1PIntroFuncStart) without the fighters. The
 * source loads the full files of every kind the intro shows and one figatree
 * heap per actor (21 for the Yoshi Team, 13 for the Polygons): Yoshi's Main
 * alone asked 144,640 B against 79,460 B free after the stage-1 fight
 * (artifacts/bugs/2026-10-01_1p-campaign). The cards become static pictures
 * (ndsSC1PIntroMakeStaticFighters); every other part of the scene -- sky,
 * banners, VS decal or bonus picture, labels, figures, stage info, names, the
 * ally line, announcer, BGM, timing and exits -- runs as the source wrote it. */
/* The baked stills (owner 2026-10-01: "Rendered stills"). Each is one card
 * group of the source's live intro -- the player's card, an ally's, or the
 * stage's VS fighters -- rendered by this renderer in the bake ROM
 * (NDS_1P_INTRO_BAKE, scripts/menus/bake_1p_intro_stills.ps1) at the tic the
 * cards settle, display-captured from the 3D layer, cropped and stored as a
 * 256-colour image with its screen box (scripts/menus/pack_1p_intro_stills.py).
 * They are written into the BG3 bitmap, which sits above the sky (BG2) and
 * under the banners, names and VS decal (OBJ), where the source draws its
 * fighter cameras. Which still a card uses is a file name: the card id, kind
 * and costume the source reads (sc1PIntroInitVars), and for the VS fighters
 * the stage, preferring a variant baked for this player's kind and costume
 * (the source recolours an opponent that matches the player). */
#define NDS_SC1P_INTRO_STILL_MAGIC 0x31493153u /* "S1I1" */

typedef struct NDSSC1PIntroStillHeader
{
    u32 magic;
    u16 x;
    u16 y;
    u16 w;
    u16 h;
    u16 tex_w;
    u16 tex_h;
    u16 colors;
    u16 reserved;
} NDSSC1PIntroStillHeader;

__attribute__((used)) volatile u32 gNdsSC1PIntroStillsDrawn;
__attribute__((used)) volatile u32 gNdsSC1PIntroStillsMissing;

static s32 ndsSC1PIntroBlitStill(const char *path, u16 *layer, u32 pitch)
{
    NDSSC1PIntroStillHeader header;
    FILE *file;
    u16 *palette;
    u8 *texels;
    u32 bytes;
    u32 y;
    s32 ok = FALSE;

    ndsFsLock();
    file = fopen(path, "rb");
    if (file == NULL)
    {
        ndsFsUnlock();
        return FALSE;
    }
    if ((fread(&header, 1u, sizeof(header), file) == sizeof(header)) &&
        (header.magic == NDS_SC1P_INTRO_STILL_MAGIC) &&
        (header.colors != 0u) && (header.colors <= 256u) &&
        (header.w <= header.tex_w) && (header.h <= header.tex_h) &&
        ((u32)header.x + header.w <= 256u) &&
        ((u32)header.y + header.h <= 192u))
    {
        bytes = (header.colors * sizeof(u16)) +
                ((u32)header.tex_w * header.tex_h);
        palette = syTaskmanMalloc(bytes, 0x4u);
        if ((palette != NULL) &&
            (fread(palette, 1u, bytes, file) == bytes))
        {
            texels = (u8 *)(palette + header.colors);
            for (y = 0u; y < header.h; y++)
            {
                const u8 *row = texels + (y * header.tex_w);
                u16 *dst = layer + ((header.y + y) * pitch) + header.x;
                u32 x;

                for (x = 0u; x < header.w; x++)
                {
                    if (row[x] != 0u)
                    {
                        dst[x] = palette[row[x]] | 0x8000u;
                    }
                }
            }
            ok = TRUE;
        }
    }
    fclose(file);
    ndsFsUnlock();
    return ok;
}

static char *ndsSC1PIntroPutDigits(char *p, u32 value, u32 count)
{
    char *end = p + count;

    p = end;
    while (count-- != 0u)
    {
        *--p = (char)('0' + (value % 10u));
        value /= 10u;
    }
    return end;
}

/* nitro:/intro/<kind><id>[_<fkind>_<costume>].s1i, the id `digits` wide. */
static void ndsSC1PIntroStillPath(char *out, char kind, u32 id, u32 digits,
                                  s32 fkind, s32 costume)
{
    const char *text = "nitro:/intro/";
    char *p = out;

    while (*text != '\0')
    {
        *p++ = *text++;
    }
    *p++ = kind;
    p = ndsSC1PIntroPutDigits(p, id, digits);
    if (fkind >= 0)
    {
        *p++ = '_';
        p = ndsSC1PIntroPutDigits(p, (u32)fkind, 2u);
        *p++ = '_';
        p = ndsSC1PIntroPutDigits(p, (u32)costume, 1u);
    }
    text = ".s1i";
    while (*text != '\0')
    {
        *p++ = *text++;
    }
    *p = '\0';
}

static void ndsSC1PIntroDrawStill(char kind, u32 card, FTDemoDesc *desc,
                                  u16 *layer, u32 pitch)
{
    char path[48];

    ndsSC1PIntroStillPath(path, kind, card, 1u, desc->fkind, desc->costume);
    if (ndsSC1PIntroBlitStill(path, layer, pitch) != FALSE)
    {
        gNdsSC1PIntroStillsDrawn++;
    }
    else gNdsSC1PIntroStillsMissing++;
}

/* Written at the scene's first draw, not in FuncStart: the source-menu pump
 * clears both overlay layers once before the first draw
 * (ndsSeamRunSourceMenuScene). */
static s32 sNdsSC1PIntroStillsStage = -1;

static void ndsSC1PIntroMakeStaticFighters(s32 stage)
{
    sNdsSC1PIntroStillsStage = stage;
}

static void ndsSC1PIntroBlitStills(void)
{
    s32 stage = sNdsSC1PIntroStillsStage;
    u32 pitch = 0u;
    u16 *layer;
    char path[48];

    if (stage < 0)
    {
        return;
    }
    sNdsSC1PIntroStillsStage = -1;
    layer = ndsPlatformGetOriginalSpriteOverlayLayer(TRUE, &pitch, NULL,
                                                     NULL, NULL);
    if ((layer == NULL) || (pitch == 0u))
    {
        gNdsSC1PIntroStillsMissing++;
        return;
    }
    /* VS fighters first: the cards' cameras draw over them. The VS still for
     * this player's kind and costume when one was baked, else the stage's. */
    ndsSC1PIntroStillPath(path, 'o', (u32)stage, 2u,
                          sSC1PIntroPlayerFighterDemoDesc.fkind,
                          sSC1PIntroPlayerFighterDemoDesc.costume);
    if (ndsSC1PIntroBlitStill(path, layer, pitch) == FALSE)
    {
        ndsSC1PIntroStillPath(path, 'o', (u32)stage, 2u, -1, 0);
        if (ndsSC1PIntroBlitStill(path, layer, pitch) == FALSE)
        {
            gNdsSC1PIntroStillsMissing++;
        }
        else gNdsSC1PIntroStillsDrawn++;
    }
    else gNdsSC1PIntroStillsDrawn++;

    switch (stage)
    {
    case nSC1PGameStageDonkey:
        ndsSC1PIntroDrawStill('a', 5u, &sSC1PIntroAlly2FighterDemoDesc,
                              layer, pitch);
        ndsSC1PIntroDrawStill('a', 4u, &sSC1PIntroAlly1FighterDemoDesc,
                              layer, pitch);
        ndsSC1PIntroDrawStill('p', 3u, &sSC1PIntroPlayerFighterDemoDesc,
                              layer, pitch);
        break;

    case nSC1PGameStageMario:
        ndsSC1PIntroDrawStill('a', 2u, &sSC1PIntroAlly1FighterDemoDesc,
                              layer, pitch);
        ndsSC1PIntroDrawStill('p', 1u, &sSC1PIntroPlayerFighterDemoDesc,
                              layer, pitch);
        break;

    default:
        ndsSC1PIntroDrawStill('p', 0u, &sSC1PIntroPlayerFighterDemoDesc,
                              layer, pitch);
        break;
    }
}

static void ndsSC1PIntroFuncStartStatic(void)
{
    LBRelocSetup rl_setup;

    rl_setup.table_addr = (uintptr_t)&lLBRelocTableAddr;
    rl_setup.table_files_num = (u32)&llRelocFileCount;
    rl_setup.file_heap = NULL;
    rl_setup.file_heap_size = 0;
    rl_setup.status_buffer = sSC1PIntroStatusBuffer;
    rl_setup.status_buffer_size = ARRAY_COUNT(sSC1PIntroStatusBuffer);
    rl_setup.force_status_buffer = sSC1PIntroForceStatusBuffer;
    rl_setup.force_status_buffer_size = ARRAY_COUNT(sSC1PIntroForceStatusBuffer);

    lbRelocInitSetup(&rl_setup);
    lbRelocLoadFilesListed(dSC1PIntroFileIDs, sSC1PIntroFiles);
    gcMakeGObjSPAfter(0, sc1PIntroFuncRun, 0, GOBJ_PRIORITY_DEFAULT);
    gcMakeDefaultCameraGObj(0, GOBJ_PRIORITY_DEFAULT, 100,
                            COBJ_FLAG_FILLCOLOR | COBJ_FLAG_ZBUFFER,
                            GPACK_RGBA8888(0x00, 0x00, 0x00, 0xFF));
    sc1PIntroInitVars();
    efParticleInitAll();
    efManagerInitEffects();
    sc1PIntroMakePicturesCamera();
    sc1PIntroMakeDecalsCamera();
    sc1PIntroMakeBannersCamera();
    sc1PIntroMakeSky();
    sc1PIntroMakeBanners();

    if (sc1PIntroCheckNotBonusStage(sSC1PIntroStage) != FALSE)
    {
        sc1PIntroMakeVSDecal();
    }
    else sc1PIntroMakeBonusPicture(sSC1PIntroStage);

    sc1PIntroMakeLabels(sSC1PIntroStage);
    sc1PIntroMakeFigures(sSC1PIntroStage);
    sc1PIntroMakeStageInfo(sSC1PIntroStage);

    if (sc1PIntroCheckNotBonusStage(sSC1PIntroStage) != FALSE)
    {
        /* sc1PIntroInitFighters makes the ally line with the ally cards. */
        if ((sSC1PIntroStage == nSC1PGameStageDonkey) ||
            (sSC1PIntroStage == nSC1PGameStageMario))
        {
            sc1PIntroMakeAllyText(sSC1PIntroStage);
        }
        ndsSC1PIntroMakeStaticFighters(sSC1PIntroStage);
    }
    if (sSC1PIntroStage == nSC1PGameStageBoss)
    {
        syAudioPlayBGM(0, nSYAudioBGMBossStage);
    }
    else syAudioPlayBGM(0, nSYAudioBGM1PIntro);

    sySchedulerSetTicCount(0);
}
#endif

#if NDS_1P_INTRO_BAKE
/* LAB still bake (scripts/menus/bake_1p_intro_stills.ps1). At tic
 * gNdsIntroBakeTic the 3D layer alone is display-captured into bank D (LCDC,
 * 0x06860000) and ndsIntroBakeCaptured() stops for the debugger, which dumps
 * it; one boot bakes one configuration, poked before the intro. Bit n of
 * gNdsIntroBakeShow keeps the card whose card_anim_frame_id is n (0..5: the
 * player's and allies' cards), bit 6 the VS fighters. The clear is
 * transparent and antialiasing off, so a pixel is a fighter's exactly when its
 * capture alpha bit is set. */
volatile u32 gNdsIntroBakeTic __attribute__((used)) = 240u;
volatile u32 gNdsIntroBakeShow __attribute__((used)) = 0x7fu;
volatile u32 gNdsIntroBakeArmed __attribute__((used));
volatile u32 gNdsIntroBakeCount __attribute__((used));
volatile s32 gNdsIntroBakeMember __attribute__((used)) = -1;
volatile s32 gNdsIntroBakeDepth __attribute__((used));
static GObj *sNdsIntroBakeMemberGObj;
static CObj *sNdsIntroBakeStageCamera;
extern float cosf(float x);
extern float sinf(float x);
extern float sqrtf(float x);

void __attribute__((noinline, used)) ndsIntroBakeCaptured(void)
{
    gNdsIntroBakeCount++;
    __asm__ volatile("" ::: "memory");
}

static void ndsSC1PIntroBakeBeforeDraw(void)
{
    GObj *fighter_gobj;

    glDisable(GL_ANTIALIAS);
    glClearColor(0, 0, 0, 0);
    for (fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
         fighter_gobj != NULL;
         fighter_gobj = fighter_gobj->link_next)
    {
        u32 group = (fighter_gobj->proc_display ==
                     sc1PIntroVSFighterProcDisplay) ? (1u << 6) :
            (1u << (ftGetStruct(fighter_gobj)->card_anim_frame_id & 7));

        if ((gNdsIntroBakeShow & group) == 0u)
        {
            fighter_gobj->flags |= GOBJ_FLAG_HIDDEN;
        }
    }
}

static void ndsSC1PIntroBakeAfterDraw(void)
{
    if (gNdsIntroBakeArmed == 0u)
    {
        if ((u32)sc1PIntroTotalTimeTics == gNdsIntroBakeTic)
        {
            if ((sNdsIntroBakeMemberGObj != NULL) &&
                (sNdsIntroBakeStageCamera != NULL))
            {
                /* The member's distance from the stage camera, x16, for the
                 * far-to-near composite. The pose moves the figure on TransN,
                 * TopN's child (the Demo pose's root motion), turned by
                 * TopN's yaw; the collision helpers' cached joint matrices
                 * are never refreshed in the intro, so this reads the live
                 * DObj transforms instead. */
                DObj *topn = DObjGetStruct(sNdsIntroBakeMemberGObj);
                DObj *transn = (topn != NULL) ? topn->child : NULL;
                Vec3f pos = { 0.0F, 0.0F, 0.0F };
                f32 dx;
                f32 dy;
                f32 dz;

                if (topn != NULL)
                {
                    pos = topn->translate.vec.f;
                    if (transn != NULL)
                    {
                        f32 yaw = topn->rotate.vec.f.y;
                        f32 c = cosf(yaw);
                        f32 sn = sinf(yaw);
                        Vec3f t = transn->translate.vec.f;

                        pos.x += (t.x * c) + (t.z * sn);
                        pos.y += t.y;
                        pos.z += (t.z * c) - (t.x * sn);
                    }
                }
                dx = pos.x - sNdsIntroBakeStageCamera->vec.eye.x;
                dy = pos.y - sNdsIntroBakeStageCamera->vec.eye.y;
                dz = pos.z - sNdsIntroBakeStageCamera->vec.eye.z;
                gNdsIntroBakeDepth =
                    (s32)(sqrtf((dx * dx) + (dy * dy) + (dz * dz)) * 16.0F);
            }
            vramSetBankD(VRAM_D_LCD);
            REG_DISPCAPCNT = DCAP_ENABLE | DCAP_MODE(DCAP_MODE_A) |
                             DCAP_SRC_A(DCAP_SRC_A_3DONLY) |
                             DCAP_SIZE(3) | DCAP_OFFSET(0) | DCAP_BANK(3);
            gNdsIntroBakeArmed = 1u;
        }
        return;
    }
    if ((REG_DISPCAPCNT & DCAP_ENABLE) == 0u)
    {
        gNdsIntroBakeArmed = 0u;
        ndsIntroBakeCaptured();
    }
}

/* Team stages bake one member per boot (gNdsIntroBakeMember >= 0), which is
 * what fits: eighteen Yoshis' figatree heaps, eight Kirbys' hats on the one
 * hat slot an intro fighter has, or ten polygons' files do not fit beside each
 * other in the scene heap. The pack step composites the members far to near
 * by gNdsIntroBakeDepth, the member's distance from the stage camera. -1 is
 * the source's own whole VS set. */

static sb32 ndsSC1PIntroBakeIsTeamMember(s32 stage)
{
    return ((gNdsIntroBakeMember >= 0) &&
            ((stage == nSC1PGameStageYoshi) || (stage == nSC1PGameStageKirby) ||
             (stage == nSC1PGameStageZako))) ? TRUE : FALSE;
}

static void ndsSC1PIntroBakeHeap(s32 index)
{
    if (sSC1PIntroFigatreeHeaps[index] == NULL)
    {
        sSC1PIntroFigatreeHeaps[index] =
            syTaskmanMalloc(gFTManagerFigatreeHeapSize, 0x10);
    }
}

static sb32 ndsSC1PIntroBakeShowsCards(void)
{
    return ((gNdsIntroBakeShow & 0x3fu) != 0u) ? TRUE : FALSE;
}

static sb32 ndsSC1PIntroBakeShowsVS(void)
{
    return ((gNdsIntroBakeShow & (1u << 6)) != 0u) ? TRUE : FALSE;
}

/* sc1PIntroMakeFighter for one card, only when the bake shows it. */
static void ndsSC1PIntroBakeMakeCard(FTDemoDesc fighter, s32 card, s32 heap)
{
    if ((gNdsIntroBakeShow & (1u << card)) == 0u)
    {
        return;
    }
    ndsSC1PIntroBakeHeap(heap);
    sc1PIntroMakeFighterCamera(fighter.fkind, card);
    sc1PIntroMakeFighter(fighter, card, &sSC1PIntroFigatreeHeaps[heap]);
}

/* sc1PIntroSetupFighterFiles, minus the kinds this bake never makes. */
static void ndsSC1PIntroBakeSetupFighterFiles(s32 stage)
{
    if (ndsSC1PIntroBakeShowsVS() == FALSE)
    {
        /* Cards only: the shown cards' kinds (sc1PIntroInitFighters'
         * card numbering) and no VS fighter at all. */
        if ((gNdsIntroBakeShow & ((1u << 0) | (1u << 1) | (1u << 3))) != 0u)
        {
            ftManagerSetupFilesAllKind(sSC1PIntroPlayerFighterDemoDesc.fkind);
        }
        if ((gNdsIntroBakeShow & ((1u << 2) | (1u << 4))) != 0u)
        {
            ftManagerSetupFilesAllKind(sSC1PIntroAlly1FighterDemoDesc.fkind);
        }
        if ((gNdsIntroBakeShow & (1u << 5)) != 0u)
        {
            ftManagerSetupFilesAllKind(sSC1PIntroAlly2FighterDemoDesc.fkind);
        }
        return;
    }
    if (ndsSC1PIntroBakeIsTeamMember(stage) == FALSE)
    {
        sc1PIntroSetupFighterFiles(stage);
        return;
    }
    if (ndsSC1PIntroBakeShowsCards() != FALSE)
    {
        ftManagerSetupFilesAllKind(sSC1PIntroPlayerFighterDemoDesc.fkind);
    }
    switch (stage)
    {
    case nSC1PGameStageYoshi:
        ftManagerSetupFilesAllKind(nFTKindYoshi);
        break;

    case nSC1PGameStageKirby:
        ftManagerSetupFilesAllKind(nFTKindKirby);
        break;

    default:
        ftManagerSetupFilesAllKind(nFTKindNStart + gNdsIntroBakeMember);
        break;
    }
}

/* sc1PIntroInitFighters without its ally line, which reads the 2D files the
 * bake never loads. */
static void ndsSC1PIntroBakeInitFighters(s32 stage)
{
    switch (stage)
    {
    case nSC1PGameStageDonkey:
        ndsSC1PIntroBakeMakeCard(sSC1PIntroAlly2FighterDemoDesc, 5, 2);
        ndsSC1PIntroBakeMakeCard(sSC1PIntroAlly1FighterDemoDesc, 4, 1);
        ndsSC1PIntroBakeMakeCard(sSC1PIntroPlayerFighterDemoDesc, 3, 0);
        break;

    case nSC1PGameStageMario:
        ndsSC1PIntroBakeMakeCard(sSC1PIntroAlly1FighterDemoDesc, 2, 1);
        ndsSC1PIntroBakeMakeCard(sSC1PIntroPlayerFighterDemoDesc, 1, 0);
        break;

    default:
        ndsSC1PIntroBakeMakeCard(sSC1PIntroPlayerFighterDemoDesc, 0, 0);
        break;
    }
}

/* One member of sc1PIntroInitVSFighters' team loops (sc1pintro.c:1257-1306),
 * the member's own iteration of that loop and nothing else. */
static void ndsSC1PIntroBakeInitVSMember(s32 stage)
{
    s32 i = gNdsIntroBakeMember;
    GObj *fighter_gobj;

    sNdsIntroBakeStageCamera = sc1PIntroMakeStageCamera(stage, 32);
    ndsSC1PIntroBakeHeap(i + 1);

    switch (stage)
    {
    case nSC1PGameStageYoshi:
        fighter_gobj = sc1PIntroMakeVSFighter(nFTKindYoshi, stage, i, &sSC1PIntroFigatreeHeaps[i + 1], 32);

        if ((sSC1PIntroPlayerFighterDemoDesc.costume == i % SC1PGAME_STAGE_YOSHI_VARIATIONS_COUNT) && (sSC1PIntroPlayerFighterDemoDesc.fkind == nFTKindYoshi))
        {
            ftParamInitAllParts(fighter_gobj, i % SC1PGAME_STAGE_YOSHI_VARIATIONS_COUNT, 1);
        }
        else ftParamInitAllParts(fighter_gobj, i % SC1PGAME_STAGE_YOSHI_VARIATIONS_COUNT, 0);
        break;

    case nSC1PGameStageKirby:
        fighter_gobj = sc1PIntroMakeVSFighter(nFTKindKirby, stage, i, &sSC1PIntroFigatreeHeaps[i + 1], 32);
        sc1PIntroSetKirbyTeamModelPartIDs(fighter_gobj, stage);
        {
            /* The member's hat is a copy-hat image the battle admits before
             * GO and the intro never loads (ftParamSetModelPartID, unlike the
             * copy path, binds no image); a lone member owns hat slot 0. */
            FTStruct *fp = ftGetStruct(fighter_gobj);
            s32 part = fp->modelpart_status[6 - nFTPartsJointCommonStart].modelpart_id_curr;

            if ((part >= 3) && (part <= 13))
            {
                (void)ndsRendererNativeEnsureKirbyCopyHat(0u, (u32)part, 0u);
            }
        }

        if (sSC1PIntroCheckCostumeUsed(stage, nFTKindKirby, 0) != FALSE)
        {
            ftParamInitAllParts(fighter_gobj, ftParamGetCostumeCommonID(nFTKindMario, 1), 0);
        }
        break;

    default:
        fighter_gobj = sc1PIntroMakeVSFighter(nFTKindNStart + i, stage, 0, &sSC1PIntroFigatreeHeaps[i + 1], 32);
        break;
    }
    sNdsIntroBakeMemberGObj = fighter_gobj;
}

/* sc1PIntroFuncStart for the bake: the capture is the 3D layer alone, so only
 * what draws there is built. Of the intro's four files only SC1PIntro is
 * loaded -- its camera animations frame both the cards and the VS fighters;
 * the bonus pictures and the names are 2D (the four together cost 400 KB of
 * the scene heap, probe zm02). A VS-only bake makes no cards. */
static void ndsSC1PIntroFuncStartBake(void)
{
    LBRelocSetup rl_setup;
    s32 i;

    rl_setup.table_addr = (uintptr_t)&lLBRelocTableAddr;
    rl_setup.table_files_num = (u32)&llRelocFileCount;
    rl_setup.file_heap = NULL;
    rl_setup.file_heap_size = 0;
    rl_setup.status_buffer = sSC1PIntroStatusBuffer;
    rl_setup.status_buffer_size = ARRAY_COUNT(sSC1PIntroStatusBuffer);
    rl_setup.force_status_buffer = sSC1PIntroForceStatusBuffer;
    rl_setup.force_status_buffer_size = ARRAY_COUNT(sSC1PIntroForceStatusBuffer);

    lbRelocInitSetup(&rl_setup);
    lbRelocLoadFilesExtern(dSC1PIntroFileIDs, 1, sSC1PIntroFiles,
                           syTaskmanMalloc(
                               lbRelocGetAllocSize(dSC1PIntroFileIDs, 1),
                               0x10));
    gcMakeGObjSPAfter(0, sc1PIntroFuncRun, 0, GOBJ_PRIORITY_DEFAULT);
    gcMakeDefaultCameraGObj(0, GOBJ_PRIORITY_DEFAULT, 100,
                            COBJ_FLAG_FILLCOLOR | COBJ_FLAG_ZBUFFER,
                            GPACK_RGBA8888(0x00, 0x00, 0x00, 0xFF));
    sc1PIntroInitVars();
    efParticleInitAll();
    efManagerInitEffects();
    ftManagerAllocFighter(FTDATA_FLAG_SUBMOTION,
                          (ndsSC1PIntroBakeIsTeamMember(sSC1PIntroStage) != FALSE) ?
                              2 : sc1PIntroGetFighterAllocsNum(sSC1PIntroStage));
    ndsSC1PIntroBakeSetupFighterFiles(sSC1PIntroStage);

    for (i = 0; i < (s32)ARRAY_COUNT(sSC1PIntroFigatreeHeaps); i++)
    {
        sSC1PIntroFigatreeHeaps[i] = NULL;
    }
    sNdsIntroBakeMemberGObj = NULL;
    sNdsIntroBakeStageCamera = NULL;
    if (sc1PIntroCheckNotBonusStage(sSC1PIntroStage) != FALSE)
    {
        if (ndsSC1PIntroBakeShowsCards() != FALSE)
        {
            ndsSC1PIntroBakeInitFighters(sSC1PIntroStage);
        }
        if (ndsSC1PIntroBakeShowsVS() != FALSE)
        {
            if (ndsSC1PIntroBakeIsTeamMember(sSC1PIntroStage) != FALSE)
            {
                ndsSC1PIntroBakeInitVSMember(sSC1PIntroStage);
            }
            else
            {
                for (i = 0; i < sc1PIntroGetFighterAllocsNum(sSC1PIntroStage);
                     i++)
                {
                    ndsSC1PIntroBakeHeap(i);
                }
                sc1PIntroInitVSFighters(sSC1PIntroStage);
            }
        }
    }
    scSubsysFighterSetLightParams(-20.0F, 30.0F, 0xFF, 0xFF, 0xFF, 0xFF);
    sySchedulerSetTicCount(0);
}
#endif

static void ndsSC1PIntroDraw(void)
{
    GObj *fighter_gobj;
    sb32 visible = FALSE;

#if NDS_1P_INTRO_BAKE
    ndsSC1PIntroBakeBeforeDraw();
#endif
#if NDS_1P_INTRO_STATIC
    ndsSC1PIntroBlitStills();
#endif

    /* Source reveal timing lives in GObj hidden flags (Yoshi/Kirby
     * unhide by tic, Zako is always shown). Mirror the VS preview rule:
     * retained BG0 must follow the live set, not the previous scene. */
    for (fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
         fighter_gobj != NULL;
         fighter_gobj = fighter_gobj->link_next)
    {
        if ((fighter_gobj->flags & GOBJ_FLAG_HIDDEN) == 0u)
        {
            visible = TRUE;
            break;
        }
    }
    ndsPlatformSet3DLayerEnabled(visible);
    /* Every Intro fighter/stage camera uses (10,10)-(310,230) inside the
     * source 320x240 frame; present the same window the VS preview uses. */
    ndsPlatformSet3DViewportSource(10, 10, 310, 230);
#if NDS_TASK37_PROFILE && defined(NDS_TASK37_PROFILE_INTRO_DRAW) && \
    NDS_TASK37_PROFILE_INTRO_DRAW
    /* LAB: the ARM9 profile window is the intro's first draw (~80 VBlanks
     * on 10-02 while its BGM already played). */
    {
        static u32 sNdsIntroDrawProfiled;

        if (sNdsIntroDrawProfiled == 0u)
        {
            ndsTask37ProfileWriteMarker(NDS_TASK37_PROFILE_MARKER_RESET);
        }
        scManagerFuncDraw();
        if (sNdsIntroDrawProfiled == 0u)
        {
            sNdsIntroDrawProfiled = 1u;
            ndsTask37ProfileWriteMarker(NDS_TASK37_PROFILE_MARKER_DUMP);
        }
    }
#else
    scManagerFuncDraw();
#endif
    ndsPlatformReset3DViewport();
#if NDS_1P_INTRO_BAKE
    ndsSC1PIntroBakeAfterDraw();
#endif
}

void sc1PIntroStartScene(void)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    ndsFighterIntroTransientReset();
#endif
#if NDS_1P_INTRO_STATIC
    dSC1PIntroTaskmanSetup.func_start = ndsSC1PIntroFuncStartStatic;
#endif
#if NDS_1P_INTRO_BAKE
    dSC1PIntroTaskmanSetup.func_start = ndsSC1PIntroFuncStartBake;
#endif
    ndsBaseSC1PIntroStartScene();
}

#endif /* NDS_P2_1P_GAME */

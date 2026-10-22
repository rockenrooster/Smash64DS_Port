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
#include <nds/nds_p4.h>
#include <string.h>
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
/* P7 C1: the announcer's name call for the player (sc1PIntroUpdateAnnounce)
 * says a content's or Metal Mario's own name; see ndsSC1PIntroPlayFGM. */
void *ndsSC1PIntroPlayFGM(u16 id);
#define func_800269C0_275C0(id) ndsSC1PIntroPlayFGM(id)
/* P7 C1: the player's name label too (ndsSC1PIntroMakeSObj). */
SObj *ndsSC1PIntroMakeSObj(GObj *gobj, Sprite *sprite);
#define lbCommonMakeSObjForGObj(gobj, sprite) ndsSC1PIntroMakeSObj(gobj, sprite)
#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pintro.c"
#undef lbCommonMakeSObjForGObj
#undef func_800269C0_275C0
#undef sc1PIntroFuncStart
#undef scManagerFuncDraw

/* P7 C1: who the campaign's player is beyond the kind the source reads -- a
 * Remix content rides its parent's kind (nds_p4.c gNdsP4OnePlayerContent),
 * Metal Mario rides Mario's (battleship_ftmanager.c gNdsP7OnePlayerMetal). */
extern u8 gNdsP7OnePlayerMetal;
extern u8 gNdsP4OnePlayerContent;

void *ndsSC1PIntroPlayFGM(u16 id)
{
    /* sc1pintro.c:1708-1722 fighter_voices, the name call the source makes
     * once, before sSC1PIntroIsAnnouncedFighterName goes TRUE. */
    static const u16 kNames[12] = {
        nSYAudioVoiceAnnounceMario, nSYAudioVoiceAnnounceFox,
        nSYAudioVoiceAnnounceDonkey, nSYAudioVoiceAnnounceSamus,
        nSYAudioVoiceAnnounceLuigi, nSYAudioVoiceAnnounceLink,
        nSYAudioVoiceAnnounceYoshi, nSYAudioVoiceAnnounceCaptain,
        nSYAudioVoiceAnnounceKirby, nSYAudioVoiceAnnouncePikachu,
        nSYAudioVoiceAnnouncePurin, nSYAudioVoiceAnnounceNess
    };
    s32 fkind = sSC1PIntroPlayerFighterDemoDesc.fkind;

    if ((sSC1PIntroIsAnnouncedFighterName == FALSE) &&
        (fkind >= 0) && (fkind < 12) && (id == kNames[fkind]))
    {
        const NDSP4Fighter *f = ndsP4Fighter(gNdsP4OnePlayerContent);

        if ((f != NULL) && (f->parent_kind == fkind) &&
            (f->present != NULL) && (f->present->announce_fgm != 0u))
        {
            id = (u16)f->present->announce_fgm;
        }
        else
        if ((gNdsP7OnePlayerMetal != 0u) && (fkind == nFTKindMario))
        {
            id = nSYAudioVoiceAnnounceMMario;
        }
    }
    return func_800269C0_275C0(id);
}

#include "nds_p4_onep_art.generated.h"

/* The player's own name sprite, NULL for the source's: a content's is
 * Remix's 1P name texture (SinglePlayer.asm add_to_single_player), a file of
 * its own (scripts/p4/p4_onep_art.py) read into the scene's heap; Metal
 * Mario's is the intro file's METAL MARIO text, as Remix names him. */
static Sprite *ndsSC1PIntroOwnName(void)
{
    const s32 fkind = sSC1PIntroPlayerFighterDemoDesc.fkind;
    const u32 content = (u32)gNdsP4OnePlayerContent;
    const NDSP4Fighter *f = ndsP4Fighter(content);

    if ((f != NULL) && (f->parent_kind == fkind))
    {
#define NDS_SC1P_INTRO_NAME_ROW(c, file, offset) \
        if (content == (u32)(c)) \
        { \
            const void *id = (const void *)(uintptr_t)(file); \
            void *data = lbRelocGetExternHeapFile( \
                id, syTaskmanMalloc(lbRelocGetFileSize(id), 0x10)); \
            return (data != NULL) ? \
                lbRelocGetFileData(Sprite *, data, (const void *)(uintptr_t)(offset)) : NULL; \
        }
        NDS_P4_INTRO_NAME_ROWS(NDS_SC1P_INTRO_NAME_ROW)
#undef NDS_SC1P_INTRO_NAME_ROW
        return NULL;
    }
    if ((gNdsP7OnePlayerMetal != 0u) && (fkind == nFTKindMario))
    {
        return lbRelocGetFileData(Sprite *, sSC1PIntroFiles[0],
                                  &llSC1PIntroMetalMarioTextSprite);
    }
    return NULL;
}

/* sc1PIntroMakeName (:544) makes the name GObj's first SObj from the
 * player's kind's sprite; for a content or Metal Mario it is their own name
 * instead, before sc1PIntroSetNamePositions lays the row out by its width. */
SObj *ndsSC1PIntroMakeSObj(GObj *gobj, Sprite *sprite)
{
    if ((gobj != NULL) && (gobj == sSC1PIntroNameGObj) &&
        (SObjGetStruct(gobj) == NULL))
    {
        Sprite *own = ndsSC1PIntroOwnName();

        if (own != NULL)
        {
            sprite = own;
        }
    }
    return lbCommonMakeSObjForGObj(gobj, sprite);
}

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
/* A content's or Metal Mario's own card not baked yet (P7 C1). */
__attribute__((used)) volatile u32 gNdsSC1PIntroOwnStillsMissing;

/* How far each still moves outward from where the bake captured it (owner,
 * 2026-10-06: "Rendered stills are still not horizontally offset correctly,
 * they need to be closer to the screen side edges"). The source's card cameras
 * use the viewport 10..310 (sc1pintro.c:1177), which the DS maps to 8..248, and
 * the bake's capture ends there, so every still stopped 8 px short of the
 * edge with sky beyond it. The player's and allies' cards move left, the
 * stage's VS fighters right; what passes the edge is clipped. */
#define NDS_SC1P_INTRO_STILL_EDGE_SHIFT 8

/* A row at a time (2026-10-18): reading the whole texture at once asked the
 * scene heap for colors * 2 + tex_w * tex_h bytes, 66,048 for Captain
 * Falcon's 256x256 player card, 8 KB more than stage 1's intro had left, and
 * the campaign froze there (ndsSyMallocOverflowHalt, walk walk-c1a). The
 * palette and one row now sit on the stack and nothing is allocated. */
static s32 ndsSC1PIntroBlitStill(const char *path, u16 *layer, u32 pitch,
                                 s32 shift)
{
    NDSSC1PIntroStillHeader header;
    FILE *file;
    u16 palette[256];
    u8 row[256];
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
        (header.tex_w <= sizeof(row)) &&
        ((u32)header.x + header.w <= 256u) &&
        ((u32)header.y + header.h <= 192u) &&
        (fread(palette, sizeof(u16), header.colors, file) == header.colors))
    {
        ok = TRUE;
        for (y = 0u; y < header.h; y++)
        {
            u16 *dst = layer + ((header.y + y) * pitch);
            u32 x;

            if (fread(row, 1u, header.tex_w, file) != header.tex_w)
            {
                ok = FALSE;
                break;
            }
            for (x = 0u; x < header.w; x++)
            {
                const s32 sx = (s32)header.x + (s32)x + shift;

                if ((row[x] != 0u) && (sx >= 0) && (sx < 256))
                {
                    dst[sx] = palette[row[x]] | 0x8000u;
                }
            }
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

    /* P7 C1: the player's card for a content (p<card>_c<NN>_<costume>) or
     * for Metal Mario (kind 13), baked as the source kinds' are; until one
     * is, the card the source kind shows. */
    if ((kind == 'p') && (desc == &sSC1PIntroPlayerFighterDemoDesc))
    {
        sb32 own = FALSE;

        const NDSP4Fighter *f = ndsP4Fighter(gNdsP4OnePlayerContent);

        if ((f != NULL) && (f->parent_kind == desc->fkind))
        {
            /* p<card>_<NN>_<C> with the content's number, then the 'c'. */
            u32 at = sizeof("nitro:/intro/p0_") - 1u;

            ndsSC1PIntroStillPath(path, kind, card, 1u,
                                  (s32)gNdsP4OnePlayerContent, desc->costume);
            memmove(&path[at + 1u], &path[at], strlen(&path[at]) + 1u);
            path[at] = 'c';
            own = TRUE;
        }
        else
        if ((gNdsP7OnePlayerMetal != 0u) && (desc->fkind == nFTKindMario))
        {
            ndsSC1PIntroStillPath(path, kind, card, 1u, nFTKindMMario, 0);
            own = TRUE;
        }
        if (own != FALSE)
        {
            if (ndsSC1PIntroBlitStill(path, layer, pitch,
                                      -NDS_SC1P_INTRO_STILL_EDGE_SHIFT) != FALSE)
            {
                NDS_DIAG(gNdsSC1PIntroStillsDrawn++);
                return;
            }
            gNdsSC1PIntroOwnStillsMissing++;
        }
    }
    ndsSC1PIntroStillPath(path, kind, card, 1u, desc->fkind, desc->costume);
    if (ndsSC1PIntroBlitStill(path, layer, pitch,
                              -NDS_SC1P_INTRO_STILL_EDGE_SHIFT) != FALSE)
    {
        NDS_DIAG(gNdsSC1PIntroStillsDrawn++);
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
        NDS_DIAG(gNdsSC1PIntroStillsMissing++);
        return;
    }
    /* VS fighters first: the cards' cameras draw over them. The VS still for
     * this player's kind and costume when one was baked, else the stage's. */
    ndsSC1PIntroStillPath(path, 'o', (u32)stage, 2u,
                          sSC1PIntroPlayerFighterDemoDesc.fkind,
                          sSC1PIntroPlayerFighterDemoDesc.costume);
    if (ndsSC1PIntroBlitStill(path, layer, pitch,
                              NDS_SC1P_INTRO_STILL_EDGE_SHIFT) == FALSE)
    {
        ndsSC1PIntroStillPath(path, 'o', (u32)stage, 2u, -1, 0);
        if (ndsSC1PIntroBlitStill(path, layer, pitch,
                                  NDS_SC1P_INTRO_STILL_EDGE_SHIFT) == FALSE)
        {
            NDS_DIAG(gNdsSC1PIntroStillsMissing++);
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
/* P7 C1: nonzero while the player's card is made, which a content bake
 * makes as the campaign player's content (nds_p4.c ndsP4MatchContent):
 * every intro fighter is made as player 0. */
volatile u32 gNdsIntroBakePlayerCard __attribute__((used));
/* P7 C1 (owner 2026-10-19): an authored content's congratulations still is
 * its player card in a studio framing -- the player card's camera aimed at
 * the fighter's root plus gNdsIntroBakeStudioY, from gNdsIntroBakeStudioDist
 * away and gNdsIntroBakeStudioYaw degrees round from the front (0: the card's
 * own camera). scripts/p6/p6_congra_plates.py makes the plates from it. */
volatile s32 gNdsIntroBakeStudioDist __attribute__((used));
volatile s32 gNdsIntroBakeStudioY __attribute__((used));
volatile s32 gNdsIntroBakeStudioYaw __attribute__((used));
/* The still's pose, studio or card: 0 keeps the card's, otherwise the demo
 * status the player's (or an ally's) card fighter takes at its first drawn
 * frame (nFTDemoStatusWin1.. the Results victory poses), captured at the
 * bake's tic. */
volatile s32 gNdsIntroBakeStudioStatus __attribute__((used));
static GObj *sNdsIntroBakeMemberGObj;
static CObj *sNdsIntroBakeStageCamera;
extern float cosf(float x);
extern float sinf(float x);
extern float sqrtf(float x);

void __attribute__((noinline, used)) ndsIntroBakeCaptured(void)
{
    NDS_DIAG(gNdsIntroBakeCount++);
    __asm__ volatile("" ::: "memory");
}

/* The player card's camera (display link 31, sc1PIntroMakeFighterCamera)
 * aimed at the card's fighter for the studio still. */
static void ndsSC1PIntroBakeStudio(GObj *fighter_gobj)
{
    DObj *topn = DObjGetStruct(fighter_gobj);
    DObj *transn = (topn != NULL) ? topn->child : NULL;
    GObj *camera_gobj;
    Vec3f pos;
    f32 yaw = (f32)gNdsIntroBakeStudioYaw * (3.14159265F / 180.0F);
    f32 dist = (f32)gNdsIntroBakeStudioDist;

    if (topn == NULL)
    {
        return;
    }
    pos = topn->translate.vec.f;
    if (transn != NULL)
    {
        f32 c = cosf(topn->rotate.vec.f.y);
        f32 sn = sinf(topn->rotate.vec.f.y);
        Vec3f t = transn->translate.vec.f;

        pos.x += (t.x * c) + (t.z * sn);
        pos.y += t.y;
        pos.z += (t.z * c) - (t.x * sn);
    }
    pos.y += (f32)gNdsIntroBakeStudioY;
    for (camera_gobj = gGCCommonLinks[16]; camera_gobj != NULL;
         camera_gobj = camera_gobj->link_next)
    {
        CObj *cobj;

        if ((camera_gobj->camera_mask & COBJ_MASK_DLLINK(31)) == 0u)
        {
            continue;
        }
        cobj = CObjGetStruct(camera_gobj);
        cobj->vec.at.x = pos.x;
        cobj->vec.at.y = pos.y;
        cobj->vec.at.z = pos.z;
        cobj->vec.eye.x = pos.x + (dist * sinf(yaw));
        cobj->vec.eye.y = pos.y;
        cobj->vec.eye.z = pos.z + (dist * cosf(yaw));
        cobj->vec.up.x = 0.0F;
        cobj->vec.up.y = 1.0F;
        cobj->vec.up.z = 0.0F;
    }
}

/* The intro's fighter cameras load their view into the modelview (XObj kinds
 * 7 and 15, sc1PIntroMakeFighterCamera / the stage cameras), which the lean
 * fighter path never takes (renderer_fighter_lean.c, the camera decline:
 * every card was skipped and the capture held no fighter). The bake moves
 * the view to the projection (the even form of the same kind) before each
 * draw: the same clip matrix, the battle cameras' convention. The XObj's
 * padding byte marks the fold, so the fighter's light, which the source
 * gives in view space under such a camera, is turned into world space
 * (ndsSC1PIntroBakeLightToWorld). */
#define NDS_SC1P_INTRO_BAKE_FOLD_TAG 0xB7u
#define NDS_SC1P_INTRO_BAKE_CARD_FAR 65536.0F
/* renderer_adapter_fighter.c: the light capture's hook, set around the
 * bake's draw (ndsSC1PIntroBakeBeforeDraw / AfterDraw). */
extern void (*gNdsFighterDisplayLightHook)(Light *light);
void ndsSC1PIntroBakeLightToWorld(Light *light);
/* renderer_adapter_matrix.c: the texgen LookAt's hook, set alike. */
extern void (*gNdsAdapterLookAtHook)(LookAt *look_at);
void ndsSC1PIntroBakeLookAtToWorld(LookAt *look_at);
/* decomp sys/matrix.h */
void syMatrixLookAtF(Mtx44f *mf, f32 eye_x, f32 eye_y, f32 eye_z, f32 at_x,
                     f32 at_y, f32 at_z, f32 up_x, f32 up_y, f32 up_z);
void syMatrixModLookAtF(Mtx44f *mf, f32 eye_x, f32 eye_y, f32 eye_z,
                        f32 at_x, f32 at_y, f32 at_z, f32 roll, f32 up_x,
                        f32 up_y, f32 up_z);

static void ndsSC1PIntroBakeFoldCameraViews(void)
{
    GObj *camera_gobj;

    for (camera_gobj = gGCCommonLinks[16]; camera_gobj != NULL;
         camera_gobj = camera_gobj->link_next)
    {
        CObj *cobj = CObjGetStruct(camera_gobj);
        s32 i;

        if (cobj == NULL)
        {
            continue;
        }
        /* The N64's microcode does not clip at the far plane (the depth
         * saturates); the DS's does. Giant DK's farthest ally stands some
         * 16,300 units from its card camera (sc1PIntroGetFighterCObjDesc,
         * card 5) against the source's far of 16,384, and the bake kept only
         * his head (owner 2026-10-20: "only shows head of fartherest fighter
         * which was luigi"). A card shows one fighter, so a farther plane
         * costs nothing. */
        if (((camera_gobj->camera_mask &
              (COBJ_MASK_DLLINK(29) | COBJ_MASK_DLLINK(30) |
               COBJ_MASK_DLLINK(31))) != 0u) &&
            (cobj->projection.persp.far < NDS_SC1P_INTRO_BAKE_CARD_FAR))
        {
            cobj->projection.persp.far = NDS_SC1P_INTRO_BAKE_CARD_FAR;
        }
        for (i = 0; i < cobj->xobjs_num; i++)
        {
            XObj *xobj = cobj->xobjs[i];

            if ((xobj != NULL) && (xobj->kind >= 7) && (xobj->kind <= 17) &&
                ((xobj->kind & 1) != 0))
            {
                xobj->kind--;
                xobj->unk05 = NDS_SC1P_INTRO_BAKE_FOLD_TAG;
            }
        }
    }
}

/* Under a folded camera (above), a direction the source gives in view space
 * -- the fighter's light, the texgen LookAt -- into world space: the view the
 * camera's own kind builds (objdisplay.c's LookAt / ModLookAt forms,
 * renderer_adapter_matrix.c), its rotation applied transposed. FALSE when the
 * current camera was not folded. */
static sb32 ndsSC1PIntroBakeDirToWorld(s8 dir[3])
{
    CObj *cobj = (gGCCurrentCamera != NULL) ?
        CObjGetStruct(gGCCurrentCamera) : NULL;
    Mtx44f mf;
    u32 kind = 0u;
    f32 world[3];
    f32 len;
    s32 i;

    if (cobj == NULL)
    {
        return FALSE;
    }
    for (i = 0; i < cobj->xobjs_num; i++)
    {
        XObj *xobj = cobj->xobjs[i];

        if ((xobj != NULL) && (xobj->unk05 == NDS_SC1P_INTRO_BAKE_FOLD_TAG))
        {
            kind = (u32)xobj->kind | 1u;
        }
    }
    if (kind == 0u)
    {
        return FALSE;
    }
    if ((kind == 7u) || (kind == 13u))
    {
        syMatrixLookAtF(&mf, cobj->vec.eye.x, cobj->vec.eye.y,
                        cobj->vec.eye.z, cobj->vec.at.x, cobj->vec.at.y,
                        cobj->vec.at.z, cobj->vec.up.x, cobj->vec.up.y,
                        cobj->vec.up.z);
    }
    else
    {
        const sb32 y_up = ((kind == 9u) || (kind == 15u)) ? TRUE : FALSE;

        syMatrixModLookAtF(&mf, cobj->vec.eye.x, cobj->vec.eye.y,
                           cobj->vec.eye.z, cobj->vec.at.x, cobj->vec.at.y,
                           cobj->vec.at.z, cobj->vec.up.x, 0.0F,
                           (y_up != FALSE) ? 1.0F : 0.0F,
                           (y_up != FALSE) ? 0.0F : 1.0F);
    }
    for (i = 0; i < 3; i++)
    {
        world[i] = ((f32)dir[0] * mf[i][0]) + ((f32)dir[1] * mf[i][1]) +
                   ((f32)dir[2] * mf[i][2]);
    }
    len = sqrtf((world[0] * world[0]) + (world[1] * world[1]) +
                (world[2] * world[2]));
    if (len < 1.0F)
    {
        return FALSE;
    }
    for (i = 0; i < 3; i++)
    {
        f32 v = (world[i] * 127.0F) / len;

        dir[i] = (s8)((v < 0.0F) ? (v - 0.5F) : (v + 0.5F));
    }
    return TRUE;
}

/* The display contract's light capture (renderer_adapter_fighter.c). */
void ndsSC1PIntroBakeLightToWorld(Light *light)
{
    if (light != NULL)
    {
        (void)ndsSC1PIntroBakeDirToWorld(light->l.dir);
    }
}

/* The texgen LookAt (renderer_adapter_matrix.c
 * ndsRendererAdapterCurrentLookAt): Metal Mario's chrome read the camera-space
 * reset directions against world normals and drew near black. */
void ndsSC1PIntroBakeLookAtToWorld(LookAt *look_at)
{
    if (look_at != NULL)
    {
        (void)ndsSC1PIntroBakeDirToWorld(look_at->l[0].l.dir);
        (void)ndsSC1PIntroBakeDirToWorld(look_at->l[1].l.dir);
    }
}

static void ndsSC1PIntroBakeBeforeDraw(void)
{
    GObj *fighter_gobj;

    glDisable(GL_ANTIALIAS);
    glClearColor(0, 0, 0, 0);
    ndsSC1PIntroBakeFoldCameraViews();
    gNdsFighterDisplayLightHook = ndsSC1PIntroBakeLightToWorld;
    gNdsAdapterLookAtHook = ndsSC1PIntroBakeLookAtToWorld;
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
        else if (group & ((1u << 0) | (1u << 1) | (1u << 3)))
        {
            /* The pose applies to a card in its own framing too (owner
             * 2026-10-19: Palutena's card took her base's intro pose, the
             * staff through her head; her cards take a victory end pose). */
            if ((gNdsIntroBakeStudioStatus != 0) &&
                (ftGetStruct(fighter_gobj)->status_id !=
                 gNdsIntroBakeStudioStatus))
            {
                scSubsysFighterSetStatus(fighter_gobj,
                                         gNdsIntroBakeStudioStatus);
            }
            if (gNdsIntroBakeStudioDist > 0)
            {
                ndsSC1PIntroBakeStudio(fighter_gobj);
            }
        }
    }
}

static void ndsSC1PIntroBakeAfterDraw(void)
{
    gNdsFighterDisplayLightHook = NULL;
    gNdsAdapterLookAtHook = NULL;
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
    gNdsIntroBakePlayerCard = ((card == 0) || (card == 1) || (card == 3)) ? 1u : 0u;
    sc1PIntroMakeFighter(fighter, card, &sSC1PIntroFigatreeHeaps[heap]);
    gNdsIntroBakePlayerCard = 0u;
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
            /* A content's own files with its parent's (P7 C1). */
            gNdsIntroBakePlayerCard = 1u;
            ftManagerSetupFilesAllKind(sSC1PIntroPlayerFighterDemoDesc.fkind);
            gNdsIntroBakePlayerCard = 0u;
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
    ndsFighterIntroTransientReset();
#if NDS_1P_INTRO_STATIC
    dSC1PIntroTaskmanSetup.func_start = ndsSC1PIntroFuncStartStatic;
#endif
#if NDS_1P_INTRO_BAKE
    dSC1PIntroTaskmanSetup.func_start = ndsSC1PIntroFuncStartBake;
#endif
    ndsBaseSC1PIntroStartScene();
}

#endif /* NDS_P2_1P_GAME */

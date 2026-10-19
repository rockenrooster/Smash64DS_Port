#ifndef NDS_P5_H
#define NDS_P5_H

/* P5 (docs/P5/New_Stages.md): the new VS stages. scripts/p5/stages.json is the
 * registry -- one row per stage with the port's own decisions -- and
 * scripts/p5/p5_stages.py --header writes it as NDS_P5_STAGE_ROWS into the
 * build tree (nds_p5_stages.generated.h). The three vanilla 1P arenas keep
 * their source ground kinds (Meta Crystal 13, Duel Zone 14, Final Destination
 * 16); Remix's stages take the kinds after the source's last ground
 * (nGRKindBonus2Ness, 40): First Destination 41, Bowser's Stadium 42, Hyrule
 * Temple 43, Dinosaur Land 44. */

#include <PR/ultratypes.h>

#if NDS_P5
#include "nds_p5_stages.generated.h"

/* TRUE when the select offers this P5 stage: it has a registry row and it has
 * landed. P7 adds the save's unlock to this predicate. */
static inline u32 ndsP5StageAdmitted(u32 gkind)
{
#define NDS_P5_ADMITTED_ROW(g, token, unlock, landed, inishie, none, b0, b1, frame) \
    if (gkind == (u32)(g)) { return (landed) ? TRUE : FALSE; }
    NDS_P5_STAGE_ROWS(NDS_P5_ADMITTED_ROW)
#undef NDS_P5_ADMITTED_ROW
    return FALSE;
}

/* The stage's P7 unlock id (stages.json "unlock_id"), or -1. */
static inline s32 ndsP5StageUnlockId(u32 gkind)
{
#define NDS_P5_UNLOCK_ROW(g, token, unlock, landed, inishie, none, b0, b1, frame) \
    if (gkind == (u32)(g)) { return (s32)(unlock); }
    NDS_P5_STAGE_ROWS(NDS_P5_UNLOCK_ROW)
#undef NDS_P5_UNLOCK_ROW
    return -1;
}

/* P7 C2: a content's own bonus board (stages.json `bonus`): 1 Break the
 * Targets, 2 Board the Platforms; 0 for every other kind. */
static inline u32 ndsP5BonusClass(u32 gkind)
{
#define NDS_P5_BONUS_CLASS_ROW(g, token, cls)     if (gkind == (u32)(g)) { return (u32)(cls); }
    NDS_P5_BONUS_ROWS(NDS_P5_BONUS_CLASS_ROW)
#undef NDS_P5_BONUS_CLASS_ROW
    return 0u;
}

/* A Remix VS stage (as opposed to a vanilla 1P arena or a content's bonus
 * board): its kind is past the source's last ground, nGRKindBonus2Ness (40). */
static inline u32 ndsP5StageIsRemix(u32 gkind)
{
    return ((gkind > 40u) && (gkind <= NDS_P5_GKIND_MAX) &&
            (ndsP5BonusClass(gkind) == 0u)) ? TRUE : FALSE;
}

/* Remix Camera.asm: the stages whose row routes the match camera to the
 * Mushroom Kingdom status (Hyrule Temple, Dinosaur Land). */
static inline u32 ndsP5StageCameraInishie(u32 gkind)
{
#define NDS_P5_CAMERA_ROW(g, token, unlock, landed, inishie, none, b0, b1, frame) \
    if (gkind == (u32)(g)) { return (inishie) ? TRUE : FALSE; }
    NDS_P5_STAGE_ROWS(NDS_P5_CAMERA_ROW)
#undef NDS_P5_CAMERA_ROW
    return FALSE;
}

/* Remix FD.asm music_fix_: a stage whose row names a second song switches to
 * it at exactly `frame` updates after its scene loaded, if the first is still
 * the stage's song (Final Destination: Master Hand's entrance, 24, then Final
 * Destination, 25, at 840). Returns the song, or -1. */
static inline s32 ndsP5StageSongSwitch(u32 gkind, u32 frame, u32 bgm_default)
{
#define NDS_P5_SONG_ROW(g, token, unlock, landed, inishie, none, b0, b1, at) \
    if ((gkind == (u32)(g)) && ((at) != 0) && (frame == (u32)(at)) && \
        ((s32)bgm_default == (b0))) { return (b1); }
    NDS_P5_STAGE_ROWS(NDS_P5_SONG_ROW)
#undef NDS_P5_SONG_ROW
    return -1;
}
#endif

/* Which bonus board a kind is: 1 Break the Targets, 2 Board the Platforms
 * (the source's nGRKindBonus1Mario..Ness 17-28 and nGRKindBonus2Mario..Ness
 * 29-40, and P7 C2's content boards), 0 for every other kind. */
static inline u32 ndsGrBonusClass(s32 gkind)
{
    if ((gkind >= 17) && (gkind <= 28))
    {
        return 1u;
    }
    if ((gkind >= 29) && (gkind <= 40))
    {
        return 2u;
    }
#if NDS_P5
    return ndsP5BonusClass((u32)gkind);
#else
    return 0u;
#endif
}

#endif

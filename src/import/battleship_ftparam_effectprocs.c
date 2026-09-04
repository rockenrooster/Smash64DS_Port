/*
 * Strong ftParam pause/resume effect procs, verbatim from BattleShip
 * decomp/BattleShip-main/decomp/src/ft/ftparam.c:1355-1423.
 *
 * Effect: ftParamProcPauseEffect / ftParamProcResumeEffect walk the effect
 * link (gGCCommonLinks[nGCCommonLinkIDEffect]) for effects attached to the
 * fighter (ep->fighter_gobj match, gated on fp->is_effect_attach) and set /
 * clear ep->is_pause_effect on each, freezing attached visuals during hitstop.
 *
 * Callers: smash attack proc_lagstart / proc_lagend
 * (decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattacks4.c:119-120), Captain Falcon specials
 * (ftcaptainspecialn.c:105,118,159,172) and Kirby specials
 * (ftkirbyspecialhi.c:239,267,279,293,306).
 *
 * Why missing: the only definitions in the build were weak no-op stubs at
 * src/import/battleship_ftcommon_normal_moveset.c:37,42 (W in nm for both
 * ELFs), so attached effects never paused. ftParamRunProcEffect had no port
 * definition either (only a comment mention in battleship_efmanager.c), so
 * this TU carries the source-exact runner plus the four pause/resume
 * functions and nothing else.
 *
 * Chose verbatim transcription over a textual include of the decomp region
 * the way battleship_ftpublic.c:151 includes a whole .c file, because the
 * region also holds ftParamStopEffect / ftParamProcStopEffect, which already
 * have a strong port definition (reloc_backend_compat_shims.c:2825) -- the
 * include would drag in unrelated duplicate symbols.
 */
#include <PR/ultratypes.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <sys/obj.h>

// 0x800E9B64
void ftParamRunProcEffect(GObj *fighter_gobj, void (*proc)(GObj*, EFStruct*))
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->is_effect_attach)
    {
        GObj *effect_gobj = gGCCommonLinks[nGCCommonLinkIDEffect];

        while (effect_gobj != NULL)
        {
            EFStruct *ep = efGetStruct(effect_gobj);

            GObj *next_effect_gobj = effect_gobj->link_next;

            if ((ep != NULL) && (fighter_gobj == ep->fighter_gobj))
            {
                proc(effect_gobj, ep);
            }
            effect_gobj = next_effect_gobj;
        }
    }
}

// 0x800E9C78
void ftParamPauseEffect(GObj *effect_gobj, EFStruct *ep)
{
    ep->is_pause_effect = TRUE;
}

// 0x800E9C78
void ftParamProcPauseEffect(GObj *effect_gobj)
{
    ftParamRunProcEffect(effect_gobj, ftParamPauseEffect);
}

// 0x800E9CB0
void ftParamResumeEffect(GObj *effect_gobj, EFStruct *ep)
{
    ep->is_pause_effect = FALSE;
}

// 0x800E9CC4
void ftParamProcResumeEffect(GObj *fighter_gobj)
{
    ftParamRunProcEffect(fighter_gobj, ftParamResumeEffect);
}

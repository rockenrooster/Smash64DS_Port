/* P2-3 Kirby runtime state machine: BattleShip specials verbatim.
 *
 * Inhale (the vacuum, the two-body swallow, spit-as-star and Copy), Final
 * Cutter, Stone and his forward throw keep their source status bodies as the
 * behavioral authority. The copied neutral specials are in
 * battleship_kirby_copy.c, the Final Cutter beam in battleship_kirby_weapons.c
 * and the victim half of Inhale in battleship_ftcommon_capturekirby.c. */
#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <reloc_data.h>
#include <sys/audio.h>
#include <sys/develop.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#include "battleship_kirby_common.h"

#include <nds/nds_p4_contents.h>
#if NDS_P4
#include <nds/nds_p4.h>
/* P4: a content's inhale runs this file on its own statuses (Remix's
 * DededeNSP hooks patch each status change here for Dedede's id): every
 * status this file sets goes through the content's kirby_inhale_status. The
 * copy table is read from the port's copy of Kirby's rows, his file or not
 * in the match (the original fighters' rows are the same). */
#define ftMainSetStatus(g_, s_, f_, a_, fl_) \
    ftMainSetStatus((g_), ndsP4KirbyInhaleStatus((g_), (s_)), (f_), (a_), (fl_))
#define gFTDataKirbyMainMotion ndsP4KirbyCopyFile()
#if NDS_P2_KIRBY
/* S8: a content's power travels with copy_id (FTStruct.nds_p4_kirby_copy).
 * The catch records it where it has just chosen the copy_id (its eat
 * sound), the copy takes it (ndsP4KirbyCopyInitCopyVars, below, around the
 * source's), and the loss of the copy clears it (its sound). The definition
 * keeps the source body (first argument's type); both call sites pass
 * fighter_gobj. */
void battleship_ftKirbySpecialNCopyInitCopyVars(GObj *fighter_gobj);
#define ftKirbySpecialNCopyInitCopyVars(a_) NDS_P4_KIRBY_COPYINIT_##a_)
#define NDS_P4_KIRBY_COPYINIT_GObj battleship_ftKirbySpecialNCopyInitCopyVars(GObj
#define NDS_P4_KIRBY_COPYINIT_fighter_gobj ndsP4KirbyCopyInitCopyVars(fighter_gobj
#define func_800269C0_275C0(fgm_) NDS_P4_KIRBY_FGM_##fgm_
#define NDS_P4_KIRBY_FGM_nSYAudioFGMKirbySpecialNCopyEat \
    (ndsP4KirbyCatch(kirby_fp, victim_fp), \
     func_800269C0_275C0(nSYAudioFGMKirbySpecialNCopyEat))
#define NDS_P4_KIRBY_FGM_nSYAudioFGMKirbySpecialNLoseCopy \
    (ndsP4KirbyLoseCopy(fp), func_800269C0_275C0(nSYAudioFGMKirbySpecialNLoseCopy))
#define NDS_P4_KIRBY_FGM_nSYAudioFGMKirbySpecialNCopyUnk \
    func_800269C0_275C0(nSYAudioFGMKirbySpecialNCopyUnk)
#define NDS_P4_KIRBY_FGM_nSYAudioFGMKirbySpecialNCopyThrow \
    func_800269C0_275C0(nSYAudioFGMKirbySpecialNCopyThrow)
#endif
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbyspecialn.c"
#if NDS_P4
#undef ftMainSetStatus
#undef gFTDataKirbyMainMotion
#if NDS_P2_KIRBY
#undef ftKirbySpecialNCopyInitCopyVars
#undef func_800269C0_275C0

/* ftKirbySpecialNCopyInitCopyVars with the power taken alongside copy_id.
 * The source keeps a copy whose copy_id it already holds: a content's power
 * names its parent's kind, so a Kirby with Fox's blaster who swallows Falco
 * must still take the power (Remix's ids differ there). The power is in
 * place before the source's take path runs as written, so the parent's copy
 * row it passes ftParamSetModelPartDefaultID draws nothing; the power's hat
 * (its Remix hat, drawn in its source hat's place, S8) follows it. */
static void ndsP4KirbyCopyInitCopyVarsPower(GObj *fighter_gobj);

void ndsP4KirbyCopyInitCopyVars(GObj *fighter_gobj)
{
#if NDS_P2_KIRBY && NDS_P2_MMARIO
    /* A Kirby taking a copy also takes or drops Metal Mario's metal
     * (nds_p4.c ndsKirbyMetalTake), whether or not the copy_id moves. */
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 take = ((fp->fkind == nFTKindKirby) &&
                 (fp->motion_vars.flags.flag1 != 0)) ? TRUE : FALSE;

    ndsP4KirbyCopyInitCopyVarsPower(fighter_gobj);
    if (take != FALSE)
    {
        ndsKirbyMetalTake(fp);
    }
#else
    ndsP4KirbyCopyInitCopyVarsPower(fighter_gobj);
#endif
}

static void ndsP4KirbyCopyInitCopyVarsPower(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 power = ndsP4KirbyPendingPower(fp);
    const NDSP4KirbyPower *p = ndsP4KirbyPowerOf(power);
    sb32 take;

    if ((fp->fkind != nFTKindKirby) || (fp->motion_vars.flags.flag1 == 0) ||
        ((power == 0u) && (fp->nds_p4_kirby_copy == 0u)))
    {
        battleship_ftKirbySpecialNCopyInitCopyVars(fighter_gobj);
        return;
    }
    take = ((fp->passive_vars.kirby.copy_id != fp->status_vars.kirby.specialn.copy_id) ||
            (fp->nds_p4_kirby_copy != power)) ? TRUE : FALSE;
    if ((take != FALSE) &&
        (fp->passive_vars.kirby.copy_id == fp->status_vars.kirby.specialn.copy_id))
    {
        fp->passive_vars.kirby.copy_id = nFTKindKirby;
    }
    if (take != FALSE)
    {
        ndsP4KirbyClearSilhouette(fp);
        fp->nds_p4_kirby_copy = (p != NULL) ? (u8)power : 0u;
        fp->nds_p4_kirby_hat = 0u;
    }
    battleship_ftKirbySpecialNCopyInitCopyVars(fighter_gobj);
    if ((take != FALSE) && (p != NULL))
    {
        ftParamSetModelPartDefaultID(fighter_gobj, FTKIRBY_COPY_MODELPARTS_JOINT,
                                     p->hat_base);
        ftParamResetModelPartAll(fighter_gobj);
    }
}
#endif
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbyspecialhi.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbyspeciallw.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbythrowf.c"

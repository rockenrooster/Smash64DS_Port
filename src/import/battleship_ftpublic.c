/* BUGS.md, crowd row: the reactive audience.
 *
 * Everything this row called "structurally absent" was one translation unit.
 * `ft/ftpublic.c` is 362 lines, and its entire external surface already exists
 * in the port: `func_800269C0_275C0` / `func_80026738_27338` are the DS FGM
 * backend's play/stop (reloc_backend_compat_shims.c), `ftParamGetPlayerNumGObj`
 * is ported, and the GObj kind, links, collision bounds and battle state it
 * reads are all live. So the actor is compiled in place rather than translated,
 * which keeps its thresholds, cooldowns, repeat limits and queue ordering
 * source-exact by construction -- the row's own acceptance requirement.
 *
 * What the actor asks for, and therefore what the pack has to carry: the
 * per-fighter chant `dFTCommonDataPublicFighterCallFGMs[fkind]` (Mario 609,
 * Fox 605), and the reactions Cheer 618, Amazed 619, GaspClap 620,
 * DamageL/M/S 622/623/625 and GaspL/M/S 615/616/617. A cue the pack does not
 * carry fails closed and is silent; it does not fault.
 */
#if NDS_IMPORT_BATTLESHIP_FT_PUBLIC

#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <mp/map.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/objdef.h>
#include <sys/objman.h>
#include <nds/nds_ftpublic.h>

/* The dash-run damage proof used to live inside the stub that stood here, so
 * its counters would have died with the stub. It is a port-side recorder now
 * (reloc_backend_compat_shims.c) and both paths call it. */
void ndsFighterDashRunRecordPublicCheck(GObj *fighter_gobj, f32 knockback,
                                        sb32 is_force_curr_knockback);

volatile u32 gNdsFtPublicActorMakeCount;
volatile u32 gNdsFtPublicProcUpdateCount;
volatile u32 gNdsFtPublicCommonCheckCount;
volatile u32 gNdsFtPublicPlayCommonCount;
volatile u32 gNdsFtPublicLastCommonFGM;
volatile u32 gNdsFtPublicCallStartCount;
volatile u32 gNdsFtPublicLastCallFGM;

#define ftPublicMakeActor battleship_ftPublicMakeActor
#define ftPublicCommonCheck battleship_ftPublicCommonCheck
#define ftPublicPlayCommon battleship_ftPublicPlayCommon
#define ftPublicTryStartCall battleship_ftPublicTryStartCall
#define ftPublicProcUpdate battleship_ftPublicProcUpdate

#include "../../decomp/BattleShip-main/decomp/src/ft/ftpublic.c"

#undef ftPublicMakeActor
#undef ftPublicCommonCheck
#undef ftPublicPlayCommon
#undef ftPublicTryStartCall
#undef ftPublicProcUpdate

/* Counters, not behaviour. Each wrapper forwards unconditionally; they exist
 * because "the crowd is silent" has three distinct causes -- the actor never
 * ran, it ran and never decided to play, or it decided and the pack had no
 * entry -- and the FGM backend's own UnsupportedCallCount only separates the
 * third. LastCommonFGM/LastCallFGM name WHICH cue was asked for. */
void ftPublicPlayCommon(u16 new_sfx)
{
    gNdsFtPublicPlayCommonCount++;
    gNdsFtPublicLastCommonFGM = new_sfx;
    battleship_ftPublicPlayCommon(new_sfx);
}

sb32 ftPublicTryStartCall(GObj *gobj, f32 knockback, s32 player_num)
{
    sb32 started = battleship_ftPublicTryStartCall(gobj, knockback,
                                                   player_num);

    if (started != FALSE)
    {
        gNdsFtPublicCallStartCount++;
        gNdsFtPublicLastCallFGM = sFTPublicCallID;
    }
    return started;
}

void ftPublicProcUpdate(GObj *public_gobj)
{
    gNdsFtPublicProcUpdateCount++;
    battleship_ftPublicProcUpdate(public_gobj);
}

void ftPublicCommonCheck(GObj *fighter_gobj, f32 knockback,
                         sb32 is_force_curr_knockback)
{
    gNdsFtPublicCommonCheckCount++;
    ndsFighterDashRunRecordPublicCheck(fighter_gobj, knockback,
                                       is_force_curr_knockback);
    battleship_ftPublicCommonCheck(fighter_gobj, knockback,
                                   is_force_curr_knockback);
}

void ftPublicMakeActor(void)
{
    /* The compat masks the stub used to set are what scvsbattle's manager
     * checklist reads; the real actor has to keep setting them or the battle
     * scene reports a manager it did run as missing. */
    gNdsSCVSBattleCompatManagerMask |= 1u << 4;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_FIGHTER_MANAGER;
    gNdsFtPublicActorMakeCount++;
    battleship_ftPublicMakeActor();
}

#endif

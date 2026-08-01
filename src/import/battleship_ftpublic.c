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
#include <nds/nds_startup.h>

/* THE COMPILE SEAM, and it is the whole reason this flag had never been built.
 * Every name below is one the port already owns, declared differently from what
 * the decomp TU expects. None of it is behaviour.
 *
 * `func_800269C0_275C0` is the DS FGM backend's play. `sys/audio.h:71` declares
 * it returning `void *`; `ftpublic.c:4` re-declares it returning
 * `alSoundEffect *`, which is a hard conflict rather than a warning. Renaming
 * moves that declaration AND every call site in the file together -- the one
 * case where the `#define` seam's usual limitation is exactly what is wanted --
 * so the source's `alSoundEffect *` assignments type-check against a shim that
 * forwards to the single real definition. */
#define func_800269C0_275C0 ndsFtPublicPlayFGM
alSoundEffect *ndsFtPublicPlayFGM(u16 fgm_id);

/* Declared in no port header. ftParamGetPlayerNumGObj is defined in
 * reloc_backend_compat_shims.c and used by fighter code that reaches it through
 * other seams. */
GObj *ftParamGetPlayerNumGObj(s32 player_num);

/* `ft/ftdef.h` has it; the port's fighter.h stops at U8_MAX. The source uses it
 * as a sentinel tic count (`U16_MAX + 1`), so the value has to be the decomp's. */
#ifndef U16_MAX
#define U16_MAX 65535
#endif

/* The same one-line accessor battleship_gmcollision.c defines for the same
 * reason: the decomp reaches a GObj's DObj through it and the port's headers
 * do not carry the macro. */
#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)(gobj)->obj)
#endif

/* ft/ftcommondata.c is NOT compiled in this port -- `nm` finds no
 * dFTCommonDataPublicFighterCallFGMs in the shipping ELF -- so the crowd
 * actor's one data dependency is supplied here, transcribed from
 * `decomp/BattleShip-main/decomp/src/ft/ftcommondata.c:113-142` entry for
 * entry. Twenty-six entries: twelve fighters in fkind order, a VoiceEnd hole,
 * Giant Mario, then twelve more VoiceEnd. Only Mario (609) and Fox (605) are
 * reachable in P1 and only those two are packed; every other row resolves to a
 * cue the pack does not carry, which fails closed and is silent. */
u16 dFTCommonDataPublicFighterCallFGMs[26] = {
    nSYAudioVoicePublicMario,
    nSYAudioVoicePublicFox,
    nSYAudioVoicePublicDonkey,
    nSYAudioVoicePublicSamus,
    nSYAudioVoicePublicLuigi,
    nSYAudioVoicePublicLink,
    nSYAudioVoicePublicYoshi,
    nSYAudioVoicePublicCaptain,
    nSYAudioVoicePublicKirby,
    nSYAudioVoicePublicPikachu,
    nSYAudioVoicePublicPurin,
    nSYAudioVoicePublicNess,
    nSYAudioFGMVoiceEnd,
    nSYAudioVoicePublicMario,
    nSYAudioFGMVoiceEnd,
    nSYAudioFGMVoiceEnd,
    nSYAudioFGMVoiceEnd,
    nSYAudioFGMVoiceEnd,
    nSYAudioFGMVoiceEnd,
    nSYAudioFGMVoiceEnd,
    nSYAudioFGMVoiceEnd,
    nSYAudioFGMVoiceEnd,
    nSYAudioFGMVoiceEnd,
    nSYAudioFGMVoiceEnd,
    nSYAudioFGMVoiceEnd,
    nSYAudioFGMVoiceEnd,
};

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
#undef func_800269C0_275C0

/* The one real definition, reached through the rename above. The port's
 * backend returns `void *` and the source assigns it to `alSoundEffect *`;
 * the cast is the whole shim. */
alSoundEffect *ndsFtPublicPlayFGM(u16 fgm_id)
{
    return (alSoundEffect *)func_800269C0_275C0(fgm_id);
}

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

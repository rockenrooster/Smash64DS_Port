/*
 * Bounded BattleShip ftcommonescape.c import for NDS guard-escape proofs.
 * Public symbols are remapped so the port backend can gate original Escape.
 */
#include <ft/fighter.h>
#include <nds/nds_p4_contents.h>
#include <sys/obj.h>
#if NDS_P4
#include <nds/nds_p4.h>
#endif

sb32 ftCommonGuardCheckInterruptEscape(GObj *fighter_gobj);
sb32 ftCommonLightThrowCheckInterruptEscape(GObj *fighter_gobj);

#define ftCommonEscapeProcUpdate ndsBaseFTCommonEscapeProcUpdate
#define ftCommonEscapeProcInterrupt ndsBaseFTCommonEscapeProcInterrupt
#define ftCommonEscapeProcStatus ndsBaseFTCommonEscapeProcStatus
#define ftCommonEscapeSetStatus ndsBaseFTCommonEscapeSetStatus
#define ftCommonEscapeGetStatus ndsBaseFTCommonEscapeGetStatus
#define ftCommonEscapeCheckInterruptSpecialNDonkey \
    ndsBaseFTCommonEscapeCheckInterruptSpecialNDonkey
#define ftCommonEscapeCheckInterruptDash \
    ndsBaseFTCommonEscapeCheckInterruptDash
#define ftCommonEscapeCheckInterruptGuard \
    ndsBaseFTCommonEscapeCheckInterruptGuard

void ndsBaseFTCommonEscapeProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonEscapeProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonEscapeProcStatus(GObj *fighter_gobj);
void ndsBaseFTCommonEscapeSetStatus(GObj *fighter_gobj, s32 status_id,
                                    s32 itemthrow_buffer_tics);
s32 ndsBaseFTCommonEscapeGetStatus(FTStruct *fp);
sb32 ndsBaseFTCommonEscapeCheckInterruptGuard(GObj *fighter_gobj);

/* P4: only Yoshi rolls straight into his shield (Remix: J Yoshi too,
 * yoshi_shield_fix_7; nds_p4.h NDS_P4_PARENT_KIND). */
#if NDS_P4
#define nFTKindYoshi NDS_P4_PARENT_KIND(fp, nFTKindYoshi)
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonescape.c"
#if NDS_P4
#undef nFTKindYoshi
#endif

#undef ftCommonEscapeProcUpdate
#undef ftCommonEscapeProcInterrupt
#undef ftCommonEscapeProcStatus
#undef ftCommonEscapeSetStatus
#undef ftCommonEscapeGetStatus
#undef ftCommonEscapeCheckInterruptSpecialNDonkey
#undef ftCommonEscapeCheckInterruptDash
#undef ftCommonEscapeCheckInterruptGuard

#if NDS_P2_DONKEY
/* Giant Punch charge cancellation is source-owned by the common Escape helper.
 * Re-export the already imported body for DK instead of cloning its stick/button
 * test in fighter-local code. */
sb32 ftCommonEscapeCheckInterruptSpecialNDonkey(GObj *fighter_gobj)
{
    return ndsBaseFTCommonEscapeCheckInterruptSpecialNDonkey(fighter_gobj);
}
#endif

#ifndef NDS_P4_RUNTIME_H
#define NDS_P4_RUNTIME_H

#include <ft/fighter.h>
#include <nds/nds_p4_roster.h>

typedef struct FTComputerAttack FTComputerAttack;

/* The source enum and its 27-row tables keep their original identity. These
 * accessors extend only their consuming seams; neither sentinels 27/28 nor a
 * native resource owner are interpreted as a selectable fighter. */
FTData *ndsP4GetFighterData(s32 kind);
FTStatusDesc *ndsP4GetStatusDesc(s32 kind, s32 status);
FTComputerAttack *ndsP4GetComputerAttacks(s32 kind);
u8 *ndsP4GetComputerInputScript(s32 input);
sb32 ndsP4IsMetaKnight(s32 kind);
s32 ndsP4ReplaceStatus(s32 kind, s32 status);
void ndsP4ResetFighterData(u32 data_flags);
void ndsP4BindFighterMotionData(s32 kind);
FTOpeningDesc *ndsP4GetOpeningDescs(s32 kind);
s32 ndsP4GetThrownScriptColumn(s32 victim_kind);

/* Relative event offsets also apply to new native menu motion files. Legacy
 * opening descriptors retain the original absolute-script contract. */
void *ndsP4GetMenuEventScript(FTStruct *fp, FTMotionDesc *motion);

#endif

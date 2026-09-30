#ifndef NDS_METAKNIGHT_LIFECYCLE_H
#define NDS_METAKNIGHT_LIFECYCLE_H

#include <ft/fighter.h>
#include <sc/scene.h>

/* Exact no-copy star properties from the resolved kirby_inhale_struct row;
 * existing legacy rows remain the owning Kirby source data. */
s32 ndsMetaKirbyVictimStarDamage(FTStruct *victim, const FTKirbyCopy *legacy);
f32 ndsMetaKirbyVictimStarScale(FTStruct *victim, const FTKirbyCopy *legacy);
/* Required own-source victory theme; audio admission supplies this symbol.
 * There is no parent/default-theme substitute. */
void ndsP4MetaKnightPlayVictoryBGM(void);

/* New Meta identity uses its own record and opponent cells. These accessors
 * never alias a legacy fighter's aggregate or matchup record. */
LBBackupVSRecord *ndsMetaVSRecord(s32 kind);
u16 *ndsMetaVSRecordKO(s32 kind, s32 opponent);
u16 *ndsMetaVSRecordPlayerTally(s32 kind, s32 opponent);
u16 *ndsMetaVSRecordPlayedAgainst(s32 kind, s32 opponent);

#endif

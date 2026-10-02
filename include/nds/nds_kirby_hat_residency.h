#ifndef NDS_KIRBY_HAT_RESIDENCY_H
#define NDS_KIRBY_HAT_RESIDENCY_H

#include <PR/ultratypes.h>

/* Source-derived VS copy set; runs after fighter creation and before BGM/GO. */
void ndsKirbyHatPrepareMatch(void);
/* The 1P ladder's set: the fight's fighters plus, on the Kirby Team stage,
 * the team's copy table and the manager's final copy (team_copy_kinds NULL
 * elsewhere). Runs after the fighter loop and before the first draw. */
void ndsKirbyHatPrepare1PMatch(const u8 *team_copy_kinds, u32 team_count,
                               s32 team_final_copy);
void ndsKirbyHatResidencyHalt(u32 reason) __attribute__((noreturn));
/* Copies whose hat could not be made resident (drawn without it). */
extern volatile u32 gNdsKirbyHatMissingCount;

#endif

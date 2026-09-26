#ifndef NDS_KIRBY_HAT_RESIDENCY_H
#define NDS_KIRBY_HAT_RESIDENCY_H

#include <PR/ultratypes.h>

/* Source-derived VS copy set; runs after fighter creation and before BGM/GO. */
void ndsKirbyHatPrepareMatch(void);
void ndsKirbyHatResidencyHalt(u32 reason) __attribute__((noreturn));

#endif

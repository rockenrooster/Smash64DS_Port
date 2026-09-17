/* Source-backed BattleShip sine table for original sys/matrix.c. */

#include <nds.h>

/* P2-2p8 stall budget, falsifier arm. This table is u16[0x800] = EXACTLY 4,096
 * bytes -- the whole ARM9 data cache -- and every read indexes it randomly with
 * `& 0x7FF`. In a 4 KB four-way cache that maps four lines onto every set, so
 * the table can evict anything. It measures 4,150 tk/fr over 198 accesses, or
 * 20.94 stall cycles per access, which is near-total miss: it does not stay
 * resident and it displaces whatever would have.
 *
 * Declaring the alignment ahead of the definition applies it without editing
 * decomp text or the linker script. 4,096-byte alignment is what an MPU region
 * needs -- regions are naturally aligned and power-of-two sized -- so this is
 * the difference between covering exactly this table and covering whatever
 * shares its pages. The region is configured in
 * ndsLabUncacheSinTableIfRequested (src/nds/main.c).
 *
 * The alignment is CONDITIONAL because of what it cost when it was not.
 * Forcing this one array to a 4 KB boundary moved it from 0x02153e84 to
 * 0x02155000 and shifted everything after it in `.main.rw`, which cost
 * +49,152 WORK-H P50 -- of which +46,336 landed in STG, a bucket with no
 * connection to a sine table. Uncaching the table on top of that alignment then
 * returned -6,912, about 2.2x the 3,160 predicted from its own access cost, so
 * eviction relief is real and roughly doubles a candidate's direct saving.
 *
 * The first attempt applied the alignment unconditionally and compared against
 * an unaligned control. That confounded placement with cacheability and read as
 * a 42,240 regression for "uncaching", which was the opposite of the truth.
 * One variable per arm. Evidence:
 * artifacts/performance/2026-09-16_p2-2p8-sintable-uncached/. */
#if NDS_LAB_UNCACHED_SINTABLE
extern u16 gSYSinTable[0x800] __attribute__((aligned(4096)));
#endif

#include "../../decomp/BattleShip-main/decomp/src/sys/sintable.c"

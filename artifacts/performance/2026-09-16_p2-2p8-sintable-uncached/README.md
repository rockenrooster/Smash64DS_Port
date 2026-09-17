# One array's address is worth 49,152 ticks — and eviction relief doubles a candidate's sizing

The data-locality ranking's cheap falsifier, run and then deconfounded. It
returned two separable results, and the larger one is not the one it was
designed to test.

## The subject

`gSYSinTable` (`include/macros.h:108`, defined via
`src/import/battleship_sys_sintable.c`) is `u16[0x800]` — **exactly 4,096 bytes,
the size of the entire ARM9 data cache** — and every read indexes it randomly
with `& 0x7FF`. In a four-way 4 KB cache that maps four lines onto *every set*,
so it both misses constantly (measured **4,150 tk/fr over 198 accesses, 20.94
stall cycles each**) and displaces whatever else wanted those sets.

Making it non-cacheable needs no call-site, allocator or linker change: one
alignment attribute so an MPU region can cover it exactly, and one region
configured at boot (`src/nds/main.c`, region 3 — ARM946E-S resolves overlapping
regions by highest number, and crt0 leaves 2 and 3 unused).

## Three arms, every divergence witness identical

Triangles 349,031 / 333,618, `gNdsFtrPlanHit` 6,217, `gNdsGCDrawsActiveMax` 203
in all three, so these are the same workload.

| arm | WORK-H P50 | STG P50 | FTR P50 |
|---|---:|---:|---:|
| control (unaligned, cached) | **1,588,928** | 344,768 | 355,712 |
| alignment only (cached) | 1,638,080 | 391,104 | 356,224 |
| alignment + uncached | 1,631,168 | 388,864 | 351,680 |

| effect | isolated by | WORK-H | STG |
|---|---|---:|---:|
| **4 KB alignment alone** | control -> align-only | **+49,152** | **+46,336** |
| **uncaching, given the alignment** | align-only -> uncached | **-6,912** | -2,240 |

## Result 1: frame cost is hypersensitive to data placement

Forcing **one 4 KB array** to a 4 KB boundary moved it from `0x02153e84` to
`0x02155000`, shifting everything after it in `.main.rw`, and cost **+49,152
WORK-H P50 — 3.1% of the frame**. Almost all of it (**+46,336**) landed in
**STG**, a bucket with no connection to a sine table.

That is larger than any optimization this campaign has successfully landed, from
a change that alters no code and no algorithm — only addresses.

It is the third independent sighting of the same mechanism, and the second at
~50,000:

| event | change | STG |
|---|---|---:|
| N05.03, 16-slot flat cache | +1,680 B of cacheable table | **+51,520** |
| this, alignment only | one array moved 0x117C | **+46,336** |
| N05.03 DTCM move | 1,584 B out of cacheable RAM | **-7,936** |

The stage's working set conflicts acutely with whatever shares its cache sets.
**The direction here is adverse — the original layout was better — so this is
not a banked win.** What it establishes is the *size of the lever*: deliberate
placement is worth tens of thousands of ticks, in either direction, and it
requires no allocator surgery, no decomp edit and no fidelity argument.

## Result 2: eviction relief roughly doubles a candidate's direct saving

Uncaching the table, measured against the correct control, returned
**-6,912**. Its own access cost predicted **3,160** (4,150 tk/fr less the
uncached re-read cost at 5-7 cycles/word). The measured saving is **2.2x** that.

The excess is the eviction relief the ranking could only guess at. Solving the
intended calibration `Cv = 1 + (4150 - delta)/198` gives a negative Cv, which is
the arithmetic saying the saving exceeds the table's own cost — the table was
costing more by *displacing* other data than by being slow itself.

**Consequence for the data-locality ranking:** its figures are **floors, not
ceilings**. The top candidate — a VRAM arena for `GObj`/`DObj` +
`FTStruct`/`FTParts`, sized at 55,669 tk/fr (11.2% of the gap) from direct
access cost alone — should be read with a comparable multiplier on the eviction
side, because those structures are far larger than 4 KB and miss far more often.

## The methodological failure, recorded

The first run applied the alignment **unconditionally** and compared against an
unaligned control. That arm differed by *both* placement and cacheability, and
read as **+42,240** — reported as if uncaching had cost that. Solving for the
uncached-word cost gave **235 cycles/word**, which is absurd and is what exposed
the confound.

One variable per arm. This campaign has paid for that rule repeatedly
(`prove-the-control-differs`, `one-frame-per-build-is-not-an-ab`), and it was
broken here by making a supporting change unconditional rather than putting it
behind the same flag as the thing being tested.

## Disposition

Both the alignment and the MPU region are behind `NDS_LAB_UNCACHED_SINTABLE`,
default 0. Verified reverted: `gSYSinTable` is back at `0x02153e84` and the
rebuilt ELF differs from the pre-experiment control by **7 bytes, all embedded
git stamp**.

Not kept as a win: the -6,912 is below the 14,080-tick cross-build significance
floor on its own, and it is only reachable through a +49,152 alignment. The
finding is the ratio and the placement sensitivity, not this array.

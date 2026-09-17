# N05.03: the invalidation cache is keyed wrong, the fix works, and it cannot be paid for

The board's recorded blocker for N05.03 was that an O(1) generation stamp needs
an import overlay over 23 decomp latch sites. It does not: the per-PC profile
pointed at a cheaper defect that needs no decomp edit at all. That defect is
real, the repair works exactly as predicted, and it is **cancelled three times
over by the dcache**. This is a measured negative result with a mechanism.

## The defect

`ndsFTParamsFlatWalkFor` (`src/port/reloc_backend_compat_shims.c`) caches the
flattened descendant list in a direct-mapped table hashed `(root >> 4) & mask`.
Its comment sizes that table at **four** entries and justifies it as *"a
per-FIGHTER-root steady-state cache … one resident entry per live player."*

**The cache is not keyed per fighter root.** `ndsFTParamsInvalidateSubtree` is
called with a **joint** — `ftParamsUpdateFighterPartsTransform` passes whichever
DObj's transform changed — so the number of distinct keys per frame is the
number of invalidated joints, not the number of players.

The profile had already said so. The three hottest instructions in the whole
function are the flatten walk's pointer chase, and that code only runs on a miss:

| PC | rate | tk/fr | CPI | instruction |
|---|---:|---:|---:|---|
| `0x01ffcc8c` | 184.4/fr | 3,876 | **42.04** | `ldr r3,[r6,#16]` |
| `0x01ffcc8e` | 184.4/fr | 2,832 | **30.71** | `ldr r6,[r6,#8]` |
| `0x01ffcd6e` | 102.6/fr | 2,492 | **48.59** | `ldr r0,[r3,#0]` |
| `0x01ffcce2` | 290.1/fr | 632 | 4.36 | `str r5,[r1,#4]` ← the clear |

Walk **59%**, clear **14%** of a 20,348 tk/fr bucket.

## The measurement

Four arms, one instrument. Every arm reports `gNdsFtPartsFlatHits`,
`Misses`, `Conflicts` (a miss that evicted a *different root still current this
heap generation* — a hash conflict, not a cold start), `Overflows` and
`CountMax`. **Every divergence witness is identical in all four arms** —
`gNdsFighterDLAllDrawP0/P1HardwareTriangleCount` 349,031 / 333,618,
`gNdsFtrPlanHit` 6,217, `gNdsGCDrawsActiveMax` 203,
`gNdsDamageSlashEffectsSeen` 540 — so these are comparisons of the same
workload, unlike the pose-cap arm that had to be thrown out for divergence.

| arm | table bytes | hits | misses | conflicts | miss rate |
|---|---:|---:|---:|---:|---:|
| **4 slots x 96** (shipped) | 1,584 | 16,403 | 16,237 | 15,490 | **49.7%** |
| 8 slots x 48 | 1,632 | 16,705 | 15,935 | 15,177 | **48.8%** |
| 16 slots x 48 | 3,264 | 31,458 | 1,182 | 414 | **3.6%** |
| 32 slots x 96 | 12,672 | 31,634 | 1,006 | 230 | **3.1%** |

| arm | SRC P50 | STG P50 | **WORK-H P50** |
|---|---:|---:|---:|
| **4 slots (control)** | 558,656 | 344,960 | **1,595,328** |
| 8 slots x 48 | 562,176 | 345,984 | 1,598,656 (+3,328) |
| 16 slots x 48 | **543,616 (-15,040)** | 396,480 (+51,520) | 1,629,312 (**+33,984**) |
| 32 slots x 96 | **543,552 (-15,104)** | 400,320 (+55,360) | 1,636,224 (**+40,896**) |

## What the four arms establish

**1. The diagnosis is right.** 95.4% of the shipped configuration's misses are
hash conflicts, not cold starts, so the working set genuinely does not fit. This
is the hazard `docs/optimization/SRC.md` lists as *"possible hash conflict
demonstrated synthetically; actual conflict rate unmeasured"* — measured here at
**95.4% of misses** on the real four-fighter match.

**2. The repair works, and it is worth exactly what the per-PC split predicted.**
At 16 slots the miss rate falls 49.7% -> 3.6% and **SRC falls 15,040**, inside
the -10,000..-15,000 band derived from "the walk is 59% of 20,348".

**3. There is a cliff between 8 and 16 slots.** Doubling associativity at
*constant* memory (8 x 48 = 1,632 B against the shipped 1,584) changes nothing
at all — 48.8% against 49.7%, and WORK-H moves +3,328, below the 14,080-tick
cross-build floor. Sixteen slots removes 93% of the misses. **The working set is
between 9 and 16 distinct roots**, which is what four fighters times about four
invalidated joint roots should be. The conflicts are a capacity problem, and the
capacity threshold is 16.

**4. Sixteen slots cannot be paid for. The ARM9 dcache is 4 KB.** The shipped
1,584-byte table is already 39% of it; 16 slots at 3,264 B is **80%**; 32 slots
at 12,672 B exceeds it threefold. STG — which has nothing to do with this cache
— rises **+51,520 at 3,264 B and +55,360 at 12,672 B**. Nearly the same rise for
a 7.8x difference in table size, so this is not the volume of memory added; it
is that crossing the dcache evicts the stage's working set either way. The SRC
win of 15,040 buys a 51,520 loss somewhere else.

**5. Capacity 96 is twice what the game uses.** `Overflows = 0` in both the 16-
and 8-slot arms at `MAX = 48`, so no subtree ever exceeded 48 parts. The shipped
bound of 96 costs 192 bytes per slot for headroom nothing reaches.

## Verdict

**NO-GO, and the reason is structural rather than incidental.** The lane needs
>= 16 resident slots to hold its working set, >= 16 slots costs >= 3,264 bytes,
and >= 3,264 bytes is 80% of the data cache. The arithmetic win is 15,040 and
the eviction is 51,520.

That is the same shape as every other lane this campaign has lost — the
collision ring (issue -1,717 against icache +1,854), the GX compose bank
(+22,848 from FIFO traffic), and now this. **Fetch, not arithmetic.**

## What remains, and what it is worth

One design would keep the hit rate inside the dcache: store `u8` joint indices
instead of `FTParts *`, giving 48 + 12 = 60 bytes per slot and 16 slots in
**960 bytes — smaller than the shipped table**. It reintroduces an index-to-part
lookup at clear time, but from a small contiguous `fp->joints[]` array rather
than a scattered `next` chase.

It is not being built. Its ceiling is the 15,040 already measured, which is
**3.0% of the 496,382 gap**, and the campaign's arithmetic is closed: no
combination of remaining leaf levers reaches the gate. Recording it here so the
next agent does not re-derive it.

## Incidental finding: `.itcm` is full

The first build of the overflow counters failed to link — *"region `itcm`
overflowed by 24 bytes"*. The census records **104 bytes free of 32,632**, and
an unconditional high-water compare inside the ITCM-resident invalidate path
needed more than that. Both counters were moved to the cold overflow branch.

This is a standing constraint on every candidate in this area, and it is the
same one that made `SRC.md`'s bound-domain candidate a no-go: **any lever that
adds resident code to these paths is paying in a currency the build has already
spent.**

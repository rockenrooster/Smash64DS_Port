# The placement hazard is real, the proposed mechanism is refuted, and the fix covers a quarter of it

Moving one 4 KB array swings the frame ~49,152 ticks with no code change. The
proposed explanation was that this is **heap-mediated**: shifting `.data`/`.bss`
moves `__end__`, which moves `fake_heap_start`, which moves the taskman arena
base — and that arena was aligned to only **16 bytes**, half a cache line. Pin
the arena to the 1,024-byte cache-set period and the hazard should collapse.

**It did not.** The fix is free and worth keeping, but it addresses about a
quarter of the effect and the dominant term is somewhere else.

## Four arms, every divergence witness identical

Triangles 349,031 / 333,618, `gNdsFtrPlanHit` 6,217, `gNdsGCDrawsActiveMax` 203
throughout.

| arm | arena align | `gSYSinTable` | WORK-H P50 | STG P50 |
|---|---|---|---:|---:|
| **A** | 16 B | as shipped | **1,588,928** | 344,768 |
| B | 16 B | 4 KB aligned, cached | 1,638,080 | 391,104 |
| C | 16 B | 4 KB aligned, uncached | 1,631,168 | 388,864 |
| **D** | **1,024 B** | as shipped | **1,588,544** | 345,600 |
| E | 1,024 B | 4 KB aligned, uncached | 1,620,864 | 391,360 |

| quantity | isolated by | value |
|---|---|---:|
| **cost of pinning the arena** | D − A | **−384 WORK-H, +832 STG** |
| alignment swing, unpinned | C − A | **+42,240 WORK-H, +44,096 STG** |
| alignment swing, pinned | E − D | **+32,320 WORK-H, +45,760 STG** |

## What this establishes

**1. Pinning is free.** −384 WORK-H is far inside noise, heap low-water is
unchanged at 111,680 against a 24,404 floor, and `gNdsTaskmanArenaChosenSize`
stays 1,355,520. It costs at most 1,023 bytes and loses no arena.

**2. It removes a real but minority share of the hazard.** The WORK-H swing
falls 42,240 → 32,320, a 23% reduction. That share genuinely was the arena base
re-phasing under `__end__`, and it is now invariant to `.text`/`.data`/`.bss`
size changes.

**3. The dominant term is NOT heap-mediated.** The **STG** swing is unchanged —
44,096 against 45,760 — and STG is where almost all of the effect lives. Pinning
the arena did nothing for it.

**4. The static-access budget cannot explain what survives.** The placement
investigation bounded every *named static* the entire 114-function STG subtree
touches at **23,691 tk/fr**, and used that to argue the swing could not come
from static placement. With the heap now pinned, a **45,760** STG swing remains
and static placement is the only candidate left standing. Those two numbers are
incompatible. Its own note anticipated this outcome: *"If it survives near
+49,152, the mechanism is in `.bss` statics after all and my 23,691 ceiling is
wrong — which would itself be the most informative outcome available."*

The likely error is scope: 23,691 counts stall on loads whose **base register**
resolved to a named static. A load through a pointer that happens to *point at*
a static is classified allocator-placed, so static-resident data reached
indirectly is invisible to that budget. Cache conflicts do not care how the
address was computed.

## What is kept, and what is not claimed

The 1,024-byte arena alignment is **kept**: free, verified, and it removes one
real source of cross-build variance. `src/port/diagnostics_taskman_heap.c`, with
the slack at the four `calloc` sites raised from 16 to 1,024 bytes to match.

**It is not a fix for the placement hazard**, and the earlier statement that it
was is corrected here. Until the surviving STG term is understood:

> **Any edit that changes `.data`/`.bss` sizes can still move WORK-H by tens of
> thousands of ticks with no code change.** The 2026-09-16 clean rebuild moved
> it −50,432 with no source change at all. Cross-build A/B arms that differ by a
> `#if` altering a static's size are not controlled comparisons, and the
> 14,080-tick significance floor understates their real variance.

That standing risk is why this is recorded as a hazard rather than an
opportunity. It also means several banked cross-build figures in this campaign
carry unmodelled variance of this magnitude — the same-ROM route A/B, where
`.text` is byte-identical and only a `volatile` word differs, is the only form
immune to it.

## The next question, if this is pursued

Re-run the static-access attribution **without** the base-register restriction:
classify by the *target address range* of each missing load rather than by how
its base was computed. If static-resident data reached through pointers is a
large share of the STG residual, the 23,691 ceiling lifts and static layout
becomes a real subject. If it does not, then something outside both models is
producing a 45,760 swing and neither the heap nor the statics explain it.

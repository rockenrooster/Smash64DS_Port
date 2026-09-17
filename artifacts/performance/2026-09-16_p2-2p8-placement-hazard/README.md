# The placement hazard: real, closed as a lever, and my "contradiction" was not one

> **READ THE RESOLUTION AT THE BOTTOM FIRST.** The analysis in this first half
> reaches a conclusion that the re-attribution below **refutes**: it treats
> 45,760 > 23,691 as a contradiction requiring a scope error in the attribution.
> It is not a contradiction — the two figures measure different quantities — and
> there is no scope error. The first half is kept because the four-arm
> measurement in it is sound and load-bearing; its *interpretation* is not.


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

---

# RESOLVED: there was no contradiction, and the lane is closed

The section above called 45,760 > 23,691 a contradiction and proposed a scope
error in the attribution. **Both were wrong, and the re-attribution proves it.**

Redoing the attribution by **target address range** rather than base-register
provenance — independently reproducing the 644,328 denominator to 1 tk/fr, and
`gSYSinTable` 3,152 and `sNdsRendererHardwareTextureCache` 2,131 exactly —
**confirms** the original figure: STG resolved moved-static is **20,548 tk/fr**.
It did not lift it. And every named data object in the top 20 is reached by
**base register**, not through a pointer, so the suspected indirect-reach class
barely exists.

## Why it was never a contradiction

The two numbers are not the same quantity:

| | |
|---|---|
| **23,691** | stall **on** statics |
| **45,760** | stall **caused by moving** statics |

In a set-associative cache, **the cost of relocating X is paid by whatever X
evicts**, not by accesses to X. There is no arithmetic requirement that the
second be less than or equal to the first, so a static-**access** budget can
never bound a conflict-miss swing. The error was in kind, not in scope, and no
re-scoping could have fixed it.

Everything I said downstream of that mistake is withdrawn: **the data-locality
ranking does not inherit an error**, and the VRAM-arena candidate stands as
sized at 55,669 tk/fr / 11.2%.

## The mechanism reproduces quantitatively

| step | value |
|---|---|
| shift | 0x117C = 4,476 B; mod 1,024 = **380** (11.875-line set rotation); mod 32 = **28** (line boundaries move *inside* every object) |
| re-phased | 1,069 KB of `.main.rw`+`.main.bss` |
| traffic | 9,644 fills/fr at a 26.2% miss rate = 36,809 accesses/fr |
| **+2.9 points** | 1,067 extra fills x 44 cyc = **46,968 ticks** |
| observed | 45,760 / 46,336 / 49,152 |

"Nothing collided with anything nameable" is the correct reading. The carrier is
diffuse re-phasing of a working set **78x oversubscribed** against a 4 KB cache,
and **78.2% of STG's unresolved stall sits in functions that name no moved
static at all**.

## Why the lane is closed as a performance lever

**The shipped layout is already the best of the five arms measured** (A and D,
STG 344,768 and 345,600). The experiment found a *worse* phase, not a better
one. Banking the 45,760 would require finding a phase better than current —
a blind search over a 1,024-byte space against a 14,080-tick noise floor, with
no evidence a better phase exists and no nameable conflict to aim at.

It also cannot generalise: the taskman arena is a per-scene bump allocator, so a
phase tuned on Dream Land with one roster is a fresh draw for every other
combination, against the any-roster/any-stage contract.

## What survives

Only **variance control**, which is what the 1,024-byte arena alignment already
buys: 0 ticks, and every future cross-build A/B means something. The remaining
share is the diffuse static phase, and the falsifier for pinning it is recorded
below — but it is expected to fail, and if it does this lane should be closed
for good rather than reopened.

**Falsifier (one build, one arm pair):** group the four hot address-materialised
moved statics — `sNdsNativeStageOwnerExecution` (7,360 B),
`sNdsRendererTask36ReplayOwner` (24,256), `sNdsFighterPackets` (14,192),
`sNdsRendererHardwareTextureCache` (5,456) = **51,264 B** — at a 1,024-byte
aligned group in the linker script, then re-run the `gSYSinTable` alignment arm.
If the STG swing collapses, hot-static phase was the carrier. **If it survives,
the carrier is the diffuse 1,069 KB and no implementable static-side lever
exists.**

## One hole, stated

The 42,593 ceiling assumes a function only touches statics whose addresses it
materialises. That fails for `memset`/`memcpy`/`armCopyMem32`, which receive
pointers — 249/290/1 unknown call sites against 13/4/0 known, carrying 34,386
tk/fr of unresolved stall. If much of that is static-destined the STG ceiling
rises. Closing it needs a runtime witness, not static analysis.

## Incidental: a stdlib module was shadowed in the scratchpad

`grp.py` in the session scratchpad shadowed the standard library's `grp`, which
`shutil` and `tarfile` import. Any script importing numpy from that directory
silently ran an unrelated fighter-draw analysis and printed its report — which
is the stray output that contaminated several command results earlier in this
session. Renamed. Worth knowing: a scratchpad file named after a stdlib module
corrupts every later tool that runs there.

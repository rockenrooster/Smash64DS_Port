# ARM9 D-cache working-set census — and why no layout candidate clears the bar

**Date:** 2026-08-14
**Profile:** `artifacts/performance/2026-08-12_c125-slice48/profile/arm9-profile.csv`
(whole match, `regions=1601`, 3,660,281,186 cycles, 922,736,967 instructions),
against `builds/build-c125-profile/…tickhud-hwtri.elf`. No build, no ROM, no
emulator run was spent on this census.
**Tool:** `scripts/census-dcache-working-set.py` (new).

**UNITS: every number below is a PROFILE CYCLE. Two profile cycles = one project
tick.** Halve before comparing against any floor, gate or ticks/frame budget.
(Corrected 2026-08-14 — the first revision compared cycle figures directly
against tick floors and overstated every candidate by 2x.)

**Verdict: STOP. No single data-layout/locality candidate reaches the 8–16K
ticks/frame gate.** The addressable pool is large (276,984 cyc/frame =
138,492 ticks/frame) but flat across ten families and ~400 sites, the three
biggest addressable families are
`decomp/`-owned structs this repo may not reorder, and the largest family that
*is* port-owned already carries a proven general bound. Details and every
rejected candidate below.

**Two findings worth more than the negative result:**

1. **The single most expensive "load" in the build is not a cache miss.**
   `ndsRendererTask36ReplayRun`'s `ldr r3,[r1,#184]` at **507 cyc/execution,
   16,629 cyc/frame = 8,315 ticks/frame** is a poll of `DMA0CNT` at `0x040000B8` — a busy-wait for a
   synchronous DMA to the GX FIFO. Ranking loads by cyc/execution without
   resolving the base register puts it at #1 by a factor of two and sends the
   next layout task straight at it.
2. **`gcRunAll` costs two cache lines per GObj where one would do**, because
   `flags` sits at offset 124 and the rest of the walk lives at 4/20/21. It is
   worth ~3,000 cyc/frame and it is **unfixable** here: `GObj` is defined in
   `decomp/…/objtypes.h` and `battleship_sys_objman.c` `#include`s decomp's
   `objman.c` in place.

---

## Phase 1 — classified hot-load census

The existing `analyze-dcache-stalls.py` excludes literal-pool and stack loads but
not MMIO, so this pass walks backward from each hot load to where its base
register was last set and classifies the address space.

Excess is `cycles − executions × 3`, i.e. over a conservative cached-`ldr` cost.

| class | excess cycles | /frame | share |
|---|---:|---:|---:|
| **cacheable** | 443,451,773 | **276,984** | 88.9% |
| mmio | 31,904,981 | 19,928 | 6.4% |
| timer (the instrument itself) | 8,969,950 | 5,603 | 1.8% |
| unknown (base unresolved) | 14,582,124 | 9,108 | 2.9% |
| **total** | 498,908,828 | 311,623 | |

Only `cacheable` can be helped by a data-layout change.

### The MMIO rows, so they are never mistaken for misses again

| cyc/ex | execs | /frame | function | instruction |
|---:|---:|---:|---|---|
| **507.2** | 52,800 | **16,629** | `ndsRendererTask36ReplayRun` | `ldr r3,[r1,#184]` base `0x4000000` |
| 530.0 | 2,304 | 758 | `ndsRendererSubmitNativeRebirthHalo` | `ldr r3,[r0,#184]` base `0x4000000` |
| 17.8 | 57,600 | 534 | `ndsRendererNativeStageBeginRun` | `ldrh r2,[r6,#96]` base `0x4000000` |
| 8.0 | 86,400 | 270 | `ndsRendererNativeStageBeginRun` | `ldrh r3,[r3,#96]` base `0x4000000` |

The top row's context is unambiguous:

```
str r0, [r2, #176]   ; DMA0SAD
str r0, [r2, #180]   ; DMA0DAD
str r3, [r2, #184]   ; DMA0CNT, enable | fixed-dst | GXFIFO timing
ldr r3, [r1, #184]   ; <-- 507 cyc/ex
cmp r3, #0
blt .-8              ; spin while bit31 (enable) is set
```

507 cycles per poll is the ARM9 being held off the bus by the active DMA, not a
line fill. **This is a scheduling question, not a layout one, and it is not in
this task's scope** — see "Not implemented, and why" below.

### Ranked cacheable sites (top 20 of ~400)

| cyc/ex | execs | /frame | function | instruction |
|---:|---:|---:|---|---|
| 27.2 | 512,624 | 7,743 | `gcPlayDObjAnimJoint` | `ldrb r5,[r4,#5]` — `AObj->kind` |
| 17.8 | 599,160 | 5,544 | `memcpy` | `ldr r4,[r2,#4]` |
| 41.8 | 164,464 | 3,987 | `ndsFTParamsInvalidateSubtree` | `ldr r1,[r2,#0]` |
| 41.4 | 157,494 | 3,778 | `ftParamUpdateAnimKeys` | `ldr r3,[r4,#116]` |
| 43.1 | 142,751 | 3,578 | `ndsBaseGcRunAll` | `ldr r3,[r0,#20]` — `GObj->func_run` |
| 35.7 | 146,614 | 2,995 | `ndsBaseGcRunAll` | `ldr r3,[r0,#124]` — `GObj->flags` |
| 18.6 | 303,680 | 2,960 | `ndsRendererNativeApplyStateDelta` | `ldrb r3,[ip,#8]` |
| 32.8 | 130,981 | 2,441 | `ndsBaseGcRunAll` | `ldr r0,[r0,#4]` — `GObj->link_next` |
| 80.6 | 48,960 | 2,372 | `ndsFighterMarioFoxDLAllDrawForSlot` | `ldr r3,[r3,r0]` |
| 41.5 | 95,700 | 2,301 | `gcPlayAnimAll` | `ldr r3,[r3,#16]` |
| 44.4 | 88,984 | 2,300 | `glBindTexture` | `ldr r2,[r0,#16]` |
| 35.6 | 95,700 | 1,951 | `gcPlayAnimAll` | `ldr r4,[r3,r2]` |
| 41.2 | 79,479 | 1,895 | `ftDisplayMainDrawDefault` | `ldr r5,[r4,r2]` |
| 37.7 | 86,355 | 1,871 | `ndsRendererAdapterBuildDObjLocalMatrix` | `ldrb r3,[r1,#4]` |
| 38.1 | 79,479 | 1,744 | `ftDisplayMainDrawDefault` | `ldr r0,[r0,#4]` |
| 33.6 | 89,441 | 1,710 | `ndsBaseGcRunAll` | `ldrb r3,[r0,#21]` |
| 35.3 | 83,479 | 1,686 | `ndsFighterDisplayContractCountFlags` | `ldr r0,[r4,#16]` |
| 15.8 | 209,600 | 1,673 | `ndsRendererNativeStagePreparedTextureValid` | `ldr r1,[r2]` |
| 34.9 | 83,479 | 1,664 | `ndsFighterDisplayContractCountFlags` | `ldrb r3,[r4,r7]` |
| 34.1 | 84,818 | 1,649 | `ndsFighterMarioFoxDLAllDrawForSlot` | `ldr r0,[r0,#20]` |

### Grouped by working set — the shape that decides the task

| family | cyc/frame | sites | share | largest single site |
|---|---:|---:|---:|---:|
| Z other (80 functions) | 43,001 | 147 | 24.8% | 1,248 |
| **B GObj list walks** | **22,262** | 26 | 12.8% | 3,578 |
| G stage renderer | 21,750 | 40 | 12.5% | 1,673 |
| **A AObj / anim playback** | **20,766** | 22 | 11.9% | 7,743 |
| D FTStruct / FTParams | 16,319 | 23 | 9.4% | 3,987 |
| E renderer stats / traversal | 14,458 | 20 | 8.3% | 2,960 |
| H matrix / transform | 12,575 | 20 | 7.2% | 1,871 |
| F native fighter dense/prepared | 11,497 | 9 | 6.6% | 1,895 |
| I memcpy / bulk | 6,835 | 3 | 3.9% | 5,544 |
| C MObj / material | 4,590 | 6 | 2.6% | 1,496 |
| **total (top-400 sites)** | **174,300** | | | |

**Every family is flat.** The largest single site in the whole cacheable pool is
7,743 cyc/frame and belongs to the one lane already closed. No family
concentrates enough into one structure to fund an 8–16K change.

## Phase 2 — working-set sizes

ARM9 D-cache: **4 KiB, 32-byte lines = 128 lines total.**

| family | record | visits/frame | lines touched/frame | bytes/frame | vs 4 KiB | traversal |
|---|---:|---:|---:|---:|---|---|
| B GObj (`gcRunAll`) | ~136 B, spans 5 lines | 91.6 | **~183** (2/node) | 5,856 | **1.4x over** | pointer chase `link_next` |
| A AObj (`gcPlayDObjAnimJoint`) | 36 B | 357.8 | ~358 (1/node) | 11,456 | **2.8x over** | pointer chase `next` |
| F prepared dense vertices | 12 B x 541 | 1,878 corners | 203 (whole table) | 6,492 | **1.6x over** | random by dense id |
| E `NDSRendererStats` | large; hot offsets seen at 8, 548, 1056 | 83.1 runs | ≥3 distinct lines/run | — | reuse distance exceeds cache between runs | scattered fields |

Every hot family's per-frame footprint already exceeds the entire cache, and
three of the four are pointer-chased rather than sequential. That is the
mechanism behind the flat profile: nothing stays resident, so cost is spread
evenly over every structure rather than concentrated in one fixable place.

## Phase 3 — candidates, and why each was rejected

The task's own rule was applied to each: *which cache line stops being touched,
or which two lines become one, or how does reuse distance drop below 4 KiB?*

### 1. `GObj` hot-field repack — REJECTED, blocked and sub-floor

`gcRunAll` reads `link_next`(4), `func_run`(20), a byte at 21, all on **line 0**,
and `flags`(**124**) on **line 3** — two fills per node.

```
GObj, 136 B, 5 lines:
  line 0  [  0.. 31]  id  link_next(4)  link_prev  link_id dl_link_id
                      frame_draw_last obj_kind(15)  link_priority  func_run(20)
                      gobjproc_head(24)  union(28)          <-- walk reads 4, 20, 21
  line 1  [ 32.. 63]  dl_link_next dl_link_prev dl_link_priority proc_display
                      camera_mask camera_tag
  line 2  [ 64.. 95]  buffer_mask  gobjscripts[0..2]
  line 3  [ 96..127]  gobjscripts[3..4] gobjscripts_num obj(116)
                      anim_frame(120)  flags(124)            <-- walk reads 124
  line 4  [128..    ] func_anim  user_data
```

Answers the rule cleanly — line 3 stops being touched by the run walk, two lines
become one. **But: ~3,000 cyc/frame, one third of the floor. And it cannot be
built anyway** — `GObj` is `decomp/BattleShip-main/decomp/src/sys/objtypes.h`,
and `src/import/battleship_sys_objman.c:23` `#include`s decomp's `objman.c` in
place. `decomp/` is read-only by hard rule.

### 2. `AObj` side array / hot-cold split / dense repack — REJECTED, already closed

`artifacts/performance/2026-08-13_sitr-aobj-layout/REFUTED_AOBJ_SIDE_ARRAY.md`
proved the record costs **exactly one line fill per visited node**, and that fill
serves `next`(0), `track`(4), `kind`(5), `length_invert`(8), `length`(12),
`value_base`(16), `value_target`(20), `rate_base`(24), `rate_target`(28). There
is no separable header miss: removing the `kind` load *migrates* the fill to the
first payload load. Its general bound — **no representation exceeds a
10,491 cyc/frame ceiling**, at 20-byte records with zero maintenance — closes the
whole family, not just one shape. This census re-measures the same site
(7,743 cyc/frame) and adds nothing that reopens it.

### 3. `NDSRendererStats` hot/cold split — REJECTED, sub-floor

Family E totals 14,458 cyc/frame but its largest site is 2,960, and the observed
offsets (8, 548, 1056) are already far apart — a hot block would have to capture
fields across `ndsRendererSyncTextureTile`, `ndsRendererNativeApplyStateDelta`
and `ndsRendererNativePrepareProductionRun`, whose hot subsets barely overlap.
The 2026-08-14 per-run investigation predicted this lane would be worth chasing
for cold `stats` loads; measured, `PrepareProductionRun`'s own loads are
6,713,721 excess = **4,193 cyc/frame**, half the floor.

### 4. Fighter dense/prepared table reordering — REJECTED, does not delete traffic

Family F is 11,497 cyc/frame, but the prepared-dense table is *already* the
compact 12-byte DS-native form (R2-03 E29) and is indexed randomly by 1,878
corners a frame. Reordering it changes which lines are touched, not how many:
6,492 bytes against a 4 KiB cache is 1.6x over regardless of order. **Fails the
rule** — no line stops being touched.

### 5. `memcpy` at 5,544 cyc/frame — REJECTED, not a layout question

599,160 executions of one load inside `memcpy`. The fix is fewer/larger copies at
the call sites, which is a call-graph change, not a data-layout change, and the
census cannot attribute it to a caller without a separate pass.

### 6. Family Z — REJECTED, structurally undistillable

43,001 cyc/frame spread over **80 functions and 147 sites**, largest function
2,552 and largest single site 1,248. There is no structure to fix; this is the
long tail of a pointer-linked N64 data model.

## Not implemented, and why

The task's gate: *"Do not implement if its realistic predicted saving is below
the current measurement floor… If no such data-layout candidate exists, STOP and
report that result."*

**UNIT CORRECTION, 2026-08-14.** Every figure in this document is a PROFILE
CYCLE. **Two profile cycles are one project tick.** The campaign floors are in
TICKS, so a cycle figure must be halved before it is compared against one. The
paragraphs below originally compared cycle counts directly against tick floors
and so overstated every candidate by exactly 2x; they are corrected in place.

Campaign floors, from `REFUTED_AOBJ_SIDE_ARRAY.md`: cross-build placement
**±8,544 ticks**, that arm's repeat spread **9,664 ticks**, `HANDOFF.md`'s
bankable bar **~16,000 ticks** — i.e. ±17,088 / 19,328 / ~32,000 *cycles*. Best
available layout candidate: **~3,000 cycles = ~1,500 ticks**, under a fifth of
the placement floor, and blocked by `decomp/` besides. Nothing was built, and the
shipped ROM is untouched —
`smash64ds-battle-playable-hwtri.nds` still `2015FBD1F68B81C03626D8C6D473C8BCBCF527A3A26DFE86FF19BD74ECBB1360`.

**The DMA busy-wait was deliberately not implemented either.** It is 16,629
cyc/frame = **8,315 ticks/frame** — the largest single site in the build, but
**not at the bankable bar**: this document originally read its cycle figure as
ticks and called it "the only single site at the bankable bar", which is wrong by
2x. At 8,315 ticks it sits just under the ±8,544 cross-build placement floor, so
even a perfect deletion would not be measurable on its own. Beyond that, (a) it
is a scheduling change, not the data-layout/locality change this task scoped, and
(b) it is very likely the trap the task's own CRITICAL RULE describes: if the GX
FIFO is the bottleneck, the CPU has nothing to do during the transfer and
removing the spin moves the wait rather than deleting it. It needs its own task,
starting with the question *"is the geometry engine saturated during that DMA, or
is the CPU merely choosing to wait?"* — measurable with a GXSTAT FIFO-depth
sample at the poll, no ROM change required to answer.

## The durable conclusion

The ARM9 is memory-bound — 88.9% of data-load excess is cacheable traffic,
276,984 cyc/frame — but **the cost is uniformly distributed over BattleShip's
original pointer-linked data model, and the three largest addressable families
(B 22,262 + A 20,766 + D 16,319 = 59,347 cyc/frame) are all `decomp/`-owned
structs this repo may not reorder.** Where the port owns the layout it has
already compacted it (12-byte dense vertices, A3I5 atlas, dense anim records).

That is not "locality work is finished"; it is "**locality work has no remaining
single-structure lever**". Anything further in this direction has to change how
much data is *visited* — node counts, call counts, visit rates — rather than how
it is arranged. `HANDOFF.md` already records the same conclusion for the AObj
lane: *"what is left in this lane is call count… nothing about how they are laid
out."* This census extends that from one family to all ten.

## Reproduce

```bash
arm-none-eabi-objdump -d builds/build-c125-profile/smash64ds-battle-playable-tickhud-hwtri.elf > c125.dis
python scripts/census-dcache-working-set.py \
  artifacts/performance/2026-08-12_c125-slice48/profile/arm9-profile.csv \
  --dis c125.dis --regions 1601 --top 400
```

Caveat carried from `HANDOFF.md`: this is a **tick-HUD instrumented** profile.
The `timer` class (5,603 cyc/frame) is the instrument, and placement differs from
the shipped ELF — check the shipped ELF before costing any placement work off it.

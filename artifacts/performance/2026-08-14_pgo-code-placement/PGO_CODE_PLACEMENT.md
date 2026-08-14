# Profile-guided code placement — STOP at Phase 4. The hot set is 33.6x the I-cache.

**Date:** 2026-08-14
**Target:** ≥17,000 WORK-H P95 ticks from link-order change alone.
**Result:** the placement model cannot produce a credible candidate. The hot
instruction working set overflows the I-cache by **33.6x**, every one of the 64
sets is already conflicted, and the hot lines are **already distributed to within
2.5% of a perfectly even spread**. There is no conflict pathology to fix.
**No linker order was changed. No ROM was built. The shipped ROM is untouched.**

A second, independent blocker is recorded in §6: the repo's emulator emits
profile `v2`, which has no `icache_fill` column, so this task's own
PROFILE VALIDATION section and success criterion #2 cannot be satisfied on the
current toolchain even if a candidate existed.

**UNITS: 2 profile cycles = 1 project tick.**

---

## 1. Phase 0 — the census bug, and what fixing it retracted

`scripts/census-dcache-working-set.py` mapped an unresolvable base register to
`cacheable`. Fixed to `unknown`, counted in neither direction. Re-running the
report **inverts its headline**:

| class | as published (fail-open) | corrected (fail-closed) |
|---|---:|---:|
| cacheable | 276,984 cyc/fr — **88.9%** | **239 cyc/fr — 0.1%** |
| mmio | 19,928 cyc/fr — 6.4% | 19,928 cyc/fr — 6.4% |
| timer | 5,603 cyc/fr — 1.8% | 5,603 cyc/fr — 1.8% |
| unknown | — | **285,853 cyc/fr — 91.7%** |

**"The ARM9 is memory-bound — 88.9% of data-load excess is cacheable traffic" was
never measured.** It was the default. Only 0.1% is proven cacheable. The walk
resolves immediate-MOV/ORR bases only, and most hot loads take their base from an
argument or a pointer chain, so 91.7% is genuinely unattributed — much of it
probably cacheable, but probability is not what that sentence claimed. The
census's STOP verdict is unaffected and strengthened. `CENSUS.md` carries the
retraction.

## 2. Phase 1 — I-cache geometry, and an honest note on its provenance

| property | value | source |
|---|---|---|
| line size | **32 bytes** | **Verified in-toolchain.** `devkitPro/libnds/include/nds/arm9/cache.h:74` — `DC_InvalidateRange` warns "Base address and size must be cache line size (32-byte) aligned" |
| I-cache size | 8192 bytes | ARM946E-S TRM / DS hardware configuration — **not present in this project or toolchain** |
| associativity | 4-way | same |
| sets | 8192 / 32 / 4 = **64** | derived |
| index | `set = (addr >> 5) & 63` | derived |
| **set period** | **2048 bytes** | derived — two addresses collide iff congruent mod 2048 |

**Stated plainly: only the 32-byte line is confirmed from a source inside the
project.** The size and associativity are standard ARM946E-S/DS values that I
could not cite to an in-tree document, and the task said not to work from memory.
Rather than build a CP15 `c0,c0,1` (Cache Type Register) read to settle it, §4
shows the **conclusion is insensitive to the uncertain parameter** — which is the
cheaper and stronger answer. If the lane is ever reopened, one CP15 read
settles it exactly.

D-cache geometry (4 KB) was deliberately not reused here; the two differ.

## 3. Phase 2 — the hot code map

Instrument: `scripts/census-icache-placement.py` (new). Inputs: the c125 whole-
match PC profile (1,601 regions) and the linked ELF's disassembly. ITCM-resident
code is excluded — it is zero-wait and never occupies an I-cache line, and
including it would measure the binary rather than the cache.

```
executed non-ITCM functions                1,300
  their total text footprint             404,608 B = 49.4x the I-cache
distinct 32B lines actually fetched        8,596  = 275,072 B = 33.6x the cache
```

Top of the map, with the span each function occupies:

| cyc/frame | bytes | lines | sets | function |
|---:|---:|---:|---:|---|
| 56,681 | 2,820 | 89 | 64 | `ndsRendererCommitNativeStageSegment` |
| 54,473 | 7,396 | 232 | 64 | `ndsFighterMarioFoxDLAllDrawForSlot.constprop.0` |
| 39,260 | 604 | 19 | 19 | `gcPlayDObjAnimJoint` |
| 38,574 | 1,028 | 33 | 33 | `ndsR2AnimValueQ` |
| 35,182 | 616 | 20 | 20 | `ndsRendererMtxMulAffine20p12` |
| 27,664 | 1,836 | 58 | 58 | `ndsRendererNativeStageBeginRun.part.0` |
| 27,329 | 2,056 | 65 | 64 | `ndsRendererAdapterBuildFighterTraRotRpyDirect20p12` |
| 23,598 | 512 | 16 | 16 | `ndsRendererNativeStageLoadNoZMatrix` |
| 22,604 | 2,396 | 75 | 64 | `ndsR2FtAnimParseDObjFigatree` |
| 21,187 | 392 | 13 | 13 | `ndsRendererLoadHardwareSplitMatrices` |
| 20,561 | 2,440 | 77 | 64 | `ndsRendererAdapterBuildDObjXObjMatrix` |
| 19,456 | 716 | 23 | 23 | `ftParamUpdateAnimKeys` |

**The two hottest functions alone occupy 321 lines = 10.3 KB = 1.25x the entire
I-cache.** They cannot both be resident whatever order they are linked in, and
six of the top twelve individually span all 64 sets.

## 4. Phase 4 — the pathologies, and why none of them is present

The task named five patterns. Measured against the hottest 2,000 lines:

```
hottest 2,000 lines spread over 64/64 sets
  distinct hot lines per set: max 40, median 32, min 20
  sets holding more than 4 hot lines (guaranteed conflict): 64/64
  perfectly even spread would be 31.2 lines/set
```

**1. Hot caller/callee far apart** — irrelevant at this footprint. Adjacency helps
when the pair can be co-resident; a 2,000-line hot set against 256 line-slots
(64 sets x 4 ways) means the callee's lines are evicted between calls regardless
of how close the two are linked.

**2. Cache-set collisions** — present in *every* set, which is the same as saying
there is no collision *pathology*. A pathology is an uneven distribution that
reordering can flatten. This distribution is already flat: **median 32 against a
theoretical-best 31.2**, range 20–40. The best conceivable reordering improves
mean set pressure by **2.5%**, against a **7.8x** overflow of line capacity.

**3. Cold text between hot members** — cannot pay. Evicting cold text from between
two hot functions frees lines in a cache that is already oversubscribed 33.6x;
the freed lines are immediately taken by other hot code.

**4. Hot cluster larger than cache** — **this is the case, by 33.6x.** The task
asks whether the phases are "temporally separable" so they can occupy
conflict-minimised ranges. They are not: `ndsRendererCommitNativeStageSegment`
(stage), `ndsFighterMarioFoxDLAllDrawForSlot` (fighter draw), `gcPlayDObjAnimJoint`
/ `ndsR2AnimValueQ` (animation) and `ndsRendererMtxMulAffine20p12` (matrix kernel,
called from both draw paths) interleave within every single frame. There is no
phase boundary to align an address range to.

**5. Alignment pathology** — not reached. Padding is priced in main RAM and the
project is already under the GObj-cap heap threshold; spending bytes to shift a
distribution that is already within 2.5% of optimal is not defensible.

### Why the verdict does not depend on the unverified geometry

The only uncertain inputs are cache size and associativity. The conclusion holds
across every plausible value:

| assumed I-cache | hot-set overflow | verdict |
|---|---:|---|
| 4 KB | 67.2x | capacity-bound |
| **8 KB (actual)** | **33.6x** | **capacity-bound** |
| 16 KB | 16.8x | capacity-bound |
| 32 KB | 8.4x | capacity-bound |

Placement optimisation pays in the band where the working set is *near* cache
size and the conflict distribution is *uneven*. This build is an order of
magnitude outside that band on the first axis and already near-optimal on the
second. Pettis–Hansen ordering and set-conflict minimisation both target a regime
this binary is not in.

## 5. Phases 3 and 5 — not built, and why that is the correct call

The weighted call-transition graph (Phase 3) and the host-side placement
optimiser (Phase 5) were **not** built. Building an optimiser whose objective
function is known in advance to have a 2.5% ceiling on its dominant term, in
order to generate candidate orderings that cannot be validated (§6), would be
spending the budget to confirm §4 rather than to test it.

`scripts/census-icache-placement.py` is retained: it is the measurement that
gates the lane, it is deterministic, and re-running it is how a future cycle
would notice if the footprint ever came down far enough to matter.

## 6. The independent instrument blocker

Even had a candidate existed, this task's evidence bar could not be met.

Success criterion #2 requires "PC profile shows reduced stall/cycle cost in the
targeted hot cluster", and PROFILE VALIDATION asks for "targeted high-cycle
instruction fetches become cheaper — rather than only: WORK-H happened to move".

The repo's emulator emits **`format=melonDS-arm9-retail-profile-v2`** — nine
columns, `total_cycles` only, no stall partition. The instrument that separates
instruction fetch from issue, D-cache and interlock is the **v3 stall
attributor** (`melonDS-Accurate` branch `r2-stall-attributor`, commit `4a1abf61`),
which adds `issue, icache_fill, dcache_fill, write_buffer, bus_contention,
dma_hold, cart_spin, interlock, halt_wait, gx_paid, gx_blamed`. That branch was
**never adopted into the repo** (`ClaudeOpus5_R200a_StallAttributor_20260727.md`
§Status), `emulators/` holds binaries only, and `strings` on the shipped
`melonDS.exe` finds `v2` and none of the v3 column names.

Without it, a placement candidate could only ever be banked on "WORK-H happened
to move" — the exact inference this task forbids. And R2-00a additionally
established that **WORK-H P95 is inflated on precisely the frames the gate is
decided by**, so steering placement by that tail without an independent stall
instrument is steering by a known artifact.

**That the v3 attributor's own numbers are the reason this lane looked promising
is worth stating: `icache_fill` (1,525,043) measured almost exactly equal to
`issue` (1,522,083).** Instruction fetch really is expensive here. It is simply
not *reorderable* — §4 is why.

## 7. Section sizes

Unchanged, because nothing was built. Recorded for the next cycle's baseline,
from `smash64ds-battle-playable-hwtri.elf` (2026-08-14 12:58):

| | |
|---|---|
| shipped ROM | `smash64ds-battle-playable-hwtri.nds`, 12,232,704 B |
| shipped ELF | 10,203,972 B |
| executed non-ITCM text | 404,608 B |
| distinct lines fetched | 275,072 B |

## 8. Verdict — close the placement lane, and the named next owner

**Link-order placement cannot pay on this binary.** The reason is structural and
survives every plausible cache geometry: the hot code is 33.6x the cache and its
set distribution is already within 2.5% of optimal.

Two things follow, and they are more useful than a shuffle would have been:

1. **Instruction fetch is a large, real, measured cost** — `icache_fill` ≈ `issue`
   in the only instrument that has ever separated them. The lever that reaches it
   is **reducing the hot footprint**, not rearranging it: 404,608 bytes of
   executed text against an 8 KB cache. That is a *code size* problem wearing a
   cache costume, and it is the named next architectural owner.
2. **The v3 stall attributor should be adopted before any further memory-lane
   work.** Three separate lanes this week (D-cache layout, call-frame, placement)
   have each ended by needing a stall class the shipped profile does not carry.
   Adopting `r2-stall-attributor` is a one-time cost that unblocks all three.

## Reproduce

```bash
arm-none-eabi-objdump -d builds/build-c125-profile/smash64ds-battle-playable-tickhud-hwtri.elf > c125.dis
python scripts/census-icache-placement.py \
  artifacts/performance/2026-08-12_c125-slice48/profile/arm9-profile.csv \
  --dis c125.dis --regions 1601
python scripts/census-dcache-working-set.py \
  artifacts/performance/2026-08-12_c125-slice48/profile/arm9-profile.csv \
  --dis c125.dis --regions 1601 --top 6
```

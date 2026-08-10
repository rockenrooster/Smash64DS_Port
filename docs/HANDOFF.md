# Handoff

Updated: 2026-08-09. **The gate arm's tail was cartridge I/O: the animation
cache arena had been full and refusing loads all match. Fixed at `f082b3c8` —
`WORK-H` P95 1,639,299 → 1,447,318, gap 326,938; cycle 108's AObj16 warm-time
prebake takes ~23,000 more.** The campaign remains on
R2-07's performance gate. Published pair:
`smash64ds-battle-playable-hwtri.nds` `AFD28273…`, `smash64ds.nds`
`54C07FAC…`.

## Read this first: every 128-frame measurement in the archive is unusable

**The 128-frame window reads the cheapest 6% of the match** — P95 understated
~306,000 and the over-gate rate five times. `sample-tick-hud-buckets.ps1` takes
repeated ring dumps (`-Samples` to 4096, `-RingStopStride` 96, ROM
byte-identical). Never take a gate reading on 128 frames again.

## The two baselines — label every figure with its arm AND its coverage

Both arms run the **same 60-second match** (coverage 86.7%), both windows ending
43 frames past the buzzer. Slips 0 in every row.

| arm | role | `WORK-H` P50 | P95 | over gate |
|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE (owner, 2026-08-05)** | 1,112,576 | **1,447,318** | 754/1600 |
| **Boundary** mode 163 | shipped configuration | 1,082,112 | 1,476,672 | 673/1600 |

**Gate baseline is 1,447,318 as of `f082b3c8`**, less cycle 108's ~23,000; head
reads P50 1,107,008 / P95 1,411,283 / 707 over gate. Boundary is not re-banked,
so its 1,476,672 is stale-high. The soak's long match is
`NDS_R2_SOAK_MATCH_MINUTES`, and `probe-match-window.ps1` reads the match timer
out of the guest so a window cannot claim coverage it did not have.

The owner's bar: the whole match under the P95 budget on the both-CPU config,
loading states excluded; the shipped ROM stays the Boundary hwtri pair.
`Makefile:305-308` still forbids reporting a both-CPU P95 as the Boundary
figure. **Re-pin `EXPECTED_CENSUS_SHA256` in the commit that changes what it
covers** — a stale pin kept the verifier red for 35 commits.

## What is dead, so nobody re-derives it

- **Effect DObj submits** — Boundary-only: `MISC` is 99.3% of the Boundary
  excursion but **~12.1%** of the gate arm's; G3 refuted in cycles 88–91.
- **Projectiles** — weapon DObj submit medians **44 ticks/frame**; not the tail.
- **Particles** — flat ~47,000/frame; a P50 lever only, retiring SwitchPlan §7
  option 2 (15 Hz round-robin) as a *gate* answer.
- **The force-load seam** — closed cycle 108; see the next-step section.
- **Texture thrash**, **`Find`**, **`Material`**, **`FTR` as the gate**.
- **Task 56 strips** — REVERT: **the ROM hangs the present loop**; its
  `PERF_LEDGER` KILL row has no completed run behind it.

## RAM: both budgets are near their floor — price a change before writing it

- **Static/boot.** `scripts/check-boot-headroom.ps1 -Build <dir>` after every lab
  build (OK / UNPROVEN / OVER CLIFF, exit 1). Highest `fake_heap_start` proven to
  boot **`0x02294804`**, lowest proven to fail **`0x02294b24`**; the gate arm
  links at `0x0228c004` for **34,816** proven. **Text counts as much as bss.** A
  failing arm never reaches presented frame 1 and reads as a hung emulator.
- **`gSYTaskmanGeneralHeap`.** `gNdsTaskmanGeneralHeapFreeMin` is **42,136**
  against the anim cache's 32,768 `KEEP_FREE`. The two are coupled: freeing
  `.bss` lowers `fake_heap_start`, which enlarges the heap.

**The `Tex` (dl-pointer, bind-ordinal) memo is REFUTED** — 471 hits of 10,336
consults, 7,517 evictions of 7,525 fills, `Tex` ticks *up*.

## Next single step — split `SINT`, `SPHD`, `SHDT`, `SCPU` on the over-gate frames

**44.2% of frames are over gate (707/1600), not ~5%.** `WORK-H` P50 1,107,008
clears the 1,120,380 gate by only 13,372, so the median frame barely passes and
the excess summed over every over-gate frame is **91,928,908 ticks**. Splitting
every bucket by over-gate vs under-gate names the discriminators: `SRC`
**+171,383**, essentially all of it `GCRA` (`gcRunAll`, the whole simulation),
which decomposes into `SINT` **+88,082**, `SPHD` +28,941, `SHDT` +27,190,
`SCPU` +23,531 — 167,744 of 171,430, so nothing is hiding. `FTR` separates the
populations by only **+13,768**, which retires fighter draw (and its
`memset`/`memcpy` concentration) as a gate lever for good. Board has the table.

**Size every proposal against the board's sensitivity curve.** The median clears
the gate by only **13,372** and **238 frames sit within 50,000 of it from
above**, so a body-wide 50,000 moves 238 frames from 20 FPS to 30 FPS
(707 → 469 over gate) while reading as an ordinary P95 delta. **Only soft float
is the right size** — ~98,500 ticks/frame, i.e. the 100,000 row, 707 → 295.
Everything else on the map is 500–5,000 ticks against a 291,000 gap. Recompiling
is refuted (`Makefile:3165-3179`), so the arithmetic must actually not happen;
at that scale it becomes a `PROJECT_GOAL` sacrifice-order call needing the owner.

**The force-load seam is closed (cycle 108).** Pre-finalizing and handing back
the arena pointer is structurally impossible: `ftmain.c:4623` **discards the
return value** and animates from `fp->figatree_heap`, so the destination copy is
mandatory. Violating it reads as a different match, not as slow. Do not add
another caching layer to the loader; board has the three facts it paid for.

**The soft-float bill is mapped** by
`scripts/analyze-leaf-helper-attribution.py`, off an existing profile with no
build and no run: **8.9% of non-idle work**, led by animation evaluation 2.57%,
collision 1.79%, matrices 1.33%.

**But the machine is MEMORY-BOUND, so do not open a conversion campaign.**
Non-idle **CPI 2.85 — 65% of non-idle cycles are stall, not issue**. The
soft-float helpers are the *most efficient* code in the build (`fadd` CPI 1.19,
`fmul` 1.14, ITCM-resident), while `ftMainProcUpdateInterrupt` runs at **11.53**
and `ftMainProcPhysicsMap` at **8.80** — and those two are exactly the `SINT`
and `SPHD` over-gate discriminators. `gcPlayDObjAnimJoint`'s hottest instruction
is `ldrb aobj->kind` at **24.1 cycles/execution** × 143,916: one D-cache miss per
AObj node per frame, ~360 live nodes against a **4 KB** cache. **Data layout and
working-set size are the lever; instruction count is not.** Rank by stall
(cycles − instructions), not by cycles — board has the table and the method.

## How the load frame is priced, and what is already closed

**106–108 priced the load frame** via `--split-by-symbol
ndsRelocFinalizeLoadedFile` (74 load frames vs 326 control, premium
650,610/frame): reloc + copy 23.6%, animation re-evaluation 24.3% and **real
gameplay work**, `armWaitForIrq` 21.1% idle. **`ndsRelocAssetIDForToken` is
CLOSED as a caching target**; still open is bounding its two scans by an
init-time `[min,max]`.

**Do not bring a micro-fix.** R2-06 E11's rule: *a load-frame-only saving of
~8,000 ticks cannot be banked through P95, because relinking moves the tail by
more than the saving.* Clear ~16,000 in one change, or **move the work off the
gameplay frame** — cycle 105's arena fix, cycle 108's prebake.

**Measure a placement-sensitive seam on ONE binary with a runtime route.** Two
separately-linked arms of cycle 108's prebake differed only by 3,584 bytes of
scratch and read P50 25,760 apart — 4.5× the cross-build floor — with the
*better* arm reading worse. `sample-tick-hud-buckets.ps1 -SetGlobals name=value`
pokes a `.data` global at the first frame-complete marker (standing rule 7); the
poke lands after ~3 warm steps, so an OFF arm is partial and must be scaled.

**Do not re-derive these.** The Makefile's `?= 0` defaults are not the shipped
config (41 overridden). `.text.hot` is closed in both directions
(`linker/nds_hot_text.ld:179-201`) and the Task 37 census sections C/D are a cost
ranking, never a placement prediction. Hoisting the animation range check in
`ndsRelocAssetIDForToken` was done by R2-06 E11 and lost
(`reloc_backend_assets.c:1840-1895`). These cost one null build to re-learn.

**Latent cliff, unowned:** `sNdsAObjEvent32NormalizedCount` reads **973 of 1,024**
after one minute; overflow silently **skips the animation attach**. 8 bytes/entry.

**The load-frame exclusion is REFUTED — do not apply it.** The owner's "loading
states excluded" bar must not go through the `SRC > 2x median` rule: it is
circular for SRC, swings the gap **3.08x**, and drops frames that are not loads.
Audit: `scripts/analyze-load-frame-exclusion.ps1`.

**Boundary for all of it.** Same geometry, textures and materials; the effect
models are a closed `BUGS.md` row the owner confirmed by eye. A change that
alters a visible pixel of the shield, revival platform, impact wave or reflector
needs the owner.

## Measurement rules this cycle established or re-proved

- **Per-bucket placement floor is ≥8,544**, not the ±5,376 for `WORK-H` P95.
  Judge on `WORK-H`; buckets locate, they never decide.
- **1.85 cycles of `FTR` mean per byte of added ARM text.** A change that adds
  text must beat its own footprint.
- **Verify a counter is live in the shipped configuration BEFORE the measuring
  run**, and **eliminate candidates with a liveness probe on an already-built
  ROM** before spending one. Six per-stop counters read 0 all match because they
  were proof-scoped.
- **`ALL` is VBlank-quantized** and hid a +52,928 that came straight out of the
  wait. Read `WORK-H`.
- **Do not multiply a number back by what you divided it by** — that agreement
  is circular and is not evidence.
- **Read a memo's Evicts, not its Hits** (cycle 107). Misses that are nearly all
  evictions mean undersized *or* mis-keyed, and the hit rate cannot tell you
  which — 8× the table moved one 41.8% → 50.1% while evictions stayed.
- **`--split-by-symbol` on an existing profile CSV is free** and partitions
  frames by whether they ran that symbol. It fully attributed the load frame
  with no build and no emulator run.
- **Read the caller before designing around a return value.** Cycle 108 built a
  zero-copy loader that `ftmain.c` could never use, because the caller discards
  the pointer. One `rg` on the call site would have cost nothing.

## Restart surface

All parked items live on the board's **Parked** list (one place, not two).

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Boundary contains only `battle_playable_realtime`, mode 163.

`docs/P1_EXECUTION_BOARD.md` is the only dynamic queue — rewritten cycle 79
from a 10,207-line log; history in
`docs/optimization/archive/P1_EXECUTION_BOARD_pre-cycle79.md`.
`docs/Smash64DS_Runtime2_SwitchPlan.md` is the charter. `docs/BUGS.md` carries
the owner's verdicts — they edit it directly, so preserve their wording.

A clean checkout must build through `build.ps1`, not bare `make`: four of six
generated `.inc` files are gitignored. For iteration, `make p1-tick` builds the
measuring ROM and `make p1` the published battle pair — bare `make` builds the
P2 ROM P1 does not ship. Never pass `-j`, never override
`MAKEFLAGS`, one build at a time. Never build a published target name for lab
work — those hardcode output to the project root whatever `BUILD=` says.
Preserve canonical mode 163, renderer mode 9, mip 0, static textures, source
countdown, Dream Land water at frame 0, Task 16 `1/1/1`. Do not edit `decomp/`.

Run `New-Smash64DSSnapshot.ps1` last, and nothing after it.

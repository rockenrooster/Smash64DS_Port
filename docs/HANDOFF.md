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

Re-banked cycle 80 on the corrected seed; both arms run the **same 60-second
match** (coverage 86.7%), both windows ending 43 frames past the buzzer.

| arm | role | `WORK-H` P50 | P95 | over gate |
|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE (owner, 2026-08-05)** | 1,112,576 | **1,447,318** | 754/1600 |
| **Boundary** mode 163 | shipped configuration | 1,082,112 | 1,476,672 | 673/1600 |

**Gate baseline is 1,447,318 as of `f082b3c8`, gap 326,938**, less cycle 108's
~23,000. Boundary inherits the same fixes and is not re-banked, so its 1,476,672
is stale-high. Slips 0 in every row. The soak's long match is
`NDS_R2_SOAK_MATCH_MINUTES` and `probe-match-window.ps1` reads the match timer
out of the guest, so a window can no longer claim coverage it did not have.

The owner's bar: the whole match under the P95 budget on the both-CPU config,
loading states excluded; the shipped ROM stays the Boundary hwtri pair.
`Makefile:305-308` still forbids reporting a both-CPU P95 as the Boundary
figure. **Re-pin `EXPECTED_CENSUS_SHA256` in the commit that changes what it
covers** — a stale pin kept the verifier red for 35 commits.

## Effect DObj submits are a BOUNDARY-arm diagnosis — do not re-brief it as the gate

`MISC` is 99.3% effect-submit excursion **on Boundary** but only **~12.1% of the
gate arm's `WORK-H` excursion**. G3's packet path was refuted on mechanism in
cycles 88–91; board carries it.

## What is dead, so nobody re-derives it

- **Projectiles** — weapon DObj submit medians **44 ticks/frame**; not the tail.
- **Particles** — flat ~47,000/frame, hot–cold delta 4,838. A P50 lever only,
  retiring SwitchPlan §7 option 2 (15 Hz round-robin) as a *gate* answer.
- **The force-load seam** — closed cycle 108; see the next-step section.
- **Texture thrash**, **`Find`**, **`Material`**, **`FTR` as the gate**.
- **Task 56 strips** — REVERT: **the ROM hangs the present loop**. The
  `PERF_LEDGER` KILL row citing `FTR` +5,824 has no completed run behind it.
- **L7 fixed-point collision** — +534 won against 6,481 lost to its own text.

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

## Next single step — animation RE-EVALUATION, not another caching layer

**The force-load seam is closed (cycle 108).** Pre-finalizing and handing back
the arena pointer is structurally impossible: `ftmain.c:4623-4624` **discards
the return value** and animates from `fp->figatree_heap`, so the destination
copy is mandatory — and `reloc_backend_assets.c:7396-7407` already encoded that
fact. Violating it reads as a different match, not as slow (`ForceLoadTotal`
353 → 3,210). A hit is now a mandatory ~2.3 KB memcpy plus ~21 pointer writes.
Board has the three facts the attempt paid for.

What is left on the load frame is **animation re-evaluation, 158,393/frame
(24.3%)** — real gameplay work. Attack it as specialization or a lower update
rate. Do not add another caching layer to the loader.

**The soft-float bill is mapped, and it is a BASE-cost lane — it lowers P50 and
P95 together.** `scripts/analyze-leaf-helper-attribution.py` attributes a leaf
helper to its callers off an existing profile (no build, no run): **8.9% of
non-idle work**, led by animation evaluation 2.57%, collision 1.79%, matrices
1.33%. `battleship_ftAnimParseDObjFigatree` and `gcPlayDObjAnimJoint` are
**5.34% counting self time, ~75,600 ticks at P95**. The helpers are already
libgcc ARM assembly in ITCM, so only call volume is available.

**Go after the ARITHMETIC, not the comparisons.** `fadd`+`fsub` 3.46%, `fmul`
2.24%, `fdiv` 1.04% — **6.74% against the compares' 1.32%**. The compare lane is
priced and closed: `include/nds/nds_fcmp.h` is bit-exact (proven over all 2^32
patterns by `scripts/check_fcmp_exact.py`) and applied at the largest caller for
a real but **sub-floor** −3,136 P50 / −4,739 P95, and the port-editable ceiling
for the *whole* lane is only ~0.5%. `-ffinite-math-only` does not remove these
calls — checked compile-only. Any conversion must also clear two paid-for
constraints: L7 lost on **text size** (1.85 cycles of `FTR` per added byte), and
`-mthumb` has no SMULL.

## How the load frame is priced, and what is already closed

**Cycle 105 removed the cartridge I/O; 106–108 priced what is left.** Every
remaining `SINT` spike is a force-load frame with payload/header reads **+0**;
worth **121,331 at P95**, of which the prebake took ~23,000. Attributed free via
`--split-by-symbol ndsRelocFinalizeLoadedFile` (74 load frames vs 326 control,
premium 650,610/frame): reloc + copy **23.6%**, animation re-evaluation
**24.3%** and **real gameplay work**, `armWaitForIrq` 21.1% idle.

**`ndsRelocAssetIDForToken` is CLOSED as a caching target** (board has the
table). Still open, unmeasured: bound its two 143/158-entry scans by an
init-time `[min,max]` rather than caching them.

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

## Standing: the load-frame exclusion is REFUTED — do not apply it

The owner's "loading states excluded" bar must not go through the
`SRC > 2x median` rule: it is circular for SRC, swings the gap **3.08x** across
thresholds, and drops frames that are not loads (100 of 122 isolated
singletons). Audit: `scripts/analyze-load-frame-exclusion.ps1`.

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

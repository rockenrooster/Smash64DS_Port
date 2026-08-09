# Handoff

Updated: 2026-08-09. **The gate arm's tail was cartridge I/O: the animation
cache arena had been full and refusing loads all match. Fixed at `f082b3c8` —
`WORK-H` P95 1,639,299 → 1,447,318, gap 326,938.** The campaign remains on
R2-07's performance gate. Published pair:
`smash64ds-battle-playable-hwtri.nds` `AFD28273…`, `smash64ds.nds`
`54C07FAC…`.

## Read this first: every 128-frame measurement in the archive is unusable

**The 128-frame window reads the cheapest 6% of the match** — Boundary frames
441–568 read `WORK-H` P95 1,156,992 (8.7% over gate) against 1,463,104 and 44.6%
whole-match, i.e. P95 understated ~306,000 and the over-gate rate five times.
`sample-tick-hud-buckets.ps1` takes repeated ring dumps (`-Samples` to 4096,
`-RingStopStride` 96, ROM byte-identical). Never take a gate reading on 128
frames again.

## The two baselines — label every figure with its arm AND its coverage

Re-banked cycle 80 on the corrected seed; both arms now run the **same 60-second
match** (coverage 86.7%, clock 52 s → 0 s, logic:presented 2.000), and both
windows end 43 frames past the buzzer in GAME SET.

| arm | role | coverage | `WORK-H` P50 | P95 | over gate |
|---|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE (owner, 2026-08-05)** | 86.7% | 1,112,576 | **1,447,318** | 754/1600 |
| both-CPU, pre-`f082b3c8` | superseded cycle 105 | 86.7% | 1,102,720 | 1,639,299 | 691/1600 |
| **Boundary** mode 163 | shipped configuration | 86.7% | 1,082,112 | 1,476,672 | 673/1600 (42.1%) |

**Gate baseline is 1,447,318 as of `f082b3c8`, gap 326,938.** The animation arena
was full all match and the tail was cartridge I/O; board carries cycle 105.
Boundary inherits the same fix and is not re-banked, so its 1,476,672 is
stale-high. Slips 0 in every row. The soak's long match is
`NDS_R2_SOAK_MATCH_MINUTES` and `probe-match-window.ps1` reads the match timer
out of the guest, so a window can no longer claim coverage it did not have.

The owner's bar: the whole match under the P95 budget on the both-CPU config,
loading states excluded; the shipped ROM stays the Boundary hwtri pair.
`Makefile:305-308` still forbids reporting a both-CPU P95 as the Boundary
figure. Both-CPU is only ~10% worse at P95 — harder, not a different animal.
**Re-pin `EXPECTED_CENSUS_SHA256` in the commit that changes what it covers** —
a stale pin aborted the Boundary profile in pre-flight and kept the verifier red
for 35 commits.

## Effect DObj submits are a BOUNDARY-arm diagnosis — do not re-brief it as the gate

`MISC` is 99.3% effect-submit excursion **on Boundary** but only **~12.1% of the
`WORK-H` excursion on the gate arm** (recoverable 33,699–75,264, not ~315,000),
and G3's packet path was refuted on mechanism in cycles 88–91: effect geometry is
the per-instance payload, and the integer painter-depth slot exists *only*
because the CPU owns the vertex. Every list already stops at its own `G_ENDDL`
(1,360 of 1,360, none at the 8192 cap) at 626 ticks per command, so there is no
overrun to fix either. Board carries all of it.

## What is dead, so nobody re-derives it

- **Projectiles** — weapon DObj submit medians **44 ticks/frame**; not the tail.
- **Particles** — flat ~47,000/frame, hot–cold delta 4,838. A P50 lever only,
  which retires SwitchPlan §7 option 2 (15 Hz round-robin) as a *gate* answer.
- **Texture thrash** (1 upload per ~1,408 frames, 0 evictions — `Tex` is entirely
  cache-**hit** cost), **`Find`** (0.44%), **`Material`** (0.25%, and it clears
  §3.11), and **`FTR` as the gate** (anti-correlated with the tail).
- **Task 56 strips** — REVERT: **the ROM hangs the present loop** (no presented
  frame 12 in 900 s against 10–13 in 30 s). The `PERF_LEDGER` KILL row citing
  `FTR` +5,824 has no completed run behind it.
- **L7 fixed-point collision** — +534 won against 6,481 lost to its own text.

## RAM: both budgets are near their floor — price a change before writing it

Two separate shortages, both real, both nearly spent:

- **Static/boot.** `scripts/check-boot-headroom.ps1 -Build <dir>` after every lab
  build (OK / UNPROVEN / OVER CLIFF, exit 1). Highest `fake_heap_start` proven to
  boot **`0x02294804`**, lowest proven to fail **`0x02294b24`**; the current gate
  arm links at `0x0228c004` for **34,816** proven. **Text counts as much as bss.**
  A failing arm never reaches presented frame 1 and the harness reports a timeout
  that looks exactly like a hung emulator.
- **`gSYTaskmanGeneralHeap`.** `gNdsTaskmanGeneralHeapFreeMin` is **42,136**
  against the anim cache's 32,768 `KEEP_FREE` (cycle 105). The two are coupled:
  freeing `.bss` lowers `fake_heap_start`, which enlarges the heap.

**The `Tex` (dl-pointer, bind-ordinal) memo is REFUTED** — built as approved and
reverted: 10,336 consults, **471 hits (4.56%)**, 7,517 evictions of 7,525 fills,
`Tex` ticks *up*; working set ~175 keys ≈ 6.3 KB.

## Next single step — one PRE-FINALIZED resident copy per warmed animation

**Cycle 105 removed the cartridge I/O; 106–107 priced what is left.** Every
remaining `SINT` spike is a force-load frame (per-frame probe, 5 of 30, no
exceptions either way) with payload and header reads **+0**, so a cache *hit*
costs 117,000–570,000 ticks. Worth **121,331 at P95** (capping `SINT` at its
median gives 1,325,987). Fully attributed in cycle 107, free from the existing
profile CSV via `--split-by-symbol ndsRelocFinalizeLoadedFile` (74 load frames vs
326 control, premium 650,610/frame): the reloc + copy family is
**153,252/frame (23.6%)** and is port-side overhead delivering bytes already in
RAM; animation re-evaluation is **158,393/frame (24.3%)** and is **real gameplay
work** — do not brief it as waste. `armWaitForIrq` takes 21.1% and is idle.

**`ndsRelocAssetIDForToken` is CLOSED as a caching target.** It is provably pure
and 100% of its cycles are on load frames, but three measured configurations
(64/512 evicting, 512 no-evict) topped out at a 50.1% hit rate with 10,405
evictions, and no-eviction filled all 512 slots with keys that never repeat.
E11's data composes with that: the 74.3% of calls resolving inside the chain are
the *cheap* ones, and the cost is the 14.3% that miss and walk both 143+158-entry
scans — exactly the unstable keys. Board has the table. Still open, unmeasured:
bound the two scans by an init-time `[min,max]` instead of caching them.

**Do not bring a micro-fix.** R2-06 E11's rule, re-proved twice this cycle:
*a load-frame-only saving of ~8,000 ticks cannot be banked through P95, because
relinking moves the tail by more than the saving.* Either clear ~16,000 of tail
movement in one change, or **move the work off the gameplay frame** — which is
what cycle 105's arena fix did for the I/O half. The shape: pre-finalize each
warmed animation once, so the force load returns a pointer instead of memcpy +
fixups + swap + normalize. Blockers to answer first: fixups write absolute
pointers from `loaded->data`; normalization writes `command->u` **in place**;
the destination is caller-owned via `lbRelocGetForceExternHeapFile(file_id,
heap)`. It must REPLACE the per-load destination copy — 197,184 more bytes do
not exist.

**Do not re-derive these; each is already documented where it lives.** The
Makefile's `?= 0` defaults are not the shipped config (41 overridden, every large
lever already on). `.text.hot` is closed in both directions
(`linker/nds_hot_text.ld:179-201`) and the Task 37 census sections C/D are a cost
ranking, never a placement prediction. Hoisting the animation range check in
`ndsRelocAssetIDForToken` was done by R2-06 E11 and lost
(`reloc_backend_assets.c:1840-1895`). Between them these cost one null build to
re-learn.

**Latent cliff, unowned:** `sNdsAObjEvent32NormalizedCount` reads **973 of 1,024**
after one minute. Overflow makes `ndsAObjEvent32NormalizeScript` return FALSE and
the caller then **skips the animation attach entirely**. 8 bytes an entry.

**The heap is spent.** `gNdsTaskmanGeneralHeapFreeMin` is 42,136 against the
anim cache's 32,768 `KEEP_FREE` — 9,368 of margin. Free `.bss` before taking
more; that lowers `fake_heap_start` and enlarges the heap the arena callocs from.

## Standing: the load-frame exclusion is REFUTED — do not apply it

The owner's "loading states excluded" bar must not go through the
`SRC > 2x median` rule: it is circular for SRC, swings the gap **3.08x** across
plausible thresholds, drops frames that are not loads (100 of 122 isolated
singletons), and moves the gate arm 3.08x against Boundary's 1.09x — no loading
filter could do that. Audit: `scripts/analyze-load-frame-exclusion.ps1`.

**Boundary for all of it.** Same geometry, same textures, same materials — the
effect models are a closed `BUGS.md` row the owner confirmed by eye and paid for
deliberately. Cheaper, never worse. A change that alters a visible pixel of the
shield, revival platform, impact wave or reflector needs the owner.

## Measurement rules this cycle established or re-proved

- **Per-bucket placement floor is ≥8,544**, not the ±5,376 that applies to
  `WORK-H` P95 — calibrated by an arm that only *removed* code and moved `FTR`
  +8,544. Judge on `WORK-H`; buckets locate, they never decide.
- **1.85 cycles of `FTR` mean per byte of added ARM text.** A change that adds
  text must beat its own footprint.
- **Verify a counter is live in the shipped configuration BEFORE the measuring
  run**, and **eliminate candidates with a liveness probe on an already-built
  ROM** before spending one. Six per-stop counters read 0 all match because they
  were proof-scoped; the GX flush and the OAM path came off the list for free.
- **`ALL` is VBlank-quantized** and hid a +52,928 that came straight out of the
  wait. Read `WORK-H`.
- **Do not multiply a number back by what you divided it by.** The "8192 × 12.54
  = 102,727 against 102,730" agreement was circular and was not evidence.
- **Read a memo's Evicts, not its Hits** (cycle 107). Misses that are nearly all
  evictions mean undersized *or* mis-keyed, and the hit rate cannot tell you
  which — 8× the table moved one 41.8% → 50.1% while evictions stayed.
- **`--split-by-symbol` on an existing profile CSV is free** and partitions
  frames by whether they ran that symbol. It fully attributed the load frame
  with no build and no emulator run, after an over-gate split had failed to.

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

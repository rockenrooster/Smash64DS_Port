# P1 Execution Board

Updated: 2026-08-01 04:20 Central

Boundary: `battle_playable_realtime`, mode `163`

## THE PARTICLE DRAW WAS 98% MISSING AND NOTHING SAID SO (2026-08-01)

Dream Land's bank landed, and landing it broke the common bank's draw. Four
both-CPU soaks on `build-pupupu`, each one a counter the previous run did not
have:

| run | emitted | missed | what it proved |
|---|---|---|---|
| 1 | 2,725 | 127,989 | the regression exists |
| 2 | 1,677 | 128,295 | re-admitting texture 0 did **not** fix it |
| 3 | 1,677 | 126,621 | `EFCommonID 0` **and** `PupupuID 0`; stride 128,278 |
| 4 | **109,560** | **2,084** | pointer-identity key; misses are Dream Land's 0/1 |

Two independent defects, and each masked the other:

- **`efParticleInitAll` resets `sEFParticleBanksNum`.** `InitAllCount 2`,
  `BankRegisterCount 3` — three bank loads across two resets, so two banks were
  handed slot 0 and every common particle took Dream Land's atlas stride. The
  key is `sEFParticleScriptBanks[slot]` now, which is the pointer the slot was
  registered with and cannot collide.
- **`QUAD_MEASURED_LIVE` was graded from a SINGLE-CPU mask.** Both-CPU is
  `0x08400007` — ids 0, 1, 2, 22, 27, not the `(22, 27)` on record. Texture 0
  fits only because atlas cells are capped at 16×16 now (`QUAD_CELL_MAX`,
  box-averaged); at 32×32 the shelf packer gives it a row of its own and wastes
  half of it. Then run 4's own numbers regraded the Dream Land half: 3,741
  strided draws with 2,084 misses at pre-stride 0 and 1, so all three of its
  textures are live, not just the sheet its two named scripts reference.

Admitted set is `{0, 2, 22, 27, 64, 65, 66}`, 6,400 of 8,192 bytes.

**Audio is CLOSED for a both-CPU match.** The same run: `FgmPlayCalls 194`,
`SupportedPlayCount 194`, `UnsupportedCallCount 0`, `PlayFailCount 0`,
**`MissRingCount 0`**. 271 `Magnify` and 368 `FoxWin` were the last two the ring
named; pack 700,892 → 707,300 B, 83 → 85 entries. Acoustic acceptance is still
the owner's ear.

**Mario's missing fireballs were the GObj cap, and the fix is 14,080 bytes.**
`SpawnCall 11 / SpawnSuccess 7` with `SpawnFailGObj 4 / SpawnFailPool 0`, so
`gcMakeGObjSPAfter` refused four times. Sampled at the refusal:
`sGCCommonsMaxNum 47` with 47 active and **`HeapFree 14,796`** against the
25,600 `ifCommonSetMaxNumGObj` threshold — the cap was latched for the whole
match. `sizeof(WPStruct)` is **704**, so `WEAPON_ALLOC_MAX = 32` spends
**22,528 bytes** on a pool whose measured high-water is **one**.
`NDS_R2_WEAPON_POOL = 12` returns 14,080 and the latch stops firing:
**`SpawnCall 16 / SpawnSuccess 16`, both fail counters 0.**

**This is the arena-margin constraint, paid down for the first time.** The same
latch is what aborted the L7 oracle ROM and what keeps the crowd actor default
off at `GENERALFREE 17,316`. 14,080 bytes is roughly three of the boot search's
4,096-byte steps, and it came from a pool the P1 milestone cannot use.

**Sudden Death entered on the fixed build** (`SuddenDeathPrepareCount 1`,
NO-FREEZE to Results) — the first soak in the campaign to reach it naturally,
because a match where every fireball spawns is a match that can tie.

### Five-minute stress soak on the fixed build — clean

`build-pupupu`, both-CPU, `-MinutesToRun 5.0 -PressStartEverySeconds 175`:

| | |
|---|---|
| verdict | **NO-FREEZE** |
| quads emitted / missed | **347,100 / 0** |
| fireballs | `SpawnCall 16 / SpawnSuccess 16`, both fail counters **0** |
| FGM | 282 play calls, **0 unsupported**, miss ring **0** |
| Sudden Death | `SuddenDeathPrepareCount 1` |
| GObj cap | `sGCCommonsMaxNum` −1 |

`RematchCount 0` — the START taps landed during Sudden Death rather than at
Results, so the rematch clause is still carried by the owner-confirmed
2026-07-31 lane (four battle entries, three Results screens) and wants one more
run on this content.

### The crowd actor RUNS, and the number that decides it is now measured

A seven-minute both-CPU soak with `NDS_IMPORT_BATTLESHIP_FT_PUBLIC=1`:
NO-FREEZE, `gNdsFtPublicActorMakeCount 2`, `CommonCheckCount 36`, **560,419
quads with zero misses**, FGM miss ring empty, no spawn refused.

It stays **default 0** anyway, and for the first time that is a measurement.
`gNdsTaskmanGeneralHeapFreeMin` — the battle-time low-water, sampled every
presented frame, because the soak's end-of-run `GENERALFREE` reads the Results
scene and cannot see a battle-time dip — read **23,544** with the actor on and
**26,876** with it off, one build apart on the same tree. The actor costs
**3,332 bytes**; the shipping configuration clears the 25,600
`ifCommonSetMaxNumGObj` threshold by **1,276** and no longer latches at all,
while the actor lands **2,056 under**. With it on the cap did latch — it simply
latched late enough to refuse nothing. `PROJECT_GOAL.md` ranks audio fidelity
first in the sacrifice order and gameplay fidelity above it, so the crowd yields
until ~2,100 more bytes are found. **`NDS_R2_WEAPON_POOL` is where the last
14,080 came from; the item pool is not a candidate — the port's
`itManagerInitItems` is already a no-op stub, so items cost nothing today.**

## THE FULL-CONTENT BASELINE — WORK-H P95 1,240,128, gap 120,128 (2026-08-01)

Every prior gate figure on this board was measured with the particles off. That
number cannot be optimized against any more, and the switch plan's own R2-07
review says so ("the current 1,208,960 baseline is particles-off; do not
optimize against it after enabling particles"). This is the replacement, taken
on the committed tree with both particle flags at their new default 1.

`artifacts/performance/r207-baseline-particles-on-128.json`, `git=85e43f4`,
`dldi=ON`, 128 settled frames (439..566), one stop via `-RingDump`:

| bucket | P50 | P95 | max |
|---|---|---|---|
| **WORK-H** | **926,336** | **1,240,128** | 1,492,928 |
| FTR | 376,896 | 380,800 | 382,016 |
| STG | 175,552 | 183,936 | 186,368 |
| SRC | **281,280** | **533,760** | 714,560 |
| MISC | 54,464 | 167,808 | 176,768 |
| BG | 3,840 | 3,968 | 4,096 |
| AUD | 2,432 | 3,648 | 128,384 |

VBlank intervals `2:453 3:98 4:11 5+:4`, max interval 19, 566 intervals total,
**cadence violations 0**.

**Gap to the gate: 120,128.** Particles cost about 31,000 of it (1,208,960 →
1,240,128), which is the honest price of six admitted textures drawing real
efcommon scripts, and it is *inside* the range the switch plan predicted for
cosmetic systems.

**The tail owner is unchanged and it is still `SRC`.** FTR's spread is 1.01 and
STG's is 1.05 — both are flat floors, and neither is where a P95 lever lives.
`SRC` runs a 1.90 spread and a **+252,480 excursion above its own median**,
which is twice the whole gap.

### The over-gate population, per frame — TWO owners, and the second one is new

The gate is a rank, so the useful comparator is the over-gate COUNT, not the
percentile: **18 of 128 frames exceed 1,120,000**, and P95 ≤ 1.12M needs that at
six or fewer. The 7th-highest frame is 1,263,808, so the tail has to come down
143,808 — or twelve frames have to leave the set.

Ranked from the run's own CSV, against the clean set's medians
(`FTR` 377,088 · `STG` 175,488 · `SRC` 276,032 · `MISC` 54,400):

| frame | WORK-H | FTR | STG | SRC | MISC |
|---|---|---|---|---|---|
| 475 | 1,492,928 | 375,552 | 175,680 | **658,944** | **131,776** |
| 517 | 1,431,872 | 372,672 | 184,128 | **714,560** | **135,552** |
| 495 | 1,411,200 | 376,896 | 175,936 | **533,760** | **173,888** |
| 544 | 1,375,104 | 377,600 | 175,616 | **629,824** | **165,952** |
| 542 | 1,353,536 | 370,432 | 176,192 | **649,600** | **131,840** |
| 478 | 1,330,368 | 380,096 | 185,024 | **571,136** | **167,808** |
| 519 | 1,263,808 | 374,592 | 186,368 | **507,712** | **170,752** |

**`FTR` and `STG` are flat on the worst frames — `FTR` is 1,536 BELOW its clean
median on the worst frame of the run.** The whole excursion is `SRC` (+250K to
+440K) and `MISC` (+77K to +120K), and they co-occur.

**`MISC` is not miscellaneous, and this is the new finding.** It is
`DrawTicks − (FTR + STG + BG + HUD)` plus the GX flush
(`taskman_seam.c:5008-5026`) — i.e. **everything drawn that is not a fighter,
the stage, the background or the HUD**: weapon DObjs, effect DObjs, particles.
Its clean value is ~54,000 and it steps to ~131,000 or ~170,000, two discrete
levels. That is the transient-combat-object draw path, and it is exactly the
generic `ndsRendererScanList` / `ExecuteDisplayListWithVertexCache` route that
Runtime 2 exists to delete — measured on an over-gate frame rather than assumed.

Counterfactuals from the same CSV, to size each owner honestly:

- return `MISC` to its clean median on all 13 elevated frames → over-gate
  **18 → 12**, P95 ≈ 1,216,960. **Not sufficient alone.**
- return `SRC` to its clean median → over-gate **18 → 1**. **`SRC` is the gate.**

So the order is: `SRC` first and `MISC` second, and `FTR`/`STG` are not on the
critical path for the gate at all however large they look in the P50.

**This retires an entire class of candidate, and Task 56 is the first casualty.**
`NDS_TASK56_FIGHTER_PRIMITIVES=2` compiles the fighter topology into strips —
47% fewer `VERTEX16` submissions, exactly the quantity Tasks 53/55 proved is the
only thing that moves the geometry floor — and it is still not a gate lever,
because it touches `FTR` and `FTR` is flat where the gate is decided. A matched
A/B was built and started on 2026-08-01 (control
`artifacts/performance/r207-t56-control-128.json`: `WORK-H` P95 1,249,600, 19 of
128 over gate, `FTR` P50 383,040) and **abandoned on that reasoning rather than
on a result** — its arm exceeded the sampler's 900 s ceiling twice, and the
expected value of a third attempt is a couple of frames at best against a lever
whose sign was already measured negative (`PERF_LEDGER`, *Task 56 … KILL*:
`FTR` +5,824, +1.0%). Recorded in the Makefile beside the flag.

Same test applies to every remaining renderer-side idea in
`optimization/OPTIMIZATION_IDEAS.md`: the fighter packet/DMA path, profile-0,
the stage-native flags. They are architecture work with real value for the
four-fighter future, and **none of them is the gate**.

## THE OVER-GATE PREMIUM IS DIFFUSE — measured, not inferred (2026-08-01)

`run-task37-profile-census.ps1 -SplitOverGate`, the partition the harness had
never exposed. **71 marked frames against 57 control, premium 474,032
cycles/frame**, ranked per symbol. Grouped:

| group | cycles/frame | of premium | of the real-work premium |
|---|---:|---:|---:|
| **idle** (`armWaitForIrq`) | 210,427 | **44.4%** | — |
| **tick-HUD console** | 64,100 | 13.5% | **24.3%** |
| soft float (`__aeabi_f*`, `sqrtf`, the div helpers) | 51,843 | 10.9% | 19.7% |
| asset load / reloc / FAT | 23,666 | 5.0% | 9.0% |
| animation (`ftAnimParse`, `gcPlayDObjAnim`, cubic) | 15,610 | 3.3% | 5.9% |
| **collision** | **13,944** | **2.9%** | **5.3%** |
| unattributed tail | ~94,442 | 19.9% | 35.8% |

**Two things this settles, and both contradict what the board has been acting on.**

**1. L6's "the over-gate frame is a hit-detection frame and 66.2% of its premium
is soft-float" does not survive on the current tree.** Collision is **2.9%** of
the premium. `func_ovl2_800ED490` is 3,790 cycles/frame and
`gmCollisionSetInvertMatrix` 2,710 — together less than `memcpy`. That
retrospectively explains the L7 result: the conversion won 534 cycles/frame
because 534 cycles/frame is the size of the thing it converted. **The estimate
was wrong, not the implementation.** Every "L7 should close the gap" figure on
this board descends from that misattribution.

**2. 44.4% of the premium is IDLE, which is the partition measuring the
quantum.** A frame that misses the gate costs an extra VBlank, and most of that
extra VBlank is `swiWaitForIrq`. So the raw premium overstates the recoverable
work by nearly half, and the honest denominator is the ~263,605 cycles/frame of
real work — of which **the largest single block is the tick HUD's own console**,
which the shipped profile-0 ROM does not run.

**So there is no lever, and that is now a measurement rather than an absence of
ideas.** After idle and the instrument, nothing exceeds 20% and the tail is a
third. This is the shape `PROJECT_GOAL.md` §"Sacrifice Order" and the
SwitchPlan's own "honest options" list were written for: the remaining moves are
cosmetic rate reduction, visual fidelity, and a **compensated 30 Hz simulation**
— and the plan reserves that last call for the owner in as many words
("substantially the same gameplay experience"). The uncompensated ceiling was
already measured at −119,744 P95 against a 120,128 gap.

**Do not run another point optimization against clause 4 without first
re-reading this table.** Five of the campaign's levers were picked from the L6
attribution that this refutes.

## CHECKPOINT — Latest green, both published ROMs rebuilt on the 83-cue pack (2026-08-01 05:10)

`verify-all.ps1 -Profile Latest` **passed**. Both published ROMs rebuilt from
that tree because the FGM pack changed under them:
`smash64ds-battle-playable-hwtri.nds` **11,896,832 B** with its flag-identical
tick-HUD sibling **11,898,880 B** — `check-published-roms.ps1` passed,
`check-tickhud-parity.ps1` reports **55 make flags compared, 2 allowlisted
differences, 0 drift**. `smash64ds.nds` untouched per the owner's standing
instruction. The +20,480 bytes are almost entirely the pack (682,036 → 700,892),
which is NitroFS card data and therefore costs no taskman arena.

Note the ordering trap this pass hit: the FGM checker validates the *pack*, not
the *header*, so three duplicate enum constants (`GuardOn`, `GuardOff`,
`GamePause` already existed in two other blocks of `gmsound.h`) passed every
audio check and were caught only by the Latest profile's compile. **A new cue's
enum needs a build, not just `check-audio-fgm-phase-pack.ps1`.**

## ONE BOTH-CPU SOAK SETTLED SIX ROWS, and three of them the wrong way (2026-08-01)

`soak-freeze-watch.ps1 -Build build-ftpublic -MakeFlags
NDS_IMPORT_BATTLESHIP_FT_PUBLIC=1 -MinutesToRun 3.5`, first ever build of that
flag. **Verdict NO-FREEZE**, 2,100 presented frames, one completed match into
Results. What it settled, in order of how much it changes:

**1. `NDS_IMPORT_BATTLESHIP_FT_PUBLIC=1` BUILDS, LINKS AND RUNS.** It never had.
The compile seam cost five declarations and one data table — see the commit and
`PORTING.md`. `gNdsFtPublicActorMakeCount 2`, `CommonCheckCount 30`: the actor
is created and the source's check runs.

**2. But it puts the arena UNDER the GObj latch.** `general heap free 17,316`
and the soak's own warning fired: *the GObj pool cap FIRED at 48*. It did not
crash — 48 was enough this time — but the flag cannot be defaulted on as it
stands. **Arena is now the binding constraint on closing the crowd row**, not
the cue set and not the actor.

**3. Five of the seven crowd counters CANNOT FIRE, and that is structural.**
`ProcUpdateCount`, `PlayCommonCount`, `LastCommonFGM`, `CallStartCount` and
`LastCallFGM` are absent from the ELF entirely — the soak's symbol guard dropped
them. The `#define` seam renames a decomp definition *and its intra-TU
references together*, so `ftPublicMakeActor` registers the **inner**
`battleship_ftPublicProcUpdate` and the wrapper that carries the counter is
unreferenced, then garbage-collected. Only `CommonCheck` survives because its
caller is outside the file. **This is the already-recorded "wrapping a decomp
function to count its INTERNAL callers" refutation, hit again by someone who
had read it.** Instrument the source's own statics — `sFTPublicCallCount`,
`sFTPublicCommonOrder`, `sFTPublicCallOrder`, `sFTPublicCommonALSound` — which
exist because the code writes them.

**4. The particle atlas coverage worry is NOT REAL on this match.**
`gNdsParticleQuadEmitCount 144,592`, `EmitMax 41` per frame, and
**`QuadMissCount 0`** across a full both-CPU match including Results. The six
admitted textures cover everything drawn. `gNdsParticleTextureUseMask` is still
`0x08400000` — **bits 22 and 27, two textures**. So "six of 31 admitted" is not
the open risk the VFX rows call it; the open risk is the *scripts that never
run at all*.

**5. Whispy's leaves and dust are refused, confirmed by id.**
`gNdsParticleRejectRing`: script **0** and script **1**, bank 0, **reason 2**
(script id out of range), twice each. Reason 2 on an empty bank is exactly the
Pupupu registration gap — `ndsParticleLoadEFCommonBank` covers the common bank
and every other bank takes `ndsParticleRegisterEmptyBank`. Nothing to do with
VRAM or the atlas.

**6. The fireball row REPRODUCED.** `SpawnCallCount 15` against
`SpawnSuccessCount 14` — **one request in fifteen produced no weapon**, on a run
that also reports `WeaponCountMax 1`. So at most one fireball is ever live, and
the refusal is the source's own concurrency limit meeting a second request.
That is a much narrower question than "sometimes they don't spawn".

Two more worth carrying: `gNdsR2AnimCacheArenaOverflows 109` on a single match,
and `gNdsRendererTask36ReplayArenaStaleCount 4,144`.

## R2-08 — the switch, reduced to a checklist (2026-08-01)

Written down now so the switch is a mechanical step when its one open
acceptance clause closes, rather than a re-derivation. §6 of the SwitchPlan
lists five acceptance items; four are reachable today.

| # | clause | state | what closes it |
|---|---|---|---|
| 1 | Boundary green on the Runtime 2 battle path | **reachable** | `NDS_R2_PATH := 1` in the published *and* tick-HUD blocks, then `verify-all -Profile Latest`. R2-06 E0 already ran Boundary green through `NDS_R2_PATH=1` with engagement verified in both ELFs (`ndsR2BattleRun` present/absent) |
| 2 | visual gate: synchronized diffs + the owner's approval | **partly** | the diffs are cheap; the owner's eye is owed on the particle draw either way |
| 3 | **P95 ≤ 1.12M DLDI-on** | **OPEN — the only real blocker** | see the baseline row above |
| 4 | full 3600-tick soak, zero flashes/corruption/hangs | **reachable** | `soak-freeze-watch.ps1` exists now; R2-06's "no soak instrument" note is stale |
| 5 | owner retail play test | **owner's** | explicitly outside the autonomous goal |

Two mechanical details that must not be rediscovered: the tick-HUD block has to
take `NDS_R2_PATH` too or every bucket after the switch reads a different binary
than the shipping ROM (the standing rule at `Makefile:1089`), and the switch
commit is also the one that updates the public-build pin. Runtime 1 stays behind
its flag as the oracle until the migration is declared mature.

**R2-06 E0 measured the switch itself as performance-neutral** (every bucket
inside the 5,000–7,000 placement floor), so flipping it neither helps nor hurts
clause 3. Do not wait for the switch to move the histogram; it will not.

## THE ARENA LATCH IS THE TREE'S REAL BUDGET LINE, and it is ~5 KB away (2026-08-01)

Third occurrence of the same failure, and the first one that was *provoked* on
purpose rather than stumbled into. `NDS_R2_COLLISION_L7_ORACLE=1` is a
read-only instrument that decides nothing — and building it aborts the ROM at
the GO countdown:

```text
lr_usr        ifCommonTrafficMakeSObj+68      the countdown storing through NULL
GENERALHEAP   free=20272                      threshold is 25,600
COMMONSMAX    45   COMMONSACTIVE 45            the cap FIRED, and it is sticky
MALLOCOVF     0                                the allocator is HEALTHY
```

So the instrument's own `.text` cost 5,328 bytes more arena than the tree has.
`ifCommonSetMaxNumGObj` caps the GObj pool when the general heap drops under
25 KiB, `gcMakeGObj` then returns NULL, and the countdown stores through it —
exactly the class `PORTING.md` recorded on 2026-08-01, reached this time by
adding a *diagnostic*.

**The durable form: `.text` costs taskman arena one-for-one, and the shipping
tree now has roughly five kilobytes of margin before the latch fires.** That is
the binding constraint on every remaining R2-07 item that adds code —
`NDS_IMPORT_BATTLESHIP_FT_PUBLIC` (still default 0 for this reason), a wider
particle texture set, and any lever that adds a fast path beside a slow one.
It is also why `soak-freeze-watch.ps1` reports `GENERALFREE`: read it before
adding code, not after the countdown dies.

**Do not re-run this oracle to answer L7.** Its answer already exists and is
recorded in `include/nds/nds_r2_collision_mtx.h` — 460 live samples on
2026-07-31, joint scale 1.1138–1.1199, deviation 2/5/21 in 1/4096 units at the
1/4/16-unit probes against a bound of 82, zero over-bound, zero singular. The
`Makefile` comment that still said the domain "has never been read off the
running game" is what sent this run; it is stale by a day and is corrected in
the same change as this row.

## R2-07 clause 2 — the particle INTERPRETER is clean; the DRAW is a texture-VRAM ORDERING bug (2026-08-01)

Three tick-HUD ROMs, identical but for the particle flags, one 2.5-minute
single-CPU soak each. This is the A/B that should have been run before any of
the ownership theorising:

| build | verdict | Violation | StageBuild | evidence |
|---|---|---|---|---|
| `build-tickhud-control` (no flags) | NO-FREEZE, 1 match | 0 | 2 | reuse 2041 |
| `build-tickhud-particles-runtime` (`RUNTIME=1`) | NO-FREEZE, 1 match | **0** | **2** | 14 scripts, 138,274 visible particles, structs 41/64, generators 8/12, 4 rejects |
| `build-tickhud-particles-draw` (`+DRAW=1`) | **ABORT at the GO countdown** | 1 | 197 | `ifCommonTrafficMakeSObj`, `MALLOCOVF=0` |

**So the 2,961-line interpreter costs nothing in correctness terms and lives
inside its fixed arena — §3.11 satisfied, measured.** Every symptom the board
had been chasing belongs to the DRAW flag alone.

**And it is not ownership, it is a contiguous VRAM run.** Raw capacity was never
short: 262,144 texture VRAM, 136,192 static, 57,344 for the interface's three
A3I5 atlases (256x128 + 128x128 + 128x64), 32,768 for the particle atlas, ~30 KB
spare. What is short is a *contiguous* 32,768 — libnds splits blocks per bank,
and this allocator has already refused a 4,096-byte upload with 268,800 free
(`PORTING.md`, second-entry corruption). Preparing the atlas between the static
set and the interface took the largest free run before the interface asked for
one. It is prepared **last** now, so the interface gets first refusal and the
cosmetic atlas fails closed.

**Two retractions.** The ownership exemption in
`ndsRendererHardwareRecordBattleStaticTextureHit` could never have fixed
ViolationCount 1 — that count is the stage's texture upload failing, not the
atlas binding. And the freeze diagnosis "heap exhaustion" was wrong: the PC was
parked in `__excpt_entry`, whose self-branch looks exactly like the allocator's
`while (TRUE);`. `soak-freeze-watch.ps1` now separates the two verdicts.

### The DRAW abort is the GObj LATCH, and the margin is 1,176 bytes

Not VRAM, not ownership. `ifCommonSetMaxNumGObj` (`ifcommon.c:3156`) caps the
GObj pool at whatever is active when the general heap drops under 25 KiB free;
past the cap `gcMakeGObj` returns NULL and the countdown's
`ifCommonTrafficMakeSObj` stores through it. Captured directly:

```
GENERALHEAP free=23032   COMMONSMAX=45   COMMONSACTIVE=45   SPRITESACTIVE=27
```

`COMMONSMAX` is `-1` until the cap fires, so 45 IS the fault. Deficit **2,568
bytes** against the draw's **+3,008 `.text`** (853K → 866K → 869K across the
three builds). Full write-up in `docs/PORTING.md`; **this is the second
occurrence** — the first was the source-sized particle pools.

**The number that matters is not 2,568, it is 1,176**: that is all the margin
`NDS_R2_PARTICLE_RUNTIME=1` alone leaves over the threshold. Any feature adding
more than ~1 KB of image trips this, silently, as a countdown crash with a clean
allocator. The soak reports `general heap free bytes` on every run now and warns
under 29,600.

Pools regraded to their measured high-water (41/8/2 → 48/10/6) returns 2,872
bytes, which clears it by 304 — enough to measure, not enough to build on. **The
standing fix is structural:** `ifCommonSetMaxNumGObj` is a no-op once
`gcGetMaxNumGObj() != -1`, so an explicit P1 GObj bound set at battle scene
start pre-empts the heuristic entirely. That needs one number first — the
natural `sGCCommonsActiveNum` high-water from an uncapped run, which the clean
counter list now reports.

### MEASURED — the draw's cost is NOT in the tick budget, it is 196 five-VBlank frames

All DLDI-on, 128 samples, same emulator sha, same tool, same session.

| | `l7-control` (git 800a934) | `tickhud-control` (HEAD) | `+PARTICLE_DRAW=1` |
|---|---|---|---|
| `WORK-H` P50 | 921,664 | 923,840 | 926,784 |
| `WORK-H` P95 | **1,208,960** | **1,294,976** | **1,221,760** |
| `FTR` P50 | 382,976 | 385,344 | 379,136 |
| `SRC` P95 | 523,008 | 546,112 | 522,176 |
| `MISC` P50 / P95 | 44,672 / 155,456 | 45,120 / 156,544 | **54,656 / 166,720** |
| VBlank 2/3/4/5+ | 461/91/10/4 | 457/96/9/4 | **287/81/2/196** |

**Read the histogram, not the P95.** The draw's own tick cost is the `MISC`
line — about **+10,000** — and `WORK-H` P95 is *better* than the control it was
built from. The pacing is destroyed anyway: **196 of 566 frames present at five
or more VBlanks**, against 4 in both controls. 196 ≈ the 197
`gNdsR2StagePrepareBuildCount` rebuilds the same run reports, so it is one
five-VBlank frame per stage-owner rejection. The 128-sample window sits in a
quiet stretch (`ALL` max 1,683,008 = three VBlanks), which is exactly why a P95
alone would have called this a win.

**Two control readings, and the P95 between them is not signal.**
`build-l7-control` re-measured with the current tool returns 1,208,960 and mean
973,484 to the digit, so the harness is deterministic and the attested figure
stands. Against it the current tree reads +86,016 P95 — from **+16 bytes of
`.text` and +64 of `.bss`**. At 1.85 cycles/byte that is 148 cycles, so the
86,016 is rank movement, not work: 17 → 18 frames of 128 over gate, on a tail
steep enough that one frame is tens of thousands of ticks. **Quote the
over-gate COUNT for this population, not P95** — the standing
rank-the-whole-distribution rule, in its sharpest form yet.

### NAMED — reason 2, 196 times: the stage's source texture will not resolve

Two instrument rounds, because the first one measured nothing. The existing
`gNdsRendererTask36*RejectReason` words are **latches reset at the top of every
prepare**, so an end-of-run read describes the last frame — both returned 0 on a
run whose battle had rebuilt 197 times. Counting versions
(`NDS_R2_STAGE_ROUTE_PROBE=1`) answer it:

```
gNdsR2StageKeyMissInvalid    197   Generation 0  Stamp 0  Config 0  Assets 0
gNdsR2StageRejectCounts[2]   196   [1] [3] [4] [5] [6] all 0
```

So the topology, config and asset bases never move: **every rebuild is the
previous frame's owner having rejected**, and every rejection is site 2 —
`ndsRendererHardwareResolveStageSourceFrameTexture` returning FALSE
(`nds_renderer.c:22413`). The chain is complete and measured end to end:

```
particle atlas takes 32,768 B of texture VRAM
  -> the stage's source texture stops resolving, ~1 frame in 10
  -> PrepareRun FALSE -> native stage owner rejects -> r2_prepared_valid = 0
  -> next frame rebuilds (197) and draws through the GENERIC renderer
  -> 196 of 566 frames present at five or more VBlanks
```

**VRAM CAPACITY IS REFUTED — halving the sheet moved the rejections by ZERO.**
`gNdsParticleTextureUseMask` reads `0x08400000`, which is bits **22 and 27**
(not 22/23/26 — read it as bits), so a live match draws two source textures
against the sixteen the static reachability set admits. A 128x64 sheet freed
**16,384 bytes** of texture VRAM and kept both, and the run came back
**196 rejections and 197 rebuilds, to the digit, identical**. So
`ResolveStageSourceFrameTexture` is not failing for want of bytes, and the
coverage cut (9 of 31 textures) bought nothing and is reverted rather than
kept. What survived the experiment is `QUAD_MEASURED_LIVE`: admitting the
measured set first is correct at any sheet size and is free.

**The last link was already instrumented.** R2-07 E2 ungated the texture reject
mask and a first-rejection cache census on `NDS_R2_STAGE_ROUTE_PROBE` for
exactly this chain; the symbols only had to be read:

```
gNdsRendererProfileTextureRejectReasonMask 4096   (TEXIMAGE)
census at the first rejection:
  Free 7   Live 41   Pinned 25   ThisFrame 16   Evictable 0
```

TEXIMAGE means `glTexImage2D` refused and the eviction retry then ran out of
things to evict. **Nothing is evictable**: of 41 live entries, 25 are pinned
(24 static + the atlas) and the other 16 were touched this frame. The control
runs the same working set with 24 pinned and does not reject, so the entire
difference is **one pinned entry and the VRAM block behind it**.

And since 128x64 rejected identically to 128x128, the shortfall is not the
sheet's byte count — it is where a 16-32 KB block lands in libnds's per-bank
splitting. The same allocator has already refused a 4,096-byte upload with
268,800 free (`PORTING.md`).

### FIXED — 64x64 (8,192 B). Stage builds 197 → 2, five-VBlank frames 196 → 4

One generator constant. The sheet is 8,192 bytes now and every symptom is gone,
to the control's own numbers:

| | control | draw 128x128 | draw 128x64 | **draw 64x64** |
|---|---|---|---|---|
| atlas bytes | — | 32,768 | 16,384 | **8,192** |
| `StagePrepareBuildCount` | 2 | 197 | 197 | **2** |
| `StagePrepareReuseCount` | 2,041 | 1,846 | 1,846 | **2,041** |
| `StageRejectCounts[2]` | — | 196 | 196 | **1** |
| `TextureRejectReasonMask` | — | 4096 | — | **0** |
| VBlank 2/3/4/5+ | 457/96/9/**4** | 287/81/2/**196** | — | 451/102/9/**4** |
| `WORK-H` P50 / P95 | 923,840 / 1,294,976 | 926,784 / 1,221,760 | — | 939,200 / 1,250,688 |
| `MISC` P50 | 45,120 | 54,656 | — | 55,232 |
| quads emitted / missed | — | 90,165 / 0 | 90,014 / 0 | **117,937 / 0** |

**So the particle draw costs ~10,100 ticks and five extra three-VBlank frames,
and nothing else.** The 5+ population is back to the control's 4. Quads emitted
went UP because the stage stopped falling back, and `QuadMissCount` is still 0 —
the six admitted textures (0, 3, 9, 22, 27, 37) cover everything this match
draws.

**8,192 bytes is now a MEASURED HARD BOUND, not a budget.** 16,384 rejected
exactly as 32,768 did, so more coverage cannot come from a bigger sheet; it has
to come from a second small atlas, a smaller per-texture format, or giving the
atlas its own VRAM instead of the cache's. Six of 31 textures is the open risk
for the remaining `BUGS.md` VFX rows (Whispy leaves/dust, Results confetti, the
KO burst) — and `gNdsParticleQuadMissCount` is what will say so, per effect,
rather than a silent gap.

### BOTH FLAGS DEFAULT 1 (2026-08-01)

`NDS_R2_PARTICLE_RUNTIME` and `NDS_R2_PARTICLE_DRAW` are on by default. A build
with no flag overrides soaks NO-FREEZE with `StagePrepareBuildCount` **2**,
reuse **2,041**, `sGCCommonsMaxNum` **-1** (the GObj cap never fired),
**114,523 quads emitted and zero missed**. Live HUD on that ROM:
`FPS 29.0`, `ALL 1,119,744`, `FTR 377,088`, `STG 175,168`, `MISC 54,656`,
`WORK 964,800 / 1,259,136` over n:128, `VBI 2:962 3:167 4:13 5+:4 max:19` —
screenshot `artifacts/visibility/2026-08-01_particle-draw-default-on.png`.

**Latest verification profile PASSES with both flags on**, and both published
ROMs are rebuilt from it: `smash64ds-battle-playable-hwtri.nds` (11,876,352 B)
with its flag-identical tick-HUD sibling (11,878,400 B, parity 55 flags /
2 allowlisted / 0 drift). `smash64ds.nds` is untouched per the owner's standing
instruction.

**Owed: the owner's eye.** The render-fidelity doctrine makes the owner the
visual oracle, and this is the first build where the real efcommon scripts draw
textured rather than as recoloured 16-vertex stand-ins. The pacing and
correctness evidence is above; what nobody has judged yet is whether the
particles LOOK right.

Until then `NDS_R2_PARTICLE_DRAW` stays 0: the draw is correct (90,165 quads,
zero atlas misses, NO-FREEZE, pools 41/48 and 8/10) and unshippable.

## R2-07 clause 2 — `NDS_R2_PARTICLE_RUNTIME=1` BUILDS, and its first boot names the real constraint: `.text` COSTS ARENA (2026-07-31)

**The build seam is closed.** The 790-error wall was one thing: the port's
`include/ft/fighter.h` and the decomp's `ft/ftdef.h` both declare **725
enumerators**, and `battleship_lbparticle.c` is the only TU that reaches both —
it compiles decomp sources in place while the decomp header web reaches the port
mirror through the shadowed names `ft/fttypes.h`, `sc/scene.h`, `it/item.h`,
`wp/weapon.h`. Angle-bracket includes inside decomp cannot be redirected
per-file, so the mirror had to be able to stand down.

Measured before writing the guard, which is what made it safe: **all 725 shared
enumerators hold IDENTICAL values**, and the port header declares them in
**nineteen** blocks (FTKind, FTPlayerKind, FTKeyEventKind, FTSpecialCollKind,
FTMotionEvent and fourteen anonymous status/motion/joint blocks). So
`SSB64_NDS_FTDEF_MIRROR` brackets all nineteen — **one macro, not nineteen** —
and it deliberately does not define itself, because a self-defining guard would
have silently dropped eighteen of them. Verified both directions textually before
building: guard off, the enumerator set is byte-identical to before (916); guard
on, 191 port-only enumerators remain and **zero** duplicate `ftdef.h`. The
default build never defines it (the TU is inside the flag's `ifeq`), so every
other translation unit is untouched. `nFTPartsJointNumMax` is the one port-only
enumerator inside a mirrored block and is split out — `FTPARTS_JOINT_NUM_MAX` is
defined from it.

**The board's earlier advice was half right.** "Prefer a block-level answer over
N single guards" — yes, one macro. But the premise that this is "a *series*, the
next type down, guarding them one at a time costs a full build each" was wrong:
the tag overlap is five names, the enumerator overlap is 725, and **nothing about
it needed to be discovered one build at a time.** Two greps and a value
comparison sized the whole thing before a single compile.

**AND THE FIRST BOOT FAILS — heap exhaustion, and the mechanism is not what the
memory budget said.** `soak-freeze-watch.ps1` on `build-particles-on`
(`artifacts/verification/freeze-soak/2026-07-31_161212-FROZEN-PICTURE.txt`):
spinning in `ndsSyMallocOverflowHalt` from `syTaskmanMalloc`, request **4,896
bytes** — exactly Dream Land's grpupupu bank — against **4,032 bytes of
headroom**.

| | runtime OFF | runtime ON | delta |
|---|---|---|---|
| `.text` | 886,840 | 910,864 | **+24,024** |
| `.data` | 135,152 | 135,240 | +88 |
| `.bss` | 1,710,696 | 1,711,080 | **+384** |
| taskman arena secured | 1,269,760 (`0x136000`) | **1,245,184 (`0x130000`)** | **−24,576** |
| arena search steps | 26 | **32** | +6 × 4,096 |

**`.text` growth costs taskman arena one-for-one, and that is not written down
anywhere.** The standing note says BSS competes with the runtime `calloc` that
sizes the heap; this build adds almost no BSS and still loses 24,576 bytes of
arena, because the whole image lives in main RAM and the downward search starts
above it. The arena landed **exactly on the `0x130000` floor** — the floor that
is a contract with the Task 36 replay guard and must not be lowered — so there is
no search headroom left at all.

So the deficit is **~24,576 (arena) + 4,896 (a bank allocation the stub never
made) against 4,032 spare ≈ 29 KB**, and **the "115,277 B of arena spare answers
the memory question" line is REFUTED**: the spare was never the binding
constraint, the arena *sizing* is.

### The named lever was REFUTED, the real one was 22,528 B away, and the arena freeze is FIXED (2026-07-31 evening)

**"The lever is the 82,752 B of packed DS textures currently linked into
`.text`" was wrong — they were never in `.text`.** `-fdata-sections` plus
`--gc-sections` had already discarded `gNdsParticleTextureData` (`0x14100`) and
`gNdsParticlePaletteData` (`0x2a0`); both sit under **Discarded input sections**
in `build-particles-on/.map`, because nothing references them until the quad
path exists. Moving them would have freed exactly zero. The pack's real image
cost is **12,148** (script bank 10,912 + texture rows 752 + offsets 476 + two
scalars), and the runtime's total is **24,728** with the interpreter's 12,540.
Check the map before believing a size claim about linked data nothing reads.

They were moved anyway, to `nitro:/particles/efcommon_particle_textures.ds.bin`
— not for size but because they become live the instant the quad path
references them, and at that point there is no image room and no cheap way
back. `check-nds-particle-banks.ps1` now fails if either array reappears as a
declaration.

**The lever that worked is `sNdsTask39HitSparkPixels`:** 22,528 B, the largest
single `.rodata` object in the image, DMA'd into OBJ VRAM once at prepare time
and never read again — and the generator was already writing byte-identical
content to `assets/effects/task39_hit_sparks.rgb5a1.bin`. Streamed back one
512-byte cell at a time with explicit `u32` stores (DMA cannot source from DTCM
where the stack lives; VRAM rejects 8-bit writes).

| taskman arena | before | after | delta |
|---|---|---|---|
| control, particles off | 1,269,760 | **1,290,240** | +20,480 |
| particles on | 1,245,184 | **1,269,760** | +24,576 |

`MALLOCOVF=0`. Particles-on now boots, loads the bank (`LoadResult=1`, 55
packed, 0 rejected), and **renders Dream Land with both fighters and a live HUD
at TIME 01:00** before dying at match start. Control soak NO-FREEZE through a
full match to Results; Boundary green.

### The next blocker, sized to the byte: the ORIGINAL'S OWN 25 KiB GObj latch

`ifCommonSetMaxNumGObj` (`ifcommon.c:3156`) freezes the GObj pool at the current
active count as soon as the general heap drops below **25 KiB free**. It fired
at 45. `ifCommonCountdownMakeInterface` then asked for the 46th, got NULL, and
the inlined `ifSetSObj` at `ifcommon.c:2222` wrote through it.

Read at `__excpt_entry`, before calico's handler double-faults (which is what
made the first capture unreadable — always break there, not on the frozen PC):

```
LR_ABT=0208cd14   -> str r0, [r5, #132], r5 = interface_gobj = NULL
COMMONS active=45 max=45
HEAP start=0x22b4870 ptr=0x23ea460 end=0x23ea870 free=1040
ARENA=1269760 steps=26   MALLOCS=843   BANKBYTES=10912 packed=55
```

**1,040 bytes free.** Not an allocator overflow — a heap that is full. Do not
add a NULL check; the owning seam is the heap. Three levers, each measured:

| lever | bytes | cost |
|---|---|---|
| `efParticleInitAll` pools: 112x96 + 24x92 + 80x192 (`efparticle.c:28`) | **28,320** | must be re-sized by measured high-water, and `StructsMax` was **0** at the crash — nothing had used them |
| bank arena copy (`memcpy` at `battleship_lbparticle.c:499`) | **10,912** | exists only because `ndsParticleNormalizeHeader` byte-swaps in place; normalize in the generator and point at `.rodata` |
| float `printf`: `.rodata.categories` 14,328 + `libc_a-svfprintf` 10,047 | **~24,375** | every diagnostic format string has to become integer-only |

Any two clear the latch. Trimming the reachable script set (55 packed, P1 seams
= 13 that reach the bank) cuts the linked bank **and** its arena copy, so it is
worth more than its 10,912 suggests. Do **not** answer it by lowering the arena
floor.

## R2-07 SUCCESSIVE MATCHES — CLOSED. Four battle entries, zero freezes (2026-07-31)

The owner's P1 bar, now written into `PROJECT_GOAL.md`: *"pressing start at
results screen restarts the P1 match, up to infinite successive matches"*.
Four defects stood in the way and **all four were state that outlived a scene
boundary the taskman arena rewinds** — the law is now `SwitchPlan §3.12` and the
full write-up is in `docs/PORTING.md`. In the order they were found:

| # | carrier | symptom | fix |
|---|---|---|---|
| 1 | stage prepared-run cache keyed on config POINTER + asset bases | second entry drew match one's stage | key adds `topology_generation` + `topology_stamp` |
| 2 | texture VRAM had no owner across the boundary | `glTexImage2D` refused 4,096 B against 268,800 free; run 42 failed `PrepareRun`; generic fallback, white pond, `STG` 2.76M, 4.2 FPS | `ndsRendererHardwareResetSceneTextureVram` at every battle entry |
| 3 | `gSYTaskmanDLHeads` never rewound in the battle loop | `used=61488` vs `len=61440` — **48 bytes** — `syTaskmanCheckBufferLengths` `while(TRUE);` froze match two ~8 s in | source's own `func_80004AB0()` once per presented frame |
| 4 | `sMNVSResultsFighterGObjs` trusted across a Results re-entry | dead GObj into `gcMoveGObjDL`; ARM9 ABORT mode, `lr_usr` at `ftParamMoveDLLink+18`; no Results screen at match two's GAME SET | cleared in `mnVSResultsStartScene` |

Evidence, all in-repo: SD lane **three consecutive bit-identical runs**
(`r2_prepared_valid` 1, BuildCount 4, ReuseCount 391 rising, key 2/`0xaaa3106e`
→ 3/`0x48ea3cde`, `STG` **169,536**, 28.0 FPS, pond textured —
`artifacts/verification/sudden-death/2026-07-31_14{2938,3102,3140}`); the
**same-binary control arm** `-NoTexVramReset` still reproduces the defect
(BuildCount **92**, ReuseCount frozen 303, `STG` **3,148,992**, 4.2 FPS, white
pond — `_143234`), which is what makes #2 attributable rather than coincident;
rematch lane **four battle entries / three Results screens / NO-FREEZE**, reset
count 4, prepare count 4, violations 0, `STG` 169,408, owner-confirmed
(`artifacts/verification/freeze-soak/2026-07-31_151642-NO-FREEZE.png`).

Permanent guards, no probes: `gNdsRendererSceneTextureVramResetCount` must read
one per battle entry, `gNdsR2StagePrepareBuildCount` two per entry with
ReuseCount rising. The `NDS_R2_STAGE_ROUTE_PROBE` census and refused-request
stash are **deleted** — they answered their question and one of their `glGlob`
reads was rightly refused by the native-stage field certificate.

**Owed on this row:** one Latest run and the owner's eye check on a natural tie
plus a rematch. Not owed: any further mechanism hunting.

This is the only dynamic P1 queue. `PROJECT_GOAL.md` owns the milestone and
fidelity contract. `HANDOFF.md` owns restart commands, `KNOWN_ISSUES.md` owns
durable gaps, `PERF_LEDGER.md` owns measurements and rejected experiments, and
`PORTING.md` is append-only history.

## Artifact Identity

Pinned public-build identity from `README.md`:

```text
smash64ds-battle-playable-hwtri.nds
11,428,864 bytes
SHA-256 4D795B4E83B335598B20A3B5953FDB1821797CC5E0A825FA96A0643ABBA4A090
```

Current local root artifact, rebuilt 2026-07-30 05:18 through `build.ps1` with
**R2-07 R0c+R0d** resident on top of E32/E64b/E65/E67/E69:

```text
smash64ds-battle-playable-hwtri.nds
11,512,832 bytes
SHA-256 5B6E82A8B8CA8D8AC903EBE30FFACF0F8E60BFD3A4A94C60A0C9182B3B1D0CCD
```

Same **11,512,832** bytes as the 04:46 R0c-only ROM but a different hash
(`152253A8…`) — R0d's two `always_inline`s are a wash on size, and this build system has
no reproducible ROM hash anyway (documented below). +1,024 bytes over the 2026-07-29
19:11 ROM (11,511,808 / `80CCD2EE…`), which is R0c's reciprocal-multiply sequences net of
the 18 bytes `ndsSpriteLerpPrimEnv` lost. **Latest** passed on both configurations (R0c/d
touch the startup-logo blitter path, so Boundary alone would not have covered them). The
matching flag-identical tick-HUD instrument is `builds/build-tick-hud-buckets/`
(**11,514,880** / `63B14923…`) — refreshed with this ROM, per the owner's 2026-07-22 rule
that the two move together; `builds/build-r0c-div/` and `builds/build-r0d-inline/` are the
two A/B arms. All three tick-HUD ROMs are 11,514,880 bytes and hash differently, which is
the already-documented non-reproducible-ROM-hash property, **not** a source discrepancy.

**`build.ps1` could not run with its own defaults until this build, and that is worth
knowing before anyone else tries to publish.** `-Jobs` defaults to `0`, meaning "let the
build tool decide" — the `make` path guards that with `if ($Jobs -gt 0)`, but the
BattleShip asset-extractor call passed `cmake --build --parallel 0` unconditionally.
cmake rejects it with a **usage dump and exit 1**, so `.\build.ps1 -Rom <rom>` failed in
`build-BattleShip-ExtractAssets` and read like a broken extractor rather than a bad
argument. Fixed by omitting the flag when `$Jobs -le 0`, matching the make path.

The tick-HUD instrument ROM was rebuilt with it, per the owner's 2026-07-22 rule
that the two move together — 11,514,880 bytes,
`E60467212F745086376B83A6108730BD357CAD48E3DC6BF4671BDA230A7B5DA6`, in both
`builds/build/` and `builds/build-tick-hud-buckets/`. It has no root output; that
target writes under `$(BUILD)` by design.

**This is a release candidate, not a release.** `README.md`'s public-build pin and
`build.ps1`'s `OUTPUT_SHA256` still name the audited published ROM, and they stay
that way: publishing needs the owner's authorization for that specific push, so
`build.ps1` will warn that the local build differs from the audited reference. That
warning is expected here and is not a source regression.

## FREEZE CLASS ROOT-CAUSED — heap exhaustion spins in `syMallocSet`'s `while (TRUE);` (2026-07-29)

The owner reported *"lots of freeze bugs that seem random"* plus *"sometimes hitting
a shielded player causes a freeze"*. **One cause explains the class.** Caught by
`scripts/soak-freeze-watch.ps1` on its first run, 3.5 minutes into the both-CPU
ROM. Capture: `artifacts/verification/freeze-soak/2026-07-29_202114-FROZEN-PICTURE.txt`
and `...-frozen.png`.

```text
#0  syMallocSet (bp=gSYTaskmanGeneralHeap, size=3472, alignment=16)
        decomp/src/sys/malloc.c:30   ->   while (TRUE);
#1  syTaskmanMalloc                  taskman.c:273
#2  ndsR2AnimCacheStore (size=3472)  src/port/reloc_backend_assets.c:5600
#3  ndsRelocForceLoadFighterAObj16File          ...:5850
#4  lbRelocGetForceExternHeapFile (llFTMarioAnimAttackAirDFileID)
#5  battleship_ftMainSetStatus (status_id=213, flags=8)
#6  ftCommonAttackAirCheckInterruptCommon
#7  ftCommonDamageFallProcInterrupt -> ftMainProcUpdateInterrupt -> gcRunAll
```

**`decomp/src/sys/malloc.c:30` is literally `while (TRUE);`.** The BattleShip
allocator does not return NULL on exhaustion, it hangs — an N64 assert reached for
real here. So *any* heap exhaustion in this port presents as a total freeze, with no
error and no recovery.

**The defect is port-side, at `src/port/reloc_backend_assets.c:5588`.**
`ndsR2AnimCacheStore` is a *speculative* cache that carefully handles
`payload == NULL` by bumping `gNdsR2AnimCacheRejects` and returning — and that NULL
check is **dead code**, because `syTaskmanMalloc` cannot return NULL, it can only
spin. An optional cache is calling a fail-by-hanging allocator on the shared
`gSYTaskmanGeneralHeap`, from inside a gameplay frame, and it never frees or evicts:
`sNdsR2AnimCacheCount >= NDS_R2_ANIM_CACHE_ENTRIES` bounds the entry COUNT and
nothing bounds the BYTES.

Why this is the class and not one instance:

- **"random"** — it depends on which animations have been triggered, so it depends
  on play. The heap fills gradually and the hang lands on whichever move needs a
  not-yet-cached asset.
- **"hitting a shielded player"** — a shield hit drives rebound/damage-fall, which
  interrupts into a new status, which triggers an on-demand animation load, which
  allocates. Shield hits are not special; they are a common route to an uncached
  status.
- **why no verifier caught it** — the scripted Boundary minute leaves Mario standing
  still and never triggers enough distinct animations. Two CPUs attacking
  continuously fill the heap in ~3 minutes. A separate `build-tick-hud-buckets`
  probe then timed out after 900 s waiting for a function that ROM reaches in ~90 s
  of guest time, which is very likely the same hang in the single-CPU config.

Every alternative mechanism is ruled out by the same capture: `REG_IME=1`,
`REG_IE=0x00070069`, `REG_IF=0` (interrupts enabled and serviced — not the
interrupts-disabled VBlank wait), `GXSTAT=0x06009700` (no FIFO stall — not a
geometry-engine deadlock), `IPCFIFOCNT=0x8505` (no IPC wait — **the audio/FGM
hypothesis is refuted for this bug**, despite the freeze-diagnostics breadcrumb set
being built around FGM enter/return). `COUNTERS=2006,447,894,0` — VBlanks 2,006
against 447 presented frames: the IRQ path alive, the main loop dead.

The frozen frame is worth looking at for one more reason: melonDS's title reads
**`[114/60]`**. The emulator was running *faster* than real time while the game was
dead, so no host-side FPS reading could ever have detected this. Only a guest
counter or the picture can.

### Addendum: the fix holds, and there is a SECOND exhaustion site with a 192 KiB cliff under it

Fix committed `e686675b` — the animation cache takes a 128 KiB static arena and
stops borrowing `gSYTaskmanGeneralHeap`. Verified so far:

- **Links.** `nm -S` puts `sNdsR2AnimCacheArena` at 0x20000 = 131,072 exactly, no
  BSS overflow.
- **Soak.** ~~Both-CPU ROM at HEAD past 6 minutes; pre-fix single-CPU tick-HUD 24
  minutes clean.~~ **BOTH WITHDRAWN 2026-07-29** — the detector was hashing the
  melonDS title bar, which carries the host FPS counter, so a hung ARM9 produced a
  fresh hash every poll. See the instrument section below. The **210-second pre-fix
  reproduction stands**: its capture shows `COUNTERS=2006,447,894`, i.e. real
  gameplay progress before the hang.
- **But the arena is not what saves battle start.** At `fkind=1` entry the cache
  holds `ArenaUsedBytes=1392`, `Fills=1`, `Overflows=0`. Its benefit is entirely
  mid-match. Re-confirmed unattached on 2026-07-29 at `ANIMARENA=1392,0` — **zero
  overflows**, so the arena is correctly sized for what it is asked to hold and is
  provably not the battle-start site.

**Second site, older than the fix and independent of it.**
`ftManagerSetupFilesAllKind` at battle start, on `NDS_R2_BOTH_CPU=1`:

| point | used | free |
|---|---:|---:|
| `fkind=0` (Mario) entry | 916,224 | 132,352 |
| `fkind=1` (Fox) entry | 990,640 | 57,936 |

Fox then requests **116,752** against **57,936** — short by **58,816**, on a
1,048,576-byte heap. First captured on a build with `NDS_R2_ANIM_CACHE=0` and no
arena at all, so it predates the fix. `NDS_FREEZE_DIAGNOSTICS` is excluded
explicitly: FREEZE=1 and FREEZE=0 builds overflow identically at the same site, and
the common factor is `NDS_R2_BOTH_CPU`.

**Under it is a 192 KiB granularity cliff, and it is ours.**
`src/port/diagnostics.c:7403` searches for the arena from `0x150000` downward in
`0x1000` steps, but the loop floor is `0x130000`; on total failure it drops to a
coarse list whose first entry is `0x100000`. Measured on this ROM:
`gNdsTaskmanArenaChosenSize = 1048576` with `gNdsTaskmanArenaAllocFailCount = 33`
— exactly the number of steps from 0x150000 to 0x130000, i.e. the whole fine range
failed and it **discarded 196,608 bytes in one step** against a shortfall of 58,816.
The comment directly above that loop records that a *coarser* version of this same
cliff was already fixed once for exactly this reason.

**MY AUTHORIZATION TO EXTEND THAT LOOP TO 0x100000 IS RETRACTED (2026-07-29).** It
was wrong on the merits and the implementer was right to hold. `0x130000` is not an
arbitrary floor: it is a **contract with the Task 36 replay admission guard**
(`src/nds/nds_renderer.c:4362`, documented at `include/nds/nds_renderer.h:124-134`),
which admits an arena only when `gNdsTaskmanArenaChosenSize >= 0x130000`. Every
value the extension would newly land — 0x100001..0x12FFFF — is below that guard, so
the change would convert a loud hang into a **silently disabled measured render
path**. That is the exact trade this campaign forbids.

The real defect is the two constants being written twice, in two files, with a
contract between them and nothing tying them together. Sizes, against the
1,245,184-byte (`0x130000`) admission floor:

| build | arena | above floor | replay |
|---|---:|---:|:--|
| published | 1,286,144 (`0x13A000`) | +40,960 | admitted |
| tick-HUD buckets | 1,277,952 (`0x138000`) | **+32,768** | admitted |
| `build-r2-bothcpu` | ≤ 1,245,183, reads 1,048,576 | below | **silently off** |
| documented tick-HUD | 1,359,872 (`0x14C000`) | +114,688 | — |

So **no published or measured P95 was taken with replay off** — the measurement ROM
clears the floor. But it clears it by 32,768 bytes, which is **eight fine-loop
steps**: 32 KiB of further BSS growth in the tick-HUD build would cross the floor
with no error, no log line, and a quietly different renderer. The actionable items
are therefore (a) name the threshold once and share it between the search and the
guard, (b) make a below-floor arena report itself instead of degrading silently, and
(c) find why the arena is 81,920 bytes short of its documented `0x14C000` at all.
Not (d) lower the floor.

**The shipped ROM is not affected at battle start.** Its fine search succeeds and
gives 1,286,144, against 990,640 used plus a 116,752 request = 1,107,392, leaving
~178 KB. So the owner's freezes were the mid-match cache exhaustion — which is what
he reported and what is now fixed — and the battle-start overflow is specific to a
stress config that is 192 KiB poorer. Worth fixing because that config is the soak
vehicle, but it is not a shipped-ROM defect.

**The GDB-attach confound is now SETTLED — there was none.** It was recorded here as
open because every battle-start number came from an attached run while the
*unattached* soak showed "a changing picture" for minutes, leaving a choice between
nondeterminism at the ~58 KB margin and attach-perturbed `calloc`. **Neither: the
changing picture was the host FPS counter in the window title.** With the
chrome-free hash, the unattached both-CPU ROM at HEAD reports `FROZEN-FROM-START`
inside 40 seconds, and its capture is the same site to the register —
`ftManagerSetupFilesMainKind` ← `ftManagerSetupFilesAllKind(fkind=1)`, `r5 = 116752`
requested against `r1 = 57936` free, `COUNTERS=849,0,0,0` (zero presented frames).
Battle start is deterministic, attaching perturbs nothing, and the site reproduces
3/3 unattached. Evidence
`artifacts/verification/freeze-soak/2026-07-29_210752-FROZEN-FROM-START.txt`.

`x/1i $pc` now prints `b.n <self>` at the top of every capture, which names this
whole freeze class from one line without a backtrace.

The remaining fix must be port-side (`decomp/` is read-only, the `while (TRUE);`
stays) and must be priced first — how much of the exhaustion is the cache versus the
underlying on-demand loads, because fixing only the cache when the loads also fill
the heap postpones the hang instead of removing it. Do **not** interpose
`syTaskmanMalloc` to return NULL globally: decomp callers do not all check, so that
trades a hang for a null deref. And note what a soak can and cannot verify: a
passive soak cannot reach a second match at all, because `mnVSResultsCheckExit`
(decomp `mnvsresults.c:266`) exits on a `START_BUTTON` tap with **no timeout** and
the DS results loop (`src/port/taskman_seam.c:6968`) is update-bounded only when
`NDS_HARNESS_FAST_LOGIC != 0`, which every shipped target pins to `0`. Cross-match
drift needs a synthesized START, not a longer run; the owner capped soaks at 5
minutes on 2026-07-29 for that reason.

## THE 128 KiB ARENA WAS A REGRESSION — a static buffer priced against the wrong build (2026-07-29)

The owner, on the first ROM built with the freeze fix: *"it never gets to the
battle, fix it."* Correct, and it was mine. **A static BSS buffer added to fix a
heap problem competes with the heap it was meant to protect**, and the
arena-search cliff turns that competition into a catastrophe.

`ndsTaskmanArenaBytes` (`diagnostics.c:7403`) picks the arena with a downward
`calloc` search from `0x150000` in `0x1000` steps, floored at `0x130000`, then
falls to a coarse list starting at `0x100000`. So **crossing that floor costs
196,608 bytes in one step, not the 0x1000 the loop implies.** Measured on the
shipped hwtri ROM:

| arena bytes | `ChosenSize` | fails | battle start |
|---:|---:|---:|:--|
| none (pre-fix) | 1,286,144 | 0 | starts |
| **131,072** | **1,048,576** | **33** | **HANGS** — 116,752 asked vs 58,024 free |
| 32,768 | 1,269,760 | 26 | starts |
| **16,384** (shipped) | **1,269,760** | 26 | starts |

The cruelty is that 131,072 was *affordable*: 1,286,144 − 131,072 = 1,155,072,
still above the 1,107,392 battle start needs. **The cliff, not the size, broke it.**

**Then 32 KiB failed too, by 32 bytes, and that is the transferable lesson: the
budget belongs to the TIGHTEST configuration, not the shipped one.** 32 KiB was
sized against the shipped build's 1,286,144 − 1,245,184 = 40,960 of headroom. The
tick-HUD target starts from 1,277,952 and has only **32,768**. Arena plus counters
is 32,800. Measured BSS settles it — and incidentally exonerates the flag that had
been blamed:

| build | bss | vs tick-HUD baseline |
|---|---:|---:|
| tick-HUD, no arena | 1,709,416 | — |
| both-CPU + 32 KiB arena | 1,742,216 | **+32,800 = the arena and nothing else** |
| shipped + 16 KiB arena | 1,719,464 | +16,416 |

So **`NDS_R2_BOTH_CPU` adds no measurable BSS**, and the both-CPU build's inability
to start a battle — recorded above as a "second exhaustion site, older than the fix
and independent of it" — was **this same regression**, not a pre-existing defect.
That entry is corrected: there is one site, not two. It reproduced with
`NDS_R2_ANIM_CACHE=0` because the arena is compiled in regardless of that flag.

**First genuine clean stress result in the campaign**, both configurations, with a
counter read that actually executed:

| ROM | match | presented | `SyMallocOverflow` | arena | anim-cache overflows |
|---|---|---:|---:|---:|---:|
| both-CPU stress | **completed (Results=1)** | 2,043 | **0** | 1,257,472 | 302 rejected safely |
| shipped hwtri | **completed (Results=1)** | 2,043 | **0** | 1,269,760 | 296 rejected safely |

Both arenas clear the `0x130000` floor, so Task 36 replay is admitted in both. The
302 overflows are the fix working: every one of them would have been a hang.

**But 16 KiB does NOT solve the underlying problem, and the next step is named.**
Two fills against 296 overflows means the cache is barely a cache. The measured
working set is 41 assets / 91,104 bytes, and BSS cannot afford that at any point in
this program. **Reserve it from the taskman arena once at battle start instead:**
that costs the boot-time search nothing, so it cannot cross the cliff, and it still
fits — 1,286,144 − 91,104 = 1,195,040 against the 1,107,392 battle start needs,
leaving ~87 KB. That is the fix that makes the cache do its job.

Owner also confirms **Sudden Death has its own issues**; a 2.5-minute soak entered
`ndsBaseSCVSBattleStartSuddenDeath` and sat in the renderer's native stage display
commit. Not diagnosed, and NOT the allocator class — the PC was a working `cmp`,
not a self-branch. Separate row.

### R2-06 E3 — the arena moves to the HEAP. Cache proven effective, P95 −43,904, gate still 40,448 short (2026-07-29)

The 16 KiB static arena was measured, not assumed, and it cost the gate: **WORK-H P95
1,096,768 → 1,204,352, +107,584, gate missed by 84,352**, with `Fills=2` against
`Rejects=44` and 76 overflows in 128 frames. A miss is not free —
`ndsRelocAssetLoadHeaderAndData` (`nds_reloc_assets.c:601-634`) does a real
`fopen`/`fread`/`fclose` through NitroFS, which on the owner's DLDI configuration is
SD I/O inside a gameplay frame. P50 moved only +14,080 while P95 moved +107,584: a
tail-shaped regression, exactly what a handful of frames doing file reads looks like.

**Fixed by reserving the arena from `gSYTaskmanGeneralHeap` instead of BSS**, at the
measured working-set size of 92,160 (41 warm assets = 91,104). Three properties make
that safe where BSS was not:

- **BSS cost is now +32 bytes**, three words, against 92,160 if it were static.
  Measured: tick-HUD bss 1,709,448 versus a 1,709,416 no-arena baseline, and
  `gNdsTaskmanArenaChosenSize` back to **1,273,856** — clear of the `0x130000` floor,
  so the cliff and the Task 36 replay guard are both out of the picture.
- **Reserved lazily on first store, not at battle start.** By then
  `ftManagerSetupFilesAllKind` has already taken the fighters' 116,752 bytes, so this
  can never be the allocation that starves battle start. A
  `NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE = 32768` reserve means it declines rather than
  consuming the last of the heap, and declining costs nothing — it degrades to the
  on-demand load the port used before the cache existed.
- **Heap rewind is detected, not hooked.** `syMallocReset` rewinds the bump cursor to
  `start` on a scene load, which would leave every cached payload pointing into
  memory the next scene reuses. `ndsR2AnimCacheArenaStillOwned` tests that the block
  still ends at or before the cursor — two pointer compares — and it is on the
  **read** path as well as the reserve path, because a hit on a stale entry is silent
  corruption and strictly worse than the hang this change set removes.

Cache now provably effective: `Rejects=0`, `ArenaOverflows=0`, `Hits` 35 → 79,
`ReservedBytes=92,160`, `ReserveFailCount=0`, `Invalidations=0`. **WORK-H P95
1,204,352 → 1,160,448, −43,904.** Evidence `artifacts/performance/r206-arena-heap-128{.json,-rows.csv}`.

**Still 40,448 over the gate and 63,680 over the stored baseline, and the residual is
NOT mostly new work.** Bucket P95 deltas against `r203-e69b-mtxcopy-128` (git
`0b39c1a`):

| bucket | Δ P50 | Δ P95 |
|---|---:|---:|
| `ALL` | −128 | **0** |
| `WORK` | +10,816 | +13,440 |
| `FTR` | +5,888 | +6,016 |
| `SRC` | +3,072 | **+30,912** |
| `WAIT` | −9,664 | **+63,488** |
| `OTHR` | −10,112 | **+63,104** |

`ALL` P95 is **identical** at 1,680,000, so presented pacing did not change and the
VBlank histogram is still median-3. `FTR` +6,016 is at the 5,000-7,000 placement
floor. `WAIT` and `OTHR` moved by nearly the same amount in the same direction, which
is a redistribution rather than added cost — and WORK-H is the P95 of a per-frame
*difference*, so it is not the difference of these P95s. **The one real regression to
chase is `SRC` +30,912.** Prime suspect, unproven: the imported `syMallocSet` wrapper
adds an out-of-line `ndsSyMallocWouldFit` call to **every** region's allocation, not
just the taskman heap, and the graphics heap is allocated from per frame. Cheap test
is to make that helper `static inline` and re-measure; do that before looking
anywhere else.

### R2-06 E4 REFUTED — the fit check was already inlined; SRC +30,912 is not the wrapper (2026-07-29)

E3 named the imported `syMallocSet` wrapper's out-of-line `ndsSyMallocWouldFit` call
as the prime suspect for `SRC` P95 +30,912, on the reasoning that the wrapper covers
**every** `SYMallocRegion` including the per-frame graphics heap. Tested by forcing
the fit test inline at the call site. **Refuted, and cleanly:**

| | ROM sha | `SRC` P95 | `WORK-H` P95 |
|---|---|---:|---:|
| out-of-line call | `8F0CDAAC…` | 471,232 | 1,160,448 |
| forced inline | `EAEDFED0…` | **471,232** | **1,160,448** |

**A different binary, and every one of the eleven sampled buckets byte-identical.**
GCC had already inlined the call at the only hot site — a non-static function defined
in the same translation unit still gets inlined at `-Os` while its standalone body is
emitted for external callers — so there was never anything to win. The change was
reverted (at equal cost, less code wins) and the refutation is recorded at the
function so it is not re-proposed.

Two things worth keeping from a null result:

- **This harness reproduces bit-exactly when the sampled path is untouched.** Eleven
  buckets identical to the tick across a genuine binary change is stronger determinism
  than the campaign's 5,000-7,000 placement floor implies. That floor is about code
  MOVING under the sampled path, not about run-to-run variance — so a delta measured
  this way, on one commit, is real.
- **`SRC` +30,912 is still unexplained**, and it is measured against a different
  commit (`0b39c1a`), so cross-commit placement is not excluded for it the way it is
  for E4's within-commit comparison.

### R2-06 E4b — DLDI-ON COSTS ~29,696 P95. Every earlier number is DLDI-off (2026-07-29)

**Owner: *"turning on DLDI in melonDS caused the performance regression. DLDI is needed
for parity with retail hardware."* That is the missing variable, and it settles this.**

Rebuilt R2-03 E69 at its own commit `4916656d` and re-measured it. Evidence
`artifacts/performance/r206-e69-recheck-128{.json,-rows.csv}`:

| E69, same commit, same code | `WORK-H` P95 | vs 1,120,000 gate |
|---|---:|---|
| as published, DLDI **off** | **1,096,768** | 6/128 over, margin 23,232 |
| rebuilt, DLDI **on** | **1,126,464** | **MISSED by 6,464** |

**Identical source. The +29,696 is the emulator's I/O configuration, not the code.**
DLDI changes how `nitro:/` is reached — NitroFS resolves through the SD-card driver
instead of the card ROM interface — so it prices real I/O latency into the frame.

**I got the cause wrong first and it is worth recording why.** I wrote this section up
as "the E69 baseline does not reproduce", blaming an uncommitted working tree, because
the sampler stamps `gitShort` from HEAD and that JSON reads `0b39c1a` (the commit
before E69). The code reproduced perfectly. What changed underneath it was the harness:
DLDI was forced on earlier the same day in `3eb9ecdb`, after every baseline in this
board had been captured. **A cross-commit performance delta is meaningless unless the
emulator configuration is identical, and nothing in the artifact recorded it.**

**Consequence 1 — DLDI-on is now the honest figure and the gate was already missed.**
The owner needs retail parity, so DLDI-off understates the real cost. E69 on its own
commit is 1,126,464. **The "margin 23,232" this campaign carried for several cycles —
and that R2-07's particle budget was sized against — was a DLDI-off artifact.** Every
P95 in this board captured before `3eb9ecdb` is optimistic by roughly 29,696 and should
be read as a lower bound, not a result.

**Consequence 2 — `SRC +30,912` is withdrawn. It is 1,600 BETTER.** Comparing
like-for-like (both DLDI-on):

| bucket | E69 rebuilt | current `c7052883` | delta |
|---|---:|---:|---:|
| `ALL` | 1,680,000 | 1,680,000 | 0 |
| `WORK-H` | 1,126,464 | 1,160,448 | **+33,984** |
| `WORK` | 1,284,544 | 1,313,792 | +29,248 |
| `OTHR` | 430,848 | 461,376 | **+30,528** |
| `WAIT` | 413,952 | 445,120 | **+31,168** |
| `SRC` | 472,832 | 471,232 | **−1,600** |
| `FTR` | 391,552 | 392,896 | +1,344 |
| `STG` | 180,544 | 180,672 | +128 |
| `MISC` | 120,128 | 120,448 | +320 |

So this session's real cost is **+33,984 WORK-H**, not +63,680, and it is **not in any
named gameplay or render bucket** — `FTR`/`STG`/`SRC`/`MISC` are all within ~1,300.
It is entirely `OTHR` +30,528 / `WAIT` +31,168.

#### CORRECTION (2026-07-29, same day) — the +33,984 is a P95 tail artifact. The body is +4,352 and the over-gate count IMPROVED

Both candidates below were closed without a build, and the answer is that there was much
less to explain than the P95 pair implied. **`P95` is a position in a sorted list**, and
reading only that position mis-stated this by a factor of eight.

| percentile | E69 rebuilt | current HEAD | delta |
|---|---:|---:|---:|
| P25 | 938,944 | 940,160 | +1,216 |
| P50 | 971,712 | 976,064 | **+4,352** |
| P75 | 996,928 | 1,001,088 | +4,160 |
| P90 | 1,068,288 | 1,076,800 | +8,512 |
| P95 | 1,126,464 | 1,160,448 | +33,984 |
| **frames over 1,120,000** | **9/128** | **8/128** | **−1** |

**The body cost is inside the noise floor, and by the gate's own criterion — how many
frames exceed 1.12M — the current tree is marginally BETTER than the E69 baseline.** The
+33,984 is one or two expensive frames' position in the sorted tail; with 128 samples P95
is index 120, so a single excursion appearing there moves it by tens of thousands. This is
the same lesson E6 taught with two ROMs of identical arithmetic differing 23,040 at P95.

**Do not compare these two runs frame-by-frame.** Paired by index they read "worse on 86
of 128" with P10 **−89,088** and P90 **+115,712** — a ±100,000 spread, because the
anim-cache changes *when* loads complete and the two runs are in different game states at
the same frame index. Order statistics are comparable across these ROMs; paired frames
are not. (E6's paired comparison WAS valid: those two ROMs ran the same simulation.)

**Candidate 1, residual SD reads: REFUTED, no build spent.** All counters are plain
globals, so one run on the HEAD ROM answered it. Over the whole run to frame 924:
`gNdsR2AnimWarmFailed=0` (39 assets, 84,096 bytes, warm list complete),
`gNdsR204AnimForceLoadTotal=81 / Distinct=29 / Repeat=52` (unchanged from Task 73's
82/29/53), `gNdsR2AnimCacheHits=79`, **`Misses=2`**, `Fills=2`, `Rejects=0`,
`ArenaOverflows=0`, arena used 87,824 of 92,160. **The cache serves 79 of 81 force-loads;
exactly two file loads reach the SD driver in the entire run.** Two `fopen`/`fread` cycles
cannot set a P95 — they land on two frames out of 128.

**Candidate 2, heap layout: no longer worth a build.** It was the explanation for a
33,984-tick body cost that does not exist. The arena also cannot usefully shrink — it uses
87,824 of its 92,160 — so the experiment would trade a real freeze guard for a
noise-floor measurement.

**What this leaves.** The gate is still missed: **P95 1,160,448 against 1,120,000, 8 of
128 frames over.** But this session did not cost it 33,984, and the campaign's remaining
gap is the same tail R2-03 E35 already named — `SRC` P95 471,232 against P50 308,992
(spread 1.53) and `OTHR` P95 461,376 against P50 162,496 (spread 2.84). **The body is
already close; the work left is the excursion, not the average frame.**

Superseded text follows for provenance. The two candidates as first written:

1. **Residual cache misses that are now SD reads** — a warm-list gap becomes a full
   `fopen`/`fread` through DLDI. *Refuted above: warm failures 0, misses 2.*
2. **Heap layout** — the 92,160-byte reservation shifts every later allocation and
   re-maps the working set across cache sets. *Moot above: there is no body cost to
   explain, and the arena cannot shrink.*

**Two standing corrections to how this campaign records evidence, both machine-fixable:**

- **An artifact must record the emulator configuration it was captured under.** DLDI is
  the proof: a 29,696-tick swing with identical source, invisible in every JSON. The
  sampler should stamp the DLDI setting (and anything else `Set-MelonDSAutomationProfile`
  pins) beside the ROM hash.
- **`gitShort` stamps HEAD, not the working tree.** E69's JSON names the commit before
  the change it measures. Either stamp `git status --porcelain` alongside it or refuse to
  write an artifact from a dirty tree.

### R2-06 E10 first pass — the animation setup is NOT it either. 74% of the premium is still unattributed (2026-07-30)

E8 said the relocation is 21.5% of the load-frame premium and the other ~78% is "the action
change". This prices the action change's two obvious costs, and **they are not it.**
`NDS_R2_LOADFRAME_TIMING=1` (new, lab default off) brackets the two already-interposed
animation-add wrappers; counters differenced across frames 797..924. Evidence
`artifacts/performance/r206-e10-loadframe-128{.json,-rows.csv}`.

| | calls | ticks | per call |
|---|---:|---:|---:|
| `gcAddAnimJointAll` — **the action changes** | **7** | **101,376** | **14,482** |
| `gcAddDObjAnimJoint` | 320 | 192,128 | 600 |
| — O2R script normalize | | 72,000 (37.5%) | |
| — decomp setup | | 76,288 (39.7%) | |
| — bracket + admit residual | | 43,840 (22.8%) | |

**Only 7 action changes happen in 128 frames**, against 16 load frames — so a load is not
one-to-one with an action change, and 9 of the 16 load frames have no animation setup at all.

| mechanism | ticks | share of the 2,225,152 premium |
|---|---:|---:|
| relocation (E8) | 478,080 | 21.5% |
| action-change animation setup | 101,376 | 4.6% |
| **attributed** | **579,456** | **26.0%** |
| **UNATTRIBUTED** | **1,645,696** | **74.0%** |

**`gcAddDObjAnimJoint` is body cost, not premium.** 2.5 calls/frame at 600 ticks is ~1,501
ticks on *every* frame, so it does not belong in the premium column at all — counting it there
would have inflated the attribution to 31% on a quantity that is flat across load and clean
frames alike. Worth stating because it is the same self-vs-premium confusion Task 78 made in
the other direction.

**Frame 909 settles it independently.** It is 1,629,568, roughly +650,000 over the clean
median. Seven action changes averaging 14,482 cannot produce that on one frame, and neither
can 18 relocations averaging 26,560.

**Stop naming functions and sample.** Three mechanisms have now been priced by hand-placed
brackets — fighter fallback (0), effects (4 sparks), relocation (21.5%), animation setup
(4.6%) — and the majority of the premium is in none of them. The next instrument must be a
**sampling profiler with its window pinned to the load frames**, i.e. `NDS_TASK37_PROFILE=1`
over frames the Task 75 ring has already identified (843, 869, 890, 898, 909, 924), against a
matched control drawn from the 112 clean frames. E53 ran that instrument but chose its window
by WORK-H alone and could not separate draw from update; now the frames can be chosen by the
load marker and the bracket ownership is known to be `SRC`.

### R2-06 E10 second pass — the premium is now FULLY ATTRIBUTED, and it is the load pipeline entire (2026-07-30)

Evidence `artifacts/task37-census/r206-e10/{census.txt,census.json,arm9-profile.regions.csv}`,
DLDI **on**, window 796..923, `NDS_TASK37_PROFILE=1` with
`NDS_TASK37_PROFILE_PER_FRAME_REGION=1`. The raw 169 MB per-PC CSV stays on disk uncommitted;
`census.json` carries every row cited here.

**The frames are partitioned by the profile itself, not by a previous run's ring.** A region is
a load frame iff it executed `ndsRelocFinalizeLoadedFile`. That found **16 of 128** — the same
16 E8 counted — and the region ids map to frames 809, 828, 843, 847, 863, 864, 869, 870, 885,
890, 896, 897, 898, 907, 909, 924. Cross-check: E8's over-gate list was 809, 842, 843, 869,
890, 898, 909, and every one of those except 842 is in this set, which E8 had already explained
as *adjacent to* load frame 843. **Two independent instruments, one partition.** Region totals
are also VBlank-quantized multiples of 560,190 (frame 909 = 8 VBlanks, the clean median = 4),
which is the third confirmation that region r accumulates presented frame `START + r`.

**Region cycles are `ALL`, so two subtractions come first.** The raw split reads 2,941,011 per
load frame against 2,320,749 clean, a premium of **620,262** — but `armWaitForIrq` accounts for
**247,439** of that on **+4 instructions**, which is quantization slack, not work; and the
tick-HUD's own printf family (15 symbols, `_svfprintf_r` and friends) accounts for **45,917**,
which is the `HUD` bucket `WORK-H` subtracts and is an instrument artifact — bigger numbers
print more digits. **Work premium = 326,906/frame.**

| mechanism | ticks/frame | share of the 326,906 |
|---|---:|---:|
| relocation family (17 symbols) | 121,012 | 37.0% |
| — `ndsRelocFinalizeLoadedFile` | 54,168 | 16.6% |
| — `ndsRelocAssetIDForToken.part.0` | 39,475 | 12.1% |
| — the other 15 | 27,369 | 8.4% |
| `battleship_ftAnimParseDObjFigatree` | 42,450 | 13.0% |
| animation setup + cubic + float helpers | ~61,000 | ~19% |
| `memcpy` (the cache-hit payload copy) | 15,670 | 4.8% |
| `ftMainSetStatus` + fighter-part invalidation | ~25,400 | ~7.8% |
| **all positive work rows, 513 symbols** | **349,268** | **107%** |

**107% means attributed, not over-counted** — the negative rows make up the difference, and the
answer to E10 is that **there is no 74% lever**. The premium is the on-demand asset-load
pipeline as a whole, spread over hundreds of symbols. E10's first pass was not wrong about its
two mechanisms; it was wrong to expect a third one of similar size to hold the rest.

**Two of the three biggest contributors had never been named in this campaign**, and both were
invisible to hand-placed brackets because nobody suspected them.

**`ndsRelocAssetIDForToken` is the cleanest lever the phase has found.** Entry-PC counts:
**630 calls across the 16 load frames and exactly 0 across all 112 clean frames**, at
**1,003 cycles / 550 instructions per call**, 39.4 calls per load frame. It cannot regress a
clean frame's work because it never runs on one. Note the correction it forces: Task 71 priced
this chain at 9,306/load frame, but `ndsRelocMarioBattleAnimAssetIDForToken`,
`ndsRelocFoxAnimAssetIDForToken` and `ndsRelocIsMarioFoxAnimID` are **not separate FUNC symbols
in the ELF** — GCC inlined all three into `.part.0`, so Task 71 priced the compare chain and
not the two linear array scans behind it. The real figure is **4.2x** its estimate.

**Task 74's veto is still binding on its own approach.** The comment at
`reloc_backend_assets.c:1796` records that a 512-byte direct-mapped memo measured **+11,584
WORK-H P50** and moved `STG` by 8,128 — layout, not logic — and asks for an instrument
resolving below ~8,000 ticks before re-attempting. That precondition is now met at ~1 tick
resolution. What is *not* relicensed is adding data: the failure mode was 512 bytes of
`.main.bss` shifting placement on all 128 frames to save on 16. **Any retry must add zero
bytes** — the tokens are link-time constants, so 39.4 calls/frame resolving the same handful of
tokens is the thing to attack, at the call sites, not with a table.

**`battleship_ftAnimParseDObjFigatree` is a different and larger prize.** Unlike the token
chain it runs on **every** frame: 100.8 calls/frame clean (27,308 ticks) against 142.2 on a
load frame (69,758), 491 cycles per call. So ~27,000/frame is body cost that P50 pays too. It
is not a load-frame fix and must not be judged as one.

**Method note for the next task.** `scripts/run-task37-profile-census.ps1` now takes
`-PerFrameRegion` (default on) and `-SplitBySymbol`, and stamps DLDI. The split partitions the
census frames by a marker symbol *the profile itself observed* and ranks every symbol by
per-frame delta, with the instruction delta beside it: **same instruction stream costing more
is a cache effect, more instructions is real work.** That column is what separated the three
real mechanisms from the 495-symbol tail. It also refuses to run when the marker is absent from
the ELF, because a partition keyed on an inlined or deleted name silently classifies every
frame as a control frame — the `addr2line` trap in a new costume.

### R2-06 E12 — the animation half is 59.4% STALL, and the one compute-bound symbol is the one already fixed (2026-07-30)

E11 closed the load-frame route. This opens the body route, which is the one the E11 wall does not
apply to: body cost is paid on **every** frame, so cutting it moves P50 and P95 together. Same
evidence set, `artifacts/task37-census/r206-e10/census.json`, whole 128-frame window, DLDI on.

| symbol | ticks/frame | insn/frame | cyc/insn | stall | tier |
|---|---:|---:|---:|---:|---|
| `ndsR2CubicValueFixed` | 48,623 | 28,048 | **1.73** | **42.3%** | `.main` |
| `gcPlayDObjAnimJoint` | 40,973 | 14,018 | 2.92 | 65.8% | `.text.hot` |
| `battleship_ftAnimParseDObjFigatree` | 32,614 | 10,419 | 3.13 | 68.1% | `.main` |
| `ftParamUpdateAnimKeys` | 12,139 | 3,959 | 3.07 | 67.4% | `.main` |
| `gcParseMObjMatAnimJoint` | 7,627 | 1,745 | 4.37 | 77.1% | `.main` |
| `gcPlayMObjMatAnim` | 2,422 | 917 | 2.64 | 62.1% | `.main` |
| `gcAddDObjAnimJoint` | 1,055 | 97 | 10.90 | 90.8% | `.main` |
| `lbCommonAddFighterPartsFigatree` | 695 | 125 | 5.56 | 82.0% | `.main` |
| **family** | **146,148** | **59,329** | **2.46** | **59.4%** | |

**146,148/frame independently reproduces E60's caller-attributed 146,942** by a completely
different instrument, which is the strongest confirmation that figure has had. **86,819 ticks per
frame of it is stall** — the family executes 59,329 instructions and waits for 86,819 cycles.

**Against the switch plan's own frozen budget, this is the whole gate miss.** §4 allots **100K to
"fighter visual pose / animation"** (and a separate 150K to "60 Hz gameplay core, two logical
ticks"). Animation measures **146,148, i.e. 46,148 OVER budget — larger than the 40,448 the gate is
missed by.** So the milestone does not need a hunt for 40,000 spread across the frame: **bringing
one over-budget subsystem back to its declared number clears the gate with ~5,700 to spare.** That
is §3.1 "design backwards from the budget" applied literally, and it is the first time this campaign
has had a single subsystem that accounts for the entire miss.

Note the budget lines also *require* the §3.6 split: 146,148 currently conflates the visual pose
with the gameplay joint work that collision reads, and the plan charges those to two different
lines. Attributing the gameplay share to the 150K gameplay-core line is part of the work, not an
accounting trick — it is how the two budgets in §3.2 are supposed to be read.

**The pattern across the whole campaign now has one explanation.** `ndsR2CubicValueFixed` is the
only member near compute-bound at 1.73 cyc/insn, and it is exactly the one E64b/E65/E67/E69
successfully optimized — fixed-point arithmetic wins on a compute-bound kernel. Everything that
has been **refuted** removed instructions or added data and never improved locality: E6's memo
(+7,168), E53's mirror (+11,584), E64 arm A's cache, E66's `.text.hot` (+24,448), E11's
compare-chain hoist (+15,744). The tier table says why — `.main` runs at **3.52 cyc/insn: 43.5%
memory stall, 28.0% non-mem stall, ~28% actual work**, and `gcPlayDObjAnimJoint` is **already
resident in `.text.hot` and still 65.8% stall**, so its stall is *data*, not code. Placement
cannot reach it and neither can instruction count.

**Scale.** 86,819/frame of animation stall against a 40,448 gate gap. Recovering even half of it
clears the gate from body cost alone, on every frame, with no exposure to the E11 wall.

**Unpriced hypothesis — measure before believing it.** R2-06 E6 established (board §"E6") that the
battle task passes `aobjs_num = 0`, so `sGCAnimHead` starts empty and `gcGetAObjSetNextAlloc`
(`decomp/.../objman.c:602`) `malloc`s each of ~300 live 32-byte AObjs **individually**, and the
per-frame walk follows their `next` pointers. That is a pointer chase over ~300 scattered heap
blocks every frame, which is the shape that produces exactly this stall signature. E6 recorded
"nothing can index them" as a *blocker*; read as a *diagnosis* it is the most likely cause, and
the engine's own non-zero `aobjs_num` path allocates the set contiguously, so the fix may be
mechanically equivalent by construction rather than an approximation.

**That hypothesis is REFUTED, by the check it asked for, before any build was spent.**
`syMallocSet` (`decomp/BattleShip-main/decomp/src/sys/malloc.c:12`) is a **pure bump allocator** —
`bp->ptr = aligned + size`, no free list, no reuse — so a burst of `syTaskmanMalloc(32)` calls
returns **contiguous** blocks. `gcAddAnimJointAll` allocates a fighter's AObj set in one call, and
E10 counted only 7 such calls in 128 frames, so there is little to interleave between them. **The
AObjs are already contiguous and there is no scatter to fix.** "Nothing can index them" (E6) is
true of the *code*, not of the *addresses*. Do not propose an AObj pool.

**What the arithmetic actually says.** 301 AObjs x 32 bytes = **9,632 bytes**, and the ARM9 dcache
is **4 KB**. A contiguous walk that long **evicts itself every pass**, so contiguity buys the line
fills being sequential and buys nothing else — the set cannot be resident. And 9,632 bytes is far
too small to explain 86,819 stall cycles: at a generous ~20 cycles per 32-byte EWRAM line fill,
86,819 implies **~4,300 line fills, order 139 KB touched per frame** (assumption stated because
mem-stall also counts write-buffer drains and uncached accesses, so treat it as an upper bound).
**The working set is the whole animation graph** — DObj joints, MObjs, script payloads, matrices —
not one array, and it is more than 30x the dcache.

**E13 step 1 — the collision dependency is now exact, and it is per-hitbox, not per-joint.** A
hitbox resolves its joint once, at `ftmain.c:223`,
`attack_coll->joint = fp->joints[attack_coll->joint_id]`, and the per-frame read is
`gmCollisionGetFighterPartsWorldPosition(attack_coll->joint, &attack_coll->pos_curr)`
(`ftmain.c:1882` and `:1907`, once per live hitbox per state). That function walks **up the parent
chain** and consumes `parts->mtx_translate`, so what collision needs at 60 Hz is not the whole
skeleton — it is **the ancestor chains of whichever joints the live hitboxes name**. Other callers
of the same helper are effects/items spawn positions (`ftparam.c:1795/1890`, `itmain.c:332/458`,
`lbcommon.c:1469`), which are not gate-critical.

**E13 step 1b — the collision joint set is HARD-BOUNDED at 15, by struct size.** `FTStruct` carries
`FTAttackColl attack_colls[4]` and `FTDamageColl damage_colls[11]` (`include/ft/fighter.h:3141`,
`:3148`), so **at most 15 joints per fighter can be collision-read at once**, against
`nFTPartsJointNumMax = 37` slots and ~25 live (E10's 100.8 parse calls/frame over 2 fighters x 2
logical updates). The hitbox four are live only while attacking; the hurtbox eleven persist. Joint
ids reach the collision through the animation script, not a C table —
`attack_coll->joint_id = ftParamGetJointID(fp, ftMotionEventCast(ms, FTMotionEventMakeAttack1)->joint_id)`
(`ftmain.c:222`) — with the same shape for effects (`:426`), hit status (`:459`) and damage colls
(`:477`).

**The switch plan already prescribes the fix, and forbids the shortcut.** §3.5 is explicit: *"Do
not begin by compromising the simulation: 30 Hz gameplay creates correctness risk across one-frame
hitboxes, collision crossings, landing, shields, grabs, input timing, and CPU behavior."* Its rate
table keeps **gameplay mechanics at 60 Hz** and puts **visual fighter pose at 30 Hz**. §3.6: *"The
renderer must never require render skeleton == gameplay skeleton... the renderer consumes a compact
generated pose evaluated at presentation rate. They may share source data; they must not share one
expensive runtime representation."* **The 146,148-tick shared walk IS that forbidden single
representation.** So E13 is not "halve the rate" — it is "give the renderer its own pose", which is
R2-04's own title, and R2-04 E6 recorded that E5 paid down *loading* rather than pose.

**What E13 still owes:** the union of the ancestor chains of the collision joints, as a fraction of
the ~25 live. **Count it; do not estimate.** Note the parse/play walk is *unconditional* over every
joint (`ftParamUpdateAnimKeys`'s `joint_limit` loop), which is what makes the render-only remainder
recoverable at all.

**E13 step 1c — the two halves of that number have different costs, and one is nearly free.**

- **Hurtboxes are static per fighter, an instrument for them already exists, and IT REPORTS NOTHING.**
  They come from `fp->attr->damage_coll_descs[]`, applied once at init
  (`reloc_backend_fighter_model.c:2479-2495`, `ftparam.c:700-717`), not from move scripts.
  `ndsFighterMarioFoxRecordDamageCollShell` (`:2498`) counts the live hurtboxes at `:2513-2519` and
  publishes `gNdsFighterInitP0/P1DamageCollCount` (`:2574`/`:2586`) beside `...DamageCollJoint0`, the
  first hurtbox's `joint_id`. **Measured, DLDI-on, ROM `6B1F787B`
  (`artifacts/performance/r206-e13-hurtbox-128.json`): all five read ZERO** — both counts, both
  `Joint0`s, and `gNdsFighterInitDamageCollMask`.
  **That is not "no hurtboxes", it is "the recorder never wrote".** The tell is `Joint0`: its reset
  value is `0xffffffffu` (`taskman_seam.c:1098-1099`), so a value of `0` means the reset never ran
  *either* — everything is sitting at BSS zero. And `ndsResetStartupDiagnostics`, which contains that
  reset (`taskman_seam.c:30`, declared `nds_startup.h:728`), has **zero callers anywhere in
  `src/`** — dead code.

  **WHY it never wrote, run down to the mechanism: the family is COMPILE-TIME gated out of
  Boundary, and it is not a defect to fix.** The recorder is reached only through
  `ndsFighterMarioFoxInitStateFromOriginalOrder` (`reloc_backend_diagnostic_recorders.c:18194`,
  called once at `:18518`) behind *two* nested predicates, `ndsFighterMarioFoxStructProofEnabled()`
  and `ndsFighterMarioFoxInitProofEnabled()`, and both are `#if` chains over
  `NDS_DEV_SCENE_HARNESS` (`reloc_backend_fighter_model.c:601`, `:634`) naming only the retired
  `*_MARIOFOX_STRUCT` / `_INIT` / `_WAIT` / `_DL_*` harness modes. Boundary is none of them, so the
  call is compiled out. Confirmed empirically rather than by source reading: `gNdsFighterMarioFoxInitCount`
  reads **0** on a Boundary run (`artifacts/performance/r206-e13-initcount.json`), and it is
  incremented unconditionally at `:18351` immediately before the recorder, so a zero there proves the
  enclosing function never ran.
  **Therefore: never cite a `gNdsFighterInit*` value from a Boundary run — it is structurally 0, not
  measured — and do not "wire it up".** AGENTS.md forbids exactly that direction ("Do not add
  proof-only branch reruns"; "New harness modes are only for scene-level capabilities"), and these
  predicates are themselves *runtime* `if`s in live gameplay code
  (`reloc_backend_compat_shims.c:936`, `:955`, `:1230`, `:1807`, `:1858`…) that the
  "graduate imported subsystems live" rule wants migrated or deleted.
  **The right instrument already existed:** `scripts/census-fighter-gameplay-joints.ps1` (Task 77 E1)
  reads `fp->attr->damage_coll_descs[]`, the foot/item joints and the `animlock`/`setup_parts` masks
  straight off the live `FTStruct` over GDB, touching none of the proof globals. *Lesson: grep
  `scripts/` for an existing census before chasing a diagnostic global — this cost a run and two
  wrong inferences.*
  *(Two corrections, both mine, both caught before they were published as results: I first wrote that
  the count was "thrown away and needs a one-line addition" — wrong, I had read only to `:2559` and
  the assignment is at `:2574`. I then suspected all eight `reloc_backend_*.c` were compiled twice,
  being both `#include`d into `reloc_backend.c` and listed in the Makefile — also wrong: the build
  produces no standalone `reloc_backend_*.o` and the ELF has no duplicated FUNC symbols, so that
  Makefile list is not a second compile rule.)*
- **Hitboxes are dynamic and do need a probe.** `ftParamGetJointID` (`ftparam.c:534-541`) is a
  passthrough apart from `-2 → attr->joint_itemlight_id`, so script joint ids are *direct* indices
  into `fp->joints[]` with no small per-fighter table to enumerate. The ids therefore only exist
  inside the animation-script motion events, and the set actually exercised has to be logged from a
  run.

**Neither number is the ancestor-chain union yet** — both give the *leaf* joints. The union still
needs the model's parent links, which the DObj tree carries at runtime. `census-fighter-gameplay-joints.ps1`
now emits them (`FTPAR=` records, one per joint slot 4..36 per fighter) and closes the leaf set under
parenthood host-side, reporting `60Hz SET` / `30Hz OK` counts and the max chain depth.
**`DOBJ_PARENT_NULL` is `((DObj*)1)`, not NULL** (`decomp/src/sys/objtypes.h:32`) — a `parent != 0`
test walks into address 1, and every chain walk in this repo must test against 1.

**E13 step 1d — THE 60 Hz OBLIGATION IS *LOCAL TRS ONLY*, AND COLLISION ALREADY MEMOISES ITS OWN
CHAINS PER FRAME. This is what makes §3.6's split cheap.** Read the collision path to the bottom
rather than stopping at "it walks the live joint chain":

- `gmCollisionTransformMatrixAll(dobj, parts, mtx)` (`decomp/src/gm/gmcollision.c:29-74`) reads
  **`dobj->translate.vec.f`, `dobj->rotate.vec.f`, `dobj->scale.vec.f`** and builds the rotation
  matrix itself out of `lbCommonSin`/`lbCommonCos`. **It consumes the joint's LOCAL TRS. It does not
  read one matrix the render pose walk produces.**
- The read path `gmCollisionGetFighterPartsWorldPosition` (`:489-517`) walks
  `main_dobj = main_dobj->parent` and **early-exits at the first ancestor whose
  `unk_dobjtrans_0x5 != 0`** — i.e. one already resolved to world this frame — composing the rest on
  demand and setting `transform_update_mode = 1` per joint so a second hitbox on the same chain pays
  nothing. The setup walk `func_ovl2_800EDBA4` (`:330-395`) bounds chain depth structurally with
  `DObj *setup_dobj[18]`.
- **Both flags are per-frame, and this is the proof the earlier withdrawal lacked.**
  `unk_dobjtrans_0x5`/`0x6` are `u8` members of a union with `s32 unk_dobjtrans_word`
  (`decomp/src/ft/fttypes.h:657-668`), and `ftParamsUpdateFighterPartsTransformAll` (`ftparam.c:2161`,
  the `:2181` reset) walks the whole tree storing `parts->unk_dobjtrans_word = 0` and
  `transform_update_mode 1 → 0`. It is called every frame from `ftMainProcPhysicsMap`
  (`ftmain.c:1847`) immediately *before* the per-hitbox reads at `:1882`/`:1907`. **The port
  implements the same invalidation** (`reloc_backend_compat_shims.c:1485-1500`).
  *(This supersedes the withdrawn "already lazily cached" claim, which named the wrong field: I cited
  `unk_dobjtrans_0x5/0x6`'s one-time init at `ftmanager.c:263-265` and withdrew for lack of a
  per-frame reset. The reset exists — it is the union store, not a per-byte assignment, which is why
  a grep for the field names missed it. Grep the union, not the member.)*

### R2-06 E17 ANSWERED — the action-change re-add is 11,313/load frame, NOT the 78.5%. Two thirds of the premium has no named owner (2026-07-30)

Measured on a `NDS_R2_RELOC_FIXUP_TIMING=1 NDS_R2_LOADFRAME_TIMING=1` build, ROM `8711BF90`, DLDI on,
counters read at frames 797 and 925 and differenced (`artifacts/performance/r206-e17-a-start.json`,
`r206-e17-b-end.json`).

**The control validates the method before the result is read** — the reason both flags were on:
`gNdsR2FixupFinalizeCalls` differences to **exactly 18**, E8's own call count, and the AObj16 share is
**90.6%** (487,680 of 538,112) against E8's 88.4%. Totals run ~13% above E8's because this is a
different, doubly-instrumented ROM; the *shares* and the call count reproduce, which is what the
differencing had to establish.

| in-window 797..925 | delta | per call | per load frame |
|---|---:|---:|---:|
| `gcAddAnimJointAll` — the action-change re-add | **7 calls**, 104,192 | 14,885 | — |
| `gcAddDObjAnimJoint` | 320 calls, 169,472 | 530 | — |
| ...normalize / base / residual | 57,728 / 76,544 / 35,200 | | |
| **re-add family, double-counting removed** | **~181,000** | | **~11,313** |
| whole in-frame relocation | 538,112 | 29,895 | ~33,632 |
| **premium with NO named owner** | **~1,506,000** | | **~94,127 (67.7%)** |

**The falsifier passed — the re-add does run, 7 times — and the lever failed anyway.** Registered
threshold was R ≥ 640,000 to build, 256,000-640,000 to stack, below 256,000 to stop. **R ≈ 181,000**,
so by the rule written before the data: *the re-add is not the story.* It is **10.4%** of the
~1,747,000 non-relocation premium, and relocation plus re-add together explain only **32.3%** of the
139,072/load-frame premium.

*Double-counting note, because the raw numbers overstate it:* `gcAddAnimJointAll` calls
`gcAddDObjAnimJoint` per joint, so its 104,192 already contains most of the 320 inner calls
(7 calls x ~25 joints ≈ 175 of 320). Adding the two counters would have read ~274,000 and cleared the
stacking threshold spuriously. The family total above adds `AddAnimAllTicks` to only the ~145 inner
calls made outside it.

**And this corrects E8's hypothesis, not just its size.** E8 wrote *"the load marker is largely a proxy
for 'a fighter changed action this frame'"*. There are **16 load frames but only 7 whole-GObj re-adds**
in the same window, so fewer than half the load frames carry one — the load marker and the
action-change re-add **do not coincide**. `gNdsR2AddAnimAllMaxTicks` differences to **0**, so the
expensive 54,784-tick call is pre-window and every in-window call is cheaper than average suggests.

**Where this leaves R2-06: the load-frame premium is now measured to be spread, exactly as E10 found
the frame-wide premium to be spread.** Two thirds of it — ~94,127/load frame — has no named owner
after relocation and re-add are subtracted, and the three cheap named candidates are now all sized:
relocation 33,632, re-add 11,313, O(n²) scan 17.3%-of-a-small-thing. **Per the registered rule the next
step is to bracket the status transition and the hit/collision work** — but E10's independent
full-attribution result predicts they will also be spread, so **whoever picks this up should first ask
whether R2-06 is the right phase at all**, rather than adding a fourth bracket to a premium that has
now twice refused to concentrate. The honest reading is that the gate needs a *structural* change of
the kind the switch plan's §3 prescribes, not another lever inside the current structure.

*Harness note: `-Samples 1` fails the sampler's own count check ("produced 12 of 1 samples") — it
over-runs the stop loop. Use `-Samples 8` as the floor for a single-point counter read.*

### R2-06 E16 — E9 IS THE SMALL HALF. The load-frame premium is 78.5% ACTION CHANGE, and that has never been sized (2026-07-30)

E15 closed the animation body, promoting E9 to the only live lever. **Re-reading E8's own numbers
before building it says E9 cannot clear the gate and is not the biggest thing on the load frame.**
No new measurement — this is arithmetic on the table at `#### The relocation is only 21.5% of it`
above, which had already differenced the cumulative counters across the window.

| | in-window ticks | per load frame | share of premium |
|---|---:|---:|---:|
| load-frame premium, 16 frames | 2,225,152 | 139,072 | 100% |
| **whole** in-frame relocation, 18 calls | 478,080 | ~29,880 | **21.5%** |
| ...of which E9's two payload walks (71.9%) | 348,608 | **~21,788** | **15.7%** |
| **everything else — the action change** | **~1,747,000** | **~109,000** | **78.5%** |

**So E9's ceiling is ~21,788 per load frame, not the ~19,400-of-a-bigger-thing it reads as in the
queue.** Sized against P95: the gate is missed by 40,448, P95 is the 122nd of 128 sorted frames and
sits inside the load-frame population, so dropping every load frame by 21,788 moves P95 by about
that much — **1,160,448 → ~1,138,660, still 18,660 over.** It clears the ~16,000 tail-movement wall
and the ~5,400 cross-build floor, so it would be *measurable*, but it does not close the gate, and
E8 said as much already: *"Removing the whole relocation would clear perhaps 1-2 of the 8 over-gate
frames, not all of them."* Against that it is **"a real refactor of the boundary computation to work
in offsets"** (line 1169). **A marginal, non-closing win for a large refactor is the wrong next
build.**

**The unattacked 78.5% is the actual target, and E8 already named its mechanism:** *"The other
~109,000 per frame is the cause of the load, not the load: a fighter changing action, which also runs
the status transition, the animation-script re-parse and the hit/collision work. The load marker is
largely a proxy for 'a fighter changed action this frame.'"* **~109,000 per load frame, 1.75M
in-window, and it has never been bracketed as its own event.** E10's independent attribution
corroborates that it is real and reachable: `ftAnimParseDObjFigatree` at **13.0%** of the premium is
exactly the animation-script re-parse this describes, and it is a *different* cost from the
steady-state animation walk E13/E14/E15 just closed — that walk *plays* cached AObjs, this one
*rebuilds* them on an action change.

**Next experiment (E17): bracket the action change.** Put timing around the status transition, the
`gcAddAnimJointAll` re-add path (`battleship_sys_objanim.c:1163`, already flagged as *"the whole-GObj
variant, which is what a fighter action change goes through: it walks the DObj tree and re-adds every
joint's animation"* with `gNdsR2AddAnimAllCalls`/`Ticks` counters that exist and are uncollected), and
the hit/collision work, differenced across the window exactly as E8 did the relocation. **The
instrument is already in the source under `NDS_R2_LOADFRAME_TIMING` — collect it before writing any
new probe.** Register the threshold first: this is 78.5% of a 139,072 premium, so unlike every lever
since E10 it has room to clear 40,448 outright.

**E17's plan and threshold, registered before the data exists (rule 7).** The instrument covers only
**one of the three** components E8 named — `NDS_R2_LOADFRAME_TIMING` brackets the animation **re-add**
(`gNdsR2AddAnimAllCalls`/`Ticks`/`MaxTicks`, `gNdsR2AddDObjAnimCalls`/`Ticks`/`MaxTicks`/
`NormalizeTicks`/`BaseTicks` — all eight in `battleship_sys_objanim.c:1099-1106`), not the status
transition and not the hit/collision work. Measure the instrumented third first and only bracket the
rest if it fails to account for the ~109,000; that avoids writing probes for costs that turn out small.

> **Falsifier, checked first:** if `gNdsR2AddAnimAllCalls` differences to ~0 across the window, the
> re-add is not on the load frames at all and the "action change" attribution is wrong — say so and
> re-derive rather than bracketing further.
> **Then:** let `R` = in-window re-add ticks. **R ≥ 640,000** (~40,000/load frame) makes this a
> gate-closing lever on its own and it goes to a build. **R in 256,000-640,000** (16,000-40,000/frame)
> is a stacking win — pair it with E9's ~21,788 rather than shipping alone. **R < 256,000** and the
> re-add is not the story: bracket the status transition and the hit/collision work before proposing
> anything, because then ~109,000 is genuinely spread and E10's "no single lever" applies to the load
> frame too.

*Process note, second occurrence this cycle: the answer was already in the board and I went to the
code and an artifact first. The `r206-e8-fixup-timing-128.json` artifact is the **cumulative-from-boot**
read whose single 21,353,728-tick boot sprites call swamps everything — read alone it says sprites is
88.1% and AObj16 8.9%, the exact inverse of the in-window truth. I briefly took that as a
documentation defect in `reloc_backend_assets.c:3089`. It is not; the comment is right and the board
had already differenced the two reads. **Read the board section for a task before instrumenting it,
and never quote a cumulative counter as a per-window figure.***

### R2-06 E15 — DO NOT BUILD. Both arms are sub-floor, and the disassembly refuted my own estimate 4x (2026-07-30)

E14 left one candidate: shrink `AObj` so the pool leaves the dcache, plus store the four
segment-constant values as Q12 so the cubic stops converting them. **Sized before building, and it
does not clear the floor. Neither arm does, and jointly they only straddle it.**

**Arm A — remove four of the cubic's six f32→fixed conversions. REFUTED, and my estimate was wrong by
about 4x.** Only `t` (`aobj->length * aobj->length_invert`) and `length_q` (`aobj->length`) depend on
the advancing `length`; `value_base`, `value_target`, `rate_base` and `rate_target` are constant for
the segment (`battleship_sys_objanim.c:213-241`), so storing them Q12 would delete four conversions.
I priced those at ~20 ticks each on the assumption they compile to `__aeabi_fmul` + `__aeabi_f2iz`.
**They do not.** Disassembling the kernel at HEAD finds **exactly one `bl` in the whole function —
`__aeabi_fmul`, the one genuine float x float at `:213` — and zero `__aeabi_f2iz`.** GCC emits the
scale-by-power-of-two as an exponent adjust and the cast as an inline shift sequence, which
`target("arm")` makes cheap. So the conversions are already close to free.
Corrected: the kernel is 2,032 bytes = **508 static ARM instructions** but executes only
**116 per call** (28,048 insn/frame ÷ 242 calls), so four inline conversions are on the order of
**~4,000-10,000 ticks/frame**, not the ~15,000-19,000 I had. *The frame-wide `__aeabi_f2iz` total —
2,173 instructions/frame, far too few for 1,452 conversions — was the tell, and one objdump settled
it before a build.*

**Arm B — pool to DTCM. Sub-floor AND blocked on fit.** 221 nodes swept twice per presented frame
over ~249 lines is ~498 fills plus ~498 write-backs (it is read-modify-write); at Task 96's measured
10-15 ticks per line that is **~12,450 ticks/frame**, below the 20,000 floor on its own. And at 20
bytes/node the pool is 4,420 bytes against the **4,264** E29 left below the boot stack's low-water
mark — it does not fit without also reaching ≤19 bytes/node.

**Verdict: ~16,000-22,000 jointly, straddling the floor, in exchange for a change that touches the
struct layout, the allocator, the linker placement, the cubic kernel, and the Step/Linear paths that
are currently bit-identical to the decomp** (43.6% + 1.7% of nodes — E64b/E65's equivalence bound
covers the cubic only, so those would need their own re-validation). **E11's lesson is that only a
change big enough to clear the floor decisively is worth building; straddling is not decisive.
E15 is closed unbuilt.**

**Which exhausts the animation body at this granularity — three levers, three refutations, no builds
spent: E13 (pose fewer joints, f=0.840), E14 (reorder/flatten, ~2,900), E15 (shrink + Q12,
~16,000-22,000 straddling).** The 146,148 is 59.4% stall, but the stall is spread thin enough that no
single sub-lever inside it clears the placement floor. **So stop drilling into animation and go back to
the frame:** E8 already established that **clean-frame P95 is 1,056,640 — inside the gate by 63,360 —
and every over-gate frame is one of the 16 asset-load frames.** The gate is a load-frame problem, E11
closed "remove a little load-frame work", and the surviving category is the one E11 named explicitly:
**move the work off the frame — E9, ~19,400/load frame**, which is ~139,072 of premium concentrated in
12.5% of frames, so a shift of the whole load-frame population moves P95 by nearly its full amount
rather than a diluted one. That is the next build, and it must clear the ~16,000 tail-movement wall to
count.

### R2-06 E14 — the AObj walk is READ-MODIFY-WRITE over a working set 2.7x the dcache, and E29's lever does not fit (2026-07-30)

E13 closed the "pose fewer joints" route, so the animation over-budget has to be paid per joint.
E12 left one unpriced suspect: the per-frame AObj pointer chase. **Source-side facts first, because
three of them change what the experiment can be. All read-only, no build.**

- **`AObj` is 36 bytes, not the 32 the E12/E6 memo assumed** — `next` 4, `track`+`kind` 2 padded to
  4, seven `f32` 28, `interpolate` 4 (`decomp/src/sys/objtypes.h:124-136`; the port defines no
  override, so this is the struct that runs). **36 > 32 means every node straddles at least two
  32-byte lines**, and `gcGetAObjSetNextAlloc` requests alignment `0x4`
  (`decomp/src/sys/objman.c:608`), so nothing forces line alignment either.
- **The walk touches the WHOLE struct, so hot/cold splitting is not available.**
  `gcPlayDObjAnimJoint` (`src/import/battleship_sys_objanim.c:251-325`) reads `kind`@4, `track`@5,
  `length`@12, `value_base`@16, `value_target`@20, `rate_base`@24, `rate_target`@28,
  `length_invert`@8, `interpolate`@32 and `next`@0 — every offset in the object.
- **And it WRITES: `aobj->length += dobj->anim_speed` (`:266`) every node, every frame.** So this is
  a read-modify-write sweep, dirtying every line it touches and paying write-back on eviction — not
  the read-only chase the hypothesis described. It also pulls in `dobj->rotate/translate/scale` and
  `dobj->parent_gobj->flags` per node, so the DObjs are in the working set too.

**N IS 221, MEASURED — not the "~300" the memos carried.** `sGCAnimsActiveNum` read on a settled
Boundary frame, DLDI on, ROM `6B1F787B` (`artifacts/performance/r206-e15-aobjcount.json`); the XObj
pool alongside it is 133. Every figure below uses 221.

**The cache arithmetic, on this repo's own measured geometry (4 KB dcache, 32-byte lines — R2-03
E29).** 221 nodes x 36 bytes = **7,956 bytes = ~249 distinct lines against a 128-line cache**, so the
pool is **1.94x the dcache** and cannot be made to fit — no reordering will change that. Ordering only
decides *reuse*: in address order one line serves ~0.9 nodes and the sweep costs ~249 misses; in
scrambled order each node misses separately, ~442. **~193 misses is the whole prize**, and only if
traversal order differs from address order today.

**It probably does, and for a specific reason: `syMallocSet` is a bump allocator, but AObjs are
recycled through a LIFO free list.** `gcGetAObjSetNextAlloc` pops `sGCAnimHead` and
`gcSetAObjPrevAlloc` pushes back (`objman.c:602-622`, `objhelper`/`objman.h:64`), so the first
allocation burst is contiguous and every subsequent free/realloc cycle scrambles list order against
address order. **This is also why E12's "AObj pool" refutation was right but incomplete** — the
bump allocator does make them contiguous, and contiguity was never the problem; *traversal order* is.

**E29's exact lever does not fit, and that is the binding constraint.** E29 won 26,816 by moving
10,820 bytes of randomly-indexed fighter tables into DTCM, which has no cache lines at all — the same
size and the same shape as this pool. But it also recorded what it left: **7,144 DTCM bytes remain
between the tables and `__sp_usr`, only 4,264 of them below the boot stack's measured low-water mark**
(`docs/optimization/ClaudeOpus5_R203_E29_FighterTablesInDTCM_20260728.md:125-129`). ~10,800 bytes do
not fit in either figure. **Do not propose moving the AObj pool to DTCM without shrinking it first.**

**THE REORDER LEVER IS REFUTED WITHOUT A RUN, ON A PRECEDENT THIS CAMPAIGN ALREADY PAID FOR.**
Task 96 measured exactly this class — a scattered per-frame linked chain, flattening as the lever, the
same ~20,000 bar — and established the two constants that decide it
(`docs/optimization/archive/ClaudeOpus5_Task96_TheChainIsNotTheCost_20260726.md:43-68`):

- **A 32-byte line fill from DS main RAM is ~20-30 ARM9 cycles = 10-15 ticks**, not the 30-60 that
  doc had carried as a deliberate worst case.
- **The ARM946E-S has no hardware data prefetcher**, so a flat array is not prefetchable where a chain
  is not — the *entire* benefit of flattening is the line-count difference, with nothing behind it.

Task 96's own case was **more** favourable than this one — 259.7 lines saved per frame, against a
structure re-walked 104 times over 338 nodes — and its honest ceiling was **~2,600-3,900 ticks/frame**,
so it stopped. Here the reorder prize is bounded by the node count: at most **193 x 15 ≈ 2,900
ticks/frame**, or ~5,800 charging the dirty write-back at full price. **Both are far below the 20,000
P95 placement floor, so E14's reorder/flatten arm is dead and its run is cancelled** — per rule 7, a
measurement that cannot change the decision is not worth taking. *This should not have needed
re-deriving; Task 96 is the same lever on the same hardware and was one grep away.*

**What survives is a different lever with a different mechanism, and N = 221 makes it land.** Shrinking
`AObj` from 36 bytes to ~20 — fixed-point for the seven `f32`s, which E64b/E65 already established as
equivalent to within 0.0028 rad / 0.0067 units, and which `gcPlayDObjAnimJoint` already converts to
fixed *internally* at `battleship_sys_objanim.c:240-244`, so storing fixed **removes** conversions
rather than adding them — does not win by line count. It wins by taking the pool from 7,956 bytes to
**4,420**, into DTCM, where there are no cache lines at all and where E29's identically-shaped
10,820-byte move was worth **26,816**. The read-modify-write matters here too: DTCM removes the
write-back traffic, not just the fills.

**But the fit is MARGINAL and that is the first thing E15 must settle.** E29 left 7,144 DTCM bytes, of
which only **4,264 are below the boot stack's measured low-water mark** — and 4,420 exceeds that by
156 bytes. So either E15 lands in the 2,880 bytes the stack has been *seen* to use, which needs its own
justification, or the struct has to reach **≤19 bytes/node**. One clean route to that exists and is
worth more than the bytes: **N = 221 < 256, so the free-list/chain `next` can be a `u8` index into a
fixed pool array instead of a 4-byte pointer** — which drops 3 bytes AND deletes the pointer chase
outright, converting the walk to an indexed sweep. Do not treat that as a bonus; it is the part of the
change that makes the rest safe to size.

**E13 ANSWERED — THE REPRESENTATION SPLIT IS REFUTED BY ITS OWN SIZING. Mario and Fox are ~84%
collision-load-bearing, so there is no render-only remainder to move to 30 Hz (2026-07-30).**

Measured, `scripts/census-fighter-gameplay-joints.ps1` extended to close the leaf set under
parenthood, one stop at frame 439, `artifacts/performance/r206-e13-joint-census.json`:

| fighter | live joints | 60 Hz closure | **f** | render-only | max chain depth |
|---|---|---|---|---|---|
| Mario (slot 0) | 24 | 21 | **0.875** | **3** — joints 10, 22, 27 | 4 |
| Fox (slot 1) | 26 | 21 | **0.808** | **5** — joints 10, 22, 27, 28, 29 | 3 |
| combined | 50 | 42 | **0.840** | 8 | — |

**f = 0.840 against a pre-registered cutoff of 0.70, so: DO NOT BUILD.** The ceiling is
`(1 − 0.840) × 73,074 = ` **~11,692/frame**, roughly half the >20,000 P95 placement floor — E11's
situation exactly, a provable saving P95 cannot show. And 11,692 is an *upper* bound:
`ndsR2CubicValueFixed`'s cost follows animated *tracks* rather than joints, and
`ftParamUpdateAnimKeys`'s own 12,139 is loop overhead that a shorter loop still pays.

**Why f is so high — this is the durable fact, not the arithmetic.** The skeleton is dense with
collision. Hurtboxes alone are **11 joints** (`damage_colls[11]`, and both fighters populate
essentially all of it: Fox 5 6 8 9 12 14 15 19 20 24 25, Mario the same less one `-1`), `animlock`
adds **6 more** (7 13 18 21 23 26) which are code-placed and so load-bearing by construction, and the
closure over parents — only 3-4 deep — fills in the intermediates. **The effect joints (9 12 15 20 25)
are a strict subset of the hurtbox set: `cosmetic-only` is EMPTY for both fighters.** What is left over
is 8 joints across two skeletons, all extremities.

**The hitbox half is no longer needed, and that is what makes this final rather than provisional.**
E13 step 1c planned a probe to log the `attack_colls[4]` joint ids out of the motion events. It is
moot: those four ids either name joints already inside the closure (**f unchanged**) or name one of
the eight leftovers (**f rises**). **f can only increase**, so 0.840 is a floor and the verdict cannot
flip. The planned probe is cancelled rather than left open — a queue item that cannot change a
decision is not a queue item.

Two reads that could have looked like defects and are not: Mario's gameplay set names joint **28** and
Fox's names **30**, neither of which is live in its own model (Mario's live slots are exactly 4..27,
Fox's 4..29) — those are `attr` item-joint ids for joints the model never instantiates, correctly
skipped by the closure's live guard, and costing nothing in the loop either (`joint == NULL` →
`continue`). And both fighters' closures are the *same* 21 slots, which is not a copy-paste artifact:
they share the common skeleton layout from `nFTPartsJointCommonStart`.

**So §3.6's split is sound as a principle and simply has no purchase on these two fighters** — which
is a fact about Mario and Fox, not a defect in the plan, and it will differ per fighter. The animation
over-budget therefore **cannot** be paid by posing fewer joints. It has to be paid by making the
per-joint work cheaper, which returns the phase to E12's standing hypothesis: **86,819/frame of the
146,148 is stall, and the unpriced suspect is the ~300 individually-`malloc`'d 32-byte AObjs walked by
pointer chase every frame** (R2-06 E6: the battle task passes `aobjs_num = 0`). E12 already forbade
building on that before counting the distinct cache lines it touches. **That count is now the phase's
next step**, and unlike E9 it is body cost, so the E11 wall does not apply to it.

*Kept for the record because pre-registration is what made this cheap:*

**E13 step 2 — the sizing rule, PRE-REGISTERED before the census number is known.** Written down
first on purpose: E11 was decided after its number arrived, and a threshold chosen afterwards is not
a threshold. The loop to split is `ftParamUpdateAnimKeys` (`reloc_backend_compat_shims.c:1553`),
whose trip count is already a per-fighter runtime value — `ndsFTStructJointLoopLimit` returns
`nFTPartsJointCommonStart + fp->nds_common_joint_count` (`:1535`), so "~25 live" is readable, not
estimated. `NDS_TASK106_UPDATES_PER_PRESENT = 2`, and only the *second* logical tick can be reduced
(the presented one must pose everything), so with **f = |60 Hz set| / |live|**:

> **saving ≈ (1 − f) × 146,148 / 2 = (1 − f) × 73,074 per presented frame**

- **f ≤ 0.45 → ≥ 40,190: clears the 40,448 gap on this lever alone. BUILD.**
- **0.45 < f ≤ 0.70 → 21,922-40,190:** beats the >20,000 P95 placement floor, but not the gap alone.
  Build only as part of a stack, and say so when reporting it.
- **f > 0.70 → < 21,922: AT OR INSIDE the P95 floor. DO NOT BUILD this lever** — that is E11's
  situation exactly (a real, provable saving that P95 cannot show), and the answer there is to record
  the number and stop.

Two honest deductions from that ceiling, both of which only lower it: the cosmetic MObj inner loop
(`:1610-1616`) is **10,049/frame** of the 146,148 (E12: `gcParseMObjMatAnimJoint` 7,627 +
`gcPlayMObjMatAnim` 2,422) and is render-only *unconditionally*, so it is a component of this split
and **not a standalone cut** — halved it is ~5,025, a quarter of the P95 floor. And the three symbols
worth having are `ndsR2CubicValueFixed` 48,623 + `gcPlayDObjAnimJoint` 40,973 +
`battleship_ftAnimParseDObjFigatree` 32,614 = **122,210 of 146,148**, so the lever is real only if the
*parse* can be skipped too, not just the play.

**Which forces one design constraint: do not implement this by skipping a tick.** The parse advances
each joint's animation cursor, so a skipped joint silently falls a tick behind and its pose lands on
the wrong absolute animation frame — a drift, not a rate reduction, and it would show as jitter that
looks like a different bug. The 30 Hz joints must be **advanced by two and evaluated once**.

**Consequence for E13, and it is the design:** the 60 Hz path owes collision only *"`translate` /
`rotate` / `scale` are current on the ancestor closure of the live collision joints"*. Composition to
world is already lazy, already memoised, already restricted to the chains actually touched, and
already priced inside gameplay — not inside the 146,148. So the animation walk's **only** 60 Hz
obligation is writing local TRS for that closure; evaluating the remaining joints, and the entire
render-side matrix build, is render-only and may run at presentation rate. That is precisely §3.6's
*"may share source data; must not share one expensive runtime representation"*, and it means the
split does **not** need a duplicated skeleton — it needs the *unconditional* joint loop in
`ftParamUpdateAnimKeys` to become two loops over a precomputed 37-bit mask. **Size it with the
census before building it** — three of the last four source-derived mechanisms in this phase were
refuted by their own measurement.

**So the only lever with the right shape is touching fewer bytes per frame, not rearranging them.**
`NDS_TASK106_UPDATES_PER_PRESENT = 2`, so this walk runs **twice per presented frame**;
`PROJECT_GOAL.md` explicitly permits "skeletal poses to update at 30 Hz" and "reduced animation
update rates", and halving the rate is worth order **73,000/frame** against a 40,448 gap. **But it
is not free and must not be filed as a visual-only change:** R2-04 E57 established that hitboxes
walk the **live joint chain** (`gmcollision.c:489`), so halving the pose rate halves the hitbox
update rate, which is gameplay. Any such task has to separate the joint update that collision
reads from the one only the renderer reads, and price the split before assuming it exists.

### R2-06 E11 REFUTED — the work really was removed, and P95 still got worse (2026-07-30)

**Do not bring another small load-frame cut.** E10 named `ndsRelocAssetIDForToken` the cleanest
lever in the phase: 630 calls on the 16 load frames, **zero on all 112 clean frames**, so a fix
could not regress a clean frame's *work* by construction. E11 built the fix, proved it removed
the work, and it lost the gate anyway. That is the result worth keeping.

**The change.** Hoist the `ndsRelocIsMarioFoxAnimID` range check above the two inlined pointer
scans, and delete the five `MARIO_ANIM_WAIT`/`WALK1..3`/`WALK_END` compares it subsumes.
Provably identical, not merely faster: the range is `[0x1f3, 0x31f]`; every compare that returns
something other than its own argument tests either a file-id global's link-time address
(≥ `0x02000000`) or one of `0x58` / `0x5f` / a bank id (all ≤ `0x13b`), so none can match a token
in the range; every remaining compare it could reach has the form `if (token == X) return X;`.
**Negative bytes added** — the Task 74 veto on adding data was respected, not worked around.

**It did what it claimed.** Function `39,475 → 31,808` per load frame (**−7,667**, **−5,103
instructions**), and the equivalence guard passed exactly: the 16 marked load-frame regions were
**bit-identical** between arms, so the same files loaded on the same frames.

**And the gate got worse.** Against a matched control rebuilt from HEAD in the same harness
(`r206-e11-control-128` vs `r206-e11-tokenfirst-128`, DLDI on, ring dump, 128 samples):

| | P10 | P50 | P90 | **P95** | P99 | over 1.12M | mean |
|---|---:|---:|---:|---:|---:|---:|---:|
| matched control | 919,744 | 976,704 | 1,080,064 | **1,179,520** | 1,233,792 | **9** | 995,922 |
| candidate | 917,824 | 973,824 | 1,068,864 | **1,195,264** | 1,292,992 | **11** | 993,011 |
| delta | −1,920 | −2,880 | −11,200 | **+15,744** | +59,200 | **+2** | −2,911 |

**Two HEAD controls disagree with each other by P95 +5,376 and ±1 over-gate frame**
(`r206-head-control-128` vs `r206-e11-control-128`, identical source, different ROM hash — the
build is not reproducible, already recorded at §"R2-06 E8"). So +15,744 is about 3x the observed
noise, not noise. The window also shifted 797..924 → 798..925 → 799..926 across the three arms,
which is **harness variance, not the change** — worth knowing before anyone reads a frame number
as fixed. The candidate's over-gate set is the control's nine shifted +1 **plus 829 and 848**,
and regions 32 and 51 say both of those are **load frames**: the change pushed two more load
frames over the gate while making the function cheaper.

**The lesson is about size.** A load-frame-only saving of ~8,000 ticks **cannot be banked through
P95 on this ROM**, because relinking moves the tail by more than the saving. This is Task 74's
outcome reached by a different route, and it is now much better evidenced: Task 74 could argue
its 512 bytes of `.main.bss` were the cause, while E11 added negative bytes and lost the same
way. Combined with E10 (no single large lever; the premium is 513 symbols), the queue's shape
changes: **stop accumulating small load-frame cuts.** What remains viable is (a) one change large
enough to clear ~16,000 of tail movement, or (b) **moving the work off the gameplay frame**,
which is E9's hoist — it changes *when* the work happens rather than where the code sits, and is
therefore the only remaining candidate that is not fighting the linker.

The guard is recorded at `reloc_backend_assets.c:1796` beside Task 74's, because that comment is
what stopped E11 from re-attempting the memo and is where the next attempt will look.

### R2-06 E8 — EVERY over-gate frame is an asset-load frame, and the clean-frame P95 MEETS THE GATE (2026-07-30)

E7 narrowed the excursion to `SRC`. This names the event, and it is the most actionable
result the phase has produced. `NDS_TASK75_LOAD_CENSUS=1` re-points the per-frame census
ring at `gNdsTask75AssetLoadCount`, which is bumped inside
**`ndsRelocFinalizeLoadedFile`** (`reloc_backend_assets.c:3398`) — so the ring marks exactly
the frames that relocate an asset. Evidence
`artifacts/performance/r206-e8-load-census-128{.json,-rows.csv}`.

**16 of 128 frames take a load, and 8 of the 9 over-gate frames are among them.**

| population | n | P50 | P90 | P95 | max |
|---|---:|---:|---:|---:|---:|
| load frames | 16 | 1,113,152 | 1,277,888 | 1,305,088 | 1,617,152 |
| **clean frames** | **112** | **974,080** | **1,006,144** | **1,056,640** | 1,200,896 |
| all frames | 128 | 978,752 | 1,073,216 | 1,161,152 | 1,617,152 |

- **Clean-frame P95 is 1,056,640 — INSIDE the 1,120,000 gate by 63,360.**
- **1 of 112** clean frames is over gate; **8 of 16** load frames are.
- Load frames carry a median **+139,072 (1.14x)**, and `SRC` alone accounts for it:
  `SRC` P50 444,544 on load frames against 305,216 clean, **+139,328**. That is E7's bracket
  attribution confirmed to the tick.
- Frame 909 is **1,617,152**, i.e. **+643,072** over the clean median. Same frame E53
  profiled as 910–913.
- The one exception is frame 842, adjacent to load frame 843.

**So the milestone turns on getting the load's work out of the gameplay frame**, not on
shaving the average frame — the average frame is already 145,920 under budget.

#### The relocation is only 21.5% of it. Do not over-claim this.

`NDS_R2_RELOC_FIXUP_TIMING=1` (new, lab default off) prices
`ndsRelocFinalizeLoadedFile`'s five passes. The counters are cumulative from boot, so they
were read twice and differenced — at window start and at window end — because the very
first `ndsRelocNormalizeBattleInterfaceSprites` call is **21,353,728 ticks** on its own and
would otherwise swamp everything. In-window, frames 797..924:

| pass | ticks | share |
|---|---:|---:|
| `ndsRelocNormalizeFighterAObj16File` | **422,848** | **88.4%** |
| `ndsRelocNormalizeBattleInterfaceSprites` | 24,000 | 5.0% |
| `ndsRelocApplyInternalPointerFixups` | 16,320 | 3.4% |
| `ndsRelocApplyExternalPointerFixups` | 2,560 | 0.5% |
| `ndsRelocNormalizeFighterAttributesFile` | 1,472 | 0.3% |
| register/alias residual | 10,880 | 2.3% |
| **total, 18 calls** | **478,080** | 26,560/call |

**478,080 is 21.5% of the 2,225,152 the 16 load frames carry in premium** — about 29,880
per load frame against a 139,072 premium. The other ~109,000 per frame is the *cause* of the
load, not the load: a fighter changing action, which also runs the status transition, the
animation-script re-parse and the hit/collision work. **The load marker is largely a proxy
for "a fighter changed action this frame."** Removing the whole relocation would clear
perhaps 1–2 of the 8 over-gate frames, not all of them.

Two independent corroborations that this is an event and not simulation volume: Task 106
proved the `SRC` excursion **survives halving the update rate unchanged** (+518,016 vs
+522,720), and the anim cache is already at a 79/81 hit rate with `WarmFailed=0`, so this is
not I/O.

#### Inside the normalizer: the O(n^2) is NOT the cost. It is the payload, walked twice.

**I first named the O(n^2) successor scan at `reloc_backend_assets.c:3050-3081` as the lever
from reading the code. That was an inference, and the measurement refutes it.** Three brackets
inside the function, differenced across the same window (18 calls, 484,480 ticks,
26,916/call):

| sub-pass | ticks | share | shape |
|---|---:|---:|---|
| per-script normalize (`ndsRelocNormalizeAObj16Script`) | **197,248** | **40.7%** | 936 u16 words/call @ 11.7 ticks |
| lane swap over the payload | **151,360** | **31.2%** | 468 u32 pairs/call @ 18.0 ticks |
| O(n^2) successor scan | 83,584 | 17.3% | 647 inner iterations/call @ 7.2 ticks |
| unbracketed residual | 52,288 | 10.8% | — |

**The O(n^2) is real but small, because n is only 25.4.** `table_bytes`/call is 102 bytes, so
the table holds ~25 entries and n^2 is 647 iterations at 7.2 ticks each — exactly what a
compare loop should cost. Scaling was the wrong worry.

**The actual shape is that the payload is walked TWICE.** `data_size`/call is 1,972 bytes; the
lane swap walks the whole script region as u32 pairs, then the per-script normalize walks the
same 936 words again as u16 (17.8 scripts/call x 52.6 words). Together **71.9% of the
function** — 348,608 of the 478,080 in-frame relocation.

**And hoisting them out of the frame IS viable, which corrects the other half of what this
row first said.** All three sub-passes use only pointer *differences* (`script_end - value`,
`value - base`), so their output is a pure function of the file bytes. What is
address-dependent is only that they *read* the absolute pointers
`ndsRelocApplyInternalPointerFixups` wrote — and that pass is 3.4% of the total. So a cache
holding the payload with its table in OFFSET form and its script region already normalized,
restored by memcpy plus the cheap internal fixups, removes ~72% of this function. That is a
real refactor of the boundary computation to work in offsets, not a small edit.

#### The instrument is proven inert, and ROM SHA is NOT an identity check here

`NDS_R2_RELOC_FIXUP_TIMING` touches a shipped source file, so it was verified rather than
assumed. With the flag off: **no `gNdsR2Fixup*` symbol survives in the ELF**, and BSS is
1,709,448 — exactly the known pre-instrument value. Flag on costs +744 text / +640 BSS.

Then the flag-off ROM was re-measured over the same window and reproduces
`r206-head-control-128` in **all eleven buckets identically** — P50 976,064, P95 1,160,448,
mean 994,146, min 893,056, max 1,641,792, VBI 771/138/11/4, named 1,002,569.

**Three distinct ROM hashes — `8F0CDAAC`, `1C1136BA`, `D9CF3781` — measure bit-identically.**
So this build system does not produce a reproducible ROM hash for identical source (a
container/timestamp field moves), and **`romSha256` in an artifact must not be used to decide
whether two runs are comparable.** Use the measurement, the ELF symbol table, or the section
sizes. This is the fourth independent confirmation that the harness itself has zero
run-to-run noise.

#### The ceiling on all of it, and where the gate lever actually is

**The entire in-frame relocation is 21.5% of the load-frame premium, so no sub-optimization
of it can be worth more than that.** Best case is ~19,400 of the 139,072 per load frame,
moving P95 from 1,161,152 to roughly 1,142,000. Worth banking under the operating model's
"keep every repeatable correctness-preserving gain" — **not the gate.**

Averaged over the window the whole relocation is 3,735/frame, *under* the 5,000-7,000
placement noise floor, so any version of this must be judged **on the load frames only** —
legitimate now that E6 established this harness has zero run-to-run noise.

**The remaining ~78% of the premium is the action change itself** — the status transition, the
animation-script re-parse and the hit/collision work that make the fighter need a new
animation in the first place. **That is where the next investigation goes, not here.**

### R2-06 E7 — the fighter fallback is REFUTED as the excursion cause. 0 of 256 draws fell back (2026-07-30)

E53 named this **"the highest-value unowned row on this board"** and asked for exactly one
build: read the fallback counters on the excursion frames. Done, and the answer closes the
row. `NDS_TASK68_FALLBACK_CENSUS=1`, same 128-frame window,
`artifacts/performance/r206-e7-fallback-census-128{.json,-rows.csv}`:

```
native-owner: 256 draws over 128 frames, 256 eligible, 0 fell back (0.0%)
  [animLock:0 selected:0 displayList:0 materialCount:0 validate:0 matrices:0
   materialPrep:0 inputs:0 contract:0 postGx:0 begin:0]
frames with a fallback: 0 of 128
```

**Two fighters x 128 frames = 256 draws, every one eligible, every one native, at every one
of the eleven rejection points.** So E53's leading story — the fighter abandoning the native
owner for the generic interpreter — **cannot be the excursion on the shipped build.** E32's
shuffle fold closed it, precisely as E53 predicted it would: with
`NDS_R2_FIGHTER_SHUFFLE_FOLD=1` the condition at `reloc_backend_renderer_dl.c:12295`
narrows to `is_use_animlocks` alone, and animlocks never fire in this scene (E31/E32 both
measured 0; this measures 0 over a fourth window).

**Consequence for the queue: E32's parked flash residual is no longer blocking a gate
lever.** The lever it was blocking has already been delivered.

**Effects are refuted too, and that cost no build** — Task 39's counters are already in the
shipped tick-HUD ROM. Over the whole run: `HitSparkSpawnCount=4`, `HitSparkDrawCount=0`,
`ShieldDrawCount=0`, `FlashDrawCount=20`, `ArenaRejectCount=0`, `ObjVramBytes=22,528`.
**Four hit sparks in 924 frames cannot produce 8 over-gate frames in 128.** (Its tick
brackets all read 0, so Task 39 timing is gated elsewhere — a separate, minor instrument
gap, not a result.)

#### What the excursion actually is: `SRC`, and the draw brackets are FLAT

The 8 over-gate frames in `r206-head-control-128` are 809, 842, 843, 869, 890, 898, 909,
924 — gaps 33/1/26/21/8/11/15, so a recurring condition firing on ~6% of frames, not one
event. Median clean frame against median over-gate frame:

| bucket | clean median | over-gate median | delta |
|---|---:|---:|---:|
| `FTR` | 390,208 | 388,896 | **−1,312** |
| `STG` | 176,736 | 174,240 | **−2,496** |
| `SRC` | 307,328 | 550,016 | **+242,688** |
| `MISC` | 47,200 | 67,232 | +20,032 |
| `AUD` | 1,280 | 1,568 | +288 |
| `OTHR` | 161,088 | 503,008 | +341,920 |
| **`WORK-H`** | **974,848** | **1,208,320** | **+233,472** |

`SRC` is `gNdsTickHudSourceTicks` (`taskman_seam.c:4937`) — its own bracket, the source
update, **not a draw bucket**. `OTHR` is `ALL − named` and Task 66's own comment says it is
mostly the VBlank wait, which `WORK-H` already subtracts, so its +341,920 is quantisation
and idle rather than work. Netting the real buckets: +242,688 +20,032 −1,312 −2,496 +288
= **+259,200 against a WORK-H rise of +233,472. `SRC` alone is ~92% of the excursion.**

**The specialized draw owners are not involved at all** — `FTR` and `STG` are *lower* on the
expensive frames. That re-confirms E35's original "`SRC` owns the gate, 25 of 26 over-gate
frames" on the current build, and it **qualifies how E53's symbol profile must be read**:
E53 ran `NDS_TASK37_PROFILE=1 NDS_TICK_HUD_DRAW=0`, so its symbol attribution could not
separate draw from update, and it charged 292,899 ticks of renderer symbols to the
excursion under the heading "a second renderer running". Those symbols are real, but the
bracket that owns the time is the source update. Frame 909 here (`WORK-H` 1,641,792, `SRC`
920,000) is the same event as E53's 910–913, so the two profiles are of one thing.

**Next step, and it is a profile not a build-and-hope.** Attribute by BRACKET, not by
symbol: the question is what the source update does on ~6% of frames that it does not do
otherwise. Frame 809 is a separate cause and should be excluded from the pool — its `AUD`
is 91,904 against a clean median of 1,280 (71x) while its `SRC` is the *lowest* of the
eight, so it is an audio event, not the `SRC` condition. Do **not** re-run the fallback
census; it is answered. Do **not** re-profile by symbol alone; E53 already did and the
bracket ownership is what was missing.

### R2-06 E6 REFUTED — the Horner fold, and why E61 §5's −56,774 was never available (2026-07-29)

E61 §5 tabled three routes for the cubic and E64b took only the first. The third,
**"fixed-point Horner, 6 ops @ ~4 ticks → 3,735/frame, saves 56,774"**, was the
largest number left on the board. It was built, measured twice, and **REFUTED. Do
not re-propose it.** The arithmetic is right; the estimate priced operations and
ignored that the folded coefficients have to live somewhere.

**The expansion is correct and the numerics are fine.** With `u = length`,
`t = u·li`, the Hermite form collects into two polynomials whose coefficients are
integer combinations of the four values — no divide, nothing that needs `u`:

```
P(t) = c0 + c2·t² + c3·t³      c0 = vb   c2 = 3(vt−vb)   c3 = 2(vb−vt)
Q(t) = d0 + d1·t  + d2·t²      d0 = rb   d1 = −(2rb+rt)  d2 = rb+rt
value = P(t) + u·Q(t)
```

`check_r2_cubic_error_bound.py` on the shipped composition: **rotation 0.002979,
translation 0.005524** against the 0.02 gate, zero saturations — i.e. *better than
the shipped Hermite kernel on translation* (0.0067). Two numerical traps were found
and fixed inside the bound harness, before any ROM was built:

- **P and Q cannot share a width.** Both in Q12 Horner deviated **0.0718** world
  units. `Q` is multiplied by `u` up to 85, so one Q12 quantum of `Q` reaches the
  result as 85/4096 = 0.021 — over the gate by itself. `Q` has to accumulate in Q28
  with no intermediate rounding, which is what the Hermite kernel was really buying
  by keeping `h_rb`/`h_rt` in Q16 and summing into an `s64`.
- **`length_invert` needs its own scale.** Quantising `li` to Q16 before multiplying
  by `u` amplifies its quantum ~90x: 2.2e-4 of `t`, which against a 120-unit swing
  is 0.040 off. That is the one thing the `length * length_invert` **soft-float**
  multiply was buying — an f32 24-bit mantissa, quantised to `t` only once. Q24 for
  `li` recovers it. **This is why the fmul was not free to delete.**

**Both arms measured WORSE. Two ROMs, 128 frames each, frames 796..923, DLDI-on:**

| arm | P50 | P95 | over-gate |
|---|---:|---:|---:|
| control `407d9195` (Hermite), `r206-arena-heap-128` | 976,064 | 1,160,448 | 8/128 |
| **control re-built at HEAD**, `r206-head-control-128` | **976,064** | **1,160,448** | **8/128** |
| E6, fold materialised per node | 984,768 | 1,143,680 | — |
| E6, same but cache code deleted, `r206-e6-horner-128` | 983,232 | 1,166,720 | 10/128 |

Paired by frame index, the last arm is **worse on 117 of 128 frames, median +7,360**.

**The control replicate is the load-bearing row.** Rebuilt from HEAD into a third
distinct ROM (`1C1136BA` vs the control's `8F0CDAAC`) and re-measured, it reproduces
the control in **every one of the eleven buckets** — P50, P95, mean, min, max, the
VBlank histogram, `gNdsR2CubicEvals` — not approximately, identically. So this
harness has **no run-to-run noise at all** on a fixed configuration, and the E6
numbers above are the change, not scatter. It also retires any suspicion that E4b's
+33,984 was measurement scatter.

Which makes the *other* comparison the interesting one. **The two E6 ROMs execute
IDENTICAL arithmetic** — the cache never bound, see below — and they differ by
**23,040 at P95** while agreeing to ~1,500 at P50. With harness noise at zero, that
entire spread is **code layout**, and it is the third independent time this ROM has
shown 6,000–24,000 of P95 sensitivity to where a function lands (Task 94 −6,144,
E66 +24,448, this). **A cubic-sized change must be judged at P50 and on paired
frames; P95 on this ROM cannot resolve anything under ~20,000.** The middle row's
apparent −16,768 P95 is exactly that trap, and it is what a P95-only reading would
have graduated.

**Why it costs.** Anchored on the measurement rather than an op count: the
universal-fold arm is +49.5 ticks/node over Hermite, and Hermite ≈ the Horner
evaluator + one `__aeabi_fmul` + one SMULL, so **materialising six coefficients
costs ~107 ticks/node** — a `noinline` call, a 48-byte stack struct written and read
back, and four range clamps. The expansion removes an fmul and one multiply (~58
ticks) and pays 107 to do it. It does **not** remove conversions: five move into the
fold and one stays in the evaluator, so the count is six either way.

**The cache is the only thing that would pay, and it has no index here.** Built it:
a coefficient table on the taskman heap indexed by the AObj's slot in the pool
`gcSetupObjman` is handed, with a five-word exact validity compare — deliberately
neither of E64 arm A's two costs (no BSS; and `.text.hot` names only
`gcPlayDObjAnimJoint`, which stayed **272 bytes**, unchanged from E65). It never
engaged, and the engagement counters are what caught it: `CoefBytes=0`, `Folds=0`,
`Hits=0`, `BindFails=0`. Probed the statics directly — `sNdsR2CubicCoefPool` a real
address, **`sNdsR2CubicCoefPoolNum = 0`**.

**This configuration's task setup passes `aobjs_num = 0`.** `sGCAnimHead` starts
empty and `gcGetAObjSetNextAlloc` (`objman.c:602`) mallocs each of the ~300 live
AObjs **individually** at 32 bytes. There is no array to index. That is a durable
fact about this ROM and it refutes every AObj-indexed scheme, not just this one.

The two remaining routes were priced, not tried, and both land at the noise floor:

1. **Pointer hash** — arm A's shape, with its conflict misses, and arm A regressed
   +21,632 at an 86.4% hit rate.
2. **Pre-seed the pool** at `gcSetupObjman` so an index exists. Costs 61 KB of heap
   and moves every later allocation — the same heap-layout perturbation R2-06 E4b
   has open as a suspected 33,984. And it still would not pay: a cached node pays
   the 5-word compare + 15% of a 107-tick fold + one extra 48-byte coefficient cache
   line (~48 ticks) against the 58 it removes. **≈ −1,500/frame.**

**The general correction, and it applies to the rest of E61 §5's table:** that table
prices arithmetic operations. A memo is also a *memory stream*. Here the stream costs
what the arithmetic saves, and it is the more likely reason arm A regressed at an
86.4% hit rate than the BSS alone. Kernel diff and bound JSON were kept out of tree;
the bound result is reproducible from this description in one host run.

### R2-06 E5 — the designed lever: split the render skeleton from the gameplay skeleton

The plan of record already prescribes this and the tree violates it. §3.5 budgets
**"visual fighter pose 30 Hz"**; §3.6 is explicit: *"The renderer must never require
render skeleton == gameplay skeleton… the renderer consumes a compact generated pose
evaluated at presentation rate. They may share source data; they must not share one
expensive runtime representation."*

R2-04 E57 measured the violation: `gmCollisionGetFighterPartsWorldPosition`
(`gm/gmcollision.c:489`) places every hitbox by walking the **live** joint chain, so
dropping the whole chain to 30 Hz moves hitboxes and is a gameplay change. **E57
refuted halving the SHARED chain — it did not refute splitting it**, which is the
designed answer and is what §3.6 asks for.

The prize is the right size. E60/E61: the animation path is **146,942 ticks/frame**,
the cubic is **99.6%** of it, **149.4 cubic nodes/frame at 405 ticks each**, against a
**40,448** shortfall. Evaluating the full visual skeleton at presentation rate while
keeping only the hitbox-bearing joints at 60 Hz is worth far more than the gap.

**Measure before building**, per §3.9: how many of those 149.4 nodes does gameplay
actually read, and which? If hitboxes touch a small fixed subset, the split is cheap
and the rest of the skeleton is free to run at 30 Hz. If they touch nearly all of it,
this collapses back into E57 and must be recorded as such rather than forced.

## INSTRUMENT: the freeze detector was hashing the host's FPS counter (2026-07-29)

**Two soak verdicts withdrawn, and the failure mode is worth more than either.**
`Get-MelonDSWindowFrameHash` hashed `GetWindowRect`, which includes the window
**title bar** — and melonDS renders its frame-rate readout there (`[83/60] melonDS
1.0`). A completely hung ARM9 therefore produced a *different* hash on every poll,
because the host kept counting frames it was presenting from a dead guest. The
window frame also let the desktop bleed in at the edges. Withdrawn:

- "both-CPU post-fix, clean past 592 s, 2.8x past the failure point" — that run was
  hung at battle start the entire time, showing its boot screen.
- "tick-HUD pre-fix, NO-FREEZE 25 min, **150/150 samples distinct**" — and that
  perfect score was the tell. A live game repeats a frame now and then; a ticking
  counter never does. 150 of 150 distinct is a property of the instrument, not the
  ROM.

Fixed in `scripts/lib/melonds-screenshot.ps1`: `Get-MelonDSWindowBitmap` gained
`-ClientOnly` (`GetClientRect` + `ClientToScreen`, `PW_CLIENTONLY` on the
`PrintWindow` fallback) and the hash now uses it unconditionally. Measured on the
live emulator: window 616x955, client 600x916 — the 39-pixel title strip carrying
the FPS digits is exactly what used to be in the hash. Three consequences were
folded back into `scripts/soak-freeze-watch.ps1`:

- `NEVER-STARTED` replaces `NO-FREEZE` when the end-of-run attach reads zero
  presented battle frames. A run that never rendered gameplay was not soaked.
- `FROZEN-FROM-START` replaces the old blanket `CAPTURE-STATIC` when the picture
  never moved *and* `Measure-MelonDSWindowDistinctColors` finds more than two
  colours in the guest area. Chrome-free, "never moved" usually means a dead ROM,
  so the instrument must stop excusing it; a uniform grab is still `CAPTURE-STATIC`.
- `x/1i $pc` leads the capture, and the arena counters print in their own `printf`
  so one missing symbol cannot take `COUNTERS` down with it.

**Validated against a known-hung ROM, which is the test the original never had.**
`build-r2-bothcpu` could not start a battle 3/3; the old detector called it "alive,
54 distinct frames". The fixed detector returns `FROZEN-FROM-START` in under 40 s
with `b.n <self>` at the PC and `COUNTERS=849,0,0,0`. A liveness metric that can see
anything the guest does not draw is not a liveness metric — that is the rule this
cost, and it is recorded in `optimization/TASK_STANDING_RULES.md`.

**Three further defects in the same instrument, all the same shape: a verification
step was added and never confirmed to run.**

1. **`Invoke-SoakGdb` never existed.** The clean-run counter read called it from the
   day it was written and nothing defined it, so **every `NO-FREEZE` this
   instrument ever produced was pixels-only** — no match count, no arena size, no
   overflow latch. The failure lands *after* the verdict prints, which is why it
   was invisible. Now defined once and shared with the freeze path, which had been
   duplicating the same launch inline. **Machine-checked:** `check-melonds-policy.ps1`
   now AST-resolves every hyphenated command in every `scripts/*.ps1` against
   cmdlets, the file's own functions, and its dot-sourced libraries. It found a
   second live hit on its first run, negative-tests correctly, and treats a
   dot-source through a variable as "accept everything" so it cannot cry wolf.
2. **The trip threshold was shorter than the game's own dead air.** 4 samples ×
   10 s = 40 s, against a scene hand-off this board measured at ~30 s of NitroFS
   asset reloading with the last frame still on screen. A Sudden Death scene load
   duly tripped as a freeze. Default is now 8 (80 s).
3. **A static picture was being reported as a hang without checking.** The capture
   already contained the discriminator — `x/1i $pc` — and nothing read it. Verdicts
   now downgrade to `*-UNCONFIRMED` unless the PC disassembles to a branch to its
   own address, which is exactly what `while (TRUE);` and `ndsSyMallocOverflowHalt`
   compile to. A spin is now *confirmed* rather than assumed.

The clean read also prints `gSCManagerTransferBattleState.time_limit` and warns when
the soak outran the match, per the owner: *"if you want to run a longer soak for any
reason, then you also need to change the match timer to match the soak time."*
Default soak is 2.5 min (owner: *"5 mins is too long for the stress ROM"*), ceiling 5.

## HARNESS: DLDI was pinned by nothing, so automation never ran the owner's I/O configuration (2026-07-29)

The owner reports many freeze bugs and adds: *"I can reproduce the bugs in melonDS
with dldi checked."* That points at a configuration nothing in this repo controlled.

`[DLDI] Enable` was set in eleven `melonDS.toml` files and normalized by none of
them. Measured state before the fix:

```text
emulators/melonds/melonDS.toml           Enable = true      <- what the owner plays
emulators/melonds-attributor/...         Enable = false
emulators/melonds-runners/slot0..slot8   Enable = false     <- every scripted run
```

Neither `Set-MelonDSManualProfile` nor `Set-MelonDSAutomationProfile` touched the
section, so the split was invisible and permanent. **Every sharded verifier run in
this campaign used different I/O than the owner's play session**, which is a
sufficient explanation for "Boundary green on every graduation" coexisting with
"lots of random freezes": no automated run was in a position to see them.

Why it matters mechanically, not just procedurally: DLDI changes how `nitro:/` is
reached. On a flashcart the card interface belongs to the cart, so calico resolves
NitroFS by reading the `.nds` image back off the SD card through the DLDI driver
rather than through the card ROM interface. That is the path the published ROM runs
on real hardware, and the port reads `nitro:/` **by string path at runtime** — a GDB
attach during the results-screen probe caught the PC inside
`nitroromResolvePath` resolving `"FTMarioAnimWai"`. So the freeze class and the
DLDI setting plausibly share a mechanism, and the DLDI-off default hid it.

Fixed at the seam: a new `Set-MelonDSDldiProfile` is applied by both canonical
profiles, `check-melonds-policy.ps1` asserts the `[DLDI]` block per section (a bare
`Enable = true` needle would be satisfied by the unrelated `[Instance0.Gdb]` key),
and `Set-MelonDSWindowConfig.ps1` normalizes all eleven configs. `ImagePath` is
forced to the one repo-owned `emulators/melonds/dldi.bin` because the runner slots
have no image of their own and a relative path resolved, per slot, to a file that
does not exist; the automation profile additionally forces `ReadOnly = true`
because up to nine runners share that single image concurrently.

The assertion is negative-tested against each way it could be wrong: DLDI off, a
relative image path, a writable shared automation image, and a config whose only
`Enable = true` is GDB's — all four are rejected, and both real profiles are
accepted.

Two further items this exposed, both fixed here:

- `Set-MelonDSWindowConfig.ps1` **threw on every invocation** — R2-00a added
  `emulators/melonds-attributor/` and the normalizer's classifier had no branch for
  it, so the one tool that applies canonical configs to disk had been dead since.
  It now classifies the attributor and normalizes only the window and DLDI for it,
  keeping its local `dldi.bin`, because nothing in the repo assigns that build
  ports or a renderer and inventing them would silently change a measurement
  instrument.
- Nineteen `Start-Process` sites in `scripts/` still lacked `-WindowStyle Hidden`
  despite the CLAUDE.md rule; every melonDS launch had been fixed by hand and
  almost every `gdb` launch had not. Also fixed and also now machine-checked.
  See commit 69c11bb.

**Standing consequence: a setting that changes reproduction must be pinned by a
profile and asserted by a checker, not left to whatever is on disk.** Three of
today's four harness defects were the same shape — a rule applied by readers
instead of by code.

## R2-07 R0 MEASURED — the VS Results screen is 21.9M ticks per frame, 1.5 FPS (2026-07-29)

The owner: *"the results screen runs at less than 1 fps."* Confirmed, and the number
is far worse than the battle frame this campaign has spent weeks on. R2-07's clause
names the "GAME SET → results flow" and holds it to the same P95 ≤ 1.12M; nothing
had ever measured it.

Method, zero builds. The Results loop is the branch at `src/port/taskman_seam.c:6950`
and it carries no instrument: no `ndsPlatformBeginFrame`, no debug HUD, no VBlank
pacing, and it never increments `gNdsBattlePlayablePacingPresentedFrames` — which is
why the on-screen FPS reads `0.0` and the tick HUD reads `n:0` on that screen, and
why photographing the HUD there yields nothing. GDB stops freeze the emulator, so
`sVBlankCount` does not advance while stopped: a line breakpoint at each phase
boundary turns the loop into an exact VBlank timeline, and one breakpoint on
`ndsMNVSResultsRecordFrame` with an `ignore` count prices whole iterations. Both
are host-speed independent.

Cost is not flat across the scene — it climbs steeply as the wallpaper, fighters and
text come in, so where the window sits changes the answer by a factor of twenty:

| window | iterations | VBlanks/iter | ticks/iter | guest FPS |
| --- | --- | --- | --- | --- |
| results tics 1–15 | 15 | 2.0 | 1,120,380 | 30.0 |
| results tics 1–101 | 101 | 8.5 | 4,781,028 | 7.0 |
| results tics 131–181 | 50 | **39.0** | **21,858,614** | **1.5** |

Partition of the expensive window (50 iterations, 1,951 VBlanks; label order
verified as a consistent rotation, so `-Os` did not reorder the body and charging
each interval to the boundary that opened it is valid):

| phase | ticks/iter | share |
| --- | --- | --- |
| `tfunc->scene_draw()` | **19,550,631** | **89.4%** |
| `ndsSObjPreviewEndFrame` (commit) | 1,187,603 | 5.4% |
| `tfunc->task_update()` | 1,120,380 | 5.1% |
| input, audio, record, heapReset, endFrame | 0 each | 0.0% |

**The scene draw is 17.5× the entire frame budget by itself.** Not the sprite
commit, and not the source update — both of those are about one frame's budget each,
which is roughly what the battle path spends on comparable work.

Two structural reasons already visible in the source, neither yet measured
individually:

1. **The native OAM path is gated to the battle scene.** In
   `src/port/sprite_preview_backend.c:2410`, `ndsIFCommonNativeOamDrawGObj` is only
   attempted when `scene_curr == nSCKindVSBattle`. On Results every sprite instead
   goes through the software blitter `ndsDrawSObjIntoPreview` into a 320×240
   staging buffer, which is then nearest-downscaled to 256×192 in place and
   row-copied into BG VRAM. The DS has hardware sprites; this scene does not use
   them.
2. **Two full layers per frame.** `ndsDrawLayeredSObjFrame` splits Results by
   `dl_link_id != 26`, and each layer opens with
   `ndsPlatformBeginOriginalSpritePreview(320, 240, …)` — a 153,600-byte clear.
   Measured: `gNdsOriginalSpritePreviewCommitCount` advanced 22 in 101 iterations
   with `gNdsOriginalSpriteBg2CopyBytes` +2,162,688 = 22 × 98,304, i.e. exactly one
   full 256×192 background commit per commit and no foreground commits at all in
   that window.

Also worth recording, because it is the *visible* half of the owner's complaint: the
hand-off is dead air. A real-time capture put the frozen last battle frame on screen
at t≈105 s and "FOX WINS" at t≈135 s. The scene load in between reloads both
fighters' asset sets by string path through NitroFS.

**Correction to that framing (2026-07-30, read-only).** The loader is not the thing
to attack. BattleShip's `ftManagerSetupFilesAllKind`
(`decomp/BattleShip-main/decomp/src/ft/ftmanager.c:352`) is **already guarded** by
`if (*data->p_file_main == NULL)` — it reloads only because the battle teardown
already freed the files out of the taskman arena. So the lever is the **arena
lifecycle across the `nSCKindVSBattle` → `nSCKindVSResults` transition**, and the
first step is a measurement, not a change: bracket the transition with the same
`sVBlankCount` GDB technique `census-vsresults-blit.ps1` uses, around
`ndsBaseMNVSResultsStartScene` / `ndsMNVSResultsSetupFilesKind` / the file loads, and
find how much of the ~30 s is fighter assets versus everything else. Queued as R1.

Next, in this order: attribute the 19.55M inside `scene_draw` before touching
anything (the same loop that produced this table can bracket it), then decide
between admitting the native OAM path to Results and reducing the two-layer 320×240
software pipeline. Do not pre-commit to a fix; 89.4% in one phase is a partition,
not a cause.

### Second-entry item 3 ANSWERED — the "119 KB" was never a delta; item 4 audit closed (2026-07-31)

Run `artifacts/verification/sudden-death/2026-07-31_010341-sudden-death-entry.log`,
`build-sd-stg` with `NDS_R2_SECOND_ENTRY_DIAG=1`.

**The premise is withdrawn.** `~119 KB` was `1,226,768 − 1,107,392`. Those are
different quantities: the first is a measured high-water on this build, the
second is a derived "990,640 used plus a 116,752 request" peak demand from the
arena-sizing work above (line 198 of this file), computed for a stress config
that the same paragraph calls **192 KiB poorer**.

**Measured with the per-caller ledger instead.** Both entries begin their setup
from an **identical rewound baseline of 319,968 bytes** — read directly at
`scVSBattleStartBattle` and at `scVSBattleStartSuddenDeath`, not inferred.

| | allocated | high-water |
|---|---|---|
| match one, setup + a full minute of runtime | 925,816 | 1,245,784 |
| Sudden Death setup | 906,568 | 1,226,768 |

Sudden Death's footprint is **19,248 bytes LOWER**. 31 callers, overflow 0, no
new call sites, 22 of them byte-identical across both entries. Rematch is the
one remaining total; `soak-freeze-watch.ps1 -SecondEntryDiag` now reports it.

**Two things had to be measured before that could be said.** The ledger is
cumulative from boot, so "every caller exactly doubled" only proves
`sd = boot + m1` — equal to `m1` alone **iff** boot is zero. It is:
`SD-LEDGER-M1BASE-TOTAL=0`. And the wrapper's intra-TU blind spot is
`906,800 − 906,568 = 232` bytes, so the delta is safe to quote. Without those
two reads this was a coincidence wearing a proof's clothes.

**Item 4, persistent BSS holding taskman-heap pointers or VRAM handles: one
real gap, four already correct, nothing blanket-reset.** The animation cache
was stale and is fixed by the heap generation (engaged here: `SD-HEAP-GEN=2`,
`SD-CACHE-GEN-MISMATCH=1`). The native OAM texture names were stale — cleared
only at boot while their VRAM is released every scene change — and are fixed by
`ndsIFCommonNativeOamDiscardTextures` in the scene-cache eviction. The
static-texture latch, the stage validation cache and the T36 replay owner were
already discard- or generation-guarded. Detail in `docs/BUGS.md`.

**Harness defect fixed at its seam, in both lanes.** These scripts rebuild the
ROM every run and their build lines did not carry
`NDS_R2_SECOND_ENTRY_DIAG=1`, so a hand-built diag ELF was silently overwritten
and the gdb script kept reading symbols that no longer existed. **A gdb command
file aborts on the first erroring command, silently, leaving a bare prompt** —
two runs reached `battle-start` and printed nothing, which reads exactly like an
emulator hang. `capture-sudden-death-entry.ps1` and `soak-freeze-watch.ps1` now
both take `-SecondEntryDiag`, put the flag on the make line, verify it in the
generated config header the way `NDS_R2_BOTH_CPU` is verified, and the former
strips every diag-only read when the flag is off. If a lane prints nothing after
an early stage, check the ELF exports before suspecting the emulator.

### R2-07 clause 2 — BOTH particle branches merged and BUILDING (2026-07-31)

**The particle-bank work compiles for the first time since it was written.**
Generator branch (2,710 insertions) and runtime branch (**151 files, 46,023
insertions**) both merged onto `codex/r2-runtime2`;
`smash64ds-battle-playable-tickhud-hwtri` builds clean with the bank linked.

**It was never broken — it was stale against a directory move.** The
2026-07-30 area-folder reorganisation broke this branch **twice, in two
languages**, and both failures were invisible until something ran the
generator, which is exactly why it sat "unreviewed, unbuilt":

| where | symptom | truth |
|---|---|---|
| `generate_nds_particle_banks.py` | checker said *"pack differs from its BattleShip sources"* | `ModuleNotFoundError` — the checker turns any non-zero exit into that message |
| `Makefile` `.inc` rule | `No rule to make target …generate_task39_effect_census.py` | prerequisite still pointed at the pre-reorg path |

Both now resolve by location (`scripts/2d_vfx/`), so a future reshuffle breaks
loudly instead of importing something else.

**Verified, not just built.** The pack reproduces byte-identically from the
BattleShip sources — the R2-05 E0 bar for a generator:

```
NDS_PARTICLE_BANKS=PASS  55/119 reachable efcommon scripts, 23/47 textures
136,248 B N64 texture -> 82,752 B DS
95,043 B linked (93,760 payload + 1,283 index) of 210,320 B arena headroom
115,277 B spare · 6 bit-exact CI4 textures · linear texel order pinned · .inc built
source=0xa2a1e85f  table=0x8db9d3bd
```

Docs check and tick-HUD parity also green, so the instrumented ROM is still the
shipping program.

**A/B RUN, AND THE MERGE IS INERT.** 128-frame sample against
`r207-L10-sqrt-128`, the pre-merge control from the same session and harness
(`artifacts/performance/r207-particles-128*`): **every bucket byte-identical** —
`WORK-H` P95 1,232,448, P50 921,920, VBI `2:465 3:89 4:9 5+:4 max:19`,
named 978,456.

By this session's own rule, identical arms mean the input did not take effect,
so I checked the built config instead of publishing a "costs nothing" result:
**`NDS_R2_PARTICLE_RUNTIME 0`** (`Makefile:596`). The runtime is default-off,
the linked bank never executes, and **zero cost is the correct reading — of a
subsystem that is not running.** It is not evidence the particles are
affordable.

**So clause 2 is merged, buildable, byte-reproducing and priced at nothing
because it is switched off.** The pricing experiment is `NDS_R2_PARTICLE_RUNTIME=1`
plus a re-sample against this same control, and it is a real one: `lb/lbparticle.c`
is a 2,961-line bytecode interpreter against a cosmetic budget of roughly 23K
ticks (~fourteen textured-quad binds at Task 98's 1,621/bind).

**`NDS_R2_PARTICLE_RUNTIME=1` DOES NOT BUILD, and the cause is localized.** Ran
it; the failure is **symbol drift between the runtime branch's placeholder and
the generator branch's header**, which is precisely the seam two independently
developed branches would break and precisely what "unreviewed, unbuilt" was
hiding:

- `src/nds/nds_particle_banks_placeholder.c:24` — **conflicting types for
  `gNdsParticleScriptBank`**: the placeholder declares `const u8[1]` while the
  generated `.inc` defines the real array. Same for `gNdsParticleTextures`
  (`:36`, `const NDSParticleTexture[1]`). The placeholder is meant to stand in
  when the runtime is off; with it on, both definitions are live.
- `:28` **`NDS_PARTICLE_SCRIPT_IDS` undeclared** — generator emits
  `NDS_PARTICLE_SCRIPT_COUNT`. `:14` **`NDS_PARTICLE_UNPACKED_OFFSET`
  undeclared** — generator emits `NDS_PARTICLE_UNPACKED_56`. **The placeholder
  was written against an older generator contract.**

The `fighter.h` / `lbcommon.h` redeclaration noise in the same log is include-order
fallout from that TU and should be re-read after the symbol drift is fixed, not
chased first.

**FIXED, exactly as the code itself prescribed.** `nds_particle_banks_placeholder.c`
opens with *"delete this file once it is in the build"* and `Makefile:1588` said
*"Delete it with the same commit that adds the generated data."* The generated
data is in the build, so the file is deleted and dropped from `CFILES`. **All
placeholder-drift errors are gone.**

**The remaining failure is a DIFFERENT class, and separating them was the
point.** 826 errors, all in the `battleship_lbparticle.c` translation unit, and
they are missing declarations rather than conflicts:

- `battleship_lbparticle.c:69` — **`ndsBaseLbParticleMakeGenerator` implicitly
  declared** ("did you mean `lbParticleMakeGenerator`?"). The import seam's
  `#define` rename has no matching port-side definition.
- `lbparticle.c:1620/1640` — **`guMtxCatF`, `guMtxIdentF` implicitly declared**:
  the libultra matrix helpers the particle code needs are not reachable from
  this TU.
- `nds_startup.h:866` `sb32`, `lbcommon.h:43` `alSoundEffect`, then the whole
  `fighter.h` enum re-declaring — **include-order cascade**, secondary.

**CORRECTION — I had the priority backwards, and the build says so.** I fixed
the two implicit declarations first (prototypes for the renamed constructors,
plus `<PR/gu.h>` for `guMtxCatF`/`guMtxIdentF`) and the count went **826 → 836**.
**Reverted.** The implicit declarations are themselves cascade.

**The root cause is the FIRST error: `nds_startup.h:866` uses `sb32` in a
prototype, and `sb32` is not defined at that point in this translation unit.**
`battleship_lbparticle.c:36-40` includes `nds_scene_harness_config.h`,
`nds_particle_runtime.h` and `nds_startup.h` **before** `lb/library.h` — so it
is the one TU that reaches `nds_startup.h` with no decomp type header ahead of
it. Every other TU gets the base types first by accident of include order.
Nothing caught it because this file has never been compiled.

**FIXED.** `lb/library.h` now precedes the `nds/` headers in
`battleship_lbparticle.c`, and **the `sb32` error is gone** (826 → 825, and it
is no longer first). Fixed in the file being brought into the build rather than
in `nds_startup.h`; **the better seam is still for `nds_startup.h` to include
its own types**, and it stays a trap for the next TU that includes it early.

**Then `alSoundEffect` — FIXED** (`825 → 824`). Same shape one level down:
`lb/lbcommon.h:43` declares an `alSoundEffect *` field without the type, and
`include/sys/audio.h:31` is where the typedef lives. Added ahead of
`lb/library.h`.

**Next root error, and it is NOT cascade after all:**
`include/ft/fighter.h:50` **`redeclaration of 'enum FTKind'`**. I had filed the
`fighter.h` enums as fallout twice; now that the two errors above are gone it
stands first, which makes it a root cause. **`enum FTKind` is defined in BOTH
`include/ft/fighter.h` and `decomp/.../ft/ftdef.h`**, and this TU reaches both.

**It is the same collision class the seam already solves twice** —
`SSB64_NDS_LBTRANSFORM_DECLARED` at the top of this file, and
`SSB64_NDS_MPOBJECTCOLL_DECLARED` at `include/ft/fighter.h:237`. **FIXED that
way** — `SSB64_NDS_FTKIND_DECLARED` guards the port copy, the seam defines it
and takes the decomp's. **824 → 790, and no new errors anywhere else**, which is
the check a change to a header ~100 TUs include actually needs.

**The remaining pattern is now visible, and it changes the approach.** Next up
is `include/ft/fighter.h:94` **`redeclaration of 'enum FTPlayerKind'`** — the
same duplication, the next type down. This is a *series*, not a one-off: the
port's `fighter.h` and the decomp's `ft/ftdef.h` overlap across many
declarations, and guarding them one at a time costs a full build each.

**Prefer a block-level answer over N single guards** — one guard around the
whole overlapping region of `fighter.h`, rather than one per type.

**Correction to the commit that recorded this:** I wrote that the port
`fighter.h` "arrives indirectly through `nds/nds_startup.h`" and that fixing
that header's dependencies "would likely resolve both". **Wrong** —
`nds_startup.h` includes only `<stddef.h>` and `<PR/ultratypes.h>` and pulls no
fighter header at all. Both copies arrive through the decomp/port header webs
(`lb/library.h`, `sc/scene.h`, `sys/taskman.h`), so **the two problems are
unrelated and `nds_startup.h` is not the shared seam.** Its own `sb32` defect is
real and still worth fixing, but it will not touch the enum series. Establish
which header actually pulls each copy before choosing where the block guard
goes.

**Working method for this file, which has never been compiled and so surfaces
one seam at a time:** fix the first error, rebuild, re-read the first error.
Three root causes down (reorg import path, reorg `.inc` prerequisite,
placeholder drift, `sb32` order — four), each invisible until something ran it.

**So clause 2's remaining work is the import seam for one file**, not the merge
and not the generated data. **Still not done after that:** the pricing
measurement, a Boundary run with the runtime on, a visual check, and the eight
open VFX rows, which stay open until the effects are on screen. The `115,277 B`
of arena spare answers the *memory* question only.

### R2-07 COMPLETION SEQUENCE — the phase is mostly CONTENT, not performance (2026-07-31)

Read back from the SwitchPlan's own clause list rather than from the board's
momentum, because the two had drifted apart. **R2-07 has four clauses and I have
been working only the fourth.**

| # | clause | state |
|---|---|---|
| 1 | START on Results restarts the match | **CLOSED** (BUGS.md row 1) |
| 2 | particle banks, SFX/voice/BGM, HUD, GAME SET → results flow, with budgets | **OPEN — untouched this session** |
| 3 | **all P1-scoped rows in `BUGS.md` fixed** | **OPEN — 9 of 10 rows** |
| 4 | gate: demo loop in budget, battle P95 ≤ 1.12M DLDI-on | OPEN, **1,208,960** at HEAD (deterministic harness, two identical control runs); L9+L10+L9b banked, **L7 wired, measured and REVERTED** — no lever named |

**`BUGS.md` P1 rows, actual status.** One `FIXED`, one part-fixed, eight open —
and **seven of the ten are VFX/SFX**, which is clause 2 wearing clause 3's
clothes:

`FIXED` START-restart · **OPEN** rematch drawn wrong (duplicated fighters over
corrupted stage) · **OPEN** "Time Up" VFX+SFX · **OPEN** Results VFX+SFX/BGM/FGM
· **OPEN** crowd noise · **OPEN** Sudden Death FPS/freeze/animation · *part*
wind hazard (gameplay+SFX fixed, **VFX open**) · **OPEN** wrong VFX (foot dust,
fireball hit, Fox down-B, hard landing) · **OPEN** upward-KO VFX+SFX never play
· **OPEN** KO VFX wrong.

**The particle-bank branches are INTACT — verified, not assumed.** Both carry
real work and neither is checked out anywhere:

- `worktree-agent-a15dedc9b2cf19349` — the **generator**: 1,310-line
  `generate_nds_particle_banks.py`, a 207-line checker, the generated JSON and
  header. 2,710 insertions over 9 files.
- `worktree-agent-a8c9ad131bc0073b0` — the **runtime**: 151 files,
  **46,023 insertions**, including the renderer and taskman seams.

**So the next action is content, not another tick.** Start from those branches —
the handoff's "do not rewrite" still holds, and 46,023 insertions is not
something to re-derive. Six of the eight open VFX rows need them.

**Sequence:** clause 2 (particle banks off the two branches, then SFX/BGM rows)
→ clause 3's non-VFX remainder (rematch corruption, Sudden Death) → clause 4
→ R2-08 (flip the Boundary, rebuild the published ROMs, owner retail test).
**Performance is one of four clauses and is the only one with momentum; the
other three are where the phase actually is.**

**Clause 4 no longer has a named lever.** L7 was the last one and it is refuted
(see the L7 REVERTED row). Its measurement also corrected the baseline, in both
directions at once:

| | `WORK-H` P95 | mean | artifact |
|---|---|---|---|
| L9 control `2a53c061cd1` | 1,281,856 | 989,892 | `r207-L9-control-128.json` |
| L10 `b06f16567dc` | 1,232,448 | 966,759 | `r207-L10-sqrt-128.json` |
| **HEAD `800a934`** | **1,208,960** | 973,484 | `r207-L7-control-128.json` + `-control2-` |
| this file's old claim | 1,147,200 | — | **none — no artifact exists** |

So HEAD is **23,488 better** than the last attested figure, not worse; the
27,200 gap was never measured. The real gap is **88,960**, and the two HEAD runs
are bit-identical, so it is a number to work against rather than re-derive.
Two things to note while reading the table: `r207-particles-128.json`
(`1294a388175`) reports P95 **and** mean identical to `r207-L10-sqrt`
(`b06f16567dc`) to the digit, which on a deterministic harness means either the
same binary or the same run relabelled — do not cite it as an independent point.
And P95 fell 23,488 while the mean ROSE 6,725, so HEAD is not uniformly faster.

Before naming another
lever, read the two general findings the L7 measurement produced — hot ARM text
costs ~1.85 cycles/frame of FTR mean per byte, and replacing soft float with
fixed point was worth 99 cycles per call, not the 800 the estimate assumed.
Both say the same thing: **the next lever must delete work, not relocate it.**

### R2-07 H1 — where the static RAM went, and why the SD margin is thin (2026-07-31)

Sudden Death survives on 42,992 bytes free, and the taskman arena is sized by a
boot loop that steps **down 0x1000 per failed `calloc`** — so every byte of
static RAM comes straight out of the game's heap, and the next 4 KB anyone adds
re-freezes SD exactly as L9's sine table did. `nm -S --size-sort` on the tickhud
ELF, largest `.bss`/`.data` first:

| bytes | symbol |
|---|---|
| 441,600 | `gSYFramebufferSets` |
| 204,800 | `sNdsAudioFgmCache` |
| 185,696 | `sNdsRelocSceneFileBuffer` |
| **153,600** | **`sOriginalSpritePreview`** |
| **153,600** | **`sOriginalSpriteDisplayPreview`** |
| 140,800 | `gSYZBuffer` |
| 32,768 | `sNdsRendererHardwareTextureScratch` |
| 30,688 | `sNdsRendererTask36ReplayOwner` |
| 29,184 | `sNdsRelocLoadedFiles` |
| 27,136 | `sNdsFighterDLAllDrawStates` |
| 13,824 | `sOriginalDLPreview` |

**The two preview buffers are 307,200 bytes — 7× the entire SD margin — and
they are declared ungated** (`nds_platform.c:114` and `:196`, no `#if`), while
the functions that DRAW them, `ndsPlatformDrawOriginalSpritePreview` and
`ndsPlatformDrawOriginalDLPreview`, sit inside `#if !NDS_RENDERER_HW_TRIANGLES`
— compiled out of every battle ROM, which builds `NDS_RENDERER_HW_TRIANGLES 1`.

**They are NOT trivially dead, so do not just delete them.** Checked before
claiming it: the write at `:482` is outside any `!HW_TRIANGLES` guard, and `:511`
is a live getter returning `sOriginalSpriteDisplayPreview`. So in a
hardware-triangle build the buffers are still populated and still handed out —
they are drawn nowhere, which is a different thing from unused.

Next step is a liveness audit, not a deletion: find every reader of both buffers
that survives `NDS_RENDERER_HW_TRIANGLES=1`, and establish whether the getter's
callers are diagnostic (verifier/preview) or on the shipping path. If they are
diagnostic, gating both buffers on `!NDS_RENDERER_HW_TRIANGLES` returns 307,200
bytes to the arena — 75 pages of arena sizing loop, against an SD margin that is
currently one page.

### R2-07 clause 2 — THE PARTICLE RUNTIME RUNS A FULL MATCH. The memory blocker is closed (2026-07-31, night)

`NDS_R2_PARTICLE_RUNTIME=1`: **NO-FREEZE, full one-minute match to Results,
`MALLOCOVF=0`**, on both the human-vs-CPU and the both-CPU stress config. Bank
loads (`LoadResult=1`, 55 packed, 0 rejected), draw seam runs 20,578 times.

Three levers closed the 25,600-byte gap, and the general heap tracks them:

| | free heap at the latch |
|---|---|
| start of day | **1,040** |
| + float printf out of the image (+3 arena steps) | — |
| + pools sized for P1 instead of the whole game | **14,756** |
| + bank normalized in place instead of copied | **boots** |

- **Pools.** `efParticleInitAll` reserved 112/24/80 = 28,320 bytes of arena for
  four players, items and every stage's hazards. P1 is two fighters on one
  stage; 40/10/24 now, and `StructsMax` is still **0**, so even that is loose.
  The interposition is safe because `efParticleInitAll` has no caller inside
  `efparticle.c` or `lbparticle.c` — the `#define` moves the definition without
  taking a call site with it.
- **Bank copy.** `syTaskmanMalloc(10,912) + memcpy` existed only to obtain
  somewhere writable to byte-swap into. Dropping `const` from
  `gNdsParticleScriptBank` gives that for nothing — same image bytes, `.data`
  instead of `.rodata`. One-shot latch, because an in-place swap run twice
  swaps back and the bank outlives the scene (§3.12).

**NEXT DEFECT, LOCALIZED: the efcommon pack registers in a bank slot the seams
do not ask for.** `ScriptStartCount` is **0** on both configs — including the
both-CPU stress run where FGM play calls rose 104 → 187, so fights and hits are
definitely happening. The new reject ring names every refusal, and **not one is
the efcommon bank**:

| bank | script | reason | count |
|---|---|---|---|
| 1 | 98 | unreachable in pack (fail-closed, working) | 8 |
| 0 | 0 | script id out of range | 2 |
| 0 | 1 | script id out of range | 2 |
| 0 | 112 | unreachable — the Results confetti the generator excludes | 2 |

Reason 2 on bank 0 means `sLBParticleScriptBanksNum[0] == 0`, i.e. **bank 0 is
empty**, so the pack did not register there. Find which slot
`ndsParticleLoadEFCommonBank` actually filled and which id the seams pass; that
mismatch is the whole of "the scripts do not run". Script 112 is a separate,
known gap — `docs/BUGS.md` already records that the generator excludes it.

Also seen on the stress config and not chased: `gNdsR2AnimCacheRejects` 109 with
`ArenaOverflows` 109, against 0 on the passive run.

### R2-07 arena — three steps bought, from a float formatter nothing ever called (2026-07-31, evening)

The arena is R2-07's critical path: it blocks `NDS_R2_PARTICLE_RUNTIME=1`, and on
2026-07-31 it blocked a 2 KB *instrument*. The board listed float `printf` as a
~24,375-byte lever. It is real and it is now banked.

newlib picks its formatter by **symbol, not by format string**. Seven call sites
— six `"%s"`/`"%lu"` path builders in `nds_reloc_assets.c` and the tick-HUD line
printer, whose **73 call sites contain no `%f` at all** — used `snprintf` /
`vsnprintf`, which links `_svfprintf_r` (10,047), `_dtoa_r` (4,608) and `__mprec`
(2,480). Switching to `sniprintf` / `vsniprintf` drops all three:

| | before | after |
|---|---|---|
| float formatter linked | 17,135 B | **0** |
| taskman arena (tickhud) | 1,290,240 | **1,302,528** |
| arena search failures | 21 | **18** |

**+12,288 = three arena steps, in every configuration**, NO-FREEZE through a full
match to Results, `MALLOCOVF=0`, Boundary and Latest green.
`scripts/check-printf-integer-only.ps1` bans the float-capable spellings so it
cannot come back; the HUD already shows how to print "24.1" without one.

`libc_a-categories.o` (14,420) is still linked, now pulled by
`libc_a-iswspace_l.o` rather than by the formatter. That chain is inside libc and
was not chased; it is the remaining ~3 steps of the original estimate.

### R2-07 L7 — WIRED, MEASURED, REVERTED. The kernel wins 534 and the code size costs 6,481 (2026-07-31, night)

Supersedes every "L7 should close the gap" estimate below. **Do not re-attempt
L7 as "convert gmCollisionSetInvertMatrix"; that experiment is finished.**

The wiring works and the arithmetic is right. Neither was the problem.

**Hook.** `gmCollisionSetInvertMatrix` cannot be intercepted — a `#define`
rename moves a decomp definition and its call sites together, and its only
caller (`func_ovl2_800EDE00`) is in the same file, as are that function's nine
callers. The first externally-visible ring is the eight `gmCollisionCheck*`
entry points plus `func_ovl2_800EE018`. Wrapping those and filling
`unk_dobjtrans_0x9C` before delegating makes the decomp's float prepare
early-return on its own latch — same joints, same order, no speculative work,
every hit-test decision still in decomp code. **Engagement proven:**
`FillCount=691` over 128 frames, `DeclineCount=0`, `SkipCount=41`.

**Arithmetic.** A full-matrix form was needed (the frame form is not a matrix,
so it cannot be wired without rewriting every consumer). First draft measured
0.132935 — straight back to the frame's old RED — because **the amplifier is the
INPUT quantisation, not the output rounding**: 20.12 puts 2^-13 in every
rotation cell, the cofactors carry it into R^-1, and t multiplies it by 400.
Reading the rotation block at **6.26** instead fixes it outright, and the result
beats the frame on every domain, including the one the frame loses:

| domain | frame max | matrix max |
|---|---|---|
| near-unit 0.90–1.10 (gated) | 0.014758 | **0.000283** |
| moderate 0.50–1.50 | 0.069663 | **0.000488** |
| conservative 0.25–2.00 | 0.367933 | **0.000900** |

`scripts/check-r2-collision-mtx.ps1` is new and registered — the harness the
header had been claiming for a week did not exist, which is how its quoted
figures went stale by a revision. It also caught a real overflow the eye did
not: at det < 1/32 the Q26 reciprocal leaves int32 and the conservative sweep
read **160.755318 world units** until the determinant guard was added.

**And it is still a REVERT, on a deterministic harness.** Two control runs came
back **bit-identical in every bucket**, so nothing below is noise:

| | control | cand1 Q30/ldivmod | cand2 Q26/hwdiv |
|---|---|---|---|
| `WORK-H` P95 | 1,208,960 | 1,259,328 | **1,300,928** |
| `WORK-H` mean | 973,484 | 985,180 | **979,635** |
| `SRC` mean | 316,298 | 317,158 | **315,764** |
| `FTR` mean | 382,862 | 387,126 | **386,296** |
| `STG` mean | 172,997 | 177,942 | **176,044** |

**The kernel wins 534 cycles/frame in its own bucket and loses 6,481 in two
buckets that contain no collision code at all.** That is placement cost, and the
proof it is placement is that it scales with the code, not with the work: cand1
added 2,332 bytes of ARM text for +4,264 FTR mean, cand2 added 1,840 for +3,434.
**1.83 and 1.87 cycles of FTR mean per byte** — the same constant twice.

Three things this settles, all of them general:

1. **Hot ARM text costs ~1.85 cycles/frame of FTR mean per byte on this ROM.**
   Any lever that adds code must beat its own size at that rate before its own
   saving counts. This is the measured form of the standing "tail fix must not
   add to the body" rule, and it is the first time the campaign has a number.
2. **Soft float here is far cheaper than the campaign assumed.** 61 `__aeabi_*`
   calls were replaced by 36 SMULLs, a hardware divide and 24 bit-twiddled
   conversions, and the whole substitution was worth **99 cycles per call**
   (534/frame over 5.4 fills). "66.2% of the premium is soft-float" is a true
   statement about where cycles are; it is not a promise that fixed point is
   cheaper. Price the replacement, never the target.
3. **Wrapping is additive; only replacing is subtractive.** The float
   `gmCollisionSetInvertMatrix` (295 Thumb instructions, ~590 bytes) stayed
   linked the whole time. A conversion that deletes what it replaces starts
   6,481 cycles/frame ahead of one that sits beside it.

What survives: `include/nds/nds_r2_collision_mtx.h` (kernel, graded),
`scripts/check-r2-collision-mtx.ps1` (harness, registered), and the hook
recipe above. What is gone: the wiring and its `NDS_R2_COLLISION_L7` flag.

**If collision is attempted again it must be the whole subsystem, not one leaf.**
`func_ovl2_800ED490` is the bigger half — 228 instructions and **63** soft-float
calls against the invert's 61, run 40×/frame against 34 — and it is a plain 3×4
compose with no divide and no cofactors, so its fixed-point form is far leaner
than the inverse's. Converting compose **and** invert together, with the decomp
versions dropped rather than bypassed, is the only shape where the arithmetic
win can exceed the placement cost. There is also a cheaper inverse available on
the measured domain: the joint matrix is a rotation scaled per row, and the live
oracle measured all three scales as a single value 1.114–1.120, so
`R^-1[c][r] = M[r][c] / s_r^2` — three reciprocals and nine multiplies instead
of nine cofactors, a determinant and nine multiplies — with `vec_scale` already
computed by `func_ovl2_800EDE5C`. It needs an orthogonality guard, because a
composition of non-uniformly scaled rotations shears.

### R2-07 L7 — the arithmetic is CLEARED on the real domain, and the named hook does not exist (2026-07-31, evening)

The SwitchPlan's own next action. The kernel was green on a *synthetic* sweep and
its header said so: *"It still has to be measured against real hurtbox dimensions
and real joint scales."* `NDS_R2_COLLISION_L7_ORACLE=1` is that measurement — a
read-only pass after the gameplay tick over every part whose invert latch the
decomp set this frame, re-doing the inverse in 20.12 and comparing against the
decomp's own `gmCollisionGetWorldPosition`. 460 samples, one natural mode-163
match, NO-FREEZE, arena unchanged at 1,290,240:

| | measured | falsifier | bound |
|---|---|---|---|
| joint scale | **1.1138 – 1.1199** | gated 0.90–1.10 | — |
| deviation, probe 1 unit | **0.00049** | 0.016609 | 0.0200 |
| deviation, probe 4 units | **0.00122** | " | " |
| deviation, probe 16 units | **0.00513** | " | " |
| over bound | **0 of 460** | — | — |
| singular joints | **0** | — | — |

**The real domain is one scale, ~1.12, spanning 0.006.** The 0.25–2.00 sweep that
reads 0.427738 is not a domain SSB64 visits; the gated sweep is near-right but
centred slightly low and should move to 1.11–1.12. And the synthetic number is
**pessimistic** — 0.016609 against 0.00513 measured, so the margin at the
furthest probe is ~4x the bound, not the 1.2x the falsifier implies.
**L7's arithmetic is cleared. Only the wiring is left.**

**AND THE HOOK THIS ROW NAMES DOES NOT EXIST IN THE SHIPPING BUILD.** The row
below concludes "the port already owns the structure collision reads" because
`ftGetParts` resolves to `sNdsFighterPartsPool`. The linker map refutes it:
`.bss.sNdsFighterPartsPool` contributes **0 bytes** to `build-tickhud` and
**33,152** the instant anything outside references it — `ndsFighterPartsSyncDObj`
and `ndsFighterStructPopulateJointsRecurse` are eliminated there too, and `nm`
finds `sNdsFighterStructPool` but not this one. Anything hooking L7 on that pool
would fill an array nothing populates. The oracle walks
`gGCCommonLinks[nGCCommonLinkIDFighter]` → `gcGetTreeDObjNext` → `ftGetParts`
instead, which is live in every configuration and costs no storage: **+2,260
bytes** against the pool draft's +35,428.

That 33,152 is also a standing warning. It is **eight arena steps**, and the
first oracle draft dropped the battle straight under `ifCommonSetMaxNumGObj`'s
25 KiB latch — the same failure the particle runtime hits, reached by adding an
*instrument*. The arena is now tight enough that measurement itself has to be
sized. Check `mapdiff` on any new lab flag before running it.

### R2-07 L7 — kernel GREEN, seam FOUND at the entry surface (2026-07-31, later)

Supersedes the RED status below on both counts.

**Kernel GREEN.** The `(p - t).R^-1` restructure stopped storing `-t.R^-1`, which
was the amplifier: `t` is a world coordinate in the hundreds and `R^-1` has
entries around `1/scale`, so forming that product in 20.12 commits a large
intermediate's rounding error to storage where nothing later cancels it. Matched
control, same binary and RNG stream, only the kernel swapped:

| domain | before | after |
|---|---|---|
| near-unit (0.90–1.10, **gated**) | 0.126987 | **0.016609** |
| moderate (0.50–1.50, reported) | 0.133385 | 0.051753 |
| conservative (0.25–2.00, reported) | 0.400510 | 0.427738 |
| compose (near-unit, gated) | 0.017817 | 0.017817 |

Compose being byte-identical is the control that says the win is the invert path
and nothing else. Two things that control caught: the figures the header had been
carrying (0.0226 / 0.3706) were stale from an older revision, and the
conservative domain got marginally **worse** — at extreme scale spread the
translation was never the dominant term, so removing it buys nothing there.

**Seam FOUND, and the blocker below was diagnosed at the wrong altitude.** A
`#define` rename moves a definition and its call sites *together*, so it can add
a differently-named copy but can never intercept a call made inside the same
file. All four leaves — `gmCollisionSetInvertMatrix`, `func_ovl2_800ED490`,
`func_ovl2_800EDE00`, `func_ovl2_800EDE5C` — are called only from within
`gmcollision.c`. But **every one of the ~20 `gmCollisionCheck*` entry points is
called from outside**, as are `func_ovl2_800EDBA4` (5 files),
`func_ovl2_800EE018` (3), `func_ovl2_800EDA0C` (2). L7 is not blocked.

**Do not interpose `func_ovl2_800EDBA4` alone** even though it is the cheapest
hook: its internal callers `func_ovl2_800EDE00`/`800EDE5C` *are* the
hit-detection path, so they would keep the float version while external callers
took fixed point — two writers of `mtx_translate` with different rounding.

Shape: a native owner over the whole `gmCollisionCheck*` surface behind
`NDS_R2_COLLISION_NATIVE`, decomp retained as oracle, matching R2-03's fighter
renderer. Nothing exists to extend — `reloc_backend_mp_collision.c` is MAP
collision (stage geometry), not GM hit detection, and there is no `NDS_R2`
collision flag family yet.

What makes the restructured kernel legal, verified by reading every consumer
rather than assuming: `unk_dobjtrans_0x9C` is produced once at
`gmcollision.c:465` and consumed **only as a point transform** — `:537` via
`gmCollisionGetWorldPosition`, and `:1396/:1441/:1515/:1549/:1574/:1740/:1774/:1799`
via `gmCollisionTestRectangle`. It is never composed, never re-inverted, never
stored onward.

**The hook is probably the latch, not an interposition.** `func_ovl2_800EDE00`
and `func_ovl2_800EDE5C` are *lazy*: each early-returns when its latch is
already set —

```c
if (parts->unk_dobjtrans_0x7 == 0) { ...float work...; parts->unk_dobjtrans_0x7 = 1; }
```

— and those latches are cleared **only at part creation**
(`ft/ftmanager.c:264-265`), never per frame by the decomp. So a port-side pass
that fills `unk_dobjtrans_0x9C` in fixed point and sets the latch *before* the
decomp's prepare runs makes the float work skip itself, with **every hit-test
decision left in decomp code**. That is a far smaller change than owning the
~20 `gmCollisionCheck*` functions, and it cannot alter hit results beyond the
kernel's proven bound.

**And the port already does exactly this shape.**
`src/port/reloc_backend_fighter_model.c` owns
`sNdsFighterPartsPool[GMCOMMON_PLAYERS_MAX][...]` (`:20`, bzero'd `:1463`,
populated `:1670`), writes `mtx_translate` and `unk_dobjtrans_0x9C`, and sets
all three latches to 1 — i.e. "already prepared, do not recompute".

**CONFIRMED: `ftGetParts` does resolve to that pool.** It is only
`(FTParts *)(dobj)->user_data.p` (`include/ft/fighter.h:440`), and
`reloc_backend_fighter_model.c:1715` sets `dobj->user_data.p = parts` with
`parts = &sNdsFighterPartsPool[slot][joint_id]` (`:1670`), inside
`ndsFighterPartsSyncDObj`. **So the port already owns the structure collision
reads, and L7 is a fill inside an existing owner — not a new owner, and not the
`gmCollisionCheck*` surface.** That is a much smaller change than the paragraph
above proposed.

**RESOLVED 2026-07-31: the three latches are a union with a word, and the port
already owns the clear.** `FTParts` (`include/ft/fighter.h:409`) declares

```c
union {
    struct { u8 unk_dobjtrans_0x4, _0x5, _0x6, _0x7; };
    s32 unk_dobjtrans_word;
};
```

so `parts->unk_dobjtrans_word = 0` clears **all three prepare latches in one
store**. That is what `ndsFTParamsInvalidateFighterParts`
(`src/port/reloc_backend_compat_shims.c:1462`) does as it walks the joint tree,
reached per frame through `ftParamsUpdateFighterPartsTransform` whenever a
joint's transform changes — and the decomp does the same at `ft/ftparam.c:2183`,
`:2302`, `:2411`.

All three observations reconcile with no contradiction: the port sets the latches
at init (one-shot), the word-clear knocks them down per frame when joints move,
and `func_ovl2_800EDE00` therefore re-runs `gmCollisionSetInvertMatrix` — 34× on
a hit-detection frame, 0× on a clean one, exactly as L6 measured.

**So L7's hook is `ndsFTParamsInvalidateFighterParts`.** It is port-owned, it is
per-frame, and it is the precise moment the float path is about to be re-entered
— which is where the fixed-point fill belongs. Note it clears the word *without*
dereferencing anything, so the fill must come after, at the point of first use,
not inside the invalidation walk.

**Superseded — this was the open question, kept for the reasoning:**
`ndsFighterPartsSyncDObj` is called from `ndsFighterStructPopulateJointsRecurse`
(`:2043`) and once for the root (`:2084`) — a joint-**population** pass, i.e.
setup, not per-frame. Nothing in the decomp or the port clears
`unk_dobjtrans_0x5/0x6/0x7` after creation (`ft/ftmanager.c:264-265` is the only
clear). Yet L6 measured `gmCollisionSetInvertMatrix` running 34× on an over-gate
frame, so the float prepare *is* re-running. Both cannot be true as stated.

Resolve that before writing any fill. The candidate is
`transform_update_mode`: `func_ovl2_800EDBA4` (`gmcollision.c:340-372`) walks
parent-ward and breaks on the first part whose `0x5` is set, and at the root it
re-runs `gmCollisionTransformMatrixAll` only when `transform_update_mode == 0`.
Find who zeroes `transform_update_mode` (or the latches) each frame — that call
site is where the fixed-point fill belongs, because it is the point the float
path is about to be re-entered.

**Do NOT re-propose sourcing collision's joints from the renderer's 20.12
pipeline.** R2-04 E57 already refuted it: hitboxes walk the *live* joint chain
(`gmcollision.c:489`), and the renderer runs at presentation rate, not the 60 Hz
gameplay rate.

**L7a REFUTED (the `-marm` one), do not retry.** Building
`battleship_gmcollision.o -marm` — the move worth −511,174 ticks/tic for
`nds_renderer.o` — measured WORK-H P95 1,147,200 → 1,144,896, i.e. **−2,304**,
inside the ±5,376 cross-build floor, and `SRC` moved the wrong way by the same
amount. The renderer's win was SMULL, which ARMv5TE Thumb lacks; `gmcollision.c`
has no doubles and no unsuffixed literals, so its float work is f32 helper calls
whose libgcc mode the caller's flag does not change.

### R2-07 L7a — kernel written, falsifier RED, nothing wired in (2026-07-31)

**`include/nds/nds_r2_collision_mtx.h`** is the 20.12 replacement for
`func_ovl2_800ED490` and `gmCollisionSetInvertMatrix`, and
**`scripts/check-r2-collision-mtx.c`** is its host falsifier — the kernel lives
in a header for the same reason `nds_r2_sqrtf.h` does, so the checker exercises
the shipping code rather than a transcription of it. **Nothing calls the kernel.
This was deliberate: collision decides hits, so the arithmetic gets proven
before the call path is touched, and the seam problem (L7 SCOPED) makes wiring
the harder half anyway.**

**The falsifier earned its keep immediately, twice:**

| revision | invert max, world units |
|---|---:|
| first draft | **124.58** |
| reciprocal scaled at 2^36 not 2^24 | 0.4514 |
| 24-bit cofactors in the 3×3 only | 3.6967 |
| 24-bit cofactors in **both halves** | **0.3706** |

The first was a 2^12 scaling error: `det` is itself 20.12, so `1/det` at 24
fractional bits needs a 2^36 numerator, and the 2^24 form made the final shift
discard all twelve fractional bits — every inverse came back rounded to whole
world units. **None of that reached a ROM, a build or a measurement.**

The third row is the useful surprise: raising precision in the 3×3 block alone
made the result **eight times worse**. The rotation and the translation stop
being a matched pair, and their errors no longer cancel in the transformed
point. **Both halves must share intermediate precision** — a rule that is
invisible until something measures the composed result rather than the cells.

**Current state, and it is RED.** Splitting by scale domain the way E65 splits
by rate — because a bound without its domain is not a bound — gives:

| scale domain | compose max | invert max | gate |
|---|---:|---:|---|
| near-unit 0.90–1.10 | 0.017817 | **0.126987** | **RED** |
| moderate 0.50–1.50 | 0.020182 | 0.133385 | (reported) |
| conservative 0.25–2.00 | 0.020594 | 0.400510 | (reported) |

**This refutes my own framing of the amplifier.** I wrote that 1/det was the
steepness term, by analogy with E65's L·|rate|. It is not the main one: at
near-unit scale, where 1/det ≈ 1, the inverse is still **0.127 — six times the
bound**. Scale only takes it from 0.127 to 0.40.

**The real amplifier is the world translation.** The inverse carries a point of
magnitude ~400 (a fighter's world position on Dream Land) through a matrix whose
cells are quantised to 1/4096, so relative error ~1.2e-4 × 400 ≈ 0.05 per term.
Compose is nearly domain-independent (0.0178 → 0.0206) for the same reason: its
error is input quantisation, not arithmetic. **20.12 is simply not enough
precision for collision at this world scale, and no amount of care inside the
kernel changes that.**

**Which names the fix, and it is a restructure, not more bits.** Compute
`local = R⁻¹·(p − t)` instead of `R⁻¹·p + t'`. Algebraically identical, but the
subtraction happens at full input precision and the rotated quantity is then
±20 rather than ±400 — the error should fall by the ratio of those magnitudes.
That is also how the source's own hit test is shaped, since `gmCollisionTestRectangle`
subtracts `offset` right after transforming. **Do that before adding fractional
bits.**

**Do not add this checker to a verifier profile while it is red**, and do not
gate a wider scale domain until `scripts/census-fighter-gameplay-joints.ps1`
says which one SSB64 actually visits.

### R2-07 L11 NULL — no libm trig left on the battle path after L9 (2026-07-31)

Swept for more of L9's shape (a port stub standing in for cheaper source code)
before spending a build. **Nothing left worth taking**, and the sweep is recorded
so it is not repeated:

- **No other port function is a bare libm wrapper.** An AST-shaped scan of
  `src/port` and `src/nds` for `f32 X(...) { return <libm>(...); }` returns
  `lbCommonSin`/`Cos` only — the two L9 already fixed.
- **16 direct `sinf`/`cosf` call sites remain**, but 11 are in
  `opening_movie_backend.c` (not the battle scene) and the rest are one-shot
  (`atan2f` on stick range, a `cosf` in an angle compare). None is per-joint.
- **`__sinf`/`__cosf` (`n64_stubs.c:13`) are libm-backed and produce ZERO rows in
  the post-L9 census** — their callers are `efmanager`/`lbparticle`, which do not
  run in this scene. **Do not "fix" them**: on N64 `__sinf` is libultra's
  polynomial, a *different* function from `lbCommonSin`'s 4096-step table, and
  `syMatrix*` calls the former while gameplay calls the latter. Substituting one
  for the other would change results with no source mandate — and it would buy
  nothing here anyway.
- **The trig kernels are gone.** `__kernel_sinf`, `__kernel_cosf` and
  `__ieee754_rem_pio2f` no longer appear in the census top table at all, which
  is L9's engagement confirmed a second way, from the symbol side.

**So the trig seam is closed and the remaining float is the collision matrix
body — L7, and it needs the entry-point rewrite, not another stub swap.**

### R2-07 L10 KEEP — the hardware square root was BUILT AND OFF; 3-VBlank frames 97 → 89 (2026-07-31)

**`NDS_R2_FIXED_SQRT` has been `?= 0` since R2-03 E1 built it.** E1 measured
`sqrtf` 15,760 → 9,720 ticks/frame (**−6,040**), bit-exact, Boundary green — and
then left the flag off, because its 8-frame A/B read "flat on every bucket" and
the saving sat inside the placement floor. Nothing refuted it; it just never
graduated. `git log -S` finds one commit and no reversal.

**L6 changes the reading.** `__ieee754_sqrtf` runs **87 times on an over-gate
frame against 26 on a clean one (3.34×)**, for a **26,007-cycle delta** — 5.1% of
the over-gate premium. So the saving concentrates on exactly the frames that miss,
which makes it a P95 lever, not the mean-only lever E1's flat A/B implied. **A
lever measured as flat on a mean can still be a gate lever; ask where its work
sits before shelving it.**

`scripts/check-r2-fixed-sqrt.ps1` re-run before flipping: **12,807,569 inputs,
8,775,610 handled by the hardware path, 4,031,959 declined to newlib, 0
mismatches.** Bit-exact, so no equivalence bound is owed and the state hash must
not move — Boundary green confirms it did not.

**A/B against L9 as the matched control**, same session, harness and window
(`artifacts/performance/r207-L10-sqrt-128*`):

| | L9 (control) | L10 | delta |
|---|---:|---:|---:|
| **`WORK-H` P95** | 1,244,608 | **1,232,448** | **−12,160** |
| `WORK-H` P50 | 914,880 | 921,920 | +7,040 |
| `WORK-H` max | 1,512,320 | **1,375,104** | **−137,216** |
| `SRC` P95 | 531,648 | 522,368 | −9,280 |
| **3-VBlank frames** | 97/567 | **89/567** | **−8** |
| 2-VBlank frames | 456 | 465 | +9 |

**The histogram is the headline, per AGENTS.md** — eight frames moved from three
VBlanks to two, which is perceived pacing, and the worst frame fell 137,216.
P95 −12,160 is 2.3× the ±5,376 floor. **P50 +7,040 is against a bit-exact
change, so it is placement, not work** — the honest reading of a mean that moves
the wrong way when the arithmetic is provably identical.

**Do not try to make it faster; it is at its safe floor.** E1 noted the residual
is libnds's `sqrt64` polling (`write / poll-busy / write / poll-busy / read`),
which invites two "obvious" follow-ups, and both are closed:

- **Use the 32-bit unit instead.** Cannot: `nds_r2_sqrtf.h:69` scales to
  `mantissa << (odd + 23)` with a 24-bit mantissa, so `scaled` is always in
  [2^46, 2^48). The 64-bit unit is the only one that can serve it.
- **Replace the poll with a fixed cycle wait.** This is a *hardware-timing*
  assumption, and the campaign has no retail-hardware loop to validate it — a
  delay that is long enough under the accurate melonDS and short on a real DS
  reads back an incomplete root and silently corrupts a gameplay value.
  AGENTS.md reserves hardware tests for exactly this, so it stays unbuilt until
  there is a device to prove it on.

### R2-07 L9 KEEP — SSB64's own sine table, −37,248 `WORK-H` P95, matched control (2026-07-31)

**The port had `f32 lbCommonSin(f32 a) { return sinf(a); }`.** SSB64 does not
call a libm sine: `lb/lbcommon.c` (0x800C7840) indexes a 1024-entry quarter-turn
table, and gameplay reads those exact quantised numbers. **The stub was both
slower and the approximation** — the table is the reference behaviour, so this is
a fidelity fix that happens to be faster, not a trade. The port never compiled
`lbcommon.c`, so the table had nothing to link; `src/port/lbcommon_sin_table.c`
carries it, extracted verbatim (1024 entries, each within 2e-6 of
sin(n·2π/4096), first 0.0, last 1.0).

**Direct proof it engaged**, L6 census window, same instrument, per over-gate
frame: **trig 46,772 → 12,600 cycles (−34,172) with sin/cos calls 197 → 196.**
The call count holding while cost falls 3.7× is the proof — this is per-call
price. `__ieee754_rem_pio2f` argument reduction and `__kernel_sinf`/`_cosf` are
gone; a 4096-step table needs none of them.

**Matched control, same session, same harness, same 128-frame window**
(`artifacts/performance/r207-L9-{control,sintable}-128*`):

| bucket | control | L9 | delta |
|---|---:|---:|---:|
| **`WORK-H` P95** | **1,281,856** | **1,244,608** | **−37,248** |
| `WORK-H` P50 | 933,056 | 914,880 | −18,176 |
| **`SRC` P95** | 565,312 | 531,648 | **−33,664** |
| `SRC` P50 | 287,872 | 279,360 | −8,512 |
| `FTR` P50 | 388,736 | 381,376 | −7,360 |
| 3-VBlank frames | 99/567 | 97/567 | −2 |

**−37,248 is 6.9× the ±5,376 cross-build floor, and 90% of it is `SRC`** — the
bucket L6 named. First cut landed against the L6 population; it does not close
the gate (1,244,608 vs 1,120,000, still 124,608 over) but it is real and banked.
**Boundary GREEN** — which matters more than usual here, because trig values
change and therefore trajectories do. The gate that could have caught a fidelity
problem is the one that passed.

**The verdict was nearly published backwards.** Against the standing
1,160,448–1,179,520 baseline, L9's 1,244,608 reads as a large regression — and
the honest reading is that **the standing baseline is stale**: the matched
control at HEAD is 1,281,856, so the branch drifted up ~100K since it was taken.
Without the control this session, L9 would have been reverted as a regression
when it is a 37,248 improvement. **Re-baseline before citing that range again.**

### R2-07 L7 SCOPED — collision recomputes in float what the renderer already has in 20.12 (2026-07-31)

Source reading only, no build spent. Four findings, in the order they change the
plan:

**1. The redundant-inversion hypothesis is REFUTED.** 34 inversions per frame
looked like missing memoization. It is not: `FTParts` already carries three
lazy latches — `transform_update_mode` (local valid), `unk_dobjtrans_0x5` (world
valid), `unk_dobjtrans_0x7` (inverse valid) — and `func_ovl2_800EDE00`
(`gmcollision.c:461-473`) skips the inverse whenever `0x7` is set. The latches
are cleared once per frame, so **34 is 34 distinct joints genuinely needing an
inverse**, not one joint inverted 34 times. There is no hoist to take, and the
algorithm is already the cheap general one — a 3×3 cofactor inverse, not a
solve. **The lever is the arithmetic type, exactly as L6 said, and nothing
cheaper sits in front of it.**

**2. A PARTIAL conversion is penalised at the boundary.** The obvious small
first cut — convert only `func_ovl2_800ED490` and `gmCollisionSetInvertMatrix`,
the two hottest — leaves them reading and writing `Mtx44f`. That is 12 elements
in and 12 out per call across 74 calls = ~1,776 conversions/frame at ~20-30
cycles, **~44,000/frame of pure boundary tax against a modelled ~163,000 win**.
Sizing the two functions from L6's own per-call prices (`fadd` 41.0,
`__mulsf3` 28.1): `800ED490` is 36 mul + 27 add × 40 calls ≈ 84,744/frame,
`SetInvertMatrix` ≈ 63,070/frame, plus 37,634 of measured self time.
**So L7 is a subsystem conversion or it is nothing** — the unit is the joint
transform representation, not a function.

**3. That unit is SMALL.** `mtx_translate` has **44 references in the entire
decomp** (gmcollision 14, lbcommon 10, ftmain 6, the rest 1-2 each);
`unk_dobjtrans_0x10`/`0x9C` add 30, also concentrated in gmcollision. Port side
is ~66, in four files. **~110 sites total** — a bounded change, not a rewrite.

**4. A fixed-point COMPOSE already exists in the renderer — but not a fixed-point
leaf build, and it is only 4× not 10×.** `ndsRendererAdapterBuildDObjLocalMatrix`
(`reloc_backend_renderer_dl.c:1516`) produces `NDSRendererMatrix20p12` and
**never reads `parts->mtx_translate`**, so the port does derive joint transforms
in 20.12. But its *leaf* is `syMatrixTraRotRpyRSca` — **float**, measured at
**1,408.5 self cycles/call** — and only the composition
(`ndsRendererMtxMulAffine20p12`) is fixed point. **Reusable machinery is the
compose, not the whole build.** Measured per-call, same run, same frames:

| | self cyc/call | calls/frame | self cyc/frame |
|---|---:|---:|---:|
| `func_ovl2_800ED490` (float 4×3 compose) | 533.7 | 40.0 | 21,348 |
| `gmCollisionSetInvertMatrix` (float 3×3 inverse) | 479.0 | 34.0 | 16,286 |
| `ndsRendererMtxMulAffine20p12` (**20.12** compose) | 670.8 | 55.0 | 36,894 |
| `syMatrixTraRotRpyRSca` (float leaf) | 1,408.5 | 5.0 | 7,042 |

Self time understates the float rows: they push their arithmetic into
`__aeabi_fadd`/`__mulsf3`, the fixed-point row does not. At the run's own
marginal price (337,927 delta cycles / ~7,557 delta calls = **44.7 cycles per
float call**), `800ED490`'s 63 float ops × 40 calls cost **112,644/frame** on
top of its self time, and `SetInvertMatrix`'s 58 × 34 cost **88,148**. So float
compose ≈ **2,652 cycles/call** against the existing fixed-point compose at
**670.8 — 4.0×, not the ~10× a naive cycles-per-op model gives.**

**Sized honestly, L7 lands ON the line, not past it.** Compose + inverse
together are **238,426/frame (46.7% of the premium)**; converting both saves
~**187,794**. The overshoot is **295,376**, so that is **64% — NOT ENOUGH.** The
other ~3,065 extra float calls (`TransformMatrixAll`, `TestRectangle`,
`GetWorldPosition`, `SetMatrixNcs` with its `lbCommonSin`/`Cos`) are worth
~137,000 more, and only converting **all** of it reaches ~290,000 against a
295,376 overshoot. **Do not scope L7 as "the two hot functions" and expect the
gate** — that was my first plan and the measurement kills it.

**5. L8 (unroll the 20.12 compose) is REFUTED UNBUILT, by its own arithmetic.**
`ndsRendererMtxMulAffine20p12` is ARM (0x02002e44, even), 616 bytes, 155
instructions, 7 `smull`, **zero `__aeabi_lmul`** — not the Thumb trap. It
executes **379 instructions/call at 2.2 cycles/insn** and the renderer pays
**36,894/frame** for it, so "unroll it" looked collectable. It is not:

- The inner products are **already unrolled** (`nds_renderer.c:5221-5223`, three
  explicit terms). Only the 3×3 row/col loops remain, so unrolling buys loop
  overhead and nothing else.
- Per cell the work is 3 `SMLAL` + `ndsRendererRoundShiftS64` +
  `ndsRendererClampS64ToS32` + a store ≈ 20 instructions; 12 cells + loads is
  ~280 against the measured 379. **The whole prize is ~99 instructions × 2.2 =
  ~218 cycles/call, 55 calls = ~12,000/frame** — under E11's own ~16,000 bar,
  before charging the icache cost of the added bytes, which is exactly how E11
  went +15,744 while removing real work.

**The collectable cost in that function is the per-cell s64 round-and-clamp, not
the loop.** `ndsRendererRoundShiftS64` (`:4983`) is round-half-away-from-zero
via negate/add/shift/negate, and `ndsRendererClampS64ToS32` (`:4951`) adds two
64-bit compares — together ~14 of the ~20 instructions per cell. `SMULL`'s own
64-bit result could be shifted down in two instructions
(`mov lo,lsr#12` + `orr hi,lsl#20`) instead, but that changes the rounding and
the saturation, so it is **not bit-exact and needs an equivalence bound**.
**Fold it into L7**, which is establishing one anyway, rather than spending a
separate build on the sub-threshold half.

**What is NOT yet established, and must be L7's first step rather than its
assumption:** that the two are the same quantity. The renderer composes toward
a camera-relative MVP and collision needs world; the renderer runs at the 30 Hz
present while collision runs at the 60 Hz sim (`SRC` accumulates twice per
presented frame); the populations differ (55 renderer calls vs 40 compositions +
34 inversions). Verify the local-matrix equality on real joints before any
sharing is designed — a shared cache built on an unverified equality is the
Task 36 replay mistake again.

**Equivalence bound is mandatory** (E64b/E65 precedent). Collision decides hits;
a boundary flip is amplified by damage, knockback, and hitstun. 20.12 gives
1/4096 resolution against world coordinates in the hundreds, so the bound should
be stated in world units and hit/no-hit outcome counts, not in ULPs.

### R2-07 L6 ANSWERED — the excursion is FLOAT COLLISION, and it is a lever (2026-07-31)

**The over-gate frame is a hit-detection frame, and 66.2% of what makes it
expensive is soft-float.** Evidence `artifacts/task37-census/r207-L6-over/`
(window 517..521, 4 over-gate + 1 clean) and `.../r207-L6-clean/` (508..512, all
clean). Both windows agree; the headline comes from the **in-run** split, so it
carries none of the ±5,376 cross-build floor.

| per presented frame, ARM9 cycles | over-gate ×4 | control ×1 | delta |
|---|---:|---:|---:|
| total (VBlank-quantized) | 3,360,589 | 2,240,196 | +1,120,393 |
| **work** (total − `armWaitForIrq`) | **2,536,136** | **2,025,745** | **+510,390** |
| idle | 825,130 | 214,855 | +610,274 |
| **soft-float** | **520,017** | **182,090** | **+337,927** |

The idle row is pacing arithmetic, not a cause: a frame occupies 3 VBlanks
instead of 2, so its slack grows by `1 VBlank − Δwork`. **The gate is 2,240,760
cycles, so an over-gate frame overshoots by only 295,376 (147,688 ticks) — and
the soft-float delta alone is 337,927, which is LARGER than the overshoot.**

**Composition of the +510,390 work premium** (`census-over-gate-split.txt`,
`--split-over-gate`): `__aeabi_fadd` 27.4%, `__mulsf3` 22.5%, `__ieee754_sqrtf`
5.1%, `__divsf3` 2.2%, trig 5.9% → **soft-float 66.2%**. Named callers with
extra self time are all collision: `func_ovl2_800ED490` 4.2%,
`gmCollisionSetInvertMatrix` 3.2%, `gmCollisionTransformMatrixAll` 2.3%,
`gmCollisionTestRectangle` 2.1%, `gmCollisionGetWorldPosition` 1.4%,
`func_ovl2_800EDBA4` 1.7% — `gm/gmcollision.c`, the hitbox/hurtbox tests and the
lazy joint-world-matrix walk.

**Entry-PC call counts make it binary, 10 frames, two independent runs:**

| per frame | 518 | 519 | 520 | 521 | 522 | 509 | 510 | 511 | 512 | 513 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `gmCollisionSetInvertMatrix` | 34 | 34 | 34 | 34 | **0** | 0 | 0 | 0 | 0 | 0 |
| `func_ovl2_800ED490` | 40 | 40 | 40 | 40 | **0** | 0 | 0 | 0 | 0 | 0 |
| `__aeabi_fadd` | 5,329 | 5,562 | 5,321 | 5,245 | 1,953 | 1,681 | 1,719 | 1,513 | 1,483 | 1,491 |
| `__aeabi_fmul` | 5,608 | 5,793 | 5,624 | 5,551 | 1,559 | 1,550 | 1,553 | 1,415 | 1,398 | 1,399 |

Zero overlap, and the counts are *constant* at 34/40 — a fixed-size workload
switched on by state, not a variable search. Float calls rise 2.75×/3.62× in
lockstep: ~7,557 extra float ops per frame, ~222 per collision test, which is
the size of a 4×4 inverse plus transform.

**Two things this refutes.** `ndsRendererAdapterBuildDObjLocalMatrix` is
**1.06×** (55 vs 52 calls) — **render and animation are FLAT across the
excursion**, confirming L3/E7 from an independent instrument.
`ndsRelocFindLoadedFileContaining` is **0.5%** of the premium — **these frames
do no meaningful asset loading**, which is L2's "24 of 28 do no load" reached a
second way.

**L7 is therefore named: fixed-point collision math.** The float here is
decomp-sourced but not frozen — a port-side DS equivalent is explicitly wanted.
Removable cost is high: the deltas price `__aeabi_fadd` at **41.0 cycles/call**
and `__mulsf3` at **28.1**, against ~4–6 for an inline 20.12 `SMULL`. Needs an
E64b/E65-style equivalence bound, because collision decides hits and a boundary
case can flip a result that damage and knockback then amplify.

**The instrument defect that nearly buried this.** Arm B was first run with
`-NoBuild`, so it measured arm A's ROM and produced a **byte-identical 7.4 MB
CSV**; the "identical censuses" were one census labelled twice, not a windowing
failure — `-StartFrame` works, but only through a rebuild, since the window is a
compile-time constant. Re-running arm A reproduced `cycles=15685712` exactly,
which is what proved melonDS deterministic and the duplication explained.
`run-task37-profile-census.ps1` now reads `builds/$Build/nds_build_config.h` and
refuses to measure a ROM whose baked window differs from the one asked for. From
arm A alone the natural publication was "soft-float is 9.95%, the largest
non-idle consumer" — true of the whole run, and **wrong by 6.7× about the
excursion in the other direction**. The control arm is what turned a
whole-run percentage into a per-frame cause.

### R2-07 L3 — the excursion is SIZED: +272,576 `WORK-H`, all of it `SRC`/`OTHR`, render flat (2026-07-31)

Straight from `artifacts/performance/r207-L1-rows.csv` — the pacing-comparable
arm — split into its 28 over-gate and 100 clean frames. No new run.

| bucket | clean P50 | over-gate P50 | delta |
|---|---|---|---|
| `SRC` | 280,768 | 516,224 | **+235,456** |
| `OTHR` | 209,152 | 442,048 | **+232,896** |
| `MISC` | 46,784 | 81,408 | +34,624 |
| `FTR` | 390,016 | 388,672 | **−1,344** |
| `STG` | 169,856 | 170,752 | **+896** |
| **`WORK-H`** | **924,224** | **1,196,800** | **+272,576** |

**The render side is exonerated quantitatively, not by argument.** `FTR` is
*negative* on over-gate frames and `STG` moves under 1K — both inside noise. So
no amount of fighter or stage work closes this; E7 said so and this is the
per-frame confirmation at 28 frames rather than 9.

**`SRC` and `OTHR` are not additive — they overlap.** Their deltas are within
2,560 of each other (+235,456 vs +232,896), and their sum (468,352) far exceeds
the `WORK-H` delta (272,576), so `OTHR` is largely the same work `SRC` brackets
rather than a second population. **Do not add them when sizing a lever.**

**The target, stated as a number.** `ALL` is VBlank-quantised: over-gate frames
sit at 3 VBlanks (~1,679,xxx), clean at 2 (~1,119,xxx). The median over-gate
frame carries **1,196,800 `WORK-H` against the 1,120,380 gate — ~76,420 over**,
inside a total excursion of 272,576 above the clean median. `SRC` on those
frames spans **258,240 … 746,624** (P50 516,224), so they are not one uniform
event and a single fixed cost will not explain them.

**L4 is constrained before a single profiler run, from the same rows.**

*Shape:* the 28 over-gate frames come in **short adjacent runs** — 10 of the 27
inter-frame gaps are exactly 1 — while recurring across the whole 439..566
window (median gap 3, max 15). **Bursts, not a steady state.** A per-frame
constant is ruled out twice over: by L3's 258,240…746,624 spread and by this.

*Correlation with the other buckets, all 128 frames:*
```
SRC vs ALL   +0.723      SRC vs MISC  +0.402
SRC vs FTR   -0.385      SRC vs OTHR  +0.395
SRC vs STG   +0.238      SRC vs WAIT  +0.395
SRC vs BG/AUD/HUD ~0     (independent, as expected)
```
`ALL` at +0.723 re-confirms `SRC` drives the frame. **The lead is `FTR` at
−0.385: when `SRC` is high, fighter work is LOWER.** Work moving *out* of the
`FTR` bracket while the frame gets more expensive is the signature of the
fighter falling off its native owner onto a path counted elsewhere — and R2-03
**E31 recorded exactly that mechanism** (the hitlag/`AnimLock` shuffle disables
the native owner, "5/5 burst frames"), which also matches the burst shape here.

**Do not treat that as settled — R2-06 E7 REFUTED fighter fallback as the
cause**, on the older 9-over-gate baseline. So L4 was run as a direct per-frame
test, not an argument.

**L4 RESULT: fighter fallback is REFUTED again, at 28 frames this time.**
`-FallbackCensus` samples `gNdsTickHudNativeOwnerFallbackCount` per frame, and
it was being **collected and then dropped at CSV-write time** — the same shape
as `-ExtraGlobals`, now fixed (`fbTotal` column). With it wired
(`artifacts/performance/r207-L4-rows.csv`, `build-l2-census`):

```
fallback frames  0        total fallbacks over 128 frames  0
over-gate 28   ∩ fallback 0
```

**The native fighter owner never falls back on any frame in the window**, so the
`SRC`↔`FTR` −0.385 is not the E31 hitlag/`AnimLock` mechanism, and **E7's
refutation is corroborated rather than overturned.** The −0.385 has another
owner.

**LIVENESS CONFIRMED — the zero is real.** The harness already prints the
per-reason breakdown; it just does not reach the CSV:
```
native-owner fallback: 0 of 127 frames took one
[calls:1132 eligible:1132 animLock:0 selected:0 displayList:0 materialCount:0
 validate:0 matrices:0 materialPrep:0 inputs:0 contract:0 postGx:0 begin:0
 animLoad:43 animResident:2]
```
`calls:1132` with `eligible:1132` proves the census is live and every call was
eligible, so **fighter fallback is refuted outright, not by a dead counter.**
Every genuine fallback reason — `animLock` included, which was the E31
mechanism — is exactly 0.

**It also produced a lead, `animLoad:43` — and L5 KILLED IT. The "different
population" reading was mine and it was WRONG.** Per-frame
(`gNdsTickHudNativeOwnerFallbackByReason[13]`, `r207-L5-rows.csv`):
```
animLoad frames  12    total 15
over-gate        28
BOTH              5  ->  444,449,464,477,543
```
**Identical to L2's Task-75 intersection** — same 12 frames, same total of 15,
same five frames. `animLoad` and `gNdsTask75AssetLoadCount` count the **same
population**, not different ones. The `43` in the census summary is a
differently-bracketed window (it spans the seeding presents), and comparing it
against a per-frame delta of 15 is what produced the false lead. **Compare
like-bracketed numbers, or do not compare them.**

**Standing after L1–L5.** The excursion is **not** asset loading of any counted
kind — Task 75 and `animLoad` are the same 12 frames (L2, L5) — **not** the
render buckets (L3, `FTR` negative and `STG` under 1K), and **not** fighter
fallback (L4, refuted twice, now with `calls:1132`/`eligible:1132` liveness
proof). **23 of 28 over-gate frames carry no counted load, no fallback, and no
render excursion**, yet each is ~76,420 over the gate inside a 272,576
`WORK-H` excursion.

**Every named candidate is now exhausted, so L6 must widen rather than guess.**
The remaining facts to design against: the cost lives in `SRC`/`OTHR` (which
overlap, do not sum them); it arrives in **bursts** (10 of 27 gaps are 1); it
has a **wide spread** (`SRC` 258,240…746,624); and it correlates **negatively**
with `FTR` (−0.385) without being fallback. A per-frame counter census will not
find it — there is no counter left to read. **Profile the `SRC` bracket itself
on a named over-gate frame against a matched clean one** (R2-03 E35's method:
it profiled 517-521 against 508-512), which is what identifies work that no
existing counter brackets.

### R2-07 L2 — ANSWERED, and it CONTRADICTS E8: only 4 of 28 over-gate frames load (2026-07-31)

Instrument built (`-PerFrameGlobals`, below), then run.
`artifacts/performance/r207-L2-rows.csv` now carries
`gNdsTask75AssetLoadCount` as a per-frame column.

```
L2 load frames (13)   444,449,464,468,477,494,498,504,531,535,543,565
L1 over-gate (28)     446,449,452,453,456,459,464,469,473,475,476,477,478,486,
                      495,500,515,517,518,519,520,521,528,536,542,543,544,556
intersection           4 of 28  ->  449,464,477,543
```

**R2-06 E8's claim — "every over-gate frame is an asset-load frame", carried
into `HANDOFF.md` and the SwitchPlan status as "the entire over-gate
population" — does not survive a per-frame read. 24 of 28 over-gate frames do
no asset load at all.** Total loads across the 128-frame window: 15.

**The comparability assumption is NOT load-bearing — corroborated within one
build.** The intersection above pairs two builds (L1 normal, L2 census), which
is only valid if a `frame` id names the same game moment in both. That
assumption can be removed entirely: the census arm's own rows carry the load
column *and* its own `ALL`, so intersect them with each other.

| reading | over-gate frames that load | assumption |
|---|---|---|
| cross-build (L2 loads × L1 over-gate) | 4 of 28 | frame ids comparable |
| **within-build (census arm alone)** | **5 of 28** | **none** |

Same answer either way, and the assumption-free reading is the stronger of the
two: **loads own 18% of the over-gate population, not all of it.** (The census
arm's over-gate set is inflated by its own instrumentation, so its 28 is not
the shipped count — but the *ratio* is computed inside one arm and is unaffected
by that.) E8's own 9-over-gate baseline is also a different, older build than
L1's 28, which is a further reason its "every over-gate frame" never
generalised.

**What it changes.** "Buy headroom by eliminating in-match loads" was sized
against the belief that loads own the whole over-gate population. If loads own
4 of 28, eliminating them cannot close the gate on its own, and the remaining
24 need their own attribution — which is where L1 already pointed: **every
over-gate frame is an `SRC` frame.** Load elimination stays worth doing for
§3.8 (it is a correctness clause, not only a performance one), but it must not
be budgeted as the gate's answer.

### R2-07 L2 instrument — `-PerFrameGlobals`, and three harnesses fixed on the way (2026-07-31)

**`-ExtraGlobals` was a RUN TOTAL, not a per-frame column — this is what
blocked L2, and it is now fixed.** `-PerFrameGlobals` samples the named globals
in the **per-frame** printf beside the buckets and writes them as trailing
`-RowsCsv` columns named after the symbol. Use it for anything per-frame;
`-ExtraGlobals` remains correct only for run totals. The original defect:
`sample-tick-hud-buckets.ps1:570-581` matches **one** `TICKEXTRA=` line from the
whole gdb output at the end of the run; the per-sample printf
(`$sampleFields`, :216) carries only `gNdsTickHudBuckets[]` plus the fallback
fields. So `-ExtraGlobals gNdsTask75AssetLoadCount` answers "how many asset
loads did this run do", never "which frames did them", and `-RowsCsv` writes
`frame,<buckets>,WORK-H` with no extra column (verified: the L2 CSV header has
none). **The fix is to sample the named globals in the per-frame printf** — a
`-PerFrameGlobals` that appends to `$sampleFields` and to the CSV header —
rather than reading them once at the end. Until that exists, L2's intersection
is not obtainable and no amount of re-running helps.

**Three harnesses carried the same dropped-build-flag defect and are now fixed.**
Each rebuilds the ROM on every run and each omitted flags its own instructions
require, so a run silently measured a build without the thing it asked for:
`capture-sudden-death-entry.ps1` and `soak-freeze-watch.ps1` (both now take
`-SecondEntryDiag`, put it on the make line, and verify it in the generated
config header) and `sample-tick-hud-buckets.ps1` (now takes a generic
`-MakeFlags` pass-through, so the next flag needs no edit). The last of these
documented the census requirement **in its own header, twice**, and still never
passed it.

**`-AllowRepeatedFrames` added, and deliberately narrow.** The census ROM is
slow enough that both iterations of the 60 Hz loop land in separate samples, so
the duplicate-frame guard fired on 22 of 128 rows. The guard already
distinguishes the two causes by payload equality: identical = a stale read (an
instrument defect), differing = a real second iteration. The switch downgrades
**only** the second case, still fails hard on a stale read, and warns that the
rows are set-only and never pacing-comparable.

**Do not quote this build's pacing.** It shows 28 of 128 frames at `ALL`
≈1,680,000 — exactly 3 VBlanks — against 2 VBlanks on clean frames. That is the
census instrumentation's own cost, which is precisely why the L2 instruction
says to read this build for the load-frame SET only.

### R2-07 L1 — every over-gate frame is an `SRC` frame, 19 of 128, and the harness is frame-reproducible (2026-07-30)

Per-frame rows from the same configuration as L0
(`artifacts/performance/r207-L1-rows.csv`, `-RowsCsv`), DLDI-on, frames
439..566.

**19 of 128 frames exceed the 1,120,000 gate on `WORK-H`, and all 19 carry a
raised `SRC`.** `SRC` median is 284,864; the *smallest* over-gate frame still
reads 489,536, or **1.72x the median**, and the largest reads 746,624 at frame
517. There is no over-gate frame with an ordinary `SRC`. That reproduces E8's
finding on the current build and sharpens it into a usable predicate: **in this
window, `SRC` above roughly 487,000 is necessary for a frame to miss the gate.**

They arrive in bursts -- 449/452/453, 464, 469, 475-478, 495, 517-521, 536,
542-544 -- which is the load-event shape, not a per-frame cost. Frame 517 is the
same frame E35 had to profile by hand.

**`OTHR` moves INVERSELY and is not a second owner.** The worst frames have the
*lowest* `OTHR` (234,880 at frame 517) and the marginal ones the highest
(570,432 at 469), because `OTHR` is the residual `ALL` minus the named buckets.
Do not read it as an independent excursion.

**`HUD` spikes on different frames than `SRC` does** (74,880/345,280 on 542/544
while 517 and 475 sit at 960), which is the second, independent confirmation
that the `HUD` bucket is the instrument and unrelated to the gate.

**The harness is frame-reproducible within a build.** L0 and L1 were separate
emulation runs of the same ROM and returned identical `named=998,520` and an
identical VBlank histogram. So a per-frame comparison against a matched control
is legitimate *within* a build -- the E11 +/-5,376 floor is a CROSS-build
property, not run-to-run scatter.

**Next (L2): name what `SRC` is doing on those 19 frames.** The `SRC` bracket is
`gNdsTickHudSourceTicks` -- the source-side update -- so this is the §3.8
question stated precisely: which first-use load, discovery, relocation or
rebuild fires on frames 449/475/517/542 and not on the other 109. Take it with
the `NDS_TASK75_LOAD_CENSUS=1` + `NDS_TASK68_FALLBACK_CENSUS=1` build so
`gNdsTask75AssetLoadCount` exists, read that build for the load-frame SET only
(it is not pacing-comparable), and intersect it with this frame list.

### R2-07 L0 — the anim cache is CLEAN, so it is not the in-match load (2026-07-30)

> **CORRECTION, same session.** This entry originally also claimed `HUD`'s
> 338x spread was an unpriced excursion and made it the L1 target. **That was
> wrong and is withdrawn.** `HUD` = `gNdsTickHudForegroundTicks +
> gNdsRendererProfileHudTicks`, and the second term brackets
> `ndsPlatformRenderDebugHud()` (`taskman_seam.c:4813`) — **the tick HUD's own
> console render, i.e. the instrument.** It is already known and already
> handled: `sample-tick-hud-buckets.ps1:446-454` documents the >300x spread in
> its own comment and subtracts it **per sample** to form `WORK-H`, "since the
> published profile-0 ROM carries no tick HUD at all". That is exactly why this
> campaign headlines `WORK-H` and not `WORK`. Chasing it would have optimised
> the meter. **Third recurrence of R2-03 E43's "the bracket priced its own
> instrument".** Before naming any bucket as a target, check what its writers
> bracket.

Opening the load-elimination lane the switch plan names as R2-07's option 1 ("buy
headroom first"), which is also red-queue item 1 and the unmet half of R2-04's
clause: *all animation streams for the match prepared at load; no first-use
loading during gameplay* (§3.8). One census run, DLDI-on, 128 samples via
`sample-tick-hud-buckets.ps1 -RingDump`, frames 439..566, at `1b34621667`.
Evidence `artifacts/performance/r207-L0-cache-census.json`.

**The R2 animation cache is healthy and is NOT the source.**
`gNdsR2AnimCacheRejects=0`, `gNdsR2AnimCacheArenaOverflows=0`, 42 hits against 2
misses and 2 fills, arena 87,824 used of 92,160 reserved. **This retires the
standing unowned observation `ArenaOverflows 109 / Rejects 109`** — that reading
predated the arena's move onto the taskman heap and has been carried forward as
if it were current. The cache is serving the match. Whatever still loads
mid-match is a different asset class, so do not re-open the cache as the lever.

**R2-04's animation clause is MET, and the source says so structurally.**
`sNdsR204AnimWarmList` (`reloc_backend_assets.c:5851`) holds **exactly 41
entries** — the same 41-asset / 91,104-byte working set the board measured — and
`ndsR2AnimCachePreloadStep` walks it one asset per scene update, finishing
inside the countdown, long before the window sampled here. The run confirms it
from the other side: **2 fills across 128 frames**, so only two assets outside
the warm list were touched mid-match. Two fills cannot explain nineteen
over-gate frames. **The in-match loads are therefore NOT animation streams**,
which is what makes §3.8's remaining question a different one: what else is
first-used mid-match?

**Where the excursion is, with `HUD` correctly discarded as the instrument.**
The two buckets the campaign has spent itself on are flat and at their
established values -- `FTR` P50 389,888 / P95 392,896, spread **1.01**; `STG`
169,920 / 177,216, spread **1.04** against R2-02's 177,088. The real spread is
`SRC` 284,864 / 564,032 (**1.98**), `OTHR` 226,112 / 470,464 (2.08), `MISC`
46,912 / 159,360 (3.40), and `AUD` mean 7,762 against a max of 127,296. That
agrees with the board's standing account (E35: `SRC` owns the gate; E8: the
premium is entirely `SRC`) rather than adding a new owner to it -- so L0's
contribution to this lane is a **subtraction**: the anim cache is out, and
`FTR`/`STG` are confirmed not to be carrying the excursion.

**Next (L1): the load-frame population itself, in `SRC`.** That needs the
`NDS_TASK75_LOAD_CENSUS=1` + `NDS_TASK68_FALLBACK_CENSUS=1` build so
`gNdsTask75AssetLoadCount` exists and the per-frame ring points at it; the
ordinary tick-HUD build does not define that symbol, and `-ExtraGlobals`
correctly refused it here. Read that build for the load-frame set only, never
for pacing -- the script warns it is not pacing-comparable to a baseline. Then
ask the §3.8 question: what is still first-used mid-match now that animation
streams are cached clean? Note the E11 rule for any delta from here: the
cross-build floor is P95 +/-5,376, small load-frame cuts cannot be banked, and
only work that LEAVES the frame counts.

**Do not read this run's `WORK-H` P95 (1,273,024) as a regression or a new
baseline.** It is a different 128-frame window than the board's earlier readings
and no matched control was run beside it. What it does license is the
composition above, because `FTR` and `STG` land on their known values in the
same run -- and those are the only two buckets the day's renderer change
(`reloc_backend_renderer_dl.c:7850`, the Sudden Death material-walk bound) can
touch, so that change is not implicated in the spread.

### R2-07 R1 ANSWERED — the "30 s GAME SET dead air" is 6.10 s, it is NOT a load, and R1 collapses into R2 (2026-07-30)

R1 was queued on the strength of a real-time capture that put the frozen last battle frame at
t≈105 s and "FOX WINS" at t≈135 s. **That 30 s is stale by construction** — it was taken before
R0c, when Results ran at 39.975 VB/iter, and R0c/R0d/R0e/R2a have since made the scene 3.9× faster
without anyone re-measuring the hand-off. Measured now, end to end:

| span | control | R2b affine | delta |
|---|---|---|---|
| battle taskman exit → 1st Results tick | 0.735 s | 0.735 s | — |
| → wallpaper (source tic 80) | 2.641 s | 2.641 s | **0.000** |
| → result panels (source tic 120) | 5.365 s | **4.112 s** | −1.253 s |
| **GAME SET → "FOX WINS"** | **6.10 s** | **4.85 s** | **−20.5%** |

**Both of R1's premises are refuted.** The original framing blamed the loader; the board's own
correction blamed "the taskman arena lifecycle, not the loader". Neither is the lever:

- The whole transition is **0.735 s = 12% of the dead air**. Of it, `mnVSResultsFuncStart` is
  21,851,904 ticks (0.652 s, 88.7%) and the two `ftManagerSetupFilesAllKind` calls are 11,193,728
  (0.334 s). **The fighter reload the owner suspected is 5.5% of the dead air** — a residency system
  for it buys a third of a second, so do not build one for this reason.
- The dead air is the scene's **own scheduled reveal, paid at the scene's per-frame cost**. The
  source holds the wallpaper until Results tic 80 and the panels until tic 120
  (`mnvsresults.c:2843-2844`), so the last battle frame stays on screen for eighty Results frames
  whatever the loader does.

**The instrument validates itself, which is why this is trusted.** `ToWallpaperTicks` came back
*identical* on the two arms — 88,507,072 vs 88,507,008, 64 ticks apart over 2.6 s. That is the
prediction, not a coincidence: nothing draws the wallpaper before tic 80, so R2b structurally cannot
move that phase, and two different binaries agreeing to 1 part in 38,000 says the bracket is sound
and the guest is deterministic. Every bit of R2b's win lands after tic 80, where it cuts **46.0%**
(2,282,133 → 1,231,744 ticks per Results tic, 4.08 → 2.20 VBlanks).

**Closing R1 into R2.** The residue is near the floor a mechanically-equivalent port must pay: the
source itself spends ~2 s of 60 Hz time reaching tic 120, which a 30 Hz presentation doubles to ~4 s
against the 4.85 s measured. Nothing here is worth a build on its own, and the one number that still
moves it — per-frame Results cost — is exactly what R2 owns. Second independent reason to graduate
R2b. Instrument is permanent: six counters in `battleship_mnvsresults.c`, read by
`soak-freeze-watch.ps1`, four timer reads per Results entry.

### R2-07 R4c GRADUATED — Results was on the WRONG RENDERER. Native owner on Results: −1,120,380/tic (−65.9%), 1.52x over the gate → 0.52x INSIDE it (2026-07-30)

**Owner approved the fighter look on sight, 2026-07-30: *"use the native renderer, it's already been approved"*.**
The entry below is kept in the order it was learned, because the verdict inverted twice and the
reasoning is the point: it was first read as a fidelity trade, then as a bookkeeping win, and it was
actually a renderer-selection bug. Final numbers: **3.04 → 1.04 VBlanks per source tic, 1,701,577 →
581,197 ticks**, fighter draw 1,449,776 → 364,784, battle unregressed on a clean matched-window
128-sample A/B (`ALL` p95 1,680,064 → 1,120,384; `FTR` p95 390,208 → 391,040, noise).

#### How it was originally written up (superseded above, retained for the reasoning)

`NDS_R2_FIGHTER_NO_ORACLE=1` gives the fighter draw the same
`ndsRendererHardwareSetNoOracle(TRUE)`/`(FALSE)` bracket the stage draw has always had. Measured on
the Results lab, matched source, only the flag differing:

| | oracle on | oracle off | Δ |
| --- | --- | --- | --- |
| VBlanks / source tic | 3.04 | **1.05** | −1.99 |
| ticks / source tic | 1,701,577 | **588,200** | **−1,113,377 (−65.4%)** |
| `FTR` / source tic | 1,449,776 | 363,899 | −1,085,877 (−74.9%) |
| vs the 1.12M gate | 1.52× over | **0.53× — inside** | |

That is the whole gate and more, in one bracket. **It is also a visual regression, so it does not
graduate.** Matched-tic pair at Results tic 160: **8,107 of 240,000 guest pixels differ (3.38%), max
channel delta 247, and the diff mask is *exactly the two fighters*** — background, floor, wallpaper
and the WINS text are untouched. Both fighters still render with all their geometry; what they lose
is **shading**. Mario's facial and overall shading flattens and Fox washes out — the characters go
flat-lit, and they are the focal point of the screen.

**The hypothesis this entry was built on is refuted.** R4c's re-baseline reasoned that the oracle
path was proof/counter bookkeeping, because the per-list reset it gates clears a prefix the source
calls "exclusively per-list proof/counter output". It is not bookkeeping: the oracle path also
carries the fighter **lighting/material derivation**, which is why
`ndsRendererAdapterBuildNativeMaterialSnapshot` was 18.8% of the memset callers and
`ndsRendererHardwarePrepareLitDirection` sat beside it. The proof-counter clearing rides along with
real rendering work; it cannot be removed by switching the path off.

**Why this is not simply taken anyway.** `PROJECT_GOAL.md`'s sacrifice order does rank visual
fidelity below stable 30 FPS, so a flat-lit fighter is contract-*permissible* if nothing cheaper
exists. But the rendering-side rule gates on "a reported fidelity budget plus the owner's visual
approval", and this is a large, character-level delta on the screen the owner has just added to the
P1 milestone. It also lands on the **shared** fighter draw, so it would flatten fighters in the match
as well, where R2-03/R2-06 spent the whole campaign keeping them lit. Flag stays `?= 0`.

The useful residue is a **priced ceiling**: everything the oracle path costs the fighter draw is
1,085,877/tic, and any cheaper way of keeping the lighting while dropping the proof work is bounded
above by that. The next question is whether the lighting/material derivation can be separated from
the proof/counter prefix inside the oracle path, rather than the path being taken or left whole.

> **R4f ASKED AND REFUTED WITHOUT A BUILD, same day — and it corrects the sizing above.** Two
> findings, both from reading rather than running.
>
> **(1) The prefix is not exclusively proof output, so it cannot simply not be cleared.**
> `ndsFighterDLDrawResetTransientRendererStats` (`:4748`) says "The prefix is exclusively per-list
> proof/counter output. The renderer state begins at `othermode_h`." It is not.
> `hardware_texture_format`, `hardware_texture_width` and `hardware_texture_height` all sit *before*
> `othermode_h` and are all **read for decisions**, not merely published: the texture memo compares
> `memo->format != stats->hardware_texture_format` (`nds_renderer.c:18545`) and two sites build masks
> with `1u << stats->hardware_texture_format` (`reloc_backend_renderer_dl.c:8597`, `:11825`).
> Skipping the `bzero` would leak the previous part list's texture format/size and corrupt the R2-03
> E12 texture memo's hit/miss. The comment is wrong and is the kind of wrong that costs a build; it
> is corrected in place.
>
> **(2) The proof clearing was never the money, so R4c's win was not what this entry assumed.** The
> cleared prefix is ~75 `u32` — about 300 bytes per part list. Priced through the census: `memset` is
> 8.80% of the frame, half of it is the fighter draw, and 70% of *that* is these two call sites — so
> the whole prefix clear is **~3.1% of the frame, on the order of 52,000 ticks/tic**. Against R4c's
> measured 1,085,877 that is under 5%. **The overwhelming majority of what the no-oracle bracket
> removed is the lighting/material derivation itself — real rendering work that produces the shading
> the pixels lost — not bookkeeping.**
>
> So the honest statement of the remaining problem is not "proof work is hiding in the fighter draw".
> It is: **the fighters' lighting and material derivation costs ~1.0M ticks per Results tic, and
> switching it off is so far the only measured way to get inside the gate.** Removing the proof
> clearing is worth ~52,000 and does not change that. R4f is closed; do not spend a build on it.

> **R4c's VERDICT IS INVERTED by R4g's first reading, same day. `no_oracle` is not a proof switch —
> it selects the RENDERER, and the two scenes are on different ones.**
>
> `reloc_backend_renderer_dl.c:12603` enters the **native fighter owner** only when
> `(detailed_output == FALSE) && (no_oracle != FALSE)`. So the flag chooses between the generic DL
> interpreter and the specialised native owner that R2-03 spent the campaign building — which is why
> switching it moved 74.9% of `FTR` and changed the lighting (E48: "flash is a raw vertex colour,
> **native owner lights it**"; E59: "generic lighting never ran").
>
> Where the bracket actually sits, in `reloc_backend_movement.c`:
>
> | site | enclosing function | bracketed? |
> | --- | --- | --- |
> | `:13597` | `ndsSceneMipCachePresentFrame` | yes (`:13591`/`:13599`) |
> | `:13669` | `ndsStageGCDrawAllLoopSubmitHardwareFrame` | **no** |
> | `:13808` | `ndsStageGCDrawAllLoopPresentHardwareFrame` | yes (`:13724`/`:13810`) |
>
> The canonical battle present is `taskman_seam.c:4796` →
> `ndsFighterMarioFoxStageGCDrawAllLoopPresentHardwareFrame` → `:13808`, **inside** the bracket.
> **Battle already draws its fighters with the native owner.** VS Results reaches
> `ndsFighterDisplayContractSubmit` through the scene draw with no bracket anywhere on the path, so
> it draws the same fighters with the **generic interpreter plus oracle** — four times the cost, and
> lit by a different code path.
>
> So R4c is not "trade fighter shading for frame rate". It is **"put Results on the same renderer the
> match already uses"**, and the shading delta is Results and battle having disagreed all along. That
> reverses the burden of proof: the question is no longer whether to accept a regression, but which
> of the two looks is intended — and battle's is the one the campaign optimised, verified and shipped.
>
> **Not yet graduated, because the claim is one step short.** The path evidence is code-read, not
> pixels: nothing here has yet put a native-owner Results fighter beside a native-owner battle
> fighter and shown they agree. That comparison is confounded by scene lighting (Dream Land versus
> the Results backdrop), so it needs designing rather than eyeballing. Until then R4c stays `?= 0`
> and the owner's look at the zoom pair is the fastest route.

> **R4h RAN, and it found a defect in R4c before it found anything about the pixels.**
>
> The experiment: `NDS_R4H_BATTLE_GENERIC_FIGHTERS` (lab only) drops **only** the fighters back to the
> generic interpreter inside an otherwise identical battle frame, stage bracket intact, so scene,
> camera and lighting environment are held constant and the renderer is the sole variable — the
> confound the Results pair could not escape. Exact-frame capture at battle frame 400, both arms:
> **2,765 of 240,000 guest pixels differ (1.15%), max channel delta 251**, localised to the fighters
> and the damage HUD. The two renderers do produce visibly different fighters in the same scene, and
> the shipped battle build is the native-owner arm by construction. So R4c makes Results agree with
> the match rather than disagree with it.
>
> **The defect.** `ndsFighterDisplayContractSubmit` is reached from
> `ndsFighterDisplayContractSubmitStageFighters` **inside** the battle bracket, and R4c's first
> implementation ended with `ndsRendererHardwareSetNoOracle(FALSE)`. In a match that would have
> cleared the stage's own setting for everything drawn after the first fighter — silently dropping
> the rest of the stage present off the native owner. It now saves and restores, which makes the
> bracket a no-op wherever the flag is already set, i.e. on the whole battle path. Re-measured after
> the fix: Results **581,197/tic, 1.04 VBlanks, 0.52× — inside the gate**, so the correctness fix cost
> nothing. Battle `FTR` p50 388,736 → 389,056 and p95 392,064 → 392,576, unchanged within noise,
> which is the signal that the no-op holds.
>
> **Still not graduated, and now for a smaller reason.** That battle run carried an outlier — `ALL`
> max 5,874,240 against the control's 2,240,576, `SRC` max 4.49M — which moves `WORK` p95 and makes
> the run unusable as a clean battle result even though the bucket the change touches is flat. Two
> things remain: one clean battle re-run, and the owner's read on which fighter look is intended. The
> second is no longer "accept a regression?" but "Results and the match disagree; the match's look is
> the one the campaign shipped — align them?"
>
> **The clean battle re-run is DONE and R4c is safe.** Both arms re-sampled at `-StartFrame 600`, 128
> samples, same window, so the earlier outlier is out of the comparison:
>
> | bucket p95 | control | R4c on |
> | --- | --- | --- |
> | `ALL` | 1,680,064 | **1,120,384** |
> | `WORK` | 1,197,760 | **1,106,112** |
> | `WORK-H` | 1,071,488 | 1,070,848 |
> | `FTR` | 390,208 | 391,040 |
>
> Every named bucket is flat within noise and `WORK` p95 falls 91,648 — the battle path is not
> regressed, which is what the save/restore no-op predicted. `FTR` moving +832 is the direct
> confirmation that battle fighters were already on the native owner and this changes nothing for
> them. The R4h lab probe has been deleted from `reloc_backend_movement.c` and the Makefile per the
> no-permanent-probes rule; its findings are recorded here and the flag it proved out stays.
>
> **R4c is now one line from graduation and everything measurable about it is green.** What is left is
> not a measurement: `AGENTS.md` gates rendering-side changes on "synchronized screenshot diffs plus
> **the owner's visual approval**", and this changes how the fighters look on a milestone screen. The
> diffs are reported, the pairs are in `artifacts/visibility`, and the question to answer is which of
> the two fighter looks is intended. On yes: set `NDS_R2_FIGHTER_NO_ORACLE ?= 1`, run Latest, and
> Results lands at **581,197/tic — 0.52× of the gate**, from 2,814,955 this morning.

Evidence: `artifacts/performance/r4c-fighter-no-oracle-on-20260730.json`,
`artifacts/visibility/2026-07-30_r207-r4c-results-tic160-candidate.png`,
`artifacts/visibility/2026-07-30_r207-r4c-diff.png`, and the fighter zoom pair
`2026-07-30_r207-r4c-fighters-{r4e,r4c}.png`.

### R2-07 R4c RE-BASELINED on the ARM renderer — memset is fighter-draw work, not a subsystem; the fighter draw runs with the oracle ON while the stage runs with it OFF (2026-07-30)

First valid Results attribution in the campaign: every earlier census profiled a Thumb renderer (see
R4e). Re-censused on the `-marm` build, 60 frames, 201,664,950 cycles:

| symbol | % | note |
| --- | --- | --- |
| `ndsRendererExecuteNativeFighterRootHardware` | 17.03% | largest single symbol |
| `memset` | 8.80% | 140 bytes of code |
| `armWaitForIrq` | 7.62% | idle, not work (was 18.85% pre-R4e) |
| `memcpy` | 6.45% | |
| `ndsFighterMarioFoxDLAllDrawForSlot` | 6.44% | |
| `ndsFighterMarioFoxVisitDLDrawCommand` | 4.93% | |
| `__aeabi_lmul` | **absent** | was 7.79%; R4e removed it |

`memset` + `memcpy` = 15.25% looked like a second lever beside the fighter draw. **It is not.** A
`$lr` sample at a `memset` breakpoint (80 dynamic hits on the Results lab) attributes **50.0% to
`ndsFighterMarioFoxDLAllDrawForSlot` and 18.8% to `ndsRendererAdapterBuildNativeMaterialSnapshot`** —
both inside the fighter draw. So ~69% of memset is already inside `FTR`, and the fighter draw owns
the frame even more completely than the 85.2% bracket says.

**Static call-site counts would have sent this the wrong way.** 96 functions contain a `bl memset`
and the ranking by *static* site count is not the ranking by *executed* calls — `ndsAudioFgmDiagnosticsReset`
has 10 sites and never runs here. Dynamic `$lr` sampling is the method; it also answered a question
the source had been carrying unanswered since Task 91 (`reloc_backend_renderer_dl.c:12192`: "memset
is 38,393 ticks/frame across the whole program and nothing says how much of it is this"), and that
comment now records the answer.

Inside the fighter draw the offsets localise further: the three `bzero`s in the reset block are only
15% of its memset traffic, while **70% is two call sites in
`ndsFighterDLDrawResetTransientRendererStats` (`:4748`)**, which clears the *per-list proof/counter
prefix* once per part list, per fighter, per frame.

**The lead for the next R4c experiment.** That reset runs only when `detailed_output` is set, which
is driven by `sNdsRendererHardwareNoOracle`. The **stage** draw already brackets its own draw with
`ndsRendererHardwareSetNoOracle(TRUE)`/`(FALSE)` (`reloc_backend_movement.c:13559`); the **fighter**
draw does not, so fighters render through the oracle path and pay its per-list proof-counter clearing
on every frame of the shipped ROM. Whether the fighter draw can take the same no-oracle path the
stage already takes is the question — it is a behavioural change to the render path, so it needs its
own A/B plus a matched-tic visual pair, not a fold into this entry.

### R2-07 R4e GRADUATED — the Results lab built its renderer `-mthumb` while every battle ROM built it `-marm`. −553,188/tic (−24.5%), pixel-identical (2026-07-30)

**This is an instrument defect that was reported as an optimization, and the correction matters more
than the number.** Chasing R4c's fighter cost, the Results census put `__aeabi_lmul` — the 64-bit
multiply helper — at **20,935,676 cycles, 7.79% of the frame**, third behind the idle spin and the
fighter root, out of **86 bytes** of code. `objdump` put 31 of its 79 call sites in five functions
that all disassembled to **Thumb with zero SMULL between them**.

ARMv5TE Thumb has no `SMULL`. It has no 32×32→64 multiply at all, so every `(s64)a * b` in a Thumb
function is a `bl __aeabi_lmul` regardless of how the operands are typed. The 20.12 fixed-point
matrix and vertex math is exactly that shape, per vertex.

The cause was one line:

```make
ifeq ($(NDS_DEV_SCENE_HARNESS_ID),163)     # battle only
nds_renderer.o: CFLAGS += -marm
endif
```

`results_playable` is harness **164**. It was added to the tick-HUD Makefile block under a comment
promising it "must differ from the tick-HUD ROM in the scene it boots and in **NOTHING** else" — and
this rule, keyed on the harness ID rather than on the target, silently broke that promise. So
**every Results number in this campaign — R0c, R0d, R0e, R2a, R2b, R4b, R4d — was measured on a
Thumb-compiled renderer while battle measured an ARM-compiled one.** Those deltas remain valid
against each other (all arms shared the defect); their absolute cost was inflated.

The fix keys on the latency surfaces (`NDS_ARM_RENDERER_HARNESS_IDS := 163 164`) rather than on one
ID, so the next latency ROM cannot inherit the same trap.

| | Thumb (before) | ARM (after) | Δ |
| --- | --- | --- | --- |
| VBlanks / source tic | 4.03 | 3.04 | −0.99 |
| ticks / source tic | 2,254,765 | 1,701,577 | **−553,188 (−24.5%)** |
| `FTR` / source tic | 1,710,498 | 1,449,776 | −260,722 (−15.2%) |
| vs the 1.12M gate | 2.01× | **1.52×** | |

Guest picture **PIXEL-IDENTICAL** against HEAD: 240,000 guest-viewport pixels at Results tic 160,
0 differing, max channel delta 0.

**One approach was built and withdrawn.** Before finding the Makefile line, the same win was chased
with `__attribute__((target("arm")))` on the five hot functions behind an `NDS_R2_ARM_MULTIPLY` flag.
It worked — 3.11 VBlanks, 1,743,591 — but it is a hand-rolled partial duplicate of a compile flag the
target should already have had, covering five functions instead of the translation unit, and it
measured **42,014 ticks worse** than fixing the flag. Flag, macro and all eight attribute sites were
deleted. What survives from it is one line of C: `ndsRendererTransformVertex20p12` declared its `x`,
`y`, `z` as `s64` when the source fields are `s16` and the matrix is `s32`, so every product fit in
47 bits and never needed the 64×64 path; they are `s32` now, same value, and the ARM codegen is one
`SMULL` instead of a widened sequence.

**The battle path was measured and is unaffected** — it already had `-marm`. Both battle arms
produced a **byte-identical loadable ARM9 image** and identical 128-sample percentiles, which is how
the flag was found to be inert there: the tick-HUD sampler reported the same `sha` twice, and the
`.o` files differed while `objcopy -O binary` output did not. No battle P95 in the campaign moves.

### R2-07 R4d GRADUATED — Results presented every frame TWICE; the second present rendered nothing and only burned a VBlank. −560,190/frame (−19.9%), pixel-identical (2026-07-30)

Chasing R4c's "second waiter" found it, and it was not a waiter at all — it was a whole second
present. **VS Results ran 2.00 `ndsPlatformEndFrame` calls per source tic. The battle path runs
1.00.** Half of every Results frame was a present that submitted no geometry, so its `glFlush` was
skipped, but the `swiWaitForVBlank` inside `ndsPlatformEndFrame` is not conditional and ran anyway.

Attribution, all from one two-stop census over 80 source tics (`census-results-frame-cost.ps1`):

| counter | per source tic | reading |
| --- | --- | --- |
| `task_update` / `scene_draw` | 1.00 / 1.01 | the scene updates and draws once, as designed |
| GX submits / flushes | 1.00 / 1.00 | **only one present renders anything** |
| `sTicks` (presents) | 2.00 | but two presents happen |
| `gNdsFrameCounter` | 1.00 | and the surplus one is `main.c:63` |

The mechanism is the coroutine seam. Threads are coroutines (`portCoroutineResume`/`Yield`,
`libultra_os.c`), the scene loop is resumed from `ndsOsRunThreads()` at `main.c:53`, and when it
yields, `main.c` falls straight through to its own `BeginFrame`/`RenderDebugHud`/`EndFrame` — a
fallback present meant for when *nothing else* is driving the display, firing once per scene
iteration on top of the scene's own present.

Fix (`NDS_R2_MAIN_PRESENT_GUARD`, `main.c`): sample `ndsPlatformTicks()` before `ndsOsRunThreads()`
and present only if it did not advance. Self-correcting — during boot and loading, when no scene
loop presents, the fallback still runs exactly as before. The generic scene loop also picked up the
`ndsPlatformRenderDebugHud()` call the battle present already had (`taskman_seam.c:4810`), so a
scene that drives its own presentation draws its own HUD instead of relying on a present being
removed.

Matched-source A/B, both arms rebuilt from identical source with only the flag differing:

| | guard off | guard on | Δ |
| --- | --- | --- | --- |
| presents / source tic | 2.00 | 1.00 | −1 |
| VBlanks / source tic | 5.03 | 4.03 | **−1.00** |
| ticks / source tic | 2,814,955 | 2,254,765 | **−560,190 (−19.9%)** |
| `WAIT` / source tic | 977,046 | 417,410 | −559,636 |
| `FTR` / source tic | 1,709,228 | 1,710,498 | +0.07% (noise) |
| GX submits, flushes | 1.00 | 1.00 | unchanged |

The saving is **exactly one VBlank** and the removed cost is **entirely wait** — real work is
untouched to within noise, which is what a redundant present should look like when it is removed.
Guest picture **PIXEL-IDENTICAL**: 240,000 guest-viewport pixels at Results tic 160, 0 differing,
max channel delta 0.

Against the 1.12M gate Results is now **2,254,765 = 2.01× over**, down from 2.51×, and `FTR` at
1,710,498 is 75.9% of the frame — so R4c's fighter question is now the whole remaining problem
rather than one of two.

**Tooling lesson, and it invalidates the shape of earlier comparisons.** The first diff of this pair
reported 74 differing pixels at max channel delta 243 and read as a regression. Every one of them was
melonDS's **title bar**: `[77/60] melonDS` versus `[61/60] melonDS` — the emulator's host-speed
readout, which changes precisely *because* the candidate is faster. Comparing the full window makes
every genuine speedup look like a visual regression, and the bigger the win the worse it reads.
`scripts/compare-capture-pair.ps1` (new, shared — R2b, R4b and R4d had each done this by hand) now
crops to the 400x600 guest viewport by default and needs `-IncludeWindowChrome` to do otherwise.

### R2-07 R4c SIZED from the real ROM — the Results fighters ARE the gate: 61.0% of non-wait work, and dropping them lands 2 VBlanks (2026-07-30)

> **Per-frame figures below are per PRESENT and Results ran two presents per source tic — see the
> retraction note at the end of this entry, and R4d above.**

No build needed for this one. `smash64ds-results-lab-hwtri` runs **only** Results, so its
cumulative tick-HUD buckets are Results-scoped by construction — the first Results cost numbers in
this campaign that are neither census-inflated nor mixed with battle frames. 326 Results frames,
measured mode 5 VBlanks = 2,800,950 ticks:

| bucket | ticks/frame | % of frame |
| --- | ---: | ---: |
| `FTR` fighters | **1,087,212** | 38.8% |
| `WAIT` VBlank wait | 1,018,354 | 36.4% — idle, not work |
| `BG` background | 12,354 | 0.4% — R2b did its job |
| unbracketed remainder | 682,968 | 24.4% |

**Non-wait work is 1,782,595/frame and the fighters are 1,087,212 of it — 61.0%.** Drop them
entirely and work falls to 695,382 = **1.24 VBlanks**, which presents at 2 and meets the gate with
room to spare. Put the other way: reaching 1,120,380 needs **−662,215 off the fighters, a 61% cut**,
and needs nothing at all from anything else on the screen.

**RETRACTED THE SAME DAY BY A PROBE.** The claim that removing the fighters lands 2 VBlanks was
arithmetic on a bracket, and the bracket was not what I assumed it was. A build that skips
`ndsRendererExecuteNativeFighterRootHardware` outright on the Results-only ROM measures, over 774
frames:

| | control | fighter draw skipped |
| --- | ---: | ---: |
| P50 / P95 | 5 VB — 2,800,950 | **3 VB — 1,680,570** |
| 2-VBlank frames | 117 (36.0%) | 117 (15.1%) |
| 3-VBlank frames | 0 | **650 (84.0%)** |
| `FTR` bracket/frame | 1,087,212 | **426,274** |

Two corrections, both against the paragraph above:

1. **Removing the fighter draw does not reach the gate.** It is worth two whole VBlanks — 5 → 3,
   −1,120,380 ticks, the largest single Results movement measured so far — and P95 still lands at
   1,680,570, **1.5× over the 1.12M gate**. "−662,215 off the fighters and nothing else" was wrong.
2. **`FTR` is not all draw: 39% of it survives skipping the hardware execute.** 426,274 ticks/frame
   of fighter work is update/animation, which no draw-side lever can touch. It is still 68% of
   non-wait work in the probe build, so fighters remain the top owner — but via two levers, not one.

So R4c needs both halves of the fighter cost and likely more besides; no single measured lever closes
Results, the same shape R2-06 E10 reached for the battle frame. Probe deleted after answering, per
the no-permanent-probes rule.

**THE REAL WORK BUDGET FOR THE 1.12M GATE IS ABOUT 560,190, NOT 1,120,380 — Results pays an
unconditional whole-VBlank tax per iteration.** This is why every sizing in this entry kept coming out
optimistic, including the probe`s.

`ndsPlatformWaitForScheduledVBlank` (`nds_platform.c:3046`) is a **do-while**: it calls
`swiWaitForVBlank()` at least once unconditionally, and only keeps waiting while
`sEarliestPresentVBlank` is set and unreached. That target is set in exactly one place,
`ndsPlatformSchedulePresentAtVBlank`, called from exactly one site — `taskman_seam.c:4903`, the
**battle** present path. **Results never schedules a present**, so on this scene the condition is
always false and the loop burns one whole VBlank every iteration no matter how little work it did.

The 1.12M gate is two VBlanks. One of them is spent waiting by construction. So Results has to fit
*all* of its work inside a **single** VBlank period, 560,190 ticks — half the budget the headline
figure implies. Against that, current non-wait work of 1,782,595 is 3.2× over, not 1.6×.

It also explains the shape of everything measured here: the 36% of frames already at exactly 2
VBlanks are the near-zero-work frames before source tic 120 (one VBlank of work-span plus the
mandatory wait), and removing the entire fighter draw moved 5 → 3 rather than to 2 because the tax
survives the cut.

**Not fully explained, and the next thing to chase:** measured `WAIT` is 1,018,354/frame ≈ 1.8
VBlanks, which is *more* than one unconditional call accounts for. Something waits a second time —
likely the source`s own VI retrace path. Find it before designing around the tax, because if the
second waiter is removable it is worth more than any fighter lever on the table.

> **ANSWERED AND PARTLY RETRACTED BY R4d (below), same day.** The second waiter was real and *was*
> removable — it was a second `ndsPlatformEndFrame`, from the fallback main loop. Two corrections to
> the paragraphs above. **(1) The per-frame figures in the table are wrong by 2×.** They divide
> free-running accumulators by source tics, but Results ran **2.00 presents per source tic**, and
> these buckets are zeroed only in the battle presentation loop (`taskman_seam.c:5077`, `:7769`),
> which this scene never reaches — so they are per-*present*, not per-tic. The impossible 1.8-VBlank
> "single" wait was that error announcing itself: one `swiWaitForVBlank` cannot exceed one VBlank, and
> 1.747 / 2 = 0.87 is legal. **(2) The tax is one VBlank per logical frame, not per present, and after
> R4d it is paid once instead of twice.** The "real work budget is 560,190" conclusion still holds for
> the *post*-R4d single present; it was never the whole story, because the frame was paying it twice.
> Method fix, now in `scripts/census-results-frame-cost.ps1`: **difference two stops** and divide by
> presented frames (`sTicks`), never read an accumulator once and divide by a scene clock.

**A POSE MEMO IS RULED OUT — owner, from the running game, 2026-07-30: "they do animate in the
results screen."** Worth recording because the source reads the other way at a glance and would have
cost a build. `mnVSResultsFuncRun` (`mnvsresults.c:3227+`) does nothing per tick after
`mnVSResultsInitFightersAllTic` except ramp `sMNVSResultsCharacterAlpha` by `0x16` until it saturates
(~12 tics) and push it through `scSubsysFighterSetLightParams` — no animation advance anywhere in the
Results run function. That reads as "posed once, then static", which would have made the whole
fighter pose memoisable exactly like R4b's layer.

It is wrong. `mnVSResultsInitFighter` ends in `mnVSResultsSetFighterStatus`
(`mnvsresults.c:892-920`), which calls `scSubsysFighterSetStatus` with the kind's **win or lose
status** — a normal fighter status whose animation is then advanced by the fighter system itself, not
by the Results scene. So the Results run function's silence means the animation is driven from
elsewhere, not that there is none. `gcPlayDObjAnimJoint` appearing in the census was the hint and it
was not followed up.

R4c is therefore a **rate** problem, not a memo problem: `PROJECT_GOAL.md` permits reduced animation
update rates and 30 Hz skeletal poses, which is the lever that survives. Note the Results loop
already presents at ~5 VBlanks, so check what the animation actually advances per presented frame
before assuming a halved rate is free — the mismatch may be smaller than it looks.

The original sizing's one durable part: the bimodality agrees independently — one owner and the
already-measured bimodality agrees with it independently — the 36% of frames that already sit at 2
VBlanks are exactly the ones before source tic 120, which is when the fighters are initialised
(`mnvsresults.c:2799-2846`).

**Why a 61% cut is plausible here and would not be mid-match.** On Results the fighters are not
being played: they hold a win/lose pose and fade in at alpha `0x16` per tic. `PROJECT_GOAL.md`
explicitly permits reduced animation update rates and 30 Hz skeletal poses, and the switch plan's
own headroom options list running cosmetic systems below simulation rate. The pose is nearly static,
so a Results-only specialisation or a reduced pose rate is in scope where it would be unacceptable
during gameplay.

**Heed the switch plan's warning when building it:** do *not* implement a reduced rate as "every Nth
frame, update everything". The gate is a P95, and batching the skipped work onto one frame in N
converts a mean win into a P95 spike. Spread or memoise instead.

### R2-07 R4a MEASURED — the first post-R2b Results profile: the software compositor STILL owns 41.03% of the frame, and idle is 25.82% (2026-07-30)

The owner extended the 1.12M gate to the Results screen ("the same tick budget philosophy should
apply to the results screen, 1.12M cap and work backwards"), and approved R2b on sight, which cleared
Gate 0 of `docs/optimization/VS_RESULTS_30FPS_RESEARCH_20260730.md`. This is its Gate 1: the profile
that did not exist. `run-task37-profile-census.ps1 -Scene Results -Frames 40`, Results tics 131..171,
DLDI on, **0.00% unattributed**, 343 of 3,402 FUNC symbols hit. Evidence, config and ROM hash at
`artifacts/task37-census/r207-r4-postr2b/`; flags verified in the generated header, not assumed —
`NDS_FAST_WALLPAPER_AFFINE 1`, `NDS_R2_RESULTS_AFFINE 1`.

| owner | ticks/frame | %tot |
| --- | ---: | ---: |
| `armWaitForIrq` | 2,024,737 | 25.82% — **idle, not work** |
| `memset` | 985,377 | 12.56% |
| `ndsPlatformCommitOriginalSpritePreviewLayer` | 975,856 | 12.44% |
| `memcpy` | 669,786 | 8.54% |
| `ndsDrawSObjIntoPreview` | 586,712 | 7.48% |
| **software compositor subtotal** | **3,217,732** | **41.03%** |
| `ndsRendererExecuteNativeFighterRootHardware` | 563,596 | 7.19% |
| `ndsFighterMarioFox` DL family | 381,533 | 4.86% |

**Read this as ownership inside this binary, never as absolute cost.** The window totals 7,842,772
ticks/iteration against the phase-aligned histogram's 3,921,330, because a `NDS_TASK37_PROFILE=1`
build is not the measured build — it carries per-frame region markers and a different ITCM pack. R0f
already paid for treating one instrument's absolute row as another's; do not repeat it by subtracting
across these two.

**What it settles.** R2b deleted the background layer and the compositor is *still* the largest owner,
so the foreground staging path — one 153,600-byte clear, the blits, the 320×240→256×192 downscale, one
98,304-byte VRAM copy — is the Gate 2 target on measurement rather than on sizing. `memset` plus
`memcpy` alone are **21.10%**: a fifth of the Results frame is buffer traffic for a screen the DS would
show for nearly free on BG/OAM.

**And the 25.82% idle is the number that decides how to judge the next cut.** Rule 12 again: 2,024,737
ticks/frame of slack means any cut smaller than that lands as spin and the wall clock will not move.
The compositor at 3,217,732 is the only measured block bigger than the slack, which is the second
independent reason it is the right target — a fighter cut (7.19%) or a DL cut (4.86%) would be
invisible to a cadence instrument no matter how real it is.

**Open question this profile raises, for whoever takes R4b.** R0h measured two staging clears at
830,978 ticks/frame combined; with the background layer gone, one clear should remain, yet `memset`
reads 985,377. Profiling overhead explains some of it. It does not obviously explain all of it. Before
building the hardware UI path, confirm with an in-build byte counter (`gNdsSObjForegroundStagingClearBytes`
and its background twin already exist at `sprite_preview_backend.c:2558-2567`) that exactly one 320×240
clear and one 98,304-byte copy happen per Results frame. If a background clear is still running under
the affine path, that is a cheap fix R2b left on the table and it should be taken first.

### R2-07 R2b GRADUATED — the Results background moves to the hardware affine BG; the whole background layer disappears, −1,736,589 ticks/frame (2026-07-30)

The owner asked the question that unblocked this: *"we already know affine backgrounds and native
stages work well, can we just apply that to the results screen?"* Yes. R2's original plan was a
dirty-flag on the software compositor; the affine BG deletes the layer instead of skipping it.

Behind `NDS_R2_RESULTS_AFFINE ?= 0`. Both arms built from identical source, flag the only difference,
`NDS_FAST_WALLPAPER_AFFINE=1` in both.

| arm | VB / 40 iters | VB/iter | ticks/frame | Results FPS | `I/4b 300x220` census row |
|---|---|---|---|---|---|
| A control | 405 | 10.125 | 5,672,000 | 5.85 | 206 VB, 50.9% |
| B candidate | **281** | **7.025** | **3,935,335** | **8.43** | **absent** |

−3.1 VBlanks/iteration = **−1,736,589 ticks/frame**, against the 1,746,558 R0h sized for the whole
background layer (blit 565,315 + clear 415,489 + downscale 487,191 + VRAM copy 278,563) — within
**0.6%**. The layer is gone, not merely cheaper, and the census row vanishing is the direct proof: the
wallpaper is no longer drawn in software at all.

Confirmed on a second, independent, quantum-free instrument. `soak-freeze-watch.ps1` is a fixed
wall-clock race, so it cannot be fooled by the VBlank flooring of standing rule 11: 2.5 minutes,
`gNdsVSResultsTickCount` 500 → 780, **+56%**, both arms NO-FREEZE, both
`gNdsBattlePlayablePacingPresentedFrames = 2043` — the battle path is provably unperturbed.

What it took, and both parts were needed:

1. The combine bakes. `ndsSpriteLerpPrimEnv` only ever reads the source intensity, so for I/4b the
   whole prim/env combine collapses to **sixteen palette entries** baked into the decode cache once
   per scene. That is what lets a *combining* wallpaper into a cache that had no way to represent
   per-pixel work. Every entry has bit 15 set — proven at runtime, not assumed: the cache's
   fully-opaque precondition is what admits the last-writer mapping, and the census row disappearing
   means it passed.
2. **Full-bleed the mapper.** `ndsSObjDrawOpaqueWallpaperFinal` maps each of the 256 overlay columns
   into 320-wide preview space (`preview_x = 1.25x`) and drops any column outside
   `[origin_x, origin_x + width*scale)`. Dream Land sits at (0,0) and covers everything; the Results
   wallpaper sits at **(10,10)**, which left `preview_x < 10` and `>= 310` unmapped — an **8-pixel**
   backdrop frame on all four sides, because 10/1.25 = 8. Fixed by mapping the whole preview onto the
   whole source: origin 0, scale = preview/source per axis. Costs nothing — 281 VBlanks before and
   after the fix, because the extra seed pixels are written once per scene.

Two process notes, both nearly expensive:

- My first fix went to `ndsSObjFastWallpaperGetTransform` on the theory that the *hardware* was
  double-applying the offset. It changed nothing, because `ndsSObjDrawCachedWallpaperFinal` reads
  `sobj->pos` directly at `sprite_preview_backend.c:1574` and bakes the offset into the seed pixels.
  One build and one capture spent on a guess; the mapper's algebra was free to read and gave the
  exact number. **Derive geometry from the mapper, not from measuring a screenshot.**
- The control and candidate captures were taken at the same *wall clock* and therefore at different
  *scene ticks*, because the candidate runs Results 44% faster. The candidate showed panels the
  control had not revealed yet, which made a genuine 8-px regression look like it might be nothing
  but a different animation frame. Ruled out by capturing the control at a matched scene state — it
  is full-bleed both early and late. See standing rule 13.

Status: **GRADUATED 2026-07-30.** The owner approved the matched-tic capture pair on sight — "the
affine one looks perfect" — so `NDS_R2_RESULTS_AFFINE` is now `?= 1` and
`check-gbi-decode-fixtures.ps1` pins it default-**on** instead of default-off. Evidence at
`artifacts/visibility/2026-07-30_r207-r2b-results-{control-software,candidate-affine,letterbox-defect}.png`
and `artifacts/performance/r207-r2b-{control,candidate}.json`. Note the flag is inert wherever
`NDS_FAST_WALLPAPER_AFFINE` is 0 — the seed capture is compiled out — which is why the differ target
is unaffected; every published and lab target already forces that flag to 1.

This is a correct prerequisite, **not** a route to 30 FPS on Results. Retract the sentence this entry
used to carry, that `IA/8b 24x37` owns 85.1% of the remainder: that row is a next-hit interval, so it
spans the commit, the fighters, camera/display work and the platform wait as well as the glyph. It is
a locator for the unpartitioned tail, never symbol attribution — the owner's research doc
(`docs/optimization/VS_RESULTS_30FPS_RESEARCH_20260730.md`) makes the same correction, and the R2b
JSON records `phases:false`. The real post-R2b owner is unknown until the census below lands.

### R2-07 R2a BUILT — the glyph lerp is gone and the VBlank census reads EXACTLY FLAT, because the loop has 1.48 VBlanks of idle slack (2026-07-30)

**Threshold pre-registered before the profile ran** (standing rule 7): KEEP if the per-PC profile puts
`ndsDrawSObjIntoPreview` down **≥100,000 ticks/frame** with no other symbol up by more than the
~14,000-tick floor; REVERT below that, or if total cycles rise. The lerp family measures 182,901
ticks/frame in `ndsSpriteLerpPrimEnv`'s own source lines alone, so a real removal must clear 100,000
comfortably — the threshold is deliberately generous against my own estimate.

**The change.** R0h's per-PC data priced the seven IA/8b glyphs at **538,300 ticks/frame for 6,882
pixels — 78.2 ticks/pixel against the specialized wallpaper row's 8.6.** They are expensive for two
reasons, and BattleShip settles both: `mnvsresults.c:1204` sets `scalex` and clears `SP_FASTCOPY`
outright, so they take the **rect-fill** arm, *and* they run the per-pixel `ndsSpriteLerpPrimEnv`. R2a
takes only the second: the sixteen-entry table R0e already builds is exactly `lerp(sobj, n*17)`, which is
what the IA arm asks for, so the call becomes a lookup inside the unchanged generic loop. The alpha test
stays where it was rather than being folded into the table, because the I4 combine arm has no alpha test
and folding would change the wallpaper when alpha is zero. Table gate widened to either format; the
paired row keeps its own format test so an IA sprite cannot fall into it.

**The census says 410 VBlanks against 410 — identical windows, zero change.** That is not resolution:
the window is a 40-frame aggregate resolving ~14,000 ticks, and the predicted saving was ~227,000.
**The cause is idle slack.** R0h measured `armWaitForIrq` at **830,260 ticks/frame, 14.82%** — the
Results loop already waits ~1.48 VBlanks every frame, so work removed below that threshold becomes spin
and the wall clock cannot move. Rule 11's 560,190-tick quantum was the smaller of the two problems and
this is now **rule 12**: measure the slack before trusting a wall-clock instrument, and pick the
instrument from the size of the expected win.

`PROJECT_GOAL.md` and AGENTS.md both settle the disposition in advance of the number: milestone tick
targets are "directional, not per-cut discard gates", and the instruction is to "keep every repeatable
correctness-preserving gain and accumulate it toward the target". A flat wall clock is not evidence a
lever is worthless.

**ARBITRATED: KEEP, −200,133 ticks/frame off the blitter, double the pre-registered threshold.** Same
profiler, same window, `artifacts/task37-census/r207-r2a/`:

| | R0e | R2a | delta/frame |
| --- | --- | --- | --- |
| `ndsDrawSObjIntoPreview` | 88,289,261 | 72,278,619 | **−200,133 ticks (−18.1%)** |
| `armWaitForIrq` (idle spin) | 66,420,781 | 81,903,105 | **+193,529 ticks** |
| total cycles | 448,150,712 | 448,150,320 | **−392 (flat)** |
| instructions | 148,358,461 | 136,768,388 | **−289,752** |

**The two middle rows are the whole story: −200,133 of work out, +193,529 of spin in, matching to 3%.**
That is rule 12 demonstrated rather than argued — the reclaimed time is *visible*, sitting in the idle
counter, and the wall clock is flat because the loop had nowhere else to put it. Nothing else regressed
past the ~14,000-tick floor; the worst mover was `ndsRendererExecuteNativeFighterRootHardware` at +2,753.
Only one inlined `ndsSpriteLerpPrimEnv` copy survives in the whole blitter (the table build), verified by
counting `muls` against source lines.

**The banked total on this scene is now real work removed, not FPS.** Blitter 1,103,616 → 903,483
ticks/frame. Results FPS is unchanged at 5.85 and will stay there until a cut exceeds the ~830K slack —
which is precisely what R2 (the compositor, 3,466,102) is sized to do, and why R2a is worth keeping *now*
rather than after it: with the slack consumed by a big structural cut, these 200,133 ticks become FPS.

Latest on the final source (fallback and per-pixel null test removed) before the source commits.

**R2 sized from the same artifact, and the answer is uncomfortable — say it before building anything.**
Both compositor stages run once per layer, so splitting them by layer is arithmetic on known geometry
(2 × 153,600-byte clears, 2 × 49,152-pixel downscales, 2 × 98,304-byte VRAM copies) plus the measured
per-arm blit split:

| layer | blit | clear | downscale | VRAM copy | total/frame |
| --- | --- | --- | --- | --- | --- |
| background (wallpaper, link 26) — **static** | 565,315 | 415,489 | 487,191 | 278,563 | **1,746,558** |
| foreground (glyphs + tints, links 29/34/35) | 338,167 | 415,489 | 487,191 | 278,563 | **1,519,410** |

Only the background is static: the tints **fade**, decrementing alpha every frame, and they are
foreground. So a dirty-flag skip is worth **1,746,558 ticks/frame** and is pixel-identical when correct —
the overlay is single-buffered and `nds_platform.c:781-783` already documents relying on its contents
persisting. With 830,260 of slack to absorb first, the frame drops by ~916,300 ticks = **1.64 VBlanks:
10.25 → ~8.6 VB/iter, 5.85 → ~7.0 FPS.**

**That does not approach the gate, and no arrangement of these four stages does.** 2 VBlanks means
removing ~4.6M of 5.6M, i.e. the entire software compositor *and* most of the fighter draw. The Results
screen is a static wallpaper, seven static glyphs, two fading tints and two 3D fighters — content the DS
would show for nearly free on hardware BG layers and OAM, which this scene does not use at all. So the
honest statement is that **R2's dirty-flag is a correct prerequisite worth 1.75M, not a route to 30 FPS
on this screen**, and reaching the gate here is a presentation-architecture decision (hardware BG + OAM
for `nSCKindVSResults`, the ungated half of `sprite_preview_backend.c:2456-2458`) whose scope the owner
should see before it is started. Do not begin the architecture change on the strength of this note; do
begin the dirty-flag, which is needed either way.

### R2-07 R0h ANSWERED — there was never a residual; R0f's split was an instrument artifact, and the real owner is a four-stage software compositor (2026-07-30)

`run-task37-profile-census.ps1 -Scene Results -Frames 40`, DLDI on, window Results tics 131..171.
**448,150,712 cycles / 40 iterations = 5,601,884 ticks/frame**, against the VBlank census's 5,741,947 —
the two instruments agree on the total to **2.4%**. They do not agree on anything else.

| symbol | ticks/frame | %tot | what it is |
| --- | --- | --- | --- |
| `ndsDrawSObjIntoPreview` | **1,103,616** | 19.70% | stage 1: composite sprites into 320×240 staging |
| `ndsPlatformCommitOriginalSpritePreviewLayer` | **974,382** | 17.39% | stage 3: in-place nearest 320×240 → 256×192 |
| `memset` | **830,978** | 14.83% | stage 0: two 153,600-byte staging clears |
| `armWaitForIrq` | 830,260 | 14.82% | **idle spin** — not work |
| `memcpy` | **557,126** | 9.95% | stage 4: 2 × 192 × 512 B into BG VRAM |
| `ndsRendererExecuteNativeFighterRootHardware` | 284,169 | 5.07% | the two 3D fighters |
| fighter DL family (3 symbols) | 229,178 | 4.09% | |

**The four compositor stages are 3,466,102 ticks/frame — 61.9% of the scene** — and every one of them
runs **twice**, once per layer, on content that does not change: a static wallpaper and seven static
glyphs. Only the fighters animate, and they are 3D on a different path. 397 of 3,390 FUNC symbols were
hit and unattributed cycles are **0.00%**, so this is a complete partition, not a top-N view.

**R0f was wrong about the clear, and the reason matters more than the error.** `-Phases` reported
`(layer begin) = 0.000 VBlanks` over 82 hits, and I concluded the staging clear was free. It is
830,978 ticks/frame — **1.48 VBlanks split across two sub-VBlank calls.** `sVBlankCount` is an integer,
so every interval that instrument reports is **floored, and the remainder lands in a later interval**.
Two 0.74-VBlank clears each read 0 and their cost was silently redistributed into the neighbouring row —
the wallpaper blit's. **That is the entire "~1.6M unexplained inside the wallpaper call" that R0f
published and R0g spent a build chasing.** The blitter's real cost is 1,103,616 ticks/frame for 72,882
pixels across both arms, and `--pc-detail` puts its pair loop at **1.72 cycles per instruction** — the
most ordinary number in this campaign. Nothing was ever wrong with it.

**So R0g's revert stands but its premise was false**, and the honest ordering is: the store fold measured
−0.06% because *the thing it was trying to explain did not exist*. R0e's 68.7% cut on the wallpaper is
still real — the profiler confirms the 33.8 → 16.45 → 5.15 VBlank progression was measuring a genuine
multi-VBlank interval, which is the resolution this instrument does have.

**Rule added to the census script itself, not just a doc:** a VBlank-quantised instrument is sound for
totals and for intervals of several VBlanks, and cannot attribute anything finer. R0f's table now
carries that caveat above the code that produces it.

**Queued as R2, and it is a structural change with a measured 3.47M-tick target:** the DS has hardware
BG layers and 128 hardware sprites, and this scene uses neither — it software-composites, clears,
downscales, and row-copies two full screens per frame for static content. The obvious cut is to stop
redoing the background layer every frame; sizing which of the two layers owns how much of the 3.47M is
the first step, and `--pc-detail` on `ndsPlatformCommitOriginalSpritePreviewLayer` plus the existing
`gNdsSObj{Background,Foreground}StagingClearBytes` counters can answer it without a new instrument.

### R2-07 R0g REVERTED — the store is NOT the residual (−0.06%), and the run that proved it also found a defect I had just introduced (2026-07-30)

**Wallpaper cost per call: 4.0000 → 3.9974 VBlanks. −0.06%.** Folding the pair's two `strh` into one
32-bit `str` halves the count of main-RAM halfword stores in the hot loop and **changes nothing**. So the
~1.6M ticks/frame this call costs beyond its instruction count are **not the store**, and R0f's
"the only per-pixel main-RAM write is one `strh`" reasoning, while true, was not the answer. Reverted:
it was one more instruction per pair (19 vs 18) and needed a runtime alignment gate to be safe, so at
equal measured cost the smaller code wins. The revert is **comment-only** against `55c8a2c` — the
graduated R0e ROM is still the valid artifact and needs no rebuild.

Both remaining models for the residual are now dead: **it is not instruction count** (R0e removed 103
instructions/pixel at 1.31 cycles each; the surviving 9 instructions would have to cost 7.8 cycles each)
and **it is not the store**. What is left inside that call is the source `ldrb`, the palette `ldrh`, and
the per-row/per-strip prologues — none of which look like 1.6M either. **This is now the case Task 37
exists for**, so R0d's queued idea comes back with a better justification than it had: the question is
no longer "how many instructions" (objdump answered that) but "which address inside this function
accumulates cycles", and the per-PC profiler is the only instrument that answers it. Queued as R0h,
which needs the header extended with a Results-scene window (`NDS_TASK37_PROFILE` currently keys its
markers off battle presented-frames; the Results loop never increments that counter, so it needs a
second tick site keyed on `sMNVSResultsTotalTimeTics` and a flag to disable the battle one).

**The defect: two GDB breakpoints at one address are not independent.** `-Phases` logged the iteration
boundary with a second breakpoint on `ndsMNVSResultsRecordFrame`, where breakpoint 3 already sat as the
window terminator. The logging breakpoint's `commands` block ends in `continue`, which **swallowed
breakpoint 3's stop**, so the window never closed: the run went to its 1800 s timeout having covered
**6,169 iterations instead of 40**, straight past the Results screen into sprite kinds no other arm has
ever seen (`IA/8b 62x13`, `CI/4b 8x10`, `RGBA/16b 42x35`, 36,374 hits on `IA/8b 8x10`). The script's own
comment above breakpoint 3 had already warned about exactly this — *"2 and 3 are at different addresses,
so 2's `continue` cannot swallow 3's stop"* — and I broke the invariant it was documenting. Fixed by
moving the frame boundary to `ndsPlatformReadInput`, which is once per frame at its own address.

*The one thing the broken window bought:* the per-call wallpaper figure is window-independent (same
sprite, same 66,000 pixels, same destination every call), so 6,169 samples at 3.9974 against 41 at
4.0000 is a **stronger** wash verdict than the intended run would have given. That is luck, not method.

### R2-07 R0f MEASURED — the layer CLEAR is free, the layer COMMIT is 43.1%, and the wallpaper call still hides 1.6M (2026-07-30)

`census-vsresults-blit.ps1 -Phases` on the R0e ROM (`33A9E063`), same window. 491 costed intervals,
406 VBlanks over 40 iterations = **10.15 VB/iteration**, agreeing with R0e's window figure of 10.250.

| interval | hits | VB/iter | ticks/frame | share |
| --- | --- | --- | --- | --- |
| `(layer commit)` | 81 | **4.375** | 2,450,831 | **43.1%** |
| I/4b 300×220 wallpaper | 41 | **4.100** | 2,296,779 | **40.4%** |
| seven IA/8b glyphs | 287 | 1.675 | 938,318 | 16.5% |
| `(layer begin)` | 82 | **0.000** | 0 | **0.0%** |

**Two hypotheses died here, both of them mine.**

1. **The 153,600-byte staging clear is NOT a cost.** `ndsPlatformBeginOriginalSpritePreview` was hit 82
   times and accumulated **zero** VBlanks. It is `memset` per row over 240 rows and it is below the
   measurement floor. Every earlier note that paired "commit plus the next Begin's clear" as the owner
   was half wrong; only the commit costs anything. Delete the clear from the candidate list.
2. **R0's "22 commits in 101 iterations, zero foreground" does not describe this window.** Here it is
   **81 commits in 40 iterations** — two per iteration, both layers, every frame. Not a contradiction:
   `sMNVSResultsDrawWallpaperTic` is 80, so R0's window straddled tics where the wallpaper GObj did not
   exist yet. **A counter read at one point in a scene that builds up over 180 tics is not a property of
   the scene.** That is why this run re-derived it inside the same window as the cost.

**What is still unowned: ~1.6M ticks/frame INSIDE the wallpaper's own call.** 2,296,779 ticks for 66,000
pixels is 34.8 ticks/pixel ≈ **70 ARM9 cycles**, against a loop body of **9 instructions**. R0e removed
~103 instructions/pixel and 135 cycles/pixel — 1.31 cycles per instruction removed, which is sane — so
the model is right about what was removed and wrong about what is left. Nine instructions cannot cost 70
cycles. The only per-pixel main-RAM traffic left is **one halfword store**, so **R0g** folds the pair's
two `strh` into one `str`: the base is always 4-byte aligned (`preview_pitch` 320, `origin_x` 10, so
`(y·pitch + origin_x)·2 ≡ 0 mod 4` for every row) and each pair advances exactly 4 bytes, so the bytes
written are identical. It emits **19 instructions per pair against R0e's 18**, which makes it a clean
discriminator: instruction count predicts a small loss, store count predicts a large win.

**Caveat on `(layer commit)`, stated before anyone builds against it:** that interval ends at the next
layer *begin*, so it also contains the scene's two 3D fighter GObjs — they draw at display link 9, ahead
of the wallpaper's link 26 — plus the scene update and present. `-Phases` now also logs the iteration
boundary to split it. **Do not read 43.1% as the cost of committing a layer.**

### R2-07 R0e BUILT — the Results wallpaper loses 68.7%; Results is 3.9× faster than R0 and the layer pipeline is now the owner (2026-07-30)

**21.525 → 10.250 VBlanks/iteration, −11.275 (−6,316,143 ticks/frame, −52.4%).** Cumulative from R0:
**39.975 → 10.250, −74.4%**, 22,393,595 → **5,741,947 ticks/frame**, 1.50 → **5.85 FPS, a 3.9×
speedup.** ROM `33A9E063`, `artifacts/performance/r207-r0e-rowlut.json`, same window, instrument, and
`-SkipIterations 130 -Iterations 40` as R0c/R0d — every row below is one measurement of the same forty
iterations, so the arms are directly comparable:

| arm | VB/iter | ticks/frame | FPS | wallpaper VB/iter | wallpaper share |
| --- | --- | --- | --- | --- | --- |
| R0 baseline | 39.975 | 22,393,595 | 1.50 | 33.83 | 84.8% |
| R0c reciprocal multiply | 22.550 | 12,632,284 | 2.66 | — | — |
| R0d `always_inline` lerp | 21.525 | 12,058,089 | 2.79 | 16.45 | 76.8% |
| **R0e paired row + palette** | **10.250** | **5,741,947** | **5.85** | **5.15** | **50.7%** |

**The change is a specialized row, and it is BIT-EXACT by proof rather than by screenshot.** Under the
prim/env combine the output colour is a pure function of the 4-bit intensity, so sixteen values cover
every pixel the generic loop can emit; the palette is built once per call from the same
`ndsSpriteLerpPrimEnv` expression. The row then reads **one source byte per PAIR of destination
columns**, because a 4-bit row packs both nibbles of a pair in one byte and the low nibble is always the
odd column. That pairing survives `SP_TEXSHUF`: the odd-row swizzle is `source_x ^= 8`, which cannot
touch bit 0, so it reduces to `^ 4` on the byte index. Both halves are checked by
`scripts/check_sprite_lerp_exact.py` (already wired into `check-gbi-decode-fixtures.ps1`) — the index
algebra exhaustively over **every width 1..320 × both row parities**, including the odd-width tail whose
last column is even and therefore takes the HIGH nibble of byte `pairs`.

**Emitted body is 18 Thumb instructions per 2 pixels, register-resident** (`2037ae0..2037b06`, one
`ldrb`, two `ldrh`, two `strh`, one `ldr [sp]` per pair) against the generic loop's ~112 per pixel.
`ndsDrawSObjIntoPreview` grows 3,796 → 4,152 bytes (**+356**) and the ROM is unchanged at 11,514,880.
Every other caller of this blitter is untouched: `fast_i4` is NULL unless
`results_wallpaper_combine != 0 && record_startup == 0 && !is_scaled && bmfmt == I && bmsiz == 4b &&
origin_x >= 0`, and the horizontal extent is re-checked per strip so the specialized row only ever runs
where it writes exactly the pixels the generic loop would have.

**R0e is also what makes the remaining cost legible, and it is NOT the pixel loops.** Sizing both
arms from the emitted code at ~2.4 cycles/instruction: the I4 wallpaper is 66,000 px × 9 instructions
≈ 0.71M ticks, and the seven IA/8b glyphs are 186 × 37 = **6,882 px** × ~90 ≈ 0.74M — together about
**1.5M of the 5.74M**. Even deleting both loops outright would leave ~4.2M against a 1.12M gate. **So
do not open the per-pixel loops again**
(the IA/8b arm would take the same 16-entry palette — its colour is `lerp(sobj, (ia>>4)*17)`, a pure
function of one nibble — but that is worth at most ~0.6M and should wait its turn).

**Where the other ~4.2M goes is NOT yet measured, and I am deliberately not naming a cause.** The
obvious suspect is the two-layer 320×240 software pipeline, but **R0's own numbers argue against the
simple version of that story**: `gNdsOriginalSpritePreviewCommitCount` advanced only **22 in 101
iterations**, with **zero foreground commits** in that window — so the commit path cannot be a
per-frame cost, and the frame also draws two 3D fighter GObjs that nothing here has priced. Naming the
layer boundary from source reading is the exact move that produced four refutations in R2-06 and two
withdrawn claims in R0d.

**The instrument now exists: `census-vsresults-blit.ps1 -Phases`** adds breakpoints on
`ndsPlatformBeginOriginalSpritePreview` and `ndsPlatformCommitOriginalSpritePreviewLayer` and charges
each interval to itself instead of to whichever blit precedes it — the analysis was rewritten around a
single ordered event stream, so without the switch it still reduces exactly to the blit-to-blit deltas
the R0/R0c/R0d/R0e artifacts were measured with. Run that first. If the residual is still unowned after
it, the next boundary to add is the fighter draw, not another guess.

### R2-07 R0d BUILT — inlining the per-pixel lerp takes another VBlank; Results is now 46.2% cheaper than R0 (2026-07-30)

**22.00 → 21.00 VBlanks/iteration, −1.00 (−560,190 ticks/frame).** Cumulative with R0c:
**39.00 → 21.00, −46.2%**, 21,847,410 → **11,763,990 ticks/frame**, 1.53 → **2.85 FPS**. ROM
`3410D098`, `artifacts/performance/r207-r0d-inline.json`, same window and instrument as R0c.

`ndsSpriteLerpPrimEnv` is now `static inline __attribute__((always_inline))`. **`-Os` would not inline
it on its own** — R0c's ELF still had two real `bl` sites into a `.isra.0` clone, and the callee
pushed and popped **eight** registers (r4-r7 plus r8/r9/sl/lr) around three multiply-shift channels.
**Verified in the ELF: the symbol is GONE entirely and there are zero `bl` sites**, for +156 bytes on
the blitter (3,640 → 3,796), about the one extra copy two call sites predict.

**On whether −1.00 VBlank is real, because it sits exactly at this instrument's resolution:** the
window went **902 → 861 VBlanks over 41 iterations** — a dead-consistent integer step, which
quantisation noise would not produce (it would land on a fraction). The per-call figure moves
sub-VBlank in agreement, **16.85 → 16.05 VB/call** on the wallpaper. Both support it; it is a small
real win, not a rounding artifact, and it is reported as −4.5% rather than dressed up.

*Kept rather than reverted on the E11 standard: the placement-floor argument that killed E11 and E15 is
about **P95 on the battle gate**, where relinking moves the tail by more than a small saving. This is a
different scene with no such tail (the Results loop is unpaced, and 41 of 41 iterations agree), and the
change costs 156 bytes with no fidelity question at all — inlining is semantically inert.*

**This is the opposite choice from E65's on `ndsR2CubicValueFixed`, and the distinction is worth
keeping straight:** that one is deliberately `noinline` to hold ONE copy of its six inlined conversions
inside `.text.hot`'s curated 8 KiB. This blitter is not in `.text.hot`, so the constraint does not
apply. Both comments now say so at their own site.

**Remaining: ~11.76M ticks/frame against 1.12M, still 10.5× over.** The wallpaper is still 16.05 of
21.00 VBlanks, i.e. **~8.99M ticks for 66,000 source pixels ≈ 136 ticks/pixel ≈ 272 ARM9 cycles.**
R0d's first draft said the visible loop body explains only ~40 of those and that the residual was
unattributed. **Both halves of that were wrong, and the way to find out was to stop reading C and
disassemble the ELF** — `objdump -d -l` on `ndsDrawSObjIntoPreview`, whose path for one I4 pixel is:

| segment | Thumb instructions | note |
| --- | --- | --- |
| loop tail, `dst_x_q16 += scale`, `dst_x_start` | ~14 | every loop-carried value is a `ldr [sp,#N]` |
| seven-way format chain | **16** | `2037b44→2037a94→2037aa0→2037aa4→2037e90`, 3 taken branches |
| I4 unpack | ~21 | texshuf test, `^3` swizzle, `ldrb`, nibble select |
| inlined prim/env lerp | ~45 | 3 channels × (2 `ldrb`, 2 `muls`, add, `×257>>16`) + pack |
| texshuf/`record_startup` tests, `color != 0`, bounds, `strh`, `drawn_pixels++` | ~16 | |
| **total** | **~112** | ~28 of them memory accesses, ~6 taken branches |

At ~2.4 cycles per instruction — Thumb `-Os` with a load-use interlock on nearly every spill — that is
~270 cycles. **The measurement was never mysterious: the cost IS instruction count, in a generic
per-pixel loop, and the C hid it because `-Os` spills the entire loop body to a 272-byte frame.**

**Also correcting R0d's draft on a fact, not just an estimate: the wallpaper is NOT scaled.**
`mnVSResultsMakeWallpaper` (BattleShip `mnvsresults.c:694`) sets only `pos = (10,10)` and the prim/env
colours — it never touches `scalex`/`scaley` — so 300×220 lands 1:1 inside 320×240 with a 10-pixel
border and `is_scaled` is FALSE. The rect-fill arm never runs. The census JSON also pins the rest of
the shape: `nbitmaps = 9`, `attr = 0x240` = `SP_TEXSHUF | SP_OVERLAP`, and **no `SP_FASTCOPY`**.

**Task 37 was therefore not needed, and the lesson is cheaper than the run would have been:** when a
tick count disagrees with a reading of the C by ~3×, disassemble before instrumenting. The emitted code
is free to look at, needs no emulator, and here it answered in one command what a per-PC profile of the
Results scene would have cost a full match-length run to say. Recorded in
`docs/optimization/TASK_STANDING_RULES.md`.

The structural answers stay queued behind R0e — admitting the native OAM path to Results
(`sprite_preview_backend.c:2410`) and/or killing the two-layer 320×240 pipeline with its 153,600-byte
clear per layer.

### R2-07 R0c BUILT — the Results frame is 43.6% cheaper, BIT-EXACT, and it was three library divisions per pixel (2026-07-30)

**39.00 → 22.00 VBlanks per Results iteration, −17.00 (−43.6%).** At 560,190 ticks/VBlank that is
**21,847,410 → 12,324,180, i.e. −9,523,230 ticks per frame**, and 1.53 → **2.72 FPS**. Measured by
`scripts/census-vsresults-blit.ps1` (new), matched control and candidate over the same window
(results tics 131..171, 41 iterations), ROMs `9B601111` → `DD9D59BE`, artifacts
`r207-r0c-blit-split.json` and `r207-r0c-div-candidate.json`.

**R0a's hypothesis was right and the per-call split proved it before anything was edited.** Eight
blits per iteration: one **I/4b 300×220** wallpaper — 66,000 px, **90.6% of the pixels** — plus seven
IA/8b text glyphs of 333–1,443 px. The wallpaper was **33.00 of the 39.00 VBlanks (84.8%)**, dead
consistent across all 41 calls. **So the I4 arm alone was the target and the other six format arms
were noise, exactly as R0a suspected.** *The window also independently reproduced R0's 39.0
VBlanks/iteration to the second decimal, which validates the method before it is used for a delta.*

**The cause was not the seven-way dispatch chain R0a expected. It was `/ 255u`.**
`ndsSpriteLerpPrimEnv` runs once per blitted pixel and did three `/ 255u`, and **at `-Os` GCC does not
turn a compile-time constant divisor into a reciprocal multiply — it emits `blx __udivsi3`,** because
the call is smaller than the multiply-shift sequence. On a core with no divide instruction, at 66,000
iterations, that size-for-speed trade is catastrophic. The fix is
`NDS_SPRITE_DIV255(x) = (x * 257 + 257) >> 16`.

**It is bit-exact, and the bound that makes it exact is the non-obvious part.** `intensity` and
`inverse` are **complementary** (they sum to 255), so the numerator cannot reach 2·255² — its true
maximum is **255² + 127 = 65,152**, inside the range where the identity holds. Above 65,535 it starts
returning values one too high. `scripts/check_sprite_lerp_exact.py` proves all of it exhaustively —
`div255` over every reachable numerator, the whole channel expression over all 16.7M
(colour, env, intensity) triples, *and* that the failure cliff sits above the reachable range so the
complementarity argument is load-bearing rather than incidental. **It fails if anyone widens
`intensity` past `u8` or makes `inverse` independent of it.**

**Verified in the ELF, not assumed:** `ndsSpriteLerpPrimEnv` went **118 → 100 bytes with
`__udivsi3` ×3 → ×0**.

**Two honest corrections to my own change, both smaller than the headline:**
- **The `(nibble * 255u) / 15u → nibble * 17u` edits removed ZERO library calls.** 255/15 = 17 exactly
  so they are bit-exact and the intent is now explicit, but GCC had already strength-reduced those —
  the blitter's `__udivsi3` count is **2 before and 2 after**. The win is the lerp's three, not four.
- **Those two remaining `__udivsi3` are not on this path at all.** Both are
  `ndsSObjWallpaperLastSource` (`:619`) dividing by a runtime `scale_q16`, which is genuinely
  irreducible — and it lives in the Dream Land wallpaper cache that R0a proved *rejects* the Results
  wallpaper, so on Results they never execute.

**Still 12.3M ticks/frame against a 1.12M budget, so R2-07's Results clause is not met** — this is one
lever on an 11× overrun, not a fix. What it does establish is where the rest is: with three divisions
per pixel gone the wallpaper is still 16.85 VBlanks/call, so **~9.4M ticks/frame is the remaining
per-pixel work on 66,000 pixels** — the per-pixel `bl ndsSpriteLerpPrimEnv` call with its 8-register
push/pop still stands (both call sites confirmed present at `2037d58`/`2037f08`), and R0a's dispatch
hoist is still unbuilt. **Next: inline the lerp and specialize the I4 loop; the OAM-path and two-layer
questions remain untouched and are still the structural answer.**

*Generalises beyond this scene: `-Os` emitting `__udivsi3` for constant divisors is a whole-repo
hazard, not a Results one. Any `/` by a constant inside a per-pixel, per-vertex or per-joint loop is
suspect, and the check is one `objdump | grep __udivsi3` over the function.*

### R0a — one candidate REFUTED by reading, and a third that outranks both (2026-07-29)

**"Also ungate the wallpaper cache for Results" is REFUTED, no build needed.** I had
described the Results fix as a gating change on two flags — the OAM path at
`sprite_preview_backend.c:2410` and `cache_wallpaper` at `:2391`. The second is
wrong. `cache_wallpaper` feeds `ndsSObjDrawCachedWallpaper`, whose gate
(`ndsSObjGetOpaqueWallpaperCache`, `:680`) is a **Dream Land specialization**: it
requires `asset_id == NDS_RELOC_ASSET_STAGE_DREAM_LAND`, exactly 300×220, 44
bitmaps, `bmheight 5`/`bmHreal 6`, `G_IM_FMT_RGBA`/`G_IM_SIZ_16b`, and a fully
opaque source. The Results wallpaper is a different asset and reaches the blitter
through the **I4** branch with `results_wallpaper_combine` set. Every one of those
checks would reject it, so ungating `cache_wallpaper` on Results buys a failed probe
and one `gNdsSObjWallpaperCacheFallbackCount++` per frame. Results needs its own
path, not this one.

**A third candidate, and it is cheaper and wider than either of the first two.**
`ndsDrawSObjIntoPreview`'s innermost per-pixel loop (`:1739-1935`) re-evaluates a
seven-way `sprite->bmfmt`/`bmsiz` dispatch chain **per pixel**, when the format is
invariant for the whole sprite — the I4 wallpaper pays six failed comparisons on
every one of its pixels before reaching its own branch. Separately, the `CI` arm
(`:1676-1714`) walks all of `src_bytes` **every frame** purely to recompute
`ci_max_index` for a palette-range validation, on immutable asset data. Hoisting the
dispatch into a per-format specialized loop and lifting the palette validation out of
the per-frame path are mechanical and behaviour-preserving, need no decision about
OAM or layers, and pay back in every scene that uses this blitter — Results, title,
opening portraits, opening movie.

**Do not start there blind, though.** The same caution that applies to the first two
applies here: 8.3 calls/frame against 19,413,481 ticks is 2.34M per call *on
average*, and the split across those calls is unknown. One full-screen wallpaper
plausibly dominates, in which case the I4 arm alone is the target and the other six
format arms are noise. Bracket per call first.

**R0b — the battle-frame safety question is now ANSWERED, and the answer clears the change
(2026-07-30).** R0a said to check whether `ndsDrawSObjIntoPreview` is live on the battle frame before
editing it, because the gate had little margin. **It is not live — it executes zero instructions
there**, so specializing it *cannot* regress the battle gate. Established without a build, at three
levels of strength:

- It is `static` (`sprite_preview_backend.c:1478`) and so absent from the E10 census's 846-symbol
  table — but that table lists only *sampled* symbols out of the ELF's **12,828**, and 29 of the 846
  read zero, so absence there is not by itself evidence.
- `nm` confirms the symbol is genuinely **in** the census's own ELF, local, at `0x0203774c`
  (`builds/build-task37-profile/…tickhud-hwtri.elf`) — so it was a candidate for sampling and was not
  merged away.
- **The raw 176 MB per-PC profile has ZERO rows in `[0x0203774c, 0x0203974c)`** — a generous 8 KiB
  window covering the whole function — across all 128 frames. Not one instruction retired.

So the battle path really does take the OAM route end to end: `ndsIFCommonNativeOamBeginFrame` 2,153,
`ndsIFCommonNativeOamCommit` 394, `ndsPlatformFastWallpaperQueueTransform` 2,629,
`grWallpaperCalcPersp` 2,057, `ndsDrawLayeredSObjFrame` 863 — **9,550 ticks/frame for the whole
sprite/wallpaper family**, with the blitter itself never entered. *Parts* of
`sprite_preview_backend.c` are live on the battle frame; the per-pixel blitter is not.
**`gNdsSObjWallpaperCacheFallbackCount` does not contradict this** — it counts a different function
(`ndsSObjDrawCachedWallpaper`'s probe), not this one.

*Method worth reusing: "does this code run in configuration X" is answerable from a per-PC profile by
address range alone, with no symbol table, no rebuild, and no reliance on a name — which sidesteps the
recurring trap that `addr2line` and symbol tables name deleted and inlined functions.*

**R0b also confirms R0a's CI-scan claim and sharpens it in two ways (read-only).** The scan is real:
`:1682-1700` walks **every byte** of `src_bytes` to compute `ci_max_index`, and its *only* downstream
consumer is the palette bounds check at `:1701-1706` and the single boolean `ci_palette_ready` it sets
(`:1806`, `:1829`). **An O(src_bytes) scan that yields one bit, on immutable asset data.** Two
corrections to how R0a scoped it:

- **It is per-BITMAP, not per-frame.** `ci_max_index` is declared at `:1620`, *inside* the
  `for (bitmap_index = 0; …)` loop opened at `:1603`, and zeroed each iteration — so a sprite with
  `nbitmaps = n` pays *n* full scans per call per frame. For scale, the Dream Land wallpaper the cache
  path recognises carries **44** bitmaps.
- **But it is NOT on Results' dominant path.** The gate is `sprite->bmfmt == G_IM_FMT_CI`, and the
  Results wallpaper reaches the blitter through the **I4** branch (R0a). So this cut cannot be part of
  the Results fix — it pays in the CI scenes (title, opening portraits, and whichever Results sprites
  are CI rather than I4). **Do not bundle it with the Results work and do not credit it against
  R2-07's gate.**

The fix shape follows `PROJECT_GOAL.md`'s "compute once, not every frame": the max index is a property
of immutable asset bytes, so it belongs computed at load/relocation time and stored, not rederived per
bitmap per frame. Do **not** instead validate against the format maximum (15 for 4b, 255 for 8b) — that
is conservative in the wrong direction and would reject sprites whose palette is legitimately shorter
than the format allows, which is a behaviour change rather than an optimisation.

## R2-03 E63 SIZED — a flash-colour table is 2,164 bytes of ROM, and the `.rgba` field is confirmed normals (2026-07-29)

Static only; no build, no emulator. Two facts the next attempt should not re-derive.

**1. `sNdsNativeFighterDenseVertices[].rgba` holds packed NORMALS, confirmed
numerically.** E58/E59/E62 said so; here is the arithmetic, because the field name
argues the opposite and that is how E48–E55 went wrong. First entry `0x3157b1ff` →
`(0x31, 0x57, 0xb1)` as s8 is `(49, 87, −79)`, magnitude
√(49²+87²+79²) = **127.6 ≈ 127** — a unit normal in F3DEX's s8 encoding, with the
trailing `0xff` the alpha byte F3DEX keeps alongside. Every entry ends `ff`. So
**E49's `NDS_R2_UNLIT_VERTEX_EPOCH` could never have worked**, and its rainbow
speckle was normals interpreted as colour, exactly as E62 measured.

**2. The table a fix needs is small.** 541 dense vertices × `u32` = **2,164 bytes**,
and it is `static const`, so it lands in **ROM, not BSS**. That matters because the
footprint trap that killed E64 arm A (10,240 bytes of BSS plus `.text.hot` growth)
does not apply to a `const` table, and `PROJECT_GOAL.md` trades ROM for speed
freely. Cost is not the obstacle here.

**What is still unknown, and it decides between two very different fixes:** whether
the flash is **one colour across the fighter** or per-vertex. If uniform, no table is
needed at all — one `GFX_COLOR` per flash epoch plus clearing the lighting bit. If
per-vertex, the 2,164-byte table is required. E48's probe slots 8..11 were built to
answer precisely this (slot 11 == 0 means uniform), and they read zero because E59
then proved the generic software lighting never runs — so the instrument is pointed
at a path that does not execute. **E63 proper needs one probe on the path that does
execute (`ndsRendererNativeShadeProductionActions`), not another generic-path
sample.** Size the fix after that answer, not before.

## R2-03 E69 GRADUATED — the matrix copies were `bl memcpy`. P95 **1,096,768**, over gate 7/128 → 6/128, margin 23,232 (2026-07-29)

`artifacts/performance/r203-e69{,b}-mtxcopy-128{.json,-rows.csv}`. Sixteen sites,
no new state, no flag.

**E68b said the class was matrix moves, and Task 86 had already built the fix.**
`ndsRendererMatrixCopy20p12` has been in `nds_renderer.h` since Task 86 — sixteen
explicit element assignments, with a comment explaining that `*dst = *src` on a
64-byte matrix becomes `bl memcpy` because GCC will not open-code sixteen words it
cannot prove aligned. **It had two call sites.** Every other matrix move in the
adapter was still a plain struct assignment, including all four of E68b's top
`memcpy` callers.

E69 routes them through it, and adds the clear half —
`ndsRendererMatrixIdentity20p12(dst, one)` — because both identity builders were
`memset(out, 0, 64)` plus a four-iteration diagonal loop, i.e. a library call and a
loop to write twelve zeros. `one` is a parameter only because the adapter and the
renderer use different macro names for the same fixed-point scale.

| `WORK-H` | E67 | arm 1 (13 sites) | **arm 2 (+3 stage)** | delta vs E67 |
|---|---:|---:|---:|---:|
| P50 | 974,656 | 967,488 | **966,848** | **−7,808** |
| P75 | 1,000,512 | 994,816 | 994,304 | −6,208 |
| P90 | 1,070,656 | 1,061,120 | 1,060,160 | −10,496 |
| **P95** | 1,109,312 | 1,101,760 | **1,096,768** | **−12,544** |
| P99 | 1,195,456 | 1,200,576 | 1,194,816 | −640 |
| max | 1,493,632 | 1,484,992 | 1,485,184 | −8,448 |
| **over gate** | 7/128 | 7/128 | **6/128** | **−1** |
| `FTR` P50 | 390,080 | 384,512 | 384,256 | −5,824 |

**Paired by frame, better on 94 of 128, median −7,232.** The win lands in `FTR`,
which is where `BuildDObjLocalMatrix` and `BuildDObjWorldMatrix` run — the
mechanism and the bucket agree, which is the check E66 failed.

**Engagement is structural rather than a counter:** `objdump` shows
`ndsRendererAdapterBuildDObjWorldMatrix` at **zero** `memset`/`memcpy` calls (it was
E68b's 9.4% row), `BuildDObjLocalMatrix` 3, `BuildPersistentStageWorldMatrix` 5 → 2.
Arm 2's three extra conversions were worth a −1,088 median on 85/128 on their own,
so the remaining sites are still worth something but individually below the floor.

**Where the gate stands after E32 + E64b + E65 + E67 + E69:** P95 **1,228,928 →
1,096,768**, cumulative **−132,160**, over gate **17/128 → 6/128**. Margin to
1,120,000 is **23,232** — three times the placement noise floor, and the first
figure in this campaign that leaves room for R2-07 to spend.

**What is left of this lever.** 402 static `memset`/`memcpy` sites remain
program-wide. `nds_renderer.c` still has ~12 matrix struct assignments
(`*hardware = *composed`, `state->modelview = *input->modelview_matrix`, …) and
E68b put `ndsRendererLoadHardwareMatrices` at 5.7% and `MtxLoadN64ToDS20p12` at
1.3%. That file builds `-marm`, where GCC has more registers for LDM/STM and may
already be inlining, so **re-measure before assuming the same win applies there.**

## R2-03 E68b ANSWERED — `memset`/`memcpy` is 58,700 ticks/frame; the top three callers are 53% of it. E68 withdrawn (2026-07-29)

`artifacts/performance/r203-e68b-memcall-callers-nm.json`. Two 90 s GDB runs, no
build. Task 37 prices the class independently of this census: **`memset` 30,520
ticks/frame (235 calls, 130 each) + `memcpy` 28,180 (311 calls, 90 each) =
58,700**, the largest addressable class after soft float, and pure data movement —
exactly what §9 of the switch plan says to avoid rather than optimise.

**25,648 attributed samples, 92.1% renderer-side, and 0% now charged to a name
absent from the ELF:**

| caller (sites merged) | share | memset | memcpy |
|---|---:|---:|---:|
| `ndsFighterMarioFoxDLAllDrawForSlot.constprop.0` | **18.8%** | 3,251 | 1,579 |
| `ndsRendererAdapterBuildDObjLocalMatrix` | **18.6%** | — | 4,772 |
| `ndsRendererAdapterBuildPersistentStageWorldMatrix` | **15.8%** | 785 | 3,262 |
| `ndsRendererAdapterBuildDObjWorldMatrix` | 9.4% | — | 2,324 |
| `ndsRendererAdapterBuildNativeMaterialSnapshot` | 6.2% | 1,582 | — |
| `ndsFighterDisplayContractSelectDL` | 5.8% | — | 1,495 |
| `ndsRendererLoadHardwareMatrices` | 5.7% | 1,464 | — |
| `ndsRendererAdapterCaptureStageWorldSourceKey` | 2.7% | — | 690 |
| `ndsMPCollisionEnsureLineGroups` | 2.6% | 676 | — |
| `battleship_ftAnimParseDObjFigatree` | 1.8% | 451 | — |

**The top three are 53.2% of the class, about 31,200 ticks/frame.** The `memcpy`
half is 64-byte `NDS_RENDERER_MATRIX_20P12` structs (`s32 m[4][4]`): GCC will not
inline a 16-word copy at `-O2 -mthumb`, so even a plain `*dst = *src` struct
assignment becomes a real `bl memcpy`.

**Where the fighter draw's 3,251 memsets come from, and it is the most promising
lever in the table.** `ndsFighterMarioFoxDLAllDrawForSlot` has 30 static
`memset`/`memcpy` sites, but only three are the `bzero`s behind
`detailed_output`, which is false in the Boundary configuration. The rest arrive
through inlining, and two calls at `reloc_backend_renderer_dl.c:12217-12218` run
**unconditionally**, twice a frame:

```c
ndsRendererInitVertexCache(&persistent_renderer_vertices);
ndsRendererInitStats(&persistent_stats);
```

The file's own comment calls that vertex cache "traversal-owned but too large for
BattleShip's nested task stack" — i.e. it is big, it is cleared per fighter per
frame, and E13 already established the shipping software-preview callback is null.
**Price what the clear actually initialises before clearing all of it** (Task 104's
emblem: a 1,292-byte clear plus copy transporting one live 4-byte field).

**E69, and note which sizing rule applies.** This is *work removal*, not placement,
so E67's precedent governs rather than E66's. Two independent changes:

1. A shared inline 16-word matrix copy/clear at every site that moves an
   `NDS_RENDERER_MATRIX_20P12` — `BuildDObjLocalMatrix`,
   `BuildPersistentStageWorldMatrix`, `BuildDObjWorldMatrix`, both
   `MtxIdentity20p12` (`reloc_backend_renderer_dl.c:330`, `nds_renderer.c:5272`,
   each `memset(out, 0, 64)` plus four diagonal stores — a library call and a loop
   to write twelve zeros).
2. Narrow or skip the unconditional vertex-cache/stats clear in the fighter draw.

Sized together at ~14,000-20,000 ticks/frame if half the class is call overhead.
It touches several functions in a 12,000-line file, so have the paired-by-frame
comparison ready before starting.

### E68 is WITHDRAWN, and the reason is a harness defect worth more than the result

**E68's first run charged 47.3% of its samples to function names that are not in
the binary,** because `census-softfloat-callers.ps1` resolved return addresses with
`addr2line` alone and DWARF still describes functions GCC inlined away. Both
published conclusions were artifacts:

- *"`ndsRendererAdapterDObjWorldIndexHash` memcpys, 9.0% — a hash should read its
  input, not copy it."* **Wrong.** That function is three shifts and a mask
  (`reloc_backend_renderer_dl.c:1717`), contains no `memcpy`, and is not a symbol
  at all. Its samples belong to `ndsRendererAdapterBuildDObjWorldMatrix`.
- *"`ndsFighterMarioFoxDLAllDrawForSlot` does not appear in the dynamic
  attribution at all — a call site is not a call."* **Wrong, and backwards.** It is
  the single largest caller at 18.8%. The static site count had been pointing at
  the right function; the measurement that appeared to refute it was the broken one.

The fix: the `nm -S` text symbol table is now the authority for the name,
`addr2line` keeps only the source path (which `Get-Gate` keys on, and which DWARF
gets right even for inlined code), and the run prints its rename count — 25 of 65
addresses on the re-run, 0% unresolved afterwards. **This is the third census to
lose to the same call**, after Task 37 (32% of PCs renamed) and Task 84 (82% of
samples resolved into BSS). Both Python censuses already carried the override *with
a comment explaining it*; this PowerShell harness never got it, because nobody
grepped across languages. `TASK_STANDING_RULES.md` now states the rule and the
fleet-wide check.

The class total (58,700, from the Task 37 profile) and the renderer-side split
(gated by source path) were never affected.

## R2-03 E67 GRADUATED — the port was doing DOUBLE-precision degrees→radians. P95 1,109,312, margin now 10,688 (2026-07-29)

`artifacts/performance/r203-e67-floatdtor-128{.json,-rows.csv}`. One header line
changed; `include/macros.h`.

**This is a specification fix that happens to be faster, not a trade.** The port's
degrees→radians macros read

```c
#define F_CST_DTOR32(x) ((f32)((x) * (M_PI / 180.0)))
#define F_CLC_DTOR32(x) ((f32)(((x) * M_PI) / 180.0))
```

`M_PI` and `180.0` are both `double`, so the argument was **promoted** and every
runtime call compiled to four library calls — `__aeabi_f2d`, `__aeabi_dmul`,
`__aeabi_ddiv`, `__aeabi_d2f`. BattleShip's own `macros.h` writes these with
`DTOR32` (a `float`) and `180.0F`. The port's versions now match it character for
character, so **the folded constants are now BattleShip's values** where before
they could differ by a ULP. The ARM9 has no FPU and its double helpers are far
worse than its float ones: the Task 37 census measured `__aeabi_ddiv` at **349
ticks per call** against `__aeabi_fdiv`'s 53.

| `WORK-H` | E65 | **E67** | delta |
|---|---:|---:|---:|
| P50 | 982,848 | **974,656** | **−8,192** |
| P75 | 1,006,208 | 1,000,512 | −5,696 |
| P90 | 1,070,592 | 1,070,656 | +64 |
| **P95** | 1,113,984 | **1,109,312** | **−4,672** |
| P99 | 1,212,864 | 1,195,456 | −17,408 |
| over gate | 7/128 | 7/128 | 0 |

**Paired by frame, better on 91 of 128, median −6,976, mean −6,018** — and the
prediction from the census was ~6,800, so this is one of the few levers this
campaign has sized correctly before building it. Every bucket moved down a little
(`FTR` −1,664, `STG` −1,472, `SRC` −3,520, `MISC` −640), which is what a macro used
across gameplay, camera and effects should look like.

**The margin is what matters here.** 1,120,000 − 1,109,312 = **10,688**, against
E65's 6,016. The gate reading is now *above* the 5,000–7,000 placement floor rather
than inside it, so it survives a relink. That, not the tick count, is E67's value.

**How it was found, since the method transfers.** Not a profile — a static census
of the disassembly for double-precision helper call sites, grouped by function.
Sixteen functions used them and twelve shared one signature, `f2d → dmul → ddiv →
d2f`, which is degrees-to-radians written with unsuffixed literals. **Any use of
`double` on this target is a defect until proven otherwise**, and it is greppable
from the ELF without an emulator run:

```bash
arm-none-eabi-objdump -d BUILD/x.elf | grep -cE "bl.*__aeabi_(d|f2d)"
```

Only ~14 calls a frame reach it, which is why no profile ever named it: the cost is
in the per-call price, not the frequency. `_dtoa_r`/`_svfprintf_r` still reference
the double helpers, so they stay linked; nothing else in the battle path does.

Residual double users, all decomp source and all small: `ftComputerProcWalk` and
`func_ovl3_8013877C` carry an `__adddf3`/`dsub` pair from an unsuffixed literal in
their own bodies. Left alone — `decomp/` is read-only and the whole remaining
double population is under 14 calls/frame.

## R2-03 E66 REFUTED — the census ranks `.text.hot` candidates it cannot predict. Second wrong-sign estimator (2026-07-29)

`artifacts/performance/r203-e66-cubic-hottext-128{.json,-rows.csv}`. Reverted;
the linker comment now carries the result so it cannot be retried.

E65 split `ndsR2CubicValueFixed` into its own ARM function, which shrank
`gcPlayDObjAnimJoint` 2,096 → 272 bytes and left **3,944 bytes free** in
`.text.hot`. The Task 37 census then ranked that same 2,032-byte callee the **#1
unplaced candidate** for the space — 1,815,752 recoverable non-mem stall cycles,
about **7,093 ticks/frame** — marked `fit`. It was admitted immediately after its
only caller, which is the most favourable placement available.

| `WORK-H` | E65 | E66 | delta |
|---|---:|---:|---:|
| P50 | 982,848 | 983,744 | +896 |
| P75 | 1,006,208 | 1,009,088 | +2,880 |
| P90 | 1,070,592 | 1,078,592 | +8,000 |
| P95 | 1,113,984 | 1,138,432 | **+24,448** |
| over gate | 7/128 | 8/128 | +1 |

**Paired by frame number, E66 is worse on 103 of 128 with a +2,624 median.** That
is a mechanism, not noise — a consistent small regression on nearly every frame,
which is exactly the shape Task 94 recorded when it *removed* a member. Adding
2,032 bytes re-addresses the other eleven and costs more than the placement gains.

**The durable finding is about the instrument, not the cubic.** Task 94's comment
already warned "do not size a placement move from a tier cyc/insn ratio — that
estimator predicted −7,894 and got the sign wrong." E66 used a *different and much
more specific* estimator — per-symbol recoverable non-mem stall for the exact
candidate — and it got the sign wrong too, by a similar margin. **Sections C and D
of the Task 37 census are a cost ranking, not a placement prediction.** `.text.hot`
is now closed in both directions: nothing may be added and nothing removed.

## R2-03 E65 — **WORK-H P95 1,113,984: the gate reading is MET**, and the fidelity fix is what found it (2026-07-29)

The chase for accuracy is what exposed the cost. That order matters, because the
cost was invisible from the source.

`artifacts/performance/r203-e65-q16arm-128.json`,
`...-r2.json`, `...-rows.csv`. 128-frame ring dump, same window and same
Boundary config as E64b's base.

| `WORK-H` | E64b base | **E65** | delta |
|---|---:|---:|---:|
| P50 | 996,736 | **982,848** | **−13,888** |
| P90 | 1,098,496 | **1,070,592** | **−27,904** |
| **P95** | 1,149,568 | **1,113,984** | **−35,584** |
| P99 | 1,249,600 | 1,212,864 | −36,736 |
| max | 1,521,472 | 1,493,952 | −27,520 |
| over gate | 9/128 | **7/128** | −2 |
| `SRC` P50 | 331,392 | **309,120** | −22,272 |
| `SRC` P95 | 479,744 | **443,904** | −35,840 |

**The whole distribution moved down, not just the P95 index** — which is the
standard this board set for itself after the E32 top-14 mistake. VBlank histogram
improved too: 4-interval frames 3 → 1.

**Read the margin honestly. P95 is 6,016 under 1,120,000, and the build-placement
noise floor is 5,000–7,000.** So this is *at* the gate, not comfortably inside it,
and one unlucky relink could put it back over. It is a real reading of a real
build, and it is the first time this campaign has produced one at or under budget;
it is not yet headroom. `FTR` P95 also rose 4,992 — the cubic left `.text.hot`
(see below), so the other members were re-addressed, exactly Task 94's mechanism.

**Where the −35,584 came from, and it was not the arithmetic.** Lifting the basis
to Q16 needs 64-bit squares. **This TU compiles `-mthumb`, and Thumb on ARMv5TE
has no `SMULL`**, so GCC emitted `bl __aeabi_lmul` — eleven call sites in
`gcPlayDObjAnimJoint`, eight on the executed path. The Q16-in-Thumb arm measured
**WORK-H P95 +36,032 / P50 +25,472** against E64b: a regression, from a change
that is pure precision at the C level.

One `__attribute__((noinline, target("arm")))` on `ndsR2CubicValueFixed` turned
those eight library calls into four `SMULL`/`SMLAL` — including the six **E64b was
already paying for**. Measured effect of the attribute alone, same commit, same
tree: **WORK-H P95 1,185,600 → 1,113,984, −71,616.** Side effects, both good:

- Only two soft-float calls remain in the whole evaluation, and they are the two
  that are irreducible — `__aeabi_fmul` for `length·length_invert` and
  `__aeabi_i2f` for the result.
- `gcPlayDObjAnimJoint` shrank 2,096 → 272 bytes and the cubic moved out to
  ordinary `.text`, so **`.text.hot` dropped 6,072 → 4,248 bytes** — 1,824 bytes
  returned to the curated 8 KiB working set.

**`-marm` is NOT a general lever here, and the census says so.** Ranking every
function in the ROM by 64-bit/divide helper call sites: `__aeabi_lmul` appears at
only 37 sites ROM-wide, and the top holders are `ndsOpeningRoomRenderDLPreview`
(33, not a battle path), libc `gmtime_r`/`mktime`/`strtol` (never called), and
`ndsPlatformRenderDebugHud` (the instrument itself). The cubic was special because
it was eight 64-bit multiplies on a path taken 148 times a frame. Of the 303
integer-helper sites, 210 are `__divsi3`/`__udivsi3`/`idivmod` — **integer
division, which ARM9 has no instruction for in either mode**, so ARM would not
touch them. `nds_renderer.o` already carries `-marm` (Makefile:2298); do not
follow this with a blanket sweep.

Battle-path divide holders, if a later cycle wants them: `ftDisplayMainCalcFogColor`
(9 `__divsi3`), `ftMainUpdateColAnim` (8), `syTaskmanRunTask` (6 `uidivmod`),
`ifCommonPlayerDamageUpdateDigits` (6 int + 18 float).

**Caveat on the base comparison.** The E64b base was measured at `063667a`;
`b951270` has landed since, which un-stubs `efManagerQuakeMakeEffect` and so
*adds* per-frame work. The E65 arms both carry it. The confound therefore runs
against E65, making −35,584 a floor rather than a ceiling. The thumb→ARM
−71,616 is confound-free: same commit, same tree, one attribute.

### E65's other half — E64b's equivalence, SETTLED with a bound instead of a hash

`scripts/check_r2_cubic_error_bound.py`. Evidence:
`artifacts/performance/r203-e65-cubic-error-bound.json`.

**The instrument `KNOWN_ISSUES.md` named for this was the wrong one, and that is
a third instance of the same gate-design bug.** The Task 9 state hash is a
bit-exactness hash. E64b is *authorized as non-bit-exact* — the code comment in
`battleship_sys_objanim.c` says so in as many words. So the hash cannot pass; it
can only ever report "differs", and that answer carries no information about
whether gameplay moved. Two builds and two emulator runs would have bought a
result that was knowable in advance.

**The right instrument for a non-bit-exact change is an error bound.** This is
it, and it needs no emulator, no ROM and no build: it extracts the shipped kernel
from between the `NDS_R2_CUBIC_FIXED_KERNEL_BEGIN/END` markers, extracts
`gcGetInterpValueCubic` from the decomp verbatim, compiles both on the host
(IEEE-754 single, round-to-nearest, same as the N64 for this purpose), and sweeps
the input domain. Extraction rather than a copy, so the bound can never be
measured against stale code. Runs in about four seconds.

**The first run was RED, and the mechanism is worth keeping.** The deviation
scales with `L·|rate|` — the curve's own steepness in value units per `t` —
because two of the four Hermite basis terms carry a factor of `length`, so a Q12
basis quantum reaches the result multiplied by that. Measured:

| domain | max &#124;error&#124; | rms | worst steepness |
|---|---:|---:|---:|
| rotation (radians, ≤2π) | 0.0130 | 0.0013 | 3.0 |
| translation (world units, ≤60) | **0.1065** | 0.0074 | 104.0 |
| conservative (±300, 4× chord rate) | 0.7615 | 0.0558 | 2240.0 |

Identical arithmetic, and a translation track deviates 8× further than a rotation
track purely because its steepness is 35× larger. **That is why the domain has to
be stated rather than assumed** — quoting one number for "the cubic's error"
would have been meaningless.

**Three fixes, all cheap, all kept:**

1. **Round instead of truncate**, at all six float→fixed conversions and every
   requantising shift. One ADD each. Removed the bias (mean signed error
   −9.1e-5 → +2e-6) and took the conservative worst case 0.998 → 0.762.
2. **`(1−t)² = 1 − 2t + t²`**, reusing the already-rounded `t²`. Deletes a
   multiply, a shift *and* a rounding step. It was the single worst term: at
   t = 0.92 the truncated square held 24 Q12 counts, so its own quantum was 4% of
   itself, and `h_rb` multiplied that by `length`.
3. **Q16 for the basis, Q12 for the values.** The values' own quantum lands
   straight in the result and 1/4096 of a radian is already invisible; the basis
   quantum gets amplified, so it is the one that needed bits. Costs two
   32×32→64 multiplies for `t²`/`t³` (SMULL, one instruction each).

**After all three, every gated domain is green:**

| domain | max &#124;error&#124; | before | rms |
|---|---:|---:|---:|
| rotation | **0.0028** rad (0.16°) | 0.0130 | 0.0005 |
| translation | **0.0067** units | 0.1065 | 0.0007 |
| conservative | 0.0432 | 0.7615 | 0.0031 |

The gate is 0.02 world units / radians. Joint values reach gameplay only through
`gmCollisionGetFighterPartsWorldPosition` (`gm/gmcollision.c:489`), which places
hitboxes in world units; Dream Land fighter hitbox radii are single-digit units
and the smallest gameplay-relevant separation is well above 0.1, so 0.0067 cannot
flip a hit decision. **E64b's numerical equivalence is no longer unverified.**

**Do not read this as a licence to skip in-situ checks generally.** A bound over a
stated domain is the right answer for an arithmetic substitution whose inputs are
enumerable. It would be the wrong answer for a change to control flow, lifetime,
or ordering, where the failure is not a rounding error.

## R2-03 E64b GRADUATED — the cubic in fixed point, −26,944 P95 / −20,352 P50. Boundary green; equivalence settled by E65 (2026-07-29)

`NDS_R2_CUBIC_FIXED := 1`. Owner-authorized 2026-07-29 as a non-bit-exact
change. **Boundary green with Fox CPU live.**

> **CORRECTION (same day).** This entry first said "the Task 9 state hash never
> moved, so nothing needed re-bounding". **That was wrong and the claim is
> withdrawn.** `NDS_TASK9_STATE_HASH ?= 0` and nothing in `verify-all.ps1` or the
> Boundary harness references it, so the hash **was never evaluated** — not
> unchanged, *unmeasured*. I read a passing Boundary line about "Task 9 float
> ITCM" as the state hash; they are different checks. The hash does cover `AOBJ`
> and `DOBJ` records, i.e. precisely the joint values this changes, so it is the
> right instrument and it still owes an answer. **E64b's numerical equivalence is
> UNVERIFIED**; only its performance and Boundary-liveness are established.
> Tracked in `KNOWN_ISSUES.md`. The lesson generalises: *a verifier that is not
> compiled in cannot pass.* Check the flag, not the absence of a failure line.

E60/E61 priced the target: 149.4 cubic evaluations a frame at ~405 ticks each,
14 soft-float ops, 99.6% of the animation path's float. The rewrite is exact in
real arithmetic — with `t = length·length_invert` the original's expression is
the standard cubic Hermite:

```
value = vb·(2t³−3t²+1) + vt·(3t²−2t³) + rb·L·(1−t)² + rt·L·(t²−t)
```

so only the *rounding* changes: Q12 truncation instead of MIPS single precision.
Step (43.6% of nodes) and Linear (1.7%) keep the decomp's own expressions and
stay bit-identical.

| `WORK-H` | E32 base | **E64b** | delta |
|---|---:|---:|---:|
| P50 | 1,017,344 | **996,992** | **−20,352** |
| P95 | 1,176,512 | **1,149,568** | **−26,944** |
| `SRC` P50 | 342,016 | 332,672 | −9,344 |
| over gate | 12/128 | **9/128** | −3 |

Engagement proof: **135,871 evaluations, 0 saturations**. P50 moving as far as
P95 confirms E60's reading that float is a *flat* per-frame cost.

**Arm A was a regression and the reason is worth keeping.** It added a
256-entry cache of the Q12 conversions keyed on the source float bit patterns.
The mechanism worked — 86.4% hit rate, zero saturations — and the frame still got
worse: **P95 +21,632, `SRC` P50 +17,792, over-gate 16/128.** Two footprint
causes, both already written down in this repo:

- **10,240 bytes of new BSS.** "The noise floor is not measurement error, it is
  the price of adding data", and that floor is 5,000–7,000.
- **The `.text.hot` member grew 500 → 1,824 bytes.** Task 94's own comment in
  `linker/nds_hot_text.ld` says that list is a curated 8 KiB working set and
  perturbing one member re-addresses the other ten, which it measured at 6,144.

Arm B spends nothing: no cache, no BSS, 32-bit intermediates wherever `t`'s
Q12 range allows, and hand-rolled float↔Q12 converters because
`(s32)(v * 4096.0f)` is two soft-float calls where bit manipulation is a dozen
integer ops. **Do not re-add the cache.**

**Where the gate stands after E32 + E64b + E65:** P95 **1,228,928 → 1,149,568 →
1,113,984**, a cumulative **−114,944**, over-gate **17/128 → 9/128 → 7/128**. The
1,120,000 gate reading is met, by 6,016 — inside the placement floor, so at the
gate rather than through it. E65's entry above has the honest margin discussion;
the second pass at the animation path it called for is what E65 was.

## R2-03 E32 GRADUATED — −52,416 WORK-H P95, 17/128 → 12/128 over gate (2026-07-29)

`NDS_R2_FIGHTER_SHUFFLE_FOLD := 1` in both shipped Makefile blocks. The hitlag
shuffle no longer knocks the native fighter owner off its path, so the generic
display-list interpreter stops running as a second renderer for the ~5 frames of
a hitlag burst.

**Owner-approved 2026-07-29 with a known visual residual** (the struck fighter
does not flash white). E62 established that is a generator gap, not a runtime
bug, and every non-flash frame is pixel-identical. Tracked in `KNOWN_ISSUES.md`.

128-frame ring dump, frames 794..921, same window as the control:

| `WORK-H` | control | E32 | delta |
|---|---:|---:|---:|
| P50 | 1,013,952 | 1,017,344 | +3,392 |
| **P95** | 1,228,928 | **1,176,512** | **−52,416** |
| max | 2,040,896 | 1,536,832 | −504,064 |
| **over gate** | **17/128** | **12/128** | **−5** |

P50 +3,392 is inside the 5,000–7,000 placement floor. E54 projected −51,136 and
13/128; delivered −52,416 and 12/128. **Boundary green.** Evidence:
`artifacts/performance/r203-e32-graduated-clean-128{.json,-rows.csv}`.

**Two process notes, both worth carrying forward.**

1. **The first measurement was confounded and had to be discarded.** A helper
   agent was editing `reloc_backend_mp_collision.c`,
   `reloc_backend_compat_shims.c` and `nds_mp_floor_crossing.h` in the *same
   worktree*; its edits (14:29–14:34) predate that build (14:45) and dump
   (14:46). Re-measured with those changes stashed: P95 1,176,512 vs the
   confounded 1,172,992, over-gate 12 vs 15. The confound did not change the
   verdict, but it could have. **One worktree, one writer** — use
   `isolation: "worktree"` for a concurrent implementer.
2. **P95 is index-sensitive when the tail is sparse.** The harness uses
   `floor((n-1)*0.95)`; `int(n*0.95)` is one position higher and reads 39,680
   different on this distribution. Over-gate count and max are convention-free
   and moved decisively (−5 frames, −504,064), which is why they lead here.

## R2-03 E62 — E32 is a GENERATOR gap, not a visual-approval call. E49's flag built and REFUTED with a picture (2026-07-29)

**Correcting two things this board and `HANDOFF.md` have said, including my own
E59 entry.** The first direct look at `artifacts/visibility/e32-*.png` — never
done across E32 and E47–E59 — settles it.

**1. The arms were read backwards.** The board records the owner drawing "dark
maroon where the generic path draws light grey", implying corruption. Zoomed:

- **`e32-off` (generic) = Mario washed out to near-white.** *That is the hurt
  flash.* It is the correct render.
- **`e32-on` (E32/native owner) = Mario in his normal red cap and blue
  overalls.** Nothing is corrupt. **The owner simply never applies the flash.**

**2. The regression is confined to flash frames.** Pixel diff over the top
screen, both arms:

| frame | differing pixels |
|---|---:|
| 480 (hitlag) | 1,551 (1.35%) |
| 481 (hitlag) | 1,266 (1.10%) |
| 510 | **0** |
| 511 | **0** |

**E32 is bit-identical everywhere except the flash.** That is a far narrower
defect than "the owner renders the fighter wrong".

**Mechanism, confirmed from E59's own numbers:** `NDS_RENDERER_GEOM_LIGHTING` is
`0x00020000`. E59 recorded the owner's `geometry_mode = 0x00220105` — lighting
**set** — while the generic path's lit function took its
`!(geometry_mode & LIGHTING)` early-out, which is exactly why E59 saw zeros.
Two different fighters, two different `stats`. **The flash clears `G_LIGHTING`
for the struck fighter and draws its vertex colours raw.** Under
`NDS_R2_FIGHTER_HW_LIGHT` the owner skips the diffuse/ambient write when
`epoch_lit` is false but still emits `GFX_NORMAL` with `POLY_FORMAT_LIGHT0` set,
so the hardware lights the flashing fighter with **stale** diffuse/ambient.

**E62 built E49's existing fix and it is REFUTED.**
`NDS_R2_UNLIT_VERTEX_EPOCH` (default 0, never enabled in any shipped block)
already drops `POLY_FORMAT_LIGHT0` and emits
`ndsRendererR2DenseVertexColor15(dense_id)` instead of the normal. Built with
`NDS_R2_FIGHTER_SHUFFLE_FOLD=1 NDS_R2_UNLIT_VERTEX_EPOCH=1`:

| | vs the correct generic render |
|---|---:|
| E32 alone | 1,551 px (1.35%) |
| **E32 + unlit route (E62)** | **2,199 px (1.91%) — WORSE** |

`artifacts/visibility/e62-on-480.png` shows why: Mario renders in **rainbow
speckle**. `ndsRendererR2DenseVertexColor15` reads
`sNdsNativeFighterDenseVertices[].rgba`, and **that baked table holds the F3DEX2
packed normal**, not a colour. **E49's own stated objection — "a baked table
cannot show the flash" — was right, and this is the picture proving it.**

**E48 and E58 were each right about a different vertex stream, which is why
they read as contradictory.** The *live* display-list vertices on a flash epoch
are colours (E48, 273/273, material 0). The *baked dense* table is normals
(E58). They are not the same data.

**So E32 is not a fidelity-budget question and I was wrong to call it one.** The
owner does not possess flash-colour data to draw. Closing it needs a
**generator** change — bake the unlit flash variant's vertex colours as a second
dense table beside `sNdsNativeFighterDenseNormals` — plus a per-epoch select on
`geometry_mode & LIGHTING`. That is ordinary specialization work of exactly the
kind `PROJECT_GOAL.md` prefers, and it needs no owner decision. **E63 should
size that table** before writing the generator; the runtime half already exists
and is proven to reach the emit path.

Lab flags only; both default 0, nothing shipped.

## R2-04 E6 ANSWERED — E5 paid down LOADING, not pose. R2-04's rate clause is done; its budget clause is E61 (2026-07-29)

Answered from artifacts already on disk; no build. Closes the last pending R2-04
row.

**What E5's cache actually is.** `sNdsR2AnimCache` lives in
`reloc_backend_assets.c:5565` and is filled by `ndsR2AnimCachePreloadStep`, pumped
from `battleship_scvsbattle.c:204`. It caches **animation asset loads**, not pose
evaluation. On the post-E5 ordinary-frame profile it costs **170 ticks/frame** and
the loading class is down to `ndsRelocGetFileData` at 3,532.

That is R2-04's *"Absorbs Task 75: all animation streams for the match prepared
at load; no first-use loading during gameplay"* clause — **satisfied**. It also
explains E52 independently: "E35's 25-of-26 `SRC` reading no longer holds, it
predated E5 removing the loading component." And it explains E60: the soft-float
caller distribution barely moved (57.17% → 58.06%) because **E5 never touched
pose evaluation at all.**

**R2-04's rate clause is also already satisfied, and cannot go further.** The
phase says "evaluated once per presented frame (30 Hz), not per gameplay tick"
and warns against assuming full cubic evaluation must run twice per rendered
frame. E57 measured the renderer already at presentation rate
(`DLAllDrawForSlot` 2.0 calls/frame, `AdapterBuildDObjLocalMatrix` 50.0), so the
*visual* side is at 30 Hz today. The remaining 60 Hz evaluation is the
**gameplay** skeleton, and E57 showed it is load-bearing:
`gmCollisionGetFighterPartsWorldPosition` (`gm/gmcollision.c:489`) places every
hitbox by walking the live joint chain. §3.6's split is therefore already
implemented as far as the contract permits — halving the remaining half is a
gameplay change, not a rate decoupling.

**What is left of R2-04 is purely its budget clause**, and E60/E61 price it:
pose evaluation is **146,942 ticks/frame against the provisional 100,000
budget**. Rate cannot close that; only cheaper evaluation can, which is E61's
cubic (~50,000). **R2-04 does not need another experiment — it needs the E61
owner decision.**

## R2-06 E0 — the Runtime 2 battle path is PERFORMANCE-NEUTRAL, and Boundary is green through it (2026-07-29)

First measurement of `NDS_R2_PATH=1` end to end. **Engagement verified before
reading anything**: `ndsR2BattleRun` is present in the R2 ELF and absent from the
control, and the two config dumps read `NDS_R2_PATH 1` and `0`.

Two-CPU stress config both arms, same commit, 128 frames. R2-06 makes the
**2-VBlank share** the headline metric:

| | Runtime 1 (A) | Runtime 2 (B) | delta |
|---|---:|---:|---:|
| **2-VBlank** | **614 (66.7%)** | **609 (66.1%)** | **−5** |
| 3-VBlank | 291 | 295 | +4 |
| 4-VBlank | 15 | 15 | 0 |
| 5+ | 1 | 2 | +1 |
| `WORK-H` P50 | 1,130,112 | 1,131,008 | +896 |
| `WORK-H` P95 | 1,497,664 | 1,510,208 | +12,544 |

**Every delta is inside the 5,000–7,000 placement floor. The switch neither costs
nor saves anything.**

**Gate status:** *"Boundary green"* — **PASSES**, engagement verified in the
proof ROM. **Equivalence — PASSES** (E1 below). *"Histogram materially better
than the Runtime 1 A-side on the same commit"* — **NOT MET**, and amended in the
plan. *"Soak clean"* — **still owed, the only open R2-06 item.**

### R2-06's "soak clean" clause has NO INSTRUMENT

There is **no soak harness in the repo.** `ls scripts/ | grep -iE 'soak|stability|long'`
returns nothing, and the only `soak` string anywhere is the word in this plan's own
gate text. So R2-06 cannot currently be closed as written — not because the path
fails a soak, but because **nothing is able to run one.**

This is the same failure class as E64b's state hash: *a gate that names an
instrument which does not exist, or is not compiled in, reads as unmet forever and
gives no signal either way.* Two of R2-06's three clauses turned out to have this
shape once measured — the histogram clause measured the wrong thing, and the soak
clause has nothing to measure with.

**What a soak needs to assert here**, so whoever builds it does not have to
re-derive it: sustained operation across a full match into Time Up and Results
without hang, corruption, or nondeterminism; no arena/heap drift
(`gNdsTaskmanArenaBytes` is the existing counter); no cadence violations or
`slips`; and correct handling of `ndsR2BattleRun`'s `terminal_update` branch
(`src/nds/r2/nds_r2_battle.c:103`), which is the switch's highest-risk code —
BattleShip's `syTaskmanRunTask` checks `LoadScene` immediately after `task_update`
and never draws the terminal update, and the R2 loop has to reproduce that.

### R2-06 E2 — the R2 path crosses the match boundary cleanly. Still not a soak

**The tick sampler was the wrong instrument and cost two failed runs.** E2 first
asked for 256 tick-HUD samples from frame 3300; a GDB stop per presented frame
that deep exceeded the 900 s ceiling twice (once also losing the listener on a
port collision). The question was never a percentile question — it was *does the
terminal branch survive Time Up* — and a real-time capture answers it directly,
with no GDB at all, in 90 s.

`capture-melonds.ps1` on `builds/build-r2-06-path`, two shots:

- `artifacts/visibility/2026-07-29_r206-e2-matchend-timeup.png` — TIME 00:06,
  FPS 27.3, match live and both fighters in play.
- `artifacts/visibility/2026-07-29_r206-e2-matchend-results.png` — **TIME 00:00,
  FPS 29.9**, camera pulled back to the wide stage view, tick HUD still filling
  its 128-entry ring, `cadenceViolations` clean, no hang and no corruption.

So `ndsR2BattleRun`'s `terminal_update` branch handles the 3600-tick boundary and
the loop keeps presenting through it. That is the switch's highest-risk code and
it is now exercised.

Note the numbers on those shots are the **two-CPU stress config** (`build-r2-06-path`
carries `NDS_R2_BOTH_CPU`), so ALL 1,679,552 → 1,119,872 across the boundary is a
stress-config reading. **Never quote it as the Boundary figure.**

**This is still not a soak, and must not be recorded as one.** It is one match, one
run, at real time; it asserts nothing about arena drift across repeated matches,
nothing about rematch lifetime, and it read no `gNdsTaskmanArenaBytes`. The
instrument gap above stands.

### R2-06 E1 — equivalence: every semantic and geometry counter is byte-identical

Free, from the two Boundary runs already on disk (no build, no emulator run).
Both arms, same commit, canonical mode 163:

| counter | Runtime 1 | Runtime 2 |
|---|---|---|
| `ftrContract` | `6784/6784/geom0x222005/cycle0x100000/rm0xc4112078/light424/424/bounds424/0` | **identical** |
| `ftrTri` | `132712/p067840/p164872/own424` | **identical** |
| `oracle` | `0/0/0` | **identical** |
| `binds` | `54` | **identical** |
| `combine` | `0/0/lit0/mat0/proj126` | **identical** |
| `intrinsicM3` | `9/121/828` | **identical** |
| `intrinsicM4` | `24/136192/hits646/fence0` | **identical** |
| `aobj32` | `40/293/reuse8/fail0` | **identical** |
| `water` | `2/0/1` | **identical** |

**132,712 triangles, 424 fighter contracts, 6,784 contract checks, 646 matrix
residency hits, and zero oracle mismatches — matching exactly.** This is not a
full state hash, but it is a far broader instrument than the one E64b is waiting
on, and it covers precisely what a loop-structure switch could plausibly break:
draw ordering, contract counts, triangle emission, material/combine selection and
matrix residency.

**Harness trap worth keeping.** These counters are *not* in the `Tee-Object` logs
— `Tee-Object` writes UTF-16, so GNU `grep` silently finds nothing in them. Read
them with PowerShell `Select-String`, or from the persisted tool output. Separately:
PowerShell variables are **case-insensitive**, so a `$t` scratch variable
clobbers `$T`; that cost a wrong "file missing" here before it was spotted.

**The gate clause needs reinterpretation, and that is the real finding.** It was
written expecting the R2 path to carry the wins. It cannot: `ndsR2BattleRun`
(`src/nds/r2/nds_r2_battle.c:57`) is the same loop shape as the Runtime 1 body it
replaces — `updates_per_present` inner ticks, present, finish — and **everything
that actually saves time is already enabled in both arms** (R2-02 stage direct,
R2-03 fighter direct/E32/E64b, R2-04 loading and rate). The switch is an
**architecture and correctness step, not a performance step**, and asking it to
show a better histogram measures the wrong thing.

So R2-06 should gate on *equivalence plus soak*, and the histogram comparison
belongs on the phases that feed it. Recorded in the switch plan.

## R2-05 E1 — the fighter-special-case gate PASSES too; R2-05 is complete (2026-07-29)

R2-05's second clause: *"Same generators, same direct path, **zero
fighter-specific runtime special cases**. Any hand-patched Mario exception found
here is a generator defect to fix."*

**A raw `Mario` grep is the wrong instrument** and reads as a huge violation —
701 hits in `reloc_backend_mp_collision.c` alone. Almost all of them are
`ndsFighterMarioFox*`, which names a function serving **both** fighters.
Separating `Mario(?!Fox)` from `(?<!Mario)Fox`:

| file | Mario-only | Fox-only |
|---|---:|---:|
| `reloc_backend_assets.c` | 194 | 204 |
| `reloc_backend_ftdata_symbols.c` | 175 | 187 |
| `reloc_backend_fighter_model.c` | 49 | 49 |
| `nds_renderer.c` | 31 | 27 |
| `nds_audio_fgm.c` | 20 | 21 |

**Symmetric everywhere on the draw and asset path** — these are per-fighter data
tables, which is exactly the shape the plan asks for ("build tooling generic,
runtime specialized"), not hand-patched exceptions.

The asymmetries are all outside the gate's scope and all the same thing —
**diagnostic counters carrying stale Mario-era names**: `taskman_seam.c` 63/15
(`MarioTickCount`, `MarioDispatchCount`, `MarioNextSceneKind`…),
`sprite_preview_backend.c` 18/0 (`MarioDrawVisibleSObjCount`…), `diagnostics.c`
55/27. The one entry that looked like a real move-specific branch,
`MarioTornado`, is `gNdsFighterInitP0PassiveMarioTornado` **and** `…P1…` — a
symmetric per-player counter named for a Mario-only move, not Mario-only code.

**R2-05 is therefore complete: reproducibility (E0) and no fighter special cases
(E1).** Actionable hygiene item, not a gate failure: those Mario-era counter
names in *shared* seams actively mislead — a `…MarioTickCount` in `taskman_seam.c`
reads as fighter-specific behaviour where there is none, and it is what made this
audit look alarming before it was measured. Rename to a neutral or per-slot form
when those seams are next touched.

## R2-05 E0 — generator reproducibility gate PASSES; one generator defect found (2026-07-29)

R2-03's two levers are both owner-blocked (E32 on the hurt flash, the cubic on
the Task 9 hash), so the next switch-plan phase with an autonomously-settleable
gate is **R2-05**: *"generators reproduce the `.inc` files byte-identically from a
clean checkout."* That half now passes.

Six generated artifacts exist. Four ship a `--check` mode; the other two were
regenerated and byte-compared:

| artifact | bytes | tracked | result |
|---|---:|---|---|
| `nds_native_fighter_owner.generated.inc` | 408,284 | no (gitignored) | **reproducible** |
| `nds_native_stage_owner.generated.inc` | 75,388 | no | reproducible |
| `task39_hit_sparks.generated.inc` | 141,031 | no | reproducible |
| `battle_playable_static_textures.generated.inc` | 28,684 | no | reproducible |
| `dreamland_ds_mesh.generated.inc` | 15,022 | **yes** | reproducible, current |
| `task39_effect_census.generated.h` | 16,042 | **yes** | reproducible, current |

The R2-05 artifact — the 408 KB fighter owner IR — was additionally generated
twice under **different `PYTHONHASHSEED` values (1 and 12345)** and both runs are
byte-identical to each other and to the working copy. Dict/set iteration order is
the usual source of generator nondeterminism and it is excluded here.

**A clean checkout can build**: four of the six are gitignored, but `build.ps1`
(the clean-checkout entry point, not bare `make`) invokes each generator and then
asserts its output exists via `$generatedOutputs`. Bare `make` assumes they have
already been produced — worth knowing before diagnosing a missing-`.inc` failure.

**Generator defect found — `generate_task39_effect_census.py` writes source line
numbers into permanent dated evidence.** Its "ownership evidence" column embeds
`src/port/reloc_backend_compat_shims.c:<line>`, so any unrelated edit to that
file silently invalidates 60 rows of
`artifacts/performance/2026-07-21_task39-visual-effects-census.md`, and
re-running the generator rewrites a **dated** artifact with today's line numbers.
Running it here shifted `7713 → 7774` and `12870 → 12963`. **Do not run it**; the
committed copy is a 2026-07-21 snapshot and AGENTS.md makes `artifacts/performance`
permanent evidence. Recorded in `KNOWN_ISSUES.md`. It is not in `build.ps1` or the
Makefile, so nothing triggers it accidentally.

The other half of R2-05's gate — *"zero fighter-specific runtime special cases"* —
is not yet audited; a helper agent is currently editing
`reloc_backend_mp_collision.c` for the open `docs/BUGS.md` gameplay defects, and
that file carries the largest Mario-identifier count in the runtime, so the audit
waits until the tree is quiet rather than racing it.

## R2-03 E61 — it is the CUBIC. Pose table refuted by size; two levers now close the gate (2026-07-29)

Full report: `optimization/ClaudeOpus5_R203_E61_TheCubicIsTheLever_20260729.md`.
Census behind `NDS_R2_ANIM_CENSUS` (default 0), counting only via the Task 95
interposition. Cross-checks Task 96 exactly: longest chain **9**, and
96,308 calls ÷ 104.1 calls/frame = 925 frames against a run ending at 934.

**1 — the kind mix.** Cubic **54.8%** (149.4/frame, ~14 float ops), Step 43.6%
(118.7/frame, zero float), Linear 1.7%, Other 0. **The cubic is 99.6% of the
animation's float**, and E60's 60,509 ticks/frame across 149.4 nodes is **405
ticks per cubic evaluation** — 14 soft-float ops at ~29 each. This also rules out
the layout reading Tasks 95/96 assumed: had the nodes been mostly Linear, 405
ticks each would have been impossible.

**2 — `anim_speed`.** `1.0` on 99.726% of calls, **`0.0` never**, `0.5` on
0.274% (bits `0x3F000000`). Dyadic, so a half-frame index is still exact.

**3 — discarded evaluations: zero.** `GOBJ_FLAG_NOANIM` skips = 0. No free win.

**The load-time pose table is REFUTED — on memory, not correctness.** 272.7
nodes/frame ÷ 2 anim ticks ≈ 68 per fighter = 273 bytes/pose; an 80-frame
animation at half-frame resolution is 42.6 KB/fighter; the 63 animations
reachable in a natural match (Task 40) are **2.62 MB resident** against 4 MB of
main RAM the match already mostly occupies. Streaming on transition is 42.6 KB =
7–11 ms on cart, most of a frame, on transitions that happen constantly. **Do
not propose it again.**

**What is left, priced.** No bit-exact option remains.

| route | saves |
|---|---:|
| fixed-point cubic (14 ops @ ~5 ticks) | **50,051** |
| float Horner after per-parse pre-expansion | 34,512 |
| **fixed-point Horner** (6 ops @ ~4 ticks) | **56,774** |

Pre-expansion is available because `length_invert`/`value_base`/`value_target`/
`rate_base`/`rate_target` are constant between parse events, so the cubic is a
fixed polynomial in `length`. Reassociation alone makes it inexact, so it buys
nothing fixed-point does not.

**These two levers close the gate:** 108,928 − 51,136 (E32) − ~50,000 (cubic) =
**~7,800 remaining**. Each is blocked on a different owner decision — E32 on the
hurt flash (now a fidelity-budget/visual-approval question, not a measurement:
E48–E59 closed the mechanism line), and the cubic on the Task 9 state hash.
`PROJECT_GOAL.md` requires mechanical equivalence and permits "fixed-point
replacements"; the hash asserts bit-exactness, a stronger claim than the
contract makes. The change is confined to `gcGetInterpValueCubic` evaluating
already-parsed track state — not parsing, collision, physics or CPU logic — so
the honest acceptance test is a hitbox-overlap differential over a full match
(the only path to gameplay is `gmCollisionGetFighterPartsWorldPosition`, E57),
not the hash.

## R2-03 E60 — ANIMATION owns the gate, not collision. Task 78 stopped it on a self-vs-inclusive error (2026-07-29)

**The board's `SRC`-half claim is wrong and the animation lever must reopen.**
Full report:
`optimization/ClaudeOpus5_R203_E60_AnimationIsTheGate_20260729.md`. Zero builds
were spent on the attribution — the E53 profiles were already on disk.

Ordinary frames 876–879: total 1,120,324, idle 150,837, **WORK 969,487**.

| | self | via `fadd`/`fmul` | inclusive |
|---|---:|---:|---:|
| `gcPlayDObjAnimJoint` | 34,022 | **60,509** | **94,531** |
| `battleship_ftAnimParseDObjFigatree` | 12,115 | 5,703 | 17,818 |
| `ndsBaseGcPlayMObjMatAnim` | 5,201 | 4,560 | 9,761 |
| seven more animation symbols | 24,709 | 123 | 24,832 |
| **animation total** | **76,047** | **70,895** | **146,942** |

**146,942 ticks/frame, 15.2% of WORK — larger than the whole 108,928 gap.**

**Collision is not the float cost.** Ranked by caller, the entire collision
family (`ndsStageMPSegmentIntersection2D` 1,479, `ndsMPFloorSegmentCrosses‑
DownwardKernel` 862, `gmCameraUpdateInterests` 708, `mpProcessUpdateMain` 678) is
**under 4,000 ticks/frame** — below the build-placement noise floor. The `SRC`
*bucket* attribution was right; reading it as `gmcollision.c` was a guess that
no caller-level measurement ever supported. **Delete the "float→fixed on the
collision path" row.** The renderer share is 15,709 (15.1%), inside switch plan
§3.9's "10–20K usually too small" band, so it is not architecture work either.

**Why every previous reading missed it: a leaf helper is charged to itself,
never to its caller.** Task 78 §3 totalled animation at 82,807 *self* ticks and
stopped against a 100,000 target; its own §4 listed `fadd`+`fmul` = 119,912 as a
*separate* family. Applying E60's measured shares to Task 78's own numbers:
82,807 + 67.9% × 119,912 = **164,236 — 1.64× its target, not 0.85×.** Both
numbers were in that report, on facing pages, in different families.

**Tasks 95 and 96 stand and do not block this.** They refuted the *layout*
route (hoist works/frame regresses; 0 of 15,687 adjacent `AObj` pairs). The
*arithmetic* route has never been attempted, because Task 92 §5 declared it
frozen. That freeze is the **Task 9 state-hash verifier, not the product
contract** — `PROJECT_GOAL.md` requires mechanical equivalence and explicitly
lists "precomputed animation data", "quantized animation poses", "fixed-point
replacements" and "reduced animation interpolation" as allowed. Task 77 E1 and
E57 forbid computing a *different* pose; neither forbids computing the same pose
more cheaply.

**Also: float is a flat cost, not an excursion cost.** Ranking E53's
+420,227/frame excursion delta by symbol gives 376,434 across 151 symbols that
are exactly zero on control (the E54 fallback, confirmed) and 173,981 across
symbols on both — whose top is fixed-point *matrix* work (`LoadHardwareMatrixPair`
+10,185, `MtxMul20p12` +8,478, `BuildDObjLocalMatrix` +8,080). `__aeabi_fadd`
does not appear in the growth list at all. **That is what makes animation the
right target: it moves P50 and P95 one for one, where E32 touches five frames.**

**E61 sizes the table before any code is written** — three integers: distinct
(animation, frame) pairs reachable in Boundary; bytes per pose; and whether
`anim_speed` ever leaves {0, 1}. If the pose is a pure function of (animation,
frame) it is precomputable at load time *with the identical float arithmetic*,
which is bit-exact by construction — the state hash never sees a different value
and no owner decision is needed. Precedent for the fidelity required:
`scripts/generate_pupupu_water_aot.py` already AOT-compiles the material
animation script, rounding after every MIPS single-precision operation. If the
table does not fit, the fallback is a per-fighter generated evaluator, not a
smaller table.

Harness fixed at its seam: `census-softfloat-callers.ps1` multiplied shares by a
hardcoded `191,810` from the Task 81 partition and printed the product as a
measurement; on the current build that constant is **84% high**. It now reports
the share and names the scale's provenance.

## R2-03 E59 — the generic software lighting NEVER RAN. E58 is retracted, and the flash line is CLOSED (2026-07-29)

**Six experiments have now been spent on the hurt-flash mechanism (E48, E49,
E50, E55, E58, E59) and the line is closed without a mechanism.** Read this
entry before proposing a seventh.

E59 latched, on hitlag frame 911 (A) and ordinary frame 904 (B), the resolved
light pair inside the generic `ndsRendererHardwareLitShadeColorPrepared` and
`stats->light_color_1/2/mask` + `geometry_mode` at the native owner's shade
entry (`ndsRendererNativeShadeProductionActions`). E54 established that only one
fighter falls back per hitlag frame, so 911 runs **both** paths at once and the
two halves of the snapshot are a same-frame comparison.

| slot | A (911, hitlag) | B (904, ordinary) |
|---|---|---|
| generic `light_1` | **0** | 0 |
| generic `light_2` | **0** | 0 |
| generic `light_color_mask` | **0** | 0 |
| owner `light_color_1` | `0xFFFFFF00` | `0xFFFFFF00` |
| owner `light_color_2` | `0x4C4C4C00` | `0x4C4C4C00` |
| owner `light_color_mask` | 3 | 3 |
| owner `geometry_mode` | `0x00220105` | `0x00220105` |
| owner shade calls | 31 | 49 |

**Two results, and they point the same way.**

1. **The generic lit-shade path resolved light colours zero times on frame 911.**
   This is not an epoch-sampling artifact — the slots are overwritten, so zero
   means *never written*, and one execution would have stored `0xFFFFFF00`. The
   function therefore either was never called or took its `stats == NULL ||
   !(geometry_mode & G_LIGHTING)` early-out every time (`nds_renderer.c:8251`).
   Either way **no software lighting produced E55's 273 samples.**
2. **The owner's light state is byte-identical between a hitlag frame and an
   ordinary one** — all four fields, on the last epoch of each frame. The flash
   is not a light-colour change on the owner's side either.

**Retract E58.** Its claim — that with `G_LIGHTING` set the vertex RGB bytes are
an F3DEX2 packed normal and the emitted colour comes from `light_color_1/2` —
required the lit path to be the one producing E55's samples. It was not running.
E58's supporting observation ("24 distinct raw values collapse to 8 outputs")
rests on `gNdsR2FlashRawPending`, a value parked by the *outer*
`ndsRendererHardwarePackedVertexColor` and read by the *inner*
`ndsRendererHardwarePackedValidVertexColor`; if the inner function has any
caller that does not go through the outer one, the raw/output pairing is
misaligned and the collapse is an artifact of the probe. That pairing was never
verified. **The 76-grey is still real** — it is `light_color_2`'s
`0x4C4C4C` ambient — but it is the owner's constant, present on ordinary frames
too, so it is not the flash.

**Why the line closes rather than continuing.** Even a perfect E32 leaves the
gate missed: E54 projects P95 1,177,792 and 13/128 over gate, still **57,792
above 1,120,000**. The flash is the blocker on a lever that cannot close the gap
by itself, while the `SRC` half that *can* is untouched. Standing rule from the
switch plan §3.9 applies to investigation budget as much as to ticks. **The next
build goes at `SRC`** (E60), not at a seventh flash probe.

Probe retained behind `NDS_R2_FLASH_PROBE` (default 0) with slots 12..19 so a
future owner of E32 does not rebuild it. Do **not** re-derive: the flash is not
vertex colour (E48/E49/E50/E55), not material colour (E47), not light colour
(E59), not the fold arithmetic and not E16's hardware lighting (E41, three-way
capture), not `color_modulate` (E36).

## R2-03 E58 — RETRACTED by E59. Those bytes are a NORMAL, not a colour (2026-07-29)

**Three experiments have now modelled the hurt flash as something happening to a
vertex *colour*. There is no vertex colour on these runs.** E58 dumped the raw
decoded `NDSRendererInputVertex` alongside the lit output for the same 24 stride
samples on hitlag frame 911, and the pairing is impossible for any colour model:

| raw source RGB | lit output |
|---|---|
| `(46,163,73)` | `(76,76,76)` |
| `(5,126,20)` | `(76,76,76)` |
| `(186,34,101)` | `(76,76,76)` |
| `(198,152,45)` | `(36,15,17)` |
| `(3,127,2)` | `(36,15,17)` |

**Wildly different inputs collapse to identical outputs.** 24 distinct saturated
raw values — `(199,174,177)`, `(142,6,201)`, `(222,37,139)` — and 8 distinct
outputs. No per-vertex colour multiply can do that.

`nds_renderer.c:8246` says why:

```c
if ((stats == NULL) ||
    ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) == 0u))
{
    return ((u32)vtx->r << 24) | ((u32)vtx->g << 16) |
        ((u32)vtx->b << 8) | (u32)vtx->a;   /* lighting OFF: bytes ARE a colour */
}
diffuse = light_1;   /* lighting ON: bytes are a NORMAL, and the colour comes */
ambient = light_2;   /* from stats->light_color_1 / light_color_2 */
```

With `G_LIGHTING` set — which it is here — the RGB bytes are the **F3DEX2 packed
normal**, and the emitted colour is built from the **light colours**, modulated
by the normal's diffuse term. Everything observed follows:

- the greys are white lights, and **76 is the ambient-only floor** that every
  back-facing normal clamps to (which is why 10 of the first 24 were exactly 76);
- the reds are runs whose `light_color_*` is red;
- E50's "172/273 differ" is just 273 different normals.

### What this retires, and what it opens

- **E48's "the flash is a raw vertex colour"** — misread normal bytes as a colour.
- **E55's "the flash replaces the colour"** (mine, below) — inherited that error.
  0/541 baked vertices being achromatic while flashed ones are is real, but the
  explanation is that one side is a *colour table* and the other is *lighting
  output*, not that a flash whitened anything.
- **E49's option 1, a per-epoch constant colour** — dead again, and this time for
  a structural reason rather than a sampling one. There is no colour to inject.

**The flash is a light-colour change.** `stats->light_color_1` /
`light_color_2` are per-run material state, applied by the very state replay
E26 wants to fold. That is the field to compare between a hitlag frame and an
ordinary one, and against what the native owner's shade path
(`ndsRendererNativeShadeProductionActions`, `ndsRendererR2MaterialColor15`)
feeds its own lighting. E47 refuted *material colour* derivation — `light_color_*`
is a different field and was never tested.

**Do not model the flash as vertex data again.** Four experiments have now died
on that premise (E48, E49, E50, E55).

## R2-03 E55 — the flash REPLACES the colour; E50's refutation was an inference error (2026-07-29)

**E50 closed the cheapest fix for E32 on a wrong inference, and this reopens it.**
E50 recorded that 172 of 273 vertices carry a different `vertex_color` on a
hitlag frame and concluded "the flash is not uniform, so a per-epoch constant
colour is dead". That measured the **output** of lighting and inferred the
**input** was per-vertex. It is not.

Per-vertex `vertex_color` in call order, latched on **two** hitlag frames (911
and 912 — hitlag freezes the pose, so a second hitlag frame is the only valid
pair; an ordinary frame has 0 calls by E48):

Two samples were taken, and **the second corrects the first — record both.**

| | first 24 calls | **every 11th of 273** |
|---|---|---|
| A vs B, elementwise | **identical** | — |
| **pure grey (R==G==B)** | 24/24 = **100%** | **18/24 = 75%** |
| distinct values | 8 greys, `4C4C4C`..`FFFFFF` | 18 greys + 4 reds |
| **baked table, pure grey** | **0 of 541 = 0%** — `(49,87,177)`, `(210,92,74)`, `(102,76,255)`, … | |

**The prefix was unrepresentative and a prefix sample said 100%.** The stride
finds a second family, every member red-tinted with `R > G ≈ B`: `(36,15,17)`,
`(82,28,24)`, `(140,102,102)`, `(255,236,236)`. `0x240F11FF` is E50's recorded
minimum and appears **twice** in 24 stride samples, so it is a real repeated
vertex, not an outlier.

### What is established

**The flash REPLACES the colour; it does not transform it.** A lerp toward white
preserves hue, so a baked blue `(49,87,177)` would stay blue-ish. Not one of the
541 baked vertices is achromatic and 75% of flashed ones are, with the greys
spanning 76..255 — the range of a lighting term. So **E55's own route-1 lerp
hypothesis is REFUTED**, and with it E50's inference that a per-vertex output
implies a per-vertex input.

**E49's option 1 is alive, at per-epoch granularity.** It said exactly: *"if it
is genuinely uniform it needs **one colour per epoch**, not per-vertex data — a
runtime override the emit can apply without touching the baked table."* Two
constant families across 273 vertices spanning many epochs is what that looks
like. The owner already computes lighting (E48), so feeding it a per-epoch
constant instead of the baked colour is:

- **no per-vertex data**, so E49's `static const` blocker does not apply;
- **exact**, because it hands the same lighting the same input the source does;
- **E32 keeps its measured −51,136** (E54) — emit cost unchanged.

### The next probe, and it is one build

**Record the epoch index alongside the colour.** The hypothesis is now "the
flashed source colour is constant *within* an epoch", and the sample deliberately
crosses epochs, so mixed families are expected rather than contradictory. If each
epoch's samples are one value, the override is a per-epoch table and E32 is
unblocked at pixel parity — which is R2-03's own stated gate and needs no
subjective approval.

Note the reds' channel ratios are **not** constant (`G/R` 0.34..0.93), so they
are not one red material under one light. Two lights of different colour mixing
per normal would do it, as would texel modulation on textured runs. The epoch
index separates those.

**Standing lesson, twice over:** *a per-vertex output does not imply a per-vertex
input* (E50 sampled after lighting and attributed the variation to what precedes
it) — and *sample a stride, never a prefix*. The prefix here was 100% grey and
the population is 75%.

**Standing lesson:** *a per-vertex output does not imply a per-vertex input.*
E50 measured after a per-vertex transform (lighting) and attributed the variation
to the thing before it. When a probe reports "not uniform", ask which side of the
pipeline it sampled.

## R2-03 E56 — E26 re-measured post-E46, and the plan's own policy demotes it (2026-07-29)

`HANDOFF.md` has pointed every restart at E26 as "the best unowned work that
needs no owner decision", sized 26,944 by E43. E46 shipped after that, so the
number was re-taken on the current build with
`NDS_TASK91_DRAW_PHASE_CENSUS=1 NDS_R2_SPAN_LEAN_TIMING=1`, 128 frames, 920
presented:

| | E43 (pre-E46) | **now (post-E46)** |
|---|---:|---:|
| before-span | 26,944.3 | **23,844** |
| before deltas/frame | 134.5 | 136.8 |
| **ticks per delta** | 200.3 | **174.2** |
| after-span | 13,703.7 | 13,719 |
| after deltas/frame | 47.9 | 49.2 |
| **replay total** | 40,648 | **37,563** |

Delta counts are within 2% across the two arms, so the arms do the same work and
the −3,100 is E46's ITCM placement, as E46 claimed. **E26's target is 23,844, not
26,944 and certainly not the 33,708 the spec still quotes.**

### Why that demotes it

The switch plan's §3.9 noise policy: *"<10K ignore unless free; 10–20K usually
too small for architecture work; 20–50K consider if simple and exact; 50–100K
valuable; 100K+ major target."*

23,844 is at the **bottom** of the "consider if simple and exact" band, and E26
is exact but **not simple** — it needs a generator change, a per-epoch install
path, and E34-b's carve-out that `prim_color`/`env_color` and companions must be
left live. It also does not recover the whole 23,844: it replaces ~3 dispatched
writes per epoch with one bulk install, and the install is not free. E39 already
refuted the cheap variant (operand elision, 7.4% hit rate, ~3,700).

**E26 is not refuted — it is demoted.** Take it if the gate is close and it is the
last thing standing; do not open it as the headline while a measured 51,136
(E32) and a ~26,000 structural halving (E57 below) are unclaimed.

## R2-04 E57 — the visual pose is evaluated TWICE per presented frame (2026-07-29)

Exact call counts, free from the E53 control census (an entry PC retires once per
call), over four settled frames:

| symbol | calls/frame | ticks/frame |
|---|---:|---:|
| `gcRunAll` | **2.0** | 8,853 |
| `gcPlayAnimAll` | 10.0 | 7,860 |
| `gcPlayDObjAnimJoint` | **164.0** | **34,022** |
| `battleship_ftAnimParseDObjFigatree` | 104.0 | 12,115 |
| `ftParamUpdateAnimKeys` | 4.0 | 6,191 |

`gcRunAll` at exactly 2.0 confirms the structure the plan assumes: **60 Hz
gameplay, 30 Hz presentation, and the animation evaluation runs inside the
gameplay tick** — so every number above is paid twice per presented frame.
Animation evaluation totals **~52,000 ticks/frame**.

R2-04's charter is exactly this: *"Generated visual-pose evaluation feeding the
direct draw path, decoupled from the gameplay skeleton (§3.6): evaluated once per
presented frame (30 Hz), not per gameplay tick. Do not assume full cubic pose
evaluation must run twice per rendered frame because gameplay is 60 Hz."*
R2-04 E1/E5 delivered the animation *cache*; the *rate decoupling* is untouched.

**Upper bound ~26,000 ticks/frame, flat — it moves P50 and P95 equally**, which
is worth more than E26 and is a phase deliverable rather than a micro-cut.

### ANSWERED from source, and it refutes the free-win reading

`gm/gmcollision.c:489`, `gmCollisionGetFighterPartsWorldPosition`, is how a
hitbox becomes a world position:

```c
while (main_dobj != DOBJ_PARENT_NULL) {
    parts = ftGetParts(main_dobj);
    if (parts->unk_dobjtrans_0x5 != 0) {
        gmCollisionGetWorldPosition(parts->mtx_translate, vec);
        return;
    } else if (parts->transform_update_mode == 0) {
        gmCollisionTransformMatrixAll(main_dobj, parts, parts->unk_dobjtrans_0x10);
        parts->transform_update_mode = 1;
    }
    gmCollisionGetWorldPosition(parts->unk_dobjtrans_0x10, vec);
    main_dobj = main_dobj->parent;
}
```

**Hitboxes are placed by walking the live joint DObj chain and multiplying
through each joint's transform.** They are not `ftParam` tables keyed on
animation frame. So the odd tick's pose is load-bearing for hit detection:
evaluating once instead of twice moves every hitbox to the previous tick's pose
on odd frames. That is a **gameplay change** under `PROJECT_GOAL.md`'s sacrifice
order (items 3 and 4), not a visual one — the same class of decision as E35's
float→fixed, and not an unowned free win.

### And the renderer side is already at presentation rate

The corollary matters more than the refutation. §3.6's requirement is that the
*renderer* not re-derive an expensive second representation per gameplay tick —
and the same census says it does not:

| symbol | calls/frame | |
|---|---:|---|
| `ndsFighterMarioFoxDLAllDrawForSlot` | 2.0 | once per fighter per **presented** frame |
| `ndsRendererAdapterBuildDObjLocalMatrix` | 50.0 | 25 joints x 2 fighters, once per presented frame |
| `ndsRendererExecuteNativeFighterOwnerProduction` | 2.0 | once per fighter per presented frame |

**R2-04's rate-decoupling mandate is already satisfied on the renderer side.**
The ~52,000 of animation evaluation is gameplay-owned 60 Hz work living in the
`SRC` bucket, not renderer work in `FTR`. Anyone reading R2-04's charter as "we
still owe a 30 Hz pose" should read this row first: what is left of that charter
is the `SRC` owner decision, already stated.

## R2-03 E55 — E49's "structurally out of reach" is too strong (2026-07-29, unbuilt)

E49 concluded that because `sNdsNativeFighterDenseVertices` is `static const`,
**every** approach reading generated vertex data cannot show the flash, and
retired the family. That conclusion is correct about the *baked table* and wrong
about the *owner*, and the difference matters because E54 has now priced E32 at
−51,136 P95 — the largest measured lever left.

Where the generic path's colour actually comes from (`nds_renderer.c:6679`):

```c
ndsRendererDecodeInputVertex(input, src + (i * 16u));
state->vertex_colors[index] = ndsRendererHardwareLitShadeColorPrepared(
    stats, input, prepared_light_direction);
```

`state->vertex_colors[]` is **not a raw colour** — it is the lit shade of the
*live* source vertex, decoded from `src`, the loaded asset's vertex bytes. The
hurt flash rewrites those bytes. So the value E48 saw at
`ndsRendererHardwarePackedResolvedColor` is `LitShade(live source vertex)`, and
the native owner's disagreement is that it computes `LitShade(baked constant)`.

**The owner is not barred from the flash; it is missing a transform.** Three
routes, in increasing cost, and none of them is the retired "read the table"
family:

1. **The flash is a per-vertex transform of the base colour.** E50's own numbers
   fit it: range `0x240F11FF`..`0xFFFFFFFF`, first `0x4C4C4CFF`, 172/273
   differing — that is what a lerp toward white over differing base colours looks
   like, not an arbitrary repaint. If `live = lerp(baked, white, t)` for one
   per-frame `t`, the owner reproduces it exactly with one lerp per vertex and no
   memory traffic, and E32's tick win survives intact.
2. **Read the live source vertex.** Bake `dense_id -> (asset slot, byte offset)`
   in the generator and have the emit decode the live 16 bytes on flash frames
   only. Exact by construction — same bytes, same function — but it costs the
   generic path's per-vertex work on exactly the frames E32 is trying to make
   cheap, so it is close to E49's option 2 in a different dress.
3. E49's option 3, approximate by another mechanism, unchanged.

**Probe route 1 before building anything.** On a hitlag frame, record for the
same `dense_id` both the baked `rgba` and the live `state->vertex_colors[]`, and
test whether a single `t` reproduces every pair. That is one build and it either
hands E32 back its −51,136 or eliminates the cheapest remaining route. The E48
probe already latches per-vertex values on frame 911 and is the natural host.

This defect has now cost eight experiments, six of them reasoned rather than
measured. **Do not build route 1 without the pair dump.**

## R2-03 E54 — it IS the fighter falling back, and E32 is worth 51,136 (2026-07-29)

E53 found 292,899 ticks/frame of generic display-list interpreter appearing from
zero on excursion frames and named two candidate causes: the native fighter owner
falling back (E31/E32), or a third owner drawing generically (E35's reading).
**It is the fallback.**

`NDS_TASK68_FALLBACK_CENSUS=1` plus `NDS_TASK91_DRAW_PHASE_CENSUS=1`, 128 frames:

```
native-owner: 256 draws, 256 eligible, animLock-reason fallbacks: 5
gNdsR2FallbackShuffleTics = 25 (cumulative)   gNdsR2FallbackAnimLocks = 0
```

**Five fallbacks, every one of them `shuffle_tics`, zero animation locks.** The
reason code is shared by both halves of the `is_use_animlocks || shuffle_tics`
disjunction at `reloc_backend_renderer_dl.c:12275`; Task 91's split settles which.

The census ROM is ~137,664 ticks/frame slower than the clean one and its VBlank
histogram shifts 2:726→2:314, so **presented frame N is not the same game tick in
both builds** and the fallback frame list cannot be mapped across. The clean
build's own `FTR` column settles it without any alignment:

| clean-build frames with `FTR` > 500,000 | `FTR` | excess over median | over gate |
|---|---:|---:|:-:|
| 909 | 898,048 | +509,824 | yes |
| 910 | 896,448 | +508,224 | yes |
| 911 | 898,368 | +510,144 | yes |
| 912 | 895,616 | +507,392 | yes |
| 913 | 886,848 | +498,624 | yes |

**Exactly five frames, exactly five fallbacks, and they are consecutive** — one
hitlag burst. 909–913 are also the frames E53 profiled. `FTR` median is 388,224,
so a fallback costs **~507,000 ticks/frame**, of which E53 attributed 292,899 to
twelve symbols that are zero when the native owner runs.

### What E32 is worth, across the whole distribution

Capping `FTR` at its median on those five frames (E35's projection method, but
over all 128 frames — never the visible top):

| | P50 | P95 | max | over gate |
|---|---:|---:|---:|---:|
| as measured | 1,013,696 | 1,228,928 | 2,040,896 | 17/128 |
| **`FTR` capped** | 1,011,264 | **1,177,792** | 1,531,072 | **13/128** |

**E32 is worth −51,136 P95 and four over-gate frames.** It halves the gap
(108,928 → 57,792) and does not close it, which corroborates E35's verdict at a
different measurement. The twelve frames that remain over gate — 795, 809, 842,
843, 864, 869, 885, 890, 898, 899, 901, 907 — are the `SRC` half, the owner's
float→fixed decision.

**E32 is therefore the largest single lever left and it is blocked on the hurt
flash, not on its value.** The value is now measured rather than projected from
the top of the distribution.

### E53's lookup is a symptom of this, not a second problem

`ndsRendererOwnerHashStablePointer` (`reloc_backend_renderer_dl.c:5040`) calls
`ndsRelocFindLoadedFileContaining` on every display-list pointer it hashes, and
that hash only runs when the generic path walks a display list. That is why the
lookup goes 39 → 106.5 calls/frame and 1.49 → 16.87 entries deep on exactly the
frames the fighter falls back: it is hashing a *wider set of loaded files*
because it is walking lists the native owner never touches.

**So E53's 34,644 is inside E32's ~507,000, not additive.** Fixing the fallback
removes the lookup cost with it. Do not count them separately, and do not
re-open the lookup as an independent target — E53 already measured that
optimising it in place loses to its own placement cost.

### A per-epoch fallback is not a small change

E50 closed E32's fix family on the premise that the native owner must
*reproduce* the hurt flash, and E54's "the flash frames are the fallback frames"
suggests a cheaper option: let the owner handle the shuffle and drop only the
flash *epoch* to the generic path, which would be pixel-identical and need no
visual approval. It is not available cheaply. The owner executes a flat
root → epoch → run walk over generated tables (`sNdsNativeFighterEpochs`,
`NDSNativeEpoch` at `nds_renderer.c:3763`); the generic path carries its own
`NDSRendererTraversalState` and display-list cursor. Interleaving them mid-draw
means reconstructing the generic traversal state at the owner's current position.
Recorded so the idea is not re-derived as though it were easy.

### Harness defect found and fixed

`sample-tick-hud-buckets.ps1` summed the enum's last two entries into the
fallback total. Task 73's `AnimForceLoad`/`AnimForceResident` ride along on that
counter bank because it already had plumbing, and they are **not** native-owner
reasons — the enum says so in a comment. The run above reported **"23 fell back
(9.0%)" and "16 frames with a fallback"** for a window whose real answer is
**5 and 5**, with `animLoad:18` dominating the breakdown and pointing squarely at
animation residency instead of at the hitlag shuffle that was actually firing.
The summary now excludes them and prints them separately as what they are.

Evidence: `artifacts/performance/r203-e54-fallback-census-128{.json,-rows.csv}`
and `-summary.txt` (which preserves the pre-fix wording).

## R2-03 E53 — the excursion is a RENDERING PATH SWITCHING ON (2026-07-29)

**The most useful profile the phase has taken.** E52 said the P95 excursion is
half `FTR`, half `SRC`, and that the `FTR` half is a few frames at 2.3x rather
than a drift. This profiles one of those runs against a matched control and says
what the 2.3x is.

Two `NDS_TASK37_PROFILE=1 NDS_TICK_HUD_DRAW=0` ROMs, four frames each:
**excursion 910–913** (over gate in two independent 128-frame runs) against
**control 876–879** (below median in both). Gross delta 420,227 ticks/frame;
`armWaitForIrq` accounts for 12,380 of it, leaving **+407,847 of work**.

Twelve symbols are **exactly zero on the control frames**:

| symbol | excursion ticks/frame |
|---|---:|
| `ndsRendererHardwareSubmitVertex` | 96,238 |
| `ndsRendererSubmitHardwareTriangle` | 51,037 |
| `ndsRendererScanList` | 50,913 |
| `ndsRendererHardwareBeginTriangleBatch` | 19,540 |
| `ndsRendererAdapterPrepareInitialMatrices` | 12,217 |
| `ndsRendererHardwarePackedVertexColor` | 11,642 |
| `ndsRendererAdapterPrepareMaterialSegment` | 11,098 |
| `ndsRendererDecodeInputVertex` | 10,601 |
| `ndsRendererAcquireCurrentMatrixSnapshot` | 8,830 |
| `ndsRendererInitTraversalState` | 7,852 |
| `ndsRendererHardwareResolveOrBindTexture` | 6,680 |
| `ndsTaskmanArenaBytes` | 6,251 |
| **total** | **292,899** |

That set is the **generic display-list interpreter**. It is not more of the same
work — it is a second renderer running. 292,899 is 72% of the whole excursion and
**12x anything else on this board's unowned queue.**

### The likely cause is already written down, and must still be confirmed

`reloc_backend_renderer_dl.c:12275` disables the native fighter owner, per
fighter, when `is_use_animlocks || shuffle_tics != 0`. E32's census counted
**5 shuffle fallbacks and 0 animlock fallbacks** over frames 460..500, so on the
shipped build (`NDS_R2_FIGHTER_SHUFFLE_FOLD=0`) the trigger is **hitlag**, and
E32's fold is precisely the repair — parked since E48/E49/E50 on the hurt-flash
regression.

**Do not act on that without the counter.** E35 saw a smaller (66,498) version of
the same symbol set and read it as "a third owner drawing" — an extra effect
object, not the fighter falling back. Both stories predict generic-renderer
symbols appearing from zero and they need different fixes.
`gNdsR2FallbackShuffleTics` and `gNdsR2FallbackAnimLocks` already exist behind
`NDS_TASK91_DRAW_PHASE_CENSUS`; one build reading them on frames 910–913 settles
it. **That is the highest-value unowned row on this board.**

### Second finding: a 160-byte lookup at 34,644 ticks/frame

`ndsRelocFindLoadedFileContaining` linearly scans `NDSRelocLoadedFile` records
behind a one-entry MRU. The record is **304 bytes, 256 of them
`extern_file_ids[64]`**, and the scan compares two fields — `data` and
`data_size` — so every iteration strides 304 bytes to read 8.

| | excursion | control |
|---|---:|---:|
| ticks/frame | **34,644** | 3,635 |
| calls/frame | 106.5 | 39.0 |
| **entries scanned per call** | **16.87** | **1.49** |
| leading loop load | 9.35 cyc/insn | — |

**The one-entry MRU thrashes.** On control frames it hits and the scan is 1.5
deep; on excursion frames the call rate nearly triples *and* the depth goes 11x,
so the cost goes 9.5x. Same mechanism as the finding above — the generic path
touches a wider set of loaded files.

### The mirror was built and is REFUTED

`NDS_R2_RELOC_EXTENTS` gave the scan an 8-byte `{base,size}` mirror so the same
17 entries walk 136 contiguous bytes instead of seventeen lines over 5 KB — same
entries, same order, same predicate, same first match, kept in step at the three
sites that can move it (register / reset / AObj16 compaction). Matched A/B: same
source vintage, adjacent build directories, identical 128-frame window 793..920.

| | control (`=0`) | candidate (`=1`) | delta |
|---|---:|---:|---:|
| `WORK-H` P50 | 1,013,696 | 1,018,240 | **+4,544** |
| `WORK-H` P95 | 1,228,928 | 1,240,512 | **+11,584** |
| frame-aligned median | — | — | **+4,352** |
| frames worse / better | — | — | **92 / 36** |

**`STG` moved +1,600 on 99 of 128 frames.** A reloc-lookup change cannot touch the
stage bucket, so that is the 768 bytes of new BSS displacing other data — and it
costs more than the mirror saves. `FTR` +3,544 on 119 of 128 says the same.
Reverted; `reloc_backend_assets.c` and the Makefile are byte-identical to HEAD.

**Why it lost even though the profile was right.** The 34,644 exists on ~14% of
frames. On the other 86% the scan is 1.49 deep and the whole function costs
3,635/frame — so there was at most 3,635 to win there, against a placement
penalty that applies to *every* frame including the P95 ones.

Evidence: `artifacts/task37-census/r203-e53-{excursion,control}/`,
`artifacts/performance/r203-e53-{cand,ctlb}-128{.json,-rows.csv}`.

### Two standing rules this earned

1. **A fix aimed at the tail must not add cost to the body.** This build's
   placement noise floor is 5,000–7,000 ticks, and that floor *is* the price of
   adding data. 768 bytes of BSS is not free. Before optimising something that
   only appears on N% of frames, multiply the win by N and compare it to the
   noise floor — if it does not clear it, the experiment cannot be read even if
   the mechanism is real.
2. **Never frame-align two builds across an excursion.** The over-gate frames in
   this A/B showed `WORK-H` −119,744 and `SRC` −78,724, which reads as a huge
   win and is an artefact: `SRC` cannot be affected by this change, and the two
   runs' excursions land on different frame indices because pacing diverges. On
   flat frames frame-alignment is the sharpest instrument available; across an
   excursion only the percentiles are valid. Both were computed here and only
   one of them means anything.

## R2-03 E51 — the MP line scan is O(1) already: REFUTED (2026-07-29)

E35 named a collision block worth 75,088 ticks/frame on `SRC` excursion frames.
Reading `src/port/reloc_backend_mp_collision.c` turned up what looked like the
structural defect in it: **three** functions answer a question about a `line_id`
the same way — `ndsMPGetLineKindForLineID` (kind), `ndsMPFindLineEndpoints`
(endpoints), `ndsMPFindLineYakumonoID` (owning yakumono) — each scanning
`i < yakumono_count` groups, capped at 64, by `nMPLineKindEnumCount` kinds, on
every call, from roughly **fifty** call sites, several inside candidate loops.
One precomputed `line_id -> (group, kind)` table built at the existing
`ndsStageCollisionLoopPrepareRuntime` seam replaces all three, bit-exactly, and
is textbook `PROJECT_GOAL.md` "compute once, not every frame".

`NDS_R2_MP_PROBE=1` bracketed all three and counted group iterations. One
128-frame run, frames 793..920, against the graduated R2-04 E5 build:

| counter | value | per frame |
|---|---:|---:|
| `gNdsR2MPScanTicks` | 12,480,896 | 13,566 |
| `gNdsR2MPScanGroups` | 45,214 | 49.1 |
| `gNdsR2MPKindCalls` | 9,172 | 10.0 |
| `gNdsR2MPEndpointCalls` | 18,366 | 20.0 |
| `gNdsR2MPYakumonoCalls` | 17,676 | 19.2 |
| **`gNdsStageCollisionLoopYakumonoCount`** | **1** | — |
| **`gNdsStageCollisionLoopTotalLineCount`** | **7** | — |

**9,172 + 18,366 + 17,676 = 45,214, which is `ScanGroups` exactly.** Every call
scans one group, because Dream Land's collision geometry *has* one group and
seven lines. The 64-group bound is a defensive clamp on data that never
approaches it. There is no O(n) to remove; the table would replace a
one-iteration loop with an array index.

**Residual, for the record.** 12,480,896 / 45,214 = 276 ticks per call; net of
E43's ~50-tick instrument, ~226. At 49.1 calls/frame the whole family costs
**~11,300 ticks/frame**, about 1% of the frame — and the scan is not why. 226
ticks for at most four range compares points at the bodies instead: every one of
the three re-runs `ndsStageCollisionLoopGeometryReady()` (seven pointer tests) on
entry, and the endpoint variant does six `ndsMPO2RReadU16Kernel` byte-swapped
reads plus float conversions. Not worth pursuing at that size, but that is where
it would be if someone ever needs the 11,300.

**This also settles what E35's collision block is.** ~11,300/frame here against
75,088/frame there means the block is `gmCollision*` matrix work, not MP line
lookup — corroborating E35's softfloat headline rather than offering an
alternative to it.

Probe reverted; `src/port/reloc_backend_mp_collision.c` is HEAD plus a five-line
comment recording the trip count so the table is not re-proposed. Evidence:
`artifacts/performance/r203-e51-mpscan-refuted-{128.json,128-rows.csv,counters.txt}`.

### Standing rule this earned

**A loop's declared bound is not its trip count.** E48's rule said a "which path
does this take" question is a measurement, not a reading; this is the same rule
for "how many times does this run". `min(yakumono_count, 64) * 4` reads as a
256-iteration worst case and is a 4-iteration actual one, and no amount of
careful source reading would have said so — only the counter did. Recorded in
`docs/optimization/TASK_STANDING_RULES.md`.

## R2-03 E52 — post-E5 the excursion is HALF fighter, half simulation (2026-07-29)

E35 concluded "25 of the 26 remaining over-gate frames are `SRC` excursions".
That was measured before R2-04 E5 graduated, and E5 removed the on-demand-loading
component E35 had sized at ~49,536. Re-decomposing on the graduated build changes
the conclusion, so it is recorded rather than left to be re-derived.

128-frame ring dump, frames 792..919, `WORK-H` P50 **1,015,872**, P95
**1,232,640**, gate 1,120,000, **18/128 frames over**. The eighteen over-gate
frames against the eighteen frames centred on the median:

| bucket | median 18 | over-gate 18 | delta | share of excursion |
|---|---:|---:|---:|---:|
| **FTR** | 387,847 | 528,836 | **+140,988** | **50.0%** |
| **SRC** | 339,378 | 474,738 | **+135,360** | **48.0%** |
| MISC | 85,109 | 88,996 | +3,886 | 1.4% |
| AUD | 1,276 | 5,138 | +3,861 | 1.4% |
| BG | 4,011 | 4,025 | +14 | 0.0% |
| STG | 181,461 | 179,029 | −2,432 | −0.9% |
| `WORK-H` | 1,015,275 | 1,297,184 | +281,909 | 100% |

Over-gate frames: 809, 842, 843, 864, 869, 885, 890, 898, 899, 901, 907, 909,
910, 911, 912, 913, 924, 926.

**`FTR` is flat and then spikes.** Its own percentiles on the same window are P50
388,096, P95 392,192 — spread 1.01 — with **max 903,168**. So `FTR` does not
drift upward on expensive frames; a handful of frames run it at 2.3x. Those are
E31's AnimLock/hitlag frames where the native owner is disabled and the generic
path runs the whole fighter, which is precisely what E32 fixes and E32 is parked
on the visual regression (E48/E49/E50).

**Consequence for the queue.** Both halves of the excursion are owner-gated:
`FTR` behind E32's fidelity decision, `SRC` behind E35's float→fixed decision on
`gmcollision.c`. The largest **bit-exact, ownerless** mechanism left is E26 —
the before-span fold, sized 26,944/frame by E43 — and its prerequisite is still
E45's open question: whether 186 ticks per delta is instruction-side. Measure
that before building the fold.

Evidence: `artifacts/performance/r204-e5-animcache-graduated-128{.json,-rows.csv}`.

## R2-03 E50 — the flash is NOT uniform; E32 is parked, SRC resumes (2026-07-29)

E49 left one cheap option alive: if the flash were a single colour across the
fighter, a per-epoch constant override would reproduce it without touching the
baked table. Measured on hitlag frame 911, over the same 273 vertices:

| slot | value | reading |
|---|---:|---|
| 8 min `vertex_color` | 604,967,423 | `0x240F11FF` — dark red-brown |
| 9 max `vertex_color` | 4,294,967,295 | `0xFFFFFFFF` — white |
| 10 first seen | 1,280,068,863 | `0x4C4C4CFF` — mid grey |
| **11 differing from first** | **172 of 273** | **not uniform** |
| B (ordinary frame) 2 / 11 | 0 / 0 | function never runs |

**63% of the fighter's vertices carry a different colour from the first.** The
constant-colour option is dead. Combined with E49's `static const` finding, every
fix that keeps the native owner on hitlag frames now requires per-vertex runtime
colour, which the generated table cannot supply by construction.

Worth noting for whoever picks this up: the minimum vertex colour `0x240F11FF` is
itself a dark red-brown, close to what the native owner's lit path produces. The
"dark maroon" may be the lit result landing near the bottom of the same range
rather than an unrelated wrong colour.

### E32 is parked, deliberately

Two options survive and **both need the owner**, because both trade appearance
rather than correctness:

1. Keep the generic fallback on flash frames only — E32 then covers the shuffle
   but not the flash, surrendering most of its win.
2. Approximate the flash by another mechanism (polygon alpha, a tint pass) —
   a fidelity-budget call and a visual approval, not a pixel match.

**Priority says stop here.** E32 is worth `WORK-H` P95 **−35,648** (1,232,640 →
1,196,992, over-gate 18 → 14). Capping `SRC` to its median is worth **−170,112**
(→ 1,062,528, over-gate 18 → **6**) and is the only lever measured to land the
1,120,000 gate. Eight experiments have gone into a −35,648 defect while the
gate-owning lever sat untouched. Returning to `SRC`.

## R2-03 E49 — DO NOT GRADUATE, and it proves the flash is structurally out of reach (2026-07-29)

Built E48's fix: epochs the generic path would draw from a raw vertex colour drop
`POLY_FORMAT_LIGHT0` and emit `GFX_COLOR` from the baked dense `rgba` at all four
emit sites. Flag `NDS_R2_UNLIT_VERTEX_EPOCH`, default 0 and staying there.

Top-screen diff against the reference, E48's frame pairs:

| frame | E47 arm | **E49 arm** | bounding box |
|---|---:|---:|---|
| 910 hitlag | 4,025 | **1,436** | x 147..223 y 142..197 |
| 911 hitlag | 4,104 | **1,539** | x 146..223 y 138..201 |
| 903 control | **0** | **780** | x 153..246 y 117..234 |
| 904 control | **0** | **838** | x 153..245 y 118..223 |

**Unlighting is directionally right**: the hitlag delta falls 64% and its bounding
box collapses from the whole screen to the fighter's flash region. **But the
control regressed from pixel-perfect to 780/838**, so the predicate also claims
ordinary epochs, where the native owner's lit appearance is the accepted one and
the generic path never runs to contradict it (E48: 0 calls on frame 904).

### Why the hitlag delta stops at 1,436 instead of going to zero

`src/nds/nds_native_fighter_owner.generated.inc` declares
`static const NDSNativeDenseVertex n[541]` — **the dense vertex colours are
compile-time constants.** The hurt flash is a *runtime* rewrite of the vertex
colours the display list feeds `state->vertex_colors[]`. No emit sourced from the
baked table can reproduce it, at any indentation. E49 therefore draws the fighter
unlit in its *un-flashed* colours: right shape, wrong values.

**This is the finding, and it retires a whole family of candidate fixes.** Every
approach that reads the generated vertex data is structurally incapable of
showing the flash. The remaining options are:

1. **A per-epoch constant colour.** The reference flash is a uniform white
   silhouette, so if it is genuinely uniform it needs one colour per epoch, not
   per-vertex data — a runtime override the emit can apply without touching the
   baked table. Cheapest, and the only one that keeps E32's tick win.
2. **Keep the fallback for flash frames only.** E32 then covers the shuffle but
   not the flash, and the `FTR` excursion survives on the frames where the flash
   is live. Costs most of E32's −35,648.
3. **Approximate the flash by another mechanism** (polygon alpha, a tint pass).
   Gates on the fidelity budget and the owner's eye rather than on a pixel match.

Option 1 is the one to price, and pricing it starts by measuring whether the
flash colour is actually uniform across the fighter's vertices on a hitlag frame
— which the E48 probe can answer by recording min/max of `vertex_color` instead
of a branch count. Measure before building; this defect has now cost seven
experiments, six of which were reasoned rather than measured.

## R2-03 E48 — E32's regression MEASURED after six wrong guesses (2026-07-29)

**The flash is a raw vertex colour, and the native owner lights it.** Measured,
not reasoned: `NDS_R2_FLASH_PROBE` counts which branch of the generic colour path
draws each vertex, per presented frame, latched at one hitlag frame and one
ordinary one.

| slot | frame 911 (hitlag) | frame 904 (ordinary) |
|---|---:|---:|
| 0 material-only | 0 | 0 |
| 1 no-vertex → `RGB15(31,31,31)` | 0 | 0 |
| **2 resolved** | **273** | 0 |
| 3 lit-shade recompute | 0 | 0 |
| 4 total calls | **273** | **0** |
| 5 last material colour | **0** | 0 |
| 7 last flags | **2** | 0 |

Flags `2` is `use_vertex_color = TRUE, use_material_color = FALSE`, and slot 3 is
zero, so all 273 arrived through the `vertex_color_valid != FALSE` entry. The
generic path therefore computes

```text
PackedResolvedColor(vertex_color, material = 0, use_material = FALSE)
  -> RGB15(vertex_color >> 27, >> 19, >> 11)
```

— the vertex colour **raw**. No material, no shade, no combination.
`ndsRendererHardwarePackedValidVertexColor` never calls `LitShadeColor` on that
route: **a valid vertex colour suppresses lighting.** The native owner has no
such rule. It decides `epoch_lit` from `geometry_mode & LIGHTING` alone and runs
the hardware lighting engine, which is where the dark maroon comes from.

The reference flash is **saturated white**, not the light grey this board and the
E32 report both recorded — the source writes white vertex colours and the generic
path emits them unchanged.

`gNdsR2FlashSnapB` being **all zero** is a second result worth keeping: on
ordinary frames this function is never called at all, because the fighter is on
the native owner and the stage is on the Task 36 replay. 273 against 0 is
independent confirmation that the fallback fires on hitlag frames and nowhere
else.

### Six hypotheses, six builds, six wrong

E36 `color_modulate`; E41 the fold arithmetic, then E16's hardware lighting; E42
`USE_VERTEX`; E47 the material derivation; and E48's own stated prediction, which
was slot 1 (white) and was also wrong. Every one asked *how the material combines
with the shade*. On these frames there is no material and no shade, so the
question was mis-framed from the start, and no amount of source reading was going
to correct it — only counting the branch did.

**Standing consequence:** E45's rule ("prefer one direct bracket over any amount
of algebra") was written for tick questions and never generalised. It applies to
any question about which path the code takes. Recorded in
`TASK_STANDING_RULES.md`.

### The fix, and its real scope

The native owner must reproduce the precedence: an epoch whose vertices carry
valid vertex colours and no material emits those colours **unlit**. Under
`NDS_R2_FIGHTER_HW_LIGHT` — `override`-forced to 1 on the hwtri targets — the
per-vertex software colour loop is *compiled out*, not skipped, and E29 removed
the `packed_color` field it wrote, so there is currently no path in the native
owner that emits a vertex colour at all.

Cheapest correct shape: mark such epochs, and in the emit write `GFX_COLOR` from
the baked `sNdsNativeFighterDenseVertices[].rgba` with the polygon attribute's
light mask cleared, instead of `GFX_NORMAL`. That is *less* work than lighting,
not more — but it touches the ITCM-resident emit, which is the hottest code in
the frame, so it is an implementation task rather than a one-line correction.

`NDS_R2_MATERIAL_DYNAMIC` (E47) is refuted and must graduate or be deleted with
this fix; it currently fixes nothing.

## R2-04 E4/E5 — GRADUATED, −132,352 WORK-H P95, Boundary green (2026-07-29)

E3 said the whole 301-ID animation space does not fit the arena and that the
answer was to preload the set the match actually uses. E4 measured that set and
E5 fixed how it is delivered.

### E4 — the working set is 41 assets, 91,104 bytes

`gNdsR204AnimSeen` dumped at frames 1801..1928: `Total=230, Distinct=41,
Repeat=189` — **82.2%** repeats, up from 64.6% at frame 928, so the set is
converged, not still growing. Decoding the bitmap gives 14 Mario and 27 Fox
animations totalling **91,104 bytes: 12.5%** of the 728,064 the full space would
need. That fits with room to spare.

The list is in `sNdsR204AnimWarmList[]` and is derived from observed play, so
`gNdsR204AnimForceLoadRepeat / Total` is its own regression check: if that ratio
falls, the list has drifted from what the match uses. An asset missing from it is
a performance outcome, never a correctness one — it simply takes the on-demand
path it takes today.

With the list resident: **`gNdsR2AnimCacheMisses` 29 → 2**, and both survivors
are pre-battle loads that happen before the warm walk is armed. No gameplay
frame loads an animation.

### E5 — a prepare-at-load burst is bounded by the BGM packet, not by generosity

E4 loaded all 41 in one call at `scVSBattleStartBattle`. Boundary refused the
build, and the failing run's own audio telemetry named the mechanism:

| field | control | E4 |
|---|---:|---:|
| `gNdsAudioBgmSeamMissCount` | 0 | **1** |
| `gNdsAudioBgmErrorStopCount` | 0 | **1** |
| `gNdsAudioBgmOverrunCount` | 0 | **1** |
| `gNdsAudioBgmPlaying` | 1 | **0** |
| `gNdsAudioBgmStopCalls` | 0 | 0 |

Playback stopped without anyone calling stop. The stream is double-buffered at
8,196 bytes per packet against 44,100 bytes per second, so the main thread owns a
hard **~186 ms** budget between buffer seams; 41 back-to-back NitroFS walks plus
84 KB of cartridge reads do not fit inside it, and missing one seam kills BGM for
the rest of the match. Cache off passes 4/4, so this is causal, not a flake.

E5 arms the walk at battle start and steps **one asset per
`scVSBattleFuncUpdate`**. The countdown is far longer than the 41 frames this
needs, and a stepped frame costs exactly what the on-demand path already costs
when a fighter changes action — which demonstrably does not miss a seam.

### Result, frames 802..929

| bucket | control | E1 | E4 burst | **E5 stepped** |
|---|---:|---:|---:|---:|
| **WORK-H P95** | 1,364,992 | 1,311,360 | 1,236,096 | **1,232,640** |
| delta vs control | — | −53,632 | −128,896 | **−132,352** |
| `gNdsR2AnimCacheMisses` | — | 29 | 2 | 2 |
| `gNdsR2AnimWarmLoaded` | — | — | 39 | 39 |

E5 reproduces E4's gain (the 3,456 difference is inside the 5,000–7,000
build-placement noise floor) and is **2.5× E1's**. Read `WORK-H`, not `WORK`:
E5's raw `WORK` P95 is 1,363,840 because one frame in the window took a 333,760
`HUD` excursion (`HUD` spread 325.94), and `WORK-H = WORK − HUD` is exactly the
series that removes the instrument.

Pacing cost of stepping is visible and confined to the ramp: VBI 3-intervals
118 → 186 over the whole 929-frame run, ~41 of which are the stepped loads during
the countdown. The burst arm paid the same work as one ~1-second stall instead.

**Boundary passed** with `NDS_R2_ANIM_CACHE=1` in both the published and
tick-HUD blocks, so the flag is graduated and default-on there. That also closes
E1's separate block: the lower-screen FPS-counter assert that refused the
cache-only arm 2/2 did not fire here. It is not explained, only no longer
reproducing — see `HANDOFF.md` for the open question and the shadow probe built
for it. The `Pupupu locked-30 presentation slipped` warning is pre-existing and
appears in the control and in the failing E4 run alike.

### Standing consequence

Recorded in `TASK_STANDING_RULES.md`: **prepare-at-load work on a live scene seam
is bounded by the BGM packet duration, not by loading-time generosity.** "Loading
time is cheap" is true of a loading *screen*; it is not true of a seam where the
music is already streaming. Anything longer than one packet has to be stepped.

### Known gap, not reachable in this milestone

The cache holds `syTaskmanMalloc` pointers and is never reset, so a second match
in one boot would hand out pointers into a torn-down heap. The milestone boots
directly into one match and has no rematch flow (`PROJECT_GOAL.md`, out of
scope), so this is not reachable today. Whoever adds match restart owns clearing
`sNdsR2AnimCacheCount` at the same seam that tears the arena down.

## R2-04 E1 — animation cache BUILT, −53,696 WORK P95, BLOCKED on Boundary (2026-07-29)

E0's plan built and behaving exactly as sized. `NDS_R2_ANIM_CACHE=1` keeps each
animation's **byte-swapped, pre-fixup** payload keyed by `asset_id` and re-runs
the fixups against the real destination, which preserves `lbRelocGetForceExternHeapFile`'s
"force" semantic — pristine data restored — while removing the NitroFS walk and
the cartridge read.

Engagement, frames 801..928, matching E0's window exactly:

| counter | value |
|---|---:|
| `gNdsR2AnimCacheHits` | **53** |
| `gNdsR2AnimCacheMisses` | 29 |
| `gNdsR2AnimCacheFills` | 29 |
| `gNdsR2AnimCacheBytes` | **66,016** |
| `gNdsR2AnimCacheRejects` | **0** |

53 hits against E0's 53 predicted repeats and 29 fills against 29 distinct: every
repeat served, nothing rejected, 66 KB resident.

| bucket | control | E1 | delta |
|---|---:|---:|---:|
| **WORK P95** | 1,365,952 | **1,312,256** | **−53,696** |
| WORK-H P95 | 1,364,992 | 1,311,360 | −53,632 |
| **fallback WORK-H median** | 1,284,928 | 1,229,632 | **−55,296** |
| clean WORK-H median | 1,013,376 | 1,011,712 | −1,664 |
| WORK P50 | 1,019,776 | 1,017,728 | −2,048 |
| VBI 2: / 3: | 793 / 128 | 801 / 121 | — |

**The gain is entirely on the excursion frames** (−55,296) with clean frames
essentially unmoved (−1,664). That is the shape R2-04's gate asks for, and P95 is
the metric `PROJECT_GOAL.md` gates on.

### BLOCKED: Boundary fails with the flag on

**`battle_playable lower-screen rolling FPS counter did not sample actual
presentation cadence.`** `FPS_HUD=290,13,15,17485504` — the harness recomputes
288 from the HUD's own `frames`/`ticks` inputs and the HUD reports 290.

Reproduced deliberately rather than assumed: **two runs with the flag on fail,
one clean control run with it off passes** (`Boundary verification profile
passed`). A third run, the control taken through the harness script directly
rather than `verify-all.ps1`, failed on an unrelated blank-capture
(`0/49152 dominant-green pixels`) and is a flake, not evidence — noted so the
next reader does not count it as a second control.

The likely mechanism is that the assert is an internal-consistency check between
the HUD's rolling value and an instantaneous recomputation, and this cut makes
the frame rate **non-stationary** — the first 29 loads are slow misses, then the
match speeds up — so a rolling average legitimately disagrees with a spot
recompute. That would make the assert an artifact rather than corruption. **It is
not graduated on that theory.** The flag stays default 0 until someone shows
which of the two is wrong; a verifier failure is a failure.

### E2 — causation is firm and BOTH explanations are refuted

**Causation, measured rather than assumed:** flag on **2 of 2 runs fail**; flag
off **3 of 3 runs pass** (`Boundary verification profile passed`). The one control
that failed differently — a blank capture, `0/49152 dominant-green pixels`, from
running the harness script directly instead of through `verify-all.ps1` — is a
flake and is **not** counted as a control.

**Refuted #1 — "non-stationary rate".** `nds_platform.c:2236-2239` writes all four
HUD fields adjacently from locals, and `fps_x10` is computed from exactly the
`elapsed_frames`/`elapsed_ticks` published beside it. Only two writers exist (that
group and the reset at :2145-2148, also a group). The harness reads all four in
one GDB `printf` at a breakpoint, so the read is atomic too. The assert therefore
holds across *any* cadence change, and a rolling-versus-spot mismatch cannot
happen. This theory was wrong.

**Refuted #2 — "the harness `BUS_CLOCK` constant is stale".** `NDS_R204_FPSHUD_SHADOW`
publishes a shadow of the same locals in the same breath. Sampled state:

| | primary | shadow |
|---|---:|---:|
| fps x10 | 265 | 265 |
| frame window | 14 | 14 |
| tick window | 17,721,728 | 17,721,728 |

`gNdsR204FpsHudShadowBusClock = 33,513,982`, identical to the harness constant,
and recomputing gives exactly 265. **The publish path is self-consistent and the
constant is right.**

So the observed `FPS_HUD=290,13,15,17485504` — where the recompute is 288 — is an
**intermittent** state that the probe did not catch, and it is not explained by a
wrong constant, a non-atomic publish, or a rate change.

### What to do next

1. **Read the shadow where the verifier reads the primary.** The probe above ran
   under `sample-tick-hud-buckets.ps1`, which stops at its own breakpoints and
   sampled a healthy sample; the anomaly belongs to the verifier's stop. Add the
   four shadow globals to the `FPS_HUD` printf at
   `verify-battle-mariofox-gcrunall-loop-harness.ps1:2100` and re-run with the
   flag on. If shadow and primary disagree *there*, something rewrites the
   primary between publish and that stop, and the writer is the bug.
2. **Independently, do E0's other half regardless: preload the working set at
   match start.** It is what R2-04 actually specifies, it takes the removable
   share from 64.6% to 100% rather than leaving 29 misses, and whatever this
   assert is reacting to, a match whose animation set never loads mid-gameplay
   cannot trigger it. **Read E3 below first — the obvious form of it does not
   fit.**

## R2-04 E3 — "preload everything" does not fit, measured (2026-07-29)

Before building the preload, the budget. The animation assets are individual
files under `nitro:/reloc/reloc_animations/`
(`nds_reloc_assets.c:130`/`:175` synthesise the paths), so their sizes are
readable off disk without running anything:

| set | files | bytes | avg |
|---|---:|---:|---:|
| `FTMarioAnim*` | 143 | 360,320 | 2,519 |
| `FTFoxAnim*` | 158 | 367,744 | 2,327 |
| **all** | **301** | **728,064 (711 KB)** | 2,419 |

**711 KB is not affordable.** `MEMARENA` reports a ~1.35 MB taskman arena and
`MEMRELOC` already accounts for 681,632 bytes of reloc data. Registering all 301
would also overrun `NDS_RELOC_LOADED_FILE_CAPACITY` (96) three times over. So the
literal reading of the phase bullet — *all* animation streams resident — is not
available on this hardware budget, and anyone starting from that sentence will
build something that cannot fit.

The measured working set is **29 assets / 66,016 bytes**, which fits trivially.
That gap is the whole design question, and it has two honest answers:

1. **Generated warm list.** Emit the match's actual animation set as a table and
   preload exactly it at `scVSBattleStartBattle` (`src/import/battleship_scvsbattle.c:133`,
   beside `ndsRendererHardwarePrepareBattleStaticTextures` and
   `ndsIFCommonNativeOamPrepareClouds` — the existing prepare-at-load seam, and
   port-side so it is editable). `PROJECT_GOAL.md` explicitly endorses
   compile-time asset conversion and heavy loading-time preparation. A miss still
   falls back to the on-demand load, so an incomplete list is a performance
   outcome, never a correctness one. Risk: the list is derived from observed play
   and a gameplay change silently drops coverage, so it needs the E0 counters
   kept as its regression check (`repeat/total` should stay at 100%).
2. **Budget-bounded eager fill.** Keep the cache lazy but give it a byte budget
   near 128–192 KB and let it hold whatever the match touches. That is what E1
   already does, minus the match-start warm, and it is why E1 works at all.

**Do not start by preloading the 301.** The cache's failure paths degrade safely,
so trying it would not corrupt anything — it would just reject nearly everything
and read as a null, and the reason would be this table.

## R2-04 E0 — the phase is SIZED and its gate is reachable (2026-07-29)

R2-03's remaining lever is unattributed (E45/E46 left ~110 ticks per delta with
no owner), while **R2-04 is untouched and its gate is the one currently missed**:
"SRC-class P95 excursions gone from the histogram", plus "absorbs Task 75: all
animation streams for the match prepared at load; no first-use loading during
gameplay". E35 already showed 25 of 26 over-gate frames are SRC excursions. This
sizes the phase.

### The excursions are animation loads, and they are separable

Tick-HUD census with `NDS_TASK68_FALLBACK_CENSUS=1 NDS_TASK75_LOAD_CENSUS=1`,
128 samples at two windows:

| window | fallback frames | fallback WORK-H median | clean WORK-H median | clean P95 |
|---|---:|---:|---:|---:|
| 439..566 | 13 of 128 | 1,329,280 | 983,936 | 1,408,128 |
| 801..928 | 17 of 128 | 1,284,928 | 1,013,376 | **1,117,248** |

Fallback reasons in the second window: **`animLoad` 19, `animLock` 5, every other
reason 0.** `animLoad` is `lbRelocGetForceExternHeapFile` re-reading a fighter
animation off the cartridge inside the frame that needs the move — NitroFS open,
cartridge read, word byte-swap, then internal/external pointer fixups, per call.

**In the 801..928 window the clean P95 is 1,117,248, under the 1,120,000 gate.**
So the phase's gate is not a distant target: it is what the histogram already
reads once the animation loads are removed.

### Why the existing counter said the opportunity was zero, and why that was wrong

`animResident` reads **0** in both windows. That counter asks whether the
*destination heap* already holds the asset, and the destination is caller-owned
and reused, so it almost never does. It refutes a destination-side residency
check — not the opportunity. The right question is how often the **same
animation** is force-loaded more than once, which a 128-frame (~4 s) window
cannot answer because Mario returns to Wait/Walk/Jump across the whole minute.

New lab counters (bitmap over the 301 Mario+Fox animation IDs), cumulative to
frame 928:

| counter | value |
|---|---:|
| `gNdsR204AnimForceLoadTotal` | **82** |
| `gNdsR204AnimForceLoadDistinct` | **29** |
| `gNdsR204AnimForceLoadRepeat` | **53 (64.6%)** |

**Two facts fall out, and they decide the implementation.**

1. **64.6% of the cartridge reads are repeats.** An asset-keyed cache removes
   them; the destination-keyed check that read 0 never could.
2. **The match's animation working set is 29, not 301.** The whole ID space is
   301 (`MARIO_ANIM_WAIT` 0x1f3..`MARIO_ANIM_FIRE_FLOWER_AIR` 0x281 = 143, plus
   `FOX_ANIM_FIRST` 0x282..`FOX_ANIM_LAST` 0x31f = 158), and
   `NDS_RELOC_LOADED_FILE_CAPACITY` is **96** — so preloading *everything* is
   impossible and was the obvious wrong turn. Preloading the working set is not:
   29 fits with room to spare, and it takes the removable share from 64.6% to
   **100%**, which is literally R2-04's "prepared at load, no first-use loading
   during gameplay".

29 is measured to frame 928, roughly two thirds through the 3,600-tick match, so
budget for growth — but the headroom against 96 is large enough that this is a
sizing note, not a risk.

### Implementation note for whoever builds it

Cache the **byte-swapped, pre-fixup** payload. `ndsRelocApplyInternalPointerFixups`
writes absolute pointers derived from `loaded->data`, so a fixed-up image is
position-dependent and cannot be memcpy'd to a different heap. Copy the swapped
image in, then re-run the fixups — that removes the NitroFS walk and the
cartridge read, which is the part Task 71 profiled, and keeps pointer correctness
by construction.

## R2-03 E46 — the delta path into ITCM: GRADUATED, −12,416 WORK P50 (2026-07-29)

E45 left ~186 ticks per delta application unexplained after eliminating the tile
republish (~23, E44), the invalidation macro (one store) and the span entry (~33,
E45). The census ELF names the remaining candidate outright:

| symbol | address | where |
|---|---|---|
| `ndsRendererNativeApplyStateDelta` | `0x01ff9934` | ITCM |
| `ndsRendererNativeApplyStateSpan` | `0x02003a14` | main RAM |
| `ndsRendererSyncTextureTile` | `0x02003ae8` | main RAM |
| `ndsRendererRecordSetTile` | `0x0200d4e8` | main RAM |

The switch was already ITCM-resident; the loop that calls it and every helper it
dispatches to were not. So all 134.5 before-span applications a frame left
zero-wait ITCM for icache-served main RAM and came back. `.itcm` had **2,912
bytes free** and the whole path is ~1,088.

`NDS_R2_DELTA_PATH_ITCM=1`, placement and nothing else:

| arm | before-span | after-span | total |
|---|---:|---:|---:|
| lean baseline | 26,944.3 | 13,703.7 | 40,648.0 |
| **delta path in ITCM** | **24,494.8** | **13,025.2** | **37,520.0** |
| | −2,449.5 | −678.5 | **−3,128.0** |

Delta counts identical across arms (134.5 / 47.9 / 46.4 epochs) — the same work,
fetched faster. Per delta the two spans agree at **−18.2 and −14.2 ticks**, two
independent populations, which is the cross-check that separates a real effect
from layout luck on a change that is *itself* a relocation. `.itcm` goes 29,856 →
30,872, **1,896 bytes still free**.

### Frame level: the bracket understated it 4x

−3,128 is below the 5,000–7,000 whole-frame placement noise floor, so the bracket
alone could not license a frame-level claim. The tick-HUD A/B over 128 frames
(439..566, `NDS_TICK_HUD_DRAW=0`, both arms built from the same tree):

| bucket | control | E46 | delta |
|---|---:|---:|---:|
| **FTR P50** | 404,672 | 392,640 | **−12,032** |
| **WORK P50** | 1,010,240 | 997,824 | **−12,416** |
| WORK-H P50 | 1,006,848 | 996,480 | −10,368 |
| FTR P95 | 913,152 | 904,384 | −8,768 |
| STG P50 *(untouched)* | 173,312 | 174,080 | +768 |
| SRC P50 *(untouched)* | 327,360 | 326,144 | −1,216 |

VBlank histogram `2:472 3:87 4:4 5+:2` → `2:476 3:85 4:3 5+:2`.

**The gain is 4x the state-span bracket** because the bracket only ever saw the
two spans. `ndsRendererNativeApplyMaterial` (27.7/frame) calls
`RecordSetTile`/`RecordSetImage`/`RecordLoadTlut`/`RecordSetTileSize`, and the
texture prepare calls `SyncTextureTile` once per run (46.4/frame) — all of them
relocated too. Per E40's rule the untouched buckets bound the noise at ±1,200, and
FTR moved 10x that.

`ALL` P50 is unchanged at 1,119,808 in both arms. That is expected and is **not** a
refutation: `ALL` is VBlank-quantized at 560,190/VBlank, and reading it as flat is
what previously killed four good levers. `WAIT` rises by roughly what `WORK` sheds,
which is what a still-VBlank-bound frame looks like when real work is removed.

**GRADUATED.** `override NDS_R2_DELTA_PATH_ITCM := 1` in both the published and
tick-HUD Makefile blocks; **Boundary green**. No visual approval is owed — unlike
E16 and E32 this changes no arithmetic, only where the instructions live, so the
output is identical by construction.

**It does not explain the 186.** Instruction fetch is confirmed as a real
component and it is ~16 ticks of ~186, under 10%. Four mechanisms are now
measured and none is the driver: republish ~23, entry ~33, fetch ~16,
invalidation ~1. Roughly 110 ticks per application remain unattributed, and that
residue — not any of these — is what E26's fold has to be sized against. The next
instrument is R2-00a's stall attributor rather than another guess.

## R2-03 E44/E45 — E26 re-scoped, and two wrong answers on the way (2026-07-29)

With E43's corrected 26,944 in hand, the question was where inside the
before-span it goes. Two candidates were built and measured; both are refuted,
and the second refutes an arithmetic shortcut that would otherwise have
mis-scoped E26 by 3x.

### E44 — deferring the tile republish: BUILT, worse, reverted

`ndsRendererSyncTextureTile` republishes 19 fields from the active tile and runs
on every SETTILE/TEXTURE/SETTILESIZE. Statically that is **43 of the before-span's
140 applications, spread over only 17 spans** — so 26 republishes are overwritten
by a later one in the same span before anything reads them. Nothing inside a span
reads what it publishes (the `Record*` writers read `texture_tiles[]` and the
`texture_*` scalars, never `texture_render_tile_*`), and the texture prepare calls
Sync again at `nds_renderer.c:12176` before it reads, so deferring to one call per
span is exact.

It engaged exactly as predicted — **68.2 deferred, 39.4 flushed, 28.8 republishes
actually removed per frame** — and lost:

| arm | before-span | after-span | total |
|---|---:|---:|---:|
| lean baseline | 26,944.3 | 13,703.7 | 40,648.0 |
| + deferred Sync | 26,286.9 | 14,852.7 | 41,139.6 |

Before −657.4, after **+1,149.0**, net **+491.6 worse**. Reverted.

**The number worth keeping: `SyncTextureTile` costs ~23 ticks per call**
(657.4/28.8). The republish is **3.7%** of the before-span. It is not the cost,
and the two `stats` fields the deferral needed cost more in the after-span than
they saved in the before-span.

### E45 — the per-span entry is not the cost either, and the fit that said so was unsound

E44's result was read as evidence that the cost is fixed per span rather than per
delta, and a two-equation fit over the two spans
(`A*spans + B*deltas = ticks`) solved to **A = 392.3 fixed, B = 63.0 per delta**,
putting **68.5% of the before-span in per-span entry** and capping E26 at 8,478.
The only substantial thing on entry is `ndsRendererNativeSourceBoundary`'s
`ndsRendererHardwareEndBatch()`, so that was bracketed directly:

- **78.3 boundary calls/frame** (the static estimate was 74.3 — the model's span
  count was right)
- 80.4 ticks per call *as measured*, but the bracket inflated the two span totals
  by 7,426/frame across those calls, so ~47 of the 80.4 is the timer read itself
- **real cost ~33 ticks/call, ~2,585/frame — 14% of the 18,466 the fit claimed**

**The fit was unsound and this is the lesson.** Two equations in two unknowns has
zero degrees of freedom: it always solves, and it cannot be checked. It forced a
single per-delta coefficient onto two populations that genuinely differ — the
after-span's deltas cost more because `ApplyMaterial` runs between the spans and
dirties what they touch — and the only place that difference could go was the
"fixed" term. E43 had just earned a rule about measuring instead of assuming; this
is the same mistake wearing arithmetic.

**Corrected picture of the before-span (26,944/frame, 134.5 deltas, ~47 spans):**
entry ~1,900, **applications ~25,000, i.e. ~186 ticks per delta.** It is per-delta
after all.

### What this leaves E26

E26's target is back to roughly the whole before-span, **~25,000/frame**, because
the fold removes the applications themselves. What 186 ticks per delta *is* remains
open — it is far more than the switch and stores account for, and the leading
untested candidate is instruction-side: `ApplyStateDelta` lives in the fighter's
code section while the `Record*` helpers are generic renderer code, so every
application may cross into cold code. The repo emulator models icache, so this is
measurable. **Measure that before building the fold** — and note ITCM has only
1,024 bytes free, so if it is icache the repair is not simply "move it".

Two facts already banked for the fold itself: **LOAD_BLOCK appears only twice in
the entire before-span program**, so the `texture_loads[]` history ring — the
hardest thing to reproduce in a bulk install — touches at most 2 of 47 spans and
those two can keep the replay; and **COMBINE alone is 30.7% of applications**
(43 of 140), which is 4 stores and a counter, the cheapest possible fold target.

## R2-03 E43 — the replay bracket was pricing its own instrument (2026-07-29)

E38 sized E26's before-span at **33,708/frame** and that is the number the phase
has been planning against. It is 20% wrong, and the reason was already written
down in `docs/HANDOFF.md` as an unquantified caution: `ApplyStateDelta` opens
with a per-delta census block — E20's identical-operand arrays and E25c's effect
histogram — and `ndsRendererNativeApplyStateSpan`'s loop carries a second one,
E20's frame-stamp check. Both sit **inside** E38's bracket and run 134.5 times a
frame on the before-span alone.

`NDS_R2_SPAN_LEAN_TIMING=1` keeps E38's brackets and compiles both per-delta
blocks out. Same ROM target, same window (tick-HUD, frames 439..919):

| arm | before-span | after-span | total replay |
|---|---:|---:|---:|
| census in bracket (E38) | 33,707.6 | 16,243.3 | 49,951 |
| **census out of bracket** | **26,944.3** | **13,703.7** | **40,648** |
| instrument | 6,763.3 | 2,539.6 | 9,303 |

Delta counts are identical across arms (134.5 / 47.9 / 46.4 epochs), so the arms
are doing the same work. The instrument prices out at **50.3 ticks/delta on the
before-span and 53.0 on the after-span** — measured independently and agreeing to
5%, which is the cross-check that this is the instrument and not a code-placement
artefact.

**Consequences.** E26's before-span target is **26,944, not 33,708**. The replay
is **40,648, not 49,951**. The real per-delta cost is 200.3 (before) and 286.1
(after), not 250.6 and 339.1. E26 is still the largest single mechanism left in
R2-03 and still worth building — it is sized 20% smaller, not refuted.

### Standing rule this earned

A timing bracket must not enclose its own instrument. These two census blocks
predate the brackets and were correct when they were only counting; E38 wrapped a
timer around them without noticing. Any bracket around a per-item loop should be
built with the per-item census compiled out, and if both are wanted they are two
arms, not one build. Recorded in `docs/optimization/TASK_STANDING_RULES.md`.

## R2-03 E42 — E32's dark fighter is NOT `USE_VERTEX`: BUILT, REFUTED (2026-07-29)

E41 left the E32 regression traced to the native owner's material-colour path and
owed a diff of it against the generic path. That diff has a real finding in it:
`ndsRendererHardwarePackedVertexColor` has a branch the native owner does not.
With `USE_MATERIAL` set and `USE_VERTEX` **clear** the generic path applies no
lighting at all and returns the material colour *alone* — it replaces the shade.
`ndsRendererNativeShadeProductionActions` reads only `USE_MATERIAL` out of
`policy->vertex_flags` and always multiplies the material *into* a lit shade, in
both the E16 hardware fold and the software loop. A hurt-flash material on such a
run would be tinted dark instead of replacing the body colour, which is exactly
the reported symptom, and it explained E41's oddest result — the software arm
being *further* from the reference rather than corroborating it.

**It is unreachable.** `sNdsNativeFighterDirectPolicies[4]` in
`src/nds/nds_native_fighter_owner.generated.inc:487` — every one of the four
families sets `NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX`. The branch cannot execute
for any fighter epoch, so it cannot be the divergence. Built and measured anyway
rather than argued from the table, because the table is generated:

| frame | E32 candidate vs reference | E42 candidate vs reference |
|---|---:|---:|
| 480 (hitlag) | 1,826 | **1,786** |
| 481 (hitlag) | 1,536 | **1,496** |

The 40-pixel move is not the fighter. E42-candidate against E32-candidate differs
by **33 pixels in a 7x9 box at x 47..53, y 11..19** — the bottom-screen FPS
readout, which differs between any two runs. Reverted; the file is byte-identical
to HEAD. Do not re-derive this: **the flat-colour branch is dead code for the
fighter and adding it back is speculative.**

### What this leaves for whoever returns to E32

Four candidate mechanisms have now been eliminated by implementation, not by
argument: `color_modulate`'s affine lerp (E36), the fold's own arithmetic and
hardware-vs-software lighting (E41), and `USE_VERTEX` (E42). What survives is the
**lit-shade input itself**: the generic path takes an early exit on
`vertex_color_valid` and uses a baked `vertex_color` *without lighting it*, where
the native owner always re-lights `sNdsNativeFighterDenseVertices[].rgba`. That is
the only remaining structural difference between the two colour paths. Measure it
before building anything — dump both paths' inputs on frame 480 rather than
reasoning about the arithmetic, which is how E36, E41 and E42 were each spent.

### Harness defect this exposed

`capture-cut-g-exact-frames.ps1:257` asserts GO!-overlay state — recognized
calls, draw calls, SObj and OAM object counts, commit calls, `FrameIdle == 0` —
on **every** exact-frame capture. At frame 480 the match is 568 source ticks in,
the GO overlay is long gone, and the native OAM block is legitimately idle, so
the assert throws for both arms. The screenshots are written first (the file-
existence assert is *after* it at line 279), which is why E32 got its PNGs and
this defect went unrecorded. Every future mid-match exact-frame capture pays a
full emulator boot and then dies. **Fixed**: the always-true invariants (native
OAM owns the overlay, no fallback, no hot convert, no runtime upload, prepare
succeeded) are asserted every frame; the GO census is asserted only when
`gNdsIFCommonNativeOamFrameIdle == 0`, and when idle the opposite is asserted —
zero recognized/draw/commit/cloud-draw calls — so an overlay that goes idle while
still drawing is still caught. The pair check now also rejects a pair straddling
the idle transition. Verified both ways: frames 480/481, which threw before,
capture clean, and probe frames 200/201 (`time_passed = 8`) take the presenting
branch.

**Still open, and it is not mine to re-pin.** The GO-presenting constants are
themselves stale — at frame 200 this build reports 3 OAM objects against an
expected 23, 608 prepare-palette bytes against 32, 3 cloud textures against 2,
57,344 cloud-texture bytes against 65,536, and 10 cloud draws against 2. So this
assert has not passed on *any* frame for some time, which also means
`verify-battle-playable-realtime-harness.ps1:21`'s `FastCaptureFirstFrame = 438`
has been dead. The constants postdate `4f4528f` (countdown GO and source-alpha
flare fidelity) and `6da286e` (crisp IFCommon alpha coverage). Re-pinning them by
observation would bless whatever the current state happens to be as correct,
which is a countdown-GO fidelity judgement and belongs to that owner, not to a
fighter-shade experiment. **Action for that owner: re-derive the GO census
constants, or delete them if the invariants above are the real contract.**

## R2-03 E40 — state tables to DTCM: BUILT, NULL, reverted (2026-07-29)

E39 established the replay's cost is memory, not logic: 2.9 genuinely distinct
writes per epoch at ~250 ticks each. E29 had won **26,816** by moving the fighter
geometry tables into DTCM, so the same lever was applied to the replay's two
tables — `sNdsNativeFighterStateDeltas` (840 B) and `sNdsNativeFighterStateSequence`
(196 B), together ~1 KB against 3,432 free. Bit-exact by construction: same data,
different address.

Built, gated and measured. `check-task20-dtcm-layout.ps1` passes with all four
fighter tables resident and `__irq_table` still 32-byte aligned at `0x02ff2600`.

| bucket | Δ P50 | Δ P95 |
|---|---:|---:|
| `FTR` (the target) | −4,544 | −384 |
| `WORK` | −3,776 | +1,280 |
| **`OTHR`** (cannot be affected) | **+5,568** | −64 |
| **`SRC`** (cannot be affected) | +128 | **−3,584** |

**NULL, and the pair proves it on its own.** `OTHR` and `SRC` cannot depend on
where the fighter's delta tables live, yet they moved ±3,500–5,568. That is the
build-placement noise, measured *inside the same comparison*, and −3,776 is
indistinguishable from it.

**Reverted rather than kept, and the reason is DTCM scarcity, not process.**
AGENTS.md says to keep every repeatable correctness-preserving gain, but
"repeatable" is exactly what one pair inside its own noise cannot establish — and
the 1,036 bytes are not free. E29 already recorded that
`sNdsNativeFighterPackedCorners` needs 3,756 and "does not fit safely"; spending
a quarter of the remaining headroom on an unmeasurable gain forecloses a better
tenant.

**Why it did not repeat E29's win:** E29 moved 8,656 bytes touched **1,878 times
a frame** in random order, which could not fit the 4 KB dcache. E40 moves 1,036
bytes touched **182 times** — and the 840-byte delta table already fitted
comfortably. Same lever, an order of magnitude less to win. **Size a placement
move by accesses per frame against cache capacity, not by "this worked before".**

### Standing rule this earned

**An A/B on a placement change must report an untouched bucket.** The 5,000–7,000
floor is a remembered constant; `OTHR`/`SRC` are a *measured* bound for the exact
pair in hand, and they cost nothing to read because the sampler already collects
them. Had only `FTR` been reported, −4,544 would have looked like a modest KEEP.

## R2-03 E39 — operand elision BUILT and REFUTED on engagement (2026-07-29)

Built the cheapest version of E26's idea and killed it with its own counter, for
about one build's cost. **Reverted; do not rebuild it.**

The elision is exact, by the argument E20 already wrote into
`ndsRendererNativeApplyStateDelta`: every case writes `stats` purely from
`w0`/`w1`, so identical operands to the previous application of the same effect
mean identical writes — and if the state did not change, the texture prepare
built from it is still valid, which kills the invalidation E25b identified as the
real cost. GEOMETRY excluded (the one read-modify-write case, and eliding it
would also skip `geometry_command_count`); validity cleared on every material
application and per owner execute.

**Engagement: 7,898 elided against 99,179 applied — 7.4%.** At E38's 251
ticks/delta that is ~3,700 ticks/frame, *below* the 5,000–7,000 build-placement
noise floor, so an A/B would have returned noise and a KEEP would have been
unearned. The counter answered it without one.

**The mechanism is structural, and it is worth carrying into E26.** The
before-span averages only **2.9 deltas per epoch** (134.5 over 46.4), mostly of
different effects, and `ApplyMaterial` resets any cross-epoch cache on 28 of
those 46.4 epochs. There is almost nothing for an operand cache to hit. So the
33,708 is **not** redundant work — it is ~3 genuinely distinct writes per epoch
paying ~250 ticks each in dispatch, call and invalidation overhead. E26 must
therefore replace the *dispatch*, not deduplicate the *writes*: one install per
epoch instead of three calls.

### Hazard found the hard way, and it is not specific to this cut

The first build cached across owners and put **every frame in the 5+ VBlank
bucket**. `ndsRendererNativeApplyStateDelta` is **shared** — the stage owner and
the hierarchy modes reach it through their own spans — and the counter proved it
immediately: **850 applications a frame against the fighter's 182.4**. Anything
memoising in that function must be armed around the fighter production spans
specifically, or it silently elides the stage's state writes against operands
cached from the fighter. Confining it restored the histogram to 2:445 3:111 4:8
5+:2, against the control's 2:442 3:115 4:6 5+:3.

**Read that as a general rule: before memoising in a shared renderer helper,
count its calls from the owner you think you are optimising.** A 4.7x
discrepancy between "the deltas I sized" and "the deltas that arrive" is the
whole bug, and one engagement counter exposes it.

## R2-03 E38 — E26 scoped: fold the BEFORE-span, and only that (2026-07-29)

E26 is R2-03's own bullet in the switch plan ("per-epoch generated submit
consuming only baked facts — no `PrepareProductionRun` policy re-checks, no
traversal-state/stats dependency"). E34-b settled that the material must stay
live. What was never measured is how the replay splits **across** that material,
which is what decides how much of E26 is tractable: the before-span is pure
prologue, while folding the after-span means re-applying static writes *over*
live material writes and getting the ordering exactly right.

Timed separately (`NDS_TASK91_DRAW_PHASE_CENSUS`, frames 439..919, 46.4 epochs a
frame):

| span | ticks/frame | deltas/frame | ticks/delta |
|---|---:|---:|---:|
| **before** | **33,707.6** | 134.5 | 250.6 |
| after | 16,243.3 | 47.9 | 339.1 |
| **total replay** | **49,951** | 182.4 | |

> **Superseded in part by E43.** Every tick figure in this table is inflated by
> the per-delta census sitting inside the bracket: the before-span is **26,944**,
> the after-span **13,704**, the replay **40,648**. The *split* below — 67.5% /
> 73.7% — survives, because both arms move together. Use 26,944 as E26's target.

**The before-span is 67.5% of the cost and 73.7% of the deltas — and it is the
half with no ordering problem.** So E26 reduces to: bake the resolved
post-before-span state per epoch, install it, leave `ApplyMaterial` and the
after-span exactly as they are. Target **26,944/frame** (E43-corrected) for a
change with no material interaction to reason about.

Install every field *except* `prim_color`/`env_color` and their companions: E34-b
showed those are the only state that varies at runtime, so leaving whatever the
live path put there is both correct and what keeps the material live.

**Also note the replay is now 49,951, not the 65,026 E25b sized it at.** E12, E28
and E29 have shipped since. Size E26 against 33,708, not against a share of
65,026 — and re-measure before claiming a share of any older total.

After-span deltas cost 339 ticks each against the before-span's 251, which is
consistent with the material application between them dirtying what the
after-span then re-touches.

## R2-03 E32 — DO NOT GRADUATE: visual regression found, mechanism named (2026-07-29)

**The visual gate was answered by measurement, and the answer is no.** Flag stays
default 0. Details in `ClaudeOpus5_R203_E32_ShuffleFold_20260729.md`.

Frame-locked captures of the same presented frames from both arms
(`capture-melonds.ps1 -ExactFirstFrame N -ExactSecondFrame N+1 -FoxCpuMode 1
-SoftwareRenderer`, each arm built to its own `NDS_OUTPUT_ROOT`):

| frame | differing pixels | share |
|---|---:|---:|
| 480 / 481 (hitlag) | 1,826 / 1,536 | 0.661% / 0.556% |
| 510 / 511 (control) | 188 / 188 | 0.068% |

The control makes the comparison sound: on non-hitlag frames the arms are
pixel-identical apart from the bottom-screen `FPS`/`UP` readout, which must
differ because the arms run at different speeds. The `CUTG_EXACT` rows agree byte
for byte at every frame, state hash included — E32 is render-side only.

**On hitlag frames the struck fighter renders dark maroon instead of light grey**
(`artifacts/visibility/e32-compare-480.png`, `e32-compare-481.png`). That is the
hurt flash, not the shake: per E34, `prim_color`/`env_color` are the only
per-epoch state that varies at runtime, and Task 39's hurt flash is what varies
them. The reference arm falls back to the generic path during animlock (E31) and
gets its flash handling; the candidate stays on the native owner and applies the
material differently.

**E32 is really two changes and only one was measured.** It was framed as folding
the shuffle into the world matrix; its actual effect is *not falling back during
animlock*, and the fallback was also hiding a material seam. Fix the native
owner's hurt-flash colour to match the generic path, then re-run these four
captures — the tick win (`FTR` P95 913,920 → 412,992) is real and worth returning
for.

**E36, the first hypothesis, is REFUTED — do not retry it.** The damage flash
looked like the mechanism: `ndsRendererHardwareModulatePackedColor` is an affine
lerp `L(x)=x*k+c`, the engine computes `ambient + diffuse*dot`, and E16 writes
`L()` into both registers, so `L(A)+L(D)*d = L(A+D*d) + c*d`. Built the exact
correction (ambient full lerp, diffuse scale only) and captured: **1,826 -> 1,827
pixels.** `color_modulate`'s alpha is zero on these frames, and the *material*
path is multiplicative anyway (`ndsRendererHardwareScaleMaterialChannel5`), which
distributes over `ambient + diffuse*dot` exactly — so `prim_color` is carried
correctly. The change was **reverted rather than shipped unproven**; the
arithmetic still predicts a real defect wherever alpha is non-zero, but nothing
in 439..566 exercises it.

**E41 ISOLATED THE CAUSE: the native owner's material-colour path.** Three arms
of the same presented frame 481 (`artifacts/visibility/e41-three-way-481.png`),
built by flipping the `HW_LIGHT` override for one diagnostic build and restoring
it immediately:

| arm | `HW_LIGHT` | fold | Fox renders | px vs reference |
|---|---|---|---|---:|
| reference | 1 | off (generic) | **light grey** | — |
| candidate | 1 | on | **dark maroon** | 1,536 |
| E41 | **0** | on | **dark maroon** | 3,027 |

Software shading did not restore it, it moved *further* away — so E16's hardware
lighting is refuted. Fox is in the same position in all three, so the fold's
arithmetic is refuted too. What remains: control frames 510/511 are
pixel-identical between arms, so the native owner draws Fox correctly on ordinary
frames; the only thing hitlag adds is the hurt flash writing `input->materials[]`
(E34's 108 runtime changes), and E36 already excluded `color_modulate`. So the
flash arrives through `stats->prim_color` and the native owner tints with it
where the generic path brightens.

**The fix is to diff `ndsRendererR2MaterialChannel` →
`ndsRendererHardwareScaleMaterialChannel5` (native, multiplicative into
diffuse/ambient) against `ndsRendererHardwarePackedVertexColor` (generic,
combines material with the vertex colour and its validity mask). This is a
rendering-correctness fix owed regardless of E32** — E32 only decides whether
those frames are ever drawn by the native owner.

**Superseded below:** the source reading that pointed at E16. The fold's
arithmetic looks right: `dFTDisplayMainShufflePositions` holds ±50/±100 in source
world units and E32 copies them through the port's documented 4096 conversion
unchanged, and `ftdisplaymain.c:1205` applies them with
`G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW` between view and model — a world-axis
translation, which is what adding into the world matrix's translation row
reproduces. One part is still open: the source has **two** application sites
(`ftdisplaymain.c:1205` for the body, `lbcommon.c:1629` for an *attach* DObj), so
E32's per-binding loop may over-apply.

**The leading explanation is now E16, not E32.** Hitlag frames are the only
frames where the native owner and the generic path draw the same fighter, so
E16's hardware lighting has never been compared against the software shade on
identical input — E32 is merely what made that comparison happen. E16 was
graduated on frames that could not expose it.

**Obstacle, found the hard way:** `NDS_R2_FIGHTER_HW_LIGHT` is `override`-forced
to 1 for the hwtri targets (`Makefile:543`, `:657`), so a command-line
`NDS_R2_FIGHTER_HW_LIGHT=0` is **silently ignored** — the build succeeds and the
config header still reads 1. Always check the built `nds_build_config.h` rather
than trusting the command line.

**Harness note:** exact-frame capture is *not* gated to the Cut G window by frame
number, only by its assertion set — the captures land, then the GO-text
assertions throw. `capture-melonds.ps1` also passed its own `-FoxCpuMode -1`
"unset" sentinel into a callee validating `0..1`, so the default invocation died
after a full emulator boot; **fixed in this commit** by normalising the sentinel
up front.

## R2-03 E35 — the gate is owned by the SIMULATION, not the renderer (2026-07-29)

**Highest-value row on the board, and it is no longer a fighter row.** Full
report: `docs/optimization/ClaudeOpus5_R203_E35_SrcExcursion_20260729.md`.

`WORK-H` P50 is **1,011,200**, inside the 1,120,000 gate. Only P95 misses, so the
question is what an *expensive* frame runs. Three results, from a 128-frame ring
dump (frames 439..566) carrying the Task 75 per-frame load counter, plus two
`NDS_TICK_HUD_DRAW=0` per-PC census windows:

**1. Loading was oversized by half.** Load-free P95 1,419,264 against 1,468,800
over all frames, so eliminating on-demand loading is worth **~49,536**, not the
~103,488 the board has carried since Task 75 E0. Still real, still not the gate.

**2. E32 does not land the gate.** Applying its measured `FTR` cap frame by frame
across all 128: 34/128 over gate -> **26/128**, P95 1,468,800 -> **1,377,408**.
Reading only the worst fourteen frames suggested it might land the gate outright;
it does not. **Rank the whole distribution, never the visible top of it** — a P95
is a position in a sorted list, decided by the frames just below the ones that
catch the eye.

**3. 25 of the 26 remaining over-gate frames are `SRC` excursions**, 13 of them
load-free and arriving in consecutive runs (452–453, 475–477, 517–521, 542–543).
`SRC` is `scVSBattleFuncUpdate` x2 — the SSB64 simulation.

Profiling 517–521 against a matched control at 508–512, per frame, excluding
`armWaitForIrq` (a consequence: three VBlanks instead of two):

| block | ticks/frame |
|---|---:|
| **softfloat** | **283,072** |
| collision (`gmCollision*`, `ndsStageMP*Sweep*`) — four functions enter from zero | 75,088 |
| a third owner drawing — all zero in control | 66,498 |
| overlay 2 (`func_ovl2_800ED490`, `func_ovl2_800EDBA4`) — zero in control | 24,773 |

`MISC` confirms the draw half independently: 47,424 -> 125,184–157,888.

**The float is attributed, and it is NOT the float Task 92 closed.** Task 92 E0
closed soft-float as a conversion target on a ~90-second average whose largest
caller was `gcPlayDObjAnimJoint` at 54.2%. In this excursion **every caller it
classified is flat** — `gcPlayDObjAnimJoint` +2,217/frame, the renderer float
callers +20 to +131 — while a population that is *exactly zero in the control*
appears with 62,830/frame of caller self time: `func_ovl2_800ED490` (a `Mtx44f`
multiply, 27 mul + 21 add per call), `func_ovl2_800EDBA4` (walks a joint DObj to
its root and back rebuilding world matrices), `gmCollisionSetInvertMatrix`,
`gmCollisionTransformMatrixAll`, `gmCollisionTestRectangle`,
`gmCollisionGetWorldPosition`. All of `decomp/.../gm/gmcollision.c`.

**The excursion is hit detection with live hitboxes.** That also explains its
other two signatures: the consecutive-frame runs are an attack's active frames,
and `MISC` tripling is the hit effect drawing as its own owner. Task 92's verdict
closed the class it measured; this caller set was not in it, so it is not
evidence against acting here.

**The exactness-preserving cut was priced and REFUTED before it was built.** The
obvious first move was an E5/E12-shape redundancy memo — `func_ovl2_800EDBA4`
carries two memo flags cleared once a frame by `parts->unk_dobjtrans_word = 0`
(`ftparam.c:2185`). Exact call counts, free from the profiler CSV because a
function's entry PC retires once per call, say there is nothing to memo:
**`func_ovl2_800ED490` runs 27.2 times a frame**, `GetWorldPosition` 32,
`TransformMatrixAll` 22.6, the rest 16 each. The within-frame memo already works.

**The cost is arithmetic and the unit price is the finding:** 230,850 cycles over
6,053 `fadd` calls is **38 cycles per soft-float add** (`__mulsf3` 27). The
excursion adds ~6,410 float ops/frame for ~217,734 cycles. Ordinary frames
already run ~5,952 ops (~182,000); excursion frames ~12,362 (~400,000).

**OWNER DECISION, sized and ready.** The only lever is float→fixed on the
collision path. `PROJECT_GOAL.md` permits it — "Mechanical equivalence is
required. Bit-exact or numerically identical execution is not" — and its
sacrifice order ranks gameplay fidelity *above* stable 30 FPS. But `gmcollision.c`
decides hit detection and is verifier-gated by the Task 9 state hash, and
re-bounding a bit-exact gate is not a call to take unsupervised. Combined with
E32 this is roughly the whole remaining gap: the 26 projected over-gate frames
span 1,152,192–1,614,080, and removing ~280,000 puts all but four under 1,120,000.

**Caveat now on record:** `_ntrcardRomReadSector` measured +95,357 on the
excursion with the HUD drawn and −95,356 (entirely in the control) with it
compiled out — same frames, same deterministic match. Cartridge reads complete
against wall time, so **never attribute cartridge activity to a frame across two
differently-timed builds.** The load *counter* is frame-stable because a finalize
is a software event; the sector read is not.

## R2-03 E34 — E26's premise MEASURED: the epoch state is 99.5% static (2026-07-29)

**E26 is viable, and this is the number it was missing.** Its §2a correction
raised the live material as a per-epoch problem; the harder version is that
`ndsRendererNativeApplyMaterial` writes the same `stats` fields, so contamination
can propagate *forward* into later epochs through any field their before-span
does not itself rewrite. If that happened often, the baked table would be a
fiction. So it was measured before either design was built.

`NDS_R2_FIGHTER_EPOCH_STATE_PROOF=1` hashes the state each epoch hands to its
runs — the fields `PrepareProductionRun` and the shade read: `geometry_mode`,
`othermode_h/l`, combine w0/w1, env/prim colour, texture flags/tile/on/scale,
the two light colours, the light direction, and the **active** 20-word
`NDSRendererTileState` — keyed by epoch index, counting frames whose value
differs from the one already stored.

| | frames 439..919 |
|---|---:|
| samples | 22,566 (47.0/frame) |
| **changes** | **108 — 0.48%** |

**The state is a function of the epoch index 99.5% of the time.** That is the
same shape as E5's run facts (1.9% churn) and it says the fold is a table plus a
small repair, not a table plus a fiction.

**And the residue has an owner.** 108 changes over 480 frames is ~0.2/frame,
which tracks the hitlag population E31/E32 mapped — Task 39's hurt flash writes
`input->materials[]` live, and hitlag covers roughly 10 frames in 128. So the
0.48% is very likely the flash colour, which is exactly the field E26 §2a already
says must stay a runtime write ("bake the table and write colour per frame" —
E1a's shape). Confirm that attribution by re-running the proof with `prim_color`
and `env_color` dropped from the hash: if changes go to zero, the fold is clean
and colour is the only runtime input.

**Method note:** `gNdsR2EpochStateUnstableEpochs` reads 0 in this window because
the census reports *deltas* and that counter saturates during the pre-439 warm-up.
Its absolute value needs a single-stop read, not a windowed difference — do not
read that 0 as "no epoch is unstable", which contradicts the 108.

### E34-b — the attribution CONFIRMED, and the limit of what it licenses

`NDS_R2_FIGHTER_EPOCH_STATE_PROOF=2` is the same hash with `prim_color` and
`env_color` removed. Same window, same ROM shape:

| | frames 439..919 |
|---|---:|
| samples | 22,296 (46.4/frame) |
| **changes** | **0** |

Level 1 is the positive control: the identical hash caught 108 changes, so it is
demonstrably time-sensitive, and dropping exactly two fields took it to zero.
**Apart from the two colours, the per-epoch state is exactly a function of the
epoch index.** §2a's "two snapshots plus an after-span field mask" is therefore
unnecessary — one snapshot per epoch plus two colour writes reproduces it.

**But do not read this as a construction guarantee, and do not bake the material
out.** `ndsRendererAdapterBuildNativeMaterial` rebuilds every material from the
live `MObj` each frame (`src/port/reloc_backend_renderer_dl.c:7830`), and its
texture-derived fields — palette image, TLUT, block image, tile sizes, texture
state — are keyed off `mobj->texture_id_curr`/`texture_id_next`, which
`ndsRendererAdapterSaveNativeMaterialTextureIds` exists specifically to
save/restore. A texture animation moves them. This window contained none, which
is why only colour varied; a window that contains one would show more, and a
table baked from this measurement would render the wrong texture.

**So E26's safe shape is: fold the two static spans, keep `ApplyMaterial` live
and unchanged.** The measurement licenses removing the *replay*, not the
material. Anything that also removes the material application must first prove,
from the source rather than from a window, that no fighter material's texture
identity moves during a match.

## R2-03 E33 — the run prepare still has no hot spot, re-confirmed (2026-07-29)

Per-run split on the current build (`NDS_R2_FIGHTER_RUN_PROOF=2`, frames
439..919), 62.8 runs a frame:

| run phase | ticks/frame | share |
|---|---:|---:|
| Tail | 14,224 | 29% |
| TexPrep | 12,506 | 26% |
| UV | 11,705 | 24% |
| Validate | 8,757 | 18% |
| TexReuse | 1,216 | 3% |
| sum | 48,408 | |

`TexPrepCount` 47.0/frame against `TexReuseCount` 16.7 — the same 46.4-of-62.8
full prepares E25b found, unchanged by E28/E29/E32 as expected (none of them
touched the invalidation).

**Nothing here is a cut.** Four phases between 18% and 29% is exactly E25's
"PrepareProductionRun has no hot spot", now re-confirmed on a build three cuts
newer. **Do not go hunting for one.** The cost is structural — it is the
per-invalidation re-prepare — and E26's replacement is the answer, not a
micro-optimisation of Tail or UV. E5 already refuted the UV loop specifically.

**Two instrument defects found doing this, both of which waste a build:**

- **At `NDS_R2_FIGHTER_RUN_PROOF=1` every one of these tick counters reads
  exactly 0.** They need level 2. A run at level 1 reports a complete, plausible
  all-zero table rather than failing.
- **`gNdsR2RunCallCount` reads 0 at level 2 as well** — it belongs to the E5
  falsifier, not to these brackets, and is not wired on the canonical production
  path. Its own comment warns about precisely this ("hooked that table and
  honestly reported zero calls"); it is still true. Do not use it as the liveness
  denominator for the tick split — use `TexPrepCount + TexReuseCount`.

Do **not** compare the 48,408 above against the census build's
`gNdsR2SubmitPrepTicks` 39,043: different bracket boundaries **and** different
binaries, which the standing rules forbid comparing.

## R2-03 partition at HEAD, and where the phase stands (2026-07-29)

Census build, all three cuts on, frames 439..919:

| phase | ticks/frame | pre-E28 |
|---|---:|---:|
| Walk | 3,224 | — |
| Validate | 10,148 | — |
| Preflight | 3,224 | 3,272 |
| **Root** | **44,502** | 44,785 |
| **State replay** | **61,441** | 72,798 |
| Shade | 22,579 | 57,715 |
| **Submit** | **94,395** | 105,630 |
| — of which Prep | 39,043 | 41,928 |
| execute sum | 226,141 | 284,200 |

**Caveat, in E30's family: the State bracket is instrument-inflated.**
`ndsRendererNativeApplyStateDelta` carries a `NDS_TASK91_DRAW_PHASE_CENSUS` block
that runs *per delta* — three array reads, two compares and two array writes
against 194.4 deltas a frame — and it sits **inside** `gNdsR2ExecStateTicks`.
Call it 6,000–10,000 of the 61,441. The other brackets wrap whole calls and are
not exposed this way. Before sizing a state-replay cut, re-measure with the
per-delta census block compiled out, or the cut will be sized against a number
that includes the probe.

**Ranked remaining, against R2-03's provisional 250,000 budget with FTR P50 at
408,512:**

1. **State replay 61,441 coupled to Prep 39,043 = ~100,500.** E25b showed these
   are one mechanism: 194.4 deltas invalidate `texture_prepare_valid`, forcing
   46.4 of 62.8 runs into the full prepare despite a 99.5% texture-memo hit rate.
   E26's spec (`..._E26_Spec_GeneratedEpochState_20260728.md`, **including its
   §2a correction** — two snapshots plus an after-span write mask, material stays
   a runtime step) is the design. This is the switch plan's own R2-03 bullet.
2. **Submit emit ~55,352** = 29.5 ticks/corner over 1,878 corners, after E29 put
   both vertex tables in DTCM. High for four FIFO writes; unexamined.
3. **Root 44,502** over 32 roots. `ndsRendererNativeApplyRootLightPreamble` is
   not it — that is a handful of `stats` writes under `optimize("Os")`. The
   remainder is `BindProductionRoot` + E17's split matrix load + `glStoreMatrix`,
   and E22/E23 already priced the matrix load and refuted the projection hoist.

**DTCM is now full at its safe margin.** Data tops out at `0x02ff2298` against
the `0x02ff3000` ceiling — 3,432 bytes. `sNdsNativeFighterPackedCorners` (3,756)
does **not** fit. It could be made to fit by raising the ceiling toward the
measured boot-stack low-water at `0x02ff3340`, but that leaves under 500 bytes of
margin and the table is read sequentially, where the data cache already does
well. Not worth the stability risk — treat the DTCM lever as harvested.

## R2-03 E32 — the hitlag shuffle folded; FTR's tail is GONE (2026-07-29)

**KEEP candidate, flag default 0, awaiting the owner's visual approval.**
`NDS_R2_FIGHTER_SHUFFLE_FOLD`.

The renderer was disabling the entire native fighter owner whenever
`fp->shuffle_tics != 0` — i.e. giving up its fast path on **every hit**. A split
counter attributed all 5 fallbacks in frames 460..500 to `shuffle_tics` and
**zero** to `is_use_animlocks`.

It never needed to. `ftdisplaymain.c:1205` is one `G_MTX_PUSH` +
`syMatrixTra(x, y, 0)` + `gSPPopMatrix` around the whole fighter draw, and
`lbcommon.c:1627` writes the identical effect as `f[3][0] += x; f[3][1] += y;` on
the part's **world** matrix before the camera. `PrepareNativeOwnerMatrices`
already builds exactly that matrix, so the offset goes in at the same point in
the same space — **mechanically equivalent by construction, not an
approximation.** `shuffle_tics` leaves the eligibility disjunction;
`is_use_animlocks` stays (measured firing zero times).

| | FTR P50 | FTR P95 | FTR max | frames > 600k | WORK P95 |
|---|---:|---:|---:|---:|---:|
| E30 | 404,672 | 913,920 | 918,976 | **11** | 1,467,840 |
| **E32** | 408,512 | **412,992** | **414,656** | **0** | **1,381,120** |

The bimodal distribution collapsed to flat. Frames over the 1,120,000 gate
**35/128 -> 27/128**; VBlank `2:472 3:87 4:4 5+:2` -> **`2:489 3:72 4:4 5+:1`**.
Engagement read from the same run: `gNdsR2ShuffleFoldedFrames = 20`, two fighters
across ten burst frames. Ordinary frames pay `FTR` +3,456 median (the per-binding
adds), at the noise floor and bought back many times over.

**NOT verified: the visual gate.** A zero offset would flatten `FTR` identically
by simply not shuffling, so "the burst disappeared" is *not* evidence the effect
survived. Only a screenshot or play test confirms the fighter still shakes, by
the right amount, and that electric hits shake horizontally. Build the same ROM
with `NDS_R2_FIGHTER_SHUFFLE_FOLD=0` for the comparison arm — that arm is the
generic path and is correct by construction. Flag stays default-0 until approved.

**Boundary on the enabled arm: PASSED**, with the flag defaulted to 1 for the
whole run and the default reverted after. A first attempt was discarded because I
reverted the default *while that run was still building* — `make` re-reads the
Makefile per invocation and the profile runs several, so its (passing) result was
not trustworthy. **Never edit a build flag while a verifier is running; the tree
a verifier reads has to be still.**

The committed state is flag **default 0**: every hunk is inside
`#if NDS_R2_FIGHTER_SHUFFLE_FOLD` and the eligibility condition falls through to
its original `#else`, so the shipping configuration is unchanged by construction.

**Gate now: WORK P95 1,381,120 = 1.23x, gap 261,120** (from 1.37x at the R2-08
readiness table). Remaining tail is `SRC` asset loading (~103,488, Task 75 E0)
plus `OTHR`/`MISC`.

Write-up: `docs/optimization/ClaudeOpus5_R203_E32_ShuffleFold_20260729.md`.

## R2-03 E30 — the median is inside the gate; the tail is three other things (2026-07-29)

**The single most important row on this board.** E28+E29 took 58,304/frame out
of the fighter. `WORK` P50 fell 1,071,488 -> **1,010,240, inside the 1,120,000
gate**. `WORK` P95 went 1,496,064 -> 1,467,840 — **essentially not at all.**

**The steady-state fighter cost and the P95 gate are now different problems.
More median cuts will not close the gate.** Do not queue another median cut
without a reason that survives this row.

Decomposing the 8 worst `WORK` frames against the median frame — the actual
frames, not independently-sorted columns — gives three causes that do not
co-occur (3 frames HUD-only, 3 FTR-only, 2 both):

| bucket | excess over median frame | share |
|---|---:|---:|
| **FTR** | 2,538,432 | **41.9%** |
| **HUD** | 1,868,608 | **30.9%** |
| **SRC** | 1,298,624 | **21.4%** |

**HUD was the instrument, and it is now switchable off.** `HUD` is 960 at the
median and **345,024** on 9 of 128 frames, periodic at 13.25 presented frames =
0.494 s = `NDS_BATTLE_FPS_HUD_SAMPLE_TICKS` (`BUS_CLOCK / 2`). It is the tick
HUD's own block: eleven 128-entry ring sorts and thirteen `vsnprintf`/`iprintf`
console lines. **None of it exists in the published ROM**, and the GDB sampler
reads `sBattleTickHudRing` directly and never reads `sBattleTickHudP50/P95`.
`NDS_TICK_HUD_DRAW=0` removes it: `WORK` P95 **1,548,032 -> 1,467,840**, VBlank
`2:446 3:109 4:9` -> **`2:472 3:87 4:4`**, frames over gate 39 -> 35.

**Every measurement this campaign took on the tick-HUD ROM carried ~345,024
ticks of instrument on ~7% of frames — exactly the frames the P95 gate is
decided on.** Pass `NDS_TICK_HUD_DRAW=0` for measurement; default stays 1 for
device reads and screenshots. (`sBattleTickHudRing` is now `volatile`: with
nothing in the ROM reading it, `--gc-sections` deleted the array and the sampler
failed. A measurement buffer whose only consumer is a debugger must say so.)

**Highest-value unowned row: the FTR bursts — and they are a NATIVE-OWNER
FALLBACK.** `FTR` is bimodal, 401,856 median or ~900,000 with nothing between, on
frames **478–482 and 544–548**: two contiguous five-frame events 62 frames apart.
Both probes are done and they agree:

- **Geometry does not spike.** P0 triangles/frame: 468–473 **256.0**, 473–478
  **384.0**, 478–483 (burst) **320.0**. The window *before* the burst draws more
  than the burst does.
- **The native execute gets cheaper.** Phase census over the burst: **38.2 epoch
  calls instead of 58.8**, 49.0 submits instead of 80.4, and **178,800 ticks
  instead of 286,988** — while whole-frame `FTR` doubles. Both bursts are
  counter-identical (191 epochs, 245 submits over 5 frames): one event, twice.

So the work **left the native execute** and reappeared outside every bracket the
phase census owns. `FTR` also brackets the DObj walk, the revalidation and the
owner prep (E2/E3 113,199/frame, E4 MatrixPrep 91,338/frame), so the excess is in
one of those or in a fallback to the generic interpreter. **Optimising the native
fighter execute cannot touch these frames either way.**

**ANSWERED — it is the hitlag shuffle turning the native owner off.**
`NDS_TASK68_FALLBACK_CENSUS=1` over frames 460..500 (40 frames, containing the
burst): `FallbackCount = 5`, reason **[2] `AnimLock` = 5**, every other reason 0,
denominators `Calls`/`Eligible` 82/82. **One fallback per burst frame.** The site
is `reloc_backend_renderer_dl.c:12224`:

```c
if (native_owner_enabled && (production_mode || hierarchy_mode) &&
    ((fp->is_use_animlocks != FALSE) || (fp->shuffle_tics != 0u)))
    native_owner_enabled = FALSE;      /* whole fighter -> generic path */
```

`shuffle_tics` is SSB64's hitlag shuffle (`fttypes.h:1146` "Model shift timer",
set from `ftParamGetHitLag` in `ftparam.c:236`). Two hits ~2 s apart with ~5
presented frames of hitlag each is exactly the signature.

**The source makes the fix easy.** `ftdisplaymain.c:1205` is one `G_MTX_PUSH` +
`syMatrixTra(x, y, 0)` around the *whole* fighter draw and one `gSPPopMatrix` —
a constant whole-model translation from
`dFTDisplayMainShufflePositions[is_shuffle_electric][shuffle_frame_index]`. It
touches no geometry, material or animation. The native owner already loads a
per-root matrix (E17's `ndsRendererLoadHardwareSplitMatrices`), so **folding the
offset into that load reproduces the source exactly at ~zero per-frame cost**
instead of dropping the whole fighter to the interpreter on every hit.
Mechanically equivalent by construction, not by approximation.

Expected: ~500,000 excess ticks removed from ~10 frames per 128 — 41.9% of the
tail excess. **Check first** which half of the disjunction fires: `AnimLock` is
shared by `is_use_animlocks` and `shuffle_tics`, so split the counter or read
`fp->shuffle_tics` on a burst frame before assuming shuffle is the whole story.

**Harness fixed in the same change.** `census-fighter-draw-phases.ps1` collapsed
its window twice in one session and printed a complete, plausible table both
times — the second produced a "no fallback occurred" reading from the wrong
frames, which was briefly written down as a refutation. GDB `if` at top level
resumes exactly once (Task 96's rule), so a missed stop lands the script's own
`continue` somewhere later unnoticed. It now throws unless the A stop is exactly
`StartFrame` and the window is `WindowFrames` (+1 tolerated on B). **A
measurement that quietly answers a different question is worse than one that
fails.**

`SRC` (21.4%) is Task 75 E0's known load population, sized at ~103,488, unchanged
by Runtime 2.

**Gate now: P50 passes; P95 1,467,840 = 1.31x, gap 347,840** (was 1.37x).

Write-up: `docs/optimization/ClaudeOpus5_R203_E30_TailDecomposition_20260729.md`.

## R2-03 E29 — the fighter's hot tables move to DTCM, −26,816 (2026-07-28)

**KEEP.** The emit reads `sNdsNativeFighterPreparedDense` (8,656 bytes) and
`sNdsNativeFighterDenseNormals` (2,164) once per corner, 1,878 corners a frame,
in packed-corner order — randomly. Both sat in main RAM behind the ARM9's **4 KB
data cache**: a 2.7x overcommit, so essentially every corner missed. **DTCM —
16 KB of single-cycle uncached CPU-local memory — held 184 bytes.**

Paired 128-frame A/B against E28:

| bucket | better | worse | median delta |
|---|---:|---:|---:|
| **FTR** | **128/128** | **0** | **−26,816** |
| STG | 108 | 20 | −1,280 |
| WORK | 120 | 8 | −28,096 |

P0/P1 triangle counts identical (136,640 / 146,880). VBlank `2:438 3:117` ->
`2:446 3:109`.

**`STG` improved even though the stage never touches these tables.** Data-cache
pressure is a whole-frame shared resource, so moving a table out of main RAM pays
subsystems that never referenced it. Worth remembering when a cut's benefit shows
up outside its own bucket.

**Why the space was free, and why that needed measuring.** `__sp_usr` sits at the
top of DTCM and the region length spans the space the stack grows down into, so
the linker cannot catch a collision — a good reason the space had gone unused.
Measured: at the frame marker `sp = 0x02296530`, main RAM. Game code runs on a
Calico thread stack; only the *boot* stack enters DTCM, reaching `0x02ff3340`,
2,880 bytes down. 12,948 contiguous bytes were untouched at frame 900.

**Guard rails, because this fails silently.** Linker
`ASSERT( __dtcm_bss_end <= 0x02ff3000 )` encodes the measurement; a new
`.dtcm.fighter` section placed first and followed by `. = ALIGN(32)` keeps
Calico's `__irq_table` on its boundary regardless of the data-driven table sizes
(the Task 20 gate caught exactly that); and the Task 20 allow-list now carries
both owners with the DMA/IPC/ARM7 audit recorded. `forbiddenDmaRefs=0`.

**The struct shrink is bundled, not claimed.** Dropping `shaded_rgba` and
`packed_color` (dead under `HW_LIGHT` — every epoch is lit, so the loop writing
them never runs) takes the struct 16 -> 12 bytes. The `_Static_assert` demanding
16 was **right on its own terms**: two per 32-byte line, no straddling. Measured
alone in main RAM the shrink was a median −5,376 with a **mean of −1,122** — at
the noise floor, the straddle penalty eating the win. In DTCM there are no cache
lines, so 12 is strictly better and buys 2,164 bytes of margin under the boot
stack. That is the only reason it ships.

**Two process failures.** `make` does **not** regenerate the generated includes —
`build.ps1` does. The first E29 build changed the struct to four fields against a
four-day-old include holding six positional initializers; GCC warned, assigned
`gx_z = 0`, and produced a complete A/B on a ROM with every Z coordinate zeroed.
The generator now emits designated initializers so that mismatch cannot recur
silently. And the build-output `Select-String` filter dropped the warning —
**filter build output for new warnings, not for a fixed list of expected ones.**

**Next, and now cheap:** 7,144 bytes of DTCM remain, 4,264 under the boot stack's
low-water mark. `sNdsNativeFighterPackedCorners` (3,756) fits, though it is
streamed rather than randomly indexed and should benefit less.

Write-up: `docs/optimization/ClaudeOpus5_R203_E29_FighterTablesInDTCM_20260728.md`.

## R2-03 E28 — E16's dead producers, −31,488 (2026-07-28)

**KEEP.** E16 skipped the per-dense-vertex shading loop with a runtime flag but
left the work that computes that loop's *inputs* running on every lit epoch:
`ndsRendererHardwarePrepareLitDirection` (nine 32x32->64 multiplies, three
64-bit squares, an `sqrtf`, three float divides) and
`ndsRendererHardwareGetLightShadeLut`. Neither result has any other consumer in
a shipping configuration.

Paired 128-frame A/B, one tree, control = `NDS_R2_FIGHTER_SOFT_LIGHT_KEEP=1`:

| bucket | better | worse | median delta |
|---|---:|---:|---:|
| **FTR** | **128/128** | **0** | **−31,488** |
| WORK | 113 | 15 | −31,680 |

WORK frames over the 1,120,000 gate: **52/128 -> 40/128**. VBlank histogram
control 2:409 3:148 4:7 5+:2 max:18, candidate 2:438 3:117 4:9 5+:2 max:18 —
29 frames moved from a 3-VBlank interval to a 2-VBlank interval.
`gNdsFighterDLAllDrawP0/P1HardwareTriangleCount` identical in both arms
(136,640 / 146,880 over 480 frames): geometry is bit-identical, as the mechanism
requires — the removed values had no reader.

**The lesson, and it is general.** A flag that skips a *consumer* does not skip
its *producers*, and a single tick bracket around both cannot tell you which one
you removed. E24 read this same function and concluded "the action walk isn't
the cost" — correct, and it missed this because the dead work is in the preamble
*above* the walk, inside the condition that decides whether the epoch is lit.
**Price a skipped loop's inputs separately from the loop.**

**Second lesson, methodological.** The sorted-percentile table read
`WORK P95 +73,664` and every other number negative. That was an artifact: each
column's P95 is a different frame, and the P95 frame is an excursion frame whose
placement moves between arms (`WAIT` P95 fell by almost exactly the same amount,
which is the tell). Both arms run the same deterministic ROM from the same start
frame, so **frame N is the same game state in both — pair by frame number, not
by sorted percentile.** The pairing is free and it is what turned an ambiguous
result into 128/128.

**E27 is REFUTED and its probe is removed.** `gNdsR2MaterialOnlyInvalidations`
measured 2.0/frame against 28.0 material applications: 26 of 28 material
invalidations of the texture prepare hit a prepare the before-span deltas had
already dirtied. A split validity would reach ~1,800 ticks, below the noise
floor. It stays a necessary *component* of E26, not a cut of its own. The probe
also read `state.texture_prepare_valid` inside `ndsRendererNativeApplyMaterial`,
which the M3 stage falsifier correctly rejects as an unclassified read — the
standing "remove temporary probes" rule would have caught it before Boundary did.

Graduated R2-03 total is now E17 17,600 + E16 35,072 + E28 31,488 = **84,160**
of the 250,833 gap (34%).

Write-up: `docs/optimization/ClaudeOpus5_R203_E28_DeadSoftLight_20260728.md`.

## Bug #10 — closed and folded in (2026-07-28)

`06992f10812` "Fix Mario pelvis texture clamp", cherry-picked from `2cbc6189d15`
on `codex/fix-mario-bottom-rendering` onto the R2 branch so authorship is
preserved. Epoch 0 loads a 32x24 CI4 source into a 32x32 DS texture; its N64 T
axis is CLAMP with mask 5, so coordinates 24..31 resolve to row 23, while the DS
sampler wrapped through the eight zero-padded transparent rows — the aperture
was *inside* textured pelvis triangles, not at a geometry or culling seam, which
is why five earlier causes were eliminated. One line in
`ndsRendererHardwareTextureMaskedClampNeedsWrap` disables wrap when the logical
clamp edge is at or before the mask period.

It arrives with its own gates rather than needing new ones: a host fixture for
the exact 32x24 case, a structural pin in `check-gbi-decode-fixtures.ps1` so the
line cannot be silently reverted, the `pause_under20` camera oracle, and the
controller-playback DTCM move that oracle needs in order to write pads over GDB.
The DTCM layout checker was not relaxed to accommodate it — every Calico
boundary assertion survives, parameterised by the new 32 bytes, with added
all-or-none and per-symbol address/size/alignment pins.

Folding it in did surface a real harness defect, fixed in the same cycle.
Boundary failed twice on the locked-30 pacing gate reading
`logic/present = 422/212` with a phase histogram summing to 211. The ROM was
right: taskman's own counter and the fighter route both read 424 updates for 212
presents, an exact 2:1. Two terms compared counters incremented at *different*
instructions of one iteration, so they were asserting where the debugger stopped.
Both are now a four-state stop-phase model that rejects five of the eight sign
combinations — strictly stronger than the equality it replaced — with taskman's
independent counter disambiguating the one aliasing pair. E8 did not create the
window; it changed where in the frame the stop lands. Full derivation in
`docs/optimization/TASK_STANDING_RULES.md`.

The opt-in Task 25R trace carried the third instance of the same defect and is
now fixed too (`6221406`). Its rows all come from one fixed marker, so the skew
is constant and the contract is *stronger* than the four-state model: take the
skew from the first row, require it reachable, require every later row to agree
— a dropped or doubled update then disagrees with its neighbours instead of
hiding inside a tolerance. The final reconcile runs the BPLAY_PACE snapshot
through `Test-BattlePlayablePacingStopPhase` rather than a logic-only bound.
Eight synthetic cases cover it with no ROM or emulator; the registry pins the
new contract and bans the old equality.

## R2-03 gate MISSED 2.00x, and the 56% nobody had measured (E13/E14, 2026-07-28)

The owner observation below is now measured, and it turned over the phase.

**The gate.** Fighter draw, both fighters, bracketed on the tick-HUD ROM over 479
frames: **501,624 ticks/frame against §7's 250,000 for the pair.** Over by a
factor of 2.00. Mario alone measures 237,219 per draw call; either fighter on his
own very nearly exhausts the budget written for both. That budget was set in
R2-00b without a measured per-fighter cost.

R2-03 has shipped -47,486 (E9+E10, E12) against a 250,833 gap — 19% of it.

**Where the draw actually goes** (per frame, both fighters):

| phase | ticks | share |
|---|---:|---:|
| Walk / Validate / Reset | 20,595 | 4.1% |
| OwnerPrep (matrices + materials) | 143,684 | 28.6% |
| Build production inputs | 37,292 | 7.4% |
| **`...ExecuteNativeFighterOwnerProduction`** | **279,617** | **55.7%** |
| tail | 20,436 | 4.1% |

Both shipped cuts landed in the 28.6%. The 55.7% had never been bracketed, so it
was never a candidate — E3's split stopped at the point the owner inputs are
built, and everything past it went into one unnamed remainder.

**The 3D hardware is idle.** `GXSTAT` sampled either side of 946 fighter
submissions: FIFO entries 0 entering, 0 leaving, max 0, geometry engine busy on
0 of 946. Positive control passes (OR of raw words `0x06009F00`, bit 26 =
FIFO-empty set, so the register is live and the zeros are real).

**The ARM9 is the whole cost.** Cutting fighter polygons is the *wrong* lever: it
spends visual fidelity to work around a CPU failing to feed hardware that has
headroom. `PROJECT_GOAL.md` permits the trade; this says we have not earned it.

**E15 corrects what comes next.** E14 read "447 ticks per hardware triangle" off
this bracket and recommended a captured command stream on R2-02 E2's precedent.
That statistic divided an inclusive bracket by the wrong denominator — most of
the bracket is not per-triangle work. **The emit is ~99 ticks/triangle and 20% of
the execute**, so a DMA'd stream caps out near 62,693 against a 250,833 gap. The
recommendation is withdrawn; see the E15 section below for the real ranking.

Full write-ups: `docs/optimization/ClaudeOpus5_R203_E13_FighterPriceAndGate_20260728.md`,
`..._R203_E14_SubmitSplitAndGxIdle_20260728.md`.

## R2-03 E15 — the fighter is a per-epoch machine (2026-07-28)

The execute partitions completely. Per frame, both fighters (instrumented build;
brackets cost ~31,165/frame, so absolutes are inflated ~10-20% and the ranking is
the finding):

| phase | ticks/frame | share |
|---|---:|---:|
| Preflight | 3,247 | 1.0% |
| Per-root: bind, composed matrix, `glStoreMatrix`, light preamble | 40,785 | 13.1% |
| Per-epoch: two state spans + material | 52,065 | 16.8% |
| **Per-epoch: shade actions** | **86,207** | **27.7%** |
| Run prepare | 42,520 | 13.7% |
| Raw emit | 56,873 | 18.3% |
| Cross emit | 5,820 | 1.9% |
| residual | 18,487 | 5.9% |

**48.5 epochs and 66.2 runs per frame, averaging 12.7 triangles per epoch.** Each
epoch pays ~2,850 ticks of state and shade *before a triangle is emitted*, against
~1,255 of prepare-and-emit. **~70% of the execute is per-epoch and per-root setup;
20% is geometry.**

Ranked leverage:

1. **Shade actions, 86,207.** E1 refuted memoising it *across frames* and that
   stands — but E1 never asked whether the shade recomputes **per epoch** what is
   constant **per root**, which a cross-frame memo cannot see. 48.5 epochs against
   ~28 roots is the shape that makes it worth asking.
2. **Epoch state spans, 52,065.** R2-02 F found adjacent-run redundancy in the
   stage's spans; the fighter's have never been checked.
3. **Per-root 40,785** over ~28 roots — contains the GX matrix load and
   `glStoreMatrix`. Whether every root needs its own palette store is unasked.
4. Run prepare 42,520 — already cut by E12, diminishing.
5. Emit 62,693 — ordinary, and the least promising per unit of risk.

Write-up: `docs/optimization/ClaudeOpus5_R203_E15_ExecuteSplit_20260728.md`.

## R2-03 E17 — split matrix load BUILT, −17,600, awaiting owner approval (2026-07-28)

**First implementation of the E16 sequence, and it stands on its own.**
`NDS_R2_FIGHTER_HW_MTX`, default 0.

The fighter composed modelview x projection on the CPU (one 4x4 20.12 multiply
per root) and loaded the product. Now both are loaded separately and the
geometry engine — idle on 946 of 946 submissions per E14 — performs the
multiply. The compose is skipped outright; E16b proved it has no other consumer
under mode 9.

| bucket | A: composed | B: split | delta |
|---|---:|---:|---:|
| **WORK P50** | 1,118,144 | 1,099,584 | **−18,560** |
| **WORK P95** | 1,585,408 | 1,528,064 | **−57,344** |
| **FTR P50** | 507,456 | 489,856 | **−17,600** |
| STG P50 (control) | 175,552 | 175,296 | −256 |
| **VBlank 2 / 3** | 320 / 233 | **381 / 167** | **+61 frames at 30 FPS** |

The size matches the mechanism: ~28 roots x a 4x4x4 20.12 multiply ≈ 18,000
predicted against 17,600 measured. `STG` moving 256 is the placement floor and is
the control on whether `FTR` is real.

**NOT GRADUATED — needs the owner's eye.** Vertex positions now round in
hardware rather than on the CPU, a sub-pixel difference, and `AGENTS.md` gates
rendering-side changes on visual approval rather than exactness. **Boundary
passes in BOTH configurations, flag 0 and flag 1** — the second run was nearly
skipped, and verifying only the default would have meant approving a change on a
green run of the arm it replaces. The candidate capture
(`artifacts/visibility/ClaudeOpus5_R203_E17_SplitMatrix_candidate_20260728.png`)
shows both fighters and the stage correct with no distortion.

**On approval:** add `override NDS_R2_FIGHTER_HW_MTX := 1` to the published
`smash64ds-battle-playable-hwtri` block and the tick-HUD block. E16's hardware
lighting then builds on top of the vector matrix this establishes.

**Correction to E16a/E16b's stated reason.** Both claimed the fighter loads
through "matrix mode 1, position only, which never updates the vector matrix".
Wrong: libnds names mode 1 `GL_POSITION` and mode 2 `GL_MODELVIEW`, and the code
already used mode 2, so a vector matrix was always being written. Found when the
invented name `GL_MODELVIEW_VECTOR` failed to compile. The prerequisite survives
for a narrower reason — what landed in the vector matrix was the composed MVP,
and normals must not be rotated by a projection. Same fix, wrong cause.

Write-up: `docs/optimization/ClaudeOpus5_R203_E17_SplitMatrixLoad_20260728.md`.

## R2-03 E25 — PrepareProductionRun has no hot spot (2026-07-28)

Measurement, reusing E11's existing `NDS_R2_FIGHTER_RUN_PROOF=2` instrument.
480 frames, 62.8 runs/frame, ranking only:

| phase | ticks/frame | share |
|---|---:|---:|
| tail (field writes + batch begin) | 13,753 | 27.7% |
| texture prepare | 13,076 | 26.3% |
| UV | 12,206 | 24.6% |
| entry validate | 8,761 | 17.6% |
| texture reuse | 1,283 | 2.6% |

**Four roughly equal quarters**, so no partial optimization reaches the 42,281.
Each re-derives a fact E5 measured at **1.9% churn**: ~63 runs a frame each
rebuild a description of themselves that changed for one run in fifty.

That is exactly the switch plan's R2-03 bullet — *replace* PrepareProductionRun
with a per-epoch submit consuming baked facts, rather than optimize inside it.
E12 already proved the trade on the texture quarter alone (−32,724).

**The state replay and the prepare are one mechanism.** `TexPrepCount` is
46.4/frame against 62.8 runs — the full prepare runs on 74% of them — while
E12's texture memo hits 99.5% (`R2_TEXMEMO=1899,9,9,0,0`). The reconciliation is
that `ndsRendererNativeApplyStateDelta` calls
`NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE` on every OTHERMODE / COMBINE / TEXTURE
/ GEOMETRY / IMAGE / TILE delta, and E20 counted **194.4 delta applications a
frame**. The replay's job is to move state; moving state is what makes the
prepare expensive.

State replay 65,026 + prepare 42,281 = **107,307/frame, over half of what R2-03
still owes, and they are the same problem.** This reframes the last four
results: E20/E21 priced the delta *write* (cheap, ~280 ticks) when the cost is
the invalidation it triggers; E23 and E24 each removed one side of a coupled
pair and measured null. **Any cut that optimises one side while the other keeps
re-dirtying the state will read as null** — which is exactly the pattern
observed.

**E25c rules out the cheap fix.** Splitting the 194.4 applications by effect:
COMBINE 41.7, TEXTURE 35.7, LIGHT_COLOR 27.4, TILE 23.9, OTHERMODE 13.4, IMAGE
10.6, PRIM 6.7, GEOMETRY 2.0. Invalidating total **127.3/frame**, of which
**70.2 move the 20-word tile state** against 57.1 cheap scalars. A generation
counter would bump more often than there are runs (62.8), and hashing the tile
per run is a worse ratio than E8's losing memo. **There is no cheap validity key
for a value legitimately rewritten more often than it is read — the memo variant
is refuted, not deprioritised.**

**NEXT (unowned):** this is why the plan specifies a generated per-epoch submit
with *no traversal-state dependency* rather than a faster prepare or a cheaper
replay. The generator knows each epoch's final state at build time, so neither
the replay nor the re-derivation needs to exist at runtime. Breaking the
coupling cannot be done from either end alone, so the memo-only variant
(extending `sNdsR2RunTextureMemo` with `poly_fmt`, `scale_s/t`, `origin_s/t`,
`offset`, `vertex_flags`, `textured`) should be treated as a fallback, not the
plan: with 194.4 invalidations a frame still landing, its hit rate would be the
observed 26%, not E5's 98.1%.

**ITCM is now full**: E16 left 1,024 bytes free (31,744/32,768), and the census
and run-proof instruments together overflow it by 172. Measure with one at a
time; anything new on that chain needs `noinline` outside
`.itcm.native_fighter`.

Write-up: `docs/optimization/ClaudeOpus5_R203_E25_PrepareRunPartition_20260728.md`.

## R2-03 E24 — the shade's action walk is not the shade's cost (2026-07-28)

**NULL, reverted.** After E16 the shade's per-action loop is pure bookkeeping:
`stats->vertex_count` (a max) and `gNdsRendererProfileSourceVertexLoadCount` (a
sum), both pure functions of the static action table. E24 baked them per epoch
at load and replaced the walk with two lookups — the switch plan's "consume
baked facts" applied to its smallest piece.

**+2,048 FTR P50**, inside the placement floor; triangle counts unchanged, so it
worked and simply saved nothing. The census bracket for the shade reads 58,105,
but that is `ApplyMaterial` plus instrument overhead, **not** the walk. E18's
ranking-only caveat holds for the third time.

Redirects R2-03's remaining work to where the census actually points:
`SubmitPrepTicks` **42,281/frame** — `ndsRendererNativePrepareProductionRun`,
which is precisely the plan's R2-03 bullet ("no PrepareProductionRun policy
re-checks"). E5 already measured those facts at 1.9% churn and E12 memoised the
texture half for −32,724; the rest is UV and policy.

**Process failure recorded**: reverting E24 with `git checkout --` destroyed a
second agent's uncommitted bug-#10 probes in the same file. Recovered from the
21:23 snapshot, which archives the working tree. Rule added to
`TASK_STANDING_RULES.md`; eleven commits of hunk-filtering were undone by one
destructive command.

## R2-03 E16 — hardware lighting GRADUATED, −35,072 (2026-07-28)

The fighter's per-vertex software shade moves onto the DS geometry engine. Four
parts: a load-time `GFX_NORMAL` table, one `GFX_DIFFUSE_AMBIENT` per epoch
carrying the source light colours folded with the material and the damage flash,
`GFX_NORMAL` instead of `GFX_COLOR` in the emit, and `POLY_FORMAT_LIGHT0`.
Light 0 is white and the *source's* light colours are the material's diffuse and
ambient, so the engine evaluates the source equation rather than an approximation.

| bucket | A (E17 only) | B (E17+E16) | delta |
|---|---:|---:|---:|
| **FTR P50** | 489,536 | 454,464 | **−35,072** |
| WORK P50 | 1,099,328 | 1,063,360 | −35,968 |
| VBlank histogram | 2:381 3:171 4:11 5+:3 | **2:418 3:139 4:6 5+:3** | |

**Geometry bit-identical**: P0 181,440 / P1 173,502 in both arms. Engagement:
`gNdsR2LightVectorWrites = 1,114` (2/execute). Boundary green with the flag on;
ITCM 31,744/32,768. `NDS_R2_FIGHTER_HW_LIGHT` defaults 0 and requires E17.

**E16a**, which decided the design: the light direction changes **0 times in
22,296 epochs**, so `GFX_LIGHT_VECTOR` — which stores the vector transformed by
the vector matrix *at write time* — is written once per execute under an identity
matrix. Colours move on 32% of epochs and material on 72%, and both fold into the
single per-epoch `GFX_DIFFUSE_AMBIENT`.

**Three bugs, all worth their rules.** (1) libnds's `NORMAL_PACK` does not mask
its z argument, so a negative z sign-extends into bits 30-31 — the *light number*
in `GFX_LIGHT_VECTOR`. Every light vector went to light 3, which `POLY_FORMAT`
never enables, so light 0 kept a zero vector and the fighters rendered
ambient-only. (2) Only two of the **four** production emit paths were converted;
`PrimitiveGroups` and `CrossRun` kept writing a colour the shade no longer
updates. (3) Returning early from the shade skipped the action walk that
maintains `gNdsRendererProfileSourceVertexLoadCount`, which Boundary caught as
the complete-stage owner entering the generic transform path — geometry was
already identical, so only the harness saw it.

**Harness gap recorded**: no frame-locked cross-build capture exists.
`capture-melonds.ps1 -ExactFirstFrame` is gated to the Cut G GO-text window, and
live captures drift because the faster arm reaches a later match clock at the
same wall delay. Every rendering-side change from here needs one.

E17+E16 together are −52,672, 21% of R2-03's 250,833 gap. **The phase does not
close on them**, and what remains is not another cut of this kind: E22 showed the
per-root matrix loads are all distinct, E21 showed the state replay is not
redundant, and E18 capped the shade at 53,760 of which E16 takes 35,072. The rest
needs a structural change to what the fighter draw *does*.

Write-up: `docs/optimization/ClaudeOpus5_R203_E16_HardwareLighting_20260728.md`.

## R2-03 E22/E23 — the last unpriced item resolves into E16 (2026-07-28)

**E22 measurement stands. E23 implementation REVERTED — sub-floor.** The per-root
matrix bracket was the phase's last unpriced item. It is now priced, and it is
not where the money is.

480 presented frames, `NDS_R2_FIGHTER_HW_MTX=1`, per frame:

| counter | per frame | share |
|---|---:|---:|
| matrix loads performed | 30.0 | |
| elided by the existing generation check | **0.0** | 0% |
| **identical projection** | **29.0** | **96.7%** |
| identical modelview | 0.0 (6 in 480 frames) | 0.04% |

The modelview is genuinely per-root; the projection is per-frame camera data
re-pushed 29 extra times. E23 skipped it, proved engagement from the same run
(`Skipped=16,750 / Loaded=1,114`, 93.8%), and measured **−3,008 FTR P50 /
−4,800 WORK P50** — under the 5,000–7,000 placement floor, and matching a
first-principles estimate of ~2,900. Reverted rather than keep a hot-path
`memcmp` and 64 bytes of BSS for a delta the instrument cannot resolve from zero.

**The durable lesson: a redundancy's share is not its cost.** The repeated work
is GX FIFO traffic, and E14 already proved this path never backpressures. FIFO
writes are stores; stores are cheap. A 96.7% redundancy rate on cheap work is
worth less than a 7% rate on expensive work. Price one instance before pricing a
cut from a share.

Also: E22's first pass compared projection and modelview *jointly* and reported
**zero** redundancy. The 96.7% only appeared when the halves were scored
separately — E21's rule one level down, so a call that writes several things
needs one counter per thing.

**Two things settled for free.** E17's candidate and control emit
`P0HardwareTriangleCount = 136,640` over the same 480-frame window, identical to
the digit — E17 changes no geometry at all, a stronger check than it shipped
with. (E20/E21's "320/frame" was that quantity over a different window; over
439..919 the rate is 284.7. The control must come from the same window.)

**Queue after this: nothing is unpriced.** The per-root matrix load is ~6,000–
8,000 in total; the balance of that bracket is
`ndsRendererNativeApplyRootLightPreamble`, which is E16's territory. **E16 is the
only cut left in the phase**, and E22 folded the last open item into it.

Write-up: `docs/optimization/ClaudeOpus5_R203_E22_E23_ProjectionReload_20260728.md`.

## R2-03 E21 — the state-delta guard is REFUTED (2026-07-28)

**E20's cut does not exist. Do not build it.** The section below stands as the
measurement it was; this is what its own falsifier returned.

Every case in `ndsRendererNativeApplyStateDelta` writes `stats` purely from
`delta->w0`/`w1`, so identical operands to the previous application of that
effect means identical writes. Per-effect operand tracking, validity cleared on
every material application (conservative direction):

| counter | per frame | share |
|---|---:|---:|
| delta applications | 194.4 | |
| within-frame index repeats (E20) | 124.8 | 64.2% |
| **identical-operand applications** | **14.0** | **7.2% of applications, 11.2% of repeats** |
| of those, GEOMETRY | 0 | |
| material invalidations | 29.7 | |

**Only 14 of 194.4 applications a frame re-write what is already there** —
~3,920 ticks, below the 5,000–7,000 placement floor. The other ~110 "repeats"
are the same knob set to *different* values, which is necessary work. A guard
would pay a compare on all 194.4 to skip 14: **E8's shape**, which cost +16,301
and was deleted.

E19's structural check passes (P0 triangles 320/frame, control rate).

**The durable lesson: count identity of the write, not identity of the target.**
The two differ by 9x here, and the first produced a 35,000-tick opportunity that
does not exist. Third time this cycle a plausible headline survived until one
more counter was added — after E13's inert probe and E19's collapsed geometry —
each costing one build to catch.

**Queue after this:** E16 is again the only large cut identified in the phase
(35,000–50,000), E17 awaits visual approval, and the per-root matrix work
(~40,000, inflated bracket) is the only unpriced item left worth measuring. E17
already establishing E16's vector matrix now matters more, not less.
*(E22 has since priced that item and folded it into E16.)*

Write-up: `docs/optimization/ClaudeOpus5_R203_E21_StateGuardRefuted_20260728.md`.

## R2-03 E20 — the state replay repeats itself 1.8x a frame (2026-07-28)

**Superseded by E21 — the 64.2% is real but is not redundancy. Kept for the
reasoning trail.**

E19 refuted deletion as a pricing method, so this asks R2-02 F's question
instead: not what the phase costs, but how much of it is **redundant**.

479 frames, both fighters, per frame:

| counter | per frame |
|---|---:|
| state-span calls | 80.2 |
| **delta applications** | **194.4** |
| **repeats within the frame** | **124.8 (64.2%)** |
| distinct applications | 69.6 |
| span cost | 54,510 |

70 deltas exist and 69.6 distinct applications happen a frame: **every delta is
applied once for real and ~1.8 more times redundantly**, worth **~35,000
ticks/frame** at 280 ticks an application.

E19's structural check applied — P0 triangles 320/frame, its control rate — so
the arm measures what it claims.

~~**Worth 25,000–30,000 realised.** Best return-to-risk on the board.~~
**Withdrawn by E21: the realised figure is ~3,920, below the placement floor.**

**Falsifier before building, one build:** "applied twice in a frame" is not
"the second was a no-op" — something between them may have changed that state. So
the guard must be **value-based, not frame-based**, and the question is how many
of the 124.8 repeats write a value equal to the current one. If most write a
different value, this collapses the way E19's method did.

Write-up: `docs/optimization/ClaudeOpus5_R203_E20_StateSpanRedundancy_20260728.md`.

## R2-03 E19 — the state spans cannot be priced by skipping them (2026-07-28)

**Method refuted, no number produced.** `NDS_R2_FIGHTER_STATESPAN_SKIP`, default
0, must not be used to cost this phase.

E18's pricing method was pointed at the next ranked item and reported **−251,520
FTR P50** — five times the bracket, and fiction. The spans establish the texture,
polygon-format and geometry-mode state the emit requires; without them every run
is rejected before submitting. Hardware triangles went **320/306 per fighter to
8/0**, so the delta is ~618 triangles a frame ceasing to exist.

**E18 was re-checked against the same failure and holds:** its arm reads 320/306,
identical to control, so the shade skip removed the colour computation and
nothing else. **53,760 stands.**

**What the spans still need.** The only figure remains E15's bracket, ~52,000,
which carries that build's 10-20% inflation and is ranking-only. The right method
is **R2-02 F's, not E18's**: measure how much of the replay is *redundant* — how
many adjacent epochs re-apply state already current — which is the same question
R2-02 F asked of the stage's spans, needs a counter rather than a deletion, and
prices the achievable cut rather than the whole phase.

Write-up: `docs/optimization/ClaudeOpus5_R203_E19_StateSpanMethodRefuted_20260728.md`.

## R2-03 E18 — E16's ceiling is 53,760, not 90,295 (2026-07-28)

**Correction to the number this board carried as the phase's largest
opportunity.** `NDS_R2_FIGHTER_SHADE_SKIP`, lab only, default 0.

E16 priced hardware lighting at "most of 90,295" off E15's shade bracket. That
bracket came from a build carrying the whole E15/E16 census — whose own write-up
says its absolutes are inflated 10-20% and only its ranking is safe — and it
enclosed the per-epoch preamble as well as the per-vertex loop the cut replaces.

Measured directly by skipping that loop, both arms at `HW_MTX=1`:

| bucket | shade on | shade skipped | delta |
|---|---:|---:|---:|
| **WORK P50** | 1,099,584 | 1,044,800 | **−54,784** |
| **FTR P50** | 489,856 | 436,096 | **−53,760** |
| STG P50 (control) | 175,296 | 173,824 | −1,472 |
| **VBlank 2 / 3** | 381 / 167 | **431 / 123** | **+50 frames at 30 FPS** |

Engagement is unarguable: both fighters render as **black silhouettes** against
an untouched stage (`artifacts/visibility/ClaudeOpus5_R203_E18_ShadeSkip_silhouettes_20260728.png`).

**Ceiling, not expected value.** Hardware lighting still writes GX light and
material state per epoch or root, so the honest range for E16 is **35,000–50,000**.

**Does E16 still justify itself — yes, but it is no longer obvious.** 53,760 is
21% of R2-03's 250,833 gap, and with E17's 17,600 the two are ~28% of it. Nothing
identified in the phase is larger; the next ranked items are epoch state spans
(~52,000) and per-root matrix work (~40,000). But it is a four-part change with a
light-space risk against a 35,000–50,000 return, not the ~90,000 that was on this
board. **Sequencing: E17 graduates on its own first, and the epoch state spans
should be priced the same way before E16 is built** — they may be cheaper per
tick won.

Write-up: `docs/optimization/ClaudeOpus5_R203_E18_ShadeCeiling_20260728.md`.

## R2-03 E16 — the shade pass IS the DS's hardware lighting (2026-07-28)

Premise proven without exception, and it is the largest cut identified in R2-03.

`ndsRendererHardwareLitShadeColorPrepared` computes, per vertex,
`ambient + diffuse * dot(normal, light_dir) / 127` — with `light_color_1` as
diffuse, `light_color_2` as ambient, and the `rgba` field of the dense vertex
holding the **normal** (F3DEX packs normals there for lit vertices). That is,
term for term, the Nintendo DS geometry engine's hardware lighting equation.

Measured over 479 frames, both fighters:

| counter | per frame |
|---|---:|
| **lit epochs** | **48.5** |
| **unlit epochs** | **0** |
| epochs on the LUT path | 48.5 (100%) |
| epochs applying a material | 27.7 (57%) |
| **vertices lit** | **513.1** |
| vertices copied from a shared source | 21.5 |

**Not one fighter epoch in a match is unlit**, at ~169 ticks per shaded vertex.

**Why E1's refutation is explained rather than worked around.** E1 found the
shade output changes on 1,796 of 1,835 frames. It does: the light direction is
transformed into each root's local space by that root's modelview, the fighter
animates, so every dot product changes every frame. It is unmemoisable for a
structural reason — and that is exactly the problem DS hardware solves, by
setting the light vector once in view space and applying the current matrix per
vertex in silicon.

**`GFX_LIGHT_VECTOR`, `GFX_LIGHT_COLOR`, `glLight` and `POLY_FORMAT_LIGHT` appear
nowhere in `src/nds` or `src/port`.** The renderer has never used DS hardware
lighting, while E14 measured the geometry engine idle on 946 of 946 fighter
submissions.

Design: pack normals into `GFX_NORMAL` words at load time; set light and material
per root (~28/frame) instead of per vertex (~534/frame), folding `color_modulate`
into the material; emit the precomputed normal word instead of the computed
colour word — **one FIFO word either way, traffic unchanged**. Expected: most of
90,295 ticks/frame.

**Prerequisite the design does not survive without.** The fighter loads an
identity projection plus the **CPU-composed MVP** as the modelview, through
`ndsRendererHardwareSetMatrixMode(GL_MODELVIEW)` — mode 1, position only, which
**never updates the vector matrix**. Normals are transformed by the vector
matrix, so a naive `GFX_NORMAL` would light against whatever was left there, and
loading the composed MVP into it instead is equally wrong because normals must
not be rotated by the projection.

Fix: load projection into `GL_PROJECTION` and modelview into
`GL_MODELVIEW_VECTOR` (mode 2). The plumbing exists —
`NDSRendererNativeFighterRoot` already carries both `composed_matrix` and
`modelview_matrix`, and only the projection needs adding. The row-3 unit scaling
commutes with the right-multiply by the projection, so split loading reproduces
the current transform exactly, modulo hardware-versus-CPU rounding. The light
vector is then written once per frame in view space while the vector matrix is
identity.

**SETTLED — the compose is deletable, so the matrix change ships first.** Traced
every `state->matrix` / `matrix_valid` reference in `nds_renderer.c`. Under
canonical mode 9 the composed matrix has exactly two consumers: the hardware load
at 23773 (the call being replaced) and a `matrix_valid == 0` **flag test** at
17594 in `ndsRendererNativePrepareProductionRun`. Every other reference —
`ndsRendererNativeLoadVertexBlock`'s CPU vertex transform via
`ndsRendererNativeApplyVertexActions` (sole call site 18838),
`ndsRendererComposeMatrix` via `ndsRendererNativeBindOwnerRootState` (18789),
both inside the non-production `ndsRendererExecuteNativeFighterRootHardware`;
plus hierarchy mode 7 at 19174/19473 and the generic DL interpreter — is
unreachable from `ndsRendererExecuteNativeFighterOwnerProduction`.

So `ndsRendererAdapterComposeNativeRootMatrix` can be deleted and a 4x4 multiply
per root leaves the 120,407 MatrixPrep bracket **independently of the lighting**.
The matrix change is therefore its own graduation, and the correct ordering is
matrix first, lighting on top — cheaper and far less risky than one four-part
change. The replacement must keep `state->matrix_valid` TRUE for the 17594 test
and carry a generation key equivalent to `state->matrix_generation` so the
existing hardware-matrix de-duplication still elides redundant loads.

**NOT IMPLEMENTED.** It touches the matrix mode, the load-time table format, the
emit's per-vertex word, and per-root light/material state. Being a rendering-side
change it gates on a screenshot pair plus **the owner's visual approval**: the DS
light model is not bit-identical to the N64's and colours will shift slightly.
`PROJECT_GOAL.md` lists "simplified lighting" among the allowed compromises, but
the call is the owner's.

Write-up: `docs/optimization/ClaudeOpus5_R203_E16_ShadeIsHardwareLighting_20260728.md`.

**Next: implement E16 behind a flag, capture the A/B screenshot pair, and put it
in front of the owner.**

### Open, not chased: GXSTAT bit 15 is set

Matrix stack overflow/underflow error latched at least once during a normal
match. It is a sticky flag and may date from init or teardown rather than
gameplay, and nothing observable is wrong. Recorded because it is an error bit
that is on.

## One fighter is worth ~400,000 ticks (owner observation, 2026-07-28)

**Superseded by the section above — measured at 271,424 WORK P50, not ~400,000.
Kept for the reasoning trail.** The inference below was sound but read the
quantization boundary as the whole cost; the actual transition needed less than
the boundary implied because `WAIT` absorbed part of it.


The owner noticed that knocking Fox off-screen, so he stops rendering, takes the
build to **~29 FPS** from ~20. That is not a small effect and it is arithmetically
informative.

The frame is VBlank-quantized at 560,190 ticks. Wall is **1,531,768** = 2.73
VBlanks, which rounds up to 3 → 20 FPS. Landing on 2 VBlanks needs wall
**≤ 1,120,380**, a saving of **~411,000**. Removing one fighter produced exactly
that transition, so **one fighter costs on the order of 400,000 ticks/frame** —
render, pose, matrices and everything downstream.

Two consequences.

**`PROJECT_GOAL.md` §7's budget table looks mis-proportioned.** It allots 250,000
to *combined* fighter rendering and 100,000 to fighter pose. Two fighters at
~400,000 each is ~800,000 of a 1,120,000 frame. Either the budget or the
implementation is wrong by a factor of two, and the budget was never validated
against a measured per-fighter cost.

**It is also a free instrument.** Suppressing one fighter's draw is a controlled
A/B that the tick-HUD reads directly, and it partitions the per-fighter cost into
render versus pose versus matrix without any new probe. Queued as the next
measurement after the R2-03 gate, reporting the 2/3/4/5+ VBlank histogram and max
interval per `AGENTS.md` — never min FPS.

Recorded as an owner observation, not a measurement: the FPS figure is a HUD
reading, and the ~411,000 is inferred from the quantization boundary rather than
bracketed.

**Outcome (E13).** Built as `NDS_R2_DRAW_SUPPRESS_MASK` and run. The observation
reproduces exactly — the median frame moves from three VBlank intervals to two,
and the 2-interval share goes 217/566 to 458/566 (histogram `2:458 3:102 4:5
5+:1`, max 17). The cost is **271,424 WORK P50**, not ~400,000.

The frame is also **CPU-bound, and this pair is what proves it**: `WAIT` went
*up* when Fox stopped drawing, 246,720 to 271,232. A rasterizer-bound frame that
loses a quarter of its pixel load waits less; a CPU-bound frame that loses
271,424 ticks of ARM9 work finishes earlier and waits longer.

## R2-03 E11/E12 — the fighter had no key for the cache that already existed

`ClaudeOpus5_R203_E11_PrepareRunSplit_20260728.md`,
`ClaudeOpus5_R203_E12_RunTextureMemo_20260728.md`.

**`PrepareProductionRun` 82,042 → 49,318 ticks/frame; the texture prepare inside
it 45,952 → 12,362.** Graduated to the published block.

E5 proved this function is a pure function of `run_index` and then declined to
build the memo because "~119 UV writes/frame can't explain 21,504 ticks". The
arithmetic was right and the premise was wrong: **a census row is self time.**
E5's bracket read ~21,500, the frame census row read 22,205, and four brackets
inside the function read **82,042** — the difference being the texture resolver
it calls out to, which the census charges separately to
`ResolveOrBindTexture` (18,803) and `SyncTextureTile` (12,004).

The cut is not a new mechanism. The resolver already opens with a site cache
keyed on `state->source_command_site`; the native fighter path does not
interpret display lists, so it has no site and has **never once hit that cache**.
The memo is the same cache re-keyed on `run_index`. R2-05 gets it for free.

| counter | value |
|---|---|
| memo hits | 1,074 (8.4/frame — every textured call) |
| fills | **9 in total, not per frame** |
| stale entries | 0 |
| mismatches, level 2 | **0 of 1,083** |

Nine distinct textured runs, resolved once each for the whole match. Predicted
35,000–45,000 in E11 before building; delivered **−32,724**, recorded as
measured rather than rounded into the band.

Three rules added to `TASK_STANDING_RULES.md`: check whether an instrument
measured the symbol or the work before rejecting a candidate as too small; ask
of every shared cache a native path inherits what its key is and whether this
caller has one; and a default-off `#if` does not hide a probe from a
source-level checker.

## R2-02 F — generic emit split, and the stage target moved

`ClaudeOpus5_R202_F_GenericEmitSplit_20260728.md`, `ea6b1fc`. The per-segment
counters existed and had never been read. **Segments 1/2/3/6 — Whispy's eyes and
mouth, both flower beds — cost 43,998 ticks/frame for 21 triangles**, against
segment 4's 22,843 for 76. At 2,095 ticks per triangle they, not segment 4, are
the largest remaining stage lever; the "segment 4 is the largest" line below is
superseded. R2-02's plan already named them ("small specialized update+draw
path") and that path was never built.

Three cuts refuted on the way, each with a number:

| candidate | measurement | verdict |
|---|---|---|
| merge adjacent runs | 1.0 of 21 repeats the previous state; 18 rebind a texture | dead, ~1,200 |
| revive Task 51 | 0.0 triangles take the path, 1,634 ticks/frame failing, emit +4,754 | structurally dead |
| guard the texture bind | 21,978/frame over 54 runs, both guards already present | near the floor |

Task 51's 2026-07-23 kill named its own revisit condition — find a scene where
bindings 20–29/33–38 submit GX — and that condition is now met. It still fails,
for a *different* reason: `Task51EnsureWorld` rejects on `task36_segment_active`,
and only a rigid binding opens that bracket, which an actor segment does not
have. Pinned so the next reader does not repeat the three builds.

The texture-bind floor caps the actor rewrite near **30,000**, not 44,000.

## R2-03 E5 — the premise is proven, the obvious cut is not worth building

Three counters over one canonical match settle whether R2-03's baked-facts
submit is possible (`ClaudeOpus5_R203_E5_RunFactMemo_20260728.md`,
`de34e051181`, `12968f83dd2`, `fad10d4cf91`):

| question | measurement | answer |
|---|---|---|
| do a run's facts ever change? | 0 misses / 112,300 calls | no |
| does the function ever reject? | entry 112,367 == success 112,367 | no |
| does a UV write ever change anything? | 0 changes / 208,874 writes | no |

`ndsRendererNativePrepareProductionRun` is a pure function of `run_index` in the
canonical configuration. The switch plan's "consuming only baked facts, no
policy re-checks, no per-frame texture identity proof" is achievable, and the
table can be generated rather than discovered.

**But the obvious implementation banks nothing.** The UV loop is only ~119
writes/frame — about 13 of the 67 runs are textured, touching 106 of 541 dense
vertices — which is low thousands of ticks against the bucket's 21,504. The cost
is spread across per-call validation and `texture_prepare_*` bookkeeping, and
`texture_prepare_valid` is already a cache with 44 invalidation sites. Building
a memo for the arithmetic was dropped on the measurement rather than attempted.

Next on this phase is an internal cost split of the function, not an
implementation. Two side effects any memo must preserve are recorded in §4d of
the writeup: the GX bind at `:17313` and the harness-visible texture-prepare
counters.

**The bigger fighter lever is MatrixPrep at 91,338/frame** — four times this
bucket, moving every frame, and where the bulk of the ~460K gap to the 1.12M
gate has to come from.

## Runtime 2 (2026-07-27)

The owner approved `Smash64DS_Runtime2_SwitchPlan.md` and it is now the live
renderer direction; `optimization/archive/NATIVE_RENDERER_PLAN.md` is history.
R2 phases are rows here, measured under `TASK_STANDING_RULES.md`.

| phase | state | evidence |
|---|---|---|
| R2-00a stall attributor | **done, gate met** | `optimization/ClaudeOpus5_R200a_StallAttributor_20260727.md` |
| R2-00b re-baseline + budgets | **done** | `optimization/ClaudeOpus5_R200b_BaselineAndBudgets_20260727.md` |
| R2-01 battle-path skeleton | **done, gate met** | `NDS_R2_PATH`, `src/nds/r2/`; Boundary green |
| R2-02 Dream Land direct runtime | **stage budget MET — 177,088 vs 180,000; E1a/E2/E7/E8 shipping** | `optimization/ClaudeOpus5_R202_E8_PreflightElision_20260728.md` |
| R2-03 fighter direct draw | **unowned — not started** | |
| (R1 harvest) hardware sqrt | done, KEEP | `optimization/ClaudeOpus5_R203_E1_HardwareSqrt_20260728.md` (filename mislabels it R2-03) |

**R2-02's stage budget is MET.** `STG` P50 is **177,088** against the 180,000
provisional budget — 2,912 under — after E1a, E2, E7 and E8. E3 is retracted and
E4 refuted its whole approach; neither contributed. The two soft-float files
named `R2-03` are Runtime 1 harvest, not that phase; they are corrected in place
per the never-rename rule.

```text
STG P50   351,488  baseline
          256,704  after E1a  (-94,784)  prepare-run elision      -- clean
          224,320  after E2   (-30,912)  GXFIFO DMA rigid replay  -- clean
          212,480  after E7   (-11,840)  view-projection hoist    -- bit-exact
          177,088  after E8   (-35,392)  preflight elision        -- bit-exact
          180,000  budget
          -------
           -2,912  UNDER

         (173,120  E3 and E4-C both  (-51,200)  BOTH REVERTED: that number is
                                                the price of not drawing the
                                                flowers, not of drawing them
                                                faster)
```

**E8 is the first arm that followed §7 rather than optimising around it.** For
the five segments the Task 36 replay does not serve, the owner preflight cleared
a 1,292-byte `NDSRendererStats`, initialised a traversal state, and replayed 21
run-level and 16 binding-level state spans to produce a `preflight_stats` and a
traversal state that **nothing reads** once E1a's prepared run table is valid:
`CapturePreparedSegment` early-returns for an ineligible segment, and
`sNdsNativeStageOwnerExecution.traversal` is referenced nowhere outside the
function. The one member that escapes the loop, `sync_command_count`, is now
memoised beside `epoch_mask`. Task 104 had written that sentence down already,
one level lower and for three segments; it was true of the other five and of the
whole loop body. Engagement reads exactly **5 elisions per frame**, and the Task
36 replay stays READY at its full 3,916 words.

Pacing: **2-VBlank frames 13 → 198 of 565**, `WAIT` P50 −202,368, `WORK` P95
−77,504. The DS top screen is **pixel-identical** to the pre-E8 arm — 0 of
121,600 pixels — at presented frame 500 and at the `time_remain` 1800
simulation-clock lock, against a control arm proven reproducible run-to-run.

**All three kept cuts now ship.** `NDS_R2_STAGE_DIRECT`, `NDS_R2_STAGE_DMA` and
`NDS_R2_STAGE_VIEWPROJ` are default-on in the published
`smash64ds-battle-playable-hwtri` block and in the `tickhud`/`proof` block —
`STG` P50 **351,488 → 212,480, −40%**, and the frame moves off the 3 VBlanks the
previous shipping ROM sat at. None of the three spends the fidelity budget, so
none of them needed the owner's visual-oracle call: that clause governs
approximations, and these are exactness-preserving. E7 is bit-identical to its
control on all 42 composed matrices at six frames spanning the camera's range of
motion. The tick-HUD block sets the three *without* `override`, deliberately —
they are the live A/B surface for the rest of the phase — and the graduated
default tick-HUD build hashes `DFBE1ED0E2BB97DB`, byte-identical to the explicit
lab build, so measurement and shipping are the same binary.

**E7 also corrected a wrong rationale that had already been written down twice.**
The cut was designed as an associativity hoist that would spend the Task 49
Tier-2 pixel budget. Dumping `binding_composed` out of both ROMs showed no delta
at all: `ndsRendererAdapterBuildCameraMatrices` already returns
`projection = MtxMul(lookat, persp)` with `modelview_valid` FALSE for the battle
camera, so the compose was `world × (lookat × persp)` — **one multiply per
binding, never two** — and the −11,840 is the per-binding camera-cache lookup and
three 64-byte `MTXCOPY` memcpys that stopped happening. Both E6 and E7 were
designed against arithmetic and both resolved to memory traffic. **Do not size
the next stage matrix lever by counting multiplies.**

**The mechanism, established by E4** —
`optimization/ClaudeOpus5_R202_E4_ActorSegmentsRefuted_20260728.md`. A **rigid**
binding's captured stream is `PUSH` + `MULT4x4` of a constant world under the
camera the segment bracket loads live every frame, so it replays. A **dynamic**
binding's stream is a `MATRIX_LOAD4x4` per triangle of projection × view × model,
so replaying it pins that geometry to the camera the capture frame happened to
have — which is exactly why the flowers sat in a fixed screen band under every
camera. Hence the invariant, now written into both masks:

> `NDS_TASK36_REPLAY_SEGMENT_MASK` must name exactly the segments whose every
> binding is in `NDS_RENDERER_TASK36_RIGID_BINDING_MASK`.

E3 broke it by widening one mask. E4 arm C restored it by widening both — and
lost the flower beds anyway, for an unrelated reason: **the rigid emit path is
single-binding by construction.** `ndsRendererNativeStageEmitNoZTriangle` drops
a triangle whose corners are not all bound to the run's own binding, and the two
flower beds are the only cross-matrix geometry on Dream Land — 10 of their 15
triangles. That is the `cross_matrix_triangles=10` that
`M3_NATIVE_STAGE_CHECK_OK` prints on every Boundary run, and it had been on
screen the whole time.

It is also why the flowers are expensive: a cross-matrix triangle falls to the
generic tail, which loads a composed matrix **once per vertex**. 15 flower
triangles cost 35 matrix loads a frame; Whispy's 12 single-binding triangles cost
12.

Two hypotheses died cheaply on the host and should have died before E3 landed:
every actor triangle carries coordinate shift 0 (so Task 51's missing shift
compensation is irrelevant), and `NDS_TASK51_STAGE_NATIVE` defaults to 0 and is
compiled out of every ROM measured (so E3's premise — "Task 51 already baked
those world matrices" — was false).

**Nothing shipped.** `NDS_R2_STAGE_ACTORS` is deleted. The published ROMs are at
defaults and Boundary-green at **62.750%**, `stage_body` green 44.848% / detail
52.242%. One real defect was found and kept: replay asserted
`task36_local_pushed = TRUE` for every run, so each admitted actor segment bought
an unmatched `glPopMatrix(1)`. Capture now records the run's actual `PUSH`/`POP`
balance.

**The stage partition, re-measured on the graduated program 2026-07-28**
(`census-stage-run-phases.ps1`, frames 439–499, `build-r2-02-census-e7`, total
242,574). This is what E8 was aimed from, and what the *next* stage arm must be
aimed from — the majority is no longer preflight:

```text
prepare owner                 111,849   46.1%
  prepare matrices             42,557          (54,901 before E7)
  renderer prepare owner        49,840
    apply state span             20,370   21 calls @   970   <- E8 elides
    init stats + traversal       13,565    5 calls @ 2,713   <- E8 elides
    unattributed                 13,721          (16 binding-level state spans)
    prepare run                     995   21 calls @    47   (E1a: was 98,828)
  validate task36 world          8,588
  prepare materials              5,623
display commit                130,219   53.7%
  generic emit                   67,126   21 runs @ 3,196, 103 tris @ 652
  replay                         29,124   33 runs @   883
  loop overhead                  13,120   54 iterations
  per-segment scaffolding        13,852    8 commits @ 1,732
```

**The next stage lever is `generic emit`, 67,126 ticks/frame** — the 21 runs and
103 triangles the Task 36 replay does not serve, at 3,196 per run against the
replay's 883 and 652 per triangle against ~294. E4 established it cannot be
reached by widening the replay masks. layer1 (segment 4) is 76 of those 103
triangles across only 6 of the 21 runs, so the cost is per-run dominated and the
15 actor-segment runs are the expensive half.

The older defaults-build partition below (total 401,506) is retained only as the
pre-E1a reference; do not aim new work from it.

```text
prepare owner (preflight)     238,609   59.4%
  renderer prepare owner        165,045
    prepare run                  98,828   21 calls @ 4,706  <- E1a takes this
      head policy/memset/tex       69,379
      dense vertex loop            22,339   143 dense @ 156
    apply state span             30,117   21 calls @ 1,434  <- NOT elided by E1a
    init stats + traversal       16,793    5 calls @ 3,359  <- 1,292-byte clear
    task36 reuse check              693
    validate topology               610
    unattributed                 18,004
  prepare matrices               54,242   16 dynamic bindings @ ~3,390
  validate task36 world           8,577
  prepare materials               5,675
  config / frame setup            2,523
display commit (actual submit) 162,399   40.4%
finish owner                       498
```

**Read this against §7's actual instruction, which has not been followed.**
R2-02 says the static majority becomes *"a fully direct owned path: no generic
preflight, no stats temporaries, no per-frame texture resolution; the runtime
shape is `DreamLand_Run17()`, not discover/validate/rebuild/resolve/prepare/
submit"*. E1a, E2, E3, E4 and E5 all optimised the discover/validate/prepare
pipeline instead of replacing it. Segment 0 already has the prescribed shape —
`ndsRendererNativeStagePrepareGeneratedSegment0`, gated by
`NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE`. Segments 1–7 do not. **Extending
that generated program to the remaining segments is the phase's own design and
is the credible lever §8 asks for before any budget is relaxed.**

Ranked by size, and all of it is preflight the direct path deletes rather than
optimises:

1. `prepare matrices` **54,242** — `ndsRendererAdapterPrepareInitialMatrices`
   walks each dynamic binding's DObj parent chain every frame. The flowers'
   worlds are provably constant (E4 arm C: the runtime rigid-constancy check
   accepted them), so this is recomputing a known-constant world and then
   composing the camera onto it. Splitting camera from world is what Task 36
   already does for the 26 rigid bindings via `MULT4x4` under a once-loaded
   camera. Largest single item and the most clearly structural.
2. `apply state span` **30,117** — 21 calls E1a's `r2_reuse` memo does not
   cover, because it sits outside that guard. Careful: it mutates the running
   `state` that later runs consume, so it cannot be skipped per-run without
   also proving the successor's incoming state.
3. `init stats + traversal` **16,793** — a 1,292-byte blanket clear plus
   traversal init, 5× a frame, for the five segments Task 104's elision does not
   reach. §3.4 names this shape explicitly; extend Task 104's pattern.
4. `unattributed` **18,004** inside the owner span — uncensused, and bigger than
   item 3. Bracket it before assuming it is small.

Items 1–3 total 101,152 against a 44,320 requirement, so the budget is reachable
without relaxing it — the work is structural, not another memo.

**Also on this row — de-cross the flowers in the generator.** For each of the 15
foreign corners emit a duplicate dense vertex pre-transformed into the run's
binding space (`v' = W_run⁻¹ · W_foreign · v`, a compile-time transform because
both worlds are constant). That is +15 dense vertices, no new runs or triangles,
and it makes every flower triangle single-binding. Only then widen
`NDS_RENDERER_TASK36_RIGID_BINDING_MASK` and `NDS_TASK36_REPLAY_SEGMENT_MASK`
together; gate the transform on the Task 49 Tier-2 differ (the inverse-multiply
is where fixed-point error enters); verify with a frame-locked crop of segments 3
and 6 plus a triangle count and `task36_runtime_rigid_mask` read from the run
that produced the buckets. Whispy (20–24) is out of scope — materially animated,
and at 12 single-binding triangles it was never the expensive half.

**Closed 2026-07-28: the R2-02 flags are graduated and the published ROMs carry
them.** This row previously asked the owner to make that call. It was the wrong
ask — the owner is the visual oracle for changes that *spend the fidelity
budget*, and all three of these are exactness-preserving, so the decision was
the verifier's and not a matter of taste.

**What else is left in the stage.** layer1 (segment 4) is 22,738 ticks/frame for
76 triangles and is still generic: its six runs submit through the raw composed
matrix (binding 29, submit classes 0 and 6), which is the camera and genuinely
moves. Moving it onto the segment-bracket path is generator work worth ~19,000 —
now the *largest* remaining stage lever, and still not enough for the gate alone.

**R2-03 is owned; E0–E3 are done and the target is named.** The frame is
re-baselined on the post-R2-02 program: **REAL WORK 1,264,844 against the
1,120,000 budget, gap 144,844** (it was 407,000 at Task 65).

**The sizing is finished and the target is one span: fighter matrix
preparation, 91,338 ticks/frame — 63% of the gap.**
`ndsFighterMarioFoxDLAllDrawForSlot` costs 497,231 ticks/frame inclusive (the
census's 37,206 is self time); the split is walk 3,138, reset 6,675,
revalidation 9,916, owner prep 113,855 (**matrix 91,338** + material 21,504),
submit and tail 361,936. Two independent builds agree to 0.6%.

Three named mechanisms cover 93% of the gap: fighter matrix prep 91,338,
fighter material prep 21,504, stage layer1 22,738. The matrix half is not a
memo — the pose moves every frame — it is §7's generated per-epoch submit
consuming baked facts, and soft-float's 177,503 is largely the same ticks
counted another way (the source's joint transforms are float; the render side
converts them to 20.12 every frame). The material half *may* be a memo and
deserves the E3 falsifier first, at one build for ~21,500.
`optimization/ClaudeOpus5_R203_E4_MatrixPrepIsTheTarget_20260728.md`.

Two candidates were closed on the way, both with evidence rather than opinion:
the shade loop is **not** a memo (inputs and outputs both changed on 1,796 of
1,835 frames), and walk + revalidation is 4% of the function, not the 37% a
self-time-versus-inclusive mix-up made it look like.

**R2-03 E1 took `sqrtf` from 15,760 to 9,720 ticks/frame, −6,040**, bit-exact
against IEEE over 8.7M checked inputs, Boundary green. The 8-frame A/B read
**flat on every bucket** — the saving sits inside the 5,000–7,000 placement
floor, and the symbol census is what resolved it. That constructive half is now
in `TASK_STANDING_RULES.md`: when the predicted saving is near the floor, gate
on the census, which times the function directly and has no placement term.

Only 38% though, not the 17× the hardware's 13-cycle latency suggests: libnds's
`sqrt64` is write / **poll-busy** / write / **poll-busy** / read, and the I/O
polling costs about what the software root did. **On this hardware a coprocessor
is only worth it if the result can be collected without spinning on it.**

**R2-02 E1a took `STG` P50 −94,784, down on 128/128 frames**, and 4-VBlank
frames fell 50 → 12 out of 566. Boundary green; required-region detail 62.792%
vs 62.778%. `NDS_R2_STAGE_DIRECT`, default 0, owner visual approval outstanding.
`STG` is now 256,704 against the frozen 180K budget — gap 76,704, was 171,488.
E2 is `ndsRendererAdapterPrepareNativeStageMatrices` (55,077 bracketed), which
is **not** frame-invariant: the camera moves, so a reuse key will not work
there and E0's sizing method does not transfer.

### CLOSED — the gate metric is sound (R2-00c, 2026-07-27)

**R2-00a's phantom-work finding is refuted, and the row it opened is closed.**
It compared halt measured in a profile ROM against `WAIT` measured in a
different tick-HUD ROM; placement differs between builds, so a frame index does
not name the same workload in both. One ROM carrying both instruments
(`NDS_TASK37_PROFILE_PER_FRAME_REGION=1`, new) settles it over 128 frames of one
run: `ALL` agrees to **0.04%**, `WAIT` to a constant **−851 ticks/frame**, and
the 27 excursion frames (median −860) are no different from the other 100
(median −847). `WORK-H` P95 is not inflated by a mis-scoped bracket; the 1.12M
gap is real. Evidence:
`optimization/ClaudeOpus5_R200c_WaitBracketAudit_20260727.md`.

**What replaced it is a real optimization row.** The excursion is genuine
execution — `armWaitForIrq` falls 323,450 ticks/frame and **+286,619** of work
takes its place, on 21% of frames — and the same per-frame regions attribute it:
softfloat ~49,600, **the tick HUD measuring itself ~44,300**, cart read +
relocation + bulk copy ~36,000, geometry submission ~14,500, collision ~5,700,
animation ~2,700, then a diffuse tail over ~59,000 PCs. Four unrelated causes on
the same frames, which is why five previous tasks found no single mechanism.

Two consequences worth acting on:

- **The frames are not load-free.** `_ntrcardRecvByCpu` + `ntrcardRomRead` are
  12,639 ticks/frame higher there. Task 75's preload targets something real, but
  its ~103,488 estimate must be re-derived against the measured ~36,000.
- **`WORK-H` cannot remove all of the instrument.** `ndsPlatformTickHudSample()`
  runs after the buckets are latched, so the percentile sort (19,605
  ticks/frame) lands in the *next* frame's `ALL`. ~2% of the P95, not 33%, but
  it is the metric charging the ROM for being measured.

R2-00a's other findings stand: no GX, DMA or cart *stall*; ledger closed;
bit-identical reproduction of the prior census.

### The frame, re-ranked on attribution that holds (R2-00c §7)

`task65_subsystem_census.py` named functions with `addr2line -f`, which resolves
through DWARF — and DWARF still describes functions the linker
garbage-collected. It charged 24,240 ticks/frame to `ndsRendererTask29GXRecord`,
which is not in the binary. The census now bisects the ELF symbol table and
overrides addr2line; that **renames 18,987 of 59,366 PCs, 32%**. Aggregates
survive (REAL WORK 1,446,638 vs R2-00b's 1,446,348, 0.02%); the per-symbol table
did not, and that is what targets are picked from.

| group | ticks/frame | % of work | cyc/insn |
|---|---|---|---|
| soft-float | **177,857** | **12.3%** | 1.19 |
| matrix | **156,627** | **10.8%** | 2.35 |
| gx-submit | 144,852 | 10.0% | 2.72 |
| texture-resolve | 108,681 | 7.5% | 4.91 |
| `mem*` | 98,207 | 6.8% | 2.60 |

**Soft-float is the largest block and it is not stalled** — 1.19 cyc/insn, and
`__aeabi_fadd` is already hand-written ITCM assembly. Nothing to win by making
it faster; the only lever is calling it less, i.e. float→fixed at the call sites
in imported gameplay and animation. **Matrix construction is 156,627, not the
55,077 R2-02 E2 was sized at** — the bracket saw one call, the census sees seven
symbols across stage and fighter. Re-scope E2 against that.

The attributor is installed repo-local at
`emulators/melonds-attributor/melonDS.exe` (`D81FC0BF…`) rather than replacing
`emulators/melonds/melonDS.exe`, so measurements taken with `DE80E46B…` stay
comparable. `check-melonds-policy.ps1` passes with it present.

**R2-00b replaced the stale Task 65 baseline.** REAL WORK is **1,446,348**
ticks/frame, not 1,527,277; the gap to the 1.12M gate is **326,348**, not
407,277. Stall is 62.1% of work (memory 555,943, non-memory 342,494), so the
architectural premise is unchanged — memory stall alone still exceeds the whole
gap.

It also corrected an attribution defect Task 65 shipped: `task65_subsystem_census.py`
filed `src/port/reloc_backend_renderer_dl.c` under `PORT/reloc`, charging
**147,777 ticks/frame of renderer adapter work to a bucket named after
loading.** Corrected, **the renderer is 723,554 ticks/frame — 50.0% of the
frame's work** — and all gameplay is 190,649 (13.2%). Any plan built on Task
65's §2 table under-counted the renderer by that amount.

Note for every future phase gate: the census attributes by where code lives and
the tick-HUD buckets attribute by bracket. They are not interchangeable, and the
shared kernels (616,701 ticks/frame) are what differ between them. State which
view a gate quotes.

## Red Queue

1. **Stable 30 FPS:** qualify representative active gameplay at
   P95 <= 1.12M ARM9 ticks per presented frame on the accuracy-focused custom
   melonDS fork. Hardware remains the final check for mechanisms the emulator
   cannot referee.
2. **Mario/Fox completeness:** replace battle-reachable weak status callbacks
   with source-backed behavior and prove both complete movesets naturally.
3. **Dream Land completeness:** close the remaining Whispy material/animation
   presentation debt without reintroducing gameplay-time texture conversion.
4. **Audio completeness:** implement or explicitly qualify every reachable
   voice, pitch schedule, composite cue, and overlapping match-audio path.
5. **Final acceptance:** run the CPU-on one-minute match, complete-match capture,
   owner play/listen pass, reserve gate, Results transition, and teardown proof
   on the exact candidate ROM.

**Performance lane (2026-07-28):** `WORK-H` P95 **1,579,584** after R2-02 E8,
against the 1,120,000 gate — gap **459,584**. (It was 1,647,424 after Task 104;
E7 and E8 took the rest.) `WORK` P50 is 1,163,328 and P95 1,592,320. VBlank
intervals 2:198 3:349 4:14 5+:4 of 565, max 18 — the median frame is still three
intervals, but 35% now present in two where 2% did before Runtime 2. Two search spaces are closed by measurement — exactness-preserving
(Tasks 78–96) and visual approximation in its payload form (Tasks 98–99). The
raster axis was opened in `optimization/RASTER_AXIS_CAMPAIGN.md` and **Task 100
closed it at the first test** — a quarter of the frame's pixels stopped being
drawn and `STG` moved −320 against a ≥40,000 criterion, for the architectural
reason that the DS rasterizer consumes already-swapped polygon RAM and cannot
stall the CPU. Pixels join words and triangles; do not propose another fill,
coverage, AA or overdraw lever.

**Task 103 ran and moved the lane.** Partitioning `STG` in place found that
Tasks 51–55, 99 and 100 all worked the run loop, which is only 35% of the
bucket; **61% (238,254 ticks/frame) is outside the segment commit entirely, in
the owner prepare path, and has never been profiled.** It also found the 21
generic runs the Task 36 replay does not serve cost 63,903 ticks for 103
triangles, and that GX words cost 9.51 ticks each — retiring Task 55 E2's "words
are free" as a below-noise null.

E3/E4 then closed the attribution exactly — all four writers of
`gNdsTickHudStageTicks` tapped with zero added instrument, partition closing to
192 ticks (0.05%) against the build's own `STG`.

**Task 104 took the first cut out of it — KEEP, default on, Boundary green.**
On each of the three Task 36 replay-hit segments the owner cleared a 1,292-byte
`NDSRendererStats` and then overwrote all 1,292 bytes with a copy, to transport
**four live bytes** (`sync_command_count`, the only member read after the segment
loop). Eliding both accesses: `STG` P50 **−22,016**, `WORK-H` P50 **−26,240**,
P95 **−28,352**, VBlank 4-interval **39 → 28**, `FTR` flat. `WORK-H` P95 is now
**1,647,424**. Detail in `optimization/ClaudeOpus5_Task104_FourLiveBytes_20260727.md`.

That result also explains Task 103 E7's 28% realisation and produced a standing
rule: **size a memory lever by bytes that stop being touched, not instructions
that stop executing** — removing one of two accesses to the same cache lines
relocates the misses rather than eliminating them.

**Task 105 then closed the rest of that axis at E0, for one census run and no
builds.** `memset`'s residue is ~16,018 ticks split five ways (Task 84 E1.3
priced `InitStats` at 72% of the family's time), and a re-attributed `memcpy`
census found ~294 matrix copies/frame across five sites worth 3,300–10,600
nominal each — every one discounting to 1,000–3,000 under Task 104's own rule,
below the floor. Two rows in that census are inlined-range artifacts and are
marked as such. **The memory-traffic axis is harvested;** the residue is
structural, in `NDSRendererMatrix20p12` being 4×4/64 B for affine transforms the
DS loads as 4×3/48 B, and is not worth a Runtime 1 refactor.

**Task 106/107 E0 then sized the last untested large lever and re-aimed the
lane.** A 30 Hz simulation (`NDS_TASK106_UPDATES_PER_PRESENT=1`, default 2,
nothing shipped) is worth `WORK-H` P50 −158,592, taking the median to
**1,119,616 — 384 ticks under the gate**. But `WORK-H` P95 falls only −119,744,
because the `SRC` excursion above its own median is **+518,016 on the control
and +522,720 on the candidate — unchanged**. Halving the update rate halves
median `SRC` and leaves its tail intact: the excursion is asset loading driven
by animation events, and fewer update ticks do not reduce how many distinct
animations a match loads.

**The gate is a tail statistic, and Task 75 E0 has now measured what owns it.**
A load counter at `ndsRelocFinalizeLoadedFile`, ringed per frame, discharges
Task 71 §5's obligation — and answers it **no**. All 5 load frames in the window
are `SRC` excursions, so a load is *sufficient*; but **2 of the 7 excursions
carry no cartridge activity at all** (frames 453 and 454, at 2.0× and 1.9× the
`SRC` median), so a load is *not necessary*. The counter cross-validates against
the independent native-owner counter exactly (7 loads, `animLoad:7`).

Sized against the distribution rather than one frame: `WORK-H` P95 is 1,656,896
over all frames and **1,553,408 over load-free frames only**, so eliminating
on-demand loading is worth **~103,488** — 19% of the 536,896 gap, against Task 71
§5's extrapolated ~170,000. And the resulting P95 would be frame 454, a load-free
excursion, so the preload buys 103,488 and hands the gate to an unidentified
cause.

**Highest-value unowned row: profile a load-free `SRC` excursion.** Frame 453 —
single-frame spike, `SRC` 636,096, zero loads, no fallback, `FTR`/`STG` at
median. Task 71's per-PC census windowed on the frame is the instrument; its own
window (469–470) contained a load, so this population has never been profiled.
Whether the residual shares a cause with the loads (relocation, figatree parse)
decides whether one fix serves both and whether the preload's ceiling is higher
than 103,488. Row 51's preload bridge is real but must not start as a subsystem
against 19% of the gap.

Stage levers, still unowned, now second in priority:

1. **The `PrepareRun` head — 67,119 ticks/frame over 21 calls**, the largest
   block inside `ndsRendererPrepareNativeStageOwner` (now ~138,600 after Task
   104) that nothing has attacked. Long span, so the sizing is trustworthy.
   Task 81's closed stage memo does **not** cover it: that was a texture-identity
   memo at the bind seam, and Task 81 measured zero stage texture binds in
   battle. **Highest-value unowned row on the board.**
2. **`ndsRendererAdapterPrepareNativeStageMatrices` — 55,077 ticks/frame at one
   call per frame.** Never profiled; same in-place span method.
3. **Bring the 21 generic stage runs under the Task 36 replay** — 63,607
   ticks/frame for 103 triangles, less the replay's own ~1,785/run. Note this
   cannot be done by widening `NDS_TASK36_REPLAY_SEGMENT_MASK`, which would
   freeze dynamic stage geometry; mode 2 replays complete rigid segments only.

(1) and (2) are per-frame preparation over a topology Task 44 has already proven
unchanged, which is the shape an incremental update attacks. With one call per
frame there is no per-run transfer problem of the kind that killed Task 79 E1.

Task 62's reduced DS-native static mesh remains a **REVERT**. A source-exact
follow-up now preserves material/UV/color/alpha and matches the flag-0 top
screen pixel-for-pixel, but submits the same 525 static vertices. The reduced
candidates have no run/material provenance, so the corrected Task 60/61 gates
recommend none. Keep `NDS_DREAMLAND_DS_MESH=0`; details and the earlier
CPU/GX reduction remain rejected-experiment evidence in
`optimization/archive/Task62_AB_Results.md`.

## Lane Ownership

| Surface | Owner |
|---|---|
| Goal, fidelity, milestone, definition of done | `PROJECT_GOAL.md` |
| Dynamic queue, artifact identity, blockers | this file |
| Exact restart surface and next packet | `HANDOFF.md` |
| Stable architecture | `ARCHITECTURE.md` |
| Verification workflow | `VERIFYING.md` |
| Durable unresolved gaps | `KNOWN_ISSUES.md` |
| Measurements and rejected experiments | `PERF_LEDGER.md` |
| Chronological history | `PORTING.md` |

The current dirty Task 62 follow-up/runtime files are user-owned. Preserve them;
do not infer qualification or overwrite them during documentation cleanup.

## Acceptance Matrix

| Acceptance condition | State | Current evidence / blocker |
|---|---|---|
| Mario human vs original level-3 Fox CPU, Dream Land, one-minute Time, items off | Pass configuration | Boundary registry exposes only canonical mode 163 |
| Original Wait -> countdown -> GO, timer, scoring, Time Up, Results | Focused gates pass | Final exact-ROM CPU-on owner run remains red |
| Mario and Fox complete source-equivalent gameplay behavior | Red | Battle-reachable weak callbacks remain |
| Dream Land collision, platforms, blast zones, wind, camera | Pass for current P1 stage | Dynamic presentation debt remains red separately |
| Recognizable Dream Land presentation and required animation | Red | Whispy material/animation debt; Task 62 candidate rejected |
| Complete overlapping BGM, FGM, voices, announcer, crowd | Red | Exact pitch/composite/voice coverage and listen gates remain |
| Stable 30 FPS, representative P95 <= 1.12M ticks | Red | No current qualifying full-match result |
| Stable reserve, no corruption, clean teardown | Focused gates pass | Requalify after the final content/performance candidate |
| Reproducible public artifact | Red | Current local root ROM differs from the pinned public identity |

## Integration Rule

Keep only correctness-preserving, verifier-covered progress. Rendering may use
the fidelity budget in `PROJECT_GOAL.md`; gameplay must remain mechanically
equivalent to the original. Run the smallest relevant check, then one widest
relevant verifier for a kept checkpoint.

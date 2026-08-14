# R2-08 staged — SwitchPlan §6 items 1 and 4 re-verified on today's tree

2026-08-13, HEAD `bf22a37eec3`. **Nothing is landed by this cycle.** This file is
the runbook R2-08 executes plus the evidence that the two acceptance items an
agent *can* discharge are green on the current tree, not on 2026-07-29's.

**Why here and not `docs/`.** `docs/README.md` ends with *"Do not add a new
planning or workflow document. Extend the existing owner or delete obsolete
material. New top-level docs must be indexed here."* — and `check-docs.ps1:100`
enforces the index. A dated evidence package with a procedure attached is the
`artifacts/performance/<date>_<cycle>/` shape the campaign already uses
(`…/2026-08-13_c-stress/STRESS_GATE.md`, `…/2026-08-13_c-residue/OWNER_DECISIONS.md`).
The board's Parked list and `HANDOFF.md` carry the one-line pointer.

## 1. What `NDS_R2_PATH=1` selects today

One `#if`, one call. `src/port/taskman_seam.c:7931-7937` replaces the Runtime 1
battle loop (the `#else` at `:7938-9088`, ~1,150 lines inside a ~30-condition
`NDS_DEV_SCENE_HARNESS` chain) with `ndsR2BattleRun()`
(`src/nds/r2/nds_r2_battle.c:57-116`, 60 lines). That loop drives eight host
operations defined in the same translation unit under `#if NDS_R2_PATH`
(`taskman_seam.c:5247-5414`): `Prepare`, `IterationBegin`, `UpdateOnce`,
`NaturalMotionPassed`, `Present`, `Finish`, `UpdatesPerPresent`, `UpdateMax`.
Every one is Runtime 1 code, unchanged, called in the same order. The R2 loop's
only difference is that `is_battle_playable` and `use_realtime_presentation` —
compile-time 1 and 1 under the Boundary configuration — are folded, so roughly a
dozen branches per iteration are gone. `Makefile:562-566` adds
`nds_r2_battle.c` to `CFILES` only when the flag is 1, so the 0 arm's link input
set is unchanged rather than merely equivalent.

It fails closed rather than mis-specialising: `taskman_seam.c:16-21` `#error`s on
any harness but `battle_playable` and on `NDS_HARNESS_FAST_LOGIC`, and
`nds_r2_battle.c:49-55` `#error`s on the `NDS_SCENE_MIP_CACHE_LAB` seed path.

**It is only the loop selector.** The 14 non-zero `NDS_R2_*` overrides in the
published block (`Makefile:1374-1541`) — `FIGHTER_HW_MTX`, `HW_LIGHT`,
`SHUFFLE_FOLD`, `CUBIC_FIXED`, `DELTA_PATH_ITCM`, `ANIM_CACHE`, `AOBJ16_PREBAKE`,
`STAGE_VALIDATE_STRIDE`, `STAGE_DIRECT/DMA/VIEWPROJ/PREFLIGHT`,
`FIGHTER_MTX_DIRECT`, `FIGHTER_RUN_MEMO` — plus every default-on R2 flag ship
today with `NDS_R2_PATH 0`. This is why the switch cannot move the histogram
(SwitchPlan §7, R2-06 E0) and why requiring it to would measure the wrong thing.

### Static footprint of the switch: +80 bytes of ARM text, 0 data, 0 bss

Same tree, same target (`smash64ds-battle-playable-tickhud-hwtri`), both-CPU,
`check-boot-headroom.ps1`:

| | text | data | bss | `fake_heap_start` | proven headroom |
|---|---:|---:|---:|---|---:|
| `build-c147-ctl` (`NDS_R2_PATH 0`) | 981,588 | 148,288 | 1,471,976 | `0x0226d904` | 159,488 |
| `build-c152-r2path` (`NDS_R2_PATH 1`) | **981,668** | 148,288 | 1,471,976 | `0x0226d964` | **159,392** |

+80 B text is ~148 cycles at the standing 1.85 cycles-of-`FTR`-mean-per-byte
constant, i.e. two orders under the placement floor. The boot cliff is not in
play.

## 2. Drift audit — has the R2 path fallen behind Runtime 1?

The risk this cycle exists to test: the R2 host surface is a *copy* of the loop
body, so an edit to Runtime 1 that is not mirrored silently forks the two.

`git log -L 7987,8218:src/port/taskman_seam.c` (the Runtime 1 loop) and
`git log -L 5247,5414:src/port/taskman_seam.c` (the R2 host block) return **the
same four commits** since R2-01 landed in `754d83554c0`: `b04c6aed27b`,
`8061bf6dc32`, `f4fad3d0036`, `666e99a2148`. Every edit to the loop since the
seam was cut was mirrored.

**One divergence, diagnostic-only.** `666e99a2148` added
`gNdsPositionProbeUpdateInPresent = update_in_iteration;` to the Runtime 1 loop
(`taskman_seam.c:8051-8053`) and did **not** add it to
`ndsR2HostBattleUpdateOnce`, which took only the hurtbox capture
(`:5341-5348`). `NDS_R2_POSITION_PROBE ?= 0` (`Makefile:2017`), so no shipped or
measured ROM is affected; but a probe build on the R2 path would record every
capture with that index stuck at 0 and read as "everything happened in tick 0".
Two lines to fix, and it belongs in the same change as the switch or before it.

The tick-HUD reset list is *not* drifted: both sites clear the same 17
`gNdsTickHud*` accumulators, so every bucket the sampler prints
(`SHDT`/`SWRM`/`GCRA`/`SCPU`/`SCAT`/`SPRM`/`SINT`/`SPHD`/`SPHC`) is reset on
both paths.

## 3. §6 item 1 — Boundary verifier green through `NDS_R2_PATH=1`

```powershell
$env:NDS_R2_PATH='1'; .\scripts\verify-boundary.ps1 -DelaySeconds 3 -RunnerSlot 2
```

`NDS_R2_PATH` is `?=` (`Makefile:558`) and no block overrides it, so a process
environment variable reaches every `make` in the harness chain. Verbatim:

```text
Boundary verification profile passed.
```

exit code 0, and the **full 18,960,856-byte console log** scans `Exception:` 0,
`=FAIL` 0, `FAILED` 0. Key arms, verbatim (full list in `boundary-r2.log`):

```text
battle_playable Pupupu realtime pacing smoke passed: frames=211 fps=241/481
  ticks=292349952 rprof=0 gxram=466/1099 gxstat=0x6009600/ctrl=0x19
  ftrContract=6784/6784/... ftrTri=132712/p067840/p164872/own424 oracle=0/0/0
  ... aobj32=40/293/reuse8/fail0
Harness registry check passed: 3 harness mappings, 4 verifier scripts, 0 drift.
Attack visual effects passed: 178/178 Mario/Fox motion calls ...
Task 9 float ITCM passed: phase2=1 task16Compare/I2f/AddSub=1/1/1 itcm=29792/32768
Renderer ITCM placement passed: elf=smash64ds-battle-playable-proof-hwtri.elf ...
Task 20 DTCM layout passed: elf=smash64ds-battle-playable-proof-hwtri.elf ...
Published ROM contract passed: smash64ds-battle-playable-hwtri.nds, smash64ds.nds
```

**Engagement, both directions, before any number was read** (the R2-06 E0 rule):
`ndsR2BattleRun` is defined at `0x020875c4` in
`builds/build-battle-playable-proof-hwtri-harness/smash64ds-battle-playable-proof-hwtri.elf`
together with all eight `ndsR2Host*` entries, and `nm` finds **0** matches for
`ndsR2Battle` in the `NDS_R2_PATH 0` control ELF. The generated
`nds_build_config.h` for that build reads `#define NDS_R2_PATH 1`,
`NDS_SHIP_TELEMETRY 1`, `NDS_TICK_HUD 0`.

**What this arm does and does not cover.** Boundary's GDB verifier builds and
runs the **lab** target `smash64ds-battle-playable-proof-hwtri`
(`verify-battle-playable-harness.ps1:91-93`), which is what carries the R2 path
here. Its *screenshot* arm
(`verify-battle-playable-realtime-harness.ps1:316-336`) captures the **root
published** `smash64ds-battle-playable-hwtri.nds`, which is still
`NDS_R2_PATH 0` — so the visible-region and texture-detail gates in this run are
**not** R2-path evidence. The R2 path's picture is covered instead by §4's soak
filmstrip, and after the switch it is covered automatically because the published
ROM *is* the R2 ROM.

**Root ROMs untouched by this cycle**, hashed before and after:

```text
smash64ds.nds                        11,915,264  54C07FAC80C50418949908701F7C2BDBF27512C5F96AC09086FABBB0DF6AC68A
smash64ds-battle-playable-hwtri.nds  12,225,536  524448C99C31B62672A63F29914438059D5F9700E10306D147D6342B3223ADEE
```

## 4. §6 item 4 — clean 3600-tick soak on the R2 path

```powershell
.\scripts\soak-freeze-watch.ps1 -NoBuild -Build build-c152-r2path `
  -Target smash64ds-battle-playable-tickhud-hwtri -MatchMinutes 0 -MinutesToRun 5.0 `
  -PollSeconds 5 -IdenticalFramesToTrip 16 -PressStartSeconds 150 -PressStartOnResults `
  -RunnerSlot 2 -MakeFlags NDS_R2_PATH=1,NDS_R2_BOTH_CPU=1 `
  -SaveFramesTo artifacts\visibility\2026-08-13_c-r2path\frames `
  -JsonOut artifacts\performance\2026-08-13_c-r2path-recheck\soak-r2path.json
```

**`verdict: NO-FREEZE`**, 5.0 wall minutes, 59 filmstrip PNGs, trip threshold 80 s
(16 polls x 5 s). The match length is the canonical one, and the harness proves
it from inside the guest rather than from the flag: **`match timer confirmed
in-guest: 1 minute(s)`** — the 3600-tick match SwitchPlan §6 item 4 names, not a
soak-lengthened one. `-MakeFlags` re-asserted `NDS_R2_PATH 1` and
`NDS_R2_BOTH_CPU 1` against the built `nds_build_config.h` (see §8 — that check
only started working this cycle).

**The restart chain runs on the R2 path.** Three START presses, each on a
detected Results screen (t+153 s, t+174 s, t+261 s):

| counter | value | reading |
|---|---:|---|
| `gNdsSCVSBattlePlacementInitCount` | **3** | three battle-scene entries |
| `gNdsVSResultsStartCount` | **2** | two completed matches |
| `gNdsVSResultsRematchCount` | **2** | both Results screens dismissed by START into a new match |
| `gNdsRendererSceneTextureVramResetCount` | 3 | the scene-owned VRAM reset ran on every entry |
| `gNdsRendererBattleStaticTexturePrepareCount` | 3 | ditto the pinned static set |
| `gNdsRelocSceneReentryEvictCount` | 1 | a same-kind re-entry was caught stale and evicted |
| `gNdsSCVSBattleSuddenDeathPrepareCount` | 0 | no tie this run (both matches had a winner) |

**Risk counters — the cycle-11 battery, all clean:**

| counter | value | required | verdict |
|---|---:|---|---|
| `gNdsObjAnimRunawayCount` | **0** | 0 | PASS — the anim-joint fix holds on the R2 path |
| `gNdsRelocResolveMisalignCount` | **0** | 0 | PASS |
| `gNdsAObjEvent32NormalizeFailCount` | **0** | 0 | PASS (ledger: 744 in the last scene, 569 scripts, 5,677 reuses) |
| `gNdsTaskmanGeneralHeapFreeMin` | **70,392** | ≥ 32,768 | PASS — also clear of the 25,600 GObj cap |
| `gNdsTaskmanArenaAllocFailCount` / `gNdsSyMallocOverflowCount` | 0 / 0 | 0 | PASS |
| `gNdsObjmanPanicCount` / `gNdsObjmanPanicMask` | 0 / 0 | 0 | PASS — both freeze guards silent |
| `gNdsRelocHeapDeclineCount` | 0 | 0 | PASS |
| `gNdsR2AnimCacheArenaOverflows` / `Rejects` / `RangeFaults` | 0 / 0 / 0 | 0 | PASS (`GenerationMismatches` 4 = the ordinary second-entry drop) |
| `gNdsR2TexProofSweepFailCount` | **0** | 0 | PASS — slice 50's certificate survives two rematch entries |
| `gNdsRendererBattleStaticTextureViolationCount` | 0 | 0 | PASS |
| `sGCCommonsMaxNum` | **-1** | never latched | PASS |
| `gNdsBattlePlayablePacingCadenceViolationCount` | **0** | 0 | PASS |

**Cadence.** Battle, from the pacing block that resets at the last battle entry:
2 VBlanks x 940, 3 x 137, 4 x 5, 5+ x 13, max 26, 0 violations. Results screen:
**4,419 samples, 2 x 3,819, 3 x 2, 4 x 6, 5 x 2, 6 x 4, 15+ x 1 — P95 = 2
VBlanks**, i.e. Results holds 30 Hz through the chain.

**Picture.** Three filmstrip frames read directly, spanning the whole chain, with
no flash, tear, corrupt tile, missing fighter or stuck frame. Curated into
`artifacts/visibility/2026-08-13_c-r2path/` (the 59-frame filmstrip stays local,
following the c-stress cycle's 6-of-121 convention):

- `soak-t061-match1.png` — match 1, TIME 00:25, `CPU L3 [MARIO] DMG 89% STOCK
  x1` / `CPU L3 [FOX] DMG 27% STOCK x1`, Dream Land with tree, platforms,
  flowers and moving sky; FPS 25.2 / UP 50.3.
- `soak-t168-results1-foxwins.png` — Results 1: `FOX WINS`, both fighters posed,
  KOs/TKO/Pts/Place panel, 2nd/1st with the laurel.
- `soak-t225-match2-after-start.png` — **match 2 after the START restart**, TIME
  00:25, 92% / 72%, full stage with pond and both fighters; FPS 27.8.

## 5. Equivalence spot-check — R2-06 E0 re-confirmed on this tree

Not a §6 item; it is the claim §7 rests on (*"the two arms are indistinguishable
… because every saving the campaign produced is enabled on both sides"*),
measured 2026-07-29 and never since. Whole match, both-CPU gate arm, 1,600
samples, frames 439–2038, DLDI on, `-RingDump` (17 stops), `slips=0` both arms.
Control `builds/build-c147-ctl` (`artifacts/performance/2026-08-13_c-collision-stack/a-c147-ctl.json`),
candidate `builds/build-c152-r2path` (`r2path-bothcpu.json`).

| | `NDS_R2_PATH 0` | `NDS_R2_PATH 1` | delta | floor |
|---|---:|---:|---:|---|
| `WORK-H` P50 | 924,864 | 929,344 | **+4,480** | ~5,700 cross-build |
| `WORK-H` P95 | 1,210,944 | 1,204,352 | **−6,592** | ≥14,080 (≈17,000 measured) |
| `ALL` P50 | 1,118,144 | 1,118,080 | −64 | |
| `ALL` P95 | 1,678,720 | 1,678,528 | −192 | |
| VBI 2 / 3 / 4 / 5+ | 1740 / 272 / 13 / 13 | 1734 / 276 / 17 / 12 | | max 26 both |
| cadence violations | 0 | 0 | | |

**Both deltas are inside their floors and they point in opposite directions**,
which is the signature of placement rather than work. Nothing about the switch
costs or saves anything — the 2026-07-29 finding still holds on a tree that has
absorbed slices 43–52 since.

**The match is the same fight.** End-of-run globals, read from the same run that
produced the buckets:

| global | `PATH 0` | `PATH 1` | |
|---|---:|---:|---|
| `gNdsBattleTextHudP0Damage` / `P1Damage` | 0 / 58 | 0 / 58 | equal |
| `gNdsBattleTextHudP0Stock` / `P1Stock` | 1 / 1 | 1 / 1 | equal |
| `gNdsStarKOSparkleCount` / `gNdsDamageSparkScaleCount` | 0 / 14 | 0 / 14 | equal |
| `gNdsAObjEvent32NormalizedHighWater` | 1,177 | 1,177 | equal |
| `…NormalizeScriptCount` / `…CommandCount` | 183 / 1,177 | 183 / 1,177 | equal |
| `…NormalizeReuseCount` / `…FailCount` | 1,574 / 0 | 1,574 / 0 | equal |
| `…HashHitCount` / `…HashMissCount` / `…Overflow` | 1,574 / 1,371 / 0 | 1,574 / 1,371 / 0 | equal |
| `gNdsShieldAnimJointInstallCalls/Attach/Null` | 80 / 1,344 / 800 | 80 / 1,344 / 800 | equal |
| `gNdsObjAnimRunawayCount` | 0 | 0 | equal |
| `gNdsTaskmanGeneralHeapFreeMin` | 70,592 | 70,592 | **equal to the byte** |
| `gNdsTaskmanArenaAllocFailCount` / `gNdsRelocResolveMisalignCount` | 0 / 0 | 0 / 0 | equal |
| `gNdsAObjEvent32HashProbeCount` | 3,613 | 3,628 | **+15 (+0.42%)** |

**The one non-equal counter is not a divergence, and it is provable from the
source rather than argued.** `ndsAObjEvent32HashSlot`
(`src/import/battleship_sys_objanim.c:950-957`) hashes the command's **address**
(`(u32)(uintptr_t)command >> 2`), and `FindNormalized` adds `probes + 1` per
lookup (`:1129`). Two different binaries load the same animation files at
different arena addresses, so the same 2,945 lookups walk slightly different
collision chains. Lookups, hits, misses, overflow and the ledger's whole content
are identical; only the probe *cost* moved. A gameplay divergence cannot be
invisible to hits/misses while showing up in probes.

## 6. The switch — the exact change R2-08 lands

### 6.1 One Makefile line, twice

Insert in the **published** block after `Makefile:1369`
(`override NDS_TASK36_HW_COMPOSE := 2`) and in the **tick-HUD/proof** block after
`Makefile:1602` (the same line in that block), i.e. immediately before each
block's `# R2-03 E17` comment:

```make
# R2-08: THE SWITCH (SwitchPlan §6). The Boundary configuration is produced by
# the Runtime 2 battle path. The Runtime 1 loop stays in-tree as the oracle, so
# it needs a way to be built: `override` defeats a command-line 0, exactly as it
# does for NDS_R2_CUBIC_FIXED below, and without an escape hatch a Runtime 1
# control arm silently builds identical to the candidate.
ifneq ($(NDS_R2_LAB_R1_PATH),1)
override NDS_R2_PATH := 1
endif
```

Both blocks or neither: `check-tickhud-parity.ps1` compares
`make print-benchmark-flags` for the two targets and allows exactly two keys to
differ (`TARGET`, `TICK_HUD`); `BENCH_MAKE_R2_PATH` is emitted at
`Makefile:3656`, so a one-sided edit fails that checker. **Run it — it is
hand-run and nothing calls it.** Verified green on today's tree before the
switch, so a failure after it means the edit, not arrears:
`Tick-HUD parity passed: 54 make flags compared, 2 allowlisted differences, 0 drift.`

`NDS_R2_PATH ?= 0` at `Makefile:558` stays 0: SwitchPlan §5 keeps the family
default off, and the published targets carry the override.

The insertion point was chosen against the static pins, not by taste. The
adjacency-sensitive pin is `check-gbi-decode-fixtures.ps1:2253`, which matches
`SHIP_TELEMETRY := 0\s*TICK_HUD := 1\s*else\s*…` — do **not** insert inside
that run. Everything else (`:2294-2296`, `check-harness-registry.ps1:213-217`)
joins its terms with `.*?` and tolerates an inserted line.

### 6.2 Rebuild the published pair and the flag-identical sibling

```powershell
make p1        # smash64ds-battle-playable-hwtri (root) + tick-HUD sibling
```

`Makefile:3074-3079`; the two sub-makes run sequentially, one build at a time,
no `-j`, `MAKEFLAGS` untouched. `make p1-tick` alone is the cheap compile check.
Bare `make` builds the P2 ROM the milestone does not ship — do not use it.

### 6.3 Update the public-build pin in the same kept change

`DECOMP_PIN.txt` lines 20-22 carry `OUTPUT_NAME` / `OUTPUT_BYTES` /
`OUTPUT_SHA256`; `build.ps1:610-621` compares the freshly built root ROM against
them. Take the new values from that build's own report (`Bytes:` / `SHA-256:`),
or:

```powershell
(Get-Item smash64ds-battle-playable-hwtri.nds).Length
(Get-FileHash smash64ds-battle-playable-hwtri.nds -Algorithm SHA256).Hash
```

Two cautions the runbook has to carry. **The comparison is a `Write-Warning`,
not a failure** (`build.ps1:616`), so a stale pin will not stop anything — it
only makes the board's *"Reproducible public artifact"* row red, which it
already is: the pin still reads the 11,428,864-byte `4D795B4E…A090` ROM against
today's 12,225,536-byte `524448C9…ADEE`. And **ROM hashes are not reproducible
across rebuilds**: NitroFS packs its directory entries in a nondeterministic
order (board, Artifact Identity), so the pin records *an* artifact, and the
sound executable comparison is `objcopy -O binary --only-section=.text` on the
two ELFs.

### 6.4 Re-verify after the switch, and what changes about it

```powershell
.\scripts\verify-boundary.ps1 -DelaySeconds 3 -RunnerSlot 2   # no env var now
.\scripts\check-tickhud-parity.ps1
.\scripts\check-published-roms.ps1
```

After the switch the Boundary screenshot arm becomes real R2 coverage, because
the root ROM it captures is the R2 ROM. That closes the gap named in §3 and is
the reason the visual gate (§6 item 2) should be taken *after* the flip, on the
published ROM, not before it on a lab build.

## 7. SwitchPlan §6 acceptance, as of 2026-08-13

| # | item | state |
|---|---|---|
| 1 | Boundary verifier green on the Runtime 2 battle path | **GREEN 2026-08-13** — §3, engagement proven both directions |
| 2 | Visual gate: synchronized screenshot diffs + owner's visual approval | **OWNER** — SwitchPlan `:385`. Take it on the published ROM after the flip (§6.4) |
| 3 | Performance gate P95 ≤ 1.12M DLDI-on, VBlank histogram | **RED, and not the switch's** — the R2 arm measured 1,204,352 against 1,120,380 (gap 83,972) and the `PATH 0` arm 1,210,944. R2-07 owns this; the switch neither helps nor hurts it (§5) |
| 4 | Full 3600-tick soak, zero flashes/corruption/hangs/unexplained state | **GREEN 2026-08-13** — §4 |
| 5 | Owner play test on retail hardware, recorded | **OWNER** — SwitchPlan `:391` |

Items 1 and 4 are the two an agent can discharge, and both are now measured on
`bf22a37eec3` rather than inherited from 2026-07-29. **R2-08 is one Makefile
line plus a pin, gated on 2, 3 and 5.**

## 8. Instrument defect found and fixed this cycle

`soak-freeze-watch.ps1:250` read `<root>\<Build>\nds_build_config.h` while every
build directory in this repo lives under `<root>\builds\` — the path never
existed, `Test-Path` was always false, and **every guard in that block was
skipped in silence on every soak ever run**: both-CPU, match timer,
second-entry-diag, and the `-MakeFlags` verification. Those guards exist because
a soak of the wrong ROM boots, moves, and reports `NO-FREEZE`. Fixed to resolve
the directory the same way the ROM path two lines above it does
(`Resolve-Smash64DSBuildPath`), which is what makes §4's `-MakeFlags
NDS_R2_PATH=1,NDS_R2_BOTH_CPU=1` an assertion instead of a decoration.

## 9. Files

| file | what |
|---|---|
| `boundary-r2.log` | the Boundary run's PASS lines + the full-log failure scan |
| `r2path-bothcpu-run.log` | the R2 arm's bucket table and end-of-run globals |
| `r2path-bothcpu.json` / `-rows.csv` | the same run, machine-readable, 1,600 rows |
| `soak-r2path.json` | the soak verdict and counters |
| `../../visibility/2026-08-13_c-r2path/soak-t*.png` | the three cited soak frames |

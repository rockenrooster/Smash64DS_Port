# The fighter draw contract changes 51 times in a 53-second match

**Date:** 2026-08-16.
**Basis for every gate figure:** `build-c220-camship`,
`../2026-08-16_camera-ship/ship220-rows.csv`, whole match, 1,600 samples,
frames 439–2038, gate arm `NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 KEEP_CACHE=1`,
mode 163, DLDI ON. Apparatus 24,947, gate 1,120,380, rank-80 **1,210,624 raw /
1,185,677 net**, band 41–120 **1,218,356**, **REQUIREMENT +65,297**. `size.py`
re-derives all four from the basis rows before printing any result and needs no
emulator.
**Census capture:** `build-c222-ftrcensus`, `c222-run.log` / `c222.json`,
`romSha256 D8573DDD…`, 1,600 samples, frames 440–2039, `slips=0`, DLDI ON.
`config-diff.txt` is **two substantive lines** against the basis — the census
flag and the git string.
**UNITS: 2 profile cycles = 1 project tick.**

**Status: 1 lab build, 1 emulator run of my own, 1 Boundary. 0 production
behaviour changed, 0 defaults flipped, nothing published, nothing re-banked.
THE LEVEL IS UNMOVED AT +65,297.** Both root ROMs SHA-256 identical before and
after (`root-roms-before.txt` / `root-roms-after.txt`), `54C07FAC…` /
`6C939434…`.

**Boundary PASSED, exit 0, 0 `Exception:` over 316,172 lines**
(`boundary.trimmed.log`), `DECOMP_PRISTINE=PASS`. Its pacing smoke is exact
against the previous six runs — `ticks=294353408` identical to the tick,
`ftrTri=132712/p067840/p164872/own424`, `itcm=30164/32768 free=2604`,
`renderer=12896`, `binds=54`.

That smoke line is also a warning worth keeping: it reads
`ftrContract=6784/6784/…/light424/424/**bounds424/0**` over 212 frames — the
magnify branch never fires in the smoke window, while over the whole 2,039-frame
match it fires **142** times. A 212-frame window would have concluded the head
has no side effects. *Whole-match instrument only.*

---

## 0. Predicted 70% unchanged. Measured 98.75%.

`PREDICTION.md` was written and committed to before the ROM was built. It
predicted the contract would be unchanged on **70%** of captures, band 55–85%,
and set a decision rule: ≥60% → worth specifying, <40% → dead.

| counter | predicted | **measured** |
|---|---:|---:|
| `Same / Captures` | 70% | **98.75%** |
| `CountSame / Captures` | 97% | 99.71% |
| `DObjSame / Captures` | 95% | 99.71% |
| `DLSame / Captures` | 72% | **99.71%** |
| `PreSame / Captures` | 90% | 98.75% |
| `KeySameContractDiff / Captures` | 1–5% | **1.20% — and it is 49 of the 51 changes** |
| `MaxRun` | ≥ 60 | **848** |
| `EventTotal / Captures` (instrument check) | 16.0 | **16.08** |

Raw, from the run's own `extras:` line:

```text
gNdsFtrContractCaptures=4,076   gNdsFtrContractSame=4,025
gNdsFtrContractCountSame=4,064  gNdsFtrContractDObjSame=4,064
gNdsFtrContractDLSame=4,064     gNdsFtrContractPreSame=4,025
gNdsFtrContractKeySame=4,049    gNdsFtrContractKeySameContractDiff=49
gNdsFtrContractKeyDiffContractSame=25
gNdsFtrContractChangeTotal=51   gNdsFtrContractMaxRun=848
gNdsFtrContractZeroEvents=164   gNdsFtrContractEventTotal=62,920
gNdsFighterDisplayContractBoundsPassCount=3,823
gNdsFighterDisplayContractBoundsFailCount=142
```

**The instrument check could have failed and did not.** `EventTotal` divided by
the captures that produced any events is `62,920 / (4,076 − 164) = 16.08`, against
Boundary's independent `ftrContract=6784/6784` over 212 frames = 32.0 events a
frame = **16.0 per fighter**, and against two profile call rates measured at
32.00. And `4,076 + 2` bootstrap captures `= 2 x 2,039` presented frames exactly,
so **every presented frame of the run captured both fighters** — the census saw
the whole match, not a subset.

**Restricted to the sample window** (stops 1–16, frames 535–2039, per-stop
deltas in `c222.json:ringStopReads`): **3,008 comparisons, 45 changes = 98.50%
unchanged.** The rate is stationary — the worst 96-frame window carries 8
changes and five windows carry 0 or 1.

---

## 1. What the 51 changes are

`Same == PreSame == 4,025` and `CountSame == DObjSame == DLSame == 4,064`, so
the two populations nest exactly:

- **51 preamble changes** (material state: geometry mode, cycle type, render
  mode, prim colour, env colour, light direction).
- **12 count changes, 12 DObj changes and 12 DL changes** — the three counters
  are equal at 4,064. That they are the *same* 12 frames follows: `event_count`
  cannot change without changing the number of words mixed into the DObj and DL
  hashes, so {count-change} ⊆ {dobj-change} ∩ {dl-change}, and equal
  cardinality then forces the three sets equal (short of a 2⁻³² FNV collision).
  **So there are no DL-only changes**: in this match neither fighter ever
  swapped a display list under a live event list. The 12 are the whole event
  list appearing or disappearing.

The structural population is accounted for: **164 captures produced zero
events**, and `BoundsFailCount = 142` of them are
`ftDisplayMainProcDisplay`'s off-screen magnify early return
(`decomp/BattleShip-main/decomp/src/ft/ftdisplaymain.c:1131-1152`); the
remaining 22 are one of that function's other early returns (`fp->is_invisible`
at `:1088`). 164 zero-event captures across 12 transitions is ~6 off-screen
episodes of ~27 captures each. **Every one of those decisions is taken in the
head of the function, before the tree walk starts.**

## 2. The DObj-tree key is REFUTED, and it is refuted twice over

The census hashed a candidate memo key alongside the contract: per DObj, its
`flags`, `dl`, `dv`, `dls`, `dls[0..1]` and its `FTParts` flags — exactly what
`ftdisplaymain.c:753-841` branches on — accumulated by the tree walk
`ndsFighterDisplayContractCountFlags` already performs.

- **Unsound.** `KeySameContractDiff = 49`. The key reads "unchanged" on **49 of
  the 51 frames where the contract actually changed**. A key-guarded memo built
  on it would draw the wrong material on 49 frames a match. It catches 2.
  That is the expected consequence of §1: the changes are decided by `fp` state
  the tree does not carry.
- **Expensive.** FTR P50 read **301,120** on the census ROM against **290,432**
  on the basis, `+10,688`. This is a **cross-build** pair, so against the ~5,700
  P50 placement floor it is engagement evidence and an order of magnitude, not
  a banked price — but the sign and the size are what a walk that adds
  `dls[0]`, `dls[1]` and `parts->flags` pointer chases to every node predicts.
  *The compare was never the cost*, again.
- `KeyDiffContractSame = 25`: it also fires 25 times when nothing changed.

**Do not build a DObj-tree-keyed memo for this contract.** The +10,688 is also
the engagement proof that the census code ran, and the reason **this ROM must
never be read for ticks**.

## 3. The ceiling is 19,300, not 34,307 — the head cannot be skipped

`FTR_LANE.md` §5 sized the item as "delete the capture pass" = 34,307 tk/fr
(`ndsBaseFTDisplayMainProcDisplay` 30,190 + `CountFlags` 4,117). That is a
**correct ceiling and the wrong target**, because the capture pass is not
read-only. `ndsBaseFTDisplayMainProcDisplay` **is** decomp's
`ftDisplayMainProcDisplay` (`src/import/battleship_ftdisplaymain.c:141` renames
it), and its head writes:

- `fp->is_magnify_show`, `fp->magnify_pos`, `gIFCommonPlayerInterface.magnify_mode`
  and `ifCommonPlayerArrowsUpdateFlags()` — the **off-screen player arrow HUD**
  (`ftdisplaymain.c:1131-1152`);
- `gLBCommonScale`, `sFTDisplayMainSkyFogAlpha`, `sFTDisplayMainIsShadeFog`;
- the scene light, twice: `ftDisplayLightsDrawReflect` at `:1173` and again at
  the tail with `gMPCollisionLightAngleX/Y`, which is what restores the stage's
  own light for whatever draws next.

**`BoundsFailCount = 142` proves the magnify branch fires in the canonical
match**, so this is not a theoretical objection. The sound target is therefore
the **walk**, `ftDisplayMainDrawAll → ftDisplayMainDrawDefault`, whose inclusive
cost `FTR_LANE.md` measured at **19,300 tk/fr**.

## 4. Sizing, uniform D on the basis's own 1,600 rows (`size.py`)

```text
CONTROL  rank-80 1210624 raw / 1185677 net   level +65297   band 41-120 1218356
FTR P50 290432, band 41-120 290400, ratio 1.000
measured hit rate 0.9875 (4,025 unchanged of 4,076 comparisons)

candidate                                                  D   rank-80   moved  ratio     level
the >=14,080 cross-build placement floor, for reference 14080   1196544   14080  1.000   +51217
CountFlags deleted -- diagnostic-only, NO memo needed     4117   1206507    4117  1.000   +61180
WALK memo at the measured 0.9875 (SOUND, recommended)    19058   1191566   19058  1.000   +46239
WALK memo + CountFlags deleted                           23175   1187449   23175  1.000   +42122
WHOLE capture memo at 0.9875 (needs the head proven)     33877   1176747   33877  1.000   +31420
the requirement itself                                   65297   1145327   65297  1.000       +0
```

**The recommended item is 19,058 = 29.2% of the requirement, and it clears the
≥14,080 cross-build floor by 35%.** With `CountFlags` it is **23,175 = 35.5%**.
Conversion is 1.000 because `FTR` is flat (band/P50 = 1.000), so D *is* the
rank-80 move — that is the whole reason this lane is worth working.

## 5. FOUND IN PASSING: `CountFlags` is 4,117 tk/fr of diagnostics

`ndsFighterDisplayContractCountFlags` is a recursive walk of both fighters' DObj
trees, run every frame, whose only outputs are
`gNdsFighterDisplayContractHiddenCount` and `…NoTextureCount`. Nothing in the
runtime reads either: the writers are `reloc_backend_renderer_dl.c`, the reset
is `taskman_seam.c:3148-3149`, and the only readers are
`verify-battle-mariofox-gcrunall-loop-harness.ps1:2065` and `probe-ko-vfx.ps1`
— neither in Boundary. **4,117 tk/fr, level +61,180, fidelity-neutral, no memo
required.** Under the floor on its own, so it is a rider on §4's memo, where it
is free: once the walk is memoised the same tree is not visited at all.

## 6. The design the measurement points at — SPECIFIED, NOT BUILT

The key must be the **head's own output**, not a tree walk. When
`ftDisplayMainDrawAll` is reached, the head has already written every scalar
that decides the preamble into `sNdsFighterDisplayContract`
(`geometry_mode`, `cycle_type`, `render_mode`, `prim_color`, `env_color`,
`light`, `light_valid`, `light_count`) and has already taken its early-return
decisions. Comparing those ~10 words costs a compare, not a walk, and by §1 it
covers **51 of 51** observed changes.

```
ndsFighterDisplayContractCapture
  head runs unchanged            <- magnify HUD, lights, fog: all side effects kept
  if (head scalars == cached[slot] && !tree_dirty[slot])
      replay cached events[] + preambles[] + tail scalars
  else
      ftDisplayMainDrawAll(...); cache
```

Shim, not a decomp edit: `battleship_ftdisplaymain.c` already renames
`ftDisplayMainProcDisplay`, so `ftDisplayMainDrawAll` can be redirected the same
way. It is called from exactly two sites, both inside `ProcDisplay`
(`:1220`, `:1226`).

**`tree_dirty` is the one thing this cycle cannot prove, and it has a cheap
sound answer.** The census shows 0 DL-only changes in this match, but that is
one match, not an invariant. The fighter joint DL writers are a **closed set in
one file**: `decomp/BattleShip-main/decomp/src/ft/ftparam.c:780`, `:794`, `:813`,
`:863`, `:873`, `:887`, `:934` — every other `->dl =` in the tree belongs to
items, weapons or the stage. A one-line dirty set at those writers (an overlay
patch under `scripts/import-overlays/battleship/`) makes the key sound by
construction, which is the `[[bind-where-broken-not-where-read]]` pattern this
repo already shipped for `gMPCollisionGeometry`.

**Not built this cycle, and the reasons are sizes rather than taste:**
1. The cache is 2 x (512 B events + 768 B preambles + scalars) ≈ **2.6 KB of
   bss**, and `[[ram-is-not-free-gobj-cap]]` records the heap low-water already
   below the GObj-cap threshold. That check belongs with the build.
2. The sound key needs the `ftparam.c` hook, i.e. a second seam with its own
   `check-decomp-pristine.ps1` obligation.
3. The item's size and shape both moved during this cycle (34,307 → 19,300;
   tree key refuted). Build the corrected design, not the one that was wrong
   this morning.

## 7. What was NOT done

- **No memo, no route arm, no production behaviour change.** The only source
  added is behind `NDS_R2_FTR_CONTRACT_CENSUS ?= 0`.
- **The 39 preamble-only changes were not attributed to a specific field.**
  They are `fp`-level material state (`colanim`, fog, shade) by construction,
  but which one is unmeasured. It does not change the design — all of them are
  head-decided — and it would change the *explanation*.
- **`gmCameraLookAtFuncMatrix`'s second call** (`FTR_LANE.md` §5's 5,143 rider)
  was not touched.
- **`STG`** (P50 175,424) was not touched; §4 of `FTR_LANE.md` still holds.
- No closed lane reopened. The float→fixed class, link-order placement, the
  D-cache layout lane and cold-path out-of-lining all stay closed.

## Reproduce

```powershell
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c222-ftrcensus `
     NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 NDS_R2_BOTH_CPU=1 `
     NDS_R2_FTR_CONTRACT_CENSUS=1
pwsh -File scripts\sample-tick-hud-buckets.ps1 -NoBuild -Build build-c222-ftrcensus `
     -RingDump -Samples 1600 -StartFrame 439 -TimeoutSeconds 5400 `
     -RowsCsv artifacts\performance\2026-08-16_ftr-capture-memo\c222-rows.csv `
     -JsonOut artifacts\performance\2026-08-16_ftr-capture-memo\c222.json `
     -PerStopGlobals gNdsFtrContractCaptures,gNdsFtrContractSame,gNdsFtrContractChangeTotal,gNdsFtrContractKeySame,gNdsFtrContractKeySameContractDiff `
     -ExtraGlobals gNdsFtrContractCaptures,...,gNdsFighterDisplayContractBoundsFailCount
python artifacts/performance/2026-08-16_ftr-capture-memo/size.py
```

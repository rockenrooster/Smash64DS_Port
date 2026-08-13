# `SHDT` pair-level broad-phase reject — REFUTED by call count, no build spent

**Outcome: the lane's rank-1 shape in `RESIDUE.md` §4 is dead.** A conservative
exact pair-level reject before the per-part hit tests cannot reach the −26.6%
that buys 16,000 `WORK-H` P95, because **≥97.09% of fighter-pair evaluations are
already rejected before any geometry test by early-outs the source has and this
port runs**, and the whole fighter-pair path — deleted outright — is only
**18.7%** of the lane.

No source change, no build, no emulator run. Every figure below is an exact
dynamic call count or a measured cycle total off two artifacts already on disk.

---

## 0. Sources and how to reproduce

- `../2026-08-12_c123-rebank/profile/` — `build-c123-profile`, `BOTH_CPU 1`,
  `NDS_TICK_HUD_DRAW 0`, `regions=1601`, per-PC. Symbol cycles and **exact
  dynamic call counts** (the instruction count at a `bl` IS that site's call
  count).
- `../2026-08-12_c130-fire-gate/c130-gate-rows.csv` — the 1,600-sample
  `BOTH_CPU 1` whole-match gate run, lane level, current code.

`ticks/frame = cycles / (2 x regions) = cycles / 3202` (`RESIDUE.md` §0).

```
arm-none-eabi-objdump -d builds/build-c123-profile/smash64ds-battle-playable-tickhud-hwtri.elf > c123.dis
python scripts/analyze-leaf-helper-attribution.py \
  artifacts/performance/2026-08-12_c123-rebank/profile/arm9-profile.csv \
  --census artifacts/performance/2026-08-12_c123-rebank/profile/census.json \
  --dis c123.dis --helpers "<the SHDT call chain>"      # -> shdt-call-funnel.txt
  ... --helpers "<the hit-consequence chain>"           # -> shdt-consequence-funnel.txt
  ... --helpers softfloat                               # -> softfloat-head.txt
```

`analyze-leaf-helper-attribution.py` was written to attribute soft-float leaves.
Pointing it at ordinary functions turns it into an exact call-count funnel for
any call chain, off a profile that already exists. That is the reusable finding
here and it is why this cycle spent no build.

---

## 1. The source ALREADY has the early-out, and the port runs it

`decomp/BattleShip-main/decomp/src/ft/ftmain.c`:

| line | early-out | stage |
|---|---|---|
| `3019`–`3038` | self / capture / throw / team | pair, free |
| `3043`–`3076` | scan `other_fp->attack_colls`; **`if (k == 0) goto next_gobj`** | **pair-level, before any geometry** |
| `3162` | `gmCollisionCheckFighterInFighterRange(other_attack_coll, this_gobj)` | **per-hitbox broad phase vs the victim's `hit_detect_range` AABB** |
| `3169` | `if (k != 0)` gates the shield and hurtbox loops | pair-level |

`Makefile:1909` is `override NDS_IMPORT_BATTLESHIP_FTMAIN := 1`, and
`src/port/reloc_backend_diagnostic_recorders.c:5678` is a tick-HUD bracket
around `battleship_ftMainProcSearchHitAll`. **The port executes that exact
code.** Nothing of slice 28's shape ("the whole-line reject the source has and
this port lost") exists here: nothing was lost. The brief's preferred design —
restore the source's own early-out — has nothing to restore.

---

## 2. The funnel — exact dynamic call counts, whole match

`shdt-call-funnel.txt`. 6,232 `ftMainProcSearchHitAll` invocations per match
(predicted 2 fighters x 3,600 sim ticks = 7,200; measured 86.6% of that, the
shortfall being KO/respawn). 1601 presented regions, so **3.893 invocations per
presented frame**.

| call site | calls / match | per presented frame | cyc/call, self |
|---|---:|---:|---:|
| `battleship_ftMainProcSearchHitAll` | 6,232 | 3.893 | 223.1 |
| `battleship_ftMainSearchHitFighter` | 6,232 | 3.893 | 759.6 |
| `battleship_ftMainSearchHitItem` | 6,232 | 3.893 | 134.1 |
| `battleship_ftMainSearchHitWeapon` | 6,232 | 3.893 | 248.5 |
| `battleship_ftMainSearchGroundHit` | 6,232 | 3.893 | 333.2 |
| `ftParamGetBestHitStatusAll` | 6,232 | 3.893 | 82.6 |
| `ftMainGetGroundHitObstacle` | 6,046 | 3.777 | 176.7 |
| **`gmCollisionCheckFighterInFighterRange`** | **331** | **0.207** | 416.3 |
| **`gmCollisionCheckFighterAttackDamageCollide`** | **1,987** | **1.241** | 160.5 |
| `gmCollisionCheckFighterAttackShieldCollide` | **0** | 0 | — |
| `gmCollisionCheckWeaponInFighterRange` | 519 | 0.324 | 780.9 |
| `gmCollisionCheckWeaponAttackFighterDamageCollide` | 1,198 | 0.748 | 237.4 |
| `gmCollisionTestRectangle` | 3,185 (= 1,987 + 1,198) | 1.990 | 399.4 |
| `func_ovl2_800EDE5C` | 3,210 (= 3,185 + 25) | 2.005 | 128.9 |
| `gmCollisionTestSphere` | 25 | 0.016 | 1,536.0 |
| `battleship_ftMainProcessHitCollisionStatsMain` | 19 | 0.012 | 2,607.3 |

`ftMainSearchHitHazard` is **not** in this bracket — its 6,232 calls come from
`battleship_ftMainProcSearchCatch`. Do not charge its 469 tk/frame to `SHDT`.

### The rejection rates that decide the cycle

- **≥5,901 of 6,232 pair evaluations (≥94.68%) never reach any geometry test.**
  The `k == 0` early-out at `ftmain.c:3076` costs a flag scan and rejects them.
- Of the ≤331 hitbox evaluations that reach the broad phase, `FTDAMAGECOLL_NUM_MAX`
  is **11** (`include/ft/fighter.h:322`), so the 1,987 measured hurtbox tests
  require **≥181 passers**. The source's own broad phase therefore rejects **at
  most 150 in the whole match — ≤2.41% of pair evaluations.**
- **Combined: ≥97.09% of pair evaluations are rejected before any per-part
  overlap test.**
- Interactions found: `battleship_ftMainUpdateDamageStatFighter` **14 calls a
  match**, `ftMainProcessHitCollisionStatsMain` **19**, shield collide **0**
  (`shdt-consequence-funnel.txt`). So **≥99.7% of pair evaluations end with zero
  fighter-vs-fighter interactions** — and 94.68% of them already cost nothing
  but the flag scan.

**A new reject can only fire where the existing ones do not: at most 331 of
6,232 evaluations (5.31%), of which ≥181 are genuinely in range and must still
be tested exactly.**

---

## 3. The arithmetic against the 16,000 bar

Cycle totals include the soft-float charged to each site (`softfloat-head.txt`);
`gmCollisionTestRectangle` and `func_ovl2_800EDE5C` are split between the
fighter and weapon paths by their exact call counts.

| component | cyc / match | tk/frame | % of `SHDT` mean 14,227 |
|---|---:|---:|---:|
| `battleship_ftMainSearchHitFighter` self — the `k` flag scan | 4,733,545 | 1,478.3 | 10.39% |
| fighter narrow phase (range + damage collide + `TestRectangle` + `800EDE5C` shares) | 3,802,384 | 1,187.5 | 8.35% |
| **whole fighter-pair path** | **8,535,929** | **2,665.8** | **18.74%** |
| required for ΔP95 = 16,000 (`RESIDUE.md` §2) | — | **3,784** | **26.6%** |

**Deleting fighter-vs-fighter hit detection outright reaches 18.7%**, which on
`RESIDUE.md` §2's own interpolation pays ≈11,200 — under the 16,000 bar and
inside the 9,664 repeat spread. A conservative exact reject reaches a fraction
of that. There is no version of this change that clears the bar.

The `SHDT` baseline is fully explained, which is the control that this
accounting is not missing a term: the always-run parts (bracket apparatus 652 +
both `ProcSearchHitAll` selves 674.9 + the `k` scan 1,478.3 + `SearchHitItem`
261.0 + `SearchHitWeapon` self 483.6 + `SearchGroundHit` 648.5 +
`GetBestHitStatusAll` 160.8 + `GetGroundHitObstacle` 333.7) sum to **4,693
tk/frame** against the c130 arm's measured **median 4,416** — 6.3% apart on two
different builds.

---

## 4. Where the lane's cost actually is — inheritance, NOT priced

`SHDT` on the c130 gate arm: mean 14,227, median 4,416, P95 88,704, max
519,744. The rank-80 frame — the frame that *is* P95, `WORK-H` 1,220,480 — has
**`SHDT` 189,312**, 15.5% of itself.

- **88 frames of 1,600 carry 70.3% of all `SHDT` ticks** (15,993,152 of
  22,763,200), in **38 discrete runs**, 30 of them ≤2 frames long. The flat
  baseline (4,416 x 1,600) is 31.0% of the lane.
- Lane co-movement, those 88 frames vs the 1,497 under 10,000: `SHDT` **x41.2**,
  **`SPRM` x26.4** (1,963 -> 51,900), `SPHD` x1.40, `SINT` x1.24, `WORK-H`
  x1.34 — while `FTR` x1.02, `STG` x0.99, `SCPU` x1.00, `MISC` x1.07 are flat.
  The excursion is a **fighter-proc event**, not a renderer, AI or particle one.
- Search-side work accounts for **6,866 tk/frame of the 14,227 lane mean
  (48.3%)**. The other **~7,361 tk/frame (51.7%)** is inside the same bracket
  and is **not** the fighter-vs-fighter consequence path, which fires 14 times a
  match.

**So the lane's leverage is real and its owner is unidentified.** It is a
discrete, ~38-events-a-match cost that moves `SHDT` and `SPRM` together while
leaving every renderer lane flat, and it sits exactly on the frames P95 is
decided on. That is the opposite of the flat-frames population a broad-phase
reject would empty ("cluster where the percentile lives"). **Not designed, not
priced, and it must not be re-opened as a broad-phase question.**

### The cheap route to that owner is CLOSED — do not spend it again

`task37_census.py --split-top-frames 88` over the c123 profile
(`split-top88-frames.txt`) attributes a **1,302,481 cyc/frame premium** on the
88 costliest frames to the FAT/DLDI family (`memcpy` 48,218 · `armCopyMem32`
31,537 on 61/88 · `get_fat` 30,556 on 61/88 · `f_lseek` 19,407 · `f_read`
12,018), `ndsRelocNormalizeFighterAObj16File` 25,837 on 72/88,
`ndsRelocAssetIDForToken` 13,162 and `battleship_ftMainSetStatus` 10,529 on
72/88 (6,994 of it memory stall).

**That is not a finding and it is not the `SHDT` excursion.** It reproduces what
`../2026-08-12_c123-rebank/split-top80.txt` already said — it is how slice 46
was found (`SLICE46.md:65-66`) — and the two frame populations barely overlap:
matching the profile's 88 marked regions against the c130 arm's 88 `SHDT ≥
30,000` frames gives **30.7% at ±2 frames against a 24.6% chance baseline**
(12.5% exact). The costliest frames of the profile are asset-load frames; the
`SHDT` spike frames are something else, and ranking by total cost cannot
separate them.

**So the discriminating measurement keys on `SHDT` itself, not on frame cost** —
a counter inside the bracket on the current arm, which is a build. That is the
next cycle's first step, and its prediction is already constrained: whatever it
is fires ~38 times a match, costs ~180,000 ticks when it does, and raises
`SPRM` by 26x on the same frame.

---

## 5. What this cycle did NOT do

- No counters, no source change, no lab build, no emulator run, no ROM. Both
  root ROMs unchanged: `smash64ds.nds` `54c07fac…6ac68a`,
  `smash64ds-battle-playable-hwtri.nds` `524448c9…23adee`.
- Did not identify the ~7,361 tk/frame excursion owner, and closed the one
  zero-build route to it (above).
- Did not re-measure the `SHDT` lane on current code; §4's lane figures are the
  c130 gate arm and §2–§3's call counts are c123, which predates the fox-gun
  overlay and the flame fix (+9,856 `WORK-H` P95 combined, inside that arm's own
  repeat spread). The refutation does not depend on that: the required cut is
  26.6% of the lane and the whole fighter-pair path is 18.7% of it, a 1.4x
  margin that no +9,856 relabelling closes.

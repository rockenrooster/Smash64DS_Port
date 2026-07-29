# Handoff

Updated: 2026-07-29. **Restart surface only, capped at 150 lines.** Anything
durable goes to its owning doc, not here: the board owns the queue and every
result, `PERF_LEDGER.md` measurements, `KNOWN_ISSUES.md` durable gaps and
harness traps, `optimization/TASK_STANDING_RULES.md` how a task is run.

Runtime 2 phase status. **Of the two levers that close the gate, E32 is ordinary
engineering (a generator gap, E62) and only the cubic is an owner decision.**

| phase | state |
|---|---|
| R2-00a/b/c, R2-01, R2-02 | gated |
| R2-03 | shipped E12/E28/E29/E46; E32 needs a generator change, the cubic needs a decision |
| R2-04 | loading clause done (E5/E6), rate clause done as far as the contract permits (E57/E6); **budget clause 146,942 vs 100,000 needs the E61 decision** |
| R2-05 | reproducibility half PASSES (E0); fighter-special-case audit not yet run |
| R2-06/07/08 | not started; gated behind the above |

## Where the gate stands

128-frame ring dump, frames 793..920, graduated build (`WORK-H`): P50
**1,013,696**, P95 **1,228,928**, gate 1,120,000, **17/128 over**. **P50 is
inside; only P95 misses, by 108,928.** Evidence
`artifacts/performance/r203-e53-ctlb-128{.json,-rows.csv}`.

## What owns the miss

E52 re-decomposed the excursion after E5 graduated: **E35's "25 of 26 over-gate
frames are `SRC`" no longer holds** — it predated E5 removing the loading
component. The over-gate frames split in half: **`FTR` +140,988 (50%), `SRC`
+135,360 (48%)**. E53 profiled frames 910–913 against control 876–879:
**+420,227/frame**, of which **376,434 is 151 symbols exactly zero on control** —
the generic display-list interpreter, a second renderer running. The rest is
fixed-point *matrix* work; **`__aeabi_fadd` does not grow at all**, so float is a
flat per-frame cost, which is why E60/E61's lever moves P50 and P95 together.

## E54: the fighter falls back, and E32 is worth −51,136

`NDS_TASK68_FALLBACK_CENSUS=1` + `NDS_TASK91_DRAW_PHASE_CENSUS=1`, same 128
frames: **5 native-owner fallbacks, every one `shuffle_tics`, zero animation
locks**. The clean build has **exactly 5 frames** with `FTR` > 500,000 —
909-913, consecutive, ~507,000 each over the median, all over gate. One hitlag
burst. Capping `FTR` at its median there projects E32 across the distribution:

| | P50 | P95 | max | over gate |
|---|---:|---:|---:|---:|
| as measured | 1,013,696 | 1,228,928 | 2,040,896 | 17/128 |
| **`FTR` capped** | 1,011,264 | **1,177,792** | 1,531,072 | **13/128** |

**E32 is worth −51,136 P95 and four frames** — it halves the gap without closing
it. Blocked on the hurt flash, not on its value.

## The other half is ANIMATION, not collision (E60/E61 — this replaces the `SRC` row)

**The board carried "float→fixed on the collision path, `gmcollision.c`" for
several cycles. It is wrong by ~20x and the row is deleted.** A leaf helper is
charged to itself, never to its caller, so every float op the animation path
executes was booked to `__aeabi_fadd`/`__aeabi_fmul` and read as a separate,
larger family. Caller-attributed: `gcPlayDObjAnimJoint` **34,022 self + 60,509
helper = 94,531**; the whole animation path **76,047 + 70,895 = 146,942**.

**146,942 ticks/frame, 15.2% of WORK 969,487 — larger than the whole gap.** The
entire collision family is **under 4,000**, below the placement noise floor. The
renderer share is 15,709, inside §3.9's "too small for architecture work" band.

E61 then found **the cubic is 99.6% of that float**: 149.4 cubic nodes/frame at
**405 ticks each**, against 118.7 Step nodes at zero float and 4.5 Linear.
`anim_speed` is `1.0`/`0.5`, **never 0**; `GOBJ_FLAG_NOANIM` skips are **0**.

**Task 78 stopped the animation compiler on a self-vs-inclusive error** — it
compared 82,807 *self* ticks to a 100,000 target while its own §4 listed
`fadd`+`fmul` = 119,912 as a *separate* family. Corrected: 164,236, **1.64x its
target, not 0.85x**. Tasks 95/96 refute only the *layout* route.

**The pose table is REFUTED by size** (E61) — 2.62 MB resident against 4 MB of
main RAM, or 42.6 KB/7–11 ms streamed per transition. Do not propose it again.

## Two levers close the gate: one is engineering, one is a decision

```
gap 108,928  −  E32 51,136  −  fixed-point cubic ~50,000  =  ~7,800 left
```

- **E32** — blocked on a **generator gap, not a decision** (E62; the earlier
  "fidelity-budget / visual approval" framing here was wrong). The flash clears
  `G_LIGHTING` and draws vertex colours raw; the owner keeps `POLY_FORMAT_LIGHT0`
  and hardware-lights with stale diffuse/ambient, so it draws Mario *unflashed* —
  not corrupt. E32 is pixel-identical to the generic path on every non-flash
  frame (510/511: 0 px). E49's `NDS_R2_UNLIT_VERTEX_EPOCH` already implements the
  runtime half but is **refuted**: it emits the baked dense `.rgba`, which holds
  packed **normals**, giving rainbow speckle and a *worse* diff (2,199 vs 1,551).
  **Needs the generator to bake the flash variant's vertex colours** as a second
  dense table. E63 sizes it.
- **The cubic** — blocked on the Task 9 state hash. `PROJECT_GOAL.md` requires
  mechanical equivalence and lists "fixed-point replacements" as allowed; the
  hash asserts bit-exactness, which is stronger than the contract. The change is
  confined to `gcGetInterpValueCubic` evaluating already-parsed track state, and
  its only path to gameplay is `gmCollisionGetFighterPartsWorldPosition` (E57),
  so the honest acceptance test is a hitbox-overlap differential over a full
  match, not the hash.

**R2-03 E26 — demoted** to 23,844/frame over 136.8 deltas (needs both
`NDS_TASK91_DRAW_PHASE_CENSUS=1` *and* `NDS_R2_SPAN_LEAN_TIMING=1`; the second
alone does not define the brackets). Bottom of §3.9's "20–50K if simple and
exact" band and E26 is not simple. **It must replace the dispatch, not the
writes** (E39); read its spec only with board entries E34/E34-b/E39/E43/E45/E56.

**R2-04 E57 — REFUTED.** `gmCollisionGetFighterPartsWorldPosition`
(`gm/gmcollision.c:489`) places every hitbox by **walking the live joint chain**,
so halving the twice-per-frame evaluation is a gameplay change. With E6 this
closes R2-04's rate clause.

## Refuted this cycle — do not re-derive

- **E51**, a `line_id -> (group, kind)` table for the three scans in
  `reloc_backend_mp_collision.c`: `gNdsStageCollisionLoopYakumonoCount = 1`, so
  the loop that reads as a 64×4 worst case has a trip count of **one**.
- **E53**, an 8-byte `{base,size}` mirror for `ndsRelocFindLoadedFileContaining`.
  Exact, still P95 **+11,584** and 92/128 worse; `STG`, which it cannot touch,
  moved +1,600 on 99.
- **The flash as vertex data** (E48/E49/E50/E55/E58), **the animation pose
  table** (E61, 2.62 MB resident vs 4 MB RAM), and **fixed-point collision**
  (E60, the whole family is under 4,000 ticks/frame).

## Restart

Branch `codex/r2-runtime2`, not merged to master. Boundary
`battle_playable_realtime`, mode `163`.

```powershell
$env:DEVKITPRO = 'C:/devkitPro'; $env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

**Do not rebuild `smash64ds.nds` for P1 work** (owner, 2026-07-28). **Do rebuild
the tick-HUD ROM whenever the published one is** (owner, 2026-07-22) — keep its
Makefile block flag-identical. **Do not pass `-j` to `make`**; the Makefile sets
`MAKEFLAGS += -j$(NDS_JOBS)` from `nproc` and an explicit `-j` overrides it. One
build at a time regardless — generators write to shared paths outside `$(BUILD)`.
A clean checkout must build through `build.ps1`, not bare `make`: four of the six
generated `.inc` files are gitignored and only `build.ps1` regenerates them.

Preserve canonical mode 163, renderer mode 9, mip 0, static texture residency,
source countdown, Dream Land water at source frame 0, Task 16 `1/1/1`. Do not
edit `decomp/`. **Bug #10 is FIXED and folded in** — `06992f10812`,
cherry-picked from `2cbc6189d15` to preserve authorship, with a host fixture, a
structural pin, and the `pause_under20` oracle.

# Handoff

Updated: 2026-07-29. **Restart surface only, capped at 150 lines.** The board
owns the queue and every result; `PERF_LEDGER.md` measurements and rejected
experiments; `KNOWN_ISSUES.md` durable gaps and harness traps;
`optimization/TASK_STANDING_RULES.md` how a performance task is run. Anything
durable goes there, not here.

Runtime 2: R2-00a/b/c, R2-01, R2-02 gated. R2-03 shipped E12/E28/E29/E46. R2-04
graduated E5. E32 is **parked** on a visual regression — not awaiting a yes/no.

## Where the gate stands

128-frame ring dump, frames 793..920, on the graduated build:

| | `WORK-H` |
|---|---:|
| P50 | 1,013,696 |
| **P95** | **1,228,928** |
| gate | 1,120,000 |
| over gate | **17 / 128** |

**P50 is inside the gate; only P95 misses, by 108,928.** Evidence:
`artifacts/performance/r203-e53-ctlb-128{.json,-rows.csv}`.

## What owns the miss

E52 re-decomposed the excursion after E5 graduated: **E35's "25 of 26 over-gate
frames are `SRC`" no longer holds** — it predated E5 removing the loading
component. The over-gate frames split in half: **`FTR` +140,988 (50%), `SRC`
+135,360 (48%)**. E53 profiled frames 910–913 against control 876–879: work delta
**+407,847/frame**, with **twelve symbols exactly zero on control frames**
summing to **292,899** — the **generic display-list interpreter**, a second
renderer running.

## E54: the fighter falls back, and E32 is worth −51,136

`NDS_TASK68_FALLBACK_CENSUS=1` + `NDS_TASK91_DRAW_PHASE_CENSUS=1`, same 128
frames: **5 native-owner fallbacks, every one `shuffle_tics`, zero animation
locks**. The clean build has **exactly 5 frames** with `FTR` > 500,000 —
909-913, consecutive, ~507,000 each over the median, all over gate. One hitlag
burst; E35's "third owner" reading does not apply. Capping `FTR` at its median
there projects E32 across the whole distribution:

| | P50 | P95 | max | over gate |
|---|---:|---:|---:|---:|
| as measured | 1,013,696 | 1,228,928 | 2,040,896 | 17/128 |
| **`FTR` capped** | 1,011,264 | **1,177,792** | 1,531,072 | **13/128** |

**E32 is worth −51,136 P95 and four frames** — it halves the gap without
closing it. Largest single lever left, blocked on the hurt flash (E48/E49/E50,
and E55's correction), not on its value. The twelve frames still over gate are
the `SRC` half.

**Never compare a census build's frame numbers to a clean build's.** The census
flags cost ~137,664 ticks/frame and shift the histogram 2:726 → 2:314, so frame
N is a different game tick in each. Correlate through a build-internal column.

## The `SRC` half is an owner decision, not an experiment

Float→fixed on the collision path. `PROJECT_GOAL.md` permits it and ranks
gameplay fidelity above stable 30 FPS, but `gmcollision.c` is verifier-gated by
the Task 9 state hash and re-bounding a bit-exact gate is the owner's call.

Cheap sub-levers all refuted — do not re-derive: `func_ovl2_800ED490` runs 27.2
times a frame (nothing to memo); the cost is arithmetic at 38 cycles per
soft-float add; and the float leaves are already lowered (`_arm_addsubsf3.o`,
`_arm_muldivsf3.o` ITCM-resident, `NDS_TASK16_FLOAT_ADDSUB=1`).

## Best unowned work: E58, and it can unblock E32

**E55 reopened the cheapest fix for E32, which E50 closed on a bad inference** —
E50 measured 172/273 vertices differing and concluded the flash was per-vertex,
but that sampled lighting's **output** and inferred its **input**. Two hitlag
frames say otherwise:

- **A and B are elementwise identical** — the flash does not ramp within a burst.
- **0 of 541 baked vertices are achromatic; 75% of flashed ones are**, greys
  spanning 76..255 (a lighting term). A lerp preserves hue; only a **replacement**
  removes chroma. So the flash replaces the source colour and the per-vertex
  variation is lighting.

That restores **E49's option 1 at per-epoch granularity** — its own words, *"one
colour per epoch, not per-vertex data, a runtime override the emit can apply
without touching the baked table."* The owner already computes lighting (E48), so
feeding it a per-epoch constant is exact, needs no per-vertex data, and **keeps
E32's measured −51,136**.

**E58 is the one build that decides it: record the epoch index alongside the
colour.** The stride sample crosses epochs, so its two constant families (greys,
and reds with `R > G ≈ B`) are expected, not contradictory. If each epoch's
samples are one value, build the override — it lands **pixel parity against
Runtime 1**, R2-03's own stated gate, so it needs no subjective approval.

**R2-03 E26 — demoted.** Re-measured with
`NDS_TASK91_DRAW_PHASE_CENSUS=1 NDS_R2_SPAN_LEAN_TIMING=1` (both flags — the
second only compiles the per-delta census out from inside the brackets, the
first defines them): before-span **23,844/frame over 136.8 deltas = 174.2 each**,
after 13,719 over 49.2, replay **37,563**. E46 took 3,100 off E43's 26,944. That
is the *bottom* of the plan's §3.9 "20–50K consider if simple and exact" band and
E26 is exact but not simple. Read its spec only alongside the board's
E34/E34-b/E39/E43/E45/E56 entries. **E26 must replace the dispatch, not the
writes** (E39).

**R2-04 E57 — REFUTED from source.** Animation evaluation (~52,000/frame) runs
twice per presented frame, so halving it looked like ~26,000 free — but
`gmCollisionGetFighterPartsWorldPosition` (`gm/gmcollision.c:489`) places every
hitbox by **walking the live joint chain**, not from `ftParam` tables, so the odd
tick's pose is load-bearing and halving it is a *gameplay* change. Corollary:
the renderer is already at presentation rate (`DLAllDrawForSlot` 2.0 calls/frame,
`AdapterBuildDObjLocalMatrix` 50.0), so **R2-04's rate-decoupling mandate is
already satisfied on the renderer side.**

## Refuted this cycle — do not re-derive

- **E51**, a precomputed `line_id -> (group, kind)` table for the three scan
  functions in `reloc_backend_mp_collision.c`. Dream Land reports
  `gNdsStageCollisionLoopYakumonoCount = 1` and 7 lines: the loop that reads as a
  64×4 worst case has a trip count of **one**.
- **E53**, an 8-byte `{base,size}` mirror for `ndsRelocFindLoadedFileContaining`.
  Exact by construction, still a regression — P95 **+11,584**, 92/128 worse,
  `STG` (untouchable by it) +1,600 on 99. The lookup is a *symptom* of the
  fallback anyway.
- **E55 route 1**, a lerp model of the flash — it replaces, not transforms.

## Restart

Branch `codex/r2-runtime2`, not merged to master. Boundary
`battle_playable_realtime`, mode `163`.

```powershell
$env:DEVKITPRO = 'C:/devkitPro'; $env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

**Do not rebuild `smash64ds.nds` for P1 work** (owner, 2026-07-28). **Do rebuild
the tick-HUD ROM whenever the published one is** (owner, 2026-07-22) — same
program plus the Task 41 timers, the instrument every measurement runs on; keep
its Makefile block flag-identical.

**Do not pass `-j` to `make`.** The Makefile sets `MAKEFLAGS += -j$(NDS_JOBS)`
from `nproc` (32 here); an explicit `-j` overrides and caps it. One build at a
time regardless — generators write to shared paths outside `$(BUILD)`.

Preserve canonical mode 163, renderer mode 9, mip 0, static texture residency,
source countdown, Dream Land water at source frame 0, Task 16 `1/1/1`. Do not
edit `decomp/`. **Bug #10 is FIXED and folded in** — `06992f10812`,
cherry-picked from `2cbc6189d15` to preserve authorship, with a host fixture, a
structural pin, and the `pause_under20` oracle.

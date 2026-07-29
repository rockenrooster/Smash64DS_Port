# Handoff

Updated: 2026-07-29. **This file is the restart surface and nothing else, capped
at 150 lines.** `P1_EXECUTION_BOARD.md` owns the queue and every experiment's
result, `PERF_LEDGER.md` owns measurements and rejected experiments,
`KNOWN_ISSUES.md` owns durable gaps and harness traps,
`optimization/TASK_STANDING_RULES.md` owns how a performance task is run.
Anything durable belongs in one of those, not here.

Runtime 2: R2-00a/b/c, R2-01 and R2-02 gated. R2-03 has shipped E12, E28, E29
and E46. R2-04 has graduated E5. E32 is **parked** on an unresolved visual
regression — it is not awaiting a yes/no.

## Where the gate stands

128-frame ring dump, frames 793..920, on the graduated build:

| | `WORK-H` |
|---|---:|
| P50 | 1,013,696 |
| **P95** | **1,228,928** |
| gate | 1,120,000 |
| over gate | **17 / 128** |

**P50 is inside the gate. Only P95 misses, by 108,928.** Evidence:
`artifacts/performance/r203-e53-ctlb-128{.json,-rows.csv}`.

## What owns the miss

E52 re-decomposed the excursion after E5 graduated, and **E35's "25 of 26
over-gate frames are `SRC`" no longer holds as written** — that predated E5
removing the on-demand-loading component. The over-gate frames now split almost
exactly in half: **`FTR` +140,988 (50%), `SRC` +135,360 (48%)**.

E53 then profiled excursion frames 910–913 against control frames 876–879
(`NDS_TASK37_PROFILE=1`, `NDS_TICK_HUD_DRAW=0`). Work delta **+407,847
ticks/frame**, and twelve symbols are **exactly zero on the control frames**,
summing to **292,899**: `ndsRendererHardwareSubmitVertex` (96,238),
`ndsRendererSubmitHardwareTriangle` (51,037), `ndsRendererScanList` (50,913),
`HardwareBeginTriangleBatch` (19,540), `DecodeInputVertex` (10,601), and seven
more. That is the **generic display-list interpreter** — not more of the same
work, a second renderer running. `FTR`'s own percentiles agree: P50 388,224,
P95 392,448, spread 1.01, **max 898,368**.

## The single highest-value next step

**Settle what turns that path on. One build.**

`reloc_backend_renderer_dl.c:12275` disables the native fighter owner, per
fighter, when `is_use_animlocks || shuffle_tics != 0`; E32's census counted 5
shuffle fallbacks and 0 animlock fallbacks over frames 460..500, so on the
shipped build the trigger would be **hitlag** and E32's fold is the repair. But
E35 saw a smaller (66,498) version of the same symbol set and read it as **a
third owner drawing** — an effect object, not the fighter falling back. Both
stories predict generic-renderer symbols appearing from zero; they need
different fixes.

```powershell
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-r2-e54 NDS_TASK68_FALLBACK_CENSUS=1 NDS_TASK75_LOAD_CENSUS=1
```

then sample with `-FallbackCensus -RingDump` and correlate the per-frame
fallback ring against the over-gate frames (809, 842, 843, 864, 869, 885, 890,
898, 899, 901, 907, 909–913, 924, 926). `gNdsR2FallbackShuffleTics` and
`gNdsR2FallbackAnimLocks` under `NDS_TASK91_DRAW_PHASE_CENSUS` split the
disjunction if the ring says the fighter is falling back.

## The `SRC` half is an owner decision, not an experiment

Float→fixed on the collision path. `PROJECT_GOAL.md` permits it ("Mechanical
equivalence is required. Bit-exact … is not") and ranks gameplay fidelity above
stable 30 FPS, but `gmcollision.c` is verifier-gated by the Task 9 state hash and
re-bounding a bit-exact gate is the owner's call.

Its cheap sub-levers are all refuted, so do not re-derive them:
`func_ovl2_800ED490` runs 27.2 times a frame (no redundancy to memo); the cost is
arithmetic at 38 cycles per soft-float add; and the float leaves are already
lowered — `_arm_addsubsf3.o` and `_arm_muldivsf3.o` are ITCM-resident and
`NDS_TASK16_FLOAT_ADDSUB=1` ships a hand-written integer replacement.

## Best unowned bit-exact work

**R2-03 E26 — fold the before-span.** E38's brackets enclosed their own
instrument; E43 rebuilt with `NDS_R2_SPAN_LEAN_TIMING=1` and the real figures are
**before 26,944/frame over 134.5 deltas, after 13,703.7 over 47.9, replay
40,648** — not 33,708 / 16,243 / 49,951, which appear throughout the E26 spec and
are 20% high. Fold the before-span only: it is 66.3% of the cost and the half
with no ordering problem.

**E46 shipped after those numbers were taken** (delta path into ITCM, `FTR` P50
−12,032), so **re-measure with `NDS_R2_SPAN_LEAN_TIMING=1` before sizing the
fold** — the remainder is smaller than 26,944.

Read `ClaudeOpus5_R203_E26_Spec_GeneratedEpochState_20260728.md` only alongside
the board's E34/E34-b/E39/E43/E45 entries, which correct it in several places.
E39 already refuted the operand-elision version (7.4% hit rate, ~3,700
ticks/frame): **E26 must replace the dispatch, not deduplicate the writes.**

## Refuted this cycle — do not re-derive

- **E51**, a precomputed `line_id -> (group, kind)` table for the three scan
  functions in `reloc_backend_mp_collision.c`. Dream Land reports
  `gNdsStageCollisionLoopYakumonoCount = 1` and 7 lines: the loop that reads as a
  64×4 worst case has a trip count of **one**.
- **E53**, an 8-byte `{base,size}` mirror for `ndsRelocFindLoadedFileContaining`.
  Correct by construction and still a regression — P95 **+11,584**, 92 of 128
  frames worse, and `STG` (untouchable by this change) +1,600 on 99 of them. The
  768 bytes of new BSS cost more than the scan saved.

## Restart

Branch `codex/r2-runtime2`, not merged to master. Boundary
`battle_playable_realtime`, mode `163`.

```powershell
$env:DEVKITPRO = 'C:/devkitPro'; $env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

**Do not rebuild `smash64ds.nds` for P1 work** (the owner, 2026-07-28) — P1 is
the battle vertical slice and `smash64ds-battle-playable-hwtri` is the only
published ROM it touches. **Do rebuild the tick-HUD ROM whenever the published
one is rebuilt** (the owner, 2026-07-22): it is the same program plus the Task 41
timers and it is the instrument every measurement runs on, so keep its Makefile
block flag-identical.

**Do not pass `-j` to `make`.** The Makefile sets `MAKEFLAGS += -j$(NDS_JOBS)`
from `nproc`; an explicit `-j` on the command line overrides it and caps the
build. One build at a time regardless — the asset generators write into shared
paths outside `$(BUILD)`.

Preserve canonical mode 163, intrinsic renderer mode 9, mip 0, static texture
residency, source countdown, Dream Land water frozen at source frame 0, and Task
16 compare/i2f/addsub `1/1/1`. Do not edit `decomp/`.

**Bug #10 is FIXED and folded into this branch** — `06992f10812` "Fix Mario
pelvis texture clamp", cherry-picked from `2cbc6189d15` so authorship is
preserved, with a host fixture, a structural pin, the `pause_under20` camera
oracle, and the controller-playback DTCM move that oracle needs.

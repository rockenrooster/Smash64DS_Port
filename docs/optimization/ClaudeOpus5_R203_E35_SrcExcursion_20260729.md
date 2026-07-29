# R2-03 E35 — the load-free `SRC` excursion, profiled

**Date:** 2026-07-29
**Status:** measured. Redirects the gate lane away from the fighter renderer.
**Instrument:** `scripts/sample-tick-hud-buckets.ps1 -RingDump -FallbackCensus`
with `NDS_TASK75_LOAD_CENSUS=1`, plus two `scripts/run-task37-profile-census.ps1`
windows built `NDS_TICK_HUD_DRAW=0`.

## 1. Why this and not more fighter work

R2-03 has shipped four cuts (E12, E28, E29, and E32 pending approval) and E33
re-confirmed the run prepare has no hot spot. `WORK-H` P50 is **1,011,200** —
inside the 1,120,000 gate. The gate is missed only at P95, so the question is not
"what does a frame cost" but "what does an *expensive* frame cost", and the board
had one unowned row for it: profile a load-free `SRC` excursion.

## 2. Loading is not the gate, and was oversized

128-frame ring dump, frames 439..566, with the Task 75 per-frame asset-load
counter riding the census ring. It cross-validates exactly against the
independent native-owner reason counter: 16 loads, `animLoad:16`.

| `WORK-H` | P50 | P95 | max | over gate |
|---|---:|---:|---:|---:|
| all frames | 1,011,200 | 1,468,800 | 2,111,296 | 34/128 |
| load-free only | 997,696 | **1,419,264** | 1,556,160 | 21/114 |

**Eliminating on-demand loading is worth ~49,536 at P95, not the ~103,488 the
board has carried since Task 75 E0.** That estimate came from a different window;
this one says the preload bridge buys about half what it was sized at. It is
still real, and it is still not the gate.

## 3. Projecting E32 across the whole distribution

E32's own report gave `FTR` P95 913,920 -> 412,992. Applying that cap frame by
frame (`WORK-H` − max(0, `FTR` − 412,992) + E32's measured 3,456 ordinary-frame
tax) over all 128 frames:

| | P50 | P95 | max | over gate |
|---|---:|---:|---:|---:|
| measured, E32 off | 1,011,200 | 1,468,800 | 2,111,296 | 34/128 |
| **projected, E32 on** | 1,001,152 | **1,377,408** | 1,614,080 | **26/128** |

A first read of only the worst fourteen frames suggested E32 might land the gate
on its own. It does not — across the full 128 the count falls 34 -> 26. **Rank
the whole distribution, never the visible top of it.** A P95 is a position in a
sorted list, and the frames that decide it are the ones just below the ones that
catch the eye.

**25 of those 26 remaining frames are `SRC` excursions** (SRC ≥ 506,176 against a
327,040 median). Thirteen are load-free, and they arrive in *consecutive runs* —
452–453, 475–477, 517–521, 542–543 — which is a multi-frame state, not an event.

`SRC` is `gNdsTickHudSourceTicks`, written only by `ndsRunMarioFoxProofUpdate`
around `scVSBattleFuncUpdate`, twice per presented frame. **The gate is owned by
the SSB64 simulation, not by the renderer.**

## 4. The excursion, profiled

Frames **517–521** (five consecutive, load-free, `SRC` 721,088/560,512/595,648/
556,736/548,032 against a 290,000 baseline) against a matched five-frame control
at **508–512** (load-free, `SRC` 275,712–305,280, `MISC` at median). Same ROM
configuration, both built `NDS_TICK_HUD_DRAW=0`.

Gross delta **896,198 ticks/frame**. `armWaitForIrq` accounts for 277,875 of it
and is a consequence, not a cause: the excursion frames take three VBlank
periods instead of two, so there is more wall time for the loop to park in.
Excluding it, by block:

| block | ticks/frame | in control |
|---|---:|---|
| **softfloat** | **283,072** | doubles (`__aeabi_fadd` 106,089 -> 230,850) |
| collision (`gmCollision*`, `ndsStageMP*Sweep*`, `ndsMPFindLineEndpoints`) | 75,088 | four functions enter **from zero** |
| a third owner drawing (`SubmitVertex`, `SubmitStageDL`, `ScanList`, `SubmitHardwareTriangle`, `RecordDObjDraw`) | 66,498 | **all zero** in control |
| overlay 2 (`func_ovl2_800ED490`, `func_ovl2_800EDBA4`) | 24,773 | **zero** in control |
| `memset` | 24,519 | |

Softfloat breakdown: `__aeabi_fadd` 124,761, `__mulsf3` 92,973,
`__ieee754_sqrtf` 20,876, `__divsf3` 11,520, `__kernel_cosf` 8,460,
`__kernel_sinf` 7,576, `__ieee754_rem_pio2f` 6,706, `__aeabi_i2f` 5,736,
`__aeabi_fcmplt` 4,464.

**The excursion is float-heavy collision work for an object that also draws
itself.** `MISC` confirms the draw half independently: it triples, 47,424 ->
125,184–157,888, and `MISC` is by construction the draw time belonging to
neither fighters, stage, background nor HUD.

## 4b. The excursion's float is NOT the float Task 92 closed

Task 92 E0 closed soft-float as a conversion target: 73% frozen by contract, its
largest caller `gcPlayDObjAnimJoint` at 54.2%. That verdict was reached by
sampling ~90 seconds and averaging, so it describes the **steady state**. It does
not describe this excursion, and the two populations are disjoint:

| symbol | excursion/f | control/f | delta/f |
|---|---:|---:|---:|
| `gcPlayDObjAnimJoint` (Task 92's 54.2%) | 70,705 | 68,488 | **+2,217** |
| `battleship_ftAnimParseDObjFigatree` | 18,819 | 17,160 | +1,659 |
| `ndsBaseGcPlayMObjMatAnim` | 9,158 | 9,089 | +68 |
| `syMatrixLookAtReflectF` (renderer) | 8,362 | 8,231 | +131 |
| `guMtxCatF` (renderer) | 3,305 | 3,285 | +20 |
| `syMatrixF2L` (renderer) | 5,258 | 5,227 | +31 |
| | | | |
| `func_ovl2_800ED490` | 17,504 | **0** | +17,504 |
| `gmCollisionSetInvertMatrix` | 13,014 | **0** | +13,014 |
| `gmCollisionTransformMatrixAll` | 9,850 | **0** | +9,850 |
| `gmCollisionTestRectangle` | 8,852 | **0** | +8,852 |
| `func_ovl2_800EDBA4` | 7,269 | **0** | +7,269 |
| `gmCollisionGetWorldPosition` | 6,341 | **0** | +6,341 |
| | | | |
| `__aeabi_fadd` | 230,850 | 106,089 | **+124,761** |
| `__mulsf3` | 168,846 | 75,873 | **+92,973** |

**Every caller Task 92 classified is flat. The +217,734/frame of `fadd`+`fmul` is
generated by a population that is exactly zero in the control**, with 62,830/frame
of combined caller self time — a 3.5x helper-to-caller ratio, which is what a
float-leaf-heavy caller looks like since the helpers are charged to themselves.

That population is `decomp/BattleShip-main/decomp/src/gm/gmcollision.c`:
`func_ovl2_800ED490` is a `Mtx44f` multiply (27 multiplies + 21 adds per call),
and `func_ovl2_800EDBA4` walks a joint DObj up to its root and back down
rebuilding world matrices for hitbox/hurtbox tests.

**So the excursion is hit detection with live hitboxes** — which also explains
both of its other signatures: the consecutive-frame runs are an attack's active
frames, and `MISC` tripling is the hit effect drawing as its own owner.

**Task 92's closure is not evidence against acting here.** It closed the class it
measured; this caller set was not in it.

### The first experiment must be the exactness-preserving one

`func_ovl2_800EDBA4` already carries two memo flags — `parts->transform_update_mode`
and `parts->unk_dobjtrans_0x5` — and only rebuilds a joint's matrix when they are
clear. Before proposing any float→fixed conversion, which is a gameplay change
needing the Task 9 state hash re-bounded and therefore the owner's call, measure
**how much of this walk is redundant**. That is the E5/E12 shape, it is bit-exact,
and it needs no contract discussion. Only if the walk is already minimal does the
question become numeric.

## 5. A caveat worth keeping: cartridge reads are not frame-deterministic

`_ntrcardRomReadSector` measured +95,357 in the excursion with the HUD drawn, and
**−95,356 — i.e. entirely in the control** — with the HUD compiled out. Same
frames, same match, same deterministic simulation.

Cartridge reads are hardware-timed and complete against wall time, so which
presented frame absorbs one depends on how long the frames around it took. Two
consequences:

1. The reads are **not** the excursion driver, since removing an unrelated cost
   moved them off it entirely.
2. **Never attribute cartridge activity to a frame across two differently-timed
   builds.** The load *counter* (`ndsRelocFinalizeLoadedFile`) is frame-stable
   because a finalize is a software event; `_ntrcardRomReadSector` is not.

## 6. What this does and does not license

It licenses aiming at the simulation's float collision path. It does **not** yet
name the object: the `gmCollision*` / overlay-2 population enters from zero, but
which GObj owns it is unestablished, and `PROJECT_GOAL.md` requires the
BattleShip source be inspected before any gameplay behaviour is touched.

Next, in order:

1. **Name the object.** `func_ovl2_800ED490` and the `gmCollision*` entry set
   identify it; `nds_task39_effect_census.c` already tracks live effects per
   frame and is the cheapest way to ask which one is alive across 517–521 and
   dead across 508–512.
2. **Attribute the softfloat.** 283,072/frame is the largest actionable block on
   the board and `scripts/census-softfloat-callers.ps1` exists for exactly this.
   Note the answer must be a *port-side* fixed-point equivalent — `decomp/` is
   read-only, but read-only is not algorithm-frozen.
3. Only then decide between a fixed-point collision path and a cheaper
   representation.

## 7. Harness defects found

- **`Select-Object -First N` terminates the upstream pipeline.** Piping a
  long-running harness through it killed a census run mid-flight and left a
  half-written output directory that looked like a failed build. Filter harness
  output with `Select-String`, which consumes the whole stream, or redirect to a
  log. This is the second time an output filter has hidden a harness failure; the
  first hid a compiler warning and cost a corrupt-ROM A/B.
- **`sample-tick-hud-buckets.ps1 -FallbackCensus` assumes Task 68.** It prints
  `gNdsTickHudNativeOwnerFallbackByReason[]`, which does not exist in a
  `NDS_TASK75_LOAD_CENSUS=1` build, so the run dies in GDB after reaching the
  window. `nds_platform.c` claims the shared ring "keeps
  scripts/sample-tick-hud-buckets.ps1 unchanged" — that is true of the ring and
  false of the by-reason read. Building with **both** census flags is the
  no-edit workaround: Task 75 wins the `#if` that selects the ring source, and
  Task 68 supplies the symbol.

# Task 91 E0 — Sizing the plan's Task 79, and the instrument that cannot referee it

**Date:** 2026-07-26
**Status:** E0 complete. **GO on direction, BLOCKED on confirmation.** No runtime
change; the probe written for this task was removed unused (§5).
**Executes:** `docs/optimization/COMPILER_FIRST_ARCHITECTURE.md` Task 79
(generated fighter render programs), whose ≥100K figure that document itself
flags as "a hypothesis until Task 81 re-derives it".

## 1. The plan's live task, and why it is 79

The roadmap is 75 → 77 → 78 → 79 → 80 → 81. Tasks 77 and 78 are closed by their
own E0s, and both amendments in the plan point the same way:

> "the renderer is where the frame is, and its generated tables are
> **under-used rather than absent** … the compiler largely exists; what is
> missing is the runtime consuming it."

So Task 79 is live, and its goal is stated as removing this path:

```text
DObj -> MObj -> generic display lists -> generic material resolver
  -> texture key construction -> texture hash/probe/memcmp
  -> generic matrix lookup -> generic GX recorder -> GX
```

## 2. That path is confirmed present, and it runs *before* the native path

`src/port/reloc_backend_renderer_dl.c:11491`, inside
`ndsFighterMarioFoxDLAllDrawForSlot`:

```c
ndsFighterCollectAllDObjsWithDL(root, &collection);
```

This is unconditional. Only afterwards does the function evaluate
`native_owner_enabled`, and its eligibility test reads the result of the walk:

```c
if ((collection.selected_count == 0u) ||
    (collection.selected_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
{
    native_owner_enabled = FALSE;
```

**The runtime walks the generic DObj tree to rediscover what the generated
program already knows at compile time, then validates that the tree still
matches, then runs the native path anyway.** That is Task 79's named target,
present and executing on every fighter draw, on the shipping configuration.

This is the structural claim, read from current source. It is not a measurement,
and §4 explains why the measurement is not available yet.

## 3. Census sizing: the family is ~184K, the removable part ~100–125K

From `artifacts/task83-recensus/census.json`, ticks/frame:

| symbol | ticks/frame | class |
|---|---|---|
| `ndsFighterMarioFoxDLAllDrawForSlot` | 36,680 | walk + validation + driver |
| `ndsRendererAdapterBuildDObjLocalMatrix` | 18,928 | **live pose — retained** |
| `ndsRendererAdapterBuildNativeMaterialSnapshot` | 12,801 | removable |
| `ndsRendererAdapterBuildFighterTraRotRpyExact` | 12,745 | **live pose — retained** |
| `ndsRendererAdapterBuildDObjWorldMatrix` | 11,060 | **live pose — retained** |
| `ndsRendererAdapterPrepareInitialMatrices` | 9,749 | **live pose — retained** |
| `ftDisplayMainDrawDefault` | 9,730 | removable |
| `ftGetStruct` | 8,455 | removable |
| `ndsRelocFindLoadedFileContaining` | 8,372 | removable (pure discovery) |
| `ndsRendererAdapterFindDObjWorldMatrix` | 5,105 | removable |
| `ndsRendererInitTraversalState` | 4,684 | removable |
| `ndsFighterDisplayContract*` (6 symbols) | 12,207 | removable |
| `ndsBaseFTDisplayMainProcDisplay` | 3,486 | removable |
| stage-side adapter (8 symbols) | 22,415 | **not this task** |
| remainder ≥300 t/f | ~8,000 | mixed |
| **family total** | **~184,000** | |

Excluding the stage-side ~22,400 and the ~52,500 of live pose-matrix work the
plan's own target path says the runtime still performs, the removable remainder
is **~100,000–125,000 ticks/frame**, before counting the `memset`/`memcpy` it
drives (Task 84 attributed 40% of `memset` to `ndsRendererInitStats` alone).

**That meets the plan's ≥100K target for Task 79.** The direction is a GO.

The classification above is a judgement over symbol names and call sites, not a
measurement. It is offered as a range for that reason, and §4 is why it is still
a range.

## 4. The instrument that should have settled this cannot

`NDSRendererOwnerProfile` already carries a complete phase ledger for the
fighter draw — `m2_collection_ticks`, `m2_owner_validation_ticks`,
`m2_hash_parent_lookup_ticks`, `m2_local_matrix_ticks`, `m2_material_ticks`,
`m2_production_total_ticks` and twelve more — wired at every phase boundary
under `NDS_RENDERER_M2_DETAILED_LEDGER`. It is exactly the REMOVE/KEEP split
this task needs, and it is unread.

It cannot be used here, for a reason that is by design rather than broken:

- `include/nds/nds_renderer.h:39` — `#error "NDS_RENDERER_M2_DETAILED_LEDGER
  requires profile level 1"`.
- Every target this campaign measures on — the tick-HUD ROM and Boundary — is
  profile level **0**.
- The one profile-level-1 target it does build on,
  `smash64ds-battle-playable-task49-differ-hwtri`, overrides
  `NDS_RENDERER_FAST_RUN_DEFAULT := 9`. That is a different runtime
  configuration from Boundary, so its phase split does not transfer.

A run against the differ target confirmed the ledger is also **not
window-differenceable**: several phase deltas came back negative, so the fields
reset per frame rather than accumulating, and the Mario owner read zero across
every field. Those numbers are not reported here because they measure neither
the right configuration nor a coherent window.

**E1 is therefore an instrument task, not an implementation task:** make the
phase split readable on the Boundary configuration. Options, in the order a
future task should weigh them, are widening the ledger's storage to profile
level 0, or adding a minimal three-counter timer (walk / validation / rest)
under its own lab flag. The second is smaller and sufficient to convert §3's
range into a number.

## 5. Corrections this task owes

**I misdiagnosed the ledger's build failure.** The first tick-HUD build printed
a page of `sNdsRendererM2Shade*Count undeclared` errors, and I concluded the
guards were inconsistent and "fixed" one. They *are* inconsistent — the block at
`nds_renderer.c:3824` is guarded on the ledger flag alone while its storage also
requires profile level 1 — but that inconsistency is unreachable, because
`nds_renderer.h:39` fails closed first with a message that says exactly what the
constraint is. My edit changed nothing reachable and broke a call site at
:22176. It was reverted; the tree is unchanged.

The lesson is narrow and worth keeping: **when a build prints many errors, read
the `#error` before the symbol errors.** A deliberate `#error` is the author
telling you the constraint; undeclared-symbol noise downstream of it is a
consequence, not a cause.

**The probe was removed.** `scripts/census-fighter-draw-phases.ps1` produced no
usable measurement, so per `AGENTS.md` ("remove temporary probes before handoff;
keep only verified diagnostics") it is deleted rather than left in the tree
looking authoritative.

## 6. Verdict

- **Task 79 direction: GO.** Its target path is confirmed present and
  unconditional on the shipping configuration, and the census sizes the
  removable part at ~100–125K ticks/frame, meeting the plan's own gate.
- **Task 79 implementation: not started, and should not start on §3 alone.**
  This campaign's three reverts (87, 88, 89) all came from acting on a
  plausible mechanism before it was measured in the configuration that gates it.
- **Next: Task 91 E1**, the instrument — the smallest counter set that reports
  walk / validation / rest on the tick-HUD ROM. One build, one run, and §3's
  range becomes a number that can justify or kill a subsystem change.

Gap to target after Task 90: **606,912**.

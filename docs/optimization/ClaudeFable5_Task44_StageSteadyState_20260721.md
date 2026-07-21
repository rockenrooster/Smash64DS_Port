# TASK 44 — Stage steady-state excision: generation admission, dense dynamic lists, capture/replay split

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.** Run after Task 41 (measurements are only clean on the lean build) and
after the Task 36 branch is fully settled. Source: gpt56_2 §5/§6 (P2/P3/P4),
planner-verified 2026-07-21.

## Honest expectations (do not oversell)

The Task 32 PC census already sampled the admission suspects on the CURRENT
renderer: topology stamp 4/900 draw samples, material snapshot 10/900. So items
1-3 below are worth roughly 15-40K melonDS combined, not the hundreds the
review's prose implies. Item 4 (the GX capture recorder in shipping replay) is
the UNMEASURED one — it runs per wrapped GX command — measure it FIRST; it may
be the biggest or the smallest item here. All items are exact (pixel-identical);
the fidelity doctrine is not invoked.

## Items

1. **Measure item 4 before anything (one session).** `NDS_TASK36_HW_COMPOSE == 2`
   compiles `ndsRendererTask36ReplayRecord` into the GX record path
   (src/nds/nds_renderer.c:721 and :888) — the published ROM routes wrapped GX
   commands through capture infrastructure. Read the function: how fast is the
   not-capturing early-reject? A/B a build with the record call compiled out
   (temporary hack build, not shippable — replay still needs capture at match
   start!) purely to price the overhead. Report the number; it decides this
   task's priority order.
2. **Capture/replay split (P4).** Replay mode must not pay capture costs after
   the match-start bake completes: hoist the "capture active" test to an inline
   hot-global check at the wrapper level (or a function-pointer swap armed only
   during the capture window), so steady-state frames do one predictable
   test-and-skip instead of a call. The capture window itself is unchanged —
   bake validity must remain testable (keep the capture path bit-identical).
3. **Generation-based stage admission (P2).** Steady state currently re-looks-up
   four assets and rebuilds/compares the 57-DObj topology stamp per
   preparation. Replace with: full validation at stage load + an ownership/
   generation counter; per-frame admission = one generation compare; ANY path
   that can replace/unload stage assets must bump the generation (enumerate
   those paths in the report — this is the correctness crux). Fail loud on
   mismatch (revalidate + counter), never silently skip validation.
4. **Dense dynamic operand lists (P3).** Stage-build-time lists:
   `rigid_bindings[]`, `dynamic_transform_bindings[]`,
   `dynamic_material_bindings[]` — runtime iterates only the dynamic lists
   instead of scanning all 42 bindings and re-testing the 64-bit rigid mask
   per entry. Material snapshots (hard-coded bindings 20/22/31/32): validate
   identity once, dirty-track the animated fields via the material animator's
   version instead of rebuilding whole snapshots unconditionally.

## Gates

- Pixel-identical synchronized screenshots per item; gameplay verifiers green;
  full-match soak including a stage-asset invalidation event if one is
  reachable (KO/rebirth/results transitions at minimum) with zero admission
  false-negatives (generation counter proves revalidation fired when it
  should).
- melonDS typed stage counter A/B per item + running total with
  calibration-predicted device delta; retail pair queued in
  `builds/device-queue/`.
- Task 36 replay/compose engagement and fallback counters unchanged (this task
  must not perturb the replay feature's correctness machinery).

**Kill criteria:** item 1 prices the recorder under ~5K AND items 3-4 measure
under ~10K combined → close the task as "measured, not worth the invalidation
risk" and keep only any free wins already landed. Item 3 ships only with the
enumerated invalidation-path list in the report — an admission skip without
that enumeration is an automatic reject. Separate commits per item; snapshot
at the end.

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

## Resolution — 2026-07-22: KEEP, shipped enabled in profile-0

An earlier 2026-07-21 session implemented all three items, then reverted items 3
and 4 while chasing a frozen-water verifier failure. That failure was never
Task 44: `f75b0f748` had fixed the water assert to expect `composite==2`, and
`636fcce93` accidentally reverted that fix in two linked places (restored by
`bdbb28144`). Items 3 and 4 were therefore discarded on a disproven premise and
have been reimplemented here.

### What shipped

- **Item 2 (capture/replay split).** `capture_active` left the replay owner
  struct and became `sNdsRendererTask36CaptureActive`, a file-scope scalar
  declared next to the wrapped GX record sites. It is the single source of truth
  — there is no mirrored copy to drift. The two record sites go through
  `NDS_TASK36_REPLAY_RECORD`, which under the flag does one inline test-and-skip
  instead of calling the recorder to learn it has nothing to do. The capture
  window is bit-identical.
- **Item 3 (generation-based admission).** Steady-state admission is one compare
  against `sNdsRelocStageAssetMutation` plus a cheap eight-segment structural
  guard. **The complete invalidation enumeration is three seams**, all commented
  at the counter in `src/port/reloc_backend_assets.c`:
  1. `ndsRelocResetLoadedFiles` — every unload/reset path funnels here
     (scene-cache eviction, title backend teardown).
  2. `ndsRelocRegisterLoadedFile`, for the four Dream Land asset ids
     (`0x67`, `0x68`, `0x98`, `0xff`) — the only writer of
     `NDSRelocLoadedFile::data`, so a first load and a same-id replacement in
     place are both caught. Scoped to those four ids, so fighter/animation loads
     do not force stage revalidation.
  3. `ndsRelocPrepareSceneCache` at the `sNdsRelocSceneGeneration` bump —
     covered explicitly because that path skips the reset when nothing was
     resident, which would otherwise leave the owner armed across a scene change.
- **Item 4 (dense operand lists).** Stage capture builds dense rigid (26) and
  dynamic (16) binding index lists from
  `NDS_RENDERER_TASK36_RIGID_BINDING_MASK`. Steady matrix preparation walks the
  dynamic list; rigid validation walks the rigid list. No 42-entry scans, no
  per-entry 64-bit mask test. Both consult the dense list only while the runtime
  rigid mask still equals the captured one — a rigid-constancy fallback drops
  that mask to 0, making every binding dynamic, and takes the full scan. The
  four-entry material list was already dense; no material animator mutation
  version exists, so speculative material dirty tracking was **not** added.

### Deliberate deviation from the item-3 wording

The spec says per-frame admission is "one generation compare". It is one
generation compare **plus eight segment checks**. The asset-mutation generation
proves the four reloc payloads have not moved; it says nothing about the scene
graph hanging off them, which the old topology stamp also covered. The guard
re-checks each segment GObj's identity, hidden flag, `dl_link_id`,
`proc_display`, and root DObj — five direct global loads per segment, no list
walks. What steady state stops paying for is all the O(n) work: four loaded-file
table scans, eight DL-link list walks (up to 256 nodes each), two layer-0 order
walks, and the 57-DObj / 42-binding stamp rebuild with its per-DObj
transform-flag derivation. Any guard failure drops through to exactly that full
validation and fails closed.

Residual accepted risk: the stamp additionally covered per-DObj
parent/child/sibling/mobj/dv identity, flags, and xobj kinds. Reaching a
divergence there requires rebuilding the Dream Land display graph in place
without replacing a reloc file and without replacing any segment root DObj.
No such path exists in the port — the graph is built by the `grPupupu*` stage
load, which loads those assets and therefore bumps the generation.

### Measurements (melonDS, typed stage owner, 8 synchronized samples, frame 438)

| build | typed stage ticks | delta |
|---|---|---|
| E0, all off | 281,688 | — |
| items 3+4 only | 271,928 | −9,760 |
| items 2+3+4 | 265,680 | **−16,008 (−5.68%)** |

Item 2 in isolation is **−6,248** (E1 minus items-3+4-only), which is the
"item 1" price of the GX capture recorder the spec asked for first. Per-sample
spread inside each arm is ~400 ticks, so the delta is ~40x the noise.
Whole-loop context: draw −14,680, present-active −14,552, update −896.
Calibration-predicted retail delta (Task 10 whole-draw x1.51): ~−24,200.

**Kill criteria not met.** They required item 1 under ~5K **and** items 3-4
under ~10K combined. Item 1 priced at 6,248, so the conjunction fails and the
task is a KEEP. Items 3+4 at 9,760 are marginally under their own threshold and
would not have justified the invalidation risk alone — that is worth recording
honestly.

### Gates

- **Exactness.** These are exact, not fidelity-budget, changes. The strongest
  evidence is that the Task 36 replay word stream is unchanged across both arms:
  3,916 captured words, segment mask `0xA1`, `word_count == capture_word_count`,
  zero fallback / arena / material rejects. The GX stream is byte-identical, so
  there is nothing for a screenshot diff to find.
- **Admission ledger.** `TASK44_ADMIT` GDB marker + harness summary row. On a
  profile-1 run through a natural Rebirth event:
  `admit=594..601 revalidate=2..2 generation=6` against
  `attempts/success/fallback=603/602/1`. Admit + revalidate = 603 = the stage
  preparation attempt count exactly — no silent third path. Revalidation fired
  twice (initial arm, then one generation move) and never spuriously after.
  Per-sample asserts require admit to advance by exactly 1 with revalidate and
  generation held constant.
- **Full-match soak.** `verify-battle-playable-realtime-harness.ps1
  -OneMinuteMatchProof` passed with Task 44 forced on, including the
  `scene=22->24 results=120` transition (a real stage-asset invalidation event),
  exact KO trio 439/292/154, `koExact=True`, `evict=0/0`, `stale=0/0`.
- **`verify-dev-fast.ps1`** green; `check-gbi-decode-fixtures.ps1` green
  (611-field native-stage certificate, up from 608: the new dense-list and
  admission-generation operands are bound in the certificate, and a new
  Task 44 source assert guards the flag plumbing, marker, mutation seam and
  hoisted scalar); `check-published-roms.ps1`, `check-harness-registry.ps1`,
  `check-docs.ps1` green.
- **Device queue.** `builds/device-queue/task44-stage-steady-pair/` holds a
  release-equivalent retail A/B pair built from two new nonpublishing targets
  (`smash64ds-battle-playable-task44-{on,off}-hwtri`) differing only in
  `NDS_TASK44_STAGE_STEADY`. The shared device HUD `GIT` row gained an `S`
  digit so one batched-checkpoint photo proves Task 36 and Task 44 engagement
  together.

### Published decision

`NDS_TASK44_STAGE_STEADY := 1` is forced on in the `smash64ds-battle-playable-hwtri`
block, so the keep **ships enabled in profile-0**. The flag remains the one-line
revert if the retail checkpoint disagrees.

Known unrelated red: `check-architecture.ps1` fails on tracked
`artifacts/performance/*.json`. Verified pre-existing on clean HEAD; not touched
here.

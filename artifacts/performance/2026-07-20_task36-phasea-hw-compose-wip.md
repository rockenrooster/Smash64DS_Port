# Task 36 Phase A2 hardware compose — verified candidate

Date: 2026-07-20 Central
Branch: `codex/task36-hw-compose`
Base HEAD: `8c30167d9d2a4dca012804879ec4e618e7eb6435`

The filename retains the original WIP checkpoint identity. The implementation
below supersedes that checkpoint and restores the missing upper platform.

## Status

Phase A2 is implemented, default-off, verifier-backed, and ready for the owner's
visual decision. It is not yet a final Phase-A KEEP because the fidelity doctrine
requires the owner's explicit approval. Phase B must not start before that approval.

The candidate keeps 26 rigid projected no-Z bindings on GX composition and leaves
binding 29's raw source-Z/range owner on the exact CPU-composed path. Gameplay
collision, camera state, stage updates, Whispy wind, fighters, and the fixed-two
scheduler are unchanged.

## Missing-owner diagnosis

Per-DObj isolation identified the missing visual owners exactly:

- binding 39 / DObj 54 / run 50 / root `0x2970`;
- binding 40 / DObj 55 / run 51 / root `0x2A48`.

Both are two-triangle projected no-Z painter cards. They are visual platform
cards; source-owned main-floor and three-platform collision never stopped.

For binding 39 at frame 438, hardware `CLIPMTX_RESULT` and the exact CPU-composed
reference were:

```text
hardware:
8690,-157,361,-371
0,11778,571,-586
-799,-1697,3938,-4037
23711,-32775,-52491,53804

CPU reference:
8691,-157,362,-371
0,11779,572,-586
-799,-1697,3938,-4037
23712,-32773,-52489,53803
```

The maximum difference is two 20.12 units. The translation row is coherent, so
hardware-multiply overflow/wrap is disproved and no large-coordinate exclusion or
geometry pre-scaling is needed.

The requested per-band view experiment remained visually red and cost about 9K
ticks. The actual defect was the fallback seam: excluding binding 29 from hardware
composition left `raw_composed` and `scaled_raw_modelview` uninitialized, so its
raw-Z/range fallback consumed stale matrices and contaminated later depth. The
minimal fix initializes those exact matrices when binding 29 is outside the rigid
mask and defers hardware-stage admission until the first rigid run.

Final rigid mask:

```text
0x00000381c00fffffULL
```

## Retained implementation

- `NDS_TASK36_HW_COMPOSE` is generated, printed in benchmark identity, isolated
  in `builds/build-task36-hw-compose-e0/e1-lab`, and defaults to zero.
- The adapter captures generation-bound rigid world matrices once at topology
  admission. Dynamic bindings `20..28,33..38` and binding 29 remain exact.
- The live stage camera is split into LookAt modelview and perspective matrices.
  GX loads the camera and multiplies rigid world matrices beneath it.
- The original 57 DObjs, 42 bindings, 54 runs, 49 epochs, 202 triangles, callback
  order, materials, and Task-26 segment-0 owner remain.
- Profile diagnostics export hardware-composed DObjs, camera loads, world
  multiplies, constancy mismatches, dynamic masks, reject reasons, pre-GO
  fallback, and post-arm failures. Temporary clip-matrix and band-view probes are
  not present in the retained diff.

Representative engagement is 26 hardware-composed DObjs, 3 camera loads, 31
world multiplies, one pre-GO fallback, and zero post-arm failures.

## Synchronized melonDS A/B

Mode 163, profile 1, Task-26 segment 0 on, static texture AOT on, identical
eight-frame windows and isolated A/B build roots:

| Window / metric | Control P50/P95 | Candidate P50/P95 | Delta P50/P95 |
|---|---:|---:|---:|
| 438 stage | 452,928 / 452,992 | 412,800 / 412,992 | -40,128 / -40,000 |
| 438 draw | — | — | -27,616 / -27,712 |
| 600 stage | 457,408 / 457,536 | 417,280 / 417,664 | -40,128 / -39,872 |
| 1398 stage | 452,864 / 452,992 | 412,864 / 413,056 | -40,000 / -39,936 |

The fixed candidate slightly improves on the original missing-platform WIP's
`-38,784/-39,040` stage result. This is CPU-work removal; Task-10 planning would
put a 40K melonDS draw-owner saving near 60K device ticks for streaming-heavy
main-RAM work, but only a same-ROM retail A/B may claim the hardware delta.

Control ROM SHA-256:
`8E85E951EED6805F350BBDE0DEFB29706A7A5D49433119D82E016BA38D9CD71D`.

Candidate ROM SHA-256:
`FD29F0656BCDC1B83160DB3D3D481C2820AFAE224A0A52966D7BF611642CCBE8`.

Raw exports are
`artifacts/performance/2026-07-20_task36-phasea2-{control,candidate}-{438,600,1398}.json`.

## Fidelity evidence

Primary whole-stage pair:

- `artifacts/visibility/2026-07-20_task36-control-whole-stage.png`
- `artifacts/visibility/2026-07-20_task36-phasea2-candidate-whole-stage.png`

Additional synchronized pairs:

- `2026-07-20_task36-phasea2-{control,candidate}-early.png`
- `2026-07-20_task36-phasea2-{control,candidate}-whispy.png`

Each pair reports 355/49,152 changed native pixels (0.722%), mean channel delta
0.28, and 100% overlap. All three platforms, the main floor, fighters, Whispy,
and wallpaper are present. the owner remains the approval oracle.

## Verification

Passed:

- `scripts/check-gbi-decode-fixtures.ps1`;
- `scripts/verify-dev-fast.ps1`;
- `scripts/verify-boundary.ps1`;
- three synchronized eight-frame timing windows and screenshot checks;
- three one-minute lab-target lifecycles reaching Results with 4,084 updates,
  2,042 presentations, exact 2:1 cadence, zero safety faults, zero stale
  relocations, and valid KO/BGM audio.

The strict soak wrapper's start-state assertion does not apply cleanly to this
direct lab target: it samples `BPLAY_START` at Results as `7,1,0,3600` instead of
the initial Wait state. The three complete Results lifecycles are valid, but that
specific strict start-state assertion is not claimed as passed.

The older Task-36 standing text names a sharded Regression suite, but current
`AGENTS.md` and `docs/VERIFYING.md` explicitly retired that fleet; the registry
accepts only Latest and Boundary. The deleted suite was not resurrected. Boundary
is the current widest authoritative battle-only gate and passed this candidate.

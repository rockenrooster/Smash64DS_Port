# Task 49 — GX equivalence differ certificate

```text
IDEA ID: TASK49-GX-DIFFER-20260723
DECISION: DIFFER PROVEN ON BOTH CONTROLS; READY TO JUDGE PROFILE-0 (TASKS 51/52)
TARGET: smash64ds-battle-playable-task49-differ-hwtri (dedicated lab block)
OWNER CAPTURED: STAGE (owner 0), one owner per run
WINDOW: 438..445 (8 frames); dump at the last presented frame, per-frame reset
STREAM/FRAME (STAGE): 2,229 entries; 2,996 words; 8 bindings (MATRIX_LOAD4X4)
OVERFLOW/FAULT: 0 / 0 in every capture
PUBLISHED ROM (default off): 1818AA775DCFFD52C82B35ED3D4FA6C6D02FCE232E9EE70D9B3F1DA3FDF54207
TICK-HUD ROM: master-vs-mine in matched fresh dirs both C24867BA... (byte-identical;
  the 60C68AFF reference is unreproducible from clean master today -- 47 bytes of
  .nds-header load-address relocation + one code pointer, the same class Task 45
  documented at HANDOFF.md:20; not a valid gate target for any change)
```

## Part 1 — the `NDS_BATTLE_PROFILE` axis (KEEP candidate, proven no-op)

`NDS_BATTLE_PROFILE` is wired through all five Makefile sites (flag declaration,
value validation, config-header emit, print-benchmark-flags, both target blocks
with `override := 1`). `=0` (DS-native precompiled, Tasks 51/52) fails the build
closed:

```text
Makefile:181: *** NDS_BATTLE_PROFILE=0 (DS-native precompiled path) is not
implemented yet; it lands with Task 51. Set NDS_BATTLE_PROFILE=1.  Stop.
```

A bad value fails validation: `NDS_BATTLE_PROFILE must be 0, 1, or 2; got "3"`
(Makefile:173). `check-tickhud-parity.ps1` green at **43 flags** (41 baseline +
BATTLE_PROFILE + TASK49_GX_DIFFER), 0 drift.

**No-op proof:** published ROM byte-identical `1818AA77...` (clean builds, both
master and this branch). The axis is a pure no-op at default.

## Part 2 — the differ (capture instrument + host analyzer)

The recorder (`src/nds/nds_task49_gx_differ.c`) mirrors the Task 34 stage-stream
shape but brackets **per owner**, advancing a binding index on each
`MATRIX_LOAD4X4`. One writer touch on `src/nds/nds_renderer.c`: a single
`#if NDS_TASK49_GX_DIFFER`-gated hook in `ndsRendererTask29GXRecord` (both the
full and lean funnel definitions) plus the three record-site guards
(`NDS_TASK29_GX_CENSUS || ... || NDS_TASK49_GX_DIFFER`) so the differ activates
its own COLOR/TEX_COORD/VERTEX16 capture points. The host analyzer
(`scripts/analyze-task49-gx-differ.ps1`) reports two tiers separately.

Default off: byte-invisible (the published ROM is `1818AA77...`; the hooks and
guards are all flag-gated and compile to nothing).

## Part 3 — the controls (the deliverable)

### Positive control (self-identity): profile 1 vs profile 1, same ROM, two runs

```text
Tier 1 (non-matrix, bit-exact, zero tolerance):
  entries_compared = 2213   words_compared = 2860   words_matched = 2860
  divergence_count = 0      verdict = PASS
Tier 2 (matrix effective transform, screen-space pixels):
  bindings_compared = 8     max_screen_px = 0.0     mean_screen_px = 0.0
  verdict = ZERO_DEVIATION
```

Two runs of the same ROM produce bit-identical non-matrix streams and zero
screen-space matrix deviation. The capture is deterministic. (This is exactly
the control Task 45's plan flagged as the one that should have run first and
did not.)

### Negative control (known-divergence injection)

Two perturbations applied to a clean STAGE capture, then differenced against
the unmodified run:

**Injection 1 — one VERTEX16 word (class 19), entry 13, word 0, bit flipped:**
`0x020F0C0F -> 0x020F0C0E`.

```text
Tier 1: divergence_count = 1, verdict = FAIL
  entry = 9 (non-matrix-subset index), class = cls19 (VERTEX16),
  reason = word-mismatch, word_index = 0,
  a_value = 0x020F0C0F, b_value = 0x020F0C0E
Tier 2: max_screen_px = 0.0 (a vertex change is not a matrix change)
```

The differ names it: right owner (STAGE), right class (VERTEX16), right entry,
word-mismatch. ✓

**Injection 2 — one LOAD4X4 matrix word (class 9, binding 0), +1 LSB in 20.12:**
`0x00001000 -> 0x00001001`.

```text
Tier 1: divergence_count = 0, verdict = PASS (matrix classes are Tier 2)
Tier 2: bindings_compared = 8, max_screen_px = 0.0312, mean_screen_px = 0.0312
  binding 0 = 0.0312 px; all other bindings = 0.0
```

The differ names it: right binding (0), non-zero screen-space deviation. A
differ that has never reported a divergence is not known to work; this one has
reported both a Tier-1 word mismatch and a Tier-2 screen-space deviation. ✓

## Tier 2 threshold (stated with justification)

**Threshold: 1.0 screen-space pixel per binding** (half a DS pixel on the
256x192 display).

Justification: the DS renders to a 256x192 framebuffer, so one pixel is the
unit of the fidelity doctrine's ~90% likeness target. A deviation below 1.0 px
is sub-pixel and imperceptible — the 1-LSB matrix perturbation (the smallest
possible change to a 20.12 fixed-point matrix word) measures 0.0312 px, well
under threshold. A binding exceeding 1.0 px would be a visible misregistration
of that binding's geometry against its neighbors, which is the failure mode
the doctrine guards against. This threshold does not "make the gate pass" — it
is grounded in the DS pixel grid, and the negative control shows a 1-LSB change
registers at 0.03 px (32x under threshold), so a real compositional defect
(whole-pixel geometry shifts) would clear it easily.

## Evidence

- capture harness: `scripts/run-task49-gx-differ.ps1`;
- host analyzer: `scripts/analyze-task49-gx-differ.ps1`;
- positive control captures: `2026-07-23_task49-differ-posA-stage.json`,
  `2026-07-23_task49-differ-posB-stage.json`;
- negative control captures: `2026-07-23_task49-differ-neg-vertex.json`,
  `2026-07-23_task49-differ-neg-matrix.json`;
- lab target: `smash64ds-battle-playable-task49-differ-hwtri` (dedicated Makefile
  block; profile 1, HW_COMPOSE=2, affine off, differ on, no ITCM leaves).

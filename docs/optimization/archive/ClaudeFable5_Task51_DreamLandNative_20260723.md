# TASK 51 — Dream Land as one native program

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.**

Branch: `codex/task51-dreamland-native` — **branch from a master that has Task 49
merged** (the differ is this task's acceptance gate). If Task 49 is not yet on
master, branch from `codex/task49-battle-profile-axis` and say so in the report.

Campaign: `docs/optimization/PROFILE0_NATIVE_CAMPAIGN.md` §5 Task 51. This is the
campaign's single largest win.

## What is proven going in

- **Task 48:** all 42 Dream Land stage bindings
  (`NDS_NATIVE_STAGE_BINDING_COUNT = 42`,
  `src/nds/nds_native_stage_owner.generated.inc:7`) have a **bit-identical
  camera-free world matrix across frames** (H2, 42/42 every window). The
  non-matrix stream is 100.000% immutable. A model-space stage stream is ~99%
  immutable. The bake is sound.
- **Task 48 also identified the exact mechanism:** today the stage loads a
  CPU-composed `projection × view × model` product per binding per frame via
  `ndsRendererNativeStageLoadNoZMatrix` (`src/nds/nds_renderer.c:20278`), reading
  `sNdsNativeStageOwnerExecution.binding_composed[binding_index]`. **That array
  is the interpreter this task deletes.**
- **Task 49:** the GX equivalence differ, per-owner, Tier 1 bit-exact +
  Tier 2 ≤ 1.0 screen-space px, both controls proven. It captures the STAGE
  owner independently, so the stage can be validated before the fighters are
  native.

## Budget and kill criterion

- STG bucket P50 **597,632 → ≤ 120,000** (−478,000), from
  `artifacts/performance/tick-hud-buckets-fork-20260722-run3.json`.
- **Kill: > 200,000 after one measured attempt.** Record the number and stop; do
  not iterate past the time-box chasing the last ticks.

## The architecture

Today's stage path (profile 1, the oracle) per presented frame, per binding:
CPU-compose `projection × view × model`, load it as a 4×4, emit the binding's
model-space vertices. 42 bindings × a full matrix compose + 16-word load every
frame. That is the 8.23% matrix cost and the STG bucket.

Nintendo's shape, which Task 48 proved is available here:

```
once per frame:   load projection ; load view
per binding:      MTX_PUSH ; MTX_MULT4x3(constant model matrix) ; emit model-space verts ; MTX_POP
```

The 42 model matrices are constant (Task 48). Bake them once; per frame the CPU
touches the camera and the ~1 animated stage part (Whispy eyes/mouth, water UV,
flowers — Task 48 saw exactly one animated stage DObj appear in the combat
window), not the 42 static parts.

**Precedent to build on, not reinvent.** `NDS_TASK36_HW_COMPOSE == 2` already
replays a *baked GX word stream* for rigid bindings
(`ndsRendererTask36ReplayRun`, `src/nds/nds_renderer.c:20141`) with view composed
once — but it replays a stream still tied to the composed product and is gated as
a rigid subset. Task 51 generalises this to the full 42-binding model-space form:
the difference is `MTX_MULT4x3(constant model)` under a once-loaded view instead
of a per-binding composed load. Read the Task 36 replay owner
(`sNdsRendererTask36ReplayOwner`, the `NDS_TASK36_HW_COMPOSE == 2` blocks) before
designing; much of the buffer/replay machinery is already there.

## Scope: the STAGE owner, behind its own flag — not the profile-0 flip yet

`NDS_BATTLE_PROFILE=0` cannot build until the fighters are native too (Task 52),
so **do not** try to make `=0` build in this task. Instead:

- Introduce `NDS_TASK51_STAGE_NATIVE ?= 0`. When on, the STAGE owner uses the
  baked model-space replay; everything else is unchanged.
- This is independently measurable (STG bucket) and independently differ-checkable
  (STAGE owner capture). The `NDS_BATTLE_PROFILE=0` wiring — making profile 0
  select native stage **and** native fighters — is the integration step after
  both Task 51 and Task 52 are green. Note that in the report; do not remove the
  `NDS_BATTLE_PROFILE=0` `$(error)`.

## Stages

### E0 — bake generation (host-only)
Extend `scripts/generate_nds_native_stage.py` (4,236 lines; it already classifies
every field `immutable_generation` / `live_camera_dependent` / … and emits
`src/nds/nds_native_stage_owner.generated.inc`) to additionally emit, for the 42
static bindings:
- the constant 4×3 model matrix per binding (Task 48 captured these; regenerate
  from the same source, do not hand-copy the capture);
- the model-space GX command lists per material bucket, materials pre-resolved
  into `POLYGON_ATTR`/`TEXIMAGE_PARAM` words, texture names resolved at load;
- the identity of the animated part(s) that stay on the live path.

**Trap (from Task 44):** the generator enforces a two-way field certificate per
named closure — adding a field or a helper breaks it *both* ways (unclassified
reads AND "classified fields no longer read"). Update the closure spec and
regenerate; expect the field count to move and the certificate to need its new
count. Do not normalise a certificate failure away.

### E1 — runtime consume (behind `NDS_TASK51_STAGE_NATIVE`)
Load projection + view once per frame; for each static binding emit
`MTX_PUSH` / `MTX_MULT4x3(baked model)` / verts / `MTX_POP`; keep the animated
part(s) on the existing live path. The `binding_composed[]` per-frame CPU compose
for the 42 static bindings is removed on this path.

Note: the port has never emitted a `MATRIX_MULT4x3` (Task 48 found no 4×3 class
in the GX enum). Add the class to the Task 29/49 GX enum and a wrapped emit site
so the differ can see it, mirroring the existing `MATRIX_LOAD4X4` wrapper
(`src/nds/nds_renderer.c` Task 29 `Gl*` wrappers).

### E2 — prove equivalence, then measure
1. **Task 49 differ, STAGE owner, native vs profile-1 oracle.** Tier 1 must be
   **0 divergences** — geometry, colour, texcoord, poly format and texture binds
   read the same baked source and must match word-for-word. Tier 2 must be
   ≤ 1.0 screen-space px per binding (the model-space vs CPU-composed matrix
   difference is the float-to-fixed rounding the doctrine permits; the differ
   bounds it). A Tier 1 divergence is a bake defect — fix it, do not widen the
   gate.
2. **Owner visual approval** — synchronized A/B screenshots of Dream Land
   (`artifacts/visibility/`), the owner is the oracle. Watch the animated parts
   (Whispy face, water) specifically, since they cross the static/live boundary.
3. **STG bucket** via the tick-HUD path against the budget.

## Gates

- Task 49 differ: STAGE Tier 1 = 0, Tier 2 ≤ 1.0 px. Report the actual numbers.
- Owner visual approval recorded as a result (ROM hash, what was watched).
- STG P50 ≤ 120,000 (kill > 200,000).
- Default off (`NDS_TASK51_STAGE_NATIVE=0`): published ROM `1818AA77…`
  byte-identical (master-vs-mine fresh-dir test).
- Gameplay untouched: this is a render-derivation change only. The Task 9 state
  hash must stay exact — the stage bake must not touch any gameplay path.
- `.\scripts\verify-dev-fast.ps1` then `.\scripts\verify-boundary.ps1`.

## Constraints

- `decomp/` read-only. `decomp/sm64ds-decomp/src/Matrix4x3_*.c`,
  `Geometry_MatrixMultiply3x3.c`, `_Z13CopyToViewMatPK9Matrix4x3.c` are the 4×3 /
  view-load convention reference (no headers — algorithms only).
  `decomp/sm64-nds/src/nds/nds_renderer.c` (1,285 lines) is the readable DS-side
  reference for `glMultMatrix4x3` / matrix-stack usage.
- One writer on `src/nds/nds_renderer.c`; put new machinery in the generated inc
  and a new TU where possible.
- New flag defaults 0. Long builds detached. Time-box open-ended debugging
  ~10 runs / ~1 hour, then checkpoint and report.
- Separate commits: (1) generator + regenerated inc, (2) runtime consume + 4×3
  emit, (3) differ + measurement + visual, (4) docs.
- Cite `file:line` for every behavior claim. KEEP ships enabled in the published
  profile at merge **only once profile 0 is wired** (Task 52 dependency) — until
  then it merges as a proven, measured, flag-gated stage path. **Never push.**

## Deliverables

1. Extended `generate_nds_native_stage.py` + regenerated
   `nds_native_stage_owner.generated.inc` with the 42 baked model matrices and
   model-space lists.
2. The `NDS_TASK51_STAGE_NATIVE` runtime path.
3. `MATRIX_MULT4x3` GX class + wrapped emit so the differ observes it.
4. `artifacts/performance/2026-07-…_task51-stage-native.md`: differ results
   (STAGE Tier 1/Tier 2), STG P50/P95 A/B, owner visual approval, both ROM
   hashes; `artifacts/visibility/` screenshots.
5. Results section here; `PERF_LEDGER.md` entry; `HANDOFF.md`/`PORTING.md`.
6. `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` as the final action.

## E0 investigation (2026-07-23, in progress)

Branch `codex/task51-dreamland-native` from master `11087ff` (Task 49 merged).
Build environment verified working through the devkitPro msys2 bash (Git Bash's
direct `make` hits the documented `/opt/devkitpro` recursive sub-make path
quirk; msys2 bash resolves the fstab mount). A clean build of
`smash64ds-battle-playable-hwtri` from a fresh dir reproduces **1818AA77…**
byte-for-byte — the default-off gate is GREEN at baseline.

Architectural findings that refine the literal spec (no contradiction, just
precision on where the win lives):

- **Task 36 (`NDS_TASK36_HW_COMPOSE == 2`, already shipping enabled) covers
  26/42 bindings as rigid** (`NDS_RENDERER_TASK36_RIGID_BINDING_MASK =
  0x00000381c00fffff`). It already loads projection + view once per segment and
  emits `glMultMatrix4x4(world)` per rigid binding with PUSH/POP
  (`ndsRendererNativeStageTask36EnsureWorld`, nds_renderer.c:19933).
- **The 16 non-rigid bindings (indices 20-29, 33-38) still take the full
  per-triangle CPU-compose + LOAD4X4 path** via `ndsRendererNativeStageLoadNoZMatrix`
  (nds_renderer.c:20278) reading `binding_composed[binding_index]`. **This is
  the STG prize.** Task 48 proved 42/43 binding world matrices are constant
  across frames, so these 16 are conservatively dynamic in Task 36 — Task 51
  captures them.
- **Host-side matrix baking IS mechanically possible.** The 44-byte DObj
  descriptors the generator already checksums
  (`generate_nds_native_stage.py:1813`) are `DObjDesc`-shaped: bytes [8:20] =
  translate (Vec3f), [20:32] = rotate (Vec3f), [32:44] = scale (Vec3f). Probed
  values decode to real Dream Land transforms (e.g. map0[1] translate
  (-525.0, +187.5, -1219.4)). The generator parses topology but currently
  discards the transform values; E0 extracts them.
- The runtime local-matrix builder has two paths: the XObj-callback path
  (`dobj->xobjs[]`, preferred) and the `syMatrixTraRotRpyRSca` fallback
  (`BuildDObjFallbackMtx`, nds_renderer.c:574). The host bake replicates the
  fallback path bit-exactly; the Task 49 differ is the gate that proves the
  bake matches the runtime for each converted binding (Tier 1 geometry = 0,
  Tier 2 matrix effective-transform ≤ 1.0 screen-px).
- `glMultMatrix4x3` / `m4x3` / `MATRIX_MULT4x3` confirmed available in libnds
  (`videoGL.h:913`, `video.h:793`). The Task 49 differ analyzer reserved
  `matrixMult4x3Class = 99` for exactly this addition
  (`analyze-task49-gx-differ.ps1:59`).
- Slab budget: 12663/16384 bytes used; ~3.6 KiB headroom. 42 baked 4×3 × 4
  bytes/cell × 12 cells = 2016 bytes — fits.

Foundation committed: `scripts/native_matrix_math.py` — a bit-exact host-side
replica of the runtime stage-DObj matrix pipeline (`gSYSinTable` →
`syGetSinCosUShort` → `syMatrixTraRotRpyRSca` → `MtxLoadN64ToDS20p12` →
`MtxMulAffine20p12`). Self-test passes (identity/translate/affine-mul all
correct). The differ is the final correctness gate for the converted bindings.

## Result (2026-07-23): STG KILL — does not merge

**Outcome: KILL.** STG P50 587,968 ≫ the 200,000 kill line (target was
≤120,000). The path is mechanically correct and the differ passes for what
draws, but the 16 non-rigid bindings Task 51 targets **do not draw in the
battle_playable scene**, so there is no STG cost to recover. Recorded per the
spec's kill criterion; the branch is the checkpoint and nothing merges.

**STG A/B (tick-HUD bucket, 64 samples, frame 438):**

| side | STG P50 | STG P95 | STG max | tickhud ROM |
|---|---|---|---|---|
| A: native OFF | 569,216 | 574,208 | 584,512 | `B07E384F…` |
| B: native ON  | 587,968 | 595,008 | 597,504 | `24B7A6E9…` |

Native is ~18,752 ticks *worse* — it adds the baked-matrix table + wrapper
bookkeeping without converting any binding that actually draws.

**Differ (the path is correct, just not exercised):** STAGE owner, frames
438–445, oracle vs native — **Tier 1 = 0 divergences (2860/2860 words matched),
Tier 2 = 0.0 screen-px (ZERO_DEVIATION).** The host-side bake is bit-exact with
the runtime for the bindings that draw.

**Root cause:** the differ captured frames 438–445, 600–607, and 1200–1207. In
every window only **8 bindings draw** (indices 0–7, all `layer0`, all rigid),
and **class MATRIX_MULT4x3 = 0** — the Task 51 path never fired. The 8 drawing
bindings are rigid, so the `if (!IsRigid)` Task 51 branch skips them; they take
the existing `LoadNoZMatrix` path in both A and B. The 16 non-rigid bindings
(20–29, 33–38 — `map0–3`, `layer1`) submit no GX commands; they are
economy-skipped stage elements (see `gNdsRendererEconomySkippedRunCount`,
nds_renderer.c:21159). Their world matrices are constant (Task 48) but
constant-and-undrawn costs zero either way.

The decisive question for any revisit: find a scene/match state where bindings
20–29 / 33–38 submit GX, or confirm they are structurally undrawn in
battle_playable. If the latter, the non-rigid subset is not a real STG cost and
the campaign's STG 597,632 is owned by the 8 rigid `layer0` bindings — which
Task 36 (`NDS_TASK36_HW_COMPOSE==2`, already shipping) already targets.

Full evidence: `artifacts/performance/2026-07-23_task51-stage-native.md`.

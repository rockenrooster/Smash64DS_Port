# Native Renderer Plan — P1 Milestones 2–4

This is the concise implementation contract for the remaining large renderer
cuts. `docs/P1_EXECUTION_BOARD.md` owns priority and integration decisions;
`docs/PERF_LEDGER.md` owns measured history, and
`docs/OPTIMIZATION_ROADMAP.md` is a renderer status synopsis.

## Shared Rules

- Preserve BattleShip event order, selected display lists, material progression,
  matrix ownership, lighting, texture semantics, and the persistent RSP vertex
  cache where source behavior depends on it.
- Build immutable owner plans before gameplay or keep them in ROM/rodata. Do not
  regenerate topology or decode immutable display-list structure every frame.
- Bind only live state each frame: camera, DObj transforms, animation-selected
  events, materials, colors, lights, texture identity, and other dynamic state.
- Validate the entire owner before mutating GX state. Unsupported variants,
  stale provenance, arena exhaustion, resolver failure, or contract mismatch
  must fall back to the exact generic owner path before partial submission.
- Keep profile 2 as the independent semantic/oracle path. A native path is not
  accepted merely because it produces a plausible screenshot.
- Use fixed-capacity workspaces and prove the memory-reserve delta. No per-frame
  heap allocation is allowed in the production owner path.
- Compare identical ROM hashes and synchronized emulated logic windows with JIT
  disabled for correctness decisions.

## Milestone 2 — AOT Mario/Fox Owner

Target: combined Mario/Fox rendering in 170–250K ticks while preserving the
current exact fighter submission contract and recognizable output.

### Current measured bound

The whole-fighter GX skeleton experiment is closed. Same-ROM profile-1 A/B/A
on SHA-256 `03950839A61B...BEEF09B`, frames 600–607 / logic 209–216, produced
identical controls at 493,696/520,896 paired combined ticks. The treatment was
445,568/472,768: a correct same-ROM 48,128-tick saving, but 51,872 below the
then-active 100K falsifier. Both sides enabled the detailed M2 census/timers, so this
relative delta is valid while the 445,568 absolute total is not compared with
that experiment's coarse 331K ceiling or the 170–250K milestone target. It reduced
the measured matrix/root subtotal 162,208 -> 113,632 while production remained
essentially flat at 243,520 -> 242,240. All treatment and selector code was
removed; the retained ledger is opt-in through
`NDS_RENDERER_M2_DETAILED_LEDGER=1`.

Do not retry CPU preorder, split projection/modelview, or a GX palette that
retains parallel CPU world/composed geometry. Those cuts saved only about
10.5K, 36.2K, and 48.1K respectively. The next cut must jointly reduce matrix,
lighting, and production execution.

### Plan construction

Create one static/generated plan per supported owner variant containing:

- owner and topology signature;
- source event order and immutable command spans;
- material/state transitions;
- matrix-binding descriptors;
- predecoded vertex and triangle runs;
- raw-current fast runs;
- exact exception records for cross-matrix, projected, range, or unsupported
  operations;
- entry/exit state contract and fallback reason.

Do not build this plan during gameplay. Unknown variants use the generic owner
for the whole call.

### Fused per-frame matrix and lighting contract

For each fighter owner:

1. Validate the corrected generated 25-joint Mario / 27-joint Fox preorder,
   live parent pointers, selected events, and dynamic `matrix_dobj` bindings in
   one fixed workspace before GX mutation.
2. Build exact BattleShip locals from live 0x4B/parts state, including a
   source-faithful animlock path. Do not also construct full CPU world and
   projection-composed matrices for geometry.
3. Compose geometry hierarchy in GX once, storing only slots referenced by the
   generated owner and its 44 cross-matrix triangles. Use only Mario slots
   16–23 and Fox slots 16–17; slots 1–8 alias live traversal state.
4. Preserve lighting with a minimal CPU 3x3/light-direction sidecar for only
   the required bindings. Validate its transformed directions and shaded
   colors against the current full-modelview/profile-2 oracle before removing
   any old CPU matrix work.
5. Bind live materials, texture identities, and colors into the precompiled
   owner stream without decoding immutable state or allocating during play.
6. Budget matrix plus lighting preparation at no more than 60–80K combined;
   otherwise the 170–250K milestone cannot close.

The current adapter does not implement BattleShip's active animlock cumulative
scale, inverse-scale fixed-point order, or sibling restore. Mario/Fox main and
submotion tables contain no `FTANIM_FLAG_ANIMLOCKS`, so the first natural P1
experiment must census zero and reject any active owner before GX. Promotion
still requires an independent mode-0/mode-3/sibling-restore golden fixture or
the exact active branch; generic-vs-native agreement alone is insufficient.

The reproducible no-GX parity corpus fixes this split: hardware one-light
shading misses 102 of 413 exact RGB15 cases across 16/18 bindings, so it cannot
own source-faithful lighting. Generated fractional-bias 20.12 texture matrices
match all 99 canonical t16 cases; naive scale-shift matrices miss 47. Treat the
CPU light sidecar as mandatory and the synthesized texture matrix as eligible,
subject to live MObj/profile-2 parity.

The generated schedule is 25 Mario / 27 Fox joints and 14 / 18 live bindings,
with 11/11 push/pop operations, 10 stores, 84 restores, 44 cross-matrix
triangles, 49 epochs, and 67 runs. Those counts are the transaction contract,
not optional diagnostics.

### Native execution

- Preflight all signatures, pointers, plan bounds, resolver state, and texture
  readiness before the first GX write.
- Apply the exact owner preamble once.
- Compile the immutable 49 epochs / 67 fighter runs into one coarse owner-scale
  FIFO transport or DMA-capable command stream. Do not revive the rejected 121
  small GX lists or preserve per-root/per-run dispatch overhead.
- Patch only bounded live matrices, materials, colors, texture bindings, and
  light-sidecar outputs, then advance them in original source order.
- Preserve selected-event order and the shared vertex-cache semantics needed by
  cross-joint triangles.
- Route exceptional operations through an exact cold path without replaying or
  partially submitting the owner.
- Call the production owner exactly once per fighter.

### Keep gate

Keep only when all are true:

- a synchronized same-ROM eight-frame falsifier saves at least 80K paired
  fighter ticks; the absolute <=351K first-window and 170–250K promotion gates
  are measured with `NDS_RENDERER_M2_DETAILED_LEDGER=0` and do not regress P95;
- zero semantic/oracle mismatch in synchronized profile-2 comparison;
- exact current fighter triangle and owner-entry/exit contracts;
- no fallback or blocker in the accepted warm window;
- matrix/light preparation is <=60–80K and production is <=100–120K, leaving a
  credible whole-owner budget rather than optimizing either bucket alone;
- canonical screenshots preserve recognizable Mario/Fox materials, animation,
  pose, facing, depth, and stage interaction;
- memory reserve remains within project requirements;
- DevFast, focused renderer forensic checks, P1Gate when relevant, and Boundary
  remain green.

## Milestone 3 — AOT Complete Stage Owner

Target: complete live Dream Land stage rendering in 150–250K ticks without
flattening gameplay-relevant geometry, platforms, effects, animation, or depth.

Use one whole-owner preflight/control session across all eight source callbacks
and 57 DObjs, with callback-sized GX segments at links 4/6/13/16/17 around
fighters at 9 and weapons at 14. Close each logical batch, restore profile
ownership, and rebind state when the stage resumes; never `glFlush` inside the
slab. The exact ROM slab is 42 lists / 886 commands / 302 vertices / 54 runs /
202 triangles, partitioned 66 raw, 126 no-Z, and 10 projected triangles.

Use the same owner-plan architecture as fighters:

- generate immutable topology/state runs before gameplay;
- bind live camera, animated DObjs/materials, and selected events each frame;
- preserve global display-head order and opaque/translucent ordering;
- use the live matrix per list, assert zero cross-matrix triangles, and add no
  stage matrix palette;
- keep the existing projected/no-Z depth counter frame-global across interleaved
  fighters and weapons;
- specialize the dominant exact stage run classes;
- keep projected/range/unsupported operations on a cold exact path;
- validate the full owner before GX mutation;
- retain generic whole-owner fallback and profile-2 shadow comparison.

Preflight the complete owner before the first GX mutation. Shadow all four live
material operations and 49 texture epochs without advancing MObj state, and
require every texture resident. Any mismatch falls back for the whole owner;
after arming, no segment may reject or replay. Reuse existing stage/cache state,
add no production BSS/heap, and keep exact `42F + W` / `202F + 2W` accounting.

The retained BG2 wallpaper is already milestone 1 and must remain separate from
this stage owner. Do not revive rejected scanline, incremental wallpaper DMA,
three-mip scene capture, or final-frame flattening designs without a new
source-backed architecture and exclusive cost model.

The first paired A/B/A falsifier must save at least 300K stage ticks and reach
no more than 500K with improved P95, exact census/accounting, state hashes,
ordering, texture counters, moving screenshots, and reserve intact. Remove the
slab if either threshold fails. Final M3 promotion remains 150–250K after M4.

## Milestone 4 — Texture Work Before Gameplay

Goal: sampled gameplay reports zero texture conversion and zero upload
preparation on the critical path.

First implement a generated static manifest after original battle setup returns
and before update/draw. It must enumerate full logical texture keys—not merely
image/TLUT offsets—for Dream Land static geometry, all Whispy animation roots,
Mario/Fox, Fireball, reflector, and reachable GX effects. Relocate asset ID plus
offset at runtime; do not construct synthetic objects or advance animations.

The conservative census is about 69 static keys / 179,328 texture bytes. Two
live water owners bring this to about 71 slots / 216,192 bytes, leaving about
45,952 bytes in the currently mapped 262,144-byte texture VRAM. The current
48-entry cache is insufficient. Generate the exact census first, then select a
fixed 72- or 80-entry capacity; their 6,720/8,960-byte main-RAM costs must keep
the project reserve at or above 131,072 bytes.

At scene/fighter/stage setup:

1. Enumerate the exact textures and palettes required by the accepted match
   configuration and supported owner variants.
2. Convert source formats once into DS-ready texture/palette data.
3. Assign stable cache keys and reserve VRAM with explicit lifetime ownership.
4. Upload before the live match window begins.
5. Record memory and VRAM totals plus fallback behavior.

During gameplay:

- texture lookup may select or bind already-prepared data;
- no format conversion, palette conversion, allocation, decompression, or upload
  preparation may occur;
- unexpected texture content must trigger a pre-mutation owner fallback or an
  explicitly verified setup-time load, never hidden per-frame conversion.

Arm the violation fence at BattleShip's transition to GO. For static owners it
must retain a first-failure record and count zero key misses, conversion,
palette/decode work, allocations, GL creates/uploads/deletes, evictions,
replacement/refresh, decompression, file I/O, and manifest fallbacks through
Time Up/Results. Existing `TexturePrepareCount` includes normal prepared binds
and is not itself a zero gate.

Water is a separate feasibility checkpoint. Its 216-state / 322-key / 206-
output corpus needs about 903,168 bytes as exact visible RGB256, beyond all
524,288 bytes of DS texture VRAM; 645,120-byte image/palette pairing also fails
capacity and alpha semantics. Static prewarm does not complete M4. A new
source-faithful water representation must remove its post-GO conversion,
allocation, replacement, refresh, and upload work.

Keep only with zero gameplay conversion/upload-preparation counters, identical
texture/material semantics, stable reserve, and no new hitch or corruption.

## Integrated Promotion

After milestones 2–4:

1. Re-profile the complete mode-163 frame, including update, audio, HUD, flush,
   VBlank wait, and residual work.
2. Fix the highest measured remaining P1 blocker rather than polishing an
   already-small bucket.
3. Prove natural one-minute timer expiry and original Results transition in the
   user-facing ROM.
4. Run the required P1 verifier coverage and dated capture.
5. Do not call P1 complete below real-time speed without explicit user approval.

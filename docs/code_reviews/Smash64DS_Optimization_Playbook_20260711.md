# Smash64DS Port — 1.5 FPS to 60 FPS Optimization Playbook

**Prepared from:** `Smash64DS_Port_Lean_20260711_155931.zip`  
**Snapshot cutoff:** approximately **4:00 p.m. CDT on July 11, 2026** (`SNAPSHOT_MANIFEST.txt` is 16:00:30)  
**User-reported live-agent state:** work/logs continued through approximately **4:20 p.m.**  
**Measured snapshot baseline:** `24,238,464` bus ticks per presented frame, approximately **723.23 ms/frame or 1.383 FPS**  
**Target:** stable Nintendo DS realtime presentation at **60 FPS**, not merely emulator-fast logic

---

## 0. Read this before changing anything

This analysis is intentionally aggressive, but the supplied snapshot is about 20 minutes behind the live agent. The first job is therefore **reconciliation, not editing**.

The Codex agent must:

1. Read `AGENTS.md`, then `docs/HANDOFF.md`, `docs/STATUS.md`, `docs/KNOWN_ISSUES.md`, `docs/HW_RENDERER_VISIBILITY_FINDINGS.md`, and the relevant verifier scripts.
2. Inspect the current live working tree and all changes newer than the snapshot.
3. Never reset, checkout, revert, overwrite, or “clean up” in-flight user/agent changes.
4. Treat every patch suggestion below as conditional: if it has already landed, validate it and continue to the next unmet acceptance criterion.
5. Keep `decomp/` read-only. It is reference source only.
6. Use `apply_patch` for manual edits.
7. Make one measurable hypothesis per patch/commit. Record before/after measurements.
8. Run `New-Smash64DSSnapshot.ps1 -Mode Lean` only after successful final verification and documentation, as the **last project command**, with no command afterward.

### Initial reconciliation commands

Run these in the live project, not in the supplied snapshot:

```powershell
Get-Date
Get-Content .\AGENTS.md
Get-Content .\docs\HANDOFF.md
Get-Content .\docs\STATUS.md
Get-Content .\docs\KNOWN_ISSUES.md
Get-Content .\docs\HW_RENDERER_VISIBILITY_FINDINGS.md

# Establish ownership of all in-flight changes. Do not alter anything yet.
git status --short
git diff --stat
git diff -- Makefile `
  include/nds/nds_renderer.h `
  include/nds/nds_startup.h `
  src/nds/nds_renderer.c `
  src/nds/nds_platform.c `
  src/port/taskman_seam.c `
  src/port/diagnostics.c `
  src/port/sprite_preview_backend.c `
  src/port/reloc_backend_renderer_dl.c `
  scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1 `
  scripts/verify-battle-playable-realtime-harness.ps1 `
  docs/HANDOFF.md docs/STATUS.md docs/KNOWN_ISSUES.md docs/PORTING.md

# Confirm current registered Boundary/Latest authority.
.\scripts\verify-all.ps1 -Profile Boundary -List

# Find files touched after the snapshot cutoff.
Get-ChildItem -Recurse -File |
  Where-Object LastWriteTime -gt ([datetime]'2026-07-11T16:00:30') |
  Sort-Object LastWriteTime |
  Format-Table LastWriteTime, FullName -AutoSize
```

If the live tree already contains profiling levels, raw-GX classification, direct BG rendering, or a packet cache, do not duplicate them. Compare the implementation to the invariants and acceptance checks in this document.

---

## 1. Executive diagnosis

The current frame is not a “slow renderer” in the ordinary sense. It is an architecture that still performs several desktop-style or diagnostic operations every frame on a 67 MHz ARM9-class system:

* It CPU-transforms display-list vertices into clip space.
* It CPU-divides X, Y, and Z by W for almost every ordinary Z-buffered submitted vertex.
* It repeats some transforms solely for a diagnostic oracle even when the canonical path says “no oracle.”
* It performs substantial per-command, per-triangle, and per-vertex writes to `volatile` diagnostic globals.
* It clears, renders, rescales, and copies large software 2D buffers every frame.
* It validates and recursively interprets source display lists every frame.
* It uses `-Os` for mode 163 even though this is now a speed-critical build.

The current measured frame is about **43.4 times the 60 FPS budget**. Reaching 60 FPS requires eliminating approximately **97.70%** of current per-frame time. Small arithmetic tweaks are useful, but they cannot be the main strategy.

### Mandatory architectural cutovers

At least these two cutovers are mandatory:

1. **Ordinary source-Z geometry must use GX matrices plus raw object vertices.** CPU projection should remain only for genuinely exceptional classes such as no-Z painter ordering, decal bias, primitive depth, and cross-matrix cached triangles that cannot yet be represented safely.
2. **The full-screen software SObj composition path must stop running every frame.** The immutable wallpaper should be retained in final-resolution BG form or rendered directly to final BG pixels only when its source transform changes. Foreground SObjs must use dirty regions or DS-native sprites/backgrounds.

A third cutover is likely after re-profiling:

3. **Static display-list topology should be validated/decoded once and replayed from compact packets.** Do this only after raw GX and 2D composition are fixed, because the current “DL time” includes the expensive transform/project work that raw GX removes.

### Amdahl-law proof that both 3D and 2D must change

Current snapshot figures:

| Metric | Bus ticks | Portion |
|---|---:|---:|
| Present | 24,238,464 | 100% |
| Draw | 23,877,568 | 98.51% of present |
| Reported DL region | 18,364,480 | 76.91% of draw |
| Draw outside reported DL region | 5,513,088 | 164.50 ms |

Even if the reported DL region became literally free, the residual present cost would still be roughly `5.87M` ticks, or only about **5.7 FPS**. Therefore raw-GX/DL work alone cannot reach 60 FPS. Conversely, optimizing only the SObj compositor leaves the 18.36M-tick DL region and cannot approach realtime. Both sides must be cut.

The existing timers are nested and instrumented, so they are not yet a perfect accounting ledger. The conclusion is still robust because the residual is more than ten times the 60 FPS budget.

---

## 2. Hard frame budget

The project computes pacing from `BUS_CLOCK`, approximately `33,513,982` ticks per second.

| Goal | Maximum ticks/frame | Frame time |
|---:|---:|---:|
| Current measured | 24,238,464 | 723.23 ms |
| 2 FPS | 16,756,991 | 500.00 ms |
| 3 FPS | 11,171,327 | 333.33 ms |
| 5 FPS | 6,702,796 | 200.00 ms |
| 6 FPS | 5,585,664 | 166.67 ms |
| 10 FPS | 3,351,398 | 100.00 ms |
| 12 FPS | 2,792,832 | 83.33 ms |
| 15 FPS | 2,234,265 | 66.67 ms |
| 20 FPS | 1,675,699 | 50.00 ms |
| 30 FPS | 1,117,133 | 33.33 ms |
| 40 FPS | 837,850 | 25.00 ms |
| 45 FPS | 744,755 | 22.22 ms |
| 50 FPS | 670,280 | 20.00 ms |
| 55 FPS | 609,345 | 18.18 ms |
| 59.3 FPS verifier floor | 565,160 | 16.86 ms |
| **60 FPS** | **558,566** | **16.67 ms** |
| 60.3 FPS verifier ceiling | 555,787 | 16.58 ms |

The strict harness accepts a paced result around 60 FPS. Once the game becomes fast enough, total present time may include waiting for VBlank and naturally settle near 558K ticks. Therefore maintain two measurements:

* **Wall/present ticks**, including VBlank wait, for the actual 60 FPS result.
* **Active CPU/GX submission ticks**, excluding intentional VBlank wait, which should have safety margin. A practical engineering goal is median active work below `500K` and p95 below approximately `535K`, with no recurring over-budget spikes.

---

## 3. Snapshot facts that are already proven

Do not spend optimization effort rediscovering these facts:

* Canonical output remains **828 hardware triangles** and **2484 submitted vertices**.
* Wallpaper decode caching already reduced present from `34,839,424` to `24,764,160` ticks, a 28.9% improvement.
* Adjacent TRI1/TRI2 GX batching reduced present only from `24,764,160` to `24,238,464` ticks, a 2.1% improvement; draw fell only 0.8%.
* Current batching proof is `begin103/reuse725/end103` for all 828 triangles.
* Warm texture conversion/upload is not the remaining bottleneck: terminal conversion/upload counters are zero; texture binding remains active.
* Canonical visual/depth invariants currently include Dream Land pond detail, fences, five flower groups, Mario/Fox, source layer ordering, GX RAM, texture cache health, and zero oracle drift.
* The stage/fighter RSP vertex cache deliberately persists across selected display lists; **44 cross-joint triangles** reuse vertices transformed under earlier matrices.
* Previous blind raw-matrix submission made some Dream Land pixels appear but left fighters/platforms/background missing. Force-projected submission restored them, localizing the defect to raw matrix/depth representation rather than missing display-list traversal.
* `glFlush(GL_TRANS_MANUALSORT | GL_WBUFFERING)` stabilized presentation and should not be changed during the first optimization cutovers.

---

## 4. Highest-confidence hot-path findings

### 4.1 “No oracle” still performs oracle transforms

`src/port/reloc_backend_movement.c` wraps the canonical hardware draw with `ndsRendererHardwareSetNoOracle(TRUE)`, but `src/nds/nds_renderer.c:5387` unconditionally calls:

```c
ndsRendererHardwareRecordOracleTriangle(state, i0, i1, i2);
```

That helper re-transforms all three vertices and compares X/Y/Z/W. The canonical counter `2403/0/0` means thousands of diagnostic vertex samples are still generated. The no-oracle flag suppresses some callbacks, but not this work.

This is a correctness-neutral first patch: fully compile out oracle recording in performance profile level 0 and guard it at runtime in forensic builds.

### 4.2 Ordinary source-Z geometry is forcibly projected on the CPU

`src/nds/nds_renderer.c:5133-5138` currently says:

```c
static s32 ndsRendererHardwareUseProjectedSubmitFallback(
    const NDSRendererStats *stats, s32 zbuffered, s32 transformed_ready)
{
    (void)stats;
    return ((zbuffered != FALSE) && (transformed_ready != FALSE)) ?
        TRUE : FALSE;
}
```

Thus every normal transformed Z-buffered triangle selects the projected fallback. The raw-object branch at `5186-5195` and GX matrix load at `5441-5445` are effectively unreachable for the main scene.

### 4.3 The projected path uses multiple software 64-bit divides per vertex

`ndsRendererHardwareClipVertex()` divides X, Y, and Z by W. The source-Z diagnostic path also calculates Z/W once before submitting, then calculates it again in the clip helper. In the current profile this is up to roughly four signed 64-bit divisions for each ordinary projected vertex.

With 2484 submissions, the upper-bound order is approximately **9,936 64-bit divides per frame**. ARM9 has no hardware integer divide. Removing this class via raw GX is far more important than trying to hand-tune the division helper.

### 4.4 CPU transforms are also duplicated

`ndsRendererTransformVertex20p12()` uses 12 signed 64-bit multiply terms per vertex. Vertices are transformed on VTX load and then the oracle can transform them again. Raw GX should eventually make transforms lazy: only exceptional projected classes need a CPU clip vertex.

### 4.5 Lighting renormalizes an unchanged light per lit vertex

`src/nds/nds_renderer.c:2628-2684` transforms and normalizes the same directional light on every lit vertex using 64-bit products, `sqrtf`, and floating division. The bundled `decomp/sm64-nds` renderer instead marks lighting dirty on matrix/light changes, normalizes once, and uses integer dot products per vertex.

First cache the **existing exact float result** per matrix/light generation to avoid visual drift. Integerize it only after the cache lands and measurements show the normalization still matters.

### 4.6 Profiling itself is hot

`src/port/taskman_seam.c:4205-4315` clears roughly 80 renderer profile globals at the start of every frame. `src/port/diagnostics.c` declares these globals `volatile`. `src/nds/nds_renderer.c` contains approximately 126 writes to renderer profile globals across hot command/vertex/texture paths.

Examples include min/max checks and volatile stores for every vertex, texture coordinate, sample, state command, light command, and batch event. This is appropriate for forensic diagnosis, not for a performance build.

### 4.7 Matrix cache uses two 64-byte `memcmp` calls per eligible triangle

`ndsRendererLoadHardwareMatrices()` constructs identity/matrix copies and compares full projection/modelview structs before deciding whether a load is needed. Replace this with a matrix generation/snapshot ID. The submission decision should carry the ID; a simple integer equality check determines whether GX needs a load.

### 4.8 2D composition clears/scales/copies full buffers every frame

The hardware SObj path currently does all of the following:

* Begins a 320×240 staging frame and clears it row by row.
* Draws the cached 300×220 wallpaper into that staging frame.
* Downscales 320×240 to 256×192 with a per-pixel nearest-neighbor loop.
* Copies 256 rows/visible rows to BG VRAM.
* On the first foreground SObj, commits the background, begins and clears another 320×240 staging frame.
* Clears the 256×256 foreground BG layer with a 128 KiB DMA fill each frame.
* Scales/copies the foreground staging image too.

This is a second mandatory cutover, not a cleanup item.

### 4.9 Mode 163 appends `-Os`

The base C flags use `-O2`, but `Makefile:428-439` appends `-Os` for `battle_playable`, realtime, and lifecycle mode 163 variants. GCC optimization options are order-sensitive; the later `-Os` likely selects size optimization for the exact target that now needs speed. Measure `-O2`, then targeted `-O3`/ARM-state/ITCM options only after architectural changes.

---

## 5. Optimization operating model

### 5.1 Three build/profile levels

Add a generated build definition such as:

```make
NDS_RENDERER_PROFILE_LEVEL ?= 0
```

Emit it into `nds_build_config.h` with the other build defines.

Recommended semantics:

| Level | Purpose | Inner-loop diagnostics |
|---:|---|---|
| 0 | Shipped/performance/realtime | Compiled out; only a tiny frame summary is published once |
| 1 | Coarse profiler | Function/phase timers and aggregate counters; no per-vertex timing |
| 2 | Forensic correctness | Existing oracle, ranges, texture samples, matrix probes, detailed verifier data |

Do not merely test `if (profile_level)` in inner loops. Use preprocessor guards so level 0 contains no counter load/store instructions.

Example:

```c
#if NDS_RENDERER_PROFILE_LEVEL >= 2
#define NDS_PROFILE_FORENSIC(stmt) do { stmt; } while (0)
#else
#define NDS_PROFILE_FORENSIC(stmt) do { } while (0)
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 1
#define NDS_PROFILE_COARSE(stmt) do { stmt; } while (0)
#else
#define NDS_PROFILE_COARSE(stmt) do { } while (0)
#endif
```

Keep a non-volatile frame-local profile struct. Publish only once at `ndsBattlePlayableFrameCompleteMarker()` to the small set of `volatile` symbols read by GDB.

### 5.2 Coarse profiler must be mutually intelligible

The current `DLTicks`, `TextureTicks`, `MaterialTicks`, and other timers overlap. Add either mutually exclusive zones or clearly label nested regions. A low-overhead level-1 profile should report:

* gameplay/update active ticks
* platform begin-frame ticks
* BG2 wallpaper clear/draw/commit ticks
* BG3 foreground clear/draw/commit ticks
* 320→256 scaler ticks, if any remains
* stage/fighter adapter setup ticks
* display-list validate/decode ticks
* CPU transform ticks or transform count
* perspective divide ticks or divide count
* light-cache refresh count and shade count
* GX matrix/state ticks
* GX vertex/FIFO emission ticks
* texture bind/convert/upload ticks
* flush/end-frame active ticks
* VBlank wait ticks
* HUD ticks

Useful work counters:

```text
s64_mul_terms
s64_div_calls
sqrt_calls
cpu_transformed_vertices
raw_current_triangles
raw_snapshot_triangles
projected_cross_matrix_triangles
projected_noz_triangles
projected_decal_triangles
projected_prim_triangles
projected_range_triangles
rejected_triangles
validated_commands
replayed_packets
matrix_generation_changes
matrix_loads
light_cache_refreshes
background_pixels_written
foreground_pixels_cleared
foreground_pixels_written
dma_bytes
stats_bytes_zeroed
stats_bytes_copied
```

Do not call `cpuGetTiming()` around each vertex, command, or triangle in the performance build. Count operations there; time whole phases.

### 5.3 Benchmark protocol

For every retained optimization:

1. Use the same mode, ROM inputs, camera state, HUD setting, emulator renderer/JIT setting, and verifier delay.
2. Record the ROM hash and profile level.
3. Ignore at least the first three warmed frames.
4. Capture at least 8–16 comparable complete frames.
5. Record median, p95, min, max, raw/fallback classifications, active CPU ticks, present ticks, and visual verifier result.
6. Run the baseline again after the candidate if environmental noise is suspected.
7. Do not claim wins below about 2% unless they are structurally obvious and repeatable.
8. Track code size and memory headroom. Current docs report about 237,948 bytes of headroom and a 128 KiB reserve; do not consume this casually.

The attached CSV template has the recommended columns.

---

## 6. Ordered implementation plan

The ordering matters. Do not begin with a full display-list compiler or broad compiler flags. Land the highest-confidence and highest-leverage cuts first.

---

# Phase A — Establish trustworthy low-overhead measurement

## A1. Reconcile the live tree

Follow Section 0. Identify which optimization patches the live agent already started after 4:00 p.m. Preserve them.

## A2. Add profile levels without changing renderer behavior

Implementation requirements:

* Level 0 removes per-command/per-triangle/per-vertex diagnostics at compile time.
* Level 1 keeps only coarse phase timers/counters.
* Level 2 retains all existing verifier/oracle observations.
* Existing GDB symbols may remain for compatibility, but level 0 should update only a compact set once per frame.
* Include the active profile level in the frame marker/verifier output so results cannot be confused.
* Add a static check or build log line proving which profile was compiled.

Suggested compact performance summary:

```c
typedef struct NDSRendererPerfFrame
{
    u32 profile_level;
    u32 frame_number;
    u32 active_ticks;
    u32 present_ticks;
    u32 vblank_wait_ticks;
    u32 hardware_triangles;
    u32 hardware_vertices;
    u32 submit_class_count[NDS_HW_SUBMIT_CLASS_COUNT];
    u32 matrix_loads;
    u32 texture_binds;
    u32 background_pixels;
    u32 foreground_pixels;
    u32 reject_mask;
} NDSRendererPerfFrame;
```

Use a normal static struct during the frame and one bounded copy to exported volatile storage at the frame-complete marker.

## A3. Capture a fresh baseline

The baseline must identify, at minimum:

* level-0 performance result
* level-1 coarse breakdown
* level-2 correctness/oracle result
* 828 triangles / 2484 vertices
* wallpaper cache key/build/hit state
* visual screenshot pass
* ROM parity/build identity

Do not proceed based only on the old 24,238,464-tick number if the live agent changed code after 4:00 p.m.

### Phase A acceptance

* Level 0 produces the same screenshot and source-state invariants as level 2.
* Level 0 has zero inner-loop oracle/profile work by disassembly or map inspection.
* Level 1 phase accounting is understandable; overlapping timers are explicitly marked or eliminated.
* Baseline median/p95 is saved to the benchmark ledger.

---

# Phase B — Low-risk hot-path removals

Each item should be an independent patch and benchmark.

## B1. Correct the no-oracle guard

Minimum patch shape:

```c
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (sNdsRendererHardwareNoOracle == 0u)
    {
        ndsRendererHardwareRecordOracleTriangle(state, i0, i1, i2);
    }
#endif
```

Also inspect every caller of `ndsRendererHardwareRecordOracleVertex/Triangle` and every transformed-triangle recording path. Ensure the canonical performance frame does not perform hidden oracle transforms.

Do not remove the level-2 oracle. It remains the correctness proof for raw GX.

## B2. Compile out hot forensic writes

Wrap these at level 2 or replace them with frame-local aggregate updates:

* raw/HW vertex min/max and saturation tracking
* texture coordinate min/max
* per-vertex texture sample classification
* combine-mode distinct tracking
* first-reject details
* light command/fallback detailed counters
* matrix element mirrors
* per-triangle oracle and projected depth ranges

At level 0, no code should touch the old volatile globals until the one frame-summary publication.

## B3. Cache normalized light direction on dirty state

Create a generation/key that changes when either:

* the relevant modelview/composed matrix changes, or
* the light direction changes.

First implementation must preserve the exact current arithmetic result:

```c
typedef struct NDSRendererLightCache
{
    u32 matrix_generation;
    s32 source_x;
    s32 source_y;
    s32 source_z;
    s8 normalized_x;
    s8 normalized_y;
    s8 normalized_z;
    u8 valid;
} NDSRendererLightCache;
```

On cache miss, run the existing float transform/`sqrtf`/divide once and store the resulting integer direction. On every lit vertex, perform only the dot product and color math.

After exact-result verification, an optional second patch can replace the one cache-miss float normalization with an integer square root. Do not mix that numerical change into the caching patch.

## B4. Replace exact-safe 64-bit texture-coordinate multiply

Current texture-coordinate scaling casts to 64-bit. With `s16 coord` and `u16 scale`, the product fits a signed 32-bit range for the observed formula. Use an exact signed 32-bit multiply and preserve signed shift/origin semantics. Add host fixtures for extremes:

```c
s32 scaled_t16 = ((s32)coord * (s32)scale) >> 17;
s32 origin_t16 = (s32)(origin << 2);
```

Verify negative coordinates and `0xffff` scale explicitly.

## B5. Replace matrix `memcmp` with generation IDs

Whenever projection/modelview/matrix-word state changes, increment a nonzero generation. GX matrix-load state records the last loaded generation and scale representation. The fast decision becomes integer equality, not two struct comparisons.

Be careful with wraparound: generation zero can mean invalid; on wrap, invalidate loaded state and restart at one.

## B6. Remove `-Os` from the performance A/B build

Do not change all build profiles at once. Create a measured mode-163 speed configuration:

1. baseline current flags
2. `-O2` without trailing `-Os`
3. optional targeted `-O3` after architectural cuts

Record ROM/code size and runtime. Keep the fastest verifier-clean option that fits memory. Do not enable global `-ffast-math`; renderer depth/matrix/animation code is sensitive to numerical behavior.

### Phase B acceptance

* Visual and source-state gates unchanged.
* Level-2 oracle still reports zero mismatch.
* Level-0 oracle sample work is zero/absent by design.
* Light normalization refresh count is proportional to matrix/light changes, not lit vertices.
* Matrix loads are keyed by generations.
* Every retained patch has a repeatable benchmark entry.

---

# Phase C — Repair and enable hybrid raw GX submission

This is the largest 3D optimization and the most correctness-sensitive phase.

## C1. Do not globally flip the fallback

A blind change from `TRUE` to `FALSE` previously produced missing fighters/platforms/background. The correct first cutover is **hybrid** and explicitly classified.

Add an enum similar to:

```c
typedef enum NDSRendererHWSubmitClass
{
    NDS_HW_SUBMIT_RAW_Z_CURRENT_MATRIX = 0,
    NDS_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX,
    NDS_HW_SUBMIT_PROJECTED_CROSS_MATRIX,
    NDS_HW_SUBMIT_PROJECTED_NO_Z,
    NDS_HW_SUBMIT_PROJECTED_DECAL,
    NDS_HW_SUBMIT_PROJECTED_PRIM_DEPTH,
    NDS_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX,
    NDS_HW_SUBMIT_REJECT,
    NDS_HW_SUBMIT_CLASS_COUNT
} NDSRendererHWSubmitClass;
```

The sum of all non-reject classes must equal submitted hardware triangles. Keep counters in level 0 because eight aggregate increments per triangle are cheap enough initially; later they can be scene/frame summary counters.

## C2. First raw eligibility rule: all three slots belong to current matrix

`current_transform_vertex_mask` is cleared when the composed matrix changes and marks vertices loaded under the current matrix. Use it. Do not rely only on `vertex_valid_mask`, because that mask includes vertices retained from older matrices.

Initial eligibility sketch:

```c
u32 tri_mask = (1u << i0) | (1u << i1) | (1u << i2);

bool raw_current =
    source_zbuffered &&
    !decal_depth &&
    !prim_depth &&
    state != NULL &&
    state->matrix_valid &&
    ((state->current_transform_vertex_mask & tri_mask) == tri_mask) &&
    ndsRendererHardwareRawCoordinatesFit(v0, v1, v2) &&
    ndsRendererHardwareRawMatrixCompatible(state);
```

For the first cutover:

* Raw-submit only `raw_current` triangles.
* Keep all no-Z, decal, primitive-depth, and cross-matrix triangles projected.
* Keep eager CPU transforms temporarily. This separates matrix/depth correctness from the later lazy-transform optimization.
* End the current GX batch before changing matrix generation/submission representation.

## C3. Fix the raw coordinate/matrix scale algebra

### Confirmed current behavior

`NDS_RENDERER_HW_WORLD_UNIT_SHIFT` is 8. A source coordinate is submitted as:

```text
v16_value = source_coordinate << (12 - 8)
```

Interpreted as DS 4.12 fixed point, this represents `source_coordinate / 256`. Let `S = 256`.

The current matrix conversion divides only modelview row 3, columns 0–2 by 256. It leaves the homogeneous component and projection translation row unscaled. That is not generally homogeneous-equivalent to the CPU matrix transform and is a strong explanation for the earlier raw-path visibility/depth failure.

### General combined-matrix solution

The CPU code uses row-vector semantics:

```text
clip = [x y z 1] * C
```

where `C` is the composed matrix. Raw GX receives:

```text
v_hw = [x/S, y/S, z/S, 1]
```

Choose a hardware matrix `C_hw` such that:

```text
rows 0..2 of C_hw = H * rows 0..2 of C
row 3 of C_hw     = (H/S) * row 3 of C
```

Then:

```text
v_hw * C_hw = (H/S) * ([x y z 1] * C)
```

The hardware clip vector differs only by one common homogeneous factor, so normalized X/W, Y/W, and Z/W are identical.

Start with `H = 1`. If row-3 precision is inadequate and rows 0–2 have fixed-point headroom, test a larger power-of-two `H` and prove no matrix overflow. Never select `H` by screenshot alone.

### Split-matrix alternative

For ordinary affine modelview matrices, another exact representation is:

* divide modelview translation X/Y/Z by S while retaining modelview `m[3][3] = 1`, and
* divide the **entire projection row 3** by S.

The current code performs only the first half. The combined-matrix path is preferable initially because it also handles matrix-word/composed state uniformly and reduces order/layout ambiguity.

### Saturation rule

With the current source-to-v16 shift, source coordinates beyond approximately `[-2048, 2047]` saturate. A raw triangle must fall back if any coordinate would clamp. Do not silently submit saturated geometry.

## C4. Prove matrix layout/order/scale with GX `PosTest`

The scale derivation above is a strong hypothesis, not permission to assume libnds matrix layout. Add a bounded level-2 matrix oracle that runs outside `glBegin`.

For a selected matrix generation:

1. End the current batch.
2. Load the proposed GX matrix representation.
3. Call `PosTest` on a small set of representative source vertices.
4. Read `PosTestXresult/Yresult/Zresult/Wresult`.
5. Compare against `ndsRendererTransformVertex20p12()`.
6. Prefer homogeneous cross-product checks so a common clip scale is accepted:

```text
hw_x * cpu_w == cpu_x * hw_w
hw_y * cpu_w == cpu_y * hw_w
hw_z * cpu_w == cpu_z * hw_w
```

Use 64-bit intermediates and a documented tolerance only for fixed-point quantization. Also compare W sign and clip-plane classification.

Probe set:

* identity matrix, origin and basis vectors
* positive/negative translation
* stage battle camera matrix
* Mario root matrix
* Mario child/joint matrix
* Fox root/child matrix
* a matrix-word update case
* vertices near raw v16 range limit
* positive and negative W cases
* near/far depth representatives

Run this only in profile level 2, preferably once per unique matrix signature or on a bounded first frame. The performance build must contain no `PosTest` oracle loop.

### Matrix failure decision tree

* Identity/basis probes fail: matrix layout/transposition or load order is wrong.
* Basis passes but translations fail: row-3/homogeneous scaling is wrong.
* X/Y pass but Z/W fail: projection row/depth representation is wrong.
* `PosTest` matches but scene geometry is absent: inspect poly state, clipping, matrix batch boundaries, and raw coordinate saturation.
* Scene appears but occlusion is wrong: inspect Z/W mapping and W-buffer semantics while leaving no-Z/decal paths projected.

## C5. Load one composed matrix generation, not two structs per triangle

For the first robust implementation:

* Build the corrected composed hardware matrix once per matrix generation.
* Load the other GX matrix as identity.
* Cache the loaded generation/scale-boost ID.
* Ensure all matrix loads occur outside `glBegin`/the active triangle batch.
* Include the matrix generation in the batch key; a matrix change is always a hard batch boundary.

Only return to split projection/modelview loading if a measured reason requires it.

## C6. Preserve projected exceptional classes

Do not optimize these simultaneously with the ordinary-Z cutover:

* source no-Z background/foreground painter ordering
* decal Z bias
* primitive depth
* incompatible/saturated matrices or coordinates
* cross-matrix cached triangles

The bundled `sm64-nds` renderer also uses raw vertices for ordinary Z-buffered geometry and special projected/`PosTest` handling for exceptional depth classes. Follow that architecture, not necessarily every implementation detail.

## C7. Add per-slot matrix snapshot IDs

After current-matrix raw submission is visually correct, recover more triangles without breaking the 32-slot persistent cache.

Extend the vertex cache with something like:

```c
u16 vertex_matrix_snapshot[NDS_RENDERER_MAX_VTX];
u16 vertex_clip_snapshot[NDS_RENDERER_MAX_VTX];
```

Maintain a small frame/draw-owned snapshot table:

```c
typedef struct NDSRendererMatrixSnapshot
{
    NDSRendererMatrix20p12 composed;
    m4x4 hardware;
    u32 generation;
    u16 raw_compatible;
    u16 scale_boost;
} NDSRendererMatrixSnapshot;
```

On VTX load, assign the current snapshot ID to each loaded slot. `G_MWO_POINT_ST` changes S/T only and must not change the matrix snapshot. A triangle can use `RAW_Z_SNAPSHOT_MATRIX` when all three slots share one valid snapshot ID. Mixed-snapshot triangles remain projected.

Use a bounded table and a fallback on overflow. Initial size 64 is likely inexpensive, but choose it from observed unique-matrix counts and memory ledger, not guesswork.

## C8. Make CPU transforms lazy

Only after raw current/snapshot submission is proven:

* VTX load stores source vertex and matrix snapshot ID.
* Do not compute clip X/Y/Z/W for raw-compatible vertices in level 0.
* When a projected class needs a slot, transform it using that slot’s snapshot matrix and cache the clip result.
* Invalidate clip cache only when the slot is overwritten or its position changes.
* `MODIFYVTX ST` does not invalidate clip position.
* Level-2 oracle may eagerly or lazily transform as needed, but performance level 0 should transform only projected exceptions.

This removes both 64-bit transform work and perspective division for the main raw path.

## C9. Reduce remaining projected divisions only after classification shrinks

Once ordinary source-Z geometry is raw, count the remaining projected vertices. If projection remains significant:

* remove the duplicate source-Z diagnostic Z/W divide in level 0
* compute output Z once and reuse it for diagnostics in level 2
* investigate an exact specialized signed 64-by-32 division helper or one reciprocal per vertex only with exhaustive fixtures

Do not risk depth drift for a division optimization before raw GX has removed the dominant class.

### Phase C acceptance

A successful first raw cutover must satisfy all of these:

* `raw_z_current > 0` and preferably covers most ordinary current-matrix source-Z triangles.
* Submission-class sum equals 828 triangles; rejects remain zero.
* No raw-coordinate saturation in accepted raw triangles.
* Level-2 `PosTest` matrix oracle has zero mismatches within documented fixed-point tolerance.
* Existing renderer oracle, texture, GX RAM, visual screenshot, depth/layer, flower/fence/fighter, and ROM parity gates remain green.
* The 44 known cross-joint cases remain correct; initially they may stay projected.
* `s64_div_calls` and `cpu_transformed_vertices` drop sharply in level 0.
* The raw path does not issue matrix loads inside a GX triangle batch.

---

# Phase D — Remove full-screen software 2D composition

This is required even if raw GX is extremely successful.

## D1. Instrument exact pixel traffic first

Add level-1 counters around:

* 320×240 staging clears
* wallpaper staging writes
* 320×240→256×192 scaler reads/writes
* BG2 VRAM copy bytes
* foreground staging clear/writes
* BG3 full clear bytes
* BG3 copy bytes
* wallpaper transform-key changes
* foreground dirty rectangles

The target is not merely lower function time. It is elimination of unnecessary pixel traffic.

## D2. Exact low-risk wallpaper cutover: render directly to final 256×192 BG2

Preserve the current two-stage nearest-neighbor result exactly before attempting affine hardware.

Current stage 1, wallpaper source lookup in 320×240 space:

```text
source = ((((relative + 1) << 16) - 1) / scale_q16)
```

Current stage 2, 320×240 to 256×192 center-nearest lookup:

```text
preview_x = floor((step_x/2 + screen_x*step_x) / 65536)
step_x = floor((320 << 16) / 256) = 81920

preview_y = floor((step_y/2 + screen_y*step_y) / 65536)
step_y = floor((240 << 16) / 192) = 81920
```

Compose these mappings directly:

```c
for (screen_x = 0; screen_x < 256; screen_x++)
{
    u32 preview_x = ((step_x >> 1) + screen_x * step_x) >> 16;

    if (preview_x < wallpaper_dst_start_x ||
        preview_x >= wallpaper_dst_end_x)
    {
        x_map[screen_x].valid = FALSE;
    }
    else
    {
        u32 relative = preview_x - origin_x;
        u32 source_x = ((((relative + 1u) << 16) - 1u) /
                        scale_x_q16);
        if (source_x >= source_width) source_x = source_width - 1u;
        x_map[screen_x].source = source_x;
        x_map[screen_x].valid = TRUE;
    }
}
```

Do the analogous Y map. Then write final BG2 pixels directly from the immutable decoded wallpaper cache. Handle pixels outside the source draw bounds exactly as the staging clear did.

This removes:

* the 320×240 background clear
* the 320×240 wallpaper staging writes
* the full 320→256 scaler pass
* the separate visible-row copy from compacted staging

## D3. Skip wallpaper redraw when its source key is unchanged

The decoded wallpaper is immutable, but its source GObj transform remains live. Cache only the final BG2 layer keyed by source state, not the whole gameplay frame.

Suggested key:

```text
asset_id
owner_scene
owner_generation
platform/decode-cache epoch
layout fingerprint
origin_x, origin_y
scale_x_q16, scale_y_q16
combine/alpha mode
final mapping version
```

If the key is unchanged and BG2 VRAM ownership/epoch is unchanged, perform zero wallpaper pixel writes that frame. This preserves source-controlled motion while making a static canonical background effectively free.

If the wallpaper covers every output pixel and is opaque, do not clear BG2 first. If coverage is partial, directly write zero only to uncovered strips/pixels or clear only the affected region.

## D4. Foreground BG3 dirty-region path

The current `ndsPlatformClearOriginalSpriteOverlayLayer(TRUE)` clears an entire 256×256 16-bit bitmap: 128 KiB every frame. Replace it with previous/current dirty rectangles.

Maintain:

```c
typedef struct NDSRect
{
    s16 x0, y0, x1, y1; /* half-open */
    u16 valid;
} NDSRect;

static NDSRect previous_foreground_rect;
static NDSRect current_foreground_rect;
```

For each visible foreground SObj, accumulate its clipped 320×240 bounds. Convert the union of previous and current intermediate bounds to the exact affected 256×192 output bounds using the same center-nearest map. Then:

1. Clear only the union in BG3/staging.
2. Redraw current SObjs clipped to the needed region.
3. Scale/copy only affected output rows/columns, or preferably draw directly to final-resolution BG3 using composed X/Y maps.
4. Save current as previous.

The union is necessary to erase pixels where a moving object used to be.

Do not assume the fence/HUD is static until counters prove it. If a foreground layer key is unchanged, reuse it. If only damage digits change, update their rect, not the entire BG.

## D5. Optional final hardware-affine wallpaper

Only if the exact final-resolution direct renderer is still too expensive, evaluate a one-time resampled 256×256 BG2 plus affine scroll/scale registers.

Risks:

* source wallpaper width is 300, larger than one 256-wide bitmap
* BG affine coefficients have limited fixed-point precision
* source transform and clipping equations must match existing visual output

Build a host fixture over every observed wallpaper origin/scale state. Accept only if sampling error is bounded and the screenshot/depth/layer gates remain unchanged. The exact direct mapper is a safer first landing.

### Phase D acceptance

* No 320×240 full clear or full scaler remains on the canonical hardware frame.
* Unchanged wallpaper key produces zero BG2 pixel writes.
* Changed wallpaper key writes at most final-resolution affected pixels, not both staging and final buffers.
* BG3 clear/copy bytes are proportional to dirty area, not 128 KiB every frame.
* Background remains behind 3D; fence/HUD remain in front according to source layer behavior.
* Screenshot and lifecycle/results scene gates remain green; do not optimize battle at the expense of results/opening overlays.

---

# Phase E — Re-profile, then compile/replay static display lists

Do not assume the current 18.36M “DL ticks” remain after Phases B–D. That timer currently includes transform, perspective divide, lighting, state work, and diagnostics. Re-profile first.

## E1. Separate runtime state from forensic statistics

`NDSRendererStats` is a large structure used both as renderer state and as a forensic report. The fighter path zeroes arrays, initializes stats, copies persistent state in/out, and accumulates per-part reports for many selected parts every frame.

For performance level 0, create a compact hot runtime state containing only fields required to execute GBI/RDP state:

```text
geometry mode
othermode high/low
combine state
colors/alpha/depth
texture image/tile/load state
light state
current matrices/generations
RSP vertex cache
resolver context
batch state
```

Keep detailed counters/reports in level 2. Avoid full `NDSRendererStats` zero/copy per part in level 0. Preserve state carry across selected lists exactly.

A less invasive first step is a profile-level branch:

* level 2 retains per-list `stats[i]` and accumulation
* level 0 uses one persistent runtime state and only compact frame counters

## E2. Cache validated/decoded topology, not a composed frame

Safe cache target: immutable display-list command topology and resolved static references. Unsafe target: final stage/fighter triangles after dynamic matrix/material/animation state.

Suggested packet operations:

```text
PKT_SET_GEOMETRY
PKT_SET_OTHERMODE_H
PKT_SET_OTHERMODE_L
PKT_SET_COMBINE
PKT_SET_COLOR
PKT_SET_TEXTURE_STATE
PKT_SET_TILE
PKT_LOAD_BLOCK_OR_TILE
PKT_BIND_TEXTURE_KEY
PKT_LOAD_MATRIX_REF
PKT_LOAD_VTX_REF
PKT_MODIFY_VTX_ST
PKT_DRAW_TRI_BATCH
PKT_CALL
PKT_JUMP
PKT_RETURN
PKT_END
```

Packets should preserve original source ordering and bind dynamic state at replay time.

## E3. Cache key and invalidation

At minimum, key compiled packets by:

* asset ID
* owner scene/generation
* loaded base/range
* display-list offset, not an unstable raw pointer alone
* texture data layout
* material segment generation/provenance
* branch resolver provenance
* data resolver provenance
* display-contract/static signature
* packet compiler version

Invalidate on any owner generation/range/provenance change. On any uncertain command or resolver result, fall back to the existing interpreter and count the reason.

## E4. Preserve these semantics exactly

* 32-slot RSP vertex cache persists across selected lists.
* Vertex slots retain their matrix snapshot at VTX load.
* `G_MWO_POINT_ST` changes source S/T for the specified slot.
* Branch call versus jump and return behavior remains exact.
* Matrix-word updates and matrix ownership remain exact.
* Segment E material table resolution remains source-backed.
* TEXEL1/water animation reload keys remain dynamic.
* no-Z phase transition remains in source order.
* TRI batching stops on every non-TRI source opcode unless the packet compiler proves an equivalent state boundary.

## E5. Arena and memory budget

Use a fixed scene-owned arena. Do not add per-frame heap allocation. Start with a measured 32–64 KiB budget and report:

* packets compiled
* packet bytes
* cache hits/misses
* fallback reason mask
* peak arena usage
* invalidations

Current documented headroom is not permission to use all free memory; preserve the 128 KiB reserve and audio/scene margins.

### Phase E acceptance

* Warm-frame packet hit rate is near 100% for immutable stage/fighter list topology.
* Dynamic animations, material changes, water/TEXEL1 refreshes, matrices, and source transforms remain live.
* Fallback is safe and reason-coded.
* Per-frame command validation/recursive decode count collapses on cache hits.
* Visual, source-state, lifecycle, Boundary, and ROM parity gates remain green.

---

# Phase F — Targeted compiler and memory-placement tuning

Only after architecture is sound:

## F1. Optimization flags

Measure these independently:

* mode 163 `-O2` without trailing `-Os`
* targeted `-O3` for renderer/adapter translation units
* ARM state rather than Thumb for only the hottest translation unit/functions
* LTO if supported cleanly by the current devkitARM setup

Inspect generated code or map/disassembly for critical helpers. Do not assume a flag helps.

## F2. ITCM/DTCM

The bundled `sm64-nds` renderer places its display-list interpreter, VTX/TRI handlers, and hot state in ITCM/DTCM. Follow this precedent only after measuring map capacity.

Likely candidates after the cutovers:

* packet replay loop
* raw vertex batch emission
* compact runtime state
* matrix/light cache state

Do not put large textures, packet arenas, or forensic stats in tightly limited TCM. Confirm linker placement and overflow from the map.

## F3. Arithmetic

After raw GX and light caching:

* convert remaining exact-safe s64 multiplies to s32 only with range proof
* hoist invariant divisions out of loops
* precompute constant screen mapping arrays
* use DMA for contiguous rows/rectangles where alignment and size justify it
* avoid `%` in texture/sample hot paths by using power-of-two masks when source tile semantics prove it

Do not introduce approximate reciprocal/depth math without exhaustive fixtures and device visual/depth proof.

---

## 7. Verifier migration required by optimization

The current canonical verifier expects positive oracle samples and a positive projected-fallback count. Those assertions become invalid for a profile-0 raw-GX performance frame. Do not weaken correctness; split responsibilities.

### 7.1 Diagnostic correctness run, profile level 2

Must prove:

* CPU renderer oracle samples > 0
* oracle mismatch = 0
* oracle max delta = 0
* raw-GX `PosTest` homogeneous comparisons pass
* texture conversion/sample/state invariants
* matrix/light cache refresh correctness
* submission-class accounting
* screenshot/depth/layer invariants

### 7.2 Performance/canonical run, profile level 0

Must prove:

* active profile is 0
* hardware triangle total = 828 for the canonical anchor
* submission-class sum = triangle total
* raw ordinary-Z count > 0 and projected exceptional counts are reason-specific
* reject count = 0
* raw saturation count = 0
* texture cache health and dynamic TEXEL1 refresh remain valid
* no forbidden full-screen 2D work after Phase D
* ROM/build parity
* realtime pacing

The performance run should not require oracle samples. Instead it requires a recent verifier-covered profile-2 matrix/oracle run from the same source revision/build configuration family.

### 7.3 Existing verifier commands

Iteration:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
```

Architecture slice completion:

```powershell
.\scripts\verify-p1-gate.ps1 -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

Realtime target:

```powershell
.\scripts\verify-battle-playable-realtime-harness.ps1 `
  -RequireRealtime60Fps `
  -SkipScreenshot
```

Also run the normal screenshot version so speed is not accepted with a visual regression.

Final milestone verification should include relevant RegressionCore/full regression according to `AGENTS.md` and `docs/VERIFYING.md`. Use the five-minute soak for milestones, not every micro-edit.

### 7.4 Static verifier assertions to add

Useful assertions after the cutovers:

```text
profile_level == expected
sum(submit_classes) == hardware_triangles
raw_current + raw_snapshot > 0
reject == 0
raw_saturation == 0
matrix_oracle_mismatch == 0          # profile 2
full_background_clears == 0          # profile 0 after Phase D
full_foreground_clears == 0          # canonical after Phase D
wallpaper_writes == 0 when key stable
packet_fallback_reason == 0 on known canonical lists
active_ticks_p95 <= engineering budget
pacing_fps_x10 in [593, 603]
```

---

## 8. Recommended patch/commit sequence

This sequence minimizes the chance of conflating correctness failures.

### Patch 1 — Profile-level infrastructure

Files likely touched:

* `Makefile`
* generated build-config emission
* `include/nds/nds_startup.h` or a new narrow profile header
* `src/port/taskman_seam.c`
* `src/port/diagnostics.c`
* verifier scripts

Behavior must remain identical. Produce fresh level-0/1/2 baselines.

### Patch 2 — No-oracle guard and forensic compile-out

* Guard `ndsRendererHardwareRecordOracleTriangle`.
* Remove per-vertex/profile volatile writes from level 0.
* Keep level-2 exact diagnostics.

### Patch 3 — Exact dirty light cache

* Same rendered integer light output.
* Cache refresh keyed by matrix/light generation.
* Add refresh/shade counters.

### Patch 4 — Matrix generation IDs and raw matrix `PosTest` fixture

* No production raw cutover yet.
* Replace matrix memcmp path where safe.
* Build corrected composed raw matrix candidate.
* Prove layout/order/scale with device `PosTest` and host fixtures.

### Patch 5 — Hybrid raw current-matrix source-Z

* Add submit enum/counters.
* Raw only when all three slots are current, compatible, and in range.
* Keep eager transforms and all exceptional/cross-matrix paths projected.

### Patch 6 — Per-slot matrix snapshots and lazy transforms

* Raw-submit same-snapshot cached triangles.
* Mixed snapshots projected.
* Transform only projected exceptions in profile 0.

### Patch 7 — Exact direct final-resolution wallpaper

* Preserve current two-stage nearest-neighbor output exactly.
* Cache by source transform key.
* Skip unchanged BG2 writes.

### Patch 8 — Foreground dirty-region composition

* Remove full 256×256 clear.
* Previous/current union.
* Exact output mapping and layer ordering.

### Patch 9 — Re-profile and compact runtime state

* Split hot renderer state from forensic stats.
* Remove per-part full zero/copy in profile 0.

### Patch 10 — Validated display-list packet cache

* Static topology only.
* Fixed arena and complete invalidation/fallback diagnostics.

### Patch 11 — Compiler/TCM tuning

* Remove `-Os` for speed build.
* Measure targeted O3/ARM/ITCM/LTO variants.

Do not combine Patches 4–8 into one large branch. Every cutover needs its own visual and performance proof.

---

## 9. Stop/go gates and milestones

These are engineering stage gates, not guaranteed speed predictions.

| Milestone | Required evidence |
|---|---|
| M0: trustworthy baseline | level-0/1/2 results; same visuals; low-overhead counters |
| M1: diagnostics removed | no hidden oracle/profile inner-loop work; measurable speedup |
| M2: raw GX active | ordinary-Z raw count dominant; matrix oracle clean; major divide reduction |
| M3: 2D cutover | no full 320×240 clear/scale; static wallpaper writes zero; dirty BG3 |
| M4: under 30 FPS budget | present/active work near or below 1,117,133 ticks without visual drift |
| M5: packet/runtime-state cut | warm decode/validation nearly gone; active work below 744,755 ticks |
| M6: realtime | strict 59.3–60.3 FPS gate; active p95 with margin; screenshots and regressions green |

Useful tick checkpoints:

```text
12,000,000 ticks  ≈ 2.79 FPS
 5,585,664 ticks  = 6 FPS
 2,792,832 ticks  = 12 FPS
 1,117,133 ticks  = 30 FPS
   744,755 ticks  = 45 FPS
   609,345 ticks  = 55 FPS
   558,566 ticks  = 60 FPS wall budget
```

If a supposedly architectural patch moves only 1–2%, stop and confirm it actually removed the intended operation through counters/disassembly.

---

## 10. What not to optimize now

Do not spend the next work block on:

* more TRI begin/end batching tweaks; the last one improved draw by only 0.8%
* warm texture conversion/upload; terminal conversion/upload is already zero
* caching the final composed stage/fighter frame
* rewriting gameplay or replacing imported BattleShip behavior
* deleting the persistent 32-slot RSP cache
* blindly forcing all geometry to raw GX
* changing no-Z/decal/primitive-depth behavior during the ordinary-Z cutover
* global `-ffast-math`
* broad approximate matrix/depth arithmetic
* running full regression after every arithmetic micro-patch
* optimizing VBlank/update paths before level-1 profiling shows they matter
* adding more permanent one-bit proof markers

---

## 11. Troubleshooting map

### Raw count remains zero

Check, in order:

1. classification happens before `zbuffered` is forced false for projected submission
2. `current_transform_vertex_mask` is populated on VTX loads
3. matrix validity/generation is not reset too aggressively
4. coordinate range check is not rejecting everything
5. matrix probe compatibility flag is set after a successful proof
6. decal/prim-depth detection is not accidentally true for ordinary geometry

### Raw count is high but objects disappear

1. Run identity/basis/translation `PosTest` probes.
2. Compare W sign and clip planes, not only X/Y.
3. Verify entire homogeneous row scaling.
4. Confirm matrix layout and multiplication order.
5. Confirm matrix loads end the batch first.
6. Confirm raw vertex source slots all share the loaded matrix snapshot.
7. Check v16 saturation.
8. Preserve current W-buffer flush mode.

### Geometry appears but depth is wrong

1. Leave no-Z/decal/prim classes projected.
2. Verify raw Z/W from `PosTest` against CPU clip Z/W.
3. Check projection row 3 scaling and `H` boost.
4. Check whether `GL_WBUFFERING` expects the same W sign/range.
5. Confirm the source geometry mode/Z mode at classification time.

### Raw GX is much faster but frame is still below 10 FPS

That is expected if the software 2D compositor still clears/scales/copies full screens. Inspect pixel and DMA counters immediately.

### Direct wallpaper is visually shifted by one pixel

Compare the exact two-stage formulas. Common errors:

* omitting the scaler half-step (`step/2`)
* using floor of `relative<<16 / scale` instead of the existing “last source” formula
* applying final-screen origin instead of 320×240 preview-space origin
* clamping before checking source draw bounds
* changing signed origin behavior

### Foreground trails remain

Clear the union of previous and current dirty rectangles, not only current. Ensure the dirty rect is converted through the exact output mapping and expanded for any filter/sample footprint.

### Packet cache breaks animation

The cache captured dynamic state. Remove matrices, material values, texture image keys, TEXEL1 fraction, source transforms, or branch resolver results from baked packet payloads; keep references/operations and bind current state during replay.

---

## 12. Source locations to inspect first

| Area | Snapshot location | Why |
|---|---|---|
| Frame profiling/reset | `src/port/taskman_seam.c:4205-4315` | dozens of volatile resets and total frame sequence |
| Profile globals | `src/port/diagnostics.c:2811+` | volatile exported hot counters |
| CPU transform | `src/nds/nds_renderer.c:526-555` | 12 s64 multiply terms per transform |
| Matrix composition | `src/nds/nds_renderer.c:1149-1185` | matrix order and current-slot mask invalidation |
| Vertex load/cache | `src/nds/nds_renderer.c:1600-1693` | slot transform ownership |
| MODIFYVTX ST | `src/nds/nds_renderer.c:1696-1734` | packet/cache semantics |
| Raw coordinate conversion | `src/nds/nds_renderer.c:1800-1823` | source÷256 representation and saturation |
| Oracle transform | `src/nds/nds_renderer.c:1893-1944` | repeated diagnostic transform |
| Lighting | `src/nds/nds_renderer.c:2628-2725` | per-vertex normalize/sqrt/divide |
| GX matrix load | `src/nds/nds_renderer.c:4908-5003` | incomplete world scaling and memcmp cache |
| Projected divide | `src/nds/nds_renderer.c:5029-5068` | X/Y/Z by W |
| Forced fallback | `src/nds/nds_renderer.c:5133-5138` | ordinary-Z raw path disabled |
| Vertex submission | `src/nds/nds_renderer.c:5141-5258` | raw/projected branch and duplicate depth divide |
| Triangle submission | `src/nds/nds_renderer.c:5286-5527` | classification/oracle/batching |
| DL interpreter | `src/nds/nds_renderer.c:5531+` | validation/decode/recursive state |
| Fighter adapter | `src/port/reloc_backend_renderer_dl.c:6701-6852` | repeated zero/copy/setup/execute per part |
| Wallpaper cache draw | `src/port/sprite_preview_backend.c:467-522` | exact source lookup formula |
| SObj frame split | `src/port/sprite_preview_backend.c:1140-1196` | background commit and second full staging clear |
| Preview clear/scale/commit | `src/nds/nds_platform.c:243-494` | full-screen software 2D traffic |
| Mode flags | `Makefile:96-102, 420-439` | Thumb/O2 base and trailing `-Os` |
| Raw GX precedent | `decomp/sm64-nds/src/nds/nds_renderer.c:305-455` | raw ordinary-Z, projected exceptions |
| Dirty lighting precedent | `decomp/sm64-nds/src/nds/nds_renderer.c:458-515` | normalize only when dirty |

Line numbers may move in the live tree. Search by function name after reconciling.

---

## 13. Codex completion checklist

Before calling an optimization slice complete:

- [ ] Reconciled all post-4:00 p.m. in-flight changes without overwriting them.
- [ ] Recorded source revision, build flags, profile level, ROM hash, median, and p95.
- [ ] Change is source-backed by BattleShip and/or `sm64-nds` where architecture is involved.
- [ ] `decomp/` remains unchanged.
- [ ] No final composed gameplay frame was cached.
- [ ] Dynamic matrices/materials/textures/wallpaper transforms remain live.
- [ ] 828 canonical triangles and required visual elements remain.
- [ ] Submission classes sum exactly; reject/saturation are zero.
- [ ] Level-2 oracle/matrix probes remain clean.
- [ ] Level-0 contains no forensic inner-loop work.
- [ ] Full-screen software 2D operations are absent after Phase D.
- [ ] Benchmark improvement is repeatable and not emulator noise.
- [ ] `verify-dev-fast.ps1 -Build -DelaySeconds 3` passes during iteration.
- [ ] P1Gate and Boundary pass at architecture milestones.
- [ ] Strict realtime and normal screenshot runs both pass at final target.
- [ ] Relevant lifecycle/regression/soak gates pass.
- [ ] `docs/STATUS.md` and `docs/HANDOFF.md` remain concise and accurate.
- [ ] Static checks pass.
- [ ] Lean snapshot is the last command, with no command after it.

---

## 14. Bottom line for the agent

Do not try to “optimize 24 million ticks” one loop at a time. Remove whole classes of work:

1. Remove forensic work from the performance build.
2. Prove and enable corrected raw GX for ordinary source-Z geometry.
3. Make CPU transforms/projective divides lazy exceptions.
4. Stop rebuilding full-screen 2D layers every frame.
5. Re-profile, then cache static display-list topology and compact the runtime state.
6. Only then use compiler/TCM tuning to close the final gap.

The first decisive technical proof is the raw matrix `PosTest` oracle. The first decisive system proof is that both raw-GX and 2D composition counters collapse. The final proof is not a profiler estimate: it is the strict 60 FPS harness plus unchanged visual/depth/source-state and regression gates.

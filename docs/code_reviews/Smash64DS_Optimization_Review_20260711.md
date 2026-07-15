Optimization package

I analyzed the snapshot through 4:00:30 p.m. CDT. Because the live agent continued working through approximately 4:20 p.m., every document begins with a mandatory reconciliation step: inspect the live diff, preserve all in-flight changes, and skip anything that has already landed.

Smash64DS_Optimization_Playbook_20260711.md — detailed diagnosis, implementation phases, pseudocode, matrix derivation, profiler design, 2D compositor cutover, packet-cache design, verifier changes, troubleshooting, and acceptance gates.
Smash64DS_Codex_Master_Prompt_20260711.txt — a concentrated instruction set designed to hand directly to the active Codex agent.
Smash64DS_Optimization_Benchmark_Template.csv — 70 fields covering median/p95 timing, submission classes, transforms, divides, pixel traffic, cache behavior, memory, and verifier results.
Most important conclusions
1. This needs architectural removal, not ordinary optimization

The latest snapshot measurement is:

24,238,464 ticks/frame
723.23 ms/frame
1.383 FPS

The 60 FPS budget is:

558,566 ticks/frame
16.67 ms/frame

That is a 43.4× gap. Approximately 97.70% of current frame time must disappear.

The reported display-list region is 18,364,480 ticks. Even if that entire region became free, the remaining present cost would still be around 5.87M ticks—only about 5.7 FPS. This proves that both major sides of the frame must change:

CPU projection/display-list rendering.
Full-screen software SObj composition.

Optimizing only one cannot reach 60 FPS.

2. Ordinary Z-buffered geometry is deliberately forced through the slow path

src/nds/nds_renderer.c:5133-5138 currently returns projected fallback for every transformed Z-buffered triangle.

That means the normal raw-GX branch already present in the source is effectively unreachable for the main stage/fighter geometry. Each ordinary projected vertex performs CPU clip-space work and multiple software 64-bit divisions.

For up to 2484 submitted vertices, the current path performs on the order of thousands of signed 64-bit divisions per frame—potentially close to 10,000 when the duplicate diagnostic Z/W calculation is included. The ARM9 has no hardware integer divider.

The main 3D cutover should therefore be:

ordinary source-Z + compatible shared matrix
    -> corrected GX matrix + raw object vertex

no-Z / decal / primitive depth / mixed-matrix cache case
    -> retain projected fallback

It must be a classified hybrid path, not a global fallback toggle.

3. “No oracle” still performs oracle transforms

The canonical frame enables ndsRendererHardwareSetNoOracle(TRUE), but ndsRendererSubmitHardwareTriangle() still unconditionally invokes:

ndsRendererHardwareRecordOracleTriangle(state, i0, i1, i2);

That helper re-transforms three vertices and compares X/Y/Z/W. The canonical oracle count of 2403/0/0 is evidence that this diagnostic work still runs.

The first low-risk patch is to compile it out of the performance build and retain it only in the forensic profile:

#if NDS_RENDERER_PROFILE_LEVEL >= 2
if (sNdsRendererHardwareNoOracle == 0u)
{
    ndsRendererHardwareRecordOracleTriangle(state, i0, i1, i2);
}
#endif

The playbook also calls for three explicit profile levels:

0 = performance/shipped
1 = coarse phase profiling
2 = full forensic/oracle diagnostics

Level 0 should contain no per-command, per-triangle, or per-vertex volatile diagnostic writes. It should publish only a compact summary once at the frame-complete marker.

4. The likely raw-GX matrix failure is now much more specific

The current raw path submits each source coordinate divided by 256 in DS 4.12 form. However, the matrix conversion scales only modelview translation X/Y/Z by 256. It does not scale the homogeneous and projection terms consistently.

For the CPU row-vector transform:

clip = [x y z 1] × C

and raw hardware input:

v_hw = [x/S y/S z/S 1], S = 256

a homogeneous-equivalent combined GX matrix is:

rows 0–2 of C_hw = H × rows 0–2 of C
row 3 of C_hw    = (H/S) × row 3 of C

Then:

v_hw × C_hw = (H/S) × CPU_clip

X/W, Y/W, and Z/W are preserved.

The safest first implementation is to load the corrected composed matrix into one GX matrix and load identity into the other. Start with H = 1; use a larger power-of-two boost only if fixed-point precision requires it and matrix ranges prove it safe.

This is still a hypothesis until verified against the actual GX matrix conventions. The playbook requires a bounded profile-level-2 PosTest oracle that tests:

identity and basis vectors
translation
battle camera
Mario and Fox root/child matrices
matrix-word cases
near/far depth
positive and negative W
raw-coordinate range edges

It should compare homogeneous cross-products rather than expecting identical clip scaling:

hw_x × cpu_w == cpu_x × hw_w
hw_y × cpu_w == cpu_y × hw_w
hw_z × cpu_w == cpu_z × hw_w

That distinguishes layout, order, scale, W-sign, and depth failures without screenshot guessing.

5. The 32-slot vertex cache makes a blind raw path incorrect

The project intentionally preserves the RSP vertex cache across selected display lists. Documentation records 44 cross-joint triangles that reuse vertices loaded under earlier matrices.

vertex_valid_mask alone is therefore insufficient for raw submission. A raw triangle is initially safe only when all three slots are in current_transform_vertex_mask.

After that path works, each vertex slot should receive a matrix snapshot ID. Three vertices sharing one snapshot can be raw-submitted under that snapshot. Mixed-snapshot triangles remain projected.

Only after both forms are verified should CPU transforms become lazy:

raw-compatible vertex:
    no CPU clip transform

projected exception:
    transform using the vertex slot’s saved matrix snapshot
    cache the result

This preserves cross-list and cross-joint behavior while removing most transform and division work.

6. Lighting repeats expensive invariant work per vertex

ndsRendererHardwareLitDiffuseNumer() currently transforms and normalizes the same directional light for every lit vertex, including sqrtf and floating division.

The bundled sm64-nds implementation normalizes only when matrix/light state becomes dirty. The first patch should cache the current exact float-derived integer direction, keyed by matrix generation and light-state generation. That avoids visual differences while eliminating repeated square roots and divisions.

Integer normalization can be a later, separately verified patch.

7. The SObj compositor is a separate hard blocker

The hardware path currently performs approximately this sequence every frame:

clear 320×240 staging
draw 300×220 wallpaper into staging
scale 320×240 -> 256×192
copy to BG2
begin and clear another 320×240 foreground staging image
draw foreground SObjs
scale/copy foreground
clear entire 256×256 BG3 with 128 KiB DMA fill

The recommended low-risk cutover preserves the current sampling result exactly but renders directly to final 256×192 BG2 pixels.

It composes the existing mappings:

preview_x =
    ((step_x / 2) + screen_x × step_x) >> 16

step_x =
    (320 << 16) / 256
    = 81920

source_x =
    ((((preview_x - origin_x + 1) << 16) - 1) / scale_x_q16)

The same applies to Y.

The final BG2 result should then be cached using the live source state:

asset and scene generation
decode-cache epoch
origin
scale
combine mode
mapping version

When that key is unchanged, the wallpaper should write zero pixels that frame.

Foreground BG3 should track previous and current dirty rectangles, clear their union, and update only affected output pixels. The full 128 KiB clear must disappear.

8. Display-list packets come after raw GX and 2D

The current 18.36M DL timer includes much more than command decoding: transforms, divisions, lighting, state updates, diagnostics, and GX submission are nested inside it.

After the first two cutovers, the agent must re-profile. If command validation and adapter setup remain hot, compile immutable display-list topology into fixed-arena packets while binding dynamic state during replay.

The cache must not bake:

current matrices
fighter poses
material values
wallpaper transforms
animated TEXEL1 state
final transformed triangles

It must preserve:

32-slot cache state
per-slot matrix ownership
G_MWO_POINT_ST
branch call/jump behavior
segment-E material resolution
dynamic texture keys
source no-Z transitions
exact command ordering
9. Compiler tuning is real but secondary

Mode 163 appends -Os after the base -O2. The agent should A/B an -O2 performance build without the trailing size optimization.

Targeted -O3, ARM-state hot translation units, LTO, and ITCM/DTCM placement may help close the final gap, but they should follow the architectural cuts. Global -ffast-math is specifically excluded because matrix, depth, animation, and material behavior are numerically sensitive.

Recommended execution order

The package instructs the agent to proceed in this order:

Reconcile everything changed after 4:00 p.m.
Add profile levels and capture fresh level-0/1/2 baselines.
Remove hidden oracle and volatile forensic work from level 0.
Cache light normalization and replace exact-safe arithmetic.
Add matrix generations and the GX PosTest proof.
Enable hybrid raw submission for current-matrix ordinary-Z triangles.
Add per-slot matrix snapshots and lazy CPU transforms.
Replace full-screen wallpaper composition and add BG3 dirty regions.
Re-profile.
Split compact runtime state from forensic stats and add packet replay only if still justified.
Tune compiler/TCM placement.
Pass both the strict realtime harness and normal screenshot/regression gates.

The most important instruction for the agent is: do not accept a speedup merely because the screenshot looks plausible, and do not accept visual correctness while the intended expensive operation still appears in counters or disassembly.
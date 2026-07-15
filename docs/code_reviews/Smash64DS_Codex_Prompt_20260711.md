You are optimizing the current live Smash64DS_Port working tree from roughly 4:20 p.m. CDT on July 11, 2026. A separate analysis was made from a 4:00 p.m. snapshot, so first reconcile the live tree. Never overwrite, reset, revert, or duplicate in-flight changes.

MISSION
Reach verifier-clean 60 FPS in the battle_playable realtime Nintendo DS build while preserving original BattleShip behavior, current Dream Land/Mario/Fox visuals, depth/layer ordering, texture/TEXEL1 behavior, lifecycle/results behavior, ROM parity, Boundary/P1 gates, and source ownership. This is an optimization phase, not a gameplay rewrite.

HARD RULES
- Read and obey AGENTS.md.
- Treat decomp/ as read-only.
- Inspect relevant BattleShip source and bundled decomp/sm64-nds before backend architecture changes.
- Keep backend work in src/nds or src/port and narrow declarations in include/.
- Use apply_patch.
- Do not revert user/agent work.
- One measurable hypothesis per patch.
- Do not cache a composed gameplay frame.
- Do not change no-Z/decal/primitive-depth semantics in the first ordinary-Z raw-GX cutover.
- Keep the persistent 32-slot RSP vertex cache and cross-list state semantics.
- Run New-Smash64DSSnapshot.ps1 -Mode Lean only after all successful final checks, as the final project command, and run no command afterward.

FIRST ACTIONS — NO EDITS YET
1. Read AGENTS.md, docs/HANDOFF.md, docs/STATUS.md, docs/KNOWN_ISSUES.md, docs/HW_RENDERER_VISIBILITY_FINDINGS.md, docs/VERIFYING.md, and relevant verifier scripts.
2. Run git status --short, git diff --stat, and focused diffs for Makefile, nds_renderer, taskman_seam, diagnostics, nds_platform, sprite_preview_backend, reloc_backend_renderer_dl, realtime/canonical verifier scripts, and active docs.
3. Identify all changes after 2026-07-11 16:00:30 and preserve them.
4. Run verify-all.ps1 -Profile Boundary -List to confirm current authority.
5. If any task below already landed, validate it against its acceptance criteria and continue to the next unmet task.

BASELINE/BUDGET
Snapshot baseline was 24,238,464 BUS_CLOCK ticks, about 723.23 ms or 1.383 FPS. BUS_CLOCK is about 33,513,982; 60 FPS is 558,566 ticks/frame. Approximately 97.7% of current frame work must disappear. The old DL timer was 18,364,480 ticks; even deleting it entirely leaves roughly 5.87M present ticks, so both 3D submission and software 2D composition require architectural cutovers.

ORDERED PLAN

A. TRUSTWORTHY PROFILE LEVELS
Add NDS_RENDERER_PROFILE_LEVEL 0/1/2 to generated build config:
- 0 performance/shipped: compile out all per-command/per-triangle/per-vertex forensic diagnostics; publish only a compact frame summary once at frame-complete marker.
- 1 coarse: whole-phase timers/counters, not timing every vertex/command.
- 2 forensic: existing oracle/range/texture diagnostics plus bounded GX matrix PosTest probes.
Keep existing GDB symbols if needed, but do not write volatile globals in hot loops at level 0. Measure active CPU ticks separately from VBlank wait. Capture level-0/1/2 baseline medians and p95 after at least 3 warm frames and 8–16 samples.

B. LOW-RISK REMOVALS
1. Guard/compile out ndsRendererHardwareRecordOracleTriangle in no-oracle/profile-0 frames. It is currently called unconditionally in ndsRendererSubmitHardwareTriangle and repeats transforms despite canonical no-oracle mode.
2. Compile out vertex range, texture sample, combine distinct, projected-depth range, matrix mirror, and similar volatile writes at profile 0.
3. Cache the current exact float-normalized light direction by matrix/light generation. First preserve exact current integer output; integerize later only if measured.
4. Replace exact-safe texture-coordinate s64 multiply with s32 plus fixtures for signed extremes.
5. Replace per-triangle projection/modelview memcmp matrix cache with generation/snapshot IDs.
6. A/B mode 163 without trailing -Os. Do not use global fast-math.
Benchmark each separately.

C. HYBRID RAW GX — MAIN 3D CUTOVER
Do not blindly flip the projected fallback. Add submit classes:
RAW_Z_CURRENT_MATRIX, RAW_Z_SNAPSHOT_MATRIX, PROJECTED_CROSS_MATRIX, PROJECTED_NO_Z, PROJECTED_DECAL, PROJECTED_PRIM_DEPTH, PROJECTED_RANGE_OR_MATRIX, REJECT.
The class sum must equal hardware triangles (828 canonical) and reject must be zero.

Initial raw eligibility requires source Z, not decal, not prim depth, valid matrix, all three vertex slots in current_transform_vertex_mask, no raw v16 saturation, and a matrix representation that passed the diagnostic oracle. Cross-matrix/no-Z/decal/prim remain projected. Keep eager CPU transforms in the first cutover to isolate correctness.

RAW MATRIX ROOT-CAUSE HYPOTHESIS TO PROVE
Source coordinates are submitted as source/256 in DS 4.12. Current code divides only modelview translation xyz by 256; it does not scale homogeneous/projection terms consistently. For CPU row-vector clip C and S=256, a combined GX matrix can preserve NDC by:
- rows 0..2 = H * C rows 0..2
- row 3 = (H/S) * C row 3
Then raw hardware clip = H/S times CPU clip. Start H=1; increase only with range proof.
An affine split alternative is modelview translation xyz/S plus entire projection row 3/S. The current implementation is only half of this.

Build a profile-2 PosTest oracle outside glBegin. Probe identity/basis/translation, battle camera, Mario/Fox root/child, matrix-word, near/far, positive/negative W, and range-edge vertices. Compare homogeneous cross-products hw_x*cpu_w vs cpu_x*hw_w (and y/z), W sign, and clip classification. Diagnose layout/order/scale rather than screenshot-guessing. Load a corrected composed matrix generation with the other GX matrix identity initially. Matrix changes are hard batch boundaries.

After current-matrix raw works, add per-slot matrix snapshot IDs. A triangle whose three slots share one snapshot can load that snapshot and raw-submit. Mixed snapshots remain projected. Preserve G_MWO_POINT_ST S/T changes and cross-list 32-slot cache behavior. Then make CPU clip transforms lazy: only projected exceptions transform/divide.

D. REMOVE FULL-SCREEN SOFTWARE 2D
Instrument pixels/bytes first. Current canonical path clears/draws a 320x240 background staging frame, scales it to 256x192, copies it to BG2, begins/clears another 320x240 foreground staging frame, and clears a full 256x256 BG3 (128 KiB) every frame.

First land an exact final-resolution wallpaper renderer that composes the existing two nearest-neighbor mappings:
preview_x = ((step_x/2) + screen_x*step_x) >> 16, step_x=(320<<16)/256=81920;
source_x = ((((preview_x-origin_x)+1)<<16)-1)/scale_x_q16;
and analogous Y. Write final BG2 directly from the immutable decoded cache. Cache final BG2 by asset/generation/cache epoch/origin/scale/combine/mapping version; if the source key is unchanged, write zero wallpaper pixels. Do not clear an opaque full-coverage wallpaper.

For foreground BG3, track previous/current dirty rectangles, clear their union, redraw/copy only affected output pixels, and preserve layer order. Eliminate the full 256x256 clear. Use hardware affine only after the exact direct mapper is verified and if still necessary.

E. RE-PROFILE, THEN PACKET CACHE
After raw GX and 2D cutovers, remeasure. If decode/adapter is still hot, split compact runtime renderer state from the huge forensic NDSRendererStats. At profile 0 do not zero/copy full per-list stats arrays.
Compile immutable display-list topology once into a fixed scene-owned packet arena, while binding matrices/materials/textures/TEXEL1/source transforms dynamically during replay. Key by asset ID, scene/generation, DL offset/range, texture layout, material/branch/data resolver provenance, display-contract signature, and compiler version. Preserve branch call/jump, 32-slot cache, per-slot matrix snapshots, MODIFYVTX ST, segment E material resolution, dynamic TEXEL1, source ordering, and no-Z transition. Safe fallback with reason counters. Do not cache final triangles/frame.

F. TARGETED FINAL TUNING
Only after architecture: measure O2 vs targeted O3, ARM state for hot TU/functions, LTO, and ITCM/DTCM placement based on map capacity. No global fast-math or approximate depth math.

VERIFICATION SPLIT
Profile 2 correctness run: positive oracle samples, zero mismatch/max delta, zero PosTest matrix mismatch, texture/GX/depth/screenshot invariants.
Profile 0 performance run: profile marker 0, 828 triangles, submit-class sum exact, raw count positive/dominant, reject/saturation zero, texture cache/TEXEL1 healthy, no forbidden full-screen 2D work, ROM parity, pacing.
Do not require oracle samples in the profile-0 speed run; require the separate profile-2 proof from the same source revision/config family.

ITERATION COMMAND
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3

ARCHITECTURE MILESTONES
Run P1Gate and Boundary plus canonical screenshot/parity. At final target run:
.\scripts\verify-battle-playable-realtime-harness.ps1 -RequireRealtime60Fps -SkipScreenshot
and the normal screenshot run, then required lifecycle/regression/soak/static/doc checks.

DO NOT PRIORITIZE
More TRI batching, warm texture conversion/upload, final-frame caching, gameplay rewrite, deleting cross-list RSP cache, blind all-raw toggle, global fast-math, or full regression after every micro-edit.

REPORT AFTER EACH PATCH
- exact hypothesis
- files/functions changed
- profile level and build flags
- before/after median and p95 active/present ticks
- operation counters removed (divides/transforms/pixels/validation/etc.)
- raw/fallback class counts
- code size and memory impact
- verifier and screenshot result
- next bottleneck from fresh measurements

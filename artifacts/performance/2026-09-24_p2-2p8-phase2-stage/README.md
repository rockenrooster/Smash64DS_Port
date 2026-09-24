# P2-2p8 Phase 2: four-fighter stage execution

Owner priority (2026-09-24): at least 95% of work targets four concurrent VS
fighters at stable 30 FPS. Campaign work is deferred; pre-stage intros use
static images. The uncommitted live-Intro experiment was archived and reverted.
No subagents. Preserve existing owner deletions and hardware-reference edits.

State: IMPLEMENTED_NOT_ACCEPTED (segment 5/7 bring-up), based on `98ebd1e2e51`.
Specification: `artifacts/performance/2026-09-23_p2-2p8-phase-specs/phase2-spec.md`
and architecture A1 / Phase 2. Work from the measured executor costs, not a new
profiling pass or a generic rendering abstraction.

Retained baseline: `builds/build-p2p8-s7/smash64ds-p2-fourcpu-tickhud-hwtri.nds`,
SHA-256 `50B8AF5804D12C984E09CB276212E297E3CD1944519274E22B45831CE2A509C8`.
Slice-7 receipt/rows remain authoritative: WORK-H P50/P95/P99
1,393,664 / 2,021,027 / 2,665,534 ticks, 19.21 FPS, 10.59% two-VBlank;
STG P50/P95/P99 324,288 / 368,925 / 374,611. Native failures 39 remain open.
These are one roster on Dream Land, not all-roster or shipping-memory acceptance.

Next: extend the compiler to Dream Land segments 0/4, then 1/2/3/6; bake
immutable inputs and remove the repeated prepare/submit machinery. Then VS stages.
Retain exact material/order/visibility and live painter-depth/near-plane behavior.
No source edits under a build; Makefile owns parallelism. Use focused checks
during implementation, one relevant four-CPU qualification at the batch boundary.

Owed: integrated timing/cadence, native output, shipping heavy-roster memory,
all-stage/roster coverage, removal of replaced machinery and final integration.
Deferred 1P obligations do not authorize dropping existing content or claiming
universal retirement. Root ROM remains r54. Job: none at this checkpoint.

Current producer inventory (generated in memory, no files rewritten): Dream Land
has 202 triangles, 54 runs, 42 bindings across eight segments. Segments 5/7
contain 17/28 projected-no-Z triangles and already have Task36 capture/replay:
use their captured words as the first compiled-program oracle, as spec C/S1
requires. Then cover segments 0/4 and 1/2/3/6 before the batch timing decision.
The descriptor implementation is a package:
`scripts/stages/native_stage_descriptors/__init__.py`. The old segment-0
generator resolves native run inputs but does not emit the replacement GX stream.

Rollback check: all five live-Intro source/tool files match `98ebd1e2e51` again;
the obsolete camera test is removed from scripts. Documentation checks pass.
No further campaign work is queued.

First batch: `compile_nds_stage_gx.py` emits self-contained native GX runs for
segments 5/7, with explicit view/world, current painter-depth, colour and UV
patches. The runtime loads the checked NitroFS payload into the scene arena;
`gNdsP2StageProg=1` arms it for the same-ROM A/B. Default remains 0 during
bring-up. No performance claim yet; the existing prepare/submit machinery is
still present until all classes are covered. Host decode/corner/stack and
negative-input check: `python -m pytest scripts/stages/test_stage_gx.py -q`
passed (1 test).

The retained slice-7 ROM/ELF/map/config are now copied and hash-checked under
`builds/p2p8-phase2-control-s7/`; see `baseline-identity.json` and
`baseline-nitrofs.csv`. Reuse `builds/build-p2p8-s7` for the candidate.
Switching from the deferred all-content build requires the incoming particle
producer first. Explicit producer dependencies now cover its three indirect
consumers, whose GCC `D:/` dependency paths differed from Make's `/d/` paths.
Producer preflight passed with Yoster=0; the first `D:/` target spelling was a
no-op, and the `/d/` producer spelling generated the correct output/stamp.
The candidate config hash matches the preserved baseline exactly. Template:
1,834 GX words, 334 patches, 10,656 B mutable body (10,704 B on disk).
The first build job was 23994 (`build-seg57.log`); scoped probes use
`tools/run-stage.ps1` (96 samples in the bring-up pair).

First build failed on duplicate `SYMallocRegion` declarations from the added
taskman include, and a misnamed painter-depth helper. Removed the redundant
include (the renderer already declares the allocator) and use the existing
`ndsRendererHardwareNextProjectedDepth`. Rebuild only the affected inputs;
the failed build left the baseline ROM unchanged. GDB Python is unavailable;
the GX oracle uses ordinary binary memory dumps instead.
The first incremental link then exposed 848 B ITCM overflow: GCC inlined the
patcher into the old hot commit function (+1,288 B; baseline had 440 B spare).
Keep the new patcher out of line during coexistence. This is an implementation
failure, not runtime evidence; the retained ROM remains the control.

Segment 5/7 r2 build passed, including the native-only link guard:
ROM `D4AEFF8E9F0E7143BAD024BE3D4B6C500B88CB3708A65808B3BEA21B97C63F77`.
ITCM has 128 B spare. NitroFS differs from slice 7 only by the 10,704 B
`stages/dreamland.gxp` file (`seg57-nitrofs.csv`). Default route remains 0.

Early bring-up probe (`seg57-on.json`, 96 samples) loaded once, emitted
679 runs / 177,898 words across 97 presents, with zero declines, native
failures or fighter packet faults in that window. General heap low-water was
122,332 B, arena 1,253,120 B. This entry/countdown-heavy window is NOT four-fighter
performance acceptance; retain the whole-match slice-7 performance baseline.

Captured-GX oracle (`seg57-gx-oracle-r1-comparison.json`): seven runs, all
135 corners / 45 triangles match the old runtime capture exactly in positions,
colours and textured UVs. New path GXSTAT `0x06000000`, no stack error.
View/projection and painter-depth changes still need the live-camera visual gate.
The first dump attempt failed because this Windows GDB treats quoted dump
filenames literally; unquoted workspace paths fixed the harness, with a new
label preserving the failed attempt. No ROM inputs changed between these probes.

Same-ROM early control (`seg57-off.json`): STG P50/P95 319,872 / 373,504;
compiled 369,216 / 372,608 ticks. WORK-H P50/P95 780,416 / 1,312,256 versus
818,816 / 1,361,920. Mean-ALL FPS 28.03 versus 27.90; both VBI histograms
2/3/4/5+ = 91/4/0/2 (97 presents), max 9 versus 10. This prototype is an
enabling dependency, **not a banked performance win**. Full projection/world and
colour/UV patching remains expensive; reduce this while extending the batch.
Replay pairs are identical in order over all 96 rows (`seg57-digest.json`).

Live-camera captures at match tics 3300 and 3298 match pixel-for-pixel, with
all four CPU fighters in battle. Images and the measured viewport are documented
in `artifacts/visibility/2026-09-24_p2-2p8-stage-gx/README.md`.
Preserved candidate: `builds/p2p8-stage-seg57-r2/` (ROM/ELF/map/config).
Input hashes: `seg57-inputs.json`. All build/emulator jobs are terminal.
Owed: remaining Dream Land segments, all VS stages, full-match and shipping
memory gates, old executor retirement and integration. Phase acceptance is RED.

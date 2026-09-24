# P2-2p8 Phase 2: four-fighter stage execution

Owner priority (2026-09-24): at least 95% of work targets four concurrent VS
fighters at stable 30 FPS. Campaign work is deferred; pre-stage intros use
static images. The uncommitted live-Intro experiment was archived and reverted.
No subagents. Preserve existing owner deletions and hardware-reference edits.

State: IMPLEMENTED_NOT_ACCEPTED (full Dream Land geometry/attributes), based
on `98ebd1e2e51` with the first bring-up committed as `e019998977b`.
Specification: `artifacts/performance/2026-09-23_p2-2p8-phase-specs/phase2-spec.md`
and architecture A1 / Phase 2. Work from the measured executor costs, not a new
profiling pass or a generic rendering abstraction.

Retained baseline: `builds/build-p2p8-s7/smash64ds-p2-fourcpu-tickhud-hwtri.nds`,
SHA-256 `50B8AF5804D12C984E09CB276212E297E3CD1944519274E22B45831CE2A509C8`.
Slice-7 receipt/rows remain authoritative: WORK-H P50/P95/P99
1,393,664 / 2,021,027 / 2,665,534 ticks, 19.21 FPS, 10.59% two-VBlank;
STG P50/P95/P99 324,288 / 368,925 / 374,611. Native failures 39 remain open.
These are one roster on Dream Land, not all-roster or shipping-memory acceptance.

Next: replace the remaining stage preparation/material executor, physically
remove obsolete replay storage/code, and extend compiled data across VS stages.
MISC draw-list replacement and full integration remain part of this phase.
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

Next batch in progress after pushed checkpoint `e019998977b`: segments 0/4/5/7,
175 triangles, raw-Z/range and no-Z programs. Format v2 bakes known-constant
worlds, validates them against the live stage at load, and carries run bounds.
All 12 world matrices in the retained seg57 capture exactly matched the source
baker before removing their patches. Camera no-Z runs use a conservative live
near-plane bound; uncertain runs refresh near flags before the existing clipper.
The real C bound helper passed 256 host cases including s32 saturation and the
eye plane; the 175-triangle decoder/negative-input check passed too (2 tests).
Host payload: 34,992 B, 5,990 words, 1,211 patches. Colour/UV baking, the four
actor segments and executor retirement remain unfinished. Next: frozen build,
then a focused entry/battle probe with the new `gNdsP2StageProgNearRuns` counter.
Layers build passed native-only: ROM
`55D6EA4F8A054AAE83CE2BE5ABD466A52C6176131AC4D43E1E657FA09BB24D8B`.
Running `tools/run-stage.ps1 -Arm layers-on -Route 1 -StartFrame 350 -Samples 192`
to sample actual four-fighter combat; cumulative counters retain entry coverage.
That probe completed: 21,099 draws / 3,240,590 words through 541 presents,
0 declines, 0 near runs, 0 native failures; heap minimum 91,296 B. Its combat
window STG P50/P95 was 380,032 / 385,216, WORK-H 1,390,720 / 1,971,136 ticks;
mean-ALL FPS 18.83. Cadence 164/314/55/8, max 10, is cumulative from boot,
not just the 192-row combat window. No performance acceptance claim.

The layers capture differed at 771 of 120,000 game pixels (0.6425%) from the
retained old-path tic-3300 reference. The requested side-by-side and pixel mask
are `layers-side-by-side.png` / `layers-pixel-diff.png` in the visibility folder.
The captured-matrix check found a maximum 1.415567 DS pixels over 297 formerly
CPU-composed corners, failing the 1-pixel gate. Snapshot at stage finish entry:
`layers-matrices-r1-*`. The first attempt at the frame-complete marker read
already-cleared pointers; its data is invalid, retained under `layers-matrices-*`.
The checker models integer GX operation order from upstream melonDS GPU3D.cpp;
it proves the captured inputs only. Pre-fix ROM/ELF/config/template are preserved
under `builds/p2p8-stage-layers-before-precision/`.

Precision repair implemented, not yet built: non-rigid no-Z runs retain the
existing composed matrix, patched into projection with an identity modelview.
This preserves the source composition/rounding boundary while keeping compiled
vertex submission. Z columns now patch directly in the DMA buffer. The compiled
consumer also selects the normal prepared-texture certificate, rather than the
unpopulated replay-table certificate. Host checks pass; frozen build 8278 is
running (`build-layers-precision.log`), inputs unchanged until it exits. Next:
capture/check the matrices before stage cleanup and repeat the matched-pixel
gate. Actor segments and attribute baking remain next. The owner's requested
pre-fix side-by-side and magenta pixel diff were generated and displayed.
Precision build 8278 completed successfully: ROM
`98BFB92CC900AEBF60B74974A9243FA93BF9171FC3E03640BF966DD626E04C42`,
native-only link guard passed. Capturing `layers-precision-matrices` at the
same stage-finish seam and match tic before repeating the visual comparison.
That matrix gate passed: max 0.004198721 DS pixels over 297 formerly CPU-composed
corners. Rendered difference fell to 35 / 120,000 pixels (0.0292%, maximum
channel delta 49), consistent with the remaining source-Z matrix rounding;
the explicit <=1-pixel matrix gate is satisfied at this camera. The owner said
"looks good" to the shown Dream Land comparison on 09-24. This is approval of
the shown view, not all stages/camera states. Preserve the checked image under
`builds/p2p8-stage-layers-precision/` (including its v2 template).

Full Dream Land batch now implemented: all 202 triangles / eight segments.
Format v3 adds per-run binding masks for conservative cross-binding bounds.
Cross triangles patch each corner's composed projection and share exactly one
painter-depth allocation per triangle. Existing near clipping stays available.
Known colours and explicit static UVs are baked from source state spans, with
first-use semantics matching the prepared-dense producer. Material/implicit UVs
stay live. In the retained 175-triangle capture, all 525 colour and 489 UV words
chosen for baking matched exactly: 1,014 runtime patches removed. The full-stage
host payload is 31,732 B / 6,979 words / 255 patches. Three host tests pass,
including the real C near-bound and colour/texel selectors; next frozen build.
Full-stage build completed (93516), native-only guard passed. ROM
`EAB96F041F2F02B361E8B9465F6DCB7894D0694EC30C09E2ACF1987F1A0A120C`.
Running `full-dreamland-matrices` to check projected positions and shared
per-triangle painter depth, especially the new cross-binding programs.
Full-stage matrix/depth check passed: 378 formerly CPU-composed corners, max
0.004198721 DS pixels; all 126 no-Z triangle depth sequences correct, including
the 10 cross-binding triangles. All 54 runs drew on each of 346 presents
(18,684 total), with no declines/near runs or GX stack error at tic 3300.
Now measuring the frozen complete Dream Land geometry/attribute batch with
`full-dreamland-on/off`, 1,972 samples from frame 2 on this same ROM. This is the
first full-match comparison for the completed stage-program batch; MISC,
remaining stage preparation/retirement and the other VS stages still remain.
The full comparison finished: all 1,972 ordered replay pairs match; all
106,542 compiled draws engaged, with 0 declines/near runs. Both arms retain
the 39 known native failures. `full-dreamland-summary.json` records linear
P50/P95/P99: WORK-H 1,403,200 / 2,034,726 / 2,664,134 off versus
1,420,288 / 2,038,003 / 2,702,051 on. FPS 19.10 versus 19.01; stage median
328,448 versus 344,768. No banked win. Heap minima 122,412 versus 93,540 B.
Two hardware clipping-count differences remain to explain (ordered rows 1155
and 1495; gameplay digests match), so full output closure remains owed.

Root cause found in the remaining preparation: the legacy replay mask still
reserved segments 5/7, excluding them from the R2 preflight elision, even though
compiled GX suppresses their actual captures. The shared replay-mask producer
now returns zero for the compiled route. This turns off capture, replay and
their unnecessary preflight together; the normal prepared table remains the
texture/material authority during this transition. Physical code/storage
retirement is still due. Preserve the measured ROM/ELF/config/template under
`builds/p2p8-stage-full-before-preflight-retirement/`. Next frozen build and
full-match measurement with preparation/capture engagement counters.
Replay-preflight removal build passed: ROM
`50C0A91CE47FF773C633A09501584791B85EE06DCBEAE3E9D6682F4C1CC12D7B`.
Before timing, extend the matrix/depth check to the two differing hardware
clip-count frames (1157 and 1497). The vertex programs did not change in this
build; the captured source matrices will test the remaining rounding explanation.
The two differences are now explained by screen-plane crossings under the
bounded matrix rounding. Match by gameplay digest, not the sampler's inferred
frame label: `clip-digest-matrices-a/b-comparison.json`. At the first state,
run 37 corners 17/20/22/25 move from just outside the right plane to inside;
at the second, run 39 corner 4 and run 40 corner 0 cross the lower plane.
Maximum projected errors are 0.00730232 / 0.01319101 DS pixels, with no winding
flips and all 126 no-Z depth sequences correct. These changes explain the
hardware clipping-count delta within the explicit <=1-pixel matrix contract.
The before-barrier counter-read timing was investigated but is NOT the cause:
at the exact digests the GPU was idle and counts did not change on a status
read. Earlier frame-number probes were one source sample later; keep their
results as neighboring-camera coverage only.

Now running `preflight-retired-on`, full 1,972 samples, on `50C0A91CE47FF773`.
The named timing invalidator is the shared replay-mask repair. Reuse the retained
full-dreamland off/on controls; this changes neither the GX payload nor geometry.
Capture/preflight counters must prove the obsolete work stopped. Physical
retirement of its code/storage, other VS stages, MISC and shipping gates remain.

Measured checkpoint, 2026-09-24 (`preflight-retired-summary.json`):
WORK-H P50/P95/P99 1,401,952 / 2,020,816 / 2,689,292 ticks; STG
324,032 / 331,805 / 334,272; FTR 184,224 / 268,150 / 774,282;
MISC 199,232 / 400,352 / 517,118. Mean-ALL FPS 19.17; 207/1,973 presents
in two VBlanks (10.49%); 2/3/4/5+ histogram 207/1,386/326/54, max 11.
All 1,972 replay pairs match the retained same-input control. Engagement:
106,542 program draws, 1 initial prepare, 1,972 reuses, 15,776 preflight
elisions, 0 replay capture words/outcome, 0 program declines/near routes.
Native failures remain the same 39 as control; direct rejects/packet faults 0.
Heap low-water 93,540 B, lab arena 1,253,120 B. The linked lab's static image
is +5,312 B versus slice 7 (text +5,280, BSS +32); shipping CSS's old 5,296 B
margin must be remeasured after retirement, before promotion. Do not substitute
the lab arena for shipping's smaller arena or claim the heavy roster now fits.

Compared with the retained native control, stage P95 is lower by 40,797 ticks
(10.95%), but WORK-H P95 falls only 13,910 (0.68%) and median 1,248 ticks.
Compared with the compiled version still doing replay preflight, this repair
removes 20,736 median STG / 18,336 median WORK-H ticks. This is an enabling
checkpoint, not phase acceptance or a claim of meaningful overall FPS progress.
The remaining preparation and MISC machinery must still be replaced.

Code added/retired: compiled all 202 source triangles and baked immutable
attributes; per-frame native vertex emission and replay preparation no longer
execute on the compiled route. Physical legacy code/storage remains linked for
the temporary control and other stages, explicitly still owed. Current source
and ELF/config/template hashes: `full-stage-inputs.json`. Root ROM remains r54.

Other VS-stage input census was generated without writing assets: Yoster
19 bindings/4 segments/58 runs/164 triangles; Castle 12/4/40/136; Jungle
30/4/72/182; Zebes 26/2/56/151 (3 alpha-ramp triangles); Sector 19/2/70/299;
Hyrule 15/3/75/206; Yamabuki 21/4/92/243; Inishie 24/7/65/176, with source-Z
cross runs 50/52 (4 triangles). These require explicit compiler coverage;
do not assume Dream Land's no-Z cross case covers Inishie's source-Z case.

Final image acquisition exposed two distinct issues, kept separate:
1. `full-stage-t3300.png` was desktop wallpaper, not the emulator. It is invalid
   evidence. The exact-frame helper only copied a screen rectangle after a long
   wait. `Save-ExactFrameWindowCapture` now uses `PrintWindow` on the known HWND
   (software-renderer path) and fails if that capture fails. Valid replacement:
   `full-stage-r1-*`; matched controls use the same method.
2. The valid full-stage comparison then showed 1,398 differing pixels around
   the foreground/water. The legacy dispatch tested CROSS before submit class,
   routed Dream Land's five no-Z cross runs through the source-Z emitter and
   switched painter bands early. The source generator's geometry/othermode
   classification says no-Z; `EmitNoZTriangle` already handles foreign bindings.
   The dispatch now restricts source-Z cross emission to non-no-Z classes,
   preserving Inishie's source-Z handling. This is a consumer/reference repair,
   not permission to change the compiled path's source depth behavior.

Reference-repair ROM `12A5031B9D55C3E9B97886FCD74BB5E1EF59EDA94E38F5EC5C9A4F214BA294DD`
built native-only. Its control image (`source-depth-control-t3300.png`) differs
from the full compiled image by just 2 / 120,000 pixels (0.0017%, max channel
delta 16). The earlier mismatching water region is resolved. The subsequent
comment-only source edit occurred after build completion, verified by timestamps.
`source-depth-off/on` now collect the corrected same-ROM reference and candidate
over 1,972 samples each. Earlier timing controls are historical and must not be
presented as the corrected renderer's performance baseline. Full natural shipping,
other-stage and sibling acceptance remains owed; no bug is marked FIXED here.

Corrected final pair completed (`source-depth-summary.json`), both on `12A5031B9D55C3E9`:
control WORK-H P50/P95/P99 1,397,152 / 2,023,600 / 2,643,889 versus compiled
1,401,888 / 2,035,286 / 2,670,957 ticks. Stage 317,760 / 364,701 / 369,445
versus 325,120 / 333,056 / 335,251. FTR 184,352 / 268,358 / 773,990 versus
184,192 / 268,118 / 770,455; MISC 198,816 / 399,773 / 517,884 versus
198,784 / 399,680 / 518,483. FPS 19.175 versus 19.147. Cadence off/on:
211/1,378/329/55 max 9 versus 208/1,375/334/56 max 10 (1,973 presents each).
Replay is identical over all 1,972 ordered pairs. Program engagement and heap
are unchanged from the preceding candidate: all 106,542 draws, 0 declines,
0 near routes, 93,540 B heap minimum, 15,776 preflight elisions, 0 captures.
Both arms still have 39 native failures. Four host checks pass, including host
execution of the real triangle dispatch for no-Z and source-Z cross bindings.

**No overall performance win is banked.** Stage P95 improves 8.68%, but median
stage work and total work increase slightly; the full replacement of preparation,
material setup, per-run submission and retained caches remains necessary. Earlier
headline deltas against the faulty legacy reference are superseded by this pair.
The two tiny hardware clipping-count differences remain the same and are bounded
by the captured screen-plane rounding analysis. Source input/ELF/config/template
identity for this final pair is recorded separately in `corrected-pair-inputs.json`.
`source-depth-inputs.json` identifies the preceding 50C0 build; the corrected
12A5 ROM/ELF are preserved in `builds/p2p8-stage-corrected-reference/`. The one
post-build source change only corrected the cross-emitter comment.
All runtime/build jobs are terminal. Root ROM still hashes to r54; no publication.

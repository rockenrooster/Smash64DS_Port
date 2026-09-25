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

Material/submission replacement starts from pushed checkpoint `497825f14c5`.
GXP v4 inserts native polygon/texture/palette operands, resolved directly from
admitted libnds texture objects. The material binder no longer executes on each
compiled run. Adjacent visible programs with equal alpha state share one DMA;
hidden runs, alpha changes and cold clipping split the span. The existing DMA0
fence owns submission. Five host checks pass, including real-C material lookup
and queue boundary checks. The libnds archive's actual palette getter was
inspected: FORMAT_EXT returns gl_palette_data.addr, not a readable GX register.
Named invalidator: changed material execution and submission scheduling. Reuse
the preserved 12A5 controls; qualify the frozen candidate's output and timing.
The receipt's earlier in-place patch hit a Windows mapped-file restriction;
writing through an open handle succeeded. No evidence or owner file was lost.
Material/submission candidate built native-only (246 inputs):
`02DA58513F8C7B72ACDC60CF3E838F52A9A63C334B93D39E4834225A07515D7D`.
Payload 33,028 B, 7,195 words, 309 patches, all 202 triangles. Full four-CPU
`material-batch-on` now running, 1,972 samples, slot 9/GDB 3423. Source frozen;
replay/timing/output and matching capture remain owed.
`material-batch-on` completed: all 1,972 replay pairs and every GPOL/GVTX pair
match the preceding compiled control. 106,542 program runs use 53,271 DMAs
(27/frame rather than 54). Native failures remain 39, declines/near routes 0.
WORK-H P50/P95/P99 1,388,096 / 2,020,896 / 2,645,330; STG 316,992 / 324,928 /
327,123; FTR 185,344 / 269,760 / 782,912; MISC 200,064 / 402,016 / 519,170.
Cadence 223/1,366/329/55, max 10, 1,973 presents. Heap 88,148 B, lab arena
1,249,024 B. Source-depth image differences at tics 3300/3298 are 2/8 pixels;
the first view is unchanged from the preceding compiled image.
Review found one old binder side effect still needed by retained cache consumers:
copy the prepared sampler word to texture_entry.params. The direct GX word was
already correct; preserve that metadata write too. This is the explicit
invalidator for the final rebuild/run. Preserve 02DA as the pre-repair candidate
in builds/p2p8-stage-material-batch-before-entry-state. No ROM is published.
Final material batch built native-only: 49263A2E789B4059081F60F46DE1EA1BEC5E2CAA9E4BF54F49D96A8CDF515018. Full material-final-on running; source and payload identity: material-final-inputs.json.

Final material/submission checkpoint (`material-final-summary.json`):
ROM `49263A2E789B4059081F60F46DE1EA1BEC5E2CAA9E4BF54F49D96A8CDF515018`.
WORK-H P50/P95/P99 1,391,936 / 2,026,259 / 2,656,071 ticks; STG 316,864 /
325,184 / 327,315; FTR 184,800 / 269,645 / 779,412; MISC 199,264 / 400,826 /
519,379. Quantiles use linear interpolation over all 1,972 samples. Mean-ALL
FPS 19.197; 215/1,973 two-VBlank presents (10.897%); 2/3/4/5+ histogram
215/1,380/321/57, max 10, slips 0. Replay and GPOL/GVTX match the preceding
compiled control on every sample. All 106,542 runs submit with 53,271 DMAs;
0 program declines/near routes, 0 packet faults/declines/direct rejects, same
39 native failures. Heap low-water 88,148 B, lab arena 1,249,024 B.

Compared with the preceding compiled path: WORK-H median -9,952, P95 -9,027;
STG median -8,256, P95 -7,872. Compared with the corrected native reference,
WORK-H P95 is +2,659 ticks and FPS differs only 0.02. **No overall win banked.**
The final static image adds 1,920 B (text 1,888, BSS 32) to 12A5; the arena
steps down 4,096 B, plus the GX payload grows 1,296 B. Shipping CSS and heavy
roster must be remeasured after retirement; these lab margins do not prove fit.
The old per-run material binder and per-run DMA wait no longer execute on the
compiled route. Their legacy code, replay storage and the temporary route still
exist: remove those next, with hard-on qualification, then finish preparation,
other VS-stage and MISC coverage. This remains IMPLEMENTED_NOT_ACCEPTED.
Final captures inspected: 2/8 changed pixels at tics 3300/3298, same as the preliminary material batch; see visibility material-final-pixels.json. ROM/ELF/config/template preserved in builds/p2p8-stage-material-final. All jobs terminal. Next: remove Task36 replay storage/hooks and qualify compiled Dream Land hard-on; then remaining preparation, all VS stages and MISC. Shipping root remains r54.

Task36 retirement, from `3eefca0fd42` (2026-09-24): the capture/replay owner,
recorder hooks, per-run replay consumer, copied texture certificate, routing
controls and obsolete replay diagnostics are removed. Compiled Dream Land has
no gNdsP2StageProg on/off variable. Cold native clipping and not-yet-compiled
stages retain their existing native emitters. The buffer alone was 24,256 B
in the preserved baseline ELF; linked size/arena recovery still requires build.
The native-only guard now rejects retired symbols and was checked against the
old ELF: it finds the recorder, replay run, buffer, capture flag and route flag.
The stress and renderer benchmark consumers now require compiled-stage loads,
draws and DMA activity instead of the removed replay counters.
26 host tests pass (GX/compiler checks plus native-only guard tests); five changed
PowerShell files parse. The stage producer's only packet-byte change is deletion
of the obsolete replay-bound #define; its descriptor/checker SHA pin was updated
from c28c1e4c to b3833549 after an inspected diff. Source geometry is unchanged.
The first one-use patch producer stopped on an unmatched comment anchor before
editing source; corrected and applied through apply_patch. Stage checking exposed
both stale include-hash pins; the producer and checker now agree. No old evidence
was relabelled. Full stage check, native-only build, hard-on run/capture, and
shipping CSS/heavy-roster heap proofs are due; this is IMPLEMENTED_NOT_ACCEPTED.
Stage check now passes all 12 fail-closed perturbations and blob round-trip. Remaining link-layout and benchmark consumers of the deleted replay leaf were updated. The GBI check identified the old compiler-load hook ahead of the texture-proof anchor; load/admission now follows the whole-table proof and the optional mesh branch. GBI recheck running; runtime sources are frozen for replay-retired-build.
Retirement build passed (native-only, 246 actual inputs):
0A14A9F95D133F4B49922E148FDACD9207DEA404861DA064D330FA75A3B81C78.
Static delta against 49263A2E: text -11,328 B, data 0, BSS -24,324 B; total
-35,652 B. All retired symbols are absent. GXP payload is byte-identical:
6BAB61773CC324C89F4D5D6EFCB492827E8AE695F7D05F524CF858520C541942.
GBI fixtures now pass in full after updating the obsolete bulk-replay painter
assertion to the compiled per-triangle depth contract (already host/runtime
checked). Sources frozen. Full replay-retired four-CPU run is starting; actual
arena/heap/FPS/output and shipping CSS/heavy-roster measurements remain due.
Hard-on lab run completed: all 1,972 replay and geometry-count pairs match; both image crops are pixel-identical to 49263A2E. Heap minimum 120,916 B and arena 1,281,792 B (+32,768 B each), mean-ALL 19.413 FPS. Lab preserved in builds/p2p8-stage-replay-retired. Actual shipping/freeplay control DB66E237 is preserved in builds/p2p8-stage-retirement-shipping-control. Incoming Yoster=1 particle producer preflight precedes the one freeplay build; no double-build workaround and no root publication.
Actual shipping freeplay built native-only (318 inputs), 0879E5500C8622A1555E7E88257CC97F5012ADE10D5574EABA47F77A48CF2247. Menu walk/fast logic/tick HUD are all 0. Its static image is 36,860 B smaller than the preserved slice-7 freeplay control (text -12,600, BSS -24,260). The producer preflight was a no-op; the actual build generated particles at line 41422 before all three consumers at 41694/41705/41709, so no second make was needed. Natural CSS reservation has 221,136 B free, 38,064 B over the 183,072 B requirement, fail=0. Link/Yoshi/Pikachu preview counters and inspected captures are valid. The first CSS pictures were desktop pixels despite successful CopyFromScreen; the shared running-window capture helper now uses HWND PrintWindow directly. Retaken r1 images are valid; invalid originals are not committed. No ROM publication.
Shipping CSS proof is complete: 0879E550 is preserved under builds/p2p8-stage-replay-retired-shipping. Next a diagnostic sibling of the same all-content freeplay configuration adds only menu walk=1 and argmax roster=1 to drive Captain/Link/Pikachu/Kirby through the normal shell allocation path. This is a capacity probe, not shipping timing or a publication candidate. Source/runtime inputs frozen for that build.
Hard-on lab timing (`replay-retired-summary.json`): WORK-H P50/P95/P99
1,378,016 / 1,990,128 / 2,612,394; STG 310,080 / 318,080 / 320,576;
FTR 184,416 / 269,565 / 780,259; MISC 199,328 / 400,058 / 515,463.
Mean-ALL 19.413 FPS, two-VBlank 242/1,973 (12.27%), histogram
242/1,386/293/52, maximum 10, slips 0. WORK-H improves 13,920 median /
36,131 P95 ticks versus the preceding compiled build; this is the measured
whole batch, not a gain attributed to each removed component or any other case.
All 106,542 compiled runs and 53,271 DMAs engage, 1 load, no declines/near routes;
native failures remain 39, direct/packet rejects 0. All replay/count pairs match.
The two captured 120,000-pixel game surfaces are exactly identical to control.
This banks scoped lab memory/frame-cost progress; the 30 FPS/native-zero gate
and every-roster/stage coverage remain RED. Lab arena/heap are not shipping fit.

Heavy-roster capacity remains RED on the all-content shipping sibling BB81E3B0
(menu walk=1, argmax=1, fast logic/tick HUD/profile=0). Captain is created in
battle, then Link's external dependency asset 224 is refused with 4,222 B free.
The added failure marker and addr2line identify ndsRelocEnsureLoadedAsset at
reloc_backend_assets.c:8756: its heap-fit guard, with heapdeclines=1. The loader
halts as pack reason 14, kind 5; zero battle frames. The diagnostic arena is
916,992 B, 364,800 B below the lab. General low-water UINT_MAX is uninitialized,
not a measurement. Exact remaining roster-wide memory demand is not sized here.
The older ~130 KB shortage was a different, restricted shell configuration;
do not reuse that estimate as the current all-content shipping margin.
`replay-retired-heavy-summary.json` records the result, and the exact image is
preserved under builds/p2p8-stage-replay-retired-heavy. The first capacity probe
lacked the allocation-vs-format distinction; the second added the missing extern
failure marker, reused the same ROM and confirmed the heap refusal. No build or
whole-match timing rerun was needed. All jobs are terminal.

Task36 storage, hooks and replay are physically retired; compiled Dream Land is
hard-on with unchanged geometry/pixels. Remaining phase-2 work is preparation,
other VS-stage compilation, native MISC lists and further renderer/cache removal.
The shipping capacity failure and global 30 FPS/native-zero gates remain open.
Root stays r54; no publication, no campaign work, no subagents.
Next batch, after 5f7b1212097: old Task103 costs predate compiled GX and retirement. One current diagnostic partition is required to select the remaining producer-to-consumer replacement. The existing census collector now supports the four-CPU target and reads compiled submission plus live preparation counters, removing retired replay counters/reporting. Diagnostic build build-p2p8-stage-prep-census (seeded from the qualified lab) sets only NDS_TASK103_STAGE_RUN_PHASE=1; source inputs frozen, job 26709. These diagnostic timings are not acceptance FPS.

Current diagnostic 1445944F completed over frames 439..499: matrix preparation
135,635 ticks/frame, material preparation 6,203, config 4,017, renderer prepare
6,462. No repeated state-span/PrepareRun work occurs in the warm window. Commit
is 153,840 ticks (132,599 in the run loop); preparation totals 157,254. All 54
compiled runs/27 DMAs per frame engage. Use this partition only, not acceptance FPS.

Shared-camera candidate: stage bindings now consume one frame camera product,
source perspective and Mod1 billboard operands. Individual world transforms and
source multiplication order remain live. The context is stack-scoped, not a
persistent mirror. The obsolete dynamic-binding list is removed; the rigid
source-key guard remains. The existing generic camera function still serves
other native effects through the same formulas. The host test executes the real
kind48/cache branches: 24 camera states x 7 bindings match the per-draw path,
including the collapsed eye distance, with one shared perspective/Mod1 build.
The consumed-field policy follows the two factored camera producers; stale
field pins were detected and corrected at the producer. Runtime matrix, image
and timing proof is still due. The diagnostic baseline is retained separately.
Shared-camera host suite: 32 checks passed initially and one source-token pin required updating to the factored Mod1 producer; the full Yoster suite then passed 8/8. GBI/source fixtures pass in full and the stage packet remains b3833549 (geometry unchanged). Baseline 0A14A9F9 matrix snapshots at the two preserved gameplay digests are captured as frame-camera-control-a/b. Candidate runtime sources now freeze for frame-camera-build; no comparison may borrow the candidate composed matrices as its own reference.
Shared-camera build passed native-only: 8E6E7D8DA131FFC7807FA024AB32D22BBAACA240A5C6CFFDE165BF62FAD9F71B. Static image -312 B (-280 text, -32 BSS) against retirement control. All 42 composed matrices (672 cells), rigid mask and hidden mask are bit-identical at both recorded gameplay digests (frame-camera-matrix-pairs.json). The candidate uses the original frame multiplication order, with world transforms remaining live. Full frame-camera four-CPU timing/output run now starts; source inputs remain frozen.
Shared-camera full run completed: all 1,972 replay/count pairs match. WORK-H
P50/P95/P99 1,337,280 / 1,962,762 / 2,587,860; STG 267,520 / 276,224 / 278,419;
FTR 185,152 / 270,803 / 777,661; MISC 197,920 / 398,525 / 514,865. Mean-ALL
19.811 FPS; 317/1,973 two-VBlank presents (16.07%), histogram 317/1,353/255/48,
max 11, slips 0. WORK-H improves 40,736 median / 27,366 P95 ticks against the
retirement control; stage improves 42,560 median / 41,856 P95 ticks. This is
one measured default-roster case, not an all-case gain. Engagement is unchanged:
106,542 runs, 53,271 DMAs, one load, no declines/near routes; native failures
remain 39 and packet/direct rejects 0. Lab heap/arena remain 120,916/1,281,792 B.
Both time-matched game captures are pixel-identical to 0A14A9F9. The lab image
is preserved under builds/p2p8-stage-frame-camera. Shipping build/footprint and
CSS check follow; the prior heavy-roster capacity failure remains open.
Shared-camera shipping build passed native-only (318 inputs): B99B32FB80593D92164ED84426347B48DDC66ED20D22AE043ADFDEB34A15B1CD. Static image is 320 B smaller than 0879E550 (text -288, BSS -32). CSS reservation remains 221,136 B free with 38,064 B margin, fail=0; all three selected preview surfaces are pixel-identical to the retirement control. The shipping general heap span remains 916,992 B. The prior BB81E3B0 heavy-roster failure is retained: this camera batch changes no pre-battle allocation path, and makes no new capacity claim. Shipping artifact preserved in builds/p2p8-stage-frame-camera-shipping; root remains r54. All jobs terminal. This is scoped IMPLEMENTED_NOT_ACCEPTED progress; persistent world-cache/hierarchy work, other VS stages, MISC and global gates remain open.

Next batch after 0bb6d5766bc: replace the persistent stage-world cache with a
preorder pass over the already validated DObj table. Mark dynamic bindings and
their ancestors, build each required world once in source parent order, and
consume consecutive display-head bindings. Only the stage's bounded depth is
kept on the stack; there is no permanent mirror or hash/source-key cache.
The baked rigid-world guard remains. Other native consumers keep the prior
64-entry frame-local allowance; the old 64 stage slots and 4,608-byte metadata
allocation are removed. Legacy persistent-cache counters/readers are retired.
A real-C host test compares all 128 binding masks against independent parent
chains, exercises multi-head nodes and live changes, and rejects malformed
parents/depths. The optional-cache reserve/alignment/scene-retry test passes at
its new 4,352-byte size. Eight focused host checks and all GBI fixtures pass.
Control 8E6E7D8D and its paired matrix snapshots are retained. Candidate sources
freeze for world-pass-build; runtime matrix, output, frame cost and heap proof
remain due. This is IMPLEMENTED_NOT_ACCEPTED, not a new capacity claim.
World-pass build passed native-only: D27BB2D06D63E9875E20AA4D81E5E752902A7F86238105743D107C7D2AAE3EBA. Old persistent-cache definitions are absent from the ELF. Linked static image -660 B (text -520, data -8, BSS -132); allocator recovery remains to be measured. Source frozen for paired matrix capture at the existing gameplay digests; job 47950. Reuse the 8E6E7D8D frame-camera-candidate-a/b control matrices.
World-pass paired comparison passes: every cell of all 42 composed matrices and both masks match the saved 8E6E7D8D states at both gameplay digests. Full world-pass four-CPU run 32930 is active, with frame/node/binding engagement counters. Sources remain frozen; no performance or allocation gain is yet claimed.

World-pass D27BB2D0 full run completed and regressed; it is not banked. WORK-H
P50/P95/P99 1,371,904 / 1,991,757 / 2,606,667; STG 300,160 / 302,877 /
304,403; FTR 185,344 / 270,787 / 774,326; MISC 199,776 / 398,480 / 508,950.
Mean-ALL 19.449 FPS; two-VBlank 256/1,973, histogram 256/1,367/300/50,
maximum 10. All 1,972 replay and geometry-count pairs match. Engagement:
1,973 flat frames, 80,893 worlds (41/frame), 53,271 matrix consumers;
106,542 compiled runs, 53,271 DMAs, one load, no declines/near routes. Native
failures remain 39. Heap 113,492 B (-7,424), arena 1,265,408 B (-16,384).
The 8,960 B cache allocation recovery is outweighed by the arena loss. Config,
ROM file inventory and ARM7 match the control; ARM9 static shrank 660 B, so
the arena loss is unexplained. Preserved image: builds/p2p8-stage-world-pass-before-static.
All jobs terminal. Refinement reuses captured affine worlds for bindings whose
orientation still follows the camera, with the existing source-key guard.
This avoids rebuilding authored static chains; their camera composition remains
live. The real-C test covers both masks independently. New producer semantics
invalidate the first candidate's timing and require a new frozen qualification.
Static-world refinement: all eight focused host checks and full GBI/source
fixtures pass; generated Dream Land packet remains b3833549, manifest follows
701 consumed fields. Source frozen for world-static-build (9589). The memory
diagnostic only reads boot/allocation state from preserved ROMs; it changes no
reserve or arena policy and supplies no acceptance timing.
Refinement 7442844501FD3D0EEFD1009CCE86FDA9A63EC2BCB05458FA6C98BE300BDAF8EA
build passes (246 native link inputs), static -540 B from 8E6E7D8D. Both
42-matrix comparisons and masks remain exact. Full world-static run 66525
active; inputs frozen. Boot probes on the preserved control, first attempt and
refinement locate the 16 KB arena loss before stage work: same heap ceiling
0x023f0000, same large allocation counts/sizes, but early newlib growth differs
by four pages. Two 65,592 B aligned requests and seven 16,440 B aligned stack
requests occur in both images. The control uses four 20,480 B stack growths;
the refinement uses five, while avoiding one earlier 4,096 B growth. This is
allocator placement/fragmentation, not growth of stage cache storage. The
current arena chooser retains the smaller result; no reserve/padding workaround
has been applied. GDB backtraces after coroutine switches are cache-incoherent
and are not evidence of guest corruption; raw request sizes and final arena
counters are the usable observations.

**World-pass outcome: REJECTED, production changes reverted.** The refinement
74428445 built only 28 worlds/frame (55,244 total), but WORK-H P50/P95/P99
1,351,680 / 1,980,259 / 2,619,994 still regresses the retained 8E6E7D8D
control by 14,400 / 17,498 / 32,134 ticks. STG 282,240 / 284,992 / 286,611;
FTR 185,216 / 270,269 / 770,452; MISC 199,520 / 398,157 / 508,837. Mean-ALL
19.650 FPS; two-VBlank 289/1,973 (14.65%), histogram 289/1,360/276/48,
max 11, slips 0. Heap/arena remain 113,492 / 1,265,408 B. All replay/count
pairs and both matrix states match; compiled engagement/native failures remain
unchanged. No gain or cache retirement is banked from either attempt. The
preorder pass recomputes worlds which the persistent source-key cache already
reuses; static-affine reuse removes some of that work but does not recover the
control's cost. Stop refining this replacement. Next: the other eight compiled
VS stages, then native MISC lists; cache retirement remains owed.

Preserved final attempt: builds/p2p8-stage-world-static; source and host test in
world-static-rejected.patch, based on 0bb6d5766bc. world-pass-rejected-summary.json
owns both measurements and the retained control. Production files are restored
to HEAD through focused patches and the consumed-field manifest through its
generator. The restore helper first hit Windows text-encoding/BOM errors;
explicit UTF-8 corrected them before the successful patch. All runtime jobs are
terminal; no new shipping build or publication is justified for reverted code.
Root remains r54. Continue serially, without campaign work or subagents.
Restore check: runtime/source helpers match 0bb6d5766bc exactly. Regenerating
the consumed-field manifest also exposes the prior checkpoint's unstaged
shared-camera manifest update (691 fields); retain that producer output now.
It is documentation of the already-qualified camera code, not a ROM change.
The rejected patch applies cleanly against the base index; docs check passes.

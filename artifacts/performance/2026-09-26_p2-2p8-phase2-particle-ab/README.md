# P2-2p8 Phase 2 particle batching

Status: **CHECK / IMPLEMENTED_NOT_ACCEPTED**.

Recovery on 2026-09-26 found the particle work already in the uncommitted
renderer and verifier, beyond the board's previous particle-start cursor.
`gNdsP2ParticlePacket` selects immediate control (0), RAM/DMA packets (1),
or packed direct FIFO writes (2, current default). The implementation preserves
source order and batches material changes by alpha, texture and palette.
It does not yet implement the architecture's view-space particle centres.

The recovered `armed-*` / `control-*` rows measure the earlier DMA build;
`direct-armed-*` measures the current direct-FIFO build. Their identities must
not be mixed into a same-ROM comparison. Current ROM in
`builds/build-p2-fourcpu-tickhud/`:
`A3845609C75675D9C2E807A8AA4018579F5F319E11518E925371EF1C6BF59E4A`.
Its direct armed run contains 1,972 samples at frames 2..1,973 with no boot
pokes. Existing evidence is retained; no completed arm is restarted.

Next: collect the missing direct-FIFO same-ROM control with the sole boot
poke `gNdsP2ParticlePacket=0`; compare replay, native/resource/output counters,
MPRT/MISC/WORK-H and cadence. The recovered ROM and consumed inputs are frozen
during that run. Log: `builds/p2-phase2-particle-direct-control.log`.
Job: exec-command session **15915**, launched through `pwsh` with the direct
four-fighter verifier, `-NoBuild -Build build-p2-fourcpu-tickhud -RunnerSlot 4`.
Completion requires the writer exit, `.exit` file and complete four sidecars;
the known native-failure verdict must be reported separately from measurements.

Owed: review/fix the recovered implementation, source-backed command/geometry
checks, matched output captures, remaining view-space/camera/FireGrind work,
retire rejected and temporary routes, final hard-on qualification, shipping
memory/CSS checks if static growth changes, integrated verification and full
roster-stage acceptance. The known 39 native failures and heavy-roster shipping
allocation failure remain open.

The new goal authorizes up to eight GPT-6 Luna Max helpers, superseding the
older serial-only note. Two read-only helpers inspect the particle source
contract and evidence; the integrator owns all shared edits/builds/timing.
No build or emulator was live at recovery; older preparation is not a new
implementation result.

## Recovered routes: no measured gain

Direct control session 15915 terminated with exit 1 after complete sidecars;
both arms fail only the standing native count 39, with identical identity/root
and material pointer 37,498,584. Both replay comparisons are identical for all
1,972 frames (`dma-replay-compare.json`, `direct-replay-compare.json`).

Direct FIFO, same ROM A3845609, control -> route 2:

| Bucket (ticks) | Control mean / P50 / P95 | Direct mean / P50 / P95 |
| --- | --- | --- |
| MPRT | 48,907 / 47,168 / 81,728 | 49,760 / 48,384 / 83,200 |
| MISC | 229,659 / 214,144 / 400,704 | 230,596 / 215,616 / 401,152 |
| WORK-H | 1,454,191 / 1,355,776 / 2,614,016 | 1,455,136 / 1,357,888 / 2,605,568 |

Mean-ALL presentation is about 18.87 -> 18.88 FPS. VBlank 2/3/4/5+ populations:
276/1324/223/150 -> 274/1332/218/149, maximum 10 in both, population 1,973.
Route 2 emits 8,386 quads, 4,426 material groups, 159,160 words, zero fallback.
The earlier DMA same-ROM pair is also slightly slower in MPRT/MISC. Neither
packet-only route is banked as a performance win. The preserved direct ROM,
ELF, config and recovered diff are in `builds/p2p8-particle-direct-a3845609/`;
hashes are in `direct-inputs.json`. Native pixels were not qualified.

## View-space producer/consumer experiment

Route 3 retains the 0x4C camera's split look-at/perspective factors in the
existing frame-scoped CObj cache, at their producer before composition. The
particle pass consumes that same-frame camera once, preserves source world
centres and signed affine X/Y scales, then transforms each fixed centre once.
Billboard legs are two scalar extents instead of six world-space components;
the ordered material/vertex stream shares the existing Whispy DMA buffer.
Other native camera types retain the existing world-space implementation during
the experiment; their conversion and coverage are explicitly still owed.

An audit also identified first-pass palette state: the opener's logical keys
can match while a preceding ENV palette remains bound. The first generic
packet now forces the resolved raw texture/palette state. No geometry is sorted.

Next: focused C fixtures and one serialized incremental four-CPU build, then
engaged geometry/capture and same-ROM route-0/3 comparison. The source is frozen
for the build at launch. The missing native owner behind the standing 39 errors
was traced to source DamageFlyMDust, asset 83/root 0xCA58; a helper is preparing
new producer/executor files for later integrator wiring, outside this build.
Build job: exec-command session **31364**, standard Makefile-owned parallelism,
`TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-fourcpu-tickhud`.
Log/exit: `builds/p2-phase2-particle-view-build.log` / `.exit`.
That build passed; inspection removed the redundant stack copy of split camera
factors by writing them directly into the existing cache entry. The final r2
build (session 11950, exit 0) also removes six float scale multiplies from the
view-space producer and reuses each planar X/Y/Z vertex coordinate. ROM:
`608C79AC150A39BFF519DF2AEA77271A59C3F368F87B764BFE5E33CC9E561F48`.
Native-only enforcement passes with 246 actual link inputs; configuration hash
is unchanged from the recovered candidate. No root ROM was published.

Focused host proof: `scripts/test_particle_view.py` passes four tests against
the actual C header, including overflow without input mutation. Representative
camera geometry error is at most 0.094991 px / 0.00000469992722 NDC depth;
this model is not a target pixel or camera-integration proof. The production
FIFO fixture also passed its original buffered/direct ordering cases; planar
mode coverage is being added before the next timing interval.
Exact control capture job: exec-command session **76288**, isolated runner 4,
repo-local interpreter/software renderer, source clock 1500/1498, sole particle
route write 0. Log: `builds/p2-phase2-particle-view-capture-control.log`;
images: `artifacts/visibility/2026-09-26_p2-2p8-particle-view/control-{a,b}.png`.
Source/ROM frozen; no authoritative timing or other emulator overlaps it.
Control capture 76288 and armed capture 97804 both exited 0. At source tics
1500/1498 the inspected 400x294 gameplay crop (window origin 8,56, excluding
the debug FPS overlay) is pixel-identical in both matched pairs: 0 of 117,600
pixels differ. Adjacent control presents differ by 41,805 pixels (35.5485%),
confirming why the source-clock lock matters. This is only those captured states;
particle engagement and the remaining states must be checked separately.

`python -m pytest scripts/test_particle_packet.py scripts/test_particle_view.py -q`
passes all five tests, including actual-C planar mirror masks 0..3, signed
extents, and ordered scale changes 0->1->2. Log:
`builds/p2-phase2-particle-host-tests.log`.
Next job: full 1,972-sample route-3 match on frozen 608C79AC, runner 4, collecting
timing/replay/engagement/native/resource sidecars together. Log:
`builds/p2-phase2-particle-view-armed.log`; no other emulator or heavy host work.
Active job handle: exec-command session **37040**. Completion requires process
exit plus complete timing, row, coverage and memory output. Task49 particle
coverage is still owed: packet DMA bypasses its current record funnel, and its
effect vertex analyzer must model the particle matrix push/pop/scale stack.
Route-3 run 37040 exited 1 solely on the standing 39 native failures after all
sidecars completed. Engagement is positive: 5,919 view passes, 11,192 transformed
centres, zero view rejects, 8,386 generic packet quads, zero packet fallbacks.
MPRT mean/P50/P95 = 44,940 / 44,096 / 78,208 ticks; WORK-H P50/P95 =
1,354,304 / 2,638,784 ticks. The same-ROM control is now required before any
delta claim; raw values from earlier binaries are not its comparator.
Next job is `view-control-*`, identical ROM with sole boot poke
`gNdsP2ParticlePacket=0`; log `builds/p2-phase2-particle-view-control.log`.

## View-space same-ROM result

Control session 32594 is terminal, exit 1 only on the same native count 39.
`view-replay-compare.json`: 1,972/1,972 identical digest pairs, none skipped.
Both arms use 608C79AC and the same source item law/roster/camera. The candidate
engagement above is positive and has zero view/packet failures.

| Metric | Control | View-space route 3 |
| --- | ---: | ---: |
| MPRT mean / P50 / P95, ticks | 48,681 / 46,912 / 81,920 | 44,940 / 44,096 / 78,208 |
| MISC mean / P50 / P95, ticks | 228,661 / 213,440 / 398,528 | 224,823 / 210,048 / 396,352 |
| WORK-H mean / P50 / P95, ticks | 1,455,444 / 1,359,744 / 2,611,264 | 1,451,281 / 1,354,304 / 2,638,784 |
| FPS (timer rate / mean ALL) | 18.86 | 18.91 |
| VBlank 2 / 3 / 4 / 5+; maximum | 275 / 1326 / 222 / 150; 11 | 279 / 1332 / 214 / 148; 11 |

Percentiles use the sampler's floor((N-1)*p) rank, N=1,972; cadence covers
1,973 presents. **KEEP as an unaccepted candidate:** MPRT mean improves 3,741
ticks and WORK-H mean/P50 improve 4,163/5,440. There is **no P95 improvement**:
WORK-H P95 worsens 27,520 ticks. `view-paired-deltas.csv` retains every row;
large positive frame deltas include shifted audio refill costs (e.g. frame 718:
WORK-H +108,032, AUD +114,688, MPRT -6,592). This explains a measured contributor,
not an exemption from the worse tail or permission to subtract audio from the
gate. No repeat is needed to force a favorable statistic.

These arms intentionally match the recovered NDL-off configuration. Before
acceptance, integrate the pending native dust owner, qualify the final hard-on
MISC configuration, and carry all shipping/camera/stage/Task49 debt. The frozen
control/route-3 comparison is complete; proceed with the integration work.

## DamageFlyMDust native closure

The missing source owner is asset 83/root 0xCA58, a four-vertex/two-triangle
DObjDLLink with seven TEXID step frames. Source flags at MObjSub+0x30 are zero;
the leading 0x0010 is padding, not FRAC. The source-default snapshot is
CURRENT_IMAGE|RENDER_TILE_SIZE|TEXTURE (0x1600). Initial audit assumptions about
FRAC/next-image interpolation were corrected before target qualification.
Host O2R payload is big-endian; only the word-swapped runtime image uses XOR-3
byte reads. Testing both lane expressions against unswapped source data was an
invalid audit comparison, also corrected before target qualification.

The native owner consumes the real material and matrices. At load time it
converts the seven current images into disjoint A5I3 intensity bands: frame 0
needs band 3 only; frames 1..6 use all four. Total: 25 texture planes, 25,600 B
VRAM plus palettes. Each source texel appears in exactly one band, preserving
all native RGB5 intensity and alpha5 values. Draws bind resident textures; no
runtime filling, scene compositor or binary-alpha fallback is used. The shared
quad kernel gains a resident-texture entry, keeping its matrix/UV/color rules.

`test_native_damage_fly_mdust.py` validates the source root/frame pointers and
executes the actual C converter for all 65,536 IA16 values plus all 7,168 source
texels, including runtime word swapping and disjoint coverage. Combined with
the particle fixtures, **7 tests pass**. Runtime admission, all-frame native
engagement, pixels, resource/pacing and integration proof remain owed.
First build failed on an extern volatile qualifier; fixed at the declaration.
Build job **82509** is the corrected incremental build; log/exit:
`builds/p2-phase2-dust-build-r1.log` / `.exit`. Inputs are frozen while it runs.
Next: inspect actual texture admission before the longer natural-match proof.
Admission probe 64559 exited 0 on `3EB44059`, NDL armed: all 25 planes / 25,600 B
are resident, preparation failures 0, native failures 0 at startup; heap low-water
168,928 B, arena 1,281,792 B. No dust object had spawned in that eight-frame
window, so this proves admission only. `dust-admission.json` owns the identity.
The final source also checks the repeated frame-index lookup result to remove
the compiler's may-be-uninitialized warning; no capacity/texture change.
Next: full natural four-CPU run with NDL armed, including all seven dust-frame
engagement masks, native/resource checks and timing. New build log:
`builds/p2-phase2-dust-build-r2.log`.
Build 37303 exited 0, native-only 246 inputs. New ROM:
`8B4D66EE18945BA77036C51C8D1A7B37E8D5083458601C0A344F5D8097B2D7F1`.
Active full-match job: exec-command session **61032**, runner 4, NDL boot word 1,
default particle route 3. Log/exit `builds/p2-phase2-dust-integrated.log` / `.exit`;
sidecars `dust-integrated-*` in this receipt directory. No parallel emulator or
heavy host check; source and generated inputs frozen until writer exit.

### Integrated natural-match result (scoped GREEN; performance RED)

Session 61032 exited **0** with all four sidecars complete. NDL is explicitly
armed at boot; particle route 3 is the ROM default. Dust draws **39**, rejects
**0**, current-frame mask **0x7f**, triangles **294**, alpha-zero draws **0**.
All 25 planes are resident (25,600 B), preparation failures **0**. Both native
failure and direct-reject counts are **0**, replacing the old 39 NO_PROGRAM
events. DamageSlash runtime texture updates are also 0. This is source-driven
engagement, with no effect/status/item injection.

`dust-replay-compare.json` matches every one of the 1,972 gameplay digest pairs
against the preceding view-space run. Heap low-water 122,412 B; arena 1,281,792 B.
WORK-H mean/P50/P95: **1,441,015 / 1,344,768 / 2,587,264 ticks**. MISC P50/P95:
205,632 / 354,368; MPRT 44,928 / 80,512. Mean-ALL **18.98 FPS**; VBlank
2/3/4/5+ = **282/1341/206/144**, maximum **11**, population **1,973**. Product
performance is still RED; the script's correctness GREEN is not 30 FPS acceptance.
This combines new dust output, view-space particles and NDL; it is not a new
causal timing claim for any individual constituent.

Capture session 10521 exited 0 at exact frames 1042/1043 within dust's observed
1036..1052 draw window. Both pictures were inspected: the source dust puffs are
visible with graded coverage. Images are `dust-1042.png` / `dust-1043.png` under
`artifacts/visibility/2026-09-26_p2-2p8-particle-view/`. This does not establish
all cameras or scene transitions. The updated GBI fixture completed GREEN
(`builds/p2-phase2-dust-gbi.log`). Documentation checks pass after compacting
receipt detail out of the capped live board; no cap was raised.

**Current job: none.** All build, sampler and capture handles above are terminal.
**Next:** remove settled particle experiment routes, qualify the final intended
MISC configuration without a boot poke, and check the shipping CSS/memory shape.
Remaining: particle Task49 stack/packet instrumentation, other camera/stage and
natural Fireball/efground engagement, scene lifetimes, integrated Latest/Boundary
as applicable, renderer retirement and all-roster/stage performance acceptance.
MF residency/ARM7 audio and later architecture phases are still unimplemented.
Root r54/P1 artifacts remain unchanged; this is IMPLEMENTED_NOT_ACCEPTED.

### Checkpoint capability / publication observation

The scoped `git add` attempt failed before staging: `.git/index.lock` already
exists, zero bytes, created/last written 2026-09-25 23:00:11 local. A process
inspection found no `git.exe`; ownership is nevertheless unverified, so the
lock was not removed (`VERIFYING.md`). The user has been asked whether it is
abandoned. Commit/push remain owed. Retry only after confirmed ownership/owner
authorization or observed legitimate removal; do not repeat unchanged writes.
The initial index was empty. Unrelated owner deletions/docs and lab-generated particle
configuration are preserved. All relevant source, producers, tests and receipts
are on disk; no Git failure discards their completed evidence.

The owner subsequently confirmed the lock was abandoned and authorized removal.
No Git process was active on recheck; only the confirmed lock path was removed.
Scoped staging then succeeded. This is the observed authorization change that
permits checkpointing; the earlier failed attempt did not establish a commit.

No published target was built or overwritten. A direct file check found root
`smash64ds.nds` absent; r54 is the last documented publication, not a newly
verified on-disk root identity. The verified lab ROM/ELF/config are preserved in
`builds/p2p8-misc-dust-8b4d66ee/`; `dust-inputs.json` records their hashes and
scoped source identities.

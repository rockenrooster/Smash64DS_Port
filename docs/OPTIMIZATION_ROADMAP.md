# Optimization Roadmap — Cut G Accepted; M2–M4 Measured

Updated: 2026-07-15

P1 deadline: 2026-07-19 23:59 Central

Runtime boundary: canonical `battle_playable_realtime`, mode 163

This is a concise renderer status and experiment synopsis, not a project queue.
P1 priority and lane decisions live only in `docs/P1_EXECUTION_BOARD.md`.
Detailed measurements, including rejected experiments, belong in
`docs/PERF_LEDGER.md`; the exact native-owner implementation contract belongs in
`docs/optimization/NATIVE_RENDERER_PLAN.md`.

## Current Truth

The current user-facing artifact is:

```text
smash64ds-battle-playable-hwtri.nds
14,368,768 bytes
SHA-256 F8EFEE10ED15457CD79A9B71B9766B5247BE870C332FB12316431F8301A0A94A
```

The canonical and shipped copies match byte-for-byte. Refresh this identity
after the final build; compilation alone never proves a milestone.

The latest focused M2 A/B/A used profile-1 ROM `03950839...BEEF09B`, frames
600–607 / logic 209–216, and reported about 10.1–10.2 FPS. It is laboratory
evidence, not a canonical artifact. After rejecting and removing the treatment,
the profile-1 rebuild is `0192BAFF...AE5E9`; neither laboratory ROM is shipped.
The current profile-0 ROM above was rebuilt and recaptured at exact completed
frames 438/439. Gameplay still reports two uploads / 36,864 bytes.

## Milestones

| Milestone | State | Measured boundary / target |
|---|---|---:|
| M1 — simple hardware-affine BG2 | Complete | 1,856/1,856 ticks; beats the ≤35K ceiling |
| M2 — AOT DS-native Mario/Fox | In progress | ledger-off 431,168/458,688; evidence supports 50–75K first cut; target 170–250K |
| M3 — AOT DS-native complete stage | Strict slab falsifier specified | 800,608/803,456; target 150–250K |
| M4 — conversion off gameplay path | Static-manifest implementation specified; water representation blocked | ~69 static keys fit; exact 216-state water does not fit current representation |

The owner targets are active renderer ticks, not FPS estimates. Re-profile the
whole frame after each accepted owner cut.

## Accepted M1 — Cut G

Cut G retains one complete source-rendered Dream Land wallpaper in native
256x192 BG2. Live BattleShip `grWallpaperCalcPersp` state updates DS affine
registers while original stage, fighter, effect, interface, and foreground
ownership remains live.

Acceptance requires:

- one complete 49,152-pixel seed and no retained-wallpaper fallback;
- one affine update per live frame, zero coverage failure, and nonidentity
  motion;
- affine work no greater than 35,000 ticks;
- the exact two-fighter/626-triangle contract;
- source timer/control synchronization through Wait → GO;
- semantic counters and exact completed-frame 438/439 screenshots.

The accepted profile-1 window measured 1,856/1,856 wallpaper ticks. Do not
revive the rejected three-mip, display-capture, scanline/HBlank, sparse-
correction, or guessed-camera-window designs without a distinct source-backed
architecture and a new exclusive cost model.

## Approved Lower-Screen HUD

The bottom screen now shows FPS, timer, Mario/Fox identity, stock, and damage
as change-driven text. Countdown, traffic light, 3/2/1, and GO remain on the
top screen.

Same-ROM synchronized frames 600–607 measured steady top-interface route
removal as:

```text
foreground     788,160/788,288 -> 0/0 ticks
draw         3,044,672/3,047,872 -> 2,256,192/2,259,328 ticks
draw median                                  -25.90%
present median                               -17.57%
loop median                                  -17.50%
```

The repeated disabled control reproduced the original values. Treat the HUD as
accepted presentation, not a remaining visual milestone. Keep its battle-only
lifecycle, post-GO source-state gate, and Results teardown proof.

## Accepted Native Countdown / GO

The BattleShip thread and timing remain authoritative: it owns
the rod/frame/lamps, 3→2→1 changes at source ticks 120/180/240, GO at 300,
control unlock/timer start, and teardown at 420. The accepted DS backend keeps
those source GObjs/SObjs live and maps their position, scale, color, and order
to setup-converted main bitmap OAM chunks.

On integrated profile-1 ROM `CCC30624...FEBAF`, synchronized frames 187–194
reduced foreground median from 1,863,232 to 10,336 ticks; inclusive native OAM
median/P95 was 11,584/11,584. Setup converted 16 source assets / 59 tiles into
93,824 OBJ-VRAM bytes once. Runtime conversion, palette payload, and upload
were zero; the final objects cleared at frame 511 and frame 600 had no retained
OAM tax. The source window retained 10 SObjs, 24–26 OAM objects, Wait/GO locks,
timer state, semantic hashes, and complete synchronized screenshots.

Bitmap OBJ has one-bit per-texel transparency, so nonzero partial-alpha texels
in the RGBA GO art are currently opaque. This is bounded presentation debt,
not permission to restore the 1.86M-tick software compositor.

## M2 — Finish the Existing AOT Fighter Owner

Do not create another per-root interpreter. The live generated production owner
already contains 32 roots, 49 epochs, 67 runs, 626 triangles, and 541 immutable
vertices. It preserves event order, typed live materials, selected display
lists, persistent vertex-cache semantics, and the 44 cross-matrix triangles.

The current ledger-off synchronized owner is about 431K combined. Candidate
`54379201b2` packages exact 32-root/49-epoch/67-run/626-triangle/541-vertex/
44-cross-matrix bookkeeping into 17,704 bytes with 4,324 bytes of scratch. It
is tooling eligibility only: it has no live matrices, typed materials, exact
light sidecar, GX execution, device cycles, or independent profile-2 oracle.
Do not count it as M2 progress or paste its per-root microbench interpreter into
production. Rejected experiment measurements remain in `PERF_LEDGER.md`.

Source audit adds one hard preflight rule: current generic local-matrix code is
incomplete for active animlock, although Mario/Fox main and submotion tables use
no `FTANIM_FLAG_ANIMLOCKS`. Natural P1 samples must census zero and any active
owner must reject before its first GX write. M2 promotion still requires an
independent three-branch mode-0/mode-3/sibling-restore fixed-point fixture (or
the exact BattleShip active branch); a neutral Mario/Fox screenshot is not proof.

The next architecture is two non-overlapping packets. First, preflight corrected
25/27 joint schedules and 14/18 bindings, let GX walk/store the hierarchy, and
remove parallel CPU world/composed 4x4 construction while retaining an exact
CPU 3x3 normal/light sidecar. Retained measurements support about 50–75K after
GX command cost, so the unchanged ≥80K gate may falsify it. Second, compile the
49-epoch/67-run state
transaction and dense-corner emitter into one owner call without retrying 121
small GX lists. Profile 2 remains the independent light/matrix/state oracle.

GX push/pop and explicit store/restore share one matrix-stack namespace. Mario
binding IDs `1,2,5,6,8,9,11,12` currently map to physical slots 1–8, while Fox
maps to 1–2; those physical slots would overlap a live GX traversal stack. The
corrected packet reserves Mario slots 16–23 and Fox slots 16–17, retaining the
measured 11 pushes, 11 pops, 10 stores, 84 restores, and 44 cross-matrix
triangles without aliasing hierarchy state.

The no-GX parity gate makes the ownership boundary explicit. Across 413 exact
binding/normal RGB15 cases, even exhaustive per-binding one-light material
choices miss 102 cases (16/18 bindings), so DS hardware lighting is demoted.
Across 99 canonical t16 cases, naive scale-shift mapping misses 47 while the
generated floor/ceil 20.12 coefficients plus fractional bias miss zero.
Therefore geometry hierarchy and synthesized texture matrices may move to GX;
lighting must stay in the exact CPU sidecar unless a new oracle-exact design
supersedes this result.

Falsification gate:

- a synchronized same-ROM A/B must save at least 80K combined fighter ticks and
  reach <=351K in the first window. Current evidence supports only 50–75K, so
  expect this gate may reject the design; the 170–250K promotion gate is measured
  only with `NDS_RENDERER_M2_DETAILED_LEDGER=0`;
- the chosen design must retain a credible route to the 170–250K target rather
  than polishing either measured bucket in isolation;
- production remains exactly 70 runs / 686 fast triangles, partitioned
  60+320+306 with bounded 29/0/0 fallbacks;
- an independent adjacent-completed-frame `WEAPON_FRAME` delta supplies `q`;
  that terminal frame is exactly 2,484+6q vertices, 828+2q triangles, and class
  census 648/44/(126+2q)/10;
- semantic, owner entry/exit, vertex-cache, GX matrix, texture/upload, and
  screenshot gates remain coherent;
- no per-frame allocation and no reserve regression.

For the hierarchy packet specifically, require matrix+light <=80K, ledger-off
combined <=351K, and the safe slot assignment above. For the epoch/run packet,
require >=60K incremental saving, production <=120K, and final ledger-off
combined 170–250K.

Promote Mode 8 to canonical profile 0 only when synchronized counters,
profile-2 oracles, captures, and natural runtime all agree.

## M3 — AOT Complete Stage Owner

Use one strict whole-owner preflight/control session across the eight source
stage callbacks, segmented at links 4/6/13/16/17 around fighters at 9 and
weapons at 14. Each callback closes its logical GX batch, restores profile
ownership, and rebinds state on resume; it must not flush. Preserve BattleShip
display order, live state, animation, effects, depth, and translucency.

Do not revive the rejected source-shaped typed stage executor or mode-9 record
executor. The exact slab contract is 42 lists / 886 commands / 302 vertices /
54 runs / 202 triangles, partitioned 66 raw, 126 no-Z, and 10 projected. Keep
known projected/dynamic records cold, assert zero cross-matrix triangles, use
the live matrix per list, and keep no-Z depth frame-global. Resident slab data
must be <=16 KiB with no new BSS/heap; any mismatch falls back before mutation.

Across completed runtime traversals, exact cumulative accounting is:

```text
stage submits   = 42F + W
stage triangles = 202F + 2W
```

`F` is completed source traversals and `W` is cumulative source link-14 weapon
quads. Unmarked setup traffic fails the gate.

The single paired falsifier must save at least 300K stage ticks and reach <=500K
with P95 improved while retaining all 42/886/54/202 counts, full-frame 828
geometry, semantic/state hashes, texture counters, order, motion, and
screenshots. Remove the slab if either threshold fails. M3 promotion remains
150–250K after M4 removes the conversion wall.

Promotion requires synchronized A/B/A timing, exact geometry/state hashes,
profile-2 semantic parity, stable memory reserve, and successful moving
screenshots.

## M4 — Move Texture Work Before Gameplay

The 600-frame census already rejected a small final-output cache:

```text
322 complete keys; 206 final outputs
3,739,648 bytes for full residency
reuse distance 216
effectively zero hits in a 2–4-slot LRU
```

Do not repeat that cache or overlap experiment. Instead, at scene/fighter/stage
setup:

1. enumerate every texture/palette needed by the accepted match and owner
   variants;
2. convert to DS-ready layouts once;
3. assign stable lifetime keys and VRAM ownership;
4. upload before the live gameplay window;
5. publish RAM/VRAM/reserve totals and explicit fallback state.

During gameplay, binding prepared data is allowed; conversion, palette
conversion, allocation, decompression, and upload preparation are not. The
promotion gate is zero gameplay conversion/upload-preparation counters with
unchanged texture/material semantics.

The deterministic host generator reproduces the exact period-216 stream for
all 18,000 simulated frames: 322 live keys, 206 outputs, 3,024,896 oracle
pixels, zero mismatches, and a 1,560,960-byte payload plus 11,060-byte index.
It is now a permanent DevFast fixture, not a promoted runtime design.

The first direct runtime trial replaced conversion with an on-demand NitroFS
read and one prepared output per owner. It was rejected. In synchronized
frames 171..202, direct versus off inclusive renderer median/P95 was
1,537,984/1,573,376 versus 1,536,896/1,572,224 ticks; loop P95 was unchanged,
draw P95 worsened by 21,376 ticks, and the window incurred 12 payload reads.
Audio-adjusted reserve was only 153,184 bytes, 22,112 above the floor.

The rejected literal pair-index representation is history in `PERF_LEDGER.md`.
The RGB256 generator reserves index 0 for transparency and uses opaque indices
1–90. It is hardware-visible exact across 3,024,896 map bytes and 58,604 palette
bytes with zero alpha/RGB/visible mismatch.

M4 is now split at an explicit feasibility checkpoint. A generated pre-GO
manifest can own the static Dream Land, complete Whispy animation roots,
Mario/Fox, Fireball, reflector, and reachable GX-effect keys. The conservative
packet estimates 69 static keys / about 179,328 texture bytes; retaining two
live water owners adds 36,864 bytes, for about 71 slots / 216,192 bytes. That
fits the currently mapped 262,144-byte texture VRAM with about 45,952 bytes to
spare, but not the current 48-entry cache. Generate the exact census before
selecting a 72- or 80-entry fixed cache; those expansions cost 6,720 or 8,960
bytes of main RAM and must retain at least 131,072 bytes of reserve.

Setup belongs immediately after original battle setup returns, before live
update/draw. The post-GO fence must latch any static-owner key miss, conversion,
palette/decode work, allocation, eviction, GL create/upload/delete, refresh,
decompression, file I/O, or manifest fallback. Normal prepared-key binding is
allowed and must not be miscounted as conversion.

Water remains a separate representation checkpoint: 216 temporal states,
322 exact keys, and 206 outputs require about 903,168 bytes as exact visible
RGB256, versus 524,288 bytes of total DS texture VRAM. The 645,120-byte
map/palette pairing also fails capacity and alpha semantics. Static-manifest
success may therefore claim only zero post-GO work for static owners; M4 closes
only after a source-faithful water representation eliminates every remaining
gameplay conversion, allocation, replacement, and upload.

## Measurement And Correctness Rules

Every work cycle must:

1. enumerate Boundary membership, read active docs, and preserve the dirty
   tree;
2. read relevant BattleShip source before gameplay/renderer behavior changes
   and read `decomp/sm64-nds` before DS-backend architecture changes;
3. identify an exclusive measured bucket and state a bounded expected win;
4. compare identical build configurations and synchronized logic/frame
   windows, using same-ROM selectors for temporary A/B when appropriate;
5. use an 8-frame falsifier, then 32- and 128-frame distributions for a kept
   architectural cut;
6. require counters, semantic traces, GX/runtime state, screenshots, memory,
   and audio state to agree;
7. append one coherent experiment record to `docs/PERF_LEDGER.md`;
8. remove temporary probes/selectors before the checkpoint unless they are
   retained natural-runtime diagnostics.

No new one-bit proof masks, synthetic branch reruns, or duplicate gameplay
harness modes are permitted. Use mode-163 natural runtime and the independent
profile-2 renderer shadow.

## Whole-Frame Closure

`BUS_CLOCK` is 33,513,982 ticks/second. One DS VBlank at 59.826 Hz is about
560,190 ticks. A robust one-VBlank P95 should be no more than roughly 520K
after update, audio, input, HUD, flush, and safety margin are included.

After M2–M4:

1. re-profile update, audio, input/HUD, renderer owners, texture work, flush,
   VBlank wait, and residuals;
2. fix the largest measured remaining P1 bucket;
3. prove the natural one-minute timer expiry and original Results transition
   in the user-facing ROM;
4. run verifier coverage plus a dated capture and manual user playtest;
5. do not claim P1 complete below real-time speed without explicit approval.

A locked-30 fallback is a separate scheduler design and is not current
milestone completion. It requires explicit approval plus 60-Hz source logic,
29.9-Hz presentation, input/audio policy, no catch-up spiral, and dedicated
latency/runtime gates.

## Finalization Order

For a successful checkpoint:

1. refresh the user-facing ROM through the canonical parity rule;
2. update `STATUS.md`, `HANDOFF.md`, relevant known issues, diagnostic docs,
   the ledger, and append-only `PORTING.md`;
3. run focused, static, DevFast, forensic, P1Gate/Boundary, and risk-
   proportionate regression checks;
4. inspect the final status and commit only the coherent owned change set;
5. run `scripts/New-Smash64DSSnapshot.ps1 -Mode Lean` as the final project
   action and run no command afterward.

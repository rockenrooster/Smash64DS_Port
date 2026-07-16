# Optimization Roadmap — Cut G/M4 Pass; M3 664.5K Rework

Updated: 2026-07-15

P1 deadline: 2026-07-19 23:59 Central

Runtime boundary: canonical `battle_playable_realtime`, mode 163

This is a concise renderer status and experiment synopsis, not a project queue.
P1 priority and lane decisions live only in `docs/P1_EXECUTION_BOARD.md`.
Detailed measurements, including rejected experiments, belong in
`docs/PERF_LEDGER.md`; the exact native-owner implementation contract belongs in
`docs/optimization/NATIVE_RENDERER_PLAN.md`.
All generated screenshots and visual evidence belong under `artifacts/visibility`.

## Current Truth

The current user-facing artifact is:

```text
smash64ds-battle-playable-hwtri.nds
14,534,656 bytes
SHA-256 3F3AC2E1A20F7D93B0E92419BA642FD5D97A275454ABEC0D1C96EF7742E6BB38
```

The canonical and shipped copies match byte-for-byte. Refresh this identity
after the final build; compilation alone never proves a milestone.

The accepted ledger-off Mode-8 reference is about 413.5K fighter ticks. The
latest detailed-ledger A0/A1 is 477,152/477,376; rejected Mode 7 is
518,336/518,784 and draws blank fighters. The last control smoke window is
about 17.7 FPS, draw 1.646M, and loop 2.241M ticks. These are laboratory
figures, not canonical acceptance. The profile-0 ROM above remains unchanged.

## Milestones

| Milestone | State | Measured boundary / target |
|---|---|---:|
| M1 — simple hardware-affine BG2 | Complete | 1,856/1,856 ticks; beats the ≤35K ceiling |
| M2 — AOT DS-native Mario/Fox | Mode 8 correct; FIFO and Mode 7 rejected | ledger-off ~413.5K; detailed 477,152; target 170–250K |
| M3 — AOT DS-native complete stage | Semantic pass / performance REWORK | stage 664,544/664,640; misses <=500K by 164,544 and >=300K saving; target 150–250K |
| M4 — conversion off gameplay path | Published short Boundary pass; full one-minute fence/reserve pending | 22 keys/21 outputs/131,072 prepared bytes; sampled gameplay work zero |

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

The current clean ledger-off synchronized owner is about 412–414K combined. Candidate
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

The display-contract capture cannot donate already-built source matrices:
`gcPrepDObjMatrix` is the current recorder stub, not BattleShip's matrix
builder. Local construction and lighting already measure about 72,896 + 79,648
ticks, so a hierarchy-only retry cannot meet the <=80K matrix+light gate. The
first packet must combine corrected 25/27 joint schedules and 14/18 bindings
with source-exact local construction, including BattleShip's 16.16
`syMatrixF2LFixedW` quantization before DS 20.12 conversion, plus GX hierarchy
walk/store and an owner-qualified binding/normal lighting sidecar. Final-color
reuse must also key live epoch light/material inputs. Preflight must validate
the dynamic `matrix_dobj`, animlock, texture residency, and every fallback
before the first GX write. Second, compile the 49-epoch/67-run state
transaction and dense-corner emitter into one owner call without retrying 121
small GX lists. Profile 2 remains the independent light/matrix/state oracle.

GX push/pop and explicit store/restore share one matrix-stack namespace. Mario
binding IDs `1,2,5,6,8,9,11,12` currently map to physical slots 1–8, while Fox
maps to 1–2; those physical slots would overlap a live GX traversal stack. The
corrected packet reserves Mario slots 16–23 and Fox slots 16–17, retaining the
measured 11 pushes, 11 pops, 10 stores, 84 restores, and 44 cross-matrix
triangles without aliasing hierarchy state.

The no-GX parity gate makes the ownership boundary explicit. The retained 413
case corpus merges owner-local binding IDs and is only a hardware-lighting
necessary-condition gate. An owner-qualified recount remains 413 cases but
misses 101 cases across 25/32 owner-bindings even with exhaustive independent
one-light material choices, so DS hardware lighting remains demoted. It is not
the runtime light-cache oracle; cache identity must include owner and dynamic
epoch light/material inputs.
Across 99 canonical t16 cases, naive scale-shift mapping misses 47 while the
generated floor/ceil 20.12 coefficients plus fractional bias miss zero.
Therefore geometry hierarchy and synthesized texture matrices may move to GX;
lighting must stay in the exact CPU sidecar unless a new oracle-exact design
supersedes this result.

A same-ROM A/B/A has formally rejected the per-joint GX hierarchy packet.
Its A0/A1 control was 417,472 combined ticks and B was 404,000: only 13,472
saved. Matrix work fell 18,144 ticks, but DL transport rose 33,120 ticks. The
candidate missed the 80K relative gate by 66,528 and was removed. Do not revive
per-joint GX hierarchy.

The follow-up whole-owner FIFO copy/patch/DMA packet is also rejected. On clean
ROM `13506F55...B98589B`, frames 600–607, exact Mode-8 A0/A1 reproduced at
413,504 median / 413,632 P95 combined fighter ticks. Packet B was 537,792 /
537,856, a 124,288-tick regression. It retained exact 70/686 geometry,
60+320+306 ownership, 29/0/0 fallbacks, batch/submit classes, and texture
traffic, but uncached validation plus copying, dynamic word patching, cache
flush, and DMA cost more than the CPU work they replaced. Its selector is
removed; Mode 9 is now reserved for M3 complete-stage work. Keep the 31,880-byte
exact host packet and independent association checker only as fixtures.

The subsequent Mode-7 hierarchy owner is also rejected. A0/A1 Mode 8 is
477,152/477,376 in the detailed window; Mode 7 is 518,336/518,784 and its
screenshot contains no fighters despite exact accounting. Remove that runtime
path and its temporary verifier allowance. A read-only direct-contract design
can avoid duplicate capture/setup work with an estimated 62–75K net saving,
but this estimate is not implementation or device evidence.

Do not retry a full staging-buffer copy or merely cache validation: even the
measured packet production wall alone regressed. The next M2 design must use
the exact fixtures to eliminate immutable command work without copying the
whole packet, retain source-exact CPU lighting, and present a bounded cost
model below 337,472 before another device selector is added.

Falsification gate:

- a synchronized same-ROM A/B must save at least 80K combined fighter ticks and
  reach <=337,472 in the first selector window. No retained treatment run proves that saving
  yet, so the gate may reject the design; the 170–250K promotion gate is measured
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

For any replacement owner, require matrix+light <=120K, total transport <=145K,
A1 exactly equal to A0, and combined <=337,472 before promotion work. The final
ledger-off M2 gate remains 170–250K.

Promote Mode 8 to canonical profile 0 only when synchronized counters,
profile-2 oracles, captures, and natural runtime all agree.

## M3 — AOT Complete Stage Owner

Use one strict whole-owner preflight/control session across the eight source
stage callbacks, segmented at links 4/6/13/16/17 around fighters at 9 and
weapons at 14. Each callback closes its logical GX batch, restores profile
ownership, and rebinds state on resume; it must not flush. Preserve BattleShip
display order, live gameplay state, effects, depth, and translucency.
Presentation animation may use the documented DS visual-fidelity policy; P1
water is intentionally frozen while the rest of the stage remains live.

The current exact packet is 12,663 bytes: 8 callbacks, 57 DObjs, 42 bindings,
886 commands, 54 runs, 49 texture epochs, and 202 triangles. It includes five
projected cross-matrix runs / ten triangles / fifteen foreign corners and zero
raw cross-matrix submissions. Twelve perturbations prove fail-closed behavior.
The complete-stage owner is linked into the published intrinsic Mode-9 target.
Boundary and exact frames 438/439 prove ordered interception, mask 255, all
8/57/42/54/202 ownership counters, 49 epochs, four material commits, cross
5/10/15, and zero fallback. Resident packet data stays below 16 KiB and any
mismatch must fall back before GX mutation.

The synchronized profile-1 frames 438–445 now measure stage-exclusive
664,544/664,640 P50/P95. Against the documented ~805K baseline this saves only
about 140K, not >=300K, and misses the <=500K first gate by 164,544. The frame
still holds exact semantic/M4 counters, draw 1,183,104/1,183,168, present
1,536,448/1,537,216, loop 1,680,448/1,680,512, and 19.6 FPS. Decision: REWORK;
attack the largest attributable internal bucket before any promotion claim.

Next bounded cut: de-duplicate prepare work by dense index in
`src/nds/nds_renderer.c`, with the zero-conflict preparation-tuple invariant in
`scripts/check_nds_native_stage.py`. The 606 corner references map to 312 dense
vertices; projected work falls from 408 to 246, removing 294 repeated
color/UV/alpha preparations, 162 transforms, and 486 projections. Expected
saving is 170–210K ticks. The first device falsifier must save >=164,544 and
reach <=500K with exact counters, semantics, fence state, and improved P95.

Whispy blink/turn/open/blow/stop, flower animation, live DObj flags, and segment
E material/FRAC mutation can change draw selection or state after startup. The
preflight therefore validates live callback/DObj/DL identity before the first
link-4 GX or material write, shadows source material mutation once, and falls
back for the whole owner on any mismatch. External effect/weapon callbacks stay
outside the slab, and every stage resume fully rebinds required GX state.

Across completed runtime traversals, exact cumulative accounting is:

```text
stage submits   = 42F + W
stage triangles = 202F + 2W
```

`F` is completed source traversals and `W` is cumulative source link-14 weapon
quads. Unmarked setup traffic fails the gate.

The first quiet-window paired falsifier must save at least 300K stage ticks and
reach <=500K with P95 improved while retaining the candidate counts, baseline
full-frame geometry, semantic/state hashes, texture counters, order, motion,
and screenshots. Remove the slab if either threshold fails. M3 promotion remains
150–250K after M4 removes the conversion wall.

Promotion requires synchronized A/B/A timing, exact geometry/state hashes,
profile-2 semantic parity, stable memory reserve, successful moving screenshots,
and a one-minute sweep through Whispy/flower state changes plus weapon/effect
activity. Changed source draw cardinality must use a separately proven variant
or whole-owner fallback; it is not forced into the idle tuple.

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

M4 is split at an explicit feasibility checkpoint. The deterministic current-
ROM corpus, including the two frozen water outputs, passes 17 source blocks, 22
complete keys, 21 deduplicated outputs, 62,464 pixels, 131,072 prepared bytes,
and 126,976 payload bytes. Host lookup hits all 22 keys and fails closed for
1,232 field mutations plus explicit invalid/miss cases. Published Boundary now
proves pre-GO preparation/pinning, exact VRAM-A ownership, positive pinned hits,
and zero sampled post-GO fence work. Full-minute reserve/teardown remains open.

The smallest honest runtime checkpoint prewarms/pins the exact generated static
set, which includes the frozen 36,864-byte water pair, after original battle
setup, then arms at Wait→GO. For synchronized post-GO frames, latch any static
key miss, conversion, decode work, allocation, eviction, GL create/upload/delete,
refresh, texture-path I/O, or fallback. Normal prepared-key binding is allowed;
water refreshes are not. This proves current-scene static closure only.

Scope the gameplay fence from GO through battle teardown. Results is a later
setup/load boundary and may prewarm before rearming; a global no-I/O claim would
instead require Results assets in the original census and reserve budget.

The exact animated tiled-water representation (167,936-byte residency and 138
replacement triangles) is retired from P1. It consumed disproportionate time,
VRAM, and semantic integration risk for cosmetic animation. The 2026-07-15 DS
visual policy instead freezes exact BattleShip frame 0, fraction 114: large
128x128 RGB5A1 is 32,768 bytes and small 32x64 is 4,096 bytes. Keep original
runs 42–43 and their 12 `PROJECTED_NO_Z` triangles; preload/pin the two 36,864-
byte outputs before GO and ignore later water material animation in the DS
renderer. The source hashes are
`f3a908659547f360ec9d3b79f80aa4c5dca829cdb36975a5d3a59667d1fdf532`
and `61b0bb44aa30033d0c8e07d924f6b38ddbafa23807692eb16aab194e57457efe`.

The last measured 172,024-byte reserve remains a pre-integration reference.
Production still must prove >=128 KiB reserve and zero open/read/seek through
one-minute teardown. Device bank ownership, recognizable still-water screenshots,
and the short-window zero conversion/upload/I/O/alloc/refresh/eviction fence now
pass. M4 completion still requires the full-minute fence/reserve plus M3 timing.

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
3. run focused, static, DevFast, forensic, and Boundary checks; add Current only
   when normal launch or shared startup/runtime can be affected;
4. inspect the final status and commit only the coherent owned change set;
5. run `scripts/New-Smash64DSSnapshot.ps1 -Mode Lean` as the final project
   action and run no command afterward.

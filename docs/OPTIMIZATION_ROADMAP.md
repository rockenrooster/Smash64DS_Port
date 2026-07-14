# Optimization Roadmap — Cut G and Native Countdown Accepted; M2–M4 Active

Updated: 2026-07-14

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
12,043,264 bytes
SHA-256 385B9F051C5CBB801089C69E13D49F9E0D19C07F1E4DA19DA943772B5553FC21
```

The canonical and shipped copies match byte-for-byte. Refresh this identity
after the final build; compilation alone never proves a milestone.

The latest focused M2 A/B/A used profile-1 ROM `03950839...BEEF09B`, frames
600–607 / logic 209–216, and reported about 10.1–10.2 FPS. It is laboratory
evidence, not a canonical artifact. After rejecting and removing the treatment,
the profile-1 rebuild is `0192BAFF...AE5E9`; the user-facing profile-0 ROM above
was not rebuilt or published. Gameplay still reports two uploads / 36,864 bytes.

## Milestones

| Milestone | State | Measured boundary / target |
|---|---|---:|
| M1 — simple hardware-affine BG2 | Complete | 1,856/1,856 ticks; required 5–35K |
| M2 — AOT DS-native Mario/Fox | In progress | ledger-off 431,168/458,688; target 170–250K |
| M3 — AOT DS-native complete stage | Open | 800,608/803,456; target 150–250K |
| M4 — conversion off gameplay path | Corpus kept; streaming rejected | 322 keys / 206 outputs / period 216; target zero conversion and zero gameplay I/O |

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
- semantic counters and a successful synchronized screenshot.

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

The latest same-ROM profile-1 Mode-8 A/B/A used SHA-256
`03950839A61B...BEEF09B`, frames 600–607 / logic 209–216. Both controls were
sample-for-sample identical:

```text
                         control A1=A2       GX skeleton B
Mario                    226,112/246,272     205,344/225,536
Fox                      267,584/274,624     240,192/247,232
paired combined          493,696/520,896     445,568/472,768
matrix/root subtotal     162,208/189,568     113,632/140,736
production               243,520/243,648     242,240/242,304
absolute target       170,000 .. 250,000 ticks (detailed ledger off)
```

The whole-fighter GX palette was correct but saved only 48,128 paired ticks
(9.75%), missing the same-ROM 100K falsifier by 51,872 ticks. Both A and B used
the opt-in detailed M2 census/timers, so the relative delta is valid but the
445,568 absolute total is not comparable to the coarse 331K ceiling or the
170–250K milestone target. Exact 49 epochs, 67 fighter runs, 626 fighter
triangles, 44 cross-matrix cases, full class census, fallback accounting, and
runtime state all passed. The treatment and selector were removed; corrected
25/27 live topology remains, and the disjoint M2 ledger is available only with
`NDS_RENDERER_M2_DETAILED_LEDGER=1`. The earlier split-projection and CPU-
preorder cuts are also closed.

The next architecture is two non-overlapping packets. First, preflight corrected
25/27 joint schedules and 14/18 bindings, let GX walk/store the hierarchy, and
remove parallel CPU world/composed 4x4 construction while retaining an exact
CPU 3x3 normal/light sidecar. This should remove 130–170K. Second, compile the
49-epoch/67-run state transaction and dense-corner emitter into one owner call,
targeting another 80–120K without retrying 121 small GX lists. Profile 2 remains
the independent light/matrix/state oracle.

Falsification gate:

- a synchronized same-ROM A/B must save at least 100K combined fighter ticks;
  the absolute <=331K first-window and 170–250K promotion gates are measured
  only with `NDS_RENDERER_M2_DETAILED_LEDGER=0`;
- the chosen design must retain a credible route to the 170–250K target rather
  than polishing either measured bucket in isolation;
- production remains exactly 70 runs / 686 fast triangles, partitioned
  60+320+306 with bounded 29/0/0 fallbacks;
- full frame remains 828 triangles with class census 648/44/126/10;
- semantic, owner entry/exit, vertex-cache, GX matrix, texture/upload, and
  screenshot gates remain coherent;
- no per-frame allocation and no reserve regression.

For the hierarchy packet specifically, require matrix+light <=80K and ledger-
off combined <=331K. For the epoch/run packet, require >=60K incremental saving,
production <=120K, and final ledger-off combined 170–250K.

Promote Mode 8 to canonical profile 0 only when synchronized counters,
profile-2 oracles, captures, and natural runtime all agree.

## M3 — AOT Complete Stage Owner

Use the same whole-owner architecture after M2. Preserve BattleShip layer and
display-head order, live camera/DObj/material state, animated platforms,
effects, depth, and opaque/translucent ordering. Validate the complete owner
before its first GX mutation; unsupported state falls back for the whole owner.

Do not revive the rejected source-shaped typed stage executor or mode-9 record
executor. Preflight the entire stage before its first GX mutation, then emit
precompiled slabs only when each original callback occurs. The exact current
contract is 42 lists / 886 commands / 302 vertices / 54 runs / 202 triangles,
partitioned 66 raw, 126 no-Z, and 10 projected. Keep known projected/dynamic
records cold and fall back for the whole stage before mutation on any mismatch.

The first falsifier must save at least 300K stage ticks and reach <=500K with
P95 improved while retaining all 42/886/54/202 counts, full-frame 828 geometry,
semantic/state hashes, texture counters, order, motion, and screenshots. M3
promotion remains 150–250K after M4 removes the conversion wall.

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
Audio-adjusted reserve was only 153,184 bytes, 22,112 above the floor. The
next falsifier is a complete pre-GO, zero-I/O pair-index/palette encoding with
byte-exact oracle expansion and measured texture-VRAM ownership.

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

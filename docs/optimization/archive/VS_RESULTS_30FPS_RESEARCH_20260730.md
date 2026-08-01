# VS Results 30 FPS research

Date: 2026-07-30  
Branch researched: `codex/r2-runtime2`  
Committed baseline: `acdeec9d5a`  
Scope: read-only research; no source implementation, build, emulator run, or
acceptance claim was performed.

Three Sol Max tracks independently covered the source/runtime contract, current
hot path, optimization history, DS reference backends, and qualification plan.
Each used CodeGraph first, used opencode-scout for broad locating, and returned
`path:line` pointers that were checked against current files before this report.
Unrelated live changes in the worktree were left untouched. They include an
in-flight rematch-reboot/OAM scene-lifetime experiment, not a qualified Results
draw optimization; none of those dirty changes is evidence for a result below.

## Decision summary

The Results screen is not close to the Runtime 2 gate.

- Runtime 2 requires 30 Hz rendering while preserving 60 Hz gameplay state.
  Two VBlanks are 1,120,380 ARM9 ticks, and the representative gate is
  `P95 <= 1.12M` (`PROJECT_GOAL.md:195-247`,
  `docs/Smash64DS_Runtime2_SwitchPlan.md:128-145,193-201`).
- The current default still renders the Results wallpaper through software;
  `NDS_R2_RESULTS_AFFINE ?= 0` (`Makefile:482-489`). Its aligned steady Results
  window is approximately 10 VBlanks / 5,601,900 ticks / 6 FPS.
- The best measured R2b candidate moves the wallpaper to the hardware affine
  background. Its aligned window is exactly 7 VBlanks / 3,921,330 ticks / about
  8.55 FPS. All 40 measured intervals remain in the `5+` bucket, max 7.
- R2b therefore still needs a five-VBlank, 2,800,950-tick cut to reach the
  two-VBlank frame. That is about 71% of the current frame. Another pixel-loop
  micro-optimization cannot close it.
- R2b is necessary but not graduated: it is default-off pending the owner's
  matched-state visual approval
  (`docs/HANDOFF.md:69-87`, `docs/PERF_LEDGER.md:6967-7003`).
- The only complete symbol-level Results profile predates R2b. The next action is
  therefore measurement, not a renderer patch: visually approve R2b, rebuild
  its exact current binary, run a new Results Task37 profile, and record a real
  Results 2/3/4/5+ histogram.
- The shortest credible implementation path is a Results-native presentation
  seam: retain the affine wallpaper, replace the remaining full-screen software
  UI clear/blit/downscale/copy cycle with DS-native BG/OAM/direct-fill ownership,
  then specialize the Results-only Mario/Fox presentation only if the new
  profile still requires it. BattleShip remains authoritative for timing, state,
  reveal order, audio events, rankings, and input.

This is a presentation-architecture problem, not a loader, file-residency,
constant-division, store-width, or gameplay-rate problem.

## Contract that must survive

### Runtime 2

- Gameplay mechanics remain 60 Hz; rendering and visual fighter poses may run at
  30 Hz. Particles may run at 15-30 Hz and backgrounds at 15 Hz where practical
  (`docs/Smash64DS_Runtime2_SwitchPlan.md:128-145`).
- The target is a two-VBlank presented frame, not an average-FPS label. Report
  P50/P95 and the 2/3/4/5+ interval histogram plus maximum interval
  (`docs/Smash64DS_Runtime2_SwitchPlan.md:174-181,193-201`).
- The Results screen is a required P1 gameplay surface, not optional polish
  (`PROJECT_GOAL.md:304-341`). R2-07 explicitly includes GAME SET to Results,
  HUD, effects/audio, START-to-rematch, and the same total budget
  (`docs/Smash64DS_Runtime2_SwitchPlan.md:399-406`).
- Rendering approximations still require synchronized screenshot evidence and
  owner visual approval. State, timing, rules, ranking, and input remain source
  equivalent (`docs/Smash64DS_Runtime2_SwitchPlan.md:183-189`).

### BattleShip Results behavior

The imported source remains authoritative:

- The normal Time Results timeline begins at tic 0, reveals the wallpaper at tic
  80, creates result text/label/confetti and initializes fighters at tic 120,
  then advances fighter alpha by `0x16` per tic
  (`decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:2799-2846,3227-3269`).
- The visible UI continues to evolve after tic 120. Time-mode rows/tints appear
  at tics 180, 210, 230, 250, 270, and 290
  (`decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:2207-2233`).
- Normal Time Results accepts START only after tic 410; Stock uses 370 and No
  Contest 200. There is no timeout exit
  (`decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:266-280,2812-2845`).
- The seven winner-name/`WINS` glyphs are created once as link-29 SObjs with
  source scale, position, prim/env color, and transparency
  (`decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:1142-1225,1292-1429`).
- Not all foreground pixels are static. Tint callbacks mutate alpha while
  drawing, the wallpaper tint ejects itself, and the result bar grows to its
  final width inside its display callback
  (`decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:1643-1724,2024-2051`).
- FFA audio order is PublicWin at scene start, WinnerIs at tic 81, winner name
  at 210, and PublicExcited at 270; the winner fanfare/Results-BGM sequence starts
  at tic 120
  (`decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:308-338,2850-2870,3270-3276,3388-3391`).
- Results loads its scene file plus shared IFCommon assets, initializes rankings
  and fighters, and faces losers toward the winner
  (`decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:71-82,2786-2795,3335-3355`).

The optimization may replace how those facts are presented on DS. It may not
delete, delay, synthesize, or independently own them.

## Current draw path

1. The scene manager enters the original `mnVSResultsStartScene`; the port wraps
   scene loading, telemetry, input/rematch, and DS presentation but compiles the
   original Results translation unit
   (`src/import/battleship_mnvsresults.c:1-27,96-112,151-193`).
2. Results task setup binds the original update and draw flow. The live port loop
   performs input/controller work, `gcRunAll`, audio and telemetry, graphics-heap
   reset, `gcDrawAll`, sprite-preview commit, and platform presentation
   (`src/port/taskman_seam.c:6997-7050`).
3. BattleShip constructs the wallpaper on display link 26, winner text on link
   29, fills/tints on links 30/34/35, result bars on link 31, emblem animation on
   link 33, and native fighters through their fighter-camera links
   (`decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:615-762,982-1042,1142-1445,1643-1782,2024-2051,2402-2485`).
4. Results does not use the battle-only native OAM shortcut. Every supported
   Results SObj enters the generic software preview path
   (`src/port/sprite_preview_backend.c:2733-2858`).
5. Starting a foreground layer clears a 320x240 RGB555 staging image, each SObj
   is decoded/blended into it, the image is scaled to 256x192, and every row is
   copied into BG VRAM
   (`src/port/sprite_preview_backend.c:2547-2568,2801-2845`,
   `src/nds/nds_platform.c:450-483,689-741,765-810`).
6. R2b recognizes only the 300x220 I/4b Results wallpaper, bakes its prim/env
   combine into 16 colors, seeds the existing affine BG owner once, and removes
   that software background layer
   (`src/port/sprite_preview_backend.c:758-797,2610-2715,2753-2765`).
7. Mario and Fox are already native 3D submissions. They are not part of the
   sprite-preview blit, but their Results win/lose motions and generic camera/DL
   path still consume time.

## Performance evidence

### Phase-aligned current histogram

The committed R2b JSONs contain 41 once-per-frame `IA/8b 20x37` timestamps.
Applying the census script's next-hit delta rule
(`scripts/census-vsresults-blit.ps1:286-295`) produces 40 complete frame
intervals:

| arm | exact interval distribution | 2/3/4/5+ | max | mean ticks | approximate FPS |
|---|---:|---:|---:|---:|---:|---:|
| software control | `9x1, 10x38, 11x1` | `0/0/0/40` | 11 | 5,601,900 | 6.0 |
| R2b affine | `7x40` | `0/0/0/40` | 7 | 3,921,330 | 8.55 |
| required | P95 at 2 | dominated by 2 | report max | 1,120,380 | about 30 |

Artifact identity:

- Control ROM SHA-256
  `9013FF1CE870B4E4924082F212D41A5D23FCEF957F6B04C80A4DC20348B4995C`,
  captured 2026-07-30T12:55:43Z
  (`artifacts/performance/r207-r2b-control.json:2-12`).
- Candidate ROM SHA-256
  `7DFD8DF0E07B4381BD90EF6ACD6C821560C8DC4AD7DFACE2099E4DC3BB5FF54E`,
  captured 2026-07-30T13:29:29Z
  (`artifacts/performance/r207-r2b-candidate.json:2-12`).
- The two generated configs differ at `NDS_R2_RESULTS_AFFINE`; both are
  realtime tick-HUD/HW-triangle lab builds with the existing fast-wallpaper
  affine path enabled (`builds/build-r2b-a/nds_build_config.h`,
  `builds/build-r2b/nds_build_config.h`).

The committed documents also publish 405/281 heterogeneous costed-event VBlanks
divided by 40 as 10.125/7.025 VB and 5.85/8.43 FPS
(`docs/PERF_LEDGER.md:6974-6983`). Those FPS values do not arithmetically follow
the stated VBlank rates, and the intervals mix different breakpoint phases.
Retain them as historical experiment output, not the acceptance histogram. The
phase-aligned stamps above are the defensible current cadence evidence.

### Kept, reverted, and superseded work

| step | decision | measured result |
|---|---|---|
| R0 | baseline | 39.975 VB, 22,393,595 ticks, 1.50 FPS |
| R0c `bf40607c` | KEEP | exact reciprocal `/255`; 22.550 VB, 12,632,284 ticks, 2.66 FPS |
| R0d `dbe144bd` | KEEP | force-inline lerp; 21.525 VB, 12,058,089 ticks, 2.79 FPS |
| R0e `55c8a2c` | KEEP | 16-color palette plus paired I4 row; 10.250 VB, 5,741,947 ticks, 5.85 FPS |
| R0f `d4fa1a6` | WITHDRAW attribution | integer VBlank flooring falsely called staging clears free |
| R0g `8c98dae` | REVERT | two `strh` to one `str`: only -0.06%, with extra instruction/gate |
| R0h `b80bb31` | ANSWERED | complete per-PC profile; software compositor is 61.9% |
| R2a `8703a0b` | KEEP | glyph table lookup: -200,133 ticks/frame; gain became idle, cadence flat |
| R2b `57d25fb` | KEEP AS CANDIDATE | hardware affine wallpaper: about -1.7M ticks; default-off pending visual approval |

Exactness for R0c/R0e is exhaustively checked by
`scripts/check_sprite_lerp_exact.py` and wired through
`scripts/check-gbi-decode-fixtures.ps1`
(`docs/PERF_LEDGER.md:6847-6895`). R0h and later corrections are authoritative
over earlier R0f phase claims (`docs/PERF_LEDGER.md:6903-6965,7019-7028`).

### Last complete symbol profile: R0h, before R2b

R0h covered 397/3,390 function symbols with 0.00% unattributed over Results tics
131-171 (`docs/PERF_LEDGER.md:6921-6938`):

| owner | ticks/frame | share |
|---|---:|---:|
| `ndsDrawSObjIntoPreview` | 1,103,616 | 19.70% |
| preview downscale/commit | 974,382 | 17.39% |
| two 153,600-byte staging clears | 830,978 | 14.83% |
| two 98,304-byte BG-VRAM copies | 557,126 | 9.95% |
| software compositor subtotal | **3,466,102** | **61.9%** |
| `armWaitForIrq` | 830,260 | 14.82%, idle rather than work |
| native fighter root | 284,169 | 5.07% |
| fighter-DL family | 229,178 | 4.09% |
| remaining measured work | approximately 792,175 | approximately 14.1% |

Geometry plus the measured blit arms split the compositor into a static
background at 1,746,558 ticks and a foreground at 1,519,410 ticks
(`docs/P1_EXECUTION_BOARD.md:1900-1908`). R2b removes the background subtotal
within 0.6%, which validates its mechanism.

There is no complete post-R2b per-PC profile. The statement that the final
`IA/8b 24x37` glyph owns 85.1% of the remainder is not valid symbol attribution:
the interval measures from that breakpoint until the next frame's first blit,
so it includes the commit, fighters, camera/display work, platform wait, and any
other tail work. The R2b JSON also records `phases:false`. Use that row only as a
locator for the unpartitioned tail, never as proof that one glyph is expensive.

### Current gap

Using phase-aligned evidence:

```text
R2b current         7 VBlanks   3,921,330 ticks
required            2 VBlanks   1,120,380 ticks
remaining cut       5 VBlanks   2,800,950 ticks
```

Deleting only the pre-R2b measured foreground compositor ceiling (1,519,410)
would still leave roughly 2.4M ticks by simple sizing. Deleting that and the
pre-R2b measured fighter-render family (513,347) would still leave roughly 1.9M
before considering quantization. These cross-phase subtractions are sizing, not
a forecast; a post-R2b profile is mandatory. They do prove that a dirty/static
glyph cache or fighter tweak alone cannot be called the 30 FPS solution.

Current performance is also a lower bound for finished P1 presentation:
confetti/particle script 112 and several Results/crowd voice cues remain absent
or unqualified (`docs/BUGS.md:58-104`). They must be present when the final gate
is measured.

## What not to pursue

- Do not rebuild a fighter/asset residency system for the Results transition.
  GAME SET to `FOX WINS` is 6.10 s control / 4.85 s R2b; battle exit to first
  Results tick is 0.735 s, and the two fighter loads are 0.334 s, only 5.5% of
  perceived dead air (`docs/PERF_LEDGER.md:7005-7017`).
- Do not retry the R0g wider-store fold. It measured -0.06% and added machinery
  (`docs/PERF_LEDGER.md:6909-6919`).
- Do not optimize `armWaitForIrq`; it is slack recovered when real work is cut.
- Do not treat the I4 Results wallpaper as a CI palette-scan problem.
- Do not trust sub-VBlank phase rows as ownership. R0f already paid for that
  mistake (`docs/PERF_LEDGER.md:7019-7028`).
- Do not lower gameplay mechanics to 30 Hz first. The plan permits 30 Hz
  presentation and identifies rendering machinery, not gameplay, as the owner.
- Do not add another framebuffer. R2b deliberately reuses the existing decode
  buffer, and the DS memory/VRAM budget has no justification for a third
  153,600-byte image.
- Do not claim that retaining the whole foreground is automatically exact. The
  seven text glyphs are static, but the foreground also contains fading tints,
  growing bars, and later-created rows. Invalidation must follow source state.

## Ranked path to the gate

### Gate 0: graduate the existing affine background

Show the owner synchronized, matched-Results-tic versions of:

- `artifacts/visibility/2026-07-30_r207-r2b-results-control-software.png`
- `artifacts/visibility/2026-07-30_r207-r2b-results-candidate-affine.png`
- the rejected letterbox capture only as failure provenance.

KEEP R2b and flip its default only on owner approval. A same-wall-clock pair is
invalid because the faster arm reaches a different source tic. If the owner
rejects a visible delta, correct that exact mapping delta and repeat one A/B; do
not discard the measured architecture without identifying the rejected pixel
contract.

### Gate 1: establish the real R2b owner

Before another implementation:

1. Rebuild one exact current R2b ROM/ELF and record all flags and SHA-256.
2. Run `scripts/run-task37-profile-census.ps1 -Scene Results -Frames 40` over
   Results tics 131+; reuse the existing profiler window rather than adding a
   new profiler (`scripts/run-task37-profile-census.ps1:21-59`).
3. Add or reuse the battle presentation histogram recorder for Results. The
   Results task loop currently reaches `ndsPlatformEndFrame` without a dedicated
   Results presented-frame interval recorder (`src/port/taskman_seam.c:6997-7050`).
   Do not create competing pacing state.
4. Report per-symbol and per-region P50/P95, the 2/3/4/5+ histogram, max interval,
   idle, bytes cleared/copied, GObj/camera/display counts, and exact scene tic.
5. Confirm the profile accounts for at least 99% of the R2b frame before choosing
   the next owner.

Kill this phase if the profile is not the exact visually accepted R2b binary or
if its frame/tic window differs from the cadence capture.

### Gate 2: one bounded Results-native UI experiment

The first structural candidate should replace the Results UI presentation, not
the Results state machine:

- Keep the source's GObj/process/tic behavior authoritative.
- Keep the R2b affine wallpaper on its existing BG owner.
- Decode/upload the small fixed Results glyph asset set once. Present static
  glyphs through retained BG/OAM or an existing DS-native sprite path instead of
  redrawing them into a 320x240 buffer.
- Translate changing tints and bars into bounded DS hardware state/direct fills,
  preserving their source alpha/width progression and display ordering.
- Apply row/header/place creation exactly on the original tics. Retain only a
  layer whose semantic state is unchanged; invalidate on create/eject, position,
  scale, bitmap, prim/env color, alpha, ordering, or scene epoch change.
- Reuse the existing wallpaper/layer epoch and OAM allocation patterns. Do not
  introduce a second compositor or generic 2D display-list interpreter.
- Eliminate the foreground staging clear, per-pixel blend, 320x240 to 256x192
  scale, and 98,304-byte VRAM copy when the hardware owner succeeds.

The measured pre-R2b ceiling is about 1.52M ticks/frame, so this experiment is a
major KEEP candidate but not a promised gate closure. KEEP any repeatable,
source-correct structural saving. REVERT on stale tint/bar state, missed timed
row, ordering error, fallback to the software layer, new frame spike, or material
memory/VRAM growth without the expected saving.

### Gate 3: specialize Results fighters only if measured

If the post-UI profile still ranks Results fighter presentation high:

- preserve the original win/lose statuses, animation frame progression, facing,
  scale, placement, lighting/fade, and 60 Hz source state;
- evaluate/present the exact Mario/Fox Results pose at 30 Hz using the existing
  DS-native fighter machinery or a generated Results-only specialization;
- do not create a second fighter state machine or approximate status selection;
- require natural Mario-win and Fox-win coverage before KEEP.

The pre-R2b measured renderer ceiling is about 513K ticks/frame. It cannot close
the current gap alone. Do not start it unless the accepted R2b/UI profile proves
the same ownership remains.

### Gate 4: cut the new top generic owner, then stop

UI plus fighter sizing still leaves generic camera/display/platform work. After
Gate 3, re-profile and remove only the new measured owner. Likely bounded seams
are redundant Results cameras/display-link traversal or presentation work made
obsolete by the native UI path, but those are hypotheses until the accepted
binary profiles them.

Stop as soon as P95 is at two VBlanks with correct content. Do not broaden the
work into a new menu renderer.

## DS reference evidence

The reference engines support retained hardware ownership rather than another
software frame composition pass:

- `sm64-nds` allocates sprite graphics once, writes OAM entries, and flushes OAM
  at frame end (`decomp/sm64-nds/src/nds/nds_renderer.c:1061-1079,1164-1169,1276-1284`).
- `sm64ds-decomp` loads compressed assets directly into BG character/screen VRAM
  and loads OBJ palettes separately
  (`decomp/sm64ds-decomp/src/func_0200f13c.cpp:17-32`).
- Its OAM path batches reset, render, flush, and load rather than rebuilding a
  full software framebuffer (`decomp/sm64ds-decomp/src/func_02034b40.c:20-76`).
- Its texture path programs DS texture/palette registers and a bounded texture
  matrix directly (`decomp/sm64ds-decomp/src/func_0204af3c.c:31-71`).

These are patterns, not code to transplant blindly. Smash64DS should reuse its
existing BG/OAM/texture owners and source-derived Results assets.

## Qualification matrix

Every candidate uses one selector and one synchronized eight-frame A/B on the
same exact ROM configuration and Results state.

### Identity and timing

- Record commit, dirty state, ROM/ELF SHA-256, generated build flags, DLDI state,
  emulator build/configuration, Results tic window, and capture time.
- Report P50/P95, exact 2/3/4/5+ counts, and max interval. The final reference
  run needs P95 in the two-VBlank bucket; rare exceptions must be identified,
  not hidden in an average.
- Record Results update count, presented frames, GObj/camera/display calls,
  SObj count/semantic hash, fighter place/status/motion, and source event tics.
- Report clears, scaled pixels, BG2/BG3 copy bytes, OAM count/matrices, texture
  uploads/fallbacks, GX waits, and idle.

### Fidelity and behavior

- Capture matched-tic control/candidate screenshots after wallpaper reveal,
  fighter/text reveal, tint/bar animation, final rows, and settled Results.
- Run deterministic screenshot analysis and state the visible delta/fidelity
  budget. The owner is the final visual oracle.
- Prove both Mario-win and Fox-win text, colors, rankings, positions, winner/loser
  poses, source fades, emblem/tints/bars, confetti, and audio order.
- Press START naturally after tic 410. Complete the second match without stale
  layer, corruption, hang, or leaked VRAM/OAM state.

### Memory and hardware

- Report BSS/map delta, taskman/graphics heap high-water, texture bytes, and exact
  VRAM bank ownership.
- A retained-BG path must allocate no third 153,600-byte buffer.
- An OAM path must remain within 128 objects and 32 affine matrices, with explicit
  Bank E/OBJ capacity and scene-lifetime invalidation evidence.
- Treat post-ready pixel writes, cache fallback, unmapped sprite formats,
  nondeterminism, flashes, and unexplained state changes as failures.

### Verifiers and promotion

- Run the smallest focused sprite/Results checker while iterating, including the
  existing exhaustive lerp fixture where relevant.
- Run one widest `Latest` verifier for a kept shared scene/backend checkpoint.
  Do not stack Boundary and Latest when Latest covers the change.
- KEEP every repeatable correctness-preserving gain. REVERT an added-code or
  added-memory experiment below the 10K floor, any fidelity/state regression, or
  any P95 spike that offsets its P50 win.
- Device A/B is acceptance, not the first experiment. When the gate is met on the
  accuracy melonDS reference, report the device 2/3/4/5+ histogram and maximum
  interval, then obtain owner visual/listen and retail-hardware acceptance.

## Evidence gaps that must remain explicit

- No complete per-PC profile exists for the R2b affine binary.
- R2b has no owner visual approval and remains default-off.
- Existing Results cadence artifacts are breakpoint censuses, not the permanent
  Results P95/histogram instrument required for final acceptance.
- The root user-facing ROM is older than current source and is not an acceptance
  artifact for this research (`docs/P1_EXECUTION_BOARD.md:22-41`).
- Missing confetti and Results/crowd cues mean current performance is a lower
  bound for finished P1 presentation.
- No device acceptance run is scheduled while the gate is unmet
  (`docs/PERF_LEDGER.md:7030-7031`).
- The source comment that still says the transition is approximately 30 seconds
  is stale; measured current values are 6.10/4.85 seconds
  (`src/import/battleship_mnvsresults.c:54-61`,
  `docs/PERF_LEDGER.md:7005-7017`).

## Concrete next task block

1. Obtain owner approval/rejection on the existing matched-state R2b images.
2. If approved, flip only `NDS_R2_RESULTS_AFFINE`, rebuild an exact current
   flag-identical candidate/control pair, and record hashes.
3. Add/reuse a Results presentation histogram; run the 40-frame Results Task37
   profile on the accepted R2b binary.
4. Publish the corrected post-R2b symbol/region ownership and 2/3/4/5+ histogram.
5. Run one Results-native UI experiment with the 2,800,950-tick required saving
   written beside its measured ceiling. KEEP or REVERT decisively.
6. Re-profile. Add Results-specific fighter presentation only if it is now the
   measured blocker.
7. Restore all missing Results content, complete START-to-rematch, run `Latest`,
   then perform owner/device acceptance.

Nothing before step 4 should be described as the final 30 FPS fix. Nothing after
the gate should be added without a new measured blocker.

# P8 -- Stability, RAM, CPU and readability

Owner brief (2026-10-22): find RAM savings, bugs and crashes/stalls, and run
refactoring / human-readability loops, so the game plays more stably and faster
and is easier to develop on. Later the same day: CPU hunting beside the RAM
hunting (loop P).

Live state and the cursor: [P8_STATUS.md](P8_STATUS.md).
`PROJECT_GOAL.md` still owns fidelity and acceptance. Nothing in this plan
changes a game rule, a visual or a sound without the owner's go.

## 0. Starting point (measured 2026-10-22)

### Memory (ship play ROM `builds/play-p7z`, `arm-none-eabi-size -A` and its map)

| region | bytes | biggest owners |
|---|---|---|
| `.main` code + rodata | 1,865,184 | `nds_renderer.o` 579,980 (346,528 ARM code, 233,452 rodata); `scene_backend.o` 326,973 (259,822 Thumb code); merged string literals 41,928; ~440 objects under 31 KB each |
| `.main.rw` | 175,580 | `battleship_ftdata.o` 37,112; `nds_p4_data.o` 18,996; `nds_renderer.o` 16,747; `nds_particle_banks.o` 14,416 |
| `.main.bss` | 875,208 | `nds_audio_fgm.o` 204,484 (163,840 cue cache); `nds_renderer.o` 183,693; `scene_backend.o` 162,980; `gSYFramebufferSets` 147,840 |
| frontend overlays | 423,040 | select 189,856; frontend 121,888; 1P head parts; Results; Training |
| ITCM | 32,728 of 32,768 | full: a new ITCM byte breaks the ship link |
| DTCM | 12,244 of 16,384 | includes the 6 KB hot stack; the system stacks share the rest |

About 0.8 MB of arena sits above the image, and a battle also borrows
347,520 B of the overlay tail. Every byte the battle-resident image loses is a
byte of battle heap, and the heavy four-content mixes are memory-bound.

**Already reused -- do not rediscover:** the framebuffer serves the fighter
packets in battle; the cue cache lends its tail; the overlay tail is lent to
VS, 1P, bonus and Training battles; stages' and articles' dead geometry bytes
are lent; the 1P head lends its idle parts; polygon models load compact; a
stage program yields to the heap floor; 3-4 player CPUs load only their low
image. Frontend code already lives in overlays (about 25 KB of
frontend-named objects remain in `.main`).

**Resident whatever the match holds** (name-based census; R0 replaces it):
about 400 KB of fighter-named, 250 KB entry/effect-named, 240 KB stage-named
and 130 KB item-named symbols; ~150 KB of per-fighter object files for 32
fighters while a match plays at most four; 901 NitroFS path strings
(36,439 B of the 41,928 B of merged string literals; factoring the directory
prefixes alone saves ~19 KB); two printf engines (`_vfiprintf_r`,
`_svfiprintf_r`, 5.8 KB each); ~30 KB of code and ~10 KB of bss named as
diagnostics. The frame-summary recorders (`nds_renderer_preamble.c`) cost
~1,300 ticks a frame and feed only dormant verifiers (a perf cut, not RAM).
Symbols inside `.ovl.frontend.*` are already lent to VS battles (1P battles
lend only the tail after `__nds_frontend_1p_end`): check the map before
counting a symbol as battle RAM.

### Performance (`build-sweep-s56`, worst-case search, 1,960 frames)

- The cast passes 1,250,000 / 95% on all 16 VS stages (worst Saffron City,
  1,208,064).
- The heavy roster (Crash, Sonic, Mr. Game & Watch, Palutena) misses
  1,250,000 / 90% on 11 of 16 stages: Hyrule Temple 1,609,344 (80.7%, segment
  0 of its program only, and that by 36 B of slack), Congo Jungle 1,518,976,
  Saffron City 1,470,144, Sector Z 1,459,584. A whole program is worth a lot:
  Hyrule Temple's mix 13, 10, 16, 3 went 1,595,136 -> 1,381,504 when its whole
  program loaded (`s44`).
- The four heavy mixes: 24 of 64 over the stress target; lowest heap 25,604.
- Memory is the limit: Hyrule Temple drawn uncompiled cost ~550K profile
  cycles a frame in mix A (`s44p`); the motion arena missed 608 times in mix A;
  lean materializations own most of the P95 set.

### Stability

`docs/STABILITY_AUDIT.md` (2026-10-20): every mode on every fighter, 0 halts.
Since then the readability pass removed ~47,000 lines (A, B-1..B-4b) with
battle replay digests as proof; menus, the 1P campaign, bonus, Training and
wireless were not walked again. Open rows in `docs/BUG_NOTES.md`: Results
freeze after a Yoshi four-player win (not reproduced); Master Hand stiff
before the fight (not reproduced); Final Destination's shooting stars and boss
backgrounds; Palutena's dress darker on Results; the client lagging the host
on hardware (a 2 GB and an 8 GB card). `docs/BACKLOG.md`: Results confetti
(owner-deferred). The ship ROM already has a freeze report (exception handler,
battle-frame watchdog, breadcrumbs: `nds_freeze_diagnostics.c`).

### Readability and tooling

- Done: A (267 fixed flags), B-1..B-4b (dead statics, discarded externs, the
  harness `#if`s, the P1 proof code). C started: `nds_renderer_native_common.c`
  split into slices (`7f0dca28600`).
- 13 port-owned files are still over 4,000 lines (section 3).
- The live build and probe tools are not in the repository: 617 scripts sit in
  two session scratchpads under `%TEMP%` (`31e5c78d-...`, `4aa2b87d-...`),
  where a temp cleanup would delete them.
- `AGENTS.md`, `docs/README.md` and `docs/HANDOFF.md` still route to the P2
  board as the live queue.

## 1. The loop

Every turn:

1. **Intake.** Read `docs/BUGS.md`, `docs/PLAYTEST_NOTES.md` and
   `docs/OWNER_QUESTIONS.md` for new owner text. An owner row goes to the top.
2. **Pick** the first open item in `P8_STATUS.md`, in this order: a crash,
   freeze or regression any run found; an owner row; the current phase item.
   After D, RAM (R) and CPU (P) batches alternate, the larger expected gain
   over risk first.
3. **One batch:** one rule or one lever, everything it touches, nothing else.
4. **Prove** it with the matching row of the table below.
5. **Commit** with explicit paths and log it: the `P8_STATUS.md` row;
   deletions also in `OLD_MACHINERY_DELETED.md`; measurements in
   `PERF_LEDGER.md` or the phase doc; bug evidence in `BUG_NOTES.md`.
6. **Background:** keep one probe campaign running in the runner slots while
   code batches go on (emulator runs do not block edits; builds do).

**Checkpoint** every five batches and at each phase end: S1-short (section 2),
the short worst-case sweep (heavy roster and one mix on Hyrule Temple, Congo
Jungle, Saffron City; the cast on Saffron City) and a play ROM for the owner.

### Proof by kind of change

| change | proof |
|---|---|
| text only: split, rename, comment, move | the touched TUs preprocess byte-identical in the five live configs (`pp_run.py`), or objects identical (`obj_cmp.py`); host tests |
| deletion | compiles in the five configs; no new `-Wreturn-type` (`warn_diff.py`); `obj_cmp` / `obj_secdiff`; gate ROM symbol diff; replay digests on three configs; the ship play ROM links (ITCM) |
| RAM lever | the ledger delta (R0) matches the prediction; the new path's engagement counter is positive; digests identical; frame-0 heap on the heavy mixes up by the bytes; WORK P95 no worse; the affected scenes walked; 0 native failures |
| bug fix | `BUG_FIXING_PROCESS.md`: reproduction, fix, witness on the failing input, the nearest regression probe |
| performance | worst-case search on the ship layout; digests; the share within two VBlanks |

## 2. Loop S -- bugs, crashes, stalls

A **crash** is a data abort, undefined instruction, malloc-overflow or objman
halt. A **freeze** is no presented frame for 2 s. A **stall** is a frame past
two VBlanks outside the gate's known set, or a scene load slower than its
ledger entry (S4).

**S1 Regression net (first).** Re-run the audit matrix on HEAD's ship layout,
because A and B deleted ~47,000 lines with only battle digests behind them:

- walk ROM: the 1P campaign for every fighter (32), both bonus practices
  (64 boards), Training for every fighter (32), the VS select (3D previews) to
  Results and back for 20 laps, the SSS on every stage;
- soak ROM: the 5-minute four-player all-items (Very High) matches m1-m8;
- melonDS-mp: two and four consoles, the four heavy mixes, Results, rematch.

Record per run: halts, native failures, lean declines, ledger refusals, heap
low-water, desyncs. **S1-short** (the checkpoint set): the campaign for Mario,
Kirby, Bowser, Sonic, Snake and Palutena; soak m5 and m6; one wireless heavy
mix; 5 rematch laps.

**S2 Fault injection** (lab builds only; never in the gate or ship ROMs):

- **Heap squeeze:** a lab flag takes 16 KB from the heap at each battle's
  first frame. The 25,600 B floor exists so that this survives: the heavy
  roster and the four mixes on Hyrule Temple, Congo Jungle, Saffron City,
  Sector Z and Yoshi's Island must finish without a halt, the stage program's
  yield and the lean caches giving way first.
- **Slow storage:** a lab flag adds a fixed delay to every card read, at
  levels that bracket slow SD cards. Measure select and stage load times,
  frames pushed past two VBlanks by motion and audio reads, and the wireless
  start (the owner's "client lags the host").
- **Long soak:** 30-minute four-player matches on the two worst stages, the
  heavy mix and the cast. Plot the source pools' growth (14.8-19.8 KB per
  5 minutes today): it must flatten; a straight line is a leak.
- **Rematch laps:** 50 VS rounds through Results; heap and pool levels at each
  round's first frame must not drift.
- **Seeded fuzz:** a walk route that picks roster, costumes, stage, items,
  rules and match length from a seed, plays, goes through Results and picks
  again; unattended on spare slots; every failure logged with its seed.

**S3 Static analysis.**

- GCC `-fanalyzer` per port TU (`src/nds`, `src/port`; never `decomp/`),
  triaged by class: NULL use after a failable allocation, use after free,
  out-of-bounds, leaks across scene changes. Fix the real ones in batches.
- Unchecked allocations: every port caller of `syTaskmanMalloc`,
  `ndsSceneAssetTryAlloc` and the reloc loaders either checks the result or is
  proved unable to fail; required setup fails cleanly, never a NULL write
  (`RAM_RECOVERY_PLAN.md`).
- Stacks: a `-fstack-usage` table for the port TUs, and the DTCM hot stack's
  paint (`coroutine.c`) extended to every coroutine stack (GObj 4 KB, service
  16 KB, relay 4 KB) and the system stacks; read the high-water in S1 and S2.
  Every stack keeps at least 25% free in the worst run; oversized ones shrink
  in R5.

**S4 Stalls.**

- A load ledger: time from a scene's exit to the next scene's first presented
  frame, for every transition the walk takes, on the ship layout (and under
  S2's slow storage). Cut the slowest by prefetch or by moving work off the
  critical path.
- Synchronous storage waits in battle: `ndsAudioStorageReadSync` spins (7.4M
  ticks in the `s48p` heavy profile) and motion reads after GO. Read
  asynchronously or prefetch at the status change; the criterion stays "no
  frame pushed past two VBlanks" (STRD).
- Menu frame spikes: the CSS cover streaming, SSS previews, Results.

**S5 Freeze report gaps.** The ship ROM's freeze report is what the owner sees
on hardware, so it must cover every scene: check it trips in menus, during
loads and before the first presented frame (`KNOWN_ISSUES.md` names that gap),
and that it shows the build identity, scene, stage, roster and heap free beside
PC/LR. Test with `gNdsFreezeDiagnosticsForceTrip` in each scene class.

**S6 Open rows.** Each gets a reproduction attempt with S2's tools before it is
closed or parked: the Results freeze after a Yoshi win; Master Hand stiff;
Final Destination's backgrounds; Palutena's dress darker on Results; the
wireless start on a slow card. Confetti stays parked until the owner says go.

**Exit S:** S1's full matrix and S2's runs end with 0 crashes, 0 freezes,
0 desyncs and 0 native failures; heap low-water >= 25,600 everywhere on the
ship layout; every stack >= 25% free; pool growth flat; every owner row fixed
or answered.

## 3. Loop C -- readability and ease of development

Owner order (2026-10-21): readability C and D before the RAM hunt. Bugs and
crashes still go first.

**C Split the giant files** as consecutive slices of the same translation unit
(preprocess-identical), each slice headed by what it holds. First delete what
is dead in a file (B's tool chain), so no slice is cut through dead code.
Source-derived `src/import` files and decomp-shaped headers keep the source's
layout. Port-owned files over 4,000 lines:

| file | lines |
|---|---|
| `src/port/reloc_backend_assets.c` | 18,245 |
| `src/nds/nds_renderer_textures_effects.c` | 16,293 |
| `src/port/renderer_adapter_stage.c` | 14,938 |
| `src/port/reloc_backend_compat_shims.c` | 11,232 |
| `src/port/renderer_adapter_matrix.c` | 8,791 |
| `include/nds/nds_startup.h` (604 `#define`s of pass and blocker codes) | 8,601 |
| `src/port/reloc_backend_movement.c` | 6,623 |
| `src/nds/nds_renderer_assets.c` | 6,556 |
| `src/nds/nds_ifcommon_oam.c` | 5,998 |
| `src/port/reloc_backend_mp_collision.c` | 5,804 |
| `src/nds/nds_menu_shell_css.c` | 5,147 |
| `src/port/taskman_seam_core.c` | 4,587 |
| `src/port/diagnostics_renderer_census.c` (delete if only dormant tools read it) | 4,129 |

**D Comments.** Every port file opens with what it owns, who calls it, the
contracts it keeps (memory, timing, what it must never do) and where its
generated inputs come from. Comment the non-obvious invariants where they
live: DMA ownership, lent spans, the heap floor, ITCM placement. No code
change.

**E Dormant machinery.** The P1/P2 verifier framework listed under "Still to
do" in `OLD_MACHINERY_DELETED.md` (owner OK, `OWNER_QUESTIONS.md` 6): keep
`docs/VERIFYING.md`'s live parts; never touch the dirty
`check-gbi-decode-fixtures.ps1`. Then the legacy fighter executor.

**F Names.** Replace history names in live code with what the thing is:
`NDS_TASK27_*`, `task37/44/49`, `R2_*` and `P2_*` prefixes where the code is
not about that phase, `compat_shims`, `battle_playable_*`. One family a batch;
objects identical apart from symbol names.

**G The Makefile (8,361 lines).** Delete the knobs no live config sets (A's
follow-up), name the ship map (`builds/<name>/.map` today), then split by
topic into `mk/*.mk` (P4, P5, ITCM placement, generated rules, harnesses, ROM
targets). Proof: `make -n` output identical for every live target and config.

**H Tools into the repository.** Promote the tools this plan runs into
`scripts/tools/` with one README: the gate ROM build (`build_ship_sweep`), the
play ROM build, the worst-case sweep (`p95sweep_x`), the digest compare, the
heap census, the lean trace, the audit drivers (campaign, soak, bonus,
training, VS walk, Metal Mario), the wireless lag driver, the capture tools,
and the readability chain (`pp_run`, `compile_check`, `warn_diff`, `obj_cmp`,
`obj_secdiff`, `unifdef_multi`, `const_pred`, `dead_tail`, `unref_*`). Same
parameter names across tools; no session paths. Then
`scripts/tools/quick_check.ps1`: host tests, the five-config compile, the ship
link, the gate ROM and three replay digests, in one command.

**I Docs.** One live route for this phase (`P8_STATUS.md`): `AGENTS.md` and
`HANDOFF.md` point at it (owner yes, Q1); `docs/README.md`'s routing table
next (it carries uncommitted edits: merge, never overwrite). Reconcile
`docs/P7/P7_STATUS.md`'s in-progress rows with their evidence (the audit, the
commits): the owner expects P7 complete, so a row is closed with its proof or
its gap becomes a bug row. Move the closed parts of `BUG_NOTES.md` (7,684 lines) and
`PERF_LEDGER.md` (9,216) to `docs/archive/`, leaving the open rows and an
index.

**Exit C:** no port-owned source over 4,000 lines; every port file headed; E
deleted; F's families renamed; the Makefile split with `make -n` identity; the
tools in `scripts/tools/` and `quick_check.ps1` green on HEAD; the docs route
to P8.

## 4. Loop R -- RAM (after D)

Rules (`RAM_RECOVERY_PLAN.md`): recover bytes at their owning representation;
prove the live set; no hidden output; no new streaming after GO to disguise a
static saving. A lever that does not measure is recorded as refuted, with its
numbers, and not reopened without a new invalidator.

**R0 The ledger (first).** `scripts/tools/ram_ledger.py` reads a map and its
ELF and prints tables by output section, object, symbol and family (fighter,
stage, item, entry/effect, menu, diagnostics, common), plus the diff of two
builds. It reports three totals: the image (`.main` + `.main.rw` +
`.main.bss`), **battle-resident** (the image plus the overlay bytes a VS
battle keeps) and menu-resident. A runtime census adds each big static
buffer's peak use per scene class (menus, VS battle, 1P, bonus, Training,
Results): a buffer's size is not its use. Baseline: `play-p7z`.

Levers, ranked by expected bytes over risk (estimates from the census; R0
replaces them):

| # | lever | expected | notes |
|---|---|---|---|
| R1 | Cold code in the ARM renderer unit built as Thumb `-Os` (`#pragma GCC push_options`, `target("thumb")`, `optimize("Os")` around cold slices: setup, validate, bind, learn, prepare); cold parts of `scene_backend` `-Os` | 30-60 KB | hot slices untouched; Phase C's slices make the boundary clean; WORK P95 must not move |
| R2 | NitroFS paths: 901 `nitro:/...` strings (from the generated X-macro rows in `nds_fighter_production.generated.h` and `sNdsRelocAssets`) stored as a directory index plus leaf names, or formatted from file ids | ~19 KB (prefixes) to ~30 KB (ids) | generator and loader change; the consumers `fopen`/`strcmp` the path |
| R3 | Entry-effect data loaded for the match's fighters and released at GO | 40-60 KB | `sNdsEntryEffect*` tables resident for every fighter; entry corners already retire at GO |
| R4 | Diagnostics out of the ship image (startup diagnostics, debug HUD, census and frame-summary recorders), one printf engine instead of two, keeping the freeze report and the counters the gate reads | 15-35 KB, and ~1,300 ticks a frame | the gate ROM keeps what it measures |
| R5 | Static buffers right-sized from R0's peaks, or scene-scoped loans like the cue cache's tail: renderer owner materials 28.8 KB, texture scratch 32 KB, refresh-large 16 KB, stage workspace 15.8 KB, owner workspace 12.4 KB, cue entry tables 35 KB, oversized stacks (S3) | 20-60 KB | peaks per scene class plus overlap |
| R6 | Residual frontend code to overlays (`nds_results_oam`, `nds_net_lobby`, staff roll data, opening data) | 15-25 KB | the overlay tail is lent to battles |
| R7 | The original fighters' native owner tables (`nds_native_fighter_owner.generated.inc`) as per-fighter files loaded with the fighter, as the P4 donors' are | 60-100 KB in a 4P match | generator change; tables only |
| R8 | Stage-specific renderer code and tables (Dream Land's Whispy, Yoshi's Island's clouds, Congo Jungle's barrel, Mushroom Kingdom's POW and Piranha Plants, ...) in one battle stage overlay | 30-60 KB | one stage per battle; ROM grows, which is allowed |
| R9 | P4 content data (`nds_p4_data`: 26 KB rodata + 19 KB rw) per content | 25-35 KB | loaded with the content |
| R10 | Spike (owner yes, Q2): per-fighter code in four overlay slots, each fighter linked once per slot | 100-200 KB in a 4P match | architecture change; only if R1-R9 leave the heavy mixes short |

**Spend** (loop P): recovered bytes go, in order, to (1) Hyrule Temple's whole
stage program for the heavy roster and the mixes (segment 0 or one segment
today; R0 measures the shortfall), then the other memory-bound programs
(Congo Jungle, Saffron City, Sector Z); (2) lean spares in the heavy mixes;
(3) the motion arena. Each
spend is measured on the worst-case set; a spend that does not move P95 goes
back to margin.

**Exit R:** battle-resident at least 128 KB under `play-p7z`'s, or every lever
R1-R9 landed or refuted with numbers; Hyrule Temple keeps its whole program
with the heavy roster and the four mixes; the floor and the digests hold.

## 5. Loop P -- CPU hunting (beside R)

Same timing as R: the cuts start after D and alternate with R's batches;
P0's profiling changes no source and may run in the background earlier.
Every CPU lever keeps gameplay identical (replay digests) and, where the draw
path changes, frames pixel-identical; a visible change needs the owner
(`PROJECT_GOAL.md`'s sacrifice order). R's spends are measured here too.

**P0 The CPU ledger (first).** A profile census of the worst-case set on a
ship-layout profiling ROM: the heavy roster on Hyrule Temple, Congo Jungle,
Saffron City and Sector Z; mix A on Hyrule Temple and Saffron City; the cast
on Saffron City. Split each run into the P95 band (frames P90-P97) and the
median band per symbol group (`run-task37-profile-census.ps1`,
`band_groups.py`, `band_all.py`, the lean trace, STRD; promoted under H).
Output: who owns the band's premium over the median, per stage and roster;
the same for the menus (the CSS with four previews, SSS, Results) and for
scene loads (S4's ledger). Re-run after every five P batches.

**Known owners today** (P0 re-ranks them): lean materializations (heavy Congo
Jungle: 77 in a 900-frame match, 48 of them repeats, no spare); status changes
(~60K ticks each: the motion parse, the status switch, the new motion's load;
~1 a band frame against 0.25 at the median); hit detection (21-30% of the
over-gate premium, 2026-10-07); stages drawn uncompiled (~290K profile cycles
a frame on Congo Jungle, ~550K on Hyrule Temple in the heavy profiles); Hyrule
Temple's floor queries; card reads (motion clips; the audio read spin, 7.4M
ticks a heavy match); the frame-summary recorders (~1,300 ticks a frame,
feeding only dormant verifiers).

**Tried and refuted -- do not redo without a new invalidator:** the uncompiled
no-Z translated painter and coloured entry corners (`s51`/`s52`, no P95 gain);
event32 prenormalize at effect init (Congo Jungle's digest diverged); a
per-fighter hurtbox epoch (digests diverged, saved nothing); the stage matrix
memo (`s28`); whole-subsystem soft-float passes (slower, parked 2026-10-06);
Hyrule Temple decimation (nothing to take).

| # | lever | target | notes |
|---|---|---|---|
| P1 | Frame-summary recorders and other per-frame instrumentation off the ship and gate hot path (the gate ring stays) | ~1,300+ ticks every frame | pairs with R4 |
| P2 | Lean materializations: spares from R's spend, fewer repeats (a learned variant kept across a status's re-entry), a cheaper materialize | the band's top owner in the heavy mixes | lean trace per match |
| P3 | Status changes: each motion's script header and event stream parsed once at load, a table-driven status switch, the new motion's first frame without a re-parse | ~60K a change, ~1 a band frame | exact: digests identical |
| P4 | Hit detection: a conservative broad phase (bounding spheres per fighter, weapon and item, from the same positions) in front of the source's exact tests | 21-30% of the band premium; item-dense scenes (Pay Day's coins) | same results, fewer tests |
| P5 | Stage draws: R's spends (whole programs), and where a program cannot fit, the segment choice weighted by measured cost | uncompiled Congo Jungle ~290K, Hyrule Temple ~550K a frame | with R |
| P6 | Collision: Hyrule Temple's floor queries (sweep keys, topology cache), per-frame work shared between fighters | Hyrule Temple's band | the `s28` memo is refuted |
| P7 | Storage waits off the frame: asynchronous audio reads instead of the spin, motion prefetch at the status change (S4) | 7.4M ticks a heavy match; post-GO reads | the STRD criterion holds |
| P8 | Code placement from P0's band: ITCM re-chosen by ticks saved per byte (it is full: swap, never add), hot data in DTCM, the band's hot functions contiguous for the 8 KB instruction cache, the source's hottest Thumb functions built ARM (`battleship_gmcollision.o` as ARM was worth -511,174 ticks in its census) | broad | the counterpart of R1 |
| P9 | Soft-float in the band: the hottest float paths to fixed point or the hardware divider and square root, one path at a time | per path | whole-subsystem passes were slower |
| P10 | Menus and loads: the CSS with four previews, SSS and Results WORK; scene-load CPU (relocation, normalization, decompression) from S4's ledger | menu frames within two VBlanks; shorter loads | |
| P11 | Hyrule Temple's half-res textures (owner-approved 2026-10-21, gkind 43 only) | Hyrule Temple's heavy roster | only if it still misses after its spend |
| P12 | Lower update rates where `PROJECT_GOAL.md` allows them (particles, background animation) | | visible: owner's yes first, and only after P1-P11 |

**Exit P:** the heavy roster and the four mixes meet the owner's stress target
(<= 1,250,000 and >= 90% within two VBlanks) on every VS stage, or every lever
P1-P11 landed or refuted with numbers; the cast keeps <= 1,250,000 / >= 95% on
all 16 VS stages with no stage worse than `s56`; replay digests identical;
no frame pushed past two VBlanks by a motion read. Record the over-target
counts at every checkpoint (`s56`: heavy 11 of 16, mixes 24 of 64).

## 6. Done

1. Exit S, on the final HEAD.
2. Exit C.
3. Exit R.
4. Exit P.
5. A final play ROM, the full S1 matrix and the worst-case search on the final
   HEAD, recorded in `P8_STATUS.md`.

## 7. Owner decisions (answered 2026-10-22)

- **Q1** Route `AGENTS.md`'s "Continue, do not restart" section to
  `docs/P8/P8_STATUS.md` instead of the P2 board? **Yes.** Done, with
  `docs/HANDOFF.md`'s route.
- **Q2** If R1-R9 leave the heavy mixes short, spike R10 (per-fighter code in
  four overlay slots; ROM grows about 1 MB; nothing visible changes)? **Yes.**
- **Q3** The P7 items still marked in progress (U3-U5's unlock chain, U9's
  layout answer, C5's probe classes)? **Only as bugs:** "P7 should probably be
  complete now." Item I reconciles `P7_STATUS.md`'s rows with their evidence;
  a real gap becomes a bug row.

## 8. Goal block

```
/goal run docs/P8/P8_PLAN.md (stability, readability, RAM and CPU hunting) as fast as possible; docs/P8/P8_STATUS.md is the cursor.
Done when:
Every owner row in BUGS.md and PLAYTEST_NOTES.md is fixed.
Stability matrix on the final HEAD (ship layout): every mode x every fighter, 30-min 4P all-items soaks, 2-4 console heavy mixes, 50 rematch laps, seeded fuzz, 16 KB heap-squeeze and slow-SD runs: 0 crashes, 0 freezes, 0 desyncs, 0 native failures; heap low-water >= 25,600; every stack >= 25% free; pool growth flat.
No port-owned source over 4,000 lines; every port file has a purpose header; history names gone; Makefile split; live tools in scripts/tools with quick_check.ps1 green.
Battle-resident RAM >= 128 KB below play-p7z (or every lever in P8_PLAN section 4 settled with numbers); Hyrule Temple keeps its whole stage program with the heavy roster and the four mixes.
Heavy roster and the four mixes: WORK P95 <= 1,250,000 with >= 90% in two VBlanks on every VS stage (or every lever in P8_PLAN section 5 settled with numbers); cast <= 1,250,000 with >= 95% on all 16 VS stages, none worse than s56; replay digests identical; 0 frames pushed by motion reads.

Guardrails:
decomp/ is read-only; game rules stay source code; no visible or audible change without the owner's OK.
Native-only ROMs; measure the config that ships; instruments and fault injection stay out of the gate and ship ROMs.
Never edit source while a build is running. One batch = one rule, proved by P8_PLAN section 1's table, committed with explicit paths.
Bugs and crashes first; readability C/D before the RAM and CPU hunts.
Ask only about visible approximations or a contract change.
Subagents: NO Subagents.
Keep docs updated.
Be as efficient and as fast as you can to completion.
```

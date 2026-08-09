# Known Issues

`P1_EXECUTION_BOARD.md` owns dynamic status and decisions. This file lists only
durable unresolved gaps.

## P1 Release Blockers

- Renderer M2 is visually correct but remains above its 170-250K target at
  385,088/388,224 ticks after source-exact light restoration, the retained
  display-capture reset, and the raw-corner dense-ID cut.
- Renderer M3 is source/semantic-correct but remains above its 150-250K target
  at 489,184/489,536 stage ticks.
- All three random Fox smash variants (IDs 372/373/374) and Mario Smash2 ID430
  now play from their exact source programs. Mario Smash1/Smash3 and exact
  damage/jump pitch automation remain fail-closed; the owner's remaining voice ear
  checks remain open. Dream Land BGM and the opening crowd are
  automation/user-confirmed.
- Fox remains the imported level-3 CPU in the public ROM. Automated visual
  captures alone select the documented Fox/countdown-off fast-iteration switch;
  final P1 still needs the owner's CPU-on manual qualification.
- The exact two-ROM build and Current checkpoint pass with a dated
  fast-iteration capture. Release still needs the final CPU-on complete-match
  capture under `artifacts/visibility` and manual user retest.
## Gameplay And Source Boundaries

- Imported `mpprocess` has static symbol/ABI closure. Moving-wall sweep,
  project-floor transforms, and coherent `mpcommon` remain P2 work for stages
  that use them. Dream Land has one unanimated collision group, so those generic
  providers are outside the P1 stage boundary.
- Natural attack-origin DamageFall and throw-origin floor recovery have focused
  runtime traces on Dream Land's 4-floor/1-ceiling/2-wall source topology.
- Original `ftcomputer.c` is live; its natural attack/guard/Recover paths pass.
- Inactive fighter statuses still use weak callbacks when they require unimported
  items, hazards, other fighters, or asset banks. Remove each stub only with its
  owning original TU and natural runtime proof.
- `ftparam.c` is not fully imported; current transform invalidators are narrow
  source-shaped compatibility seams.
- Mario/Fox special wall, ceiling, and edge adjustment is complete: `mpprocess`
  is linked live and the special-collision runner does the source's wall pair
  and ceiling-edge adjust. `check-mpprocess-private-import.ps1` now reports
  NOT-APPLICABLE against a live build; `check-mpprocess-live-link.ps1` owns it.
- Original common particle script/texture banks are not resident. All 178
  Mario/Fox motion-effect calls plus the P1 reflector, blaster-glow, and
  fireball seams route to bounded source-derived DS presentation, but they do
  not reproduce the original particle-bank textures/scripts exactly.
  This is **P1 scope, not P2**: the product contract is that a Dream Land
  Mario-vs-Fox one-minute match with items off is identical to the full game
  under those settings, so any effect that match can show has to be the real
  one. Measured 2026-07-27 and it fits:
  - `efmanager.c` reaches only 26 of the 119 `efcommon` scripts, and those
    scripts name only 18 of the bank's 47 textures.
  - That subset is 118,856 B of texture plus the whole 10,912 B script bank
    = **129,768 B**; Dream Land's own `grpupupu` bank adds 4,896 B.
  - Measured arena headroom at the Boundary stop is 210,320 B, so ~135 KB
    resident leaves ~75 KB spare -- before any N64->DS texture conversion,
    which alone should reclaim most of texture 40 (RGBA32 9 frames, 36,928 B).
  - The banks are position-independent (every internal pointer is a
    file-relative offset pointerized at load), so no relocData tooling is
    involved. See `decomp/BattleShip-main/decomp/PARTICLE_BANK_DISCOVERIES.md`.
  Remaining work is the runtime: `lb/lbparticle.c` (2,961 lines, the bytecode
  interpreter and generator model), `ef/efparticle.c` (113) and
  `ef/efdisplay.c` (107), plus a DS pack step and textured-quad draw.
  **Partial work already exists on two branches — read it before rewriting
  any of the above (2026-07-30).** Two `.claude/worktrees` agents held it
  *uncommitted*, so a worktree cleanup would have destroyed the only copy; it
  was committed first and the worktrees then removed, reclaiming 355 MB.
  - `worktree-agent-a15dedc9b2cf19349` — the DS pack step:
    `scripts/generate_nds_particle_banks.py` (1,310 lines),
    `scripts/check-nds-particle-banks.ps1` (207),
    `docs/optimization/NDS_PARTICLE_BANKS.generated.json` (1,082),
    a generated header, and the Makefile wiring.
  - `worktree-agent-a8c9ad131bc0073b0` — the runtime half:
    `src/import/battleship_lbparticle.c` (677 lines),
    `include/nds/nds_particle_runtime.h`, a placeholder TU, and edits to
    `include/PR/gbi.h` and `src/port/reloc_backend_compat_shims.c` (the
    `lbParticleMakeScriptID` stub at :12963).
  Both commits are **unreviewed, unbuilt and unverified** — they were written
  to preserve bytes, not to assert correctness. Two absolute-path symlink blobs
  (`assets`, `src/nds/generated`, both reparse points into the main worktree)
  were stripped from the second commit so the branch cannot poison another
  checkout; those directories live in the main repo and were never at risk.
  Three `%TEMP%/smash64ds-task16-*` worktrees were removed in the same pass:
  Windows temp cleanup had already deleted every file, leaving only directory
  skeletons and a 119-byte `.git` pointer, which is exactly why
  `git worktree prune` never fired on them. `Smash64DS_Port-mario-bottom` went
  too on the owner's go-ahead once Bug #10 closed (393 MB, clean) — 748 MB
  reclaimed in total, and the main worktree is now the only one. Branch
  `codex/fix-mario-bottom-rendering` and commit `2cbc6189d` are untouched:
  removing a worktree never deletes its branch, which is why this was safe.
- Fireball/weapon heavy wall/ceiling/edge collision and general common-effect
  texture-bank fidelity remain incomplete.
- Items are disabled for P1; general item manager/runtime is P2.

## Renderer And Presentation

- M2 still performs too much per-frame fighter owner work.
- M3 remains above its tick target. Dense preparation reuse, AOT coordinate
  shifts, and the zero-shift matrix specialization are retained; do not retry
  the slower incremental-matrix transport cut.
- The animated tiled-water implementation is deleted. Do not restore it; frozen
  source frame 0 is the P1 contract.
- Whispy material state and geometry remain live, but an unprepared post-GO
  mouth/eye image reuses the first pre-GO resident source frame when every other
  renderer-key word matches. This accepted P1 visual debt prevents gameplay
  conversion; complete dynamic actor texture variants remain P2 fidelity work.

## Audio

- The DS backend does not yet prove every reachable source pitch/voice event.
  The current 128,196-byte pack covers 21 exact source IDs plus six common
  punch/kick contacts (40/38/37/34/32/31) from their exact primary BattleShip
  samples and has 2,876 bytes resident headroom. Their composite forks/custom
  FX remain bounded fidelity debt. Five special/projectile hit composites
  (216/28/2/0/188) remain fail-closed. Fighter voices 375/429/431/435/440
  require live pitch schedules the packed DS format cannot represent; direct
  activation programs with source FX/loops/forks or over-cap samples likewise
  fail closed instead of playing a wrong substitute. Forked DeadExplode voice
  685 remains explicit fidelity debt.
- Existing ACK counters cannot prove the final acoustic mix. The ID626 AOT cue
  passed the owner's exact-ROM ear check; retain user retests for remaining voices.
- Dream Land BGM now has an exact nonzero initial-ring acoustic fixture and a
  natural public-ROM ARM7-shared active-channel proof; do not reopen it without
  a reproduced audible or stream-state regression.

## Memory And Lifetime

- The topology cache is validated for the current battle scene; repeated scene
  rebuild/rematch lifetime and reclamation still need proof.
- Focused current countdown/effects/audio qualifications retain at least
  174,864 bytes; final CPU-on lifecycle qualification must preserve the floor.
- N64 fixed framebuffer addresses and overlay assumptions are unsafe on DS.
- Save/backup behavior remains stubbed; no persistent SRAM/flash behavior exists.
- Overlay loading remains a compatibility no-op pending a measured DS memory plan.
- An arena overflow is a hang, not a NULL. `decomp/src/sys/malloc.c:30` answers a
  full arena with `while (TRUE);` — a developer assert on the N64, shipped code
  here. `src/import/battleship_sys_malloc.c` now latches the arena id, request,
  alignment, headroom and caller LR into `gNdsSyMallocOverflow*` and halts in the
  named `ndsSyMallocOverflowHalt`, so an out-of-memory freeze is identified from
  the PC alone. It still halts: `syTaskmanMalloc`'s decomp callers do not check
  for NULL, so a global NULL return would trade a hang for a wild write. An
  optional allocation must call `ndsSyMallocWouldFit` first and take its own
  fallback. Most `syTaskmanMalloc` call sites still commit blind — see
  `docs/optimization/archive/TASK_STANDING_RULES.md` for the unguarded list.
  Corollary for harnesses: the halt is an infinite loop, so a verifier that
  waits on a marker will **time out** rather than fail. Read
  `gNdsSyMallocOverflowCount` before reading any timeout on this build as a
  functional failure; non-zero means out-of-memory, not a broken check.

## Tooling

- Exactly two root ROMs may be published. Lab/scenario targets stay in `builds/`.
- The executable registry now contains only mode 163 plus normal runtime; 168
  legacy verifier/manager scripts and public mode mappings are deleted.
  Superseded source-side mode branches remain unreachable but pending a separate
  ROM-parity deletion pass.
- The mode-163 scene backend is still a large amalgamated TU, so unrelated slice
  edits can trigger long rebuilds. Split retained runtime TUs only after legacy
  pruning, preserving symbols and behavior.
- The private `mpprocess` verifier still shares root output names and is not safe
  beside another root build.
- Scripted melonDS uses repo-local runners. Mutable TOML audits are repair-only;
  repeated GDB attach/detach can cause packet errors.
- **`verify-task37-itcm-state-hash-ab.ps1` is RED and that is the known state,
  not a new regression.** Task 45 dumped both builds' raw `FTStruct` bytes: all
  215 differing words are main-RAM heap pointers offset by a constant `+0x180`,
  because the image shrinks 384 bytes when Task 37's leaves leave `.main` and
  every heap object below relocates. Zero gameplay values differ. The leak
  mechanism was never root-caused (two hypotheses falsified), so the gate stays
  red rather than being loosened; shipping anyway was the owner's decision on
  that evidence, 2026-07-22. Do not "fix" it by relaxing the canonicalizer.
- **`verify-dev-fast.ps1` is red on the `battle_playable` locked-30 pacing
  contract.** Pre-existing emulator-fork artefact; see `PERF_LEDGER.md:14-22`.
- **Freeing a per-frame graphics-heap allocation can turn the Boundary
  locked-30 phase accounting red (`phaseLag=-1`), 2026-08-09.**
  `NDS_R2_CAMERA_MATRIX_LEAN=3` drops a 64-byte `syMatrixAdvanceW` from
  `gmCameraLookAtFuncMatrix`, which moves every later `gSYTaskmanGraphicsHeap`
  allocation in the frame. Boundary then fails with
  `gNdsBattlePlayablePacingPhasePresentCount` summing **one ahead of**
  `gNdsBattlePlayablePacingPresentedFrames`; routes 0 and 2 of the *same ROM*
  pass. That tuple is not one the harness's four reachable stop phases allow
  (`verify-battle-mariofox-gcrunall-loop-harness.ps1:770-815`), and it should be
  unreachable: each counter has exactly one write site, `taskman_seam.c:4918`
  then `:5054`, ordered presented-then-phase with no early return between them,
  and both reset sites zero them together. So either the pacing accounting has a
  real off-by-one that only some allocation layouts expose, or the stop-phase
  model is incomplete. **Level 3 is held off by default pending that answer** --
  the reproduction is one `-SetGlobals` poke.
- **A `.data` route pairing does NOT make an arm placement-free if the arm
  changes an allocator.** The pairing guarantees identical *text*; it says
  nothing about where the frame's heap objects land. Split any candidate that
  both removes work and removes an allocation into two levels and measure them
  separately -- the row above is what conflating them costs.
- **The `battle_playable_realtime` verifier run STANDALONE fails `phaseLag=-1`
  on every arm, including commits the full profile passes.** It is only a valid
  gate inside `verify-all.ps1 -Profile Boundary`. Do not bisect with it alone;
  that reading exonerated a change the profile then convicted.
- **`Select-Object -First N` terminates the upstream pipeline.** It has killed a
  build and a census mid-flight and left directories that looked like ordinary
  failures. Redirect harness output to a log and filter with `Select-String`.
- **`sample-tick-hud-buckets.ps1 -FallbackCensus` needs Task 68's symbol even
  when the ring carries Task 75's counter** — build with *both* census flags.
- **`NDS_R2_SPAN_LEAN_TIMING=1` alone does not give you the span brackets.** It
  only compiles the per-delta census *out from inside* them (that is the whole
  point — E43 found the brackets were pricing their own instrument). The
  brackets and `gNdsR2Span{Before,After}{Ticks,Deltas}` themselves live under
  `NDS_TASK91_DRAW_PHASE_CENSUS`, so a lean-timing run needs **both** flags or
  the sampler dies with `No symbol "gNdsR2SpanBeforeTicks" in current context`
  after the emulator has already reached the window.
  `-ExtraGlobals` is re-split on commas because `pwsh -File` passes the list as
  one literal string.
- **`NDS_TASK37_PROFILE_PER_FRAME_REGION=1` exists** and gives per-frame regions
  in one run; without it the profiler reports `regions=1` and per-frame
  attribution needs two narrow-window builds.
- **Never attribute cartridge activity to a frame across two differently-timed
  builds.** `_ntrcardRomReadSector` moved entirely from the excursion to the
  control when the HUD draw was compiled out, over the same deterministic frames.
- **ITCM is effectively full** at 32,596 of a 32,736 cap, so any future placement
  work must evict first. The Task 37 census identified ~5,040 bytes of
  never-executed residents as where to look.

## Coverage Reductions

- FastIteration uses wider image motion/flat-run allowances than normal capture,
  but both frames retain independent stage/fighter/detail gates.
- Live combat no longer requires fighters inside historical fixed color crops;
  GDB still requires both selected/submitted fighter contracts and triangles.
- Retired modes 161/162 used a pre-GO input driver that conflicted with the
  restored source Wait lock. Boundary uses natural mode 163 instead.

## Build Warnings

Known nonfatal warnings come from original decomp code and narrow ABI shims:
unused/maybe-uninitialized locals, task/scheduler pointer types, original reloc
symbol pointer-to-int tables, and imported menu placeholder returns. Do not
silence them globally; fix the shared compatibility type when a warning blocks
real signal.

## Source Compatibility

- BattleShip's N64 libc headers are not globally included because they conflict
  with devkitARM/libnds.
- Shadow headers expose only imported ABI and must preserve upstream values and
  offsets when expanded.
- Broad stubs can hide missing behavior. Every retained stub needs a durable
  issue here or a runtime diagnostic.

## `generate_task39_effect_census.py` rewrites dated evidence (R2-05 E0, 2026-07-29)

**Do not run this generator.** It emits an "ownership evidence" column containing
`src/port/reloc_backend_compat_shims.c:<line>` for 60 rows, so:

1. Any unrelated edit to `reloc_backend_compat_shims.c` silently invalidates
   every line number in `artifacts/performance/2026-07-21_task39-visual-effects-census.md`.
   Running it on 2026-07-29 shifted `7713 → 7774` and `12870 → 12963`.
2. That file is a **dated** artifact under `artifacts/performance`, which
   `AGENTS.md` designates permanent evidence. Regenerating it overwrites a
   2026-07-21 snapshot with today's source layout, i.e. it falsifies the date.

It is wired into neither `build.ps1` nor the Makefile, so nothing runs it by
accident — only a person re-running generators will trip it.

**The durable fix, when this row is next touched:** a census that cites evidence
should cite a *stable* locator — symbol name, or file plus symbol — not a line
number. Line numbers are a property of the working tree at generation time and
have no place in an artifact whose whole value is that it is dated. Until then,
treat the committed copy as the record and leave it alone.

## The struck fighter does not flash white during hitlag (R2-03 E32/E62, 2026-07-29)

**Accepted deliberately, owner-approved 2026-07-29, so E32's −51,136 WORK-H P95
could ship.** Recorded here so it is not forgotten and so nobody "rediscovers"
it as a new bug.

**Symptom.** For the ~5 consecutive frames of a hitlag burst, the fighter being
hit renders in its normal colours instead of flashing white. Nothing is corrupt
or missing — it is the absence of a flash, not a wrong colour. Every frame that
is not a flash frame is **pixel-identical** to the pre-E32 generic render
(measured: frames 510/511, 0 differing pixels; 480/481 differ by 1.35%/1.10%,
confined to the struck fighter). Evidence:
`artifacts/visibility/e32-{off,on}-{480,481,510,511}.png`.

**Root cause — a generator gap, not a runtime bug.** The hurt flash clears
`G_LIGHTING` (`NDS_RENDERER_GEOM_LIGHTING`, `0x00020000`) for the struck fighter
and draws its vertex colours raw. Under `NDS_R2_FIGHTER_HW_LIGHT` the native
owner keeps `POLY_FORMAT_LIGHT0` set and emits `GFX_NORMAL` unconditionally, so
the geometry engine lights the flashing fighter using stale diffuse/ambient from
the previous epoch. **The owner has no flash-colour data to draw**:
`sNdsNativeFighterDenseVertices[].rgba` holds the F3DEX2 **packed normal**, baked
for the lit path.

**Do NOT try to fix it by enabling `NDS_R2_UNLIT_VERTEX_EPOCH`.** E49 wrote that
route and E62 built it: it drops `POLY_FORMAT_LIGHT0` and emits
`ndsRendererR2DenseVertexColor15`, which reads that same normals table, so the
fighter renders in **rainbow speckle** and the diff against the correct render
gets *worse* — 2,199 px versus E32's 1,551. Picture:
`artifacts/visibility/e62-on-480.png`.

**The real fix (E63), when it is worth a session.** Bake the flash variant's
vertex colours as a second dense table beside `sNdsNativeFighterDenseNormals`,
and select per epoch on `geometry_mode & LIGHTING`. The runtime half already
exists and is proven to reach all four emit loops; only the generator side and
the table's size are open. Size the table before writing the generator.

**Note for whoever reads the old entries:** E48 and E58 look contradictory and
are not. The *live display-list* vertices on a flash epoch are colours (E48,
273/273, material 0); the *baked dense* table is normals (E58). Different
streams.

## RESOLVED — E64b's numerical equivalence, bounded not hashed (R2-03, 2026-07-29)

Kept because the *reasoning* generalises, not because anything is open.

The fixed-point cubic (`NDS_R2_CUBIC_FIXED`) graduated on performance and
Boundary-liveness with its numerical deviation unmeasured, and this file named
the Task 9 state hash as the instrument that would settle it. **That was the
wrong instrument, and naming it wasted the correction.** The hash asserts
bit-exactness; E64b is *authorized as non-bit-exact*. It could only ever report
"differs", which says nothing about whether gameplay moved. Two builds and two
emulator runs would have bought a result knowable in advance.

**The right instrument for a non-bit-exact arithmetic substitution is an error
bound over a stated input domain.** `scripts/check_r2_cubic_error_bound.py`
extracts the shipped kernel and the decomp's `gcGetInterpValueCubic`, compiles
both on the host, and sweeps. No emulator, no ROM, ~4 s. R2-03 E65 on the board
has the numbers: worst deviation **0.0028 rad** on rotation tracks and **0.0067
world units** on translation tracks, against a 0.02 gate set by hitbox scale.
Joint values reach gameplay only through
`gmCollisionGetFighterPartsWorldPosition` (`gm/gmcollision.c:489`), so that
cannot flip a hit decision.

Two things the first RED run taught, both now in the code:

- The deviation scales with `L·|rate|`, so **one number for "the cubic's error"
  is meaningless** — a translation track deviates 8× further than a rotation
  track from identical arithmetic. State the domain or say nothing.
- Truncating shifts were biased *and* imprecise; `(1−t)²` computed by squaring
  `(1−t)` lost most of its significance near t=1. Rounding, the
  `1 − 2t + t²` identity, and a Q16 basis fixed it for the price of two SMULLs.

Generalise carefully: a bound is right when the inputs are enumerable and the
failure mode is rounding. It is the wrong answer for a change to control flow,
lifetime, or ordering.

## The Task 9 state-hash A/B harness is stale (2026-07-29)

Not E64b's problem any more (see above), but the next change that *does* need a
bit-exactness comparison will hit all three of these, so they are recorded rather
than rediscovered.

1. **`verify-task16-combined-state-hash-ab.ps1` is stale.** It builds targets
   `smash64ds-task16-combined-state-{control,candidate}` which **do not exist in
   the Makefile**. Copy its *shape* (it drives
   `verify-battle-mariofox-gcrunall-loop-harness.ps1` with `-Task9StateHashMode 1`
   and `-Task9StateHashExportPath`), not its target names.
2. **The owner harness rejects TICKHUD** — "GDB proof runs require full telemetry
   and must not use TICKHUD" — so use `smash64ds-battle-playable-proof-hwtri`.
   With that target it reached the match, emitted `TASK9_STATE_SUMMARY=4087,0,4096`
   and 4,087 `TASK9_STATE=` rows, then threw on an **unrelated** gate:
   *"Published ROM did not preserve the complete M4 residency lifecycle and zero
   post-GO fence"*. The throw happens **before** the export path is written, and
   the rows go to the console rather than the log, so nothing is persisted. Either
   satisfy that M4 gate or capture the `TASK9_STATE=` lines from the harness's own
   stdout.
3. **The tick-HUD sampler cannot substitute.** `gNdsTask9StateHashCount` reads 0
   on a tick-HUD build with `NDS_TASK9_STATE_HASH=1`, because the recorder must be
   armed through `gNdsTask9StateHashArmed` and `sample-tick-hud-buckets.ps1` only
   *reads* globals. Arming it needs a GDB write the sampler does not do.

## Two Runtime 2 gates name instruments that do not exist (R2-06, 2026-07-29)

A pattern worth naming, because it cost real time twice in one session and both
instances looked like passing verification until checked:

**A gate whose instrument is absent or not compiled in gives no signal, yet reads
like a pass.**

1. **E64b / the Task 9 state hash.** `NDS_TASK9_STATE_HASH ?= 0` and no Boundary
   verifier references it. I reported "the hash did not move" as evidence of
   gameplay safety; it had never run. A nearby passing line about "Task 9 float
   ITCM" (placement, not state) is what made it look covered.
2. **R2-06 / "soak clean".** No soak harness exists —
   `ls scripts/ | grep -iE 'soak|stability|long'` is empty and the only `soak`
   string in the repo is the word in the switch plan's own gate text. The clause
   cannot be satisfied or refuted today.

**Rule for the next cycle: before citing a gate as evidence, confirm the
instrument ran.** Check the flag in the emitted `nds_build_config.h`, or check
that the symbol is present in the ELF (`nm | grep`), or read the counter and
confirm it is non-zero. "No failure line appeared" is not evidence — that is the
same trap as `Select-Object -First N` silently killing a build, already recorded
above.

**But `nm | grep` proves presence, never absence.** 2026-07-29: I concluded the
R2 animation cache was lab-only because `ndsR2AnimCacheStore` is missing from the
shipped `smash64ds-battle-playable-hwtri.elf`. It is missing because it is
`static` and was inlined; the code is in the ROM, and the cache was live in the
build the owner plays. An absent symbol is not absent code. To decide whether a
flag is on, read the `nds_build_config.h` emitted by *that build's* directory —
match it by timestamp to the ROM — or disassemble the surviving caller and look
for the call. Checking the wrong build tree's config is how this went wrong.

Both instances are also *gate-design* bugs, not just reporting bugs: a phase gate
should name a check that exists and is wired into a profile, or say explicitly
that building the check is part of the phase.

## The freeze watchdog cannot see a hang before the first presented frame (2026-07-29)

`ndsFreezeDiagnosticsWatchdog` (`src/nds/nds_freeze_diagnostics.c`) refuses to arm
until `gNdsFreezeDiagnosticsHeartbeat != 0`, and the heartbeat only advances in
`ndsFreezeDiagnosticsHeartbeat()` on a completed presented frame. Anything that
hangs during boot or scene load — which includes every arena overflow in
`ftManagerSetupFilesAllKind`, i.e. fighter file loading at battle start — happens
with the heartbeat still 0, so the watchdog never arms, never trips, and prints
nothing. Measured: an ARM9 provably spinning at `syMallocSet`'s `while (TRUE);`
for 7+ seconds with `WatchdogTripCount` 0 and `sNdsFreezeDiagnosticsWatchdogArmed`
0. A silent watchdog in that window means nothing at all.

Two consequences worth carrying forward:

- Arm the observers **before** the first `continue`. A GDB script that installs
  its stall/exception breakpoints after a `tbreak ifcommon.c:3175; continue` will
  miss any trip that happens during the load, and the run then looks like a
  timeout with no cause.
- **Validate the negative first.** `gNdsFreezeDiagnosticsForceTrip = 1` makes the
  watchdog fire on demand; do that once per session before treating "the watchdog
  did not fire" as evidence. A watchdog that has never been made to fire is not
  an instrument, and this one has a blind window wide enough to hide a whole class
  of hang.

## A window-pixel freeze detector must exclude the window chrome (2026-07-29)

`scripts/lib/melonds-screenshot.ps1`'s `Get-MelonDSWindowBitmap` uses
`GetWindowRect` + `CopyFromScreen`, so the hash covers the **title bar and the
desktop behind the window**, not just the two DS screens. melonDS puts its frame
rate in the title (`[83/60] melonDS 1.0`), so the hash changes every poll while
the guest is completely dead.

Measured cost: a `soak-freeze-watch.ps1` run reported "alive, 54 distinct frames"
for ~10 minutes on a ROM that had frozen during its first battle load and was
displaying the boot text screen the whole time. The guest counters in its own
capture said so plainly — `sVBlankCount` 849 (~15 s of guest time), presented 0,
taskman 0, results 0 — and were misread as post-scene-reset values. A conclusion
of "clean, 2.8x past the failure point" had to be withdrawn.

Rules: crop to the client area or to the two 256x192 screens
(`Convert-MelonDSWindowTopToNativeBitmap` already knows the 8px frame and 56px
title/menu offsets) before hashing; make a guest counter that only the main loop
advances the primary liveness signal and the picture corroboration, not the
reverse; and on a clean run spend the single GDB session reading
`gNdsVSResultsStartCount` so "N minutes" becomes "N minutes across M matches". A
soak is passive — it never restarts the match — so a changing picture can be the
results screen, host chrome, or nothing at all.

## `nSYAudioBGMExplain` is 0 in the port and 34 in decomp

`check-decomp-header-mirror.py` fails on it: `include/sys/audio.h:15` declares
`nSYAudioBGMExplain = 0` while `gm/gmsound.h:30` has it at 34 ("How to Play").
Pre-existing — that header has not been touched since 2026-07-14, and the
mismatch predates the 2026-08-02 audio work that found it.

Harmless today and a landmine tomorrow: it is the ONLY occurrence in `src/` or
`include/`, so nothing reads it, but the first caller gets BGM 0 instead of the
How-to-Play track. Not fixed here because the `= 0` looks deliberate — a
placeholder meaning "not wired" reads exactly like this — and guessing wrong
silently changes which music plays. Owner's call: set it to 34 to match decomp,
or drop the declaration so the checker stops guarding a value nothing uses.

## Something hands the joint parser a misaligned animation script

`gcParseDObjAnimJoint` used to freeze the match on this (BUGS.md "Shield Freeze
is back", closed 2026-08-03). It no longer can: the four animation parsers bound
their event loops and record a fault instead of spinning. What has NOT been
found is the producer.

The 7-minute both-CPU soak of 2026-08-03 reported `gNdsObjAnimRunawayCount=10`,
`gNdsObjAnimRunawayMask=1` (bit 0, unrecognised opcode in the DObj event32
parser), `gNdsObjAnimRunawayScript=0x238561A` and `gNdsObjAnimRunawayOpcode=100`.
`0x238561A` is 2 mod 4, so it cannot be an `AObjEvent32*`; a misaligned `LDR` on
ARM9 rotates the word it reads, which is where opcode 100 comes from. The three
freeze captures show the same shape (`0x23842ea`, `r6=0x64`).

Two candidate producers are already excluded by measurement, so do not re-run
them: the shield table (`ftcommonguard1.c:238`, 49 installs probed on the live
ROM, every one 4-aligned and in-file, `joint_num=27` against Fox's 28-entry
`dFoxShieldPose_shield_anim_joint_*`), and `ndsRelocResolvePointerFromFileBase`
(its offset fallback measured `gNdsRelocResolveOffsetCount=0`, i.e. it never ran).

What is left: the parser rewriting its own pointer at `objanim.c:513/525`
(`event32 = event32->p` on SetAnim/Jump reads a raw word out of the script with
no validation), or `AObjAnimAdvance` walking off the end of a short script into
neighbouring data. The counter now names the script address on every occurrence,
so the next cycle can break on the writer instead of reproducing a freeze.

Cost today: ten joints in seven minutes end their animation one pose early.

## The `gs`-form GBI static initializers are all `{ 0 }`

`include/PR/gbi.h:193-202` defines eleven `gs*` macros -- `gsDPSetRenderMode`,
`gsDPSetPrimColor`, `gsDPSetCombineMode`, `gsDPSetAlphaCompare`,
`gsDPSetBlendColor`, `gsSPSetGeometryMode`, `gsSPClearGeometryMode`,
`gsSPSetLights1`, `gsDPPipeSync`, `gsSPEndDisplayList` -- as `{ 0 }`. The
runtime `gDP*` twins of two of these were the 2026-08-04 effect-translucency
defect (commit d9b61c1ed1); the static family was never audited.

An element-per-macro `{ 0 }` keeps a static `Gfx[]` the right LENGTH but makes
every command a G_NOOP, so the table links, draws, and silently carries no
colour, mode or geometry state. Three compiled TUs use these forms:
`src/import/battleship_ftdisplaymain.c`, `battleship_grwallpaper.c` and
`battleship_ifcommon.c` (Makefile:1665, :1745, :1754) all `#include` decomp
sources that build static `Gfx` tables with them.

Not measured broken, and not chased on 2026-08-04: the row that raised it (the
KO pillar's gold stars reading white) draws through the particle path, not a
static `Gfx` table, and the owner has accepted that row. The next cycle that
touches fighter display, the wallpaper or the HUD should decide whether each
site wants a real packet or is genuinely inert, and say which.

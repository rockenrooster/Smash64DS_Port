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

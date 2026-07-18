# Known Issues

`P1_EXECUTION_BOARD.md` owns dynamic status and decisions. This file lists only
durable unresolved gaps.

## P1 Release Blockers

- Renderer M2 is visually correct but remains above its 170-250K target at
  385,088/388,224 ticks after source-exact light restoration, the retained
  display-capture reset, and the raw-corner dense-ID cut.
- Renderer M3 is source/semantic-correct but remains above its 150-250K target
  at 489,184/489,536 stage ticks.
- One natural source voice per fighter now plays (FoxSmash1 ID372 and
  MarioSmash2 ID430); remaining variants, exact pitch automation, and Tyler's
  voice ear check remain open. Dream Land BGM and the opening crowd are
  automation/user-confirmed.
- Fox remains the imported level-3 CPU in the public ROM. Automated visual
  captures alone select the documented Fox/countdown-off fast-iteration switch;
  final P1 still needs Tyler's CPU-on manual qualification.
- The exact two-ROM build and Boundary checkpoint pass with a dated
  fast-iteration capture. Release still needs the final CPU-on complete-match
  capture under `artifacts/visibility` and manual user retest.
- Playtesting reports a CPU stall during Down+A aerials. Human-P2 Fox now has
  observer-free and target-owned freeze snapshots at status 213 / motion 188 /
  tic 6. The last target IRQ PC resolves to Calico `armWaitForIrq` with a pending
  IRQ, while the watchdog task never arms. The interrupted mode and caller are
  still unknown; capture SPSR/CPSR plus the correct banked SP/LR before changing
  gameplay. Closure then needs Fox P2, Mario, and canonical CPU-on Current gates.

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
- Mario/Fox special wall, ceiling, and edge adjustment is incomplete.
- Common particle script/texture banks are not resident; particle calls remain
  bounded no-op/diagnostic seams.
- Fireball/weapon heavy wall/ceiling/edge collision and some common effects are
  incomplete.
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
  The current pack covers 18 exact source IDs / 16 unique samples and 11
  collision cues. IDs 429 and 435 require continuous pitch schedules that the
  packed DS format cannot yet represent; forked DeadExplode voice 685 remains
  explicit fidelity debt.
- Existing ACK counters cannot prove the final acoustic mix. The ID626 AOT cue
  passed Tyler's exact-ROM ear check; retain user retests for remaining voices.
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

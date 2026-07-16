# Known Issues

`P1_EXECUTION_BOARD.md` owns dynamic status and decisions. This file lists only
durable unresolved gaps.

## P1 Release Blockers

- Renderer M2 is visually correct but above 170-250K ticks.
- Renderer M3 is source/semantic-correct but measures 611,392/611,584 stage
  ticks; a different attributable cut must reach <=500K before promotion.
- Natural DamageFall-to-main-floor recovery now passes. Throw-origin coverage,
  non-floor collision providers, and Fox recovery remain incomplete.
- The current candidate retains 128,528 arena bytes, 2,544 below the 128 KiB
  P1 floor.
- The platform verifier can accept a wrong next-frame landing; Tyler's manual
  report remains open.
- Mario Fireball early spawn/damage/rebound/draw passes, but full 140-tick
  collision, independent source-matrix parity, and long-distance visual capture
  remain open.
- Required fighter voices, pitch behavior, and audible Dream Land BGM proof
  remain open. The opening crowd overlap/loop report is fixed and user-confirmed.
- Fox is still classified level-3 CPU, but the public ROM temporarily pauses his
  decision/input. The focused CPU-on one-minute gate passes; final P1 still
  needs Tyler's re-enable decision and a published CPU-on qualification.
- Release needs the exact two-ROM build, focused verifier closure, dated captures
  under `artifacts/visibility`, and manual user retest.

## Gameplay And Source Boundaries

- Imported `mpprocess` has static symbol/ABI closure, but moving-wall sweep,
  project-floor transforms, non-floor providers, and coherent `mpcommon` are not
  graduated live.
- Natural attack-origin DamageFall-to-floor recovery has a sparse successful
  runtime trace; throw-origin and non-floor routes remain unproved.
- Original `ftcomputer.c` is live. Its natural proof has not selected Recover.
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
- M3 remains above its tick gate. The current dense preparation-reuse stack
  reached 563,296 but still missed 500K and was reverted; do not retry it or the
  slower incremental-matrix transport cut.
- Bitmap OAM has one-bit alpha, so nonzero partial-alpha GO pixels are opaque.
  This is accepted presentation debt under the 90% visual rule.
- Do not publish the hybrid IFCommon OAM lab path: its counters and source
  commits pass, but the countdown traffic-light objects are invisible at
  runtime. The proven all-bitmap path prepares before gameplay with no hot
  conversion/upload and is the published contract.
- The animated tiled-water implementation is deleted. Do not restore it; frozen
  source frame 0 is the P1 contract.

## Audio

- The DS backend does not yet prove every reachable source pitch/voice event.
- Voice 685 and ordinary Mario/Fox voices need natural audible coverage.
- The reported 24 unsupported calls are event count, not 24 identified IDs;
  capture natural ID/order before selecting assets.
- Existing ACK counters cannot prove the final acoustic mix. The ID626 AOT cue
  passed Tyler's exact-ROM ear check; retain user retests for remaining voices.

## Memory And Lifetime

- The topology cache is validated for the current battle scene; repeated scene
  rebuild/rematch lifetime and reclamation still need proof.
- Mode 163 currently misses its >=128 KiB reserve floor by 2,544 bytes after
  the resident audio/renderer allocations.
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

# Fidelity-02 item / weapon evidence — 2026-09-13

Candidate is the dirty working tree. `decomp/` was read-only. Runtime rows below
remain pending until the shared native-owner generator permits the lab build.

## Static source divergences and repairs

- Poké Ball status dispatch: pre-edit port
  `battleship_item_link_core.c:2062,2086,2110` had NULL Dropped/Thrown/Hold
  entries. BattleShip `itmain.c:45,77,109` uses
  `itMBallDroppedSetStatus`, `itMBallThrownSetStatus`, and
  `itMBallHoldSetStatus`. Candidate port lines `2106,2130,2154` now match.
  Entry-by-entry audit of the three source tables: common kinds 0..19 match;
  Ness PK Fire remains source-NULL; the shell's compiled Link Bomb slot retains
  the three source Link Bomb setters through the existing `NDS_P2_LINK` branch.
  Result: PASS, no source function is NULL in the candidate table/branch set.
- Weapon pool: BattleShip `wpmanager.c:35` allocates `WEAPON_ALLOC_MAX` (32).
  Pre-edit DS default was 3 at `include/nds/nds_startup.h:4039`. Candidate uses
  a bounded census ceiling of 10 at `nds_startup.h:4038`, adds allocator
  refusal counter `gNdsWeaponPoolRefusalCount` and successful-live high-water
  `gNdsWeaponPoolLiveHighWater` in `battleship_wpmanager_core.c:277-313`, and
  exports both in `include/nds/nds_startup.h:4021-4023`. Live depth is tracked
  O(1) on allocate/free; the rejected pop/push probe and free-list walk are gone.
  The stress verifier reads both and rejects non-zero refusals. Final pool
  value awaits the two requested whole-match measurements. `sizeof(WPStruct)`
  is 704 B, so 10 entries reserve 7,040 B total. Relative to the old 3-entry
  pool this adds 4,928 B. The supplied four-CPU baseline low-water is 33,672 B
  and the verifier floor is 25,600 B; applying that delta predicts 28,744 B,
  leaving 3,144 B above the floor. Per the package constraint, 10 is therefore
  the measurement ceiling unless RAM is recovered elsewhere; it is not being
  claimed as the final source-equivalent capacity. The final setting must be
  measured high-water plus margin with `gNdsWeaponPoolRefusalCount == 0` and
  observed `gNdsTaskmanGeneralHeapFreeMin >= 25,600` on both requested rosters.
  Review follow-up replaced the pop/push preflight with the source constructor
  using the instrumented public allocator/free pair directly. A token comparison
  against BattleShip `wpmanager.c:87-346` is exact after normalizing two harmless
  chained assignments and removing the one DS attribute-normalization call.
- ITCommonData rejection: pre-edit loader silently left the common-data pointer
  absent when its size/fallback/fit guard rejected. Candidate explicitly clears
  `gITManagerCommonData` before the load and increments
  `gNdsITCommonDataRejectCount` for both guard rejection and a NULL loader return
  (`battleship_item_link_core.c:856,861`). The counter is exported at
  `include/nds/nds_startup.h:4023`, collected by the stress verifier, and must be
  zero. Container drops substitute zero Y velocity when the source table is
  absent (`battleship_item_map_core.c:87-90`) instead of indexing NULL. Runtime
  bytes/pointer/reject census pending.
- Ground-monster destroy: pre-edit `battleship_item_link_core.c:1916-1921`
  always emitted large dust for unheld items. BattleShip `itmain.c:305` guards
  stage Pokémon kinds; candidate restores that guard at port `:1956-1957`.
- Heavy pickup voice: pre-edit port omitted BattleShip `itmain.c:472-475`.
  Candidate restores `heavyget_sfx` playback at port `:2250-2252`.
- Appear spin / spawn presentation: BattleShip `itmain.c:136` defines
  `itMainSetAppearSpin`; `itmanager.c:472-473` applies swirl + slow spin at
  common spawn, and `itmain.c:606` applies fast spin on container release.
  Candidate source-transcribes appear spin at `battleship_item_link_core.c:1833`,
  calls it from common setup at `:1545` and container release at
  `battleship_item_map_core.c:98`, and now supplies a concrete weak
  `efManagerItemSpawnSwirlMakeEffect` body at `battleship_item_link_core.c:111`.
  Until a native visual provider overrides it, that body records the existing
  Task39 `NDS_TASK39_EFFECT_SKIPPED` census instead of disappearing as an
  undefined weak reference.
- Pokémon enum: removed the pre-edit local `nITKindMBallMonsterStart` macro;
  the candidate uses `include/it/item.h:907`'s enum directly.
- Onix weapon ABI: BattleShip `wpvars.h:270-278` defines
  `wpIwarkWeaponVarsRock` and `wptypes.h:191` places `rock` in `weapon_vars`.
  Candidate adds the exact typed member at `include/wp/weapon.h:167-173,355`
  and `battleship_item_iwark.c` now uses `wp->weapon_vars.rock`. The private
  overlay and unused `raw[32]` member are gone; PK Thunder remains the union's
  32-byte maximum.

## Host checks

- `python scripts/check-untracked-dependencies.py` — PASS:
  `untracked_source_files=0 committed_refs_clean=YES`.
- `pwsh scripts/check-architecture.ps1` — PASS (`imports=249`, two pre-existing
  warnings: large-file split plan and local generated outputs).
- `pwsh scripts/check-melonds-policy.ps1` — PASS, runner slots 13/13 match.
- `pwsh scripts/check-docs.ps1` — PASS (`docs=18`, `registryEntries=6`).
- `scripts/items/item_memory_closure.py` has no `--check` argument, so the
  conditional host check does not apply.
- Direct ARM9 compile checks with the normal shell build config PASS for
  `battleship_wpmanager_core.c`, `battleship_item_link_core.c`,
  `battleship_item_map_core.c`, and `battleship_item_iwark.c`; output contains
  only the decomp's existing warnings. `git diff --check` also passes.
- `scripts/verify-p2-four-fighter-stress.ps1` parses successfully as a
  PowerShell script after the new pool/common-data assertions.
- Shared generator pre-build check currently BLOCKED outside this package.
  Latest poll after the roster-session changes reaches
  `generate_nds_native_owners.py:6806` and raises
  `IndexError: list index out of range` while indexing `run_metadata[run_index]`.
  This package does not edit or work around that generator.

## Runtime / build evidence

- Lab build `build-p2-fidelity-02`: PENDING.
- Poké Ball Thrown/Open/Pokémon engagement: PENDING.
- Stress roster weapon high-water/refusals/heap low-water: PENDING.
- Projectile-heavy roster weapon high-water/refusals/heap low-water: PENDING.
- Full stress verifier verdict: PENDING.
- ROM SHA-256: PENDING.
- ELF SHA-256: PENDING.

## 2026-09-13 build lane / build2

- Shared lane reported `FREE`; acquired with literal `ACQUIRED codex-items` and
  released immediately after `make` exited.
- `make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-fidelity-02`
  reached final link, but `.itcm` overflowed by 144 bytes.
- Build log: `builds/build-p2-fidelity-02-build2.log`.
- `NATIVE_ONLY_PASS` was absent; ROM/ELF hashes remain unavailable until the
  link-capacity blocker is resolved.
- Failed-link map proves this package is not the ITCM contributor: `.itcm` is
  `0x8070` against the `0x7fe0` region (exactly +`0x90` / 144 bytes), dominated
  by shared `nds_renderer.o` (`0x2614` + native-fighter `0x1f6c`) and
  `scene_backend.o` (`0x1b68`). Items/weapons code is mapped in main RAM.

## 2026-09-13 build2 retry after shared roster lane

- Waited for `codex-roster` to release the shared lane, reacquired with literal
  `ACQUIRED codex-items`, reran the exact target, and released immediately.
- Result is unchanged: linker `region 'itcm' overflowed by 144 bytes`; no
  `NATIVE_ONLY_PASS`, ROM, or ELF was produced. Runtime proofs and stress are
  blocked on this shared renderer/scene ITCM capacity regression.

## 2026-09-13 current-tree host closure

- `python scripts/check-untracked-dependencies.py` PASS:
  `untracked_source_files=0 committed_refs_clean=YES`.
- `pwsh scripts/check-architecture.ps1` PASS: `imports=249`, warnings=2.
- `pwsh scripts/check-melonds-policy.ps1` PASS: runner slots `13/13`.
- `pwsh scripts/check-docs.ps1` PASS: `docs=18`, `registryEntries=6`.
- `docs/p2/BUG_NOTES.md` already has the fidelity-02 10-entry/high-water/refusal
  paragraph at lines 4614-4622, so no duplicate paragraph was appended.

## 2026-09-13 runtime setup validation while link-blocked

- Read the parameter blocks of `scripts/probe-p2-fourcpu-sparse.ps1` and
  `scripts/verify-p2-four-fighter-stress.ps1`; both support `-RunnerSlot` and
  `-NoBuild`, and the stress verifier collects the four new globals in its
  same-run memory set.
- Generated `builds/build-p2-fidelity-02/nds_build_config.h` confirms
  `NDS_P2_FOUR_CPU_STRESS=1`, roster 1, and `NDS_P2_ITEM_CORE=1`.
- No stale ROM/ELF exists in `builds/build-p2-fidelity-02`; no emulator or GDB
  run was started against an unverified binary.
- Current stress verifier uses a 25,600-byte general-heap floor, reads the pool
  entries/high-water/refusal globals, requires nonzero engagement and zero
  refusals, and derives the expected entry count from `NDS_R2_WEAPON_POOL=10`.

## 2026-09-13 build3 after ITCM recovery

- Shared lane was `FREE`, then acquired with literal `ACQUIRED codex-items` and released immediately after the build completed.
- `make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-fidelity-02` PASS; log `builds/build-p2-fidelity-02-build3.log`.
- `NATIVE_ONLY_PASS: smash64ds-p2-fourcpu-tickhud-hwtri.elf, 245 actual link inputs`.
- ROM: `builds/build-p2-fidelity-02/smash64ds-p2-fourcpu-tickhud-hwtri.nds`, SHA-256 `A8A8639D3A82A06DDB563DB70AFDF9DE6174E3B9A40067AFBD0285567997E175`.
- ELF: `builds/build-p2-fidelity-02/smash64ds-p2-fourcpu-tickhud-hwtri.elf`, SHA-256 `43B3D629756F092641CE383A9A79C1BBFCD5635F7DABC198623F7D9911A7C32E`.

## 2026-09-14 staged Poke Ball proof result

- The slot-11 proof launched at 01:38 is no longer alive: final recheck found `MELONDS_COUNT=0` and `GDB_COUNT=0`.
- `builds/fidelity02-pokeball-input.gdb.out` contains no `POKEBALL_*` marker lines, including no terminal `POKEBALL_FINAL` marker.
- Witnessed stage-marker counts: maker 0, Hold 0, Thrown/Open 0, monster 0; no witness frame was emitted for any stage.
- The run ended before a bounded stage witness could be recorded; no slot-11 process remained to terminate.

## 2026-09-14 four-CPU stress timing attempt 1

- Launched `pwsh scripts/verify-p2-four-fighter-stress.ps1 -NoBuild -Build build-p2-fidelity-02 -RunnerSlot 11` from a quiet emulator lane.
- The verifier exited without a gate verdict. Its GDB failure marker stopped in libnds `vramBlock__allocateBlock` after `malloc` returned null.
- No melonDS process was present immediately before launch or after exit. A clean quiet-lane retry follows.

## 2026-09-14 four-CPU stress reproducible blocker

- Two additional quiet-lane runs reproduced the same stop in libnds
  `vramBlock__allocateBlock` on a 28-byte `malloc`; no stress PASS/FAIL verdict
  was emitted because the collector aborted first.
- The repeated `TICKEXTRA` failure snapshot reports general-heap free-min
  `128668`, ITCommonData bytes `82976`, ITCommonData rejects `0`, weapon pool
  entries `10`, weapon live high-water `0`, and weapon refusals `0`.
- High-water `0` is pre-engagement failure data, so it is not a sizing
  measurement and does not justify changing `NDS_R2_WEAPON_POOL=10`.
- The last accepted stress artifact (2026-09-13 13:23) used the same taskman
  arena size `1412864`; its whole-run general-heap low-water was `33672`.
- Current versus that accepted ELF is +4,128 text, +8 data, +1,120 BSS. Symbol
  comparison attributes much of that growth to unrelated concurrent fighter
  work. A scoped base-constructor experiment changed the ELF by only 16 bytes
  and reproduced the same stop/counters, so it was reverted.

## 2026-09-14 final rebuild and host checks

- Restored the reviewed weapon-constructor source after the no-effect sizing
  experiment, acquired/released the shared `codex-items` lane, and rebuilt the
  four-CPU target. `builds/build-p2-fidelity-02-build5.log` reports
  `NATIVE_ONLY_PASS: smash64ds-p2-fourcpu-tickhud-hwtri.elf, 245 actual link inputs`.
- Current ROM SHA-256: `20FCBBA72A521E94D9A58A8748B5081ADF63BD616F2AD46CC0EC1FAAA18900A8`.
  Current ELF SHA-256: `925028E119A1F3F9583E6AF52B25E80A1966F482C2A18F11222D2C08CE96126E`.
- `python scripts/check-untracked-dependencies.py` PASS:
  `untracked_source_files=0 committed_refs_clean=YES`.
- `pwsh scripts/check-architecture.ps1` PASS: `imports=249`, warnings=2.
- `pwsh scripts/check-melonds-policy.ps1` PASS: runner slots `13/13`.
- `pwsh scripts/check-docs.ps1` PASS: `docs=18`, `registryEntries=6`;
  rerun after the BUG_NOTES update also PASSed.
- `git diff --check` on the package files reports no whitespace error (only the
  existing CRLF conversion warning for `include/nds/nds_startup.h`).
- `docs/p2/BUG_NOTES.md` fidelity-02 now records the 10-entry weapon pool,
  pre-engagement high-water `0`, refusals `0`, ITCommonData rejects `0`, and the
  reproducible libnds allocator blocker. No pool-size change was made.

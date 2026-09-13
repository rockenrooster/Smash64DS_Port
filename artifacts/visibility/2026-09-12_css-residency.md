# CSS preview residency — 2026-09-12

## Scope and verdict

Part 1 source fix is implemented and compiles in the affected ARM9 objects, but
the required rebuilt-ROM runtime proof is **BLOCKED** by unrelated particle WIP
already present in the working tree. Part 2 was not started because the package
requires Part 1 to be green first.

## Retained pre-fix ROM identity

These hashes identify the retained 20:03 build used for the no-build
reproduction only. They do **not** contain the residency fix in this package.

- ROM: `builds/build-p2-shell/smash64ds-p2-shell-hwtri.nds`
  - SHA-256: `1837A59A18EEAA8FEB38EBA91E17E22B326A9FBAB419300573D944E126F3D225`
- ELF: `builds/build-p2-shell/smash64ds-p2-shell-hwtri.elf`
  - SHA-256: `F9FE2E9AC5A17B457B2E467CDE825D9DAD2492CDC3D44E0F8180742E63C04489`

## First divergence

- Boundary run `builds/verify-boundary-20260912-1953.log` halted in CSS at
  `PACKHALT reason=9 kind=6 scene=16 free=560648`: Yoshi compact-pack reload
  found an already-registered section asset.
- Before editing, the exact realtime no-build arm was rerun on runner slot 11:
  `-NoBuild -P2ShellFlow -FastIteration`.
- That rerun did not hit `PACKHALT`; it reached `scVSBattleStartBattle` and later
  failed the independent `M4_FENCE_FINAL` residency-lifecycle assertion. Output:
  `builds/codex-css-repro-20260912.log`.
- Generated compact-pack census rules out the proposed Kirby/Yoshi shared-file
  explanation: Kirby publishes `0xE5/0x148`; Yoshi alone publishes
  `0xF7/0x152`. The reason-9 state therefore required stale Yoshi registration.

## Fix

- Compact preview records now have explicit fighter-owner teardown before a CSS
  slot arena is reused. Teardown removes owner-tagged loaded-file records,
  matching status aliases, normalized asset state, resident metadata and the
  relative-offset memo; the existing range teardown still clears address-owned
  event/MObj/status state.
- Cancel, retire and failed-acquire paths all use that symmetric teardown.
- Extern-tree sliced loads now check depth before publication, open the stream
  before payload allocation, close/reclaim the active transaction on every
  FAIL/cancel, and reject a byte budget that cannot move one aligned word.
- The CSS-only Kirby 27-entry MainMotion copy table is restored on preview exit.

## Counters and proof status

Pre-fix retained-ROM rerun reached battle with `NATIVE_FAILURE=0,0,0,0,0,0,0,0`
and no reason-9 `PACKHALT`; its final unrelated marker was
`M4_FENCE_FINAL=3516,1,1,0,44,65408,1,0,4294966784,247,0,833,...`.

Post-fix CSS residency counters, reacquire soak, 30 Hz / two-VBlank menu cadence,
and preview-panel visibility counters are **not measured** because no valid ROM
containing this fix can currently be linked. No post-fix screenshots are claimed.

## Build blocker

`make TARGET=smash64ds-p2-shell-hwtri BUILD=build-p2-shell` stops before link in
unrelated particle work:

- `src/nds/nds_particle_banks.c`: generated table checksum is now `0x0BADFD59`
  while the source guard still requires `0x9362A565`.
- `src/nds/nds_renderer_preamble.c`: the pre-existing generator expanded the
  particle atlas from four to five sheets, causing the independent pinned native
  texture budget assertion (`... <= 8u`) to fail.

The generator change in `scripts/generate_nds_particle_banks.py` predates this
package. The second assertion is a real resource guard, so this package does not
weaken it or alter unrelated particle ownership to force a ROM.

## Completed verification

- Affected ARM9 objects compiled; `scene_backend.o` exports
  `ndsRelocReleasePreviewFighter`, `ndsRelocExternTreeSliceStep` and
  `ndsRelocExternTreeSliceCancel`; taskman/ftmanager export their new helpers.
- Required host pytest: `9 passed, 1 skipped, 10 subtests passed` using
  `C:\msys64\mingw64\bin\gcc.exe` on PATH. The skip is the optional second-host
  compiler comparison.
- `python scripts/check-untracked-dependencies.py`: PASS.
- `python scripts/check-native-owner-wiring.py`: PASS.
- `git diff --check` on package edits: PASS.

## Remaining gates

After the unrelated particle build is green: rebuild the canonical P2 shell ROM,
rerun `p2_battle_realtime` on runner slot 11, then run the CSS probe/soak and
capture all seven requested preview screenshots. Part 2 remains intentionally
unstarted until those Part 1 runtime gates pass.

# 2026-09-13 CSS compact-preview residency loop

## Defect and reproduction

Boundary `p2_shell_loop` deterministically stalls in `nSCKindPlayersVS` while
the CSS tour acquires previews in this order:

`load 0 Mario, load 1 Fox, load 4 Luigi, load 3 Samus, release 0 Mario,
load 5 Link, release 4 Luigi, load 6 Yoshi`.

The pre-fix ROM reaches `MS-LOAD-6` and then `ndsPreviewPackLoadHalt(9, 6)`;
frame markers stop. A breakpoint on the halt captured the collision before the
fail-closed spin:

- Yoshi section 1 (Model) asset: `0x152` (`YoshiModel`).
- Existing registry record: index 7, asset `0x152`, `reserved[0]=0`, scene 16,
  generation 4, data `0x022e4660`.
- The same registry walk after both preview releases still contained that
  tag-0 record. Mario/Luigi-owned preview sections retained nonzero owner tags;
  this `0x152` record was therefore not leaked by either release.

## Root cause

`sNdsPlayersVSSharedResidentAssetIDs` predates compact FPC1 preview residency.
It pins cross-fighter raw extern-tree assets once for the CSS, including
`0x152` because raw Kirby/Yoshi closure analysis shares `YoshiModel`. In the
hardware-triangle compact CSS configuration, however, Yoshi's FPC1 pack owns
asset `0x152` as its section-1 Model record. The raw shared record is registered
first with `reserved[0]=0`, so the compact acquire correctly rejects the
duplicate asset ID. `ndsRelocReleasePreviewFighter` cannot and must not retire
that tag-0 shared record; its acquire/release symmetry was not the leak.

## Fix

`src/import/battleship_mnplayersvs.c` no longer pins raw `0x152` in the shared
CSS arena when `NDS_PLAYERS_VS_COMPACT_PREVIEW` is enabled. Raw/source-oracle
profiles retain the original shared asset. The compact pack remains the sole
registry owner of Yoshi's `0x152` section, so the existing fail-closed collision
check and owner-tagged release contract remain unchanged.

## Verification

- Host checks:
  - `python scripts/fighters/test_preview_pack_loader.py`: PASS, 6 tests,
    1 skipped. The first invocation exposed that host GCC was not on `PATH`;
    rerun with `C:/msys64/ucrt64/bin` prepended passed.
  - `python scripts/fighters/test_prepared_dense_residency.py`: PASS, 9 tests.
  - `python scripts/check-untracked-dependencies.py`: PASS.
  - `pwsh scripts/check-architecture.ps1`: PASS (2 existing warnings).
  - `pwsh scripts/check-melonds-policy.ps1`: PASS. The checker now ignores
    inaccessible recursive cache directories while enumerating `*.ps1`; the
    untracked `scripts/.pytest_cache` otherwise aborted the source-policy scan.
  - `pwsh scripts/check-docs.ps1`: PASS.
- Rebuilt native-only target: PASS. `make TARGET=smash64ds-p2-shell-loop-hwtri
  BUILD=build-p2-shell-loop` completed under the shared build lane and reported
  `NATIVE_ONLY_PASS` over 262 actual link inputs. ROM SHA-256
  `397042BFF80E901E02107D0527D9FE869F3A4B251E9564AFE82E98D495B83BA8`;
  ELF SHA-256
  `3DDE7BE7482717425BC58CABCFB56349A238DDF48038CE63C5F66FD8C645D75D`.
- Rebuilt fast CSS-tour: PASS. The marker stream repeated the reproducing
  sequence through `MS-RELEASE-4`, `MS-LOAD-6`, then continued through frame
  5,520, `MS-CSSCOMMIT`, Yoshi's `MS-RELEASE-6`, `MS-SSS`, and `MS-BATTLE`.
  No `MS-COLLIDE` or `MS-EXCEPT` marker appeared. The capture completed in
  129.9 seconds of its 300-second ceiling.
- Standalone `verify-p2-shell-loop.ps1` verdict (03:32, slot 8, -Loops 1): the lap
  completes (LAP PATTERN 4 looped entries from entry 6, exact; ARENA FREE FLOOR
  114,628 B) and FAILS one assertion: `HIGHWATER PlayersVS : spread 27960 B over
  2 entries exceeds 8192 B` (series 753164,781124). The reason-9 halt is gone;
  the second character-select entry keeps 27,960 B more arena than the first.
  This is the retained failing baseline for the follow-up attribution.

## Arena-spread attribution

The 03:32 failure was an owner-lifetime bug in native fighter preview images,
not a loaded-file-registry leak. Before the repair, compact CSS acquisition
restored the scene general heap before `ndsMNPlayersVSPreviewPrepareResidentKind`.
That preparation calls `ndsRendererNativeEnsureOwnerImage`, so each admitted
preview's high/low native owner images were allocated from the scene heap even
though their logical owner was the fighter's resettable CSS resident block.
Retiring a resident block did not invalidate those static owner-image slots.
The escaped allocations were path-dependent: measured image-pair sizes on this
ELF are Luigi 19,088 B, Samus 33,024 B, Link 35,612 B and Yoshi 26,644 B.

The current shared-tree repair keeps preparation inside the resident block's
malloc region and retires image slots whose backing address lies in that block
before `syMallocReset`. A slot can therefore survive only as long as its block.
The loaded-file registry follows the same block release via
`ndsRelocReleasePreviewFighter`; no prior-preview owner tag survives as a live
compact record on the next CSS entry.

A slot-8 instrumented replay of the exact one-lap verifier walk on the current
candidate measured first/second PlayersVS high-water 732,220 / 729,788 B
(spread 2,432 B), with the same fixed entry allocations and no general-heap
native-owner-image allocation. The registry at CSS entry was tag-0 only; preview
owner tags were retired/replaced within each visit. Raw marker capture:
`builds/cssspread-current-instrumented.gdb.out`; verifier summary:
`builds/cssspread-exact-run.out`. This diagnostic pass is attribution evidence;
the uninstrumented rebuilt verifier gates below still own acceptance.

## Arena-spread follow-up verification

Required host checks on the owner repair all pass. Both host-C tests initially
reported that no compiler was on `PATH`; rerunning them with
`C:/msys64/ucrt64/bin` prepended gave `test_preview_pack_loader.py` PASS (6
tests, 1 skipped) and `test_prepared_dense_residency.py` PASS (9 tests).
`check-untracked-dependencies.py` PASS; `check-architecture.ps1` PASS with its
2 existing warnings; `check-melonds-policy.ps1` PASS; `check-docs.ps1` PASS.
The shared build lane reported `ACQUIRED codex-cssspread`; after correcting the
Codex shell's devkitPro/Python path environment, the exact shell-loop target
rebuilt and reported `NATIVE_ONLY_PASS` over 262 actual link inputs. The lane
was released before emulator use. Candidate ROM SHA-256 is
`6D1A596EFECE3544F26875EC82B05A3150315D73B90057B8B3AEA4B4FF72FDA4`;
ELF SHA-256 is
`B8E77CF90702089E37F4D44FA06BAE45A37FE6CE78DFCBCDE1692460983540A7`.
Uninstrumented slot-8 `-Loops 1` PASS: PlayersVS 732,220 / 729,788 B
(2,432 B spread), free floor 114,628 B, exact four-entry lap pattern, zero
native failures/faults. Console capture: `builds/cssspread-verifier-loops1-final.out`;
permanent verifier artifact: `artifacts/verification/2026-09-13_p2-shell-loop.txt`.
The first `-Loops 3` attempt exposed an independent re-entry lifetime bug after
lap 1: ARM9 data-aborted in `lbParticleMakeChildScriptID` while dereferencing
common particle script 91 (`bank_id=0`). Results had initialized its particle
bank in the Results taskman arena; the native CSS preview subset omitted the
source `efParticleInitAll(); efManagerInitEffects();` pair, so PlayersVS rewound
that arena and later preview code followed the stale bank-0 script table.
BattleShip `mnPlayersVSFuncStart` performs both initializers before fighter setup
(source lines 4753-4757). The native CSS owner now does the same before any
preview fighter can run. The required host/build/verifier gates are rerun on
that final candidate below. After this re-entry fix, the required host suite
again passes: preview-pack loader 6 tests/1 skipped, prepared-dense residency 9
tests, untracked-dependency check, architecture check (2 existing warnings),
melonDS policy check and docs check.

The final rebuild again acquired `codex-cssspread`, compiled the changed CSS
particle-owner path, and reported `NATIVE_ONLY_PASS` over 262 actual link
inputs; the lane was released before emulator use. Final ROM SHA-256 is
`DAEFD02E3E5800FA32AA0D5CF47F7DEAA6F0ED8F6AC6D784C7891B3A70FA7A8D`;
final ELF SHA-256 is
`6D57D95320FCF9E846527C063AB333347907C6FCAF143DB9352E96F432C5634C`.

Final uninstrumented slot-8 `-Loops 1` PASS on those hashes: PlayersVS
862,736 / 863,532 B (796 B spread), free floor 114,628 B, exact four-entry
lap pattern, zero native failures and zero faults. Console capture:
`builds/cssspread-final2-loops1.out`; permanent verifier artifact:
`artifacts/verification/2026-09-13_p2-shell-loop.txt` (the three-lap run below
replaces this rolling verifier artifact with its final capture).

## 2026-09-13 CSS residency hardening follow-up

Plan: close the review defects around resettable preview ownership before changing
the verified walk. Unbind native owner-image runtime tables before a resident block
is reset; keep animation-cache arena allocation outside that block; give compact
and source/oracle arms identical prepare/cancel lifetime rules; remove shipped
configuration warnings; publish a zero-only resident-overflow witness; add a host
check that shared CSS pins cannot collide with FPC1 section IDs; then rerun the
required host/static suite, rebuild the exact shell-loop ROM under the shared lane,
and require the uninstrumented three-lap slot-8 verifier to pass on that identity.

Host pin guard landed as `scripts/fighters/test_preview_shared_pin_disjointness.py`.
It regenerated all 12 FPC1 preview packs from source metadata and passed: the
7 compact shared CSS pins are disjoint from every emitted section asset ID.
Current generated pack file sizes range from 11,568 B (Fox) to 34,436 B (Link);
Donkey's 18,732 B pack includes its third/tail section.

Focused host/static hardening checks before the rebuild are green:
`test_preview_pack_loader.py` ran 6 tests (1 optional compiler skip),
`test_prepared_dense_residency.py` ran 9 tests, the new 12-pack pin guard
passed, `check-untracked-dependencies.py`, `check-architecture.ps1`,
`check-melonds-policy.ps1`, and `check-docs.ps1` passed, and
`verify-all.ps1 -Profile Boundary -List` still lists the three retained
Boundary runtime arms.

Continuation recheck: the clean-tree dependency guard, architecture checker
(the same 2 existing warnings), melonDS policy checker, docs checker and
`verify-all.ps1 -Profile Boundary -List` all passed again. `verify-all.ps1`
now has 14 unconditional static `Invoke-VerifyScript` calls plus the optional
registry call, matching `$expectedVerifiers = 14 + plan + registry`.

The first hardening rebuild attempt acquired `codex-cssharden` only after the
shared lane reported FREE, then failed before compilation because the recursive
make path resolved as `/opt/devkitpro/msys2/usr/bin/make` under the wrong MSYS
shell (`Error 127`). The lane was immediately released; no ROM from that failed
attempt is evidence.

After putting devkitPro's own MSYS2 `usr/bin` first on `PATH`, the lane again
reported FREE and `ACQUIRED codex-cssharden`. The exact shell-loop build then
completed and the existing packaging gate independently rechecked
`NATIVE_ONLY_PASS` over 262 actual link inputs. The lane was released before
emulator use. Hardening candidate ROM SHA-256 is
`64128C05CB87C45CD1733C30F1076F572F27F076634244D54878D30C9AF198C0`; ELF
SHA-256 is `69D8D099601B24514437A8523CA54F414FF406D84EDC9B6BBE97C6B936C9BCFB`.

Final standalone slot-8 proof on those hashes PASSed at `-Loops 3`: `LOOPDONE`
reported `walkloops=3`, `rematch=3`, zero scene rejects/mismatches, zero pose-slot
exhaustion and `rescap=0`. Native failure count remained zero. PlayersVS high-water
over four entries was 861,100..863,532 B (2,432 B spread); VSBattle spread was
332 B; arena free floor was 114,460 B; the 12-entry loop pattern was exact with
zero Sudden Death re-entries. Console capture is
`builds/cssharden-shell-loop-3-final.out`; the rolling permanent capture is
`artifacts/verification/2026-09-13_p2-shell-loop.txt`.

## Review residuals after the hardening package (orchestrator, 2026-09-13 06:40)

The 06:14 review found no gating defect; these stay open for the next CSS pass:

- Unbound owner tables index NULL rather than failing closed: make
  `ndsRendererNativeFighterTablesForResolvedRoot` (both definitions in
  `nds_renderer_assets.c`) return NULL when `tables->epochs == NULL`.
- The capacity check re-derives image sizes with its own `sizeof` switch; export
  `ndsRendererNativeOwnerImageBytes` and call it instead.
- Kirby's copy-hat image slot is outside `ndsRendererNativeReleaseOwnerImagesInRange`;
  invalidate it when its base falls in the released block.
- Two block resets in `battleship_mnplayersvs.c` (compact acquire head, oracle
  slice-begin failure) run without the range release; call it before both.
- `test_preview_shared_pin_disjointness.py` parses the pin array loosely; assert
  the parsed count equals the entry count. The script is untracked and must be
  committed with `verify-all.ps1`.
- `rescap=0` proves the capacity check never fired, not that the margin is right;
  `syMallocSet` still halts on an overflow that slips past it.

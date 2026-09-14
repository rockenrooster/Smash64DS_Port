# 2026-09-14 P2-6 1P follow-ups

## 2026-09-14 — Step 1: 1P Intro residency

- Tried making `nSCKind1PIntro` a registered BATTLE scene so the same
  `gNdsSceneManagerCurrIsBattle` predicate as the first fight selected compact
  fighter residency. The probe witnessed `CPFPCBEGIN scene=14 battle=1` for
  Mario and Link and `CPPACKKIND scene=14 kind=5` for Link.
- That configuration is not valid for the source intro: Link reaches the native
  renderer through a packed preview and then deliberately halts with reason 20,
  kind 5 (`renderer_adapter_fighter.c:4123-4130`) because the packed root has no
  admitted native owner for that draw. Source `sc1pintro.c:998-1000` explicitly
  creates the intro opponent at `nFTPartsDetailLow`; its setup loads the source
  fighter-file set through `ftManagerSetupFilesAllKind` (`sc1pintro.c:1895+`).
- Restored the existing MENU registration / full fighter-file residency while
  retaining the taskman's intro-specific native 3D and arena budget. The follow-up
  run reached the first fight with no compact `CPPACKKIND` in scene 14.
- Heap witness: `CPRINGFREE prev_kind=14 prev_free=148784`; floor 25,600 B,
  margin 123,184 B. Final active minimum after entering fight was 44,244 B.
- Failed compact evidence: `artifacts/verification/2026-09-14_1p-intro-residency.txt`.
  Accepted full-path evidence: `artifacts/verification/2026-09-14_1p-intro-fullpath.txt`.
  Intro capture: `artifacts/visibility/2026-09-14_1p-intro-fullpath.png`.
- Lab build: `NATIVE_ONLY_PASS`; ROM SHA-256
  `E5CBE9B8AB76326FA91991E01C0B28CBAF5B39E1201A8715564C8ADAF683A257`, ELF
  `F43D305D379FF58C649AD8395059E75FDAC46DC2EC1E0250269F4B5A50BAC60A`.

## 2026-09-14 — Step 2: compact flag ownership

- Compact residency is runtime-owned by the registered battle scene:
  `ftManagerSetupFilesAllKind` takes the compact closure only when
  `gNdsSceneManagerCurrIsBattle != 0` (`battleship_ftmanager.c:276-287`), and
  `ndsRelocLoadPreviewFighterUnlocked` chooses `fighters/battle/` from that same
  owner (`reloc_preview_pack.c:298-318`). Compact extern patching also refuses
  non-battle scenes (`reloc_preview_pack.c:508-511`). No `NDS_P2_1P_GAME` test
  selects compact residency in either owner.
- Existing VS-shell config: `NDS_P2_1P_GAME=0`, compact=0, menu shell=1.
  Existing four-CPU stress config: `NDS_P2_1P_GAME=0`, compact=1, four-CPU=1.
  The stress lane independently requests compact battle files; neither config
  links the 1P campaign merely to obtain the residency predicate.
- Remaining `NDS_P2_1P_GAME` gate inventory from `src`/`include` (generated files
  excluded): `ftboss*` + `wpbossbullet` = Master Hand source owners; `ftmanager`
  = campaign variant preload/Boss/detail support while compact selection stays
  shared; `ftcommon_entry/get/shieldbreakfly` = campaign strong gameplay owners;
  `grbonus3`/`grpupupu_ground` and campaign reloc/collision rows = 1P/bonus stage
  geometry and collision; item/map/tarubomb rows = campaign item strong owners.
- `mn1p*`, `mnplayers1p*`, `mncharacters`, `mncongra`, `mnmessage`, `mntraining`,
  `mvending`, `sc1p*`, `scautodemo*`, `scexplain*`, `scstaffroll` = source 1P
  menus/scenes; shared `mnbackupclear/data/option/screenadjust/soundtest/vsrecord`
  use MENU_SHELL-or-1P gates because both routes reach them; `mnvsresults` has
  the campaign result arm.
- `nds_audio_bgm.*` = 1P BGM/chunk tables; renderer/native-asset rows = campaign
  fighter/art/visual contracts; `reloc_backend_assets` = campaign assets plus
  shared compact support; scene/taskman/title files = campaign registration,
  pumping and non-campaign stubs; diagnostics/reference rows are proof-only.
- Audit-only occurrences in currently owned files (`nds_match_config.c`, menu
  shell, renderer files, item/weapon imports, compatibility shims) were left
  untouched.

## 2026-09-14 — Step 3: victory, tally and next-stage transition

- Source ownership is already wired through the imported manager:
  `battleship_sc1pmanager.c:63-69` includes the BattleShip manager and forwards
  `sc1PManagerUpdateScene`. Source `sc1pmanager.c:385-403` returns from the
  natural fight and classifies a loss only when the player stock is `-1` or
  time expires; loss calls `mnPlayers1PGameContinueStartScene` at :403-408.
- The source win arm counts bonus flags, sets `scene_prev=1PGame`,
  `scene_curr=1PStageClear`, calls `sc1PStageClearStartScene`, then increments
  `spgame_stage` (`sc1pmanager.c:444-473`). StageClear snapshots score and all
  three bonus masks (`sc1pstageclear.c:1659-1665`) and writes the final total
  back before requesting load-scene (`:2083-2095`).
- Final allowed slot-7 launch used the campaign lab ROM and ordinary controller
  playback only. It reached the first battle and GO, `CPSETUP count=1`, then
  completed 197 presented GO frames before the fixed 180-second verifier ceiling.
  No CPU abort, heap overflow, asset error, preview-pack refusal, StageClear or
  Continue marker occurred before timeout.
- Because this transition mode traps every presented frame to synthesize ordinary
  input, debugger overhead consumed the proof window before a natural KO. All
  three launches allowed by this brief are now used; no fourth launch was made.
- Gap for the next package: move the same ordinary battle-input pattern into a
  guest-side bounded playback feeder (or otherwise remove the per-frame GDB trap),
  then prove `52 -> 51 -> 14 -> 52`, `CPWINROUTE` opponent stock exhaustion,
  `CPTALLY-FINAL` score/bonus values, and a non-clear StageClear capture. No tally
  asset/scene dependency was exposed because scene 51 was never entered.
- Evidence: `artifacts/verification/2026-09-14_1p-victory-transition.txt`.
  The requested `artifacts/visibility/2026-09-14_1p-stageclear.png` was not
  produced, so the victory/tally package remains open rather than stubbed.

## 2026-09-14 — Step 4: shipping restore and host gates

- Rebuilt the root target under the shared build lane with explicit
  `NDS_P2_1P_GAME=0`; Makefile default is also `NDS_P2_1P_GAME ?= 0`
  (`Makefile:703`). Link audit: `NATIVE_ONLY_PASS: smash64ds.elf, 262 actual
  link inputs`.
- Shipping ROM `smash64ds.nds` SHA-256:
  `08615354F6F132EECA4415406F6399F88911913071237D82960A675CA78255BE`.
  Shipping ELF `smash64ds.elf` SHA-256:
  `CF27DEB3A700A834C2CEEE74682FFCEC443EDEC9D143FB0E7E44D01E3A53CDD8`.
- `python scripts/check-untracked-dependencies.py`: PASS.
- `pwsh scripts/check-architecture.ps1`: PASS (two pre-existing warnings for
  large-file split/generated outputs).
- `pwsh scripts/check-melonds-policy.ps1`: PASS.
- `pwsh scripts/check-docs.ps1`: PASS before and after the P2-6/BUG_NOTES edit.
- Campaign capacity pytest initially could not resolve a host `gcc`; devkitPro's
  MSYS `usr/bin` does not provide it here. With existing Python 3.13 retained and
  `C:\msys64\ucrt64\bin` prepended solely for its host GCC requirement, the exact
  requested test passed: `2 passed in 0.62s`.

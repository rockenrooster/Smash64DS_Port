# P2-6 — 1P entry and first ordinary fight

Date: 2026-09-13

## Candidate identity

- Target/build: `smash64ds` / `build-p2-campaign-walk`.
- Resolved ROM: `D:\Stuff\DevFolder\Smash64DS_Port\smash64ds.nds`.
- ROM SHA-256: `E1B4976147DB3AB255A8D8EEA70EE97297CA321AABF033EBA4F5E7E571DCB03A`.
- Resolved ELF: `D:\Stuff\DevFolder\Smash64DS_Port\smash64ds.elf`.
- ELF SHA-256: `42303E7EDA04DD6A5DA62AA2B9901D7E9E31FC60D47A32C7D14ED12FE3397904`.
- Build config: `NDS_P2_1P_GAME=1`, `NDS_P2_MENU_WALK=1`, `NDS_HARNESS_FAST_LOGIC=0`.
- Native-only build proof: `builds/codex-1p-native.out` — `NATIVE_ONLY_PASS: smash64ds.elf, 316 actual link inputs`.

## Source contract

- First ordinary encounter is Hyrule with Link: `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c:300,304`.
- Manager copies player fighter/costume/stock into the persistent 1P state before the route: `sc1pmanager.c:284-286`; it enters the intro at `:366` and the ordinary fight at `:385`.
- The DS bridge points `gSCManagerBattleState` at `gSCManager1PGameBattleState`, increments one bridge-application witness and starts source `sc1PGameFuncStart`: `src/import/battleship_sc1pgame_runtime.c:139,142,167-173`.
- Source CSS commits difficulty, stock and costume itself: `mnplayers1pgame.c:3266,3268,3276`.

## Route proof

Slot-7 dry run (`artifacts/verification/2026-09-13_p2-campaign-dryrun3.txt`) reached the source route through the first ordinary battle setup:

- Scene prefix: Startup → Title → ModeSelect → 1PMode → 1P CSS → 1PMode → 1P CSS → intro → 1P battle (`27→1→7→8→17→8→17→14→52`).
- First CSS visit used ordinary source B after the entry gate and returned to 1PMode (`back=1`).
- Second CSS visit changed difficulty `1→2`, stock `2→3`, selected Mario (`fkind=0`) and changed costume `0→1` through source menu input (`CPCSSMUT`).
- Intro native transient renderer engaged: at the capture stop `updates=40 submits=80 draws=1320 rejects=0`.
- Source intro audio calls executed: BGM `35` (`nSYAudioBGM1PIntro`) once, then announcer FGM `499` (Mario), `532` (Versus), and `497` (Link). These are execution-breakpoint witnesses using live call arguments, avoiding stale cached-global reads.
- `sc1PGameSetupStageAll` engaged exactly once before the failure (`CPSETUP count=1 stage=0`), with the active campaign state pointer already installed by the 1P bridge.

The run did not reach GO. Its first functional failure was a real general-heap overflow during Link residency: `syMallocSet(size=104960)` from `ftManagerSetupFilesMainKind(fkind=5)` had 51,100 B free. Backtrace: `battleship_sys_malloc.c:133,173` → `ftmanager.c:285,358` → `sc1pgame.c:2166`.

## Resource and sustained-fight proof

BLOCKED before GO by the Link full-main allocation above. The current scene arena reported 1,007,360 B on the natural prefix. The failed request needs 104,960 B with 51,100 B remaining, so the immediate shortfall is 53,860 B before alignment/any later allocations. No 600-frame or active-scene floor claim is made.

The existing compact battle-fighter owner would avoid this full-tree allocation, but its battle mode is VS-only:

- `src/port/reloc_preview_pack.c`: `ndsRelocLoadPreviewFighterUnlocked` admits/binds `battle/*.fpc` only for `nSCKindVSBattle` (around lines 299-325).
- `src/import/battleship_ftmanager.c`: compact battle dependency publication is likewise gated on `scene_curr == nSCKindVSBattle` (around lines 275-286).
- This package does not own either file. Extending that existing compact battle path to `nSCKind1PGame` is the named integration gap; P2-6 must not spoof `scene_curr`, duplicate the loader, or add a restore wrapper to bypass it.

## Visual/audio evidence

Accepted intro capture:

- `artifacts/visibility/2026-09-13_1p-intro.png`
- `assert-melonds-top-visible.ps1 -WindowScaledCapture`: 49,152/49,152 top-screen pixels differ from the clear color (100.000%); detail fraction 99.202%.
- The capture visibly shows the selected Mario alternate costume in the native 3D intro path; native transient counters report zero owner rejects.

The first failure occurred before the required fight capture, so this remains open:

- `artifacts/visibility/2026-09-13_1p-first-fight.png`

## Host checks

- `python scripts/check-untracked-dependencies.py` — PASS.
- `pwsh scripts/check-architecture.ps1` — PASS (2 existing warnings).
- `pwsh scripts/check-melonds-policy.ps1` — PASS.
- `pwsh scripts/check-docs.ps1` — PASS.
- `python -m pytest -q scripts/menus/test_campaign_fighter_capacity.py scripts/menus/test_training_runtime.py` — PASS, 4 tests; one pytest cache-permission warning only.

## Known input-reachability gap under review

The source CSS accepts four C-button costume choices, but live DS mapping currently maps X/Y only to source `U_CBUTTONS` (`src/port/controller_backend.c:49`) while R maps to `R_TRIG` (`:51`). The runtime probe uses the existing DTCM playback pad to send source `R_CBUTTONS` so it can verify campaign costume-state ownership without changing gameplay/menu state. Human-DS reachability of an alternate 1P costume remains a named gap unless another existing input seam provides it.

## Verdict

BLOCKED — natural menu/CSS/intro (including source BGM + Mario/Versus/Link announcer calls) and source-stage-setup prefix is proven through one setup call, then the 1P battle exhausts its arena loading Link because the existing compact battle-fighter path is VS-only. GO, 600 presented fight frames, active-scene floor and final fight screenshot remain open.

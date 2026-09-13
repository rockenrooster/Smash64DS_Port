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

## Cycle 2 — 2026-09-13

The compact residency predicate now follows the registered battle-scene owner instead of the VS literal: `src/port/reloc_preview_pack.c:301,313` and `src/import/battleship_ftmanager.c:278` use `gNdsSceneManagerCurrIsBattle`, published by `src/port/nds_scene_manager.c:338`. This admits the same compact `battle/*.fpc` path for the 1P ladder without spoofing `scene_curr`.

Scratch-reservation census before relying on the reclaimed 153,600 B: the shell config is HW triangles (`builds/build-p2-shell/nds_build_config.h:6`). The only source producer of `ndsPlatformBeginOriginalSpritePreview` is host-only staff-roll reference code (`src/host/graphics_reference/staffroll_reference.c:309`, commit at `:329`); the other host reference only reads the preview epoch (`src/host/graphics_reference/sprite_reference.c:1030`). The platform allocator is therefore only reachable through that host producer (`src/nds/nds_platform.c:645`), while the old framebuffer consumer is `#if !NDS_RENDERER_HW_TRIANGLES` (`src/nds/nds_platform.c:1817`, call at `:3171`). The live ROM sprite backend explicitly routes wallpapers to `ndsNativeWallpaperDraw` (`src/port/sprite_preview_backend.c:575,617`), which writes the native overlay layer (`src/nds/nds_native_wallpaper.c:324`) and commits it directly (`:340`); UI owners use the same native overlay API (`src/nds/nds_ui_kit.c:917,968,1020,1144,1222,1757`). `src/port/taskman_seam_core.c:4257` only clears preview state. No shipped battle HUD/effect/results producer was found for the old 320x240 staging buffer, so `src/port/nds_scene_manager.c:360` keeps the reservation removal.

Cycle-2 lab identity captured before the natural run: `smash64ds.nds` SHA-256 `3F2FCE61A5F61936352A4190736FE2E5C4D1C9C6074A8F5B6E196C66CA24DD3B`; `smash64ds.elf` SHA-256 `D1DB4F46A00F241EBD474893C1ABD4A58389A824C29C2B3DA8FC1D2BA850315C`.

The post-fix lab ROM used for the 600-present window was `smash64ds.nds` SHA-256 `2E3B59BA8D5F6CE12B2FA15293BC042419C51EB1531A14A49DD5FE4B54C19263`, with `smash64ds.elf` SHA-256 `47B606387FD29A942D54C055E9D7AD35CD12900380ABC680D18CA9CBCFAD40FB`. Its generated config records `NDS_RENDERER_HW_TRIANGLES=1`, `NDS_HARNESS_FAST_LOGIC=0`, `NDS_P2_1P_GAME=1`, `NDS_P2_COMPACT_BATTLE_FIGHTERS=1`, and `NDS_P2_MENU_WALK=1` (`builds/build-p2-campaign-walk/nds_build_config.h:4,6,126,158,207`). This is an explicit lab configuration: `Makefile:897` still defaults `NDS_P2_COMPACT_BATTLE_FIGHTERS` to 0 and only the four-CPU target forces it at `Makefile:2720`. Shipping 1P therefore still needs an owner decision to tie compact battle residency to `NDS_P2_1P_GAME` or change the default and re-run Boundary; this cycle does not change that Makefile policy.

Natural route witness from `artifacts/verification/2026-09-13_p2-campaign-cycle2.txt`: Startup/Title/Main Menu/1P Mode/1P CSS/back-out/re-entry reaches the first 1P fight (`CPLINE` rows 1-9), with the source CSS mutation `difficulty 1->2`, `stock 2->3`, Mario `costume 0->1`, selected=1 and back=1. `CPSETUP count=1` proves source campaign stage setup ran once. GO appears at `total_present=198`, `pacing_present=195`, `updates=392`; the sustained window completes `CPFRAMES total=797 go=600 pacing_present=794 logic=1588`. The active state is the 1P battle owner (`owner1p=1 applied=1 refused=0 setup=1`).

Memory witness: the first active-scene sample is `CPSTARTUP free=999168 ... arena=999168`; the sustained fight reaches `CPHEAP active_free_min=79948 guest_free_min=79948 arena=999168`. The compact path is paid in the live first fight: `CPFPCBEGIN kind=5 free=213228` reaches `CPPACKKIND kind=5 free=176654`; the allocation census independently records the compact Link FPC allocation as exactly 33,520 B (`2026-09-13_p2-campaign-cycle2-alloc-census.txt:97`), followed by its bounded extern/dependency allocations. The census also shows a separate 153,600 B lazy battle allocation immediately after startup (`:79`); the follow-up runtime witness requested by review will distinguish that animation-cache carve from the reclaimed dead sprite-preview reservation before final closure.

The first long run exposed source display-list kind 1 exceeding the shared 32 B battle reservation by 208 B. `src/import/battleship_sc1pgame_runtime.c:172` now gives 1P Game DL1 64 Gfx (512 B) after the shared battle rebudget, preserving the shared DL0 admission key while covering the measured 240 B requirement. The 600-present rerun confirmed that DL1 refusal is gone. That same sustained run later records a separate DL0 bound (`dl_kind=0 dl_bytes=112`); the functional 600-present requirement is satisfied, but the probe's graphics-buffer gate remains open until the shared battle DL0 owner is re-budgeted or otherwise proven. Source taskman allocates DL0/DL1 before `func_start`, so an owned `sc1p*` callback cannot safely resize DL0 afterward.

Visual proof is `artifacts/visibility/2026-09-13_1p-go.png` and `artifacts/visibility/2026-09-13_1p-first-fight.png`. `assert-melonds-top-visible.ps1 -WindowScaledCapture` reports 49,152/49,152 top-screen pixels non-clear for each (100.000%); GO has 48,437/49,152 detail pixels (98.545%), and the mid-fight capture has 48,697/49,152 (99.074%). The images show Mario versus Link on Hyrule at GO and later active combat respectively.

After restoring the root shipping configuration (`NDS_P2_1P_GAME=0`), `builds/codex-1p-shipping-build.out` records `NATIVE_ONLY_PASS` with 262 actual link inputs. The restored `smash64ds.nds` is 54,307,840 B, mtime `2026-09-13T17:17:42`, SHA-256 `3B2929856964FFD6981774402288F96FCDAD3A2785F6917F6BC8E314592CDEFD`; its ELF SHA-256 is `8E46943894FD4A72A234DACBF5B07FAD8CA11682A75A10903D87298E5F025035`.

1PIntro remains a consistency gap. `nSCKind1PIntro` is still registered as a MENU scene even though `battleship_sys_taskman.c` treats the intro as battle-like, so the source intro's LOW-detail actors still take the full fighter-file path instead of compact battle admission. The banked run enters intro with a 999,168 B arena (`CPLINE 8 curr=14`) and reaches 1P Game (`CPLINE 9 curr=52`) with the cumulative allocator-failure count unchanged at 178; `CPINTRO updates=40 submits=80 draws=1320 rejects=0` proves the full intro load fits and completes. The transcript did not print `gNdsSceneManagerRingArenaFree`, so the exact free bytes immediately before/after Link's intro load and therefore its numeric margin are not recoverable from the banked proof. The probe now has a `CPRINGFREE` witness, but the exact `2E3B59BA...` lab ROM was not retained after the shipping restore; rebuilding would create a different identity and can absorb unrelated dirty work, so that numeric ring-free witness remains open.

Review witnesses already present in the Cycle-2 transcripts are clean: `2026-09-13_p2-campaign-cycle2-600-pass2.txt:89` reports `CPPACK ... failure=0 kind=0`; cumulative battle-core extern patches progress 0→11 across Mario and 11→18 across Link (`:78-81`); scene-manager rejects stay at 0 through the route, and first 1P Game entry is `curr=52 enters=8` (`:69`). `gNdsSceneManagerRingArenaFree` is the only requested review witness not banked by the exact proof ROM. The configuration gap above remains unchanged: the proof ROM explicitly had `NDS_P2_1P_GAME=1`, `NDS_P2_MENU_WALK=1`, `NDS_HARNESS_FAST_LOGIC=0`, and `NDS_P2_COMPACT_BATTLE_FIGHTERS=1`, while shipping 1P still does not imply compact battle residency.

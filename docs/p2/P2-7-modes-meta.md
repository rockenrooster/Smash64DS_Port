# P2-7 — Persistence, Modes, Presentation and Final Closure

Qualify the existing save, scene and menu imports. Bring persistence/progression dependencies forward for their real consumers; do not wait until “polish” or restart an implemented backend because a historical pin sheet says NO SAVE.

## Package: persistence on the actual medium

**Outcome:** Correct fresh-save defaults, validated save/reload, source-compatible records/options and recoverable interrupted/corrupt writes through the DS backend.

Reuse `nds_backup.c`, the `lbbackup` import and existing host fault tests. The original proposal's write-new/rename description is not authority over landed code; inspect its actual format/version/checksum/slots and transaction sequence. Test what the backend guarantees instead of assuming a rename alone is power-loss safe.

Writes occur at explicit source-equivalent safe boundaries, not uncontrolled active-match I/O. Record who owns the save path/buffer, when a write finishes, and how failure is reported/retried without freezing the game or double-applying results. Diagnostic runs use disposable save data and must never consume or clear the owner's normal save.

**Proof:** Host fresh/corrupt/truncated/old-version/interrupted-write cases; actual file write, close and fresh process boot; prior valid slot recovery; full invalid defaults and source correction rules. Check that imported scene callers really invoke persistence. Retail-specific validation is used only where needed by current owner/platform policy; ordinary performance remains on the custom accurate melonDS.

## Package: unlocks and progression events

**Outcome:** Source US predicates, challenger/message routes, masks and unlock state persist correctly, with no development override in the production configuration.

| Unlock | Source-derived condition to test |
|---|---|
| Luigi | Bonus 1 completes all ten tasks for every starter |
| Ness | 1P Normal or above, zero continues and source zero-based stock index `< 3` (1–3 stocks) |
| Captain Falcon | US total time below the source 12-minute threshold; do not substitute the JP 20-minute rule |
| Jigglypuff | Source 1P-clear fallback after the higher-priority checks |
| Mushroom Kingdom | Source all-ground mask plus 1P-complete starters, tested at both VS and 1P trigger sites |
| Item Switch | Source VS battle counter at or above 100 |
| Sound Test | Both bonus types complete 10/10 for all twelve fighters |

These conditions are pinned in the pre-revision source sheet; verify their exact source branches when editing them. Test just-below/exact/above thresholds and disqualifying conditions; priorities matter when several qualify together. Challenger win, loss, source level-drop and return menu must update/preserve the right state. Fresh save means **source starter defaults**, not literally every fighter locked. Completed engineering admission and player-earned unlocks are separate masks/decisions.

## Package: Training

**Outcome:** Natural Training selection→stage→battle/menu→reset/exit with native presentation and source CPU/item/speed/view controls.

Use source menu categories CP, Item, Speed, View, Reset and Exit; source CPU options, item maximum/availability, speeds and views are data, not approximations. Shared item/fighter/renderer owners stay shared with VS. Reduced Training speed changes the source simulation behavior while presentation remains serviced; it must not starve input/audio or run a source frame twice. Damage/combo readouts must show the source-defined counters.

**Proof:** Change each option, create source-allowed items and reach its limit, run attacks/combos, switch speed/view, reset repeatedly and exit via the natural menu. Observe memory/object reset, correct selections and no record pollution. A registered Training scene without a working in-battle menu is not completion.

## Package: Data, Records and Sound Test

**Outcome:** Parent menu and every original child work, show the correct source/save data and return correctly.

Characters/profile presentation must render its required models/art/text; VS records need source table dimensions, scrolling/sorting/caps where applicable. Do not invent an additional screen simply because a saved statistic exists. Sound Test's source Music/Sound/Voice selection and playback/stop behavior must use the real audio backend and unlock state. Do not conflate Options mono/stereo with undocumented volume controls.

**Proof:** Fresh and seeded disposable saves, known record values/caps, navigation/cancel, every child route, native assets/cadence and audible selected samples. Where the parent is reachable but a child silently returns, classify the child failure rather than “Data absent.” Use output plus state evidence.

## Package: Options and Backup Clear

**Outcome:** Source sound/flash settings and the approved DS-specific options treatment work and persist; clear confirmations erase only the requested record classes.

Preserve owner-accepted native visuals. Source Screen Adjust is N64-specific: retain only the documented owner-approved DS treatment. The old plan alternated between dropping and importing it; neither text alone proves approval. Before changing its visible behavior, locate the decision/source contract and record the result in the existing owner. This unresolved policy detail does not block unrelated save/Training/actor work.

**Proof:** Toggle/apply/back/reboot; verify actual mono/stereo/flash behavior. Backup Clear tests all source targets, confirmation Yes/No/cancel and unaffected fields using disposable saves. Cadence, storage failures and repeated re-entry are engineering checks, not subjective visual approval.

## Package: title idle, tutorial and introductory scenes

**Outcome:** Source idle attract/demo, How to Play and opening cinematic are reachable, complete, native, skippable where specified and return to the correct scene with clean resources/audio.

Distinguish the introductory opening cinematic (deferred from P2-1 into this phase), campaign interstitials and campaign ending/credits (P2-6). “Deferred to P2-7” does not mean removed from P2; a current explicit owner pause still governs its active priority. Reuse imports/asset inventories; introduce no placeholder branding or source-scene compositor in a ROM.

**Proof:** Natural title idle, cancel/skip during each distinct scene type, full play for required content/timing, return/start game afterward, and repeated cycles with flat corresponding watermarks. Scripted demos retain source behavior/determinism; a seeded proof path must not replace normal user input in the published game.

## Package: DS platform and final game closure

**Outcome:** Correct native boot/banner, lid/sleep and supported reset behavior, safe file lifetime, reliable session return and verified final P2 artifact.

Test the actual platform hooks present; do not add unsupported low-battery or reset features from a wish list. For source/hardware differences, state the preserved user-visible behavior and approved adaptation. Exercise sleep/resume while audio and scenes are active and never leave half-published handles or incomplete save transactions exposed. Hardware-specific acceptance follows current project policy, not an invented requirement for repeated retail timing measurements.

The final mixed-mode session covers VS settings/matches/Results/rematch, campaign, bonuses, Training, Data/Options and save reload. Use targeted checks first and a broad session only for lifecycle/final closure, not after each tiny edit. Coverage identifies which original content and workloads were exercised.

## Exit checklist

- [ ] Actual save/reload/recovery works and diagnostics are isolated from the owner's save.
- [ ] All source unlock conditions, messages/challengers and defaults persist correctly.
- [ ] Training, Data children, Records, Sound Test and Options/Backup Clear are complete and cadence-clean.
- [ ] Attract/tutorial/opening and all required return/skip paths work natively.
- [ ] Applicable DS-specific reliability checks and whole-game resource/lifecycle proof pass.
- [ ] Every P2 unit and final stress/cadence contract is accepted; verified `smash64ds.nds` delivered with required owner review.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `src/nds/nds_backup.c`.
- `src/import/battleship_lbbackup.c`.
- `decomp/BattleShip-main/decomp/src/lb/lbbackup.c`.
- `decomp/BattleShip-main/decomp/src/lb/lbtypes.h`.
- `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pmanager.c`.
- `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pbonusstage.c`.
- `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1ptrainingmode.c`.
- `decomp/BattleShip-main/decomp/src/mn/mndata`.
- `decomp/BattleShip-main/decomp/src/mn/mnoption`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/P2-7-modes-meta.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/P2-7-modes-meta.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.

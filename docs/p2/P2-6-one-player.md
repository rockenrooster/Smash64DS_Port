# P2-6 — Natural Campaign Integration and Qualification

1P is active under the owner's September 10 unpause. Imported scenes, driver, variants, native exports, stage packets and host tests already exist in varying states. This plan turns those pieces into a complete playable campaign; it does not call them absent or automatically accepted.

## Authoritative seams and existing work

The source `sc1pmanager` owns the persistent run, sequencing, allies, continues and progression. `sc1pgame` owns source battle setup, waves and score events. `sc1pstageclear` owns the tally; `sc1pbonusstage` owns Bonus 1/2 integration. DS bridges replace platform entry, resources/rendering and hardware services, not campaign meaning.

Reuse `battleship_sc1pmanager.c`, `battleship_sc1pgame_runtime.c`, the imported menu/intro/tally/continue/bonus/tail scenes, variant/Boss exports and existing actual-C/host tests. The reviewed runtime bridge still contains explicit Boss/Polygon refusal paths; an export existing is not proof that the caller admits it. Resolve guards against the actual needed native/assets/behavior capabilities, never blindly remove every guard or infer readiness from the flag.

`NDS_P2_1P_GAME=0` in a published configuration is not a pause or a claim that code is missing. Flag-on candidate work must preserve the VS configuration and be qualified before publishing an enabled feature.

## Source route contract

Use `dSC1PGameStageDesc`, its enum and manager branches; table entries with placeholder venue IDs for bonus dispatch are not battle venues to instantiate.

| Sequence | Encounter | Source venue / content dependency |
|---|---|---|
| 1 | Link | Hyrule; first natural campaign fight |
| 2 | Yoshi Team (18 total) | YosterSmall; source replacements/colors/traits |
| 3 | Fox | Sector |
| 4 | Bonus 1 | Player-specific Break the Targets board |
| 5 | Mario Bros. | Castle; Mario/Luigi plus the source-selected ally |
| 6 | Pikachu | Yamabuki |
| 7 | Giant DK | Jungle; player and two source-selected allies |
| 8 | Bonus 2 | Player-specific Board the Platforms board |
| 9 | Kirby Team (8 total) | Pupupu; source two-at-once team and copy configuration |
| 10 | Samus | Zebes |
| 11 | Metal Mario | Metal / Meta Crystal |
| 12 | Bonus 3 | Race to the Finish / Bonus3 |
| 13 | Fighting Polygon Team (30 total) | Zako / Duel Zone |
| 14 | Master Hand | Last / Final Destination |

The manager separately handles challenger fights and ending/progression transitions. Derive exact enemy concurrency, stocks, handicap/CPU traits, timer/item rules and difficulty parameters from the table plus setup functions, not from total-opponent counts. Preserve per-entry all-item toggle semantics where the source sets them; no item-off campaign shortcut qualifies the original route.

## Package: entry and first ordinary fight

**Outcome:** Main menu→1P menu→1P CSS→intro→Mario versus Link/Hyrule through source campaign setup, with the selected difficulty/stock/player/costume preserved.

**Dependencies:** P2-1 bounded preview service, Link/Hyrule native content and a valid scene memory profile. Reuse compact previews. Campaign source selection remains authoritative; do not replace it with a VS descriptor, initialize battle state twice or replay startup only for a probe.

**Proof:** Natural menu input, setting changes and back/cancel, Intro animation/audio, countdown/GO and real fight behavior. Measure the actual startup/transient and active-scene floor. A very small free minimum cannot be labeled successful admission just because the first frame appears.

**Exit:** This natural prefix works and remains repeatable. **Stop:** Name the failed source→resource→native→output stage; preserve already-working prefix evidence.

## Package: common victory, tally and retry transitions

**Outcome:** Natural fight completion reaches tally and next-stage routing; defeat reaches source continue/Game Over logic.

Keep run totals/difficulty/source level-drop and remaining stocks persistent only where specified. Reset scene-local fighter/actor/intro/tally state on each entry. Cancel pending preview/asset/audio work before its owning arena is retired. Continue score arithmetic must match the source's truncation/order, not an approximate integer shortcut assumed equivalent.

**Proof:** Actual win and loss, Continue Yes/No/timeout, retry, repeat win, exit and re-enter. A direct-boot scene can prove its local display but cannot close transition wiring. Host edge tests cover arithmetic/branches; a natural route proves callbacks/events are actually reached.

## Package: ordinary and ally encounters

**Outcome:** All ordinary match descriptors and ally fights come from the manager's real source configuration.

Verify substituted opponents/costumes where the player changes the source selection, ally eligibility/randomness, team membership, damage/credit and CPU traits. Mario Bros. uses Castle, not Mushroom Kingdom. Giant DK's two allies exercise four instances with variant-specific scale/camera and memory. Link the ordinary stage contracts rather than duplicating their geometry work.

**Proof:** Source table/actual-C setup comparisons across allowed selections and difficulty boundaries, plus natural ordinary/ally fights and next-stage transition. Shared mechanics already qualified in P2-2 remain evidence; campaign setup/state persistence still needs its own engagement.

## Package: Yoshi Team

**Outcome:** The whole source team runs on YosterSmall with correct simultaneous enemies, variation cycle, spawn placement, replacement, HUD/remaining count and final completion.

Use the existing team spawn/respawn machinery and source total 18. Preserve color/trait/stock selection. Bind replacement instances to the proper native/pose/costume data; do not preload a roster-wide closure by accident. Prove repeated replacement and scene peak/floor across the full team, not one Yoshi death. Campaign small-stage assets/bounds are not the VS Yoster profile.

## Package: Kirby Team

**Outcome:** All eight opponents, source simultaneous count and copy loadouts work visibly and mechanically through the final transition.

Use source shuffled/final copy selection and hat/modelpart/article resources. `fighters/kirby.md` owns copy capability semantics; this package owns campaign population and resource lifetime across replacements. Prove copied projectile/effect creation, correct bodies/hats, cleanup and final tally. A plain Kirby mirror test does not cover the team.

## Package: Metal Mario and Giant DK

**Outcome:** Each variant retains source attributes/resistance/scale/material/audio and source battle/ally configuration.

`fighters/variants.md` owns exact variant deltas, with Meta Crystal/Jungle venue contracts. Avoid declaring an absent voice or cheap model from memory. Prove collision/hurtbox/knockback/camera meaning under scale and the natural win/next-stage path. Charge instance/capability bytes even if an immutable model is shared.

## Package: Polygon Team

**Outcome:** All source Polygon kinds and the full 30-opponent sequence work with correct traits, concurrency, replacement and final completion.

Reuse existing Polygon admissions/native exports and source motion dependencies. Derive each kind's allowed behavior rather than assume no specials or automatic cheapness. Prove that kind replacement does not leak resident data, reuse wrong material/pose ownership or grow live objects over waves. Test source final-KO/score logic and transitions; prevent duplicate completion during simultaneous deaths.

## Package: Master Hand and Final Destination

**Outcome:** The actual campaign reaches the boss, renders/executes its full source attack and child-weapon set, displays HP meaning, defeats it and reaches the source tail.

Reuse Boss imports/export and Last stage packet. `fighters/master-hand.md` owns attacks, projectiles, hit behavior and resource costs; `stages/final-destination.md` owns background/camera venue integration. Do not leave a refusal solely because an old comment says the export is still in flight. Conversely, an exported model does not prove the full attack/weapon closure.

**Proof:** Source-driven attack-family coverage, moving/off-screen behavior, damage/HP/defeat, native children/audio and natural end-of-fight transition. Cover difficulty-dependent source parameters. This workload needs measurement; four-CPU VS is not an automatic bound for boss geometry, background or attack bursts.

## Package: Bonus 1 and Bonus 2

**Outcome:** Mario's two boards first, then every one of 24 source board identities; both campaign and standalone-practice callers work.

`stages/bonus-stages.md` owns source collision/object placement, Target and platform rules, timer/failure/reset and per-fighter movement requirements. Reuse generated boards; do not author 24 courses from scratch. Distinguish campaign timing/scoring from practice records. Every board needs a complete clear and failure/restart proof, not just scene admission.

## Package: Race to the Finish

**Outcome:** The real Bonus3 course/hazards/timer/camera reaches source completion and tally.

`stages/race-to-the-finish.md` supplies the corrected contract: grounded Detect-material completion, animated GBumpers and TaruBomb spawns. No multiple-exit score-tier or forced-scroll assumption is carried from the old sketch. Source camera/timer/scoring callers decide those details. Test a natural full run, timeout/failure and another entry with reset counters/actors.

## Package: score, progression and ending

**Outcome:** Run events produce the correct tally, 58 source bonus predicates/values, continues, records, challenger selection and completed-character state, then the source ending/congratulations/credits flow.

Use the actual bonus enum/table and host test harness. Cover every predicate/data row with positive/negative or source-table evidence as appropriate; use representative natural engagements to prove event capture. Do not force 58 separate ROM rebuilds. Check score truncation/caps/order, continue handling, final fight and all related save triggers.

P2-7 owns persistence and unlock predicates; this package emits their correct events. Test exactly-once updates on replayed transitions/retry, loss/win challenger branches and the proper return menu. Ending/cinematic assets, audio, shooting/interaction and skip behavior follow source; original presentation is not “polish later.” Credits interactivity is verified from the source scene, not guessed from its label.

## Package: campaign qualification

First complete Mario start-to-credits naturally, with the required difficulty coverage from the goal/phase contract, then qualify the remaining eleven characters and all their bonus boards. Use source table tests to cover combinatorial data and natural runtime cases to cover distinct behavior/resource families. Do not claim a full matrix from one playthrough or force identical redundant full runs when unchanged proof already covers the same behavior; record exactly what each run covers.

Include all difficulties, relevant stock/continue boundaries, alternate player/costume choices, repeated entries, waves/variant/boss peaks, endings/challengers/save reload and source-derived audio/presentation. Every new screen must hold 30 Hz presentation and every workload meet its applicable final gate. The registry and existing scene-capable harnesses remain the test system; this plan adds no per-function proof-mode fleet.

## Exit checklist

- [ ] Natural menus/intro/fights/tally/retry and all route entries work with correct state ownership.
- [ ] Yoshi/Kirby/Polygon full teams, variants and Boss have complete behavior/native/output/lifetime proof.
- [ ] All bonus boards and Race complete and fail/reset correctly through their real callers.
- [ ] Score/bonuses/continue/progression/records and ending/credits are source-equivalent.
- [ ] All-character coverage, resource bounds, required cadence and owner review are satisfied.
- [ ] Feature-enabled shipping configuration is verified; flag-off safety or imports alone are not acceptance.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `src/import/battleship_sc1pmanager.c`.
- `src/import/battleship_sc1pgame_runtime.c`.
- `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c: dSC1PGameStageDesc and dSC1PGameComputerDesc`.
- `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pmanager.c`.
- `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pstageclear.c`.
- `decomp/BattleShip-main/decomp/src/sc/scdef.h: SC1PGame bonus enum`.
- `decomp/BattleShip-main/decomp/src/gr/grbonus/grbonus3.c`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/P2-6-one-player.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/P2-6-one-player.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.

# P2-2 — Capacity, Resident-Set Integration and Four-Fighter Acceptance

The four-slot source engine exists. The remaining challenge is complete, correct and affordable distinct-kind play—not another Mario/Fox mirror-only conversion. Separate resource feasibility, runtime correctness and performance verdicts.

## Existing capability to retain

Source battle creation/iteration, camera target sets, four-slot lower HUD, source team/scoring/Sudden Death/Results behavior, native owner-kind versus instance indexing, and the standing stress arm already have implementation. Retain source common-state fixes and their proofs, including shield-break, hit-status/colanim, grounded damage transfer and hitlag order. Do not restore P1 reduced pools or remove required effects to manufacture headroom.

Historical mirror tests exercise four instances but only two content kinds. They do not prove any-four-kind residency or full item/variant coverage. Local frame-64 startup/latch closure is useful but does not prove a whole match, full pack fit or teardown.

## Package: defensible capacity verdict

**Outcome:** A current-source, current-configuration byte ledger answers whether the complete scene and every supported kind/capability set fit.

**Inputs:** Existing fighter manifest, semantic pack estimator, native-image census, texture plan, current linked image and the shipping-shell capacity witness. `P2-2-pack-estimator.md` owns classification/equations. Read the current capacity evidence rather than copying an old fixed threshold.

Separate fixed image/arenas, shared scene costs, per-kind immutable data, per-instance mutable state, selected capabilities, native images, required audio buffers, binder/setup scratch and minimum reserve. Account for shared atom union and allocator alignment. Enumerate unique one-through-four-kind sets offline; price multiplicity-dependent mutable costs for four instances separately. Include costumes, required detail fallback, copied/variant powers and item/stage dependencies as explicit profiles. Do not multiply all possible content by all four players or omit mutually live content.

**Proof:** Existing estimator runs without unknown atoms/readers and emits per-class totals and the heaviest sets; a usable skeleton measurement identifies what is retained/removed and every unresolved deduction. The measured relaxed upper bound can prove RED even before an exact ceiling is known. It cannot prove GREEN.

**Stop:** An unknown input is STOP, not zero bytes. A proven deficit blocks full-fit claims. Independent native content may continue.

## Package: sufficient correctness-preserving recovery

**Outcome:** Measured, non-overlapping changes close the complete deficit, not merely a smaller startup latch.

Rank the actual live byte classes. Prefer removing redundant raw representation already replaced by native data, preserving semantic defaults, load-time conversion, sharing immutable atoms and tighter DS representations. Charge replacement tables, native images, validation/binder state and temporary overlap before crediting the saving. Keep a cumulative ledger with baseline and retained gain; eliminate double-counted savings.

Do not revive low-only residency: normal source paths can select High or use High common parts as a Low fallback. Do not revive a model-only pack as a complete post-setup ABI. Do not drop hurtbox defaults, late constructors, copy references or effects merely because initial setup completed. Fidelity changes follow measured need and owner approval; they are not assumed here.

**Proof:** Same configuration and life stage before/after, full asset/profile census, no newly unresolved consumer. A candidate that clears startup but remains short is a useful intermediate change, explicitly not residency acceptance.

## Package: integrate the typed resident working set

**Outcome:** The scene starts only with the required closed data set valid; gameplay and reconstruction never reach discarded raw storage.

Extend the existing data/loader/binder seams, not a new parallel engine. Use checked relative references or another proven compact representation; maintain aliasing/sharing semantics and explicit epoch ownership. Validate IDs, ranges, alignment and optional-invalid references before publishing the scene. Retain data consumed by respawn, damage resets, model-part changes, dynamically created weapons/items/effects and capture transitions.

Make load-time ownership explicit: staged input, converted resident output, temporary scratch, pointer publication, then retirement. Prove the *peak* while old and new data coexist. Missing required resources prevent entry with a named constraint; they do not demote rendering or disable a required fighter. No demand battle/motion/texture load after GO; streaming audio follows the separate admitted contract in `P2-texture-residency.md`.

**Proof:** Short natural scene entry, actual four bound poses/owners, diverse actions and late constructors, KO/respawn and Results/rematch. Use an existing validated access witness or scoped poisoning/check where useful; do not leave proof-only production wrappers. Repeated scene entry must not retain stale epoch pointers.

## Package: four-distinct-kind correctness

**Outcome:** The source rules remain equivalent for one human plus up to three CPUs, FFA and teams; automated four CPUs exercise the stress configuration.

Test six unordered fighter pairs at four players without changing source resolution order; credit/re-owner projectiles after reflection, grabs/capture, items and stale moves. Exercise disabled slots, repeated kinds, different costumes, friendly fire, KO/respawn, stock elimination, Time/Stock ties, partial-participant Sudden Death and four-way Results. The lower HUD shows four correct identities/damage/stocks/timer; top-screen tags/telegraphs remain native.

Camera cases include maximum horizontal/vertical separation, platform height splits, dead/up-fall and pause detail changes. Kind-unique data and per-instance pose/mutable state must not alias accidentally. Run the heaviest resource sets as well as the existing mirror control. Treat a resource, native coverage or ending/Results failure as correctness RED regardless of startup progress.

## Package: final scaling and performance

CPU optimization starts only when permitted by the current owner priority. Keep collecting cost evidence needed to diagnose a correctness/pacing regression, but do not use this plan to overturn a deferral.

When authorized, attribute representative P50/P95 and active-frame lanes; specialize/bake/cache only demonstrably repeated work, preserve gameplay and all required content, and retain repeatable gains under the current A/B policy. Eight matched frames may decide a local lever; final gate evidence remains the required whole workload and cadence population. Measure items-on configurations, varied fighters/stages/CPU behaviors and separate campaign churn/boss cases where applicable. Report tested domain and current measured worst cases honestly.

## Exit checklist

- [ ] Complete resident and transient costs plus reserve fit for all admitted resource profiles.
- [ ] Typed data remains valid through all late constructors, detail changes and scene lifetimes.
- [ ] Distinct-kind FFA/team/KO/Sudden Death/Results natural paths and lower HUD/camera pass.
- [ ] Required effects/items/audio are present; native coverage has positive output witnesses.
- [ ] Existing two-fighter regression and current four-CPU gate meet the goal on covered configurations.
- [ ] Full-fit, correctness and performance verdicts are separately banked; no historical number substitutes for current proof.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `docs/p2/P2-2-pack-estimator.md`.
- `docs/p2/P2-texture-residency.md`.
- `docs/reviews/Review_Deriving_Fighter_Live_After_Setup_Set.md`.
- `artifacts/performance/2026-09-10_pack-skeleton-ceiling/CEILING.md`.
- `src/import/battleship_scvsbattle.c`.
- `src/port/renderer_adapter_fighter.c`.
- `decomp/BattleShip-main/decomp/src/ft/ftmanager.c`.
- `decomp/BattleShip-main/decomp/src/ft/ftparam.c`.
- `decomp/BattleShip-main/decomp/src/gm/gmcamera.c`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/P2-2-four-fighters.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/P2-2-four-fighters.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.

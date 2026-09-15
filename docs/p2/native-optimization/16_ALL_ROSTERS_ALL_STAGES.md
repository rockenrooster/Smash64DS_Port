# Universal four-fighter / all-VS-stage implementation and qualification

**Revision 2, September 15, 2026.** This is a binding clarification of the requested support scope and an implementation specification. It is not evidence that any new configuration has been run. All added tasks are PLANNED under the existing P2-2p8 campaign.

## 1. Required support, not just a larger benchmark

The completed native runtime must support **every source-legal combination of four fighters on every selectable VS stage**, including repeated fighter kinds, every legal slot assignment, and the legal costumes, teams, controls, CPU levels, items, hazards and reachable detail/quality configurations belonging to the product. Every such configuration must preserve mechanics, required output, safe resource ownership and the existing stable-30-FPS contract.

Optimizing only Donkey/Samus/Link/Kirby on Dream Land is an intermediate checkpoint. Optimizing every fighter individually and every stage individually is also insufficient: simultaneous resource needs, mutable-state aliases, directed interaction order and stage/effect combinations can fail only in combination.

Fighter-specific and stage-specific native code remains encouraged. Universal support does not require a generic runtime interpreter. It requires every legal combination to have a correct, complete, efficient native execution path. Specialization must not turn into an allowlist of favored matchups.

Do not make a failing legal configuration disappear by disabling a character or stage, disallowing repeated fighters, changing a team/CPU setting, removing an item/hazard, reducing source-legal capacity, or silently downgrading quality. Safe admission refusal is containment; the legal match is still unsupported until the underlying problem is fixed. Fidelity/rate changes still follow the existing owner-approval rules.

The support claim is broader than any finite test history. Release evidence must honestly state which configurations, variants, seeds and scenarios were exercised or structurally proved. A finite test campaign cannot establish every possible input sequence. Nevertheless, **every known source-legal failure remains a blocker**, including one discovered outside the originally scheduled tests.

P2 single-console controls remain one human plus up to three CPUs; four CPUs are a stress configuration. This clarification does not move wireless multiplayer from P3 into P2, nor require source-illegal four-fighter combinations on campaign/boss-only stages. All other existing P2 scene/mode requirements remain in force. Sources: [S01, S06, S18, S35–S37] in [15_SOURCE_INDEX.md](15_SOURCE_INDEX.md).

## 2. Source-derived content and configuration catalogue

N00.06 creates a versioned host-side catalogue from actual source/rule and generated asset manifests. Do not infer support from the current test script's hardcoded roster or only from currently enabled menu rows.

Maintain two independent sets:

- **Product-required:** the complete VS roster/stage set required by the project, including required entries not yet implemented or enabled.
- **Implemented/qualified:** what the current candidate actually contains and what has valid evidence.

A required entry missing from the second set produces BLOCKED cases in the full-product report. A current-content checkpoint may omit them from execution, but must show the gap and may not use a smaller denominator to claim universal completion. All cases in a current checkpoint must still be source-legal.

Use stable canonical IDs and versioned alias mappings. UI order or labels must not define identity. For each fighter/stage include source identity, implementation status, native owner IDs, variant/dependency records, and the source/generator fingerprints. Document the exact extractor and current source table owners before implementing it; the planning package does not pretend to supply the production extractor.

A complete scenario key has at least:

| Dimension | Identity that must be preserved |
|---|---|
| Base content | Catalogue hash; four canonical fighter IDs as a multiset; canonical stage ID |
| Ordered lineup | Slot 0–3 fighter IDs, stable slot mapping and instance identities |
| Presentation | Source-legal costumes, team colors, reachable detail/quality/rate settings |
| Rules/control | Time/stock/rules and item profile, human/CPU control per slot, per-slot CPU level, teams/friendly-fire where legal |
| Workload | Declared source-normal or diagnostic class, deterministic seed/input/state-tour identity, stop/window procedure |
| Runtime identity | Commit/declared overlay, generated inputs, ROM/ELF/config, toolchain/SDK, emulator/config, save/storage, instrumentation and verifier hashes |

Separate case definition, workload definition and run identity. Changing a ROM does not change what a case means; it invalidates results for that case where the artifact dependency changed. Changing a case definition requires a new case-schema/workload version and explicit reconciliation.

## 3. Enumerate the full base space, including duplicates

For `F` required fighter kinds and `S` required VS stages:

- Unordered base cases with four fighters and repetitions: `C(F+3,4) × S`.
- Ordered four-slot cases: `F^4 × S`.

For the full planned twelve-fighter/nine-VS-stage set, these are **12,285 base cases** and **186,624 ordered cases**. These counts deliberately exclude costume/team/CPU/seed/state-history expansions; they must not be presented as the total number of all legal game states. Derive the counts again when the required catalogue changes.

Independent count cross-check:

| Multiplicity | Base rosters for 12 kinds | Slot orders per roster | Base cases over 9 stages | Ordered cases over 9 stages |
|---|---:|---:|---:|---:|
| AAAA | 12 | 1 | 108 | 108 |
| AAAB | 132 | 4 | 1,188 | 4,752 |
| AABB | 66 | 6 | 594 | 3,564 |
| AABC | 660 | 12 | 5,940 | 71,280 |
| ABCD | 495 | 24 | 4,455 | 106,920 |
| **Total** | **1,365** | — | **12,285** | **186,624** |

Host enumeration algorithm:

```text
fighters = sorted(unique canonical product-required fighter IDs)
stages   = sorted(unique canonical product-required VS stage IDs)

for roster_multiset in combinations_with_replacement(fighters, 4):
    for stage in stages:
        emit base_case(catalogue_version, roster_multiset, stage)
        for ordered_roster in unique_permutations(roster_multiset):
            emit ordered_case(base_case_id, ordered_roster)
```

Independently enumerate the ordered Cartesian product and compare exact sets. Equal totals are not sufficient: a missing case and a duplicated/substituted case can cancel in the count. Reject unknown IDs, duplicate canonical IDs, absent stages, truncated enumeration and unexplained aliases.

These are host test/admission catalogues, not a target dispatch table and not permission to bake 12,285 full match asset banks. Reuse immutable fighter/stage resources and small per-case admission metadata. Avoid a ROM-sized combinatorial expansion while checking the combinatorial support requirement.

## 4. Resource feasibility across every configuration

N02.07 evaluates all required base cases and all ordered-slot/layout obligations. Equivalent static resource computations may be reused, but every case still receives an explicitly traceable result. Prove the equivalence for the property being reused.

### 4.1 Immutable sharing versus mutable instances

Count each truly shared immutable bank once. Count writable per-instance state four times where four instances exist: animation/event cursors, topology generations, matrices, packet patch buffers, materials/visibility, hit histories, AI targets and pending service state. Four identical fighters are not one mutable fighter.

A single transient workspace may be reused only when its consumers are serialized and never retain references beyond the next overwrite. Packet DMA, GX consumption and texture raster lifetimes are distinct; a reused CPU scratch pointer must not escape into an outstanding transfer or later draw.

### 4.2 Legal future dependency closure

Derive required data from source-reachable states for the selected roster/stage and legal rule profile, not from the first animation or observed minute. Include copies, captures, held weapons/items, summon children, morphs, model-part replacements, KO/respawn, material frames, pause/detail switches and stage hazards.

For Kirby, copying can introduce an opponent's native hat/model/effect/weapon dependencies. Multiple Kirbys can hold different copies simultaneously. Track legal transitions and instance ownership. Do not load only the first possible copied bank and assume all other copies are unreachable; do not blindly union every resource in the entire game when a tighter source-backed closure suffices.

Use proved concurrent occupancy limits and mutual exclusion when useful. An expensive state not observed from level-3 CPUs is not an exclusion proof. Mid-fight copy or wave replacement is not an implicit loading boundary that waives locked-epoch policy.

### 4.3 Independent constraints and transient peaks

Check resident and transition-peak RAM, allocator reserve, stacks, ITCM/DTCM, legal VRAM bank modes, texture formats/placement, palette bases/capacity, atlas and view slots, packet buffers, matrix/geometry capacity, graphics scratch and declared service buffers. Account for old/new bank overlap at transitions. Distinguish capacity bounds from timing measurements.

The admission witness includes exact required/admitted/excluded resource identities, shared/per-instance storage, legal variant assumptions, constraint results, source/generator hashes and any failed identity. A total free-byte figure without legal placement and runtime allocation evidence is not a feasibility proof.

A failing legal case goes directly to its bank/representation owner. Improve native representation, remove obsolete state, share only immutable content, correct placement or prove a tighter legal bound. Do not return PASS merely because the loader correctly refuses entry. Runtime admission, actual allocation low-water and zero mandatory post-GO motion/texture demand still need target proof under N02.06/N10; static solving does not perform it.

## 5. Slot permutations are not automatically interchangeable

Every ordered lineup maps to one base multiset, but that is a counting relation, not a correctness or performance equivalence.

Slot order can influence instance-local storage, source update order, directed hit/grab priority, RNG consumption, target selection, team/port masks, HUD colors, draw order, transparency and cache/working-set order. A single canonical slot assignment does not justify discarding all others.

N03.11 documents and tests these properties separately:

| Reuse scope | Acceptable evidence | Does not establish |
|---|---|---|
| Static resource layout | Same exact resource/lifetime/constraint model after explicit slot mapping | Runtime timing or mechanical equality |
| Native rendering | Correct independently decoded/pictured output after legal mapping, including material/depth rules | Hit priority or AI/RNG order |
| Gameplay behavior | Source-derived directed outcomes and phase/order proof for the claimed mapping | Equal cache behavior or cadence |
| Performance | Explicit proof/bound covering the actual mapped work and timing behavior, or measured required runs | Other workloads/variants not covered by the proof |

Each certificate names the exact covered cases, property, assumptions, source/build/generated/emulator dependency fingerprints, validation and invalidation rules. A broad assertion such as `all_slots_equivalent=true` is invalid. A code reviewer declaring symmetry without a substantive proof does not close timing obligations. Conservative acceptable work bounds are not sums of independent P95 values and cannot ignore memory/bus/graphics/service effects.

**No proof means execute the remaining ordered obligation.** In the absence of accepted timing equivalence, release timing extends to all 186,624 ordered base cases at the twelve-by-nine catalogue. The minimum 12,285 full base runs are not a waiver for unknown permutations. This cost is a qualification concern, not a reason to narrow the supported product.

Exact duplicate permutations of identical per-slot descriptions are one configuration; distinct costumes, controls, team roles or mutable states may make apparently identical fighter IDs different full scenarios. Reconcile those expansions under the scenario key rather than inflating or collapsing counts incorrectly.

## 6. Efficient development and exhaustive declared release coverage

The universal endpoint must not turn every edit into an exhaustive test run. Three explicit profiles avoid that:

| Profile | Use | Result may establish |
|---|---|---|
| DEV_FAST | Focused source/host fixtures, small engaged A/B, affected owner/variant | Local implementation/correctness and a scoped candidate timing decision |
| SCREEN | All individual mechanisms, selected mixed/duplicate cases and evolving leaders | Prioritization, new failure discovery, affected-case selection |
| RELEASE_EXHAUSTIVE | All base cases, required ordered-slot obligations and declared variant/interaction tests at final identities | Completion of the declared finite configuration qualification matrix |

Pairwise testing is useful for additional axes and targeted diagnosis. It does not replace the base release matrix. Do not repeat a full release sweep after every local optimization. Run it at coherent hard-on qualification checkpoints, and after fixes invalidate/re-run the conservatively affected evidence set.

### 6.1 Full base-case run requirement

Every base roster-stage case receives a complete source-normal four-CPU scored battle with its actual slot assignment verified inside the guest. Enable items through the source-legal rule profile, retain hazards and normal gameplay timing, and preserve the source item/RNG law. Do not force a particular spawn schedule and call it natural coverage.

Before running, declare the exact rules/CPU levels/seed procedure and the accepted coverage endpoint. Baseline four-CPU level-3 mode remains a regression fixture, not evidence that level 3 is the most expensive level. A full match and state tours are complementary: a natural minute can fail to trigger critical moves or children.

A faster ROM may reach the end of the source minute before historical presented frame 1973. Extend the existing collector/driver to attest source clock, GO/Time-Up and complete populations rather than hardcoding the previous slow-ROM sample count. Verify cadence over the procedure's entire declared presented-frame population; independently qualifying intro, loading, results and transition windows is not permission to drop expensive gameplay frames. Preserve the existing percentile convention, exact gate, cadence denominator and rare-overrun allowance.

The existing stress script does not automatically accept arbitrary roster/stage parameters. N10.07 must inspect and implement a coherent pre-admission scenario seam, attest what the guest actually ran, and preserve shipping's natural input configuration. Do not invent working command-line options for an unimplemented driver.

### 6.2 Variants and adverse interactions

Require explicit obligations for legal costumes/team color layouts, teams/free-for-all/friendly-fire where applicable, each selectable CPU level, human-plus-CPU controls, and reachable quality/detail/rate states. Use source-backed equivalence to reduce identical resource classes, not to ban settings. Any performance-sensitive variant without a defensible bound/proof needs a measured case.

A finite variant/test policy is not a proof of every history. It is an explicit sampling/proof policy for axes beyond the exhaustive base roster-stage set. Publish uncovered or uncertain obligations; they cannot quietly become universal PASS. A discovered legal failure immediately adds a case and blocks completion regardless of the initial policy.

N10.09 adds source-legal four-way scenarios such as:

- Independent copy/morph/attachment states, simultaneous captures/throws, duplicate fighters on different motions and hitlag phases.
- Overlapping projectiles, translucent effects and summons with stage hazards, moving platforms, ledge recovery and camera/near-plane extremes.
- Legal cue/item/particle bursts and late KO/respawn/pause/rematch/resource-generation transitions.

Use controller/state tours already available where possible. Label manipulated diagnostic states and do not credit an impossible forced state as a natural shipping workload. Conversely, inability of a particular CPU level to trigger a legal human action does not remove that action from qualification.

## 7. Per-case gates and exact-set reconciliation

For each required run, retain independent verdicts for identity/population validity, source-normal coverage, four-way engagement, native output, resource/admission/locked-epoch policy, service/GPU health, work P95 and two-VBlank cadence. Lifecycle and required-content proof remain linked obligations.

Performance is evaluated on each declared run's complete valid population. Then use logical AND across required results. Never pool frames from unrelated rosters, stages, seeds or variants and compute one global P95 to hide a failing lineup. Do not use an average FPS or aggregate green capacity label as product proof.

The reconciler obtains expected cases from the pinned required catalogue plus qualification policy—not from existing result folders. It must emit exact sets as well as counts for:

```text
required obligations
measured valid PASS
measured FAIL
BLOCKED required content/resource/infrastructure dependency
NOT_RUN
STALE incompatible prior evidence
INFRASTRUCTURE_INVALID run/evidence
property-scoped certified reuse, listed separately
```

Scope examples: static admission may be complete while runtime remains NOT_RUN; current implemented content may pass while full product remains BLOCKED; a known legal expensive state means RED even when the canonical case passed. UNKNOWN is not PASS. Failures are not deleted by a retry, a changed seed, a new run folder or a catalogue shrink.

All required measurements and reuse certificates must be compatible with the exact release identity. A per-case result can pass only with appropriate evidence for every required property; a static certificate does not allow its nonexistent timing fields to be filled with another case's result. Out-of-scope results may be retained for research but cannot fill an expected release obligation.

The planning helpers in this package validate counts, dependencies and document consistency only. They neither validate real ROM artifacts nor implement this strict runtime-evidence reconciler; N10.08 owns the production checker and its tests.

## 8. Keep distinct measured leaders

Maintain separate leader sets for WORK-H P95, cadence/consecutive late presents, resident/transient RAM, VRAM/palette/view capacity, geometry/matrix/FIFO/overdraw, and audio/storage/IPC deadlines. Report exact stage/ordered roster/variant/workload and build identity for each.

A memory-heavy mixed roster can differ from a four-copy projectile/overdraw leader. A source-normal input trajectory on one stage can expose a deadline that another case never reaches. Do not assemble maxima from different runs into a fictitious measured worst case, and do not claim an exhaustive global temporal argmax from a finite test set.

Use SCREEN leaders for fast regression and task priority; use RELEASE_EXHAUSTIVE coverage for the release base obligation. Every legal failure found through a leader search, owner report, or targeted tour remains actionable even when not initially scheduled.

## 9. Resumption, evidence ownership and invalidation

Reuse the existing serial build and accurate-melonDS procedure. The matrix runner is a host orchestration layer, not a replacement engine scheduler or a new fleet of parallel timing emulators. Independent host set/resource calculations may parallelize. Runtime timing and exact visual acceptance remain isolated as VERIFYING.md requires.

Use an atomic per-run manifest and unique logs/storage; commit completion only after child exit, output validation, guest-case attestation and hashes. Interrupted output remains incomplete. Store failures and retry reasons; a new attempt is a new run, not an overwrite. Resume by expected case key and compatible identities.

On code/layout/toolchain/SDK changes, conservatively stale affected timing evidence. Shared renderer, scheduler, fixed math, TCM packing or linker-layout changes normally affect all cases. A narrow resource-only change may justify a smaller affected set only after verifying unchanged code/layout and complete generated dependency closure. Emulator timing/config changes invalidate timing across the matrix. Catalogue expansion adds cases; removal of an implemented row never deletes still-required product obligations.

Retain proven source/host fixtures whose dependencies did not change. Do not rerun every unrelated expensive test automatically, and do not preserve a result merely because its filename or config header matches. The release ledger must show how each piece of reused evidence remains valid.

## 10. Mandatory negative coverage fixtures

These are production checker/runner requirements, not claims they ran against a ROM during this document revision.

| ID | Deliberate defect | Expected outcome |
|---|---|---|
| COV01 | Omit a required fighter/stage because the menu disables it | Full product is BLOCKED; expected universe is not reduced |
| COV02 | Generate four distinct fighters only; omit repetitions | Base and multiplicity exact-set/count checks fail |
| COV03 | Drop one ordered lineup or map it to the wrong multiset | Ordered-to-base reconciliation fails |
| COV04 | Duplicate one case to replace a missing case at the same count | Exact-set check fails |
| COV05 | Required legal match safely refuses admission | Case resource FAIL; not success via containment |
| COV06 | Omit late copy/summon child or per-instance state in shared accounting | Resource completeness/ownership check fails |
| COV07 | Two same-kind fighters share one writable clock/packet/material block | Independent-instance fixture fails |
| COV08 | Claim all-slot timing reuse from an identical resource union | Certificate rejected for wrong proof scope |
| COV09 | Slot permutation changes directed hit/RNG/draw behavior | Relevant mechanics/rendering obligation fails; no blanket symmetry |
| COV10 | One slow required case amid many fast passing cases | Matrix performance stays RED without pooling |
| COV11 | Copy a good result to a different roster/stage/CPU/slot key | Guest/manifest identity validation fails |
| COV12 | Reuse rows from another ROM/layout/emulator/configuration | Evidence becomes STALE/INVALID unless exact reuse conditions are proved |
| COV13 | Interrupted, short, missing, malformed or invalid-window run | NOT_RUN/INFRASTRUCTURE_INVALID; no PASS from partial files |
| COV14 | Catalogue/schema/variant expands but old coverage denominator persists | Required-set digest/count mismatch; newly required cases open |
| COV15 | Aggregate RAM fits but palette/bank/atlas/transient allocation fails | Resource FAIL for that legal case |
| COV16 | Disable CPU work, required owner, items or hazard to speed the run | Coverage/output/source-normal verdict fails |
| COV17 | Drop a failing seed or overwrite failure with a favorable retry | Required-run reconciliation rejects cherry-picked evidence |
| COV18 | Discovery finds a legal failure outside sampled variants/leaders | Add regression obligation; universal closure remains RED |

## 11. Revision integration and stopping rules

This revision updates existing master/contracts/subsystem tasks, adds six bounded task cards, and keeps one P2-2p8 queue. It does not add a new milestone or replace current P2 content work. Register tests with the existing infrastructure rather than maintaining a second verifier stack.

Dependencies deliberately allow the catalogue, numeric interfaces, bank compiler and local renderer pilots to advance before an exhaustive runtime sweep. N10.07 is early host-runner infrastructure despite its release-package ID: N02.06 consumes its case-attestation/admission mode before final TCM packing. Building that driver does not require a final performance PASS. Static enumeration and resource solving are cheap early safeguards. Final universal qualification waits for complete native owners, the final fixed runtime and valid per-case evidence.

A small owner pilot can KEEP while universal support remains OPEN. A safe implementation can be IMPLEMENTED_NOT_ACCEPTED while resource/timing qualification is incomplete. Report that boundary honestly. Do not redefine required content or discard an unfavorable combination to turn a local win into a global claim.

## 12. Additional task cards

### N00.06 — Generate the legal content catalogue and base case universe

**Depends on:** N00.01

**Edit/inspect boundary:** `PROJECT_GOAL.md`; `source fighter/stage/selectability/rules tables (inspect their actual owners)`; `proposed host catalogue adapter and case generator`; `16_ALL_ROSTERS_ALL_STAGES.md`.

**Implementation sequence**

1. Read source and current generated manifests to produce stable fighter and stage IDs, aliases, legal control/rules variants, required-versus-implemented sets and source hashes. Preserve unavailable required content as BLOCKED obligations.
2. Generate all four-fighter multisets using combinations-with-replacement and all ordered rosters using the Cartesian product, both crossed with the required VS stage set. Use canonical IDs, not display labels or current menu order.
3. Emit base IDs, ordered IDs, duplicate multiplicity class, actual slot mapping and content-catalogue fingerprint. For twelve fighters and nine stages expect 12,285 base cases and 186,624 ordered cases; derive counts, do not bake these numbers into runtime.
4. Use separate full scenario keys for rules, controls/CPU levels, costumes/teams, detail/quality/rates, seed/input protocol and source/build/generated/emulator identities. Distinguish catalogue identity from run/result identity.
5. Add enumeration fixtures for zero/duplicate/unknown IDs, full versus enabled content, changed ordering/aliases, repeated fighters, missing stages and stability under catalogue input reordering. Independently count by multiplicity.
6. Keep the case catalogue host-side and deterministic. Do not add a target combinatorial dispatcher or generate one full asset bank per case. The bundled planning count helper is not a production source extractor.

**Required tests/evidence:** T-COVER COV01–COV04 and COV14; every ordered case maps to exactly one base multiset and the independent count identities match.

**Work or dependency retired:** Ambiguous combination scope and hidden omissions caused by generating tests only from accepted/enabled content.

**Done:** A versioned product-required case universe and current-content gap report exist before architecture capacities or release coverage can be declared universal.

**Stop/revert:** Do not guess absent catalogue identities or treat disabled required content as out of scope; report the exact source resolution gap.


### N02.07 — Solve exhaustive roster-stage resource feasibility

**Depends on:** N02.03, N00.06

**Edit/inspect boundary:** `existing native bank/resource manifest generators`; `docs/p2/P2-texture-residency.md`; `docs/p2/P2-1c-vram-map.md`; `proposed pure host admission enumeration`.

**Implementation sequence**

1. Evaluate every base roster-stage case and every required slot-dependent layout. Deduplicate equivalent STATIC resource computations only with an explicit identity/constraint proof; still emit one verdict for each required ordered case.
2. Build a source-reachable dependency closure for motion/model/detail/costume/copy/held-item/summon/stage/UI/audio needs. Share immutable assets once and charge writable state per active instance; legal simultaneous limits and exclusions require source evidence.
3. Check independent constraints: resident/transient RAM and allocator reserve, TCM/stacks, VRAM bank/mode mappings, palettes/atlas/view slots, packet/matrix/geometry limits and declared service buffers/deadlines. Scalar total-byte fit alone is insufficient.
4. Emit deterministic constraint witnesses, exact required/admitted/excluded sets, per-instance versus shared accounting, variant assumptions and first failed identity for every case. A performance-derived service estimate is provisional until measured.
5. Add negative cases with a missing late child, four-copy alias, legal costume overflow, one bad stage mapping, compressed-format placement conflict, equal-count wrong resource, temporary peak overflow, hidden post-GO demand and dropped required fighter.
6. Provide failed cases directly to the relevant representation/bank task. Support every legal case by improving representation or allocation; admission failure, removing a selectable lineup or unapproved quality loss is not closure.
7. Carry runtime admission/lock/lifetime obligations to N02.06 and N10. This host solver is not a new ROM measurement and may not turn static feasibility into target PASS.

**Required tests/evidence:** T-RES/T-BANK and T-COVER COV05, COV06, COV15; exact case-set reconciliation and allocation-constraint witnesses.

**Work or dependency retired:** Roster-specific capacity guesses and treating safe rejection as successful support.

**Done:** Every required resource case has a complete legal feasible allocation witness or an explicit blocking deficit; N02.06 remains open until all required deficits and runtime obligations close.

**Stop/revert:** Any unexplained missing resource case or legal allocation deficit blocks universal resource acceptance; do not hide it in a global mean.


### N03.11 — Prove duplicate-instance isolation and slot-sensitive behavior

**Depends on:** N03.02, N03.08, N00.06

**Edit/inspect boundary:** `native bound-instance and packet owners`; `native pose/topology identity contracts`; `source slot-ordered hit/grab/target behavior`; `proposed duplicate/permutation fixtures`.

**Implementation sequence**

1. Audit all mutable caches, packets, matrix palettes, visibility/material state, animation/event cursors and copy/attachment owners for kind-only keys or global last-owner assumptions. Document immutable sharing and per-instance storage.
2. Test AAAA, AAAB, AABB, AABC and ABCD layouts across all slots, using different motions, legal costumes, teams and update order. Check emitted command semantics and gameplay ownership, not only allocation counts.
3. Exercise simultaneous submissions plus status changes, copy/morph, death/respawn and same-address generation reuse; mutate only one instance and independently compare the other three against their expected state.
4. For each proposed slot equivalence, state separately what is proved for resource layout, native rendering, mechanical priority, RNG/CPU order and timing. Team/port masks, camera/HUD slot color, hit priority and draw order can invalidate a naive permutation.
5. Emit explicit covered ordered-case sets and dependency fingerprints only for proved properties. A structural certificate cannot be reused as timing proof; unproved performance assignments remain runnable obligations under N10.
6. Extend the runtime test seam without changing the natural shipping controller configuration, RNG law or legal roster. Do not retain a slow generic target renderer as the comparison oracle.

**Required tests/evidence:** T-BIND/T-POSE/T-PACKET/T-ORDER and T-COVER COV07, COV08, COV09; deliberate kind-key sharing and swapped-slot-result mutations fail.

**Work or dependency retired:** Shared mutable state keyed by fighter kind and unsupported slot-symmetry assumptions.

**Done:** Repeated fighters are independent, all required slot-sensitive mechanics have tests, and every claimed equivalence is property-scoped; remaining timing permutations are explicit obligations, not implied passes.

**Stop/revert:** A kind-shared clock/patch, wrong directed outcome or unproved timing equivalence blocks the corresponding case; preserve immutable sharing where it is valid.


### N10.07 — Implement a resumable exhaustive matrix runner

**Depends on:** N10.01, N00.04

**Edit/inspect boundary:** `scripts/lib/harness-registry.ps1`; `scripts/verify-p2-four-fighter-stress.ps1`; `existing coherent scenario/input seams`; `proposed host-only matrix orchestration`.

**Implementation sequence**

1. Consume the immutable required case catalogue and release profile. Reuse existing build/runner/ring capture infrastructure instead of adding another target scheduler or parallel build fleet.
2. Add a verified scenario configuration path: configure before admission/GO, then attest the ACTUAL ordered fighter IDs, stage, CPU/control/rules settings and generated/native owners inside the guest. Existing fixed-roster script arguments must not be assumed to accept a new case.
3. Support DEV_FAST, SCREEN and RELEASE_EXHAUSTIVE separately. Short probes can prioritize fixes; only valid full scored populations can populate release timing obligations. A fast-logic/forced-item fixture never substitutes for a natural run.
4. Measure by declared source-clock/GO-to-Time-Up policy with the established rare-overrun allowance, not a fixed present-count copied from the original slow ROM. Record intro/lifecycle windows separately; validate ring collection for a faster match that produces fewer presents.
5. Use unique immutable case/run IDs, files and disposable storage. Save atomic progress records only after child exit, hash verification, output validation and explicit per-case grading. Shard/resume without deleting failures or reusing stale evidence.
6. Serialize all timing/visual acceptance and shared builds as VERIFYING.md prescribes. Parallelize host enumeration/solver checks or isolated non-timing correctness only when allowed; never launch thousands of timing emulators concurrently.
7. Reattempt an infrastructure-invalid run only after recording and resolving its cause. Retain game failures; new attempts do not erase them. Missing/blocked/stale cases remain visible through to N10.08.

**Required tests/evidence:** T-COVER runner fixtures: interruption/resume, wrong guest roster, duplicate output, stale hash, failed child, incomplete ring, changed catalogue and exactly-once case reconciliation; T-MEAS valid full-clock population.

**Work or dependency retired:** Manual sampled roster selection and lost or silently replaced matrix results.

**Done:** The runner can enumerate, stage, attest, collect, resume and grade every required case reproducibly without altering shipping gameplay or conflating short screening with release proof.

**Stop/revert:** No runtime case-selector or trustworthy full-window collection means BLOCKED infrastructure, not passing coverage; do not manufacture case support from copied logs.


### N10.08 — Implement exact-set per-case release reconciliation

**Depends on:** N10.01, N00.04

**Edit/inspect boundary:** `proposed host coverage reconciler`; `existing product-performance row evaluator`; `templates/coverage-ledger.json`; `templates/scenario-result.json`.

**Implementation sequence**

1. Derive required base, ordered/permutation and variant/interaction obligation sets from catalogue plus qualification policy. Do not take the set of returned results as the expected universe.
2. Validate result identities, actual guest configuration, full-window completeness, relevant artifact fingerprints, positive engagement/native output, resources/services and per-run work/cadence verdicts.
3. Permit evidence reuse only for an explicit property-scoped certificate or unchanged validated identity. An admission equivalence cannot satisfy timing; no generic pass_all_slots or global average is accepted.
4. Report required, measured-valid-pass, measured-fail, blocked, not-run, stale, infrastructure-invalid and properly certified-equivalent obligation sets separately. Missing data is UNKNOWN; a single known legal failure blocks universal PASS.
5. Check every required run and configuration separately, then AND their verdicts. Never pool frames across lineups or let one fast duplicate compensate for a slow mixed roster; keep all mandatory seeds and diagnostic scope labels.
6. Implement negative fixtures COV01–COV18, especially one missing case, one bad case amid many good ones, same-count substitution, fake permutation proof, cross-ROM reuse and a safe rejected legal match.
7. Keep current-content progress separate from full-product completion and explicitly state finite input/history coverage. Reconcile new content or invalidated evidence before updating the one P2 board.

**Required tests/evidence:** T-COVER COV01–COV18 plus T-MEAS per-case gates; set identities and all independent verdicts must agree with immutable source/result manifests.

**Work or dependency retired:** False universal acceptance from pooled metrics, missing cases, guessed equivalence or a reduced enabled roster.

**Done:** A strict reconciler emits PASS only for a complete valid declared matrix with no known failing legal configurations; it otherwise reports exact unresolved obligations without claiming universal-history proof.

**Stop/revert:** Any fabricated, missing, stale, case-mismatched or unjustifiably reused result is invalid and cannot be rescued by an aggregate green summary.


### N10.09 — Qualify adverse four-way interactions and separate cost leaders

**Depends on:** N10.01, N09.05, N03.11

**Edit/inspect boundary:** `existing state-tour/input tools and source event oracles`; `stage/item/weapon/CPU/particle/audio owners`; `proposed interaction manifest and leader report`.

**Implementation sequence**

1. Define source-legal controlled tours for interactions not reliably exercised by one natural minute: four simultaneous projectiles/effects, multiple independent Kirby copies, captures/throws and held items, duplicate morph/status changes, and foreground/background depth intersections.
2. Cross stage mechanisms with fighter mechanisms: moving-platform attachment and ledge recovery, hazards near blast zones, item/summon bursts, camera zoom extremes, transparent overdraw and audio cue overlaps. Verify the setup itself follows legal capacity and source laws.
3. Cover legal CPU levels and control/team variants as separately declared obligations. Four-CPU mode is a stress tool; one-human/three-CPU source-input playback and difficult human-triggered states remain required where applicable.
4. Maintain independent leaders for WORK-H P95, cadence/consecutive misses, RAM/transient peaks, texture/palette/view capacity, matrix/geometry/FIFO/overdraw, and audio/storage/IPC deadlines. Never combine independently measured maxima into a claimed real worst-case match.
5. Use these cases to diagnose and re-price the integrated runtime, while the exhaustive base sweep remains mandatory. Any source-legal failure discovered outside the initial matrix becomes a regression case and a release blocker.
6. Publish which interactions/seeds/windows were executed and which were structurally bounded. Finite source-state tours do not prove all possible gameplay histories; keep that limitation explicit without narrowing the support requirement.

**Required tests/evidence:** T-COVER mechanism obligations and T-COLL/T-HIT/T-ORDER/T-GPU/T-AUDIO positive simultaneous engagement; distinguish natural performance from diagnostic manipulation.

**Work or dependency retired:** One blended worst-case benchmark and individual-feature tests standing in for four-way interaction pressure.

**Done:** All declared adverse mechanism cases pass their respective gates, separate measured leaders are reproducible, and no discovered legal interaction failure remains unresolved.

**Stop/revert:** Do not turn an illegal forced state into a shipping workload claim or hide a legal expensive state by banning its matchup, items, hazard, CPU level or quality setting.

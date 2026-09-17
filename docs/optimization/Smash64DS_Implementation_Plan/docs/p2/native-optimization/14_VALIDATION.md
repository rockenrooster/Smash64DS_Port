# Validation specification and final qualification

**Test IDs below are requirements to implement or map onto existing tests.** They are not claims that these tests were run while writing the plan. Existing entry points are identified where verified. New host checks/fixtures remain task deliverables.

## 1. Evidence layers

| Layer | What it proves | What it does not prove |
|---|---|---|
| Static source/type/link | Ownership, numeric reachability, target membership, section/resource bounds | Runtime engagement, mechanics, pixels or FPS |
| Independent host source oracle | Geometry/event/predicate semantics and known numeric domains | Target timing, cache/IPC coherency or complete admitted content |
| Focused target fixture | One actual runtime mechanism with positive witnesses | Whole-match P95, global roster completeness or lifecycle stability |
| Source-normal whole match | Real workload, cadence and same-run resource/engagement evidence | Untriggered moves/children, Time Up/Results/rematch or every content combination |
| Final hard-on natural-input build | Shipping configuration and integration | Unmeasured future content or changed SDK/emulator |

Use the least expensive layer capable of falsifying a change, then collect the widest relevant integrated evidence for a kept batch. Never replace an independent semantic oracle with the candidate's own generator output. [S03, S19]

## 2. Detailed fixture catalog

### T-MEAS — Measurement and product gate

- **M01 identity:** deliberately combine ROM A with ELF/config B; fail before collecting or interpreting samples. Save/debug storage and emulator policy changes must change identity.
- **M02 accounting:** verify WORK-H equals WORK−HUD per frame and reconciles the documented exclusive sum. SRC/GCRA/SINT/SCPU cannot be counted together as separate owners.
- **M03 rank:** use small hand-checkable fixtures and the real sample population to validate the current percentile convention. Do not hardcode the old rank-80 index into all runs.
- **M04 product:** fixtures that fail only P95, only cadence, both, or neither must yield four independent correctness/coverage/work/cadence fields. A correctness-only green must not become product green.
- **M05 population:** missing slow frames, malformed/duplicate payloads, ring wrap corruption, wrong frame ranges, repeated labels without justified collector semantics and zero samples must fail or be explicitly invalid, never disappear from percentiles.
- **M06 time units:** compare timer configuration, cpuGetTiming behavior and emulator cycle counters. Store the conversion and tested calibration; do not confuse two-VBlank timer ticks with raw ARM9 instruction cycles.
- **M07 apparatus:** visible HUD subtraction is per row; any remaining instrumentation/layout effect is independently described. Remove no task CPU work from the metric twice.
- **M08 controlled versus natural:** exact workload comparisons use the same source-controlled states/input/seed. Once allowed continuous numeric changes diverge long natural matches, use natural runs for outcome/coverage and controlled tours for causal timing. Report both scopes.
- **M09 tails:** store P50/P95/P99/max, cadence histogram, maximum interval and consecutive late-frame runs. A diagnostic attribution exclusion never becomes a gate-population exclusion.
- **M11 per-case gate:** one slow or resource-invalid legal case among many passing cases must keep the matrix RED. Grade mandatory seeds/runs independently; pooling their rows cannot rescue a failed case.
- **M12 window identity:** a faster ROM may finish a source-minute before historical frame 1973; use attested source-clock/GO/Time-Up policy and validate the complete ring population rather than pretending a fixed frame count still means one minute.
- **M10 negative engagement:** disable one CPU action source or omit a required owner in a fixture and prove coverage/product acceptance fails even with lower ticks.

Existing anchors: `scripts/census-tick-hud-p95-set.py`, `scripts/verify-p2-four-fighter-stress.ps1`, existing ring/capture scripts. New evaluator is N00.04. The current 1,972-sample stress clock 60→1 does not test match completion. [S03, S18]

### T-NUM — Native arithmetic

- **N01 limits:** min/max valid fields, maximum source scale/translation/speed, negative coordinates and values one unit inside/outside admitted ranges.
- **N02 rounding:** positive and negative halves, exact multiples, values adjacent to zero, floor versus truncation and chained accumulation.
- **N03 widths:** every multiply/accumulate/shift path, sum-of-squares bounds, intermediate cancellation and narrowing. Host sanitizers/checked reference must identify overflow and undefined shifts.
- **N04 divides:** positive/negative numerator and denominator, large valid widths, tiny nonzero denominator, denominator zero failure, quotient/remainder sign and constant-reciprocal equivalence.
- **N05 vectors/angles:** zero vector, near-zero direction, opposite/up-degenerate vectors, full-turn wrap, shortest signed angular delta and negative scale.
- **N06 drift:** source-controlled long integration with error envelopes, plus short exact-discrete boundary tests. Tolerance is per field/domain, never a blanket epsilon.
- **N07 independent oracle:** compare target output against independently computed high-precision/reference values, not only identical C compiled on two hosts.
- **N08 code generation:** ARM9 ABI, alignment, register clobbers for assembly, helper/veneer calls, unexpected doubles and pinned compiler flags.

### T-FLOAT — Whole-runtime no-float proof

Inject and detect: float hidden behind typedef; operator in a source-included TU; double promotion from a literal/varargs; libm through a function pointer; weak alias to a float routine; custom integer mantissa/exponent arithmetic; runtime `%f` formatting; a cold menu/transition helper; an ARM7 service callback; and an inline conversion that does not call libgcc.

Constant-only data or host-generator arithmetic is classified separately. A target build can have no FPU instructions and still perform extensive software float. Success requires the type/lowering/call/link analyses and root coverage to agree. Unknown callbacks and unresolved library roots remain blockers. A finite dynamic trace cannot establish global absence by itself. N04.06 defines the gate; N09.01 closes its temporary allowlist.

### T-CLOCK — Event timing

- **C01 speeds:** every source speed constructor and its admitted legal domain; known 1/3 and 16/3 counterexamples plus source landing/rebound cases, dyadic controls and large/small waits.
- **C02 segment edges:** exact boundary, one tick before/after, multiple events on a tick, zero wait, end/changed/null and looping.
- **C03 discontinuities:** attach at nonzero frame, seek, speed change before/on/after an event, status interrupt/re-entry and animation replacement on the same source tick.
- **C04 hitlag/pause:** state freezes and resumes at the specified source boundary; no catch-up double event or off-by-one landing.
- **C05 lanes:** different per-joint waits/loops, last-writer GObj publication and source script order.
- **C06 dynamic ratios:** rebound and other live parameter-derived speed changes; finite generated deadlines do not exempt dynamic cases.
- **C07 overflow:** long duration and tick/phase/generation wrap policy; corrupted event metadata is rejected at admission.
- **C08 behavioral outcomes:** attack/weapon spawn, landing/end/status and audio events occur at required ticks/order, not merely close pose coordinates.

Existing source tests: `scripts/fighters/test_pose_clock_differential.py`; the current generic pose oracle can inform cases but binary32 field equality is not automatically the final native representation contract. Keep the oracle independent and host-side where it interprets source formats. [S13]

### T-POSE / T-XFORM — Pose, matrices and required sockets

- Constant/linear/cubic channels at endpoints and interior extrema; quantization bounds and scale/rotation interpolation.
- Different clips sharing constant channels without sharing mutable cursor/state.
- Required-joint closure with ancestors, hidden but gameplay-active parts, held items, throws, copy hats, Samus morph, selected CSS poses and procedural attachments.
- Same root pointer after reparenting, same arena address with new lifetime, topology generation change and a hierarchy exceeding the declared bitset capacity.
- Camera-only, world-only, local-pose-only and material-only mutations update precisely their dependent outputs.
- Source transform classes including billboard/orientation replacement, translation retention, scale accumulation, negative scale/reflection, degenerate camera axes and perspective clipping.
- CPU gameplay socket and native visual transform agree within their approved numeric contract; no gameplay matrix depends on having rendered the fighter.
- Counts prove avoided local/world evaluations and matrix copies; copied bytes are not counted as eliminated just because the copy moved elsewhere.

### T-BIND / T-BANK — Bindings and native formats

- Header/ABI/schema/source hash mismatch; truncated header/table/payload; overlapping or misaligned spans; integer-overflowed count×stride; invalid pointer-relative offset.
- Wrong owner/model variant, copied foreign root, stale resource epoch, same-address allocator reuse and wrapped generation handling.
- Invalid texture view/palette/material dependency; equal-count but wrong identity; missing child or cold state.
- Four same-kind fighters with distinct legal costumes, materials, animation clocks and patch contents must not mutate shared immutable bank data. Cover AAAA, AAAB, AABB, AABC and ABCD slot layouts. A duplicate kind does not reduce the live mutable-instance count.
- N03.11 permutation fixtures must separate resource symmetry, draw equivalence, source-ordered hit/RNG behavior and timing; a proof of one cannot be used as another.
- Partial admission failure at each step must not publish a mixed-epoch scene; cancellation and scene return preserve correct retirement.
- Fast valid binding does not perform source file reads, original graphics command decoding or immutable graph rediscovery in the qualified hot draw.
- Repeated deterministic regeneration produces identical output/hashes; source identity and source-derived completeness are independently checked.

### T-GEOM / T-PACKET — Native commands and topology

Keep all six existing independent closure dimensions: source triangle completeness/order; vertex-cache-to-dense identity; matrix routing; authored facing; winding; primitive-strip/BEGIN expansion. Add packet command/parameter decode and dynamic patch proof. [S19]

Negative fixtures: drop the second G_TRI2 triangle; truncate the last triangle; alias a overwritten cache slot; reverse one strip; route a vertex to a wrong matrix; read the NDO6 alpha/flag byte as an unmasked class; remove unlit color; misalign a patch; write one word past its span; omit an initial material/matrix state; use a foreign bank under the wrong owner; and hide a required root while preserving total triangle count elsewhere. Each must fail an appropriate independent checker.

Runtime tests exercise split roots, palette/visibility changes, Link texgen, Kirby foreign roots, high/low details and previous-owner state contamination. Patch only declared dynamic words. Test a camera change during four-instance submission and an attempted write/free while DMA reads the packet. Completion and buffer-retirement witnesses belong to the actual transport owner.

### T-DEPTH / T-GPU — Native presentation and hardware limits

Test an impact ring partly in front of and partly behind a fighter; shield overlap/slice ordering; Castle roof and occluded geometry; stage translucent clouds and cutouts; acid/hazard layering; effects at near-plane limits; palette animations; and mixed translucent particles crossing characters. Source depth semantics, not aggregate nonblack pixels, define success.

Measure primitive/vertex/matrix use, FIFO waits, texture/palette/atlas placements and render completion for complete scenes. Do not assume 30 Hz increases per-buffer geometry capacity. A packet with fewer CPU instructions but excess commands, matrix-store collisions or raster stalls is rejected. An opaque substitute, missing background/platform or zero-alpha required effect is incomplete content, not reduced cost.

### T-PHYS / T-COLL / T-HIT — Gameplay mechanics

- Ground/air movement, acceleration/friction/terminal speed, jump trajectories, gravity, land/ledge timing, knockback, hitstun/hitlag and source integer timers.
- Slopes, platform corners/edges, pass-through from both directions, moving platforms, ledge approaches, high-speed swept crossings, degenerate/parallel lines, negative coordinates and blast-zone bounds.
- Conservative broad-phase supersets versus source ordered narrow results; outward rounding never drops a source-valid collision.
- Four-way simultaneous attacks, directed grabs, shields/reflects, teams/self exclusions, invulnerability, stale moves, throw/release/item sockets and weapon ownership.
- A stateful hit or status change invalidates phase-dependent shared facts before the next reader; six unordered broad pairs do not erase directed outcomes.
- Corrupt or out-of-domain numeric inputs trigger established safe failure, not silent wrapping/saturation into plausible gameplay.

Reuse existing floor/topology/hitstatus fixtures and extend from actual BattleShip behavior. Do not require random long-match bitwise identity after explicitly allowed continuous numeric differences; do require exact known discrete boundary outcomes.

### T-AI / T-ORDER — CPU and scheduler

- Every admitted CPU level, target switch, recovery, movement, defense, attack and item decision with source controller/RNG timing.
- Four CPUs are positively engaged: movement/actions/targets/attacks, not merely four live objects. Some states require human/source-controller tours rather than an ordinary CPU trigger.
- Same-tick create/delete/status/process changes, callback replacement, mixed actor types and deterministic iteration order.
- Stable handles reject old-generation reuse; a dense-list optimization cannot reorder hit/RNG outcomes through swap-delete.
- Shared query facts are keyed to the source-observation phase; test a writer between two readers that would invalidate a whole-tick snapshot.
- If optional scheduler flattening is not implemented, record that decision rather than weakening the order tests.

### T-PARTICLE / T-STAGE / T-UI

Particle spawn/emitter/RNG/lifetime and pool pressure remain source-equivalent; hidden/transparent/offscreen does not automatically mean inactive. Mixed atlas/material/alpha/depth effects test both simulation and drawing. Static stages retain moving hazards/collision/backgrounds and source animation inputs. UI tests cover four-slot HUD values, quick CSS selection/selected poses, previews, stage select, options/backup clear/data/results and every shipped transition. Prove 30 Hz cadence and native fixed arithmetic outside battle. A stale screenshot or retired software text buffer fails.

### T-RES / T-LIFE — Memory, storage and lifetime

- Exact required/admitted/excluded sets and all format/bank/palette/view/atlas constraints over every N02.07 roster-stage/layout case. A missing case, unfinished required owner or safely refused legal match is not PASS.
- Minimum free heap plus maximum transient/graphics/packet/thread/boot stack use; code-region limits must reflect original DS, not a broad DSi-capable address range.
- Repeated CSS→battle→results→CSS, rematch, sudden death, pause, KO/respawn, copy/morph, entry, campaign/other shipped modes and allocation-address reuse.
- Inject failure during allocation, relocation, upload, bind, publication and cancellation. No leaked temporary bank or partially valid scene.
- Zero mandatory motion/texture post-GO reads for locked profiles. Direct NitroROM reads count. BGM is separately declared; required one-shot service has its own proof.
- Long-soak late states and cue bursts, plus independent memory-hardest and CPU-hardest rosters.
- Reclaimed memory is available at the actual allocation boundary; an ELF BSS reduction alone is insufficient.

### T-AUDIO / T-HWMATH / T-IPC

Source cue identity/timing, finite-track ending, loop seams, pitch/pan/volume bounds and simultaneous required cues. Inject late, short, failed reads; preserve playback cursor and do not double-play a fallback. Worker tests include full queue, incomplete publication, late completion, cancellation race, stale scene generation, same-address reuse and attempted buffer reclamation with an outstanding writer. Math tests include every current shared-unit writer and interruption/overlap. Confirm no audio/input/service starvation and whole-frame net gain for retained offload.

An unselected ARM7 experiment is not a missing required feature. A selected one must satisfy all relevant tests. No TCM pointers cross into DMA or ARM7 requests; volatile is not cache coherency.

### T-TCM / T-BUILD / T-RETIRE / T-DOC

Reconcile every section byte, alias/literal/veneer/alignment and startup LMA assumption. Test IRQ/boot/context/interworking and stack limits. Build all currently shipped configurations after source membership/schema changes; generator-staleness and untracked-derived-input checks remain enforced. Confirm retired functions/data/bridges are unreachable/absent and no build flag revives a forbidden graphics route. Final source/config/ROM/ELF/evidence links must agree. Plan link/dependency validation is useful, but it is not a substitute for any game test.

## 3. T-COVER — Universal configuration coverage

**Universal support:** every source-legal four-fighter lineup on every selectable VS stage, repeated fighter kinds and legal slot assignments included. Costumes, teams, CPU/control settings, reachable detail variants, items, hazards and copy/child states remain supported. Required but unfinished/disabled content remains BLOCKED in full-product coverage. The current stress case is a regression seed, not the boundary of implementation.

**Development:** DEV_FAST and SCREEN may use individual, pairwise and targeted four-way fixtures plus measured leaders. Their scope is explicit and they never substitute for the release base matrix.

**Release:** enumerate all source-derived base roster-stage cases and ordered cases. For the full twelve-fighter/nine-stage VS catalogue this is 12,285 unordered cases with repetitions and 186,624 ordered cases. Each base case receives a full scored source-normal four-CPU battle with its actual slot assignment attested. Ordered-slot performance obligations require measured evidence or a property-specific proof of timing equivalence; absence of proof leaves the case runnable/open. Static resource equivalence alone never discharges timing or mechanics.

Variant/control/CPU/quality and adverse interaction obligations are separately declared and reconciled. Their count is not included in the two base numbers above. Finite tests do not prove every input sequence, but every discovered failing legal configuration blocks universal completion. Unimplemented required native owners stay open in existing P2 content packages.

Per-case product work/cadence, output/engagement, resource, service and validity gates are independent. No pooled percentile across lineups; no missing-case PASS; no known legal admission failure counted as successful containment; no shrinking the source-required set to current menu options. Use independent CPU/cadence/RAM/VRAM/GPU/audio leaders and invalidate affected evidence on relevant changes.

The complete algorithm, identities, proof scopes, negative fixtures **COV01–COV18** and six additional task cards are in [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md). N10.01 freezes obligations; N00.06 enumerates; N02.07 checks resource feasibility; N03.11 checks duplicate/slot behavior; N10.07 runs; N10.08 reconciles; N10.09 targets adverse interactions.

## 4. Existing commands versus proposed commands

The following existing command shape is verified from the pinned script parameter block and procedure; substitute a genuinely idle runner slot and unique output directory. This is execution guidance, **not a report that it was run here**:

```powershell
# Repository root, PowerShell 7; no parallel builds or concurrent timing.
$case = 'artifacts/performance/native-opt/baseline'
New-Item -ItemType Directory -Force $case | Out-Null
.\scripts\verify-p2-four-fighter-stress.ps1 `
  -RunnerSlot 2 `
  -JsonOut "$case/timing.json" `
  -RowsCsv "$case/rows.csv" `
  -CoverageJsonOut "$case/coverage.json" `
  -MemoryJsonOut "$case/memory.json"
```

Require the script to complete successfully and inspect its terminating errors and final verdicts. Preserve full OS-level logs for umbrella execution as `VERIFYING.md` requires. Runner slot 2 is an example, not a reservation. Omit `-NoBuild` unless ROM/ELF/config freshness is proved.

Other existing entry points:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
python .\scripts\fighters\check_native_owner_geometry_closure.py
.\scripts\check-generator-staleness.ps1
.\scripts\check-melonds-policy.ps1
```

New numeric/product/bank/packet fixtures described here must be implemented under their tasks before adding invocation examples to the operational runbook. Never invent a passing command for an uncreated checker.

## 5. Final release tasks

### N10.01 — Define universal roster-stage and interaction obligations

**Depends on:** N00.05, N02.01, N00.06

**Edit/inspect boundary:** `source-derived fighter/stage/item/scene manifest`; `scripts/lib/harness-registry.ps1`; `proposed qualification scenario manifest`.

**Implementation sequence**

1. Use N00.06 to derive the full product-required fighter/stage catalogue, every unordered four-fighter multiset and every ordered four-slot roster; never silently reduce it to enabled or presently accepted content.
2. Specify one reproducible source-normal, items-enabled four-CPU full scored battle for EVERY base roster-stage case. Freeze slot assignment, source-legal rules, CPU profile, seed/input procedure, start/stop policy and completeness witnesses before results are known.
3. Track runtime slot-permutation, costume/team, CPU-level, control-mode and quality/detail obligations separately from base roster-stage coverage. Resource equivalence, mechanical symmetry and timing equivalence are different proof scopes; an unproved permutation remains a runtime obligation.
4. Design directed and four-way state tours for all required mechanisms: copy/morph/capture, duplicate writable state, overlapping projectiles and VFX, moving hazards, item/summon children, camera extremes, audio bursts and late lifecycle states. Source-legal natural tests grade performance; manipulated diagnostic states are separately labelled.
5. Define DEV_FAST for local tests, SCREEN for prioritization and RELEASE_EXHAUSTIVE for final base-case and assigned permutation coverage. Pairwise/targeted testing supplements, never replaces, the release base matrix.
6. Pin dependency fingerprints and conservative invalidation rules. Produce separate current-content and full-product reports, with missing planned content shown as BLOCKED rather than absent.

**Required tests/evidence:** T-COVER COV01–COV18; exact required sets and counts reconcile; every base case and assigned permutation/variant obligation has a finite declared procedure, not just a fighter/stage mention.

**Work or dependency retired:** Sampled interaction coverage being presented as universal roster-stage qualification.

**Done:** The source-derived required sets, exact case universe, full-match procedures, supplemental interactions, slot-proof policy and requalification triggers are frozen and reproducible.

**Stop/revert:** A representative sample is useful during development but cannot satisfy RELEASE_EXHAUSTIVE or shrink the full product scope.

### N10.02 — Run controlled and natural four-way performance qualification

**Depends on:** N10.01, N09.05, N00.04, N10.07, N10.08, N10.09

**Edit/inspect boundary:** `scripts/verify-p2-four-fighter-stress.ps1`; `product-performance evaluator`; `final shipping and matching profiling builds`.

**Implementation sequence**

1. On final hard-on builds, run the RELEASE_EXHAUSTIVE base matrix: a complete scored source-normal battle window for every unordered four-fighter roster on every selectable VS stage, with its actual slot assignment and current-content identity verified inside the guest.
2. Discharge every ordered-slot runtime obligation under the explicit proof policy. A functional symmetry certificate does not discharge timing. Run unproved assignments and variants with their declared full-match or focused protocol; there is no default 24-way performance symmetry assumption.
3. For each required run, grade work and cadence separately using its entire declared population and collect same-run native output, actual four-way engagement, resource limits, audio deadlines, GPU completion and clock coverage. Use a source clock/time endpoint; do not assume the historical frame 1973 endpoint survives the speedup.
4. Execute N10.09 controlled and natural adverse-interaction cases and maintain separate measured CPU, cadence, RAM, VRAM/palette, GPU/overdraw and service leader sets. Fixed level-3 stress remains a regression fixture, not evidence that every legal CPU/control profile is covered.
5. Reconcile expected and observed case IDs through N10.08. Persist and report every failed, blocked, stale, infrastructure-invalid or unexecuted obligation. A known legal failure blocks universal qualification even when not one of the originally chosen leaders.
6. For RED results, retain the passing work already validated at unchanged identities, report the exact failing case and cause, fix it, and rerun the conservatively affected set. Never drop failed seeds, shorten costly runs or choose only successful retries.

**Required tests/evidence:** T-MEAS per-case product gates and T-COVER exact-set release reconciliation; zero unexplained missing/stale/blocked cases; explicit honest scope for finite dynamic histories.

**Work or dependency retired:** No runtime work by itself; this proves or rejects the integrated performance endpoint.

**Done:** Every required base case and assigned ordered/variant/interaction obligation has valid passing evidence, with independent per-case work/cadence gates and no known failing legal configuration. This qualifies the declared finite matrix, not all possible input histories.

**Stop/revert:** One legal failing case, unresolved required permutation, unavailable required fighter/stage or missing result keeps universal acceptance OPEN; pooled percentiles cannot override it.

### N10.03 — Qualify lifecycle, memory and service stability

**Depends on:** N09.05, N02.06, N08.07

**Edit/inspect boundary:** `shell/lifecycle verification scripts`; `final bank/service owners`; `source-controlled transition tours`.

**Implementation sequence**

1. Test CSS→battle→results→CSS, rematch, sudden death, pause, KO/respawn, copy/morph, intro and shipped mode transitions.
2. Collect allocation/stack/TCM/graphics/packet/IPC and audio service evidence; test interrupted admission/cancellation and repeated arena address reuse.
3. Use long natural soaks for leak and rare deadline behavior only after focused tests pass; keep timing measurements isolated from concurrent correctness runners.
4. Confirm no mandatory locked-epoch motion/texture reads and no undeclared service clients appear late in a match.
5. Exercise transitions that change roster/stage sets, including four same-kind to four distinct kinds and back, legal costumes/team layouts, and reused instance addresses. Link resource/lifecycle results to configuration keys; a successful allocation for one case cannot stand in for every allocation shape.

**Required tests/evidence:** T-LIFE/T-RES/T-AUDIO/T-IPC applicable tests, full coverage beyond the stress script’s 60→1 clock window.

**Work or dependency retired:** No runtime work by itself; validates long-lived ownership and resource closure.

**Done:** No leaks/stale handles/late writers/underruns or unqualified scene gaps remain in the claimed shipped scope.

**Stop/revert:** The 1,972-frame battle script does not cover results/rematch; missing lifecycle coverage blocks this task.

### N10.04 — Qualify visual, mechanical and numeric completeness

**Depends on:** N09.05, N10.01, N10.08, N03.11

**Edit/inspect boundary:** `independent geometry/source oracles`; `semantic fixture suite`; `proposed whole-runtime no-float checker`.

**Implementation sequence**

1. Run exact discrete gameplay/event/order tests, bounded continuous error reports and native geometry/material/transform tests across the qualification matrix.
2. Capture tricky visual states with expected-owner/resource identities and actual pixels; preserve all required content regardless of aggregate error counters.
3. Run the no-float audit on every shipped scene/service root, including menus and cold callbacks, with no temporary migration allowance.
4. Record unresolved pre-existing content defects separately; they cannot be ignored when claiming full-game completeness.
5. Reconcile required content sets with N10.08, including disabled unfinished fighters/stages and required child owners. Use zero omissions and explicit variant/permutation evidence; report sampled dynamic histories honestly rather than claiming exhaustive behavior proof.

**Required tests/evidence:** T-NUM/T-CLOCK/T-PHYS/T-COLL/T-HIT/T-AI/T-ORDER/T-GEOM/T-DEPTH/T-UI/T-FLOAT.

**Work or dependency retired:** No runtime work by itself; proves representation and fidelity closure.

**Done:** Claimed shipped scope has complete native mechanics/presentation and fixed runtime without disguised IEEE helpers.

**Stop/revert:** A matching sampled state hash alone or zero no-op error counters are insufficient.

### N10.05 — Reproduce the release from declared inputs

**Depends on:** N10.02, N10.03, N10.04, N09.06, N10.08

**Edit/inspect boundary:** `Makefile`; `build.ps1`; `scripts/verify-all.ps1`; `declared source/generated asset prerequisites`.

**Implementation sequence**

1. Rebuild the natural-input P2 target from recorded source and declared ignored derived inputs without relying on unrelated dirty files.
2. Run generator-staleness, native-only, runtime numerics, ABI/architecture and the widest relevant integration profile; inspect all final verdicts.
3. Pin final ROM/ELF/config/generator/emulator/verifier identities and attach timing, content, memory and service coverage.
4. Confirm the published target is smash64ds.nds, not a fast-logic/tick-hud/forced-item lab target or the frozen P1 artifact.
5. Publish the coverage-ledger digest, catalogue digest and per-case qualification identities with the release. Reuse existing final-binary runs only if the rebuilt ROM/ELF/config and relevant inputs are identical; otherwise mark their obligations stale and rerun the affected set before release.

**Required tests/evidence:** T-BUILD clean reproducibility and exact final identity; no broken umbrella bypassed as GREEN.

**Work or dependency retired:** Accidental undeclared dependencies and false publication acceptance.

**Done:** A reproducible release artifact is covered by all required final gates.

**Stop/revert:** Compile success or a prior green hash does not qualify a changed ROM.

### N10.06 — Close or report the precise remaining gap

**Depends on:** N10.05

**Edit/inspect boundary:** `docs/P2_EXECUTION_BOARD.md`; `docs/HANDOFF.md`; `docs/PERF_LEDGER.md`; `final campaign evidence`.

**Implementation sequence**

1. Publish measured before/after performance with the exact baseline comparison scope, per-case gate outcomes, coverage counts, independent resource/cadence/service leaders and all open failures. Do not quote only the best lineup or pool unrelated case rows.
2. Close universal four-fighter performance only after the full required base/ordered-obligation ledger is reconciled and all legal known failures are resolved. A scoped current-content checkpoint may be KEEP but is not full-product closure.
3. Summarize retired work categories and the final architecture, not lines rewritten or hypothetical summed savings. Preserve unrelated P2 content rows and the one existing execution board.
4. On relevant content/code/toolchain/SDK/emulator changes, invalidate affected evidence, rescreen prior leaders and new cases, and reopen the existing P2-2p8 row with exact failing configuration and cause. Broad linked-layout changes conservatively invalidate timing across all cases.

**Required tests/evidence:** T-DOC and T-COVER final exact-set report agrees with case artifacts; no unsupported all-combination, universal-history or aggregate-speedup claims.

**Work or dependency retired:** Ambiguous completion claims and duplicated status tracking.

**Done:** The board clearly separates local implementation, current-content coverage, exhaustive declared roster-stage qualification, fixed-runtime closure and any remaining full-game work.

**Stop/revert:** No measured pass means no closure; preserve the recoverable checkpoint and next concrete owner instead.


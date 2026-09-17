# Smash64DS — SRC Optimization Re-evaluation

**Date:** September 17, 2026.  
**Repository reviewed:** `rockenrooster/Smash64DS_Port`, `master` at `db0d088bc61ac3e85f07a349857a1b3ec7eef55b`.  
**Previous report:** `Smash64DS_SRC_Optimization_Complete.md`, source snapshot `430aca2879e9071dc2b22f944f5c2909c9ce7aa4`.

**Scope:** Re-evaluate the SRC research, not the workflow documents. Preserve 60 Hz gameplay, the admitted pose schedule, source-equivalent mechanics, fixed-point runtime as the endpoint, native-only graphics, and every legal four-fighter lineup on every selectable VS stage. This is a replacement assessment of the candidates, not a claim that an optimization has shipped.

**What was done:** Read the original consolidated report and prototype package; compare repository revisions; inspect newer experiment reports and current source; rerun the original host experiments; develop and test an additional exact cubic formulation and a horizontal-floor special case; inspect generated ARM946E-S code. The branch API returned only `master`, and the comparison reports 85 commits beyond the original snapshot. [S01](#s01)

**What was not done:** No game source or project operating document was edited, no ROM was built, no emulator or retail DS benchmark was run, no full extracted asset corpus was tested, and nothing was committed or pushed. All new code, tests, results, and assembly needed to reproduce the standalone experiments are embedded below. No companion file is required.

---

## Executive verdict

**The previous report correctly identified expensive representations, but it ranked a broad pose/transform rewrite too confidently.** The newer evidence supports a more selective attack: remove repeated static map-query preparation, compile compact pose/motion execution, replace overlapping subtree caches with one topology representation, and use exact arithmetic simplifications inside those replacements. Do not start by building a second full pose/collision framework.

The important corrections are:

1. **The approximately 74.6K-tick pose figure is not conversion overhead.** A newer source/PC audit prices the examined fixed-to-float publication at approximately 2,814 ticks per profiling region. The remainder is real parsing, evaluation, traversal, and other work. Its removal requires an actual replacement algorithm. [S03](#s03)
2. **The flat-cache conflict is now measured, not hypothetical.** The four-slot cache misses 49.7% of the time in the reported run. Enlarging it reduces SRC but worsens whole-frame work. Also, the keys are invalidated subroots, not merely the four fighter roots. [S04](#s04)
3. **The joint-cap experiment is not a valid pose-speed ceiling.** Its own report says the two arms ran different matches. Current source also shows that its `continue` skips parsing and clock advancement as well as evaluation for the capped entries. A changed workload cannot price an equivalent pose implementation. [S07](#s07), [S10](#s10)
4. **The later “SRC distribution” is actually a whole-frame non-idle profile.** Its approximately 1.614M ticks cannot be treated as an exclusive SRC decomposition. Some policy percentages in that report also reuse the sampled caller census that another report explicitly corrected. [S03](#s03), [S06](#s06), [S09](#s09)
5. **The host prototypes are useful, but not DS speed evidence.** The original results reproduce. A new exact cubic reduction also passes, yet its standalone ARM code grows slightly. Fewer multiplies are a candidate mechanism, not an automatic win.

**Recommended first substantial implementation:** a bound native map-query pilot that retires current layered lookup/preparation work, preserving query semantics and output timing. In parallel research, size the complete compact motion working set and compile a bounded pose program that replaces—not supplements—the existing interpreter state. The larger shared fixed pose/gameplay representation remains an endpoint, but it should be assembled through measured, deleting replacements rather than introduced as an all-at-once prerequisite.

No candidate here is established as sufficient by itself to close the full 30-FPS deficit. That does not disqualify useful contributions, and a depleted list of small experiments does not establish an architectural lower bound.

## Evidence labels used in this report

- **SOURCE:** visible in the inspected source at a named revision; not automatically proved engaged by a particular ROM.
- **RECORDED:** a repository report or uploaded log records a measurement. Its stated configuration and limitations stay attached; raw data was not independently rerun here.
- **HOST-TESTED:** executed in this response as a standalone experiment, not in the game.
- **DERIVED:** arithmetic or reasoning from explicitly stated inputs.
- **PROPOSED:** a replacement that still needs implementation, source-corpus coverage, and DS measurements.

---

## 1. Updated baseline and what it does—and does not—price

### 1.1 Keep historical, qualified, and experimental results separate

| Evidence population | WORK-H P50 | WORK-H P95 | Status relevant to this review |
|---|---:|---:|---|
| Original report's September 16 clean baseline | 1,575,296 | 2,311,616 | Historical; not the current source/ROM. |
| Current board's “last qualified checkpoint,” `a4eb24c9a85` | 1,600,960 | 2,320,576 | Board reports Boundary green and P2 performance red. |
| DTCM hot-scalar experiment, its own control | 1,580,544 | 2,320,768 | Separate workload/configuration identity; not interchangeable with the board row. |
| Same DTCM experiment, candidate | 1,537,344 | 2,277,696 | Reported useful result; report retains qualification/window caveats and a later alignment correction. |

Sources: historical baseline [S02](#s02), current board [S08](#s08), DTCM report [S05](#s05). “Last qualified checkpoint” does not mean the current HEAD has been fully qualified. The board itself has older summary text alongside this newer checkpoint; the row and its scope above are quoted as recorded, not reconciled into an invented new benchmark.

Using the previously documented exact **1,120,380-tick** gate:

- Current board checkpoint P50 deficit: `1,600,960 - 1,120,380 = 480,580` ticks.
- Current board checkpoint P95 deficit: `2,320,576 - 1,120,380 = 1,200,196` ticks.
- Required P95 reduction relative to that recorded checkpoint: **51.72%**.

Some newer notes use a rounded 1,120,000-tick number. Their published deficits must not be silently mixed with calculations using 1,120,380. The shipping gate and cadence policy remain authoritative. [S08](#s08), [S19](#s19)

The internal 950K objective remains only a design headroom target. It is neither a measured result nor a replacement acceptance threshold.

### 1.2 SRC attribution remains population-specific

The original report included a dirty-run mean SRC decomposition:

| Exclusive component | Mean ticks |
|---|---:|
| SINT minus SCPU | 227,156 |
| SPHD plus SPHC | 124,236 |
| GCRA remainder after its named children | 121,588 |
| SCPU | 69,399 |
| SHDT | 45,824 |
| SPRM | 10,591 |
| SCAT | 3,797 |
| SRC minus GCRA | 5,746 |
| **Total SRC** | **608,337** |

This is arithmetic on means from one table, not arithmetic on independent percentiles. The source is uploaded log `e5ad07c6-7313-48a7-822d-8de7f519f471.jsonl`, record 1384, timestamp `2026-09-16T21:09:56.377Z`. It declares `9470ffbee78+dirty(22)`, 1,972 samples, and five timer corrections. Its recorded WORK-H P95 is 2,318,208. None of those numbers is a new current baseline. [U01](#u01)

The GCRA remainder includes real scheduled callbacks. It is not empty bookkeeping. SCPU is nested inside SINT; GCRA is nested inside SRC. Do not double count these buckets.

### 1.3 A necessary correction to the newer distribution argument

The newer file titled “SRC has no big rocks” says its input is the **whole** 129-region PC profile, excluding `armWaitForIrq`, and obtains 1,614,414 ticks across 1,192 symbols. That is a whole-frame distribution, not an SRC-only distribution. Its approximately 571,666-tick sub-5K-symbol tail includes work outside SRC. [S06](#s06)

It also uses the older sampled `softfloat-callers.txt` shares, including 16,940 ticks for `func_ovl2_800ED490`. The N0409 correction reports that this sampled attribution was wrong and gives 7,108 leaf ticks / 9,783 including self for that function in its corrected attribution. The same report moves `guMtxCatF` substantially upward. [S09](#s09)

**Consequences:** use these sources to identify hypotheses and existing experiments, but do not use the sampled percentages as a proven exclusive cost map or as a calculation that closes the optimization search. No rerun of that known-biased sampled attribution is needed merely to repeat its conclusion.

For a new candidate, identify the source phase/call sites it changes and price only their actual executions. Preserve whole-frame WORK-H and cadence as separate outcome measurements.

---

## 2. Re-evaluation of each original candidate

| Original candidate | Revised verdict | What remains justified |
|---|---|---|
| One bound fixed pose/transform domain | **Keep as destination; narrow the first implementation.** | Remove concrete parser/traversal/state costs. The selected publication conversion is only about 2.8K ticks, not the entire pose budget. |
| Direct instance ownership and compact validity | **Revise substantially.** | One per-instance hierarchy must answer arbitrary subroot queries. Four root slots alone are insufficient. Do not repeat cache widening/shrinking. |
| Shared cubic bases | **Keep, subject to actual grouping and codegen.** | Algebra is exact with identical effective lengths/reciprocals and preserved clamps/order. Source-corpus group frequency is still unknown. |
| Horner cubic | **Demote to numerical R&D.** | Compact code demonstrated; staged-rounding differences and gameplay relevance remain unqualified. |
| Resident native motion banks | **Keep as tail-oriented architecture candidate.** | Eliminate payload acquisition/normalization and compile compact execution. Size the complete legal working set first; directory residency is not payload residency. |
| Bound native map queries | **Promote to first bounded structural pilot.** | Replace multiple cache/format/ready-check layers; specialize exact static cases before changing numerical rules. |
| Demand-driven combat geometry | **Keep conditional on measured avoided work.** | Conservative rejection before unnecessary transforms, with swept bounds and source phase/order preserved. Current collision matrices are already lazy. |
| Fixed hot fighter state / shared facts | **Keep, consumer-group by consumer-group.** | Compact phase-local data and invariant facts; no whole-frame stale snapshot or permanent mirrored FTStruct. |
| Compact scheduler | **Secondary.** | Source-ordered compact execution metadata may help, but the scheduler wrapper is not a 234K-tick opportunity. |
| DTCM/locality | **Retain successful current work; evaluate new bytes marginally.** | New hot-scalar results show locality is not “exhausted”; do not count already moved scalars as future savings. |

This ranking is an engineering recommendation from the evidence, not a prediction that the first row implemented will save the most ticks.

---

## 3. First bounded structural candidate: native map-query contexts

### 3.1 What is still happening

`reloc_backend_mp_collision.c` already caches vertices, endpoint coordinates, owner IDs, line kinds, and extents. A floor query still combines several of those caches with readiness checks, O2R/vertex-link access, dynamic-owner checks, local-coordinate preparation, interpolation, and normal preparation. The old caches are not proposals to add again. [S11](#s11)

The actual source still contains:

```c
return (f32)v1y + (((opx - (f32)v1x) / ((f32)v2x - (f32)v1x)) *
    ((f32)v2y - (f32)v1y));
```

Its `ndsMPGetFCAngle` already handles horizontal lines cheaply, but for slopes it calculates a ratio, square root, reciprocal, and normalized components. The NULL-angle early return also already exists. Do not credit an optimization with deleting normal work from callers that never request a normal. [S11](#s11)

### 3.2 An exact horizontal-floor specialization

For a selected, valid horizontal segment:

- `v1y == v2y`;
- `v1x != v2x`;
- local query coordinates and arithmetic are finite;
- the existing segment-selection and owner-state conditions have already passed.

Then the interpolated floor height is simply the bound segment height. There is no runtime division or zero-product to evaluate.

This preserves the height exactly in the current legacy float expression for the tested finite domain: integer s16 heights convert exactly, the fraction is finite, multiplication by zero produces signed zero, and adding it to the integer-derived height returns that same height under the current rounding convention. Signed-zero cases are included in the experiment. Floor-distance subtraction and all query side effects retain their existing order.

**HOST-TESTED:** 265,616 bitwise comparisons passed, including all 65,536 s16 heights, 200,000 random finite segment/query cases, reversed endpoints, endpoint queries, and signed-zero queries. This tests the height expression only—not pass-through semantics, platform updates, full collision behavior, or DS timing. The code and test are embedded in Appendix D. The analytical equivalence requires the stated preconditions; random tests alone would not establish it for every input.

**Important accounting:** the existing horizontal normal branch is already cheap. This candidate does not save a square root there. It removes the remaining horizontal distance arithmetic and, in the structural version, the preparation needed to recover the same segment data repeatedly.

### 3.3 Replacement design

Build or load one context per actual collision geometry. Its immutable records provide:

- Direct line-to-segment spans in source order.
- Native-endian source coordinates or their admitted fixed representation.
- Line kind, owner index, flags, adjacency and conservative bounds.
- Horizontal/vertical/general classification.
- Precomputed invariant normal/slope information for qualified segment types.

Keep dynamic owner status, translation, velocity and generation outside that immutable record. A query receives a bound context/line handle, rather than rediscovering those identities through multiple global caches.

The same design covers moving platforms: constant local geometry stays constant while the owner state changes. Rotation, deformation or other supported procedural geometry needs its own qualified dynamic update path; do not assume every stage is translation-only.

### 3.4 Keep the implementation small

Do not add a second broad map framework. Convert the known-line floor query and its directly relevant consumers first. Use the existing source-order fixtures and adversarial cases. After the pilot replaces its work, migrate adjacent wall/ceiling/sweep callers using the same context where their contracts actually agree.

Do not duplicate every coordinate and normal in both float and fixed indefinitely. An initial exact-layout pilot may retain the current arithmetic/ABI to isolate lookup removal. Its remaining float boundary must be explicit, and it is not fixed-runtime closure. The fixed migration follows the full relevant query/consumer chain.

The endpoint is a direct native query over admitted records, not a fast path that perpetually maintains five old caches behind it.

### 3.5 Why explicit contexts matter

The current implementation documents global geometry swaps and restores, plus cache invalidation at the setter to avoid mislabelling a cached vertex with another geometry's identity. Explicit contexts avoid resetting one shared cache merely to ask a query about another geometry. [S11](#s11)

A raw geometry pointer is not a lifetime guarantee. Context identity must distinguish destruction/reuse and in-place data mutations. Alternate geometry, scene rewind, stage replacement, hazard state changes and restore paths must all remain valid.

### 3.6 Cheap falsifiers and acceptance

Measure the number of queries by class and the work of the entire pilot path, including context lookup, dynamic-owner loads, descriptor traffic and caller-side preparation. Do not use the full physics/map bucket as the pilot's claimed ceiling.

Reject the design if it adds more descriptor/cache traffic than it removes or changes line selection, crossing decisions, signed distance, flags, collision order, moving-platform velocity transfer or ledge behavior.

Exercise non-monotonic lines, shared endpoints, slopes, epsilon boundaries, floor pass-through, walls/ceilings, active/inactive owners, fast movement, grabs against platforms and alternate geometry. Preserve independent BattleShip-derived expectations; copying the new generator's tables into its checker is not an oracle.

**Cost status:** the original diagnostic profile gave approximately 20.9K self ticks for the floor helper. Its descendants and caller preparation require scope-aware attribution. No aggregate saving for the replacement is measured here. [S12](#s12)

---

## 4. Pose and motion: compile execution, not a second scene graph

### 4.1 Correct the economic premise

The newer candidate-sizing artifact prices the examined pose publication at approximately 2,814 ticks. Its approximately 74,630 ticks for three pose bodies include their real work, not just conversion. On that observed roster/window, the floating multiply call sites for translation scaling in the pose player did not execute. That does not show those source branches are unreachable on every legal fighter/clip. [S03](#s03), [S10](#s10)

A count of zero `__aeabi_f*` calls also does not prove that there are no representation conversions: `ndsR2F32ToFixed` and `ndsR2FixedToF32` are inlined integer bit-manipulation routines. Likewise, the integer IEEE event clock still represents floating-point arithmetic semantically. Treat the measured 2.8K publication cost as a scoped price, not evidence that the entire representation question vanished. [S03](#s03), [S13](#s13)

The report's proposed 70K “ceiling” for the broad replacement is an estimate from an existing profile, not a measured rewritten implementation or a formal lower bound. Conversely, it is sufficient warning not to sell the broad rewrite as an established several-hundred-thousand-tick saving.

### 4.2 What the current structures already provide

The current `NdsFtPoseTrack` definition is 24 bytes and the pool capacity is 128 tracks per fighter. A fully allocated track pool therefore occupies 3,072 bytes per instance, or 12,288 for four, before joint records and other state. This is a capacity calculation from the current definition, not an assumption that all four pools are allocated in every scene or all 128 tracks are touched every tick. Some introductory comments still describe earlier sizes; use the struct definition. [S14](#s14)

The runtime already has running-joint masks, active-track masks, a held-body path, compact track storage and lazy collision matrix calculation. Reimplementing those features under new names is not progress.

### 4.3 A better replacement boundary

Compile source motion into small execution records and retain only truly mutable playback state. Candidate immutable data includes decoded destinations, source constants, branch/control metadata, segment coefficients, and qualified adjacent groups. Mutable data includes clocks, phase, active segment, current state for partial commands, and procedural overrides.

**Do not alias fields merely because two evaluation kinds usually use them separately.** The current header explicitly preserves both `length_invert` and `rate_linear_q` because no-payload commands can retain an old value in one field. It names a Samus Catch case that breaks a union-based simplification. A compiler must model partial writes and value liveness, not just decode the current opcode into a fresh zeroed record. [S14](#s14)

Keep the parser's source event order and last-writer rules. Compile static operand interpretation; do not erase the state machine's actual semantics.

The inspected SM64DS `ModelAnim::SetAnim` reference uses a same-file fast path to update flags and speed instead of rebuilding an animation binding. That is a useful pattern—bind immutable structure once—but Smash's same-clip seek/reset semantics still have to be derived from its own source. Do not copy the optimization's predicate without its contract. [S28](#s28)

### 4.4 Replace redundant scans without reordering visible work

`ftParamUpdateAnimKeys` invokes the pose engine and then walks the indexed joint table to cover unowned joints and material programs. Pose ownership is per joint; Samus's out-of-hierarchy grapple is a named exception. [S15](#s15)

A bound program can distinguish native pose work, independent joint programs, and material work without discovering those memberships repeatedly. However, three separate lists must not accidentally change source ordering. Where an interleaving can affect state, use a compact ordered schedule of typed records rather than blindly executing all joints, then all materials.

Binding/rebinding owns membership changes. A stopped program that can later restart must be reactivated by its actual writer; absence from an active list must not become permanent invisibility.

### 4.5 Share transforms only where the semantics match

CPU collision and GX rendering do not necessarily consume identical matrices. Scale compensation, animation locks, billboard/projection behavior, procedural attachments and quantization boundaries can differ. Current collision world/inverse matrices are already lazy. [S16](#s16)

Share authoritative fixed local values, validated topology and genuinely equivalent products. Derive consumer-specific matrices where required. This is smaller and safer than insisting that every renderer and collision consumer share one matrix representation.

A full fixed producer/consumer chain remains desirable for the final runtime; it must retire old float publication, generic state discovery and unused backing storage when its last consumer converts. A permanent full float mirror plus fixed copy fails the proposed architecture's own purpose.

### 4.6 Preserve the admitted simulation and pose schedules

Keep the required 60 Hz gameplay and source event timing. The current header documents an already-admitted held-body policy, including its stated hurtbox consequence; this review does not expand, reduce or silently reverse that policy. Do not infer a new permission to freeze joints, reduce CPU decisions or update all gameplay sockets less often. [S14](#s14)

Event-clock replacement is separate from curve acceleration. An integer rational clock does not automatically reproduce repeatedly rounded IEEE event boundaries. Preserve exact required event outcomes, hitlag/loop/seek/speed-change behavior and publication order with source-derived differential fixtures. No global epsilon and no “Q24 must be enough” assumption.

### 4.7 What would make this candidate worth continuing

The pilot must delete a named cost class—decoded-operand work, repeated control discovery, actual track-state bytes/loads, repeated basis arithmetic or redundant consumer preparation—not merely eliminate a few soft-float symbol names.

Report code/data bytes removed and introduced; include the actual runtime hot working set, binding frequency and incremental memory peak. A smaller immutable file is useful only if its active execution becomes cheaper or it enables an independently demonstrated residency win.

---

## 5. A new exact cubic reduction, and the original prototype audit

### 5.1 The original tests reproduce, but their scope stays limited

The old standalone tests were rerun in this response. Their JSON results exactly match the supplied package: 200,000 shared-basis cases, all 4,753 intervals in the 96-bit validity test, and both 150,000-case Horner trials. That checks reproducibility, not game integration.

The original shared-basis oracle and candidate are both reconstructed from the same source expression and omit the real saturation counter. Matching those two C implementations is useful but is not independent end-to-end proof. The new tests add an arbitrary-precision Python oracle, explicit clamp accounting, nominal interpolation cases and UBSan execution.

### 5.2 Exact endpoint-complement identity

The current cubic computes:

```text
h_base   = 2*t3 - 3*t2 + 65536
h_target = 3*t2 - 2*t3
```

Therefore, **after the source's actual rounded t2/t3 operations**, not merely in real arithmetic:

```text
h_base + h_target = 65536
```

The endpoint part of its accumulator can be rewritten exactly:

```text
vb*h_base + vt*h_target
    = vb*65536 + (vt-vb)*h_target
```

The remaining rate terms and final rounding/clamp do not change. This removes one general wide product without replacing the source curve with an approximate polynomial. It is not Horner. [S13](#s13)

`vt-vb` must fit its chosen type. The prototype proves and tests an explicit narrow input domain, and uses multiplication by 65,536 rather than a signed left shift of a possibly negative value. A production generator must validate actual source ranges; it cannot import the test-domain bounds as an asset-admission fact.

### 5.3 Combining it with shared bases

The arithmetic count for N truly co-phased cubic channels becomes:

| Form | General wide products |
|---|---:|
| Current separate evaluations | `9N` |
| Original shared-basis prototype | `5 + 4N` |
| Shared basis plus exact endpoint complement | `5 + 3N` |

For three channels: 27 → 14. For six: 54 → 23. For an isolated channel: 9 → 8.

These are expression-level counts, not cycle forecasts. Addressing, register pressure, branch cost, basis traffic and compiler choices are not free.

Group only equal effective `len` and `inv`, with the correct curve kind and execution conditions. Do not assume every channel on a joint shares time. End processing, no-payload writes, catch-up, independent clock state, scale and TraI can create exceptions. Preserve source write order and active/NOANIM behavior.

A compact adjacent-group implementation or compile-time group metadata is preferable to a runtime hash map for finding common bases. The latter could cost more than five multiplies.

### 5.4 New test results

**HOST-TESTED:**

- 455,644 scalar comparisons against an independent big-integer oracle and the old reconstructed C reference: **zero output mismatches**.
- This includes 105,644 Cartesian boundary cases, 250,000 broad random cases, and 100,000 nominal interpolation cases.
- 12,000 shared groups / 72,068 channels: **zero output mismatches** and zero basis-clamp multiplicity mismatches.
- Separate Clang UBSan run: **500,000 comparisons**, zero mismatches and no sanitizer diagnostics.

The broad suite includes 265,214 output-saturation cases; the extra nominal suite ensures the results are not merely equality after saturation. The test domains, seed, code and exact JSON are included in Appendix C.

**Diagnostic semantics:** if the old evaluator increments a clamp counter separately for each channel, sharing one clamped basis must preserve that multiplicity (or introduce a separately specified diagnostic contract). Equal final values alone do not establish identical diagnostic behavior. No actual game counter integration was tested here.

### 5.5 Code generation prevents an overclaim

Under the same standalone Clang 17 ARM946E-S/ARM/O2 settings:

| Function | Original bytes | New bytes |
|---|---:|---:|
| Basis builder | 224 | 232 |
| Channel evaluator | 108 | 116 |
| Standalone whole cubic | 332 | 340 |
| Approximate Horner from original package | 88 | Not changed |

**The exact three-product alternative is slightly larger in this compilation.** The new basis builder also stores a clamp-count witness that the original builder lacks, so those builder sizes do not isolate arithmetic alone. The channel/whole-function listings are likewise prototypes, not an equal-layout ROM comparison. It may still execute fewer or cheaper arithmetic instructions in a qualified use, but these results do not establish that. Inlining and the actual devkitARM register allocation can change the tradeoff again. The ELF symbol sizes and generated assembly are embedded; no DS timing was measured.

Thus this is a good bounded experiment inside the native pose work—not a promised leaner replacement and not the primary campaign all by itself.

### 5.6 Why Horner stays off the primary implementation path

The old Horner tests reproduce a maximum sampled difference of 3 Q12 units in their modest domain and 447 in their wider domain. Those are finite synthetic observations, not maximum errors over all clips.

A Q12 unit is the unit of its channel: translation, rotation and scale do not share a physical interpretation. An untyped “0.109 world units is harmless” argument would be invalid for a rotation or scale channel.

Horner requires actual clip/range tests, internal extrema and derivative checks, propagated joint/socket error analysis, and gameplay event/decision validation where its results are consumed. It also has explicit intermediate-overflow preconditions. No such full-corpus qualification was performed here.

---

## 6. Subtree invalidation: a new representation, not another cache-size experiment

### 6.1 The previous identity description was incomplete

The old report correctly demonstrated that `(root >> 4) & 3` can collide. It suggested binding by live instance and using subtree intervals, but the headline emphasis on four fighter roots was incomplete.

The newer measurement establishes that **multiple invalidated joints/subroots per fighter** are cache keys. The four-slot scheme records 16,403 hits, 16,237 misses, and 15,490 conflicts in the stated run. [S04](#s04)

A direct four-slot mapping is useful only if each slot owns a complete hierarchy representation capable of answering all subroot queries. Replacing one hash with a player index while retaining one arbitrary cached subtree per slot does not solve the problem.

### 6.2 Preserve the measured negatives

| Experiment | Reported result | Meaning for this review |
|---|---|---|
| 16 slots × 48 entries | SRC P50 −15,040; STG +51,520; WORK-H +33,984 | Reject that enlarged-table implementation as a frame-time win. |
| 32 slots × 96 entries | SRC −15,104; WORK-H +40,896 | Reject this larger variant too. |
| Four-slot table moved to DTCM | WORK-H −10,176, consuming 1,584 bytes | Poor measured use of scarce DTCM relative to later hot scalars. |
| Four slots shrunk to 48 | WORK-H P50 +1,216; P95 −6,016; no observed arena-page recovery | No bankable speed gain in that test; not a reason to halve required content capacity. |

Sources [S04](#s04), [S17](#s17). These are different reported comparisons; do not add their deltas.

The reports attribute the enlargement regression to cache effects. That is plausible and relevant, but table size alone does not prove the entire table is live in cache, a direct-map capacity threshold, or a universal minimum-cost representation. Altered linked layout and exact touched lines can also matter. The measured failed configurations remain failed without converting that explanation into a proof that every smaller design fails.

### 6.3 The representation that remains worth considering

Maintain one preorder hierarchy per fighter instance, with a mapping from existing joint IDs to preorder positions and a subtree end index. Every subtree is an interval in the same structure. Do not duplicate descendants in many independently cached lists.

As an illustrative byte-indexed layout for a validated N ≤96, J ≤96:

```text
preorder_to_joint[N]         <=96 bytes
joint_to_preorder[J]         <=96 bytes
subtree_end[N]              <=96 bytes
                              --------
array subtotal / instance   <=288 bytes
four-instance array subtotal <=1,152 bytes
```

That subtotal excludes owner/generation headers, root lookup, any required map for otherwise unindexed DObjs, alignment and validity state. Do not present 1,152 as the finished allocation. For larger valid hierarchies, widen indices or size the structure from the admitted corpus; a one-roster observation of ≤48 descendants is not a universal bound.

Reuse existing `fp->joints[]` where valid, rather than adding another full pointer array. For migrated callers that already know a joint ID, pass it through. For a DObj-only entry point, account for root-to-index lookup explicitly. A linear scan hidden behind an “O(1) invalidation” claim would be misleading.

This proposal differs from both the enlarged cache and its shrunken variant: it removes repeated overlapping representations and hash-miss flattening. It can initially preserve every current FTParts write and its exact order, avoiding a simultaneous collision-latch rewrite.

### 6.4 Compact validity is a second step

The old illustrative four-plane bitmap occupies 48 bytes per 96-joint fighter, 192 for four, but those are only validity bits. The current source differentiates root-local reset, descendant world invalidation, special transform modes and world/inverse/scale latch semantics. All readers/writers of the migrated latches must agree. [S15](#s15), [S16](#s16)

A mode value is not automatically a Boolean. Preserve modes that must survive a `mode == 1` reset. Preserve root inclusion/exclusion and descendants affected by ancestor or scale-compensation changes.

Do not pay for both bitplanes and old scattered clears forever. The endpoint must replace the authority; the intermediate topology-only step is useful because it can isolate a new mechanism with a smaller behavioral surface.

### 6.5 Required discriminators

Before widening the implementation, measure total descriptor/root-lookup/clear cost and affected outside-SRC work. Verify descendant membership under status changes, hidden-part creation/ejection, grabs, Kirby copy changes, respawn, scene rewind and duplicate fighters with separate generations.

The roughly 20.3K invalidate self-time is the current-profile cost envelope, not a promise to recover it all. A 10–15K local reduction that causes a larger outside regression is not a KEEP. A topology-only result with no net benefit can still inform the subsequent representation migration, but cannot be advertised as a speed win.

---

## 7. Motion residency and action-change cost

**Keep this candidate, but do not price it from an old read count alone.**

The inspected loader has a resident BPS1 directory yet still issues a payload `nitroromReadFile` when a destination is supplied. The September 15 evidence reports 676 payload reads; later bodies/working sets and exact after-GO counts must be checked for the candidate configuration. The loader did not change in the reviewed comparison. [S18](#s18), [S25](#s25), [S01](#s01)

A resident directory reduces metadata I/O. It does not mean the required motion bytes or decoded execution state are resident.

### Replacement

Compile compact motion data that serves the native player directly. Admit the complete required per-match working set before GO, sharing immutable content across duplicate fighters but keeping independent clocks, materials and patches. Include legal copy/child/item/rare-state closure, not just clips observed in one CPU trace.

Storage load elimination and parser simplification should use one representation where possible. Loading compressed bytes then rebuilding the old expansive parser state on each action can leave much of the tail unchanged.

### Memory proof

Report these separately:

```text
resident immutable banks
+ per-instance mutable playback state
+ in-flight decompression/binding scratch
+ stage/item/effect/audio obligations
+ transition overlap and stack bounds
- old storage and state actually retired
```

Do not sum mutually exclusive complete fighter variants as though all must be expanded simultaneously, but do not omit simultaneously reachable copy/weapon/effect states. A host-side static plan can express valid coexistence. It must remain conservative.

Do not replace the requirement with a bigger gameplay-time LRU or a demand read hidden in a worker. Moving a required blocking operation to another thread does not remove its deadline. BGM is a separately declared service; it is not permission for arbitrary after-GO motion reads. [S20](#s20)

### Measurement

Join action binds, payload reads/bytes, parse/bind cost and spikes using a coherent guest event identity. Measure late and rare states explicitly. Mean I/O per frame does not describe action-change P95, and eliminating a few isolated spikes does not prove cadence closure.

**No current complete-bank byte size or compression ratio was established here.** This candidate advances only with measured asset sizes and lifetime proof, not a promise that all precomputed matrices fit.

---

## 8. Current experimental overhead that should not become permanent

The current fast-mask `ndsFtPoseRun` contains a live read of `gNdsLabPoseJointCapLimit` and a `gNdsLabPoseJointCapEvaluated++` operation for each non-null visited entry even when the cap is zero. These are added experimental operations, separate from the older N0409 profile. [S10](#s10)

A qualified hard-on production configuration should compile out that cap dispatch and its experimental hot-loop observations after the experiment is retired. Keep the cap in an explicitly diagnostic native build when needed. Do not remove actual pose scheduling, validity or source state under the guise of removing telemetry.

If a verifier currently requires those symbols, change its instrumentation capability handling as real implementation work: an unavailable probe is reported unavailable, not fabricated as zero. Positive native engagement must still be proved through appropriate witnesses. The same-ROM experiment should carry symmetrical instrumentation in both arms; the final shipping shape needs its own qualification.

**This is a small hygiene candidate with a source-confirmed repeated operation, not a measured large saving.** Do not spend another extended campaign on it. It can be folded into a coherent SRC implementation batch with an explicit attribution.

There is also an important interpretive correction: because the cap's `continue` is before `ndsFtPoseParse`, it suppresses the excluded entries' parser/clock activity too. The report's “only evaluation” label is narrower than the source behavior. Even without this additional issue, its admitted match divergence already disqualifies the whole-match delta as a pose-cost ceiling. [S07](#s07), [S10](#s10)

---

## 9. Other retained candidates and their actual limits

### 9.1 Conservative combat bounds before detailed transforms

Current collision matrices are lazy. A new broad phase must avoid genuinely unnecessary requests or expensive narrow work, not add a second lazy cache. Bounds must include swept attack motion, radius/extent, current procedural state and any relevant prior position. Preserve source-directed hit/group records, shields, catches, reflect/absorb behavior and phase order. [S16](#s16), [S21](#s21)

The transformed-box absolute-matrix extent identity is useful for new aggregate bounds. However, the eight-corner AI-size routine identified in the original research is called during fighter setup. It is not a demonstrated per-frame opportunity. This remains explicitly demoted as a steady-state claim. [S22](#s22), [S24](#s24)

### 9.2 Fixed gameplay state and phase-valid facts

Convert concrete producer/consumer groups—movement/map state, collision descriptors or AI facts—rather than adding a parallel whole-game state. Preserve RNG call order and which mutations are visible when each callback runs.

The source already gates trait/behavior/objective decision work with `input_wait` while maintaining inputs. Do not invent another decision-rate reduction. List-order tie behavior such as jostling also rules out blindly replacing source-directed processing with symmetric pair updates. [S22](#s22), [S21](#s21)

A changed raw struct hash can be expected when representation changes, but it is not permission to dismiss every difference. Use explicit field correspondence and source-visible discrete outcomes; retain the raw witnesses where comparable. Never turn a failing gameplay test off simply because a fixed-point change is desired.

The authoritative product goal explicitly distinguishes mechanical equivalence from bit-identical arithmetic and permits fixed replacements. A label such as “GAMEPLAY” is not proof that no equivalent faster implementation exists. It identifies a stronger proof obligation. Current owner refusal of 30 Hz simulation remains binding. [S23](#s23), [S08](#s08)

### 9.3 Process scheduling

A compact ordered callback/context schedule is possible, but the actual scheduler self-cost is small relative to the unnamed work it dispatches. `gcRunGObjProcess` is about 7,087 ticks in the cited diagnostic profile, not the 234,325-tick unclassified remainder. [S06](#s06), [S12](#s12)

Preserve priority order, current-object/process state, pause/end/eject behavior and source-defined same-tick insertion/deletion. Four “run every phase for this fighter” loops can change interactions. Replacing the scheduler is secondary to removing work from the callbacks.

### 9.4 DTCM, locality and ARM kernels

The 508-byte hot-scalar relocation is already in the inspected linker and has a recorded −43,200 P50 / −43,072 P95 experiment. Its later report includes per-PC corroboration, an IRQ-alignment repair, and unresolved/contradictory qualification wording. Reuse its measured mechanism; do not claim its old delta for a new candidate or promote its report to universal acceptance. [S05](#s05)

The report's corrected remaining DTCM is approximately 1,460 bytes under the existing 12 KiB data ceiling. That is not a fresh 16 KiB budget. New placement needs current layout, initializers, IRQ alignment, stack and per-scene ownership checks. [S05](#s05)

For any new representation, price the actual touched working set and instructions, not “all table bytes divided by cache size” alone. Main-memory read-only data does not itself occupy instruction cache; generated executable code does. A historical ticks-per-byte slope from one kernel is not a universal charge for every future table or function.

The DS has a 4 KiB data cache and 8 KiB instruction cache; compact working sets and measured ARM/Thumb choices matter. Long multiplies can favor ARM kernels, but larger ARM code is not automatically faster. DMA/ARM7 cannot use ARM9 TCM buffers directly. These constraints support small specialized kernels and compact data, not a blanket compiler-flag conversion. [H01](#h01), [H02](#h02)

### 9.5 Offload order

First offload invariant work to build/load time. Use native GX for suitable visual work without synchronous readback for gameplay. Evaluate remaining divide/root scheduling only where it has safe ownership and independent work to overlap.

ARM7 offload is not the first SRC move: immediate collision and RNG-ordered AI impose synchronization and coherence costs. No one-frame-late gameplay fact, unsafe shared cache line, or hidden busy-wait is an acceptable saving. No new ARM7 implementation was evaluated in this review.

---

## 10. What the newer negative reports do not prove

This section corrects interpretation, not repository workflow.

### A. A changed match is not a ceiling for an equivalent implementation

The joint-cap report explicitly declares different triangles, object counts and behavior. Its +35,904 whole-match P50 is therefore not a price for equivalent pose work. Comparable ALL medians or similar scale do not repair the confound. A faithful faster evaluator could have a very different result. [S07](#s07)

### B. A failed cache size does not reject a different representation

The widened cache's whole-frame regression is real as reported. It does not prove that one shared preorder representation, compact indices, or a complete validity-authority replacement must allocate and touch the same data. Those are new mechanisms with their own costs; they must not reuse the old measured SRC delta as a promised win. [S04](#s04), [S17](#s17)

### C. An old collision-ring failure is not “just conversion overhead”

The original re-evaluation must continue to respect the corrected evidence: the tested collision ring already had favorable conversion density and suffered an instruction-fetch penalty. Do not rerun that same ring or misdescribe why it lost. A new attempt must remove further queries, traversal, state or fetch demand. [S09](#s09)

### D. A scalar relocation can help without proving every locality proposal will help

The hot-scalar experiment is a direct counterexample to the earlier broad claim that no DTCM candidate can pay. It does not guarantee the remaining 1,460 bytes will buy the same savings per byte. Nor do its unchanged counters alone resolve every sampling/window question. [S05](#s05)

### E. No single candidate closing the full deficit is not a valid universal rejection rule

The requested work spans SRC, FTR, STG and MISC. A useful SRC replacement may contribute materially without removing 1.2M P95 ticks by itself. Conversely, summing independent P95 deltas, component suppression experiments or means from other builds cannot prove the combined result.

After integration, re-rank all relevant frame populations and measure cadence again. An optimization can improve work within a VBlank quantum without changing median ALL immediately. It can also simply move a GPU wait to another bucket. Whole-frame work, chronology of waits and the presented-frame result are needed to distinguish those cases.

### F. Raw numeric differences need qualification, not automatic surrender or automatic acceptance

Source-equivalent fixed implementations are explicitly permitted, but changed collisions, move boundaries, RNG sequencing, missing required geometry or altered telegraphs are not made acceptable by calling them approximation. Investigate representation differences at their actual observable consumer. Preserve the product contract rather than either freezing every float bit forever or ignoring gameplay divergence. [S23](#s23)

---

## 11. Implementation priorities and falsifiers

These are candidate boundaries, not a replacement task board.

| Priority | Implement or size | Concrete work removed | First reason to reject or narrow |
|---|---|---|---|
| 1 | Bound native floor-query pilot, including horizontal class | Repeated line/owner/format/cache preparation and redundant horizontal distance arithmetic | More descriptor traffic than removed; changed source query result/order; no whole-frame gain. |
| 2 | Compact pose/motion execution pilot and complete-bank sizing | Repeated operand/control discovery, mutable state and bind/read work | Expanded banks do not fit or parser/state remains duplicated; actual hot curves too sparse for grouped evaluation. |
| 3 | One per-instance preorder topology replacing overlapping subtree caches | Hash conflicts and re-flattening without enlarged pointer-list storage | Root lookup or pointer indirection cancels the gain; missed hidden/alternate subtree. |
| Within 2 | Exact endpoint-complement and adjacent shared bases | One multiply/channel, plus redundant common basis calculations | Codegen/load/counter/grouping cost exceeds arithmetic benefit. |
| Within a batch | Remove retired cap instrumentation from hard-on production | Experimental volatile load/branch/count work | Lost real state or invalid engagement evidence; no sizeable win claimed without measurement. |
| Subsequent | Fixed map/gameplay consumers and demand-driven combat geometry | Remaining float chains, cold-state traffic, irrelevant narrow queries | Changed source decisions or unsound swept rejection. |
| Later R&D | Horner, larger fixed representation changes, scheduler/offload | Candidate-dependent | Unqualified source error, clock/order changes or new synchronization cost. |

### Minimum useful pilot output

For each pilot, return the implementation plus:

- Exactly which old hot operations and storage were removed.
- Code/data/TCM/transient-memory changes, including added lookup or patch costs.
- Source-derived semantic tests and positive route engagement.
- A correctly identified matched comparison and the applicable integrated hard-on result.
- Explicit untested fighters, stages and rare states; these stay obligations, not exclusions.

Do not run an exhaustive roster-stage matrix after each edit, but do not call a pilot universal support. Both duplicate and mixed fighters, legal slot assignments, changed topology and dynamic stage behavior must be covered before the corresponding replacement closes.

### What should not be built again as the next SRC experiment

Do not repeat flat-cache widening/shrinking, the unchanged old collision ring, a blanket pose cap, or a wrapper that translates every individual operation into and out of fixed point. Do not try a new family of huge bind tables merely because the previous small candidates were insufficient.

Do not begin with another global performance “ceiling” built by deleting gameplay. A changed workload is useful for debugging a dependency, not an accepted price for source-equivalent execution.

---

## 12. Bottom line

**The earlier architecture direction remains useful, but the order and claims needed correction.** The best-supported next work is smaller native data and execution that retire repeated discovery: bound map queries, compact motion/pose programs, and one hierarchy representation per instance. Exact curve arithmetic can support that work without changing clocks, but its benefit must survive actual code generation and memory behavior.

The new evidence does not support declaring SRC or optimization generally exhausted. It also does not support promising that the proposed rewrites close 30 FPS. The valid next step is to implement a materially different, bounded mechanism with whole-frame evidence—not replay old failed variants and not treat speculative savings as banked results.

The following appendices preserve the complete standalone code and results. They are research tools, not drop-in shipping patches.

---

## Appendix A — Reproduce the standalone experiments

Save the labeled code blocks to their indicated relative paths in a new **host research directory**, not the game source tree. The commands below describe the Linux GCC/Clang environment used here; they are not devkitARM build or repository-verifier commands.

```sh
# Reproduce the original package's experiments.
gcc -O2 -shared -fPIC Smash64DS_SRC_Candidates/kernels.c \
    -o Smash64DS_SRC_Candidates/kernels.so
python Smash64DS_SRC_Candidates/test_kernels.py

# New exact cubic and independent Python reference.
clang -O2 -shared -fPIC exact_cubic3.c -o exact_cubic3.so
python test_exact_cubic3.py

# Separate undefined-behavior run (requires Clang UBSan runtime).
clang -std=c11 -O2 -fsanitize=undefined -fno-sanitize-recover=all \
    sanitize_exact.c -o sanitize_exact
./sanitize_exact

# Exact horizontal expression special case, preserving ordinary float semantics.
gcc -O2 -shared -fPIC -fno-fast-math -ffp-contract=off \
    horizontal.c -o horizontal.so
python test_horizontal.py

# Standalone ARM946E-S code generation only—not DS execution.
clang --target=arm-none-eabi -mcpu=arm946e-s -marm -O2 \
    -ffreestanding -fno-builtin -ffunction-sections \
    -S exact_cubic3.c -o exact_cubic3_arm.s
clang --target=arm-none-eabi -mcpu=arm946e-s -marm -O2 \
    -ffreestanding -fno-builtin -ffunction-sections \
    -c exact_cubic3.c -o exact_cubic3_arm.o
readelf -sW exact_cubic3_arm.o
```

UBSan can detect executed signed-overflow/shift and related undefined operations; a clean run is not a proof for unexecuted inputs. The independent Python reference and the stated arithmetic bounds are separate checks. [H03](#h03)

## Appendix B — Original prototype code, retained for reproducibility

These original kernels are not newly qualified for the game. The approximate Horner retains its explicit admitted-range preconditions. The reference is a reconstruction of the inspected expression, not the game’s linked function.

### B1. Original C kernels

**Save as:** `Smash64DS_SRC_Candidates/kernels.c`

```c
/* Research prototypes, NOT integrated game code or performance-qualified DS code.
 * Reconstructed reference curve follows nds_anim_fixed.h at 430aca2879e9.
 * Build tools must validate coefficient/range contracts before using a fast path.
 * Signed right shifts here require arithmetic shift (ARM GCC/Clang and tested host).
 * No event-clock changes are implemented by this experiment.
 */
typedef signed int i32;
typedef unsigned int u32;
typedef signed long long i64;
_Static_assert(sizeof(i32) == 4 && sizeof(i64) == 8, "integer widths");

typedef struct { i32 a, b, c, d; } Curve;
typedef struct { i32 vb, vt, rb, rt; } Basis;

static i32 clip(i32 x, i32 bound) {
    return x > bound ? bound : x < -bound ? -bound : x;
}
static i32 clip64(i64 x) {
    return x > 2147483647LL ? 2147483647 :
           x < -2147483647LL ? -2147483647 : (i32)x;
}

i32 reference_cubic(i32 len, i32 inv, i32 vb, i32 vt, i32 rb, i32 rt) {
    i32 lc = clip(len, 1024 * 4096);
    i32 t = clip((i32)(((i64)len * inv + (1LL << 25)) >> 26), 2 * 65536);
    i32 t2 = (i32)(((i64)t*t + 32768) >> 16);
    i32 t3 = (i32)(((i64)t2*t + 32768) >> 16);
    i32 omt2 = t2 - 2*t + 65536;
    i32 hvb = 2*t3 - 3*t2 + 65536;
    i32 hvt = 3*t2 - 2*t3;
    i32 hrb = (i32)(((i64)lc * omt2 + 2048) >> 12);
    i32 hrt = (i32)(((i64)lc * (t2-t) + 2048) >> 12);
    i64 acc = (i64)vb*hvb + (i64)vt*hvt + (i64)rb*hrb + (i64)rt*hrt;
    return clip64((acc + 32768) >> 16);
}

/* Same clamping/rounding as the reference. Group ONLY tracks with equal
 * len and inv. Material/event phase and independently clocked joints may differ.
 * It is valid to call this for a group of one; it need not be a win there.
 */
void build_basis(i32 len, i32 inv, Basis *out) {
    i32 lc = clip(len, 1024 * 4096);
    i32 t = clip((i32)(((i64)len * inv + (1LL << 25)) >> 26), 2 * 65536);
    i32 t2 = (i32)(((i64)t*t + 32768) >> 16);
    i32 t3 = (i32)(((i64)t2*t + 32768) >> 16);
    out->vb = 2*t3 - 3*t2 + 65536;
    out->vt = 3*t2 - 2*t3;
    out->rb = (i32)(((i64)lc*(t2-2*t+65536) + 2048) >> 12);
    out->rt = (i32)(((i64)lc*(t2-t) + 2048) >> 12);
}
i32 evaluate_basis(const Basis *h, i32 vb, i32 vt, i32 rb, i32 rt) {
    i64 acc = (i64)vb*h->vb + (i64)vt*h->vt + (i64)rb*h->rb + (i64)rt*h->rt;
    return clip64((acc + 32768) >> 16);
}

/* Generator constructs A=2(vb-vt)+duration*(rb+rt),
 * B=3(vt-vb)-duration*(2rb+rt), C=duration*rb, D=vb, all Q12.
 * This variant is APPROXIMATE relative to the reference's staged rounding.
 * Preconditions: 0 <= t <= 65536 and each polynomial intermediate fits i32.
 * Endpoints are exact for admitted coefficients, but gameplay qualification
 * and a source-asset error corpus are still necessary. Do not change clocks.
 */
i32 evaluate_horner(const Curve *p, i32 t) {
    i32 x = (i32)(((i64)p->a*t + 32768) >> 16) + p->b;
    x = (i32)(((i64)x*t + 32768) >> 16) + p->c;
    return (i32)(((i64)x*t + 32768) >> 16) + p->d;
}

/* Example: 96 joints, three 32-bit words per validity plane. Caller already
 * bound preorder [first,end) and validated 0 <= first <= end <= 96.
 * Independent WORLD/INVERSE/SCALE planes may share this same range operation.
 * This primitive does NOT itself migrate FTParts readers or mode semantics.
 */
void invalidate_range(u32 bits[3], u32 first, u32 end) {
    while (first < end) {
        u32 word = first >> 5;
        u32 lo = first & 31;
        u32 next = ((word+1) << 5) < end ? ((word+1) << 5) : end;
        u32 hi = next - (word << 5);
        u32 mask_hi = hi == 32 ? ~0u : (1u << hi)-1;
        u32 mask_lo = lo == 0 ? 0u : (1u << lo)-1;
        bits[word] &= ~(mask_hi & ~mask_lo);
        first = next;
    }
}
```

### B2. Original test script

**Save as:** `Smash64DS_SRC_Candidates/test_kernels.py`

```python
"""Host-only synthetic validation; does NOT qualify game assets or DS timing.
Run after building kernels.so as described in README.md. Standard library only.
"""
import ctypes as C
import itertools
import json
import random
from pathlib import Path

ROOT = Path(__file__).resolve().parent
lib = C.CDLL(str(ROOT / 'kernels.so'))
I, U = C.c_int32, C.c_uint32
class Curve(C.Structure):
    _fields_ = [(name, I) for name in ('a','b','c','d')]
class Basis(C.Structure):
    _fields_ = [(name, I) for name in ('vb','vt','rb','rt')]
lib.reference_cubic.argtypes = [I]*6
lib.reference_cubic.restype = I
lib.build_basis.argtypes = [I,I,C.POINTER(Basis)]
lib.evaluate_basis.argtypes = [C.POINTER(Basis)]+[I]*4
lib.evaluate_basis.restype = I
lib.evaluate_horner.argtypes = [C.POINTER(Curve),I]
lib.evaluate_horner.restype = I
lib.invalidate_range.argtypes = [C.POINTER(U),U,U]
rng = random.Random(0x535243)
results = {'scope':'SYNTHETIC_HOST_TESTS_NOT_GAME_OR_DS_PERFORMANCE_PROOF'}

# Equal phase and reciprocal produce bit-identical factored basis arithmetic.
h = Basis()
count = 200_000
for case in range(count):
    duration = rng.randint(1, 1024)
    inv = ((1<<30) + duration//2)//duration
    length = rng.randint(-2*duration*4096, 2*duration*4096)
    values = [rng.randint(-32768,32767)*8 for _ in range(4)]
    want = lib.reference_cubic(length,inv,*values)
    lib.build_basis(length,inv,C.byref(h))
    got = lib.evaluate_basis(C.byref(h),*values)
    assert got == want, (case,want,got)
results['shared_basis'] = {'cases':count,'mismatches':0,
                         'duration_frames':[1,1024], 'phase':'[-2D,+2D]'}

# Every valid 96-joint interval; actual semantics/mutation bindings not included.
span_count = 0
for first in range(97):
    for end in range(first,97):
        before = [rng.getrandbits(32) for _ in range(3)]
        bits = (U*3)(*before)
        lib.invalidate_range(bits,first,end)
        for joint in range(96):
            want = 0 if first <= joint < end else ((before[joint//32]>>(joint%32))&1)
            assert ((bits[joint//32]>>(joint%32))&1)==want
        span_count += 1
results['validity_mask'] = {'all_intervals':span_count,'bit_mismatches':0}

# Horner is algebraically equivalent, not staged-rounding equivalent. Measure
# two synthetic domains; neither is a substitute for the extracted asset corpus.
def horner_trial(n, max_duration, endpoint_bound, rate_bound):
    maximum = 0
    maximum_case = None
    mismatches = 0
    endpoint_fail = 0
    for case in range(n):
        d = rng.randint(1,max_duration)
        vb,vt = [rng.randint(-endpoint_bound,endpoint_bound) for _ in range(2)]
        rb,rt = [rng.randint(-rate_bound,rate_bound) for _ in range(2)]
        coeff = (2*(vb-vt)+d*(rb+rt), 3*(vt-vb)-d*(2*rb+rt), d*rb, vb)
        assert all(abs(x)<(1<<30) for x in coeff)
        p = Curve(*coeff)
        length = rng.randint(0,d*4096)
        inv = ((1<<30)+d//2)//d
        t = (length*inv+(1<<25))>>26
        t = min(65536,max(0,t))
        want = lib.reference_cubic(length,inv,vb,vt,rb,rt)
        got = lib.evaluate_horner(C.byref(p),t)
        diff = abs(want-got)
        mismatches += diff != 0
        if diff>maximum:
            maximum = diff
            maximum_case = {'duration':d,'len_q12':length,'values_q12':[vb,vt,rb,rt],
                            'reference_q12':want,'horner_q12':got}
        endpoint_fail += lib.evaluate_horner(C.byref(p),0)!=vb
        endpoint_fail += lib.evaluate_horner(C.byref(p),65536)!=vt
    return {'cases':n,'duration_frames':[1,max_duration],
            'endpoint_abs_bound_units':endpoint_bound/4096,
            'rate_abs_bound_units_per_frame':rate_bound/4096,
            'non_bit_identical_cases':mismatches,
            'max_abs_difference_q12':maximum,'max_abs_difference_units':maximum/4096,
            'endpoint_failures':endpoint_fail,'largest_difference_example':maximum_case}
results['horner_modest_domain'] = horner_trial(150_000,120,8*4096,1024)
results['horner_wide_domain'] = horner_trial(150_000,1024,64*4096,8*4096)

# Counterexample to the assumption that four hash buckets imply four resident
# fighters. These are fabricated addresses, not captured game allocations.
roots = [0x02001000+i*64 for i in range(4)]
cache = [None]*4
misses = 0
for _ in range(100):
    for ptr in roots:
        slot=(ptr>>4)&3
        misses += cache[slot]!=ptr
        cache[slot]=ptr
results['pointer_hash_counterexample'] = {'synthetic_root_addresses':[hex(x) for x in roots],
                                        'accesses':400,'hash_cache_misses':misses,
                                        'indexed_owner_compulsory_misses':4}
(ROOT/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results,indent=2))
```

### B3. Original results, reproduced unchanged

```json
{
  "scope": "SYNTHETIC_HOST_TESTS_NOT_GAME_OR_DS_PERFORMANCE_PROOF",
  "shared_basis": {
    "cases": 200000,
    "mismatches": 0,
    "duration_frames": [
      1,
      1024
    ],
    "phase": "[-2D,+2D]"
  },
  "validity_mask": {
    "all_intervals": 4753,
    "bit_mismatches": 0
  },
  "horner_modest_domain": {
    "cases": 150000,
    "duration_frames": [
      1,
      120
    ],
    "endpoint_abs_bound_units": 8.0,
    "rate_abs_bound_units_per_frame": 0.25,
    "non_bit_identical_cases": 47258,
    "max_abs_difference_q12": 3,
    "max_abs_difference_units": 0.000732421875,
    "endpoint_failures": 0,
    "largest_difference_example": {
      "duration": 87,
      "len_q12": 166739,
      "values_q12": [
        -32147,
        21368,
        873,
        106
      ],
      "reference_q12": 1023,
      "horner_q12": 1026
    }
  },
  "horner_wide_domain": {
    "cases": 150000,
    "duration_frames": [
      1,
      1024
    ],
    "endpoint_abs_bound_units": 64.0,
    "rate_abs_bound_units_per_frame": 8.0,
    "non_bit_identical_cases": 145519,
    "max_abs_difference_q12": 447,
    "max_abs_difference_units": 0.109130859375,
    "endpoint_failures": 0,
    "largest_difference_example": {
      "duration": 1023,
      "len_q12": 3977096,
      "values_q12": [
        -79966,
        -96598,
        30604,
        30306
      ],
      "reference_q12": -1439606,
      "horner_q12": -1440053
    }
  },
  "pointer_hash_counterexample": {
    "synthetic_root_addresses": [
      "0x2001000",
      "0x2001040",
      "0x2001080",
      "0x20010c0"
    ],
    "accesses": 400,
    "hash_cache_misses": 400,
    "indexed_owner_compulsory_misses": 4
  }
}
```

## Appendix C — New exact cubic prototype and complete tests

### C1. Exact endpoint-complement kernel

**Save as:** `exact_cubic3.c`

```c
/* SRC research prototype; not game code or a measured DS speedup.
 * Algebra follows ndsR2AnimEvalQ at db0d088bc61a.
 * Requires arithmetic signed right shift (checked below).
 * Preconditions: |len| <= 2*1024*4096; 0 < inv <= 2^30;
 * |vb|,|vt|,|rb|,|rt| <= 2^26. These are TEST domains, not corpus admission.
 * All runtime source ranges still require validation by the asset producer.
 */
typedef signed int i32;
typedef unsigned int u32;
typedef signed long long i64;
_Static_assert(sizeof(i32)==4 && sizeof(i64)==8, "widths");
_Static_assert((-1 >> 1)==-1, "arithmetic right shift required");
typedef struct { i32 target, rate_base, rate_target; u32 clamp_count; } Basis3;
static i32 sat(i64 x) {
    return x > 2147483647LL ? 2147483647 :
           x < -2147483647LL ? -2147483647 : (i32)x;
}
static i32 bounded(i32 x, i32 b) { return x>b?b:x<-b?-b:x; }
void build_basis3(i32 len, i32 inv, Basis3 *out) {
    i32 lc=bounded(len, 1024*4096);
    i32 traw=(i32)(((i64)len*inv+(1LL<<25))>>26);
    i32 t=bounded(traw, 2*65536);
    i32 t2=(i32)(((i64)t*t+32768)>>16);
    i32 t3=(i32)(((i64)t2*t+32768)>>16);
    out->target=3*t2-2*t3;
    out->rate_base=(i32)(((i64)lc*(t2-2*t+65536)+2048)>>12);
    out->rate_target=(i32)(((i64)lc*(t2-t)+2048)>>12);
    out->clamp_count=(lc!=len)+(t!=traw);
}
/* Exact endpoint complement, not Horner. No stage of rounding is removed.
 * vt-vb fits i32 in the stated domain. A production producer must prove this.
 */
i32 evaluate_basis3(const Basis3 *h, i32 vb, i32 vt, i32 rb, i32 rt) {
    i32 delta=vt-vb;
    i64 acc=(i64)vb*65536;
    acc+=(i64)delta*h->target;
    acc+=(i64)rb*h->rate_base;
    acc+=(i64)rt*h->rate_target;
    return sat((acc+32768)>>16);
}
i32 evaluate_cubic3(i32 len,i32 inv,i32 vb,i32 vt,i32 rb,i32 rt) {
    Basis3 h; build_basis3(len,inv,&h);
    return evaluate_basis3(&h,vb,vt,rb,rt);
}
```

### C2. Independent oracle and grouping tests

**Save as:** `test_exact_cubic3.py`

```python
"""Independent big-integer oracle + regression tests. No ROM/asset corpus proof."""
import ctypes as C, json, random, pathlib, itertools
P=pathlib.Path(__file__).resolve().parent
lib=C.CDLL(str(P/'exact_cubic3.so'))
old=C.CDLL(str(P/'Smash64DS_SRC_Candidates/kernels.so'))
I=C.c_int32
class Basis3(C.Structure):
    _fields_=[('target',I),('rate_base',I),('rate_target',I),('clamp_count',C.c_uint32)]
lib.build_basis3.argtypes=[I,I,C.POINTER(Basis3)]
lib.build_basis3.restype=None
lib.evaluate_basis3.argtypes=[C.POINTER(Basis3)]+[I]*4
lib.evaluate_basis3.restype=I
old.reference_cubic.argtypes=[I]*6
old.reference_cubic.restype=I

def oracle(length,inv,vb,vt,rb,rt):
    clamp=lambda a,b:max(-b,min(b,a))
    lc=clamp(length,1024*4096)
    traw=(length*inv+(1<<25))>>26
    t=clamp(traw,2*65536)
    t2=(t*t+32768)>>16
    t3=(t2*t+32768)>>16
    a=2*t3-3*t2+65536
    b=3*t2-2*t3
    c=(lc*(t2-2*t+65536)+2048)>>12
    d=(lc*(t2-t)+2048)>>12
    before=(vb*a+vt*b+rb*c+rt*d+32768)>>16
    return clamp(before,2147483647),int(lc!=length)+int(t!=traw),int(abs(before)>2147483647)

rng=random.Random(0x53435232)
h=Basis3(); cases=0; saturated=0; clamp_cases=0

def check(length,inv,vals):
    global cases,saturated,clamp_cases
    want,clamps,out_sat=oracle(length,inv,*vals)
    assert old.reference_cubic(length,inv,*vals)==want
    lib.build_basis3(length,inv,C.byref(h))
    got=lib.evaluate_basis3(C.byref(h),*vals)
    assert got==want,(length,inv,vals,want,got)
    assert h.clamp_count==clamps
    cases+=1; saturated+=out_sat; clamp_cases+=clamps>0

# Boundary Cartesian product includes output saturation and both signs.
lengths=[-8388608,-4194305,-4194304,-4096,-1,0,1,4096,4194304,4194305,8388608]
invs=[1,1048576,357913941,1073741824]
values=[-67108864,-4096,-1,0,1,4096,67108864]
for length,inv in itertools.product(lengths,invs):
    for vals in itertools.product(values,repeat=4): check(length,inv,vals)
edge_cases=cases
for _ in range(250000):
    length=rng.randint(-8388608,8388608)
    inv=rng.randint(1,1073741824)
    vals=[rng.randint(-67108864,67108864) for _ in range(4)]
    check(length,inv,vals)

# Nominal interpolation, without extrapolation/phase clamp or output saturation.
for _ in range(100000):
    duration=rng.randint(1,120)
    length=rng.randint(0,duration*4096)
    inv=((1<<30)+duration//2)//duration
    vals=[rng.randint(-32768,32768),rng.randint(-32768,32768),
          rng.randint(-1024,1024),rng.randint(-1024,1024)]
    check(length,inv,vals)

# Shared-basis values and diagnostic multiplicity; no cross-joint grouping assumed.
shared_groups=12000; shared_channels=0
for _ in range(shared_groups):
    length=rng.randint(-8388608,8388608); inv=rng.randint(1,1073741824)
    n=rng.randint(2,10); lib.build_basis3(length,inv,C.byref(h))
    baseline_clamps=0
    for _ in range(n):
        vals=[rng.randint(-67108864,67108864) for _ in range(4)]
        want,c,_=oracle(length,inv,*vals)
        assert lib.evaluate_basis3(C.byref(h),*vals)==want
        baseline_clamps+=c
    assert n*h.clamp_count==baseline_clamps
    shared_channels+=n
results={'scope':'SYNTHETIC_HOST_NOT_GAME_OR_DS_TIMING',
         'seed':'0x53435232','edge_cases':edge_cases,'random_cases':250000,'nominal_cases':100000,
         'total_scalar_cases':cases,'output_mismatches':0,
         'cases_with_phase_or_length_clamp':clamp_cases,
         'output_saturation_cases':saturated,
         'shared_groups':shared_groups,'shared_channels':shared_channels,
         'shared_mismatches':0,'basis_clamp_multiplicity_mismatches':0,
         'ranges':{'len_q12':[-8388608,8388608],'inv_q30':[1,1073741824],
                   'each_value_or_rate':[-67108864,67108864]},
         'source_corpus_tested':False,'DS_timing_tested':False}
(P/'new_test_results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results,indent=2))
```

### C3. Sanitizer driver

**Save as:** `sanitize_exact.c`

```c
#include <stdio.h>
#include "exact_cubic3.c"
#include "Smash64DS_SRC_Candidates/kernels.c"
static u32 state=0x53524332u;
static u32 next(void){state^=state<<13;state^=state>>17;state^=state<<5;return state;}
int main(void){
    unsigned k; Basis3 h;
    for(k=0;k<500000;k++){
        i32 len=(i32)(next()%16777217u)-8388608;
        i32 inv=(i32)(next()%1073741824u)+1;
        i32 vb=(i32)(next()%134217729u)-67108864;
        i32 vt=(i32)(next()%134217729u)-67108864;
        i32 rb=(i32)(next()%134217729u)-67108864;
        i32 rt=(i32)(next()%134217729u)-67108864;
        build_basis3(len,inv,&h);
        if(evaluate_basis3(&h,vb,vt,rb,rt)!=reference_cubic(len,inv,vb,vt,rb,rt))return 1;
    }
    puts("UBSAN_PASS: 500000 cases; no mismatches or sanitizer diagnostics");
    return 0;
}
```

### C4. New exact-cubic result

```json
{
  "scope": "SYNTHETIC_HOST_NOT_GAME_OR_DS_TIMING",
  "seed": "0x53435232",
  "edge_cases": 105644,
  "random_cases": 250000,
  "nominal_cases": 100000,
  "total_scalar_cases": 455644,
  "output_mismatches": 0,
  "cases_with_phase_or_length_clamp": 296231,
  "output_saturation_cases": 265214,
  "shared_groups": 12000,
  "shared_channels": 72068,
  "shared_mismatches": 0,
  "basis_clamp_multiplicity_mismatches": 0,
  "ranges": {
    "len_q12": [
      -8388608,
      8388608
    ],
    "inv_q30": [
      1,
      1073741824
    ],
    "each_value_or_rate": [
      -67108864,
      67108864
    ]
  },
  "source_corpus_tested": false,
  "DS_timing_tested": false
}
```

### C5. Sanitizer output

```text
UBSAN_PASS: 500000 cases; no mismatches or sanitizer diagnostics
```

## Appendix D — Exact horizontal special-case prototype

The prototype retains the old float ABI solely for bitwise comparison. It is not a final no-float implementation or a whole collision kernel.

### D1. Height-expression reference and bound answer

**Save as:** `horizontal.c`

```c
/* Research-only reference of ndsMPLineDistanceFC and a bound horizontal answer.
 * The caller has already selected a finite, valid segment with y1==y2, x1!=x2.
 * This isolates an exact special case, not the full collision algorithm.
 * The legacy float ABI here is a test boundary, NOT the final fixed runtime.
 */
float floor_reference(float x,int x1,int y1,int x2,int y2){
    return (float)y1+(((x-(float)x1)/((float)x2-(float)x1))*((float)y2-(float)y1));
}
float floor_bound_horizontal(float bound_height){ return bound_height; }
```

### D2. Numeric tests

**Save as:** `test_horizontal.py`

```python
import ctypes as C,struct,random,pathlib,json
P=pathlib.Path(__file__).resolve().parent
lib=C.CDLL(str(P/'horizontal.so'))
lib.floor_reference.argtypes=[C.c_float]+[C.c_int]*4
lib.floor_reference.restype=C.c_float
lib.floor_bound_horizontal.argtypes=[C.c_float]
lib.floor_bound_horizontal.restype=C.c_float
bits=lambda f:struct.pack('<f',f)
f32=lambda f:C.c_float(f).value
n=0
for y in range(-32768,32768):
    want=lib.floor_reference(f32(0),-32768,y,32767,y)
    assert bits(want)==bits(lib.floor_bound_horizontal(f32(y)))
    n+=1
rng=random.Random(0x464c4f52)
for _ in range(200000):
    x1=rng.randint(-32768,32767);x2=rng.randint(-32768,32767)
    if x2==x1:x2=32767 if x1!=32767 else -32768
    y=rng.randint(-32768,32767)
    x=f32(x1+(x2-x1)*rng.random())
    want=lib.floor_reference(x,x1,y,x2,y)
    assert bits(want)==bits(lib.floor_bound_horizontal(f32(y)))
    n+=1
# Signed-zero query, reversed endpoints, and both boundary endpoints.
for y in [-32768,-1,0,1,32767]:
    for x1,x2 in [(-1,1),(1,-1),(-32768,32767),(32767,-32768)]:
        for x in [-0.0,0.0,float(x1),float(x2)]:
            assert bits(lib.floor_reference(x,x1,y,x2,y))==bits(f32(y))
            n+=1
r={'scope':'HOST_NUMERIC_SPECIAL_CASE_NOT_COLLISION_OR_DS_PROOF','cases':n,
   'bit_mismatches':0,'all_s16_heights_tested':True,
   'general_slopes_tested':False,'DS_timing_tested':False,
   'compiler_flags':'gcc -O2 -shared -fPIC -fno-fast-math -ffp-contract=off'}
(P/'horizontal_results.json').write_text(json.dumps(r,indent=2)+'\n')
print(json.dumps(r,indent=2))
```

### D3. Horizontal result

```json
{
  "scope": "HOST_NUMERIC_SPECIAL_CASE_NOT_COLLISION_OR_DS_PROOF",
  "cases": 265616,
  "bit_mismatches": 0,
  "all_s16_heights_tested": true,
  "general_slopes_tested": false,
  "DS_timing_tested": false,
  "compiler_flags": "gcc -O2 -shared -fPIC -fno-fast-math -ffp-contract=off"
}
```

## Appendix E — Compiler output

### E1. Exact sizes measured in this response

```json
{
  "old": {
    "reference_cubic": 332,
    "build_basis": 224,
    "evaluate_basis": 108,
    "evaluate_horner": 88,
    "invalidate_range": 104
  },
  "new": {
    "build_basis3": 232,
    "evaluate_basis3": 116,
    "evaluate_cubic3": 340
  },
  "compiler": "clang version 17.0.0 (https://github.com/swiftlang/llvm-project.git 10999b6d034fe318f3d56c83bddb6572593a8bb0)",
  "flags": "--target=arm-none-eabi -mcpu=arm946e-s -marm -O2 -ffreestanding -fno-builtin -ffunction-sections",
  "timing_measured": false
}
```

### E2. Complete newly generated ARM assembly

**Save as:** `exact_cubic3_arm.s`

```asm
	.text
	.syntax unified
	.eabi_attribute	67, "2.09"	@ Tag_conformance
	.cpu	arm946e-s
	.eabi_attribute	6, 4	@ Tag_CPU_arch
	.eabi_attribute	8, 1	@ Tag_ARM_ISA_use
	.eabi_attribute	9, 1	@ Tag_THUMB_ISA_use
	.eabi_attribute	34, 0	@ Tag_CPU_unaligned_access
	.eabi_attribute	17, 1	@ Tag_ABI_PCS_GOT_use
	.eabi_attribute	20, 1	@ Tag_ABI_FP_denormal
	.eabi_attribute	21, 0	@ Tag_ABI_FP_exceptions
	.eabi_attribute	23, 3	@ Tag_ABI_FP_number_model
	.eabi_attribute	24, 1	@ Tag_ABI_align_needed
	.eabi_attribute	25, 1	@ Tag_ABI_align_preserved
	.eabi_attribute	38, 1	@ Tag_ABI_FP_16bit_format
	.eabi_attribute	18, 4	@ Tag_ABI_PCS_wchar_t
	.eabi_attribute	26, 2	@ Tag_ABI_enum_size
	.eabi_attribute	14, 0	@ Tag_ABI_PCS_R9_use
	.file	"exact_cubic3.c"
	.section	.text.build_basis3,"ax",%progbits
	.globl	build_basis3                    @ -- Begin function build_basis3
	.p2align	2
	.type	build_basis3,%function
	.code	32                              @ @build_basis3
build_basis3:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r11, lr}
	push	{r4, r5, r6, r7, r11, lr}
	.setfp	r11, sp, #16
	add	r11, sp, #16
	mov	lr, #33554432
	mov	r3, #0
	mov	r6, #0
	mov	r7, #2048
	mov	r12, #0
	mov	r5, #2048
	smlal	lr, r3, r1, r0
	lsr	r1, lr, #26
	orr	lr, r1, r3, lsl #6
	mov	r1, #16646144
	mov	r3, #1069547520
	orr	r1, r1, #-16777216
	cmn	lr, #131072
	orr	r3, r3, #-1073741824
	movgt	r1, lr
	cmp	r1, #131072
	movge	r1, #131072
	cmn	r0, #4194304
	movgt	r3, r0
	cmp	r3, #4194304
	movge	r3, #4194304
	subs	r0, r3, r0
	movne	r0, #1
	cmp	r1, lr
	mov	lr, #0
	addne	r0, r0, #1
	str	r0, [r2, #12]
	mov	r0, #32768
	smlal	r0, lr, r1, r1
	lsr	r0, r0, #16
	orr	r0, r0, lr, lsl #16
	sub	r4, r0, r1
	smlal	r7, r6, r4, r3
	lsr	r4, r7, #12
	orr	r4, r4, r6, lsl #20
	asr	r6, r1, #31
	str	r4, [r2, #8]
	sub	r4, r0, r1, lsl #1
	add	r4, r4, #65536
	smlal	r5, r12, r4, r3
	lsr	r3, r5, #12
	orr	r3, r3, r12, lsl #20
	str	r3, [r2, #4]
	umull	r3, r7, r0, r1
	mla	r5, r0, r6, r7
	lsr	r7, lr, #16
	add	r0, r0, r0, lsl #1
	mul	r6, r7, r1
	adds	r1, r3, #32768
	adc	r3, r5, r6
	lsr	r1, r1, #15
	orr	r1, r1, r3, lsl #17
	bic	r1, r1, #1
	sub	r0, r0, r1
	str	r0, [r2]
	pop	{r4, r5, r6, r7, r11, pc}
.Lfunc_end0:
	.size	build_basis3, .Lfunc_end0-build_basis3
	.cantunwind
	.fnend
                                        @ -- End function
	.section	.text.evaluate_basis3,"ax",%progbits
	.globl	evaluate_basis3                 @ -- Begin function evaluate_basis3
	.p2align	2
	.type	evaluate_basis3,%function
	.code	32                              @ @evaluate_basis3
evaluate_basis3:
	.fnstart
@ %bb.0:
	.save	{r4, r10, r11, lr}
	push	{r4, r10, r11, lr}
	.setfp	r11, sp, #8
	add	r11, sp, #8
	sub	lr, r2, r1
	asr	r2, r1, #31
	mov	r12, #32768
	lsl	r2, r2, #16
	orr	r12, r12, r1, lsl #16
	orr	r1, r2, r1, lsr #16
	ldm	r0, {r2, r4}
	ldr	r0, [r0, #8]
	smlal	r12, r1, r2, lr
	ldr	r2, [r11, #8]
	smlal	r12, r1, r4, r3
	mvn	r3, #0
	smlal	r12, r1, r0, r2
	mov	r2, #0
	lsr	r0, r12, #16
	orr	r0, r0, r1, lsl #16
	rsbs	r4, r0, #-2147483647
	sbcs	r4, r3, r1, asr #16
	movlt	r2, #1
	cmp	r2, #0
	asrne	r3, r1, #16
	moveq	r0, #-2147483647
	mvn	r1, #-2147483648
	subs	r2, r0, r1
	sbcs	r2, r3, #0
	movge	r0, r1
	pop	{r4, r10, r11, pc}
.Lfunc_end1:
	.size	evaluate_basis3, .Lfunc_end1-evaluate_basis3
	.cantunwind
	.fnend
                                        @ -- End function
	.section	.text.evaluate_cubic3,"ax",%progbits
	.globl	evaluate_cubic3                 @ -- Begin function evaluate_cubic3
	.p2align	2
	.type	evaluate_cubic3,%function
	.code	32                              @ @evaluate_cubic3
evaluate_cubic3:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.setfp	r11, sp, #28
	add	r11, sp, #28
	push	{r3}                            @ 4-byte Spill
	mov	r5, #33554432
	mov	r4, #0
	mov	r7, #1069547520
	mov	r10, #0
	mov	r12, r2
	mov	r3, #32768
	smlal	r5, r4, r1, r0
	orr	r7, r7, #-1073741824
	lsr	r1, r5, #26
	orr	r5, r1, r4, lsl #6
	mov	r1, #16646144
	orr	r1, r1, #-16777216
	cmn	r5, #131072
	movgt	r1, r5
	mov	r5, #32768
	cmp	r1, #131072
	movge	r1, #131072
	cmn	r0, #4194304
	smlal	r5, r10, r1, r1
	movgt	r7, r0
	lsr	r5, r5, #16
	cmp	r7, #4194304
	orr	lr, r5, r10, lsl #16
	movge	r7, #4194304
	sub	r5, lr, r1, lsl #1
	lsl	r6, r7, #20
	add	r5, r5, #65536
	umull	r9, r0, r6, r5
	asr	r4, r5, #31
	mla	r2, r6, r4, r0
	asr	r0, r7, #31
	lsl	r0, r0, #20
	orr	r4, r0, r7, lsr #12
	ldr	r7, [r11, #8]
	mul	r0, r4, r5
	adds	r5, r9, #-2147483648
	asr	r5, r12, #31
	adc	r2, r2, r0
	orr	r0, r3, r12, lsl #16
	lsl	r5, r5, #16
	orr	r5, r5, r12, lsr #16
	smlal	r0, r5, r2, r7
	sub	r2, lr, r1
	umull	r9, r7, r6, r2
	asr	r8, r2, #31
	mla	r3, r6, r8, r7
	mul	r6, r4, r2
	adds	r2, r9, #-2147483648
	asr	r4, r1, #31
	adc	r2, r3, r6
	ldr	r3, [r11, #12]
	smlal	r0, r5, r2, r3
	umull	r2, r3, lr, r1
	mla	r6, lr, r4, r3
	lsr	r3, r10, #16
	mul	r4, r3, r1
	adds	r1, r2, #32768
	adc	r2, r6, r4
	lsr	r1, r1, #15
	orr	r1, r1, r2, lsl #17
	add	r2, lr, lr, lsl #1
	bic	r1, r1, #1
	sub	r1, r2, r1
	ldr	r2, [sp]                        @ 4-byte Reload
	sub	r2, r2, r12
	smlal	r0, r5, r1, r2
	mvn	r1, #0
	lsr	r0, r0, #16
	orr	r0, r0, r5, lsl #16
	rsbs	r2, r0, #-2147483647
	sbcs	r2, r1, r5, asr #16
	mov	r2, #0
	movlt	r2, #1
	cmp	r2, #0
	mvn	r2, #-2147483648
	moveq	r0, #-2147483647
	asrne	r1, r5, #16
	subs	r3, r0, r2
	sbcs	r1, r1, #0
	movge	r0, r2
	sub	sp, r11, #28
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, pc}
.Lfunc_end2:
	.size	evaluate_cubic3, .Lfunc_end2-evaluate_cubic3
	.cantunwind
	.fnend
                                        @ -- End function
	.ident	"clang version 17.0.0 (https://github.com/swiftlang/llvm-project.git 10999b6d034fe318f3d56c83bddb6572593a8bb0)"
	.section	".note.GNU-stack","",%progbits
	.addrsig
	.eabi_attribute	30, 1	@ Tag_ABI_optimization_goals
```

### E3. Original recorded ARM assembly

The old C symbol sizes were recompiled and reproduced; the following is the original supplied assembly listing, retained for comparison.

```asm
	.text
	.syntax unified
	.eabi_attribute	67, "2.09"	@ Tag_conformance
	.cpu	arm946e-s
	.eabi_attribute	6, 4	@ Tag_CPU_arch
	.eabi_attribute	8, 1	@ Tag_ARM_ISA_use
	.eabi_attribute	9, 1	@ Tag_THUMB_ISA_use
	.eabi_attribute	34, 0	@ Tag_CPU_unaligned_access
	.eabi_attribute	17, 1	@ Tag_ABI_PCS_GOT_use
	.eabi_attribute	20, 1	@ Tag_ABI_FP_denormal
	.eabi_attribute	21, 0	@ Tag_ABI_FP_exceptions
	.eabi_attribute	23, 3	@ Tag_ABI_FP_number_model
	.eabi_attribute	24, 1	@ Tag_ABI_align_needed
	.eabi_attribute	25, 1	@ Tag_ABI_align_preserved
	.eabi_attribute	38, 1	@ Tag_ABI_FP_16bit_format
	.eabi_attribute	18, 4	@ Tag_ABI_PCS_wchar_t
	.eabi_attribute	26, 2	@ Tag_ABI_enum_size
	.eabi_attribute	14, 0	@ Tag_ABI_PCS_R9_use
	.file	"kernels.c"
	.section	.text.reference_cubic,"ax",%progbits
	.globl	reference_cubic                 @ -- Begin function reference_cubic
	.p2align	2
	.type	reference_cubic,%function
	.code	32                              @ @reference_cubic
reference_cubic:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	push	{r4, r5, r6, r7, r8, r9, r10, r11, lr}
	.setfp	r11, sp, #28
	add	r11, sp, #28
	push	{r2}                            @ 4-byte Spill
	mov	r4, #33554432
	mov	lr, #0
	mov	r6, #1069547520
	mov	r5, #0
	mov	r12, #0
	smlal	r4, lr, r1, r0
	orr	r6, r6, #-1073741824
	lsr	r1, r4, #26
	orr	r4, r1, lr, lsl #6
	mov	r1, #16646144
	orr	r1, r1, #-16777216
	cmn	r4, #131072
	movgt	r1, r4
	mov	r4, #32768
	cmp	r1, #131072
	movge	r1, #131072
	cmn	r0, #4194304
	smlal	r4, r5, r1, r1
	movgt	r6, r0
	lsr	r4, r4, #16
	cmp	r6, #4194304
	orr	lr, r4, r5, lsl #16
	movge	r6, #4194304
	sub	r4, lr, r1, lsl #1
	lsl	r0, r6, #20
	add	r4, r4, #65536
	umull	r8, r9, r0, r4
	asr	r7, r4, #31
	mla	r10, r0, r7, r9
	asr	r7, r6, #31
	lsl	r7, r7, #20
	orr	r6, r7, r6, lsr #12
	mul	r7, r6, r4
	adds	r4, r8, #-2147483648
	adc	r8, r10, r7
	sub	r7, lr, r1
	umull	r9, r10, r0, r7
	asr	r4, r7, #31
	mla	r2, r0, r4, r10
	mul	r0, r6, r7
	adds	r4, r9, #-2147483648
	asr	r7, r1, #31
	adc	r0, r2, r0
	ldr	r2, [r11, #12]
	smull	r4, r6, r0, r2
	ldr	r0, [r11, #8]
	smlal	r4, r6, r8, r0
	umull	r8, r2, lr, r1
	mla	r0, lr, r7, r2
	lsr	r2, r5, #16
	mul	r5, r2, r1
	adds	r1, r8, #32768
	adc	r0, r0, r5
	lsr	r1, r1, #15
	orr	r0, r1, r0, lsl #17
	add	r1, lr, lr, lsl #1
	bic	r0, r0, #1
	sub	r2, r1, r0
	sub	r0, r0, r1
	ldr	r1, [sp]                        @ 4-byte Reload
	smlal	r4, r6, r2, r3
	add	r0, r0, #65536
	mvn	r2, #0
	smlal	r4, r6, r0, r1
	adds	r0, r4, #32768
	adc	r1, r6, #0
	lsr	r0, r0, #16
	orr	r0, r0, r1, lsl #16
	rsbs	r3, r0, #-2147483647
	sbcs	r3, r2, r1, asr #16
	movlt	r12, #1
	cmp	r12, #0
	asrne	r2, r1, #16
	moveq	r0, #-2147483647
	mvn	r1, #-2147483648
	subs	r3, r0, r1
	sbcs	r2, r2, #0
	movge	r0, r1
	sub	sp, r11, #28
	pop	{r4, r5, r6, r7, r8, r9, r10, r11, pc}
.Lfunc_end0:
	.size	reference_cubic, .Lfunc_end0-reference_cubic
	.cantunwind
	.fnend
                                        @ -- End function
	.section	.text.build_basis,"ax",%progbits
	.globl	build_basis                     @ -- Begin function build_basis
	.p2align	2
	.type	build_basis,%function
	.code	32                              @ @build_basis
build_basis:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r7, r11, lr}
	push	{r4, r5, r6, r7, r11, lr}
	.setfp	r11, sp, #16
	add	r11, sp, #16
	mov	lr, #33554432
	mov	r3, #0
	mov	r5, #1069547520
	mov	r6, #0
	mov	r7, #2048
	mov	r12, #0
	smlal	lr, r3, r1, r0
	orr	r5, r5, #-1073741824
	lsr	r1, lr, #26
	mov	lr, #0
	orr	r3, r1, r3, lsl #6
	mov	r1, #16646144
	orr	r1, r1, #-16777216
	cmn	r3, #131072
	movgt	r1, r3
	mov	r3, #32768
	cmp	r1, #131072
	movge	r1, #131072
	cmn	r0, #4194304
	smlal	r3, lr, r1, r1
	movgt	r5, r0
	mov	r0, #2048
	lsr	r3, r3, #16
	cmp	r5, #4194304
	orr	r3, r3, lr, lsl #16
	movge	r5, #4194304
	sub	r4, r3, r1
	smlal	r7, r6, r4, r5
	lsr	r4, r7, #12
	orr	r4, r4, r6, lsl #20
	asr	r6, r1, #31
	str	r4, [r2, #12]
	sub	r4, r3, r1, lsl #1
	add	r4, r4, #65536
	smlal	r0, r12, r4, r5
	lsr	r0, r0, #12
	orr	r0, r0, r12, lsl #20
	str	r0, [r2, #8]
	umull	r0, r7, r3, r1
	mla	r5, r3, r6, r7
	lsr	r7, lr, #16
	adds	r0, r0, #32768
	mul	r6, r7, r1
	lsr	r0, r0, #15
	adc	r1, r5, r6
	orr	r0, r0, r1, lsl #17
	add	r1, r3, r3, lsl #1
	bic	r0, r0, #1
	sub	r3, r1, r0
	sub	r0, r0, r1
	add	r0, r0, #65536
	str	r3, [r2, #4]
	str	r0, [r2]
	pop	{r4, r5, r6, r7, r11, pc}
.Lfunc_end1:
	.size	build_basis, .Lfunc_end1-build_basis
	.cantunwind
	.fnend
                                        @ -- End function
	.section	.text.evaluate_basis,"ax",%progbits
	.globl	evaluate_basis                  @ -- Begin function evaluate_basis
	.p2align	2
	.type	evaluate_basis,%function
	.code	32                              @ @evaluate_basis
evaluate_basis:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r10, r11, lr}
	push	{r4, r5, r6, r10, r11, lr}
	.setfp	r11, sp, #16
	add	r11, sp, #16
	ldm	r0, {r12, lr}
	smull	r5, r6, lr, r2
	ldr	r4, [r0, #8]
	ldr	r0, [r0, #12]
	mov	r2, #0
	smlal	r5, r6, r12, r1
	ldr	r1, [r11, #8]
	smlal	r5, r6, r4, r3
	mvn	r3, #0
	smlal	r5, r6, r0, r1
	adds	r0, r5, #32768
	adc	r1, r6, #0
	lsr	r0, r0, #16
	orr	r0, r0, r1, lsl #16
	rsbs	r6, r0, #-2147483647
	sbcs	r6, r3, r1, asr #16
	movlt	r2, #1
	cmp	r2, #0
	asrne	r3, r1, #16
	moveq	r0, #-2147483647
	mvn	r1, #-2147483648
	subs	r2, r0, r1
	sbcs	r2, r3, #0
	movge	r0, r1
	pop	{r4, r5, r6, r10, r11, pc}
.Lfunc_end2:
	.size	evaluate_basis, .Lfunc_end2-evaluate_basis
	.cantunwind
	.fnend
                                        @ -- End function
	.section	.text.evaluate_horner,"ax",%progbits
	.globl	evaluate_horner                 @ -- Begin function evaluate_horner
	.p2align	2
	.type	evaluate_horner,%function
	.code	32                              @ @evaluate_horner
evaluate_horner:
	.fnstart
@ %bb.0:
	.save	{r4, r5, r6, r10, r11, lr}
	push	{r4, r5, r6, r10, r11, lr}
	.setfp	r11, sp, #16
	add	r11, sp, #16
	ldm	r0, {r2, r3, r12, lr}
	mov	r5, #0
	mov	r6, #32768
	mov	r0, #0
	mov	r4, #32768
	smlal	r6, r5, r2, r1
	lsr	r2, r6, #16
	orr	r2, r2, r5, lsl #16
	mov	r5, #32768
	add	r2, r3, r2
	mov	r3, #0
	smlal	r5, r3, r2, r1
	lsr	r2, r5, #16
	orr	r2, r2, r3, lsl #16
	add	r2, r12, r2
	smlal	r4, r0, r2, r1
	lsr	r1, r4, #16
	orr	r0, r1, r0, lsl #16
	add	r0, lr, r0
	pop	{r4, r5, r6, r10, r11, pc}
.Lfunc_end3:
	.size	evaluate_horner, .Lfunc_end3-evaluate_horner
	.cantunwind
	.fnend
                                        @ -- End function
	.section	.text.invalidate_range,"ax",%progbits
	.globl	invalidate_range                @ -- Begin function invalidate_range
	.p2align	2
	.type	invalidate_range,%function
	.code	32                              @ @invalidate_range
invalidate_range:
	.fnstart
@ %bb.0:
	cmp	r1, r2
	bxhs	lr
.LBB4_1:
	.save	{r4, r5, r6, r7, r11, lr}
	push	{r4, r5, r6, r7, r11, lr}
	.setfp	r11, sp, #16
	add	r11, sp, #16
	mvn	lr, #0
	mvn	r12, #3
.LBB4_2:                                @ =>This Inner Loop Header: Depth=1
	bic	r5, r1, #31
	mov	r7, r2
	and	r3, r1, #31
	and	r1, r12, r1, lsr #3
	add	r6, r5, #32
	lsl	r4, lr, r3
	cmp	r6, r2
	movlo	r7, r6
	sub	r5, r7, r5
	bic	r4, r4, lr, lsl r5
	cmp	r5, #32
	lsleq	r4, lr, r3
	ldr	r3, [r0, r1]
	cmp	r6, r2
	bic	r3, r3, r4
	str	r3, [r0, r1]
	mov	r1, r7
	blo	.LBB4_2
@ %bb.3:
	pop	{r4, r5, r6, r7, r11, lr}
	bx	lr
.Lfunc_end4:
	.size	invalidate_range, .Lfunc_end4-invalidate_range
	.cantunwind
	.fnend
                                        @ -- End function
	.ident	"clang version 17.0.0 (https://github.com/swiftlang/llvm-project.git 10999b6d034fe318f3d56c83bddb6572593a8bb0)"
	.section	".note.GNU-stack","",%progbits
	.addrsig
	.eabi_attribute	30, 1	@ Tag_ABI_optimization_goals
```

## Appendix F — Source index and provenance

Repository links below are pinned to the reviewed HEAD unless identified as a compare URL. They are evidence links, not instructions to treat historical reports as current acceptance. Some historical numeric details are reused from the previous audit; this response did not rerun their ROMs.

- <a id="s01"></a>**[S01] [Repository branch and 85-commit comparison](https://github.com/rockenrooster/Smash64DS_Port/compare/430aca2879e9071dc2b22f944f5c2909c9ce7aa4...db0d088bc61ac3e85f07a349857a1b3ec7eef55b)
- <a id="s02"></a>**[S02] [Historical clean-baseline gap report](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-16_p2-2p8-gap-sizing/README.md) — `artifacts/performance/2026-09-16_p2-2p8-gap-sizing/README.md`
- <a id="s03"></a>**[S03] [Newer SRC candidate-sizing assessment](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-16_p2-2p8-src-candidate-sizing/README.md) — `artifacts/performance/2026-09-16_p2-2p8-src-candidate-sizing/README.md`
- <a id="s04"></a>**[S04] [Actual flat-cache conflict/widening experiment](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-16_p2-2p8-n0503-flat-cache/README.md) — `artifacts/performance/2026-09-16_p2-2p8-n0503-flat-cache/README.md`
- <a id="s05"></a>**[S05] [Hot-scalar DTCM experiment, correction and status](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-17_p2-2p8-dtcm-hot-scalars/README.md) — `artifacts/performance/2026-09-17_p2-2p8-dtcm-hot-scalars/README.md`
- <a id="s06"></a>**[S06] [Later whole-frame distribution presented as SRC assessment](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-17_p2-2p8-src-distribution/README.md) — `artifacts/performance/2026-09-17_p2-2p8-src-distribution/README.md`
- <a id="s07"></a>**[S07] [Joint-cap experiment and explicit divergence evidence](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-16_p2-2p8-joint-cap-ladder/README.md) — `artifacts/performance/2026-09-16_p2-2p8-joint-cap-ladder/README.md`
- <a id="s08"></a>**[S08] [Current execution-board evidence and remaining gates](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/docs/P2_EXECUTION_BOARD.md) — `docs/P2_EXECUTION_BOARD.md`
- <a id="s09"></a>**[S09] [N0409 attribution correction and previous collision-ring result](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-16_p2-2p8-n0409-profile/CANDIDATE_SELECTION.md) — `artifacts/performance/2026-09-16_p2-2p8-n0409-profile/CANDIDATE_SELECTION.md`
- <a id="s10"></a>**[S10] [Current pose implementation; cap branch before parser](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/nds/nds_ft_pose.c) — `src/nds/nds_ft_pose.c`
- <a id="s11"></a>**[S11] [Map contexts, memo layers, floor arithmetic and normal preparation](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/port/reloc_backend_mp_collision.c) — `src/port/reloc_backend_mp_collision.c`
- <a id="s12"></a>**[S12] [Original 129-region diagnostic census](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-16_p2-2p8-n0409-profile/census.txt) — `artifacts/performance/2026-09-16_p2-2p8-n0409-profile/census.txt`
- <a id="s13"></a>**[S13] [Fixed curve evaluator and integer representation conversions](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/include/nds/nds_anim_fixed.h) — `include/nds/nds_anim_fixed.h`
- <a id="s14"></a>**[S14] [Current pose structures, partial-write semantics and admitted hold behavior](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/include/nds/nds_ft_pose.h) — `include/nds/nds_ft_pose.h`
- <a id="s15"></a>**[S15] [Invalidation, subroot cache and joint/material traversal](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/port/reloc_backend_compat_shims.c) — `src/port/reloc_backend_compat_shims.c`
- <a id="s16"></a>**[S16] [BattleShip collision matrix/latch behavior](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/decomp/BattleShip-main/decomp/src/gm/gmcollision.c) — `decomp/BattleShip-main/decomp/src/gm/gmcollision.c`
- <a id="s17"></a>**[S17] [Flat-cache shrink experiment and its actual limits](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-17_p2-2p8-flatcache-shrink/README.md) — `artifacts/performance/2026-09-17_p2-2p8-flatcache-shrink/README.md`
- <a id="s18"></a>**[S18] [Payload loader versus resident directory](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/nds/nds_reloc_assets.c) — `src/nds/nds_reloc_assets.c`
- <a id="s19"></a>**[S19] [Measurement populations and independent cadence acceptance](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/docs/VERIFYING.md) — `docs/VERIFYING.md`
- <a id="s20"></a>**[S20] [Required motion/texture admission and declared BGM service](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/docs/p2/P2-texture-residency.md) — `docs/p2/P2-texture-residency.md`
- <a id="s21"></a>**[S21] [BattleShip phase order, attack positions and jostling](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/decomp/BattleShip-main/decomp/src/ft/ftmain.c) — `decomp/BattleShip-main/decomp/src/ft/ftmain.c`
- <a id="s22"></a>**[S22] [AI update gating and setup-time damage-size function](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/decomp/BattleShip-main/decomp/src/ft/ftcomputer.c) — `decomp/BattleShip-main/decomp/src/ft/ftcomputer.c`
- <a id="s23"></a>**[S23] [Authoritative equivalence, specialization and native requirements](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/PROJECT_GOAL.md) — `PROJECT_GOAL.md`
- <a id="s24"></a>**[S24] [Fighter construction calls damage-size setup](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/decomp/BattleShip-main/decomp/src/ft/ftmanager.c) — `decomp/BattleShip-main/decomp/src/ft/ftmanager.c`
- <a id="s25"></a>**[S25] [Historical 676 payload reads](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-15_p2-2p8-ftanim-reloc-final/README.md) — `artifacts/performance/2026-09-15_p2-2p8-ftanim-reloc-final/README.md`
- <a id="s26"></a>**[S26] [Original source ordered scheduler](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/decomp/BattleShip-main/decomp/src/sys/objman.c) — `decomp/BattleShip-main/decomp/src/sys/objman.c`
- <a id="s27"></a>**[S27] [Current TCM placement](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/linker/nds_hot_text.ld) — `linker/nds_hot_text.ld`
- <a id="s28"></a>**[S28] [SM64DS same-file SetAnim specialization reference](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/decomp/sm64ds-decomp/src/_ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj.c) — `decomp/sm64ds-decomp/src/_ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj.c`
- <a id="h01"></a>**[H01] [BlocksDS: optimization and ARM/Thumb/cache tradeoffs](https://blocksds.skylyrac.net/tutorial/advanced/optimizing_code/)
- <a id="h02"></a>**[H02] [BlocksDS: TCM/cache ownership and coherency](https://blocksds.skylyrac.net/tutorial/intermediate/tcm_and_cache/)
- <a id="h03"></a>**[H03] [Clang UBSan documentation](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)

<a id="u01"></a>**[U01] Uploaded historical SRC table.** `e5ad07c6-7313-48a7-822d-8de7f519f471.jsonl`, record 1384, `2026-09-16T21:09:56.377Z`. The full record is retained in the previous consolidated report. Its raw-log SHA-256 as recorded there is `ba5607955a9efb869699ac9043e95e2363225df07f6ae4c77177b469a69d88b1`. This response re-read the consolidated record but did not independently requalify that dirty ROM or its timer corrections.

### Local input identities

- `Smash64DS_SRC_Optimization_Complete.md`: SHA-256 `41e0a3cb66440fa0e57e2f75e5f6be761aeaa437d9c4e0ab597ffc9d4843595c`.
- `Smash64DS_SRC_Candidates.zip`: SHA-256 `a8f25afb5782f1e1a50c04cbdd4960172a29d27c5fe2c1ff06461f5643bac777`.

### Embedded experiment source identities

- `Smash64DS_SRC_Candidates/kernels.c`: SHA-256 `f22f5488fcb088cd1bf8d03a3fa5b0e2ed2d08d83f1dfd0a2291a69898e3f924`.
- `Smash64DS_SRC_Candidates/test_kernels.py`: SHA-256 `29817f6410c9343c2aba84a2695d1f1f6bc7c781e7491f021a2672e1813e703f`.
- `exact_cubic3.c`: SHA-256 `9193afafb2729e3ea9bed1e4aaf609a7abc0443e5cbeb146f877c8caacdf59f5`.
- `test_exact_cubic3.py`: SHA-256 `a7f1b0bcb8e692a7a5a0595f8514605bc165da3ce184a5d19f410523178d3be6`.
- `sanitize_exact.c`: SHA-256 `16dcbf631ec2cb6a4c005ddb032e5309008d2036ac60a95d4d19c852a5bd7a9c`.
- `horizontal.c`: SHA-256 `c5ed88f97cc92950a535c6ceab19f3276762419977d90f251b8906a8c9c079aa`.
- `test_horizontal.py`: SHA-256 `80a374889a618dbf5912f7de02e1c9781560ae1fe0e8afe0522c3b21a2e08a0f`.

**Final status:** revised research and executed standalone host experiments. No game implementation, publication, runtime performance pass, or universal content coverage is asserted.

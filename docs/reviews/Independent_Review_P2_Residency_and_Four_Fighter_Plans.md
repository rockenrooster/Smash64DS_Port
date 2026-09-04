# Independent review: P2 residency and four-fighter architecture

**Date:** 2026-09-04  
**Repository inspected:** `rockenrooster/Smash64DS_Port`, `master` at `4f6beb07f90cda7a4ce7663fc5b6594360f4af2a`  
**Disposition:** approve the architectural direction; revise the proof claims, accounting, and several implementation instructions before treating these documents as an executable plan.

## Overall verdict

**These are useful reviews, not a set of proved solutions. I would keep their central architecture rather than start over. I would not hand all four to an implementation agent unchanged.**

The strongest shared conclusions are: do not introduce mandatory gameplay-time storage faults; replace redundant source/native representations with scene-specific generated data; admit required resources before gameplay; retain cheap direct runtime access; and repair fallback-contaminated performance measurements before making simulation concessions. Those decisions reinforce each other. [A1–A4]

The weaknesses are more specific. Some proposed proof mechanisms are described as though they already establish completeness. A conditional memory calculation is promoted too readily into a fixed pack ceiling. The four-fighter memo gives a collision ordering that is **not the source ordering**. The VRAM review does not sufficiently specify how a host placement certificate will be realized by the actual allocator. And the documents disagree about whether all post-`GO` reads are prohibited or BGM streaming remains permitted.

| Document | My verdict | What should change before implementation |
|---|---|---|
| `Design_DS_fighter_paging.md` | **Keep as the original direction-setting memo; supersede parts of its task list.** | Remove any impression that 512 KiB is a demonstrated recovery budget, that finite poisoning proves completeness, or that Kirby's model alone closes the hole. |
| `Review_Deriving_Fighter_Live_After_Setup_Set(1).md` | **Best basis for the main-RAM work, with important safety and accounting amendments.** | Separate memory safety, semantic completeness, and size proof. Resolve the actual late consumers. Treat the size ceiling as conditional. Permit bounded estimation before a complete compiler exists. |
| `Review_DS_Texture_VRAM_Residency(1).md` | **Strongest policy recommendation of the four.** | Specify allocator/certificate correspondence, solver `UNKNOWN`, mutable-resource aliasing, and the complete legal configuration domain. |
| `4Fighter_optimization.md` | **Good diagnosis and prioritization; not yet a safe scheduling specification.** | Remove the invented six-pair source order, qualify the percentile arithmetic, preserve temporal transform semantics, and avoid unconditional material sorting or fixed-rate audio assumptions. |

**Most important correction:** `ftMainSearchHitFighter` performs a directed search for each victim, walking the fighter GObj linked list. It also uses whether the traversal has passed the victim itself to control attack-clash processing. The memo's proposed unordered slot sequence `(0,1), (0,2), …, (2,3)` is not an equivalent description of that algorithm. [R1]

### Evidence boundary

I read the four supplied files and checked selected implementation seams through GitHub. The supplied files are the documents under review; repository code is used to check their claims, not silently replace their contents. `master` is the newest of the three inspected branch heads, despite the project's older `runtime2` convention. [R0]

This review does **not** contain a newly built ROM, a new emulator profile, an exhaustive asset census, or a measured proof that all twelve fighters fit in every legal four-player configuration. Historical timing and memory figures below remain the earlier reviews' or repository reports' figures unless explicitly identified as arithmetic. Upstream libnds source is identified separately; its behavior must be matched to the project's linked version before implementation.

---

# 1. `Design_DS_fighter_paging.md`

## 1.1 Keep the rejection of mandatory gameplay paging

This is the right decision for the stated project. The memo identifies the relevant deadline: data selectable by an input, collision, status transition, or spawned object can be needed without useful prefetch notice. Loading only the bytes touched by the current pose is not a safe working-set definition. [A1]

The recommendation to generate the final representation offline is also right. Loading a full source closure, constructing its replacement beside it, and then reclaiming the original can fail at the temporary peak even when the final representation is smaller. The required accounting is peak lifetime overlap, not just the final pack length.

Keep the separation between immutable kind data and mutable instance state. Mirrors may share immutable data, but their animation cursors, current poses, damage state, material instances, and other mutations must remain independent. Sharing is a semantic property, not merely equality of initial bytes.

## 1.2 The 512 KiB target is an aspiration, not a feasibility result

The memo correctly computes, from its supplied baseline:

```text
404,336 additional closure bytes
-66,144 existing free bytes
+32,768 required free floor
=370,960 bytes of minimum recovery
```

Its subsequent recommendation to target at least 512 KiB is a reasonable risk allowance, but it is not derived from a complete allocation ledger. Nor does the N64's ability to run the original game prove that this particular DS representation, renderer, audio system, and linked image fit unchanged. [A1]

**Amendment:** retain “any four” as the engineering requirement, but replace the implied assurance with:

> The existing measurements do not establish infeasibility. Pursue generated scene-resident representations and measure the complete candidate memory map. A successful capacity certificate and runtime peak measurement—not the N64 analogy—establish that the selected design fits.

An N64 memory ledger could help explain overhead. It should not become a blocking prerequisite to measuring the DS allocation classes that already exist.

## 1.3 Kirby remains a good falsifier, not the capacity proof

The later live-set review correctly supersedes the model-only implication. Under that review's accounting assumptions:

```text
Worst-four main closures                  577,424 B
Minus every raw model, at zero replacement cost
                                        -271,040 B
Remaining                                306,384 B
Provisional optimistic pack allowance     175,604 B
Residual shortage                        130,780 B
```

This is a strong negative result **for model deletion alone under that budget**. It does not say that model replacement plus other recoveries cannot work. [A2]

Kirby is valuable because he stresses cross-owner dependencies, capability selection, model-part changes, and retained defaults. Success on him would validate a substantial part of the method. It would not prove the transformed worst roster, remaining native-owner costs, or motion-bank budget.

## 1.4 Replace “prove zero reads by testing” with a layered argument

Poisoning and watchpoints can discover omitted dependencies. A finite move tour cannot establish that no legal future state will read an abandoned range. The later review recognizes this; the original memo's closing instruction should be updated to agree with it. [A1, A2]

Likewise, strongly connected components are not an eviction proof. An acyclic object can still be pinned by one inbound live pointer. SCC analysis is useful for grouping dependencies, but safe eviction requires accounting for **all persistent roots, inbound references, and their lifetimes**. The original memo partly acknowledges this with “plus all objects that retain pointers”; make that qualification the governing rule.

**Use this memo for the direction. Use the amended live-set review for the safety plan.**

---

# 2. `Review_Deriving_Fighter_Live_After_Setup_Set(1).md`

## 2.1 Its central reframing is correct

The best idea here is to stop asking for the smallest surviving byte subset behind the old file-pointer ABI. Instead, compile a conservative semantic representation and migrate the relevant consumers to it. Preserve complete small domains; concentrate reduction effort on large presentation payloads and redundant representations. [A2]

I also agree with keeping separate profiles for:

- replacement of the current main closure;
- the stronger requirement of no mandatory fighter-asset reads after `GO`, including motion/event banks.

**Only the second profile can close the accepted no-gameplay-demand-loading requirement.** The first is useful diagnosis, not the final capacity verdict.

## 2.2 Resolve two conditional examples against the source now

### Hurtbox defaults are not setup-only

Section 11.2 says damage descriptors can become transient *if* there are no later direct reads. That condition is false in the inspected source. `ftParamResetFighterDamageCollsAll` reads `fp->attr->damage_coll_descs` to restore the live collision records. `ftmain.c` calls the reset from animation-event processing and other reset logic. [R2, R3]

This is not a contradiction of the review's broader recommendation to retain complete `FTAttributes`; that recommendation helps protect these defaults. It is an example that should no longer remain hypothetical.

**Disposition:** retain the authoritative defaults in the resident representation, or replace every reset consumer with an equivalent resident default table. Copying mutable current hurtboxes at creation is insufficient: those current records can subsequently be modified.

The general rule is:

> A construction input is not setup-only when an object can be reconstructed or reset during gameplay.

Apply that rule to respawn, model-part reconstruction, dynamically created effects/weapons, capture/throw setup, and every late constructor—not just initial fighter creation.

### Low-detail mode is not the same as “drop every high-detail source atom”

`ftParamSetModelPartID`, `ftParamResetModelPartAll`, and `ftParamInitAllParts` select common-part detail 0 when either the fighter is high detail **or the corresponding low-detail display-list entry is null**. Therefore a fighter with `detail_curr == Low` can legitimately use high-detail common-part data. [R2]

The generator must compile the **effective low-detail selection**, including this fallback. A verified low-detail-only native owner can still be sufficient; it must already contain every resource selected by the effective rule. Do not filter source atoms by their high/low label alone.

The existing review asks for a detail invariant. Keep it, but strengthen it:

```text
all runtime detail selectors obey the declared policy
AND
all fallback selections made under that policy are represented
```

## 2.3 A typed pack is not, by itself, a completeness proof

The review correctly proposes several proof obligations, but its strongest wording sometimes collapses three distinct questions:

| Question | What answers it |
|---|---|
| Is this reference legal? | Bounds, alignment, type, capability and lifetime validation. |
| Does the pack contain every value the runtime can legally request? | Complete roots, selector domains, dependency rules, and consumer migration. |
| Do the packed values preserve the intended source behavior? | Correct translators plus structural and differential verification. |

A pointer can be in bounds, correctly aligned, and point to the wrong legal model part. A manifest can classify every object it knows about while omitting an entire class of consumers. A linker audit can pass while a `void *` or integer-derived address still reaches legacy data.

These are not reasons to abandon the method. They are reasons to define its trusted boundary honestly.

**Amendment:** the assurance case should identify the trusted components explicitly: source-object reconstruction, semantic schemas, consumer inventory, translator, binder, replacement runtime seams, and selected build configuration. The tools enforce that declared model. Their coverage must itself be reviewed.

“No unclassified consumers” is a useful gate only when the method that discovers consumers covers the actual preprocessed/imported program, aliases, callbacks, assembly where relevant, and address-producing helpers. Searching known root names is not equivalent to that coverage.

## 2.4 Tighten `CONSERVATIVE_OPAQUE`

The suggested allowance for retaining a small unknown object whole needs a restriction. **Small size does not make unknown pointer semantics safe.** A retained 32-byte object could contain eight addresses into discarded files. Relocating the explicit pointers would still not explain hidden selector or computed-offset behavior. [A2]

Permit opaque retention only when at least one of these is established:

1. It is pointer-free scalar/byte data with a proven extent and interpretation at its consumers.
2. Its pointer-bearing fields, computed references, and dependencies are nevertheless completely described.

Otherwise it remains `UNCLASSIFIED`, regardless of byte count. An opaque record is not a license to preserve old addresses unchanged.

## 2.5 Make binder validation object-bounded and overflow-safe

The sample condition `offset + minimum_target_size <= section_size` is a sketch, not safe implementation code. Unsigned addition can wrap. A section-valid pointer can also target the wrong object or an illegal interior position. [A2]

Use checks shaped like:

```c
/* Validate before doing pointer arithmetic or multiplication. */
if (offset > section_bytes) return PACK_BAD_RANGE;
if (extent > section_bytes - offset) return PACK_BAD_RANGE;
if (stride == 0 || count > extent / stride) return PACK_BAD_ARRAY;
```

Then validate target atom identity/type, permitted interior addends, alignment, reference kind, counts, and capability predicates. Decode serialized fields explicitly; do not rely on host `sizeof`, host endianness, or unchecked casts of packed bytes to native structures.

A pointer-domain audit at the barrier is valuable. It proves only the inspected state at that barrier. It must be complemented by the rule that **later constructors and resets can obtain references only through approved resident roots**. Otherwise a perfectly clean initial graph can acquire an invalid reference later.

## 2.6 Keep the capacity arithmetic, but repair the accounting contract

The arithmetic producing 175,604 bytes is correct for the quantities and assumptions stated:

```text
72,148 + 173,088 - 36,864 - 32,768 = 175,604
```

It is not yet a universal or same-build pack limit. The review itself leaves additional growth and binder cost unknown. It also charges replacement native-owner data to `W`, which requires an explicit treatment of any old native-owner allocation already present in the baseline. [A2, R4]

The safest ledger is:

```text
candidate free at a specified phase
  = baseline free at that phase
  + baseline allocations actually removed
  - new resident allocations
  - other live-allocation deltas
  - net reduction in arena capacity
```

Every allocation must appear exactly once. In particular:

- Credit old native-owner bytes only if the candidate actually removes them.
- Charge the replacement owner once, whether embedded in the pack or loaded separately.
- Charge a larger motion bank against the old motion reservation it replaces, not alongside a reservation that no longer exists.
- Count linked data moved into an arena as a placement/lifetime change, not automatic total-RAM recovery.
- Do not add a battle-specific saving to an undifferentiated minimum across multiple scenes without checking which scene and phase produced the minimum.
- Check setup/upload/fixup overlap as well as the post-barrier resident state.

I am **not claiming a proven double count in the existing 175,604 figure**. I am saying its allocation correspondence is not complete enough to be the final gate. Retain it as a provisional screening calculation until a same-configuration ledger replaces it.

The later repository binding already says the provisional target must be replaced by a measured threshold. That caution should govern every subsequent use of the number. [R4]

## 2.7 “793 sets” is one axis, not the complete product domain

The combinatorics are correct:

```text
C(12,1) + C(12,2) + C(12,3) + C(12,4) = 793
```

But a set of unique kinds does not specify instance multiplicity, costume/shade combinations, two-player high detail versus four-player low detail, item/copy capabilities, or scene requirements. Four mirrors share immutable kind data but still require four instances and potentially several palette/material states. [A1, A2]

Enumerate or conservatively dominate the complete legal configurations, then memoize equivalent resource signatures. The certificate key must include every input that can change required bytes or semantics.

Also, the raw-closure argmax is not necessarily the compact-pack argmax. Kirby, Yoshi, Fox and Link are a useful stress set; **recompute the worst witness after transformation** rather than assuming it remains the same.

## 2.8 Avoid making the estimator require the entire finished solution

The proposed census is valuable, but “fully classify every consumer for all twelve before learning whether this is promising” risks becoming the whole porting project under another name.

I would use three kinds of estimate:

| Result | Meaning |
|---|---|
| Lower bound exceeds the budget | Reject that representation/budget combination now. |
| A defensible conservative representation fits | Capacity is promising; semantic qualification is still separate. |
| The bounds straddle the budget or dependencies are unresolved | `UNKNOWN`, with the exact uncertainty named. |

An upper bound is defensible only if it includes all known dependencies and replacement costs; “unknown means zero” is not an upper bound. Conversely, unresolved semantics need not prevent publishing an explicitly optimistic lower bound that already proves a design cannot fit.

After the census identifies the largest contributors, a **small, bounded vertical prototype** of one representation and binder seam is justified to measure real overhead. This is not permission to integrate all twelve into an unpriced ABI. It prevents months of tooling from depending on an untested estimate of runtime shape.

---

# 3. `Review_DS_Texture_VRAM_Residency(1).md`

## 3.1 Approve the policy

I agree with retiring the hard-coded static/dynamic partition as the admission policy while preserving fixed arrays and direct addressing as implementation machinery. I also agree that a general gameplay LRU does not solve an oversized simultaneous required set. [A3]

Separating texel storage, palette storage, and texture views is particularly important. So are exact required atlas membership, pre-game admission, stable handles, and preventing a leaf texture failure from demoting an entire native stage.

The review is appropriately cautious about the existing reject: outer reason 6 does not prove software-slot exhaustion. Keep that caution. **Do not report the diagnosis as established until the inner failure is captured.**

## 3.2 Change the solver requirement from “exact solver” to “verified successful placement”

A heuristic that finds a legal placement has established feasibility once an independent checker verifies the certificate. It does not need to prove optimality. An exact solver is useful when a heuristic fails, but it is not required for every successful plan. [A3]

Use these statuses:

```text
PASS        a complete placement certificate verifies
INFEASIBLE  a valid lower bound or complete search proves no legal placement
UNKNOWN     search limit, heuristic failure, or unsupported constraint
```

A bounded search is not a proof of infeasibility merely because it stopped. `UNKNOWN` must not be mislabeled as “does not fit.” For release admission, both `UNKNOWN` and `INFEASIBLE` block the unsupported configuration, but their engineering remedies differ.

**Recommended implementation:** fast deterministic placement first, host-side solver/search when necessary, simple independent certificate validation always. Do not implement CP-SAT or an elaborate general packing search on the DS.

## 3.3 Specify who owns actual VRAM placement

The proposed load sequence says to create names, upload, and verify actual addresses against the certificate. The missing implementation contract is: **how does the allocation API guarantee those addresses?** [A3]

In the inspected upstream libnds implementation, `glTexImage2D` chooses allocation locations internally. Palette allocation is also internal. Supplying a generated upload order does not automatically realize an arbitrary externally solved placement. [R5]

Choose one authority:

**Allocator-compatible certificate.** Pin the allocator version and initial state, model its actual allocation sequence, reserve dedicated owners consistently, and check that runtime allocation reproduces the certificate.

**Explicit placement backend.** Own the planned texture/palette regions and bind verified hardware state through a narrow backend. Do not simultaneously let another allocator believe it owns the same free space.

Either can work. The first may be a smaller migration; the second offers tighter placement control. What is unsafe is having a solver and an allocator make independent placement decisions and hoping their results coincide.

## 3.4 The 384 KiB experiment has a compressed-format caveat

The recommendation to investigate a battle-only additional texture bank is reasonable. The review also correctly says that C/D are compositor surfaces, not free memory. [A3, R6]

However, do not translate “128 KiB more texture VRAM” into “128 KiB more capacity for every texture format.” The inspected libnds compressed-texture path explicitly searches auxiliary storage in B and corresponding texel regions in A/C. A default A/B/D texture arrangement does not simply add another compressed-texel region through that path. [R5]

It can still help substantially: ordinary textures moved to D may release A/B space. But compare bank profiles using the actual format mix and implemented mapping rules, not only their nominal total byte counts. Any non-default bank-to-slot mapping also needs agreement with the address encoding in the binding backend.

This is an implementation qualification, not a reason to reject the 384 KiB experiment.

## 3.5 Restrict automatic deduplication to compatible lifetimes and mutations

Exact identical immutable texel or palette bytes are excellent deduplication candidates. Two mutable palettes with identical initial bytes are not automatically shareable if separate fighters can animate them independently. [A3]

Make the sharing key include the relevant semantics:

```text
format and interpretation
validated extent
immutable content identity OR mutable-owner/update identity
residency lifetime
allowed view interpretations
```

Keep per-instance animation/material cursors outside shared immutable storage. For mutable storage, require proof of a shared update schedule or allocate separate instances.

The review already names mutability; make it operational in deduplication and admission rather than treating it as descriptive metadata.

## 3.6 Reconcile the residency lock with declared mutable surfaces

The review both prohibits post-`GO` uploads and permits fixed-footprint mutable texture updates. That can be consistent, but the counters must encode the distinction. [A3]

A workable contract is:

```text
post_GO_new_required_residency = 0
post_GO_required_eviction = 0
post_GO_storage_demand_faults = 0
undeclared_texture_or_palette_mutations = 0

for each declared mutable surface:
    fixed allocation and handle
    bounded update extent and frequency
    all source data already resident
    measured update cost
    safe ordering relative to GPU/DMA use
```

A stable handle does not protect pixels from being overwritten while an earlier submission still needs them. Publication and reuse must respect in-flight graphics work. The same ownership rule applies when reclaiming source upload buffers used by asynchronous DMA.

The default can remain fully immutable required textures. The exception must be named and priced, not silently hidden behind “it does not allocate.”

## 3.7 Reachable closure is an assurance obligation, not automatic knowledge

“All reachable textures are knowable” is a design goal under a closed set of supported consumers and selector domains. A generated list does not independently prove that those domains are complete. Particle opcodes, material scripts, part switches, costume selection, and future constructors need the same fail-closed treatment as the semantic pack. [A2, A3]

For atlas variants, use common fragments and memoized resource signatures rather than immediately materializing every cross-product. A superset plan can certify several configurations when monotonicity is established. An “items-on” superset may cover subsets, for example, but only when the generator's format and atlas choices preserve that implication.

The important output is a checked plan for every promised configuration—not a mandated number of files, one particular solver, or one plan per raw slot tuple.

## 3.8 Keep mandatory failure and local optional degradation distinct

This distinction is one of the review's strengths. Missing required content should fail admission. An explicitly optional cosmetic may use a declared fallback or be omitted without invalidating unrelated owners. [A3]

Do not let “local degradation” become permission to omit an unplanned required move effect. And do not implement an emergency generic-stage fallback after partial native submission. The owner preflight must precede that owner's first graphics commands.

**My approval is of this policy, not a claim that the complete P2 texture union already fits.** The report still needs the actual generated witnesses.

---

# 4. `4Fighter_optimization.md`

## 4.1 Keep the baseline diagnosis; remove implied speed guarantees

The memo's strongest advice is to stop selecting an architecture from a profile contaminated by native-stage rejection and animation-cache rejection. Its distinction between accounting buckets, correlation, and causal A/B savings is also correct. [A4]

But the historical figures are not a fresh performance result. Removing a pathological fallback may remove a very large cost; it does not establish the remaining four-fighter P95, nor guarantee that a particular schedule supplies the rest.

Keep independent gates for native-path engagement, mandatory demand loads, and frame timing. A profile is useful only when the intended paths actually ran.

## 4.2 Qualify the opening Amdahl argument

The calculation that a halved subsystem would need to occupy 85.9% of a particular 1,963,648-tick frame to save 843,268 ticks is valid **for that frame under additive costs**. It is not enough to combine that frame's total with a renderer percentage derived from another population and declare a percentile improvement mathematically impossible. [A4]

The memo later gives the right remedy: retain the per-frame series, apply the hypothetical reduction per frame, and rerank. Move that qualification up to the original argument.

```text
hypothetical_frame[i] = measured_total[i] - removable_lane[i]
new_percentile = percentile(hypothetical_frame)
```

This is a screening counterfactual, not a measured optimization: changed cache behavior, dispatch, synchronization and code layout can make the real result different. Follow it with an engaged, same-binary route comparison where practical.

Inspect more than only the original top 80 frames. Improvements can cause previously cheaper frames to become the new percentile witness.

## 4.3 Delete the proposed “canonical” six-pair sequence

**This is the clearest source-level correction in the four files.**

The source algorithm starts with a victim `this_gobj`, walks `gGCCommonLinks[nGCCommonLinkIDFighter]`, and tests attacks from other fighters against that victim. Passing the victim during traversal sets `is_check_self`, which participates in the attack-versus-attack branch. It also consults capture/throw/team state and interaction records along the way. [R1]

Four fighters have six unordered pairs, but that does not mean the source performs six interchangeable symmetric operations. Directed victim/attacker visits can number twelve before rejection; clash de-duplication and hurt/shield resolution have different rules.

Replace the memo's proposed loop with:

> Preserve the existing per-victim process order, live GObj traversal order, direction of each test, attack/hurtbox order, interaction-record updates, and clash de-duplication rules. Insert conservative rejection only at a point proved to have no skipped side effects.

Do not assume GObj order equals player-slot order, particularly across destruction and recreation. A broadphase mask may be generated separately, but it must be consumed at the original semantic positions.

There is also existing project evidence against treating a new broadphase as an unexamined opportunity. An August 13 analysis found extensive existing early-outs in its two-fighter workload and rejected that candidate at the time. That does not settle a new four-fighter workload, but it means the proposal needs a **new measured witness**, not a generic optimization checklist. [R7]

## 4.4 Two slim substeps are a sensible candidate, not an established cheap kernel

Preserving two ordered 60 Hz combat boundaries is a safer starting point than replacing them with one naïve `dt = 2` update. The memo correctly discusses input edges, counters, hitlag, event timing, physics, and shared RNG consumption. [A4]

The missing proof is which work can actually leave those boundaries. A subsystem called “visual” may consume the shared RNG, mutate a model part used by another process, allocate from an order-sensitive pool, or publish an attachment transform needed by a weapon.

For each decimation candidate, record:

```text
read set
write set
RNG behavior
spawn/destruction behavior
source execution position
consumers of its outputs
accepted behavioral or visual difference, if any
```

A load-time-generated process vector also needs a runtime maintenance policy for process creation, removal, pausing, reprioritization, and same-tick execution eligibility. A static snapshot is not a replacement for a changing object/process graph.

A lower-risk sequence is to preserve the scheduler first, split one audited presentation-only process at a time, and move dispatch to dense vectors only after equivalent behavior and meaningful dispatch cost are demonstrated.

## 4.5 Share transforms by version and meaning, not just by joint ID

A gameplay-joint subset is promising. It must include ancestors and every procedural/attachment dependency needed to reconstruct the requested transforms. A list of hitbox joint IDs alone is not sufficient. [A4]

More importantly, “build each world transform once” needs a validity key. Depending on the consumer, the relevant value can be the prior collision sample, current combat substep, post-physics root, post-status-change pose, or camera-dependent render transform.

Use the principle:

```text
same fighter instance
+ same semantic pose/root version
+ same coordinate space
+ same dependency versions
=> eligible for transform reuse
```

If an earlier process changes a dependency, the cached result must be invalidated at that semantic boundary—even inside one substep. Render-only billboard or model-view transformations should not leak into gameplay collision results. Likewise, final-substep transforms cannot retroactively replace the first substep's collision geometry.

Keep previous/current collision samples wherever the existing source algorithm needs them. This is a temporal-correctness problem as much as an arithmetic optimization.

## 4.6 Material batching is not unconditionally order-preserving

The suggested compact effect records and prepared packets are good representation ideas. However, “bucket by atlas/material” can reorder draws. That is not automatically safe for translucent effects, overlapping particles, or other order-dependent rendering. [A4]

Batch adjacent compatible draws, or prove the particular class can be reordered without changing depth/blend semantics. Do not use fewer submissions as the acceptance criterion by itself.

Similarly, the sample `NDSFxInstance` is an illustration, not a sufficient schema for every effect. Attachment state, alpha/material animation, parent lifetime, and gameplay-visible events must survive somewhere.

## 4.7 Audio service is deadline-driven, not simply a 30 Hz presentation task

The current inspected code has a timer/mailbox worker handling playback seams, while `ndsAudioBgmUpdate()` calls the packet-refill service. A responsive seam worker does not mean storage refills happen independently of the main loop. [R8]

Thus “audio service at 30 Hz” should be a provisional scheduling choice backed by buffer/deadline measurements, not a general architectural law. Keep audio deadlines, storage arbitration and worst non-preemptible read duration explicit. Do not tie sound-event timing to the visual update rate merely because rendering is 30 Hz.

## 4.8 Differential tests need canonical state, not raw memory hashes

The proposed source-versus-candidate suite is valuable. Serialize semantic fields using stable object IDs; exclude pointer values, allocator addresses, padding, and uninitialized bytes. Otherwise relocation and different allocation order create meaningless mismatches.

For the first pack/representation migration, avoid simultaneously changing physics precision or scheduler semantics. Establish equivalence with the same arithmetic first. Later numerical or rate changes need their own stated tolerance and event policy. A small coordinate tolerance alone does not guarantee identical later collision outcomes.

**My conclusion:** preserve the two-substep option as the preferred hypothesis, but authorize it incrementally. Do not describe the expected reduction as already established.

---

# 5. Amendments shared by all four documents

## 5.1 One legal configuration, separate resource ledgers

The main-RAM pack, VRAM plan, and storage bundles should share a configuration identity and canonical content IDs. They should **not** become one universal allocator.

```text
configuration / semantic capability set
                 |
       canonical content identities
          /          |          \
    RAM layout   VRAM plan   storage bundle layout
          \          |          /
       validated scene-admission record
```

Each ledger answers a different question: which semantics exist, where live bytes fit, and how bytes reach their destinations. Moving a texture out of main RAM is not a completed saving until the VRAM plan admits it and the upload source can actually be released.

The shared record should identify selected kinds and multiplicities, costumes/shades, detail policy, stage/mode, item/copy capabilities, representation versions, asset hashes, RAM layout, VRAM profile, storage bundle version, and allowed mutable resources.

## 5.2 Unify the post-`GO` I/O law

The phrases “any NitroFS/DLDI read after GO” and `asset_reads_after_go == 0` are too broad for a project that intentionally streams BGM. The current BGM implementation performs packet reads during playback. [A1, A4, R8]

Use resource-classed telemetry:

```text
mandatory_battle_demand_reads_after_GO == 0
mandatory_motion_demand_reads_after_GO == 0
mandatory_texture_demand_reads_after_GO == 0
undeclared_storage_clients == 0

BGM stream:
    admitted bandwidth and buffer residency
    named service deadlines
    bounded interference from other clients
    no seam misses / underruns in the acceptance workload
```

Required one-shot gameplay cues must have their own resident or demonstrably deadline-safe policy. They should not inherit the BGM exception accidentally.

## 5.3 Logical atom deduplication does not require a single physical ROM copy

Canonical atoms and RAM union are excellent recommendations. The categorical prohibition on duplicated payload packs is stronger than necessary. [A2]

For this project, ROM size is explicitly cheap and the storage path is under investigation. A generator may duplicate a payload into a scene/fighter loading bundle to reduce seeks while retaining one canonical identity for validation and RAM deduplication.

For scale only, 495 copies of a 175,604-byte block would be 86,923,980 bytes, before costumes, stages, motion data, alignment and other content. That is **not** a proposal to emit all combinations, and it is not a fit estimate. It illustrates why physical duplication should be an empirical ROM-versus-I/O tradeoff, not prohibited on principle.

My preference remains canonical logical atoms plus a modest number of contiguous loading bundles. Do not turn every tiny semantic atom into an individual NitroFS file. An archive index removes repeated path lookup but does not by itself guarantee bounded underlying FAT seeks; the storage review's measurement requirement still applies. [R9]

## 5.4 A known-safe plan can conservatively over-approximate demand

None of these systems needs to prove the mathematically smallest possible working set. An intentionally conservative complete set that fits is enough.

Conversely, an exact count produced from incomplete roots is not a proof. Distinguish:

```text
coverage proof       are required consumers/capabilities represented?
placement proof      do the chosen representations fit legally?
scheduling proof     do required operations meet their deadlines?
```

A certificate should report all three independently. `PASS` on placement cannot compensate for `UNKNOWN` on coverage.

---

# 6. What I would authorize next

## Step 1: correct the contracts before implementing a new scheduler

Amend the collision-order instruction; retain hurtbox defaults; resolve effective-low-detail fallback; scope the no-I/O law; and label historical versus newly measured budgets explicitly. These are small changes that prevent expensive wrong implementations.

## Step 2: repair and expose the current failure paths

Capture the inner texture/native-stage failure, preserve owner-local validity, and classify the CSS storage stall separately. Keep counters for actual mandatory reads and fallback engagement. Do not price a replacement scheduler on a run dominated by broken residency paths.

## Step 3: produce a cross-checked capacity ledger and bounded estimator

Measure a specified four-slot configuration and phase. Record baseline allocations removed, candidate allocations added, static-image changes, instance growth, animation reservations, native owners, and setup peaks. Generate conservative semantic object estimates and exact texture/palette membership. Publish lower bounds, conservative estimates and unresolved terms separately.

## Step 4: implement one narrowly qualified representation slice

Select a high-value model/material/descriptor slice with complete roots and consumers. Prove its binder, late resets, and scene teardown. Measure actual replacement bytes and loader overhead. Then recompute the worst configuration rather than extrapolating a favorable single-fighter percentage.

## Step 5: commit compatible RAM, VRAM and storage plans

Implement the selected VRAM allocation authority and certificate replay. Load contiguous storage bundles into final destinations; verify references; release staging only after its users finish; lock mandatory residency. Keep CSS and Results as separate scene representations instead of dragging gameplay closures through menus.

## Step 6: re-profile, then change execution rate incrementally

Measure the intended four-fighter fast path with the actual final resident data. Finish packetization and audited transform reuse. Remove one proven presentation-only operation from one combat substep, validate source event order, and measure again. A fully restructured process schedule comes after those results—not before them.

### Minimum combined acceptance record

```text
Build/configuration:
    exact legal configuration identified
    source/schema/asset hashes agree
    no unresolved required consumer or dependency
    transformed worst-resource witnesses published

Memory:
    RAM resident and transient peaks fit their phase budgets
    VRAM placement certificate matches the allocation backend
    mutable sharing and lifetimes validated
    required safety floor met on the accepted configuration

Correctness:
    required selectors and late constructors remain valid
    no stale roots across reset/teardown/reentry
    source-directed interaction order preserved
    canonical differential state checks pass

Runtime:
    no mandatory demand loading after GO
    no undeclared residency mutation
    BGM/other approved streams meet their deadlines
    intended native paths engaged
    frame-time/cadence gates measured separately
```

---

# Final judgment

**Keep the architecture. Tighten the claims. Shorten the path from hypothesis to measurement.**

The main-RAM and VRAM reviews are directionally compatible and worth building on. The older paging memo should no longer govern sizing or completeness. The four-fighter optimization memo contains a good strategy but needs a source-exact execution contract before implementation.

The most useful contribution of these documents is replacing reactive cache behavior with explicit scene admission. Their greatest risk is making that sensible architecture sound like a completed proof, then asking an agent to implement every proposed subsystem at once.

I would move forward with **generated scene-resident data, checked placement, direct runtime bindings, and two source-ordered combat boundaries as the current baseline**. I would not claim yet that the final pack fits, that 256/384 KiB is the required VRAM profile, or that two slim substeps meet 30 FPS. Those are the next measurements and certificates to produce—not reasons to reject the approach.

---

## Sources and audit references

### Supplied documents: the material under review

**[A1]** `Design_DS_fighter_paging.md`, supplied attachment. Relevant locations: lines 18–34, hole/512 KiB target; around line 169, SCC discussion; closing recommendation around line 300. Line numbers refer to the supplied file, not a reformatted repository copy.

**[A2]** `Review_Deriving_Fighter_Live_After_Setup_Set(1).md`, supplied attachment. Relevant locations: opening decision and budget table; §3.3 opaque retention; §5 proof obligations; §6 capacity ledger; §9 implementation sequence; §11.2 damage defaults. Notable lines: 335, 695, 826–926, 1360.

**[A3]** `Review_DS_Texture_VRAM_Residency(1).md`, supplied attachment. Relevant locations: Decision; Proposed architecture; Mechanical admission; Local degradation; VRAM-bank ceiling; Acceptance gates. Notable lines: 354–374, allocation/lock; 397–407, solver requirement.

**[A4]** `4Fighter_optimization.md`, supplied attachment. Relevant locations: lines 65–90, single-lever arithmetic; 230–315, joints/substeps; 437 onward, RNG/order; 471 onward, six-pair sequence; 545 onward, measurement and implementation plan.

### Repository implementation and project evidence

**[R0]** [Branch heads](https://api.github.com/repos/rockenrooster/Smash64DS_Port/branches?per_page=100), checked for this review. Inspected `master`: `4f6beb07f90cda7a4ce7663fc5b6594360f4af2a`. The earlier branch inspection records `codex/r2-runtime2` at `3e95ab113814603840d9a9009c1b82469892674a` and `p2-pikachu` at `ad4b17c5fd15b17c344e5e864b1c3aef8c7bd039`. Search excerpts sometimes still point to `ee5985…`; decisive source findings here use the pinned `4f6beb…` file reads.

**[R1]** [`decomp/.../ft/ftmain.c`, directed fighter-hit search](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/decomp/BattleShip-main/decomp/src/ft/ftmain.c#L2998-L3210). `ftMainSearchHitFighter`, linked traversal, `is_check_self`, early-outs, clash and hurt/shield branches.

**[R2]** [`decomp/.../ft/ftparam.c`, resets and model-part selection](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/decomp/BattleShip-main/decomp/src/ft/ftparam.c#L690-L1020). `ftParamResetFighterDamageCollsAll`, `ftParamSetModelPartID`, `ftParamResetModelPartAll`, `ftParamInitAllParts`.

**[R3]** [`decomp/.../ft/ftmain.c`, late damage-collision resets](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/decomp/BattleShip-main/decomp/src/ft/ftmain.c). Search for `ftParamResetFighterDamageCollsAll` and `nFTMotionEventResetDamageCollPartAll`. Call-site evidence was also returned by GitHub search at the review's earlier `ee5985…` revision; the reset implementation itself was checked directly at `4f6beb…`.

**[R4]** [`docs/p2/P2-2-pack-estimator.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/docs/p2/P2-2-pack-estimator.md), especially its provisional budget and required skeleton-build qualification; [`src/import/battleship_ftmanager.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/import/battleship_ftmanager.c), native-owner ensure and pose-release seams. These pinned files were inspected in the preceding storage review and remain at the same checked repository head.

**[R5]** [Upstream devkitPro libnds `source/arm9/videoGL.c`](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/source/arm9/videoGL.c#L730-L1095), `glColorTableEXT`, `glAssignColorTable`, `glTexImage2D`, compressed-texture A/C+B placement, and texture-address encoding. Inspected source blob: `68a4afff407b1ac9eaa1ba480054a77e57a21fac`. This is API/backend evidence, not proof of the exact library linked into the user's ROM.

**[R6]** [`src/nds/nds_platform.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/nds/nds_platform.c), C/D bitmap configuration; [`docs/p2/P2-texture-residency.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/docs/p2/P2-texture-residency.md), current policy and bank discussion. Mapping evidence agrees with the supplied VRAM review; the policy file was inspected at this same head in the preceding storage review.

**[R7]** [`artifacts/performance/2026-08-13_shdt-broadphase/REFUTED_PAIR_REJECT.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/artifacts/performance/2026-08-13_shdt-broadphase/REFUTED_PAIR_REJECT.md). Historical two-fighter call-count analysis, not a new four-fighter benchmark. Used to qualify the proposal's novelty and expected benefit.

**[R8]** [`src/nds/nds_audio_bgm.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/nds/nds_audio_bgm.c#L640-L724), timer/seam worker; [`ndsAudioBgmUpdate` and refill servicing](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/nds/nds_audio_bgm.c#L1319-L1400); [`include/nds/nds_audio_bgm.h`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/include/nds/nds_audio_bgm.h#L255-L280). These exact pinned implementations were inspected in the preceding storage review.

**[R9]** [Calico backing-ROM reader](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/source/nds/arm9/nitrorom.c), `_nitroromFdReadImpl`; [libdvm 2.1.0 NitroFS wrapper](https://github.com/devkitPro/libdvm/blob/3006b93714b2cac7e052dbc44903b3ef29d2965a/source/nitrofs.c). Inspected in the preceding storage review. These establish why path-count reduction and backing-file seek behavior are separate questions; neither establishes the cause of the reported timeout.

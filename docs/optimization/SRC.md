# Smash64DS — SRC Optimization Research, Candidates, Prototypes, and Evidence

**Research baseline:** September 16, 2026; `rockenrooster/Smash64DS_Port` at
`430aca2879e9071dc2b22f944f5c2909c9ce7aa4`.

**Scope:** SRC first, with its pose, gameplay, collision, animation-storage, and
rendering interfaces. Preserve 60 Hz gameplay, native-only rendering, required
content, all legal four-fighter lineups on all selectable VS stages, and the
fixed-point runtime endpoint.

This single document consolidates all nine files from the SRC candidate package:
the full overview, detailed candidates, source/evidence record, C prototypes,
Python tests, recorded test results, ARM assembly, code-generation report, and
original checksum manifest. No companion download is required to read the material.
Original filenames used in commands identify the embedded blocks to save when
reproducing the standalone experiments.

**Evidence status is unchanged:** these are research candidates and standalone
host experiments, not integrated game optimizations or measured DS speedups.
Consolidating the material did not rerun the experiments, recheck the repository,
modify game code or project documentation, build a ROM, or establish 30 FPS.

## Contents

- [Research conclusions and priorities](#conclusions)
- [I. Full research overview and experiment guide](#research-overview)
- [II. Complete candidate details](#candidate-details)
- [III. Evidence, measurements, and source scope](#evidence)
- [IV. Clickable primary-source reference index](#source-links)
- [Appendix A. Complete C prototypes](#kernels-c)
- [Appendix B. Complete Python differential tests](#test-kernels-py)
- [Appendix C. Recorded host test results](#test-results)
- [Appendix D. Complete generated ARM946E-S assembly](#arm-assembly)
- [Appendix E. Recorded code-generation results](#codegen-results)
- [Appendix F. Original package checksums](#original-checksums)

---

<a id="conclusions"></a>

## Research conclusions and priorities

**The strongest SRC opportunity is replacing the mixed-representation update
pipeline, not repeatedly accelerating isolated helpers.** Start with one bound,
fixed-point pose/transform domain shared by gameplay and rendering; then replace
map-query layers with a bound native collision context, and compile compact
resident motion data into the clip representation.

### What is being targeted

| Candidate | Repeated work to remove | Evidence and limitations |
|---|---|---|
| Bound fixed pose and shared transforms | Fixed-to-float publication, later conversions, repeated hierarchy discovery, and separate transform preparation | Strong source evidence; the three diagnostic pose bodies total about 74.6K mean ticks, not predicted savings |
| Direct instance ownership and compact validity | Pointer-hash collisions, re-flattening, and scattered validity-word clearing | Possible hash conflict demonstrated synthetically; actual conflict rate unmeasured; reported invalidation self-time about 20.7K |
| Compiled/grouped curves | Per-channel reconstruction of identical cubic bases and arguments | Shared-basis arithmetic prototyped; source-corpus grouping remains untested |
| Resident native motion banks | Action-change payload reads, normalization, and track reconstruction | Payload reads remain in the inspected code; removable current tail cost not established |
| Bound native map queries | Layered memo probes, source-format reads, duplicate coordinate forms, and invariant slope/normal work | Inspected floor-query self-cost about 20.9K in the diagnostic profile; not the whole physics/map envelope |
| Demand-driven combat geometry | Detailed transforms and inverse work for interactions conservatively ruled out earlier | Architectural candidate; broad-phase rejection must preserve swept and directed interactions |
| Compact fixed fighter state and phase-correct AI facts | Cold-field and pointer traffic, repeated derived facts | Preserve source update order, decision timing, selected CPU level, and RNG order |
| Compact source-ordered process execution | Process metadata traversal and repeated context setup | Secondary opportunity; the unnamed SRC remainder is not free scheduler overhead |

Cost figures above retain the populations and qualifications in [the evidence
record](#evidence) and [research overview](#research-overview). Candidates overlap;
these costs and their eventual savings must not be added as independent P95 gains.

### Frame-time reality

The pinned clean-baseline report records WORK-H P95 **2,311,616 ticks**. Against
**1,120,380 ticks**, the deficit is **1,191,236 ticks**, approximately **51.53%**.
This is recorded repository evidence, not a reproduced ROM benchmark.

A separate uploaded, explicitly dirty run attributes a mean **608,337 ticks** to
SRC. Its exclusive breakdown appears in [the evidence section](#evidence), along
with the raw population and timer-correction warnings. It must not be mixed with
the clean baseline or treated as a release verdict.

### What the prototypes establish

The shared-basis experiment retained the reconstructed reference's output across
**200,000 synthetic cases**. It reduces the arithmetic count from `9N` to
`5 + 4N` wide products only when tracks truly share phase and reciprocal.
Actual grouping, integration overhead, and DS timing are still open.

The approximate Horner body compiled to **88 bytes**, versus **332 bytes** for the
standalone current-shape reference under the recorded Clang configuration.
The wider synthetic error trial reached **447 Q12 units (0.109130859375)**,
compared with **3 Q12 units** in the modest trial. These are observed maxima over
finite synthetic tests, not universal mathematical error bounds or permission to
change gameplay. Preserve the full error qualifications before adopting it.

The validity-range primitive passed every one of **4,753 intervals** in a 96-bit
domain. Four fabricated 64-byte-spaced addresses produced **400/400 misses** in
the current four-bucket hashing scheme. Neither result establishes live-game
correctness or an actual allocation-conflict frequency.

### Recommended implementation boundary

The first substantial replacement should combine fixed pose publication, direct
per-instance binding, compact validity, and separate bound lists for native pose,
independent joint programs, and active materials. Preserve unusual live joints
such as Samus's grapple; do not equate whole-fighter handling with joint ownership.
Shared cubic bases are the lower-risk first arithmetic experiment within this
larger change. Horner needs stronger source/range qualification.

Map-query specialization must retire existing memo layers rather than add another
cache alongside them. Motion residency must cover required legal states, not just
moves seen in one CPU trace. Source-order-preserving gameplay and scheduler
specialization follow their actual exclusive cost and dependency requirements.

**SRC alone is not established as sufficient to close the entire deficit.** The
other buckets and GPU/FIFO scheduling may also need changes. These candidates
identify concrete remaining work to remove; they are not an integrated 30-FPS claim.

---

<a id="research-overview"></a>

## I. Full research overview and experiment guide

Consolidated from `README.md`; substantive source text retained.

Research date: September 16, 2026 (America/Chicago).
Repository inspected: `rockenrooster/Smash64DS_Port`, commit
`430aca2879e9071dc2b22f944f5c2909c9ce7aa4`.

This is research plus standalone prototypes, not a documentation-policy revision,
new task queue, integrated patch, ROM build, or claim of DS performance improvement.
Keep 60 Hz gameplay, all legal four-fighter lineups/stages, native-only rendering,
required content and fixed-point runtime as the endpoint.

### Findings with the best source support

1. `src/port/reloc_backend_compat_shims.c`, `ndsFTParamsFlatWalkFor`: four
   direct-mapped buckets selected by `(root >> 4) & 3` are not four guaranteed
   fighter-resident entries. Address collisions are possible. Bind by live instance
   and generation; represent subtrees by preorder intervals. Actual collision rate
   is not measured here.
2. The same module invalidates dispersed FTParts validity words repeatedly.
   The repository profile reports 474.5 part-word clears per frame versus 14.3
   matrix recomputes. Compact validity bitplanes avoid fetching dirty cache lines
   simply to clear a flag. All direct readers/writers and special transform modes
   must migrate together; replacing just the invalidator is incorrect.
3. `src/nds/nds_ft_pose.c`, `ndsFtPosePlay`: fixed arithmetic is followed by
   fixed-to-binary32 publication into DObj, then consumed by other representations.
   Replace the complete producer/consumer chain with fixed local pose and bounded
   CPU socket/world-transform storage. The current collision transform cache is
   ALREADY lazy; adding another lazy cache is not the proposal.
4. `ftParamUpdateAnimKeys` calls the pose engine and then scans the indexed joint
   table for unowned joints and materials. Compile separate execution lists at
   bind/topology changes. Samus grapple joint 36 and independent material tracks
   must remain live; pose ownership is per joint, not per fighter.
5. `include/nds/nds_anim_fixed.h`, `ndsR2AnimEvalQ`: a cubic evaluation contains
   nine wide products. Tracks with identical length/reciprocal can share the five
   basis-building products. Alternative precompiled Horner coefficients reduce
   per-track polynomial work to three products, but change staged rounding and
   require content/range admission. Do not change animation event clocks with this
   experiment.
6. `src/port/reloc_backend_mp_collision.c`: endpoints, owners, kinds, extents and
   float vertices are ALREADY cached separately. Native bound line/segment records
   can retire repeated ready checks, layered cache probes, O2R coordinate access
   and per-hit slope/normal preparation. Do not resell adding those existing caches.
   Bind independent geometry contexts instead of swapping a global geometry pointer
   and resetting one cache set around alternate-geometry queries.
7. `src/nds/nds_reloc_assets.c`, `ndsRelocAssetLoadFighterStreamClip`: the directory
   is resident, but non-null destinations still cause `nitroromReadFile` payload
   reads. A compact required motion bank can remove action-change I/O and rebinding
   tails; blindly enabling the older larger battlepack is not the redesign.
8. Source object scheduling has both function and thread processes, ordered queues,
   deferred deletion and same-tick mutation semantics. A compact process schedule
   must preserve these semantics, not run all six phases fighter-by-fighter.
   AI already gates some decisions on `input_wait`; no extra decision decimation.

### Cost interpretation

The pinned September 16 gap report records a clean baseline WORK-H P50/P95 of
1,575,296 / 2,311,616. At the existing exact 1,120,380 gate, the P95 gap is
1,191,236 ticks (51.5326%). This research did not reproduce that ROM.

[`EVIDENCE.md`](#evidence) carries a different, explicitly dirty uploaded run's mean bucket
split, used only for identifying SRC subdomains. Do not mix its measurements
with the clean report or claim independent P95 values add.

The profile `2026-09-16_p2-2p8-n0409-profile` has 129 regions and 487,368,912 ARM9
cycles. Using that report's cycles/(2*regions) convention, the three pose bodies
sum to about 74,630 mean ticks/frame; `ndsF32AddBits` is another 15,592 self ticks
across its callers and `ftParamUpdateAnimKeys` about 12,599 self ticks. These are
cost envelopes, not savings, and shared callers must be attributed before adding.

The corrected collision-matrix family costs about 25,022 ticks, not the older
roughly 49.9K claim; its previous fixed-ring implementation was already a
whole-chain test with low conversion density. A new candidate must remove work
and memory traffic beyond that failed substitution.

### Prototype files

- [`kernels.c`](#kernels-c): current-shape reference cubic, factored basis, approximate Horner,
  and bounded 96-joint validity-range clearing.
- [`test_kernels.py`](#test-kernels-py): deterministic synthetic differential tests and address-hash
  collision counterexample. Standard Python library plus a C compiler.
- [`results.json`](#test-results): actual host test output, not DS benchmarks.
- [`kernels_arm946e.s`](#arm-assembly): Clang 17 ARM946E-S ARM-mode code generation.
- [`codegen.json`](#codegen-results): actual object symbol sizes for this standalone experiment.
- [`EVIDENCE.md`](#evidence): provenance, raw uploaded console evidence and source index.

Build and run on a Linux host with Clang and Python:

```sh
clang -O2 -shared -fPIC kernels.c -o kernels.so
python test_kernels.py
clang --target=arm-none-eabi -mcpu=arm946e-s -marm -O2 \
  -ffreestanding -fno-builtin -ffunction-sections -S kernels.c \
  -o kernels_arm946e.s
```

No game assets, SDK libraries, modified repository files or executable binaries
are distributed in this package. The emitted assembly is inspection material,
not an ABI-qualified replacement for a game function. Rebuild with the project's
actual devkitARM toolchain before evaluating placement or timing.

#### What was actually tested

Factored basis: 200,000 random synthetic evaluations, durations 1..1024 and
phases -2D..+2D, zero arithmetic mismatches with the reconstructed reference.
Grouping itself was NOT derived or tested against the game's clip corpus.

Validity range: all 4,753 intervals in a 96-bit domain, zero bit mismatches.
This tests the bit primitive, not migration of FTParts flags or live topology.

Horner, modest synthetic domain: 150,000 evaluations, endpoint magnitudes <=8
units, rates <=0.25 units/frame, duration <=120; maximum difference 3 Q12 units
(0.000732421875), with 47,258 non-bit-identical outputs.

Horner, wider synthetic domain: 150,000 evaluations, endpoints <=64 units,
rates <=8 units/frame, duration <=1024; maximum difference 447 Q12 units
(0.109130859375), with 145,519 non-bit-identical outputs. This explicitly
DISQUALIFIES a claim that the modest-domain bound holds globally. Both domains
had zero failures at the polynomial's t=0/t=1 endpoints. Source-clock timing,
full source assets, gameplay branch equivalence and extrapolation admission
remain untested for Horner.

Synthetic address counterexample: four 64-byte-spaced root addresses cause
400 misses in 400 cyclic accesses to the current four-bucket hash. A direct
per-instance mapping has four compulsory fills for that fabricated sequence.
This demonstrates a possible failure mode, not actual game allocation patterns.

No host wall-time speed ratio, instruction count, or function-size difference
is presented as a DS timing result.

### Shortlist order

Start with the bound fixed pose/transform domain, including distinct active track
lists, compact validity and instance ownership. The mask/cache change is its
small correctness-oriented first slice, not the finish line. In parallel, use the
actual map-query code to design a bound fixed collision context. Build compact
motion-bank residency as part of the clip representation, not a separate cache
framework. Grouped basis is the lower-risk arithmetic experiment; Horner needs
stronger range/content proof. Compact scheduling and AI fact reuse follow their
actual exclusive costs and mutation contracts.

For every candidate compare the same required visible/semantic workload, whole
frame WORK-H and actual cadence. A faster child that moves time into another
bucket or a FIFO wait has not necessarily sped up the frame.

---

<a id="candidate-details"></a>

## II. Complete candidate details

Consolidated from `CANDIDATE_DETAILS.md`; substantive source text retained.

These are unimplemented game-architecture candidates. See [Evidence](#evidence) for the
pinned source and [the research overview](#research-overview) for the host-only experiments. Existing measured
costs describe current work; they are not projected savings.

### 1. Bind one native pose/transform domain

Replace the chain `compact fixed track -> DObj float -> separate collision and
render transform reconstruction` with `bound clip -> fixed local pose -> demanded
CPU sockets and native draw inputs`. Keep the original update phases; this is a
representation change, not permission to reorder simulation.

At bind, resolve dense joint IDs, parent IDs, subtree intervals, static local
transforms, animation-track output addresses, independently animated materials,
and exception classes (Ncs scale compensation, animation locks, billboards,
translation-scaled fighters and dynamic attachments). Track the topology and
spawn generation independently of the address and fighter kind.

At a logic tick, process authored events at their source time, evaluate gameplay-
required joints and their ancestors, and publish required fixed socket/contact
positions. Visual-only pose follows the already-permitted presentation cadence.
Drawing consumes the applicable pose version; it does not advance animation.
World and inverse transforms are resolved only when demanded and invalidated only
by their actual inputs. The existing collision cache already does lazy evaluation;
this proposal replaces its scattered metadata and duplicated representation.

`ftParamUpdateAnimKeys` should consume three disjoint bound lists: native pose,
remaining independent joint programs, and active material programs. It must not
scan the whole indexed joint array after finishing the pose array just to rediscover
which entries are owned. Samus's extra indexed grapple joint and idle joints with
active materials are explicit correctness fixtures.

A whole-game FTStruct rewrite is not required before the first experiment.
Temporarily publish only fields needed by identified old consumers, at defined
boundaries; never synchronize all old/new fields each frame. Convert those consumers
and delete the bridge before claiming domain completion.

Failure conditions: source event-boundary change, stale capture/throw attachment,
new same-tick read ordering, required collision pose held to rendering frequency,
unqualified numeric error, an added permanent double representation, or total
copy/patch cost erasing the removed work.

### 2. Collision-free identity plus compact transform validity

Current code hashes a root pointer into four entries. The declared four-player
capacity says nothing about hash collisions or alternate roots within a fighter.
Use the existing live slot plus spawn generation and a binding-owned root/subtree
identity. Do not force game allocations to particular addresses.

Use one validity bitplane per independently invalidated property: local matrix,
world matrix, inverse and scale, plus explicit non-Boolean transform-mode semantics.
Preorder descendants form contiguous half-open ranges. Whole-fighter invalidation
is a few word stores; subtree invalidation clears a few masked words. For 96 joints,
four planes occupy 48 bytes per fighter, 192 bytes for four instances. This is only
the validity state, not matrices, topology metadata or ownership. Real capacities
must come from complete source-derived hierarchies rather than clipping to 96.

Migrate direct readers/writers in collision, CPU bounds, rendering and capture/
attachment code together. Keeping old validity bytes as a synchronized mirror
recreates the expensive scattered writes. Preserve root-local versus descendant-
world invalidation, transform mode 2, Ncs scale flags and procedural mutations.

The current report's ~20.7K mean invalidation self-time is an envelope. The
address-collision rate is not measured. The synthetic 400/400 miss example proves
possibility, not actual runtime impact. A same-work test should count actual
flat rebuilds, validity stores and matrix recomputes as well as whole-frame ticks.

### 3. Compile curve work; first share exact bases, then test Horner

The current cubic computes a normalized phase, its square/cube, two derivative
bases and four products with the input values/rates: nine wide products. Identical
phase/duration groups can compute the first five once, leaving four products per
channel: `5 + 4N` versus `9N`. Three channels therefore need 17 rather than 27
wide products; this is an arithmetic count, not an ARM cycle prediction.

Group only equal effective length and reciprocal, including catch-up, attach,
end and reapply semantics. Static generator grouping must be split if runtime
rate changes or independent clocks invalidate it. Retain direct output order if
some writes/events are observable. The synthetic prototype preserves staged
rounding and clamps but not diagnostic saturation-counter frequency; preserve
that instrumentation deliberately when integrating.

For a source Hermite segment of duration D, compile:

```
A = 2(v0-v1) + D(r0+r1)
B = 3(v1-v0) - D(2r0+r1)
C = D*r0
D0 = v0
value(t) = ((A*t+B)*t+C)*t+D0
```

Three multiplies evaluate the polynomial after phase preparation. The prototype
emits 88 bytes for this body under Clang 17 ARM946E-S/O2, versus 332 bytes for
its reference cubic; these are NOT game-function sizes or speed measurements.
The grouping kernel's basis builder plus consumer also totals 332 bytes here;
its potential benefit is amortization across channels, not magically less code.

Horner changes rounding. The tested modest synthetic domain differs by at most
3 Q12 units, but the wider domain reached 447. Validate the actual clip corpus,
interior extrema, derivatives, continuity, terminal/reapply samples and gameplay
consumers. If a coefficient or intermediate cannot fit, use a proven wider native
class or better coefficient scaling, not wraparound, hiding the channel or quietly
changing a move. Do not apply the modest-domain bound to every fighter.

Compile event16 and event32 source programs into native span/control data where
possible. Remove repeated argument expansion and interval interpretation. Do not
replace all content with executable per-frame ARM code: code resident in main RAM
also consumes the small instruction cache. A few tiny kernels plus data are the
leaner default. Optional forward differences need drift bounds and reseeding when
speed/segment changes; do not assume repeated rounded additions equal evaluation.

### 4. Compile required motion residency and status binding

BPS1 directory lookup is already resident and direct range I/O already exists.
Actual payload reads remain on a non-null destination. Eliminate the live payload
read for required motions by preparing compact per-match banks before GO.

Share immutable clip/channel/coefficient data across duplicate fighters; retain
independent cursors and material state. Deduplicate identical channels/streams,
remove unused source structures from the converted domain, and compile default
values once. Coverage includes required action branches, hidden limbs, catch/throw,
entry/death/respawn, copy ability assets, item motions and source-legal siblings.
A cache of moves observed in one CPU trace is not complete residency.

Do not blindly enable the older battlepack or expand every frame to matrices.
For illustration four fighters *32 joints*60 samples*48 bytes = 368640 bytes for
one second. That is larger than the recorded free-heap order of magnitude and
not a viable default with no memory recovery. Coefficient/key data, static-channel
elision and shared clips are better first formats.

Status change should select already-bound clip/control records, apply source
reset masks and update the affected lists/versions. It should not discover
source layouts, normalize pointers, read files and reconstruct all runtime track
objects on each transition. Strongest expected benefit is tail reduction; actual
payload-read/event frequency and blocking time require fresh attribution.

### 5. Bound native map query context, replacing layered memos

Existing endpoints, extents, owner IDs, line kinds and float vertex memos must be
retired, not duplicated. Their capacities and validity probes still leave multiple
representations and O2R reads on a successful floor hit. A bound stage context
holds flat line/segment records, source ordering, adjacent IDs, local bounds,
source flags, static normal/slope data, and owning moving-platform identity.

Each moving owner separately holds current active state and transform/version.
Queries subtract its live translation when the source does; local segment data
then remains invariant. Procedurally changed geometry has its own mutation/version
path. Alternate geometry queries take an explicit context rather than swapping
`gMPCollisionGeometry` and resetting a single global memo set.

Provide narrow typed queries for existing-line distance, swept floor crossing,
wall/ceiling correction and source floor projection. Preserve priority/tie order,
endpoint conventions, source tolerances, pass-through logic, platform velocities,
and grab/ledge behavior. A general BVH is not automatically useful for a stage
with only a few lines; direct compact loops and whole-line rejection may be best.

Compile horizontal/vertical and invariant-segment specializations. Reuse static
normals instead of normalizing a constant slope per hit. Constant reciprocal math
must have a proved operand/error range; general dynamic intersections may still
need a bounded 64-bit hardware divide. Float->fixed->float per query is not closure.
Start with a complete movement-to-collision-to-result fixed chain.

The uploaded mean SPHD+SPHC envelope is ~124K ticks, but it also contains movement,
events and socket updates. Do not assign all of it to map queries. The single
floor-helper PC self-cost in the diagnostic profile is ~20.9K ticks. These are
different scopes and do not add as independent promised savings.

### 6. Demand-driven combat geometry and conservative rejection

Build current active attack/hurt/catch/shield descriptors when their source
state changes. Maintain required old/new attack positions at 60 Hz; current-point
bounds alone can incorrectly reject a swept hit. Start with cheap conservative
candidate rejection before requesting many joint transforms or inverses.

An affine box's aggregate bounds may be obtained with transformed center and
absolute-matrix extents; preserve directed rounding/conservatism. Retain the
source narrow test, interaction direction, hit-group records, shields/reflectors,
team rules and resolution order. Four fighters have six unordered broad pairs,
not six interchangeable directional collision outcomes.

The ordinary hit-search mean is ~45.8K in the uploaded run but P95 is bursty.
This is not by itself the large whole-frame answer. Its leverage increases if it
prevents matrix/pose work rather than merely accelerating the final comparison.
Do not advertise the setup-time eight-corner AI bounds routine as a measured
per-frame hotspot; the inspected call is in fighter creation.

### 7. Fixed hot fighter state and phase-correct fact reuse

Separate co-accessed movement/control/status/eligibility fields from cold fighter
assets and presentation metadata. Use indexed references to shared immutable
attributes and native collision/pose bindings. Pick a compact array-of-structures
or split arrays from actual phase access; neither layout wins universally.

CPU decision routines already skip portions while input_wait is nonzero. Keep
that timing, selected CPU level and random-call order. Reuse derived facts only
under versions that include every producer and the relevant source phase.
Do not freeze a once-per-frame snapshot when later callbacks are meant to see
mutations from an earlier fighter in the same tick. Ground jostle has list-order
and directional tie semantics; a symmetric all-pairs formula is not equivalent.

Move a complete native producer/consumer subset rather than changing every f32
field to a typedef or maintaining two synchronized FTStructs. DTCM admission is
useful only after hot state has been reduced and existing stack/data reservations
are honored. GPU/DMA/ARM7 buffers remain outside CPU-local TCM.

### 8. Compact source-ordered scheduling, after work elimination

The existing scheduler runs object callbacks, then priority-ordered processes,
with paused checks, function/thread distinctions and deferred deletion. Build a
compact active schedule of typed callback/context records only where it preserves
those rules. Update schedule ownership at creation/end/pause instead of scanning
cold metadata to rediscover it on every tick.

Do not replace it with `for(fighter) all_phases(fighter)`; do not swap-remove and
silently reorder same-priority objects; do not give newly created processes the
wrong first execution tick. Keep a complete native route for remaining source
process kinds. Do not rewrite genuine threaded services as if every callback
were an expensive thread wakeup.

The diagnostic self-time of `ndsBaseGcRunAll` and `gcRunGObjProcess` is about
11.1K and 7.1K mean ticks respectively. The much larger unnamed SRC remainder
contains real callbacks and services, NOT ~121K of free scheduler overhead.
Schedule specialization earns priority only with savings beyond these small
wrappers, such as reduced context/pointer traffic in a larger fixed gameplay path.

### CPU offload and ARM instructions

Use build-time computation first. Use ARM long-multiply/accumulate for compact
fixed kernels; keep small cold control code in the ISA that measures best. Do
not inflate all gameplay functions into ARM/O3 to obtain a few optimized leaves.
Use hardware divide/sqrt only when operations remain after algebraic removal;
asynchronous use must not lose the result to another writer or interrupt/thread.

ARM7 offload needs an isolated, bounded input/output job and enough independent
ARM9 work to hide it without stale gameplay. Immediate collision queries and RNG-
ordered AI are poor first targets. Required storage/audio services already use
resources; moving a blocking operation to another CPU does not remove its deadline.
GX visual transforms do not imply that gameplay can get arbitrary matrices back
without serialization. Prefer CPU-native collision/socket transforms plus GX work
that needs no synchronous readback.

### Explicit non-candidates and limits

- Another reduction to 30 Hz gameplay: not permitted.
- Another body-pose hold flag: that saving is already present.
- Enabling hardware divide, packet replay or a resident BPS1 directory again:
  already implemented, not new savings.
- Reviving the fixed collision ring unchanged: previous whole-chain measurements
  already exist; low conversion density means a sandwich diagnosis is insufficient.
- Removing the real lower HUD or subtracting debug HUD twice: invalid.
- Summing independent P95 savings or treating the old shortlist as an architectural
  lower bound: invalid.
- Claiming a prototype's smaller ARM body or synthetic count predicts its DS frame
  savings: invalid.

The SRC redesign must be measured with the original whole-frame timing intact.
Changes to FTR/STG/MISC and GPU/FIFO scheduling may also be necessary to meet the
full goal. This research identifies specific remaining opportunities; it does not
claim an integrated 30 FPS result or universal source-corpus proof.

---

<a id="evidence"></a>

## III. Evidence, measurements, and source scope

Consolidated from `EVIDENCE.md`; substantive source text retained.

### Uploaded bucket record

Source: `e5ad07c6-7313-48a7-822d-8de7f519f471.jsonl`, one-based JSONL record
1384, timestamp `2026-09-16T21:09:56.377Z` (16:09:56 Central).
This is tool-output evidence, not the agent's private reasoning.
The source log SHA-256 is `ba5607955a9efb869699ac9043e95e2363225df07f6ae4c77177b469a69d88b1`.

The output below declares git `9470ffbee78+dirty(22)` and five timer corrections.
It is NOT the same binary as the clean baseline in the later repository report.
The corrections and runtime coverage were not independently requalified here.
Use this table for approximate workload attribution, not a new acceptance verdict.

```text
WARNING: cpuGetTiming() 2^22 timer-overflow artifact detected and CORRECTED on 5 of 1972 samples (one subtraction of 4,194,304 per affected bucket). The run's ALL median after correction is 1,677,952; a corrected value far from it would mean the row was a real stall and the correction wrong, so check these against it:
  frame 137: ALL 6,432,192 -> 2,237,888, 5 bucket(s) corrected
  frame 517: ALL 5,871,936 -> 1,677,632, 3 bucket(s) corrected
  frame 854: ALL 6,992,256 -> 2,797,952, 3 bucket(s) corrected
  frame 1849: ALL 6,992,448 -> 2,798,144, 5 bucket(s) corrected
  frame 1897: ALL 5,871,808 -> 1,677,504, 4 bucket(s) corrected
Wrote D:\Stuff\DevFolder\Smash64DS_Port\artifacts\verification\p2-2-fourcpu-tickhud.csv
Tick-HUD buckets: target=smash64ds-p2-fourcpu-tickhud-hwtri samples=1972 frames=2..1973 melonDS=1.0 sha=DE80E46BDCF1FD98 git=9470ffbee78+dirty(22) dldi=ON

bucket p50        p95        spread mean       min        max        %ALLp50
------ ---        ---        ------ ----       ---        ---        -------
ALL     1,677,952  2,798,144   1.67  1,967,317  1,116,992  6,719,296   100.0
FTR       356,544    743,808   2.09    368,272     16,960  4,598,336    21.2
STG       344,192    388,096   1.13    348,631    328,832  1,251,968    20.5
BG              0          0   0.00          0          0          0     0.0
AUD         3,456    121,536  35.17     13,843      1,664    259,968     0.2
HUD        20,224    444,544  21.98     64,132      5,120    726,272     1.2
SRC       553,984  1,059,712   1.91    608,337    239,552  2,134,272    33.0
MISC      249,408    479,040   1.92    268,913     75,840  1,945,408    14.9
OTHR      284,672    562,048   1.97    295,191     28,352    592,960    17.0
WAIT      254,912    532,288   2.09    266,805        768    560,192    15.2
WORK    1,639,104  2,430,912   1.48  1,700,513    703,744  6,713,920    97.7
SHDT       10,240    216,640  21.16     45,824        512    935,744     0.6
SWRM        1,152      1,216   1.06      1,120        960      1,280     0.1
GCRA      548,480  1,054,208   1.92    602,591    234,368  2,128,448    32.7
SCPU       63,424    165,632   2.61     69,399          0    231,104     3.8
SCAT        1,792      2,624   1.46      3,797      1,024    589,248     0.1
SPRM        3,136     27,776   8.86     10,591      2,688    791,360     0.2
SINT      258,688    592,448   2.29    296,555     55,616  1,176,576    15.4
SPHD      118,144    173,632   1.47    121,748     26,944    913,600     7.0
SPHC        1,152     16,832  14.61      2,488        832     34,816     0.1
WORK-H  1,589,376  2,318,208   1.46  1,636,381    697,664  6,379,520    94.7

named=1,672,128 (85.0% of ALL)  VBI 2:128 3:947 4:706 5+:192 max:12 total:1973  slips=0
Wrote builds/p2p8-clean-baseline-stress.json
```

### Exclusive mean SRC arithmetic from that record

SRC=608337; GCRA=602591; SINT=296555; SCPU=69399.

| Exclusive component | Mean ticks per presented frame |
|---|---:|
| SINT minus SCPU | 227156 |
| SPHD plus SPHC | 124236 |
| GCRA minus named children | 121588 |
| SCPU | 69399 |
| SHDT | 45824 |
| SPRM | 10591 |
| SCAT | 3797 |
| SRC minus GCRA | 5746 |
| Total | 608337 |

GCRA remainder includes actual remaining scheduled work, not just scheduler
bookkeeping. These are differences of arithmetic means from one population,
NOT differences of independent percentiles. The residual is not automatically
empty, redundant, or all reclaimable.

### Pinned source index

All repository paths below were inspected at commit
`430aca2879e9071dc2b22f944f5c2909c9ce7aa4`; approximate source line ranges
identify the reviewed portions. Larger files were read selectively around these
functions, not claimed to have been audited in their entirety.

- **Profile/report:** `artifacts/performance/2026-09-16_p2-2p8-gap-sizing/README.md` — Use corrections, not superseded sampled helper attribution.
- **Corrected call attribution:** `artifacts/performance/2026-09-16_p2-2p8-n0409-profile/CANDIDATE_SELECTION.md` — Exact BL/BLX correction and prior fixed-ring experiments.
- **PC census:** `artifacts/performance/2026-09-16_p2-2p8-n0409-profile/census.txt` — 129 regions; self-costs not promised recoverable cost.
- **Profile meta:** `artifacts/performance/2026-09-16_p2-2p8-n0409-profile/arm9-profile.meta.txt` — cycles=487368912; regions=129.
- **Flat invalidation, indexed animation dispatcher:** `src/port/reloc_backend_compat_shims.c` — ndsFTParamsFlatWalkFor, ndsFTParamsInvalidateSubtree, ftParamUpdateAnimKeys; approx2710..3290.
- **Pose:** `src/nds/nds_ft_pose.c` — Parse, Play, Run, Update, Reapply; approx940..1500.
- **Curve math:** `include/nds/nds_anim_fixed.h` — Q conventions, converters and ndsR2AnimEvalQ.
- **Collision:** `src/port/reloc_backend_mp_collision.c` — Existing separate caches340..585; endpoint query1000..1160; floor query1540..1700.
- **Motion storage:** `src/nds/nds_reloc_assets.c` — ndsRelocAssetLoadFighterStreamClip1170..1395; payload read retained.
- **Existing resident pack:** `src/nds/nds_battlepack_anim.c` — BPA2 lifetime and pointer dispatch; not BPS1 streaming implementation.
- **Object import:** `src/import/battleship_sys_objman.c` — Overlay import, existing AObj pool and gcRunAll wrapper.
- **Scheduler reference:** `decomp/BattleShip-main/decomp/src/sys/objman.c` — gcRunGObjProcess and gcRunAll approx2110..2360.
- **Simulation reference:** `decomp/BattleShip-main/decomp/src/ft/ftmain.c` — UpdateInterrupt1110..1670; PhysicsMap1810..2075.
- **CPU reference:** `decomp/BattleShip-main/decomp/src/ft/ftcomputer.c` — ProcessAll and SetFighterDamageDetectSize7720..8060; setup not proved hot.
- **Collision matrix reference:** `decomp/BattleShip-main/decomp/src/gm/gmcollision.c` — TRS/Ncs, affine composition, inverse and lazy validity0..490.
- **SM64DS animation reference:** `decomp/sm64ds-decomp/src/_ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj.c` — Same-file fast path and compact animation state.

### Primary hardware documentation consulted

- BlocksDS tutorial, “Optimizing code”: ARM/Thumb tradeoffs, multiply width,
  cache working sets, 32-byte cache lines and DMA contention.
- BlocksDS tutorial, “TCM and Cache”: CPU-local TCM and DMA/ARM7 visibility.
- BlocksDS tutorial, “Using the ARM7”: service ownership and shared-memory cost.
- libnds `math.h` documentation: hardware divide/sqrt asynchronous interfaces.

The project's Calico configuration is not identical to the BlocksDS defaults.
No default library stack layout or ARM7 memory assignment is assumed to be the
project's actual configuration.

---

<a id="source-links"></a>

## IV. Clickable primary-source reference index

These links expand the pinned paths and primary hardware references already
identified by the research. They are not a new repository audit. Source line
ranges in the preceding evidence record remain approximate; links use the
immutable inspected commit rather than the moving branch.

### Repository files

- [`artifacts/performance/2026-09-16_p2-2p8-gap-sizing/README.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/artifacts/performance/2026-09-16_p2-2p8-gap-sizing/README.md)
- [`artifacts/performance/2026-09-16_p2-2p8-n0409-profile/CANDIDATE_SELECTION.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/artifacts/performance/2026-09-16_p2-2p8-n0409-profile/CANDIDATE_SELECTION.md)
- [`artifacts/performance/2026-09-16_p2-2p8-n0409-profile/census.txt`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/artifacts/performance/2026-09-16_p2-2p8-n0409-profile/census.txt)
- [`artifacts/performance/2026-09-16_p2-2p8-n0409-profile/arm9-profile.meta.txt`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/artifacts/performance/2026-09-16_p2-2p8-n0409-profile/arm9-profile.meta.txt)
- [`src/port/reloc_backend_compat_shims.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/src/port/reloc_backend_compat_shims.c)
- [`src/nds/nds_ft_pose.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/src/nds/nds_ft_pose.c)
- [`include/nds/nds_anim_fixed.h`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/include/nds/nds_anim_fixed.h)
- [`src/port/reloc_backend_mp_collision.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/src/port/reloc_backend_mp_collision.c)
- [`src/nds/nds_reloc_assets.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/src/nds/nds_reloc_assets.c)
- [`src/nds/nds_battlepack_anim.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/src/nds/nds_battlepack_anim.c)
- [`src/import/battleship_sys_objman.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/src/import/battleship_sys_objman.c)
- [`decomp/BattleShip-main/decomp/src/sys/objman.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/decomp/BattleShip-main/decomp/src/sys/objman.c)
- [`decomp/BattleShip-main/decomp/src/ft/ftmain.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/decomp/BattleShip-main/decomp/src/ft/ftmain.c)
- [`decomp/BattleShip-main/decomp/src/ft/ftcomputer.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/decomp/BattleShip-main/decomp/src/ft/ftcomputer.c)
- [`decomp/BattleShip-main/decomp/src/gm/gmcollision.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/decomp/BattleShip-main/decomp/src/gm/gmcollision.c)
- [`decomp/sm64ds-decomp/src/_ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/decomp/sm64ds-decomp/src/_ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj.c)

### Hardware documentation cited in the research

- [BlocksDS: Optimizing code](https://blocksds.skylyrac.net/tutorial/advanced/optimizing_code/)
- [BlocksDS: TCM and Cache](https://blocksds.skylyrac.net/tutorial/intermediate/tcm_and_cache/)
- [BlocksDS: Using the ARM7](https://blocksds.skylyrac.net/tutorial/intermediate/using_the_arm7/)
- [libnds: math.h hardware-math documentation](https://blocksds.skylyrac.net/libnds/math_8h.html)

The prior analysis cites the ARM9's 8 KiB instruction cache and 4 KiB data cache
as reasons to reduce hot code/data working sets, and CPU-local TCM visibility as
a constraint on DMA/ARM7 buffer placement. The project's Calico configuration
must still determine actual memory ownership and service integration.

### Reproducing the embedded experiment

Save the following C and Python blocks as `kernels.c` and `test_kernels.py` in
the same directory. Use the host-build commands in [the experiment guide](#research-overview).
They produce a local shared library and a fresh `results.json`. The original
recorded JSON remains embedded below for comparison. The generated assembly is
provided in full as inspection material; it is not an ABI-qualified game patch.

---

<a id="kernels-c"></a>

## Appendix A. Complete C prototypes

Original filename: `kernels.c`.

````c
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
````

---

<a id="test-kernels-py"></a>

## Appendix B. Complete Python differential tests

Original filename: `test_kernels.py`.

````python
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
````

---

<a id="test-results"></a>

## Appendix C. Recorded host test results

Original recorded output. No tests were rerun during this consolidation.

Original filename: `results.json`.

````json
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
````

---

<a id="arm-assembly"></a>

## Appendix D. Complete generated ARM946E-S assembly

Original compiler output, preserved verbatim. Function size and instruction selection are not DS frame-time measurements.

Original filename: `kernels_arm946e.s`.

````asm
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
````

---

<a id="codegen-results"></a>

## Appendix E. Recorded code-generation results

Original filename: `codegen.json`.

````json
{
  "compiler": "Clang 17.0.0",
  "flags": "--target=arm-none-eabi -mcpu=arm946e-s -marm -O2 -ffreestanding -fno-builtin -ffunction-sections",
  "sizes_bytes": {
    "reference_cubic": 332,
    "build_basis": 224,
    "evaluate_basis": 108,
    "evaluate_horner": 88,
    "invalidate_range": 104
  },
  "timing_measured": false,
  "notes": "Standalone reference and candidates, not actual linked game functions; reference counters omitted."
}
````

---

<a id="original-checksums"></a>

## Appendix F. Original package checksums

These hashes describe the eight original archive members before Markdown consolidation, not the formatted sections or this combined document. All eight were checked against the uploaded archive during consolidation. Original code, assembly, Python, and JSON blocks are embedded verbatim; Markdown headings and internal references are reformatted for a single file.

Original filename: `SHA256SUMS.json`.

````json
{
  "results.json": "4f909e7bf267b80e58eef8ebab499eb87dfd1197aa80be0a1ec8ee8a93679bba",
  "codegen.json": "56d77727e4a4118f6a7aa643f883313ca3917685d6f454f1027c1729b293b207",
  "EVIDENCE.md": "c06543e0bd92cb0f2ff9fe3eb80e97cd32b2f67e23947571b654e2b31d659aa4",
  "CANDIDATE_DETAILS.md": "180b2804ace3d78324fa098f9261a5a13b24b417464a037e4ed4178a1d319795",
  "kernels.c": "f22f5488fcb088cd1bf8d03a3fa5b0e2ed2d08d83f1dfd0a2291a69898e3f924",
  "kernels_arm946e.s": "4d3cb6fe11a55d741a7f90ffd2b6bd2189b948807551b7ee202c4e06324e06a9",
  "README.md": "df8ff37e42669eadd06bb6a29427376522fa27d64d8641ba2a043ec17df6e823",
  "test_kernels.py": "29817f6410c9343c2aba84a2695d1f1f6bc7c781e7491f021a2672e1813e703f"
}
````

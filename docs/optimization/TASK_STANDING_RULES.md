# Standing rules for all optimization /task files

These rules apply IN FULL to every task file under `docs/optimization/`. Each task
file states "Standing rules apply" at the top instead of restating them. If a task
file contradicts this document, the task file wins only where it says so explicitly.

## File convention (the owner, 2026-07-20)

- **One task per .md file.** Naming: `Claude<Model>_Task<NN>_<slug>_<YYYYMMDD>.md`,
  where `<Model>` is the planner's current model — currently **`ClaudeOpus48_`**
  (was `ClaudeFable5_` through Task 51; bump it when the planner model changes).
  Resolved task files stay in place as history under the prefix of their era; never
  rename them and never append a second task to one, and archive closed tasks.

## Process

- `decomp/` is read-only. Port-side edits only (src/nds, src/port, include, linker,
  scripts, docs). **Read-only protects the reference, it does not freeze the
  algorithm (the owner, 2026-07-22):** "if we can write a DS optimized equivalent,
  we should do that." When profiled cost lands in decomp-sourced code, the move is
  a port-side DS-native equivalent behind a Makefile flag with the decomp path kept
  as the comparator/oracle — the shape Tasks 9/16 used for the float helpers. Never
  report decomp-sourced cost as unaddressable; that retires ~7% of the loop
  (soft-float) by misreading this rule.
- **Always build the tick-HUD ROM too (the owner, 2026-07-22).** Whenever the
  published `smash64ds-battle-playable-hwtri` ROM is rebuilt, rebuild
  `smash64ds-battle-playable-tickhud-hwtri` in the same change. It is the same
  program plus the Task 41 timers, so it is how the ship gets measured on device
  and in the census — a tick-HUD ROM that lags the published one silently reports
  a different binary's buckets. Its target block in the Makefile must carry every
  flag the published block carries; when adding a flag to one, add it to both.
  Task 37 shipped without this and the tick-HUD build was left a release behind.
- Verify chain: `.\scripts\verify-dev-fast.ps1` then `.\scripts\verify-boundary.ps1`.
  Full sharded Regression ONLY if shared/imported TUs changed, once, at session end.
- Long builds run detached (Start-Process → log → poll completion stamp), never
  foreground. One full Regression sweep per session maximum.
- Separate commits per task/phase. Never leave verified work uncommitted; checkpoint
  honest WIP to a branch if a session ends mid-flow.
- Time-box open-ended debugging: ~10 emulator runs or ~1 hour, then checkpoint and
  report. Cite file:line for any verifier-expectation change.
- Final steps of every task: brief HANDOFF.md/PORTING.md update, then
  `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean`.
- Agents cannot see or hear. Rendered-output claims need `capture-melonds.ps1`
  screenshots with non-clear-color pixel assertions (counters prove submission, not
  display). Audio-quality claims are flagged for a the owner listen check.
- Diagnostics/observer ROMs build `NDS_FAST_WALLPAPER_AFFINE=0` when
  `NDS_RENDERER_PROFILE_LEVEL>=1` (profile-1 + affine OOMs the taskman arena —
  known, unfixed, accepted). Shipping profile-0 keeps affine=1.

## Which number a task is judged on (Tasks 64-66, 2026-07-26)

**Search on `WORK-H` P50. Gate on `WORK-H` P95 ≤ 1.12M. Use `ALL` only to
confirm the VBlank step was actually crossed.**

`ALL` is wall time for the whole presented iteration, so it is quantized to
whole 560,190-tick VBlank periods. A change that removes real work but less
than one period *cannot* move it — the reclaimed time becomes VBlank wait.
"ALL flat" therefore never means "this removed nothing," and reading it that
way produced four uninformative verdicts (Tasks 53, 55, 56, 63), including a
KILL on a 47% fighter-vertex reduction.

`scripts/sample-tick-hud-buckets.ps1` reports the buckets a task is judged on:

- `WAIT` — the idle `swiWaitForVBlank` span. Cross-checked against the host
  per-PC profiler to 0.15%.
- `WORK` — `ALL − WAIT`, sampled as its own series so it has a real P95. The
  P95 of a difference is not the difference of two P95s.
- `WORK-H` — `WORK` minus the tick HUD's own console redraw, which costs a few
  hundred thousand ticks on the ~2 frames per second where it runs. That is
  invisible in a mean and decisive in a P95, and the published profile-0 ROM
  does not pay it.

Report P50 **and** P95 with the spread ratio, never the mean alone. Two
different problems hide in one bucket: steady-state cost lives in P50,
burstiness lives in the P50→P95 gap, and they take opposite fixes.

### Vary the build, not the run (Task 79, 2026-07-26)

**Repeat runs of the same ROM are bit-identical.** Task 79 sampled one ROM twice
and got the same value for every bucket, every percentile, and the anomalous
6.4M-tick maximum `ALL` frame — identical, not close. Run-to-run variance in
this harness is **zero**.

So a third run of the *same* ROM confirms nothing except that the harness works.
When an A/B looks noisy or surprising, re-running A is not the check; it returns
the same digits.

This also renames the ±8,000 figure. It is **not** measurement noise — it is
build-to-build **placement** variance, deterministic and real, and it appears
only when the binary changes. A change and its placement effect arrive together
and no amount of re-running separates them. To attribute an effect, vary the
build: same tree, flag off versus flag on, so everything except the change is
held fixed. That is why Task 79's two arms were built from one tree.

### Never compare two instruments across two binaries (R2-00c, 2026-07-27)

The corollary of the rule above, and it cost a whole board row. R2-00a compared
the emulator's halt accounting, measured in a `NDS_TASK37_PROFILE=1` ROM,
against the tick HUD's `WAIT`, measured in a tick-HUD ROM, and concluded the
bracket under-counted idle by 588,353 ticks/frame — that the gate metric was
manufacturing work. Placement differs between those builds, so frame 453 does
not name the same workload in both, and the disagreement being measured was the
build.

Put both instruments in **one** ROM and read them from **one** run. When that
was done, `ALL` agreed to 0.04% and `WAIT` to a constant −851 ticks/frame, with
the excursion frames no different from the clean ones. Nothing was wrong with
the bracket.

`NDS_TASK37_PROFILE_PER_FRAME_REGION=1` exists for this: it numbers each
profiled frame as its own profiler region, so the host ledger differences
against the tick-HUD ring frame by frame instead of only over the window. Use
it — a window total cannot tell "accurate everywhere" from "over-counts on
clean frames, under-counts on the tail", and the tail is what the P95 gate is
decided on.

### A regression on every frame is a mechanism, not noise

Placement variance moves a bucket by a bit on some frames. A delta present on
**128 of 128** frames with a consistent sign is something the code now does
differently. Task 79's stage bucket rose 6,880 on every frame — that was a
128-entry table being shared by three owners instead of one, and reading it as
noise would have shipped a real regression.

## Fidelity doctrine (the owner, 2026-07-20)

`PROJECT_GOAL.md` owns the fidelity contract. Gameplay/source behavior must be
mechanically equivalent and verifier-gated; bit exactness is required only for a
named quantity whose exactness the port guarantees. Rendering may use the goal's
fidelity budget.

- Rendering changes gate on a fidelity budget: synchronized A/B screenshot pairs,
  changed-pixel counts + mean delta reported, PNGs attached, and **the owner is the
  approval oracle** — never self-approve visuals. Soft flag threshold ≈5% changed
  top-screen pixels, and no structural artifacts (missing geometry, wrong textures,
  Z-fighting flicker).
- Every exactness gate must name the quantity and why it is guaranteed exact.
  Aggregate hashes that mix gameplay, render-derived, and timing-derived state
  cannot establish which contract changed.
- Approximation does not excuse unexplained differences between identical-source
  builds. Determinism and mechanically equivalent gameplay still require proof.

### The play test is a gate, not a courtesy (the owner, 2026-07-22)

"Exactness is still useful obviously but the 'eyeball test' when sacrifices are
being made matter a lot."

An automated gate measures what it was written to measure. When a change trades
something away, the owner playing it is evidence about the thing that actually
matters, and it is not a softer substitute for a verifier — on Task 37 a
state-hash gate reverted a build the owner had already played and found faster
and correct, while RNG, battle state, camera, ground and collision were all
bit-identical.

- Record a play test as a result, not an impression: what ROM (by hash), on what
  hardware, for how long, and what was watched for. "Looked fine" is unciteable
  six weeks later; "candidate hash 6D2582D4, retail, one full match, no visual
  artifacts, faster" is evidence.
- Its limit is coverage, not validity — one session cannot surface a rare
  divergence. So pair it with a gate that names which quantity it holds exact,
  and do not let either stand alone.
- An agent cannot perform this gate. Agents cannot see or hear; never
  self-approve on the strength of counters or a passing verifier alone.

## Content-completeness doctrine (the owner, 2026-07-21)

Battle-reachable game content — SFX cues, animations, visual effects the original
game plays in the shipped scene — may NOT be runtime-excluded for capacity or
representation convenience. Fail-closed exclusion is a temporary audit state, not
a shipping state. Substituting a different asset stays banned. Any exclusion that
survives a fix task requires the owner's explicit per-item sign-off in the task
report; silent or agent-chosen exclusions are a failed task. Capacity budgets
(resident caps, reserves) are negotiable levers to present to the owner with numbers
— never reasons to drop content unilaterally.

## Merge & branch policy (the owner, 2026-07-21)

- Every task runs on its own branch `codex/task<NN>-<slug>`, branched from
  CURRENT master. Do not stack a new task branch on an unmerged task branch
  unless the two tasks are declared dependent in their files — independent
  branches stay independently mergeable and revertable.
- **A KEEP is not done until it is on master.** When all gates are green and
  docs + snapshot are committed, the session's final git actions are: merge the
  task branch into master with `--no-ff` (merge message = one-line task
  verdict), then run `verify-dev-fast.ps1` once on master. Report the merge
  commit hash.
- STOP / REJECT / WIP outcomes do NOT merge. The branch stays as the
  checkpoint; name it in the report. Evidence/docs-only commits may merge when
  verify is green.
- Merging a KEEP that is still flag-gated pending its device checkpoint is
  expected (device-economy rule) — master stays one-line revertable.
- **KEEPs ship ENABLED in the published profile-0 target at merge time (the owner,
  2026-07-21).** The Makefile flag exists so a device-checkpoint revert is one
  line — it is NOT a deferral mechanism. A task whose keep is merged but not
  forced on in the published target block is incomplete; state the published
  on/off decision explicitly in the task report.
- Master must remain stranger-buildable at every merge: publish identity pins
  (README expected SHA-256, DECOMP_PIN outputs) travel in the same branch as
  any change to the published ROM.
- **NEVER push.** Master is the public GitHub repository; `git push` is the owner's
  explicit, per-event call. No agent runs push, ever.

## Device-test economy (the owner, 2026-07-20)

Retail sessions cost the owner real time. Minimize them by CLASS, not by skipping proof:

- **melonDS-sufficient class (device deferred):** mechanisms that REMOVE CPU work —
  visible in melonDS typed buckets. melonDS A/B + engagement counters incrementing
  in melonDS + a code-review note that no device-divergent fallback exists = KEEP
  behind the feature's Makefile flag. Also report the calibration-predicted device
  delta (Task 10 multipliers: streaming ×1.50, cache-resident ×0.88, GX ×0.87,
  draw ×1.51, update ×1.73).
- **Device-only class (melonDS cannot referee):** icache/dcache locality, TCM
  residency, DMA timing, card-I/O timing, pacing near VBlank bucket edges (Task
  17/32: melonDS blind; Task 30: melonDS green-lit a retail regression). This class
  is DEPRIORITIZED in planning; when attempted, build the A/B pair into
  `builds/device-queue/` and QUEUE it — do not ask the owner to run it per-task.
- **Batched checkpoints:** one retail session per campaign (whenever the owner chooses)
  runs everything in `builds/device-queue/` plus one smoke boot of the current
  published ROM, whose shared engagement-counter HUD row proves ALL shipped features
  engaged in a single photo.
- **Every keep stays behind its Makefile flag until device-confirmed**, so a
  checkpoint revert is one line. Device evidence format: the 2/3/4/5+ VBlank
  interval histogram + typed HUD rows, normalized by sample count — never min-FPS.

## Sizing a fixed-size cache (Task 90, 2026-07-26)

**A hit rate cannot size a cache. Replay the request trace.**

12% misses is equally consistent with a working set that barely overflows and
one far too large to ever hold, and those want opposite decisions. The
light-shade LUT cache sat at 4 entries and 88% hits for eighty-nine tasks
without being questioned; a 128-request trace showed the working set was exactly
6, and one digit was worth `FTR` P50 -19,584 on 128 of 128 frames.

The procedure, one emulator run:

1. Record the request key sequence in the ROM behind a lab flag, not a hit rate.
2. Replay it host-side against a FIFO of each candidate size.
3. Take the smallest size whose miss count reaches the **compulsory floor** --
   the number of distinct keys, which no cache avoids paying once. If a larger
   size buys nothing over it, do not spend the RAM.
4. Confirm in the ROM (misses go to the floor) **before** spending an A/B.

This also tells you when to stop: if no candidate size approaches the floor, the
working set is unbounded and the answer is a different structure, not a bigger
cache.

## Budget ratchets vs correctness assertions (Task 90, 2026-07-26)

A verifier pin that encodes a **measured budget** (a byte count, an entry count)
may be raised by a task that brings a better measurement of that same quantity,
recorded in the assertion message with the script that re-derives it. A pin that
encodes a **correctness contract** (set membership, exactness, a required code
shape) may not be edited to make a change pass -- revert the change and ask, as
Task 82 E1 did. When in doubt about which kind a pin is, it is the second kind.

## Placement is exhausted (Task 94, 2026-07-26)

**Four consecutive placement experiments have regressed: Tasks 87, 88, 89 and
94.** Task 82's repack was the last one that paid, and Task 83 said at the time
that the cheap half was taken. Treat that as settled: do not propose another
placement move without a new mechanism, not merely a new candidate.

Task 94 is the cleanest refutation because it had every argument in its favour.
`gcPlayDObjAnimJoint` is the largest soft-float caller in the frame (54.2% of all
`fadd`/`fmul` calls), the top-ranked zero-eviction admission on the census, 500
bytes against 720 free so nothing was displaced, and confirmed to have actually
moved (0x020013c0 -> 0x01fff424). It regressed `WORK-H` P50 by **6,144 on 122 of
128 frames**.

Two estimators, both wrong:

- **Non-mem stall "in reach"** -- Task 83 already measured this ~18x optimistic
  per symbol.
- **Tier cyc/insn ratio** (own ticks x itcm_rate / current_rate) -- predicted
  **-7,894** and got the **sign** wrong. Retired here; do not use it again.

The reason both fail is that they price the symbol that moves and ignore the
space it vacates. `STG` rose 3,712 in an arm where the stage never calls the
moved function -- that is pure re-addressing collateral, and Task 83 already
found 69% of Task 82's win came from the vacated space rather than the moved
symbols. The layout term dominates and neither estimator models it.

A curated fixed-size section (`.text.hot`, `.text.hot.draw`) is a working set,
not a list. Removing a member re-addresses every other member.

## The layout is saturated (Task 95, 2026-07-26)

**Five consecutive single-lever changes have regressed for one shared reason:
editing a hot translation unit re-addresses its neighbours, and the collateral
now exceeds the gain.**

| task | change | collateral |
|---|---|---|
| 87 | inline more 64-byte copies | +17,728 |
| 88 | remove redundant clears | +9,536 |
| 89 | refill `.text.hot.draw` | +11,648 |
| 94 | admit a function to ITCM | `STG` +3,712 |
| 95 | hoist animation invariants | `STG` +3,392 / +5,056 |

The tell in 94 and 95 is identical and unambiguous: `STG` rose in an arm whose
change the stage path never executes. Nothing moved except addresses.

Task 95 is the sharpest case because **the mechanism worked** -- the hoist
improved `FTR` on 98 of 128 frames, median -2,688, exactly as sized -- and the
frame still got worse.

Practical rule: **stop proposing single-lever changes worth under ~20,000
ticks/frame.** At this local optimum the re-addressing noise floor is comparable
to the gain, so such a change is a coin flip regardless of how sound its
mechanism is. A lever is only worth pulling now if it changes enough at once
that the working set itself shrinks -- a data-structure or representation
rewrite, not a hoist, an inline, or a placement move.

## Size a rewrite before you scope it (Task 96, 2026-07-26)

Task 95's conclusion -- pull the animation lever wholesale, not one slice at a
time -- named a subsystem-sized task. Task 96 spent one read-only probe on its
ceiling instead of a session on its body, and the ceiling killed it: **the
`AObj` chain is 337.8 nodes/frame, so flattening it is worth at most
7,791-15,584 ticks/frame** against the ~68,000 of stall it was supposed to
explain. The premise was never true.

The generalisable part is the arithmetic, not the number. A structure-layout
rewrite is bounded by `(lines touched now - lines touched packed) x cost per
miss`, and both terms are countable from a read-only probe in one build. Do that
before scoping the rewrite, every time. It is one build against a session.

Two supporting rules fall out:

- **Cross-check a new instrument against an old one before trusting it.** This
  census (source-level node counter) and Task 92 (GDB return-address sampler)
  independently imply 8.75 ticks per soft-float call from opposite directions.
  Agreement that tight is what makes a single-run census safe to act on; without
  it, a lone counter is a hypothesis.
- **A per-symbol "stall %" does not locate the stall.** Task 92 read animation's
  68.4% stall as a data-layout signal and said so in §5; measured, layout is
  5-16% of the joint player's cost and frozen arithmetic is 66%. Stall share
  tells you a class is not issue-bound. It does not tell you which memory the
  class is waiting on, and inferring the structure from the percentage is a
  guess.

## Check the constraint against the contract, not against the last task (Task 96, 2026-07-26)

Tasks 78 through 96 were all **exactness-preserving**, and no contract required
that. Task 92 §5 wrote the constraint into the plan ("a re-scoped Task 78 whose
win must come from exactness-preserving reorganization only"), every later task
inherited it from the plan rather than from `PROJECT_GOAL.md`, and by Task 96 it
had hardened into "no lever the contracts permit" -- which was false.

What the contracts actually say:

- `PROJECT_GOAL.md` §Sacrifice Order: audio fidelity, **visual fidelity**,
  original 60 Hz simulation, gameplay fidelity -- and "Stable 30 FPS is the most
  protected requirement." Frame rate outranks all four.
- `AGENTS.md`: rendering-side changes "gate on a reported fidelity budget
  (synchronized screenshot diffs plus the owner's visual approval), not pixel
  exactness."

Exactness is required for **gameplay** (state-hash gated, mechanically
equivalent). On the rendering side it was a habit, and it cost nineteen tasks of
searching a subset of the space while the frame stayed 613,888 ticks over gate.

The rule: **when a direction closes, re-read the contract before concluding the
space is empty.** Specifically, check whether the constraint you just failed
against is one the contract imposes or one an earlier task's plan invented. A
plan is not a contract, and this campaign has now twice mistaken one for the
other -- Task 92 §4 caught the same shape when Task 78's own sizing number was
treated as a gate.

## Find the tick anchor before proposing a lever (Task 98, 2026-07-27)

A reduction ratio is not a saving. "−27.2% of vertex words" and "−50% of texels"
are only savings if some prior measurement ties *that quantity* to ticks *on that
path*. Two levers were proposed to the owner in one session without that check
and both were empty:

- **Words do not cost ticks here.** Task 55 E2 elided 355 GX words per frame,
  losslessly, geometry intact, and `ALL` P50 moved **+64** over 128 samples --
  against its own prediction of ~148,000. That kills axis-reuse and VERTEX10
  encoding for both stage and fighters.
- **Texels do not cost ticks here.** `renderer: texture + material` is dominated
  by `ndsRendererHardwareResolveOrBindTexture` at 40,537 ticks/frame over 25
  binds -- **~1,621 ticks per bind, independent of texture size.**

Both anchors already existed and took minutes to find. Task 96 is the
counter-example that held up: no anchor existed for the `AObj` chain, it said so,
measured one, and the conclusion survived review.

## The cost is per-operation, not per-datum (Task 98, 2026-07-27)

The frame is dispatch overhead spread thin across many small operations, not
throughput on large ones. 79.2% stall in the texture class, 52-89% in sixteen of
seventeen classes, ~1,621 ticks to bind a texture whatever its size.

**A change only pays if it removes operations** -- binds, runs, draws,
dispatches -- not if it shrinks their payloads. This is the same shape as the
saturated-layout finding (Tasks 87/88/89/94/95) seen from the data side, and it
constrains visual approximation specifically: smaller textures, coarser
coordinates and fewer bits per vertex are all payload reductions and all buy
nothing.

**Refined by Task 99: a triangle is not an operation.** Dropping half the
stage's triangles (202 -> 101) at the emit seam moved `STG` P50 by only
**-19,584**, or ~194 ticks per triangle, leaving the stage bucket **89% fixed**.
The run structure was unchanged -- only corners inside each run were skipped --
and that is exactly why it bought so little.

The operative rule is therefore narrower than "fewer triangles": **a change pays
only if it removes a run, bind, draw or dispatch, not if it removes items from
inside one.** Task 99 also retires the estimator that produced its own
hypothesis: dividing a bucket by its item count gave ~1,832 ticks/triangle
against a measured 194, wrong by 9.4x, because it charged fixed overhead to the
per-item quantity. Do not size a per-item lever by dividing a bucket total.

## GDB `if` at top level resumes exactly once (Task 96, 2026-07-26)

In a batch script, `if <cond> / continue / end` outside a `commands` block is
not a wait loop -- it resumes once and falls through. A 30-frame sampling window
written that way collected **2 frames**. Use `while`. Every two-stop delta
census in `scripts/census-*.ps1` depends on this, so check the reported window
against the requested one before reading the numbers.


## State the datapath before probing a hardware mechanism (Task 100, 2026-07-27)

`RASTER_AXIS_CAMPAIGN.md` proposed that the stage's unattributed ~331,300 fixed
ticks were rasterization time reaching the CPU as GX FIFO backpressure. Task 100
built the probe: a quarter viewport, so a quarter of the pixels rasterize while
the GX word stream, triangle count and every vertex transform stay bit-identical.
`STG` P50 moved **-320** against a >=40,000 kill criterion.

The refutation did not need a build. The DS 3D pipeline is decoupled at the
polygon RAM -- the rasterizer consumes the *already-swapped* buffer during
scanout, so it can drop polygons past its per-scanline limit but can never stall
the CPU. The campaign had quoted `src/port/taskman_seam.c:4925` correctly and
then over-read it: that sentence describes **FIFO** backpressure, which is
geometry-engine throughput and scales with vertices, not fill.

**Before building a probe for a hardware mechanism, write down the datapath and
name the point where the proposed cost enters the CPU's critical path.** If you
cannot name that point, the probe will measure zero and the reason will be
architectural rather than empirical. This is the hardware counterpart of "find
the tick anchor before proposing a lever" (Task 98 §7), and it costs minutes
against the three builds and three measurement runs it would have saved.

## The noise floor is ~5,000-7,000, and it is now measured (Task 100, 2026-07-27)

Task 100 arm C removed one `glEnable(GL_ANTIALIAS)` call executed once at init.
It cannot change per-frame CPU work by a single instruction. It moved `STG` P50
by **+5,120** and `WORK-H` P50 by **+6,848**.

That is a direct measurement of build-to-build placement noise from a build whose
per-frame work is provably identical, and it confirms the +/-8,000 figure the
campaign had been carrying on inheritance. Read every delta against it: Task 99's
-19,584 for half the stage's triangles was only ~2.5x this floor, and the >=20,000
bar Task 95 set is barely three times it.

## A null below the noise floor is not a null (Task 103, 2026-07-27)

Task 55 E2 elided 355 GX words per frame and measured `ALL` P50 at **+64**. That
was read as "words do not cost ticks", and Task 98 §2 made it the anchor row of
its per-datum-versus-per-operation thesis.

Task 103 timed the word-push loop directly: **9.51 ticks per GX word**, 37,244
ticks/frame over 3,916 words. So Task 55 E2's lever was worth ~3,376 ticks --
below the 5,000-7,000 build-placement floor Task 100 measured. The experiment
resolved nothing, and three later tasks reasoned from it as if it had resolved
something.

**Before recording a flat result as a refutation, compute what the change was
worth in ticks and compare it to the floor.** If the expected effect is under
~7,000, a flat reading is "not measurable this way", and it must be written that
way. Reserve "refuted" for levers whose predicted size the instrument could
actually have seen.

**And then measure it the way that can see it (R2-03 E1, 2026-07-28).** The
symbol census times the function directly and carries no placement term, so it
resolves what an 8-frame A/B cannot. R2-03 E1's `sqrtf` replacement read flat on
every bucket with no consistent sign — and the census put it at **−6,040
ticks/frame**, real and repeatable, sitting inside the floor. Build the
candidate a second time with `NDS_TASK37_PROFILE=1
NDS_TASK37_PROFILE_PER_FRAME_REGION=1`, census both arms, and diff the symbol
that changed. One extra build and run buys a verdict instead of a shrug.

## Profile the whole owner before optimising a loop inside it (Task 103)

Tasks 51, 52, 53, 54, 55, 99 and 100 all worked on the stage run loop. Task 103
measured that loop at **136,236 ticks/frame against a 388,480 `STG` bucket** --
35%. The other 61% is outside `ndsRendererCommitNativeStageSegment` entirely, in
the owner prepare path, and no task had profiled it.

Seven tasks optimised a third of a bucket while calling their results statements
about the bucket. **Partition the owner top-down and confirm the block you are
about to work on is actually most of it**; the in-place span method in
`scripts/census-stage-run-phases.ps1` costs one build and one 60-frame run.

## Measure the work that never reaches the fast path (R2-02 E3, 2026-07-28)

The rule above says partition the owner before optimising a loop inside it. E3
is its sharper form: **also partition by what the fast path admits.** Eight
tasks worked on the Task 36 stage replay. The replay was never the problem. Of
54 stage runs, 33 replayed at 30,200 ticks/frame; the 21 that did not cost
68,547, and four segments inside those 21 were **27 triangles at 1,680 ticks
each** — 20% of the whole stage bucket for 8% of its geometry. Every previous
task had been tuning the admitted 61%.

An eligibility mask, a "supported" predicate, or a fast-path guard partitions
the work into a measured half and an unmeasured one. **Count and price both
halves before you optimise either.** The per-segment split that found this cost
one `NDS_TASK103_STAGE_RUN_PHASE` build and one run.

## An eligibility constant is a claim about the data; re-check it (R2-02 E3)

`NDS_TASK36_REPLAY_SEGMENT_MASK` excluded four segments because their bindings
were not in `NDS_RENDERER_TASK36_RIGID_BINDING_MASK`. That was true when it was
written. Task 51 then replaced those bindings' per-frame compose with a
`MULT4x3` of a generated constant table — after which nothing about them was
dynamic, and neither mask moved. The stale constant cost 45,349 ticks/frame for
a year of tasks, and no instrument pointed at it because the two masks are named
for the same property and agreed with each other.

**A hand-written "this is dynamic / unsupported / unsafe" constant is a claim
about the data, not a fact about it. Re-derive it against the data before
building around it** — and when a task changes what makes something dynamic,
grep for every constant that encoded the old answer.

Corollary, from the same cut: **a hand-maintained dispatch table that silently
drops what it does not recognise will eventually drop something that mattered.**
`ndsRendererTask36ReplayOpcode` had no case for the `MATRIX_MULT4x3` class Task
51 appended, so the recorder discarded it — correct for state classes the replay
re-issues, catastrophic for a matrix. Make the recogniser **fault on the classes
it must not drop** rather than trusting the next author to notice.

## Diff the changed geometry against the control, not the candidate against your expectations (R2-02 E3, 2026-07-28)

R2-02 E3 was reported KEEP with "stage visually intact ... no tearing, no
dropped or duplicated geometry". A crop of the flower band against the default
ROM shows **both flower beds destroyed** — collapsed into a smear of specks
across the trunk. The report had been written from the candidate screenshot
alone, confirming that recognisable elements were *present*. Presence is not
parity.

Two gates let it through and both will do so again:

- **Boundary passed** — it is a gameplay state-hash gate and says nothing about
  scenery.
- **Required-region detail moved 7 pixels in 7,200** (62.681% vs 62.750%),
  because the flower beds are not inside the required region. A near-identical
  detail percentage is not evidence that the render matches.

**For any render-side change: crop the geometry you changed out of the candidate
*and* the control, put them next to each other at 2× nearest-neighbour, and
look.** After a verifier run `artifacts/visibility/latest.png` and
`previous.png` are already that pair. It costs one screenshot read and it is the
only thing in the toolchain that would have caught this.

Corollary: **a falsifier bounds the hypothesis it was written for and nothing
else.** E3's invariance proof hashed the prepared vertex data over 1,828 frames
and was correct — that data really is constant. What broke was matrix-stack
state, which it never touched. Passing a falsifier is not passing a review.

## Compare a span to a span, not to a census row (R2-03 E3, 2026-07-28)

A symbol's row in the frame-work census is **self time**: every callee is
charged to its own symbol. A `cpuGetTiming()` bracket is **inclusive**. Mixing
them produces a confident wrong answer.

R2-03 E2 measured the fighter walk and revalidation at 13,670 ticks/frame,
compared that against `ndsFighterMarioFoxDLAllDrawForSlot`'s census row of
37,206, concluded the two phases were 37% of the function, and ranked the next
cut on it. Bracketing the whole call said the function is **494,863
ticks/frame** — 13× its census row. The two phases are 4%, and the real target
was a 113,199-tick span the ranking had not even named.

**Before sizing anything against a function's "total", say which total.** If the
number came from the census it is self time and the callees are elsewhere; if it
came from a bracket it is inclusive and the callees are inside. A cheap way to
find out which you are holding: bracket the whole function once. It is one
counter and it makes every span inside it a share of something real.

## Trust a span in proportion to its length (Task 103 E7, 2026-07-27)

The E-series censuses bracket code with two `cpuGetTiming()` reads. Two reads
cost on the order of a hundred ticks, so a span of a few hundred is mostly
instrument and a span of a few thousand is not.

**Before sizing a lever from an E-series span, check the per-call figure. Under
~1,000, re-derive it a different way (call counts times a known unit cost, or a
longer enclosing span) before building anything.** Combine this with the
noise-floor rule above: a lever predicted under ~7,000 cannot be resolved by an
A/B at all, so measure it differently or leave it.

> **Corrected by Task 104 (2026-07-27).** This rule was first written blaming
> bracket over-attribution for Task 103 E7's 3.5x miss, and cited E6's 3,277
> ticks/call reset as an inflated span. That attribution was wrong. Task 84 E1
> had independently priced the same clear at 3,544 ticks/call by duplication,
> with no bracket at all, and the two agree. E7 missed for the reason in the next
> rule, not because the instrument lied. The guidance above still holds on its
> own terms; the example did not belong to it.

## Size a memory lever by bytes that stop being touched (Task 104, 2026-07-27)

Task 103 E7 removed a dead 1,292-byte clear and predicted 9,831. It measured
**-2,752, 28%.** Task 104 removed the clear *and* the 1,292-byte copy that
followed it on the same struct, and measured **-22,016 on `STG`** -- 8x more from
deleting a second access to bytes the first change had already stopped clearing.

Task 84 E1.4 had named the mechanism in advance: a clear warms the cache lines
that the next writer would otherwise miss on. Remove one of two accesses to the
same lines and the misses **relocate into the survivor** instead of disappearing.
Remove both and there is nothing left to relocate into.

**Count a memory-traffic lever in bytes that stop being touched at all, not in
instructions that stop executing.** If any other code still walks those cache
lines in the same frame, assume roughly a quarter of the nominal saving until
measured -- which usually puts the lever under the noise floor and means the
honest move is to widen the change until the bytes go completely untouched, not
to build the partial one and read its null as a refutation.

This is the general form of what closed Task 84's Routes 1 and 2, and it is why
Route 3 paid: the win came from the bytes going cold, not from the call count.

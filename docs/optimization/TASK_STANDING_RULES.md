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
were not in `NDS_RENDERER_TASK36_RIGID_BINDING_MASK`.

**Correction, R2-02 E4, 2026-07-28: that constant was not stale, and this rule
was invoked on a false premise.** E3 argued Task 51 had made those bindings
constant. `NDS_TASK51_STAGE_NATIVE ?= 0` — Task 51 is compiled out of every ROM
this campaign has measured, so it had made nothing constant in any binary. The
two masks agreed with each other because **they are required to agree**: a
dynamic binding's captured stream is a per-triangle `MATRIX_LOAD4x4` of
projection × view × model, so replaying it pins that geometry to the capture
frame's camera. Widening one mask deleted the flower beds; widening both deleted
them a different way. Full account in
`ClaudeOpus5_R202_E4_ActorSegmentsRefuted_20260728.md`.

**The surviving rule is the inverse of the one first written here: before
declaring a constant stale, check that the *change you think made it stale* is
compiled into the binary you measured.** Read the build config header, not the
source. And when two constants encode the same property, ask whether they are
duplicated or *coupled* — if coupled, that coupling is an invariant and belongs
in a comment on both, not a redundancy to be cleaned up.

The general form still holds: **a hand-written "this is dynamic / unsupported /
unsafe" constant is a claim about the data, not a fact about it. Re-derive it
against the data before building around it.** Much of that re-derivation is free:
the shift census that killed E4's first hypothesis reads a checked-in generated
file and took ten minutes with no build.

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
and was correct — that data really is constant. What broke was matrix state,
which it never touched. Passing a falsifier is not passing a review.

Second corollary, added by R2-02 E4: **the verifier's own per-region numbers do
carry this signal, but only against a control arm's numbers.** E4 arm C moved
`stage_body` green from 44.86% to 50.49% and detail from 52.40% to 48.77% —
418 pixels of blossom turned into grass, visible in output the verifier already
prints on every run — while required-region detail moved 0.056pp and Boundary
passed. Record the control arm's region table alongside the candidate's, in the
report, for any change that touches stage geometry.

## Lock a cross-build render comparison on the simulation clock, not the presented-frame counter (R2-02 E8, 2026-07-28)

Two ROMs stopped at the same `gNdsBattlePlayablePacingPresentedFrames` are not
at the same moment of the match. E8 and its control at presented frame 1100 read
`gSCManagerBattleState->time_remain` **1792 and 1790** — two simulation ticks
apart — and the resulting screenshots differed on 57% of the top screen. Nothing
was wrong with either ROM: a faster build presents more frames per unit of
simulation, so the presented-frame index drifts against the match clock, and the
faster the candidate the worse the drift. The drift is the *point* of the
optimization, so it grows exactly when the comparison matters most.

**Break on `gSCManagerBattleState->time_remain` instead.** It decrements once per
simulation tick, is identical across builds by construction, and both arms then
land on the same fighter poses and the same camera. At that lock E8's top screen
was pixel-identical to its control; at the presented-frame lock it looked like a
catastrophic regression.

Corollary: **measure the control arm against itself before believing any
screenshot delta.** Two runs of the same ROM at presented frame 1100 differed by
0 pixels, which is what established that the instrument was sound and the lock
was the flaw. One extra run per investigation, and it is the difference between
diagnosing the change and diagnosing the harness.

## Capture the window, not the screen coordinates (R2-02 E8, 2026-07-28)

`Graphics.CopyFromScreen` at a window's rectangle captures whatever is on top at
those coordinates. `SetForegroundWindow` does not reliably raise a window when it
is called by a background process, so a scripted capture can silently photograph
an unrelated application. Two such captures were byte-identical, which read as
"no visual delta" and nearly passed as evidence.

`scripts/capture-melonds.ps1` already does the right thing and any new capture
must copy it: `ShowWindow(handle, SW_RESTORE)` then `SetWindowPos(handle,
HWND_TOPMOST, ...)` before reading pixels. **And look at one of the images
before trusting a diff of them** — a wrong-window capture is obvious to the eye
and invisible to a pixel count.

## If a change is supposed to move fixed-point rounding, dump the fixed point and diff it (R2-02 E7, 2026-07-28)

E7 was designed as an associativity hoist — `(A × B) × C` becomes `A × (B × C)`
— and was written up as spending the Task 49 Tier-2 screen-pixel budget, because
20.12 multiplication is not associative. The plan was a frame-locked crop against
the control arm, per the rule above.

**Dumping the matrices instead was cheaper, exact, and showed the premise was
false.** One GDB script per arm printed
`sNdsRendererAdapterNativeStageWorkspace.binding_composed` at five frames
spanning the camera's range of motion: all 42 bindings bit-identical, at every
frame. There was no delta to crop, because there was no second multiply to
reassociate — `ndsRendererAdapterBuildCameraMatrices` already returns
`projection = MtxMul(lookat, persp)` with `modelview_valid` FALSE for the battle
camera, so the compose was `world × (lookat × persp)` all along.

The rule: **when the hypothesis is about arithmetic, measure the arithmetic.** A
screenshot answers "can I see a difference", which is the weaker question and the
slower one. A dump of the actual fixed-point result answers "is there a
difference", costs one GDB run per arm, and — as here — can turn a
budget-spending KEEP into a provably bit-identical one. Reserve the crop for
changes that really do produce different numbers.

Corollary, and the reason this is a rule rather than a note: **a host model of
the arithmetic is not evidence.** A Python replay of both orderings said 15
bindings should differ by up to 1,775 LSB. The ROM said zero. The model was
wrong about which operands the runtime actually uses, which is exactly the kind
of thing a model gets wrong and a dump cannot.

Second corollary: **fix the rationale, not just the code.** The flag comment and
the Makefile block both asserted "two multiplies per binding" and "rounding order
changes". Both were wrong in the safe direction, and both would have outlived the
commit. A wrong reason in the tree is a future task's false premise.

## A saving that equals the cost of drawing something is not a saving (R2-02 E4, 2026-07-28)

Two different mask edits, a fortnight of reasoning apart, both measured `STG`
−51,200 and both had stopped drawing the flower beds. 51,200 is what those 15
triangles cost to draw. The pattern is not rare and it is not obvious in a bucket
table, because a bucket that no longer does work looks exactly like a bucket that
does it faster.

**When a cut's measured saving is close to the full attributed cost of the thing
it was meant to speed up, treat "it stopped happening" as the leading hypothesis,
not the fallback.** The distinguishing evidence is cheap:

- a count of what still executes — triangles emitted, runs replayed, calls made —
  read from the same run that produced the buckets;
- the engagement variable, read from the **persistent** owner. E4 wasted an arm
  reading `sNdsNativeStageOwnerExecution.rigid_binding_mask`, which the stage draw
  zeroes on the way out, so it reads 0 at any frame boundary and fakes a fallback
  that is not happening. The live copy was in the workspace.

Related: a *doubling* with a correct picture is the mirror image — the signature
of a fail-closed guard disabling the optimisation. E4 arm B read `STG` 465,088
against a 224,320 control and rendered perfectly, because one binding failing a
rigid-constancy check drops the whole mask and invalidates the replay.

## Read the subsystem check's counters before designing against the subsystem (R2-02 E4, 2026-07-28)

`M3_NATIVE_STAGE_CHECK_OK` prints `cross_matrix_runs=5 cross_matrix_triangles=10
cross_matrix_foreign_corners=15` on **every Boundary run**, and has for the whole
campaign. Those ten triangles are all in the two flower beds, they are the only
cross-matrix geometry on the stage, and they are the entire reason the flowers
cannot enter the single-binding fast path and the entire reason they are
expensive — a cross-matrix triangle loads a composed matrix once per vertex.

R2-02 E3 and E4 spent three ROM builds, three ring dumps and two Boundary runs
converging on that number from the other direction, while it scrolled past in the
verifier output of each of those runs.

**Before proposing a change to a subsystem, read the counters its own checker
already prints, and reconcile your model against every one of them.** These lines
look like boilerplate because they pass every time; they are a free structural
census of exactly the thing you are about to modify. The corollary for authors:
when a checker prints a counter, something once needed it — treat an unexplained
non-zero as a fact your design has to account for, not as noise.

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

## A term over two counters incremented at different instructions asserts where the debugger stopped (bug #10 fold-in, 2026-07-28)

Boundary failed twice on the locked-30 pacing gate after the bug #10 cherry-pick,
with `logic/present = 422/212` and a phase histogram summing to 211. Both terms
were exact: `bp[2] -eq (2 * bp[3])` and `phaseSum -eq bp[3]`.

The ROM was correct. Two independent counters -- taskman's own and the fighter
route's -- both read **424** updates for 212 presents, an exact 2:1. Only the
pacing tuple's members disagreed with each other, and they disagreed by exactly
one iteration.

One realtime iteration touches four counters at four instructions
(`taskman_seam.c`): the two updates run, then
`ndsBattlePlayablePresentRealtimeFrame()` bumps `DrawCalls` (:4758),
`PresentedFrames` (:4790) and `PhasePresentCount` (:4888) in that order, and only
after that call *returns* does the loop add the committed updates to
`LogicFrames` (:7890, "count only updates committed to a presented frame"). A
stop anywhere in that span reads a legal skewed tuple. The observed one matches
exactly one point: between :4790 and :4888.

**An equality between two counters that are not incremented by the same
instruction is an assertion about the debugger, not about the build.** The
harness had already learned this once -- a comment above the adjacent
`taskmanPresentLead` records a previous author relaxing that term for this exact
reason -- but the fix stopped at the one term that had failed, leaving two more
counters on the far side of the same window.

The repair is not to loosen the bound. Enumerate the reachable stop phases and
reject everything else: with M completed presentations only four tuples exist, so
modelling them rejects five of the eight sign combinations, including ones an
equality cannot express (draw ahead *while* logic lags means a counter was
dropped). Where two stop phases alias -- a late stop and a genuinely dropped
update pair are numerically identical -- disambiguate with a counter incremented
by a *different* instruction, which is what `taskmanPresentLead` is for.

Corollary: when a text-pinning registry check guards the term, it must pin the
new contract, not the old spelling. `check-harness-registry.ps1` caught this
edit twice, which is the guard working.

## A null result must prove the instrument ran (R2-03 E5, 2026-07-28)

The E5 falsifier asked whether the fighter's per-run prepared facts hold still
enough to bake. First run, over 628 frames:

```
full=0x811c9dc5 stable=0x811c9dc5 light=0x811c9dc5   fullChg=0 stableChg=0
```

Three independent hashes, all perfectly constant across an entire match. Read as
a verdict it is the strongest possible KEEP. It was nothing: `0x811c9dc5` is the
FNV-1a offset basis, and the hook had never executed.

The hook was at the tail of `ndsRendererNativePreflightFighterHierarchy`, which
fills `hierarchy_runs[]`. That table belongs to `FAST_RUN_NATIVE_FIGHTERS`
(mode 7). Canonical is mode 9, `FAST_RUN_NATIVE_COMPLETE_STAGE`, which routes
the fighter through `...OwnerProduction` instead and never writes that table at
all. The instrument was measuring a path the shipping build does not take.

**Every counter-based falsifier needs a liveness counter that is zero if and
only if the instrument did not run, and it has to be read before the result
is.** Here that was `gNdsR2RunCallCount`. Without it, "the hash never changed"
and "the code never ran" are the same observation, and the first is a headline
while the second is a bug.

Two specific traps this repo keeps re-encountering:

- **An unchanged hash is an unchanged hash of nothing** when the accumulator
  starts at a known constant. Prefer seeding from something the run produced, or
  publish the fold count beside the digest.
- **A renderer path guarded by `gNdsRendererFastRunMode` is not the live path
  until checked.** Modes 7, 8 and 9 select structurally different owner entry
  points. Grep for the mode constant, not for the plausible-looking function.

The rehooked instrument, inside the function mode 9 actually calls, reported
28 changes in 1,500 frames. The real answer and the artefact both looked like
"constant"; only the call count told them apart.

## A census row is self time (R2-03 E11/E12, 2026-07-28)

E5 proved `ndsRendererNativePrepareProductionRun` is a pure function of
`run_index` and then declined to build the memo, in one line: *"~119 UV
writes/frame can't explain 21,504 ticks."* The arithmetic was right. The premise
was that the function's cost was inside the function.

| instrument | reading |
|---|---|
| E5's own bracket | ~21,500 |
| frame census symbol row | 22,205 |
| **four brackets inside the function** | **82,042** |

The difference is the texture resolver it calls out to, which the same census
charges separately to `ndsRendererHardwareResolveOrBindTexture` (18,803) and
`ndsRendererSyncTextureTile` (12,004). Two independent instruments agreed with
each other and both under-reported the work by a factor of four, because both
measure the symbol.

**When a candidate is rejected for being too small, check whether the instrument
measured the symbol or the work.** The rejection cost this campaign the largest
single fighter cut found so far — E12 took −32,724 ticks/frame off it.

### A generic fast path can be structurally unavailable to a specialized caller

The resolver already opens with a site cache that skips the whole resolve. It is
keyed on `state->source_command_site`, the address of the `Gfx` command being
interpreted — and the native fighter path does not interpret display lists, so
it has no site, misses on the first test, and has never once hit that cache.

Nothing reports this. The cache fails by returning `FALSE`, which is
indistinguishable from a legitimate miss. Ask it of every shared cache a native
path inherits: **what is the key, and does this caller have one?**

### A default-off `#if` does not hide a probe from a source-level checker

R2-02 F0's redundancy counters were guarded by a flag that defaults to 0, so
they could not change any shipping binary — and the next Boundary run failed
anyway. `check_nds_native_stage.py` polices which fields
`ndsRendererCommitNativeStageSegment` consumes by reading the source text, and
the probe had added six `prepared_run` reads to that closure.

Classifying them to make it pass would have asserted an immutability the probe
itself disproved. The probe was deleted and its answer kept in the write-up.

**Instrumentation inside a policed closure is a change to that closure.** Run
the widest relevant verifier on a probe-only commit too; "it is default-off"
covers the binary, not the checkers.

### Build the shipping flag combination, not only the instrumented one

E12 shipped a graduation (`NDS_R2_FIGHTER_RUN_MEMO := 1`) whose implementation
sat inside the `NDS_R2_FIGHTER_RUN_PROOF` guard, beside the E5 globals it was
derived from. Every build taken during development carried `PROOF=2`, so the
definitions were always present. The shipping combination is `PROOF=0, MEMO=1`
— definitions compiled out, call sites compiled in — and it failed to compile on
the first Boundary run after graduation.

That is the same root cause as the falsifier failure earlier the same day: both
times the change was validated only in the configuration that carries the
instrument.

**Graduating a flag changes which combinations exist. Build the cross product of
the new flag against the flags it was developed beside — at minimum
`(0,0)`, `(0,1)`, `(instrumented,0)`, `(instrumented,1)` — before the verifier
does it for you.** A shared helper or macro that two flags both use belongs
outside both guards.

### `volatile` keeps the compiler honest; it does not keep the linker honest

Three probes in one cycle read an ARM branch opcode (`0xEA80003B`,
`0xEA800009`, `0xEA80003D`) out of a counter that gdb resolved to a neighbouring
address. Each time the counter was `volatile u32` at file scope with external
linkage, so the compiler emitted it — and `--gc-sections` then dropped it,
because at that flag level nothing referenced it.

`__attribute__((used, retain))` was accepted without warning by GCC 15.2 and the
symbol still did not survive this linker. What worked was giving it a real
reference: `gNdsR2TexMemoVerifyFail = gNdsR2TexMemoVerifyFail;` in a function
that exists at the shipping flag level.

Two consequences:

- **A counter written only by a higher instrumentation level does not exist at
  the lower one.** A level-2 mismatch counter read from a level-1 build is not
  zero, it is garbage, and it will read as a *failure* about as often as a pass.
- **Verify with `nm`, not by reading the value.** `nm <elf> | grep <counter>`
  answers "does this symbol exist" directly; a plausible-looking number does
  not.

This is the linker-side twin of "a null result must prove the instrument ran".
That rule covered a hook that never executed; this one covers a counter that was
never linked.

### An intervention needs its own proof, separate from the counters (R2-03 E13/E14, 2026-07-28)

The family already has two members: a hook that never executed (E5) and a
counter that was never linked (E12). E13 added a third and it is the one the
existing rules do not catch.

The probe translated a fighter 20,000 units out of the view volume so the
rasterizer's share of the frame could be measured with the CPU path unchanged.
Everything about the instrument was healthy: the code ran, the counters were
linked, the liveness counts were non-zero. It reported **-13,632 WORK P50** —
"the rasterizer is 5% of a fighter" — a clean, quotable, entirely plausible
result.

The screenshot showed the fighter still standing on the stage. Writing
`root->translate` never reaches the hardware, because the per-frame DObj world
matrix cache serves the matrix it already built. Moving the write to the prepared
modelviews did not reach it either. The measured delta was build placement, and
it sat just above the 5,000-7,000 noise floor, which is exactly where an inert
intervention lands.

**A counter proves the instrument ran. It does not prove the intervention
happened.** An experiment that changes what is drawn proves that with a
screenshot; an experiment that changes what is computed proves it with a counter
that *must* move — E13's surviving arm suppressed a fighter's draw and its
hardware triangle count had to go to zero, which is unambiguous in a way a tick
delta never is.

Prefer the intervention whose engagement proof is structural over the cleverer
one whose engagement has to be argued.

**The same shape, inverted: an all-zero reading needs a positive control.** E14
sampled the GX command FIFO either side of the fighter submission and every
field read zero — entries, maximum, engine-busy. That is a real and important
finding (the geometry engine is idle) and it is also exactly what a probe
pointed at an unmapped address produces. The raw register word is therefore
OR-ed across every sample: `GXSTAT` bit 26 is "FIFO empty", so a live read over
a drained FIFO **must** set it, and a dead read cannot. The OR came back
`0x06009F00`, and only then was the zero a measurement.

**When the result is that something is zero, add the assertion that distinguishes
"measured zero" from "measured nothing" — before reading the result.**

### Verify the arm that ships, not the arm it replaces (R2-03 E17, 2026-07-28)

E17 was measured, screenshotted, committed and written up for the owner's
approval on the strength of a green Boundary run — of `NDS_R2_FIGHTER_HW_MTX=0`,
the default, which is the arm that *stops* shipping if the change graduates. The
candidate configuration had never been through the verifier at all.

This is the same defect as "build the shipping flag combination, not only the
instrumented one", approached from the other side. That rule was written about a
graduation whose shipping combination did not compile; this is a graduation whose
shipping combination was never verified. Both are the general failure of
validating a configuration other than the one that will run.

**Before asking for approval on a flagged change, run the widest relevant
verifier with the flag in the state approval would put it in.** `AGENTS.md`
already requires user-facing ROMs to be verifier-covered configurations; a
default-off candidate awaiting graduation is a user-facing ROM one `override`
away, and the run costs one build.

Mechanically, when a harness builds with fixed make arguments and has no flag
pass-through, flipping the Makefile default, running the profile, and restoring
it is sufficient and leaves no trace.

### Re-measure an estimate on a clean build before it becomes a target (R2-03 E18, 2026-07-28)

E16 priced the hardware-lighting cut at "most of 90,295 ticks/frame" off E15's
shade bracket, and put that number on the board as the phase's largest
opportunity. E18 measured it directly, by skipping the per-vertex loop the cut
would replace: **53,760**. The estimate was ~40% high.

Two compounding causes, both known at the time. The bracket came from a build
carrying the whole E15/E16 census, whose counters inflate what they enclose —
E15's own write-up says its absolutes are inflated 10-20% and only its ranking is
safe. And the bracket enclosed the per-epoch preamble as well as the per-vertex
loop, while the cut only replaces the loop.

**A number produced by an instrument is safe for ranking and unsafe as a target.**
Before a cut is scoped, budgeted, or handed to the next session, spend one build
disabling the thing the cut would replace and measure the difference on an
otherwise clean ROM. That arm also gives an unarguable engagement proof for free:
here the fighters render as black silhouettes, which no tick delta could be
mistaken for.

This is the mirror of E13's positive-control rule. That one covered a zero that
might mean nothing measured; this covers a large number that means less than it
says.

### A skip arm prices a phase only if what remains still works (R2-03 E19, 2026-07-28)

E18's one-build pricing method — delete the work a proposed cut would replace,
measure the difference — worked cleanly on the fighter's shade loop and was
pointed at the next ranked item, the epoch state spans. It reported **−251,520
FTR P50**, roughly five times the bracket, which would have been the largest
opportunity ever recorded in the phase.

It was fiction. The spans establish the texture, polygon-format and geometry-mode
state the emit requires. Without them every run is rejected before submitting, so
the delta was ~618 triangles a frame ceasing to exist. Hardware triangle counts
went from 320/306 per fighter to **8/0**.

**Verify a skip arm with a structural counter proving the work you are NOT
pricing still happened.** The tick delta cannot distinguish "this phase was
expensive" from "this phase was load-bearing and I deleted the payload". Here the
right counter was the emit's triangle count, which had to hold at its control
rate.

E18 was re-measured against the same check before its number was allowed to
stand — 320/306, identical to control — which is why its 53,760 survives and
E19's 251,520 does not.

Corollary: **a skip arm is a dependency test as much as a measurement**, and it
converts silently into one when the skipped work feeds the work being measured.
When it does convert, the finding is the dependency, not a saving.

### Count identity of the write, not identity of the target (R2-03 E21, 2026-07-28)

E20 measured that 64.2% of the fighter's state-delta applications re-apply a
delta index the frame had already applied, and put a ~35,000 tick/frame
opportunity on the board off that number. E21 asked how many of those repeats
write the *same operands*: **7.2% of applications, 11.2% of the repeats** —
~3,920 ticks/frame, below the placement floor, against a guard that would pay a
compare on all 194.4 applications to skip 14. Refuted, and it was E8's losing
shape.

A state machine that re-visits the same **knob** with **different values** is
doing necessary work. Counting knob-visits measures the shape of a replay, not
its waste. The two differed by a factor of nine here.

**Whenever redundancy is the premise of a cut, the counter must compare the value
written against the value already present** — not the address, index, slot, or
identifier being written to. If that comparison is awkward to build, that
difficulty is information: it usually means the "redundant" writes are not
actually equal.

Third instance this cycle of a plausible headline surviving until one more
counter was added, after E13's inert offscreen probe and E19's collapsed
geometry. Each cost one build to catch and each would otherwise have been acted
on.

### A redundancy's share is not its cost (R2-03 E22/E23, 2026-07-28)

E22 measured that **96.7% of the fighter's per-root matrix loads re-push an
identical projection** — 29 of 30 a frame. E23 built the skip, proved it engaged
on **93.8%** of loads with its own counters, and measured **−3,008 FTR P50**:
0.27% of the frame, under the placement floor, and almost exactly the ~2,900 a
first-principles count predicts. Reverted.

The repeated work was GX FIFO traffic, and E14 had already established that this
path never backpressures — the FIFO is empty at both ends of every submission.
**FIFO writes are stores, and stores are cheap.** A 96.7% redundancy rate on
cheap work is worth less than a 7% rate on expensive work.

So a redundancy count is only half a proposal. **Before pricing a cut from a
share, price one instance of the thing being repeated.** In this renderer, "how
often" is a bad proxy for "how much" whenever the repeated work is FIFO traffic;
it is a good proxy only for work that touches memory the CPU must wait on.

Two further consequences worth keeping:

- **Score each field a call writes, not the call.** E22's first pass compared
  projection and modelview jointly and reported *zero* redundancy, because the
  modelview genuinely changes every root. The 96.7% only appeared when the two
  halves were scored separately. This is E21's rule one level down: a call that
  writes several things needs one counter per thing.
- **A gain the instrument cannot resolve from zero is not a gain to keep.** The
  standing rule to accumulate small correctness-preserving wins applies to wins
  that can be seen. Keeping this one would have added a hot-path `memcmp` and 64
  bytes of BSS for an unresolvable delta, which is exactly how E8 cost +16,301.

### A vendor pack macro is not a contract; check what it leaves in the high bits (R2-03 E16, 2026-07-28)

E16's hardware lighting drew the fighters as black silhouettes. The cause was
libnds's `NORMAL_PACK`, which is
`(x & 0x3FF) | ((y & 0x3FF) << 10) | (z << 20)` — **the z argument is not
masked**, so a negative z sign-extends into bits 30 and 31. Harmless in
`GFX_NORMAL`, where those bits are unused. Fatal in `GFX_LIGHT_VECTOR`, where
they are the **light number**: every light vector went to light 3, which
`POLY_FORMAT` never enables, so light 0 kept its power-on zero vector and every
dot product was zero.

The register that shares a packing helper with another register does not
necessarily share its ignored bits. **When reusing a pack macro for a different
destination, mask every field yourself.**

The bisect that found it is the reusable part, and it is cheaper than reading
hardware docs harder:

1. Force the term you doubt to its maximum and everything else to zero.
2. Force the *other* term to its maximum instead. Here ambient-only lit the
   fighters brightly, which cleared the polygon format, the material register,
   the light colour and the normal stream in one build and left exactly one
   suspect.
3. Then add an engagement counter that records the **word actually written**, not
   just that the write happened. The count said 928 writes — correct — and the
   word said `0xE0100000` where `0x20100000` was intended. The count alone would
   have proved the code ran and left the bug invisible.

**When a value is written to hardware and the result is wrong, log the value, not
the fact of the write.**

### Convert every emit path, not the one you were reading (R2-03 E16, 2026-07-28)

The fighter has **four** production emit paths — `RawTextured`, `RawUntextured`,
`PrimitiveGroups` and `CrossRun`. E16's first build changed the two Raw ones and
left the other two writing `prepared->packed_color`, a field the new shade no
longer updates, so those runs drew with whatever the previous frame left.

This is E15's shape (instrumenting a path canonical mode 9 never takes) applied
to editing rather than measuring, and it is the second time this split has caught
someone out. **Before changing per-vertex emission, grep the field being replaced
and confirm every writer is converted** — the count of call sites is the check,
and it is one grep.

### Never `git checkout --` a file another agent is also editing (R2-03 E24, 2026-07-28)

Reverting E24 with `git checkout -- src/nds/nds_renderer.c` restored the file to
HEAD and silently destroyed a second agent's uncommitted `NDS_LAB_CULL_PROBE` /
`NDS_LAB_NO_CULL` bug-#10 probes living in the same file. Eleven commits this
cycle had carefully hunk-filtered around exactly that work; one destructive
command undid what all of them protected.

It was recoverable only because `New-Smash64DSSnapshot.ps1` had run 9 minutes
earlier and snapshots the **working tree**, not HEAD — so
`7z x <snapshot> src/nds/nds_renderer.c` plus a diff restored the four lost
hunks exactly. That is luck, not a procedure.

**To revert your own change in a shared dirty file, reverse your own edits** —
`git apply -R` a patch of your hunks, or re-edit them out. `git checkout --`,
`git restore` without `--staged`, and `git reset --hard` all discard everything
uncommitted in their path, and the repo's own CLAUDE.md already forbids them
without an explicit request. This is the concrete failure that rule exists for.

If it happens anyway: the newest snapshot on the Desktop is the recovery source,
and the diff against it should contain **only** the other agent's hunks — verify
that before copying anything back.

### When repeated cuts read null, look for the coupling (R2-03 E25, 2026-07-28)

Four consecutive R2-03 experiments produced nothing: E20/E21's state-delta guard
(refuted), E23's projection hoist (sub-floor), E24's baked action walk (null).
Each was individually well-measured and individually correct.

E25 found why. `ndsRendererNativeApplyStateDelta` invalidates the texture prepare
on every OTHERMODE / COMBINE / TEXTURE / GEOMETRY / IMAGE / TILE delta — 194.4
applications a frame — which forces the full `PrepareProductionRun` on 46.4 of
62.8 runs even though the texture memo hits 99.5%. The state replay (65,026) and
the prepare (42,281) are not two phases; they are one mechanism, one of which
dirties what the other rebuilds.

**A cut that optimises one side of a producer/invalidator pair reads as null,
because the other side restores the work.** E21 priced the delta *write* and
found it cheap — true, and beside the point: the write's cost is the
invalidation it triggers downstream, which no counter on the write can see.

So when two or three consecutive well-measured cuts in the same subsystem come
back null, stop cutting and **look for what re-dirties the state**. Concretely:
for any memo or prepared value, count its invalidations per frame alongside its
hit rate. A 99.5% hit rate on a value invalidated 194 times a frame is not a
working memo — it is a memo being consulted after every invalidation, and the
two numbers only reconcile when you look at both.

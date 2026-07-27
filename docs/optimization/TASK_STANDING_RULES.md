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


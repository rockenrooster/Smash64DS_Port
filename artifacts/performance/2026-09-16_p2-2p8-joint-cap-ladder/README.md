# P2-2p8 joint lever: the ladder, and why the skeleton half is not runnable

Owner, 2026-09-16: *"you are allowed to do test builds of experimental
optimizations. Less joints etc."* This is that measurement.

The gap is **-496,382 tk/fr** against WORK-H P50 1,584,128 and a gate of
1,120,000. The only per-fighter candidate the sizing left standing is joint
count: `…/2026-09-16_p2-2p8-gap-sizing/` puts joint-proportional work at
**347,132 tk/fr**, so even deleting all of it is 70% of the gap and a 2.48x cut
is 42%. That is a model. This document is what the hardware says.

## Two arms, because the obvious one aborts

### `NDS_LAB_JOINT_CAP` — a genuinely smaller skeleton

`Makefile:254`. At fighter setup
(`lbCommonSetupFighterPartsDObjs`, `src/port/reloc_backend_compat_shims.c`), any
common-part DObj whose container index is `>= N` is never created. The pruned
node leaves `array_dobjs[id]` NULL, so every descendant fails the existing
`parent == NULL` test and prunes with its ancestor, and every downstream
per-joint loop — pose, `ftParamUpdateAnimKeys`, the invalidate walk, the matrix
build, the draw traversal — simply never sees it. That is the real lever, not a
subsystem declining to look at a skeleton that still exists.

| cap | result |
|---|---|
| **12** | **Pruned nothing.** WORK-H P50 1,583,040 against a 1,584,128 baseline (-1,088, inside noise), and `gNdsFighterDLAllDrawP0/P1HardwareTriangleCount` read **349,031 / 333,618 — byte-identical to the baseline**. Identical triangle counts prove no part was removed, so the common-part container holds **12 or fewer** nodes. |
| **1** | **Engaged, then aborted.** `TICKFAULT __excpt_entry`, frame #1 `ndsBaseFTComputerSetFighterDamageDetectSize` at `decomp/BattleShip-main/decomp/src/ft/ftcomputer.c:7970`, faulting on `ldr r3, [r0, r3]` with **r0 = 0**. |

**The abort is the finding.** `ftcomputer.c:7972` reads
`parts->unk_dobjtrans_0x5` for the parts named in the fighter's
`damage_coll_descs` — Fox's list alone names joint ids 5, 6, 8, 9, 12, 14, 15,
19, 20, 24 and 25 (`decomp/.../209_FoxMain.c:332-343`). Removing a joint leaves
`fp->joints[]` holding NULL there and the CPU AI dereferences it.

So **a reduced skeleton is not a switch.** It is a per-fighter data
re-derivation: the hurtbox descriptor table, the effect joint ids
(`effect_joint_ids`), `joint_rfoot_id`/`joint_lfoot_id` and every animation
track binding have to be re-authored against the new skeleton. That is exactly
what makes the lever cost Sacrifice Order **2 and 3** together rather than 2
alone — it moves hit-part resolution, which is gameplay.

It also means the useful cap range is 2..11, and pruning anything in it risks
the same abort for whichever fighter names the pruned joint first.

### `NDS_LAB_POSE_JOINT_CAP` — the arm that runs

`Makefile:264`. Leaves every DObj in place and only declines to **evaluate**
pose entries at index `>= N` (`src/nds/nds_ft_pose.c`, inside `ndsFtPoseRun`).
No NULL is created, so the tree, the collision parts and the AI all still see a
complete fighter; limbs freeze instead of disappearing.

This prices the **animation half** of the joint lever and nothing else. The
matrix build, the invalidate walk and the draw traversal still run over every
joint, so whatever it measures is a **floor for the lever, never the lever**.

## Instrument discipline

Both arms carry two counters, and both counters are compiled in the `=0`
control as well:

| counter | meaning |
|---|---|
| `gNdsLabJointCapPrunedCount` / `gNdsLabPoseJointCapSkipped` | engagement — nonzero only in a capped arm |
| `gNdsLabJointCapKeptCount` / `gNdsLabPoseJointCapEvaluated` | the both-arms invariant — proves the walk ran at all |

All four are in `$memoryGlobals`
(`scripts/verify-p2-four-fighter-stress.ps1`), so every four-CPU run reports
them. The cap=12 run is the reason: it looked like a clean null result and was
actually a control wearing a candidate's name, and only the unchanged triangle
counts gave it away. An engagement counter that exists solely in the candidate
build cannot prove the control was the control.

**The control is a separate build, not the banked 1,584,128.** These counters
are `volatile u32` increments inside the pose loop, which runs ~17 times per
fighter per tick; comparing a capped arm against a counter-free baseline would
charge the instrument to the lever.

Every arm is built into a copy of the canonical four-CPU directory
(`builds/build-p2-jointcap`, copied from `builds/build-p2-fourcpu-tickhud`) so
the NitroFS payload is identical — a fresh build directory ships an incomplete
ROM, and the whole-directory packing makes that silent.

## Harness note, not a result

`verify-p2-four-fighter-stress.ps1 -Build` must be a **bare** directory name.
Passing `builds/build-p2-jointcap` resolves the ROM correctly but the
battle-core manifest path is composed differently and becomes
`builds\builds\build-p2-jointcap\battle-core\battle_core_manifest.json`. The
cap=12 measurement above completed and is valid; only the post-run manifest
check threw.


---

# RESULTS

## The two runs

Same build directory, same ROM sha `DE80E46BDCF1FD98`, 1,972 samples,
frames 2..1973, DLDI on. The arm differs from the control only in
`NDS_LAB_POSE_JOINT_CAP` (0 vs 1).

| bucket P50 | control | pose cap 1 | delta |
|---|---:|---:|---:|
| **WORK-H** | **1,588,928** | **1,624,832** | **+35,904** |
| FTR | 355,712 | 358,784 | +3,072 |
| STG | 344,768 | 344,448 | -320 |
| SRC | 555,584 | 577,344 | +21,760 |
| MISC | 249,472 | 260,608 | +11,136 |
| OTHR | 281,856 | 263,552 | -18,304 |
| WAIT | 252,928 | 235,584 | -17,344 |
| ALL | 1,677,952 | 1,678,144 | +192 |

Engagement, both sides:

| counter | control | pose cap 1 |
|---|---:|---:|
| `gNdsLabPoseJointCapLimit` | 0 | 1 |
| `gNdsLabPoseJointCapEvaluated` | 258,836 | 14,647 |
| `gNdsLabPoseJointCapSkipped` | 0 | **259,778** |

**94.7% of all pose-entry evaluation was deleted.** The arm engaged exactly as
designed.

## The number above is NOT a price, and here is the proof

The arms ran different matches.

| divergence witness | control | pose cap 1 |
|---|---:|---:|
| `gNdsFighterDLAllDrawP0HardwareTriangleCount` | 349,031 | 345,084 |
| `gNdsFighterDLAllDrawP1HardwareTriangleCount` | 333,618 | **352,444** |
| `gNdsGCDrawsActiveMax` | 203 | 193 |
| source spline descriptors normalized | 1 | 4 |

Freezing the pose changes what the fighters *do*: different positions, different
AI branches, different hits, different culling. Slot 1 drew **18,826 more**
triangles over the run, not fewer. So `+35,904` is a different workload and not
the cost of animation — the same trap that
[[route-ab-cannot-price-gameplay-change]] records, and it is why the divergence
witnesses are read before the bucket table and not after.

## What it does establish

The animation lane is the largest single joint-proportional component in the
gap sizing (`ftMainPlayAnim` inclusive **138,714 tk/fr**, 64% of simulation).
Deleting **94.7%** of its evaluation produced **no reduction in WORK-H at all** —
not a smaller win than predicted, not a win at the noise floor, but a frame
that did not get cheaper under a workload of comparable scale (ALL P50 moved
+192, 0.01%).

Whatever the pose player costs, removing almost all of it does not convert into
frame time. That is consistent with the rest of this campaign:
`ndsFtPosePlay` is flat and data-stall bound, and the work it sheds is replaced
by the work a differently-behaving match creates.

## Verdict on the joint lever

Combined with the two skeleton arms above and the geometry sizing:

| variant | measured outcome |
|---|---|
| fewer **triangles** | ceiling **-12,144** (2.4% of the gap); zero CPU work is per-vertex |
| fewer **joints** (skeleton) | **not implementable as a switch** — aborts the CPU AI on a NULL joint |
| less **animation** (94.7% of pose evaluation deleted) | **no WORK-H reduction**; arms diverge, so no price is readable |

**The per-fighter geometry and joint lever is spent.** It does not reach the
gate, and its largest sub-lane does not convert even at the impossible limit.

## Harness defects found, neither affecting the numbers above

1. `-Build` must be a **bare** directory name. A path containing `/` resolves
   the ROM correctly but composes the battle-core manifest path as
   `builds\builds\<name>\…` and throws after the match completes.
2. `-SetGlobals` pokes cost enough gdb time at boot to miss the harness's own
   `-RingStartRead` frame-1 requirement, so a lab lever cannot currently be run
   as a true same-ROM A/B through this harness. Pinning the value at build time
   is the workaround used here, which makes the comparison cross-build against
   the 14,080-tick floor — `+35,904` clears that floor, which is why the
   divergence witnesses and not the floor are what disqualify it as a price.
3. The post-run report throws `Error formatting a string: Format specifier was
   invalid.` **after** writing every JSON, so the run's data is complete and the
   exit code is not. Cosmetic, but it makes a good run look like a failed one.

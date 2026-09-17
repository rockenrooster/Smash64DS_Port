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

## Status

Control (both caps 0) building and running. The pose-cap arm follows.

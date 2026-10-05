# 2026-10-05 FTParts latch clears skip clean subtrees

Two latch clears run per fighter per tick (`ndsFTParamsInvalidateSubtree`,
after the animation and after the physics move). Each walked every FTParts of
the subtree, one line fill per part. A census build (`build-gate-dirty`,
gdb count of set latch words and mode-1 parts before each clear) found
4,177 latched parts in 535,802 visited, and 20,799 of 21,437 clears with
nothing set.

Change: every writer of an FTParts latch (the gmcollision.c entry points that
reach the float latch walk, the fixed CFX ring's joint prepare, the stub joint
build) bumps `gNdsFtPartsLatchWrites` first. A flat walk records the counter
value at its last clear (words, and modes for `reset_mode`), so a clear with
an unchanged counter is skipped. Only compiled live where every writer is
wrapped (NDS_P2_JOINT_RESIDENT with NDS_P2_HURTBOX_REJECT). Same-ROM A/B word
`gNdsFtPartsCleanSkip`. The lab sweep ROM carries an opt-in verify
(`gNdsLabFtPartsCleanVerify=1`, counter `gNdsLabFtPartsCleanSkipViolations`).

Also in this commit: `ftParamUpdateAnimKeys` skips its MObj loops on a batch's
held tick (`gNdsMObjTickMul == 0`), where both wrappers already returned.

## Results

Official gate ROM (`smash64ds-p2-fourcpu-tickhud-hwtri`, 1,960 frames):

| run | build | word | P50 | P95 | over 2 VB | digest |
|---|---|---|---|---|---|---|
| gate-cs0 | build-gate-1005h | CleanSkip=0 | 831,488 | 1,148,160 | 114 | base |
| gate-cs1 | build-gate-1005h | CleanSkip=1 | 823,616 | 1,141,696 | 109 | identical |
| gate-i | build-gate-1005i | shipped (+ held-tick MObj skip) | 820,544 | 1,137,344 | 107 | identical |

Same ROM, cs0 -> cs1: paired median -6,656 over 1,902 frames.

Lab sweep ROM (`build-lab-sweepall10`, verify always on in that build, so its
WORK is ~10K over the shipped config): `a10-g0..8` against
`2026-10-05_mobj-30hz/a8-g0..8`: replay digest identical on all nine VS
stages. Saffron with the verify on: 0 violations.

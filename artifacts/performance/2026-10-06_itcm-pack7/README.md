# 2026-10-06 ITCM seventh pack (freed by the A/B-word deletions)

The A/B-word deletions shrank ITCM residents (word checks out of the wall
sweep, hurtbox and stage paths): lab ITCM had 1,256 B free, the shipping link
1,400 B. Over-gate profiles on Castle and Saffron
(`artifacts/task37-census/castle-prof16`, `saffron-prof16`, 512 frames from
400) and Sector Z (`sz-szprof09`) agree on section D's densest main-RAM
functions. Admitted (`linker/nds_hot_text.ld`, 1,048 B): ndsMObjStepScaled,
mpCollisionCheck{Floor,Ceil,LWall,RWall}LineCollisionDiff,
gcParseMObjMatAnimJoint, ndsMatrixFloatToFix, ndsMPAiFloorSnapshot,
ndsProjectRatioQ22, ndsBaseGcAddDObjAnimJoint, lbCommonGetTreeDObjNextFromRoot.
Not admitted: ndsRendererAdapterNdlDispatchEffect (1,300 B inlined in the
shipping ROM) and libgcc's __aeabi_lmul / __clzdi2 (archive members; they need
the Task 9 extraction list).

Placement only: digests identical everywhere. Shipping ITCM 32,392 of 32,736 B.

| Run | P50 | P95 (WORK-H) | over | paired median |
|---|---|---|---|---|
| Castle `g0-q17` | 828,928 | 1,182,336 | 133 | -4.3K (vs q15 sweep) |
| Saffron `g7-q17` | 861,568 | 1,190,848 | 157 | -2.2K |
| Sector Z `sz-q17` | 867,200 | 1,174,016 | 130 | -6.6K (vs q16) |
| gate `gate-q17` | 786,752 | 1,072,448 | 71 | -2.1K |
| 4 x Yoshi `yo-q17` | 871,040 | 1,227,328 | 198 | -1.9K |

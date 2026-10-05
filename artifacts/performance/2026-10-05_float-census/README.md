# 2026-10-05 Soft-float census by call site

Owner, 2026-10-05: "Software floating point should not exist, fixed point
only, its taking ITCM space."

Lab ROM `build-lab-fcen1005`: the clean four-CPU lab (`NDS_LAB_FOURCPU_WORDS=1`)
with `NDS_TASK9_FLOAT_CENSUS=1` -- every libgcc float entry point linked through
`--wrap`, and (new) every call site's exact count in a 4,096-slot table keyed
by the return address (`src/nds/nds_task9_float_census.c`). gdb dumps the table
at presented frame 1,900 (`scratchpad fcen.ps1`); `fsites.py` (copied here) maps
each return address to its function and routine and weights it by the
routine's measured ticks per call. This ROM already has the fixed-point
hurtbox decision and the deferred-effect-desc fix (2026-10-05).

| Run | Roster / stage | float calls / frame | est. ticks / frame |
|---|---|---|---|
| `sites-yoshi.txt` | 4 x Yoshi, Dream Land | 6,796 | 391,253 |
| `sites-gate.txt` | gate roster, Dream Land | 4,540 | 249,343 |
| `sites-sector.txt` | gate roster, Sector Z | 5,858 | 309,839 |

Static scope (gate ELF): 10,358 soft-float call sites in 1,071 functions; libgcc
float in ITCM ~1.9 KB (fadd 400 B, fmul 408 B, fdiv 352 B, sqrtf 220 B,
conversions and compares).

Largest classes (4 x Yoshi): the lean renderer's float local builder for
animation-lock joints (`ndsRendererAdapterBuildSourceFighterLocalMtx`, 59.5K --
Yoshi's motions set `is_use_animlocks`, which also sends every Yoshi hurtbox
test down the float latch walk: `func_ovl2_800ED490` 20K, `gmCollisionSetMatrixNcs`
11K, `gmCollisionGetWorldPosition` 10K, `gmCollisionSetInvertMatrix` 9K,
`gmCollisionTestRectangle` 6.5K); map collision (~50K); matrices (~26K); CPU
AI (~24K); trig (~19K); camera (~17K); vectors (~15K); particles (~14K).
Sector Z adds stage animation (~54K: `ndsBaseGcPlayDObjAnimJoint`,
`gcParseDObjAnimJoint`, `syMatrixTraRotRpyRSca`).

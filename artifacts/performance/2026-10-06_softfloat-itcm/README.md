# Soft float out of ITCM: priced (2026-10-06)

Owner directive: "Software floating point should not exist, fixed point only,
its taking ITCM space." Before converting callers, this prices what the
soft-float routines' ITCM residency is worth.

## E1: every soft-float routine in main RAM (placement only)

Lab ROM `build-lab-clean1006q36e1` = q35 plus: the Task 16 `__aeabi_fadd/fsub`,
`fcmp*` and `i2f` assembly and the Task 9 phase-2 `fcmpeq` moved from
`.itcm.*` to `.text.*` sections, `nds_float_conv.c` (`ui2f`, `__floatsisf`) to
`.text.ndsFloatConv`, and `NDS_TASK9_FLOAT_MAIN_MEMBERS` listing all six libgcc
members (`fmul`/`fdiv` and `f2iz` join the rest in main RAM). ITCM drops from
32,624 to 30,896 B (1,728 B freed). Not committed.

| config | digest diff | P50 | P95 | over | paired median |
|---|---|---|---|---|---|
| gate | 0 | 776,576 -> 793,536 | 1,059,072 -> 1,082,624 | 62 -> 75 | +16,704 |
| g0 (Castle) | 0 | 808,192 -> 828,672 | 1,149,824 -> 1,180,480 | 115 -> 137 | +19,520 |
| sz (Sector Z) | 0 | 859,392 -> 882,688 | 1,171,264 -> 1,190,656 | 130 -> 168 | +24,192 |

Baseline: `artifacts/performance/2026-10-06_old-machinery/*-q35.csv`.

## Census of the E1 placement (Castle, frames 200-584)

`artifacts/task37-census/castle-prof36e1`, section D (ITCM admissions by
non-memory stall per byte, over the 384-frame window). Evicted, the soft-float
routines become the best ITCM candidates in the program:

| routine | bytes | non-mem stall/byte | next best non-float candidate |
|---|---|---|---|
| `__aeabi_fadd` | 400 | 18,051 | `__aeabi_idivmod` 6,148 (32 B) |
| `__aeabi_i2f` | 92 | 11,744 | `lbParticleDrawTextures` 5,669 (16 B) |
| `__aeabi_fcmplt` / `gt` / `eq` | 52 / 52 / 36 | 8,791 / 8,691 / 8,060 | `ndsRendererAdapterNdlDispatchEffect` 3,625 |
| `__aeabi_fmul` (+ `fdiv`, one member) | 408 + 352 | 6,149 / 4,261 | `ndsF32ToFixed` 3,182 |
| `__aeabi_fcmple` / `ge`, `f2iz` | 52 / 52 / 92 | 4,510 / 4,318 / 4,477 | |

So there is no placement win: soft float is the hottest code in the frame and
earns its ITCM (about 20K cycles a frame of residency on these stages, ~70K
cycles a frame executed on Castle). Its ITCM can only be freed by removing the
calls, i.e. by converting callers to fixed point, largest first:

| caller (Castle, calls x calibrated cost) | est. cycles/frame |
|---|---|
| `mpCollisionGetFCCommonFloor` (fsub 54, fdiv 13, fmul 13, fadd 13) | 5,344 |
| `ndsBaseMPProcessUpdateMain` (fadd 71, fcmp 31, fsub 16) | 4,156 |
| `ndsMPFCSegmentCrossesKernel` (fsub 68, fmul 30) | 3,761 |
| `lbParticleUpdateStruct` | 3,672 |
| `ndsBaseMPProcessCheckTest{R,L}WallCollision` | 6,706 |
| `func_ovl2_800ED490` (hurtbox matrix compose) | 3,100 |
| `ftMainProcUpdateInterrupt` | 3,064 |
| `ndsAttackRangeTest` | 2,770 |
| `ndsFtPoseParse` | 2,191 |
| `syVector{Add,Diff,Norm,Mag}3D` | 6,401 |

Total estimate 87.9K cycles a frame over 315 callers (fcallers.py on the
census, same per-call costs as the earlier sim-side classification). The map
collision cluster alone is ~25K.

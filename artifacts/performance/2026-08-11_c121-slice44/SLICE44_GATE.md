# Slice 44 — stride the stage's transform revalidation. KEPT.

Cycle 121. `NDS_R2_STAGE_VALIDATE_STRIDE=8`. Two ROMs from one tree at
`80091257120+dirty(8)`, both `NDS_R2_BOTH_CPU=1`, 1600 samples from frame 438,
`-RingDump`, DLDI **ON**, melonDS sha `DE80E46BDCF1FD98`.

## What it deletes, and why the obvious fix was the wrong one

`--pc-detail ndsRendererAdapterStageWorldSourceKeyMatches` on the c120 profile
(`artifacts/performance/2026-08-11_c120-lane/pcdetail-sourcekeymatches.txt`),
86,390 calls over 1600 frames = **54.0 a frame**, 24,736,188 cycles = 7,719
tk/frame:

| pc | cycles | insns | cyc/insn | instruction | what |
|---|---:|---:|---:|---|---|
| 0x020362f4 | 2,831,532 | 102,715 | **27.57** | `ldrb r1, [r1, #4]` | `xobj->kind` |
| 0x02036266 | 1,962,368 | 86,389 | **22.72** | `ldrb r3, [r1, r3]` | key `xobjs_num` |
| 0x02036254 | 1,930,170 | 86,389 | **22.34** | `ldrb r2, [r0, r3]` | `dobj->xobjs_num` |
| 0x0203625c | 1,492,238 | 86,389 | **17.27** | `ldr r3, [r0, #76]` | `dobj->vec` |
| 0x020362f6 | 1,463,734 | 102,716 | 14.25 | `ldrb r2, [r7, r3]` | key `xobj_kinds[i]` |

**9.7M of the 24.7M is four cold loads.** The inlined byte-compares of
translate/rotate/scale sit at 1.0–7.6 cyc/insn: the compare was never the cost.
It is 42 DObjs and their XObjs pulled into a 4 KB D-cache every frame to prove
nothing changed. A cheaper compare buys nothing; not touching the objects does.

`ndsRendererAdapterBuildPersistentStageWorldMatrix` adds 28,892,601 cycles =
**9,017 tk/frame** on top, and it does not build anything either — it walks a
parent chain, matches the same source keys, and returns the matrix it already
had.

## Shape

Round-robin, **not** "full sweep every 8th frame". The second shape makes 12.5%
of frames expensive and P95 lands on one of them and reads flat — see the
`cluster-where-the-percentile-lives` lesson. Spreading 42/8 checks across every
frame is what moves P50 and P95 together, which the STG numbers below confirm.

## Gate

| bucket | control (STRIDE=0) | stride (STRIDE=8) | Δ |
|---|---:|---:|---:|
| **WORK-H P50** | 948,736 | **931,648** | **−17,088** |
| **WORK-H P95** | 1,246,464 | **1,210,560** | **−35,904** |
| STG P50 | 188,736 | 168,832 | −19,904 |
| STG P95 | 196,864 | 172,672 | −24,192 |
| WORK P50 | 960,448 | 943,488 | −16,960 |
| WORK P95 | 1,410,944 | 1,388,288 | −22,656 |
| FTR P50 | 294,592 | 296,704 | +2,112 |
| ALL P50 / P95 | 1,118,208 / 1,678,656 | 1,118,208 / 1,678,656 | 0 / 0 |

**2.0× and 4.2× the ±8,544 cross-build floor**, and the win is in the bucket it
was aimed at: STG. FTR is flat. ALL is unchanged because ALL is
VBlank-quantised — the memory `all-is-a-quantized-gate` says so, and this is
another instance of it, not a contradiction.

VBlank intervals, max 20 on both: control `2:1621 3:361 4:38 5+:18`, stride
`2:1644 3:343 4:35 5+:16` — better in every bucket. `slips=0` both arms.

**The control reproduces the banked gate**: 1,246,464 against the banked
1,244,480, a difference of 1,984, well inside the floor.

## Engagement — both sides

Slice 43 read `Roots = 32.06/frame` as full engagement while the producer was
declining every owner, so this slice counts producer and consumer:

- `gNdsR2Slice44RigidChecks` **6,627**, `gNdsR2Slice44RigidSkips` **46,387**.
  Ratio 6,627 / 53,014 = **0.12500**, exactly 1/8.
- 53,014 = **26 × 2,039**, and the run reports 2,038 frames. The sweep therefore
  ran on *every* frame — which is also the proof the rigid mask **never
  demoted**, since a demotion early-returns and would have frozen both counters
  at 26 × k. (`gNdsRendererTask36RigidConstancyMismatchCount` is
  `NDS_RENDERER_PROFILE_LEVEL == 1` only, so it does not exist in a tick-HUD
  ROM; the ratio is the available proof and it is a stronger one.)
- `gNdsR2Slice44StaleReuse` **28,518** against 14 stale-eligible dynamic
  bindings × 2,039 frames = 28,546 predicted. **99.9%.**

## Fidelity

Frame-locked pairs at `time_remain` 50 and 48 (`-ExactTimeRemain 50
-SoftwareRenderer`), `artifacts/visibility/2026-08-11_slice44-stride/`:

- Game viewport (8,55)–(408,350), **118,000 px, 0 differing, max channel delta
  0** on *both* tics. The only bytes that differ in the whole PNG are the
  tick-HUD text — the instrument.
- Both arms read **DMG 130% (Mario) / 51% (Fox), STOCK ×1** at the same locked
  tic, so the arms played the same match (`route-ab-cannot-price-gameplay-change`).
- Boundary: `Boundary verification profile passed.`, zero `Exception:` lines.

## What was kept on purpose

The guard is strided, not deleted. Cycle 52 and the 0x4F/0x50 XObj kinds each
cost a build proving a frozen matrix is a real fidelity bug. A binding that
stops being rigid is still caught — within 8 frames instead of 1 — and the mask
still drops. Two consequences were handled rather than assumed:

1. **Demotion is one-way within a topology.** The old code rebuilt the mask from
   the constant every frame and let the sweep clear it again; that was only
   equivalent because the sweep was complete. A partial sweep must not re-arm a
   binding it did not look at.
2. **The topology rebuild now drops the stage world cache.** Entries are keyed
   on DObj address. A recycled heap address after a scene re-entry would make a
   stale entry *match* — harmless while `validated_frame == frame` forced a
   rebuild every frame, a previous topology's matrix under the stride.

## Not graduated everywhere

`smash64ds-battle-playable-hwtri` (published P1) and the
tickhud/proof/results-lab block, which must stay flag-identical to it. **Not**
the BUGS.md #9 floor arms: those exist to compare the rigid path against the CPU
path and a stride would confound them.

## Banked

**WORK-H P50 931,648 / P95 1,210,560.** Gap to the 1,120,380 target:
**~90,180** (was ~124,100).

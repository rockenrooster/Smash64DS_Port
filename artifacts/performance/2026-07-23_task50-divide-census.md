# Task 50 — Hardware divider / sqrt: E0 caller census

Date: 2026-07-23 · Branch: `codex/task50-hardware-divider` (from master `61469f7`)
Census source: `artifacts/task37-census/census.json` (500,810,896 cycles / battle frame)
Caller disassembly: `arm-none-eabi-objdump -d` on `build/battle-playable-hwtri/*.o`

## Stage E0 result: STOP at E0

The eligible (render-side, battle-hot) divide+sqrt ceiling is **~0.55%** of the
battle budget under generous static-site attribution, and the realistic
*recoverable* figure is below the ~0.5% bar once the DS divider's async busy-wait
is counted. The `__aeabi_ddiv` "free win" the spec anticipated is absent: every
double-divide caller is cold in the battle census (0 cycles) except
`ftDisplayLightsDrawReflect` (0.0634%, 2 sites). The honest finding is the
deliverable. **No shipped code. E1 does not run.**

## The ceiling, and why it is not the estimate

`2.31%` is the summed cost of the divide/sqrt *callees* (`__ieee754_sqrtf`,
`sqrtf`, `__aeabi_fdiv`, `__aeabi_ddiv`, `__udivsi3`, `__divsi3`, plus the
`__aeabi_*divmod` integer entry points). The census attributes cost to the
*callee symbol*, never to the *caller*, so splitting it requires the call graph.

Three facts pull the recoverable figure down, in order of impact:

1. **Most divide/sqrt cost is in gameplay code the state hash forbids changing.**
   `gmcollision`, `mp*` stage collision, `ftComputer`, `ftMainProcPhysicsMap`,
   shield/jump/damage physics, `wp*` weapon collision, `lbCommonSim2D` — these are
   the hot callers and they are ineligible by construction. Their result reaches
   the Task 9 state hash.

2. **The hot render-side callers are a small set, and the DS divider is async.**
   The genuinely hot eligible callers are the billboard-look-at matrix builders
   (`ndsRendererAdapterBuildDObjLocalMatrix` kinds 33–40, `reloc_backend_renderer_dl.c:631–711`)
   and the lighting-direction normalize (`ndsRendererHardwarePrepareLitDirection`,
   `nds_renderer.c:7742–7760`). The DS hardware divider is **asynchronous with a
   busy-wait**; at the battle frame's call density (a few dozen divides/frame,
   not thousands) the per-call overhead can equal or exceed the inline IEEE float
   path it replaces. melonDS cannot referee divider timing (device-only class,
   `TASK_STANDING_RULES.md` device-test economy), so a win is not even measurable
   in the standard A/B.

3. **The double-divide "free win" is not present in battle.**
   Every `__aeabi_ddiv` caller is cold (0 cycles in the battle census):
   `syMatrixTraRotRpyD`/`RotRpyD`/`TraRotD`/`RotD` (the `D`-suffix matrix
   builders) are cold — the adapter reaches the `*F`/`*R` float/radix paths and
   the `kind` switch in `reloc_backend_renderer_dl.c:954–1083`, not the `*D`
   double builders. The only warm ddiv caller is `ftDisplayLightsDrawReflect`
   (0.0634%, 2 sites, render-side) — too small to chase. There is no accidental
   `1.0`-promotion class to fix bit-exact.

## Eligibility table — render-side (eligible) callers, ranked by census cost

| # | caller (function) | object | cost (cyc) | % | sites | eligible? | note |
|---|---|---|---:|---:|---:|---|---|
| 1 | `ndsFighterMarioFoxDLAllDrawForSlot.constprop.0` | scene_backend | 9,195,432 | 1.8361% | 11 fdiv | **yes** | fighter display-list build (render matrix/texture) |
| 2 | `ndsRendererAdapterBuildDObjLocalMatrix` | scene_backend | 4,910,138 | 0.9804% | 6 fdiv + 6 sqrtf | **yes** | billboard look-at kinds 33–40 (`reloc_backend_renderer_dl.c:631–711`) |
| 3 | `ndsRendererHardwarePrepareLitDirection` | nds_renderer | 1,668,430 | 0.3331% | 3 fdiv + 1 sqrtf | **yes** | light-direction normalize (`nds_renderer.c:7742–7760`) |
| 4 | `syMatrixLookAtReflectF` | sys_matrix | 1,021,652 | 0.2040% | 3 fdiv + 3 sqrtf | **yes** | look-at reflection matrix (render) |
| 5 | `syMatrixPerspFastF` | sys_matrix | 363,667 | 0.0726% | 5 fdiv | **yes** | perspective projection (render) |
| 6 | `ndsRendererAdapterPrepareMaterialSegment.isra.0` | scene_backend | 331,501 | 0.0662% | 11 fdiv | **yes** | material/texture-coord derivation |
| 7 | `ftDisplayLightsDrawReflect` | ftdisplaylights | 317,758 | 0.0634% | 2 ddiv | **yes** | reflection lighting (render) |
| 8 | `grWallpaperCalcPersp` | grwallpaper | 249,136 | 0.0497% | 4 fdiv | **yes** | wallpaper perspective |
| 9 | `syUtilsArcTan.part.0` | utils | 164,312 | 0.0328% | 20 fdiv | **yes** | atan trig helper |
| 10 | `syUtilsArcTan2` | utils | 73,397 | 0.0147% | 2 fdiv | **yes** | atan2 helper |
| | `guNormalize` | libultra_gu_normalize | 0 | — | 1 fdiv + 1 sqrtf | yes | cold in battle |
| | `syMatrix*D` (8 variants) | sys_matrix | 0 | — | 24 ddiv | yes | **all cold** — double path not reached in battle |
| | `syMatrix*F` (Persp/LookAt variants not above) | sys_matrix | 0 | — | many | yes | cold in battle |

The rows with measurable cost sum to **~3.65% of whole-function cost**, but the
divide/sqrt calls are a small fraction *inside* each of those functions (most of
`DLAllDrawForSlot`'s 9.2M is matrix multiply and DL emission, not divide). The
divide/sqrt cost *attributable to these callers* is the share of the callee
symbols' 11.45M cycles they generate — estimated by static-site density at
**~0.55% of total** (`__aeabi_fdiv` ~0.26% + sqrt ~0.29%).

## Eligibility table — gameplay-side (ineligible) callers, the dominant cost

These reach the Task 9 state hash and cannot change value. They account for the
**majority** of the 2.31% divide/sqrt cost.

| caller (function) | object | cost (cyc) | % | reason ineligible |
|---|---|---:|---:|---|
| `battleship_ftAnimParseDObjFigatree` | ftanim | 4,101,850 | 0.8190% | animation/figatree parse → fight state |
| `ndsStageMPSweepFloorLoopSweep` | scene_backend | 1,855,167 | 0.3704% | stage collision (`mp*`) |
| `ftMainProcPhysicsMap` | ftmain | 1,144,872 | 0.2286% | fighter physics |
| `mpCollisionGetFCCommonFloor` | scene_backend | 619,956 | 0.1238% | floor collision |
| `ndsStageMPAdjustFloorLoopWallSweep` | scene_backend | 675,100 | 0.1348% | wall/floor sweep |
| `gmCameraUpdateInterests` | gmcamera | 829,226 | 0.1656% | camera tracking = gameplay |
| `gmCollisionSetInvertMatrix` | gmcollision | 194,263 | 0.0388% | collision matrix |
| `mpProcessUpdateMain` | scene_backend | 381,608 | 0.0762% | collision |
| `gmCameraLookAtFuncMatrix` | gmcamera | 379,236 | 0.0757% | camera (gameplay) |
| `mpCollisionCheckProjectFloor` | scene_backend | 195,834 | 0.0391% | collision |
| `gmCollisionTestRectangle` | gmcollision | 146,628 | 0.0293% | collision |
| `ndsFighterDisplayContractProjectTarget` | scene_backend | 212,765 | 0.0425% | targeting (gameplay) |
| `gmCameraDefaultFuncCamera` | gmcamera | 265,707 | 0.0531% | camera |
| `ftComputer*` (8 functions) | ftcomputer | ~170,000 | ~0.034% | AI (gameplay) |
| `wpDisplayMapCollisions`/`HitCollisions` | wpmanager | — | — | weapon collision |
| `lbCommonSim2D`/`CheckAdjustSim2D`/`Mag2D`/`NormDist2D` | wpmanager | — | — | 2D physics sim |
| `ndsBaseFTCommonGuard*` (4), `Jump*`, `Damage*`, `Twister*`, `Dead*`, `Rebirth*`, `Walk*`, `Catch*`, `Rebound*`, `DownBounce*` | ftcommon_* | — | — | fighter physics/state |
| `ftParamGetHitStun` | scene_backend | 130 | 0.0000% | hitstun (gameplay) |
| `gcGetAObjTrackAnimTimeMax`/`gcParse*AnimJoint` (7) | sys_objanim | — | — | animation timing (gameplay) |

These sum to **~2.32% of whole-function cost** and contain the densest, hottest
divide/sqrt sites in the battle frame. They are exactly the cost the hardware
divider *cannot* touch.

## Out-of-battle callers (not in the battle budget at all)

The opening movies and menus (`mvOpening*MotionCameraProcUpdate`, `mnTitle*`,
`mnPlayersVS*`, `lbTransitionMakeCamera`, `ndsOpeningRoomRenderDLPreview`) call
divide/sqrt heavily but run **outside** the battle frame the census profiles.
Excluded entirely — converting them would not move the battle number.

## Why the `--wrap` approach (Tasks 9/16) is wrong here — confirmed

Tasks 9/16 could `-Wl,--wrap=__aeabi_fadd` globally because their replacements
were bit-exact golden re-implementations. `__aeabi_fdiv`/`__ieee754_sqrtf` carry
the majority of their battle cost inside gameplay code (collision, physics,
camera tracking, AI). A global wrap changes those values and fails the Task 9
state hash immediately. Site-specific conversion is the only viable shape — and
this census shows the eligible site set is too small to justify a campaign task.

## Verdict

- **Eligible render-side ceiling: ~0.55%** (static-site attribution of callee
  cost, generous). Realistic recoverable is below the ~0.5% bar.
- **No double "free win":** the only warm ddiv caller is 0.0634%; the `syMatrix*D`
  double path is cold in battle.
- **Device-only measurability:** even a candidate site cannot be proven a win in
  melonDS (divider async timing is device-only class).
- **Outcome: STOP at E0.** E1 (helper + per-site conversion) does not run. The
  hardware divider is not worth a campaign task at this eligible size; this
  census is the deliverable. The branch is the checkpoint; nothing merges.

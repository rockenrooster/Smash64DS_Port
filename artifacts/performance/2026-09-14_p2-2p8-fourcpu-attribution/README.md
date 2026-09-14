# P2-2p8 four-CPU attribution — Phase A

Date: 2026-09-14. Phase A is attribution only; no `src/`, `include/`, generator, Makefile, or verifier source was edited by this session.

## Identity and provenance

- Task-entry HEAD was `6fa07edc9bfc018fc1d6d28853121962491ebf34`. The standing Boundary artifact is stamped `b65d9c00cab`, but `b65d9c00cab..6fa07edc9bf` changes only verification/docs files, so their runtime tree is identical.
- Another session advanced shared HEAD to `a1209354b8c070a4eb64b7d835f59063834e54b3` at 08:06 while this task was running. The only runtime delta in that commit is the P2-6 1P/menu-walk playback block in `taskman_seam_battle_host.c`; this target compiled `NDS_P2_1P_GAME=0` and `NDS_P2_MENU_WALK=0`, so that block is absent from this ROM.
- The shared working tree nevertheless carried uncommitted renderer/reloc/menu/roster edits while the ROM was built. Therefore the new PC census is **current-tree snapshot evidence, not a landed-6fa cost delta**. It must not be used to attribute a gain/regression to those dirty edits.
- Instrument files checked after the build were clean: `Makefile`, `scripts/run-task37-profile-census.ps1`, `scripts/task37_census.py`, `include/nds/nds_task37_profile.h`, and `src/port/taskman_seam_battle_host.c`.
- Profile ROM SHA-256: `67F603CFF0D5179B88305550B4018FB2D33FC2EF9F3BCDCA2919EB7ADC4DF9FF`.
- Profile ELF SHA-256: `941890BBB6C27DD2263126C80EB43DDEA2A143CE64B896E3253E86B8EEC1B8A8`.
- Build: `TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-2p8-profile NDS_TASK37_PROFILE=1 NDS_TASK37_PROFILE_START=1400 NDS_TASK37_PROFILE_FRAMES=128 NDS_TASK37_PROFILE_PER_FRAME_REGION=1 NDS_TASK37_PROFILE_RESULTS=0`.
- Target-owned flags include four-CPU stress, compact battle fighters, kinds Donkey/Samus/Link/Kirby, `NDS_RENDERER_PROFILE_LEVEL=0`, `NDS_TASK56_FIGHTER_PRIMITIVES=2`, and native R2 hardware triangles. The link audit printed `NATIVE_ONLY_PASS ... actual_inputs=245`.

## Measurement window and isolation

The repo-local whole-match ring `artifacts/verification/p2-2-fourcpu-tickhud.csv` has 1,972 presented rows and is a different run from the later morning Boundary text artifact. Its `WORK-H` is P50 2,415,680 / P95 3,472,192 ticks, with 1,885/1,972 (95.6%) above the 1,120,380 gate. Active frames 200..1800 contain zero below-gate frames.

The PC census covers presented frames **1401..1528** (`START=1400`, 128 regions). This is active fighting: frame 1442 is near the ring P50 at 2,415,744 WORK-H; frames 1401 and 1481 are near P95 at 3,450,304 and 3,495,040. The run used runner slot 9 only. Before launch there were zero melonDS processes; the orchestrator lock was created for the run, every poll saw exactly one slot-9 melonDS, and the lock was removed immediately after completion. The profile contains 235,294,706 instructions, 733,848,924 cycles, and 129 regions (region 0 is outside the requested census window).

The requested `-SplitOverGate` was attempted. It correctly rejected the split because **128/128 active census regions are over its gate**, and the standing active ring has no representative fighting frame below the product gate. A clean-vs-over split would therefore invent a non-representative control. For tail attribution only, an offline 7/128 (~P95) split was used: regions 13,38,53,73,80,88,110 = presented frames 1413,1438,1453,1473,1480,1488,1510. Marked non-idle cost is 8,321,609 cycles/frame versus 5,582,545 control, a 2,739,064-cycle/frame premium.

## Whole-match ring ownership

Using the repo analyzer's percentile definition (`floor((n-1)*q)`):

| lane | P50 ticks | P95 ticks | note |
|---|---:|---:|---|
| WORK-H | 2,415,680 | 3,472,192 | whole product work minus HUD |
| FTR | 1,180,352 | 1,252,224 | already exceeds the whole-frame gate at P50 |
| STG | 348,672 | 397,952 | large body, almost flat in hot-vs-clean excursion |
| MISC | 260,096 | 505,792 | material P95 excursion |
| FTR+STG+MISC | 1,761,344 | 2,015,232 | renderer-side approximation; mean 1,685,147 = 68.8% of mean WORK-H |
| SRC | 568,448 | 1,574,528 | mean 717,410 = 29.3% of mean WORK-H |
| GCRA | 562,688 | 1,568,704 | `gcRunAll`; contains the nested simulation lanes |

Hot-minus-clean WORK-H is 1,686,496 ticks: FTR contributes +1,053,670 (62.5%), SRC +460,310 (27.3%), MISC +170,262 (10.1%), STG -5,409. Inside SRC, the excursion is led by SITR +172,476, SPHD +96,925, SCPU +74,208, and SOBJ +49,072. These are nested according to `nds_startup.h`: do not add SRC/GCRA/SINT/SCPU or SGCO to one another.

**Renderer submission vs source simulation:** by whole-match means, `FTR+STG+MISC` is 68.8% and SRC is 29.3% of WORK-H; together they explain 98.1%. At P50 the renderer approximation alone is 1.761M ticks, so the body cannot be closed by chasing rare source events alone.

## Current top 40 PC symbols by cycles

`armWaitForIrq` is idle and is excluded from work percentages below, but is retained as rank 1 because the requested ranking is by raw cycles.

|#|symbol|cycles|ticks/frame (128)|% total|
|--:|---|--:|--:|--:|
|1|`armWaitForIrq`|75,436,960|294,676|10.28%|
|2|`ndsRendererExecuteNativeFighterOwnerProduction`|20,563,943|80,328|2.80%|
|3|`__aeabi_fadd`|20,031,350|78,247|2.73%|
|4|`__aeabi_fmul`|16,058,067|62,727|2.19%|
|5|`ndsRelocNativeAssetAddress`|14,277,913|55,773|1.95%|
|6|`ndsRendererNativePrepareProductionRun.constprop.0`|12,618,247|49,290|1.72%|
|7|`ndsFighterMarioFoxDLAllDrawForSlot.constprop.0`|12,253,770|47,866|1.67%|
|8|`memset`|10,177,430|39,756|1.39%|
|9|`ndsFtPoseUpdate`|9,591,318|37,466|1.31%|
|10|`ndsRendererNativeRebuildProductionRunUv`|9,358,278|36,556|1.28%|
|11|`ndsRendererNativeApplyStateDelta`|8,919,337|34,841|1.22%|
|12|`ndsPreviewFileOffset`|8,817,138|34,442|1.20%|
|13|`ndsFtPosePlay`|8,556,911|33,425|1.17%|
|14|`memcpy`|8,073,542|31,537|1.10%|
|15|`ndsRendererCommitNativeStageSegment`|7,466,990|29,168|1.02%|
|16|`ndsPlatformRenderDebugHud`|7,252,851|28,331|0.99%|
|17|`ndsRendererNativeEmitProductionPrimitiveGroups`|7,034,943|27,480|0.96%|
|18|`get_fat.isra.0`|6,920,805|27,034|0.94%|
|19|`ndsRendererSyncTextureTile`|6,818,031|26,633|0.93%|
|20|`ndsFTParamsInvalidateSubtree`|6,002,221|23,446|0.82%|
|21|`ndsRendererR2RunTextureMemoApply.constprop.0`|5,812,346|22,704|0.79%|
|22|`ndsRendererMtxMulAffine20p12`|5,794,810|22,636|0.79%|
|23|`__aeabi_fdiv`|5,460,282|21,329|0.74%|
|24|`ndsRendererAdapterBuildNativeProductionInputs.constprop.0`|5,446,374|21,275|0.74%|
|25|`ndsFighterPacketEmitCornerTail`|5,138,397|20,072|0.70%|
|26|`ndsRendererMtxMul20p12`|5,122,089|20,008|0.70%|
|27|`lbParticleDrawTextures`|5,119,608|19,998|0.70%|
|28|`ndsFighterPacketCmd`|4,980,246|19,454|0.68%|
|29|`ndsRendererAdapterBuildPersistentStageWorldMatrix.constprop.0`|4,866,204|19,009|0.66%|
|30|`ndsRendererNativeStageEmitNoZTriangle`|4,812,617|18,799|0.66%|
|31|`ndsRendererNativeShadeProductionActions.constprop.0.isra.0`|4,775,935|18,656|0.65%|
|32|`ndsFighterPacketCmd1`|4,749,983|18,555|0.65%|
|33|`ndsRendererLoadHardwareSplitMatrices`|4,713,683|18,413|0.64%|
|34|`ndsRendererNativeStageBeginRun`|4,672,311|18,251|0.64%|
|35|`ndsRendererNativeEmitProductionPrimitiveGroupsPacket`|4,662,537|18,213|0.64%|
|36|`f_lseek`|4,282,140|16,727|0.58%|
|37|`mpCollisionGetFCCommonFloor`|4,134,014|16,148|0.56%|
|38|`glBindTexture`|4,122,443|16,103|0.56%|
|39|`ndsRendererSubmitParticleQuad`|4,003,433|15,638|0.55%|
|40|`ndsRendererAdapterSubmitStageDL`|3,968,912|15,504|0.54%|

## Memory-stall share by tier

Idle spin is excluded. Memory stalls remain ~42% of both ITCM and main cycles, so code placement alone cannot erase the body.

| tier | cycles | CPI | non-memory stall | memory stall |
|---|---:|---:|---:|---:|
| `.itcm` | 209,287,002 | 2.24 | 13.4% | 42.0% |
| `.text.hot` | 8,558,711 | 6.92 | 37.5% | 48.0% |
| `.text.hot.draw` | 15,594,797 | 2.29 | 21.9% | 34.4% |
| `.main` | 424,861,767 | 3.18 | 26.9% | 41.6% |

## Owner/subtree attribution

Subtree reports are static-reachability censuses: their totals are upper bounds when a reachable symbol is also called elsewhere. Ring buckets remain the bracket authority.

- **Fighter draw:** actual FTR ring P50/P95 = 1,180,352 / 1,252,224. In the fighter-draw subtree, direct-child exclusive costs include native owner production 320,557 tk/frame, initial matrices 62,042, native root execution 51,341, source-world compose 39,678, production-input build 21,275, plus 47,866 in the draw root. The subtree total 1,534,624 is only an upper bound because 947,733 tk/frame is shared-reachable work.
- **Native fighter production:** root 80,328 tk/frame; prepare-production-run subtree 310,304; state delta 152,693; primitive-group emit 27,480; shade 22,833; split matrices 18,413; packet primitive emit 18,213; apply material 13,107; packet replay 11,540. This is the largest coherent steady body to attack.
- **Fighter update/pose:** `ndsFtPoseUpdate` self is 37,466 tk/frame and its static subtree is 289,003 upper-bound tk/frame; `ndsFtPosePlay` self is 33,425 and subtree 221,152. On the top 7 profile frames, pose-update premium is 43,516 ticks/frame and pose-play also rises materially, tied to AObj load/normalization.
- **Source collision/interaction:** whole-ring GCRA P50/P95 = 562,688 / 1,568,704. Current soft-float attribution puts 10,086,036 cycles (39,399 tk/frame average) specifically in collision/stage-MP callers. `gmCollisionGetWorldPosition`, `gmCollisionSetInvertMatrix`, `gmCollisionTestRectangle`, `gmCollisionGetFighterPartsWorldPosition`, and `mpCollisionGetFCCommonFloor` are the largest named collision soft-float callers. The old 238K-cycle L7 estimate is not current evidence; that experiment was measured and reverted.
- **Stage:** actual STG ring P50/P95 = 348,672 / 397,952 and its hot-vs-clean excursion is -5,409 ticks, so it is body but not the P95 switch. Static stage-submit reachability totals 913,419 tk/frame upper-bound with 856,374 shared-reachable; direct root is 15,504.
- **Texture binding/resolution:** `ndsRendererHardwareResolveOrBindTexture` is only 8,635 tk/frame self now; selected adjacent self costs are SyncTextureTile 26,633, texture memo apply 22,704, `glBindTexture` 16,103, BindTextureName 9,085, and TextureColor only 599. The old August 15.1% resolve symbol is no longer the current architecture's dominant owner.
- **Matrices:** name-matched matrix/build self-time totals 45,154,000 cycles = 176,383 tk/frame. Leaders are affine multiply 22,636, 4x4 multiply 20,008, persistent stage world 19,009, DObj XObj build 15,324, cell multiply 11,338, stage no-Z load 10,465, N64-to-DS load 8,334, hardware pair load 7,827. This overlaps fighter/stage owner totals and must not be added to them.
- **Memcpy/memset:** 18,250,972 cycles = 71,293 tk/frame = 2.77% of non-idle work. Largest callers: texture-capture clear/copy 10,297 tk/frame, DObj XObj matrix 6,635, `f_read` 6,575, projected-depth vertex 4,950, N64-to-DS matrix 3,827, persistent stage matrix 3,789, stage source-key capture 3,678, material snapshot 3,352, MVP recalc 3,229, fighter draw 3,171. On the 7 tail frames, memcpy+memset are 123,682 ticks/frame; `f_read` alone accounts for 47,112 ticks/frame and +44,166 premium.
- **Soft-float:** 48,358,032 cycles = 188,899 tk/frame = 7.34% of non-idle work across all subsystems. Collision/stage-MP is 39,399 tk/frame, matrices/transform 35,207, other gameplay 22,197, renderer 17,333, animation 14,797, particles 8,801, CPU AI 6,050. Tail soft-float is about 235,865 ticks/frame, but its largest callers are particle/matrix/animation code rather than one gmcollision leaf.
- **Reloc/addressing:** `ndsRelocFindLoadedFileContaining` is 8,094 tk/frame self / 12,038 subtree upper-bound now, versus the old 1.7% symbol. Broader `ndsRelocNativeAssetAddress` + `ndsPreviewFileOffset` is 98,953 tk/frame upper-bound and is worth specialization. The real P95 reloc opportunity is repeated AObj file/normalization work on the top frames, not the generic containing-file search alone.
- **Triangle/vertex/GX:** the old generic `SubmitHardwareTriangle` / `HardwareSubmitVertex` hot pair is gone. Current fighter primitive-group emitters cost 27,480 + 18,213 tk/frame self; stage no-Z triangle+vertex cost 18,799 + 13,808; hardware `EndBatch` is 14,548; `HardwareWriteVertex16Words` is 888. Task 56 is already mode 2 and previously banked only ~11K ticks for stripification. The GX write/batch floor is real, but current CPU preparation/state work is much larger than direct FIFO submission.

## Per-fighter derived cost

Profile level 0 has no per-slot owner tick ledger, so the PC census cannot honestly assign measured ticks to Donkey/Samus/Link/Kirby separately. The exact generated low-detail production cardinalities are the best available owner counters:

| fighter | triangles | dense vertices | runs | triangle share | geometry-proportional FTR P50 / P95 estimate |
|---|---:|---:|---:|---:|---:|
| Donkey | 314 | 342 | 56 | 22.14% | 261,376 / 277,291 ticks |
| Samus | 387 | 316 | 36 | 27.29% | 322,141 / 341,756 ticks |
| Link | 226 | 353 | 52 | 15.94% | 188,124 / 199,579 ticks |
| Kirby | 491 | 586 | 47 | 34.63% | 408,711 / 433,598 ticks |

The last column is a **geometry-proportional derivation, not measured per-slot timing**; pose, run/state count, textures, specials, and cache state make true slot costs non-linear. It does establish that Kirby owns the largest immutable geometry payload and Link the smallest triangle stream.

## P95 mechanism

The 7/128 costliest-frame premium is 2,739,064 cycles/frame = 1,369,532 ticks/frame. Largest discriminators are `get_fat` +162,767 ticks/frame, `f_lseek` +101,001, `ndsRelocNormalizeFighterAObj16File` +54,203, pose update +43,516, `armCopyMem32` +39,552, memcpy +39,115, `move_window` +35,617, mutex unlock +34,576, AObj16 command words +30,131, and `f_read` +27,500. The tail therefore has a concrete residency/animation-acquisition component distinct from the steady renderer body.

## Ranked Phase-B levers

No source change was made in Phase A, so the numbers below are **measured cost available to remove**, not banked A/B savings.

Audio is first in the product sacrifice order, but it is not a body lever here: the local whole-match ring has AUD P50 3,392 / P95 141,056 ticks while FTR P50 alone is 1,180,352. Audio degradation can flatten bursts, not close the steady four-fighter gap.

1. **Bake/specialize per-fighter owner preparation, state and matrix work.** Actual FTR is 1.180M P50 / 1.252M P95. Direct exclusive fighter-draw children expose about 495K tk/frame in native production + initial matrices + native-root + source-world compose + input build before shared work; production itself is dominated by prepare-run (310K) and state-delta (153K) reachable cost. Expected recovery target: hundreds of thousands of P50 and P95 ticks if those repeated per-fighter decisions become generated packet/state data. Risk: low gameplay risk and low visual risk if generated output/hash/geometry stays exact; this is the first lever.
2. **Compensated 30 Hz simulation remains on the table, but only as a product-contract lever.** GCRA is 562,688 P50 / 1,568,704 P95. A perfect alternate-tick cut has a theoretical ceiling of ~281K P50 and ~784K P95 ticks on skipped source work, before 60 Hz-required hitbox/input/audio exceptions. Risk: highest of the top levers because cadence can change gameplay; PROJECT_GOAL permits it only when behavior stays substantially the same. Renderer body still exceeds budget by itself, so 30 Hz simulation cannot be the only fix.
3. **Make fighter animation assets resident/pre-normalized across active battle.** This is a P95 lever rather than a P50 lever: the top 5.5% frames carry 1.370M ticks/frame extra, led by FAT seek/read, AObj16 normalization, pose update and copies. Removing only the named FAT/seek/normalize/read/copy premiums offers several hundred thousand ticks on those P95 frames. Risk: low gameplay/visual risk if byte-identical clip data and pose output are retained; RAM/heap pressure is the main constraint.
4. **Matrix exact-reuse/precompute inside the native owners.** Named matrix self-time is 176K tk/frame, with additional helper costs already counted in soft-float/memops. A realistic first target is tens to low hundreds of thousands of steady ticks, especially where per-fighter owner matrices are rebuilt unchanged. Risk: low if Q/fixed-point parity and camera/live-joint dependence are preserved. This overlaps lever 1 and must be measured as one A/B, not added arithmetically.
5. **Reduce per-fighter primitive/state work before reducing geometry.** Current primitive emit self is ~46K tk/frame plus state/material/batch work; direct stage tri/vertex is ~33K and EndBatch ~15K. Task 56 strips are already enabled and historically saved ~11K ticks. Expect only tens of thousands from emitter micro-cuts unless Phase B removes actual per-fighter runs/triangles. Geometry/LOD reduction can buy more but spends visual fidelity, which is allowed before simulation under the sacrifice order and therefore requires measured visibility evidence.
6. **Remove broad soft-float selectively, not by reviving the old L7 patch.** All soft-float is 189K tk/frame average, but current collision/stage-MP callers account for only ~39K. The old fixed-point collision leaf was measured and reverted (+534 SRC cycles/frame while losing 6,481 in FTR+STG). Expected current gmcollision-only recovery is tens of thousands unless a caller-level rewrite also removes matrix/collision work. Risk: gameplay/numeric fidelity; require source-equivalence bounds.
7. **Cut redundant memcpy/memset by caller.** Total is 71K tk/frame average. Renderer/matrix callers dominate body; file-read/pose callers dominate the tail. Expect tens of thousands steady, with a larger tail benefit if combined with lever 3. Risk: low if lifetime/initialization invariants remain explicit.
8. **Texture resolve/bind is now secondary.** Selected texture self-time is ~84K tk/frame across resolve, sync, memo apply, bind and name-bind; the old resolve symbol alone is only 8.6K. Expect tens of thousands, not the August-sized win, unless Phase B proves duplicated state across four fighters. Risk: low to moderate visual correctness; exact material/texture epochs must stay identical.
9. **Generic reloc containing-file lookup is a micro-cut; broader asset addressing is the real body target.** `FindLoadedFileContaining` is only 8.1K self / 12K reachable tk/frame. Native asset address + preview offset reaches ~99K upper-bound. Prefer exact-base/owner-local offsets or scene-resident references; do not spend a package on the old generic search alone. Risk: low if provenance/range checks remain at load/registration boundaries.

## Commands and evidence

Key build command: `make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-2p8-profile NDS_TASK37_PROFILE=1 NDS_TASK37_PROFILE_START=1400 NDS_TASK37_PROFILE_FRAMES=128 NDS_TASK37_PROFILE_PER_FRAME_REGION=1 NDS_TASK37_PROFILE_RESULTS=0` after acquiring the shared build lane and setting devkitPro/devkitARM/MSYS2 PATH. Build log: `builds/build-p2-2p8-profile/build-profile.log`.

Key profile command: `scripts/run-task37-profile-census.ps1 -Target smash64ds-p2-fourcpu-tickhud-hwtri -Build build-p2-2p8-profile -NoBuild -PerFrameRegion $true -SplitOverGate -StartFrame 1400 -Frames 128 -RunnerSlot 9 -OutDir artifacts/performance/2026-09-14_p2-2p8-fourcpu-attribution/profile-current`.

Evidence under this directory:

- `ring-analysis.txt` — whole-match bucket/excursion/ceiling analysis.
- `profile-driver.log` — profile harness and the documented all-over-gate split rejection.
- `profile-current/arm9-profile.meta.txt`, `arm9-profile.regions.csv`, `arm9-profile.csv` — raw current PC census.
- `profile-current/census-base.txt/.json`, `census-top7.txt/.json` — whole-window and P95-tail census.
- `profile-current/softfloat-callers*`, `memops-callers*` — helper-to-caller attribution.
- `profile-current/subtree-*.txt` — fighter, source, stage, texture, matrix, reloc and pose reachability reports.
- `profile-current/tail-leaf-callers-presence1.txt` — tail helper callers.

## Gaps carried to Phase B

- The current PC census is a concurrent shared-tree snapshot. Do not claim any dirty renderer/reloc edit as landed cost without a clean synchronized A/B after those sessions settle.
- A representative active clean-vs-over product-gate split is impossible because active fighting has no below-gate control frames. The 7/128 split is a P95-tail classifier, not a product-gate clean control.
- Per-slot ticks are not available at renderer profile level 0. The Donkey/Samus/Link/Kirby row above is a generated-work derivation only.
- No Phase-A candidate has an A/B recovery number yet. Phase B should start with one coherent fighter-owner bake/precompute package, then a separate animation-residency package if the P95 tail remains.

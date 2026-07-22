# Task 32 — draw-path hot-text grouping

Status: **KEEP; retail histogram improved and the published target is enabled**.

Source HEAD: `04baf2ca885e5877f30fbd004603c1d32d1ed071` plus the focused
Task-32 diff. Boundary remains `battle_playable_realtime`, mode 163. This is
placement only: no renderer logic, ITCM content, Thumb mode, pixels, gameplay,
wallpaper, fighter, or imported BattleShip source changed.

## Draw census

The release-equivalent census ROM is
`builds/build-task32-pc-census/smash64ds-battle-playable-hwtri.nds`, SHA-256
`1111D6AF2ADDAF07ABA3B1C16D6071181866E7D1445BF182B46E31B0B69593D8`.
Two GDB/MI PC censuses each take 450 immediate interrupts at every second
presentation over frames 600–1498, covering about 30 seconds of source-time
combat. One starts at `gcDrawAll`; the other starts at the enclosing native
draw owner so pre-`gcDrawAll` preparation is represented. Any host delay was
rejected because unthrottled melonDS could cross a phase before stopping.

Raw evidence:

- `artifacts/performance/2026-07-20_task32-draw-pc-samples.csv`, SHA-256
  `F23B9DB7E50EA1140C63B018AAA3B11804D231BC9D348313AA1F220745F142EE`;
- `artifacts/performance/2026-07-20_task32-draw-owner-pc-samples.csv`, SHA-256
  `03BFAADEC930933FD2397BAEAA3E4537BC0E3AF45390C61813EA61DED62B85F9`.

The PC count is sampled time, not a call counter. The map/code ranking supplies
the independent size/call-frequency axis.

| Main-RAM candidate | Combined samples / 900 | Profile-1 bytes | Decision |
|---|---:|---:|---|
| `ndsRendererMtxMul20p12` | 198 | 312 | selected |
| `ndsRendererLoadHardwareMatrixPair.isra.0` | 23 | 632 | selected |
| `ndsRendererNativeStageLoadNoZMatrix` | 23 | 460 | selected |
| `ndsRendererMtxLoadN64ToDS20p12` | 21 | 256 | selected |
| `ndsRendererMtxMulAffine20p12` | 20 | 616 | selected |
| `ndsRendererCommitNativeStageSegment` | 20 | 2,276 | selected; eight segment boundary owner |
| `ndsRendererSyncTextureTile` | 20 | 212 | selected |
| `ndsRendererAdapterBuildDObjWorldMatrix` | 12 | 320 | selected |
| `ndsRendererNativeApplyStateSpan` | 12 | 276 | selected |
| `ndsRendererNativeStageEmitNoZTriangle` | 11 | 572 | selected |
| `ndsRendererNativeStagePrepareRun.constprop.0` | 9 | 1,472 | selected; 54-run preparation owner |
| `ndsRendererNativeApplyStateDelta.part.0` | 9 | 620 | selected |
| `ndsRendererAdapterFindDObjWorldMatrix` | 4 | 144 | selected |
| `ndsRendererHardwareGetLightShadeLut` | 23 | 400 | not selected; shared lower-level path after the stage set filled the budget |
| `ndsRendererAdapterBuildNativeMaterialSnapshot` | 10 | 2,388 | not selected; four calls and poor bytes/sample |
| `ndsRendererAdapterBuildNativeStageTopologyStamp.constprop.0` | 4 | 1,392 | not selected; poor bytes/sample |

The dominant raw emit, production shade, fighter executor, submit-vertex, and
float helpers sampled in ITCM and were intentionally not duplicated or moved.
The 13 selected functions account for 382/900 samples. An initial 14-function
set included the 124-byte, once-per-frame `gcDrawAll`, but profile-1 expansion
of the commit owner made the section 8,292 bytes. The linker rejected it. The
entry was removed; all sampled stage leaves remain.

## Placement proof

`NDS_TASK32_DRAW_HOT_TEXT` defaults to 0. When enabled, the generated linker
fragment feeds `.text.hot.draw` immediately after the retained Task-17
`.text.hot`; `__main_start` continues to point at Task 17. Both sections have
independent 8 KiB assertions.

- profile 1: 13 symbols / 8,168 bytes, VMA
  `0x02002758..0x02004740`;
- release-equivalent profile 0: 13 symbols / 8,060 bytes, VMA
  `0x02002758..0x020046D4`;
- Task 17 remains exactly 5,016 bytes at main-load start `0x020013C0`;
- flag-off has no nonzero `.text.hot.draw`, and all 13 symbols remain in
  `.main`.

`scripts/check-task32-draw-hot-text.ps1` proves the exact input-section/object
set, symbol survival, section order, size, 0–7-byte `.main` input-alignment
padding only, and unchanged Task-17/main-load start.

## Synchronized melonDS A/B

Profile 1, affine wallpaper off, mode 9, retained generated segment 0, static
texture AOT 1, Task-16 compare/i2f/addsub 1/1/1, frames 600–607, eight samples.
The final A2 control and candidate share HEAD and an identical focused status.

| Metric | control P50/P95 | candidate P50/P95 | delta P50/P95 |
|---|---:|---:|---:|
| complete stage owner | 461,088 / 461,376 | 465,344 / 465,728 | **+4,256 / +4,352** |
| complete draw | 857,312 / 860,736 | 861,760 / 865,024 | **+4,448 / +4,288** |
| present active | 862,112 / 865,536 | 866,592 / 869,888 | **+4,480 / +4,352** |
| source update pair | 203,616 / 217,984 | 203,456 / 217,792 | -160 / -192 |
| loop wall | 1,120,256 / 1,120,256 | 1,120,256 / 1,120,320 | 0 / +64 |

The small host regression repeats exactly in A2. This does not promote the
candidate: melonDS does not model the retail ARM9 instruction cache, while the
Task-32 mechanism exists only to improve cache locality. Retail hardware is the
referee. Semantic traces and generated-segment GX words are equal; synchronized
frame 607 is exactly 0/49,152 changed top-screen pixels with mean delta 0.00.

Host A/B artifacts:

- control A2 ROM `805C38CB74EBBDF5B4C7307DC007F6DFDE00AE315C5BDFD292108EFDF026A0B8`;
- candidate ROM `0457ACF4AF9FDF16F21122E3009DF42C0C8B0D01670668E7889A28BB1B877279`;
- `artifacts/performance/2026-07-20_task32-control-a2-early600-607.json`;
- `artifacts/performance/2026-07-20_task32-candidate-early600-607.json`;
- `artifacts/visibility/2026-07-20_task32-control-a2-frame607.png`;
- `artifacts/visibility/2026-07-20_task32-candidate-frame607.png`.

## Retail decision

The profile-1 device configs differ on exactly one generated line. Both use
mode 9, static textures, Task 16 1/1/1, generated segment 0, lower phase HUD,
and affine wallpaper 0. The HUD shows typed UPD/DRW/ACT/LOOP, 2/3/4/5+ buckets,
maximum, Git hash, and `DHT 0/1`.

- control:
  `builds/task32-draw-hot-device-pair/smash64ds-task32-draw-hot-control.nds`,
  SHA-256
  `28CCE18784D8AA413C2E58A9811547258A905C06DCFFCF5C39455BDCCF6D17EC`;
- candidate:
  `builds/task32-draw-hot-device-pair/smash64ds-task32-draw-hot-candidate.nds`,
  SHA-256
  `69B0050E6CECBBBA78FDFC43AF0945A0549049380349C69824623D976C914016`.

The repo-local captures prove the HUD/config distinction:
`artifacts/visibility/2026-07-20_task32-device-control-hud.png` (`DHT 0`) and
`artifacts/visibility/2026-07-20_task32-device-candidate-hud.png` (`DHT 1`).

the owner ran the pair on retail hardware in one session. The preserved device
photos are:

- control `DHT 0`:
  `artifacts/visibility/2026-07-20_task32-retail-control-dht0.jpg`, SHA-256
  `7981529E133041615B7163166A66974B2DC9140E866C7036AAF053BC668B7516`;
- candidate `DHT 1`:
  `artifacts/visibility/2026-07-20_task32-retail-candidate-dht1.jpg`, SHA-256
  `6436DA33D6CCB8A0E5A6C60F4833A833813AF6F2E27F95B82DACC1B2C500CD26`.

| Retail HUD | DRW latest | VBI 2 | VBI 3 | VBI 4 | VBI 5+ | max |
|---|---:|---:|---:|---:|---:|---:|
| control, `DHT 0` | 1,677,760 | 0 | 186 | 347 | 67 | 7 |
| candidate, `DHT 1` | 1,658,560 | 6 | 187 | 331 | 60 | 7 |

The photos contain 600 control and 584 candidate classified intervals, so raw
counts are not compared as though the sample sizes were identical. Normalized
4+ intervals improve from 69.00% to 66.95% (-2.05 percentage points), 5+
improves from 11.17% to 10.27% (-0.89 points), and 2–3 improves from 31.00% to
33.05% (+2.05 points); maximum remains 7. The photographed instantaneous DRW
row is also 19,200 ticks lower, but it is a half-second sample and is supporting
rather than synchronized evidence. Both photos show 15.0 FPS / 30.0 UP and
zero slip at the photographed instant. This satisfies the task's retail
histogram KEEP rule. `NDS_TASK32_DRAW_HOT_TEXT` remains generic-default 0 but is
forced to 1 by the published and release-equivalent freeze-diagnostic targets.

## Verification

- focused GBI/static fixtures: pass;
- profile-0/profile-1 flag-on placement checker and profile-1 flag-off checker:
  pass;
- post-promotion profile-0 placement: 13 symbols / 8,060 bytes, Task 17 still
  5,016 bytes at `0x020013C0`;
- `verify-dev-fast.ps1`: pass after promotion;
- `verify-boundary.ps1`: pass after promotion, including published ROM contract
  and dated visibility capture;
- promoted battle ROM: 14,692,352 bytes, SHA-256
  `B73D9BDBF36C780C44F4898213A069FFF250716F2B77C6773C22DA28B8BB98D2`;
- no imported/shared gameplay translation unit changed, so no full Regression
  shard was required.

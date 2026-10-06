# Old machinery deletion (2026-10-06)

Owner directive: "Delete Old machinery." Each step removes a concluded A/B arm
(route word, lean level, toggle) and keeps only the shipped arm. A deletion must
leave the replay digest identical and the time neutral; the paired medians are
the same-trajectory comparison (frames matched by index, digest-identical runs).

Configs: `gate` = Four-CPU gate (Dream Land), `g0` = Castle
(`gNdsLabFourCpuGkind=0`), `sz` = Sector Z (`gNdsLabFourCpuGkind=1`). Lab ROM
`smash64ds-p2-fourcpu-tickhud-hwtri`, clean (`NDS_LAB_FOURCPU_WORDS=1`).

## q33: camera A/B arms

Deleted: the float camera chain and `gNdsR2CameraFixedEnabled` (Q20.12 shipped
since 2026-08-16), the lean level word `gNdsCameraMatrixLeanEnabled` (level 2
shipped since cycle 103), the SELECT toggle build (`NDS_R2_CAMERA_FIXED_TOGGLE`)
and its HUD indicator, and the three Makefile knobs. `gmCameraLookAtFuncMatrix`
is now the former fixed body at level 2 semantics (projection Mtx still taken
from the graphics heap, W2b).

Baseline q32 = `artifacts/performance/2026-10-06_fixed-collision/*-q32.csv`.

| config | digest diff | P50 | P95 | over | paired median |
|---|---|---|---|---|---|
| gate | 0 | 778,048 -> 776,896 | 1,062,656 -> 1,059,840 | 65 -> 65 | -1,088 |
| g0 | 0 | 811,840 -> 810,048 | 1,159,808 -> 1,156,480 | 119 -> 116 | -2,496 |
| sz | 0 | 863,808 -> 858,880 | 1,168,064 -> 1,163,648 | 134 -> 128 | -2,880 |

Neutral to slightly positive (the level/route loads and the dead float arm's
code are gone from the hot path), digest identical. Kept.

## q34: folded compile-time routes and three shipped runtime words

Deleted, all with the shipped arm kept and every probe updated:

- compile-time routes that folded to the shipped arm at 0: `NDS_R2_MP_ROUTE`
  (`gNdsR2MPRoute`, the endpoint / yakumono / line-kind memos),
  `NDS_R2_RELOC_ALIAS_ROUTE` (`gNdsR2RelocAliasRoute`, the alias-scan
  reorder), `NDS_R2_TILESYNC_ROUTE` (`gNdsR2TileSyncRoute`, the tile-sync
  memo) and `NDS_R2_ANIM_ITCM_ROUTE` (`gNdsR2AnimItcmRoute`, two copies of
  the anim kernel);
- runtime words read every use in the shipped ROM: `gNdsR2AObj16PrebakeRoute`
  (1), `gNdsR2MaterialWalkBoundEnabled` (1, the Sudden Death guard) and
  `gNdsParticleCameraCacheEnabled` (`NDS_R2_PARTICLE_CAMERA_CACHE` = 1).

| config | digest diff | P50 | P95 | over | paired median |
|---|---|---|---|---|---|
| gate | 0 | 776,896 -> 776,064 | 1,059,840 -> 1,060,032 | 65 -> 62 | -704 |
| g0 | 0 | 810,048 -> 809,728 | 1,156,480 -> 1,158,400 | 116 -> 117 | -512 |
| sz | 0 | 858,880 -> 861,056 | 1,163,648 -> 1,174,272 | 128 -> 130 | -704 |

The Sector Z P95 line is window noise, not the change: from frame 60 on the
two runs read P95 1,153,728 and 1,153,856, the hundred slowest q34 frames
are a paired -1,152 median, and the frames that moved most (+85K at 1155,
+63K at 1552) are SRC-only spikes on identical game state. Per-frame
polygon/vertex counts match on all but 1, 1 and 5 frames of 1,960.

## q35: the Whispy route arms, the fighter plan route and verify, the lean route word

Deleted:

- `gNdsWhispyAOTRoute` (7) and its arms 0-6 in `battleship_lbparticle.c`: the
  source/conservative generator and struct paths (`ndsWhispyAOTUpdateStruct`,
  `ndsWhispyAOTApplyBlends`), the route-1 rigid draw arm, the float-compare
  generator match, and every `route >= N` counter split. The renderer's
  `ndsRendererSubmitWhispyNativeQuad` takes `sb32 packet` (TRUE Whispy, FALSE
  the Fox blaster glow) instead of a route, and its route 4-6 branches are gone;
- `gNdsFtrPlanRoute` (`NDS_FTR_PLAN_ROUTE` = 1) and the plan equivalence arm
  `gNdsFtrPlanVerify` with its verify function and counters;
- `gNdsFtrLeanRoute`, unread since the old fighter executor went (2026-10-05).

| config | digest diff | P50 | P95 | over | paired median |
|---|---|---|---|---|---|
| gate | 0 | 776,064 -> 776,576 | 1,060,032 -> 1,059,072 | 62 -> 62 | +384 |
| g0 | 0 | 809,728 -> 808,192 | 1,158,400 -> 1,149,824 | 117 -> 115 | -64 |
| sz | 0 | 861,056 -> 859,392 | 1,174,272 -> 1,171,264 | 130 -> 130 | -896 |

Neutral. GPOL/GVTX differ on 45 / 96 / 128 frames by +-1..5 quads, both
signs about equally: `GFX_POLYGON_RAM_USAGE` is read before the frame's flush
while the geometry engine may still be draining the FIFO, so it races the
particle pass at the end of the draw, which this change retimes. A dropped
quad would move one way only.

## q36: the HUD native-OAM word and the anim warm-step word

Deleted `gNdsIFCommonNativeOamEnabled` (1 since its lane closed; its five
fallback checks and the 819-line A/B verifier
`scripts/verify-ifcommon-native-oam.ps1` that poked it to 0) and
`gNdsR2AnimWarmStep` (4, now `NDS_R2_ANIM_WARM_STEP`). Two capture scripts print
a literal 1 where they read the OAM word.

| config | digest diff | P50 | P95 | over | paired median |
|---|---|---|---|---|---|
| gate | 0 | 776,576 -> 776,896 | 1,059,072 -> 1,059,328 | 62 -> 62 | +192 |
| g0 | 0 | 808,192 -> 809,664 | 1,149,824 -> 1,148,544 | 115 -> 113 | +576 |
| sz | 0 | 859,392 -> 859,648 | 1,171,264 -> 1,163,392 | 130 -> 126 | +64 |

Neutral.

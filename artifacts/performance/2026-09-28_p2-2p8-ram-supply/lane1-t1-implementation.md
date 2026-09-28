# Lane 1, T1 implementation: compact ground maps (2026-09-28)

Status: implemented in the tree and host-verified. **Not built, not run** (no make, no ROM, no emulator, per the
brief). Every device claim below is UNPROVEN until you run the checklist in section 5. Design and measurements:
`lane1-stage-ground-files.md`.

What it does: each of the nine VS maps (`GR<Stage>Map`) is staged as a *compact* map, the stage wallpaper's 776-byte
Sprite/Bitmap stub appended and `MPGroundData.wallpaper` pointed at it, so the map's extern tree no longer contains
the 158,928-byte wallpaper container. **158,144 B less resident per stage, all nine, Dream Land included.**

## 1. Files

| file | change | why |
|---|---|---|
| `scripts/stages/generate_compact_ground_maps.py` (new) | builds one compact map (`--map/--output`), all nine (`--all`) or just verifies (`--check`) | the transform, with a self-verify pass (fixup counts -1/+2, extern table, every source word kept, stub == container's Bitmap[44]+Sprite, pixels gone); byte-identical to the lane-1 variants |
| `scripts/stages/test_compact_ground_maps.py` (new) | 19 tests, 81 subtests | generator, extern tree, pins, Makefile contract, C constants, and the REAL runtime C (sliced from `reloc_backend_assets.c`, built for i686, run against the generator's output for all nine maps) |
| `Makefile` | flag `NDS_P2_COMPACT_GROUND_MAPS` (default line ~227), config-header echo (~6974), stamp + static-pattern staging rule for the nine map files (~7886-7933) | see section 2 |
| `src/port/reloc_backend_assets.c` | one flagged block after `ndsRelocPointerRangeInLoadedFile` (table, two counters, `...Index/...Sprite/...ExpectedSize/...WallpaperKey/...PrepareTraining`), a Dream Land diagnostic hunk in `ndsRelocFinalizeLoadedFile`, and `ndsRelocNormalizeCompactGroundMapSprite` + its 5-line hook in `ndsRelocNormalizeStageDreamLandSprite` | the runtime seams |
| `src/port/renderer_adapter_matrix.c` | `ndsRendererAdapterNativeStageAssetSize` returns the pinned size plus 784 when the loaded map is compact | packet admission compares `data_size` to the pinned size (`renderer_adapter_stage.c:4585`) |
| `src/port/sprite_preview_backend.c` | 6 lines after the provenance call in `ndsSObjDrawCachedWallpaperFinal` | translate (map, stub offset) to the wallpaper table's key (container, 0x269C8) |
| `src/port/reloc_backend_compat_shims.c` | forward decl + one call in `mpCollisionInitGroundData` (both only with `NDS_P2_1P_GAME`) | Training Mode hazard, section 3 |
| `docs/DIAGNOSTIC_REFERENCE.md` | Pupupu mask note + the two counters | documentation |

Unchanged on purpose (this is how P1 stays identical): the packet descriptors (`native_stage_descriptors/*`), the
emitted rows in `renderer_adapter_matrix.c`, `check_nds_native_stage.py` and its golden `DREAMLAND_ADAPTER_ASSET_SIZES`,
`generate_native_wallpapers.py`, `native_wallpapers.generated.inc`, `nds_native_wallpaper.c`
(`sBattleWallpaperBindings`), and the staging lists (the wallpaper containers stay staged; the opening movies load them).
**`git add` the two new scripts with the Makefile**: `scripts/check-untracked-dependencies.py` fails a committed Makefile
that names an untracked generator. Pre-change copies of the six edited files: `lane1_t1_backups/`.

Deviations from the report's section 8 outline: no edit to the wallpaper table, its bindings, the descriptors, the
rows or any pin (the report proposed re-keying/re-pinning). The alias is one translation at the only consumer of
provenance, so both draw routes keep resolving to the same table row (no BG2 reload churn between them) and the
generated table stays flag-independent. Training Mode was not in the report (section 3).

## 2. The flag

`NDS_P2_COMPACT_GROUND_MAPS ?= 1` for `smash64ds-p2-fourcpu-tickhud-hwtri`, `smash64ds-p2-shell-hwtri`,
`smash64ds-p2-shell-freeplay-hwtri`, `smash64ds-p2-shell-loop-hwtri` and `smash64ds` (the published ROM is the
free-play twin and shares its block; leaving it off would make the published ROM differ from its verified lab twin),
else 0, notably `smash64ds-battle-playable-hwtri` (P1). Not `override`, so `NDS_P2_COMPACT_GROUND_MAPS=0` on the
command line is the same-ROM A/B control. Default-on for the four-CPU target matters: it loads Dream Land, the map the
census sized. The flag reaches C through `nds_build_config.h`.

Staging: the nine map files use an explicit static-pattern rule (overrides the generic `reloc/%` copy for those nine
only). Flag 1 runs the generator, flag 0 is the same plain `cp` as before (P1's staged maps are byte-identical). A
per-BUILD stamp (`$(BUILD)/nds_compact_ground_maps.stamp`, outside `nitrofs/`, which the ROM packs whole) changes only
when the flag does, so flipping the flag inside one BUILD dir re-stages the maps.

## 3. Things the report missed, found while implementing

- **Training Mode.** `sc1PTrainingModeLoadWallpaper` (decomp `sc1ptrainingmode.c:652`, imported verbatim under
  `NDS_P2_1P_GAME`, so it is in the published ROM) force-loads its own 133,040 B wallpaper file at
  `wallpaper - 0x26C88`, i.e. over the container's slot in the source tree. With a compact map that address is 158,984 B
  *before* the map: heap corruption. `mpCollisionInitGroundData` now hands Training a container-sized region with
  `wallpaper` where the source expects it (`ndsRelocCompactGroundMapPrepareTraining`), so Training uses the same memory
  as before. Only under `NDS_P2_1P_GAME` and only in the Training scene.
- Training rewrites `wallpaper` after load, so the packet-size seam decides compactness from the loaded size
  (`pinned + 784`), not from that pointer; the load-time seams (stub normalization, diagnostic) still use the pointer.
  A source map left on NitroFS (stale copy) therefore still loads and is admitted as before.

## 4. Host checks run (all host-side; the C compile checks use the ARM cross compiler with `-c`, output in scratchpad only)

| check | result |
|---|---|
| `generate_compact_ground_maps.py --check` | all nine build and verify: 192/304/224/224/224/192/192/832/368 -> +784 B each |
| `pytest scripts/stages/test_compact_ground_maps.py` | 19 passed, 81 subtests, none skipped. Mutation test of the C harness: 5 injected faults (growth constant, missing normalizer hook, no stub-offset validation, wrong Dream Land wallpaper id, wrong Training region size) each fail it |
| `check_nds_native_stage.py` | `M3_NATIVE_STAGE_CHECK_OK`, sha256 `b3833549...` identical to before |
| `emit_native_stage_runtime_rows.py --stage <s> --check` x9 | all OK (rows == descriptors == source sizes) |
| `scene_backend.c` compile, flag OFF (undefined or 0), three configs (p2-shell, four-CPU tickhud, all-content 1P), `-g0` | **byte-identical to the pre-change object** in all three (baseline from a frozen copy of `src/` + `include/`) |
| same, flag ON | compiles clean, warning count unchanged (99/89/99), text +688 / +684 / +772 B, bss +8 B; the hot stage-prep function shrinks 48-68 B (accessor now out of line), `ndsSObjDrawCachedWallpaperFinal` +32 B |
| `check_build_flag_census.py` | same 2 pre-existing holes (`NDS_MF_HOST`, `NDS_NATIVE_OWNER_IMAGE_LINKBOOMERANG`); none from the new flag |
| `check-untracked-dependencies.py` | pass (2 untracked sources, no committed reference yet) |
| wider suites | `scripts/stages`: 164 pass, 19 fail; `scripts/menus`+`fighters` slicers over my files: 17 pass, 5 fail; Makefile-reading tests: 61 pass, 2 fail. Every failure is pre-existing: the 13 wallpaper-runtime/platform failures are a Windows harness crash (`0xC0000005`) measured before my first edit, and I re-ran the rest (Lakitu/Bronto/Yoshi cloud executors, stage camera, Zebes capture, anim-cache/CSS-sizing/reloc-metadata slicers) against the HEAD copies of `renderer_adapter_matrix.c` / `reloc_backend_assets.c`: identical failure sets. Census strict = the 2 holes above |

Not run: make (not even `-n`), any ROM build or link, melonDS, `git` writes. The Makefile edit was reviewed by eye and
by a test that pins its text; GNU make has not parsed it.

## 5. What you must verify on device

Build the P2 target (flag defaults on) and the control in a second BUILD dir or with `NDS_P2_COMPACT_GROUND_MAPS=0`.
The first build in any BUILD dir recompiles everything (the config header changed).

1. **Staged files.** In the flag-on `nitrofs/reloc/reloc_stages/`: file size and sha256 (control = the O2R original):

| stage | file | compact bytes (payload) | sha256 (first 16 ... last 8) |
|---|---|---:|---|
| Peach's Castle | GRCastleMap | 1072 (976) | 90c6716d170f13da...0871ac27 |
| Sector Z | GRSectorMap | 1184 (1088) | bbb3e73ef0f2c416...26eff63b |
| Kongo Jungle | GRJungleMap | 1102 (1008) | 15b1cf8a80dbb746...2f6a0de1 |
| Planet Zebes | GRZebesMap | 1100 (1008) | 92c9a9ade44d84a2...3a651033 |
| Hyrule Castle | GRHyruleMap | 1096 (1008) | 648e8d67125e1b23...ae18ebc9 |
| Yoshi's Island | GRYosterMap | 1074 (976) | 74c1c9a90d60ba35...881f5969 |
| Dream Land | GRPupupuMap | 1072 (976) | d572ddab6fd4e6f7...16f15b7c |
| Saffron City | GRYamabukiMap | 1740 (1616) | cb014e211d4e00b7...b1dc2ac3 |
| Mushroom Kingdom | GRInishieMap | 1258 (1152) | 06c12546ac7f7ed2...d0a7d932 |

   (Full hashes: run `generate_compact_ground_maps.py --all --output-dir <dir>` and hash.) The nine containers must
   still be staged. **P1**: `smash64ds-battle-playable-hwtri`'s nine maps equal the originals; its ROM and ELF sections
   equal today's.
2. **Engaged.** After a VS battle load: `gNdsRelocCompactGroundMapCount` = 1 per load (0 means NitroFS held source maps);
   after frames `gNdsRelocCompactGroundMapKeyCount` > 0; `gNdsRelocExternalFixupFailCount` 0.
3. **Heap free delta.** Each stage's `mpCollisionInitGroundData` allocation drops by exactly 158,144 B:
   Castle 176,880 -> 18,736; Sector 226,192 -> 68,048; Jungle 225,392 -> 67,248; Zebes 219,872 -> 61,728;
   Hyrule 185,920 -> 27,776; Yoshi 229,280 -> 71,136; **Dream Land 202,816 -> 44,672**; Saffron 239,600 -> 81,456;
   Mushroom 192,224 -> 34,080 (`lbRelocGetFileSize`). The freed bytes go to whatever takes the heap's free top.
4. **BG2 wallpaper, per stage.** Capture must equal the control arm pixel for pixel. Watch
   `gNdsNativeWallpaperLoadCount` (one stream per battle, no reload churn), `gNdsNativeBattleWallpaperFailureCount` 0,
   `gNdsSObjWallpaperShapeRejectCount` 0, `gNdsSObjWallpaperCacheFastDrawCount` rising. Dream Land is the case to look
   at: its direct binding is NONE, so route 1 through the translated key is its only wallpaper owner.
5. **Stage packet still admitted** on all nine (a size mismatch would reject at `renderer_adapter_stage.c:4585`, reason 3,
   and the stage would draw generically); use the existing per-stage identity/`STG` checks in the shell-loop verifier.
6. **Replay digest**, `--sequence`, same ROM flag on vs off: identical.
7. **Dream Land reloc diagnostic:** `gNdsStagePupupuRelocResult` `0x50555052`, `gNdsStagePupupuRelocAssetMask` and
   `...DependencyMask` low five bits `0x1F` (bit 1 is now set by the map itself), `gNdsStagePupupuWallpaperPtrReady` 1,
   `gNdsStagePupupuExternalFixupCount` one lower than the control (the wallpaper slot is internal now).
8. **Training Mode** (published/free-play ROM only), all nine stages: enters and plays, its own wallpaper draws, no crash,
   `gNdsRelocCompactGroundMapCount` increments, heap low-water equals the control (Training re-allocates the container).
9. **Static image.** The flag adds ~0.7-0.8 KB to the image (section 4). The all-content ROM's VS character select needs
   "free at the reservation" >= 183,072 (`artifacts/performance/2026-09-23_css-preview-heap/`); confirm it still clears
   on the published config (`tools/run-css-heap.ps1`), since image growth eats it byte for byte.

## 6. Reproduce the host evidence

`python scripts/stages/generate_compact_ground_maps.py --check`; `python -m pytest
scripts/stages/test_compact_ground_maps.py -q`; the compile matrix is `tools/lane1_t1_check_all.sh` over
`tools/lane1_t1_cc_scene.sh` (paths point at the scratchpad copies of the config headers and the frozen baseline);
`tools/lane1_t1_head_check.py` reruns a slicing test against the HEAD copy of `reloc_backend_assets.c`.

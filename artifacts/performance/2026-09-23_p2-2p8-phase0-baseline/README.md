# P2-2p8 Phase 0: the instrument, and the baseline it measures (2026-09-23)

Phase 0 of `docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md`. It changes the
instrument, not the game, and re-measures the four-CPU baseline every later
phase is judged against. Roster Donkey/Samus/Link/Kirby, Dream Land, items on,
whole match (frames 2-1973), `smash64ds-p2-fourcpu-tickhud-hwtri` in
`builds/build-p2-fourcpu-tickhud`.

## What changed in the ROM and the harness

| change | where | why |
|---|---|---|
| Tick-HUD on-screen block off on the gate target | `Makefile`, four-CPU target: `override NDS_TICK_HUD_DRAW := 0` | it cost ~345-450K ticks on every 9th-10th frame inside WORK-H; the sampler reads the ring, never the printed table (A9) |
| MISC split columns MWPN/MEFX/MPRT/MTEX | `include/nds/nds_startup.h`, `src/nds/nds_platform.c`, `src/port/taskman_seam_battle_host.c` | per-frame deltas of the existing cumulative weapon/effect/particle/texture-upload draw counters |
| GX list usage columns GPOL/GVTX | same | polygon/vertex list RAM of each frame (limits 2,048 / 6,144) |
| Replay digest columns DGSA/DGSB | `src/port/nds_replay_digest.c` (new), host loop | FNV-1a over every fighter, item and weapon's gameplay state (status, motion, counters, raw position/velocity bits) and the RNG seed after each logic tick; `scripts/compare-replay-digest.py` compares two runs |
| gGMCameraMatrix rebuilt after an undrawn tick while a Link boomerang lives | `ndsBattleCameraMatrixAtUndrawnTick` (host), `ndsLinkBoomerangLive` (`src/import/battleship_link_weapons.c`) | the boomerang read a camera one tick older than the source on every second tick (A6) |
| Ring-size guard; wrap correction skips non-tick columns | `scripts/sample-tick-hud-buckets.ps1` | a bucket list that disagrees with the ROM now fails before launch; the 2^22 timer-wrap correction was subtracting 2^22 from digest words |

## Runs

| arm | ROM | result |
|---|---|---|
| `control1`, `control2` | Phase 0 ROM, camera rebuild ON | measurement complete; gate threw on native failures (below); digests identical except frames the old wrap correction corrupted |
| `nocam` | same source, camera rebuild compiled out | measurement complete; **digest identical to control1** on every frame the old correction did not touch; same FTR episode |
| `mutation` | nocam ROM, `-SetGlobals gNdsItemRateOverride=5` (items VeryHigh) via the sampler | **digest diverged on 860 of 1,972 frames, first at frame 1114** (the first spawn wait under the new rate): the digest sees a gameplay change. WORK-H P50 1,805,056 / P95 4,197,696 (item-dense arm) |

## Baseline (nocam rows; `bands_nocam.txt`)

| | P50 | P95 | P99 |
|---|---:|---:|---:|
| WORK-H | 1,689,088 | 4,207,488 | 4,753,152 |
| WORK-H excluding the entry (186-200) and the tint episode (798-1043) | 1,632,064 | 2,639,232 | 3,532,288 |

Two-VBlank presents **5.7%**; mean presentation **14.4 FPS**. GX list usage: polygons
P50 559 / max 675 of 2,048, vertices max 1,618 of 6,144 -- polygon RAM is not
binding. MISC split at P50: effects 55K, particles 49K, weapons ~0, texture upload
0; the MISC residual at P50 is ~97K.

## Findings

1. **A re-record episode owns P95.** Frames 798-1043 (246 presented frames,
   8.2 s of game time): FTR ~2.6M per frame (median 363K), one texture upload per
   frame (~23.5K), and an effect drawing ~144K per frame. Deterministic: both
   controls and nocam show the identical episode.

   **Mechanism, corrected by measurement.** The first reading blamed r49's global
   tint-set generation (`gNdsR2FighterTintSetGeneration`, mixed into every packet
   key). The tint counters over the whole match refute it: builds 9, misses 24,
   evictions 0, set generation 11, hits 5,248 (`tint-run.log`) -- the tile set
   is stable. What r49 also did is re-record a tinted packet whenever its prim
   changes ("a prim change under a tinted packet re-records it",
   `2a5bf793c20`): a fighter whose prim changes every frame (a flashing colour
   animation) re-records its own packet every frame, and each re-record re-runs
   production and re-resolves every texture of that fighter. Phase 1's lean path
   must select the (already resident) tint tile by a patched texture word, never
   by re-recording.

   **Profile of the episode** (`profile-thrash/`, per-PC, frames 820-868, work
   ~4.0M per frame): `ndsRendererHardwareResolveOrBindTexture` **935,578** and
   `ndsRendererHardwareTextureColor` **226,072** ticks per frame -- textures are
   re-resolved and re-converted on the CPU every frame, not only the 8x8 tint
   tile -- then packet re-production (`ndsFighterPacketCmd` 141K,
   `ndsRendererExecuteNativeFighterOwnerProduction` 90K,
   `ndsRendererNativePrepareProductionRun` 48K, emit/shade/corner ~140K) and
   `ndsRelocNativeAssetAddress` + `ndsPreviewFileOffset` 88K. Each re-record
   re-resolves the fighter's textures; that, not tile creation, is the cost.
2. **Native failures at match start.** First cause: Link, status 225 =
   `nFTLinkStatusAppearL` (entry), `use_texture == FALSE` at
   `nds_renderer_native_common.c:8758` (texture could not be resolved/bound):
   573 failures, 254 direct rejects. The 2026-09-17 checkpoint was native 0/0 on
   this roster; 221 commits later it is not. Phase 1 must make fighter textures
   admitted before GO, not bound on demand.
3. **The replay digest is deterministic and blind to render-only change**:
   control1 and nocam (different ROMs, one with the camera rebuild) agree on all
   1,972 frames once the wrap-correction corruption is excluded.
4. The harness's roster parameterisation was already done (shield-pose asserts
   derive from the selected kinds, `verify-p2-four-fighter-stress.ps1` ~571-600).

## Reproduce

```
make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-fourcpu-tickhud
pwsh scripts/verify-p2-four-fighter-stress.ps1 -NoBuild -RunnerSlot 10 -GdbPort 3433 -RowsCsv <out>-rows.csv ...
python scripts/compare-replay-digest.py control-rows.csv candidate-rows.csv
```

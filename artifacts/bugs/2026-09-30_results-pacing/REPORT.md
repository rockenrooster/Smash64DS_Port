# VS Results pacing investigation — 2026-09-30

Status: INVESTIGATION_COMPLETE / FIXES_PROPOSED. Eight r58 emulator runs, no builds, no tracked-file changes. Integrator owns implementation, builds and acceptance. Completed within the 2.5-hour / approximately 12-run time box.

The owner's uncertainty is resolved: Results has real first-appearance stalls (FFA **27 VBlanks**, No Contest **33**) and FFA steady state alternates one/two VBlanks. Causes are small ARM7 filename-table reads, ARM9 sprite/texture preparation, and repeated ARM9 tile planning. The four review diffs in section 5 target those measured costs. No fix has been applied or accepted.

## 1. Recipe and identity

- ROM: `builds/p2p8-playtest-r58/smash64ds.nds` (63,177,728 bytes), reported build HEAD `9fdc936adeb`, including text repair `58b625464d6` and heap budget `55934833a50`.
- ROM SHA-256: `2C22DE8E17D5D016459F868D4D56D261AE64929985E4C3A504DC326F39C56439`.
- ELF: adjacent `smash64ds.elf` (27,196,248 bytes); SHA-256 `99F2775E82DBA54AF06A04411DCC5169067F6003AA5AF8DB35D575DCB6438C48`.
- Use repo-local accurate melonDS slot 0, GDB port 3300, interpreter/JIT disabled from boot. Back up and restore `melonDS.toml` byte-for-byte after every run. Writes restricted to this evidence directory and `builds/codex-results-pacing*`, apart from explicitly permitted slot 0 config/storage.
- Extended the existing `builds/codex-results-text` driver into `builds/codex-results-pacing`: natural Title → VS → CSS → SSS, aligned four-byte roster seed at SSS, **stage 0**, roster `0,8,1,6` (Mario/Kirby/Fox/Yoshi), Time rules 1, one-minute limit, one human / three CPUs. FFA runs reach natural Time Up; No Contest uses START, then the source pause-reset input (L/R/A/B). No Results roster injection, forced winner, rebudget, fast-logic changes or build.
- Per-tic data cover **every complete Results iteration 1–420** for both modes, including update and draw guest timing. Endpoint-window runs have no executed breakpoints within that span; per-tic/detailed runs use silent print-and-continue commands and guest counters. Host elapsed time and the halted FPS HUD are not used for performance.
- Guest unit: `cpuGetTiming` equivalent units = hardware timer2 count ×64; one DS VBlank is **560,190 units**, 59.8261 Hz. Timing precision is 64 units. Update S→U includes input and source update; U→A includes audio and recorder; A→D includes graphics-heap reset, native draw, sprite finalization and HUD; D→P includes flush, VBlank wait and final commits. The storage wait subset is charged to its owning phase.
- The diagnostic endpoint raises `sMNVSResultsAllowExitWait` to 421 after source-menu entry, then stops at `mnVSResultsCheckExit+16`. This preserves visible behavior with no exit input, but skips the source's input-check body at tics 410–420 (FFA) / 200–420 (No Contest). All matched modes use the same bound. Natural unmodified exit cadence remains an integrator gate; do not claim identical CPU work in those skipped checks.
- Exact detached launch command is in the receipt below. Substitute the tag/storage/mode from the run table. Run serially on slot 0 and wait for `CONFIG_RESTORED=True` before the next run. Poll at most once per two minutes with one line, e.g. `Get-Content builds\codex-results-pacing\<tag>.status -Tail 1`. Analyze completed evidence with `python builds\codex-results-pacing\analyze-pacing.py <tag>` and, for detailed runs, `analyze-detail.py <tag>`. `make-detail.py` produces return-site scripts from the exact r58 ELF; `--draw` adds the texture/geometry owners.

| Run | Tag / mode | Coverage / outcome |
|---:|---|---|
| 1 | `r58-ffa-trace1` / `ffa-trace` | All 420 tics; silent endpoint MI notification failed, scoped recovery completed; trace retained |
| 2 | `r58-ffa-window1` / `ffa-window` | Breakpoint-free Results window; endpoint and config restoration PASS |
| 3 | `r58-nc-trace1` / `nc-trace` | All 420 tics; endpoint and restoration PASS |
| 4 | `r58-nc-window1` / `nc-window` | Breakpoint-free window; histogram exactly matches run 3 |
| 5 | `r58-ffa-detail1` / `ffa-detail` | Storage/model/OAM costs; histogram exactly matches run 2 |
| 6 | `r58-nc-detail1` / `nc-detail` | No Contest storage/model/OAM sibling; all 420 tics |
| 7 | `r58-ffa-draw1` / `ffa-draw` | Fighter/texture/palette first-use and steady costs |
| 8 | `r58-nc-draw1` / `nc-draw` | No Contest fighter/texture/palette sibling |

Each storage leaf is `storage-` plus the tag suffix after `r58-` (e.g. `storage-ffa-detail1`). Every run restored the original slot-0 config bytes. See `EVIDENCE_MANIFEST.json` for artifact/script hashes and identities. For a future candidate, regenerate native return sites from that ELF and confirm the source-menu instruction offsets; the archived scripts/addresses are an r58 recipe. The analysis uses hardware timer2 low bits, so the archived stale high-counter value is never a timing input.

## 2. Per-tic measurements

Every full iteration with **more than two VBlanks** is below. Primary per-tic evidence: [FFA CSV](r58-ffa-trace1-tics.csv), [No Contest CSV](r58-nc-trace1-tics.csv). Detailed traces reproduce the long-tic identities. The producer-register storage totals below are from the detailed runs; normal run-to-run wait differences are retained in the CSVs rather than pooled away.

| Mode | Tic | VBlanks | Update ticks | Draw ticks | Synchronous storage wait | Work / source + port citations |
|---|---:|---:|---:|---:|---:|---|
| FFA Time | **80** | **6** | 18,880 | 3,079,680 | ~553,664 | Source wallpaper/tint2 spawn: `decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:2837–2845,3235–3241,694–774`; native upload/read/expand: `src/nds/nds_native_wallpaper.c:191–248,335–356`; palette/affine presentation: `src/port/sprite_preview_backend.c:824–846` |
| FFA Time | **120** | **27** | 5,238,656 | 9,692,224 | ~3,223,616, in update | Source text/label/confetti + four fighters + winner audio: `mnvsresults.c:3243–3276,2756–2795,982–991`; native model residency `src/import/battleship_ftmanager.c:839–867,725–759`, `src/nds/nds_renderer_assets.c:4972–5044`; OAM bake `src/nds/nds_results_oam.c:965–1020,1053–1103,1448–1504`; first texture preparation `src/nds/nds_renderer_textures_effects.c:12697–14203` |
| FFA Time | **210** | **4** | 150,272 | 1,711,040 | **0** | Header/KO row source creation: `mnvsresults.c:2213–2216,1902–1950`; new OAM cells via `sprite_preview_backend.c:813–818` → `nds_results_oam.c:1477,1091,1011–1019`; async audio request appears but no blocking wait |
| FFA Time | **230** | **3** | 122,624 | 1,034,368 | **0** | TKO row: `mnvsresults.c:2218–2221,1964–2020`; same first-use native OAM bake chain |
| FFA Time | **270** | **3** | 211,904 | 1,058,944 | **0** | Points row: `mnvsresults.c:2226–2229,2060–2108`; same OAM bake chain; two async requests, zero waited ticks |
| FFA Time | **290** | **6** | 131,456 | 3,154,944 | **0** | Place row / RGBA winner badge: `mnvsresults.c:2230–2233,2118–2203`; native indexed + bitmap cell baking `nds_results_oam.c:856–1020`; bitmap alpha retained by `:1183–1206` |
| No Contest | **1** | **33** | 6,562,752 | 11,578,496 | **4,529,216**, update + draw | Wallpaper + NO CONTEST text/label + four losing fighters: `mnvsresults.c:2832–2839,3235–3256,1412–1428,892–896`; same model/stream/OAM/texture owners as FFA, all reached immediately |
| No Contest | **60** | **4** | 98,624 | 1,605,312 | **0** | Header/KO row: `mnvsresults.c:2317–2320`; same native OAM first-use bake chain |

Shorter source events remain covered: FFA tint at 180 and bar at 250 stay <=2 VBlanks; No Contest tint at 30 and TKO row at 80 stay <=2. No Contest's two 2-VBlank iterations are **2 and 80**, with none in 300–420.

The old recorder runs **after update and before draw** (`src/port/taskman_seam_harness.c:127–159`), so its interval N contains draw/present N−1 plus update N. Long recorder tics are **FFA 81=6, 120=10, 121=18, 211=4, 231=3, 271=3, 291=6; No Contest 2=22, 61=4**. The histogram's separate 10 and 18 entries are the two parts of FFA tic 120's 27-VBlank full iteration. Its first-sample rule drops No Contest tic 1's update entirely (`battleship_mnvsresults.c:574–595`).

| Primary trace, complete tics 1–420 | Interval histogram | Mean VBlanks | P50 / P95 / max | Effective presents/s |
|---|---|---:|---|---:|
| FFA | 1×353, 2×61, 3×2, 4×1, 6×2, 27×1 | 1.25 | 1 / 2 / 27 | 47.952 |
| No Contest | 1×416, 2×2, 4×1, 33×1 | 1.09 | 1 / 1 / 33 | 54.982 |

Breakpoint-free recorder qualification: FFA run 2 vs run 1 changes one ordinary 1-VBlank sample to 2; every tail frequency matches. FFA run 5's histogram matches run 2 exactly. No Contest run 4 matches run 3 exactly, including total VBlank span. This qualifies guest-counter traces for this investigation; it does not prove a future candidate's performance.

Prior r57 histogram (section 4 of the Results-text report) was sufficient to find a tail, but lacked tic identities and full-iteration boundaries. It is retained as context, not reused as r58 timing proof.

## 3. Findings and root causes

Source schedule was inspected via CodeGraph before code searches. The port imports the source unchanged at `src/import/battleship_mnvsresults.c:179`; the native owners below perform the measured work on that source schedule. Source constructions allocate GObjs/SObjs, DObjs, fighter parts and figatrees; native owners allocate model-image storage and cached OBJ cells. There was no Results heap overflow or allocation halt. Allocation is part of the constructor work, not an identified waiting loop.

Detailed FFA trace (`r58-ffa-detail1-functions.csv`, `-storage-waits.csv`, `-detail-summary.json`) reproduces every long iteration and matches the uninstrumented histogram exactly. There are no unmatched function returns or unclosed entries.

- Tic 80 native wallpaper draw: 3,052,992 ticks; storage calls 74 / 582,272 inclusive ticks. Actual producer-register waits: 42 payload reads (42,240 B, 351,808 ticks) and 32 metadata reads (1,024 B, 201,856 ticks). Wallpaper expansion calls software `__udivsi3` per pixel for `/17` (r58 ELF `ndsNativeWallpaperDraw+970`); source `src/nds/nds_native_wallpaper.c:225–234` validates `%17` then indexes `/17`. Exactly 42,240 valid intensity pixels, each a replicated nibble.
- Tic 120 update: four `mnVSResultsInitFighter` calls total 5,054,656 ticks, including four `ftManagerMakeFighter` calls total 2,712,576. **424 metadata requests / 13,568 B cost 2,624,640 waited ticks**. Payloads include `fighters/kirby_high.bin` (3 reads, 30,176 B), `fighters/yoshi_high.bin` (3, 19,200 B), `animation/ftanim_stream_pack.bin` (9, 8,480 B), and initial submotions `FTKirbySubMotionAppearAlt2`, `FTMarioSubMotionTaunt`, `FTFoxSubMotionTaunt`. All offsets were resolved using the exact ROM's FAT/FNT, and waited ticks came from r0 at the storage producer's +126 instruction, not cached counters.
- Tic 120 draw: native OAM GObjs total 6,988,800 ticks, preparation 6,955,200. This is actual first-use pixel baking, not synchronous storage (zero draw wait). The sampler performs two invariant software divides per pixel (`nds_results_oam.c:782–783`; exact r58 `ndsResultsSamplePrefiltered+32/+44`). Hoisting these steps to each cell preserves the sampling arithmetic exactly.
- Later long draw OAM totals: tic 210 = 1,483,392, 230 = 800,448, 270 = 733,504, 290 = 2,895,488. These bake newly introduced header/KO, TKO, points, and place/badge sprites at the source schedule (`mnvsresults.c:2207–2233`).
- Steady tics 300–303: 86 tile-plan calls per frame, 76,288–76,864 ticks; 18 distinct width/height pairs. OAM draw totals 176,640–179,584, of which the plan search is approximately 43%. `ndsResultsPrepareSObj` and `ndsResultsEmitSObj` both call the same pure dimension-only planner (`nds_results_oam.c:1074`, `:1150`). A bounded memo of immutable plans is a direct measured repeated-work fix; no source state cache or animation changes are needed.

The storage wait is **ARM7 service time as observed by ARM9**, recorded around PXI send/receive (`src/nds/nds_audio_storage.c:177–218`, especially `:198–203`); it includes scheduling/queue time and media reads, not just physical card transfer. ARM7 serializes reads in <=8,192-byte parts (`src/nds/arm7/nds_audio_main.c:75–97`) and services requests at `:101–159`. FFA's 424 metadata requests at tic 120 and No Contest's 566 at tic 1 are all **FNT** reads, verified against ROM header ranges. ARM9 `fopen` path iteration reaches that callback through `src/nds/nds_reloc_assets.c:2297–2324`; exact file IDs and FAT payload ranges come from the same ROM. It is not an audio-refill explanation for the later row stalls: they have zero synchronous wait.

First texture use is also ARM9 work: FFA tic 120 has 16 native resolve/bind calls costing 1,487,360; 19 `glTexImage2D` calls cost 51,776 and 19 palette uploads cost 52,160. No Contest tic 1 has 16 resolve/bind calls costing 1,487,872; 18 texture/palette uploads cost **51,008 / 50,880**. Conversion/cache preparation dominates VRAM upload time. The producer is `src/nds/nds_renderer_textures_effects.c:12697–14203`, with conversion beginning at `:13688–13695`. Source texture/material state reaches the DS renderer through `src/port/reloc_backend_fighter_display_seam.c:20` and the native display-contract consumer; a source callback's ~15,000 steady ticks are not the whole native fighter cost.

OBJ pixels are baked directly into native VRAM: `nds_results_oam.c:974–977` returns cached cells; a miss allocates at `:990` and writes every resampled texel at `:856–963`, then counts the cell at `:1018–1019`. Palette colours and OAM commit in VBlank (`:1709–1735`); palette upload is distinct from CPU pixel baking. End-state engagement: FFA four fighters / 46 SObjs / 30 baked cells / 25,600 OBJ bytes / 10 banks; No Contest four fighters / 33 SObjs / 19 cells / 17,536 bytes / 7 banks. Bad provenance, VRAM/palette/cell exhaustion and rollback counters are zero after these engaged runs.

Scene entry is also visibly expensive. Existing producer timings in the breakpoint-free runs report FFA FuncStart **80,120,704**, four setup-files calls **56,087,680**, transition through first update **85,159,616**; No Contest **73,947,136 / 56,052,480 / 85,019,648**. A transition bracket of ~85 million guest units is about **152 VBlanks / 2.54 s**, before the remaining first draw. `battleship_mnvsresults.c:409–464,486–515,600–608` defines those brackets; source `mnvsresults.c:3335–3354` loads the scene files and fighter banks. Moving work from tic 1 into FuncStart alone would keep the transition stall. The proposed first-use/metadata improvements must be measured over this bracket too.

## 4. Steady state

Results is synchronized to the next DS VBlank and has **no fixed 30 Hz or guaranteed 60 Hz update lock**. The source-menu pump runs one update and one draw per iteration (`taskman_seam_harness.c:78–159`); `ndsPlatformEndFrame` flushes and calls `ndsPlatformWaitForScheduledVBlank` (`nds_platform.c:3788,3833`). That helper waits for the next VBlank, honoring an earliest deadline only when one is set (`:3688–3701`). Work that crosses a deadline therefore consumes a second VBlank. The measured long steady Present phase mostly reflects this wait; it also includes flush and OAM commits, so it is not entirely idle time.

| Tics 300–420, 121 frames | FFA Time | No Contest |
|---|---:|---:|
| 1 / 2 VBlank frames | 73 / 48 | 121 / 0 |
| Effective presents/s | 42.834 | 59.826 |
| Update P50 / P95 | 111,232 / 138,560 | 66,048 / 70,464 |
| Audio+recorder P50 / P95 | 3,840 / 3,968 | 3,648 / 3,776 |
| Draw P50 / P95 | 435,776 / 442,816 | 280,384 / 281,728 |
| Present P50 / P95 | 31,808 / 563,520 | 210,048 / 212,992 |
| Storage wait / requests | 0 / 0 | 0 / 0 |

The **FFA 2-VBlank subset** (48 frames) has update P50/P95 **123,968 / 141,248**, draw **438,784 / 444,864**, and update+audio/recorder+draw P50 **565,568** (range 555,072–593,920). Its median already exceeds one VBlank's 560,190-unit budget before all final commit/loop work. The 1-VBlank subset's active median is **542,272**; No Contest's is **350,080**. This is CPU budget pressure, not storage latency. The pure planner's ~76,500 units/frame is a sufficient-sized, measured target to create margin, although candidate cadence remains to be verified. Additional FFA source output includes the points/place rows, winner emblem and confetti; none should be removed to meet the budget.

## 5. Proposed fix diffs (not applied)

All diffs below are **review proposals, not applied or built**. They were generated against the current clean r58-equivalent source inputs; individual original-file hashes and standalone diffs are in `proposals.json` / `proposal-*.diff`. Apply A and B as one Results-OAM batch; C and D address distinct measured first-use costs. No cadence cap is proposed: the one-VBlank budget is within reach by removing measured repeated work.

A. Memoize the pure dimension-to-tile plan. This preserves the existing exhaustive search, its minimum-cell/minimum-area criterion, and its tie order. It stores immutable DS layout only, never source object/animation/palette state. 32 entries cover the measured 18 FFA / 10 No Contest steady pairs; replacement recomputes the exact original result. Added static cost is `32 * sizeof(NDSResultsOamPlanMemo) + 4` = **1,028 B under the r58 ABI** (`sizeof(NDSResultsOamTilePlan)=26`, memo entry pads to 32), requiring the existing CSS-reserve proof. The extra width guard is equivalent to the old search failing beyond four 64-pixel-wide cells.

Verification target: steady FFA total planner ticks <=15,000/frame versus ~76,500, draw P95 <=390,000 versus 442,816, and 300–420 two-VBlank frames reduced from 48 to <=1. Steady No Contest remains 1 VBlank. This is a hypothesis, not a measured candidate gain.

```diff
--- a/src/nds/nds_results_oam.c
+++ b/src/nds/nds_results_oam.c
@@ -126,8 +126,19 @@
     { 8u, 16u, SpriteSize_8x16 },
     { 8u, 32u, SpriteSize_8x32 }
 };
 
+/* Tile plans depend only on final dimensions and the constant shape table.
+ * Results repeats the same 18 pairs twice per SObj per frame. */
+#define NDS_RESULTS_PLAN_MEMO_SLOTS 32u
+typedef struct NDSResultsOamPlanMemo
+{
+    u32 key;
+    NDSResultsOamTilePlan plan;
+} NDSResultsOamPlanMemo;
+static NDSResultsOamPlanMemo sNdsResultsPlanMemo[NDS_RESULTS_PLAN_MEMO_SLOTS];
+static u32 sNdsResultsPlanMemoNext;
+
 static NDSResultsOamCell sNdsResultsCells[NDS_RESULTS_CELL_SLOTS];
 static NDSResultsOamPalette sNdsResultsPalettes[NDS_RESULTS_PALETTE_BANKS];
 /* The banks' colours, staged: they reach OBJ palette RAM with the OAM in
  * VBlank (ndsResultsOamCommit), so a tint step never recolours the frame on
@@ -431,12 +442,24 @@
                                     NDSResultsOamTilePlan *plan)
 {
     NDSResultsOamShape chosen[NDS_RESULTS_MAX_TILES];
     u32 count;
-
-    if ((plan == NULL) || (width == 0u) || (height == 0u) || (height > 64u))
-    {
-        return 0;
+    u32 key;
+    u32 i;
+
+    if ((plan == NULL) || (width == 0u) || (height == 0u) || (height > 64u) ||
+        (width > 64u * NDS_RESULTS_MAX_TILES))
+    {
+        return 0;
+    }
+    key = (height << 16) | width;
+    for (i = 0u; i < NDS_RESULTS_PLAN_MEMO_SLOTS; i++)
+    {
+        if (sNdsResultsPlanMemo[i].key == key)
+        {
+            *plan = sNdsResultsPlanMemo[i].plan;
+            return 1;
+        }
     }
     memset(plan, 0, sizeof(*plan));
     for (count = 1u; count <= NDS_RESULTS_MAX_TILES; count++)
     {
@@ -445,8 +468,14 @@
         ndsResultsSearchTilePlan(width, height, count, 0u, 0u, 0u,
                                  chosen, plan, &best_area);
         if (plan->count != 0u)
         {
+            NDSResultsOamPlanMemo *memo =
+                &sNdsResultsPlanMemo[sNdsResultsPlanMemoNext];
+            memo->key = key;
+            memo->plan = *plan;
+            sNdsResultsPlanMemoNext =
+                (sNdsResultsPlanMemoNext + 1u) % NDS_RESULTS_PLAN_MEMO_SLOTS;
             return 1;
         }
     }
     return 0;
```

B. Compute the two resampling steps once per newly baked cell instead of once per sampled output pixel. All shifts, multiply order, truncation, clamps, four taps, premultiplied interpolation, indexed quantization, and bitmap alpha threshold remain the same. No extra persistent RAM. Both indexed and bitmap writers are included.

Verification target: software divider calls inside `ndsResultsSamplePrefiltered` go from two per pixel to zero; writers execute two per new cell. Byte-compare cached OBJ texels for all baked glyph/tag/table/badge cells against r58. Require >=10% reduction of the first-use OAM totals (FFA 120 6,988,800; 290 2,895,488; No Contest 1 5,794,240); actual gain must be measured. This alone does not establish seamless first appearance.

```diff
--- a/src/nds/nds_results_oam.c
+++ b/src/nds/nds_results_oam.c
@@ -772,16 +772,14 @@
     }
 }
 
 static s32 ndsResultsSamplePrefiltered(const NDSResultsOamSource *source,
-                                       u32 final_width, u32 final_height,
+                                       u32 step_x_q16, u32 step_y_q16,
                                        u32 destination_x, u32 destination_y,
                                        u8 rgba[4])
 {
     u32 source_width = (u32)(u16)source->sprite->width;
     u32 source_height = (u32)(u16)source->sprite->height;
-    u32 step_x_q16 = (source_width << 16) / final_width;
-    u32 step_y_q16 = (source_height << 16) / final_height;
     s32 source_x_q16 = (s32)(step_x_q16 >> 1) - 0x8000 +
                        (s32)(destination_x * step_x_q16);
     s32 source_y_q16 = (s32)(step_y_q16 >> 1) - 0x8000 +
                        (s32)(destination_y * step_y_q16);
@@ -858,8 +856,10 @@
                                       u32 final_width, u32 final_height)
 {
     u32 bytes = ((u32)cell->cell_width * cell->cell_height) / 2u;
     u8 *scratch = (u8 *)sNdsResultsIndexedScratch;
+    u32 step_x_q16 = ((u32)(u16)source->sprite->width << 16) / final_width;
+    u32 step_y_q16 = ((u32)(u16)source->sprite->height << 16) / final_height;
     u32 y;
     u32 word;
 
     memset(scratch, 0, bytes);
@@ -880,9 +880,9 @@
                 (destination_y >= final_height))
             {
                 continue;
             }
-            if (ndsResultsSamplePrefiltered(source, final_width, final_height,
+            if (ndsResultsSamplePrefiltered(source, step_x_q16, step_y_q16,
                                             destination_x, destination_y,
                                             rgba) == 0)
             {
                 return 0;
@@ -927,8 +927,10 @@
                                      const NDSResultsOamSource *source,
                                      u32 final_width, u32 final_height)
 {
     u32 bytes = (u32)cell->cell_width * cell->cell_height * sizeof(u16);
+    u32 step_x_q16 = ((u32)(u16)source->sprite->width << 16) / final_width;
+    u32 step_y_q16 = ((u32)(u16)source->sprite->height << 16) / final_height;
     u32 y;
 
     dmaFillHalfWords(0u, cell->gfx, bytes);
     for (y = 0u; y < cell->cell_height; y++)
@@ -945,9 +947,9 @@
                 (destination_y >= final_height))
             {
                 continue;
             }
-            if (ndsResultsSamplePrefiltered(source, final_width, final_height,
+            if (ndsResultsSamplePrefiltered(source, step_x_q16, step_y_q16,
                                             destination_x, destination_y,
                                             rgba) == 0)
             {
                 return 0;
```

C. Replace `/17` and `%17` for an 8-bit replicated-nibble intensity with exact nibble validation/indexing. Valid bytes are exactly 0x00, 0x11, ..., 0xff; invalid intensities still fail. The 42,240-pixel Results wallpaper therefore avoids the software quotient call in the linked Thumb loop. Native RGBA wallpapers are unaffected.

Verification target: zero `/17` software-divider calls on this wallpaper, identical BG texels and palette/affine state, and first wallpaper CPU work reduced by >=1,000,000 ticks. Storage requests stay 74 if D is not applied. This gives a falsifiable improvement threshold, not a claimed benchmark.

```diff
--- a/src/nds/nds_native_wallpaper.c
+++ b/src/nds/nds_native_wallpaper.c
@@ -225,14 +225,14 @@
             const u8 *indices = (const u8 *)sWallpaperRow;
             for (x = asset->native_w; x != 0u; x--)
             {
                 u32 intensity = indices[x - 1u];
-                if ((intensity % 17u) != 0u)
+                if ((intensity >> 4) != (intensity & 15u))
                 {
                     ndsRelocAssetStreamClose(&stream);
                     return FALSE;
                 }
-                sWallpaperRow[x - 1u] = palette[intensity / 17u];
+                sWallpaperRow[x - 1u] = palette[intensity >> 4];
             }
         }
         for (x = 0u; x < asset->native_w; x++)
         {
```

D. Cache one 512-byte immutable FNT sector at the ARM9 storage callback, under its existing mutex, with cache invalidation on ROM close/open and an aligned owned ARM7 destination. Metadata only; payload reads and the ARM7 protocol stay on their existing paths. It serves the same caller bytes and counts logical returned bytes; `Requests` remains the actual transport count. Short final ROM pages retain the established reader path. Added static RAM is 520 bytes plus linker alignment.

Verification target: FFA tic-120 FNT transport requests <=50 versus 424, waited ticks <=500,000 versus 2,624,640; No Contest tic-1 FNT transport requests <=70 versus 566, waited ticks <=600,000 versus 3,494,656. Log logical read spans when validating page crossings (14 observed physical transfers straddled pages). Preserve every required payload and hash, zero storage failures, and verify ROM close/reopen invalidation. This shared startup/storage change requires the single widest Latest profile and CSS-reserve evidence.

```diff
--- a/src/nds/nds_audio_storage.c
+++ b/src/nds/nds_audio_storage.c
@@ -25,8 +25,15 @@
 static Mutex sStorageMutex;
 static Mutex sRomInitMutex;
 static NdsAudioStorageRequest sStorageRequest;
 static uint8_t sStorageBounce[512] __attribute__((aligned(32)));
+/* NitroROM path iteration asks for tiny immutable FNT slices. Retain one
+ * whole sector rather than sending an ARM7 round-trip for each slice. */
+#define NDS_STORAGE_FNT_PAGE_BYTES 512u
+static uint8_t sStorageFntPage[NDS_STORAGE_FNT_PAGE_BYTES]
+    __attribute__((aligned(32)));
+static uint32_t sStorageFntPageOffset;
+static int sStorageFntPageReady;
 static uint32_t sStorageSequence;
 static NitroRom sCardRom;
 static int sCardRomReady;
 static uint32_t sRomBytes;
@@ -232,9 +239,32 @@
     while (bytes != 0u)
     {
         uint32_t part;
         uintptr_t address = (uintptr_t)out;
-        if (((address & 31u) == 0u) && (bytes >= 32u) &&
+        uint32_t fnt_offset = g_envAppNdsHeader->fnt_rom_offset;
+        uint32_t fnt_size = g_envAppNdsHeader->fnt_size;
+        uint32_t page_offset = offset & ~(NDS_STORAGE_FNT_PAGE_BYTES - 1u);
+
+        if ((offset >= fnt_offset) && (offset - fnt_offset < fnt_size) &&
+            (bytes <= fnt_size - (offset - fnt_offset)) &&
+            (NDS_STORAGE_FNT_PAGE_BYTES <= rom_bytes - page_offset))
+        {
+            uint32_t skip = offset - page_offset;
+            part = NDS_STORAGE_FNT_PAGE_BYTES - skip;
+            if (part > bytes) part = bytes;
+            if (!sStorageFntPageReady || sStorageFntPageOffset != page_offset)
+            {
+                sStorageFntPageReady = 0;
+                ok = ndsAudioStorageCall(NDS_AUDIO_STORAGE_READ_CARD,
+                                        page_offset, sStorageFntPage,
+                                        NDS_STORAGE_FNT_PAGE_BYTES);
+                if (!ok) break;
+                sStorageFntPageOffset = page_offset;
+                sStorageFntPageReady = 1;
+            }
+            memcpy(out, sStorageFntPage + skip, part);
+        }
+        else if (((address & 31u) == 0u) && (bytes >= 32u) &&
             (address <= UINT32_MAX) &&
             ndsAudioStorageMainRange((uint32_t)address, bytes))
         {
             part = bytes & ~31u;
@@ -348,8 +378,9 @@
     (void)unused;
     mutexLock(&sStorageMutex);
     (void)ndsAudioStorageCall(NDS_AUDIO_STORAGE_CLOSE_CARD, 0u, NULL, 0u);
     sCardRomReady = 0;
+    sStorageFntPageReady = 0;
     free(sRomExtentAllocation);
     sRomExtentAllocation = NULL;
     mutexUnlock(&sStorageMutex);
 }
@@ -388,8 +419,9 @@
             if (!sRomBytes) ndsAudioStorageInitHalt(8u);
         }
         pxiWaitRemote((PxiChannel)NDS_AUDIO_STORAGE_CHANNEL);
         mutexLock(&sStorageMutex);
+        sStorageFntPageReady = 0;
         int opened = argv0 ?
             ndsAudioStorageCall(NDS_AUDIO_STORAGE_OPEN_MAP, 0u, &sRomMap, sizeof(sRomMap)) :
             ndsAudioStorageCall(NDS_AUDIO_STORAGE_OPEN_CARD, 0u, NULL, 0u);
         mutexUnlock(&sStorageMutex);
```

## 6. Integrator verification

1. Apply the chosen review diffs as a coherent producer-to-consumer batch, preserving the existing Results text repair, source schedule and heap budget. A/B both touch `nds_results_oam.c`; review the combined file. This investigation built nothing and provides no candidate acceptance.
2. Keep natural shipping logic and native-only inputs. Serialize the integrator's build; use the **one widest relevant Latest qualification** because D changes shared startup/storage, with the required CSS reserve >=183,072 B and the existing owner route. Do not stack profiles. Added static data A+D is approximately 1,548 B plus alignment; confirm actual linked growth/heap low-water rather than assuming.
3. Re-run the same stage-0, roster-0/8/1/6 one-minute FFA match and pause-reset No Contest. Record complete iteration intervals 1–420 and update/audio/draw/present timing; compare exact ROM/ELF/config identity and breakpoint-free endpoints. Capture source boundaries at 1, 30, 60, 80, 120, 180, 210, 230, 250, 270, 290 and steady state. Name every >2-VBlank tic, including any new one; do not substitute the old recorder histogram for full first-iteration cost.
4. Check the **specific numeric targets in section 5**, one owner at a time within the frozen batch: planner <=15,000/frame, FFA draw P95 <=390,000, steady 2-VBlank frames <=1/121; pixel divider hoist and >=10% first-use OAM gain; wallpaper >=1,000,000-tick CPU reduction; FNT request/wait ceilings. Keep source updates/draws/particles/fighter submissions and required pixels engaged. These are proposed falsifiable targets, not promised measured gains. Report per-scenario actual deltas; do not assign this roster's gain to all roster/stage cases.
5. Byte-compare OBJ baked pixels and palettes/BG texels against r58 and inspect visible Results progression: winner/NO CONTEST text, player tags, score digits/rows, RGBA badge alpha, both dark tints, source colours/scale/layer ordering, victory/loss animations and confetti. Allocation/order changes require source states to match, not just zero counters. Preserve four live fighters and required cell/bank capacities; storage failures, native rejection and provenance/exhaustion/rollback stay zero. Existing taskman DL-overflow counts are **pre-existing at Results entry** (FFA 28,602 / NC 2,772); Results must add zero, and they cannot be reported as a run-wide zero.
6. Measure FuncStart/setup-files/transition and first presented frame too. A shift from tic 1/120 into entry time is not a seamless-transition win. Keep post-421 natural exit/return-CSS coverage, restoring the original exit bound for that proof. Ordinary START handling and old text/heap fixes remain owed on the integrated candidate.
7. Source obligations beyond this report remain open: other rosters, stages, Team/Stock, two/three players and winner-status variants were not measured here. Verify affected siblings with the integrator's existing qualified coverage. All candidate builds, final hard-on natural timing, visual fidelity, wider gates and owner acceptance remain the integrator's responsibility.

The four small diffs remove measured waste; they do **not** claim the remaining cold-start OAM/texture/constructor work fits a single frame. If first appearances still miss cadence after these changes, the next justified batch is producer-baked OBJ texels/native texture residency or bounded prewarm using idle tics before their source appearance. FFA offers idle tics before 80/120 and each row; No Contest needs prepared data before entry because its work starts at tic 1. Preserve source spawn/animation/audio timing and charge all preparation to the complete transition. A concrete prewarm/generator diff needs the integrated residual measurement; no speculative extra renderer route is included here.

## Investigation receipt

- Initial read-only setup: repository has `.codegraph/`; used `codegraph explore` for Results schedule/recorder before code search. PowerShell 7.6.6 with `-NoProfile`/login disabled is required (profile invocation produced incomplete output).
- ROM/ELF identities above were hashed directly. Windows CIM process enumeration is denied; use scoped slot 0 process/port checks instead.
- Runs: 0. No code changes or builds.
- Final receipt supersedes this initial run count: **8 runs**, all stopped; no builds or tracked edits.

### Run 1 planned: r58 FFA per-tic trace

The r58 ELF retains only the histogram, not a per-tic Results log. `src/port/taskman_seam_harness.c:127–159` runs source update, audio, recorder, draw, then present. Recorder interval at tic N includes draw/present N−1 plus update N; it is not the full cost of iteration N. The trace therefore captures five loop boundaries (input/update start S, source update end U, audio/recorder end A, draw end D, present end P) and preserves both interval definitions.

Linked addresses from r58 disassembly: source-menu loop `0x020a6270`, boundaries +146/+52/+84/+136/+140. `cpuGetTiming` is `tickGetCount << 6` modulo 2^32, minus a constant base. `tickGetCount` reads timer2 at `0x04000108`, high counter at `0x0227e240`, and pending timer2 IRQ bit 5. `clock.gdb` duplicates that read without calling the inferior. Timing validity still requires comparison against a breakpoint-free run and timer/cache consistency checks.

Run command (detached, same recipe for later tags/modes):

```powershell
Start-Process pwsh -ArgumentList '-NoProfile','-File','builds\codex-results-pacing\run.ps1','-Arm','r55','-Tag','r58-ffa-trace1','-RomPath','builds\p2p8-playtest-r58\smash64ds.nds','-ElfPath','builds\p2p8-playtest-r58\smash64ds.elf','-StorageLeaf','storage-ffa-trace1','-Mode','ffa-trace','-SeedAtSss','-PermitSlot0ConfigChange','-TimeoutSeconds','2400' -WindowStyle Hidden -RedirectStandardOutput 'builds\codex-results-pacing\r58-ffa-trace1.launch.out' -RedirectStandardError 'builds\codex-results-pacing\r58-ffa-trace1.launch.err'
```

`-Arm r55` is a legacy required driver selector; both explicit r58 paths override it. No r55 ROM is used. Poll no more than once per two minutes, one status line only. Planned trace stops after full present 420, with all input injection breakpoints disabled on entry to the Results source-menu loop. Storage counters in each record separate ARM7 waits from source/draw work. No guest calls, subagents or builds.

### Run 1 findings / instrumentation correction

All five boundaries for all 420 iterations were captured, but the conditional silent endpoint did not deliver the collector's expected MI stop notification; recovery/cleanup is due. Future runs use the existing proven default one-shot breakpoint at `mnVSResultsCheckExit+16`, reached only at tic 421.

The stub reads cached high timer / Results tic / recorder globals stale. Raw full timer deltas initially showed 2^32 wrap artifacts; they are invalid. The analysis now orders complete S/U/A/D/P groups by execution and uses **only the hardware timer low 16 bits**, unwrapping elapsed timer periods from independent VBlank deltas (one VBlank = 560,190 cpuGetTiming units = 8,752.96875 timer2 units). VBlank quantization uncertainty is less than one frame, well below half a 65,536-unit timer period, so this resolves the period uniquely. Precision is 64 cpuGetTiming units. Phase deltas sum to each complete iteration; no remaining clock anomalies. Recorder intervals are reconstructed from A-boundary VBlank deltas, not the stale recorder global. Long reconstructed recorder tics: 81=6, 120=10, 121=18, 211=4, 231=3, 271=3, 291=6. This shows why the old histogram's 10/18 entries do not describe two separate slow full iterations.

Run-1 endpoint recovery: the inferior stopped silently at P420 but MI never emitted its expected `*stopped` notification (only two prior stop notifications in the entire raw log). Stopped only the GDB/collector descendants of recorded launcher PID 12252, identified by Windows Toolhelp parent chain; evidence `builds/codex-results-pacing/r58-ffa-trace1-recovery.json`. Launcher finally owns emulator shutdown/config restoration. No endpoints JSON from this run; the complete phase trace remains preserved. The conditional silent endpoint is removed from subsequent scripts. Run 2 planned: same r58 FFA recipe, tag `r58-ffa-window1`, `storage-ffa-window1`, mode `ffa-window`; no executed breakpoints between source-menu entry and tic 421 exit-check branch. Compare histogram against reconstructed trace to qualify observer effects. Run 3 planned after Run 2: No Contest full per-tic trace with the corrected one-shot endpoint.

Run 2 completed in 90.8 s host runtime (host runtime is not a timing metric), with endpoint at tic 421 and config restored byte-for-byte (SHA-256 `F430E1CF4BD43959950765928B0C526C495013C61A151164E326CB07CEA5FA92`). Evidence: `r58-ffa-window1-start.json` / `-end.json`. Run 3 now queued to launch: r58 No Contest trace, tag `r58-nc-trace1`, storage `storage-nc-trace1`, mode `nc-trace`; source pause+L/R/A/B reset input via the original No Contest driver. Same default roster and interpreter. Exact five per-tic boundaries and endpoint 421; no new build.

Run 2 comparison: uninstrumented recorder 2–420 histogram = 1×354, 2×58, 3×2, 4×1, 6×2, 10×1, 18×1 (419 samples). Run 1 reconstructed recorder histogram = 1×353, 2×59, 3×2, 4×1, 6×2, 10×1, 18×1. Only one ordinary frame changes by one VBlank; all tail frequencies match. Guest VBlank span is 523 versus trace 524, consistent with that one-frame variation. Existing snapshot wallpaper cache counters are not usable: ELF DWARF includes discarded/unwritten globals which alias unrelated data after section garbage collection. Exclude these counters from reasoning.

Run 3 completed: No Contest full trace and default one-shot endpoint work; config restored. Evidence `r58-nc-trace1-tics.csv`, `-summary.json`, `-start.json`, `-end.json`. Run 4 planned: tag `r58-nc-window1`, `storage-nc-window1`, mode `nc-window` with the same pause-exit recipe, no executed breakpoints within Results 1–420. Function-attribution traces will follow these baseline qualifications.

Run 4 completed, restored config. Run 5 planned: r58 FFA detailed trace, tag `r58-ffa-detail1`, mode `ffa-detail`, storage `storage-ffa-detail1`. Freeze `detail.gdb`, `trace-detail.gdb`, and `detail-sites.json` before launch. Trace ten owning functions only at scheduled long tics and steady tics 300–303, using entry registers / return sites from exact r58 ELF. Sample `ndsAudioStorageCall+126`'s **r0 waited timer64 value** (producer register) with r5 bytes / r6 ROM offset to remove uncertainty from cached storage counters. Map ROM offsets to the ROM's own FAT/FNT for file attribution. No guest calls or guest writes beyond the existing input fixture and endpoint exit bound.

Run 4 uninstrumented No Contest histogram exactly matches Run 3: 1×415, 2×2, 4×1, overflow bucket×1 (actual maximum 22), 419 recorder samples. Both have the same total VBlank span and same No Contest source status/motion identities. Run 5 completed (333.7 s host) and config restored; detailed histogram exactly matches FFA window1. Run 6 planned: r58 No Contest detailed trace (`r58-nc-detail1`, `nc-detail`, `storage-nc-detail1`) with the frozen same function sites, especially tic 1 and tic 60. This closes per-mode function/storage attribution before proposing fixes.

Run 6 completed (297.9 s host), config restored. No Contest tic 1: four fighter initializations = 5,921,152 ticks; native wallpaper = 3,054,016; OAM text/player-tag baking = 5,794,240. 566 metadata transport reads (18,112 B) waited 3,494,656 ticks; total storage producer waits are tabulated in `r58-nc-detail1-storage-waits.csv`. Tic 60 OAM draw = 1,441,856 ticks. Steady OAM draw ~116,000, plan search ~48,300 (60 calls; 10 distinct dimensions). No unmatched function records.

Exact ROM metadata mapping confirms all 424 FFA tic-120 non-payload requests are **FNT**, not FAT, header or generic padding: FNT offset 2,157,056 / size 11,456; FAT 2,168,832 / size 5,056. They span seven 512-byte pages. A simulated single FNT page cache misses 17 times in the observed start-offset sequence; 14 physical 32-byte transfers straddle page boundaries, so use a conservative <=50 metadata-request verification target, not an unmeasured 17-request promise.

Run 7 planned: r58 FFA draw attribution (`r58-ffa-draw1`, `ffa-draw`, `storage-ffa-draw1`). Frozen `draw-detail.gdb` adds exact return-site timing for source fighter display, native texture resolve/bind, `glTexImage2D`, and `glColorTableEXT`, plus OAM GObjs at the same long/steady tics. This covers the first-draw remainder outside OAM. Run 8 No Contest equivalent will follow if needed. No builds or tracked edits.

Run 7 completed, config restored. At FFA tic 120: 16 native texture resolve/bind calls total 1,487,360 ticks; actual 19 texture uploads total 51,776 and 19 palette uploads total 52,160. Therefore native texture preparation/packing is expensive; VRAM copying itself is a small portion. Source fighter-display callback body totals 51,072 on the first draw and ~15,000 steady (native DS geometry is submitted through the separate display-contract seam; do not mistake that small source callback cost for the full fighter renderer). OAM totals reproduce Run 5. No unmatched function records. Run 8 planned: r58 No Contest draw attribution, tag `r58-nc-draw1`, mode `nc-draw`, storage `storage-nc-draw1`, same frozen function sites, to qualify the first-draw sibling.

### Final receipt

Run 8 completed (127.4 s host), config restored. No Contest first draw reproduces the native-texture/OAM sibling: 16 resolve/bind calls 1,487,872; 18 texture uploads 51,008; 18 palette uploads 50,880; OAM 5,793,536. All detailed function entry/return pairs close. No further runs or builds are needed for this investigation's requested per-tic and attribution coverage.

ROM and ELF hashes were rechecked unchanged at handoff. Read-only `git diff 9fdc936adeb -- <cited source inputs>` reports no scoped source changes, so citations and proposed source diffs correspond to the measured r58 code. Slot 0 is stopped; all eight original config backups hash to `F430E1CF4BD43959950765928B0C526C495013C61A151164E326CB07CEA5FA92`, identical to the restored config. Slots 1 and 4–13 were not controlled. No subagents, repository edits, git mutations or published builds.

The GDB endpoint-notification failure and cached-counter trap are documented above. The proposal generator's first broad text match hit both the indexed sprite writer and the fill writer; it was narrowed to the exact indexed-cell header before producing the diffs. No project file was written in that failed attempt. Source/ELF line-map and native function searches needed ordinary scoped reads when CodeGraph did not resolve the new texture helper.

Candidate implementation/build/visual checks are **UNRUN**. Preserve the measured baselines and record any future invalidator. Permanent evidence to retain is this report, CSVs, JSON summaries/manifest and review diffs; raw MI logs and launcher configs remain scratch under `builds/codex-results-pacing`.

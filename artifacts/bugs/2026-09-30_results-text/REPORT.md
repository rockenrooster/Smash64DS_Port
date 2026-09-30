# VS Results bottom text investigation — 2026-09-30

Status: **INVESTIGATION COMPLETE — ROOT CAUSE CONFIRMED; PROPOSAL NOT APPLIED BY THIS INVESTIGATOR.** Nine emulator runs (seven completed; one exporter failure; one exhausted private FAT image, then successful fresh-media retry). Concurrent integrator working-tree adaptation detected at final handoff; its build, qualification and acceptance are not verified here.
Started: 2026-09-30 12:04 CDT. Time box: 2.5 hours / approximately 12 emulator runs.

**Finding:** the source's regular black tint begins at tic 180 for wins / 30 for No Contest. Its native bitmap OBJ cells take lower OAM IDs than the intact text/tag cells and select the BG underneath them instead of compositing the glyph. OAM-index-only controls restore both NO CONTEST and YOSHI WINS!. The attempted BG-priority stack writes did not engage in physical OAM and cannot establish a negative verdict. The proposed native owner repair separates tint/foreground OAM ranges and preserves source text dimming through cached palette banks. Results recorder iterations 120–400 measure nominal **45.691 FPS**, P50 **1** / P95 **2** / maximum **18 VBlanks**; no ARM9 exclusive-work profile or performance acceptance is claimed.

## Owner report and scope

Owner, r57: “bottom text doesn't stay properly (fightername WINS, NO CONTEST, etc)”. Results frame rate “probably should be higher”.

Integrator owns application, builds, and verification. This investigator writes only this evidence directory and `builds/codex-results-text*`, plus expressly authorized slot 0 configuration/storage. Runner: repository-local accurate melonDS slot 0, interpreter/JIT disabled from boot, GDB port 3300. Configuration must be backed up and restored byte-for-byte after every run. No other runner may be touched.

## 1. Recipe and ROM identity

Input: `builds/p2p8-playtest-r57/smash64ds.nds` (63,067,136 bytes) and adjacent `.elf` (27,140,500 bytes). Read-only log confirms revision chain: HEAD `2e093297c5c`, Results heap repair `55934833a50`, parent `1e086261f37`.

- ROM SHA-256: `EEA312301CB9992001E485216DE93758ED19DF90569B3D9538FF267431DEE443`.
- ELF SHA-256: `94CE4B34D9DB351CF181437B18932CFFCB43EED25F1366868E4FE2F42F59BD3C`.
- Slot 0 melonDS SHA-256: `DE80E46BDCF1FD986162DE6AFFD9EE1148F8C40565DBAF667B9C2B3EF5475715`.
- Original slot 0 TOML SHA-256: `F430E1CF4BD43959950765928B0C526C495013C61A151164E326CB07CEA5FA92` (backed up in scratch).
- Slot 0 template-media SHA-256 (read-only source for private media): `86B4F056377B884F74C9C948FF10D5FA0B8EE558C58FF6CC8F7AB26D611C6425`; its index declares a 512 MiB volume. Actual per-run media mutations are isolated in scratch.

Pre-run 1: driver copied from `builds/codex-results-freeze`, extended in `builds/codex-results-text`. Input: Title START; Mode Select down/A; VS A; CSS START; SSS A. At SSS entry, aligned whole-word roster writes seed Mario/Kirby/Fox/Yoshi (kind IDs 0,8,1,6), one human and three level-3 CPUs, one-minute Time match. The word `0x0903ppkk` sets level 3 / handicap 9 (`include/nds/nds_match_config.h:66-70`); handicap is ignored for CPUs. SSS cursor selects Castle (stage 0). No fast logic, budget/texture/source patches, or Results-state writes. Actual match runs to Time Up. Capture stops are on `ndsPlatformEndFrame` entry at source tic N+1, before its OAM commit: the window's latest scanout is approximately requested tic N and may lag another frame during the VBlank boundary. Every capture records exact halted guest tic and frame. Timing measurement is a separate run with all feed/scene breakpoints disabled across the window.

Known retained failures: `../2026-09-30_results-freeze/r57-built.png` and `../2026-09-30_results-freeze/r55-budget-owner-exit.png`, approximately Results tic 200. These establish missing later text; they do not cover its creation.

## 2. Captures by source Results tic

FFA captures complete, run `r57-ffa-captures` (EXIT 0; 106.9 s host). NO CONTEST captures complete, run `r57-nocontest-captures2` (EXIT 0; 41.6 s host).

Links show only the top screen, cropped without resampling from the retained 416×664 window captures at rectangle `(8,54,408,354)` to 400×300. Original window PNGs remain unchanged; [crop metadata](capture-crops.json) preserves their mapping. This crop was visually checked. Screenshot filenames describe approximate completed scanout; the table also gives exact halted guest tic/frame.

| Approximate completed tic | Halted source tic / frame | Bottom text | Capture |
| --- | --- | --- | --- |
| 118 | 119 / 2197 | Absent, before scheduled creation; last battle image | [118](r57-ffa-captures-tic118-top.png) |
| 121 | 122 / 2200 | Fully visible, YOSHI WINS! | [121](r57-ffa-captures-tic121-top.png) |
| 124 | 125 / 2203 | Fully visible | [124](r57-ffa-captures-tic124-top.png) |
| 130 | 131 / 2209 | Fully visible | [130](r57-ffa-captures-tic130-top.png) |
| 140 | 141 / 2219 | Fully visible | [140](r57-ffa-captures-tic140-top.png) |
| 160 | 161 / 2239 | Fully visible | [160](r57-ffa-captures-tic160-top.png) |
| 200 | 201 / 2279 | Gone | [200](r57-ffa-captures-tic200-top.png) |
| 300 | 301 / 2379 | Gone; statistics are visible | [300](r57-ffa-captures-tic300-top.png) |

Machine-readable state: [summary](r57-ffa-captures-summary.json). Detailed debugger data is retained in `r57-ffa-captures.txt`; scratch MI transcript stays under builds and is not printed into the investigation context.

| NO CONTEST approximate completed tic | Halted source tic / frame | Bottom text | Capture |
| --- | --- | --- | --- |
| 2 | 3 / 236 | Fully visible, NO CONTEST, black background during startup fade | [2](r57-nocontest-captures2-tic002-top.png) |
| 10 | 11 / 244 | Fully visible over fading background/fighters | [10](r57-nocontest-captures2-tic010-top.png) |
| 40 | 41 / 274 | Gone | [40](r57-nocontest-captures2-tic040-top.png) |
| 100 | 101 / 334 | Gone; statistics visible | [100](r57-nocontest-captures2-tic100-top.png) |

[NO CONTEST state](r57-nocontest-captures2-summary.json), [decoded hardware/shadow OAM](r57-nocontest-captures2-oam-summary.json). Hardware OAM and OBJ VRAM/palette are the physical evidence; debugger reads of shadow OAM can be stale before its VBlank flush and are not used to establish absence.

## 3. Root cause and source contract

Source: `decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c`. `mnVSResultsFuncRun` creates text and fighters at the same source tic (120; 1 for NO CONTEST). `mnVSResultsMakeString` creates one GObj, link 20, native SObj display on DL link 29, one SObj per announce-font letter. Expected lifetime: until scene exit.

All port line citations in this section refer to the **r57 baseline** (`2e093297c5c` / native Results owner hash `F598BCA7…B54EE`), also frozen as `builds/codex-results-text/proposal-before.c`. The integrator's concurrent edit changes current working-tree line numbers; the retained ROM/ELF remain unchanged.

**Cause: the native full-screen black tint selects its own OBJ pixels over the earlier text/tag OBJ pixels.** Its alpha blend then uses a BG/backdrop pixel, because the DS has one flattened OBJ layer. The previous glyph is no longer available as the second blend target. This is a composition/order failure after healthy source construction and native emission. It is not text destruction, a missing DL-29 camera pass, fighter-floor depth/translucency sorting, or glyph texture/palette eviction.

The complete chain:

1. Source `mnvsresults.c:1142-1225` constructs one link-20 GObj per string, adds `lbCommonDrawSObjAttr` on DL link 29 (`:1184-1185`), and retains its IA8 letter SObjs. Winner name/WINS!/WIN!/team/NO CONTEST are `:1229`, `:1292`, `:1376`, `:1406-1427`. Source text camera captures DL 29 at priority 20 (`:1432-1453`). The scene creates the strings and four fighters at tic 120, or tic 1 for reset (`:2832-2845`, `:3243-3255`). The text objects have no ejection/update route here and remain until scene exit.
2. Source regular tint is created at tic 180 in FFA/Stock/Teams (`:2209-2211`, `:2239-2241`, `:2261-2263`, `:2291-2293`), or tic 30 for No Contest (`:2313-2315`). Its source display increments black alpha by 9 to 128 and alpha-blends the viewport rectangle over earlier pixels (`:1643-1659`). Tint is DL 30 / camera priority 17 (`:1667-1695`), after the text camera. Later header/statistics use camera priority 15 (`:2402-2423`) and remain above the tint.
3. Port wrapper `src/import/battleship_mnvsresults.c:224-227` runs the source tint display, then emits the native tint. Text takes `src/port/sprite_preview_backend.c:1005-1031` → Results foreground selection `:758-762` → `ndsResultsOamDrawGObj` at `:813-817`. Native letters are emitted at priority 0 by `src/nds/nds_results_oam.c:1133-1166`; native tint emits four affine bitmap OBJs covering the viewport at the same BG priority (`:1452-1478`). Both allocate downward from the **same** `sNdsResultsNextOamId`, so the later tint receives lower OAM IDs and wins OBJ overlap.
4. Physical No Contest evidence: letters remain in hardware OAM 111–119, screen Y=148, palette bank 3; later tint is 107–110, alpha 6 at captured 40 / 8 at 100. The combined nine-glyph tile/palette signature is unchanged at 2,10,40,100: `D7C862E3954BDDD341D493E43370F1DCE11F7FF2E2FB0D1CFBF8D07C9AD3F97F`. [Decoded OAM](r57-nocontest-captures2-oam-summary.json) and its per-capture physical binaries establish continued residency. Native OBJ gfx/palette lives in E/main sprite storage while fighter textures use A/B/(borrowed D) and texture palettes F/G (`src/nds/nds_platform.c:583-590`); the measured bytes did not change when the text vanished.
5. Both FFA text GObjs (`0x023bf8c8`, `0x023bfb80`), five letters each, survive through halted tic 301. Flags remain zero, first-letter position `(30,180)` / `(170,180)`, source IA8 format 3/size 1, alpha 255. Native OAM GObj count rises 20 → 573 → 1573 at halted 122/201/301, and emit count 720 → 3264 → 8698. Unsupported-format/provenance/cell/VRAM/palette/OAM failure counters remain zero, with **engaged** sprite emission. Cells occupy 12 through 201, 30 at 301 of 48. [State summary](r57-ffa-captures-summary.json).
6. The stack-word attempts to change bitmap OBJ BG-priority 0→1 did **not engage**: final physical OAM still reports priority 0 at completed 44/100 (No Contest) and 203/300 (FFA). These runs are usable for onset captures, but **invalid as negative priority controls**; no verdict on a priority-only patch is inferred from their unchanged images. In contrast, the register-only index control positively engages: ordinary tint entries change from 107–110 (No Contest) / 102–105 (FFA) to 124–127 at priority 0, bitmap alpha 8. No Contest text/tags return at 44/100 and YOSHI WINS!/tags at 203/300, while source alpha/background darkening and glyph contents remain. No Contest [before](r57-nocontest-index-probe-tic040-top.png) / [after](r57-nocontest-index-probe-tic100-top.png); FFA [before](r57-ffa-index-probe2-tic200-top.png) / [after](r57-ffa-index-probe2-tic300-top.png). This is direct same-ROM causal evidence for the OAM selection seam; the complete disjoint-range/palette production proposal below still needs integration proof.

The DS OBJ-layer limitation is documented by the [melonDS author](https://melonds.kuribo64.net/comments.php?id=241). The source's tint should **dim** earlier text, not remove it. Therefore simply changing priority, omitting the tint, moving source text coordinates, or suppressing fighters is not a valid full repair. No requested sample showed partially missing letters: baseline text is fully visible before this seam, then wholly absent.

## 4. Results presented-frame intervals

Run `r57-ffa-pacing`: [endpoint counters](r57-ffa-pacing-intervals.json), EXIT 0, 107.8 s host. Baseline samples 118 (through recorder tic 119); final samples 399 (through recorder tic 400). Difference: **281 iterations, 120–400 inclusive**. Last-VBlank reference moves 5462 → 5831: **369 VBlanks**. Histogram and maximum reconcile exactly with this total.

| VBlanks per iteration | Count | Percent |
| --- | ---: | ---: |
| 0 | 0 | 0 |
| 1 | 225 | 80.071% |
| 2 | 50 | 17.794% |
| 3 | 2 | 0.712% |
| 4 | 1 | 0.356% |
| 5 | 0 | 0 |
| 6 | 1 | 0.356% |
| 10 | 1 | 0.356% |
| 15+ | 1 | 0.356% |

All other bins are zero. Maximum telemetry was reset at the opening endpoint and returns **18**; there is only one 15+ entry, so that entry is exactly 18. P50 **1 VBlank**; P95 **2 VBlanks** (nearest ranks 141 and 267); maximum **18**. Mean **369/281 = 1.31317 VBlanks**, nominal **60 × 281/369 = 45.691 FPS**. The 2/3/4/5+ counts are 50/2/1/3; 0/1 counts are explicitly retained, not pooled into 2. No ARM9 work-tick profile or largest-cost attribution was collected; VBlank intervals are not substituted for exclusive CPU ticks.

The source recorder samples after update/before draw (`src/import/battleship_mnvsresults.c:560-595`; caller `src/port/taskman_seam_harness.c:127-159`), with one update/draw/present per iteration. These are that existing recorder's interval populations, not a new scheduler. Only two endpoint halts are used; every feed/scene/conditional breakpoint is disabled before continuation. The terminal machine breakpoint is beyond the source exit-input time guard, temporarily opened at 401 instead of 410 with no input. The final memory read of the source tic is 400, consistent with the last completed/flushable frame; the guarded machine stop is on the next update. Histogram counts and reference delta independently agree. Guest time freezes during endpoint halts; stopped-screen FPS HUD 0 is excluded. Concurrent slots 1 and 4–13 were permitted by the owner; no isolation/performance acceptance is claimed. The result shows mostly single-VBlank frames with occasional stalls, and does not establish stable 30 Hz/two-VBlank UI acceptance.

[Numeric distribution and reconciliation](results-interval-distribution.json) records exact bins, percentiles, total VBlanks and the absence of ARM9 work-tick/acceptance evidence.

## 5. Proposed minimal fix (not applied)

**Review proposal only, not applied or built by this investigator.** The first divergence belongs to the existing Results OBJ allocator/composition seam. Allocate all three source tint planes from the 12 highest OBJ entries (116–127), with ordinary source SObjs/fill rectangles below them (0–115). The count is derived from the three actual source callbacks, four viewport cells each; it is not a scene-tic/depth/coordinate workaround. Existing BeginFrame clearing spans both ranges, and source objects/camera masks/lifetimes/textures are unchanged. The stack-priority experiments are unengaged and are not used to select this repair; index-controlled physical OAM/pixels establish the owning seam.

The source black tint must still darken prior text and tags (`mnvsresults.c:1654-1659`). Since DS sprites flatten into one OBJ layer, move this color operation to their existing indexed palette owner: one cached tinted bank per original color bank, refreshed only when source alpha changes. Later headers/statistics keep the original bank. Indexed tile data remains baked and reusable; shade variants do not accumulate. Stage 512 palette bytes and commit alongside OAM in VBlank to avoid recoloring a previous frame mid-scan. Extra native cache metadata/cursor is small; final linked RAM/timing and the 16-bank bound must be verified by the integrator. Prior bitmap OBJs in the tint prefix are only the source black tint planes; the winner badge is constructed/drawn later.

The index-only debugger controls prove the retention seam, **not** the full palette/staging proposal. They deliberately reuse old startup slots and leave foreground colors bright. The production proposal uses disjoint ranges, retains all tint planes, and preserves glyph dimming. No proposal was applied to tracked source. Based on `src/nds/nds_results_oam.c` SHA-256 `F598BCA73E67BAF069AA89D66A5F8780F2652BABAAB4A3108EB36245050B54EE`.

```diff
--- a/src/nds/nds_results_oam.c
+++ b/src/nds/nds_results_oam.c
@@ -2,6 +2,7 @@
 #include <stdint.h>
 #include <string.h>
 
+#include <nds/arm9/cache.h>
 #include <nds/arm9/sprite.h>
 #include <nds/arm9/video.h>
 #include <nds/dma.h>
@@ -27,6 +28,9 @@
 #define NDS_RESULTS_CELL_SLOTS 48u
 #define NDS_RESULTS_PALETTE_BANKS 16u
 #define NDS_RESULTS_MAX_TILES 4u
+/* WallpaperTint, WallpaperTint2 and Tint each emit four viewport cells.
+ * Higher OAM indices keep their bitmap pixels behind indexed foreground. */
+#define NDS_RESULTS_TINT_OBJECTS (3u * 4u)
 #define NDS_RESULTS_FILL_CELL_WIDTH 32u
 #define NDS_RESULTS_FILL_CELL_HEIGHT 8u
 #define NDS_RESULTS_TINT_CELL_WIDTH 64u
@@ -90,6 +94,8 @@
     u8 env_r;
     u8 env_g;
     u8 env_b;
+    u8 is_tint_bank;
+    u8 tint_alpha;
 } NDSResultsOamPalette;
 
 static const NDSResultsOamShape sNdsResultsShapes[] =
@@ -111,13 +117,16 @@
 
 static NDSResultsOamCell sNdsResultsCells[NDS_RESULTS_CELL_SLOTS];
 static NDSResultsOamPalette sNdsResultsPalettes[NDS_RESULTS_PALETTE_BANKS];
+static u16 sNdsResultsPalettePixels[NDS_RESULTS_PALETTE_BANKS * 16u];
+static u8 sNdsResultsTintBank[NDS_RESULTS_PALETTE_BANKS];
 static u32 sNdsResultsIndexedScratch[(64u * 64u / 2u) / sizeof(u32)];
 static u16 *sNdsResultsTintGfx;
 static u16 *sNdsResultsFillGfx[NDS_RESULTS_FILL_CELL_HEIGHT];
 static u32 sNdsResultsCellCount;
 static u32 sNdsResultsPaletteCount;
 static u32 sNdsResultsVramCursor;
-static s32 sNdsResultsNextOamId = 127;
+static s32 sNdsResultsNextOamId = 127 - NDS_RESULTS_TINT_OBJECTS;
+static s32 sNdsResultsNextTintOamId = 127;
 static s32 sNdsResultsPreviousNextOamId = 127;
 static u32 sNdsResultsFrameNeedsCommit;
 static u32 sNdsResultsActive;
@@ -223,7 +232,8 @@
     {
         const NDSResultsOamPalette *palette = &sNdsResultsPalettes[bank];
 
-        if ((palette->prim_r == prim_r) && (palette->prim_g == prim_g) &&
+        if ((palette->is_tint_bank == 0u) &&
+            (palette->prim_r == prim_r) && (palette->prim_g == prim_g) &&
             (palette->prim_b == prim_b) && (palette->env_r == env_r) &&
             (palette->env_g == env_g) && (palette->env_b == env_b))
         {
@@ -231,6 +241,34 @@
         }
     }
     return -1;
+}
+
+static void ndsResultsWritePalette(u32 bank)
+{
+    const NDSResultsOamPalette *palette = &sNdsResultsPalettes[bank];
+    u32 index;
+
+    sNdsResultsPalettePixels[bank * 16u] = 0u;
+    for (index = 1u; index < 16u; index++)
+    {
+        u32 step = index - 1u;
+        u32 inv = 14u - step;
+        u8 red = (u8)(((u32)palette->env_r * inv + (u32)palette->prim_r * step + 7u) / 14u);
+        u8 green = (u8)(((u32)palette->env_g * inv + (u32)palette->prim_g * step + 7u) / 14u);
+        u8 blue = (u8)(((u32)palette->env_b * inv + (u32)palette->prim_b * step + 7u) / 14u);
+
+        if (palette->is_tint_bank != 0u)
+        {
+            u32 remaining = 255u - palette->tint_alpha;
+
+            red = (u8)(((u32)red * remaining + 127u) / 255u);
+            green = (u8)(((u32)green * remaining + 127u) / 255u);
+            blue = (u8)(((u32)blue * remaining + 127u) / 255u);
+        }
+
+        sNdsResultsPalettePixels[(bank * 16u) + index] =
+            ndsResultsRgb15(red, green, blue);
+    }
 }
 
 static s32 ndsResultsAllocPalette(u8 prim_r, u8 prim_g, u8 prim_b,
@@ -239,7 +277,6 @@
     s32 existing = ndsResultsFindPalette(prim_r, prim_g, prim_b,
                                          env_r, env_g, env_b);
     u32 bank;
-    u32 index;
 
     if (existing >= 0)
     {
@@ -257,18 +294,10 @@
     sNdsResultsPalettes[bank].env_r = env_r;
     sNdsResultsPalettes[bank].env_g = env_g;
     sNdsResultsPalettes[bank].env_b = env_b;
-    SPRITE_PALETTE[bank * 16u] = 0u;
-    for (index = 1u; index < 16u; index++)
-    {
-        u32 step = index - 1u;
-        u32 inv = 14u - step;
-        u8 red = (u8)(((u32)env_r * inv + (u32)prim_r * step + 7u) / 14u);
-        u8 green = (u8)(((u32)env_g * inv + (u32)prim_g * step + 7u) / 14u);
-        u8 blue = (u8)(((u32)env_b * inv + (u32)prim_b * step + 7u) / 14u);
-
-        SPRITE_PALETTE[(bank * 16u) + index] =
-            ndsResultsRgb15(red, green, blue);
-    }
+    sNdsResultsPalettes[bank].is_tint_bank = 0u;
+    sNdsResultsPalettes[bank].tint_alpha = 0u;
+    sNdsResultsTintBank[bank] = 0xffu;
+    ndsResultsWritePalette(bank);
     gNdsResultsOamPaletteCount = sNdsResultsPaletteCount;
     return (s32)bank;
 }
@@ -1240,10 +1269,13 @@
     }
     memset(sNdsResultsCells, 0, sizeof(sNdsResultsCells));
     memset(sNdsResultsPalettes, 0, sizeof(sNdsResultsPalettes));
+    memset(sNdsResultsPalettePixels, 0, sizeof(sNdsResultsPalettePixels));
+    memset(sNdsResultsTintBank, 0xff, sizeof(sNdsResultsTintBank));
     sNdsResultsCellCount = 0u;
     sNdsResultsPaletteCount = 0u;
     sNdsResultsVramCursor = 0u;
-    sNdsResultsNextOamId = 127;
+    sNdsResultsNextOamId = 127 - NDS_RESULTS_TINT_OBJECTS;
+    sNdsResultsNextTintOamId = 127;
     sNdsResultsPreviousNextOamId = 127;
     sNdsResultsFrameNeedsCommit = 0u;
     sNdsResultsTintGfx = NULL;
@@ -1307,7 +1339,8 @@
     sNdsResultsCellCount = 0u;
     sNdsResultsPaletteCount = 0u;
     sNdsResultsVramCursor = 0u;
-    sNdsResultsNextOamId = 127;
+    sNdsResultsNextOamId = 127 - NDS_RESULTS_TINT_OBJECTS;
+    sNdsResultsNextTintOamId = 127;
     sNdsResultsPreviousNextOamId = 127;
     sNdsResultsFrameNeedsCommit = 0u;
     sNdsResultsTintGfx = NULL;
@@ -1339,7 +1372,8 @@
         oamClear(&oamMain, first_previous, 128 - first_previous);
         sNdsResultsFrameNeedsCommit = 1u;
     }
-    sNdsResultsNextOamId = 127;
+    sNdsResultsNextOamId = 127 - NDS_RESULTS_TINT_OBJECTS;
+    sNdsResultsNextTintOamId = 127;
     sNdsResultsPreviousNextOamId = 127;
     gNdsResultsOamBeginFrameCount++;
 #endif
@@ -1446,6 +1480,56 @@
 #endif
 }
 
+static s32 ndsResultsTintEarlierIndexedObjs(u32 source_alpha)
+{
+    s32 id;
+    u32 alpha = (source_alpha > 255u) ? 255u : source_alpha;
+
+    if (alpha == 0u) return 1;
+
+    /* The source tint follows these sprites. DS OBJ composition cannot
+     * blend OBJ over OBJ. Earlier indexed sprites use a tinted copy;
+     * later headers/tables retain the ordinary source-colour bank.
+     * The two wallpaper fades precede indexed sprites; only the ordinary
+     * Results tint reaches a nonempty foreground prefix. */
+    for (id = sNdsResultsNextOamId + 1;
+         id < (s32)(128u - NDS_RESULTS_TINT_OBJECTS); id++)
+    {
+        SpriteEntry *entry = &oamMain.oamMemory[id];
+
+        if ((entry->blendMode != OBJMODE_BITMAP) &&
+            (entry->colorMode == OBJCOLOR_16))
+        {
+            u32 bank = entry->palette;
+            u32 tinted = sNdsResultsTintBank[bank];
+            NDSResultsOamPalette *palette;
+
+            if (tinted == 0xffu)
+            {
+                if (sNdsResultsPaletteCount >= NDS_RESULTS_PALETTE_BANKS)
+                {
+                    gNdsResultsOamFailurePaletteFull++;
+                    return 0;
+                }
+                tinted = sNdsResultsPaletteCount++;
+                sNdsResultsTintBank[bank] = (u8)tinted;
+                sNdsResultsPalettes[tinted] = sNdsResultsPalettes[bank];
+                sNdsResultsPalettes[tinted].is_tint_bank = 1u;
+                sNdsResultsPalettes[tinted].tint_alpha = 0u;
+                gNdsResultsOamPaletteCount = sNdsResultsPaletteCount;
+            }
+            palette = &sNdsResultsPalettes[tinted];
+            if (palette->tint_alpha != alpha)
+            {
+                palette->tint_alpha = (u8)alpha;
+                ndsResultsWritePalette(tinted);
+            }
+            entry->palette = (u8)tinted;
+        }
+    }
+    return 1;
+}
+
 s32 ndsResultsOamEmitTintPlane(u32 source_alpha)
 {
 #if NDS_RENDERER_HW_TRIANGLES
@@ -1462,19 +1546,25 @@
         ndsResultsRecordFillFailure();
         return 0;
     }
-    if ((sNdsResultsTintGfx == NULL) || (sNdsResultsNextOamId < 3))
+    if ((sNdsResultsTintGfx == NULL) ||
+        (sNdsResultsNextTintOamId < (s32)(128u - NDS_RESULTS_TINT_OBJECTS + 3u)))
     {
         gNdsResultsOamFailureOamFull++;
         ndsResultsRecordFillFailure();
         return 0;
     }
+    if (ndsResultsTintEarlierIndexedObjs(source_alpha) == 0)
+    {
+        ndsResultsRecordFillFailure();
+        return 0;
+    }
     alpha = ndsResultsBitmapAlpha(source_alpha);
     for (i = 0u; i < 4u; i++)
     {
-        oamSet(&oamMain, sNdsResultsNextOamId, positions[i][0], positions[i][1],
+        oamSet(&oamMain, sNdsResultsNextTintOamId, positions[i][0], positions[i][1],
                0, (int)alpha, SpriteSize_64x64, SpriteColorFormat_Bmp,
                sNdsResultsTintGfx, 0, true, false, false, false, false);
-        sNdsResultsNextOamId--;
+        sNdsResultsNextTintOamId--;
         gNdsResultsOamEmitCount++;
     }
     sNdsResultsFrameNeedsCommit = 1u;
@@ -1582,6 +1672,11 @@
     {
         return;
     }
+    /* Commit palette and OAM together in VBlank; a mid-frame palette write
+     * would recolour the previous frame while its OBJ entries still show. */
+    DC_FlushRange(sNdsResultsPalettePixels, sizeof(sNdsResultsPalettePixels));
+    dmaCopyHalfWords(3, sNdsResultsPalettePixels, (void *)SPRITE_PALETTE,
+                     sizeof(sNdsResultsPalettePixels));
     oamUpdate(&oamMain);
     sNdsResultsPreviousNextOamId = sNdsResultsNextOamId;
     sNdsResultsFrameNeedsCommit = 0u;
```

## 6. Integrator verification

1. Review/apply the unified diff in the existing native Results owner, then build a serialized non-published lab candidate under the integrator's active batch rules. No build was performed by this investigator. Preserve the r57 baseline and record the final candidate's ROM/ELF/config hashes; do not ship a debugger-controlled ROM.
2. Reuse the retained failure and control evidence. On the **built, hard-on candidate**, run the normal capture modes with no index/priority controls: FFA tics 118,121,124,130,140,160,200,300, then beyond the source exit wait 410; No Contest tics 2,10,40,100, then beyond exit wait 200. Include the source tint boundaries near 30/180 and first rows at 60/210. Use physical OAM/VRAM/palette or a post-flush publication for debugger data; unflushed shadow memory and stopped FPS HUD are not verdicts. Source alpha must reach 128, text/tag brightness must dim accordingly, and later labels/statistics must retain source colors even when sharing the untinted original bank.
3. Prove all required text siblings: all twelve winner names (especially long names such as DONKEY KONG/JIGGLYPUFF/CAPTAIN FALCON), WINS!/WIN! according to the compiled region, Red/Blue/Green team text, Time, Stock, and No Contest. Check four fighters/tags, floor/wallpaper, confetti, result numbers and winner badge. Required glyphs must stay visible from creation until ordinary scene exit. No flash, partial letters, disabled tint/content, resource failure, or unmatched state is acceptable.
4. Verify foreground entries stay below 116 and the three tint callbacks occupy only 116–127; old tint slots clear each frame/scene. Verify main OBJ VRAM/cell bounds and the **16 combined normal+tinted palette-bank bound** for the sibling colors, with failure counters zero and positive text/tint emission. The proposal uses one tint copy per original bank, updates colors only as source alpha changes, and stages palette/OAM together in VBlank. There must be no growing bank count over a long Results hold. If combined colors exceed 16, repair the native palette plan before accepting; do not suppress a source sprite/plane.
5. START out through the source's ordinary wait/check, verify CSS, start another match, and enter Results again. Cover four-fighter→two-fighter and No Contest→win transitions; check OAM/palette/blend ownership resets without stale source GObjs/texture keys. Remove transient probe routes and do not use patched scripts in acceptance.
6. Measure the built candidate's Results histogram through the reveal/table phases using an uninterrupted interval window, with honest 0/1 and 2/3/4/5+ reporting. Compare to the r57 data above; collect qualified per-frame work ticks only if required by the integrator's batch. This report does not attribute ARM9 costs or satisfy authoritative pacing acceptance. Coalesce the fix into **one widest relevant Latest qualification** for the frozen integrated batch, per `docs/VERIFYING.md`; do not stack per-edit profiles. Final shipping natural-input/visibility/native-only/performance and owner acceptance remain with the integrator.

Replay command for a **baseline investigation capture** (the legacy `-Arm r54` selector is overridden by explicit r57 ROM/ELF; it does not select r54 here). For the integrator substitute the matching built candidate paths, a fresh tag, and a fresh storage leaf. Use `-Mode nocontest` for the natural pause/reset case, or `-Mode pacing` only for the disclosed telemetry window. The two `*-probe` modes are diagnostics and must not be used to qualify a build.

```powershell
Start-Process pwsh -ArgumentList '-NoProfile','-File','builds\codex-results-text\run.ps1','-Arm','r54','-Tag','review-r57-text','-RomPath','builds\p2p8-playtest-r57\smash64ds.nds','-ElfPath','builds\p2p8-playtest-r57\smash64ds.elf','-SeedAtSss','-Mode','captures','-StorageLeaf','storage-review-r57','-TimeoutSeconds','450','-PermitSlot0ConfigChange' -WindowStyle Hidden -RedirectStandardOutput builds\codex-results-text\review-r57-text.launch.out -RedirectStandardError builds\codex-results-text\review-r57-text.launch.err
# Poll at most once per two minutes; inspect completion before reading evidence.
Get-Content -LiteralPath builds\codex-results-text\review-r57-text.status -Tail 1
```

Actual config: slot 0 / ARM9 port 3300, ARM7 3301; interpreter/JIT disabled from boot; software DS graphics renderer (repository accuracy build), no GL scaling; unthrottled host (`LimitFPS=false`), guest audio running with host volume 0, folder sync off/read-only guest DLDI. Host ROM insertion still writes private FAT media; a fresh leaf prevents accumulation of tagged ROMs. Backup and final restored TOML are checked byte-for-byte by SHA-256 after each run. Stop/capture only this runner's PID. All writes and tools for this investigation stayed under the two permitted scratch/evidence prefixes plus the explicit slot-0 TOML exception; no tracked source/doc edits, Git mutations, builds or subagents.

## Investigation record

- 12:04 CDT: Read project authority, verification, bug process, and harness documentation. Explicit owner investigator scope supersedes shared-board edits and normal commit/build duties for this task.
- 12:09 CDT: CodeGraph available and used before source search. `rg.exe` cannot launch in this environment; scoped PowerShell `Select-String` is the fallback. `Get-CimInstance Win32_Process` is denied; process ownership uses `Get-Process.Path`. Existing Results OAM and interval counters are present in r57 ELF; no build needed. Initial scoped read-only Git diff found no content changes under `src/port`, `src/nds`, or the Results wrapper (line-ending warnings only).
- 12:18 CDT: Run 1 complete. Native text is visible shortly after creation and absent by tic 200; objects remain alive and native emission/capacity counters are healthy. Slot 0 TOML restoration SHA-256 matches the original byte-for-byte. Corrected the roster label: BattleShip kind 8 is Kirby and kind 6 is Yoshi.
- Pre-run 2: NO CONTEST uses the same menu/roster recipe, then normal input handling: DS START when source battle status is Go, then DS A+B+L+R (raw mask 771) while paused. This maps to source A+B+Z+R in `src/port/controller_backend.c:47-59`; `ifcommon.c:3041-3053` sets `is_reset` and ends the match. No direct scene/reset/clock writes. New capture evidence also dumps hardware/shadow OAM, OBJ VRAM, and palette to separate bounded binary files. Capture tics 2,10,40,100 will discriminate startup fade/tint interactions.
- 12:28 CDT: Run 2 `r57-nocontest-captures` reached NO CONTEST by the real pause/reset path, then failed after its first snapshot during shadow-OAM export (GDB expression syntax). It is partial state evidence, not a completed capture run. EXIT 1; TOML restoration hash matches. Collector repaired to evaluate the shadow pointer separately and pass literal numeric addresses. Rerun reason: missing captures caused by this demonstrated collector error.
- Source schedule finding: `mnvsresults.c:2209-2211` adds the ordinary darkening tint at tic 180; `:2313-2315` adds it at tic 30 for NO CONTEST. Native `ndsResultsOamEmitTintPlane` uses four full-screen affine bitmap OBJs at priority 0 (`src/nds/nds_results_oam.c:1472-1476`), the same priority as text (`:1150-1156`). Source text camera priority 20 draws before tint camera priority 17 (`mnvsresults.c:1443`, `:1685`). The predicted composition conflict is established by the physical OAM and index controls in the completed findings above.
- Pre-run 3: repeat only the failed NO CONTEST capture leg with the repaired bounded exporter; identical r57 ROM/config/roster/input route.
- 12:34 CDT: Run 3 `r57-nocontest-captures2` completed all four captures (EXIT 0; 41.6 s host), TOML restored to original SHA-256. Image/OAM analysis is next.
- Pre-run 4: `r57-ffa-pacing`, unchanged ROM, full natural one-minute Time match. Stop on `ndsMNVSResultsRecordFrame` entry at source tic 120 (histogram through 119), disable every existing breakpoint, save buckets/sample count/last-VBlank reference, reset only the maximum telemetry word. Set source exit-input eligibility bound from 410 to 401 as a diagnostic stop gate; no input is sent, and the branch remains closed through tic 400 under both bounds. One unconditional machine breakpoint at `mnVSResultsCheckExit+16` lies beyond that guard and cannot execute before tic 401. At its stop, the histogram contains samples through tic 400. Thus counter differences cover exactly recorder iterations 120–400 (281 samples), including the delayed tic-120 constructor cost, without conditional/per-frame debugger stops in the interior. Both endpoints disclose this diagnostic gate; this is investigation evidence, not a shipping acceptance run.
- 12:41 CDT: Run 4 completed (EXIT 0; 107.8 s host), and original slot 0 TOML hash restored. Raw interval endpoint JSON retained; distribution analysis next.
- Hardware OAM finding: NO CONTEST text stays in entries 111–119 at Y=148, priority 0, palette bank 3. The nine glyph graphics/palette signatures are identical at completed tics 2,10,40,100. At 40/100 the ordinary full-screen tint occupies entries 107–110 at priority 0, alpha nonzero; it precedes the letters in OBJ priority. The DS resolves sprites into one OBJ layer, so a bitmap tint cannot blend another OBJ beneath it; it selects the background instead. Primary explanation: [melonDS author's rendering notes](https://melonds.kuribo64.net/comments.php?id=241). This is consistent with all text and CP tags disappearing while the later header/statistics remain.
- Pre-run 5: `r57-nocontest-priority-probe`, same natural reset recipe. Capture exact onset near completed tics 29,30,31 and missing text at 40. Then transiently change only the aligned `oamSet` priority argument from 0 to 1 at `ndsResultsOamEmitTintPlane+126` in r57, with the source alpha untouched. Capture 41,44,100. This is a causal control, not a source edit or acceptance result; it will also expose any text-brightness difference that a final source-faithful fix must handle.
- 12:52 CDT: Run 5 complete (EXIT 0; 73.5 s host), original TOML restored. NO CONTEST is visible at completed 29 and 30, then disappears; it remains gone at 44/100 after the attempted stack-word priority edit. Final physical-OAM audit at 14:08 finds priority still 0, so this is an **UNENGAGED CONTROL**, not a refutation of a priority-only repair. The source alpha reaches 128; index/palette repair is selected from the later positively engaged index controls.
- Pre-run 6: `r57-ffa-priority-probe`: complete the same natural one-minute match, capture 179/180/181/200, then apply the same temporary priority-argument control and capture 203/300. The control is installed after the source-201 draw has already happened, so restored-text evidence starts at 203, after a subsequent native OAM commit. Purpose: establish the FFA onset and verify the shared cause in the win-text sibling.
- The pre-run 6 onset captures remain useful, but its priority control is also unengaged in the final physical-OAM audit. No priority-field result is promoted. A separate register-only OAM-index control tests the actual selection seam.
- 12:58 CDT: Run 6 complete (EXIT 0; 197.5 s host), original TOML restored. Source priority-only control is not a proposed production repair; onset/negative-control images retained for review.
- Pre-run 7: `r57-nocontest-index-probe`, normal pause/reset and unmodified output through completed tic 40. Thereafter change only the `oamSet` index argument (`r1`) for the ordinary tint's four emissions to 127..124, reusing the startup plane slots as a **diagnostic only**. Source alpha, priority, text positions, textures and palettes stay unchanged. Compare 44/100 after commit. A production repair must allocate distinct OAM ranges for all three live source tint callbacks, rather than overwrite another live plane.
- 13:06 CDT: Run 7 complete (EXIT 0; 84.8 s host), original TOML restored. The register-only OAM-index control restores NO CONTEST and CP/player tags at completed 44 and 100 while the scene stays darkened. Together with unchanged font tile/palette data, this establishes OBJ entry selection as the first divergence. No Contest capture 30 is still visible and 31 is the first missing completed-window capture; native tint creation is source tic 30. FFA 180 is still visible and 181 is gone; native creation is source tic 180. Window scanout is one frame behind the halted update/commit boundary, so exact native onset is anchored by source schedule and physical OAM, not by screenshot filenames alone.
- Source-faithful proposal prepared only in scratch: reserve the three source tint planes' 12 OBJ cells above the foreground OAM range; keep every plane/letter; tint earlier indexed palette colors using the source alpha; commit staged palette words with OAM in VBlank. The preliminary per-frame palette-key rebuild was replaced before delivery by one cached tinted bank per source color; changing alpha updates that bank, with no shade-variant growth or steady-frame palette rebuild. New staging is 512 bytes plus small native cache/cursor metadata, not a source-state mirror or screen compositor. Proposed diff is unbuilt and unapplied; integrator resource/color/pacing verification remains due.
- Pre-run 8: `r57-ffa-index-probe`, same full natural Time match. Compare unchanged missing text at 200 with index-only control at 203/300; purpose is win-text sibling causal confirmation. This is the final planned emulator run, with no profiling campaign or build.
- 13:22 CDT: Run 8 failed before the menu route: at its 600 s deadline, the guest is in `ndsAudioStorageInitHalt(reason=10)`. No Results/timing/causal evidence from this run. EXIT 1; TOML restored to original hash. This is a separate storage/init blocker, not a text verdict, and does not erase runs 1–7. Inspecting its exact reason is bounded; no profiling/build campaign is authorized. Also corrected the CPU-level label from 9 to the seeded struct's actual level 3; this does not change any retained run or capture.
- 13:28 CDT: [Private FAT inspection](storage-failure.json) explains the run-8 blocker: the 512 MiB FAT16 volume has **zero free clusters**. Each fresh tag is inserted by melonDS as another padded 64 MiB guest ROM file; the first seven are full, but the eighth file is truncated to 33,374,208 bytes. `nds_audio_storage.c:397-406` maps reason 10 to failure opening the mapped NitroROM after storage opened. Original host ROM/ELF hashes still match. Retain the failed private medium unchanged; it does not invalidate the full-sized files or completed previous runs.
- Pre-run 9: bounded retry for the missing FFA index control using a **fresh private storage leaf**, `builds/codex-results-text/storage-fresh9`, copied from the original slot-0 template. The scratch launcher now accepts a validated leaf name and redirects config/saves/state there; slot 0 TOML is still restored exactly. Added an early storage-halt breakpoint to avoid another 600 s blind wait. No shared source/media edits and no build. Retry condition changed concretely from a full/truncated private FAT to fresh media. This is the last run planned.
- 13:40 CDT: Run 9 `r57-ffa-index-probe2` completed (EXIT 0; 169.6 s host), original TOML hash restored. The fresh private medium resolves the exact run-8 initialization blocker. FFA captures at 203 and 300 restore YOSHI WINS! and player tags with active darkening after missing text at 200. [State](r57-ffa-index-probe2-summary.json), [physical OAM](r57-ffa-index-probe2-oam-summary.json). All requested baseline captures, timing distribution and both sibling causal controls are complete. No further emulator runs or builds are needed for this investigator's deliverable.
- 14:08 CDT final evidence audit: the priority stack writes did not engage (hardware priority remains 0 in both attempted runs), so their unchanged pictures are not a negative repair verdict. The index controls do engage in registers and physical OAM, and restore both messages. This correction is reflected in all conclusions; no additional runs are needed. ROM and ELF hashes still match; owned slot-0 emulator count is 0 and TOML hash is restored.
- Concurrent integration detected read-only at handoff: current `src/nds/nds_results_oam.c` SHA-256 is `9E07A8706C5EBCE650CFA8104FEFA3A2B0EF3EA8AE28979F84EA78152083647F`. Its working-tree diff adapts the proposed split ranges/cached tinted banks with dirty-word palette staging and a stacked-tint counter. **This investigator did not edit that tracked file.** Preserve the integrator's changes; the report's proposal/citations stay anchored to r57 baseline `F598BCA7…B54EE`. No build/qualification/acceptance of that concurrent implementation is claimed here.

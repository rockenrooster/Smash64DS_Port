# Exact final-wallpaper recurrence experiment

```text
IDEA ID: WALL-FINAL-RECURRENCE-1
HYPOTHESIS: Replace 448 destination-map divisions per changed frame with two
  exact quotient/remainder seeds, reuse adjacent output rows that select the
  same source row, and consume two packed u16 source indices per load.
TARGETED EXCLUSIVE COUNTER: wallpaper; whole draw is the keep gate.
MEASURED UPPER BOUND: profile-1 wallpaper 383,360 median ticks; 49,152 output
  pixels / 24,576 packed stores per changed frame.
LIVE-TREE RECONCILIATION: Started from aad7f2800b with the direct compact-CI4
  checkpoint 95b2a426e2 already present. Preserved all unrelated untracked
  reviews, roadmap files, logs, prompts, and user work.
FILES/FUNCTIONS CHANGED:
  - src/port/sprite_preview_backend.c
    ndsSObjDrawOpaqueWallpaperFinal
  - scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1
BUILD TARGET/FLAGS: smash64ds-battle-playable-coarse-hwtri; profile 1; O2;
  Thumb scene TU / ARM renderer TU. Exactness used the corresponding profile-2
  forensic target. Shipping used profile 0 with the same O2 split.
BASELINE ROM SHA-256: 9B5A4543DFB47C595280A916710D76DD5A8E230CD191D65198D21CB4B0BBCBA7
EXPERIMENT ROM SHA-256: same ROM; the runtime A/B control selected isolated
  baseline and candidate functions. The final canonical ROM is
  F596FE642FA176946E06D318293BC3543FA1A5E4C93D3D7B7A176F6406513163.
FRAME/LOGIC-TICK WINDOW:
  A0 frames 213..244; B0 frames 209..240; A1 frames 214..245. Each arm uses
  32 synchronized warm frames; ROM and emulator identity are identical.
A0 MEDIAN/P95:
  draw 1,954,816 / 1,993,984; wallpaper 383,360 / 383,488.
B0 MEDIAN/P95:
  draw 1,897,920 / 1,938,880; wallpaper 329,024 / 330,304.
A1 OR DISABLED-CONTROL MEDIAN/P95:
  draw 1,954,816 / 1,969,472; wallpaper 383,360 / 383,488.
ACTIVE/WAIT SPLIT:
  A0 1,991,424 / 2,031,424 active and 127,488 / 363,648 wait;
  B0 1,934,464 / 1,976,320 active and 181,312 / 403,776 wait;
  A1 1,991,424 / 2,006,080 active and 129,600 / 387,456 wait.
OWNER SPLIT:
  A0 stage/Mario/Fox 805,440/844,864; 357,760/357,888;
  393,792/394,048. B0 is 803,232/845,056; 357,760/357,824;
  393,856/394,112. The saving stays in wallpaper, as intended.
OP/PROGRAM BYTES/FALLBACKS: no prepared program and no persistent output-frame
  cache. Existing generic unsupported-layout fallback is unchanged.
ITCM/DTCM/BSS/STACK/ARENA DELTA: no ITCM, DTCM, BSS, or arena allocation.
  The final mapper remains in main RAM; its measured compiler local frame is
  68 bytes. The temporary dual-function A/B added 1 KiB ROM and was removed.
SEMANTIC TRACE RESULT: profile 2 preserved the 828-event stage/Mario/Fox trace
  and owner state/cache hashes. Wallpaper independently checked 11,200 exact
  source-map quotients and 1,228,800 final pixels with zero mismatch.
ORACLE/COUNTER/GX RESULT: renderer 2,484/0/0; triangles 828; classes
  648/44/126/10; batches 121/707/121; prepare/reuse 98/730; uploads
  2/36,864; GX RAM remains within the accepted dynamic contract.
CAPTURE RESULT: synchronized canonical pair
  artifacts/visibility/2026-07-13_canonical_fast_041102-8008285-p44652.png
  passed full visibility, pond/stage/bush detail, and paired-motion gates.
VERIFIER COMMANDS AND RESULTS:
  - 32-frame same-ROM profile-1 A/B/A: pass; whole draw -56,896 median ticks;
    wallpaper -54,336; P95 improved.
  - profile-2 8-frame forensic benchmark: pass; all wallpaper and renderer
    oracle mismatches zero.
  - profile-0 realtime smoke and ROM parity: pass; 14.4 FPS; 32-frame draw
    1,859,552 / 1,969,600; ROM parity exact.
  - profile-1 canonical 32-frame benchmark: pass; draw
    1,905,920 / 1,913,472; wallpaper 320,640 / 320,960.
  - docs, architecture, registry, GBI fixtures, generated dry run: pass.
  - DevFast, P1Gate, and Boundary 161/162/163: pass. Full Regression was
    intentionally skipped for the requested fast iteration.
DECISION: KEEP
NEXT MEASURED BOTTLENECK: stage 813,888 median, then Fox 396,416 and Mario
  360,128. Fixed schedules and prepared-VTX memoization are measured dead ends;
  the next renderer experiment needs a coarse owner kernel/direct transport.
```

Rejected variants in this experiment were retained only in logs: DS `div32`
calls plus row reuse saved about 15K; a per-row ARM helper regressed about 31K;
the whole mapper in ARM state regressed about 87K; recurrence plus row reuse
without packed-index unrolling stopped below the 50K whole-draw keep gate.

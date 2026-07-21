# Task 36 Phase B — rigid-stage conservation census

Verdict: **STOP_BELOW_60_PERCENT. Do not build the Phase-B bake.**

## Invalid and rejected arms

The original stalled combined diagnostic was not affine-enabled. Its ROM was
`FC507F5E101BD231A99771A851BE1E92044EBE9A244867AB4F3C257EBCDD46AE` and
its ELF was
`E8B430396B1F9D83ADC16141C4B3902BBD2CA6024B2688E5D7D64869F7C07C37`.
It was profile 1 with Task 29, Task 34, and Task 36 all enabled and
`NDS_FAST_WALLPAPER_AFFINE=0`. Its BSS was 1,853,320 bytes.
That process had already been terminated before the autopsy rule arrived, so a
live chosen-arena value or OOM counter cannot honestly be recovered.

The first standalone arm removed Task 29 and retained only Task 34 plus Task 36.
Its ROM was
`0E1B9475AFB7C6473E0472AD4B191C53FB42A5B3592F5E03892B0252B11E3DE1`,
its ELF was
`E1F24ECC9C6373B502E0982BEE1F3B04BF6A8BE21512EB4C336453A1AF27D1B3`,
BSS was 1,833,160 bytes, and the new pre-battle gate rejected it immediately:

```text
TASK34_ARENA_BOOT=1265664,27
TASK34_ARENA_REJECT=1265664,27
```

That is `0x135000`, 110,592 bytes below the required `0x150000`, after 27
failed allocation attempts. No frame capture was accepted from this arm.

## Admitted standalone arm

The mode-163 Task-34 build cannot enter the opening-action presentation path,
so its unused 153,600-byte opening-action preview cache is compiled out only
when `NDS_TASK34_STAGE_STREAM_CENSUS=1`. The per-DObj census remains bounded at
2,800 entries and 7,000 words. Final BSS is 1,680,360 bytes.

```text
ROM  E5E6F66FACD16FEDE43D072C9646E60E1A29BAD1BA0A9D25C1BE56F22EBC7AA3
ELF  13713DDE9FCF0E565B5B724246A8D7025BAF34AC4EAF5E8033858116B85F0176
profile 1; mode 9; generated segment 0 = 1; static AOT = 1
Task29 = 0; Task34 = 1; Task36 = 1; affine = 0
Task9 ITCM/phase2 = 1/1; Task16 compare/i2f/addsub = 0/0/0
```

Countdown 438–445 and early 600–607 each report exactly one arena row of
`1376256,0`, and every frame contains 2,762 entries / 6,664 parameter words
with zero overflow or faults. Capture identities:

```text
countdown JSON 41E97CD560E520DB5AB90ED0F8188AB9A9372F9C31E2FC92AB4C9B67EF4D8FC7
early JSON     A7881DFEF5CA241A821CC520011A232E0D5A81B8E9A09B225D8D2E6887EEB38A
```

Whispy 1398 did not arrive within the unchanged historical 30-second timeout.
The timeout path killed the first GDB client and attempted the required attach,
but melonDS returned a packet error and the 15-second autopsy attach itself
timed out before yielding PC/backtrace or another arena read. The ROM was not
rerun and the timeout was not extended. Because the same pre-battle arena gate
had not rejected this run, it had already passed `0x150000/0`; nevertheless,
the missing PC/backtrace is recorded as an autopsy-tool failure, not inferred.

## Conservation result

The 26 hardware-composed DObjs are:

```text
0,1,2,3,4,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,40,41,42,54,55,56
```

They own 3,929 of 6,664 stage parameter words (58.959%). Thus even an impossible
100%-constant rigid set cannot satisfy the 60% whole-stage continuation gate.
Across the 16 valid frames, 3,896 rigid words are constant:

```text
3,896 / 3,929 rigid words = 99.160%
3,896 / 6,664 stage words = 58.463% (gate 60.000%)
```

Only DObjs 0, 40, and 54 vary. Each has the same 11 varying words in one class-9
`MATRIX_LOAD4X4`: matrix lanes `0,1,2,5,6,8,9,10,12,13,14` (per-DObj flattened
ordinals `18,19,20,23,24,26,27,28,30,31,32`). These are the three live camera
view loads; local matrices, vertices, and prepared state remain constant.

Adding Whispy frames can only remove words from the conserved intersection, so
the two-window 58.463% is already a mathematical upper bound. Phase B stops at
the mandated gate. Phase A's approximately 40K-tick hardware-compose KEEP is
retained unchanged; no bake buffer, replay path, DMA path, or device ROM is made.

Focused GBI/native-stage fixtures and canonical Boundary pass after the census
and fail-closed arena/autopsy changes. The selectors remain default-off.

# Task 36 Phase B — rigid-stage conservation census

Verdict: **USER-OVERRIDE KEEP at 58.463%; replay and profile-0 promotion pass.**

## Superseding product decision and final replay result

Tyler explicitly accepted the measured 58.463% conservation on 2026-07-20,
superseding this task's original 60.000% continuation threshold. The census
below remains the evidence for the exact admitted subset; it is no longer a
stop verdict.

Mode 2 captures and replays only complete rigid segments 0, 5, and 7 (mask
`0xA1`). That is 33 runs, 26 rigid bindings, and exactly 3,916 DS FIFO words in
a fixed 4,608-word BSS buffer. Dynamic segments remain on the live path. The
final engagement row is:

```text
445,2,1,1,0,443,3,33,3916,0,0,0,3916,1376256,0,3916,161,0,0
```

This proves READY state 2; one bake attempt/success and zero failures; 3 replay
segments / 33 runs / 3,916 words; full arena `0x150000` with zero allocation
failures; captured mask `0xA1`; and zero fallback, guard rejection, capture
fault, or active capture.

Synchronized frames 438--445:

| Metric | Phase-A control P50/P95 | Replay P50/P95 | Delta |
|---|---:|---:|---:|
| Stage | 430,368 / 430,528 | 284,320 / 284,544 | -146,048 / -145,984 |
| Draw | 832,736 / 835,776 | 704,672 / 707,712 | -128,064 / -128,064 |
| Active | 836,832 / 945,600 | 708,672 / 711,680 | -128,160 / -233,920 |
| Update | 210,848 / 510,528 | 210,944 / 212,992 | +96 / -297,536 |
| Loop | 1,120,256 / 1,680,512 | 1,120,256 / 1,120,320 | 0 / -560,192 |

The replay image is exactly 0/49,152 changed pixels versus Phase A (mean 0.00,
100% overlap), with the main floor and all three platforms visible. Frames
600--607 and 1398--1405 retain the exact 202 triangles / 54 runs / 8 segments /
57 DObjs / 42 bindings / 49 epochs and remain engaged with zero fallback.

```text
Phase-A control ROM 08B8D0D78F4CFF56F58E13C291A99376D1FCD337BAA89DA633F706A122592BDF
Replay lab ROM      048F32EDD864D638228ACCEB5C61ADA0D8FAD1D8DFB8D7BCF0906F41C74762A2
Published p0 ROM    C1B3DDE3044BFF2C5F9B66F9D5CFFE7E4600A0467F43CB1CF032D3E086460761
Published p0 ELF    6E21517AA50D6C479A967AD38F65C7DF28750D3E1AC364BA045EBD400CBA74A5
```

The profile-0 one-minute Time Up/Results soak and final Boundary pass with Task
36 mode 2, generated segment 0, affine BG-0, static textures, and Task-32 hot
text enabled. Task-32 substitutes the replay emitter for the superseded generic
commit owner and fits 6,760/8,192 bytes. Retail remains the performance referee.

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
the two-window 58.463% remains the mathematical upper bound used by Tyler's
override. The final replay keeps only complete rigid segments; DMA remains
unimplemented.

Focused GBI/native-stage fixtures, the profile-0 one-minute lifecycle, and
canonical Boundary pass. The generic selector default remains zero; the
published profile-0 target intrinsically selects mode 2.

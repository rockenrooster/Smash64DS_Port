# Task 26 generated M3 segment 0

Date: 2026-07-18

Decision: **KEEP the exact segment-0 cut; stop architectural expansion at this
atomic unit.** The retained cut is independently exact and positive. Retail
evidence is one user-supplied A/B observation only; no repeatability or general
hardware-speed claim is made, and no additional retail repeat is requested.

## Bound

The generated program owns Dream Land segment 0 / `layer0` only:

- 21 DObjs and bindings 0-19;
- runs 0-25, 54 triangles, 22 epochs;
- 108 dense vertices, 123 immutable state effects, and 90 synchronization
  effects;
- no material callback event.

The generated table fixes immutable run order, class, topology, state effects,
and callback boundaries. Live generation/stamp admission, 42 composed matrices,
four material snapshots, current renderer configuration, matrix and near-plane
work, texture/color/alpha/UV selection, validation, the existing commit loop and
GX emitters, and fail-closed fallback before GX remain live. There is no runtime
opcode scan, packet copy/patch, per-frame list construction, sorting, second
topology cache, or post-GX fallback.

## Exactness

- The generated and current CPU-preparation traces match for frames 438-445:
  2,775 words per frame, 26 rows, and zero differences.
- Exact owner conservation is 15,126 words: 7,011 stage + 4,130 Mario + 3,985
  Fox.
- The live-mutation falsifier reports inject/mismatch/revalidate `1/1/1` and
  zero ARM faults, proving validation occurs before GX.
- Exact stage/frame contracts remain 8/255/57/42/54/202/49/4, 121/828, and
  202/320/306, with zero post-GX fallback, fence, or conservation failure.
- Both synchronized production and hardware-pair top-screen comparisons are
  `0/49,152` changed native pixels, mean delta 0.00, overlap 100%.
- The refreshed one-minute lifecycle completes 4,084 unchanged source updates,
  2,042 presentations, one teardown, the exact `439/292/154` Mario-KO source
  sequence, Results, and 166,672 bytes net reserve. Stable-30 remains correctly
  red.

The exact static residency repair adds the naturally observed Whispy mouth
asset 152 and late Fox material asset 313. The resulting corpus has 24 keys, 23
deduplicated outputs, 132,096 payload bytes, and 136,192 prepared bytes across
VRAM A+B. The checker reports 24 hits, 23 outputs, 1,344 classified field
misses, three explicit misses, and six invalid-key falsifiers. Payload SHA-256
is `59BFD565C4C0C18C605107E0F7B49E4CC6C360CD9CE9A67712066C2A58E182D2`.

The lifecycle also exposed a real audio-owner defect: the software owner could
lag a completed Calico hardware channel and stop a newly acquired sample.
Retiring only an exactly completed owner fixes the failure while inconsistent
ownership remains fail-closed. The focused audio qualification passes with
21 supported starts, 17 explicit unsupported cues, phase mask `0x1f`, channel
mask `0xe`, maximum three live channels, 59 envelope steps, and 187,152 bytes
headroom. This is automated channel/acoustic evidence, not a new ear test.

## Performance

All emulator rows are synchronized eight-frame P50/P95 ARM9 ticks against the
Task 25R control representation:

| Phase | Control stage | Generated stage | Stage delta | Draw delta |
|---|---:|---:|---:|---:|
| Countdown / GO | 464,128 / 464,320 | 460,544 / 460,608 | -3,584 / -3,712 | -3,296 / -3,264 |
| Early combat | 464,352 / 464,448 | 460,736 / 461,056 | -3,616 / -3,392 | -3,680 / -3,648 |
| Whispy material edge | 464,480 / 467,712 | 460,928 / 464,064 | -3,552 / -3,648 | -3,488 / -3,584 |
| Whispy steady | 464,288 / 464,576 | 460,864 / 461,056 | -3,424 / -3,520 | -3,104 / -3,200 |
| Natural KO | 468,256 / 468,480 | 464,640 / 464,896 | -3,616 / -3,584 | -3,552 / -3,520 |

The current ARM/update-hot hardware-HUD melonDS pair moves Countdown stage
465,472/465,600 to 455,232/455,232 (-10,240/-10,368), draw
1,148,864/1,217,088 to 1,137,632/1,205,888 (-11,232/-11,200), and active
1,152,896/1,221,184 to 1,141,728/1,210,048 (-11,168/-11,136). Loop remains
1,680,448 at P50 and changes only +64 ticks at P95, within the same VBlank
bucket.

The single retail A/B supplied by the user is:

| ROM | UPD | DRW | ACT | LOOP |
|---|---:|---:|---:|---:|
| generated segment 0 | 330,944 | 1,706,688 | 1,709,440 | 2,240,384 |
| control | 1,547,072 spike | 1,728,256 | 1,705,152 | 2,240,768 |
| delta | unusable | -21,568 (-1.25%) | +4,288 (+0.25%) | -384 (-0.02%) |

`ACT < DRW` in the control photograph is valid because the HUD rows are sampled
on staggered refreshes. UPD is excluded because the user observed the control
spike while that row refreshed. The draw improvement and unchanged VBlank
bucket are device-informed KEEP evidence, but one sample cannot establish a
repeatable cache/layout effect.

## Footprint and placement

- Canonical `ndsRendererPrepareNativeStageOwner`: 8,292 bytes, ARM.
- Generated-run helper: 1,472 bytes, ARM.
- Compact hot-run table: 52 bytes.
- Canonical owner/helper local frames: 192 / 168 bytes including saved
  registers.
- Instrumented A/B loaded footprint delta: +7,748 bytes (+20 ITCM, +7,696 main
  text/rodata, +32 BSS); the profile-1 device pair ROM grows 6,144 bytes.
- Canonical ITCM is 28,820 / 32,768 bytes, leaving 3,948 bytes.

The representative cut exceeds the 8 KiB emulator continuation threshold only
in the hardware-style pair and produces a favorable single retail draw result.
Because broader generated-code working-set performance cannot be falsified
without another device A/B and the user has declined repeats, controlled
expansion stops. This architecture-expansion gate does not force reversion of
the smaller exact segment-0 win.

## Evidence identity

- CPU traces:
  `2026-07-18_task26-segment0-cpu-prep-trace-e0.json`
  (`14F263B7C51A468562C74E7AFCB0508EAC455BECE21F72A2CC6C127A7983FD9A`)
  and `2026-07-18_task26-segment0-cpu-prep-trace-e1.json`
  (`486464C680B4CD13479E5A8324FA4AA0297A0B7EE7EEC720786B6FE77D4CD5DC`).
- Hardware-style melonDS captures:
  `2026-07-18_task26-hardware-control-melonds.json`
  (`E2D37CFC3C6F58AE5ED48056498FB48F90A274009E15FFFC7E6BA6B0F06830CE`)
  and `2026-07-18_task26-hardware-generated-melonds.json`
  (`620BA8E56785A30A63D0FF7C094DDB42FA1B56EB9BCF39FCBC53A1BF8E7EA443`).
- Live-fault capture:
  `2026-07-18_task26-segment0-shadow-livefault-438-445.json`
  (`848D6A82F1310E261E6A62728B2678F7ADDEE146312F4F587A47EDBFC48EDA3F`).
- Matching lab ROMs:
  `builds/task26-hardware-hud-pair/smash64ds-task26-control.nds`
  (`6D84790AFBABFDC2E0EC88757ED34C1206DA95116E58098C904770AD9430E563`)
  and `smash64ds-task26-generated-segment0.nds`
  (`8D53537CE42652915ED3DF99EE0E07CA445D09BEF888990299D5AD702B268A6A`).
- Published candidate:
  `smash64ds-battle-playable-hwtri.nds`, 14,681,088 bytes,
  SHA-256 `757ED78612607BEB8780BF197CC701570926B52EBDD745368DC32B6D44AC89E4`.

Next: Task 23R Phase 1 must remeasure only the residual work left after this
generated cut. A cache is permitted only if exact complete-key hits are at
least 20% and key-computation cost is less than half the avoided residual.

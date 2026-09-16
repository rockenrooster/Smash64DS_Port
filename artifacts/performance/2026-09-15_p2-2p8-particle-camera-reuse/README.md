# P2-2p8 particle camera reuse

Verdict: **KEEP** the renderer-owned battle-camera reuse route. The temporary
same-ROM selector used to measure it has been removed; retained code attempts
the renderer camera route directly and keeps the correctness fallbacks.

The discriminating window is presented frames 1400..1527 on the four-CPU
tick-HUD instrument. Two same-ROM comparisons both favored camera reuse:

| ROM / pair | Candidate wins | Ties | Losses | WORK-H median delta | WORK-H mean delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| `B0A929A0BD83...` control -> candidate | 100/128 | 0 | 28 | -3,840 | -4,962 |
| `1E5155DB3FC4...` fixed control -> fixed candidate | 112/128 | 1 | 15 | -6,464 | -7,078 |

Positive engagement was present in the measured candidate: 4,578 renderer
camera reuses, 3,054 cache hits and 1,527 cache misses. The fixed candidate
also recorded 1,526 fixed-basis camera uses.

These are short matched screens used for a route verdict, not the P2 release
performance gate. They do not prove stable 30 FPS or the all-lineup/all-stage
acceptance matrix.

## 2026-09-16 continuation reconciliation

The retained implementation is the hard-on renderer-camera reuse path in the
current `941f4daab56` overlay. The temporary selector from the comparisons above is
absent. The later pose-to-draw pilot is separately recorded as rejected/removed;
the matched pose-topology arm also regressed WORK-H from P50 1,644,544 / P95
2,366,976 to P50 1,645,824 / P95 2,391,040 and is not retained. A subsequent
packet live-flush probe left no retained source delta and is not part of this batch.

The current post-experiment ARM9 profile is preserved at
`../2026-09-15_p2-2p8-current-profile/`. The camera-reuse full stress capture in
this directory recorded 1,972 samples with WORK-H P50 1,600,704 and P95 2,336,896;
that remains sizing/integration evidence, not acceptance. The next unfinished action
was the focused particle-bank check; it passed on 2026-09-16 with 110/119 reachable
scripts, 41/47 reachable texture IDs and five 8 KiB atlas sheets. The later normal
hard-on build regenerated the canonical atlas to 48 packed frames; the focused
checker was rerun against that final generated state and remained GREEN.
The next unfinished action is one Boundary qualification of the frozen integrated
candidate.

Review intake watermark: `docs/reviews/Briefs/README.md` was inspected on
2026-09-16. Its visual/menu runtime candidates are outside this active performance
batch; they remain preserved for their owning rows.

## 2026-09-16 integrated qualification

The retained hard-on renderer-camera path completed the full Boundary profile on
runner 12. `builds/verify-boundary-p2p8-camera-reuse-r5.log` ends with
`Boundary verification profile passed.` The four-CPU arm collected 1,972 samples
(frames 2..1973) and recorded WORK-H **P50 1,660,224 / P95 2,380,288** ticks,
camera-reuse engagement **5,919**, native failures/direct rejects **0/0**, and
general-heap low-water **108,096 B**. The stress verifier passed its correctness,
cadence-accounting, native-owner and memory gates. The shell-driven realtime arm
still warned at **25.8 FPS**, so the product performance target remains RED.

After Boundary, the normal hard-on root build completed with
`NATIVE_ONLY_PASS: smash64ds.elf, 262 actual link inputs`. Final identities are:

- `smash64ds.nds` SHA-256
  `77A047D1F1B1D0EDFB3D6F931B11E35EE677EFE5F6EB722713ABBBABD6EEFEBB`
- `smash64ds.elf` SHA-256
  `2A83B39B92FC32E2516B738299001422FA81F5B4C3549E916E09321015CCB75A`
- `builds/build/nds_build_config.h` SHA-256
  `BDFE59516B2F8DBAB0C7A01C60336772475D03D6FD6A968DCBC9F3CAF298F0E0`

That config has fast logic off, hardware triangles on, tick-HUD instrumentation
off, particle draw/runtime enabled, the particle camera cache enabled, and the
fixed camera route enabled. No measurement selector remains. This closes the
particle-camera reuse batch as a retained implementation; it does **not** close
P2-2p8 or the remaining roster/stage release matrix.

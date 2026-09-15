# P2-2p8 Link live-texgen packet replay

Date: 2026-09-14

Verdict: **KEEP.** Link no longer has to decline the retained native fighter
packet path solely because its source model uses live `G_TEXTURE_GEN` UVs.
Recorded packets now remember bounded texgen run/site metadata and refresh only
their `FIFO_TEX_COORD` words from the current modelview + LookAt before DMA.
The math is the same Q15 path used by the ordinary native owner. Any metadata,
root, dense-id or LookAt failure invalidates the optimization and returns to the
ordinary native owner.

## Synchronized eight-frame A/B

Frames 438..445, standard four-kind CPU stress, identical build-config SHA-256
`23314b62fbbeccac413a9d677555eb1233137f84c26d5d9df0177f801f5415f3`.
The preserved pre-change control ROM is
`71236c5fa06c1c07365a159894d336bf1ae72bbd067b357a66ad76983ffb110f`;
the candidate ROM is
`5d3c022cc09248d901f7aa5a9129db3b898457e6a301d094c417d8c76ab7a061`.

| bucket/counter | control | candidate | delta |
|---|---:|---:|---:|
| FTR P50 | 643,840 | 407,232 | **-236,608** |
| FTR P95 | 670,528 | 430,016 | **-240,512** |
| WORK-H P50 | 1,786,816 | 1,544,704 | **-242,112** |
| WORK-H P95 | 2,414,912 | 2,190,400 | **-224,512** |
| ALL P95 | 2,797,952 | 2,238,144 | **-559,808** |
| packet hits | 856 | 1,141 | +285 |
| packet records | 12 | 27 | +15 |
| packet faults | 0 | 0 | 0 |
| packet declines | 300 | 0 | **-300** |
| successful live-texgen patches | — | 260 | engaged |

The packet witness is important: the candidate did not merely move code or
timing. Link exercised the new live patch route 260 times in the same run and
the old persistent decline population disappeared with zero packet faults.

## Whole-match four-CPU stress

The retained candidate completed the configuration-exact one-minute stress:
1,972 timing samples, frames 2..1973, 98.33% of the 60-second source match,
four CPUs/four fighters, active mask `0xF`, no item override.

Compared with the immediately preceding Link/Kirby BPS1 checkpoint:

| bucket | previous P50 / P95 | candidate P50 / P95 | delta P50 / P95 |
|---|---:|---:|---:|
| ALL | 2,237,888 / 3,358,080 | 2,237,824 / 2,798,464 | -64 / **-559,616** |
| FTR | 631,552 / 771,840 | 394,304 / 761,728 | **-237,248** / -10,112 |
| SRC | 566,592 / 1,267,136 | 565,056 / 1,254,976 | -1,536 / -12,160 |
| SINT | 260,992 / 838,144 | 260,544 / 811,648 | -448 / -26,496 |
| WORK-H | 1,901,568 / 2,721,408 | 1,670,976 / 2,539,584 | **-230,592 / -181,824** |

Hard guards remain clean: native failures/direct rejects 0/0, graphics-heap
overflow/no-room 0/0, weapon refusals 0, pose bind-full 0, all four fighter
slots draw, animation-stream failures 0. General-heap low-water is 118,752 B,
93,152 B above the 25,600 B safety floor.

Cadence improves materially in the slow tail (`5+` VBlanks 547 -> 313), but the
product gate is still RED: only 95/1,973 = **4.82%** of presented frames are at
2 VBlanks, versus the required >=95%, and whole-frame P95 remains above the
1.12M-tick budget. This closes a high-impact P2-2p8 slice, not P2-2p8 itself.

## Correctness / visibility

`candidate-sparse448.txt` proves the intended four-kind roster and draw mask,
zero fighter/native rejects, zero packet faults/declines, and four valid packet
slots. `scripts/fighters/test_camera_texgen_state.py` passes 6/6 and the full
native-owner geometry-closure checker passes every owner/detail.

Visibility evidence is
`artifacts/visibility/2026-09-14_p2-2p8-link-texgen-packet.png`. The software
renderer capture shows Dream Land plus all fighters, with Link rendered on the
upper platform after the live-texgen route engaged. Its automated content guard
(`...-analysis.json`) records 99.8429% nonblack gameplay pixels, 4,939 RGB
colors, all four image quadrants populated, guard PASS.

## Umbrella status

The required `verify-all.ps1 -Profile Boundary` progressed through toolchain,
docs, owner wiring, generator staleness and related static checks, then stopped
at the existing architecture rule because the owner workspace contains
untracked `decomp/alt_assets/`. That read-only owner input was preserved. The
configuration-exact `p2_fourcpu_stress` child and focused renderer/geometry
checks above pass; the Boundary umbrella is therefore reported **BLOCKED/RED in
this workspace**, not promoted to GREEN.

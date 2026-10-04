# Gate at HEAD, 2026-10-04

Tree `3472eaf646a` (+ owner WIP), target `smash64ds-p2-fourcpu-tickhud-hwtri`,
fresh build `build-gate-1004a`, default Dream Land roster, items on.
`scripts/sample-tick-hud-buckets.ps1 -RingDump -Samples 1960 -StartFrame 2`.

| | P50 | P95 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|
| WORK | 960,064 | 1,324,672 | 384 / 1,960 | 1,562 / 1,961 |
| WORK-H | 941,440 | 1,300,992 | | |

344 of the 384 over-gate frames fall after presented frame 800, when the
item spawns pile up: frames 1400-1960 read WORK 1,078,764 mean against 886,397
for 440-800, item draws (MITM) 100,283 against 454. Against the 10-03 gate
(`../2026-10-03_items-exit-gate`, P95 1,292,352) the simulation diverges at
sample 798 (`compare-replay-digest.py --sequence --resync 4`), so the item
spawns -- and the item draw cost -- differ; the two rows are not a like-for-like
cost comparison.

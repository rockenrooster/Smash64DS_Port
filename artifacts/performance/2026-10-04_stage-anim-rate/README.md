# Stage animation at a reduced rate: priced, parked, 2026-10-04

## Probe

The owner's compromise list allows decorative stage animation at a reduced
rate. A lab word in `gcPlayAnimAll` (since removed) played ground GObjs'
animations on one tick in N, staggered by GObj, freezing in between (a cost
probe, no catch-up). Gate ROM `build-gate-anim`, 1,960 presented frames each,
`-BootSetGlobals 'gNdsStageAnimDivisor=N'`.

| N | WORK P50 | WORK P95 | > 1.12M | two-VBlank | digest vs N=1 |
|---:|---:|---:|---:|---:|---|
| 1 | 870,720 | 1,212,096 | 192 | 1,757 | -- |
| 2 | 921,728 | 1,258,304 | 254 | 1,685 | diverged at sample 130 |
| 4 | 916,544 | 1,272,000 | 280 | 1,659 | diverged at sample 130 |

## Reading

On Dream Land a ground GObj's animation feeds gameplay: the replay digest
diverges at sample 130 with every divisor, and the reshuffled match reads
~50K worse at P50. Ground animation is not decorative wholesale; a reduced
rate needs a per-stage list of GObjs proven inert (digest identical with only
those slowed), and the probe cannot price the decorative share. Parked.

# Material animations at the presentation rate, 2026-10-05

`src/import/battleship_sys_objanim.c`: an MObj's material animation (colours,
texture/palette indices, scroll) is render state the replay digest does not
fold. The battle host publishes `gNdsMObjTickMul` per tick
(`src/port/taskman_seam_battle_host.c`): 0 on a batch's earlier tick (the MObj
parser and player return untouched; gcPlayAnimAll keeps its DObj work and
skips the MObj collect, play and colour pass), and on the drawn tick the
batch's tick count, run as one parse + play at that multiple of the MObj's
speed -- the same waits, lengths and values two source steps leave. Source
rate kept for the costume bake (`lbCommonAddMObjForFighterPartsDObj`, which
removes its AObjs right after) and Yoshi's Island's clouds (gryoster.c reads
their MObj `anim_wait`). Same-ROM A/B word `gNdsMObjTick30Hz` (0 = every tick).

## Official gate (`build-gate-1005c`, same ROM, 1,960 frames)

| arm | P50 | P95 | > 1.12M |
|---|---:|---:|---:|
| `gNdsMObjTick30Hz=0` (`gate-mobj0`) | 832,704 | 1,149,376 | 117 |
| `gNdsMObjTick30Hz=1` (`gate-mobj1`) | 827,904 | 1,140,992 | 112 |

Paired median -4,800; replay digest identical on every frame.

## Lab sweep (`build-lab-sweepall7` -> `-sweepall8`, cross-ROM)

Replay digest identical on all nine stages (`a8-g*.csv` against
`../2026-10-05_diag-batch4/a7-g*.csv`). Cross-ROM paired medians move both ways
(Castle +1.2K, Sector Z -1.2K, Jungle +4.0K, Zebes -4.3K, Hyrule +4.0K, Yoshi
+2.6K, Dream Land -1.0K, Saffron -0.2K, Mushroom Kingdom +1.7K): layout drift
between the two lab images. Same-ROM arms: `a8m0-g*.csv` (word 0) against
`a8-g*.csv` (word 1).

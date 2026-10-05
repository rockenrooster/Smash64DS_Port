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

## Lab sweep, same ROM (`build-lab-sweepall8`, word 0 `a8m0-g*` -> word 1 `a8-g*`)

| stage | P50 | P95 | > 1.12M | paired median |
|---|---|---|---|---:|
| Castle | 960,640 -> 956,992 | 1,286,464 -> 1,282,944 | 305 -> 295 | -2,752 |
| Sector Z | 1,029,824 -> 1,026,880 | 1,396,480 -> 1,389,184 | 606 -> 586 | -3,456 |
| Jungle | 1,003,712 -> 1,003,008 | 1,410,048 -> 1,403,968 | 537 -> 524 | -2,304 |
| Zebes | 984,512 -> 975,168 | 1,287,808 -> 1,275,136 | 352 -> 316 | -8,960 |
| Hyrule | 851,968 -> 850,624 | 1,101,824 -> 1,096,704 | 80 -> 77 | -1,792 |
| Yoshi's Island | 1,042,688 -> 1,036,544 | 1,382,656 -> 1,376,704 | 597 -> 584 | -3,968 |
| Dream Land | 895,488 -> 889,472 | 1,225,984 -> 1,215,936 | 198 -> 193 | -4,992 |
| Saffron | 1,022,144 -> 1,015,744 | 1,371,776 -> 1,369,280 | 516 -> 498 | -4,608 |
| Mushroom Kingdom | 1,024,704 -> 1,019,776 | 1,316,608 -> 1,308,416 | 473 -> 449 | -4,608 |

Replay digest identical on every stage in both arms.

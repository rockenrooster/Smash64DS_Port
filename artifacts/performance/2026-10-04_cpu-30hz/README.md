# CPU decisions at 30 Hz (owner ruling D12d): measured, not shipped, 2026-10-04

## Change measured

`src/import/battleship_ftcomputer.c`, A/B word `gNdsCpuDecide30Hz` (default 0
= source behaviour): on a presentation batch's earlier tick
(`gNdsFtPoseEvalTick == 0`) a VS-battle CPU whose input script is idle skips
the decision block of `ftComputerProcessAll` (only the behaviour-change
countdown ticks), so each CPU decides at most once a presented frame. Humans
and 1P are unaffected. It changes the match, so the replay digest changes.

## Result

Gate ROM (`build-gate-1004p`), 1,960 presented frames each. Seed 1 is the
official scenario (the port never reseeds `sSYUtilsRandomSeed`); seeds 2-4
poke it at boot (`-BootSetGlobals 'gNdsCpuDecide30Hz=A,sSYUtilsRandomSeed=S'`).

| seed | arm | WORK P50 | WORK P95 | > 1.12M | two-VBlank |
|---:|---|---:|---:|---:|---:|
| 1 | off (`gate-ai0`) | 898,624 | 1,241,408 | 233 | 1,723 |
| 1 | on (`gate-ai1`) | 891,456 | 1,248,384 | 241 | 1,712 |
| 2 | off | 886,784 | 1,208,960 | 169 | 1,781 |
| 2 | on | 858,432 | 1,249,024 | 210 | 1,736 |
| 3 | off | 855,104 | 1,196,608 | 161 | 1,790 |
| 3 | on | 871,936 | 1,208,320 | 158 | 1,789 |
| 4 | off | 888,832 | 1,265,024 | 212 | 1,740 |
| 4 | on | 852,928 | 1,219,008 | 199 | 1,755 |

(Seed 1 was measured on an earlier ROM than seeds 2-4.)

## Reading

The match itself moves P95 by tens of thousands: seed 1-4 with the source AI
span 1.197M-1.265M. Over seeds 2-4 the 30 Hz arm's mean P50 is -15.8K and
its mean P95 +1.9K -- within the match-to-match spread, so its P95 effect is
not resolved by four matches. Not shipped; the word stays at 0. Two further
consequences for the gate: a lever that changes the match moves the official
P95 by the reshuffle as much as by its cost, and the done criteria's
worst-case search across matches needs margin under 1,120,000 on the
official seed (on the same ROM, seed 4 reads 1,265,024 against seed 1's
1,222,848, `2026-10-04_attr-dedup/gate-ad1`).

# All-stage worst case on the clean sweep ROM, 2026-10-05

`build-lab-clean1005`: the gate target with every `NDS_P2_STAGE_*` and the
sweep's admitted kinds, built with the new `NDS_LAB_FOURCPU_WORDS=1` (Makefile):
the boot-poked stage/roster words of `NDS_LAB_FOURCPU_SWEEP` without its lab
instruments (stage and Fox GX FIFO hashes, item/PIM/baked accumulators, the
Fire Flower state recorder, lean-event and status-change columns). Preset
four-CPU roster, items on, 1,960 presented frames, ring dump, one run a stage
(`k1-g<gkind>`).

| gkind | stage | WORK P50 | WORK P95 | two-VBlank |
|---:|---|---:|---:|---:|
| 0 | Peach's Castle | 888,000 | 1,206,272 | 1,778 |
| 1 | Sector Z | 931,584 | 1,278,016 | 1,651 |
| 2 | Congo Jungle | 913,280 | 1,280,832 | 1,667 |
| 3 | Planet Zebes | 890,880 | 1,195,328 | 1,779 |
| 4 | Hyrule Castle | 767,552 | 1,012,288 | 1,917 |
| 5 | Yoshi's Island | 947,840 | 1,274,688 | 1,640 |
| 6 | Dream Land | 815,616 | 1,124,992 | 1,849 |
| 7 | Saffron City | 924,544 | 1,249,280 | 1,710 |
| 8 | Mushroom Kingdom | 936,448 | 1,218,560 | 1,738 |

The lab sweep ROM read 93K-125K higher on the heavy stages (Sector Z
1,370,944, Jungle 1,406,016, Saffron 1,365,504 on 10-04/05): per-stage gaps
quoted from it were too pessimistic. Dream Land here is +15K over the official
gate ROM (layout: the gate ROM compiles one stage).

Excess over Dream Land in the P92-98 band (K ticks): SRC (simulation) +69 to
+122 on every stage but Hyrule; the draw (MISC) +75 to +87 on Sector Z, Jungle
and Yoshi's Island.

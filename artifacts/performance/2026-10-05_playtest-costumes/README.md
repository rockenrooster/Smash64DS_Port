# 2026-10-05 Alternate costumes: hatted fighters left the lean path

Owner (BUGS.md, Pikachu): "Alt costumes cause P95 slowdown (4 pikachu match
is 15 FPS, Check All figher alt costumes)".

Clean four-CPU lab ROM, Dream Land, items on. `gNdsLabFourCpuKinds` puts one
kind in every slot; a repeated kind takes the character select's next costume
(`gNdsLabFourCpuCostumes`, `src/port/nds_match_config.c`), so four Pikachus wear
costumes 0-3 as they do from the shell.

| Run | Roster | P50 | P95 | over 1.12M | two-VBlank presents |
|---|---|---|---|---|---|
| `base` | gate roster | 808,384 | 1,115,264 | 92 | 1,851 |
| `pk4` | 4 x Pikachu, costumes 0-3 | 1,718,784 | 2,089,536 | 1,777 | 181 (4-VBlank 1,144) |
| `pk4c0` | 4 x Pikachu, all costume 0 | 826,880 | 1,170,368 | 142 | 1,794 |

Costumes 1-3 give Pikachu a hat: the source's accessory part, a second
display list drawn under the head joint's matrix (ftmanager.c:789,
ftdisplaymain.c:819). The display contract binds it to the joint whose parts
own it, so two roots name one matrix DObj, and `ndsFtrLeanBuildJoints` refused
a joint bound twice. Diagnostic-counter run `dg-pk4` (lab ROM with
`NDS_DIAG_COUNTERS=1`): 6,778 lean attempts, 1,671 lean draws, 5,107 declines
with reason 12 (Topology) and 5,108 First events -- the three hatted Pikachus
drew through the old path every frame.

Fix (`src/port/renderer_fighter_lean.c`): the joint table accepts one shared
joint as an alias root (`alias_dst` / `alias_src`, in the instance's padding);
after the kernel compose the alias root takes its source root's LOAD4x3 words,
and its world where the patch reads one.

| Run | Roster | P50 | P95 | over 1.12M | two-VBlank presents |
|---|---|---|---|---|---|
| `fix-pk4` | 4 x Pikachu, costumes 0-3 | 827,648 | 1,166,464 | 143 | 1,790 |
| `fix-pu4` | 4 x Jigglypuff, costumes 0-3 (bow / hats) | 771,776 | 1,047,040 | 39 | 1,904 |
| `fix-base` | gate roster | 808,960 | 1,114,176 | 90 | 1,852 |

Pikachu and Jigglypuff are the only fighters with an accessory part
(`attr->accesspart`); the other ten alternate costumes are material changes
only. Official gate with this and the day's other fixes: replay digest
identical, `../2026-10-05_playtest-gate`.

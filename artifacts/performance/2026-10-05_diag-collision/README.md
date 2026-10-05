# Collision development tallies compiled out of ship and gate (NDS_DIAG), 2026-10-05

`src/port/reloc_backend_mp_collision.c` carried 514 unwrapped `gNds*`
increments. 271 statement increments of 177 counters that nothing under
`src/` or `include/` reads (only gdb probes) now go through `NDS_DIAG(...)`,
which compiles them out where `NDS_DIAG_COUNTERS` is 0 (the published ROM
and the four-CPU gate). Counters any code reads (snapshots, conditions,
proof publishers: 154 names) are untouched. Script: a write-only scan
(statement-level `NAME++` / `NAME += x`; a name qualifies when every other
occurrence in the tree is a declaration, a plain store or another increment).

Replay digest identical everywhere.

Official gate (`build-gate-1004x` against `gate-s1-itcm6`): P50/P95
831,744/1,154,496 -> 829,824/1,152,256, over 123 -> 126, paired median -1,664,
mean -1,775.

Lab sweep ROM (`build-lab-sweepall4` -> `-sweepall5`), paired medians:
Castle -5,312, Sector Z -128, Jungle -3,648, Zebes -4,352, Hyrule -2,240,
Yoshi's Island -2,624, Dream Land -704, Saffron -2,496, Mushroom Kingdom
-4,160 (`a5-g*.csv`).

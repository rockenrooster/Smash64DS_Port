# 2026-10-05 The packet recorder and the lean oracle deleted (R1, R2)

Owner, 2026-10-05: "Delete Old machinery." With the old fighter executor
compiled out of every lean build (`2026-10-05_lean-texgen`, D1), its packet
recorder and replay had no caller in any configuration: a build with the
packet path compiles the lean path too, and a build without it never
compiled the recorder. Clean four-CPU lab (`NDS_LAB_FOURCPU_WORDS=1`), items
on; WORK-H from the ring dump, frames >= 64 (`runsum.py`).

## R1 (`build-lab-clean1005r1`, base D1): the recorder

Deleted from `src/nds/`: the recorder (`ndsFighterPacketCmd*`,
`ndsFighterPacketRecord*`, `BeginRoot`, `Finish/AbortRecord`, the record
twins of the production emitters and matrix loaders), the replay
(`ndsFighterPacketTryReplay`, `ndsRendererFighterPacketPrecheck`,
`BuildKey`, `Matches`, `ApplyTint`), every `NDS_FIGHTER_PACKET_HOOK` site,
the texgen overflow store, the recorder's counters, and the 18,048-byte
`sNdsFighterPackets` array -- the heap now starts 18,176 bytes lower. The
lab oracle comparator (`ndsFtrLeanOracleSemantic` and its decoder) went with
it. ITCM 31,504 -> 31,032 bytes (the hook sites).

| Pair | P50 | P95 | over | digest | paired median |
|---|---|---|---|---|---|
| gate `gate-d1` -> `gate-r1` | 815,616 -> 816,640 | 1,105,344 -> 1,109,248 | 88 -> 89 | identical | +1.5K |
| Sector Z `sz-d1` -> `sz-r1` | 901,184 -> 903,104 | 1,236,672 -> 1,225,536 | 201 -> 203 | identical | +4.0K |
| 4 x Yoshi `yo-d1` -> `yo-r1` | 892,288 -> 893,568 | 1,265,280 -> 1,264,448 | 242 -> 244 | identical | +1.7K |

No executed instruction changed (the deleted code was unreachable); the
paired drift is layout: every BSS address after the array moved 18 KB, and
the 472 ITCM bytes the hooks held are not yet re-admitted.

## R2 (`build-lab-clean1005r2`, base R1): the oracle routes

Routes 2/3 (oracle-exact, oracle-shipped), their forced source compose, the
wide-list lab buffer, the shadow arm, the topology hash and the oracle
counters are gone from `renderer_fighter_lean.c`, `renderer_adapter_matrix.c`,
`nds_renderer_native_common.c` and `renderer_fighter_lean.h`; the lean run
takes no route argument.

| Pair | P50 | P95 | over | digest | paired median |
|---|---|---|---|---|---|
| gate `gate-r1` -> `gate-r2` | 816,640 -> 816,448 | 1,109,248 -> 1,108,928 | 89 -> 89 | identical | -0.6K |
| Sector Z `sz-r1` -> `sz-r2` | 903,104 -> 902,336 | 1,225,536 -> 1,228,032 | 203 -> 202 | identical | -1.0K |

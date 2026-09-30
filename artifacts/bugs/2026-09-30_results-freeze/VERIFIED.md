# Results freeze — built repair verified (2026-09-30)

Owner report (r55 playtest): Mario/Kirby/Fox/Yoshi VS match froze before Results.
Investigation and proposed diff: [REPORT.md](REPORT.md) (Codex investigator,
read-only). This file records the integrated, built repair.

## Repair

`src/import/battleship_sys_taskman.c` `syTaskmanStartTask`: VS Results
(`nSCKindVSResults`) joins the scenes that take the DS buffer budget
(`ndsBattleRebudgetSceneSetup`: graphics arena 0x600, RDP output 0x10), and
keeps its source DL0/DL1 capacities. Two graphics heaps and the RDP output
buffer return 2 x (32,768 - 1,536) + (49,152 - 16) = **111,600 B** to the
Results general heap. No content, pool, file or allocator changed.

Cause (REPORT.md): the source defers the Results fighter lineup to tic 120; with
the N64 reservations in place the four-kind lineup needs ~1,002,360 B of a
956,160 B heap, and the checked allocator halts making the second fighter's
MObj (168 B requested, 56 B left). Pre-existing: the eligibility rule has
omitted Results since `48c31ffb3b1` (2026-09-05).

## Built verification (no runtime pokes)

ROM `builds/p2p8-playtest-r57/smash64ds.nds`, SHA-256
`EEA312301CB9992001E485216DE93758ED19DF90569B3D9538FF267431DEE443`
(HEAD `1e086261f37` + this repair + the libnds texture-identity reset).
Harness `builds/codex-results-freeze/run.ps1 -Arm r55 -Tag r57-built -RomPath
... -ElfPath ... -SeedAtSss -ReturnCss -CssWaitTicks 60` (no `-RebudgetResults`
/ `-RetainResultDl`): Mario human, Kirby/Fox/Yoshi CPU3, one-minute Castle,
natural Time Up into Results.

| Point | r55 (REPORT.md) | r57 built |
| --- | --- | --- |
| Results tic 120 lineup | halts in `ndsSyMallocOverflowHalt`, 56 B left | all four fighters made |
| Results general heap free (frame 2278) | - | **65,400 B** |
| Allocator overflow / native failure count | halt / - | 0 / 0 |
| START at tic 415 | - | returns to CSS, four previews loaded |

Captures: [r57-built.png](r57-built.png) (Yoshi victory, Mario/Kirby/Fox,
confetti), [r57-built-return-css.png](r57-built-return-css.png); log
[r57-built.txt](r57-built.txt).

Owner playtest of r57 (2026-09-30): two matches (Dream Land, Sector Z;
Kirby/Fox/Yoshi/Pikachu) — Results works and returns. Owner note: Results
frame rate "should be higher" (open, separate from this repair).

## Not covered here

Stock/Team/No Contest/Sudden Death Results entries and other rosters were not
driven by the harness (the owner's two matches exercised the rematch path).
`gNdsTaskmanDLOverflowCount` already reads 28,602 (kind 1, 208 B) at Results
entry: that is a battle-side DL buffer 1 overrun, unchanged by this repair and
tracked separately.

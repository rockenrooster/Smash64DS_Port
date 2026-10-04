# Yoshi's ending path (2026-10-04)

The all-fighter campaign sweep's level-9 walk CPU never beat Master Hand as Yoshi (six
fights, 0 native failures). `yoshiend.ps1` (session scratchpad) starts the walk ROM
(`builds/walk-all7`) at stage 13 with Yoshi's portrait and, 300 presented frames after GO,
sets Master Hand's `percent_damage` to 299 (his defeat is `percent_damage >= 300`,
ftbosscommon.c:221), so the next hit defeats him.

Scenes: 1PGame 52 -> StageClear 51 -> Ending 48 -> Staffroll 56, 0 native failures.
`sc48-003.png` is the Ending room with Yoshi's figure on the desk; `sc56-006.png` a
Staffroll frame.

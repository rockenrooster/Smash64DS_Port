# FD silent in VS mode — map bgm_id is the Master Hand entrance sting

**Date:** 2026-06-09 (issue #228)
**Status:** FIXED — `decomp@caa2064b5` (`scvsbattle.c` PORT fixup)
**Platforms:** all

## Symptom

VS mode on Final Destination (a port addition — the N64 game never offered
FD outside the 1P final battle): the Master Hand entrance music plays once
at battle start, then the match is silent.

## Root cause

`mpCollisionSetPlayBGM()` plays whatever `bgm_id` the stage's GroundData
carries. FD's map file (`relocData/266_GRLastMap.c:46`) carries
`nSYAudioBGMBossEntry` — the one-shot "Master Hand appears" sting — because
the **1P final battle depends on it**: sc1pgame plays the sting during the
entrance cutscene and only switches `gMPCollisionBGMDefault` to the real FD
theme (`nSYAudioBGMLast`) after the entrance completes (`sc1pgame.c:1616`).
VS has no entrance sequence, so the sting played once and nothing followed.

## Fix

The data file can't change (1P depends on it). `scVSBattlePortFixupFDMusic`
(PORT-only, `scvsbattle.c`) runs right after both `mpCollisionSetPlayBGM()`
call sites (normal battle start + sudden death): when
`gSCManagerSceneData.gkind == nGRKindLast`, it re-aims
`gMPCollisionBGM{Default,Current}` at `nSYAudioBGMLast` and starts it.
Metal Cavern and Battlefield carry looping themes in their map files and
need no remap.

## Audit hook

Any future port-added stage whose map file was authored for a 1P-scripted
music flow needs the same check: does VS's generic
`mpCollisionSetPlayBGM()` path produce a looping theme for it?

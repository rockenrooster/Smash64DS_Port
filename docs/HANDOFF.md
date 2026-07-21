# Handoff

Updated: 2026-07-20 (Task 36 Phase-B conservation stop)

`P1_EXECUTION_BOARD.md` owns current state. This file contains only the restart
surface and next packet.

## Restart

Branch: `codex/task36-hw-compose`
Boundary: `battle_playable_realtime`, mode `163`

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Preserve canonical mode 163, intrinsic renderer mode 9, mip 0, static texture
residency, source countdown, exact Dream Land water frame 0, and Task 16
compare/i2f/addsub `1/1/1`. Do not edit `decomp/`.

## Next Packet

Task 36 is closed at its Phase-B gate. Phase A commit `c08e8ee` keeps 26 rigid
bindings on hardware composition, binding 29 on the exact CPU path, all stage
surfaces visible, and approximately 40K stage ticks saved. Tyler approved the
screenshots.

The standalone Task-34 census runs Task29/34/36/affine `0/1/1/0`. Its full-arena
gate is `0x150000/0`; a degraded arm is rejected before capture. Valid countdown
and early captures contain 2,762 entries / 6,664 words per frame. The 26 rigid
DObjs own at most 3,929 words (58.959% of the stage stream), and only 3,896 are
constant across both windows (58.463%). This is below the mandatory 60% gate;
Whispy cannot raise an intersection and was not rerun after its fixed timeout.
Do not implement the Phase-B bake/replay, DMA, or device pair. Evidence:
`artifacts/performance/2026-07-20_task36-phaseb-conservation.md`.

No next implementation task is claimed in this handoff. Select the highest
unowned red P1 row from `P1_EXECUTION_BOARD.md`.

Preserve the unrelated untracked optimization task documents and user files;
they are not part of the Task 36 implementation commit.

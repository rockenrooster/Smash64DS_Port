# Handoff

Updated: 2026-07-20 (Task 36 Phase-B replay keep and profile-0 promotion)

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

Task 36 is implementation-complete. Tyler explicitly accepted the measured
58.463% conservation on 2026-07-20. Mode 2 now captures and CPU-replays the 3,916
exact GX words owned by rigid segments 0/5/7 while dynamic stage segments remain
live. The lab gate proves READY, one bake, 3 segments, 33 runs, 26 rigid bindings,
full arena `0x150000/0`, and zero fallback/faults through countdown, early combat,
and Whispy.

Against the Phase-A control at frames 438..445, stage P50/P95 falls
430,368/430,528 -> 284,320/284,544 (`-146,048/-145,984`) and draw falls
832,736/835,776 -> 704,672/707,712 (`-128,064/-128,064`). The replay image is
0/49,152 changed pixels versus Phase A, with the floor and all three platforms
visible. The profile-0 one-minute Time Up/Results soak and Boundary pass. The
published ROM is `C1B3DDE3...` with Task36/generated-M3/affine/Task32
`2/1/1/1`. Evidence:
`artifacts/performance/2026-07-20_task36-phaseb-conservation.md`.

Retail engagement/performance remains queued for the next device checkpoint;
do not claim a hardware-speed number from the melonDS result.

No next implementation task is claimed in this handoff. Select the highest
unowned red P1 row from `P1_EXECUTION_BOARD.md`.

Preserve the unrelated untracked optimization task documents and user files;
they are not part of the Task 36 implementation commit.

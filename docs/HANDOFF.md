# Handoff

Updated: 2026-07-20 (Task 36 Phase-A2 visual approval gate)

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

Task 36 Phase A2 is fixed and verified behind `NDS_TASK36_HW_COMPOSE=0` by
default. Binding 29 stays on the exact CPU path; 26 rigid bindings use hardware
composition. Three synchronized windows save approximately 40K stage P50/P95
ticks. The whole-stage, early, and Whispy screenshot pairs each change
355/49,152 pixels (0.722%, mean 0.28) with the main floor and all three platforms
visible.

Focused fixtures, DevFast, Boundary, and three complete one-minute Results
lifecycles pass. The retired Regression fleet must not be restored. The direct
lab target's strict start-state assertion samples at Results and is not claimed;
runtime lifecycle and exact 2:1 cadence are green.

Tyler must explicitly approve the screenshots before Phase B begins. After
approval: commit Phase A as KEEP, run Task 36 Phase B conservation census, and
continue only if at least 60% of rigid words are conserved. Evidence and ROM
hashes:

`artifacts/performance/2026-07-20_task36-phasea-hw-compose-wip.md`

Preserve the unrelated untracked optimization task documents and user files;
they are not part of the Task 36 implementation commit.

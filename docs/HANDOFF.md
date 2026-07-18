# Handoff

Updated: 2026-07-17 20:21 Central
`P1_EXECUTION_BOARD.md` owns all current state. This is only the restart surface.

## Restart
Branch: `master`

Boundary: `battle_playable_realtime`, mode `163`.

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

The integrated fixed-two battle ROM is 14,655,488 bytes, SHA-256
`792F1A76E94FC245820F936A26442ACA8C50C683FE44524D39E2B5B57265F05C`.
Preserve intrinsic mode 9, mip 0, static residency, source countdown, and exact
Dream Land water frame 0/fraction 114 on the original 12 triangles.

## Retained Gates

- Mode 163 uses exact fixed-two pacing with no debt or catch-up. Public/manual
  runs keep the level-3 Fox CPU and source Wait/countdown/timer path enabled.
- The natural one-minute proof completes 4,084 updates / 2,042 presents, reaches
  Results, retains 166,672 bytes after BGM, and performs one clean M4 teardown.
- Countdown, common effects, BGM/FGM, KO/rebirth, M4 `22/131072`, Task 9 state
  identity, 28,020-byte ITCM placement, and the two-ROM contract pass.
- M3 remains 489,184/489,536 ticks. M2's source-light-exact checkpoint remains
  385,088/388,224 with exact 686-triangle generic/fast parity. Current profile-0
  smoke is 20.2 FPS, so full-speed locked 30 remains red.

## Checkpoint

The Down+A playtest finding is fixed at the shared source seam. BattleShip
ClearAll preserves reusable collider payload, so the port shim now changes only
`attack_state` and `is_attack_active`; the existing damage-common probe also
requires its seeded damage 7 to survive. No transform guard, attack-specific
branch, payload rebuild, or new harness was added.

Fox as verifier-only human P2 completes all nine Down-Air callbacks and exits
naturally with logic/cpu/reads 8/0/12, 116,992 update ticks, and 205,744-byte
reserve. Mario passes with the imported level-3 Fox CPU active at 8/8/12,
134,784 ticks, and 203,536-byte reserve. The 20:19 Current run passes in 661.5
seconds, including canonical mode 163 CPU setup/proc/target 1/33/33, runtime,
registry, GBI, two-ROM publication, 28,020-byte ITCM placement, and dated visual
analysis. The battle ROM is `792F1A76...`; manual exact-ROM retest remains.

## Next Packet

Return to the measured M2 fighter emit path at the source-light-exact
385,088/388,224-tick checkpoint. Select one production cut from the retained
contract in `optimization/NATIVE_RENDERER_PLAN.md`, use one synchronized
eight-frame A/B, and stop on a decisive KEEP or REVERT. Do not reopen the
rejected wallpaper, M3 packet/order, full-inlining, or shared-tail experiments.

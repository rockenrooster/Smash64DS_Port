# Handoff

Updated: 2026-07-17 21:25 Central
`P1_EXECUTION_BOARD.md` owns all current state. This is only the restart surface.

## Restart
Branch: `master`

Boundary: `battle_playable_realtime`, mode `163`.

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

The integrated fixed-two battle ROM is 14,655,488 bytes, SHA-256
`DA8282BBBD9872DC29F7442CC6ED3E0029967A7AB1AA0E94F9EDBED172981F04`.
Preserve intrinsic mode 9, mip 0, static residency, source countdown, and exact
Dream Land water frame 0/fraction 114 on the original 12 triangles.

## Retained Gates

- Mode 163 uses exact fixed-two pacing with no debt or catch-up. Public/manual
  runs keep the level-3 Fox CPU and source Wait/countdown/timer path enabled.
- The natural one-minute proof completes 4,084 updates / 2,042 presents, reaches
  Results, retains 166,672 bytes after BGM, and performs one clean M4 teardown.
- Countdown, focused effects, BGM/FGM, KO/rebirth, M4 `22/131072`, Task 9 state
  identity, 28,052-byte ITCM placement, and the two-ROM contract pass.
- M3 remains 489,184/489,536 ticks. M2's source-light-exact checkpoint remains
  385,312/388,480 with exact 686-triangle generic/fast parity. Latest profile-0
  smoke is 22.3 FPS, so full-speed locked 30 remains red.

## Checkpoint

The production raw emitter now has separate textured and untextured callees.
The existing corpus proves 43/11 raw calls per frame; the common untextured path
removes 172 main-RAM stack word transfers. Synchronized frames 600..607 improve
combined fighter 386,624/389,824 -> 385,312/388,480 and draw
1,011,648/1,014,976 -> 1,009,824/1,013,120 with 0/49,152 changed pixels. Profile
2 remains exact on frames 180..187. Down+A remains source-fixed and gated.
The exact `DA8282BB...` ROM passes full `Latest -NoBuild` in 201.2 seconds.
Disconnected desktop capture now falls back to native `PrintWindow`; the same
visibility, region, motion, and detail gates pass instead of accepting blindly.

## Next Packet

Qualify Tyler's current-ROM report that some attack/hit sounds and visuals do
not play. Start from the exact BattleShip attack/collision/effect event and ID,
then trace the DS mapping, AOT asset, mixer/channel or renderer/effect manager,
and natural runtime output. Pack membership is not playback proof. Reuse the
existing natural attack/hit gates; do not add a one-bit harness or substitute a
generic sound/effect. Update the two reopened acceptance rows from audible and
visible exact-ROM evidence only.

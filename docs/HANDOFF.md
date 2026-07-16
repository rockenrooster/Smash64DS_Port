# Handoff

Updated: 2026-07-15 18:26 Central

This is the exact restart surface. `P1_EXECUTION_BOARD.md` owns the queue,
`STATUS.md` owns current truth, and `PORTING.md` is append-only history.

## Current Checkpoint

Branch: `codex/wip-natural-combat-source-start-collision`

Active Boundary: `battle_playable_realtime`, mode 163.

Preserve the live dirty tree. It contains the coherent M3 complete-stage owner,
M4 frozen-water/static-residency integration, profile-1 M3 timing hooks, updated
published defaults, verifier fixes, generated static assets, and documentation.
Do not restore the retired animated/tiled-water path.

The canonical published target is intrinsic mode 9 / mip 0 / static residency 1 /
hybrid OAM 1. The original launch target remains 0 / 0 / 0 / 0.

```text
smash64ds-battle-playable-hwtri.nds
14,534,656 bytes
SHA-256 3F3AC2E1A20F7D93B0E92419BA642FD5D97A275454ABEC0D1C96EF7742E6BB38
```

## Verified Evidence

Canonical Boundary passed on runner slot 2 on 2026-07-15. The natural terminal
frame reported 19.8 FPS, 828 total triangles, exact M3 ownership
`121 runs / 202 stage / 320 Mario / 306 Fox`, zero fast fallbacks, no texture
uploads, and M4 `22 keys / 131072 bytes / zero fence violations` with frozen
water `2/0/1`.

Exact completed GO frames 438/439 passed source timer/control/OAM state, full
visibility, named-region/detail, motion, and pond gates:

```text
artifacts/visibility/2026-07-15_canonical_fast_frame438-439_182430-9052820-p35520.png
  153,147 bytes; SHA-256 45DBCD24D2DAC91089A1AAD6AB430C05CB173BB4E3FCFFBACEBE9A323B040922
artifacts/visibility/2026-07-15_canonical_fast_frame438-439_182430-9052820-p35520_next.png
  153,186 bytes; SHA-256 2E12523F0C0EE55A71F2C6836B89F2BC336EC16EF9EF0DB41D402D34ED42670F
```

`artifacts/visibility/latest.png` is byte-identical to frame 438. The capture is
a complete recognizable Dream Land GO scene with Mario, Fox, the frozen pond,
and lower HUD intact; it has no blank or partial-frame corruption.

## Milestone Truth

- M1 is accepted: retained affine BG2 costs 1,856 ticks, below 35K.
- M2 renders correctly in Mode 8 but remains over target at
  477,152/477,376 ticks. Mode 7 is rejected. The 170–250K target remains open.
- M3's complete-stage Mode-9 owner is now linked and device-semantic-proven:
  8 callbacks, mask 255, 57 DObjs, 42 bindings, 54 runs, 202 triangles, 49
  epochs, four material commits, cross `5/10/15`, and zero fallback. Frames
  438–445 measure stage-exclusive 664,544/664,640 P50/P95, missing the <=500K
  first gate by 164,544 and saving only about 140K versus the ~805K baseline.
  M3 is REWORK; no 150–250K completion claim.
- M4's generated corpus has 22 complete keys, 21 deduplicated outputs, a
  126,976-byte payload, and 131,072 prepared bytes in VRAM A. The published
  short Boundary window proves pinning and zero gameplay conversion, upload,
  I/O, allocation, refresh, eviction, fallback, or fence violation. The full
  one-minute GO-to-teardown fence/reserve qualification remains pending.
  Its isolated hardware target now builds, but the first full invocation exited
  nonzero without a terminal acceptance marker; do not count it as M4 evidence.

Compilation alone does not close M2–M4.

## DS Visual Decision Rule

Gameplay, hitboxes, collision, physics, timing, rules, camera meaning, and state
flow remain source-faithful. Presentation targets roughly 90% overall likeness.
Give cosmetic pixel exactness one measured focused experiment; if it misses the
DS tick/memory budget or threatens P1, keep the cheapest recognizable
source-derived approximation, document its delta/reason, capture it under
`artifacts/visibility`, and move on.

Dream Land water is frozen at exact BattleShip frame 0, non-FRAC fraction 114,
on original runs 42–43 and the original 12 triangles. Later water material
animation is intentionally ignored. The retired 167,936-byte/138-triangle
animated replacement is history, not an active option.

## Resume Order

1. Enumerate Boundary and inspect status; preserve the dirty integration.
2. Run the roadmap dense-prepare cut, touching only `nds_renderer.c` and
   `check_nds_native_stage.py`. Expected saving is 170–210K; require at least
   164,544 saved and <=500K. Do not reopen water animation or broad cosmetic parity.
3. Run the newly routed one-minute M4 GO-to-teardown fence/reserve gate before calling M4
   complete. Fox CPU remains default-paused until Tyler requests re-enable.
4. Refresh the two root ROMs only after accepted source changes. Finish focused
   checks, Boundary, docs, status, and commit before the Lean snapshot.

## Exact Commands

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\benchmark-renderer-fast-raw.ps1 -FastRunMode 9 `
  -StaticTextureAotMode 1 -IFCommonHybridOamMode 1 `
  -RendererProfileLevel 1 -RendererM2DetailedLedger `
  -RendererBenchmarkSamples 8 -RendererBenchmarkStartFrame 438 -RunnerSlot 2
.\scripts\verify-battle-playable-one-minute-match.ps1 -RunnerSlot 3
.\scripts\check-battle-playable-static-textures.ps1
.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\verify-boundary.ps1 -NoBuild -DelaySeconds 3 -RunnerSlot 2
```

Use only repo-local scripted melonDS. Screenshots belong only under
`artifacts/visibility`. Never run or prebuild `Full`, `Regression*`, or `P1Gate`.
The Lean snapshot is always the final project command, with no command after it.

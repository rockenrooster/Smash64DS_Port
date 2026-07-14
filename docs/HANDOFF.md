# Handoff

Use this file for the active handoff only. Keep it under 150 lines. The harness
registry decides Boundary/Latest membership; `docs/PORTING.md` owns history.

## Start Here

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

Boundary and BoundaryDirect now select `battle_playable_realtime` (mode 163),
the canonical five-minute Mario-human/Fox-CPU scene. Modes 161/162 remain
registered for diagnosis but are no longer active Boundary/Latest entries:
their bounded input driver assumes pre-GO movement, which conflicts with the
restored BattleShip control lock. Do not add an unlock seed to revive them.

## Cut G Result

Cut G milestone 1 is accepted and graduated to the canonical/shipped profile-0
ROM. One complete source-rendered Dream Land wallpaper is retained in the
256x192 BG2 layer. Each live frame applies `grWallpaperCalcPersp` state through
native BG2 affine registers while the original display graph continues to draw
the stage, both fighters, effects, and foreground/interface SObjs.

The focused gate requires exactly one seed/capture, zero texture uploads,
failures, or fallbacks for the retained wallpaper, 49,152 initial BG2 pixels,
live BG3 traffic, one affine queue per live frame, one extra seed apply, no
coverage failure, a nonidentity transform, and at most 35,000 affine-update
ticks. Live frames retain both fighters and the exact 626-triangle fighter
contract.

Exact BattleShip Sprite manifests plus layered-SObj 4c/CI4/I8, TLUT, and
TEXSHUF decoding restore the timer, countdown traffic light, and GO art. The
custom percent/stock callbacks still route through unimplemented
`lbCommonPrepSObjAttr`/`lbCommonDrawSObjAttr`; their pixels are not yet visible.
The user permits those HUD elements on the lower LCD.

Exact `ftParamLockPlayerControl`/`ftParamUnlockPlayerControl` behavior now keeps
both fighters and Fox CPU inactive during Wait while the timer remains 18,000,
then unlocks controls and starts the timer at GO. The same canonical ROM passed
coherent pre-GO and post-GO windows.

Canonical/shipped ROM:

```text
smash64ds-battle-playable-hwtri.nds
12,038,144 bytes
SHA-256 4132FBB6A618AE16A3E7554A2C4928669152278DD0F84138329B4058FDF93557
```

Fresh canonical capture `artifacts/visibility/latest.png` passes full top-screen
coverage, green/detail, paired motion, named-region, horizontal-detail, and sky
gates. This is melonDS/verifier acceptance, not physical-hardware acceptance.

## Performance And Remaining Milestones

- Milestone 1: complete. Native BG2 affine update is within the 5–35K target.
- Milestone 2: open. AOT DS-native Mario/Fox renderer target is 170–250K ticks.
- Milestone 3: open. AOT DS-native complete-stage target is 150–250K ticks.
- Milestone 4: open. Sampled gameplay still converts textures and uploads
  `2 / 36,864` bytes; conversion must move entirely before gameplay.

Whole-frame presentation remains roughly 9.6–11.4 FPS. The accepted slice is a
fidelity/ownership win, not a 60 FPS claim.

## Recommended Next Work

1. Implement the AOT DS-native Mario/Fox owner and gate 170–250K ticks.
2. Implement the AOT complete-stage owner and gate 150–250K ticks.
3. Move all texture conversion/upload preparation before gameplay.
4. Implement the source custom-SObj callbacks and route percent/stock HUD to
   the lower LCD if that is the least invasive DS layout.

Keep comparisons on identical ROM hashes and synchronized windows. Require
counters, screenshots, semantic traces, and runtime state to agree.

## Focused Cut G Commands

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make TARGET=smash64ds-battle-playable-coarse-mipcache-hwtri `
  BUILD=build-battle-playable-coarse-mipcache-hwtri-harness -j16

.\scripts\verify-battle-playable-harness.ps1 -NoBuild -DelaySeconds 45 `
  -RealtimePresentation -ImportBattleShipFTComputer
```

The profile-1 Cut G target remains useful for counters; the canonical profile-0
target is the only source for the shipped filename. Never copy the lab ROM over
the shipped ROM manually.

## Verification State

GBI fixtures, docs, architecture, registry, canonical DevFast build/capture/
parity, focused profile-1 pre/post-GO, and canonical profile-0 pre/post-GO pass.
Boundary/Latest were migrated because exact source locks invalidate their old
pre-GO motion assumption. Full Regression remains follow-up.

After successful verified progress, run
`.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` as the final project action.

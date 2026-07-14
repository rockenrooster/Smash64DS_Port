# Current Status

This is the short current-truth document. Use
`docs/DIAGNOSTIC_REFERENCE.md` for marker definitions and append history to
`docs/PORTING.md`.

## Direction

The target remains a 1:1 playable Nintendo DS source port of BattleShip Smash
64. Keep `decomp/` read-only, import coherent original TU groups, and graduate
only source-backed behavior proven in continuous runtime and captures.

## Current Boundary

The registry is authoritative:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

Boundary/BoundaryDirect now select canonical `battle_playable_realtime`, mode
163. Latest keeps runtime, Title, and that same natural five-minute
Mario-human/Fox-level-3 scene. Exact BattleShip start locks made the former
161/162 bounded pre-GO input driver invalid; those modes remain diagnostic-only
and their selected live-hit coverage reduction is recorded in KNOWN_ISSUES.

Mode 163 imports the original fighter manager/main/CPU, animation/key/status
runtime, collision, camera, death/rebirth, IFCommon, normal moves, weapons,
effects, audio, and Results chain. Other mode-163 configurations still cover
stock/KO/rebirth and the one-minute Time Up -> VS Results lifecycle.

## Accepted Cut G Milestone

Milestone 1 is complete. Cut G retains exactly one full Dream Land wallpaper
seed in native 256x192 BG2 and drives it with live `grWallpaperCalcPersp`
transforms through DS affine registers. Stage geometry, animated foreground,
fighters, effects, and interface traversal remain live rather than flattened.

The profile-1 proof requires one seed/capture, no retained-wallpaper upload,
failure, or fallback, 49,152 BG2 pixels, positive BG3 traffic, exact affine
frame conservation, zero coverage failures, nonidentity motion, and an affine
update no greater than 35,000 ticks. Live frames preserve both fighters and the
626-triangle fighter contract.

BattleShip Sprite manifests and general 4c/CI4/I8 layered-SObj decode restore
the timer, countdown traffic light, and GO art. Percent/stock semantic state is
coherent, but its custom SObj callbacks still emit no pixels; lower-LCD routing
is presentation debt and is allowed by the user.

BattleShip's exact player-control gate is restored. A synchronized pre-GO
sample proves Wait, 18,000 remaining, timer stopped, both fighters locked, and
zero Fox CPU processing. A post-GO sample proves Go, remaining + passed =
18,000, timer running, both unlocked, and natural CPU activity.

The canonical target enables the retained path at profile 0 and publishes the
user-facing ROM only through the Makefile parity rule:

```text
smash64ds-battle-playable-hwtri.nds
12,038,144 bytes
SHA-256 4132FBB6A618AE16A3E7554A2C4928669152278DD0F84138329B4058FDF93557
```

`artifacts/visibility/latest.png` is a successful canonical screenshot. Both
sampled frames pass top-screen visibility, green/detail, named regions,
horizontal stage detail, motion, and sky coverage. Acceptance is melonDS-only;
physical hardware remains untested.

## Performance And Open Work

Whole-frame presentation remains about 9.6–11.4 FPS. Sampled profile-1 gameplay
still reports positive texture conversion and two uploads totaling 36,864
bytes. Therefore:

- Milestone 2, AOT DS-native Mario/Fox at 170–250K ticks, is open.
- Milestone 3, AOT DS-native complete stage at 150–250K ticks, is open.
- Milestone 4, zero texture conversion during gameplay, is open.
- Percent/stock custom-SObj rendering and optional lower-LCD layout are open.
- Whispy face, weapon detail, platform crossing, and some fighter lighting/
  facing remain presentation debt.

Rejected Cut D/F, typed-stage, mode-9, and scanline/HBlank experiments remain
closed; see PORTING and PERF_LEDGER for their measurements. Do not revive them
without a distinct source-backed architecture and an exclusive cost model.

## Verification

Passing checks for this checkpoint:

```powershell
.\scripts\check-docs.ps1
.\scripts\check-architecture.ps1
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

Focused profile-1 and canonical profile-0 pre/post-GO runs also pass. Full
Regression remains follow-up. Run the Lean snapshot only after all final checks
and status inspection; run no project command after it.

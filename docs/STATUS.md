# Current Status

This is the short current-truth document. `P1_EXECUTION_BOARD.md` is the only
dynamic queue. Use `DIAGNOSTIC_REFERENCE.md` for marker definitions and append
history to `PORTING.md`.

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
163. Latest keeps runtime, Title, and that same natural one-minute
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
the countdown traffic light and GO art on the top screen. The user-approved
lower text HUD shows FPS, timer, Mario/Fox labels, stock, and damage, updates on
state changes, and clears at VS Results.

The live countdown SObjs now use setup-converted main bitmap OAM instead of
full-layer software composition. Integrated frames 187–194 measured
11,584/11,584 native median/P95 versus 1,863,232 foreground ticks, with zero
gameplay conversion/upload, complete captures, final clear at frame 511, and no
frame-600 idle tax.

BattleShip's exact player-control gate is restored. A synchronized pre-GO
sample proves Wait, 3,600 remaining, timer stopped, both fighters locked, and
zero Fox CPU processing. A post-GO sample proves Go, remaining + passed =
3,600, timer running, both unlocked, and natural CPU activity.

The canonical target enables the retained path at profile 0 and publishes the
user-facing ROM only through the Makefile parity rule:

```text
smash64ds-battle-playable-hwtri.nds
12,043,264 bytes
SHA-256 385B9F051C5CBB801089C69E13D49F9E0D19C07F1E4DA19DA943772B5553FC21
```

`artifacts/visibility/latest.png` is a successful canonical screenshot. Both
sampled frames pass top-screen visibility, green/detail, named regions,
horizontal stage detail, motion, and sky coverage. Acceptance is melonDS-only;
physical hardware remains untested.

## P1 Release Matrix

| Area | Current state |
|---|---|
| Natural one-minute battle and Results | Natural 3,600→0/Time Up/22→24 gate passes; exact canonical-duration qualification remains |
| Gameplay | Core live; source Fireball render/damage passes; natural recovery coverage open |
| Renderer | M1 and native countdown pass; M2 active; M3/M4 open |
| HUD/countdown | User-approved lower HUD and top countdown pass |
| Audio | Five phase FGMs and recyclable handles pass; winner/Results streams exist; voices and natural transition proof remain |
| Stability/memory | One full match passes with 171,916-byte conservative reserve and zero safety faults; repetition pending |
| Release evidence | Full Regression, dated capture, and exact-ROM retest pending |

Detailed owners, gates, blockers, and evidence live only on the execution board.

## Performance And Open Work

Whole-frame presentation is about 10.3 FPS in the latest synchronized
`laboratory-profile-1` M2 window, not a canonical phase baseline. Sampled lab
gameplay
still reports positive texture conversion and two uploads totaling 36,864
bytes. Therefore:

- Milestone 2 is in progress: Mario/Fox still cost about 431K combined versus
  170–250K. Ranked next cuts are a GX hierarchy/3x3 lighting sidecar and a
  compiled epoch/run transaction.
- Milestone 3 is open: the ~801K stage owner needs whole-stage preflight and a
  fused static-slab owner before the 150–250K target is credible.
- Milestone 4 keeps an exact 322-key/206-output host corpus. A direct runtime
  NitroFS reader was measured and rejected; zero-I/O preload remains open.
- Whispy face, weapon detail, platform crossing, and some fighter lighting/
  facing remain presentation debt.

Same-ROM split projection/modelview A/B/A saved only 36,192 fighter ticks and
was rejected and removed. The compile-time generated-owner no-submit floor is
331K, so submission-only tuning cannot close M2; the next architecture must
also cut the ~178K matrix-preparation wall. A BattleShip ABI mismatch had made
Fox's up/down-smash restore command disable his damage colliders. On exact ROM
`385B9F...FC21`, natural Fox up-smash now restores all 11 active colliders to
Normal with zero mismatch and clears the no-damage flag. Repeated Mario→Fox
contact still awaits manual confirmation and a continuous natural-hit gate.

P1 uses one integration owner plus all three subagent slots whenever three
independent packets exist. Current isolated lanes cover M4 zero-I/O encoding,
M2/M3 owner parity, and KO audio while integration owns gates.

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

Focused profile-1 and canonical profile-0 pre/post-GO runs pass. The automated
one-minute natural-runtime gate also passes logic=3892, timer=3600→0/3600,
scene=22→24, safety=0, stale=0/0, and conservative reserve=171,916 bytes. Full
Regression remains follow-up. Run the Lean snapshot only after all final checks
and status inspection; run no project command after it.

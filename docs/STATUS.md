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
failure, or fallback, 49,152 BG2 pixels, zero generic foreground staging/BG3
copies, live native OAM, exact affine frame conservation, zero coverage failure,
nonidentity motion, and an affine update no greater than 35,000 ticks. Both
fighters retain the 626-triangle contract; cumulative stage totals are 42/202
per traversal plus the exact source-weapon ledger, with unmarked setup traffic
rejected.

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
14,362,624 bytes
SHA-256 57B85DDC6B2919D8962589188D6066F6CE6D0FD83B2F729175C9F339C8CCFAFD
```

Completed frames 438/439 in the dated
`artifacts/visibility/2026-07-14_canonical_fast_frame438-439_200444-1174022-p660.png`
pair pass exact GO/timer/control/OAM state, visibility, detail, named regions,
motion, and sky coverage; frame 438 is `latest.png`. Acceptance is melonDS-only.

## P1 Release Matrix

| Area | Current state |
|---|---|
| Natural one-minute battle and Results | Natural 3,600→0/Time Up/22→24 gate passes; exact canonical-duration qualification remains |
| Gameplay | Fireball render/damage passes; damage/throw collision, one-way platforms, and Fireball rebound are current defects; recovery coverage open |
| Renderer | M1 and native countdown pass; M2 active; M3 open; M4 device falsifier eligible; pause-orbit containment pending |
| HUD/countdown | User-approved lower HUD and top countdown pass |
| Audio | Phase/regular-KO FGMs and winner→Results streams pass naturally; remaining voices/pitch and audible Dream Land proof open |
| Stability/memory | One full match passes with 171,916-byte conservative reserve and zero safety faults; repetition pending |
| Release evidence | Cut G exact-frame capture passes; final dated qualification capture, Full Regression, and exact-ROM retest pending |

Detailed owners, gates, blockers, and evidence live only on the execution board.

## Performance And Open Work

Whole-frame presentation is about 10.3 FPS in the latest synchronized
`laboratory-profile-1` M2 window, not a canonical phase baseline. Sampled lab
gameplay
still reports positive texture conversion and two uploads totaling 36,864
bytes. Therefore:

- Milestone 2 is in progress: Mario/Fox still cost about 431K combined versus
  170–250K. The exact 17,704-byte transaction packet is tooling only; live
  matrices/materials/lights, device cycles, and independent parity remain.
- Milestone 3 is open: the ~801K stage owner needs whole-stage preflight and a
  fused static-slab owner before the 150–250K target is credible.
- Milestone 4 has an exact RGB256 host generator eligible for an eight-frame
  device falsifier. Runtime palette mapping and zero gameplay preparation remain.
- Whispy face, weapon detail, and some fighter lighting/facing remain
  presentation debt. Damage/throw map collision, one-way platforms, and
  Fireball floor rebound are gameplay blockers, not presentation debt.

A BattleShip ABI mismatch had made Fox's up/down-smash restore command disable
his damage colliders. On exact ROM `57B85D...FAFD`, natural Fox up-smash now
restores all 11 active colliders to
Normal with zero mismatch and clears the no-damage flag. Repeated Mario→Fox
contact still awaits manual confirmation and a continuous natural-hit gate.

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

# Fox Blaster — muzzle Y and Mario crouch collision

Bug (verbatim): *"Fox's muzzle flash and laser still spawning at the wrong Y
relative to pistol model. i cannot duck the beam as mario, also check pistol
beam collision maybe have to make it thinner after adjusting height?"*

Stage: **LOCALIZED (gameplay half: no divergence found)** — the visual half is
not yet measured.

Candidate: `builds/build-c142-crouchprobe`,
`smash64ds-battle-playable-tickhud-hwtri.nds`, branch `codex/r2-runtime2`,
HEAD `0174324ba9e` + the owner's dirty tree. Arm: `NDS_R2_BOTH_CPU 0`,
`NDS_R2_POSITION_PROBE 1`, `NDS_TICK_HUD 1`. Root ROMs unchanged this cycle
(`smash64ds-battle-playable-hwtri.nds` sha256 `524448C9…`, `smash64ds.nds`
sha256 `54C07FAC…`). **No ROM was built for any of this.**

Arm note: the probe injects Down on player 0, so it needs `BOTH_CPU 0` (Mario
human). The script's old default `build-c141-position` is `BOTH_CPU 1`, where
Mario is a CPU and ignores the playback pads — wrong arm for this trigger.

## Contract and measured result

Probe: `scripts/probe-fox-crouch-collision.ps1 -Build build-c142-crouchprobe`.
Captures: `artifacts/verification/2026-08-12_fox-hurtbox-poses.txt`,
`…_fox-crouch-hit-pos.txt`.

| Quantity | Expected (source) | Measured | Verdict |
|---|---|---|---|
| Beam spawn Y | joint17 · {60,0,0} (`ftfoxspecialn.c:20-25`) | d = **0.000000** | GREEN |
| Beam spawn Z | same | d = −0.000013 | GREEN |
| Beam spawn X | same | 716.707153 (via `attack_pos`) | GREEN |
| Attack radius | `40 * 0.5F` = 20 (`210_FoxSpecial1.c:28`, `wpmanager.c:206`) | **20.000000** | GREEN |
| Mario hurtbox descs (11) | `203_MarioMain.c:309-326`, halved by `ftparam.c:713` | all 11 exact | GREEN |
| Vertical sweep | `pos_prev`→`pos_curr` | prev.y == curr.y (X-only, +160/tick) | GREEN |
| Collision algorithm | `gmCollisionTestRectangle` (`gmcollision.c:661`) | decomp source, compiled unchanged | GREEN |

**Every quantity that source defines is source-exact.** The four hypotheses in
the brief are therefore all refuted:

- **Case A (crouch pose does not reach collision)** — refuted. One run latched
  both poses (`mask=0x3`); the SquatWait matrices differ from Wait, and at the
  hit joint 12 sits at world y **136.67** vs **246.54** standing, a ~110-unit
  drop. The crouch propagates.
- **Case B (attack radius too large)** — refuted, radius is exactly 20.
- **Case C (swept segment)** — refuted for Y: `pos_prev.y == pos_curr.y`; the
  sweep is purely horizontal.
- **Case D (DS rectangle test differs)** — closed by construction: the port
  compiles `gmcollision.c` from decomp, and `HANDOFF.md` records float in
  `gmcollision` as frozen.

## Why the beam is not duckable, in source numbers

`gmCollisionTestRectangle` transforms the laser into the **hurtbox joint's own
frame** (`mtx = parts->unk_dobjtrans_0x9C`, `gmcollision.c:1396/1515`) and does
an axis-aligned test there against `center = size + radius/scale`. Joint
rotation therefore cannot inflate vertical reach.

For Mario's slot 1 (joint 12): half-size y = 70, scale 1.1161, radius 20, so
`center.y = 70 + 20/1.1161 = 87.9`, about an offset of +68. The box spans
`local.y ∈ [−19.9, 155.9]` around joint 12. With joint 12 at world y 136.67
while crouching and the beam at **223.398254**, the beam is inside. Ducking it
would need joint 12 below ≈67 — another ~70 units, an implausibly deep crouch.

So with source-exact inputs the beam is **not** duckable. Either SSB64 agrees
(and the report is an expectation mismatch), or Fox's joint-17 pose puts the
muzzle lower than the source pose does. The second is the live hypothesis and
it is the same one the visual half needs.

## Two reporting traps found here (do not repeat)

1. **`extent_y = |m01|·sx + |m11|·sy + |m21|·sz` is a world-AABB approximation,
   not the collision.** It inflated slot 1's apparent reach by 57% on a rotated
   joint and produced the numbers 242.2 / 307.4, neither of which is a
   collision bound. It also reads `mtx_translate`, while collision consumes
   `unk_dobjtrans_0x9C` — a different matrix. Report local-space extents.
2. **A GDB inferior call hangs this target.** `call func_ovl2_800EDBA4(...)`
   left the core in libfat `get_fat` and killed the run by timeout after one
   marker line. `CLAUDE.OPUS.md` forbids guest calls for the allocator; it
   holds for any guest function. The dump now reads the in-guest probe arrays
   (`ndsPositionProbeCaptureMarioHurtboxes`), which also latches both poses in
   one ROM. `scripts/probe-fox-crouch-collision.ps1` enforces this.
3. **Maker-entry `r1` is read after GDB's prologue skip.** It reported
   `spawn.x = 0.000000` while Y and Z were bit-exact, which looked like a real
   defect. The weapon's own `attack_pos` (fraction `.707`, +160/tick) proved
   x = 716.707153. Post-`finish` `$fox_weapon->obj` reads −1.9e20 garbage.

## Visual half — LOCALIZED to the 60→30 Hz present boundary

Probe: `scripts/probe-fox-muzzle-alignment.ps1 -Build build-c142-crouchprobe`.
Captures: `artifacts/verification/2026-08-12_fox-muzzle-{alignment-c142,cams,percam}.txt`.

Everything in world space is source-exact:

| Quantity | Measured | Verdict |
|---|---|---|
| Gun draw matrix vs joint 17 | `GUN_FLOAT_WORLD` == `SPAWN_FLOAT_WORLD`, bit-identical | GREEN |
| Gun world muzzle `{60,0,0}` | (716.70717, 223.39825, 28.82047) == source spawn | GREEN |
| Beam/glow draw vs source pos | source == draw in all 10 samples (latch is inert) | GREEN |
| Fighter vs particle projection | **bit-identical** at frame 381 | GREEN |
| Composition equivalence | `shift8(W×P)`·local == `shift8(mv×P)`·world to **0.004 px** | GREEN |

> ## RETRACTED 2026-08-12 — the 3.35 px below is a MEASUREMENT ARTIFACT
>
> `ndsRendererSubmitFoxGun` takes `&gun_world`, a **stack local**
> (`reloc_backend_renderer_dl.c:16115`). The probe dumped it with `x/16dw $r0`
> after GDB's prologue skip. `CLAUDE.OPUS.md` states that stack locals and stack
> objects lie through this stub — globals and pointer-derefs only. The dump is
> provably incoherent: solving `W3 · P = composed_row3` gives a translation of
> **(−390.863, 267.473, 44.614)** while `parts->mtx_translate[3]` read at the
> same breakpoint is **(−390.903, 231.916, 42.174)**. Row 0 matches frame 317
> exactly while row 3's Y is nearer frame **316** — no single matrix is both.
>
> The reliable reads (pointer-derefs into FTParts) say the opposite and agree:
> at frame 317 the gun site and the beam site read the **identical** joint-17
> matrix, and `{60,0,0}` through it is **(−333.293, 223.398, 28.821)** — exactly
> the source spawn. **The gun and the beam are aligned; there is no geometric
> Y divergence.** Everything from "First divergent value" to the phase table
> below is withdrawn, including the pose-lag root cause and the
> "one frame late" claim built on the same dump.
>
> What remains unexplained is the owner's symptom itself. The untested candidate
> is the one this transform work cannot see: the source point `{60,0,0}` is
> where the SHOT spawns, not necessarily where the DS gun MODEL's visible barrel
> tip sits. A model-space mismatch would read exactly as "wrong Y relative to
> pistol model" while every transform above stays source-exact.
> `scripts/fox_gun_screen_bounds.py` is the instrument for that and was not run.

**First divergent value (WITHDRAWN — see the retraction above):** the ROM's own composed gun matrix. With the camera
bit-identical and the local point the same, `W_spawn × P` projects the muzzle to
screen y **98.6525** while the ROM's `GUNCOMPOSED` puts it at **95.3025**. The
joint matrix that DRAWS the gun is therefore not the one that FIRED the shot.

| presented frame | drawn gun muzzle vs fire-tick muzzle |
|---|---:|
| 381 (shot's first frame) | **+33.782 world-Y (3.35 px)** |
| 382 | −0.968 |
| 383 | +82.720 |

**Root cause.** Two unchanged 60 Hz source ticks are collapsed into one 30 Hz
present. Fox fires on substep 1 (`sub=1`, both shots), and the frame that is
actually shown carries the *next* tick's arm pose. The beam and glow sit at the
fire-tick muzzle — correct per source, and
`efManagerFoxBlasterGlowMakeEffect` (`efmanager.c:5517`) confirms the glow is a
static `pc->pos` particle, not joint-attached — so on N64 the aligned frame is
simply one of the two ticks the DS never presents. The later drift is
source-faithful; only the first presented frame is wrong.

This is a presentation-sampling defect, NOT a spawn, camera, attachment, or
collision defect. Fixing it by moving the weapon would be a fidelity bug.

**Traps this half produced:** `fox_muzzle_alignment.py` reported
`dy=+3.6989px` with `VERDICT: FAIL -- weapon motion changed source Y/Z`; the
verdict line was spurious (it paired frame 317's spawn with `SPAWN_SAVED` from
frame 381 and a garbage `beam draw world 0.000000` register read). A first
comparison of gun-vs-particle cameras used a delayed single read taken frames
after the gun draw and appeared to show a stale camera; capturing the fighter
cache per frame at the gun breakpoint showed the projections are identical and
`cacheframe` tracks the frame counter 1:1 (constant −5 offset between two
different counters, not staleness).

## Why the WIP presentation latch is inert

`ndsFoxBlasterGetPresentationPosition` measured as a pure pass-through: in all
3 `BEAM_PRESENT` and 7 `GLOW_PRESENT` samples `source == draw` exactly. The
caller ignores the return value —

```c
draw_pos = root->translate.vec.f;
(void)ndsFoxBlasterGetPresentationPosition(wp->weapon_gobj, ..., &draw_pos);
```

— so a `FALSE` return is indistinguishable from a zero delta. The latch matches
on `weapon_gobj == wp->weapon_gobj && group_id && owner_gobj`; the next cycle
must add an engagement counter on BOTH arms (matched / not-matched) before
trusting any behavior from it. A zero delta here would ALSO be expected if the
joint pose at beam-draw time equals the spawn pose while the gun overlay, drawn
elsewhere in the same present, does not — the two draws are not proven to sample
the same pose, and that is unmeasured.

The latch's header comment claims the shot spawns on hidden substep 0. **Both
observed shots spawn on `sub=1`**, so that premise is not what these captures
show; do not build on it without re-measuring.

**WITHHELD 2026-08-13 — the latch is not in the tree.** It was ungated (it
would have shipped in `smash64ds-battle-playable-hwtri`, where
`NDS_R2_FOX_BLASTER_QUAD ?= 1`), its stated premise is refuted above, and it
carries no engagement counter, so it was reverted rather than committed. It is
preserved verbatim as `wip-presentation-latch.patch` beside this file and
applies on top of the committed probe-only source. `probe-fox-muzzle-alignment.ps1`
lost its `BEAM_PRESENT`/`GLOW_PRESENT` rows with it; re-apply the patch and add
the matched/not-matched counter before re-adding them.

**Phase detail worth keeping:** the gun and beam coincide one frame LATE
(frame 382, dy −0.098 px), and `gNdsFoxGunWorldProbeFloatMtx` — captured at the
first gun draw after the spawn — equals the spawn matrix exactly. So the pose
that fired the shot is displayed on the frame AFTER the shot first appears.
Either the gun overlay's pose sampling or the weapon-spawn-to-present ordering
is one tick out of phase; which of the two owns it is NOT yet measured, and the
fix differs per owner. Do not move source weapon coordinates for either.

## Gun MODEL candidate — also refuted (2026-08-12, no ROM)

`src/nds/nds_fox_gun.c`'s baked 44-vertex table, read in source units
(the raw values ARE source units; `VERTEX_SCALE 16` is applied at submit):

| axis | model range | source shot point |
|---|---|---:|
| X | −72 … **+60** | **60** |
| Y | −42 … +54 | 0 |
| Z | −12 … +12 | 0 |

The barrel tip is at x = **+60**, exactly the source shot point, which confirms
the table is in joint-17 local space. The 12 vertices of that muzzle face span
y = −42 … −6 (centroid −24), so the shot leaves 6 units above the face's top
edge — about **0.6 px** at this scale.

This is NOT a port defect: the header records that positions are "the source
Vtx payload unchanged", `fox_gun_bake.py` pins the asset sha256 and fails
closed, and the source draws the gun by pointing joint 17's own DObj at that
display list — so the model and the shot share one matrix on N64 exactly as
they do here. The same 24-unit shot-above-bore relationship exists in the
original game.

**Every geometric hypothesis for the Fox visual row is now measured and
refuted**: spawn position, attack radius, hurtboxes, camera, composition,
attachment, pose phase, and model offset. No divergence from BattleShip
remains to fix, so this row cannot be closed by an implementation.

Unexamined, and the only candidates left — both about the QUAD rather than its
position: `ndsRendererSubmitFoxBlasterQuad` receives `scale_x = 6.333`,
`scale_y = 1.0` (`sx=0x40caaaab sy=0x3f800000`), and nothing here checked the
quad's vertical ANCHOR (centre vs edge) or those scales against the source
particle's own size. A half-height anchor error would read as "wrong Y" while
every position above stays source-exact.

## Not done / inherited

- The **visual** half is unmeasured: whether the drawn gun, muzzle flash and
  beam agree on screen with joint 17 (world y **231.915787**; muzzle
  y **223.398254**). Instrument exists: `scripts/probe-fox-muzzle-alignment.ps1`
  + `scripts/fox_muzzle_alignment.py`.
- Whether Fox's **SquatWait/SpecialN joint-17 pose** matches source is the one
  quantity above with no offline oracle yet.
- Mario's `anim_frame` reads **0.000000** in SquatWait at the hit; not yet
  checked against source SquatWait animation behavior.
- Kirby's copied Fox blaster path (`230_KirbySpecial1.c` shares
  `dFoxSpecial1_Blaster_WeaponAttributes`) is unchecked.

# Fox blaster — the owner's draw-only bore offset, applied and proved

**Owner decision, given in chat 2026-08-13:** option **B** of
`artifacts/bugs/2026-08-12_fox-crouch/BEAM_QUAD_ANCHOR.md` — *raise the beam and
muzzle-flash quads by joint-local (0, −24, 0) at draw time; gameplay, spawn,
`attack_pos` and collision stay byte-identical to source.* This file is the
record of that decision being executed, not a proposal.

`NDS_R2_FOX_BLASTER_QUAD ?= 1` and `NDS_R2_FOX_BLASTER_GLOW_AOT ?= 1` ship, so
**this changes the published ROM.** That is expected: it is an ownered visual
fix. No published target was built in this cycle; the next one will differ.

## 1. What shipped, and where it landed in the transform chain

| | file:line | what |
|---|---|---|
| constant | `include/nds/nds_renderer.h:1391-1416` | `NDS_FOX_BLASTER_BORE_OFFSET_Y 24`, `…_Q12` = `24 << 12` = 98,304 |
| **beam** | `src/nds/nds_renderer.c:14979-14991` | `ty += …_Q12` on the **decoded translation**, after `ndsRendererFoxBlasterFloatToQ12`, **before** `span_x/span_y0/span_y1` apply the source scale |
| **flash** | `src/import/battleship_lbparticle.c:2571-2606` | `draw_center_q12[1] = glow.center_q12[1] + …_Q12` into a **draw-local** array; the pool entry and `glow.pos` are not written |

**Why it is placed there and not later.** `scale.x` runs 1.0 → 53.33 over a
shot's first ten ticks (`wpvars.h:9-10`). The offset is in *unscaled* local
units, so folding it in after the scale would stretch a 24-unit raise into a
1,280-unit one as the beam grew. Adding it to the translation is the "added to
the local translation before the world transform" arm of the brief; the source
quad's own four vertices still carry the whole of `R·S`.

**Why it is a world +Y constant and not a joint-matrix multiply.** The offset
has to hold for the shot's entire flight, and the shot leaves the gun the
instant it is made — there is no joint frame travelling with it, and sampling
joint 17 at draw time would make the beam bob with Fox's arm. The admitted fast
path is exactly `rotate.z` 0 or π with velocity (±160, 0, 0), i.e.
world-axis-aligned. At the fire pose measured last cycle
(`BEAM_QUAD_ANCHOR.md` §4) joint-local (0, −24, 0) images to world
**(+3.505, +23.651, +0.049)**, so the axis-aligned form differs from the exact
image by **0.349 world Y and 3.505 world X** — 0.035 px and 0.35 px at the
measured screen scale below, against the 0.5 px composition tolerance already
recorded for this row.

**The impact flashes move too, deliberately.** All four source callers of
`efManagerFoxBlasterGlowMakeEffect` pass the weapon's own translation
(`wpfoxblaster.c:61/71/86/121`), so the muzzle flash and the impact flash both
sit on the beam's centre line. Raising the beam without them would detach them
from it.

**Known unraised path, and it never fired:** the pool-full fallback
`ndsBaseEFManagerFoxBlasterGlowMakeEffect` builds a real `LBParticle` at the
unraised `pc->pos`. `gNdsFoxBlasterGlowAOTFallbackCount` read **0** on both
arms of a whole 60-second both-CPU match, so the inconsistency is unreachable
in practice. `pc->pos` is deliberately not touched.

## 2. Gameplay invariance — by construction and by measurement

**By construction.** `ndsRendererSubmitFoxBlasterQuad` takes
`const Vec3f *translate` and never writes through it; its only caller is
`ndsStageGCDrawAllLoopDrawFoxBlasterQuad`
(`src/port/reloc_backend_movement.c:13079-13115`), inside the draw pass.
`ndsParticleDrawFoxBlasterGlowAOT`'s only writes to the pool are the pre-existing
compaction copy, which happens **before** the draw-local array is built. Nothing
on the spawn side (`wpFoxBlasterMakeWeapon`, `attack_pos`,
`gmCollisionTestRectangle`) was touched.

**By measurement.** One 60-second both-CPU match per arm, same start frame, same
sample count, 1,600 samples, DLDI ON, `NDS_TICK_HUD_DRAW 1`. Six gameplay
globals were read at **every one of the 17 ring stops** (not only at the end):

```
stop from    to     P0dmg P1dmg P0stk P1stk shots beamDraws
0    438     534        2     3     1     1     2        31
5    918    1014       60     3     1     1     3        31
10   1398   1494      104    51     1     1     7        38
15   1879   1974        0    58     1     1     8        38   <- P0 KO'd and respawned
16   1974   2038        0    58     1     1     8        38
```

**0 mismatches across 17 stops × 6 globals.** End-of-run reads agree on all
fourteen: `P0Damage 0`, `P1Damage 58`, stocks `1/1`, `SpawnCallCount 8`,
`SpawnSuccessCount 8`, `QuadDrawCount 38`, `QuadFallbackCount 0`,
`GlowAOTSpawnCount 7`, `GlowAOTDrawCount 30`, `GlowAOTFallbackCount 0`,
`GlowAOTMissCount 0`, `KOBurstAttemptCount 0`, `KOBurstCompleteCount 0`.
Same RNG stream, same collisions, same match. The only cross-arm differences in
the whole run are ±1 presented-frame **labels** at four ring seams, which the
sampler warns about by construction.

## 3. Performance — an invariance check, not a perf claim

| bucket | control `build-c136-animjoint` | candidate `build-c143-bore` | delta |
|---|---:|---:|---:|
| **`WORK-H` P50** | 924,928 | **925,184** | **+256** |
| **`WORK-H` P95** | 1,260,096 | **1,250,368** | **−9,728** |
| `ALL` P95 | 1,678,720 | 1,678,656 | −64 |
| `FTR` P95 | 323,008 | 322,816 | −192 |
| `SRC` P95 | 596,480 | 591,552 | −4,928 |
| VBlank 2 / 3 / 4 / 5+ / max | 1,697 / 310 / 18 / 13 / 26 | 1,700 / 310 / 15 / 13 / 26 | 3 frames move 3→2 |
| cadence slips | 0 | 0 | 0 |

The control run **reproduces the banked gate figure exactly** — 924,928 /
1,260,096 and the identical VBlank histogram of
`../2026-08-13_c-animjoint-fix/ANIMJOINT_FIX.md` §7 — which is what makes it a
control rather than a relabelled candidate.

**This is not a re-bank.** The change adds one integer compare-and-add per beam
submit and one per flash submit, on 38 and 30 submits in a 1,600-frame match.
`arm-none-eabi-size` reads text **981,524 → 981,340 (−184)**, data and bss
unchanged — the candidate is *smaller*, so the P95 move is placement, not work.
Judge it at P50 (+256, inside the ±8,544 floor) and treat the P95 as the
cross-build floor doing what it always does. The gate stays at
**raw 1,260,096 / net ≈1,235,149**.

## 4. Pixel proof — `EXACT_LOCK` on the guest match clock

Three synchronized pairs, both arms locked on
`EXACT_LOCK=gSCManagerBattleState->time_remain`, software renderer, guest
viewport 400×600 at window (8, 56). Beam pixels are relocData 316's own
RGBA(219, 0, 134); the test is `R>170, G<60, 90<B<190`.

| lock | what is on screen | control | candidate | result |
|---|---|---|---|---|
| **1694** | the frame *before* the first beam draw | — | — | **battle screen PIXEL-IDENTICAL, 0 / 120,000** |
| **1688** | beam + muzzle flash at peak | beam rows **202–208**, centroid **205.000**, 707 px, x 130–230 | beam rows **199–205**, centroid **202.000**, 707 px, x 130–230 | **−3.000 rows**, identical size and length |
| 1682 | beam, flash expired | rows 201–206 | rows 198–205 | beam still 2–3 rows higher |

The muzzle flash moved with it: centroid **206.367 → 203.358**, **−3.009 rows**
— the same displacement to a hundredth of a pixel, so beam and flash stay
coincident. Whole battle screen at lock 1688: 1,387 of 120,000 pixels differ
(1.16%), all of them the beam and the flash.

**The screen scale, measured here:** 24 world units = 3.000 rows ⇒ **1 row ≈ 8
world units**. That is an independent confirmation that the offset landed at
exactly its intended magnitude, and it agrees with the prior cycle's
"6 units ≈ 0.6 px".

**Where the beam now sits, in world units** (arithmetic on last cycle's measured
joint-17 matrix, `BEAM_QUAD_ANCHOR.md` §4):

| | control | candidate |
|---|---:|---:|
| beam band (world y) | 197.398 … 247.398 | 221.398 … 271.398 |
| beam centre vs **bore centre 247.049** | **24.651 below** | **0.651 below** |
| muzzle face 229.311 … 264.788 | 51% of the face overlapped | **face entirely inside the beam band** |
| beam hanging below the face | **63.8%** | **15.8%** |

Lock 1694 is the negative control the change requires: on a frame with no beam
and no flash, 120,000 pixels of battle screen are bit-identical, so the offset
is inert everywhere it must be.

### Fidelity record (`AGENTS.md` "DS Visual Fidelity")

- **Source:** owner decision 2026-08-13, option B of `BEAM_QUAD_ANCHOR.md`.
- **Visible delta:** the beam and its flash draw 24 world units (3.000 screen
  rows) higher than BattleShip draws them, onto the gun's bore line.
- **Measured reason:** BattleShip's own data spawns the shot at joint-17 local
  (60, 0, 0) while the bore centre is local (60, −24, 0), a 23.651-unit sag that
  puts 63.8% of the beam below the barrel *in the original game*. The owner
  chose alignment over source-exactness.
- **Screenshots:** `artifacts/visibility/2026-08-13_fox-bore_{ctl,cand}_t{1694,1688,1682}.png`,
  annotated crops `…_crop_t1688.png` / `…_crop_t1682.png`, diffs
  `…_diff_t1688.png` / `…_diff_t1694.png`.

## 5. What was NOT done

- **No on-screen measurement of the bore itself.** The muzzle flash's peak
  half-extent is 165 world units, 3.4× the gun's height, and it is centred on
  the shot — so it covers the muzzle for exactly the frames on which the beam's
  tail is at the muzzle. By the time it expires (lock 1682) Fox has drifted and
  the tail has left the gun. That is a property of the effect, not of the
  instrument; the bore relationship above is arithmetic on last cycle's measured
  joint matrix, and the owner's eye is the remaining check.
- No five-minute or soak arm; no published build; no re-bank.
- The `NDS_R2_FOX_BLASTER_QUAD 0` / `…GLOW_AOT 0` interpreted lab arms are
  **not** offset — they draw through the generic display-list and source-maker
  paths. Both flags ship at 1; a lab arm built at 0 is now a different picture
  as well as a different cost.

## 6. Arms, ROMs, verifier

| build | source | arm | role |
|---|---|---|---|
| `build-c136-animjoint` | HEAD `cffcea495a6` (docs-only since `607d3697455`) | `BOTH_CPU 1`, soak 0, audit 0 | **control** |
| `build-c143-bore` | HEAD + this change | same | **candidate** |

Target `smash64ds-battle-playable-tickhud-hwtri`, built one at a time, no `-j`,
`MAKEFLAGS` untouched. Emulator: repo-local `emulators/melonds/melonDS.exe`,
DLDI **ON**. The sampler's own `gitShort` reads `cffcea495a6+dirty(5)` on both
runs because it records the repo HEAD at sample time; **arm identity is the
build directory**, per `AGENTS.OPUS.md`.

`scripts/verify-boundary.ps1 -DelaySeconds 3 -RunnerSlot 2` — **"Boundary
verification profile passed."**, exit 0. The full 18,960,666-byte console log
was scanned: `Exception:` **0 hits**, `=FAIL` **0 hits** (`boundary.log` beside
this file carries the header and tail). Boundary is the right width: the change
is battle-only and touches no normal or shared startup path.

**Root ROMs byte-identical across the whole cycle**, hashed before the first
build and after the verifier:

```
smash64ds.nds                       11,915,264  54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
smash64ds-battle-playable-hwtri.nds 12,225,536  524448c99c31b62672a63f29914438059d5f9700e10306d147d6342b3223adee
```

## 7. Two things worth inheriting

- **`GlowAOTSpawnCount` reads 7 against 8 successful weapon spawns**, on both
  arms, with `GlowAOTFallbackCount` 0. Every source path that makes a blaster
  also makes a glow, and neither refusal branch was taken, so one glow is
  unaccounted for. It is identical on both arms so it cannot affect this
  cycle's conclusion, but it is an unexplained counter and belongs to whoever
  next touches this pool.
- **A probe capped on a cumulative counter it has not sized will run the whole
  match.** The beam-clock probe asked for 40 draws; the match only produces 38,
  so it ran to its 600-second ceiling after collecting everything it needed at
  frame 1468. Cap a hit loop on its own hit count, never on a global whose
  ceiling is the thing being measured.

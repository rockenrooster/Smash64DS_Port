# Peach's Castle — P2-4 stage 2

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: castle rooftop main deck with angled side ramps, raised side
  platforms, and the tower structure; asymmetric ledges.
- **Hazards/interactives**:
  - **Bumper**: fixed above center stage — strong fixed-knockback bounce on
    contact; exact knockback/priority from source (it interrupts combos and
    recoveries; players know its feel).
  - **Sliding platform**: the platform that traverses beneath/beside the
    stage on a rail (verify exact path/timing from source data) — moving
    collision carrier: fighters/items must ride it correctly.
- **Set pieces**: castle towers, background Lakitu? (verify background-only
  props).
- **Music**: Peach's Castle (SMB medley) track.
- **Visual treatment**: bright low-poly architecture — near-direct
  conversion; skybox as 2D BG.

## DS notes / risks

- First *moving platform carrier* in P2 — riding logic (fighter velocity
  inheritance, items later) lands here; get it at the shared platform seam,
  Dream Land had none.
- Bumper is the first stage-owned hitbox — wire through the engagement
  system as a stage actor, the pattern every later hazard reuses.

## Acceptance

- [ ] Collision parity sweep incl. moving platform ride/dismount cases.
- [ ] Bumper knockback equivalent (source values).
- [ ] Platform path/timing equivalent.
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked.

## First boot, and what it ruled out (2026-09-03)

Built as `TARGET=smash64ds-p2-shell-loop-hwtri BUILD=build-p2-shell-loop-castle
NDS_P2_STAGE_CASTLE=1` and run through `verify-p2-shell-loop.ps1 -NoBuild`.

**It is selected without being asked for.** `LOOPCFG ... gkind=00` — Castle is
`nGRKindCastle`, which is 0, and the stage select's cursor starts on slot 0. So
setting Castle's mask bit changes which stage the scripted lap plays. Every
stage added after this one moves another cell out of the locked set, so expect
the lap's default stage to keep changing.

**It crashes in ground-data init.** `LOOPABORT n=7 pc=01fffbe8 lr=0206ca4e`;
`nm` resolves those to `__excpt_entry` and **`mpCollisionInitGroundData +
0xb2`**. That is the *decomp* function
(`decomp/.../mp/mpcollision.c:3961-3981`), not the port's compat loader — it
indexes `dMPCollisionGroundFileInfos[gkind]`, allocates by
`lbRelocGetFileSize`, loads, and immediately dereferences
`gMPCollisionGroundData->map_geometry`. The three other assertion failures in
that run (rematch, results press, plaque count) are all the aborted lap, not
separate defects.

Ruled out, each by inspection rather than by theory:

- **Not a missing asset.** The whole dependency closure is staged in the lab
  NitroFS tree: `GRCastleMap` (id 0x103, 290 B) needs exactly `0x5a`, `0x6a`
  and `0x9c`; `MVOpeningRoomWallpaper` (0x5a, 159,008 B),
  `ExternDataBank106` (0x6a, 17,776 B) and `MiscDataBank156` (0x9c, 144 B) are
  all present and declare no externs of their own.
- **Not a malformed map file.** `GRCastleMap` and `GRPupupuMap` are both 290
  bytes with identical header shape; only the ids differ, and Castle's
  externs are exactly the three files above.
- **Not a port array overflow.** Castle's geometry is 25 vertices, 16 vertex
  links, 4 line-info groups, `yakumono_count` 4 and `mapobj_count` 36
  (`106_StageCastleFile2.c:545,581,588,636-644`). Dream Land's are 19, 7, 1,
  1 and 42. The port's caps are 128 vertices, 64 line extents, 64 line
  endpoints (`src/port/reloc_backend_mp_collision.c:341,378,428`) and 64
  yakumono DObj slots (`include/gr/ground.h:82`). Nothing is close.

So the next step is a GDB stop at `__excpt_entry` reading the fault address and
the registers, plus `gNdsRelocFileSizeFallbackCount`, `Token` and `Asset` —
the 68-byte `sizeof(Sprite)` fallback is the one shape a static read cannot
rule out from here, because it depends on what the running token resolver
answers.

# Congo Jungle — P2-4 stage 3

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: large wooden main platform, two lower side platforms, upper
  platforms; open underside.
- **Hazards/interactives**:
  - **Barrel Cannon**: patrols beneath the stage on a path, rotates; a
    fallen fighter enters it, aims with rotation, fires on input (or
    timeout? — verify) — a rescue/KO-mixup mechanic. Two-body-ish state
    (fighter-in-barrel), input semantics and launch power from source.
- **Set pieces**: jungle backdrop, waterfall (animated background — reduced
  rate per visual doctrine is fine).
- **Music**: Congo Jungle (DK) track.
- **Visual treatment**: wood/vine textures, dark palette; waterfall as
  scrolling 2D layer candidate.

## DS notes / risks

- Barrel is fighter-state machinery owned by the stage — implement via the
  stage-actor seam with a fighter-capture state (reuses capture plumbing
  from grabs/eggs).
- Barrel path timing + rotation rate are gameplay (recovery planning);
  source-exact.
- Underside camera: fights extend far below the deck — bounds check.

## Acceptance

- [ ] Collision parity sweep.
- [ ] Barrel: entry, aim, fire, cooldown, path/rotation timing equivalent.
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked.

## Source pins (verified 2026-09-03)

Internal name `Jungle`, kind `nGRKindJungle` (`gr/grdef.h:13`). Paths relative
to `decomp/BattleShip-main/decomp/src/`.

- Map `relocData/261_GRJungleMap.c`: header `dGRJungleMap_MapHeader_0x0014:31`,
  layer table `:33-39`, throw descriptor `dGRJungleMap_TaruCannThrow_HitDesc:80`.
- Collision `dStageJungleFile2_MPGeometryData_0x9AFC`
  (`relocData/108_StageJungleFile2.c:855`).
- Logic `gr/grcommon/grjungle.c`, 202 lines, one hazard -- the barrel cannon:
  `AddAnimOffset:37`, `AddAnimFill:47`, `AddAnimShoot:53`, `UpdateMove:59`
  (counts down, then rotates by a random plus/minus 0.07 step, wait 90),
  `UpdateRotate:74`, `ProcUpdate:92`, `MakeTaruCann:107`, `MakeGround:134`,
  `CheckGetDamageKind:142` (280-unit box at `:165`, giving
  `nGMHitEnvironmentTaruCann` at `:182`), pose exports `:193` and `:199`.
- Seam: **not** Whispy's velocity push. The cannon is the ground-obstacle
  capture seam -- `ftMainCheckAddGroundObstacle` (`ft/ftmain.c:1592`) into
  `ftCommonTaruCannSetStatus` / `ShootFighter`
  (`ft/ftcommon/ftcommontarucann.c:60`, `:97`).
- Music `nSYAudioBGMJungle = 5`. Stage-select icon
  `llMNMapsCongoJungleSprite` (`mn/mnmaps.c:516`), name
  `llMNMapsCongoJungleTextSprite` (`:585`).
- Risk: the shoot math reads `gMPCollisionGroundData` together with the throw
  hit descriptor (`ftcommontarucann.c:100`), and the cannon's `map_head` is
  `map_nodes - &llGRJungleMapMapHead` (`grjungle.c:112`). Both need the exact
  link symbols or capture and launch break.

## The barrel cannon seam, measured (2026-09-03)

Congo Jungle is 202 lines of stage logic, but its hazard is a fighter
*capture*, so the real question was how much of the fighter side exists. Most
of it does. Paths are relative to the repo root; decomp paths keep their
`decomp/BattleShip-main/decomp/src/` prefix implied.

**Present in the port already:**

- The ground-obstacle registry and its dispatch —
  `src/port/reloc_backend_ftmain_runtime.c:1305` (check-add), `:1323` (clear),
  `:1357` (search-hit), obstacle count 2 at `:1294`, and
  `ftMainSetHitHazard:1342` already routes both Twister (`:1347`) and TaruCann
  (`:1351-1353`).
- `ftCommonTaruCannSetStatus` — complete at
  `src/port/reloc_backend_compat_shims.c:9626-9665`, matching decomp
  `ftcommontarucann.c:60-94` including the heavy-item drop, the thrown and
  captured releases, intangibility, invisibility, the full capture-immune mask
  and the enter cue.
- `ftCommonTaruCannProcPhysics` — `compat_shims.c:9601-9624`, the parenting
  copy from decomp `:51-57`.
- The pickup cooldown tick, `reloc_backend_ftmain_runtime.c:1379-1382`.

**Missing:**

- `ftCommonTaruCannProcUpdate` and `ProcInterrupt` — stubbed at
  `src/import/battleship_ftstatus_inactive_stubs.c:45-46`. Source is
  `ftcommontarucann.c:8-33` and `:37-47`: the shoot countdown with its cue at
  half of `FTCOMMON_TARUCANN_SHOOT_WAIT`, the 180-frame auto-fire, and the
  A/B tap that fires early.
- `ftCommonTaruCannShootFighter` — absent entirely. Source is `:97-116`: the
  throw descriptor read through
  `gMPCollisionGroundData - &llGRJungleMapMapHeader +
  &llGRJungleMapTaruCannThrowHitDesc`, knockback from
  `ftParamGetGroundHazardKnockback(..., 9, 9)`, angle
  `(rotation_degrees * -lr) + 90` normalised, exit through
  `ftCommonDamageInitDamageVars(nFTCommonStatusDamageFlyRoll, ...)`, and
  `tarucann_wait = FTCOMMON_TARUCANN_PICKUP_WAIT`.
- ~~**The status arm is wired but neutered**~~ and ~~**the stage side entirely
  is a stub**~~ — **BOTH SUPERSEDED, 2026-09-04.** This section described the
  pre-landing tree and was still being read as current, so it is struck rather
  than deleted.

  The barrel landed with `NDS_P2_STAGE_JUNGLE`. The stage half is strong
  whenever that flag is on: `src/import/battleship_grjungle_ground.c:73`
  includes `grjungle.c` verbatim, so `grJungleMakeGround`, `MakeTaruCann`,
  `TaruCannProcUpdate`, `CheckGetDamageKind`, `AddAnimShoot`, `GetPosition` and
  `GetRotate` are all real definitions that beat the weak stubs at
  `src/port/battle_playable_compat_stubs.c:83,93`. The setup gate is
  `battleship_grpupupu_ground.c:566-577`, which dispatches
  `ndsGRJungleSetupInitAll` on `gkind == nGRKindJungle`.

  The fighter half is strong too: `reloc_backend_compat_shims.c:9661`
  (`SetStatus`) and `:9636` (`Physics`) are unconditional, and `:9702` guards
  `ShootFighter` (`:9767`), `ProcUpdate` (`:9809`) and `ProcInterrupt`
  (`:9847`). The status arm at
  `reloc_backend_ftmain_status_compat.c:1229-1230` assigns the real procs when
  the flag is on; the `NULL` pair this section complained about is now the
  `:1234-1235` flag-off arm.

  **The shipped ROM has the flag on**, so what the owner reported as *"barrel
  movement is incorrect"* is a behaviour question about landed code, not a
  missing implementation. Check the reloc ids and offsets at
  `battleship_grjungle_ground.c:66-69` (`0x105`/`0x5c`/`0x6c`/`0x9e`, offsets
  `0xa98`/`0xb20`/`0xb68`/`0xbf8`) first: a wrong one gives a cannon that is
  present but mis-posed, which matches the report better than absent code does.

**The seam is shared with exactly one other stage.** `ftMainCheckAddGroundObstacle`
has two callers in the whole game: `grjungle.c:126` and `grhyrule.c:172`. Note
that Planet Zebes and Mushroom Kingdom use a *different* seam,
`ftMainCheckAddGroundHazard` (`grzebes.c:219`, `grinishie.c:538`) — the two are
separate entry points in `ft/ftmain.h`, so do not assume one covers the other.

**Status promotion is global, not per fighter.** A common status is added by one
arm in `src/port/reloc_backend_ftmain_status_compat.c` (Twister's is `:1155`,
TaruCann's `:1199`), after `ndsFTMainApplyCommonStatusReset`. It applies to
every fighter; only the captured fighter's own `status_vars` are written.

**Ordering consequence.** The item core unlocks three stages (Castle, Mushroom
Kingdom, Saffron City); the ground-obstacle seam unlocks two (Congo Jungle,
Hyrule Castle). The item core wins on count, which is why it is queued first.

## Music: rendered, not landed, and why (2026-09-03)

`nSYAudioBGMJungle` is sequence 5, and the stage-neutral renderer produces a
track from it — but the output is anomalous enough not to ship without
checking, so it is recorded here rather than pinned.

| track | notes | source PCM | IMA bytes | loop span (ticks) |
|---|---:|---:|---:|---:|
| Dream Land | 1,804 | 2,843,290 | 711,920 | 298 → 96,298 |
| Peach's Castle | 2,252 | 3,719,952 | 931,400 | 3,086 → 72,206 |
| Yoshi's Island | 1,473 | 2,605,160 | 652,292 | 8,917 → 74,197 |
| **Congo Jungle** | **5,810** | **11,678,048** | **2,923,840** | **373 → 159,813** |

Congo Jungle comes out roughly four times the size of every other track, with
2,904 replica notes unrolled across channel loops against 26 to 30 for the
others, and its loop start lands at **50.05% of the stream** — the midpoint.
That is the signature of the majority-period detector choosing a period twice
the musical loop, rendering the tune twice and looping the second copy.

It is a suspicion, not a proof: this track could genuinely be longer. Before
pinning it, compare the per-channel periods the renderer's
`collect_loop_metadata` computes against the majority it picks. If the period
is doubled, the fix belongs in the renderer, and Yoshi's Island and Peach's
Castle — whose channels also disagree — should be re-checked with it.

Everything else for this stage is landed: gameplay, the cannon's fighter half,
asset rows, stage-select art and mask.

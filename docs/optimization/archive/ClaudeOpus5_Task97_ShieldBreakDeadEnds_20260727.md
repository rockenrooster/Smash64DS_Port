# Task 97 E0 — Shield break enters and never leaves, and FuraFura is unreachable

**Date:** 2026-07-27
**Status:** **E0 complete. GO on the import**, scoped below. No runtime change yet
— this one alters gameplay behavior deliberately, so it is documented before it
is made.
**Lane:** `docs/P1_EXECUTION_BOARD.md` Red Queue item 2 — "replace
battle-reachable weak status callbacks with source-backed behavior". Not the
performance campaign; that lane is blocked on the owner's visual-fidelity call
(Task 96 §5a).

## 1. Why this lane

Red Queue item 1 (30 FPS) is blocked on a decision only the owner can make.
Items 2–5 are red and are not. `AGENTS.md`'s operating model says to take the
highest-impact unowned red row, so this is item 2.

## 2. The census

`src/import/battleship_ftstatus_inactive_stubs.c` defines **98** weak status
callbacks as empty bodies. Most are correctly fenced — filtering by the Boundary
configuration (Mario vs level-3 Fox CPU, Dream Land, **items off**, one-minute
Time) removes:

| group | why unreachable |
|---|---|
| `FireFlowerShoot*`, `Hammer*`, `Harisen*`, `StarRod*`, `LGunShoot*`, `ItemThrow*`, `HeavyThrow*`, `*Get*`, `Lift*` | items off |
| `Capture{Kirby,Yoshi,Captain,Shouldered}*`, `Thrown*Star*`, `YoshiEgg*` | Mario vs Fox only |
| `TaruCann*` (barrel), `Dokan*` (pipes) | not on Dream Land |
| `Sleep*` | item-induced |

`Dead*` and `Rebirth*` are **already real** — `ftCommonDeadCommonProcUpdate` is
0x24 bytes of code, `ftCommonRebirthWaitProcUpdate` 0x4c. That lane is done.

What is left is one cluster, and it is a complete lifecycle:

```
ftCommonShieldBreakFlyProcUpdate     0x02078000  2 bytes  W
ftCommonShieldBreakFlyProcMap        0x02078004  2 bytes  W
ftCommonShieldBreakFallProcMap       0x02078008  2 bytes  W
ftCommonShieldBreakDownProcUpdate    0x0207800c  2 bytes  W
ftCommonShieldBreakStandProcUpdate   0x02078010  2 bytes  W
ftCommonFuraFuraProcUpdate           0x02078014  2 bytes  W
```

Two bytes each is `bx lr`. Six consecutive empty stubs.

## 3. The defect, which is worse than "a callback is empty"

Shield break **is** entered. `src/port/reloc_backend_compat_shims.c:1624`
implements `ftCommonShieldBreakFlyCommonSetStatus` by hand and the damage path
calls it (`reloc_backend_diagnostic_recorders.c:6735`, `:12169`, `:17248`). But
because the real callbacks were stubs, that hand-written entry point **overrides
the status table after setting it**:

```c
ftMainSetStatus(fighter_gobj, nFTCommonStatusShieldBreakFly, ...);
fp->proc_update = NULL;                        /* source: -> ShieldBreakFall  */
fp->proc_map    = ftCommonDamageFallProcMap;   /* source: -> ShieldBreakDown  */
fp->physics.vel_air.y = 0.0F;                  /* source: shield_break_vel_y  */
```

Three mechanical divergences from `ftcommonshieldbreakfly.c:23`:

1. **No launch.** Source sets `vel_air.y = attr->shield_break_vel_y`; the port
   sets `0.0F`. A broken shield does not pop the fighter upward.
2. **Wrong landing handler.** Source routes `ProcMap` to
   `ftCommonShieldBreakDownSetStatus`; the port routes to
   `ftCommonDamageFallProcMap`, i.e. into the ordinary damage-fall/DownBounce
   chain.
3. **No anim-end transition.** `proc_update = NULL` means the Fly → Fall step
   never fires.

**Net effect: `FuraFura` — the dizzy/stunned state after a shield break — is
unreachable in the port.** The whole Fly → Fall → Down → Stand → FuraFura → Wait
sequence collapses into "damage fall". That is a core SSB64 mechanic absent from
a battle-reachable path, and it is exactly the shape `AGENTS.md` forbids: a
shared defect hidden behind per-site overrides rather than fixed at its owning
seam.

## 4. Why the import is contained

Five decomp TUs, **226 lines total**:

```
ftcommonshieldbreakfly.c    80    ftcommonshieldbreakstand.c  29
ftcommonshieldbreakfall.c   30    ftcommonfurafura.c          51
ftcommonshieldbreakdown.c   36
```

**Every gameplay dependency already exists as a real implementation.** Checked
against the linked ELF:

| symbol | state |
|---|---|
| `ftAnimEndCheckSetStatus`, `mpCommonProcFighterLanding`, `mpCommonSetFighterAir`, `mpCommonSetFighterGround` | `T`, real |
| `ftMainSetStatus`, `ftMainPlayAnimEventsAll`, `ftParamCheckSetFighterColAnimID` | `T`, real |
| `ftPhysicsClampAirVelXMax`, `ftParamMakeRumble`, `gmCollisionGetFighterPartsWorldPosition` | `T`, real |
| `ftCommonCaptureTrapped{Init,Update}BreakoutVars`, `ftCommonWaitSetStatus` | `T`, real |
| `ftCommonDownBounce{CheckUpOrDown,UpdateEffects}` | defined at `reloc_backend_compat_shims.c:10033,10063`; absent from the ELF **only because `--gc-sections` dropped them for lack of a caller**. The import restores the reference. |

Constants and enums all resolve: `FTCOMMON_FURAFURA_BREAKOUT_WAIT_{DEFAULT,MIN}`
(400/90) in `decomp/.../ft/ftcommon.h:256`, `nFTCommonStatusShieldBreak*` in
`include/ft/fighter.h:1011`, `nGMColAnimFighter{ShieldBreakFly,FuraFura}` = 36/37
at `:4199`, `nSYAudioFGMShieldBreak` = 15 in `include/gm/gmsound.h:32`,
`attr->shield_break_vel_y` at `:3460`.

**Zero cascade.** No new subsystem, no new dependency chain.

## 5. Scope boundary — what the import deliberately excludes

Two functions in `ftcommonshieldbreakfly.c` are *not* battle-reachable in
Boundary and each needs a symbol the port does not have:

- `ftCommonShieldBreakFlyCommonSetStatus`'s 1P-game branch needs
  `gSC1PGameBonusShieldBreaker`, which **does not exist anywhere in the tree**.
  Boundary is VS, so the branch never executes — but it must link.
- `ftCommonShieldBreakFlyReflectorSetStatus` needs `fp->reflect_lr` (not found in
  `include/ft/fighter.h`) and `efManagerReflectBreakMakeEffect` (absent).

So the import takes the **core chain** — `Fly{ProcUpdate,ProcMap,SetStatus}`,
`Fall`, `Down`, `Stand`, `FuraFura` — and leaves the two entry-point variants
with the port's existing implementations, corrected to stop overriding the status
callbacks and to use `attr->shield_break_vel_y`. The port's entry point keeps its
NULL guard, which the source version does not have.

Cosmetic effects stay weak-stubbed, matching `efManagerEggBreakMakeEffect` which
already is: `efManagerShieldBreakMakeEffect` (declared weak at
`reloc_backend_compat_shims.c:1670`, currently GC'd),
`efManagerYoshiEggExplodeMakeEffect`.

## 6. The gate consequence, stated before the change is made

**This changes gameplay behavior on purpose.** A broken shield will launch,
progress through the real chain, and end in FuraFura. If the Boundary one-minute
match contains a shield break, the **Task 9 state hash will change**, and that is
a correct change rather than a regression to be reverted.

Per `TASK_STANDING_RULES.md` §"Budget ratchets vs correctness assertions", a
state-hash movement here is neither — it is a *behavior* change toward source
equivalence, which is the one thing that legitimately re-baselines the hash. It
must not be waved through silently: E1 records the before/after status sequence
for the affected frames and shows the new one matches the source state machine.

If the match never breaks a shield, the hash will not move at all and Boundary
stays green on the existing baseline. Either outcome is informative; neither is
assumed.

## 7. E1

1. Add five import shims under `src/import/` (auto-globbed; `Makefile:697` lists
   `src/import` in `SOURCES`), following
   `src/import/battleship_ftcommon_fallspecial.c`'s pattern.
2. Rename the two excluded entry points to `ndsBase*` in the fly shim so they do
   not collide with the port's, as
   `src/import/battleship_ftcommon_downwaitbounce.c` already does for its TU.
3. Remove the six names from `battleship_ftstatus_inactive_stubs.c`.
4. Correct `reloc_backend_compat_shims.c:1624` — drop the `proc_*` overrides,
   restore `attr->shield_break_vel_y`.
5. Boundary verifier. `PROJECT_GOAL.md` gates gameplay on mechanical
   equivalence, so this is the widest relevant gate and the only one needed.

### The one hazard, found while checking for pinned behavior

**No verifier script references `ShieldBreak` or `FuraFura` at all**, so nothing
in `scripts/` asserts the current wrong behavior and step 4 is not "editing a
correctness contract to make a change pass" (the Task 82 E1 trap).

But three **in-ROM** recorders do, at
`reloc_backend_diagnostic_recorders.c:12169`, `:12200` and `:17248`, and they
assert on `motion_id`:

```c
(victim_fp->status_id == nFTCommonStatusShieldBreakFly) &&
(victim_fp->motion_id == nFTCommonMotionShieldBreakFly) &&
(gNdsSCVSBattleLastFGM == (u32)nSYAudioFGMShieldBreak)
```

The port's entry point sets `motion_id` and `motion_script_id` **explicitly**
(`compat_shims.c:1637-1638`); the source version does not, and relies on
`ftMainSetStatus` to install them from the status table. Step 4 removes the
`proc_*` overrides — it must **not** remove the two `motion_id` assignments
until `ftMainSetStatus` is confirmed to set the same values from the table for
`nFTCommonStatusShieldBreakFly`. If it does, drop them and the recorders still
pass; if it does not, keeping them is the correct specialization and the reason
goes in a comment beside them.

Check that first in E1. It is a two-minute read of the status table and it is
the difference between a clean import and three failing proof recorders.

## 8. E1 — built, correct, and blocked on the pacing gate

**Status: implemented on branch `task97-shieldbreak-chain` (`8d6675f4bc`), not on
master.** Boundary fails, and the way it fails is not something I am permitted to
fix.

The hazard in §7 resolved cleanly: `ftmain.c:4603` sets `fp->motion_id` from
`status_struct[...].mflags.motion_id`, and the table's Fly entry carries
`nFTCommonMotionShieldBreakFly`, so the port's explicit assignments were
redundant and the three proof recorders keep their value. The table also exposed
a **fourth** divergence I had not found by reading the port: source specifies
`ftPhysicsApplyAirVelFriction`, the port ran `ftPhysicsApplyAirVelDriftFastFall`.

The import itself went exactly as §4 predicted — all eleven callbacks are real
code in the ELF (0x10–0x50 bytes where they were 2-byte `bx lr`), and
`ftCommonDownBounce{CheckUpOrDown,UpdateEffects}` relinked once the import gave
them a caller. One correction to §4: `src/import` is **not** auto-globbed;
`Makefile:697` uses `SOURCES` only for `VPATH`, and `CFILES` is an explicit list
the five TUs had to be added to.

### Why it is blocked

Boundary fails **one of eleven** conditions in the locked-30 pacing contract:

```
$tmPace[1] -eq (2 * $bp[4])        424  vs  2 x 211 = 422
```

A baseline run with these changes set aside **passes**, so the failure belongs to
this change. But it is a snapshot skew, not a broken invariant — the 2:1 contract
holds *exactly on both sides of it*:

| counter | reading | ratio |
|---|---|---|
| pacing tuple `bp[2]`/`bp[3]` | 422 / 211 | exactly 2:1 |
| taskman / frames | 424 / 212 | exactly 2:1 |

Both are internally consistent and one frame apart. The harness's own comment
immediately above the assertion says the pacing tuple "may trail the fresh
taskman/draw counters by one completed frame, but never by more than one" — and
then applies that tolerance to `bp[4] - bp[3]` while demanding *exact* equality
against taskman, which is the cross-counter comparison the comment is about.

### Why I did not fix it

I widened that one term to the documented one-frame bound, and the run then
failed `scripts/check-harness-registry.ps1:158`, which pins the **literal**
expression `$tmPace[1] -eq (2 * $bp[4])`. That guard exists precisely so an agent
cannot loosen this cross-check, and `TASK_STANDING_RULES.md` §"Budget ratchets vs
correctness assertions" names *a required code shape* as the kind of pin that may
not be edited to make a change pass. So I reverted the harness edit and parked
the work on a branch rather than shipping it. This is the Task 82 E1 outcome, and
the repo anticipated it well.

### What the owner has to decide

Three options, and none is mine:

1. **Widen the gate** to tolerate the one-frame skew it already documents, and
   update the registry pin with it. This is what I believe is correct, and it is
   exactly the change the registry is designed to make an agent ask about.
2. **Treat the skew as a real signal** — the possibility I cannot rule out is
   that the added code costs enough frame time to shift the sampling instant, in
   which case the gate is right and this fix needs the frame budget it does not
   have. That would make it hostage to the performance lane, which is itself
   blocked on the visual-fidelity call.
3. **Drop it.** The chain has been dead since the port began and nothing depends
   on it.

The branch preserves the whole implementation either way. Master stays
verifier-covered.

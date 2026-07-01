# Harnesses

<!-- HARNESS_INDEX_SOURCE: scripts/lib/harness-registry.ps1 -->

The authoritative harness index is `scripts/lib/harness-registry.ps1`. Do not
maintain a second hand-written list of all modes in this doc. Generate the
current rows with:

```powershell
.\scripts\verify-all.ps1 -Profile Full -List
.\scripts\verify-all.ps1 -Profile Boundary -List
.\scripts\verify-all.ps1 -Profile Latest -List
```

`scripts/check-harness-registry.ps1` validates that the registry, Makefile
mappings, mode defines, wrapper scripts, and profile plans stay in sync.

## Current Boundary

The current Boundary and Latest playable-spine pair is:

```text
battle_mariofox_stage_mplivehit_status_loop      mode 161
menu_chain_mariofox_stage_mplivehit_status_loop  mode 162
```

These modes keep the live battle scene on Pupupu/Dream Land, inherit the
current MP, cliff, passive/recover, wall/rebound, catch/throw, ledge, dash-run
damage setup, and `mpDamageRecover` proof stack, then add a bounded selected
Fox Jab2 live-hit lifecycle proof through Attack12, original event-backed
hitbox metadata, attack-state `Off -> New -> Transfer -> Interpolate`,
selected contact, repeat-hit rejection, source-shaped hit-interact record
updates, source-order hit-record detect gate, source-shaped same-group
attack-record carry/clear, attack-record refresh clearing, bounded global-hitstatus,
damage-detect, and slot-order hurtbox gates, a bounded shield-stat ->
GuardSetOff branch, a bounded source-order shield-contact gate plus
shield-health decrement, ShieldBreakFly branch and hitlag/input/transient
clear tail, normal shield-contact common tail clear including
special/rebound transients and `hitlag_mul` reset, shield-heal branch,
source-shaped catch-stat distance/search, shield-off and damage-detect-off
skips, damage scheduling, and
recovery consumption, plus the non-normal hitstatus and damage-resist
effect/SFX no-damage branches. Modes `161/162` inherit that damage-loop proof
and add bounded selected damage-status follow-through: status `17->52/45`,
hitlag `6->0`, callbacks `1/6/1`, post-hitlag original damage update ticks
`2->1`, one installed original damage physics tick `phys=11500/-1000`,
one installed original damage interrupt tick `interrupt=1`, one installed
original damage map no-collision tick `map=1/1`, one installed original damage
map floor-collision branch through passive checks into DownBounce
`floor=1/1/1/1`, one installed original damage map left/right-wall and
ceiling WallDamage short-circuit `wall=0x3ffff`, post-expiry DamageFall
map no-collision/floor fallback plus imported DownBounce handoff
`fallmap=0x7ff` plus cliff-catch branch and imported CliffCatch handoff
`cliff=0x3f`, selected air/fly expiry `57/50`,
source-shaped search mask `0xf`, and repeat suppression `1/1 gate=0x3f`. The current proof records
`mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 second=0/j14/x140
hurt=3/10 hbdmg=0->4/6 eff=0x1ff/0->0 resist=0xfff/7->3 rbreak=0x7f/2->-2/q2 throwAttr=0x1f/1->0 clash=0x3f/24/18 catchStat=0x1f/160000 catchSearch=0xffffffff/s3 shield=4->4/4 shc=0x7fffff/3142
so=155/134 contact=1 repeat=1 carry=0xf gate=0x3f rehit=5->0
origRehit=hit30/v40000 30->29->0 clear=1/3 ids=0x3 dmg=0->4 hitlag=6
status=17->52/45 hitlag=6->0 callbacks=1/6/1 update=2->1 phys=11500/-1000 interrupt=1 map=1/1 floor=1/1/1/1 wall=0x3ffff cliff=0x3f fallmap=0x7ff finish=57/50 search=0xf repeat=1/1 gate=0x3f`. Use `-DelaySeconds 3` for the current
Boundary/Latest profiles so the menu-chain finalizer markers are captured.

## Naming Rules

- Direct battle harnesses start with `battle_`.
- Menu-chain harnesses start with `menu_chain_`.
- Mario/Fox fighter proof pairs use `battle_mariofox_*` and
  `menu_chain_mariofox_*`.
- Stage/MP proof pairs include `stage_` and the narrow source boundary, such as
  `stage_mpcross_floor_loop`.
- A new direct Mario/Fox harness normally needs a matching menu-chain harness.
  Document any exception in `docs/HANDOFF.md` and `docs/KNOWN_ISSUES.md`.

## Adding A Harness Pair

For a normal direct/menu-chain pair:

1. Inspect the relevant original BattleShip source under `decomp/`.
2. Add or extend the narrow source import in `src/import`.
3. Add mode defines in `include/nds/nds_scene_harness.h`.
4. Add diagnostics in `include/nds/nds_startup.h` and `src/port/diagnostics.c`.
5. Wire harness seeding in `src/port/scene_harness.c`.
6. Wire taskman/scene finalization in `src/port/taskman_seam.c`.
7. Add Makefile `NDS_DEV_SCENE_HARNESS` mappings.
8. Add verifier wrapper scripts.
9. Add registry records and profile placement in
   `scripts/lib/harness-registry.ps1`.
10. Update `scripts/check-harness-registry.ps1` expected profile plans.
11. Update `docs/STATUS.md`, `docs/HANDOFF.md`,
    `docs/DIAGNOSTIC_REFERENCE.md`, and append `docs/PORTING.md`.
12. Run `check-harness-registry.ps1`, the new direct/menu verifiers,
    `verify-boundary.ps1`, and `verify-current.ps1`.

## Profiles

- `Latest`: runtime, Title, and the current direct/menu boundary pair.
- `BoundaryDirect`: current direct boundary only.
- `Boundary`: current direct/menu boundary pair.
- `Regression`: historical playable-spine coverage plus the current boundary.
- `Full`: all registered verifiers.

Use `docs/VERIFYING.md` for when to run each profile.

## Generator Direction

Manual copy/paste is now a risk. The next tooling improvement should be a
harness-pair generator that updates the registry, Makefile mapping, mode define,
wrapper scripts, docs index, and proof-template text in one controlled path.
Until then, registry checks are the required guardrail.

# Specials And Weapons Scout

Initial read-only scout for Mario/Fox specials, their projectile/effect
requirements, and the minimum DS memory-risk plan. No verifier, build,
emulator, or snapshot was run for the initial scout document.

## Current Checkpoint

As of 2026-07-06, default builds import the original common neutral-special
input, Mario fireball, Fox blaster, original `efmanager.c`, and Fox reflector.
`src/import/battleship_special_common.c` imports original `ftcommonspecialn.c`
and `ftcommonspecialair.c` exactly once for Mario-only, Fox-only, and combined
projectile builds. Ground B-input reaches
`ftCommonSpecialNCheckInterruptCommon` at
`decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonspecialn.c:88`,
tests B at `:93`, and dispatches the fighter neutral-special status at `:101`.
Air B-input reaches `ftCommonSpecialAirCheckInterruptCommon` at
`decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonspecialair.c:152`,
tests B at `:157`, and dispatches neutral air special at `:185`.

Mario fireball imports original `ftmariospecialn.c` and
`wpmariofireball.c`. The source path is BattleShip motion-event `flag0`
assignment in `ftmain.c:624`, accessory dispatch in `ftmain.c:1855`-`:1857`,
Mario's fireball accessory at `ftmariospecialn.c:17`, its `flag0` check at
`:23`, and `wpMarioFireballMakeWeapon` at `:53`. The weapon factory creates a
fighter-owned projectile through `wpManagerMakeWeapon` at
`wpmariofireball.c:160`-`:170`; the original hit callback returns `TRUE` at
`:126`-`:131`, so `wpprocess.c:469`-`:474` destroys it on a hit callback.
The default battle-playable proof now records:
`projectile=actor0/kind0 b=1 status=23 accessory=23 flag0=0 spawn=1 ok=1 destroy=0/0/0 weaponFrames=46 max=1 kindMask=0x1 attackMask=0xc dmg=13 life=140 map=0x401`
when the default reflector choreography drives Mario fireball into Fox shine.

Fox blaster imports original `ftfoxspecialn.c` and `wpfoxblaster.c`. The source
path is BattleShip `flag0` assignment in `ftmain.c:624`, Fox update callback
at `ftfoxspecialn.c:11`, its `flag0` check at `:16`, and
`wpFoxBlasterMakeWeapon` at `:25`. The blaster descriptor/factory are
`wpfoxblaster.c:11`-`:32` and `:106`-`:123`; weapon creation enters
`wpManagerMakeWeapon` at `wpmanager.c:87`, creates a weapon GObj at `:104`,
seeds attack state/damage/metadata at `:191`-`:236`, and installs weapon
processes at `:304`-`:306`. The earlier default blaster proof recorded
status `27`, one live blaster GObj, and 27 observed weapon frames; rerun a
blaster-only proof if a current post-bitfield damage value matters.

The default reflector proof imports `ftfoxspeciallw.c`, loads the FoxSpecial2
reflector effect through original `efManagerFoxReflectorMakeEffect`, and
records:
`reflector=0xff fox1 proj0 shine=9/14/9 reflect=23 lr=-1 clear=1688 proc=1 vx=49809->-49809 owner=1 attrs=ref1/abs1/shield1/count1/dmg13/size100000`.

Blaster glow and fireball particle scripts remain weak no-ops; the original
effect manager now loads `EFCommonEffects1/2/3`, but the common particle
script/texture banks are still non-resident. Broader projectile victim-damage,
shield, rebound, and free-flight coverage remain follow-up.

## Current Import Boundary

The port already imports the original `ftmain` and `gmcollision` TUs through
wrappers: `src/import/battleship_ftmain.c:71` includes
`decomp/BattleShip-main/decomp/src/ft/ftmain.c`, and
`src/import/battleship_gmcollision.c:114` includes
`decomp/BattleShip-main/decomp/src/gm/gmcollision.c`. The original special
status descriptor dispatch is in `decomp/BattleShip-main/decomp/src/ft/ftmain.c:78`
through `:95`, with Mario and Fox routed to `dFTMarioSpecialStatusDescs` and
`dFTFoxSpecialStatusDescs`; `ftMainSetStatus` later selects that table at
`decomp/BattleShip-main/decomp/src/ft/ftmain.c:4566`.

Current port declarations already expose the inactive special callbacks in
`include/ft/ftstatus_callbacks.h:169` through `:212`, but the implementations
are still inactive stubs in `src/import/battleship_ftstatus_inactive_stubs.c:92`
through `:132`.

## Fighter Special TUs

Mario status IDs are defined in
`decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmario.h:51` through
`:56`. Fox status IDs are defined in
`decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfox.h:74` through `:97`.

| Fighter special | Original TU | Status callbacks in descriptor table | Direct weapon/effect pull |
|---|---|---|---|
| Mario neutral special, ground/air fireball | `decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariospecialn.c` | `SpecialN` uses `ftMarioSpecialNProcUpdate`, ground friction, and `ftMarioSpecialNProcMap` at `decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariostatus.h:69`, `:83`-`:86`; `SpecialAirN` uses the same update, air drift, and `ftMarioSpecialAirNProcMap` at `:89`, `:103`-`:106`. The TU defines update/accessory/map/status setters at `ftmariospecialn.c:11`, `:17`, `:58`, `:64`, `:70`, `:82`, `:94`, `:103`, and `:111`. | Accessory spawns the fireball with `wpMarioFireballMakeWeapon` at `ftmariospecialn.c:53`; spawn world position uses `gmCollisionGetFighterPartsWorldPosition` at `:31`. |
| Mario up special, ground/air super jump punch | `decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariospecialhi.c` | `SpecialHi` uses update/interrupt/physics/map at `ftmariostatus.h:109`, `:123`-`:126`; `SpecialAirHi` uses the same callback set at `:129`, `:143`-`:146`. The TU defines update, interrupt, physics, pass, map, init, and setters at `ftmariospecialhi.c:10`, `:19`, `:66`, `:99`, `:111`, `:134`, `:142`, and `:155`. | No `wp/` projectile. It needs common fighter physics/map helpers and fall-special status; see calls at `ftmariospecialhi.c:14`, `:66`, `:111`, `:121`, and `:127`. |
| Mario down special, ground/air tornado | `decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariospeciallw.c` | `SpecialLw` uses update/physics/map at `ftmariostatus.h:149`, `:163`-`:166`; `SpecialAirLw` uses air update/physics/map at `:169`, `:183`-`:186`. The TU defines update, physics, maps, transitions, init, and setters at `ftmariospeciallw.c:11`, `:34`, `:64`, `:80`, `:94`, `:100`, `:106`, `:114`, `:125`, `:137`, `:149`, and `:167`. | No `wp/` projectile. It uses `ftParamMakeEffect(... nEFKindDustLight ...)` at `ftmariospeciallw.c:21` and `:25`, so it needs the effect shim or real effect manager. |
| Fox neutral special, ground/air blaster | `decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxspecialn.c` | `SpecialN` uses `ftFoxSpecialNProcUpdate`, `ftFoxSpecialNProcInterrupt`, ground friction, and edge map at `decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxstatus.h:109`, `:123`-`:126`; `SpecialAirN` uses the same update/interrupt, air drift, and wait/landing map at `:129`, `:143`-`:146`. The TU defines update, interrupt, init, and setters at `ftfoxspecialn.c:11`, `:34`, `:53`, `:62`, and `:70`. | Update spawns the laser with `wpFoxBlasterMakeWeapon` at `ftfoxspecialn.c:25`; it computes muzzle position through `gmCollisionGetFighterPartsWorldPosition` at `:24`. |
| Fox up special, start/hold/travel/end/bound | `decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxspecialhi.c` | Status rows are `SpecialHiStart`/`AirStart` at `ftfoxstatus.h:149`-`:186`, `SpecialHiHold`/`AirHold` at `:189`-`:226`, travel `SpecialHi`/`AirHi` at `:229`-`:266`, end `SpecialHiEnd`/`AirHiEnd` at `:269`-`:306`, and `SpecialAirHiBound` at `:309`-`:326`. The TU defines the status callbacks and transitions at `ftfoxspecialhi.c:10`, `:16`, `:22`, `:40`, `:46`, `:69`, `:86`, `:92`, `:147`, `:164`, `:178`, `:193`, `:218`, `:355`, `:364`, `:371`, `:402`, `:422`, and `:440`. | No `wp/` projectile. It is fighter physics/map heavy, with travel/bounce decisions at `ftfoxspecialhi.c:193`-`:235` and angle/velocity setup at `:291`-`:347`. |
| Fox down special, reflector | `decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxspeciallw.c` | Ground statuses `SpecialLwStart/Hit/End/Loop/Turn` are at `ftfoxstatus.h:329`-`:426`; air statuses `SpecialAirLwStart/Hit/End/Loop/Turn` are at `:429`-`:526`. The TU defines release/effect/update/physics/status callbacks at `ftfoxspeciallw.c:12`, `:21`, `:36`, `:45`, `:63`, `:81`, `:100`, `:114`, `:121`, `:148`, `:163`, `:192`, `:220`, `:227`, `:254`, `:266`, `:272`, `:278`, `:297`, and `:305`. | It creates a reflector effect with `efManagerFoxReflectorMakeEffect` at `ftfoxspeciallw.c:287`, sets the reflector collision sphere from `llFoxMainMotionLwReflectorFTSpecialColl` at `:293`, and toggles `fp->is_reflect` at `:110`, `:171`, and `:212`. |

## Weapon TUs Pulled

The shared weapon manager is not optional for either projectile special. Original
`wpManagerAllocWeapons` allocates a fixed pool of `WEAPON_ALLOC_MAX` structs at
`decomp/BattleShip-main/decomp/src/wp/wpmanager.c:30` and `:35`; the allocation
count is `32` in `decomp/BattleShip-main/decomp/src/wp/wpdef.h:5`.

Whole-TU group for a first weapon-manager slice:

| TU | Why it is pulled |
|---|---|
| `decomp/BattleShip-main/decomp/src/wp/wpmanager.c` | Allocates/free-lists weapon structs, creates weapon GObjs, attaches display/process callbacks, and projects initial map collision at `wpmanager.c:30`, `:50`, `:67`, `:87`, `:104`, `:272`, `:304`-`:306`, and `:320`-`:336`. |
| `decomp/BattleShip-main/decomp/src/wp/wpmain.c` | Common lifetime, destroy, gravity, reflector direction, staled damage, and attack-record reset helpers at `wpmain.c:60`, `:72`, `:91`, `:103`, `:112`, and `:118`. |
| `decomp/BattleShip-main/decomp/src/wp/wpmap.c` | Common projectile map tests and rebound behavior at `wpmap.c:10`, `:60`, `:98`, `:140`, `:168`, `:240`, and `:247`. |
| `decomp/BattleShip-main/decomp/src/wp/wpprocess.c` | Per-frame hitbox position/update and weapon-vs-weapon/weapon-callback processing at `wpprocess.c:13`, `:26`, `:85`, `:121`, `:372`, and `:463`. |
| `decomp/BattleShip-main/decomp/src/wp/wpdisplay.c` | Weapon draw/debug display procedures selected by the manager at `wpdisplay.c:28`, `:92`, `:147`, `:173`, `:179`, and `:191`. |

Projectile TUs:

| Projectile | Original TU | Descriptor/callbacks | Effect calls |
|---|---|---|---|
| Mario Fireball | `decomp/BattleShip-main/decomp/src/wp/wpmario/wpmariofireball.c` | Attribute table and descriptor at `wpmariofireball.c:12`, `:26`, `:52`, `:55`, and `:57`; weapon callbacks are listed at `:66`-`:73`; `wpMarioFireballMakeWeapon` calls `wpManagerMakeWeapon(... WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER)` at `:160` and `:170`. | Dust/spark/fire effects at `wpmariofireball.c:90`, `:115`, `:120`, and `:129`; hit SFX at `:128`. |
| Fox Blaster | `decomp/BattleShip-main/decomp/src/wp/wpfox/wpfoxblaster.c` | Descriptor points at `gFTDataFoxSpecial1` and `llFoxSpecial1BlasterWeaponAttributes` at `wpfoxblaster.c:11`, `:15`, and `:16`; callbacks are listed at `:25`-`:32`; `wpFoxBlasterMakeWeapon` calls `wpManagerMakeWeapon(... WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER)` at `:106` and `:109`. | Blaster glow on map end/hit/hop/spawn at `wpfoxblaster.c:61`, `:71`, `:86`, and `:121`. |

## Effect TUs Pulled

The specials only need a small set of named effect entry points, but the original
effect manager is one large TU.

| Needed entry point | Source and asset pull |
|---|---|
| Particle initialization and bank setup | `efParticleInitAll` allocates particle structs/generators/transforms at `decomp/BattleShip-main/decomp/src/ef/efparticle.c:28`-`:33`; `efParticleGetLoadBankID` registers script/texture banks at `:77`-`:108`. |
| Common effect manager initialization | `efManagerInitEffects` loads `EFCommonEffects1`, `EFCommonEffects2`, and `EFCommonEffects3` into taskman heap at `decomp/BattleShip-main/decomp/src/ef/efmanager.c:1734` and `:1754`-`:1756`. |
| Effect GObj creation | `efManagerMakeEffect` makes effect GObjs on `nGCCommonLinkIDEffect` or `nGCCommonLinkIDSpecialEffect` at `efmanager.c:1929` and `:1959`, adds display at `:1975`, and attaches update process at `:1920` or `:1923`. `efdisplay.c` also installs shared effect display GObjs at `decomp/BattleShip-main/decomp/src/ef/efdisplay.c:26`-`:34` and `:84`-`:97`. |
| Mario fireball/tornado particles | Dust light/expand, sparkle white, and fire grind are declared in `decomp/BattleShip-main/decomp/src/ef/efmanager.h:38`, `:43`, `:62`, and `:127`; implementations use `lbParticleMakeScriptID` at `efmanager.c:2842`, `:2868`, `:3079`, `:3105`, `:3605`, `:3607`, `:5287`, and `:5289`. |
| Fox blaster glow | Declared at `efmanager.h:136`; implemented with `lbParticleMakeCommon` at `efmanager.c:5517` and `:5521`. |
| Fox reflector model effect | Reflector anim offsets use `llFoxSpecial2ReflectorStart/Loop/Hit/EndAnimJoint` at `efmanager.c:411`-`:416`; descriptor uses `llFoxSpecial2ReflectorDObjDesc` and start anim at `:420`, `:443`, and `:445`; creation is `efManagerFoxReflectorMakeEffect` at `:4073`-`:4075`. |
| Fire spark common effect | `EFCommonEffects2` provides fire spark DObj/MObj/anim data at `efmanager.c:404`-`:407`, and `efManagerFireSparkMakeEffect` uses it at `:4007` and `:4023`. This matters if fireball hit visuals graduate past particle-only stubs. |

`efground.c` is not directly pulled by Mario/Fox specials in this slice; it is
stage-effect infrastructure. `efdisplay.c` becomes relevant if the effect
manager is imported as a live visual system rather than left behind the current
no-op/diagnostic particle shims.

## Assets

The Mario/Fox special payload files are already present in BattleShip_o2r under
`reloc_fighters_main` and already staged into NitroFS by the current build map:
`Makefile:577`-`:579` lists `MarioSpecial1/2/3`, `Makefile:584`-`:587`
lists `FoxSpecial1/2/3/4`, and `src/nds/nds_reloc_assets.c:77`-`:87` maps
their NitroFS paths. The port has reloc IDs for these files in
`include/reloc_data.h:44`, `:46`, `:48`, `:49`, `:52`, `:55`, and `:56`, with
backend token handling at `src/port/reloc_backend_assets.c:916`-`:924`,
`:940`-`:952`, and `:1234`-`:1246`.

Filesystem-measured BattleShip_o2r payload sizes:

| Asset file | Bytes | Used by | Present/staged status |
|---|---:|---|---|
| `decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/MarioSpecial1` | 148 | Mario fireball attributes via `llMarioSpecial1FireballWeaponAttributes` at `wpmariofireball.c:26`. | Present and staged. |
| `decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/MarioSpecial2` | 1,936 | Mario entry dokan effect references in `efmanager.c:1548`-`:1551`; not required for Mario specials in this slice. | Present and staged. |
| `decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/MarioSpecial3` | 736 | Staged fighter special payload; no direct Mario/Fox special path found in this scout. | Present and staged. |
| `decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/FoxSpecial1` | 146 | Fox blaster attributes via `llFoxSpecial1BlasterWeaponAttributes` at `wpfoxblaster.c:15`-`:16`. | Present and staged. |
| `decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/FoxSpecial2` | 3,712 | Fox reflector DObj/animations at `efmanager.c:411`-`:416`, `:443`, `:445`, and `:4035`; also entry Arwing anim references at `efmanager.c:5734`-`:5736`. | Present and staged. |
| `decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/FoxSpecial3` | 12,246 | Fox entry Arwing model/effect references at `efmanager.c:1559`, `:1578`, and `:5730`; not required for blaster/reflector itself. | Present and staged. |
| `decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/FoxSpecial4` | 224 | Staged fighter special payload; no direct Mario/Fox special path found in this scout. | Present and staged. |
| `decomp/BattleShip-main/BattleShip_o2r/reloc_effects/EFCommonEffects1` | 52,816 | Loaded unconditionally by original effect init at `efmanager.c:1754`; provides common effect descriptors such as damage slash/spark/orbs at `efmanager.c:104`-`:107`, `:224`-`:227`, and `:254`-`:257`. | Present and staged in NitroFS/reloc map. |
| `decomp/BattleShip-main/BattleShip_o2r/reloc_effects/EFCommonEffects2` | 28,432 | Loaded at `efmanager.c:1755`; provides fire spark and reflector-break/catch-style common data at `efmanager.c:404`-`:407`, `:543`-`:546`, and `:573`-`:576`. | Present and staged in NitroFS/reloc map. |
| `decomp/BattleShip-main/BattleShip_o2r/reloc_effects/EFCommonEffects3` | 13,696 | Loaded at `efmanager.c:1756`; provides MBall/rebirth/item-get common effects at `efmanager.c:1275`-`:1278`, `:1671`-`:1674`, and `:1701`-`:1704`. | Present and staged in NitroFS/reloc map. |
| `decomp/BattleShip-main/BattleShip_o2r/particles/efcommon_particle_scb` | 10,980 | Common particle scripts registered by `efParticleGetLoadBankID` at `efparticle.c:77`-`:108`. | Present in BattleShip_o2r; current port only has placeholder particle ROM symbols at `src/port/reloc_backend_ftdata_stubs.c:8`-`:19`. |
| `decomp/BattleShip-main/BattleShip_o2r/particles/efcommon_particle_txb` | 315,108 | Common particle textures registered by `efParticleGetLoadBankID` at `efparticle.c:77`-`:108`. | Present in BattleShip_o2r; current port only has placeholder particle ROM symbols at `src/port/reloc_backend_ftdata_stubs.c:8`-`:19`. |

## Weapon GObj Assumptions

`wpManagerMakeWeapon` assumes weapons are normal GObjs linked through the common
weapon list. It creates `nGCCommonKindWeapon` on `nGCCommonLinkIDWeapon` at
`decomp/BattleShip-main/decomp/src/wp/wpmanager.c:104`. It derives owner/team
state from the parent flag: fighter parent at `:118`, weapon parent at `:136`,
item parent at `:154`, and ground/default parent at `:173`.

Every live weapon gets:

- Display procedure selected from the descriptor/attributes, then installed on
  display link `14` at `wpmanager.c:264`, `:270`, and `:272`.
- Three processes: `wpProcessProcWeaponMain` priority 3, `wpProcessProcSearchHitWeapon`
  priority 1, and `wpProcessProcHitCollisions` priority 0 at `wpmanager.c:304`
  through `:306`.
- Optional initial map projection when `WEAPON_FLAG_COLLPROJECT` is set at
  `wpmanager.c:320`; fighter-owned projectiles project from the fighter collision
  state at `:328`-`:329`.

The shared weapon list is part of the logic contract, not just storage:
`wpprocess.c:309` and `:386` iterate `gGCCommonLinks[nGCCommonLinkIDWeapon]`
for group hit records and weapon-vs-weapon checks. The current port's
compatibility `WPStruct` shape is narrower in `include/wp/weapon.h:55`-`:87`,
but the original struct carries `MPCollData`, `WPAttackColl`, callbacks, status
vars, and display mode in `decomp/BattleShip-main/decomp/src/wp/wptypes.h:115`
through `:198`.

## Collision Integration

Fighter damage and attack collider storage is already exposed in the port:
`include/ft/fighter.h:303`-`:348` defines `FTAttackColl`/`FTDamageColl`, and
`include/ft/fighter.h:3056`, `:3063`, and `:3109` store fighter attack, damage,
and special collision pointers. Reflector state is `is_reflect` and `reflect_lr`
at `include/ft/fighter.h:2988`-`:2989`; original `FTSpecialColl` is defined at
`decomp/BattleShip-main/decomp/src/ft/fttypes.h:21`-`:27`.

Weapon attack collider storage is `WPAttackColl` in `include/wp/weapon.h:21`
through `:52` and original `decomp/BattleShip-main/decomp/src/wp/wptypes.h:80`
through `:111`.

Runtime flow:

1. Fighter GObjs install `ftMainProcSearchHitAll` as a process in the original
   fighter manager at `decomp/BattleShip-main/decomp/src/ft/ftmanager.c:862`.
2. The imported original `ftMainProcSearchHitAll` dispatches fighter, item,
   weapon, and ground-hit searches from `decomp/BattleShip-main/decomp/src/ft/ftmain.c:3795`
   and `:3803`-`:3806`.
3. Weapon-vs-fighter attack collision first checks fighter attack hitboxes with
   `gmCollisionCheckWeaponAttackFighterAttackCollide` at
   `decomp/BattleShip-main/decomp/src/ft/ftmain.c:3316`, then range at `:3333`,
   reflector/special collisions at `:3342`-`:3348`, absorb/special collisions
   at `:3356`-`:3364`, shield at `:3380`, and fighter damage colliders with
   `gmCollisionCheckWeaponAttackFighterDamageCollide` at `:3396`-`:3404`.
4. The imported original collision helpers are defined in
   `decomp/BattleShip-main/decomp/src/gm/gmcollision.c`: range at `:1153`,
   weapon-vs-fighter attack at `:1452`, weapon-vs-fighter damage at `:1498`,
   shield at `:1523`, special reflector/absorb at `:1560`, weapon-vs-weapon at
   `:1585`, and weapon attack-position extraction at `:2042`.
5. Weapon-vs-weapon and post-hit callbacks are owned by weapon processes:
   `wpProcessProcSearchHitWeapon` at `decomp/BattleShip-main/decomp/src/wp/wpprocess.c:372`
   scans other weapons through `gGCCommonLinks[nGCCommonLinkIDWeapon]` at `:386`,
   and `wpProcessProcHitCollisions` starts at `:463`.

For Fox reflector specifically, `ftMain` already knows how to resolve a
successful reflector special collision: it sets `reflect_lr` near
`decomp/BattleShip-main/decomp/src/ft/ftmain.c:2300` and `:2318`, tests
`nFTSpecialCollKindFoxReflector` at `:3973`, and later clears `reflect_lr` at
`:4010`. The currently stubbed `ftFoxSpecialLwHitSetStatus` in
`src/port/reloc_backend_compat_shims.c:1490`-`:1498` is only the narrow status
side of that wider path.

## Current Port Stubs To Retire

Delete or supersede these only when their original TU group is live:

- Special callback inactive stubs:
  `src/import/battleship_ftstatus_inactive_stubs.c:92`-`:104` for Mario and
  `:105`-`:132` for Fox.
- Weapon/particle scene-manager shims:
  `wpManagerAllocWeapons` in `src/port/reloc_backend_compat_shims.c:13063`-`:13067`,
  and `efParticleInitAll` in `:11738`-`:11741`.
- Current partial weapon common shims:
  `wpMainGetStaledDamage` in `src/port/reloc_backend_compat_shims.c:5221`-`:5228`
  and `wpProcessUpdateHitInteractStats` in `:5301`-`:5334`, once `wpmain.c`
  and `wpprocess.c` are imported.
- Particle no-op shims:
  `ftParamMakeEffect` in `src/port/reloc_backend_compat_shims.c:7096`-`:7122`,
  particle helpers in `:11858`-`:11929`, and weak battle-playable particle
  stubs in `src/port/battle_playable_compat_stubs.c:215`-`:229`.
- Special input interrupt shims:
  `ftCommonSpecialHiCheckInterruptCommon` in
  `src/port/reloc_backend_compat_shims.c:1871`-`:1875`. This is not a weapon
  manager blocker, but it should not survive the up-special slice.

## Resident Bytes Estimate

Current mode `163` reports 240,332 bytes of headroom after the effect-manager
and reflector defaults, with resident reloc bytes `747472`. The fixed reserve
remains 128 KiB.

| Component | Bytes | Headroom impact |
|---|---:|---|
| Mario/Fox special payloads already staged (`MarioSpecial1/2/3` + `FoxSpecial1/2/3/4`) | 19,148 | If current fighter loading already keeps these resident, enabling callbacks adds 0 new file bytes. If a future scene trims them out, restoring all seven costs 18.7 KiB. |
| Weapon struct pool | unknown exact; source allocates `sizeof(WPStruct) * 32` | Source-backed bound is `wpmanager.c:35` plus `wpdef.h:5`. Budget 32-48 KiB until measured because original `WPStruct` includes `MPCollData`, `WPAttackColl`, callbacks, status vars, SFX, and display mode at `wptypes.h:115`-`:198`. |
| Full original `EFCommonEffects1/2/3` heap load | 94,944 | Now resident by default; loaded unconditionally by original `efManagerInitEffects` at `efmanager.c:1754`-`:1756`. |
| Common particle scripts/textures (`efcommon_particle_scb` + `efcommon_particle_txb`) | 326,088 | Still does not fit inside the current reserve plan without streaming or another memory decision. |
| Full common effects plus common particles | 421,032 | Common effects are live; particles remain the hard memory gate for a naive visual-effects import. |

Practical conclusion: the effect manager is live, but a full `efparticle` +
common particle texture residency is still over budget before gameplay value
appears. The first particle slice must either stream/defer the 315 KiB TXB,
trim to a DS-native effect replacement for these specific particles, or move
the common particle bank into the optional extra 4 MiB DSi/debug-cart profile.

## Proposed /task Slice Sequence

1. `/task Import weapon manager core for empty live battle`

   Import `wpmanager.c`, `wpmain.c`, `wpmap.c`, `wpprocess.c`, and
   `wpdisplay.c` as one coherent `wp/` TU group. Wire `wpManagerAllocWeapons`
   from VSBattle (`src/import/battleship_scvsbattle.c:57`, original calls at
   `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:163` and `:435`).
   Gate: no projectiles spawned yet, battle still reaches current boundary, and
   weapon list/processes are alive without allocating effects.

2. `/task Mario fireball runtime slice`

   Import `ftmariospecialn.c` and `wpmariofireball.c`; activate only Mario
   neutral-special status callbacks and delete the matching inactive stubs.
   Keep dust/spark/fire visuals routed through the current effect stubs for this
   slice. Gate: Mario can enter ground/air neutral special, spawn a fireball
   GObj, run map rebound/lifetime, and hit Fox damage colliders through imported
   `ftmain`/`gmcollision`.

3. `/task Fox blaster runtime slice`

   Import `ftfoxspecialn.c` and `wpfoxblaster.c`; activate Fox neutral-special
   status callbacks and delete matching inactive stubs. Keep blaster glow behind
   effect stubs. Gate: Fox can enter ground/air blaster, spawn laser GObjs, and
   exercise weapon hit, shield, setoff, and reflect-capable metadata without
   requiring full particles.

4. `/task Minimal effect manager gate`

   Landed: the DS taskman arena was grown to `0x150000`, `efmanager.c` imports
   whole, and `EFCommonEffects1/2/3` load by default. Common particles remain
   non-resident.

5. `/task Fox reflector and remaining non-projectile specials`

   Landed for Fox reflector: `ftfoxspeciallw.c` imports whole, mode `163`
   proves a live Mario fireball reflection, and the old
   `ftFoxSpecialLwHitSetStatus` seam is deleted. Remaining non-projectile
   specials are still future small groups: `ftmariospecialhi.c`,
   `ftmariospeciallw.c`, and `ftfoxspecialhi.c`.

Manager-first beats per-special-first here. Mario fireball and Fox blaster both
need the same weapon manager, GObj link, process, map, and hit-collision
machinery; doing one special first would leave a temporary projectile runtime
that has to be replaced immediately.

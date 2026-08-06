# Samus Charge Shot Hit Detection — Investigation Notes (2026-04-13) — OPEN

**Status:** Root cause identified, fix attempted, **not yet working visually**.  The code changes that made the runtime trace look correct did not produce the expected on-screen behaviour when the user tested the build (no projectile render, no DK hit, no knockback).  This doc captures what we know so future sessions don't re-derive it.

## Symptom

In the DK-vs-Samus Kongo Jungle intro (scene 45, `nSCKindOpeningJungle`), Samus fires her charged plasma shot and the projectile passes **through** DK with no collision.  DK's melee attacks in the same scene hit Samus normally, so fighter-vs-fighter melee collision works — the bug is isolated to projectile-vs-fighter hit detection.

A separate visual clue matters: in the original N64, Samus's arm cannon flashes at the moment of firing as if she had a fully-charged beam.  In our port, the flash animation doesn't happen — Samus just fires straight out.  **This is the canary that points at the root cause:** the intro cinematic bypasses the normal charge-up flow entirely, and the bypass path is where the hit state gets mis-initialized.

## The intro cinematic bypass (this is the key)

`mvOpeningJungleMakeFighters` at `src/mv/mvopening/mvopeningjungle.c:343` presets Samus's charge level directly:

```c
fp->passive_vars.samus.charge_level = 6;
```

Then it installs `dMVOpeningJungleSamusKeyEvents` as Samus's key-event script.  That script taps B twice with only one frame of release between them:

```c
FTKEY_EVENT_BUTTON(B_BUTTON, 1),   // first B
FTKEY_EVENT_BUTTON(0, 1),           // 1-frame release
FTKEY_EVENT_BUTTON(B_BUTTON, 1),   // second B
```

The first B press enters `nFTSamusStatusSpecialNStart`.  Before the Start animation finishes and transitions to the charging Loop (`ftSamusSpecialNLoopSetStatus` at `ftsamusspecialn.c:190`, which is the function that actually spawns a charge-shot weapon via `wpSamusChargeShotMakeWeapon(is_release=FALSE)`), the second B tap arrives.  `ftSamusSpecialNStartProcInterrupt` catches it and sets `fp->status_vars.samus.specialn.is_release = TRUE`.  When the Start animation ends, `ftSamusSpecialNStartProcUpdate` checks `is_release` and transitions **directly** to `ftSamusSpecialNEndSetStatus` — skipping the Loop entirely.

So by the time `ftSamusSpecialNEndProcUpdate` at `ftsamusspecialn.c:205` runs, `fp->status_vars.samus.specialn.charge_gobj` is still `NULL` (no weapon was ever spawned for the charging phase) and control falls into the `else` branch at `ftsamusspecialn.c:234`:

```c
else wpSamusChargeShotMakeWeapon(fighter_gobj, &pos, fp->passive_vars.samus.charge_level, TRUE);
```

**That `TRUE` is the bypass.**  It jumps straight into the is_release=TRUE path of `wpSamusChargeShotMakeWeapon` at `wpsamuschargeshot.c:309` without ever going through the normal MakeWeapon(is_release=FALSE) → charging frames → proc_update transition flow.

## Why the bypass breaks hit detection

The normal charging-then-firing path spends many frames with `wpProcessProcWeaponMain` running every frame.  During charging, `attack_state = nGMAttackStateOff` (explicitly set by the is_release=FALSE branch at `wpsamuschargeshot.c:330`) and the hit-detection loop in `ftMainSearchHitWeapon` is gated behind `attack_state != nGMAttackStateOff`, so no hits can possibly register.  When the fire transition happens inside `wpSamusChargeShotProcUpdate` at `wpsamuschargeshot.c:210`, `Launch` runs and `attack_state` moves to `nGMAttackStateNew`, then `wpProcessUpdateHitPositions` is called **from inside proc_update** and again at `wpprocess.c:197` after the velocity step — two calls in the same frame that walk state through `New → Transfer → Interpolate` and populate `attack_pos[0].pos_curr` and `attack_pos[0].pos_prev` with valid values.

The bypass path skips all of that.  `wpSamusChargeShotMakeWeapon(is_release=TRUE)` calls `wpManagerMakeWeapon` (which sets `attack_state = nGMAttackStateNew` at `wpmanager.c:199` but never touches `attack_pos[0]`), then calls `wpSamusChargeShotLaunch` (which sets velocity / damage / size / fgm / priority but does not touch `attack_pos[0]` or `attack_count` either), then returns.  No `wpProcessUpdateHitPositions` has run yet.

`WPStruct` objects come out of a fixed pool (`sWPManagerStructsAllocFree` in `wpmanager.c:19`) that's allocated once via `syTaskmanMalloc` with no explicit zeroing of the attack collider fields after the first use.  The trace from a build with `attack_count` forced to `1` in Launch captured the hit state right before `Launch` ran:

```
SSB64: CS MakeWeapon: gobj=0x1027b84e0 charge_level=6 is_release=1 pos=(-846.3,202.3,72.7) state=1 cnt=0
SSB64: CS Launch ENTER: gobj=0x1027b84e0 state=1 cnt=0 translate=(-846.3,202.3,72.7)
  pos_curr=(0.0,0.0,0.0) pos_prev=(nan,0.0,0.0) charge_size=6
```

**`pos_prev.x = NaN` from stale pool memory.**  With `attack_count = 0` from the file, the hit-detection loop iterates zero times and the NaN is never read — the charge shot renders and flies through DK because no range check ever runs.  With `attack_count = 1`, the loop runs one iteration at `attack_state = nGMAttackStateNew`, and `gmCollisionTestRectangle` takes the swept-test branch (because `opkind != 2`) which reads `pos_prev` into `distx/disty/distz`:

```c
distx = sp6C.x - sp78.x;  // sp6C = pos_prev (NaN), sp78 = pos_curr
```

IEEE NaN propagates through the Cohen-Sutherland line-clip loop.  NaN comparisons always return false, but the clip logic bounces between the "outside" branches and the function can return TRUE for arbitrary joints.  The weapon is "hit" on frame 0, damage stats are applied, and the weapon gobj is torn down before the display pass — hence **no projectile renders, no DK hit, nothing visible**.

## The file's `attack_count = 0` is intentional, not a bug

With the padded-layout fix applied (`u16 _pad_before_combat_bits;` in `wptypes.h`, mirroring the ITAttributes workaround at `ittypes.h:243`), the bitfield read of `attr->attack_count` from the SamusSpecial1 ROM file (`file_id = 218`, offset 0, bitfield word 2 `= 0x0160013c` LE) decodes as `0` in both SGI BE MSB-first and clang LE LSB-first packing.  Verified against the ROM bytes directly via `python3 debug_tools/reloc_extract/reloc_extract.py extract baserom.us.z64 218`.  Other weapons decode correctly:

| Weapon | `attack_count` from file |
|---|---|
| Fox Blaster | 1 |
| Samus Bomb | 2 |
| Mario Fireball | 2 |
| Samus Charge Shot | **0** |

`0` for the charge shot is actually sensible given the design — the file's value is the **charging-phase default**, during which the hitbox must be inactive.  In the normal path this is orthogonal to the problem because `attack_state = nGMAttackStateOff` is the real gate, but in the bypass path no code ever bumps `attack_count` to `1` to actually enable hit detection once the shot is fired.

## What I tried (and why it didn't work)

### Attempt 1: Force `wp->attack_coll.attack_count = 1;` in `wpSamusChargeShotLaunch`

Makes the runtime decoded `cnt` field non-zero, so the hit-detection loop actually iterates.  **Broke rendering entirely** — with `attack_count = 1` and `attack_state = nGMAttackStateNew`, the very first hit check on frame 0 runs the swept test against the NaN `pos_prev`, produces a spurious hit against DK, `ftMainUpdateDamageStatWeapon` fires, and the weapon is destroyed before its display process runs.  User-visible result: charge shot never appears on screen, DK never reacts.

### Attempt 2: Same as Attempt 1 plus explicit `pos_curr` and `pos_prev` initialization

```c
wp->attack_coll.attack_count = 1;
wp->attack_coll.attack_pos[0].pos_curr = *translate;
wp->attack_coll.attack_pos[0].pos_prev = *translate;
```

The trace confirms both positions are now finite sane values at the Launch exit:

```
SSB64: CS Launch EXIT:  ... cnt=1 ... pos_curr=(-846.3,202.3,72.7) pos_prev=(-846.3,202.3,72.7)
```

**Still breaks visually** — per the user's direct observation on this build, the charge shot still does not render, DK still doesn't get hit, and nothing flies off the stage.  The traces say the state looks correct, but the on-screen behaviour disagrees.  I do not yet understand the disconnect.

## Things that still need investigating

1. **Verify whether `wpProcessProcWeaponMain` actually runs for the new weapon on the same frame it's spawned, or whether it's deferred to the next frame.**  Processes are added mid-frame inside a priority-5 fighter process.  If GObj iteration at priorities 4/3/2/1 picks up the new weapon, its `proc_update → proc_map → wpProcessUpdateHitPositions` chain runs before fighter hit detection at priority 1.  If it doesn't pick up the new weapon until next frame, then `attack_pos[0]` and `attack_state` stay at whatever values Launch left them with when fighter hit detection sees the weapon — meaning the explicit init in Launch is the only thing that can save us.
2. **Verify whether the display process for the new weapon runs on the spawn frame.**  If the game logic pass destroys the weapon (via hit → `ftMainUpdateDamageStatWeapon` → `wpMainDestroyWeapon`, or via a spurious proc_map collision), the render pass won't see the DObj and nothing will draw.  Needs tracing inside `wpMainDestroyWeapon` and the display proc to see when / if the weapon disappears from the GObj list.
3. **Verify `wpSamusChargeShotProcMap` doesn't mis-fire on frame 0.**  `wpMapTestAllCheckCollEnd` walks through `mpProcessUpdateMain` and tests wall/floor/ceiling collisions against the weapon's `coll_data`.  The `coll_data` is populated via `mpCommonRunWeaponCollisionDefault` at `wpmanager.c:423` during MakeWeapon.  If the fresh coll_data has stale values or the line-id fields produce a false positive on the first check, proc_map returns TRUE and destroys the weapon.  (The function itself is plain C with no LP64 hazards, but `mpProcessCheckTestLWallCollisionAdjNew` and friends are several layers deep and have not been audited.)
4. **Check if the display process needs an animation tick before the DObj is drawable.**  `gcAddGObjDisplay` at `wpmanager.c:366` adds `proc_display` at priority 14.  On frame 0 the weapon's animation / matrix may not be set up yet, and the display call could hit an uninitialised DObj path.  This would explain "no projectile renders" independent of any hit detection.
5. **Re-derive whether the SGI IDO BE compile produces the same `attack_count` layout we think it does.**  If IDO packs the word-2 bitfields differently from clang, our decoded value of `0` may be wrong — maybe in the real N64 file `attack_count` really is `1`, we're just reading the wrong bits.  (Low confidence — the decoded values for other weapons are sensible — but not ruled out.)

## Hint from the user that cracked it open

> Samus's arm cannon doesn't flash in the intro as if she had a full charge, she just shoots the fully charged beam.  Another theory is that something in the opening room cinematic bypasses the charge state to give her a full beam when she hasn't charged it, and in our code that's why the hitbox doesn't connect.

This is exactly right: the intro sets `passive_vars.samus.charge_level = 6` directly and drives Samus's button inputs through a scripted key-event sequence that fires B twice back-to-back, which short-circuits the charging Loop and takes the `is_release=TRUE` bypass in `wpSamusChargeShotMakeWeapon`.  The visual absence of the charge-flash animation is the tell that you're on this code path and not the normal one.  If you see this symptom class on any other weapon in the port, grep for `FTKEY_EVENT_BUTTON(B_BUTTON, 1)` in the relevant scene's key-event table.

## File references

- `src/mv/mvopening/mvopeningjungle.c:343` — intro presets `charge_level = 6`
- `src/mv/mvopening/mvopeningjungle.c:72`  — `dMVOpeningJungleSamusKeyEvents` with the back-to-back B taps
- `src/ft/ftchar/ftsamus/ftsamusspecialn.c:54`  — `ftSamusSpecialNStartProcUpdate` (transition from Start → Loop or End)
- `src/ft/ftchar/ftsamus/ftsamusspecialn.c:73`  — `ftSamusSpecialNStartProcInterrupt` (second B press sets `is_release = TRUE`)
- `src/ft/ftchar/ftsamus/ftsamusspecialn.c:205` — `ftSamusSpecialNEndProcUpdate` with the `charge_gobj == NULL` bypass branch at line 234
- `src/wp/wpsamus/wpsamuschargeshot.c:152` — `wpSamusChargeShotLaunch` (where the fix attempts live)
- `src/wp/wpsamus/wpsamuschargeshot.c:309` — `wpSamusChargeShotMakeWeapon` (is_release=TRUE path at line 324)
- `src/wp/wpmanager.c:199` — `wp->attack_coll.attack_state = nGMAttackStateNew` set unconditionally at spawn
- `src/wp/wpmanager.c:250` — `wp->attack_coll.attack_count = attr->attack_count;` (loads `0` from file for charge shot)
- `src/wp/wpprocess.c:26`  — `wpProcessUpdateHitPositions` (only updates `attack_pos[0]` if `attack_count > 0`; state-machine `New → Transfer → Interpolate`)
- `src/ft/ftmain.c:3354` — `ftMainSearchHitWeapon` outer gate: `attack_state != nGMAttackStateOff`
- `src/ft/ftmain.c:3435` — hit-detection loop gated on `attack_count`
- `src/gm/gmcollision.c:661` — `gmCollisionTestRectangle` with the `opkind == 2` (point test) vs swept-test branches
- `src/wp/wptypes.h` — current attempt at padded WPAttributes layout (`u16 _pad_before_combat_bits;` under `#ifdef PORT`)

## Related: the padding fix in `wptypes.h` is independently correct

Even though the overall bug is still open, the `u16 _pad_before_combat_bits` field added to `WPAttributes` under `#ifdef PORT` is independently justified: clang's default bitfield packing places `element`/`damage` at byte `0x26` in the struct, splitting the bitfield word across the `portFixupStructU16` boundary at `0x28`.  The padding forces clang to align the u32 storage units to byte `0x28`, matching the SGI IDO BE compile and giving the correct decoded values for Fox Blaster (`cnt=1`), Samus Bomb (`cnt=2`), and Mario Fireball (`cnt=2`).  Without the padding the decoded values are garbage on all weapons.  Keep this fix even if the charge-shot-specific code changes get reverted.

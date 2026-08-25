# Donkey Kong — P2-3 fighter 2 (heavy grappler archetype)

Status: **state machine landed and selectable; cargo matrix verified
2026-08-25; budgets/CSS/owner-feel still open** ·
Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/`

## Role

First structurally new archetype: super-heavyweight, huge hurtbox, no
projectile, and the game's only carry-grab. Proves the pipeline on a fighter
that shares almost nothing with Mario/Fox beyond `ftcommon`.

## Moveset uniques

- **Cargo carry**: grab leads to a carry state — DK walks/jumps while holding
  the victim, victim mashes out, and **one** cargo release (`ThrowFF` on the
  ground, `ThrowAirFF` in the air, with an air back-turn variant), not four.
  A whole extra state machine on both fighters; the hardest single item in the
  roster schedule. Get its ownership right at the shared grab seam
  (`ftcommon`), not as DK-local hacks.
- **Giant Punch (B)**: chargeable in steps, charge is storable across states,
  fully-charged properties from source. **No release armor** — the source
  `ftdonkeyspecialn.c` never touches `knockback_resist_*`; the only passive
  resist in `ftManagerInitFighter` belongs to Giant DK (48.0) and Metal Mario
  (30.0). Verified 2026-08-25.
- **Spinning Kong (Up-B)**: long horizontal recovery, multi-hit, low vertical.
- **Hand Slap (Down-B)**: ground-only quake, hits grounded opponents only,
  repeatable rhythm.
- Heaviest class: knockback resistance, big ledge-grab reach, slow jumps.

## Assets & audio

Big model — watch the polygon/texture budget (largest fighter silhouette);
bongo/jungle voice set, announcer clip, 4 costumes.

## DS notes / risks

- Carry state must interact correctly with platforms, edges (walking off
  while carrying), KO boundaries (both fighters), throws near blast zones,
  and Sudden Death — enumerated and answered in the cargo matrix below.
- Giant Punch charge persistence across knockdowns/KOs per source — answered
  in the cargo matrix below.
- Large model = matrix/draw cost outlier candidate; measure vs the
  per-fighter draw budget early, LOD if needed.

## Move inventory (swept 2026-08-24 against the source)

The whole DK state machine is present, cargo included, and the unit doc's old
"not started" was wrong. `src/import/battleship_donkey.c` includes all eleven
behavior TUs verbatim -- `ftdonkeyspecialn/hi/lw` plus all **eight**
`ftdonkeythrowf*` cargo TUs (the earlier "nine" was a miscount; the directory
holds eight) -- and `ftmain.c:64` pulls `ftdonkeystatus.h`, whose
descriptor table carries all 30 statuses the `ftdonkey.h` enum declares:

    AppearR/L, SpecialN{,Air}{Start,Loop,End,Full}, Special{,Air}Hi,
    SpecialLw{Start,Loop,End}, ThrowFWait, ThrowFWalk{Slow,Middle,Fast},
    ThrowFTurn, ThrowFKneeBend, ThrowFFall, ThrowFLanding, ThrowFDamage,
    ThrowFF, ThrowAirFF, HeavyThrow{F,B,F4,B4}

So the cargo ladder is not implementation work; it is UNVERIFIED work. The
bounded proof that closed P2-3r3 exercised Giant Punch, Spinning Kong, Hand
Slap and a driven KO -- it does not touch grab, carry, walk-while-carrying, the
cargo throw, or mash-out. That is what the cargo matrix below owed.

## Cargo matrix (verified 2026-08-25 -- board row P2-3r10)

**Two premises this doc previously carried were wrong; correcting them first
changes what the matrix has to prove.**

1. **`HeavyThrow{F,B,F4,B4}` are not cargo throws.** They are DK's heavy-ITEM
   throws. `ftCommonHeavyThrowDecideSetStatus` (`ftcommonitemthrow.c:208`)
   picks a common `nFTCommonStatusHeavyThrow*` and then adds
   `nFTCommonStatusHeavyThrow4Start` *only for Donkey Kong*, which is why the
   four appear in `dFTDonkeySpecialStatusDescs`; their only entry gate is
   `ftCommonHeavyThrowCheckInterruptCommon`, whose first test is
   `fp->item_gobj != NULL`. Items are P2-5, so with items off the four are
   unreachable and `src/import/battleship_ftcommon_itemthrow.c`'s
   deliberately-false interrupt is correct. **DK carrying a heavy crate reuses
   the same `ThrowFWait` carry ladder as cargo** -- that is the connection the
   status table encodes, and it is what P2-5 will have to re-check.
2. **The fighter-cargo release is one status pair, not four:** `ThrowFF`
   (ground) / `ThrowAirFF` (air), plus the air back-turn variant that
   `ftDonkeyThrowFFCheckInterruptThrowFCommon` selects when the stick opposes
   `lr` and `ga == nMPKineticsAir` (`ftdonkeythrowff.c:114-129`).

**Method, because "present" is not "verified".** Every function in the ladder
is BattleShip's own body compiled verbatim, so the matrix's job is not to
re-read the source -- it is to prove that the **linked binary reaches that
body** at every hop, since the port's seams are `#define` redirects and weak
stubs that can silently displace one. Rows therefore cite the source owner and
the check on the shipped image. Disassembly and `nm` are against
`builds/build-p2-fourcpu-roster4/smash64ds-p2-fourcpu-tickhud-hwtri.elf`, the
shipping-shaped four-distinct-kind build P2-3r13 accepted (`NDS_P2_DONKEY 1`,
`NDS_R2_BATTLEPACK 1`).

**The status table is the source table, and that had to be checked rather than
assumed**, because `include/ft/ftchar/ftdonkey/ftdonkeystatus.h` shadows the
decomp header on the include path. It is a forwarding shim: under
`NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_P2_DONKEY` it `#include`s the decomp
file, and otherwise supplies a 16-entry `NDS_FT_STATUS_STUB16` table. The
linked image settles it — `nm -S` reports `dFTDonkeySpecialStatusDescs` at
**0x258 = 600 B = 30 entries** of the 20-byte `FTStatusDesc` (Mario's is 0xb4 =
9 entries), i.e. the full source table, not the stub.

| Row | Source owner | Verified by |
|---|---|---|
| grab → `Catch`/`CatchPull`/`CatchWait` | `ftcommoncatch1.c`, `ftcommoncatch2.c` | imported by `src/import/battleship_ftcommon_catch.c`; the public status callbacks are aliases onto those bodies (`battleship_ftstatus_callback_aliases.c`), and `ndsBaseFTCommonCatchSetStatus` / `...CatchWaitSetStatus` are in the ELF |
| victim `CapturePulled`/`CaptureWait` and the shoulder attach | `ftcommoncapturepulled.c`, `ftcommoncapturewait.c` | same TU; the world-matrix helper `func_ovl0_800C9A38` is the port's and was fixed + owner-confirmed 2026-08-17 (`docs/BUGS.md`), `func_ovl2_800EDA0C` and `gmCollisionGetWorldPosition` are the source `gm/gmcollision.c` (imported) |
| cargo pickup: DK `ThrowF` → victim `Shouldered`, DK `ThrowFWait` | `ftcommonthrow.c:35-48` | `ndsBaseFTCommonThrowProcUpdate` disassembles to the fkind test then `bl ftCommonCaptureShoulderedSetStatus` + `bl ftDonkeyThrowFWaitSetStatus` |
| victim `Shouldered` entry (its 8-damage staled hit, colour anim, rumble 7) | `ftcommoncapture.c:131-159` | imported under `#if NDS_P2_DONKEY`; `ndsBaseFTCommonCaptureShoulderedSetStatus` calls `ftParamGetStaledDamage`, `ftCommonDamageUpdateDamageColAnim`, `ftParamUpdateDamage`, `ftParamUpdatePlayerBattleStats`, `ftParamUpdateStaleQueue`, `ftParamMakeRumble` -- the complete source body, and `ftParamGetStaledDamage` is itself source-faithful since P2-2 |
| carry `ThrowFWait` and its five-way interrupt | `ftdonkeythrowfwait.c` | `ftDonkeyThrowFWaitProcInterrupt` disassembles to the full ladder in source order: HeavyThrow → `ThrowFF` → KneeBend → Pass (`ftCommonPassSetStatusParam(gobj, 0xF1, 1.0, 0)` = `ThrowFFall`) → Turn (`ftMainSetStatus(gobj, 0xEF, …)` = `ThrowFTurn`) → Walk |
| carry walk `ThrowFWalk{Slow,Middle,Fast}` | `ftdonkeythrowfwalk.c` | `ftDonkeyThrowFWalkProcInterrupt` in the ELF; the three anims ship as `FTDonkeyAnim147/148/149` (`llFTDonkeyAnimCargo{VerySlow,Slow,}Walk`, ids `0x3b3/0x3b4/0x3b5`) |
| carry turn `ThrowFTurn` | `ftdonkeythrowfturn.c` | `ftDonkeyThrowFTurnProcUpdate` in the ELF, setter inlined into the Wait interrupt (above); anim `FTDonkeyAnim150` `llFTDonkeyAnimCargoTurn` `0x3b6` |
| carry jump `ThrowFKneeBend` → `ThrowFJumpSetStatus` | `ftdonkeythrowfkneebend.c`, `ftdonkeythrowffall.c:122` | `ftDonkeyThrowFKneeBendProcUpdate`, `ftDonkeyThrowFKneeBendProcInterrupt`, `ftDonkeyThrowFJumpSetStatus` all in the ELF |
| **walking off a platform edge while carrying** | `ftDonkeyThrowFCommonProcMap` = `mpCommonProcFighterOnFloor(gobj, ftDonkeyThrowFFallSetStatus)` (`ftdonkeythrowfwait.c:34`) | `ftDonkeyThrowFCommonProcMap` and `ftDonkeyThrowFFallSetStatus` in the ELF; the victim follows because `ftCommonThrownProcMap` re-homes onto the captor's `floor_line_id` and falls back to `mpCommonSetFighterProjectFloor` when it is `-1` |
| carry fall / fast-fall / pass-through | `ftdonkeythrowffall.c` | `ftDonkeyThrowFFallProcMap`, `ftDonkeyThrowFFallProcInterrupt` in the ELF; the `< -20.0` skip-landing branch is the source's `FTCOMMON_THROWFFALL_SKIPLANDING_VEL_Y_MAX` |
| carry landing `ThrowFLanding` | `ftdonkeythrowflanding.c` | `ftDonkeyThrowFLandingProcUpdate` in the ELF; anim `FTDonkeyAnim146` `llFTDonkeyAnimCargoLanding` `0x3b2` |
| **hit while carrying → `ThrowFDamage`** | `ftdonkeythrowfdamage.c` ← `ftCommonDamageUpdateCatchResist` (`ftcommondamage.c:447`) | **WAS BROKEN. See below.** |
| cargo release `ThrowFF` / `ThrowAirFF` (+ air back-turn) | `ftdonkeythrowff.c` | `ftDonkeyThrowFFSetStatus`, `...ProcUpdate`, `...ProcMap`, `ftDonkeyThrowAirFFProcMap`, `ftDonkeyThrowFFSwitchStatusAir`, `ftDonkeyThrowAirFFSwitchStatusGround` all in the ELF; anim `FTDonkeyAnim145` `llFTDonkeyAnimCargoAirThrow` `0x3b1` |
| **mash-out** (victim escapes, both fighters take the release knockback) | `ftcommoncapture.c:112-128` | `ndsBaseFTCommonCaptureShoulderedProcInterrupt` disassembles to `TrappedUpdateBreakoutVars` → `breakout_wait <= 0` → `ApplyCatchKnockback(dFTCommonCaptureKnockbackCatch)` + `ApplyCaptureKnockback(dFTCommonCaptureKnockbackCapture)` → both `catch_gobj` and `capture_gobj` cleared. The two release descriptors are byte-exact in `.main.rw`: `dNDSBaseFTCommonCaptureKnockbackCatch` = `361, 100, 30, 0` and `...KnockbackCapture` = `361, 80, 0, 20`, matching `ftcommoncapture.c:11,14`, and the disassembly passes Catch to the *captor* and Capture to the *victim* as the source does. Breakout budget is the source's US `(percent * 0.08) + 14`. The CPU victim's own escape is `ftcomputer.c:6039-6055` (`nFTCommonStatusShouldered` → `nFTComputerInputWiggle` after `(9 - level) * 15` tics) |
| **KO of either fighter while one is carried** | `ftcommondead.c:130` → `ftCommonThrownDecideDeadResult` (`ftcommonthrown2.c:82`) releases the grip in both directions and sends both to `mpCommonSetFighterWaitOrFall` | the port's wrapper (`reloc_backend_compat_shims.c:7514`) is an unconditional pass-through under `NDS_IMPORT_BATTLESHIP_FTMANAGER`, which every shipping configuration sets |
| **throws near blast zones** | not a special case in source: `ftDonkeyThrowFFProcUpdate` releases through `ftCommonThrownReleaseThrownUpdateStats(catch_gobj, -lr, 0, TRUE)` and the victim then takes the ordinary knockback → `ftCommonDead*` chain | same imported chain as every other throw, which P1 gated |
| **Sudden Death** | also not cargo-specific: SD re-enters through the rebirth path, and `ftcommonrebirth.c:69` calls `ftManagerInitFighter`, which clears `catch_gobj`-era state along with the rest of the fighter | `ftmanager.c` and `ftcommonrebirth.c` are imported verbatim |
| **Giant Punch charge persistence** | `ftmanager.c:594` zeroes `passive_vars.donkey.charge_level`, and it lives in `ftManagerInitFighter`, whose only callers are `ftManagerMakeFighter` (match start) and `ftcommonrebirth.c:69` (respawn). The only other clear is the release itself, `ftdonkeyspecialn.c:242`. So **charge survives damage, hitstun and knockdown, and resets on a KO respawn** | both TUs imported verbatim; no port seam sits between them |

### The defect this matrix found, and where it was fixed

`src/import/battleship_ftcommon_damage.c` redirected the source call
`ftDonkeyThrowFDamageSetStatus` to `ndsCompatFTDonkeyThrowFDamageSetStatus`, a
one-line seam in `src/port/reloc_backend_compat_shims.c` whose comment still
read *"real DK throw-damage runtime is not imported"*. That was true when it
was written and stopped being true when `battleship_donkey.c` started
compiling `ftdonkeythrowfdamage.c`. The shim body is
`ftCommonDamageGotoDamageStatus(fighter_gobj)`.

**Consequence, and it is a two-fighter state corruption, not a cosmetic
delta.** `ftCommonDamageUpdateCatchResist` is reachable with non-zero knockback
**only** for a Donkey Kong holding a fighter or a heavy item — every other
fighter's `ftCommonDamageCheckCatchResist` arm implies the zero-knockback /
hitlag-stack test that `UpdateCatchResist` immediately re-applies, which takes
the colour-anim branch instead. So the shim fired exactly on "DK is hit while
carrying someone". Source behaviour is `nFTDonkeyStatusThrowFDamage`: a
knockback-resisted stagger that **keeps the cargo** and returns to
`ThrowFWait`/`ThrowFFall`, and that re-syncs the victim
(`ftCommonThrownProcPhysics` + `ftParamsUpdateFighterPartsTransformAll`). The
port instead put DK into an ordinary `Damage` status **while `catch_gobj` still
pointed at the shouldered victim** — a fighter in `Damage`, then `Wait`, still
holding a victim that no `Wait` interrupt can throw, escapable only by the
victim's own mash-out.

**Proven in the binary before the fix, two independent ways.**
`ndsBaseFTCommonDamageUpdateCatchResist+0x4a` is
`bl 205e978 <ndsCompatFTDonkeyThrowFDamageSetStatus>`, and
`ndsCompatFTDonkeyThrowFDamageSetStatus` is
`bl <ndsBaseFTCommonDamageGotoDamageStatus>`. Independently,
`ftDonkeyThrowFDamageSetStatus` is **defined in `battleship_donkey.o` and
absent from the linked ELF** — compiled and then discarded by `--gc-sections`
because the redirect left it with no caller. Its sibling
`ftDonkeyThrowFDamageProcUpdate` (named by the status table) survives, which is
what makes the pair discriminating.

**Fixed at the owning seam**: the redirect is now `#if !NDS_P2_DONKEY`, and so
is the declaration and the shim's definition. With DK in the build the compat
name is neither declared nor defined, so the wrong form cannot be written back
by accident — it would take re-adding both halves past their comments.

**The general trap, for the next import wrapper.** *A source function can be
imported, compiled, and then thrown away, because a port `#define` retargeted
its only caller.* The cheap detector is a set difference: symbols defined in
`build/battleship_*.o` that are absent from the linked ELF, minus the ones with
an in-TU caller (those are just inlined). Run over `battleship_donkey.o`,
`battleship_ftcommon_catch.o`, `battleship_ftcommon_damage.o` and
`battleship_ftcommon_itemthrow.o` it held **one** entry before this fix and
**zero** after; the fourteen that remain in the raw difference all have their
in-TU caller listed in
`artifacts/verification/2026-08-25_p2-3r10-dkcargo/POST-FIX-linked-image.txt`.

### Runtime engagement: the lifecycle, driven rather than waited for

Two whole-match CPU traces prove the *shared* grab seam live and produced no DK
cargo pickup between them —
`artifacts/verification/2026-08-25_p2-3r10-dkcargo/fourcpu-roster4-trace.txt`
(5 grabs, 2 CatchWait, 2 throws) and `dkcargo-bothcpu-trace.txt` (4 grabs,
1 CatchWait, 1 throw). The reason is not a defect: a CPU in CatchWait smashes
the stick toward the stage centre (`ftcomputer.c:6026-6035`), so ThrowF versus
ThrowB is a coin flip on where the fighters happen to stand.

`scripts/probe-dk-cargo.ps1` drives it instead, through the game's own branch.
From a once-per-battle-frame hook (`ifCommonBattleUpdateInterfaceAll`) it walks
`gGCCommonLinks[nGCCommonLinkIDFighter]` and writes exactly two fields:
`fp->level = 9` on every fighter (grab *attempts*, out of the same field
`ftcomputer.c` reads) and `status_vars.common.catchwait.throw_wait = 0` on a
Donkey Kong already in CatchWait — precisely the condition
`ftCommonThrowCheckInterruptCatchWait` reads to choose the forward throw. No
guest function is called, no state is injected, and every read is a global or a
pointer derived from one.

Arm: `smash64ds-battle-playable-proof-hwtri` built as `build-p2-dkcargo` with
`NDS_P2_LUIGI=1 NDS_P2_DONKEY=1 NDS_P2_PROOF_FIGHTER0=2 NDS_R2_BOTH_CPU=1` —
Donkey Kong vs Fox, both CPU, Dream Land, one-minute Time, items off. Existing
flags only; no new target and no new harness mode.

Result (`dkcargo-driven-lvl9-trace.txt`), the source sequence in order at frame
1483:

    ndsBaseFTCommonCatchWaitSetStatus
    CARGOFORCE  (throw_wait := 0)
    ndsBaseFTCommonThrowSetStatus
    ftCommonCaptureShoulderedSetStatus     <- the victim is shouldered
    ftDonkeyThrowFWaitSetStatus            <- DK enters the carry
    ftDonkeyThrowFFSetStatus               <- the cargo release
    ftDonkeyThrowFFProcUpdate x39          <- the release plays out

Heartbeats at frames 600/1200/1800/2400/3000/3600 show the hook ran for the
whole match with DK cycling normally, which is what makes a zero elsewhere in
the ladder readable rather than ambiguous.

**What this run does not cover, plainly.** The CPU has no `ThrowFWait` handler,
so it taps A on its first carry frame: the carry lasted one throw, and
`ThrowFWalk`/`ThrowFTurn`/`ThrowFKneeBend`/`ThrowFLanding`, the mash-out, and
`ftDonkeyThrowFDamageSetStatus` never fired. Those rows rest on the source and
on the disassembly above — they are all inlined into the
`ftDonkeyThrowFWaitProcInterrupt` body that was read instruction by instruction
— and the probe is in the tree with its hook written for whoever wants to hold
the carry open.

## Acceptance

- [x] Move inventory sweep vs `ftdonkey` data (2026-08-24, above).
- [x] Cargo matrix: grab, carry, walk, turn, jump, edge, fall, landing,
      cargo-damage, release, mash-out, KO-while-carried, blast-zone throw and
      Sudden Death, each traced to its source owner and to the shipped image
      (2026-08-25, above). One defect found and fixed at its seam, and the
      grab → shoulder → carry → release lifecycle observed running in a live
      match.
- [x] Giant Punch charge-store semantics equivalent (2026-08-25, above):
      persists through damage/knockdown, resets on respawn and on release, and
      there is no release armour in the source to reproduce.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
- [ ] Owner feel pass on the cargo ladder specifically — the carried fighter's
      pose is placed from `ftCommonCapturePulledRotateScale`'s Euler extraction
      and no one has looked at it on a DS screen yet. Visual rows ask.

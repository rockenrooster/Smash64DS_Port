# Captain Falcon — P2-3 fighter 3

Status: **slice 1 (source inventory + ABI mirror) and slice 2 (the runtime
state machines, behind `NDS_P2_CAPTAIN`) are landed. Falcon is NOT selectable:
no native owner, no CSS/HUD/audio surfaces, no arena budget, no roster.**
Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftcaptain/` plus
`ft/ftcommon/ftcommonentry.c` and `ft/ftcommon/ftcommoncapturecaptain.c`.

## Role

Cheap, fast win after DK: no projectile, no article, standard grab — but the
fastest fall and run speeds in the game, so he stress-tests movement extremes
(traction, landing, edge slips) slower fighters never reach. Unlockable
(gating P2-7).

## Law-7 source inventory (read 2026-08-25, not remembered)

Derived by `scripts/fighters/generate_fighter_production_manifest.py` from
BattleShip `ftdata.c` + generated `relocData` + the US reloc symbol table + the
O2R headers. **Adding `"Captain"` to `BOOTSTRAP_FIGHTERS` was the only change
the generator needed** — the three generated products grew by 7,148 lines with
**zero deletions**, so the four landed fighters are byte-unchanged. That is
P2-3's exit criterion 1 ("a fighter rebuilds from BattleShip data by `make`")
holding on a fighter nobody hand-described.

| | Mario | Fox | Luigi | Donkey | **Captain** |
|---|---|---|---|---|---|
| core files | 9 slots | 9 | 9 | 9 | 9 (**6 present**) |
| local animation files | 143 | 158 | 12 | 153 | **152** |
| motion files | 143 | 158 | 143 | 153 | **152** |
| item-motion files | 19 | 19 | 19 | 18 | **19** |
| NitroFS files | 152 | 170 | 152 | 161 | **160** |
| event-32 motion files | 2 | 2 | 2 | 2 | **4** |
| special statuses | 9 | 26 | 9 | 30 | **19** |

Core closure (8 resources, `core_extern_closure` in the manifest):

| slot | symbol | O2R | id | bytes | alloc |
|---|---|---|---|---|---|
| main | `llCaptainMainFileID` | `reloc_fighters_main/CaptainMain` | 236 / 0xec | 2,234 | 102,448 |
| mainmotion | `llCaptainMainMotionFileID` | `CaptainMainMotion` | 235 / 0xeb | 7,726 | 9,696 |
| model | `llCaptainModelFileID` | `CaptainModel` | 332 / 0x14c | 51,432 | 51,536 |
| shieldpose | `llCaptainShieldPoseFileID` | `CaptainShieldPose` | 334 / 0x14e | 11,056 | 10,976 |
| special2 | `llCaptainSpecial2FileID` | `CaptainSpecial2` | 350 / 0x15e | 26,160 | 26,080 |
| special3 | `llCaptainSpecial3FileID` | `CaptainSpecial3` | 333 / 0x14d | 2,240 | 2,160 |
| — | (extern) | `reloc_extern_data/MiscData201` | 201 / 0xc9 | 2,176 | 2,096 |
| — | (extern) | `reloc_extern_data/MiscData299` | 299 / 0x12b | 272 | 192 |

**`submotion`, `special1` and `special4` are absent, and that is the source's
own answer**, not a gap in the extraction: Falcon has no projectile, so no
Special1 article file exists. Special2 carries **both** the Falcon Flyer entry
car (`efManagerCaptainEntryCarMakeEffect` reads three anim-joint tables out of
`gFTDataCaptainSpecial2` at 0x6200/0x6518/0x6598) **and** the Falcon Kick
effect (`dEFManagerCaptainFalconKickEffectDesc`, `efmanager.c:760`); Special3
is the Falcon Punch effect alone (`:790`).

`CaptainModel` sha256
`bbd56fc89524fc5a5de7d2cb88fdead3c231ad402b6039e1b63e4f1091c4669e`.

### Statuses (19, source-exact, and the port's four were WRONG)

`ftcaptain.h:57-79`. The full window is now mirrored in
`include/ft/fighter.h` with its motion twin. Before this row the port carried a
four-name placeholder whose `Attack100Start` was re-anchored to
`nFTCommonStatusSpecialStart`, putting Attack100 Start/Loop/End **one ordinal
low** (220/221/222 against the source's 221/222/223). Unread, so latent — and
invisible to `check-decomp-header-mirror.py` until row P2-3f3 taught that
checker to seed a second fold pass. Link had the identical defect and is fixed
with it. The full per-status motion / attack-id / kinetics / callback contract
is in `scripts/fighters/fighter_production_manifest.json`
(`Captain.special_status_contract`), derived from `ftcaptainstatus.h`.

| # | status | update / interrupt / physics / map |
|---|---|---|
| 0 | Attack13 | shared `ftCommonAttack13*` |
| 1-3 | Attack100 Start/Loop/End | shared `ftCommonAttack100*`, End = `ftAnimEndSetWait` |
| 4-5 | **AppearRStart / AppearLStart** | `ftCaptainAppearStartProcUpdate` |
| 6-7 | AppearREnd / AppearLEnd | `ftCommonAppearProcUpdate` |
| 8-9 | SpecialN / SpecialAirN (**Falcon Punch**) | `ftCaptainSpecialN*ProcPhysics/ProcMap` |
| 10-14 | SpecialLw / LwAir / LwLanding / AirLw / LwBound (**Falcon Kick**) | `ftCaptainSpecialLw*` |
| 15-18 | SpecialHi / HiCatch / HiThrow / AirHi (**Falcon Dive**) | `ftCaptainSpecialHi*` |

**Falcon is the only fighter whose entry is a two-status ladder.** Everyone else
gets one Appear status; `dFTCommonEntryAppearStatusIDs` gives Falcon
`AppearRStart`/`AppearLStart`, `ftCaptainAppearStartProcUpdate` runs it, and
`ftCaptainAppearEndSetStatus` hands off to `AppearREnd`/`AppearLEnd`. Two more
Falcon-only things live in `ftCommonAppearSetStatus`: `lr == -1` sets
`is_rotate = TRUE` (the 180° TopN flip in `ftCommonAppearProcPhysics`) **and**
calls `ftParamMoveDLLink(fighter_gobj, 1)`, which the Start update reverses
back to `FTDISPLAY_DLLINK_DEFAULT` once the car has passed `z > -1000`.

### Moveset uniques, from the source bodies

- **Falcon Punch (B)** — `ftcaptainspecialn.c`. Aerial version steers on the
  first `flag1` tick: `ftCaptainSpecialNGetAngle` clamps |stick_y| to 50,
  subtracts 10, floors at 0, and yields `(y*30)/40` degrees; velocity is
  `sin/cos(angle) * 65.0` with `lr` on x. `flag2` then selects air friction /
  `*0.92` decay / fast-fall drift. Ground and air swap status in place
  (`ftMainSetStatus(..., fighter_gobj->anim_frame, ...)` preserving
  RUMBLE|EFFECT|COLANIM), so a landing mid-punch keeps the animation frame.
  `efManagerCaptainFalconPunchMakeEffect` + `ftParamProcStopEffect`.
- **Falcon Kick (Down-B)** — `ftcaptainspeciallw.c`. Five statuses. Grounded
  kick rotates TopN.z by `-atan2(floor_angle)` so it hugs slopes; a wall while
  `flag1 == 1` goes to `SpecialLwBound`; `flag1 == 2` in the air goes to
  `SpecialLwAir`. `proc_hit` **and** `proc_shield` are
  `ftCaptainSpecialLwProcHit`, which halves `vel_scale` up to 6 times — the
  kick slows down each time it connects.
  `efManagerCaptainFalconKickMakeEffect`.
- **Falcon Dive (Up-B)** — `ftcaptainspecialhi.c`, and it is a command grab, not
  a hitbox: `ftParamSetCatchParams(fp, FTCATCHKIND_MASK_CAPTAINSPECIALHI,
  ftCaptainSpecialHiProcCatch, ftCommonCaptureCaptainProcCapture)`. Both sides
  are source code — the grabber's ladder here, the victim's in
  `ftcommoncapturecaptain.c`. Release is `efManagerQuakeMakeEffect(1)` +
  `ftCommonThrownReleaseFighterLoseGrip`. `flag1` re-aims: |stick_x| > 18 flips
  `lr` and rotates TopN.y by ±90°. `flag2` is a 15-tick timer during which the
  map proc uses `mpCommonProcFighterProject` instead of
  `mpCommonProcFighterCliffWaitOrLanding`. Ceiling + `MAP_FLAG_CLIFF_MASK`
  drops into `ftCommonCliffCatchSetStatus`. Ends in `ftCommonFallSpecialSetStatus`
  with drift `0.72` and landing lag `0.65`.
  **The victim tether is a reloc table read**:
  `ftCommonCaptureCaptainUpdatePositions` reads
  `llCaptainMainMotionSpecialHiVec2h` out of `gFTDataCaptainMainMotion`, indexed
  by the *victim's* fkind, and clamps the offset to 180 units. The port's reloc
  backend must resolve that symbol.
  **The "regrab" rule falls straight out of the status table and needs no
  special case:** a whiffed Dive ends through `ftCaptainSpecialHiProcUpdate` →
  `ftCommonFallSpecialSetStatus` (helpless), while a Dive that connects leaves
  via `SpecialHiThrow`, whose update callback is plain `ftAnimEndSetFall`. So
  connecting returns Falcon to ordinary Fall and whiffing does not. Note
  `ftCaptainSpecialHiProcStatus` sets `jumps_used = attr->jumps_max` either way.
- **Costumes: six, not four.** `dFTParamCostumeIDs[nFTKindCaptain]` is
  `{ { 0, 4, 1, 3 }, { 1, 5, 2 }, 5 }` — four common costumes (0/4/1/3) and
  three team costumes (1/5/2), over six distinct indices. (This file previously
  said "4 costumes"; that is the common count only.)
- `dFTParamSkeletonColAnimIDs[nFTKindCaptain] = 0x14`, the common value.

### Audio inventory (names; ordinals are region-gated, re-derive per P2-1e-1)

`gm/gmsound.h`. Ten FGM: `CaptainLanding`, `CaptainFoot`, `CaptainDash`,
`CaptainAppearCar1`, `CaptainAppearCar2`, `CaptainSpecialHi`,
`CaptainSpecialNStart`, `CaptainSpecialNPunch`, `CaptainDeadSlam`,
`CaptainDownBounce`. Twenty-three `nSYAudioVoiceCaptain*` including the two
identity lines `SpecialNFalcon` + `SpecialNPunch`, plus `FinalComeOn`.
Announcer `nSYAudioVoiceAnnounceCaptain`; crowd `nSYAudioVoicePublicCaptain`;
victory BGM `nSYAudioBGMWinFZero` (19). Kirby's copy pair
(`nSYAudioVoiceKirbyCopyCaptainSpecialN*`) is P2-3 fighter 10, not this row.

## Landed 2026-08-25

### Row P2-3f4 (pipeline slice 1)

- `"Captain"` in `BOOTSTRAP_FIGHTERS`; manifest, `fighter_production_files.mk`
  (`NDS_P2_CAPTAIN_FIGHTER_RELOC_FILES`, 160 resources) and
  `nds_fighter_production.generated.h` (`NDS_P2_CAPTAIN_ALLOC_SIZE_ROWS`)
  regenerated. Gated by `check-fighter-production-manifest.ps1`, which runs in
  every `verify-all` profile.
- The complete 19-status + 19-motion ABI mirror in `include/ft/fighter.h`,
  source-exact and now covered by `check-decomp-header-mirror.py`
  (shared names 1,393 -> 1,628).
- The stale `nFTCaptainStatusSpecialAirLw` macro in
  `src/import/battleship_efmanager.c` is gone -- `#ifndef` cannot see an
  enumerator, so it would have re-shadowed the real enum.

### Row P2-3f5 (the named blocker, then the runtime state machines)

**The blocker was two dict entries, and the runtime had modelled both opcodes
all along.** `SOURCE_STATE_EFFECTS` in
`scripts/fighters/generate_nds_native_owners.py` now maps `0xe2`
(`G_SETOTHERMODE_L`) to effect 2 and `0xf9` (`G_SETBLENDCOLOR`) to effect 12:

- **0xe2 needed no new state kind and no runtime change at all.** Effect 2 is
  `NDS_NATIVE_STATE_OTHERMODE`, and `ndsRendererNativeApplyStateDelta` hands
  `delta->w0 >> 24` to `ndsRendererRecordOtherMode`, which has always dispatched
  on the opcode byte and carries an explicit `NDS_RENDERER_OP_SETOTHERMODE_L`
  arm (`nds_renderer.c:8400`) accumulating othermode_L through the same
  shift/len bitfield decode the source uses. `sm64-nds`'s `g_setothermode_l`
  (`src/nds/nds_renderer.c:725`) is the identical four lines.
- **0xf9 needed one `case` in the fighter applier.** Effect 12
  (`NDS_NATIVE_STATE_BLEND`) already existed for the native STAGE program
  (`NDS_TASK26_BLEND` and the stage span applier); the fighter applier and
  `ndsRendererValidateNativeStateSpan` now admit it too.
- **The DS answer is the hardware alpha test, and it was already wired.**
  `ndsRendererHardwareApplyAlphaTest` (`:11134`) reads exactly these two fields:
  when othermode_L's alpha-compare bits are `G_AC_THRESHOLD` it emits
  `glEnable(GL_ALPHA_TEST)` + `glAlphaFunc(blend_color.a >> 4)`. Falcon's blend
  colour is `(0,0,0,0)`, so the reference is 0 -- pass when alpha > 0, the plain
  binary cutout the source asks for. No fidelity trade, so nothing to record
  under the sacrifice order. (`sm64-nds` discards `G_SETBLENDCOLOR` outright,
  `:1017`; this port keeps it because it IS the alpha reference.)

Evidence `artifacts/verification/2026-08-25_p2-3f/captain-high-detail-decode.txt`:
Falcon's high-detail model decodes clean -- 17 roots, 87 state deltas, 254
sequence entries, 34 vertex actions, 319 triangles, 34 runs, 34 epochs, with the
five commands landing as deltas 54/55 (OTHERMODE), 56 (BLEND), 60/61 (OTHERMODE
restore). The **low**-detail decode reproduces P2-3f4's pre-change figures
exactly (73/223/30/200/30/30), which is the control that the two opcodes changed
nothing else, and `generate_nds_native_owners.py --check` passes, so the four
landed owners' generated IR is byte-identical.

**The runtime slice, behind `NDS_P2_CAPTAIN` (default 0):**

- `include/ft/ftchar/ftcaptain/ftcaptainstatus.h` is the forwarding shim, and it
  is in `check-architecture.ps1`'s per-fighter allowlist.
- `src/import/battleship_captain.c` -- `ftcaptainspecialn.c` +
  `ftcaptainspeciallw.c` + `ftcaptainspecialhi.c` verbatim, US constants
  restated. **One constant is deliberately not verbatim:**
  `FTCAPTAIN_FALCONPUNCH_VEL_MUL` is `0.92` -- a *double* -- in `ftcaptain.h`,
  so `vel_air.y *= 0.92` compiles to two soft-float conversions plus an `aeabi`
  double multiply per axis per aerial-punch tick on a Thumb ARM9 build.
  Restated `0.92F`.
- `src/import/battleship_ftcommon_capturecaptain.c` -- Falcon Dive's victim
  side, plus `llCaptainMainMotionSpecialHiVec2h = 0x0`. The tether resolves
  with no registry entry: `ndsRelocResolveSymbolOffset` falls back to
  dereferencing an `ll*` symbol whose address is in main RAM.
- **The two `mpcommon` seams landed at the seam, not in the fighter.**
  `mpCommonCheckFighterCeilHeavyCliff` was DECLARED in `include/mp/map.h` with
  no definition anywhere; `mpCommonSetFighterWaitOrLanding` did not exist at all
  and its three lines had been copied inline into two callers. Both are now in
  `reloc_backend_compat_shims.c`, `mpCommonProcFighterCliffWaitOrLanding` is the
  source one-liner over them, and `mpCommonProcFighterWaitOrLanding` lost its
  duplicate body. Kirby, Purin, Link and Yoshi all reach the same pair.
- The Falcon branch in `battleship_ftcommon_entry.c` (registered bounded partial
  import): `AppearRStart`/`AppearLStart`, `is_rotate` on `lr == -1`,
  `efManagerCaptainEntryCarMakeEffect`, the `ftParamMoveDLLink(gobj, 1)` tail,
  and both `ftCaptainAppear*` bodies.
- `ftCommonAttack13Proc{Update,Interrupt}` aliases in
  `battleship_ftstatus_callback_aliases.c`. Attack11/12 have been there since
  the Mario/Fox slice; **13 is the one jab step SSB64 makes character-specific**,
  so it lives in each fighter's own special table and no landed fighter had ever
  named it. The remaining eight fighters all need the same pair.
- `HALF_PI32` added to `include/macros.h`, mirroring `decomp/include/macros.h:14`.

**THE WEAK TWINS WERE NOT DELETED, AND DELETING THEM WOULD HAVE BEEN WRONG.**
The brief for this row -- and item 3 of P2-3f4's own remainder list -- said to
delete the six `NDS_SPECIAL_COMMON_WEAK_STATUS` / `NDS_REFLECTOR_WEAK_STATUS`
twins and called that "the Donkey Kong pattern verbatim". Donkey Kong did not do
that: `battleship_special_common.c` still carries `ftDonkeySpecialNStartSetStatus`
and three siblings as weak twins. It cannot do that -- the dispatch tables in the
imported `ftcommonspecialn.c` name every fighter's setter **unconditionally**, so
a build with `NDS_P2_CAPTAIN=0` needs the twin to link at all. The DK pattern is
*strong definition beside weak twin, linker picks strong*, and the check that it
worked is `nm` on the linked ELF -- never a grep of `src/`.

Verified exactly there, in
`artifacts/verification/2026-08-25_p2-3f/captain-runtime-slice-linked-elf.txt`
(`make TARGET=smash64ds BUILD=build-p2-3f5-captain NDS_P2_CAPTAIN=1`): all six
`ftCaptainSpecial*SetStatus` are `T` and not one `W` remains; 42 strong
`ftCaptain*` bodies; `dFTCaptainSpecialStatusDescs` is `0x17c` = 19 entries, not
the 16-entry `NDS_FT_STATUS_STUB16`; `efManagerCaptainEntryCar`,
`...FalconPunch` and `...FalconKick` are present (all three had been
gc-sectioned away for want of a caller); the three `ftCommonCaptureCaptain*`
bodies replaced the weak inactive stub; and the Attack13 aliases disassemble to
`bl ndsBaseFTCommonAttack13Proc*`.

## Remaining, in dependency order

1. **Native owner.** The decode is unblocked, but Falcon is not a runtime owner.
   That row needs `P2_O2R_ASSETS`, `OWNER_JOINT_TREES{,_LOW}`,
   `OWNER_SETUP_PARTS`, `OWNER_PLAN_COUNTS` (the five pins below), plus
   `OWNER_CROSS_BINDING_SLOTS`, `OWNER_GX_PLAN_COUNTS`, a
   `P2_OWNER_MODEL_CENSUS` row, a `P2_RUNTIME_OWNERS` entry and the two NitroFS
   owner images. **Do not add the pins as dead dict entries ahead of that row**
   -- `build_p2_owner_model_inventory` asserts a census that does not exist yet.
   The pins, re-proved by this row's high-detail decode:
   - `P2_O2R_ASSETS["captain"]` = (`reloc_fighters_main/CaptainModel`, `0x014c`,
     sha `bbd56fc89524fc5a5de7d2cb88fdead3c231ad402b6039e1b63e4f1091c4669e`)
   - `OWNER_JOINT_TREES["captain"]` = `(0x3be0, 27)` -- `332_CaptainModel.c:1888`
   - `OWNER_JOINT_TREES_LOW["captain"]` = `(0x7900, 27)` -- `:4071`
   - `OWNER_SETUP_PARTS["captain"]` = `(0xffffff80, 0x00000000)` --
     `236_CaptainMain.c:103`, the same mask Donkey uses
   - `OWNER_PLAN_COUNTS["captain"]` = `(26, 17)` -- 25 selected joints + TopN,
     17 drawable roots, identical in both details
2. **Admission: make him selectable.** `NDS_P2_SHELL_ROSTER=3`, plus the ~40
   `#if NDS_P2_DONKEY` sites that gate the CSS/HUD/asset/renderer surfaces
   (`battleship_mnplayersvs.c`, `battleship_scsubsysdata_ft.c`,
   `nds_menu_shell.c`, `nds_renderer.c`, `battleship_ftmanager.c`), plus the
   arena budget: the worst case the roster is sized against is Luigi vs Donkey
   at 36,276 B of resident owner images with 28,772 B of headroom, so Falcon's
   images must be measured before he joins the roster, not after.
3. **Audio.** Ten FGM + 23 voice + announcer/crowd/victory-BGM ordinals
   (inventory below), re-derived per P2-1e-1.
4. **Budgets + one measured 4-CPU stress run** under the then-current stress
   config (P2_PLAN law 2/3). Per-kind unique arena cost to compare against:
   Mario 54,048 / Fox 116,752 / Luigi 41,552 / Donkey 77,360 B.
5. **Owner feel pass** on Falcon Dive grab/release/regrab and the speed extremes.

### Two open questions this row found and did not close

- **`mpCommonProcFighterProject` diverges from the source, and Falcon Dive is a
  caller.** The source (`mpcommon.c:692`) is
  `mpCommonCheckFighterProject(fighter_gobj)`, i.e.
  `mpProcessUpdateMain(..., MAP_PROC_TYPE_PROJECT)`; the port's
  (`battleship_ftstatus_map_physics_shims.c:68`) calls
  `mpCommonRunFighterCollisionDefault` instead. The source-faithful
  `mpCommonCheckFighterProject` already exists at
  `reloc_backend_compat_shims.c:14615`, so the fix is one line -- but it is a
  SHARED seam every status table's `ProcMap` can reach, so it wants its own row
  with a regression run rather than a drive-by inside a fighter landing. Falcon
  Dive uses it for a 15-tick window after Up-B starts.
- `gFTDataCaptainMainMotion` is NULL in every built configuration, because
  nothing creates a Falcon yet. `ftCommonCaptureCaptainUpdatePositions` would
  then dereference `ndsRelocGetFileData`'s NULL return. Unreachable today and
  fixed by item 2 loading the asset graph -- but it is a hard crash the first
  time a Falcon is spawned without his files, so item 2 must land the assets
  before it lands the CSS entry.

## Acceptance

- [x] Move inventory sweep vs `ftcaptain` data (19 statuses, 152 animations,
      160 NitroFS resources, source-derived).
- [x] Native-model DECODE unblocked (0xE2 + 0xF9 modelled; high detail decodes).
- [x] Runtime state machines land, link, and are verified strong on the ELF.
- [ ] Native model ADMITTED as a runtime owner (remaining item 1).
- [ ] Falcon selectable at all (remaining item 2) -- until then nothing below
      can be exercised.
- [ ] Falcon Dive grab/release/regrab semantics equivalent.
- [ ] Fast-fall/landing/edge behavior spot-checks at speed extremes.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.

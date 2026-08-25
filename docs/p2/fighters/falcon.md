# Captain Falcon — P2-3 fighter 3

Status: **pipeline slice 1 landed (source inventory + ABI mirror); runtime not
started.** Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftcaptain/`
plus `ft/ftcommon/ftcommonentry.c` and `ft/ftcommon/ftcommoncapturecaptain.c`.

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

## Landed this row (P2-3f4)

- `"Captain"` in `BOOTSTRAP_FIGHTERS`; manifest, `fighter_production_files.mk`
  (`NDS_P2_CAPTAIN_FIGHTER_RELOC_FILES`, 160 resources) and
  `nds_fighter_production.generated.h` (`NDS_P2_CAPTAIN_ALLOC_SIZE_ROWS`)
  regenerated. Gated by `check-fighter-production-manifest.ps1`, which runs in
  every `verify-all` profile. Inert: the Makefile stages a fighter's files only
  under its own `NDS_P2_<KIND>` flag, and no `NDS_P2_CAPTAIN` exists yet.
- The complete 19-status + 19-motion ABI mirror in `include/ft/fighter.h`,
  source-exact and now covered by `check-decomp-header-mirror.py`
  (shared names 1,393 → 1,628).
- The stale `nFTCaptainStatusSpecialAirLw` macro in
  `src/import/battleship_efmanager.c` is gone — `#ifndef` cannot see an
  enumerator, so it would have re-shadowed the real enum.

## Remaining, in dependency order

1. **Native model — BLOCKED on a pipeline extension, and this is the finding
   this row owes.** The high-detail `CaptainModel` display lists use **four
   `G_SETOTHERMODE_L` (0xE2)** commands and **one `G_SETBLENDCOLOR` (0xF9)**;
   `generate_nds_native_owners.py`'s `SOURCE_STATE_EFFECTS` models neither, so
   `_decode_control` raises `unsupported control opcode 0xe2` at root 6 command
   6. **Census over all five owners, both details**
   (`artifacts/verification/2026-08-25_p2-3f/captain-model-opcode-census.txt`):
   Mario 0/0, Fox 0/0, Luigi 0/0, Donkey 0/0 — Falcon is the first, and all
   five commands sit in his root 6 alone:

   ```
   root 6 cmd  6  0xe2001e01 / 0x00000001   SETOTHERMODE_L shift 0 len 2 -> G_AC_THRESHOLD
   root 6 cmd  7  0xe200001c / 0xc4113078   SETOTHERMODE_L shift 3 len 29 -> render mode
   root 6 cmd  9  0xf9000000 / 0x00000000   SETBLENDCOLOR (0,0,0,0)
   root 6 cmd 36  0xe2001e01 / 0x00000000   -> G_AC_NONE   (restore)
   root 6 cmd 37  0xe200001c / 0xc4112078   -> render mode (restore)
   ```

   One alpha-tested surface, bracketed around root 6's two draws. **The blend
   colour is (0,0,0,0), so the threshold is alpha > 0** — a plain binary
   cutout, which a DS paletted texture with a transparent index 0 already does
   in hardware. That makes this look like an implementation task (a decoder
   kind plus a runtime applier) rather than a fidelity trade, but it is still a
   new render state on a fighter model and wants its own row with an owner
   visual pass. **The low-detail model decodes clean today** (17 roots, 73
   state deltas, 223 sequence entries, 30 vertex actions, 200 triangles, 30
   runs, 30 epochs) with these pins, which is the evidence that the rest of the
   pin set below is right:
   - `P2_O2R_ASSETS["captain"]` = (`reloc_fighters_main/CaptainModel`, `0x014c`,
     sha above)
   - `OWNER_JOINT_TREES["captain"]` = `(0x3be0, 27)` — `332_CaptainModel.c:1888`
   - `OWNER_JOINT_TREES_LOW["captain"]` = `(0x7900, 27)` — `:4071`
   - `OWNER_SETUP_PARTS["captain"]` = `(0xffffff80, 0x00000000)` —
     `236_CaptainMain.c:103`, the same mask Donkey uses
   - `OWNER_PLAN_COUNTS["captain"]` = `(26, 17)` — 25 selected joints + TopN,
     17 drawable roots, identical in both details
2. **`NDS_P2_CAPTAIN` flag + status table.** `include/ft/ftchar/ftcaptain/`
   `ftcaptainstatus.h` becomes the forwarding shim (`NDS_FT_STATUS_STUB16`
   otherwise) and joins `check-architecture.ps1`'s per-fighter allowlist. Note
   the Makefile's existing rule shape: `NDS_P2_DONKEY=1` requires
   `NDS_P2_LUIGI=1` so native-owner slots stay dense.
3. **Behavior TUs — three, the DK pattern exactly.** A new
   `src/import/battleship_captain.c` including `ftcaptainspecialn.c`,
   `ftcaptainspecialhi.c`, `ftcaptainspeciallw.c` with the `FTCAPTAIN_*`
   constants restated (US arm: air accel ×1.1, air speed max ×0.8), **plus** a
   new wrapper for `ftcommoncapturecaptain.c` (absent from the port; it is one
   of the fifteen missing `ftcommon` TUs P2-3r10 counted). Delete the weak twins
   in `battleship_special_common.c` (`ftCaptainSpecialN/AirN/Hi/AirHi/AirLw
   SetStatus`) and `battleship_fox_reflector.c`
   (`ftCaptainSpecialLwSetStatus`) rather than shadowing them — the dispatch
   tables in `ftcommonspecialn.c` etc. are already imported and already name
   Falcon's setters, so the moment the real bodies link, the source table
   selects them.
4. **Two missing `mpcommon` seams.** `mpCommonCheckFighterCeilHeavyCliff` and
   `mpCommonProcFighterCliffWaitOrLanding` are in `mp/mpcommon.c`, which this
   port reimplements in `src/port/reloc_backend_compat_shims.c` rather than
   importing. Falcon Dive's map proc is the first caller of either. Port both
   at that seam; do not fork them into the fighter.
5. **Entry.** Add the Falcon branch to `src/import/battleship_ftcommon_entry.c`
   (registered bounded partial import): `AppearRStart`/`AppearLStart`,
   `is_rotate` on `lr == -1`, `efManagerCaptainEntryCarMakeEffect`, the
   `ftParamMoveDLLink(gobj, 1)` tail, and the two `ftCaptainAppear*` callbacks.
   `efManagerCaptainEntryCarMakeEffect` is already compiled in
   `battleship_efmanager.o` and dropped by `--gc-sections` for want of a caller;
   it returns the moment Falcon calls it. The car will render through the
   generic path — the AOT entry-effect owner
   (`scripts/3d_vfx/generate_nds_entry_effects.py`) bakes only `MarioSpecial2`
   (asset 356) and `FoxSpecial3` (161).
6. **CSS/HUD/audio**, then **budgets + one measured 4-CPU stress run** under
   the then-current stress config (P2_PLAN law 2/3). Per-kind unique arena cost
   to compare against: Mario 54,048 / Fox 116,752 / Luigi 41,552 /
   Donkey 77,360 B.

## Acceptance

- [x] Move inventory sweep vs `ftcaptain` data (19 statuses, 152 animations,
      160 NitroFS resources, source-derived).
- [ ] Native model admitted (blocked, item 1).
- [ ] Falcon Dive grab/release/regrab semantics equivalent.
- [ ] Fast-fall/landing/edge behavior spot-checks at speed extremes.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.

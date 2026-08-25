# Captain Falcon — P2-3 fighter 3

Status: **slices 1-3 are landed and Falcon IS SELECTABLE.** Source inventory +
ABI mirror (P2-3f4), the runtime state machines behind `NDS_P2_CAPTAIN`
(P2-3f5), and the native owner + roster admission at `NDS_P2_SHELL_ROSTER=3`
(P2-3f8). **He has NO AUDIO** -- not one of his ten FGM or twenty-three voice
cues is in the phase pack, so every one of them fails closed and he plays
silent. That, an owner feel pass, and the four-kind budget question below are
what remain.
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


### Row P2-3f8 (the native owner, then admission)

**Falcon is a runtime native owner, slot 4, and TWO of the generator's
assumptions were his to break.**

- **He has ZERO cross-matrix runs, in either detail.** Every one of his 319
  high / 200 low source triangles is single-binding and draws under the current
  root, so `OWNER_CROSS_BINDING_SLOTS["captain"]` is empty, his GX plan is
  `(1, 6, 6, 0, 0)` in both details, and he claims no physical palette slot at
  all. That is not a missing pin: the independent geometry oracle reports
  *"319 single-binding triangles face the way their own vertex normals do"* --
  all of them. Consequence for the next owner: the reserved cross-run band stays
  Donkey's 16..25, so the five free levels above it (26..30) that
  `ndsRendererAdapterBuildGxSlotTable` allocates parent slots downward from are
  **unchanged** by this landing.
- **His LOW model carries a root light preamble his HIGH model never emits**
  (`0xffffff00 / 0x804c3300`, root 6), and the generator asserted they were
  always equal -- in a comment that called itself *"currently unreachable;
  documents the ABI expectation"*. It was a false invariant that would have
  rejected a faithful decode. Both owner runtimes point at the ONE table the
  high pass emits, so the fix is the **union**: the high table is a prefix of
  it, every high root index is unchanged, and the low roots are re-indexed
  through their own table's position in it. `sNdsNativeCaptainRootLightPreambles`
  is 0x20 bytes = 4 entries on the linked ELF. **Luigi and Donkey are
  byte-identical after the change** -- the regenerated
  `nds_native_fighter_owner.generated.inc` differs from the previous one by a
  single blank separator line before the `#if NDS_P2_CAPTAIN` block, which is
  the control.

Census pins (`P2_OWNER_MODEL_CENSUS["captain"]`), 13 fields:

| detail | state | seq | vtx | tris | runs | epochs | roots | dense | corners | crossRuns | pfxLight | intraLight | restores |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| high | 87 | 254 | 34 | 319 | 34 | 34 | 17 | 291 | 957 | **0** | 68 | 6 | **0** |
| low | 73 | 223 | 30 | 200 | 30 | 30 | 17 | 205 | 600 | **0** | 68 | 4 | **0** |

`check_native_owner_geometry_closure.py` now covers him (`OWNERS` gained
`"captain"`): source/vertex/matrix-routing/facing/winding closure and both
primitive modes, high and low, 0 failures. NitroFS images
`captain_high.bin` **10,908 B** + `captain_low.bin` **7,892 B** = **18,800 B**
resident (Luigi 16,076, Donkey 20,200).

**Admission: `NDS_P2_SHELL_ROSTER` defaults to 3.** Assets (`nds_reloc_assets.c`
already carried his rows from P2-3f4); `reloc_backend_assets.c` widened at six
seams; the CSS portrait plus the in-progress question-mark plate, his Selected
figatree (`relocData/429_FTCaptainAnimSelected.c`), preview residency,
`ftManagerSetupFilesAllKind`, the fighter mask; the HUD stock icon and
portrait; and his three `efManager` descriptors resolved. Like Donkey he gets
no per-fighter CSS gate art and no selection flash -- those cap at three
fighters, and that cap is P2-1k(b), not this row.

**TWO REAL DEFECTS FOUND ON THE WAY, both fixed at their own seam.**

1. **Falcon's `FTAttributes` mixed-u16 lanes had no normalizer.**
   `ndsRelocNormalizeFighterAttributesFile` knew Mario, Fox and Donkey only.
   Without an arm his `dead_fgm_ids` / `deadup_sfx` / `damage_sfx` /
   `smash_sfx` / `itemthrow_*` / `heavyget_sfx` keep the N64 big-endian lane
   order inside each word. Offset **0x488** (`236_CaptainMain.c:87`,
   *"Pre-attributes data (290 words, 0x0488 bytes)"* -- the same shape that
   gives Donkey 0x4a4). The fail-closed validation values are gmsound.h's
   REGION_US arm, and the resolver was checked against Donkey's eight already
   in the file before being trusted. **Falcon's smash triple is
   `{ Smash3, Smash2, JumpAerial }` = 341/340/353, NOT Smash1..3** --
   `236_CaptainMain.c` says so, and a copied Donkey pattern would have been
   wrong. **LUIGI STILL HAS NO ARM AND THAT IS A LIVE GAP** (item 6 below).
2. **The battle HUD's portrait and stock palette bands overlapped, and Donkey
   was already paying for it.** Portraits load once at prepare into
   `PORTRAIT_PALETTE_BASE + i` = 5..8; stock palettes are re-uploaded per
   player into `STOCK_PALETTE_BASE + player` = 8..11 whenever a costume
   changes. Slot 8 is both the FOURTH portrait -- Donkey's -- and player 0's
   stock, so a Donkey HUD portrait drew in whatever colours player 0's stock
   icon last needed. A fifth portrait would have taken slot 9 as well. Stock
   base is now 10 (damage 0..3, white 4, portraits 5..9, stock 10..13, 14..15
   spare) with two `_Static_assert`s, so the next portrait fails the BUILD
   instead of the screen. Nothing else writes `SPRITE_PALETTE_SUB` -- the UI
   kit's sub sprites are direct-colour bitmaps.

**A THIRD DEFECT, and it is not Falcon's -- it is every generated header's.**
The UI-kit bake and its consumer race under the parallel build. `make` spells
the generated include as `$(PROJECT_ROOT)/src/nds/generated/...`, which MSYS
expands to `/d/Stuff/...`, while `gcc -MMD` writes `D:/Stuff/...` into the
`.d`: **two different nodes**, so the recorded edge carries no ordering, and
the generator and the compile of its consumer are siblings under
`$(OUTPUT).elf`. Measured here: `mn_surfaces.bin` and
`mn_ui_kit.generated.inc` at 12:23 against `nds_ui_kit.o` at 12:14, and the ROM
linked from that pair failed `p2_shell_loop` with **`BACKDROP SURFACES:
mismatch=145`** -- the stale metric table's per-surface FNV against the fresh
pack. The next `make` with no source change recompiled the file, which is the
proof it was ordering and not a bad bake. Six explicit object prerequisites now
sit beside the existing `$(OFILES): $(PROJECT_ROOT)/Makefile` line. **Rail: a
generated include needs an explicit prerequisite line; `-MMD` is not a
substitute here, and the failure mode is silent wrong art wherever the runtime
does not happen to hash-check the asset.**


### The residency proof, and the one cadence cost it exposed

`scripts/menus/probe-p2-shell.ps1` on `smash64ds-p2-shell-hwtri`, fast logic 0,
against the same probe run at roster 2 the day before
(`artifacts/verification/2026-08-24_p2-shell.txt` vs `2026-08-25_p2-shell.txt`):

- **`CSSRES ready=0x00000097`, every fail mask 0.** Bits 0/1/2/4/**7** =
  Mario/Fox/Donkey/Luigi/**Captain**. It was `0x17` at roster 2. That is the
  direct evidence that Falcon's asset graph AND both native-owner images load
  at the character select -- `main`, `sub`, `anim` and `owner` fail masks are
  all zero, so nothing degraded quietly.
- **`CSSIO ownerload` 8 -> 12, `ownerbytes` 72,552 -> 110,152.** The arithmetic
  closes exactly: (16,076 + 20,200 + 18,800) x 2 character-select entries =
  110,152, so the loader really did read Falcon's two images twice and nothing
  else changed size.
- **The cost is entirely on the character select and nowhere else.** Per-screen
  VBlank-interval histograms over the identical 11-stop walk:

| screen | roster 2 (2/3/4/5+) | roster 3 (2/3/4/5+) |
|---|---|---|
| Title | 149/1/0/0 max 2 | 149/1/0/0 max 2 |
| Mode select | 301/0/0/0 max 1 | 301/0/0/0 max 1 |
| VS mode | 1811/0/0/0 max 1 | 1811/0/0/0 max 1 |
| **Character select** | **2721/132/9/6 max 5** | **2585/228/47/8 max 5** |
| Stage select | 244/0/0/0 max 1 | 244/0/0/0 max 1 |

  Four of the five screens are byte-identical. The character select presents the
  same 2,868 frames but its **two-VBlank share falls 94.87% -> 90.13%**, with 96
  more three-VBlank frames, 38 more four-VBlank and 2 more five-plus; the max
  interval is unchanged at 5. **`PROJECT_GOAL.md` asks for >=95% two-VBlank on
  every screen, so this screen was ALREADY 0.13 points short at roster 2 and is
  now 4.87 short.** The obvious candidate is the fifth kind's synchronous
  preparation at `ndsMNPlayersVSPreviewInit` -- `ftManagerSetupFilesAllKind`
  plus two more NitroFS owner-image reads, and `CSSIO hdr` 733 -> 773 /
  `payload` 3,102 -> 3,120 moves with it -- but **frame-level placement was not
  measured, so whether these are entry-load frames or steady-state is not
  claimed here.** Measure that before sizing a fix; if it is entry load, the
  answer is the shape P2-3r16 already used on the BGM (bracket/defer the
  synchronous setup), not a cadence lever.
- `MSSURF blit=559 mismatch=0 readfail=0 hash=84d499ca` and `MSKIT
  hash=c417dfa1 mismatch=0`, both identical to roster 2 -- the UI-kit surface
  pack is consistent on this target too, which is the second confirmation of the
  build-race fix above.

### Measured budget (P2-3f8)

| figure | roster 2 | roster 3 | delta |
|---|---|---|---|
| ARM9 image end (`.main.bss` end) | 35,819,120 | 35,839,184 | **+20,064 B** |
| `p2_shell_loop` arena free floor | 258,568 | **238,088** | -20,480 |
| worst resident owner-image pair | Luigi+Donkey 36,276 | Donkey+Captain **39,000** | +2,724 |

**Per-kind arena charge** at `ftManagerSetupFilesMainKind`, the currency P2-2's
byte law is written in: `NDS_P2_CAPTAIN_ALLOC_SIZE_ROWS(0xec)` is
**102,448 B** standalone and **100,160 B unique** once the 2,288 B
`MiscData201`+`MiscData299` pair every landed kind already makes resident is
deducted -- the same 2,288 Fox and Donkey pay. Against the table:

| kind | unique arena bytes |
|---|---|
| Mario | 54,048 |
| Luigi | 41,552 |
| Donkey | 77,360 |
| **Captain** | **100,160** |
| Fox | 116,752 |

> **SUPERSEDED BY P2-3f9 (2026-08-25).** The "+46,112 B over" inference below
> was right in direction and wrong in size, and the reason matters more than the
> number: it priced the SHELL against a margin measured on the LAB arm, and the
> shell's scene arena is **36,864–73,728 B smaller** because its ARM9 binary is
> bigger (`gNdsTaskmanArenaChosenSize` 1,658,880 / AllocFail 18 on the shell,
> 1,695,744 / 9 on the stress arm, 1,732,608 / 0 on the mirror). Measured from
> the shell, the argmax roster **halted** — `ndsRendererNativeEnsureOwnerImage`
> asking 12,140 B with 5,824 B free, deficit **6,316 B**, not 46,112. It is
> fixed: 90,112 B from the graphics-heap reservation plus 287,936 B from making
> the BattlePack carve a per-match decision, and the four-kind whole-match
> low-water is now **419,052 B against the 25,600 B floor**. See board row
> P2-3f9. **A four-kind margin taken from a lab arm does not transfer to the
> shell — re-measure on the arm you are shipping.**
>
> P2-3f9 also found that **Captain in a four-fighter match costs about 30x the
> wall time of Mario** for the same guest frames, with every memory counter
> identical to the control. That is remaining item 7 below, and it is why the
> argmax roster no longer hangs but is not yet playable.

**THE HARDEST FOUR HAS CHANGED, AND IT IS NOT AFFORDABLE TODAY.** Boundary's
`p2_fourcpu_stress` runs Mario/Fox/Luigi/Donkey = 289,712 B, and P2-3r13
measured that configuration at a **24,356 B** margin over the 25,600 B floor.
The measured argmax over landed content is now Fox+Captain+Donkey+Luigi =
**335,824 B**, i.e. **+46,112 B** -- roughly twice the whole margin, before the
20,064 B this row already spent. So:

- the stress arm's roster is no longer the argmax `PROJECT_GOAL.md` asks for,
  and re-pointing it needs a byte lever first, not a roster edit;
- **a user CAN reach that configuration from the shipped character select** --
  four slots, five kinds -- and nobody has measured a four-distinct-kind match
  driven from the shell at ANY roster, roster 2 included. That is a
  pre-existing hole this row makes deeper, not one it opens.

The nearest measured lever is P2-3r13's own untaken one: the two graphics-heap
contexts hold **96 B of a 106,496 B reservation**, overflow 0, over a whole VS
match. Re-read it on Results / Sudden Death / pause zoom, then cut.

## Remaining, in dependency order

1. **Audio -- he is completely silent, and this is the largest gap.** Ten FGM
   (`CaptainLanding` 73, `CaptainFoot` 106, `CaptainDash` 117,
   `CaptainAppearCar1/2` 180/181, `CaptainSpecialHi` 182,
   `CaptainSpecialNStart/Punch` 183/184, `CaptainDeadSlam` 0x120,
   `CaptainDownBounce` 299), twenty-three `nSYAudioVoiceCaptain*`, announcer
   `nSYAudioVoiceAnnounceCaptain` **485**, crowd `nSYAudioVoicePublicCaptain`
   **604**, victory BGM `nSYAudioBGMWinFZero` **19**. Every ordinal above was
   re-derived from `gm/gmsound.h`'s REGION_US arm with a resolver validated
   against Donkey's eight known values first, per P2-1e-1. **The CSS announcer
   already works** -- `kNdsCssAnnounceVoice` is the source's own fkind-indexed
   table and index 7 is already 485. What is missing is the phase pack:
   `nds_audio_fgm.c`'s admission switch is a fail-closed allowlist and names
   none of his ids, so each cue is dropped and recorded in the miss ring. No
   verifier asserts on it, so this is silence rather than a red gate. Shape the
   work on the Donkey bank (`render-audio-fgm-phase-pack.py` `SELECTED +=`
   block, `--derive <ids>` for every field, hash pins per cue).
2. **Owner feel pass** on Falcon Dive grab/release/regrab and the speed
   extremes (fastest fall + fastest run: traction, landing, edge slips).
3. **A four-distinct-kind budget answer — CLOSED by board row P2-3f9
   (2026-08-25).** The shell-driven argmax match was measured, it halted, and
   the two levers named here were taken: the graphics-heap reservation
   (0xD000 -> 0x2000, +90,112 B, and its 96 B peak is now proven on a whole
   four-kind match rather than sampled) plus the BattlePack carve becoming a
   per-match decision (+287,936 B whenever a match holds more than two distinct
   kinds). Whole-match low-water **419,052 B** against the 25,600 B floor.
   `p2_fourcpu_stress` was re-pointed to the argmax and **backed out** — see
   item 7.
7. **HE COSTS ABOUT 30x MARIO IN A FOUR-FIGHTER MATCH, and this is now his
   largest gap after audio.** Moving `p2_fourcpu_stress` slot 0 from Mario to
   Captain made the arm take about thirty times the wall clock for the same
   guest frames (100 stops at `ifCommonBattleUpdateInterfaceAll` reached
   presented frame 45 in 240 s, against frame 49 in 8 s on the pre-change
   four-kind ROM), and the harness hit its 3600 s ceiling. **It is not memory
   and it is not the P2-3f9 arena work:** every counter matched the control
   exactly — graphics-heap peak 96 B of 8,192, overflow 0, no-room 0,
   anim-cache misses 4 / rejects 0, `gNdsBattlePackHits` 0 on BOTH arms — and
   rebuilding the arm with Captain removed but every P2-3f9 change still active
   returns it to frame 49 in 10 s. `gNdsFtrPlanBuild`/`Hit` and the slot
   triangle mask all read 0 at frame 45 on BOTH arms, so they are not the
   instrument that will find it; start from a tick-HUD bucket census on a
   two-fighter Captain match, which is cheap enough to complete.
4. **`mpCommonProcFighterProject` still diverges from the source** -- see the
   open questions below; Falcon Dive is a caller and it is a SHARED seam.
5. **The remaining eight fighters each need `ftCommonAttack13Proc{Update,
   Interrupt}` aliases** (P2-3f5's finding) and their own
   `ndsRelocNormalizeFighterAttributesFile` arm (this row's finding 1).
6. **LUIGI HAS NO `FTAttributes` NORMALIZER ARM AND HE SHIPS.** Mario, Fox,
   Donkey and now Captain each have one; `NDS_RELOC_SYMBOL_LUIGI_MAIN_
   ATTRIBUTES` does not exist, so `LuigiMain`'s six mixed-u16 words keep the
   N64 lane order and his dead/damage/smash/heavy-get FGM ids and item-throw
   scales are byte-swapped pairs. Not fixed here on purpose: it is a Luigi
   defect, it wants its own row and its own before/after read of those six
   fields, and folding it into a Captain landing would make both harder to
   attribute. **Do read it before the next audio row** -- a wrong `smash_sfx`
   would look like a missing pack entry.

### Open questions

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
- **CLOSED by P2-3f8:** `gFTDataCaptainMainMotion` was NULL in every built
  configuration, so Falcon Dive's victim tether would have dereferenced
  `ndsRelocGetFileData`'s NULL return the first time a Falcon spawned without
  his files. The admission landed the asset graph BEFORE the CSS entry, in the
  order P2-3f5 required: `NDS_P2_CAPTAIN_CORE_ASSET_ROWS` reach
  `ndsRelocAssetIDForToken` / `ndsRelocAssetIsFighter` / the alloc-size table,
  and `ftManagerSetupFilesAllKind(nFTKindCaptain)` runs at
  `ndsMNPlayersVSPreviewInit` -- i.e. before any slot can hold him.

## Acceptance

- [x] Move inventory sweep vs `ftcaptain` data (19 statuses, 152 animations,
      160 NitroFS resources, source-derived).
- [x] Native-model DECODE unblocked (0xE2 + 0xF9 modelled; high detail decodes).
- [x] Runtime state machines land, link, and are verified strong on the ELF.
- [x] Native model ADMITTED as a runtime owner: slot 4, zero cross-matrix
      runs, geometry oracle green in both details, images staged (P2-3f8).
- [x] Falcon selectable: `NDS_P2_SHELL_ROSTER=3`, portrait + question-mark
      plate, HUD stock/portrait, Selected clip, preview residency (P2-3f8).
- [x] Per-kind arena cost measured and banked: 102,448 B standalone /
      100,160 B unique; +20,064 B of ARM9 image; owner images 18,800 B.
- [ ] **Audio: nothing at all is packed** (remaining item 1).
- [ ] Falcon Dive grab/release/regrab semantics equivalent.
- [ ] Fast-fall/landing/edge behavior spot-checks at speed extremes.
- [x] Four-distinct-kind budget answered and the hang fixed (P2-3f9): measured
      from the shell, 378,048 B reclaimed, whole-match low-water 419,052 B.
- [ ] The argmax roster is affordable but NOT playable: ~30x cost, item 7.
- [ ] Owner feel pass.

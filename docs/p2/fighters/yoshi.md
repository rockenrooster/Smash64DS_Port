# Yoshi — P2-3 fighter 7

Status: gameplay admitted behind `NDS_P2_YOSHI` (specials, articles, the two-body Egg Lay capture seam, effects, HUD/CSS surfaces; lab smoke green); next the audio bank, a human-input tour (Yoshi Bomb, egg breakout), stress · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftyoshi/`

## Role

The rules-exception fighter: unique shield, armored double jump, no third
jump. Exercises every "special-case the shared systems" seam.

## Moveset uniques

- **Egg Lay (B)**: swallows the opponent and turns them into an egg —
  opponent-side state (egg struggle/mash-out, damage-in-egg rules). Two-body
  state machine like DK cargo; reuse that seam pattern.
- **Egg Throw (Up-B)**: lobbed egg projectile, arc control; **no recovery
  height** — Yoshi's recovery is his armored double jump instead.
- **Yoshi Bomb (Down-B)**: ground-pound with star spray on landing.
- **Egg Shield**: shield is an egg — does not shrink like normal shields;
  its own poke/break rules (verify exact 64 semantics in source).
- **Double-jump armor**: heavy knockback armor during the second jump — the
  recovery identity; exact armor thresholds from source.

## Assets & audio

Round model, 6 costumes (verify count), Yoshi voice samples, egg VFX,
announcer clip.

## DS notes / risks

- Shield exception must live at the shared shield seam as a declared
  variant, not a Yoshi-local copy of shield code.
- Armor is a knockback-pipeline exception — verify it composes with items
  (Star invincibility, later) and Sudden Death.
- Egg (opponent) state must handle KO-in-egg, timer expiry, thrower KO'd.

## Source-derived inventory — 2026-09-02

- `BOOTSTRAP_FIGHTERS` gains Yoshi: the manifest carries 9 core files
  (`llYoshiMainFileID` 0xf7, Model 0x152, Special2 0x162, Special3 0x153,
  ShieldPose 0x154, MainMotion 0xf6 ...), 142 motion files
  (`FTYoshiAnim` 0x717..0x7a4, 2 event32 motion files, 19 item motion
  files), 151 nitrofs files, `attributes_offset` 1148 (0x47c: 247_YoshiMain.c's
  pre-attributes data ends with the 12-byte skeleton table at 0x470).
- `reloc_backend_assets.c` gains his rows behind `NDS_P2_YOSHI` (anim/core/
  dependency token rows, AObj32 test, payload/alloc size rows, attributes
  offset arm) and the source-literal FTAttributes pin: dead voice 595 /
  DeadSlam FGM 297, DeadUp 588, Damage 590, Smash 584..586, HeavyGet 593,
  item-throw scales 0x64 (REGION_US gmsound.h ordinals; his voice run is
  583 Appeal .. 602 UnkVocalize, announcer 535, crowd 614).
- Owner generator: `P2_O2R_ASSETS` (YoshiModel, file 0x152, sha e2654cbd),
  High JointTree 0x33a0 / Low 0x6948 (28 raw descriptors + sentinel),
  `dYoshiMain_setup_parts` 0xFBFFFFE0 (descriptor 26 omitted -- a bit-walk
  owner like Samus/Link), 27 selected parts + TopN, 18 drawable roots.
- **Blocker, the next row:** `dYoshiMain_commonparts_container` sets flags
  0x01 for both details, so `ftDisplayMainDrawDefault` draws EVERY Yoshi
  joint through case 1: `DObjDesc.dl` targets a `{ dls[0], dls[1] }` pair
  (338_YoshiModel.c:1626, 19 pairs at 0x3308) -- dls[0] draws under the
  parent's matrix before `gcPrepDObjMatrix`, dls[1] after. The decoder
  assumed one DL per joint and walks the pair table as commands ("root
  0x3308 exceeds 255 commands"). Only Yoshi (and NYoshi/NPikachu/Boss,
  P2-6) use the pair form. The generator needs a pair-aware root
  discovery: post-DL as the joint's root, pre-DL as an extra root under the
  parent joint's matrix (a Z-buffered DS makes the draw order irrelevant
  for opaque geometry). Until then he is out of `P2_OWNER_MODEL_CENSUS`.
- Shared seams already live in the port: the egg shield branches in
  `battleship_ftcommon_guard.c` (source guard1/guard2 included), the
  double-jump armor (`ftYoshiJumpAerialProcPhysics`,
  `FTYOSHI_JUMPAERIAL_KNOCKBACK_RESIST` 140 under REGION_US), and Falcon's
  capture TU pattern (`battleship_ftcommon_capturecaptain.c`) for the
  opponent-side `ftcommoncaptureyoshi.c`; `ftCommonCaptureYoshi*` /
  `ftCommonYoshiEgg*` are still `NDS_INACTIVE_STATUS_STUB`s.
- Effects: `efManagerYoshiShieldMakeEffect` and `efManagerEggBreakMakeEffect`
  are DS base effects already; `YoshiEntryEgg` (Special2), `YoshiEggLay`
  (Special3), `YoshiEggEscape` (Model) descs and `efManagerYoshiEggExplode`
  (a weak NULL stub today) are the admission's effect work.
- Audio: 36 source ids (FGM 82/115/130/252..257/297/308, voices 583..602,
  announcer 535 + team 531, crowd 614); Samus/Pikachu bank pattern.

## DL-pair decoder — 2026-09-02

- Both of Yoshi's pre-matrix DLs (descriptors 2 and 15, both children of
  descriptor 1) are pure `gSPVertex` loads with no triangles: the source
  welds a joint to its parent by loading vertices under the parent's matrix
  right before the joint's own DL draws them -- the same cross-binding the
  decoder already models for Pikachu's cache-persistence welds, made
  explicit. `OWNER_DL_PAIR_MODE` now carries it: `load_o2r_payload` appends
  one synthetic DL per welded joint (post DL's leading controls, the pre DL's
  vertex loads, the post DL from its first action) past the raw O2R bytes
  (raw sha pin untouched; every decoder reads roots by payload offset, so
  the welded stream has to be real bytes), the descriptor points at it, and
  `vertex_bindings` (a new empty-for-everyone-else export table) attributes
  those loads to the parent's binding through `build_dense_geometry`, so the
  per-corner matrix restores already do the rest. Exactness guards: the pre
  DL may draw no triangles and call no material, and its light movewords
  must equal the joint DL's (N64 lights at load time; the dropped syncs,
  combiner and texture state cannot reach a vertex load).
- Inventory green: High 27 state deltas / 88 sequence / 95 vertex actions /
  320 triangles / 51 runs / 34 epochs / 18 roots, 350 dense vertices, 960
  corners, hierarchy 1/6/6, **14 cross bindings** (palette slots 16..29) and
  122 per-corner restores; Low 26/82/72/201/40/32/18, 256 vertices, 603
  corners, 8 cross bindings (16..23), 62 restores. He is the heaviest weld
  owner so far (Pikachu: 11 bindings / 130 restores).
- Frozen owner program (`nds_native_fighter_owner.generated.inc`) regenerated
  byte-identical; the production manifest diff is Yoshi's new `native_model`
  block only.

## Native owner — 2026-09-02

- `P2_RUNTIME_OWNERS` gains `("yoshi", "NDS_P2_YOSHI")`; native owner slot
  **8**, image slot **6** (`nitro:/fighters/yoshi_{high,low}.bin`, High 27
  arrays / 5,741 elements, Low 3,879). Every runtime seam that named
  Pikachu's slot 7 now names Yoshi's 8 (owner tables, image path/size/
  verify, dense normals, joint schedule/binding tables, cross palette slots,
  adapter owner/model-id 0x152/profile owner, fighter-manager and
  CSS-preview image residency, Makefile image owner + build-config flag).
  Derived mechanically from the Pikachu owner patch (`derive_yoshi_owner_patch`).
- The owner program `src/nds/nds_native_fighter_owner.generated.inc` is
  gitignored and produced by `generate()` (`main()`/`--check` still stop at
  the standing f33 `hierarchy_locals` falsifier); regenerated with Yoshi's
  64 table symbols, frozen owners byte-identical. The tracked image header
  regenerated (`nds_native_fighter_image.generated.h`, +321 lines).
- GX palette: his 14 High cross slots reserve 16..29 in the adapter's shared
  union (`ndsRendererAdapterBuildGxSlotTable`), so parent slots allocate
  30 then 15 downward -- fine for his 1/6/6 hierarchy, but the first owner to
  push the union that far; verify the slot table on the owner probe.
- Default `smash64ds.nds` (flag off) builds clean. `NDS_P2_YOSHI=1` cannot
  link until the gameplay TU lands (the status table is promoted with it).

## Gameplay admission — 2026-09-02

- Three new TUs, all BattleShip verbatim behind `NDS_P2_YOSHI`:
  `battleship_yoshi.c` (ftyoshispecialn/hi/lw: Egg Lay grabber half, Egg
  Throw, Yoshi Bomb), `battleship_yoshi_weapons.c` (wpyoshieggthrow +
  wpyoshistar; reloc tokens EggThrow 0x0c / Star 0x40 in YoshiMain) and
  `battleship_ftcommon_captureyoshi.c` (the victim half: common statuses
  CaptureYoshi/YoshiEgg with the source's US breakout constants, plus a
  verbatim copy of `ftKirbySpecialNApplyCaptureDamage` the egg calls for its
  5% -- Kirby's TU is not built yet). The port ABI gained
  `FTPartsPlacement`, `ftCommonYoshiEggDesc` (moved out of efmanager's local
  copy), `wpYoshiWeaponVarsEggThrow`/`egg_throw`, Yoshi motion (13) and
  status (14) enums, his callbacks, FGM/voice ids 82..308 / 586..602, and
  `mpCommonCheckFighterCeilHeavy` (mpcommon.c:665, the ceiling-only sibling
  of the cliff shim). Status table promoted like Pikachu's; inactive stubs
  for the five common capture callbacks are now `#if !NDS_P2_YOSHI`.
- Seams: entry arm (AppearR/L + `efManagerYoshiEntryEggMakeEffect`), three
  effect descs on the roster list (EntryEgg/EggLay/EggEscape; deferred max
  28 -> 31), reloc rows + anim stem, proof guard `NDS_P2_PROOF_FIGHTER0 == 6`,
  YoshiMain weapon-attribute normalize/pin (egg map 150/0/-150/150, star
  100/0/-100/96, zero attack offsets) through the Pikachu helper, PlayersVS
  file setup + fkind filter, Makefile CFILES.
- Shell: HUD stock/portrait owner 8 (six stock LUTs 0xA9B0..0xAA78, sprite
  0xAAA8), CSS bake fkind 6 (portrait, Yoshi emblem, name text, gate token
  YOSHI -> `NDS_CSS_GATE_FIGHTERS` 9, asserts on `_HOLD_YOSHI`), fighter mask
  arm, Selected demo clip 444. Coverage audit PASS (allowlist regexes drop
  Yoshi). `generate_battle_hud.py` now parses the O2R extern table instead
  of guessing two header extents -- YoshiModel carries ten externs (0x64).
- **ACCEPTED DELTA (visual):** `wpYoshiEggThrowProcDisplay`'s RDP
  `gDPSetEnvColor(0,0,0,255)` is inert on the DS adapter, so the egg's
  environment colour is not forced to black.
- Smoke (`build-yoshi-cpu`, both-CPU Yoshi vs Fox, 3,600 frames, 12 stops):
  no abort, files resident, no fixup/resolve/spawn failures, owner reject
  0; events observed: entry egg t=0, Egg Throw x3 (spawn + hit), Egg Lay air
  x2 -> catch -> victim `nFTCommonStatusYoshiEgg` + egg effect. Yoshi Bomb
  and the victim breakout did not occur under CPU play: tour item. Packet
  counters `records=faults, hits=0` are the lab config's (Pikachu's ROM reads
  the same), not a Yoshi defect.
  `artifacts/visibility/2026-09-02_p2-3f43-yoshi-admission-battle.png`.
- CSS live on the shell ROM (`capture-p2-shell.ps1 -Only css-default
  -CssSeries 60,60,60,60`): his cell draws the source portrait under the
  in-progress plate, and the P1 gate shows the YOSHI name text and the Yoshi
  series emblem (`..._p2-3f43-yoshi-css-default.png`, `+246.png`).
- **Harness finding:** a ROM launched as `smash64ds.nds` under the DLDI-on
  melonDS resolves nitrofs through the stale SD-image copy (kit
  `PackReadFail=1`, blank shell); lab ROMs belong in `builds/<lab>/` under
  their target name. The worktree's `assets/menus` junction was replaced by a
  real copy so a Yoshi kit bake cannot write through to the main tree.

## Audio bank — 2026-09-02

- 35 cues in `fgm_phase_pack_ima` (258 -> 293 entries, 2,725,028 ->
  3,008,240 bytes, cache unchanged at 237,568): gmsound.h's complete
  `nSYAudio{FGM,Voice}Yoshi*` run (FGM 82/115/130/252..257/297/308, voices
  583..602), announcer 535, crowd 614, and the two shared cues his motion
  scripts are the first to request (ShellHit 56, CharacterUnkZip11 640, both
  new to the port header). EggShatter2/3, JumpAerial and the three Unk*
  voices are reached only by the Sound Test today and are packed now so that
  screen does not reopen the bank. Bare forks render their targets (Foot
  105, Dash 116, DeadSlam 287, DownBounce 298, HeavyGet -> JumpAerial 592,
  UnkZip11 -> 630); EggShatter2 253's fork 667 is fused. Min IMA SNR 23.7 dB,
  no fidelity debt. `build_pikachu_selectors` became
  `build_fighter_bank_selectors(kind, ...)`; Pikachu's pin is unchanged.
- **ACCEPTED DELTA (audio):** 596 FuraSleep is six notes over 968 ticks
  (5.6 s, mostly rests) and renders to 89,060 IMA bytes at 32 kHz, past the
  61,440-byte cache slot the generator guards; it is rendered as a 16 kHz
  body (44,532 bytes, SNR 25.2 dB) through `FULL_PROGRAM_AOT_OUTPUT_RATE_HZ`.
  The snore's content is an octave-down sample, so the band loss is
  inaudible in practice. The same lever would fit Falcon's omitted FuraSleep
  356 (65,324 bytes at 32 kHz, three notes of unequal pitch that
  `runtime_note_replay` cannot carry): open item for his row.
- `check-audio-fgm-phase-pack.ps1` PASS (293 ids, 0 exclusions); pins
  moved to resident 3,008,240 / mapping 0x341b5079 / sha 401f2941.
- Runtime smoke (`build-yoshi-cpu`, both-CPU, 3,600 frames): pack loaded,
  293 supported, 159 play calls all supported, 0 unsupported / playfail /
  lookupfail / miss-ring entries.

## Acceptance

- [ ] Move inventory sweep vs `ftyoshi` data.
- [ ] Egg Lay two-body matrix (mash-out, KO, edge cases) equivalent.
- [ ] Egg Shield + DJ armor thresholds equivalent.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.

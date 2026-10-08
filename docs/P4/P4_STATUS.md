# P4 status — Smash Remix fighters on the DS

Owner of the live P4 state. Plans live beside it ([master](New_Characters.md),
[handoff](IMPLEMENTATION_HANDOFF.md), [shared plans](shared/), [cards](characters/)).
The master plan's Revision 3 (2026-10-07, owner-requested rewrite for
implementation speed) orders the work as Board 1 (shared machinery S1-S13)
and Board 2 (fighter waves by measured cost); item status is tracked here
under those ids.

## P4.0 source admission (Falco checkpoint)

### 1. Donor build recipe and canonical hashes

`decomp/smashremix` stays read-only. `scripts/p4/stage_remix.py` checks the
submodule HEAD against [source-lock.json](source-lock.json) and a clean tree,
checks the input ROM's SHA1, exports the pinned tree with `git archive` into a
fresh ignored directory, then runs the donor's own tools exactly as
`xdelta - apply original.bat` and `patch.bat` do, adding only bass's `-sym`
symbol log:

```bash
python scripts/p4/stage_remix.py --rom "<vanilla SSB64 US z64>" --dest builds/p4-staging/remix-5e04fe7
```

| Input / output | SHA1 | SHA256 |
|---|---|---|
| Source ROM (US v1.0 z64, 16 MiB) | `e2929e10fccc0aa84e5776227e798abc07cedabf` | `15592e79…f3d` |
| `original.xdelta` | `e4c34f6aec7aebbd55fab6457cf364bc89a2b04f` | `e892db9f…e1d` |
| `xdelta.exe` / `assembler/bass.exe` | `bf12620a…` / `3a98ac38…` | `847f811f…` / `fc08bea1…` |
| `roms/original.z64` (asset ROM, 44 MiB) | `7ee97d815b33619225028f25c4b1867bac7eda47` | `cf2e1875…846` |
| `ssb64asm.z64` (reference/action ROM, 62.5 MiB) | `aceb63b18ebcd0ad36bf0470768d897e81f2d6f8` | `9deecb7d…a69` |
| `logfile.log` (bass symbols, 22,828 rows) | `5c239e922974c2a7bf2f9709dc7ca0a2005409bc` | `ee0e9f92…3c6` |

Two independent stagings of the same pins reproduce every output bit-for-bit
(2026-10-07). Roles: `original.z64` carries the donor's files (Remix grows the
reloc table in place at ROM 0x1AC870 to 5,455 files; data starts at 0x1BC830);
`ssb64asm.z64` + `logfile.log` carry the assembled character structs, action and
motion tables, per-kind tables and every inserted script. Full hashes are in
each staging's `staging-manifest.json`. All staging output is ROM-derived and
stays under the ignored `builds/` tree.

### 2. Resolved export and event graph

`scripts/p4/remix_export.py --staging … --fighter FALCO --out …` reads the
linked result, not loose `.bin` files: the character struct (Remix's struct is
FTData's layout), the effective motion-desc (param) array, the special
status-desc (action) array, the menu motions and all 44 per-kind tables Remix's
`define_character` copies from the parent, each diffed against the parent.
Vanilla callbacks are named from `decomp/.../symbols/symbols_us.txt`, donor
ones from the bass log; an unnamed callback is a failure.

The event decoder walks every motion root in two address spaces (vanilla motion
files, whose pointers are reloc slots, and Remix RAM inserts, whose pointers are
absolute), with per-opcode lengths from `ftMainParseMotionEvent` and Remix's
custom 0xD0..0xDC commands from `src/Command.asm`. Unknown commands, pointers
outside a known region and zero-time loops are explicit failures.

Falco: Remix kind 0x1D, parent Fox. 219 motion descs (8 own animations, 37 own
scripts), 246 statuses of which only 0xE1/0xE2 (Phantasm) replace callbacks,
186 event blocks, 3 THROW_DATA targets, 5 self loops and exactly one
fall-through (USP_GROUND_MOVE into USP_LOOP). File closure 180: 11 donor-only
files; one vanilla ID with donor bytes, FoxSpecial3 0xA1, classified
*equivalent* (Remix inlines ExternDataBank109+0x19F8 in place of three external
references; the fixture proves the bytes equal). 0 failures.

### 3. Lowering into the existing DS pipeline

`scripts/p4/generate_p4_fighter.py` turns an export into the inputs the DS
reloc loader and fighter code already consume:

- **O2R containers** in Torch's `SSB64:RELOC` layout. The writer is
  byte-identical to Torch's own export on all 2,132 vanilla files.
- **A synthesized main-motion file** (DS ID 0x1600): the parent's motion file
  followed by every reachable Remix-inserted stream and its data, each absolute
  pointer turned into an internal relocation on one rebuilt chain. Falco's is
  10,072 B (Fox's 6,816 + the donor interval 0x8053F6A8..0x80540360). Every one
  of the 198 roots re-decodes inside the synthesized file to the identical
  command stream (round-trip check in the generator).
- **Generated C** (built into `$(BUILD)/p4`, never tracked): FTData, the motion
  and menu-motion descriptors, the status-callback overrides, the asset paths,
  the content's animation list and its `ftManagerSetupFileSize` answers.

### 4. Identity: the smallest safe seam

A P4 fighter keeps `fp->fkind` = its donor's setup parent and carries its
content in `fp->nds_p4_content` (the DS-only FTStruct tail, now 3,016 B). This
is Remix's own semantics: `define_character` copies the parent's row of every
kind-indexed table, so every BattleShip `table[fkind]` read stays in bounds and
resolves exactly as Remix does, and no legacy kind, sentinel, save byte or mask
moves. What the donor replaces is routed by content
(`include/nds/nds_p4.h`, `src/port/nds_p4.c`):

| Seam | Where |
|---|---|
| FTData of a P4 player (files, attributes, motion scripts) | `src/import/battleship_ftmanager.c`: a view of `dFTManagerDataFiles` with the parent's row swapped while that player's files load or its fighter is built |
| Replaced status callbacks + donor init hooks | `ftMainSetStatus` epilogue (`src/import/battleship_ftmain.c`) |
| Remix custom motion commands (0xD0 FSM, 0xD3 TopN translation multiplier; others counted) | the event-kind read of the three motion loops, as Remix's `load_command_` |
| TopN translation multiplier | `ftPhysicsApplyGroundVelTransN` / `ftPhysicsGetAirVelTransN` (`src/port/reloc_backend_compat_shims.c`) |
| Donor asset paths, animation and attribute normalization | `src/nds/nds_reloc_assets.c`, `src/port/reloc_backend_assets.c` |
| Parent code patched per donor (Falco's Fire Bird delay 0x16 and speed 98) | `src/import/battleship_fox_special_hi.c` |

Explicit parent comparisons in vanilla code (`fkind == nFTKindFox`) therefore
hold for Falco; Remix makes the same choice for every check it table-izes with
parent copies (jab 3, rapid jab, entry, thrown/capture IDs). Remaining
parent-specific checks are audited per fighter on its card.

### 5. Candidate risk census (main Remix tree)

`remix_export.py` on all 14 main-tree candidates (2026-10-07):

| Fighter | Parent | Own anims | Own scripts | Callback overrides | Donor-only files | Export failures |
|---|---|---:|---:|---:|---:|---|
| Falco | Fox | 8 | 37 | 2 | 11 | 0 |
| Ganondorf | Captain | 14 | 54 | 0 | 15 | 0 |
| Wolf | Fox | 125 | 52 | 10 | 97 | 0 |
| Wario | Mario | 59 | 58 | 8 | 56 | 0 |
| Bowser | Yoshi | 193 | 60 | 9 | 150 | 0 |
| Marth | Captain | 211 | 124 | 26 | 162 | 0 |
| Roy | Captain | 213 | 126 | 28 | 163 | 0 |
| Sheik | Captain | 165 | 75 | 21 | 132 | 0 |
| Peach | Fox | 201 | 106 | 13 | 154 | 0 |
| Dedede | Captain | 227 | 128 | 38 | 163 | 0 |
| Sonic | Fox | 118 | 100 | 20 | 96 | 0 |
| Banjo | Captain | 206 | 89 | 19 | 162 | 0 |
| Crash | Mario | 201 | 99 | 13 | 160 | 0 |
| Lanky | Mario | 216 | 101 | 23 | 170 | 0 |

The four "vanilla-RAM" failures were one bug (fixed 2026-10-07): menu motions
may point straight at a vanilla menu script in overlay 1 (Ganondorf's claps
use Captain's `D_ovl1_80391E84`, a lone END), and the reader mapped every
non-Remix address with the fighter overlay's RAM-minus-ROM constant, so it
decoded unrelated bytes. Overlay 1's range (ROM 0x1079C0-0x109FB0 at VRAM
0x803903E0, `smashbrothers.us.yaml`) now reads through its own constant, and
the generator copies such a script like a Remix stream. Metal Mario is the
existing MMario kind. Meta Knight, MRGAW and Snake come from the EXTRA tree,
whose nested Remix gitlink is not yet initialized here; their canary is open.

### 6. Fixtures and negative controls

`python scripts/p4/test_remix_adapter.py` (needs a local staging): Falco export
clean; Phantasm overrides vs reflector inheritance; -1/-2 motion sentinels;
loops, the single fall-through and the three throw pointers; rejection of an
unknown command, an unported custom command, a goto into vanilla RAM, a missing
inherited file and an unclassified donor-modified vanilla file; the source-lock
pin; Torch byte-identity of the O2R writer; the 0xA1 equivalence proof.

## P4.1 Falco

Build: `NDS_P4_FALCO=1` (with a local staging and export; the Makefile runs the
export and generator into `$(BUILD)/p4`). Lab: `gNdsLabP4Content` (one byte a
slot) puts a content on its parent in the four-CPU sweep ROM.

Content list (S1). `scripts/p4/contents.json` is the registry: id, name,
Remix name, title. The id is stable: it is saved, sent in the P3 descriptor
and sits in the selection id 0x40 + id. Adding a fighter takes four things:
- a registry row;
- `NDS_P4_<NAME>=1` on the build;
- its owner pins (`scripts/p4/owners/<name>.json`, `p4_native_owner.py --learn`);
- its native code in `src/port/nds_p4_<name>.c`, only if it has any.

No shared file names a fighter. The Makefile reads the registry and runs one
rule template per enabled content: export, generator, NitroFS staging, preview
pack and native images. The native-owner generator runs once for all of them.
`scripts/p4/p4_contents.py` writes `NDS_P4_CONTENT_ROWS(X)`, which these
shared sites expand:
- the content enum and fighter rows (`nds_p4.c`; per-content hooks are weak);
- the renderer's owner slots and profile owners (`nds_renderer.h`);
- the owner tables, binds and lookups (`nds_renderer_assets.c`,
  `nds_renderer_native_common.c`, `renderer_adapter_fighter.c`);
- the select's owner images and the battle image slots.

All contents' data share one TU (`nds_p4_data.c`).

Native ports (`src/port/nds_p4_falco.c`): Phantasm ground/air interrupt, air
physics and air map; the INITIAL_SETUP seed; Fire Bird launch delay and speed.
Phantasm's landing lag is 0x3EB35C29 (the copied Mario routine keeps its
`ori 0x5C29` low half), not the commented 0.35.

Native owner (renderer slot 25, `NDS_NATIVE_IMAGE_SLOT_FALCO`): his model
0x8AC goes through the existing owner generators in a donor mode
(`P4_DONOR_OWNERS` in `scripts/fighters/generate_nds_native_owners.py`, driven
by `scripts/p4/p4_native_owner.py` with the pins in `scripts/p4/owners/falco.json`).
Remix models differ from the vanilla cast in two ways the vanilla decoder
refused, and the donor mode accepts both:

- A joint's list can END with a vertex load that the next joint's triangles
  consume (cross-joint skinning). The load becomes a zero-length action epoch;
  the four bindings are pinned as cross bindings (joints 2/3/6/7 into
  16/17/18/19).
- A joint's list can be a bare G_ENDDL (Falco's joints 11 and 29). It is no
  root in the owner, and the fighter display contract records no event for it
  on a P4 fighter (`ndsFighterDisplayContractSelectDL`), so the drawn root set
  is the owner's 16 roots.

Both details draw through the lean path with 0 declines and 0 native failures
(four-CPU low detail and two-fighter high detail, Peach's Castle, captures in
`artifacts/visibility/2026-10-07_labprobe/p4a7`, `p4b1`, local). Images:
`falco_high.bin` 20,768 B, `falco_low.bin` 16,556 B.

Presentation, all generated per content from the donor's own rows:

- **Battle HUD**: the stock icon is baked exactly as the original cast's
  (`generate_battle_hud.stock_asset` on the content's model file, its costume
  LUTs from FTSprites), and the FTSprites sprites join the reloc loader's
  sprite normalization, so the damage meter's series emblem bakes at runtime
  like everyone else's (`src/nds/nds_battle_hud.c`, `reloc_backend_assets.c`).
- **VS Results**: name, "WINS!" position, announcer call and victory music from
  `add_to_results_screen` (Falco: "FALCO", 30/170, FGM 0x2D6, BGM 0x45); the
  podium fighters are made with the content, so the victory poses are his.
  The four seams are zero-argument source functions the import TU splits on
  their argument text (definition `f(void)` keeps the base, calls go to the
  port wrapper). VS records skip P4 players until the records tier (they
  would otherwise accumulate under the parent's row).
- **Character select** (P4 builds): Smash Remix's own 10x3 grid of 24-pixel
  cells (CharacterSelect.asm) with its CHARACTER_PORTRAITS art; the originals
  keep their 6x2 block, and the six Remix selections outside the P4 roster
  (Marina, Dr. Mario, Young Link, Goemon, Conker, Mewtwo) give their cells to
  Metal Mario, Meta Knight, MRGAW, Snake, Lanky and Roy. Uncompiled
  selections show the NONE plate. Tokens scale with the portraits (0.8125).
  Gates bake each compiled content's series emblem and name from the donor's
  extended FTEmblemSprites/MNPlayersCommon (0x14/0x11); every cell flashes.
  A slot's selection id is an original's fkind or `NDS_P4_SEL_BASE` + content,
  committed as the parent kind plus `NdsMatchFighterConfig.p4_content`, which
  `ndsMatchConfigApply` publishes and the P3 descriptor carries (bytes 46-49).
  Generated by `scripts/p4/p4_css.py` (tables + spec) and
  `generate_mn_ui_kit.py` (`NDS_P4_CSS`).
- **Character-select 3D preview** (S9): a content gets the original
  cast's compact preview transaction. `scripts/p4/p4_preview_pack.py`
  builds its FPC1 pack (kind `NDS_P4_SEL_BASE` + content, NitroFS
  `fighters/preview/<kind>.fpc`). Main is copied whole. Model keeps
  everything but its geometry, and every pruned list becomes a root cell
  (ENDDL plus its original offset) the native owner looks up. Falco's
  pack is 28,240 B. The loader keeps a resident row per content after
  the twelve (`reloc_preview_pack.c`). The commit loads the pack, then
  the content's motion file for its menu scripts
  (`ndsFTManagerSetupPreviewFilesP4`). The panel's preview kind maps to
  the parent kind plus the slot's preview content (`ndsP4MakeContent`).
  A finished match's selection never reaches the menus or the 1P modes
  (`ndsP4MatchContent`: VS battle and Results only). The VS select's
  animation-cache reservation adds each content's own menu clips (rows
  0-4): 88,992 B with Falco, against 52,000 B for the twelve. Scale
  comes from Remix's `menu_zoom` row (1.2 for Falco), on the select
  screen and the Results podium. A donor joint whose list is a bare
  G_ENDDL is packed as ENDDL plus 0, which the display contract skips;
  an ENDDL with an offset is a root cell and stays an event. The first
  build skipped every cell, so the panel was empty. Menu ROM: Falco idle
  and selected, both details, 0 fail masks, 0 arena overflows (capture
  `artifacts/visibility/2026-10-07_p4-css-falco4`, local). Mario, Fox
  and Kirby previews are unchanged on the P4 and P4-off menu ROMs.
- **Sounds**: the content's Remix sounds become a second FGM pack,
  `p4/audio/fgm_p4.bin` (`scripts/p4/p4_audio.py`, built into `$(BUILD)/p4/audio`).
  FGM.asm adds each with `add_sound(name, rate, type, reverb, length)`: a
  VADPCM `.aifc` in the donor tree, an FGM id (bass.out's "Added" lines) and
  one of three microcode templates. BattleShip's own `extract_fgm` decoders
  read the templates as ordinary UCD and articulation programs -- a voice is
  pitch -1200 at full volume, a chant is PublicFox's program, the sleep cue
  three cut notes -- so the channel rate, volume, pause bit and IMA encoding
  come from the vanilla pack generator's formulas. Remix's automatic note
  length (file size / 177) is shorter than most samples; its UCD does not cut
  at the note's end (`set_unk1E` 36, bit 7 clear), so the voice rings out on
  the N64, and the DS cue plays the whole sample. Falco: 15 cues, 0x2C8 crowd
  chant through 0x2D6 announcer, 119 KB. `nds_audio_fgm.c` loads the pack's
  entries after the vanilla pack's and looks donor ids up there; bodies read
  from its NitroFS file, synchronously or through the ARM7 like the vanilla
  cues (`gNdsAudioFgmP4Result`, `gNdsAudioFgmP4Count`).
  In a four-Falco lab match (1,500 frames): the pack loaded with 15 cues,
  69 plays of donor cues, 0 FGM misses, 0 play failures.
- **Victory music**: a content whose `winner_bgm` is a Remix song (id at or
  above the vanilla 0x2F) gets it rendered by the vanilla BGM renderer
  (`render-audio-bgm.py --sequence-file`) from the donor's compressed
  sequence (bass.out: "Added MIDI_<NAME>(<path>)", "MIDI_<NAME>_ID") on the
  vanilla sequence bank, and `nds_audio_bgm.c` appends its track row in P4
  builds (`nds_p4_bgm.generated.inc`). Falco's FALCO_VICTORY (0x45) uses
  programs 5-31, all vanilla instruments: 170 notes, 8.4 s, 92 KB. In a
  four-Falco lab match the Results screen plays it (track 0x45, streaming, 0
  BGM errors).
- **Costumes**: the alternates draw through the native owner like the
  original cast's (four Falcos in four costumes, lab capture `p4cos1`);
  shields keep the port's colour, as vanilla does (Remix's costume-matched
  shields are its own option).

- **CPU**: Falco's five CPU rows are ported (`NDSP4Computer` in
  `include/nds/nds_p4.h`; a NULL row keeps the parent's vanilla path).
  `ai_behaviour`: his attack list (Falco/AI/Attacks.asm CPU_ATTACKS, read
  back from the assembled table) replaces Fox's in
  `ftComputerCheckDetectTarget` (`battleship_ftcomputer_fixed.c`).
  `ai_attack_prevent` FALCO_NSP: an aerial Phantasm gets the ledge-ground
  test the parent switch gives recovery specials; up special goes to
  FOX_USP, which acts at level 10 only. `ai_long_range` NONE: no long-range
  laser (Fox's row is NSP_SHOOT); the source's lead-in and its two random
  draws still run. `recovery_logic` (Falco.asm), after the recover
  objective's walk: in the air Phantasm hold B; otherwise, with the nearer
  ledge under 2000 units away in X, the fighter below it and the ledge-grab
  box reaching it, one time in eight target the ledge and Phantasm toward
  it with Remix's NSP_TOWARDS routine (it releases Z and B first and ends a
  tick sooner than vanilla script 9, which the first port used).
  `cpu_post_process`, after the objective: in Fire Bird over ground, drop
  the command and hold the stick down to land; drop a Phantasm issued over
  ground when no ground lies 2000 units ahead toward the target; before it,
  AI.asm's shared check drops a dash attack that cannot start (not standing
  or running). Remix's added input routines keep their ids and assembled
  bytes (MULTI_SHINE 0x3A, NSP_TOWARDS 0x42, FAIR 0x43, BAIR 0x44,
  DASH_ATTACK 0x47, NULL 0x4C, `src/port/nds_p4.c`); FAIR/BAIR's
  forward/back stick values (AI.asm extend_stick_x_commands) are resolved
  for Remix routines only, after the run that sets them. Remix's level-10
  rows and its improved-AI toggle are not ported (the port's CPUs stop at
  level 9). Four CPU Falcos (lab, Dream Land, 1,900 frames): the attack
  list picked dash attack 65 times, back air 51, forward air 9, multi-shine
  1; every forward/back air stick matched the facing (60 runs); 183
  long-range lead-ins returned without a laser; 826 aerial Phantasm picks
  took the ground test; 5 recovery Phantasms; the shared check dropped 64
  dash attacks. The Fire Bird landing and the Phantasm drop did not occur
  on that stage.

- **Sword trails** (S13): Remix's SwordTrail.asm rows extend the vanilla
  afterimage. SET AFTERIMAGE's `is_itemswing` 0 is Link's sword, 1 the Beam
  Sword swing, and 2 on index Remix's `sword_trail_table`. The export reads
  that table (`sword_trails`: character, model part, axis, two colours,
  start and end). The generator emits each content's rows
  (`NDSP4SwordTrail`). Falco's ground and air Phantasm scripts set ids 15
  and 14 (model parts 1 and 0, the Y axis, cyan, -150..250).
  - The update: ftMainProcParams' afterimage switch has the vanilla cases
    only. The P4 wrapper (`battleship_ftmain.c`) runs the Link-sword step on
    the row's joint (model part + 4) and matrix row (`ndsP4UpdateSwordTrail`),
    under the source's gate.
  - The draw: no afterimage drew at all before this, Link's sword and the
    Beam Sword included. The whole fighter display runs inside the contract
    capture, whose lists are dropped. `ndsFighterDrawAfterImage`
    (`renderer_adapter_fighter.c`) is the source routine's vertex build with
    its own conversions. It is submitted natively after the fighter
    (`ndsRendererSubmitAfterImage`) on the camera with a translation to the
    first vertex, as the Fox gun overlay composes. The source routine is
    skipped in the capture.
  - DS approximation: one alpha per polygon, so each quad takes the mean of
    its two edges. The strip folding over itself blends once, not twice.
  - Lab, Dream Land, 1,200 frames: four Falcos made 27 row updates and 11
    trail draws; four Links made 278 sword-trail draws; 0 rejects. Captures
    are local: `artifacts/visibility/2026-10-07_trail-trail2.png` (Falco)
    and `trail-trail4.png` (Link).

Open for Falco: Kirby's copy (S8: Kirby takes the parent's copy, Fox's
blaster and hat; Remix gives Kirby Falco's Phantasm as statuses 0xEB/0xEC
with Kirby's own FALCO_NSP animations and scripts, kirby_falco_trail, and hat
0x12 from its extended Kirby file), the acceptance probe's witness block
(S10).

All-content memory on the VS select (2026-10-07). The select reserves every
compiled kind's menu clips before its previews load: 52,000 B for the twelve,
88,992 B with Falco. It then keeps 104 KiB free (`NDS_R2_ANIM_CACHE_CSS_KEEP_FREE`:
the four slots' figatree heaps, the surface cache and four previews' objects).
The all-content walk ROM reaches that reservation with 176,352 B free. So a P4 build with the full original content
cannot hold even Falco's clips there. The fix belongs to S9: load a content's
menu clips with its preview transaction, into its slot block, instead of
into the global reservation.

## Whole roster in the lab (S11, S14; 2026-10-07)

- **Own action arrays (S14).** The generator emits each content's whole
  special-status table from Remix's action array: motion, motion attack,
  status flags and the four callbacks, appended statuses included. A
  callback the content keeps from its parent is `NDS_P4_PROC_INHERIT`,
  filled from the parent's own table the first time the table is used, so
  the generated file never names a parent routine. `battleship_ftmain.c`
  renames the source's per-kind table to a view
  (`gNdsFTMainSpecialStatusDescsView`); for one `ftMainSetStatus` call the
  fighter's row holds its content's table (or, inside another fighter's
  call, the source table) and is put back after. This replaces the list of
  status overrides. Falco, four CPUs, 1,800 frames: 0 skipped draws, 0
  declines, trails drawn, his Phantasm callbacks ran from the table (266
  air-physics calls), inherited slots resolve to Fox's routines.
- **GO TO MOVESET FILE (S4).** Remix's 0xDB continues a script at an offset
  into the fighter's loaded main motion file. The synthesized motion file
  keeps the parent file's bytes at their offsets, so the command runs as is
  (`ndsP4RunRemixMotionEvents`); the generator checks the target lies in
  the file. Marth, Roy, Crash, Lanky, Banjo, Sonic and Dedede use it.
- **The other motion commands (S4).** `ndsP4RunRemixMotionEvents` runs
  every command the roster uses but 0xD9 SET ENV COLOR, each on the field
  the N64 offset in `Command.asm` names: 0xD1 armour is
  `knockback_resist_status` (Yoshi's double-jump field, Ganondorf), 0xD4
  `vel_air.y`, 0xD5 `is_fastfall` (Banjo), 0xD7 jumps used and ground/air
  (Wario), 0xDA the facing (Marth, Roy). 0xD6 RANDOM SFX draws two
  `syUtilsRandIntRange` numbers in Remix's order and reads its id table from
  the synthesized motion file, word-swapped like the rest of it (Lanky,
  Marth); 0xDC L VOICE is unused. 0xD2 and 0xD8 keep a launch direction and
  a hit sound per port and hitbox (Sonic, Wario, Banjo; Marth, Roy, Peach):
  ftmain's make-hitbox reads clear that hitbox's pair, as Remix's
  `create_hitbox_` does (the clear-all reset changes nothing a made hitbox
  reads); ftMainPlayHitSFX's sound call and the stats call after the
  victim's `damage_lr` in ftMainProcessHitCollisionStatsMain go through
  `ndsP4MakeHitPositionFGM` and `ndsP4UpdateHitDamageStats`, Remix's
  `apply_fgm_` and `apply_direction_`. The fast-forward loops skip 0xD4,
  0xD5, 0xD6, 0xDA and 0xDC like Remix's second table.
- **CPU rows (S7).** The generator reads each content's attack list where
  `ai_behaviour` points (rows of the source's `FTComputerAttack`, grounded
  then aerial, each list ending in -1), the Remix input routines those rows
  name from `AI.command_table` (ids from 0x31; each routine's bytes up to
  its last END before the padding: Falco's match the hand-copied arrays),
  and `ai_long_range`, which is one of two cases of the source's fkind
  switch in `func_ovl3_80138AA8`: none, or the projectile users' walk and
  shoot. Banjo, Dedede and Sheik take the projectile case under Captain,
  whose own is none, so `battleship_ftcomputer.c` carries that case's
  body after the source's lead-in. `NDSP4Computer` keeps only the
  hand-ported routines (attack-prevent, recovery, post-process).
- **Generation.** All 14 generate with `NDS_P4_LAB_FALLBACK=1`: a status
  with a donor routine that has no port runs a plain end-of-motion set in
  all four slots, and each such routine is listed in the manifest. The
  parent's routines were tried first and are unsafe on a donor's script:
  Captain's Falcon Dive procs on Marth's Dolphin Slash caught a fighter the
  script never gave throw data (fault at the release), and Mario's Super
  Jump Punch interrupt faulted under Lanky. Unported routine slots: Falco 0,
  Ganondorf 0, Bowser 9, Wario 20, Wolf 22, Peach 24, Sonic 31, Crash 32,
  Banjo 36, Sheik 37, Dedede 39, Lanky 42, Marth 44, Roy 52.
- **Native owners.** Every content now has owner pins
  (`scripts/p4/owners/<name>.json`). Remix models needed five additions to
  the owner generator, each limited to what the source does:
  - combiner `FC121805/FF17FFFF` (TEXEL0*SHADE with TEXEL0a*SHADEa alpha) is
    an alias of family 0 under the per-triangle SHADEa == 1 proof Pikachu's
    alias uses (Ganondorf, Roy, Wario, Crash, Sheik);
  - the light pair set twice before the first vertex load: the RSP lights
    at G_VTX, so the replaced layout lights nothing (Marth, Lanky, Banjo,
    Dedede, Ganondorf);
  - triangles whose three vertices an earlier joint loaded: cross runs with
    no current-joint corner, which still restore the current slot (Wolf,
    Sonic);
  - Bowser's model keeps Yoshi's `Gfx *dls[2]` pair form (FTCommonPart flag
    bit 0), now read from the donor's main file;
  - two material lists in one span (a palette MObj, then the texture MObj:
    Peach, Bowser): every call but the last closes a material-only epoch,
    so both apply in source order with the runtime's existing epoch format;
  - every alternate model part a donor's joints carry is a root variant
    (`donor_variants`): a P4 fighter's parts are set by its own scripts and by
    its parent's code (Yoshi's specials open Bowser's jaw), so the owner
    cannot list only the parts a script names. The renderer resolves them
    per content (`NDS_P4_NATIVE_<NAME>_ROOT_VARIANTS`). Without them every
    model-part swap declined the draw (Bowser: 36 per 1,200 frames). Learn
    gives a variant-only cross binding the next GX slot, and checks restores
    on the full owner, not the canonical-only inventory. Crash's owner
    exceeds the packed corner's 11-bit dense ID with all of his: learn
    leaves out the largest parts no script of his sets (two, in his pins);
  - Remix part lists that re-set the light pair after their last triangle
    (Crash, Sheik, Banjo, Dedede): dead, since every root opens with its own
    prefix (checked);
  - one root-light preamble table per owner, the union of both details,
    as the original cast's emission already does. Wario's low model has a
    preamble his high one lacks, so every low draw declined (4,202 in 1,200
    frames).
- **Sounds.** `p4_audio.py` packs every enabled content's cues in one pack
  (325 cues, 2.8 MB, 13 victory songs; Roy shares Marth's). A name Remix
  adds several times (Ganondorf's ten placeholders) takes its ids in
  declaration order. The runtime's P4 table is sized by the build's pack and
  searched by id. Ganondorf's victory laugh (88 KB) exceeds two async reads
  and is read synchronously.
- **Select screen and previews.** The CSS generator and the preview packs
  read the registry; Bowser's pack keeps his pair arrays as data.
- **Entry (an S3 seam).** Remix's `entry_action` and `entry_script` rows
  pick each fighter's appear statuses and which case of the fkind switch in
  `ftCommonAppearSetStatus` runs: another kind's case (Bowser takes the
  Blue Falcon case for his Clown Copter, Crash Samus's), none (Ganondorf),
  or a Remix routine (Marth). Remix then patches the shared makers to read
  the fighter's own files (Wolf's Wolfen inside Fox's Arwing maker). The
  generator emits `NDSP4Entry`: the content's appear statuses, and either no
  effect or the parent's own case when the content ships the parent's
  special files (Falco). Anything else needs a port; lab builds enter with
  no effect and list it. Before this, Ganondorf's and Wolf's matches hung
  at the first appear: the parent's maker read the donor's files at the
  parent's offsets.
- **Special files.** A content's special files 1-4 load into its parent's
  globals, which the parent's code reads at the parent's offsets. That is
  exact when the donor's file has the parent's layout: Remix points the
  parent's readers at a re-skinned copy (Ganondorf's Falcon Kick reads his
  own file at Captain's offsets, `captainshared.asm kick_anim_struct`), and
  for a slot the parent has no file in. Every other differing file (Wolf's
  four, Bowser's, Marth's, Crash's ...) needs its own storage and ported
  readers (S5): lab builds load the parent's file there and list it,
  shipping builds refuse. The source loader takes specials from the main
  file's extern closure, so a stand-in is loaded whole
  (`ndsP4LoadOpenSpecialFiles`). A donor file in a slot the parent has no
  storage for (Sheik's special1, Banjo's special4, Dedede's special1 under
  Captain) is read by no parent code, but loading it wrote through a NULL
  slot pointer at fighter setup; lab builds drop it, shipping builds need the
  content's own storage.
- **Own special files and articles (S6).** A slot listed in the generator's
  `OWN_SPECIAL_FILES` loads into the content's own storage
  (`gNdsP4<Title>Special<n>`) once every reader that runs for the content is
  ported: Remix's hooks on the character id become `gNdsP4<Title>Overrides`
  (the parent's effect makers the content replaces: Fox's reflector and
  Arwing entry for Wolf), and the content's own weapons and effects live in
  `nds_p4_<name>.c` (Wolf's blaster shot, slash, reflector and Wolfen).
  `scripts/p4/p4_articles.py` resolves each article Remix describes (a
  DObjDesc tree and its DObjDLLinks, a display list with a TEXID material,
  a weapon's attributes, a model part outside the model file) to
  display-list roots in the content's files; `generate_nds_entry_effects.py
  --p4` appends them after the original roots into the build's packet
  (`nds_p4_entry_effects.generated.inc`, never tracked), compiling an
  article's lists in the RDP's head order so state carries as it does on the
  N64, CI8 included. The renderer admits a root as its content file plus
  offset (effect and weapon GObjs only), selects a TEXID frame from the live
  MObj, draws a state-only list as nothing, prepares a content's textures
  only in a match that has it, and draws a held gun from another file beside
  the body at Fox's hold joint (Wolf's).
- **Parent-kind compares.** A content carries its parent's fkind, so every
  source compare with the parent's kind (`fkind == nFTKindYoshi`) that Remix
  leaves alone takes the parent's branch on the DS, where the Remix
  fighter's own id took the other one on the N64: Bowser raised Yoshi's egg
  shield, hid the fighters he grabbed and threw, jumped with Yoshi's
  double-jump armour, burst an egg on shield break, and as a CPU never used
  his up special to recover. `scripts/p4/parent_checks.py --staging <remix
  staging>` lists, per parent, each such compare in battle code, whether its
  branch instruction is still the original in Remix's build ("raw") or
  jumps to a Remix hook, and the character ids the hook's source names
  (Remix's YoshiShared hooks add J Yoshi only). Tables Remix builds with
  `add_to_table(..., id.{parent})` do give a content its parent's row: its
  CPU attack-prevent row is Yoshi's, as the DS already had. The fix is per
  decomp include: the wrapper redefines the parent's constant as
  `NDS_P4_PARENT_KIND(fp, nFTKindYoshi)` (nds_p4.h), so a content compares
  unequal, around the guard, escape, catch, capture, thrown, aerial-jump and
  shield-break files. ftcomputer.c also holds a `case nFTKindYoshi:` such a
  view cannot reach, so the objective walk runs for a content with its fkind
  past every vanilla kind (`NDS_P4_FOREIGN_FKIND`); its fkind reads are
  three compares and its callees read none. Before writing the tool,
  `remix_rom.read_ram` read overlay 3 (ft/ftcommon, ft/ftcomputer: VRAM
  0x80131B00) through overlay 2's RAM-ROM constant, 0xDC0 bytes off, and the
  decomp symbols of the menu overlays that share that VRAM cut battle
  functions short; both are fixed.
- **Jab tables (an S3 seam).** Remix replaced the source's kind tests for
  the third jab and the rapid jab with tables indexed by the character id
  (Character.asm `jab_3`, `jab_3_timer`, `jab_3_action`, `rapid_jab` and
  the rapid jab's begin, loop, ending and press-count rows) and left the
  rest alone: the third-jab test in `ftCommonAttack1CheckInterruptCommon`,
  the Captain tests in the Attack12/13 updates, the Pikachu and Kirby tests.
  The generator reads each content's eight rows into `NDSP4Jab` (Lanky's
  42-frame window, Bowser's, Banjo's and Dedede's own third-jab statuses,
  Sheik's rapid jab on Fox's rows). A content runs the two files' P4 copy
  (`src/import/battleship_ftcommon_jab_p4.c`): the files compiled again with
  every kind they test redefined past the real kinds, so the raw tests fail
  as the content's id does, and with the seven tabled functions replaced by
  versions that read the rows; the source's copies send a content there. A
  DISABLED row whose N64 path reads a stale register or stack word for the
  status (a third jab with no action row: Wario, Crash, Marth, Roy; a rapid
  jab with no count row: Marth, Roy, Peach) starts nothing. Lab check
  ("p4q", four Bowsers with every slot's A forced on alternate ticks,
  `gNdsLabForceInputSlots`): 89 first jabs, 85 second, 81 third (0xE9), 0
  native failures.
- **Remix's CPU commands.** A content's CPU runs every input script through
  the port's copy of the source interpreter (`ndsP4ComputerRunInputs`),
  which adds Remix's extensions: the stick-X values 0x81-0x84 (away from
  the target, forward and back from the facing; this replaces the earlier
  pre-decode of a run's last stick write) and the 0xFE custom commands
  (wait out the jump squat or a turn, press or release C, point the stick
  at the target). The source's interpreter read 0xFE as a no-op and the
  index after it as a command (index 1 is an A press), so Bowser's
  short-hop Fire Breath, Crash's and Sonic's routines misfired. The
  interpreter's Fox up-special test takes a content only where Remix's
  `usp_check_` does (Falco, `NDSP4Computer.fox_usp_check`). Lab ("p4q",
  four Bowsers): 4 C presses and 242 stick-to-target steps.
- **yoshi_egg (an S3 seam).** The egg Yoshi lays around a fighter is the
  fighter's row (`dFTCommonYoshiEggDamageCollDescs` by id): Bowser's and
  Dedede's are larger. A content's row is generated and lent at its kind's
  index around the egg's effect maker and its hurtbox setup
  (`battleship_ftcommon_captureyoshi.c`). Lab ("p4q", Yoshi with B forced
  every 30 ticks against three Bowsers): the eggs' hurtboxes are Bowser's
  245 at offset 230, not Yoshi's 210 at 175.
- **Remix files that relocate past their end.** Peach's turnip graphics
  (0x1418, 0x1260 bytes) has two slots aimed past the file; the N64 applies
  them unchecked. The DS loader failed the file, which failed her special2,
  which left her whole main file unrelocated (frame-0 fault in the parts
  setup). A P4 file's chain now goes on with such a slot left NULL
  (`gNdsP4RelocOutOfRangeSlots`), internal and external alike: Peach's,
  Sheik's, Banjo's and Dedede's main files open with header words that
  Remix left at the parent's offsets into small donor special files
  (Sheik's +0x4 aims at 0x760 in a 0x40-byte file), which nothing reads.
- **Banjo's main file has 208 external file ids**; the loader's table held
  144 (KirbyMain, the original cast's largest), so the whole file was
  refused. P4 builds size it 208, and the generator fails any file past it
  (`MAX_EXTERN_IDS`).
- **Special-move starters (an S3 seam, six tables).** Remix's `ground_nsp`,
  `air_nsp`, `ground_usp`, `air_usp`, `ground_dsp` and `air_dsp` replace
  the source's `dFTCommonSpecial*StatusList` rows by kind. The four source
  checks (`ftCommonSpecialN/Hi/Lw/AirCheckInterruptCommon`) now run with the
  content's rows lent into those tables at its kind's index, then put back
  (`ndsP4CheckSpecialLent`). A row equal to the parent's keeps the source
  entry; a vanilla routine is named; a donor routine with no port starts
  nothing in lab builds (`ndsP4LabSpecialStandIn`, counted and listed) and
  fails the generator otherwise. Before this the parent's starter entered
  the parent's status on the donor's row, which for a donor that never uses
  it is dead: Lanky's 0xE1 has no motion, so Mario's Super Jump Punch
  interrupt read a TransN joint the motion never made (fault at frame 493).
  Falco and Ganondorf keep all six of their parents' rows, Wolf changes
  his up specials, Bowser three rows, the other ten all six.
- **Shared parent globals after the fighters are made.** The source's
  `ftManagerSetupFilesPlayablesAll` re-reads every playable kind's file
  globals from the status buffer. A parent whose players are all P4
  children loaded none of its own files, so the globals its children share
  read NULL afterwards: four Ganondorfs faulted at frame 365 when the
  aerial Falcon Kick built its effect from no file (a NULL effect DObj;
  found with the lab exception recipe). The P4 wrapper publishes each live
  content's files again. The effect resolver also sized a descriptor's file
  by the parent's file id, which a content's same-layout copy is not, so the
  Kick's descriptor stayed deferred; it now falls back to the loaded file's
  own size (`ndsEFManagerFileSpan`).
- **The parent's motion file.** All fourteen load their parent's motion file
  (Ganondorf: 0xEB, Captain's), and Remix loads it by that id, so the
  source's status-buffer pass hands the parent's MainMotion global the same
  file, where the parent's code reads it: Captain's Falcon Dive takes its
  victim offsets from there (`ftCommonCaptureCaptainUpdatePositions`). The
  synthesized motion file keeps those bytes at their offsets, so it stands
  in for the parent's when the parent loaded none
  (`ndsP4PublishParentMotion`); the generator asserts the donor's motion file
  is its parent's. Before this, four Ganondorfs faulted at frame 384 on the
  first Dark Dive grab. A content also takes its parent's particle-bank
  fields, so a children-only match loads the bank it shares. Mixed matches
  (a parent and its child, or two children of one parent) still share one
  set of globals: S5.
- **Memory (open, S15).** A content loads its Remix files whole: Sonic's
  main closure is 329 KB, Crash's 208 KB, Falco's 86 KB. In a four-CPU
  mirror on the lab stage the files take 840 KB of the general heap for
  Sonic (Falco 582 KB) and each fighter then takes 33-49 KB more (largest
  animation, DObjs), so the fourth Sonic overflowed the heap while its parts
  were made and Crash was left 7.7 KB. A Remix fighter's own shield-pose
  file also takes the source Event32 path, nine AObjs per joint, where the
  original cast uses native shield-pose packages: four Crashes shielding
  exhausted that 7.7 KB (an allocator halt at frame 330, before the
  special-move seam changed his CPU's choices). The fix is the original
  cast's: compact packs that keep only what the native owner and the pose
  engine read, and a generated shield-pose package per content. Lab builds
  no longer load the donor special files a stand-in replaces (only the main
  file's header words name them; the generator checks): Sonic's closure
  drops to 144 KB, Wolf's to 78 KB. Compacting the model alone saves less
  than it did for the original cast: a content's high-detail preview pack
  keeps 20-125 KB of textures and structure (Banjo 143 KB, Crash 123 KB,
  Lanky and Bowser 89 KB, Falco 28 KB), where the original cast's battle
  packs, both details, are 16-41 KB. Texture VRAM is banks A and B, 256 KB
  for the stage, fighters and effects together. With the lab skip, two
  Sonics beside Mario and Captain play 1,200 frames (heap low-water
  15,584, under the 25,600 floor); two Banjos beside Mario and Fox still
  never reach the match (his 52 KB animation heap per fighter). The heavy
  contents need compact packs and smaller animation heaps first; texture
  reduction or a per-match budget would be an owner decision.

Four-CPU mirrors, lab ROM with all fourteen (`NDS_P4_LAB_FALLBACK=1`, lab
stage, 1,200 frames, exception vectors trapped), on the build with the
special-move seam, the S4 commands, generated CPU rows and the lab file
skip ("p4g"). 0 native failures in every run.

| Content | Result | Lab declines | Notes |
|---|---|---|---|
| Falco | clean | 0 | generated CPU rows replace his hand table |
| Ganondorf | clean | 0 | |
| Wolf | clean | 0 | all 11 donor routines ported and his own special files loaded (S6, "p4l", 1,500 frames): the Wolfen, blaster shot, slash and held gun draw natively from his files (the Wolfen's 10 drawn roots 240 draws each, shot 208, slash 12, gun 162); 0 native failures, 0 lab stand-ins. Before S6 his gun's list (0xB47) declined 35-111 times a run |
| Bowser | clean | 0 | all 10 donor routines ported (`src/port/nds_p4_bowser.c`: Whirling Fortress, Bowser Bomb's air physics, Fire Breath on the Fire Flower's flame with his ammo and its recharge, the forward throw's airborne slam, the Clown Copter entry on his own special2) and the Yoshi-only source branches he skips ("Parent-kind compares"; "p4o", 1,500 frames): 0 native failures, the copter's 10 drawn roots 248 draws each, 12 common shields and no egg shield, no Yoshi double jump, heap 69 KB free. His third jab (0xE9) through the jab tables; his CPU rows (BOWSER_USP_DSP ledge test, Whirling Fortress steered at the target, short-hop Fire Breath) and his Yoshi egg (245) |
| Marth | clean | 0 | 0xD9 SET ENV COLOR (8 per run) is the one unported command |
| Roy | clean | 0 | 0xD9 as Marth |
| Wario | clean | 0 | |
| Peach | clean | 325, low detail | a motion's hidden-part joint adds a root the owner lacks (17 against 16), the original cast's root-program case |
| Crash | clean | 0 | fits since the lab skip |
| Lanky | clean | 0 | |
| Sheik | clean | 0 | |
| Banjo | the mirror's fourth 52 KB animation heap overflows (S15) | | his main loads since the 208-id fix |
| Sonic | clean | 0 | fits since the lab skip (closure 329 KB to 144 KB) |
| Dedede | clean | 0 | |

## Board 1 status (master plan Revision 3)

| Item | State |
|---|---|
| S1 content list | done: registry, Makefile templates, one native-owner emit, `NDS_P4_CONTENT_ROWS` in every shared site; Falco unchanged (lab: 0 declines, menu ROM preview) |
| S2 build throughput | not started; lab and menu builds still rewrite the shared linker script under a mutex |
| S3 table seams | 19 of 33 (Falco's 12, entry_action/entry_script, the six special-move starters) plus the jab tables (jab_3, jab_3_timer, jab_3_action, rapid_jab and its four rows, "Jab tables" above) and yoshi_egg |
| S4 motion commands | 11 of the roster's 12: all but 0xD9 SET ENV COLOR, which needs the renderer's per-fighter environment colour (Banjo, Marth, Roy) |
| S5 routine work lists | generated for all 14 (`scripts/p4/routine_worklist.py --lab <lab build>` into a build directory): 296 donor routines with Remix scope, file and line, size and users (Banjo 33, Bowser 10, Crash 29, Dedede 37, Lanky 29, Marth 18, Peach 22, Roy 21, Sheik 34, Sonic 29, Wario 23, Wolf 11; Falco and Ganondorf none), plus each content's id tests in Remix's shared code (17-39); the classes (behaviour, presentation, 1P, toggle) are still read by hand. Ported: Wolf 11 of 11 (`src/port/nds_p4_wolf.c`: WolfUSP, WolfDSP, WolfNSP with his own shot, the slash, reflector and Wolfen on his own files), Bowser 10 of 10 (`src/port/nds_p4_bowser.c`) |
| S6 articles | the path exists, Wolf proves it: own special-file storage (`OWN_SPECIAL_FILES`), the parent's effect makers a content replaces (`gNdsP4<Title>Overrides`), `scripts/p4/p4_articles.py` roots compiled by `generate_nds_entry_effects.py --p4` into the build's packet, native admission, TEXID frames, state-only lists, per-match texture preparation and the held-gun sidecar ("Own special files and articles" above). Bowser's Clown Copter draws from his own special2 (his flame is the Fire Flower's weapon). Next: item articles (Peach, Wario) need native item kinds |
| S7 generated CPU rows | done for the data: attack lists, ai_long_range and the Remix input routines they name are generated for all 14 (Falco's generated list equals his hand table row for row); attack-prevent, recovery and post-process stay hand-ported per fighter (Falco's; Bowser's prevent and post-process); every content's CPU runs Remix's interpreter extensions ("Remix's CPU commands" above) |
| S8 Kirby copies | not started |
| S9 select-screen preview | done for Falco on the menu ROM; pack facts come from the export (`p4_preview_pack.py --content`); the all-content ROM cannot hold P4 menu clips in the select's global reservation (see above) |
| S10 acceptance probe | lab probes exist per topic (`gNdsLabP4Content`); no single command |
| S11 whole-roster generation | all 14 generate, learn owner pins and build into one lab ROM (`NDS_P4_LAB_FALLBACK=1`); four-CPU mirror results in "Whole roster in the lab" |
| S12 EXTRA export | not started; EXTRA's nested Remix gitlink not initialized |
| S13 sword trails | done: Remix rows exported and generated, update and native draw; vanilla Link and Beam Sword trails drawn too |
| S14 own action arrays | done: content special-status tables (Falco verified unchanged; Ganondorf 0 stand-ins) |
| S15 P4 memory | open, and it blocks mixed matches: one content with three of the original cast overflows the heap at battle start, and so does any two-content mix; mirrors fit. Lab ("p4q", Dream Land): Donkey Kong's setup takes 49.9 KB (26.3 KB pack, 2.8 KB native shield pose, 11.6 KB animation heap), Bowser's 179.9 KB (a 145.3 KB file pool: model 112.5 KB of which 32.6 KB geometry and about 80 KB texture frames, motion 11 KB, Clown Copter 11.3 KB, shield pose 5.1 KB; a 23.2 KB animation heap). Content file paths are now computed instead of stored (1,558 strings), which gives every match 47 KB: the four originals end setup with 53.2 KB, and Bowser with three originals gets past Kirby's load but runs out making parts. Sonic's four-CPU mirror overflows the heap at fighter setup, Crash's leaves 7.7 KB ("Memory" above) |

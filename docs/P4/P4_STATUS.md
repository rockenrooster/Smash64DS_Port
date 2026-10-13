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
- **Generation.** Since 2026-10-10 all 14 generate without the lab
  fallback (every donor routine, own special file and entry case has a
  port; Dedede's empty special file 3, Captain's Falcon Punch file, is an
  unread slot), so the shipping build needs no flag. Before that, all 14
  generated with `NDS_P4_LAB_FALLBACK=1`: a status
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
  the body at Fox's hold joint (Wolf's). A weapon's graphic often lives in a
  file its special file depends on (Sheik's needle, Banjo's eggs, Lanky's
  grape, Sonic's spring): the row then names the special file's pointer to
  it (`via`), and the runtime admits the list that pointer holds. A file an
  own special file names is never a lab-skipped stand-in (Sonic's file 9,
  which holds the spring). A sprite-less material takes its TLUT from the
  MObj's segment-E branch, which sets the image to `palettes[palette_id]`
  for the list's own LOADTLUT (the eggs and the grape: palette 0); a
  material whose palette moves with its TEXID (Sonic's spring: open 0,
  coiled 1) is baked frame i with palette i (`palette_follows`), and the
  runtime accepts `palette_id` equal to the TEXID there.
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
- **Remix's reflect AI.** Remix decides the CPU's reflector behavior by the
  character's own id: its `fighter_reflect` row (Falco, Wolf and Dedede
  reflect: CPUs hold their projectiles against them) and Fox's branch in
  its reflect hooks (Falco and Wolf only). A content aliasing Fox therefore
  must not be Fox there: Peach's and Sonic's own down-specials sit at Fox's
  status ids, and the default objective's reflector hold released B in the
  middle of them; their CPUs also flagged projectiles to reflect. The
  generator reads both facts from the ROM (`NDSP4Fighter.computer_reflect`,
  decoding the three hooks' compare chains), and the CPU takes them at
  every site that compares the id: the target's reflector test
  (`ftComputerCheckDetectTarget`), the incoming-attack scan
  (`func_ovl3_80135B78`, the content's fkind seen as its reflect kind), the
  default objective (`ndsP4FTComputerProcDefault`, installed per frame for
  a content), the long-range special's reflector-target roll (a content
  target as its reflect kind; Sheik shoots at Ness, as
  `improve_remix_charged_NSP_2` lets her), and the two compares Remix
  leaves on the vanilla ids (the recover jump range, the item objective's
  Fox-or-Ness target), which no content meets. One difference remains:
  Remix flags Dedede's item hazards (`extend_item_reflect_initial_` reads
  the reflect row), which keeps his CPU from arming the item shield; this
  port arms it.
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
  2026-10-11: the select screens' Selected clips (58.7 KB of .main.rw,
  `battleship_scsubsysdata_ft.c`) live in the frontend overlay's tail, which
  a VS battle borrows as scene-asset space; the two resident lookups read
  them only while the tail is resident (`ndsFrontendOverlayTailResident`).
  The VS loan grows by 58.7 KB and the select screen keeps the same heap
  (same Selected rows, 0 animation fallbacks). Kirby with Falco, Marth and
  Wario (Dream Land, 900 frames, ship ROM) now ends at 30,968 B; Kirby with
  Bowser, Dedede and Banjo still overflows making parts, and Banjo, Crash,
  Lanky and Bowser on Dream Land ask 43,036 B against 36,932.
  The 1P Game opponents' motion tables (the Polygon Team, Giant Donkey Kong
  and Master Hand, 36 KB of `battleship_ftdata.o`) moved into the overlay's 1P
  battle-time head; their FTData rows stay resident because
  `ftManagerAllocFighter` writes every kind's row. VS battles borrow 36 KB
  more: the heavy four's packs and images now all sit in the loan (422 KB of
  428 KB) and the match runs out in the general heap instead, at fighter
  setup; Kirby with Bowser, Dedede and Banjo reaches the hat admission.
  The VS select lends the overlay's other scenes to its previews: the select's
  own block (native menu shell, UI kit, `mnplayersvs`, the Selected clips and
  the sound test's tables) links last, `ndsFrontendOverlayBorrowForSelect`
  hands the 246,656 B before it to the preview pool as a third range, and the
  next scene dispatch reloads the overlay (`check_frontend_overlay.py`
  rejects any reference from the block into the lent range). A content's slot
  heap takes the larger of the two demo clips the select plays (DemoNull and
  its parent's Selected row) instead of its largest demo clip: Banjo 34.6 ->
  16.8 KB, Crash 35.8 -> 8.6, Lanky 27.5 -> 23.2, Bowser 17.3 -> 7.0. Four
  previews show for Link, Donkey Kong, Samus and Kirby, and for Marth, Peach,
  Sonic and Dedede; Banjo, Crash, Lanky and Bowser show three (the fourth
  block, about 75 KB, finds no gap). The select-to-stage-select-to-match path
  plays after the loan.
  Two boot-time libc allocations below the taskman arena shrank, so every
  scene's arena grows by about 68 KB: libfat's disc cache, which
  `nitroFSInit` builds for a ROM read through DLDI, takes 8 pages instead of
  libnds's weak default of 16 (`g_dvmDefaultCachePages`, 32 KB; NitroFS reads
  whole files, and eight pages hold the ROM file's FAT chain), and the
  service threads that only block and relay (idle 1, scheduler 3, controller
  6) take the 4 KB coroutine minimum instead of 16 KB each (they peaked at
  184-236 B in a heavy match; the game thread, 13,604 B, keeps 16 KB). The
  select places its blocks best-fit across its three ranges and takes slot
  heaps from the ranges' tops. Banjo, Crash, Lanky and Bowser now show four
  previews (38.7 KB free at the select's low point), and the same four play
  1,800 frames on Dream Land at 37,872 B low-water (the floor is 25,600);
  Banjo, Crash, Lanky and Mario 35,320. Kirby with Bowser, Dedede and Banjo
  plays at 14,452 B (under the floor), and Kirby with Banjo, Crash and Lanky
  still stops at the hat admission. Fox's blaster shot declined the native
  draw whenever the quad path refused it (a hop, reflection or tilt: 28
  failures in 1,800 frames of Samus, Mario, Fox and Pikachu); FoxSpecial4's
  list 0x40 is now a baked weapon root (`generate_nds_native_item_baked.py`),
  0 failures.
  VS Results with contents (2026-10-11). The overlay links in four output
  sections (head, middle, the Results block, the select's block) so the
  linker cannot merge strings across a lent boundary; Results lends
  [start, results_start) and the select's block as two ranges, and the
  checker resolves section-relative words (it found 48 references into lent
  ranges, the UI kit's strings among them). The podium fighters are the
  transfer state's players (`ndsMNVSResultsMakeFighter`; the source made all
  four with player 0, so a content outside port 0 drew as its parent or read
  NULL attributes), and after a three- or four-fighter match a content keeps
  its battle low detail and pack there (`ndsP4LowDetailResults`, owner
  2026-10-08). Each podium heap holds only what its fighter plays: the demo
  Null, then a winner's three poses or anyone else's Lose (Banjo's Lose 2.8
  KB against his 34.6 KB largest); nobody guards on the podium, so a
  content's guard-pose package stays unloaded (`ndsP4GuardPosesNeeded`, 27
  KB for four). Remix's Results font draws '&' (Banjo & Kazooie) from a
  Sprite it appended to IFCommonAnnounceCommon: the generator ships the glyph
  as the content's second synthesized file and the name maker draws it
  (`resultsscreen.asm add_ampersand_`). Banjo, Crash, Lanky and Dedede: 289
  KB free at the podium's start, 49 KB at frame 400; Kirby, Bowser, Dedede
  and Banjo 100 KB (it ran out before). Kirby's Remix hats: a battle whose
  arena cannot hold every reachable hat keeps one buffer per Kirby, sized
  for the largest, and loads a hat into it when he takes the power
  (Kirby with Banjo, Crash and Lanky now plays).

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
| Marth | clean | 0 | 0xD9 SET ENV COLOR (8 per run) was the one unported command; ported 2026-10-10 (S4) |
| Roy | clean | 0 | 0xD9 as Marth |
| Wario | clean | 0 | |
| Peach | clean | 325, low detail | a motion's hidden-part joint adds a root the owner lacks (17 against 16), the original cast's root-program case. Fixed 2026-10-09 by the donors' root programs ("Root programs" below) |
| Crash | clean | 0 | fits since the lab skip |
| Lanky | clean | 0 | |
| Sheik | clean | 0 | |
| Banjo | the mirror's fourth 52 KB animation heap overflows (S15) | 237-342 per 600-900 frames, low detail (beside Samus, Link and Kirby, "p4y") | his main loads since the 208-id fix. His Kazooie joints (15-21) are hidden parts: setup_parts leaves them out and his motions' anim-desc masks install them (ftMainSetStatus), so while they show the low root vector is 23 against his owner's 16 and the draw declines (validate code 3), as Peach's below. The original cast's hidden parts are owner root programs (`OWNER_ROOT_PROGRAMS`, generate_nds_native_owners.py); a content needs them derived from its motions. Done 2026-10-09 ("Root programs" below) |
| Sonic | clean | 0 | fits since the lab skip (closure 329 KB to 144 KB) |
| Dedede | clean | 0 | |

## Root programs, wide dense ids, select-screen previews (S15, 2026-10-09)

**Root programs.** A content's motions make joints draw that its
setup_parts leaves out: hidden parts their anim-desc masks install (Banjo's
Kazooie, hidden parts 4-10, in 57 motions; Crash's joint 32 in his Win3 pose
and one battle motion; Banjo's joint 36) and model parts on joints whose
JointTree entry has no list (Peach's five articles on joint 18, Lanky's
joints 13 and 18, Dedede's joint 13). Others hide every part and show one
(`HideModelPartAll`: Crash's spin shows only joint 5's second part, Sonic's
ball only joint 6). Each changed the live root vector, the owner validator
refused it (code 3) and the fighter drew nothing: Crash's select-screen
preview never appeared, Banjo vanished whenever Kazooie showed and Crash and
Sonic during their spins. `p4_native_owner.py donor_superset` runs every
motion's scripts (its main script and the parallel ones it starts,
subroutines in place; `ftMainUpdateHiddenPartID`, `ftParamSetModelPartID`,
`ftParamHideModelPartAll`, `ftParamResetModelPartAll`), takes the joints
drawing wherever a script yields a frame, adds every joint that ever draws to
the owner's canonical vector (a hidden part through setup_parts, a list-less
joint through the first part that draws it) and emits each other live vector
as a root program of model-part -1 writes. The generator's generic program
builder derives their binding parents, cross slots and cache proofs; the
renderer selects them (`ndsP4RootPrograms`, nds_renderer_assets.c): a program
matches when every root is its own list or a variant its canonical binding
owns, and a program root's variant lookup goes through that binding.
Programs: Peach 1, Crash 2, Dedede 1, Lanky 3, Banjo 4, Sonic 1; none for the
other eight. `p4_preview_pack.py` read the model-part table for four joints
fewer than the JointTree has, so Crash's joint 32 and Banjo's joint 36 lists
were never root cells; fixed.

**Wide dense ids.** With every part of every joint Banjo's owner needs 2,777
dense vertices (high) and 2,522 (low) and Crash's 2,447 (high), past the
packed 11-bit id. The generator had dropped variants until each fit, and
those it dropped declined: Banjo's item-holding hand (35 motions, low) and
Crash's (52, high). A span's count is its vertex action's own, so an owner
past 2,047 now keeps a span's whole first id and a raw run's whole corner id
(`NDS_NATIVE_DENSE_WIDE`); cross-run corners keep 11 bits beside their GX
slot, and the generator numbers the action blocks they read first
(`_wide_dense_cross_first`). No variant is dropped now. Banjo's low image
grows to about 86 KB. The load trim below brought every owner back under
2,048 (Banjo high 1,575), so no current owner takes the wide form; it stays
for a larger model.

**Shade sites.** A lean packet holds one shade site per lit epoch; Banjo's
high owner has 70 epochs and Crash's 67, past the packet's 64, which declined
their high-detail draws as Capacity. 96 now (+640 B a packet; an entry keeps
3,132 list words against the original cast's largest list, Link's 2,634).

**Select-screen previews.** On the all-content ROM the previews were off
(four 80 KiB arenas against 305 KB free). A content's preview now loads its
low-detail pack and low owner image (`ndsP4LowDetailSelect`,
`ndsRelocPreviewP4LowPack`; the high image is never admitted on the select),
and its blocks are sized per kind -- pack, image and motion file plus 4 KB
(`ndsMNPlayersVSPreviewBlockBytes`) -- and placed first-fit in two regions:
the general heap above a 140 KB floor, and the FGM cue cache's tail, lent
while the select runs (`ndsAudioFgmLendTail`: 72 KB stays for cues; the
battle gets its whole 160 KB back). Each slot's animation heap is reserved
beside its block (24 KB floor for the fighter's objects) and dropped with its
fighter. The original cast shows four previews; a mix with P4 contents three
or four; four of the heaviest contents fewer. P4 animation files are
compacted at generation (`scripts/p4/p4_anim_compact.py`: event16 keys
re-fit within 2 units, event32 converted, 77c19d0ea07; the image -88 KB).

**Battle memory.** Four of the heaviest contents (Banjo, Crash, Peach and
Dedede) ran out of memory at the countdown, then mid-match once that was
fixed. The ledger of the match's allocations put the four owner images
first (84 + 66 + 44 + 34 KB, against 11-24 KB for an original fighter's low
image). Four changes:

- Vertex-load trim (`generate_nds_native_owners.py`
  `_build_source_export_for_owners`). Remix display lists load vertex blocks
  wider than the triangles after them use: about half of every content's
  loaded vertices were never a corner (Banjo low 1,235 of 2,522, Crash 1,039
  of 2,020), nearly all at a block's two ends, and each loaded vertex is a
  dense vertex of the image (dense, normal and prepared rows, 26 bytes). A P4
  load now covers only the span between the first and last vertex a corner or
  a MODIFYVTX reads; rows stay, so epochs and bindings do not move. Low
  images: Banjo 84,464 -> 52,768, Crash 66,180 -> 40,440, Dedede 43,952 ->
  26,920, Peach 33,972 -> 26,824, Lanky 43,532 -> 31,700, Sheik 38,128 ->
  24,192; the census pins' dense counts moved, nothing else. The original
  cast's images are byte-identical.
- Each P4 player's animation heap takes its own content's largest motion
  (`battleship_ftmanager.c` `ftManagerAllocFigatreeHeapKind`); the parent
  kind's row was raised to its largest selected child, so Dedede beside
  Banjo took Banjo's 29 KB.
- The animation cache's match sizing skips P4 players
  (`ndsR2AnimCacheMatchFighterBytes`): it charged the parent's Main, which a
  content never loads, and its size walk loaded the parent's guard-pose
  package (3.3 KB).
- Half-resolution material frames in the low battle pack
  (`p4_preview_pack.py half_res_frames`, owner 2026-10-08): each MObj texture
  frame keeps a half-size image and the runtime repeats it to full size in
  one of two scratch frames when the texture converter asks
  (`ndsRelocPreviewHalfResSource`). Crash -8,472 B, Bowser -11,544, Dedede
  -5,032, Lanky -3,052, Peach -2,680. The select's previews load the same
  pack; side by side with play-1009j they look the same.

A P4 content hit by an electric attack takes the common electric flash
(`ftParamCheckSetSkeletonColAnimID`): its parent's skeleton body is a
different model.

play-1009m, Banjo, Crash, Peach and Dedede, 900 frames: heap low-water
34,396, no overflow, no native failure, no declined or skipped draw.

The heaviest four (Banjo, Crash, Lanky, Bowser; play-1009m, 900 frames per
stage) still do not fit everywhere: Castle 35,720, Hyrule 38,952, Dream Land
31,304; Mushroom Kingdom 21,736 (under the floor); Kongo Jungle runs out
making Bowser's animation heap, Zebes, Yoshi's Island, Sector Z and Saffron
while making fighter parts or the interface. The four originals with the
largest images (Kirby, Donkey Kong, Samus, Link) end at 63,200 (Castle),
47,668 (Kongo Jungle), 63,832 (Sector Z), 56,944 (Saffron). The ledger of
both matches' allocations: the four contents take about 77 KB more (images
+74 KB, battle packs +42 KB, animation heaps +49 KB, native guard poses
+15 KB, P4 tables 13.6 KB, MObjs +12 KB, against what the originals load
and the contents do not), and on every stage the match then declines the
optional stage GX program and world caches (31 KB; the stage draws
uncompiled). The FGM arena is full in that match (144,212 of 163,840 B
pinned, one cue not placed), so it cannot lend. Half-resolution
display-list textures were tried and saved 0.2-1.4 KB a content: dropped.
What is left is structural: an owner image carries 26 B a dense vertex and
the lean path reads 14 of them (the prepared s/t, the reserved half-word
and, outside texgen, the colour word are not read), and each fighter copies
its current animation into its own heap.

Open: the select shows two previews for those four (its two regions hold
about 290 KB; the four blocks and their animation heaps want about 450 KB);
a match with Banjo records 26 native failures on Fox's blaster shot
(FoxSpecial4 root 0x40, stage domain), which the original-cast match does
not.

## Kirby's copies (S8, 2026-10-11)

Remix gives Kirby a status of his own for each power it adds
(`Character.add_new_action(KIRBY, ...)` in Kirby.asm: Kirby's own
animations and motion scripts, the power's routines in the callback slots),
238 specials in all (83 vanilla + 155). `remix_export.py --fighter KIRBY`
(`KirbyExporter`) exports Kirby's moved and extended action and parameter
arrays, the vanilla Kirby read from the original image as the parent.
`scripts/p4/p4_kirby.py` keeps the statuses of the enabled contents' powers
(88 for the fourteen) and writes:

- `nds_p4_kirby.generated.h`: the DS status ids, dense from 0x12F (Kirby's
  first id past his 83 specials), and motion ids, dense from 276, the
  ported routines set (`NDS_P4_KIRBY_STATUS_MARTH_NSPG_1`, ...).
- `nds_p4_kirby.generated.c` (in `nds_p4_data.c`): the status rows, callbacks
  as hand ports or decomp names like a content's; each power's row
  (`NDSP4KirbyPower`: Remix hat id and the source hat it re-skins, the source
  hats its scripts show, the starters Remix's `kirby_ground_nsp` /
  `kirby_air_nsp` name, NULL where they are the parent's copy routine, and
  the clone swaps `Kirby.clone_power_action_swap_` makes); the Remix hat
  table (`KirbyHats.add_hat` bases); the sword-trail rows the power scripts
  select; the files a match may load.
- `nitro:/p4/kirby.tab`: the 88 motion rows. `o2r/`: the 47 power
  animations (compacted; Remix's power statuses that reuse Kirby's vanilla
  inhale and Falcon Punch keep the vanilla ids) and one 5,100-byte script
  file holding every power status's Remix motion script, its pointers
  internal relocations (round-tripped as the contents' synthesized files
  are). Its rows carry the submotion-script flag, so `ftMainSetStatus` reads
  their scripts from `*p_file_submotion`, which Kirby (no submotion file)
  points at the loaded script file.

Runtime (`nds_p4.c` "S8"):

- A VS battle Kirby beside a content with a power constructs on the match's
  Kirby FTData (`ndsP4KirbyMatchData`, through the P4 FTData view in
  `ftManagerMakeFighter`): the source's, the power rows after his 276 motions
  (heap), the script file as his submotion file. His special-status table is
  the source's 83 rows and the power rows (heap, `ndsP4SpecialStatusDescs`).
  A match without a power content keeps the source's FTData: nothing loads.
  Cost when loaded: 4,368 + 5,180 + 4,104 bytes of battle heap.
- `copy_id` stays the parent's kind, so the source's copy code (appeal,
  Kirby losing the copy, the AI) runs the parent's power;
  `FTStruct.nds_p4_kirby_copy` names the content. The catch records the
  victim content's power (or a victim Kirby's) at its eat sound; the copy
  takes it with copy_id (`ndsP4KirbyCopyInitCopyVars`: a power of the same
  parent kind as the held copy is still a new copy, as Remix's ids differ);
  the lose-copy sound, a Kirby swallowed by another and every new stock
  clear it.
- `ftMainSetStatus` swaps the parent copy's statuses for the power's
  (Falco's Phantasm for Fox's blaster, Ganondorf's punch for Falcon's, Wolf's
  blaster, Lanky's gun); the neutral-special check lends the power's
  starters; after each status change the power's `kirby_on_status` runs
  (Falco's `set_variables_` on Kirby's Fox copy).
- Kirby-holding-the-power hooks beside the content ones:
  `kirby_on_landing` (Marth's and Roy's aerial boost, `Kirby.clear_marth_flag_`),
  `kirby_on_hit_interact` (Wario's Body Slam recoil), `kirby_colanim_end`
  (Sheik's and Dedede's charge flashes, samusshared.asm
  `kirby_power_check_flash_`), `kirby_after_proc_map` (Bowser's flame
  recharge), `kirby_copy_inhale_status` and `kirby_on_custom_reflect`
  (Dedede's inhale on Kirby's DEDEDE_NSP statuses, its absorb).
- Each content's routines take their Remix Kirby branches: the status ids
  (Wolf, Marth/Roy with Roy's from his own block, Wario, Peach, Lanky,
  Banjo, Crash, Sonic, Sheik, Bowser, Dedede) and what else Remix changes
  for Kirby (Kirby's boost flag word for Marth, his shoulder hurtbox and
  0.55 recoil fall for Wario, his grape joint and offset for Lanky, his
  spin file and colours, gravity 2.1 and jump 64 for Crash, his Samus damage
  routine and status setup for Sheik, his flame joint, scale, ammo words and
  smoke for Bowser). The four Kirby-only collision routines Remix adds
  (`MarthNSP.kirby_ground_collision_`, Sheik's three and her aerial shot's
  landing) are ported.
- Hats (`scripts/p4/p4_kirby_hats.py`): a Remix hat (`KirbyHats.add_hat`)
  re-skins a source copy hat -- Kirby's joint-6 part keeps that hat's
  materials and tracks, and its lists come from KIRBY_CUSTOM_HATS. Each hat
  a power draws (its own, then the ones its scripts set: Bowser's and
  Dedede's open mouths, Kazooie) is compiled by the owner generator's own
  copy-hat path: its two lists and their vertices are appended to Kirby's
  model payload in the source hat's model-part slot, the textures they load
  move to a texture file per content (extern relocations, so the image
  carries them as foreign images), and the image is a copy-hat image with
  the trio body baked against the hat's own head; 17 hats, 6-12 KB a detail,
  texture files 16 B-1.6 KB. Read as assembled: Remix's lists keep their own
  model's material slots, so they call past the source hat's two MObjs
  (Banjo's hat calls 12, Crash's 13 and 14); on the N64 such a call runs
  into the MObjs' material lists, which follow gcDrawMObjForDObj's table, so
  the generator maps it to the MObj whose list it lands in, or drops it
  where it lands past that list's texture image (only tile sizes are left);
  Wolf's hat sets an image no load reads (its load's opcode was made a pipe
  sync), Dedede's opens with a material call its second call repeats, the
  open mouths load the trio body's vertices after their last triangle, and
  several restore Kirby's light colours for the next list: all lowered as
  the RDP sees them. At runtime the live DObj keeps the source hat's part
  (its list offset is the root identity), `FTStruct.nds_p4_kirby_hat`
  records the Remix hat (`ndsP4KirbyModelPart`; the power's own source hat
  id, which ResetModelPartAll restores, is its Remix hat), the match admits
  its images beside the source hats (`ndsRendererNativeKirbyHatSetP4Hats`),
  the slot binds the Remix hat's image in the source hat's place, and the
  Remix hat id is the trio head, with a nine-root program per hat in the
  build-local head list (the source's Yoshi-swallow face program carries
  joint 18, which no power opens). A pack's foreign images may come from a
  hat texture file (`ndsRelocNativeForeignImageAddress`).
- The weapon a power holds on joint 17 (`KirbyHats.use_extended_special_parts_pause_`:
  Wolf's gun, Marth's and Roy's swords, Lanky's gun) is selected through the
  source's pistol part, Fox's file-315 list: it leaves the body as Fox's
  copied pistol does (`ndsFighterHoldsFoxGunSource`) and draws as a sidecar
  root: Kirby's own (KIRBY_MARTH_SWORD 0xD6D, KIRBY_ROY_SWORD 0x14F8,
  KIRBY_LANKY_GUN 0x150A, rows of the content whose joint carries 0x80) or
  the content's (Wolf's gun, the WOLF_WEAPON list Remix gives Kirby); a Fox
  clone's power with neither (Falco's) keeps Fox's pistol. Before this the
  part stayed in the body's roots and the lean draw declined every frame it
  was out (Marth 235, Roy 234, Lanky 162 declines in 1,500 frames).
- The CPU sees a power's Kirby as holding no known copy (copy_id masked to
  `nFTKindEnumCount` during its decisions), as Remix's AI sees its ids.

Lab, Castle, Kirby forced to tap B beside three of a content (1,500
frames, `kirbyall.ps1`): every power taken; the power statuses reached for
Falco (Phantasm ground and air), Ganondorf, Bowser, Marth, Roy, Wario
(Body Slam, both recoils), Peach (Peach Bomber, recoil), Crash (spin and
its block), Lanky, Sheik (charge, shots), Banjo (eggs), Sonic (homing,
recoil, bounce) and Dedede (inhale, pull); Wolf's was taken in both runs
and lost to the source's random loss on damage before a B press. 0 native
failures, 0 article misses (Crash's Kirby spin is its own article row).
With the source hats standing in, three powers declined: Bowser 45, Banjo
434 (Kazooie's hat re-skins Link's, head 10, which only Link's copy program
knows, with its boomerang), Dedede 112 (his open mouth re-skins the
Yoshi-swallow face, head 14, whose program carries joint 18). With the
Remix hats and their programs (build-lab-p4an, same sweep): 0 declines for
13 powers, 1 for Crash's, 0 native failures; Bowser's hat drawn on Kirby
(capture `kh-bowser3-f380`, local). The hat must follow a model-part reset
(ResetModelPartAll writes the part directly): without it Bowser's open
mouth outlived his breath and the draw declined 42 times.

A battle of three or four admits the powers' Remix hats in the low detail
only, and a Kirby holding a power keeps the low model in its close-ups, as
a content does (owner 2026-10-08: lower content fidelity rather than a
budget); a two-fighter battle admits the high detail.

## Mad Piano (owner request, 2026-10-11)

The owner added Mad Piano (Remix PIANO, kind 0x36, parent Mario) as content
15, with a select portrait of his own: the grid is eleven columns, Mad Piano
between Mr. Game & Watch and Metal Mario, the two spare cells the NONE plate
at the left of the lower rows (`scripts/p4/p4_css.py`; Remix shows him only
on its bonus bookend). His export and generation are clean (19 donor
routines, 0 failures); `src/port/nds_p4_piano.c` ports all 19 read as
assembled: the up special (Super Jump Chomp: its begin, launch at 120 up and
50 plus half the stick forward, Mario's landing map with 0x3E205C29), and the
down special, which is Ness's absorb fused with Yoshi's command grab (open
mouth, absorb, end, the command throw with the victim hidden once the script
says so, and the ground/air switches through Remix's two tables). The hooks
Remix puts on his id are `NDSP4Overrides` rows: `absorb_hit` (Ness's absorb
routine sends him to his own absorb status), `absorb_heal` (an absorbed shot
heals 0.75 of its damage, the source twice it, and banks a third as books, at
most five), `command_capture_status` and `capture_status` (his command grab's
and his grab's victims enter ThrownDK, Shouldered here),
`shouldered_interrupt` (the victim never mashes out and is eaten while he
holds it), `fireball`/`kirby_fireball` (fireball.asm's struct_book: one book
at 60 degrees with a random 25-degree spread, plus one per banked book, each
spread a quarter wider; Kirby's from joint 7), `thrown_hidden` (his throws
hide the victim as Yoshi's do), and `ndsP4PianoOnStatus` (his tongue and held
item show only in the grab, the grab pull and the item statuses). The book is
a P4 weapon (`NDS_P4_WP_KIND_PIANO_BOOK`) on his own special file 1, drawn
natively (`p4_articles.py`). His CPU takes Ness's absorb branch
(`NDS_P4_COMPUTER_REFLECT_NESS`, generated from
extend_projectile_reflect_initial_) and holds and presses his own absorb
statuses (0xE5-0xED: `NDSP4Computer.absorb_first/_end`, Reflect.asm
_piano_absorb). He has no victory song (add_victory_bgm(-1)); his podium tag
sits at Kirby's height (label_height). Two fixes for the others came with
him: Wario's grab puts its victim in ThrownDK as Remix does
(CustomGrabAction.asm, never ported before), and the podium tag heights of
Crash, Lanky and Peach follow their label_height rows; Banjo's podium says
"WIN!".

## Metal Mario (2026-10-11)

Metal Mario is the source MMario kind (13), selectable as itself: his cell's
selection id is his fkind and his content is 0, so every match runs the
source's own MMario code (the campaign's stage 11 fighter), CPU and Kirby's
inhale (no copy, Remix's row too) included. What P4 adds is Remix's
presentation of METAL:

- **Character select** (`scripts/p4/p4_css.py` KINDS): Remix's portrait
  (portrait_offsets.METAL), name (name_texture.METAL) and Mario Bros. logo
  baked as gate art after the contents', the source's call
  (nSYAudioVoiceAnnounceMMario, FGM.asm names.METAL_MARIO). His preview is a
  select pack of his source Main and Model (`scripts/p4/p4_source_pack.py`,
  the content pack builder on BattleShip's O2R files, kind 13, 6,560 B) drawn
  by the campaign's MMario owner image; only the VS select loads it
  (`reloc_preview_pack.c`). His selected pose is Mario's (Remix's
  selection_action_table: Win3). The source's menu scale table stops at the
  twelve: his read ran past it to a near-zero scale and left the panel empty;
  he takes Mario's row, as Remix's move_table_12 does.
- **Menu motions** (`battleship_scsubsysdata_ft.c`): MetalMario.asm's table,
  rows 1-13 Mario's (copy_menu_actions), his three victories with the metal
  footstep (FGM 0x7A) at Remix's frames, Win4 silent. The census's largest
  submotion clip for MMario is raised to Mario's (`ftManagerSetupFileSize`).
- **Battle HUD**: his stock icon is baked like the cast's (MMarioModel's
  stock sprite, his one LUT); the campaign's Metal Mario had none before.
- **VS Results**: add_to_results_screen(METAL): "METAL MARIO" at x 20, scale
  0.55, "WINS!" at 185, the announcer's Metal Mario, Mario's victory song, the
  Mario Bros. emblem, Mario's podium tag height and menu zoom, set up by the
  source's per-kind loop through its last playable kind. No VS record (the
  source's records stop at the twelve).

Probes (play-1011y): select with two Metal Marios beside Mario and Mad Piano
draws all four (0 native failures, Metal block 15,264 B); four Metal Mario
CPUs on Peach's Castle, 600 frames, low-water 100,664, 0 failures; their
Results "METAL MARIO WINS!" with 0 failures. Open: Remix gives METAL six
costumes (0,1,4,5 and teams 1,3,2) from a rebuilt model (MMarioModel +1,488 B,
MMarioMain's stock LUTs 1 -> 6); the source model has one look, so his
mirrors are told apart only by their tags.

## EXTRA characters (S12, 2026-10-11)

The staging tree is EXTRA's (`joaorb64/smashremix-plus-extra` 96621af, its
nested Remix at its pin): the Makefile's `NDS_P4_STAGING` and `NDS_P4_EXPORT`
now default to `builds/p4-staging/extra-96621af` and `export-x`, and every
content exports from it (EXTRA's tree is Remix's plus three characters). Its
5,850 files push the synthesized ids up (`SYNTH_FILE_BASE` 0x1700, Kirby's
script and hat files 0x1900/0x1910); a build directory seeded from an older
staging kept files under ids that now mean something else, so the generator
now deletes the O2R files an earlier run shipped and this one does not.
Export and generator fixes the three needed: Marth's and Roy's entry routines
by symbol (`ENTRY_PORTS`), a Goto or Subroutine whose target is NULL (MRGAW's
IDLE stops its script there, as the source's script engine does), 288 extern
ids per file (MRGAW's main names 284; `NDS_RELOC_EXTERN_FILE_ID_CAPACITY`), a
parent of JIGGLYPUFF (Purin) and the `j target; nop` thunk Remix writes in a
grounded_script row. Their sounds and victory songs live under
`build/extra_characters/<folder>` (`contents.json` "sounds").

### Meta Knight (content 16, Jigglypuff's code)

Every routine is ported (`src/port/nds_p4_metaknight.c`, 0 lab fallbacks):
Shuttle Loop (up), Mach Tornado (neutral), Dimensional Cape (down), his jab
loop and three-hit forward tilt, the wing state his OnActionChanged carries
into aerials, falls, landings and aerial damage, his initial_script and the
ThrownDK mash-out fix. Three mechanisms are new and general:

- **Status replace** (`NDSP4Overrides.status_replace`, Remix's
  action_replace_map): ftMainSetStatus sets up the replacement's descriptor,
  motion and procs while fp->status_id keeps the original id (his jab runs
  JAB_START as Attack11, his forward tilt TILTF as AttackS3); the
  on_status hook gets the replacement, as Remix's on_action_changed does.
- **Static parts** (`NDSP4Overrides.static_parts`, EXTRA's StaticPart.asm):
  a joint that always exists. Setup creates it, the hidden-part loop never
  installs or ejects it, a motion that does not install it gives it no track
  (the figatree walk takes no entry for it) and the guard's joint count and
  pose walks pass over it. His is joint 12, his light-item joint: without it
  a held item read a NULL joint in every motion but his item and cape ones.
- **State-only accessory lists**: Jigglypuff's costume accessory
  (ftDisplayMainDrawAccessory) is, for his costumes 1-3, a list that only
  re-sets the lights his joint 6 list sets again; the renderer takes no root
  for it (three of four Meta Knights declined every frame before). The owner
  generator now builds a P4 donor's accessory programs (the canonical vector
  and each live program with the accessory) when its list does draw.

His two entry animations are event32 with values event16 cannot hold (wing
joints parked 16,384 units away, a joint grown past scale 8) and one cubic
key a frame: 65 and 58 KB, so four Meta Knights ran out of battle heap at
the fourth figatree heap. They are now refit as joint-synchronised linear
keys within the same two units every P4 animation keeps
(`p4_anim_compact.compact32_o2r`, checked frame by frame): 22.7 KB each, his
heap 22,704 B. His select name is 72 texels wide with ink through column 62
on a 66-wide card: the panel bake drops a P4 name's fully transparent
right-hand columns (lossless).

His sword trail (joint 34 part 0, model 0x15EE lists at 0x93D0 and 0x94F8:
two 32x32 CI4 flame quads, PRIM alpha 0x30, EXTRA's transparency.txt edits)
is drawn by nothing on the N64: its combine's second-cycle alpha is
(TEXEL1 - COMBINED) * LOD_FRACTION + 0, LOD_FRACTION stays 0 with texture LOD
off, and the XLU blender then keeps the framebuffer. The DS drew the quads
opaque, black around the flame (owner, 2026-10-11); the owner generator now
emits no triangles under that combine (`DIRECT_POLICY_COMBINE_INVISIBLE`).
A static scan of every content's files for blending lists found no other
such combine: the rest are articles (entry-effect packet, XLU-aware) or
exact texture-alpha cutouts.

Probes: lab (build-lab-p4x), four Meta Knights 1,500 frames, 0 declines, 0
native failures; forced up, neutral and down specials, jab and forward tilt
run their statuses (the jab as Attack11 through the loop and end). Play
ROM play-1011z: select with four Meta Knights (costumes 0-3), 0 failures;
four Meta Knight CPUs on Dream Land, 1,500 frames, low-water 29,088, 0
failures; Results with two of them, 0 failures.

### MRGAW (content 17, Mario's code)

Ported (`src/port/nds_p4_mrgaw.c`, 0 lab fallbacks): his 2D facing (the top,
weapon and grab joints' x scale signed against his direction every frame,
NDSP4Overrides.before_physics_map), Chef with his sausage (a weapon of his
own, `NDS_P4_WP_KIND_MRGAW_CHEF`, on his special files 1 and 3), its
refreshes and its landing, Fire (up) with its firemen kept where it began,
Judge (down: one of nine at random, judge 1 costing him 12, judge 7 making a
food item on its first hit) and his on_hit refresh of Fire. Remix's random
draw advances the seed by the audio thread's tics (netplay lockstep); the DS
advances it by the battle's logic updates, which replays reproduce.
Judge 7's food is Remix's Dango 99 times in 100 (items/Dango.asm: the Maxim
Tomato with a 10% heal), the Maxim Tomato otherwise. The Dango's attributes
sit in Remix's extended ITCommonData (0xFB+0x1010) and its model in the
common items file Remix adds (0xA96, 29.6 KB); the generator ships the
attributes and the closure they name as an item subset file of 1,096 B
(`item_subset_file`, `SUBSET_FILES`: an object starts at a pointer target and
runs to the next, every pointer in a reached object followed, the tree's
terminator and lists checked inside the subset) in his special slot 4,
loaded whole at setup. It is a P4 item kind (`NDS_P4_IT_KIND_MRGAW_DANGO`)
with the Tomato's matrices, routines and status table, made by the Tomato's
maker with the swirl and appear spin its SHOW_GFX_WHEN_SPAWNED asks; its
pickup runs the Tomato's case with heal 10 (`ndsFTCommonGetEatHeal`, Remix's
jump to 0x80145C40: the eat hook, the item's end, the 1P tomato bonus),
dispatched as a consumable (`NDS_P4_ITEM_CONSUME`). Lab, four MRGAW CPUs on
Castle, player 0 forced to down-B with every judge made judge 7, 1,800
frames: 5 Dangos made, 5 eaten (10 healed each), drawn natively, 0 native
failures, 0 declines.

### Snake (content 18, Captain's code)

Every routine is ported (`src/port/nds_p4_snake.c`, 0 lab fallbacks): the
C4 (down special: set on the ground, a wall or a fighter he faces, set off
with a second press), the grenades (neutral: pulled, held while he walks or
jumps, thrown three ways, two out at most), the Nikita (his forward smash: a
missile he steers until it hits, runs out of gas or Z cuts its engine), the
Cypher (up: a rising hover with 7 points of non-cumulative armour, let go or
cut down), his crawl, his dash attack into his crouch, his two-part forward
tilt, his up smash's mortar, five codec taunts, his CPU's Nikita, grenade and
C4 handling and its Cypher recovery, and the up special's return on landing,
rebirth and hit. His four items are P4 item kinds on his own special files
(the merged file's C4, Nikita and Cypher attributes, the grenade's file) and
the mortar a P4 weapon kind; their explosions run Link's bomb events from his
files at Link's offset, as Remix's do (his hitbox files are copies of the
bomb's, events included).
His items are translucent (the attributes' is_display_xlu: they draw through
DObjDLLinks), so an item article's entries now admit their DObjDLLink lists
(`p4_articles.py`).

As assembled: the grenade pull's end reads its ga from its own code (always
the aerial wait; a grounded Snake lands back into the wait), a Cypher a hit
destroys leaves his reference set (the hang rises on to its end), the CPU's
grenade throw goes back beyond 500 and forward within it, and the C4's team
test compares an unset register (the DS compares his own team).

CPU fix for three contents: Remix's `nsp_shoot_custom_move` (Dedede, Peach
and Snake shoot from afar with their down special) had no port; the
generator now emits it (`computer_long_range_move`) and the walk-and-shoot
case issues it.

The shipping select bakes his series logo, an I8 sprite the UI kit could not
decode before (`generate_mn_ui_kit.py` decode_i8). A seeded play build kept
the previous content count (its contents header, Kirby copies and colanims
depended only on the exports, older than the copied outputs); they now also
follow `nds_build_config.h` (`scripts/p4/p4_rules.mk`).

Probes: lab (build-lab-p4x), four Snake CPUs 1,500 frames, 0 declines, 0
native failures; every special and replaced move forced (grenade pull, wait,
throw and its fuse; Cypher start, hang, cut, landing refresh; C4 set,
detonated and exploded; Nikita launched, steered, exploded; mortar fired and
burst; forward tilt into its second kick; taunt calls; crawl); a four-Snake
CPU match of 1,800 frames draws all five articles (ndsRendererEntryEffectP4Admit
counts), 0 failures. Play ROM play-1011sn (all 18 contents): select with four
Snakes and with four MRGAWs, 0 failures; four Snake CPUs, 1,500 frames,
low-water 36,748, and four MRGAWs, low-water 51,852, 0 failures (the probe's
stage poke does not hold: Peach's Castle); Results with both, 0 failures.

## Entry images at scene start (2026-10-11)

Four Dededes on Dream Land lost every KO effect natively (6 failures): the
scene-start texture pass stopped at the first image the texture VRAM refused,
his 18th article image, and the KO palettes are made last. Each image now
fails alone, and the KO palettes need only the KO image
(`ndsRendererPrepareEntryEffectTextureSet`).

Two changes give the entries their room. An image only the original cast's
entry props name (Mario's pipe, Fox's Arwing, the barrel, capsule, Falcon's
car, Link's) is skipped when the scene never loaded the special file that
holds them: 18.5 KB in an all-P4 match (FoxSpecial3 also loads for Sector Z's
fly-by, so the Arwing stays there). And images no entry draws (the contents'
articles; Link's spin and boomerang, shield, reflector, catch, KO, reflector
break, Kirby's cutter, Samus's grapple, Falcon's kick and punch) are placed
after the entry images. Whatever the scene start could not place is placed
when the startup images retire, two frames into GO, before the dust planes
(`ndsRendererHardwarePrepareEntryEffectGameplayTextures`). The setup
functions are compact (`optimize("Os")`, with the startup release), so .main
keeps its size: it ends on a 1 KB boundary and the heap moves in whole KB.

Lab (build-lab-p4x, Dream Land, 1,500 frames): four Dededes 0 refusals, 0
native failures (were 6); Crash, Lanky, Sonic and Dedede refuse 28 late
images at scene start, all placed after GO, 0 native failures (were 16:
Sonic's entry props); the original cast (Mario, Fox, Donkey Kong, Falcon)
skips Samus's and Link's props, 0 failures; Sector Z without Fox keeps the
Arwing, 0 failures. Play ROM play-1011ko: four Dededes 0 failures,
low-water 17,608, as before. That four-content match does not fit the
shipping heap: it runs out making the damage interface at battle start
("S15 P4 memory"; the lab ends it with 10.8 KB).

## Select screen layout, costumes and the default time (owner, 2026-10-11)

Peach's first select preview showed Remix's Daisy colours. A content's slot
keeps its parent's fkind, so the source's same-fighter tests in the VS
select (`mnPlayersVSCheckCostumeUsed`, the free royal costume, the team
shade, the fighter count) took Falco, Wolf, Peach and Sonic for Fox: beside
a Fox, Peach's costume 0 counted as taken and she got costume 1. The select
now compares the content too (`ndsCss*` in battleship_mnplayersvs.c, routed
from the source's call sites by argument-token macros), and a content's
costume ids are its own rows (Remix's `set_default_costumes`,
NDSP4Present.default_costumes: four royal colours, then the red, blue and
green teams) rather than its parent's. Play ROM play-1011v: Peach beside
Peach gets 0 and 1 (pink, Daisy), Peach beside Fox, Falco and Wolf 0.

The shipping preset's match is Time, three minutes (was one); the VS menu
starts from it.

The select's screen uses its room (owner: "lots of extra room"): the grid
spans the width, eleven 29-unit cells from x 0 with portraits at 15/16
(Remix's was 24-unit cells at 0.8125, 264 units wide); the mode label and
BACK move 18 units up to the top edge; the four player cards, everything
on them and their 3D previews scale by 80/69 (owner: "the 3d preview cards
can be scaled to fit the space") at an 80-unit pitch from x 2, so they fill
the width, their centres stay on the source's preview roots (840 world units
apart, 80 source units) and the scaled 92-unit card ends at the bottom edge
(y 133 + 107); PRESS START, which sat under the panels, moves into the band
between the grid and the cards (`p4_css.py` SCREEN, read by the UI kit
bake -- placements, boxes, team buttons, the doors --,
`nds_menu_shell_css.c` -- hit boxes, cursor bounds, token homes, the door
animator -- and the preview roots and fighter scale in
battleship_mnplayersvs.c). The original-cast and 1P selects keep the source
layout. Play ROM play-1011x3: four previews filling their cards, 0
failures.

Two regressions the owner saw on play-1011x5, fixed in play-1011x6: the
previews were placed with a camera scale of 840 world units to 80 source
units, but the select's viewport maps them to the source's 69-unit panel
pitch (measured on a photo; world (0, 0) at source (160, 120)), so the outer
previews stood 14-17 pixels inside their cards; each now stands at its
card's middle (`ndsMNPlayersVSPreviewApplyOuterSlotInset`, 840/69). And the
doors' resident card copies were 7,844 B while a scaled card is 10,370 B, so
every slide failed the copy and streamed the card from NitroFS each tic (the
slow, flashing doors the underlay had fixed in September); the copy is now
sized by the bake (`NDS_MN_UI_KIT_CSS_GATE_MAX_BYTES`). play-1011x6: four
previews centred, all four card copies resident, 80 slide frames from
copies, select heap minimum 36.6 KB (43.2 before), 0 failures.

## Memory: mirrors, the event32 ledger, article data (2026-10-11)

A sweep of four-CPU mirrors on the shipping ROM (play-1011ko, Peach's
Castle, 900 frames) left six contents under the 25.6 KB floor: Bowser 2.1 KB,
Meta Knight 0.5, Dedede 17.6, Crash 23.7, Sonic 24.9, and Banjo out of heap
making the damage interface; the other twelve ended at 34-78 KB. Four causes
and their fixes:

- **The event32 normalizer's ledger** (battleship_sys_objanim.c) sized each
  player by its fkind, which for a content is its parent: Yoshi's and
  Jigglypuff's children took the parent's entry clip and its 768-entry
  event32-motion margin, though a content's motions are FIGATREE. Four
  Bowsers reserved 7,346 entries (53 KB) and held 650. A content now counts,
  per fighter, its largest event32 entry animation (`Event32Commands`: every
  fighter enters at once; only Meta Knight's entries stay event32, 518
  commands each) and, once, the entries its fighters were measured to share
  (`EVENT32_SHARED` in generate_p4_fighter.py: the live high-water of four of
  its CPUs on Peach's Castle over 1,500 frames on the lab ROM with the ledger
  forced to 4,095 entries, the stage's share on Castle included: Falco 1,630, Ganondorf 901, Wolf 593, Bowser 650, Marth 471, Roy 552, Wario 716, Peach 514, Crash 1,004, Lanky 866, Sheik 663, Banjo 842, Sonic 2,039, Dedede 580, Mad Piano 391, Meta Knight 171 (2,771 less his four entries), MRGAW 750, Snake 558),
  never under its high-detail material scripts, plus the rest of its material
  scripts in a battle of one or two (`Event32MatAnims`, counted from its
  model's parts). Counting only the model's scripts was not enough: four
  Sonics hold about 2,000 entries against 445 material commands (his
  effects' scripts), and a 1,807-entry ledger filled. A content without a
  measurement fails a shipping build.
- **Every content's data is resident in every scene**: MRGAW and Snake grew
  .main, .main.rw and .main.bss by 34 KB (Meta Knight's mirror 29 -> 0.5 KB
  from that alone). The P4 article textures' texels (105 images) moved to
  `nitro:/p4/entry_texels.bin` (55 KB raw), read at texture preparation
  (`NDS_ENTRY_EFFECT_COMPRESSION_FILE`). Each content's article corners
  (3,921 of the packet's 5,613) moved to `nitro:/p4/entry_corners.bin`, one
  block a content, loaded at battle setup for the contents in the match
  (`ndsRendererEntryP4CornersLoad`); the renderer reads a root's rows through
  `ndsRendererEntryEffectCornersSelect`. .main 1,842,848 -> 1,782,432 bytes.
- **Converted entry animations kept a key a frame**: an event32 animation
  converted to FIGATREE keeps the source's cubic keys with rates, which the
  compactor's exact rules do not re-encode, so Banjo's and Crash's entries
  stayed 29 and 20 KB and set their fighters' figatree heaps. Such a stream
  is now resampled (`_resample_stream`): its exact value at every frame, then
  the same joint-synchronised linear fit, accepted only when it never
  depends on the previous animation's pose and the simulation of both
  encodings agrees on every frame. Entry animations fit within 8 units
  (`ENTRY_ANIM_EPS`: no hitbox runs during an entry; under a degree and two
  units), the rest within 2. Banjo's entry is now FIGATREE; Meta Knight's
  event32 entries 650 -> 518 commands.

The low-water is the free heap at the end of the match, not at its start:
pools and caches grow through it (24.5 KB for four Meta Knights, 78 KB for
four MRGAWs, 56 KB of it in one stretch), so freed bytes do not all show.

Two faults the sweep found, fixed:

- **Two contents naming one file** (Marth's and Roy's entry, 0x0DA7): the
  root lookup by asset and offset returned Marth's root, whose corner block
  a Roy-only match never loads: 480 refused draws, 960 native failures in
  Roy's mirror. `ndsRendererEntryEffectRoot` now prefers the root of a
  content in the match.
- **Crash's dig effect played no animation**: its three tracks are Remix
  code-segment data (RAM_DATA) compiled into .main, and the ledger refuses a
  script no loaded file holds (reason 2, also seen in the heavy mix). The
  generator now emits a `"track"` row in the native AObjEvent32 layout
  (`event32_native_track`) and the routine attaches it as it is
  (`ndsBaseGcAddDObjAnimJoint`, as the guard poses do).

Shipping ROM (play-1011tz; Roy and Dedede on play-1011u), four CPUs on
Peach's Castle, 900 frames, low-water in KB: Falco 76.7, Ganondorf 74.1,
Wolf 48.4, Bowser 53.0, Marth 58.4, Roy 53.0, Wario 47.8, Peach 58.4,
Crash 49.8, Lanky 70.1, Sheik 67.8, Banjo 34.7, Sonic 34.5, Dedede 38.7,
Mad Piano 129.2, Meta Knight 61.6, MRGAW 50.2, Snake 65.6 -- every mirror
above the floor, 0 overflows, 0 native failures, 0 entry-texture failures,
0 corner misses. Below it: the heavy four-content mix (Crash, Lanky, Sonic,
Dedede) 7.3 KB, and Sonic's and Meta Knight's mirrors on Planet Zebes 12.6
and 19.8 KB (Zebes's stage share holds a 5,539-5,756-entry ledger). Peach
beside Mario (high detail) 66.1 KB; her select previews 0 failures.

### The ledger as sorted keys (2026-10-11)

The heavy mix's battle-start ledger (Crash, Lanky, Sonic, Dedede on Peach's
Castle) showed the event32 ledger as its largest single P4 cost: 45,640 B for
a 5,851-entry limit, of which 1,640 were live after 900 frames. Each entry was
a command pointer and a signature byte, and the open-addressed index over it
took a power of two above the limit in u16 slots (8,192 here: 16 KB). The
ledger is now one sorted array of u32 keys (battleship_sys_objanim.c,
`sNdsAObjEvent32Keys`): a command's word index in main RAM shifted up a byte
with its native word's signature below it. A lookup is a binary search, a plan
commits by sorting its keys and merging them from the top (a 256-key static
buffer, or the free tail for a stage layer's big plan, else key by key), and a
retired buffer's commands are one contiguous run removed with one memmove, so
the holes, tombstones, per-page counts (4 KiB of bss) and the index are gone.
4 B a command: the mix's ledger 45,640 -> 23,404 B, Zebes mirrors' 44 KB ->
~22 KB. Normalization itself is unchanged (the same words, the same
refusals; a command outside main RAM, which no loaded file holds, is refused
as reason 15).

Shipping ROM play-1011x3, four CPUs, 900 frames: the heavy mix on Peach's
Castle 22.1 KB (7.3 before; its ledger 45,640 -> 23,404 B), Sonic's and
Meta Knight's mirrors on Planet Zebes 46.5 and 37.9 KB (12.6 and 19.8), 0
refusals, 0 native failures. Lab, ledger forced to 4,095, 1,500 frames:
Sonic 2,039, Crash 883 and Falco 1,630 live entries as before (Crash's two
dig-track refusals are gone), Meta Knight 2,232 (2,771 before his entries
were resampled), 0 refusals. The select's new layout also moved the match's
random state, and a row's low-water moves with its match's events: Sonic's
Castle mirror read 25.5 KB (34.5 before) with an 11.6 KB smaller ledger
(see "AObjs past the pool" below).

### AObjs past the pool: shield poses kept theirs (2026-10-11)

Four Sonics on Peach's Castle ended at 25.5 KB because the heap's per-AObj
fallback had taken 22 KB: 612 AObjs past the 384-entry pool, 511 of them
made by the guard's pose. The native shield pose parses a joint's script
into AObjs, plays it once and clears the animation (nds_shield_pose.c), and
the AObjs stayed on the joint: a fighter that had shielded once held one per
animated track of every joint (Sonic 128) until it was ejected, since
nothing plays them again and fighter motions run in the pose engine's own
tracks. They now go back to the pool after each apply
(`gcRemoveAObjFromDObj`); the shield-pose generators check every script for
independence from stale AObj state, so the poses are unchanged. Kirby keeps
his: his copy of Donkey Kong's charge is an animjoint motion, which the
generic player parses over whatever his joints hold. Shipping ROM
play-1011x4, 900 frames: four Sonics 25.5 -> 47.2 KB (10 AObjs live at the
end), the heavy mix 22.1 -> 36.3 KB, 0 native failures.
Official gate ROM, the commit before against this one (each built from
the same tree with only these two files swapped): replay digest IDENTICAL
over 1,960 frames, WORK P95 1,037,888 -> 1,038,208, two-VBlank presents
96.9% -> 96.7% (`artifacts/performance/2026-10-11_ledger-shield-digest`).

## Mr. Saturn, Peach's rare pull (2026-10-11)

items/MrSaturn.asm and the pull's last branch are ported in `nds_p4_peach.c`:
one pull in 128 is rare, and three rare pulls in six now make Mr. Saturn (the
Beam Sword and Bob-omb branches were there). His ITAttributes (appended to
Remix's ITCommonData at 0x10B0) and his model (Remix's common items file
0xA96) are Peach's item subset file (`SUBSET_FILES`, 4,816 B) in her
special-file slot 4. That slot names her parent's file (FoxSpecial4, the
reflector's list), which no Fox code reads by the slot
(`PARENT_UNREAD_SLOTS`): FoxSpecial1 names it as an extern and still brings
it. He is P4 item kind `NDS_P4_IT_KIND_MRSATURN` with Remix's six states: a
grounded wait of 120 frames, then a walk at 8 a frame that turns at ledges
(the red shell's edge test) and walls and ends him after 1,500 frames; the
aerial, held, thrown and falling states are the turnip's except that he
lands back into his wait, a thrown Mr. Saturn that meets a hurtbox, shield or
clang bounces back saying one of his three words (FGM 0x5FD-0x5FF), and he
deals 6 damage and 30 shield damage. Read as assembled: his throw leaves the
hurtbox off (the turnip's enable is commented out); the walk's "switch
direction" block rewrites the facing it reads (the edge test has already
turned him); the drop's halfword 0x2D2 write (0x5674) sets pickup_wait 1,383
and only is_unused_item_bool. His model is a 32x32 CI4 quad whose TEXID 4/5
walk and 6 faces front; the tree's first DObj has no list, so the maker's
tree eject leaves the textured one as his root, and held it is the hand
joint's child, whose face and x scale the held routine sets. His sounds are
Remix's add_sound voice and two add_fgm cues that replay it 4 and 7
semitones lower (`EXTRA_SOUNDS` in p4_audio.py). Shipping ROM play-1011x5, four Peach CPUs on Peach's Castle with every pull forced to Mr. Saturn (gdb), 2,400 frames: seven Mr. Saturns, carried and thrown by the CPUs (1,363 held ticks), waiting, then walking both ways and turning (485 walk ticks, faces 4/5 swapping), drawn (photos held and walking), 0 native failures, 0 subset misses, heap low-water 59.5 KB.

## Owner playtest 1011x5 fixes (2026-10-11)

- **Ganondorf froze at Results.** The podium's motion heap was sized from
  the resident span of the parent's main motion file, but
  `lbRelocGetForceExternHeapFile` writes the whole loaded image, and
  Ganondorf's (Captain's) image is the larger: the write ran past the heap
  and the next tree walk faulted. `ndsRelocFullLoadSize` (the asset's
  allocation size, or the generated one when larger) now sizes it, and the
  heap keeps what that asks. play-1011x7, Ganondorf beats Mario: "GANONDORF
  WINS!", START returns to the character select, the stage select and a
  second match (low-water 111,768 B, 0 native failures).
- **Wario jumps lower (owner: x0.75).** Jump height goes with the square of
  the take-off speed, so `OWNER_ATTRIBUTE_SCALES` (generate_p4_fighter.py)
  multiplies his `jump_height_mul` and `jump_height_base` by sqrt(0.75) in
  the generated Main (deviation class "owner"); the double jump multiplies
  the same speed (`jumpaerial_height`), so it follows.
- **Wario's mustache and Meta Knight's wings drew as opaque blocks.** The
  texture key forced DS texels opaque (`alpha_ignores_texels`) whenever the
  combiner's C/D alpha inputs skipped the texels, but their lists take the
  texel alpha through A/B of the second cycle and draw XLU (render mode
  0xC4113878): when the blender reads alpha (CVG_X_ALPHA, ZMODE_XLU, or
  FORCE_BL without ALPHA_CVG_SEL), the key now follows the whole two-cycle
  alpha path. Select-screen photo play-1011x7: the mustache keeps its shape,
  the wings have no black edge. The first version also counted ALPHA_CVG_SEL
  alone, which every antialiased opaque mode sets: nearly every stage key
  changed, Dream Land's pinned corpus missed, and the gate went to three
  VBlanks (WORK P50 759K -> 1,428K, polygons 580 -> 424, digest identical).
  The corpus generator (`generate_battle_playable_static_textures.py`)
  mirrors the corrected test; its corpus is byte-identical under it. The
  census had been failing since the 1P task-icon OBJ bank was added (it did
  not know the bank), so no generator change could build: it learns the bank
  and is re-pinned (only `native_obj_bank_reserved_bytes` moved).
- **Ganondorf's up tilt blast trailed its animation.** Remix's frame-speed
  multiplier with flag 1 (FSM 0xD0) slows every joint below the top joint
  and leaves the top joint, the motion script's clock; the native pose took
  the top joint's speed for all its entries, so the up tilt's windup
  (x0.1875, motion 0xAE) played at full speed and the blast, timed by the
  script, came late. The pose now takes the speed of the top joint's first
  child (`gcSetAnimSpeed` writes every joint alike, so the original cast
  reads the same speed). His idle (x0.70) and motions 0xA8-0xAC (x0.90) are
  flag 1 too and now play at Remix's speed (probe: idle pose speed Q12 2864).
- **Mad Piano's teeth flickered through his closed lid.** The DS keeps z/w
  to fifteen bits in Z-buffering mode, so with the source's near plane of
  256 one depth step at a battle distance of 5,000 is about 3 world units;
  his lid's tooth layer is a double-sided polygon 1-3 units under the lid's
  face (model 0xA99). After the source's camera function, the battle
  camera's near is now a quarter of the eye-target distance, never past the
  nearest fighter less 400 units (the screen KO, the pause close-up) and
  never under 256 (`ndsCameraUpdateDepthNear`, battleship_gmcamera.c): one
  depth step grows with z instead of z squared (about 0.6 units at 5,000).
  Draw only: the camera matrix feeds draw-side readers alone and the replay
  digest folds no camera state; every renderer path reads the same CObj
  field after the sim, so stage, fighters and particles share one mapping.
  No list sets a depth-fog position. play-1011x8, two Mad Pianos on Peach's
  Castle: near 1,001 at eye z 3,858, 0 native failures. The photo A/B is
  owed: window captures were black while the console was locked.
  **Reverted 2026-10-12** (owner: no difference to the teeth, and the
  pause close-up lost the stage); see "Owner playtest 1012i fixes".

Official gate ROM with all five (`artifacts/performance/2026-10-11_pose-camera-digest`,
`fixed.*` against the ledger commit's `cand.*`): replay digest IDENTICAL over
1,960 frames, WORK P50 759,488 -> 765,824, P95 1,038,208 -> 1,045,760,
two-VBlank presents 96.7% both, polygons and vertices per frame unchanged
(580/1,381). The same ROM with the near pinned at 256 measured the camera
change apart from the alpha mistake above (`pin.*`, `noalpha.*`).
Mad Piano photo A/B (`pianobat` x7 against `pianobat11` x11, the same replay
frame): the red interior and teeth showed through both lids before, the lids
are clean after at frames 240, 300, 360 and 420.

Later the same night (play-1011x12):

- **Ganondorf's select pose.** His Selected row is Captain's clip named by
  number (file 429). The select answers the cast's Selected clips from
  compiled tables keyed by the ll...FileID token, so the number took the
  stream path and he stood in his bind pose; his slot heap was also 96 B,
  because `lbRelocGetFileSize` answers sizeof(Sprite) for a resident file
  (the Results podium's trap again). The cast's Selected numbers now map to
  their tokens (`ndsBattleShipCSSSelectedToken`), and the slot heap takes
  the whole load or the compiled copy, whichever is larger
  (`ndsMNPlayersVSPreviewClipBytes`). Photo: posed.
- **Select stutter while panning.** A carried token crossed the P4 grid's
  cells in about 7 tics against a dwell of 2, so every portrait it passed
  started a load: 34 hovers, 30 loads, each update then reading its 8 KiB
  (~9 ms) and the unsliced stages stalling 6-17 VBlanks. With a dwell of 8
  the same pan makes 5 loads and no frame of three or more VBlanks while
  panning (two-VBlank frames 28 -> 18). Open: the first stop on a P4
  fighter still pauses about 17 VBlanks in its commit stage
  (`ndsFTManagerSetupPreviewFilesP4` loads the content's files at once).
- **The shipping ROM carries the freeze report.** `TARGET=smash64ds` builds
  with `NDS_FREEZE_DIAGNOSTICS=1` (every measurement target keeps it off): a
  battle frame that stops for 3-4 s draws the stall report (PC, stack, BGM,
  radio, GX) on the lower screen, an exception draws the exception report,
  and an out-of-memory halt -- which spun silently -- now adds its arena,
  request, what was left and its caller (rows 17 and 20). ROM +10 KB.

Later (play-1011x14):

- **Meta Knight's Mach Tornado is translucent.** Its two lists (model
  0x15EE, 0x97C0 and 0x9AA0) draw a 64x64 IA8 mask under a blender that reads
  the mask's alpha. Two things drew it wrong: the native owner's family 0
  takes the mask's intensity for the colour, and the mask is dark where its
  alpha is not (`generate_p4_fighter.py` OWNER_IA_INTENSITY_MAX raises the
  intensity to full, a render-class deviation); and the DS converted IA8 with
  one-bit alpha, so almost every texel (alpha 5-15 of 15) was opaque. The
  fighter draw now brackets a request (`sNdsRendererHardwareTexelAlphaGraded`)
  under which such an IA8 tile bakes as GL_RGB8_A5: the nibble as five-bit
  alpha over an eight-step grey. BattleShip's fighters carry no IA image and
  effects, items and stages are outside the bracket, so no other key or the
  static corpus moves. Photo: a white translucent swirl. Not exact: the
  source also multiplies by SHADE alpha per vertex, which the DS cannot.

## Multiplayer freezes: the stress run (owner, 2026-10-11/12)

The owner's report: four-player matches froze on Hyrule and Sector Z, the
host sometimes with nothing on screen. The P3 lab sweep's repro 3
(`NDS_NET_LAB_SWEEP_REPRO=3`, nds_net_lobby.c) is that report as a run: two
linked consoles, two humans and two level-9 CPUs, four heavy P4 mixes in
turn on Hyrule and Sector Z, every item at the highest rate, five-minute
matches. 25 minutes in the melonDS-mp harness: no freeze, no desync, no
out-of-memory halt, heap low-water 11,952 B. The same rows as local
four-CPU matches (`shipbattle.ps1`, items poked at VeryHigh) found the two
faults the harness had not reached:

- **Out of heap at entry on Sector Z.** Sonic, Lanky, Meta Knight and Wolf
  ran the general heap out making their parts at entry (request 136 B,
  44 left): `ndsSyMallocOverflowHalt` spins silently, the frozen host of the
  report (the shipping freeze report now shows it, rows 17 and 20). Sector Z
  costs 60 KB more than Peach's Castle at battle start, most of it the map's
  extern closure (`mpCollisionInitGroundData`, 68,048 B against 18,736). A VS
  battle lends the whole front-end overlay, and the fighters' packs, images
  and files left 41,696 B of it idle: the event32 ledger, the contents'
  match tables, the shield poses and the entry corners now take that tail
  first (`ndsVSBattleOverlayTryAlloc`, nds_frontend_overlay.c; any other
  scene, or no room, takes the heap as before). The mix now ends a
  3,000-frame match with 16,784 B (under the 25,600 floor: open, below).
- **A frozen battle: the hurtbox inversion's own loop.** Marth, Roy, Sheik
  and MRGAW on Sector Z froze at frame 822, the freeze report pointing into
  `gmCollisionSetInvertMatrix`: a zero determinant, where the source spins
  forever ("zero div in gcSetInvMatrix()"). Sheik's hip had scale
  (0, 1.25, 0) on the first tick of her aerial neutral special. The native
  pose engine (nds_ft_pose.c) starts each clip's tracks in fresh slots at 0,
  while the source keeps a joint's AObjs across clips and every key takes
  the old target as its base: a first key with a payload (a line or curve
  over some frames, or a delayed step) interpolates from the previous
  clip's pose in the source and from 0 in the engine. None of BattleShip's
  1,736 clips has such a first key; 46 Remix clips do (rotation lines, and
  Sheik's scale X/Z over 7 frames). Such a key now starts from the joint's
  current rotation or scale (`NDS_FT_POSE_FIRST_KEY`), what the previous
  clip left there. The same match runs 3,000 frames (play-1012b).
- **The stages' background actors drew nothing.** The same stress match
  recorded 990 native failures on Sector Z's rocket (ExternDataBank109
  0xAB20, an effect GObj). The VS stages' efground actors (ef/efground.c
  `dEFGround*EffectDescs`: Sector Z's rockets and ship, Zebes' Ridley and
  ship, Kongo Jungle's bird, Yoshi's Island's birds and Shy Guys, Saffron's
  Butterfree, Charizard's fire, Beedrill and Pidgey, Dream Land's Dedede)
  appear 100-266 s into a match (`efGroundMakeAppearActor`: wait 6,000
  plus up to 10,000 ticks), so no census match ever met them. Their 17
  roots now compile as baked effect roots
  (`generate_nds_native_item_baked.py`, GOBJ_EFFECT; Lakitu and Bronto Burt
  keep their hand-written owners). Probe (play-1012d, four Marios, 900
  frames, the actor wait forced to 90 ticks): Sector Z 23 actors, Zebes 24,
  Kongo Jungle 25, Yoshi's Island 21, Saffron 23, every effect id of each
  stage, 0 native failures. Dream Land still failed on Bronto Burt (5,956
  in 900 frames) and Peach's Castle on Lakitu (11,007 in 600): their
  hand-written slots (generate_nds_native_ef_lakitu_bronto.py, about 10 KB
  of code, tables and workspaces) had never drawn. They tested a loaded
  file's liveness against the taskman heap generation, while a loaded file
  carries the reloc scene generation, and the Bronto slot expected the
  sprite MObj on the joint above the one the source gives it, so both
  refused every draw and the actors fell to the generic walk. The slots,
  their packets, executors and NDL route are deleted; the four roots
  (Lakitu's three quads, Bronto Burt's) are baked effect roots like the
  rest, whose live material is the drawing DObj's own MObj. The billboard
  row cores the generic walk shares keep their host test
  (`test_ef_ground_billboard_rows.py`).
- **Owner images without dense vertices.** The lean fighter renderer, the
  only one a shipping build links, reads a dense vertex's s/t and, at a
  texgen site, its normal; an imaged owner's shade words already ride in
  dense_normals. A lean build now images a P4 owner's `dense_vertices` as a
  zero-length member (`LEAN_TRIM`, generate_nds_native_owner_images.py):
  PreparedDense carries the source s/t (the production path that rewrites
  them is not linked), and the texgen patch recovers the s8 normal from the
  vertex's NORMAL word, which packs it one-to-one
  (`ndsFighterPacketTexgenRgba`). 12 B a vertex: Banjo's low image 52,768
  -> 37,132 B, Lanky's 31,700 -> 22,100, Crash's 40,440 -> 28,080. An owner
  with texgen and an unlit run (whose dense_normals hold colour words)
  would keep its dense vertices; none of the eighteen has one. Photos of
  the same match with full and trimmed images are identical.
- **Memory rows (play-1012g, items very high, 1,500 frames).** With the
  trims, the no-attack motions' compaction (a content's figatree heap is its
  largest main-motion clip, often its teeter or taunt: Banjo 22,428 ->
  13,492 B) and the Lakitu/Bronto code gone (.main -7.2 KB, .bss -1.5 KB,
  which every scene's heap gets back), every row clears the 25,600 floor.
  The heaviest four (Banjo, Crash, Lanky, Bowser): Kongo Jungle 5,872 ->
  27,292 B, Saffron 13,520 -> 40,044; on play-1012e (before the deletion)
  Sector Z 18,560 -> 34,464, Zebes 22,228 -> 42,916, Yoshi's Island 19,024
  -> 33,672, Mushroom Kingdom 54,652. Banjo, MRGAW, Crash and Lanky:
  Saffron 5,984 -> 30,488, Sector Z 24,948 -> 33,652. The P3 stress's
  Sector rows: 43,352 and 59,040. 0 overflows, 0 native failures.

## Four-CPU P95 of the contents (2026-10-12)

The first P95 measurement of the contents: four CPU mirrors of each on Dream
Land, items on, the clean sweep ROM (gate target plus the boot-poked stage,
roster and content words, `NDS_LAB_FOURCPU_WORDS`; build-sweep-p4a), 1,960
frames, WORK P95 (two-VBlank frames of 1,961):

| Roster | P50 | P95 | <= 2 VBlanks |
|---|---|---|---|
| preset originals | 780,672 | 1,078,720 | 1,884 |
| Ganondorf | 784,960 | 1,079,552 | 1,884 |
| Wario | 756,928 | 1,190,528 | 1,834 |
| Sonic | 848,192 | 1,397,184 | 1,612 |
| Marth | 886,784 | 1,568,320 | 1,647 |
| Bowser | 872,448 | 1,601,024 | 1,659 |
| MRGAW | 779,328 | 1,665,408 | 1,692 |
| Peach | 895,168 | 1,828,480 | 1,599 |
| Sheik | 801,408 | 1,834,112 | 1,651 |
| Dedede | 906,752 | 1,882,880 | 1,497 |
| Lanky | 755,456 | 1,995,968 | 1,733 |
| Banjo | 850,560 | 2,002,368 | 1,491 |
| Meta Knight | 829,632 | 2,532,032 | 1,604 |
| Crash | 841,856 | 2,794,048 | 1,352 |

The medians are the cast's; the tails are events. Two owners, from the
lean lab counters (`NDS_DIAG_COUNTERS=1` sweep ROM, 1,200 frames):

- **Lean list rematerializations.** Four Crashes materialized 682 lists
  (385M ticks, 564K each) against 45 for the original cast: 309 of their
  key events were tint-set moves. An untextured body whose prim is not
  white draws textured with an 8x8 tile of that colour, and four Crashes
  carry more than the table's 16 colours: 344 builds, 328 evictions, 829
  full-table misses, and every build or eviction moves the generation that
  retires every tinted list. The table now holds 48 (16-byte rows).
- **The simulation's tail (gcRunAll): the contents' clips loaded loose.**
  Banjo's lists hit (47 materializations), yet his gcRunAll P95 was 1.40M
  (Meta Knight 1.93M, Crash 1.31M) against the cast's 531K. A task37 census
  of four Banjos (build-prof-banjo, frames 200-1,400, split over the gate:
  304 over-gate frames, 1.78M cycles a frame over the rest) put the
  premium in animation-cache misses: ~330K in storage reads (the card read,
  its PXI round trips and mutexes, NitroROM's path walk), ~190K in the
  loaders' normalizers (byte swap, AObj16 normalization and validation,
  the sprite pass) and 158K in `ndsP4FindAnim`, which the AObj16
  admission gate asks twice for every animated joint of every clip it
  binds and which walked every content's list (~5,000 ids). The cast
  reads its clips from `animation/ftanim_stream_pack.bin`: compacted at
  build time, one sector-aligned storage command a clip, nothing to
  normalize. The contents' clips now form its sibling,
  `animation/ftanim_p4_stream.bin` (scripts/p4/p4_anim_stream.py, the same
  generator code; 2,012 clips, 7.8 MB; event32 and spline clips stay
  loose), read through the same paths; its directory stays in the ROM
  (one 8-byte row read a miss, no resident memory). `ndsP4FindAnim`
  binary-searches each content's sorted list behind a last-id memo.
  Replay digests of the four-Banjo and four-Crash runs are identical
  before and after (the animations are the same bytes).

After both (build-sweep-p4c, Dream Land, WORK P95): Lanky 1,995,968 ->
1,085,696, Sheik 1,834,112 -> 1,138,432, Marth 1,568,320 -> 1,201,408,
Bowser 1,601,024 -> 1,226,240, MRGAW 1,665,408 -> 1,255,680, Sonic
1,397,184 -> 1,281,920, Dedede 1,882,880 -> 1,321,088, Peach 1,828,480 ->
1,329,792, Banjo 2,002,368 -> 1,378,688, Meta Knight 2,532,032 ->
1,419,840, Crash 2,794,048 -> 1,894,016; the originals 1,070,912. A second
census of four Banjos (167 over-gate frames, 1.34M premium) leaves lean
rematerializations (~1M cycles each for his 2,600-word list, ~240K a
frame averaged over the over-gate frames) and the guard pose, which the
source re-applies to every joint every frame a fighter shields (NSP1
unpack, AObj add, parse and play: ~130K).

The gate (cast only, build-gate-o1012) with the stream, the search and the
48-colour table: WORK P50 767,808, P95 1,048,000 (2026-10-11: 1,038,208),
1,896 of 1,961 frames within two VBlanks, replay digest identical.

Then two more (build-sweep-p4d, same rows):

- **Crash's lists kept the global texture fence.** A lean list records
  where each tint-tile bind sits so a moved tile set re-points it; Crash's
  lists make 32-34 such binds and the record held 24 (the cast's most is
  Kirby's 19), so 300 of his 305 lists overflowed and fell back to the
  global texture fence: every texture key move anywhere (stage, items,
  effects) re-materialized them, 117 times in 1,200 frames.
  `NDS_FIGHTER_PACKET_TINT_BIND_MAX` is 40 (192 B more per packet, in the
  borrowed framebuffer).
- **`ndsP4RelocAssetPath` walked every content's file list** (the loader's
  find and the relocation's range checks ask it for every P4 file): a
  binary search a sorted list now, like `ndsP4FindAnim` (Kirby's power
  animations too).

WORK P95: Crash 1,894,016 -> 1,474,304, Peach 1,329,792 -> 1,232,320,
Meta Knight 1,419,840 -> 1,397,440, Banjo 1,378,688 -> 1,364,096, Dedede
1,321,088 -> 1,317,056; the originals 1,070,912 -> 1,070,464.

## Owner playtest 1012i fixes (2026-10-12)

- **The battle camera's depth near is reverted.** The owner: the Piano's
  teeth flickered the same before and after it, and the pause close-up lost
  the stage behind the fighter (geometry inside a quarter of the close-up's
  eye distance clipped). The camera keeps the source's near of 256 again.
- **Mad Piano's teeth sit 9.85 units deeper.** His twenty teeth (model
  0xA99, vertex blocks 0xB38 and 0xD18 of the lid joint) hang from the lid
  0.3-2.7 units under its top, which the lid draws after them, so the
  teeth won every depth tie and showed through the closed lid in the
  select and in matches. The generator moves both blocks along the lid's
  normal into the mouth (`OWNER_VERTEX_SHIFT`, generate_p4_fighter.py; the
  owner's "moved inwards just a tad"; a render-class deviation in the
  manifest). Still showing in play-1012l ("move inward more please"): with
  the lid closed the teeth hang along its rim in the planes of the case's
  side walls (left x -273 against the wall's -274, right 372 against 377,
  the curved rim alike), so they z-fought the body. Each tooth triangle
  now also moves 20 units toward the lid's centre along its own normal,
  parallel to the lid top (`OWNER_TRIANGLE_INSET`; play-1012m).
- **Meta Knight's Mach Tornado is cream, not white.** Its lists colour
  PRIM * SHADE * PRIM with PRIM (248, 248, 192) and lighting off; the owner
  draws TEXEL0 * SHADE with the mask's intensity raised to full (2026-10-11).
  The tornado's 54 vertices take PRIM squared, (241, 241, 144)
  (`OWNER_VERTEX_TINT`), which is the N64's colour.
- **The select's outer previews were cut at the screen's 3D window.** The
  fighter camera drew into the source's (10,10)-(310,230) window by
  narrowing GFX_VIEWPORT to it (DS 8..247 x 8..183), which also clipped
  every fighter at its edges: the P4 select's wider cards put the 1P and
  4P previews partly outside it (Meta Knight, Bowser). The viewport stays
  whole and the camera takes the window's projection instead (fovy 30 ->
  32.588, aspect 4/3 -> 1.30370: the same pixels; `ndsMNPlayersVSFitFighterCamera`).
- **Door slides streamed their card every tic.** The arena copy of the card
  under a sliding door (2026-10-11) needs four copies plus 48 KiB behind
  them at the select's load, which the all-content image never has, so
  every sliding tic opened the surface pack and read the card again (four
  slots at once at the entry and back from a match). BG2's offscreen rows
  (192-255 of its 256x256 bitmap, 32 KB nothing draws) hold the copy
  instead: saved from the screen right after the first streamed blit,
  restored VRAM to VRAM after (`ndsUiKitSaveSurfaceRect`; three P4 cards or
  four original ones fit; A/B word `gNdsMenuShellCssVramUnderlay`). The
  all-content walk ROM (walk-1012a, two doors sliding per select visit) with
  the arena copy forced off: streamed 24 frames of three or more VBlanks a
  visit, the VRAM copy 9, the arena copy 8; the mid-slide frame is
  pixel-identical to the arena copy's.
- **MRGAW's CP badge could not be picked up.** The P4 grid starts at x 0
  (eleven 29-unit columns), so a token on its first column (x 3-24) lay
  left of the hand's grab point (cursor x + 25) at the source's left clamp
  of 0. The hand may run to x -20 on P4 builds.
- **Bowser showed through his shield.** The shield is a camera-facing disc
  pulled toward the eye so the body it encloses stays behind it; the pull
  was half the bubble's radius, short of a big body's front. It is 1.25
  radii now (floor 150), and the disc shrinks by the distance it moved, so
  its size on screen is unchanged (`ndsEFManagerShieldQuadProcDisplay`).
- **The run texture memo covers every run.** Its 67 rows were indexed by run
  number, and a P4 owner image appends its model parts' runs after the
  canonical ones (Banjo's reach 160): every run past 66 took the full texture
  resolver at every list materialization (a quarter of a materialization's
  cost). Rows are run number modulo 67 now, tagged with the run in a byte
  the row's padding had; no memory.

Gate with all of the above (build-gate-p1012,
`artifacts/performance/2026-10-12_p4-fixes`): WORK P50 761,920, P95
1,039,936, 1,897 of 1,961 frames within two VBlanks, replay digest identical.
Memory rows (play-1012k, low-water against the 25,600 floor): hvjg 28,412
(was 25,756: .main shrank a kilobyte with the camera code), w4sf 29,208,
w4sz 33,092, hvsf 39,212; 0 overflows, 0 native failures.

## Lean walks, the guard pose and the select's panning (2026-10-12)

Four-CPU WORK P95 on Dream Land (clean sweep ROMs, 1,960 frames), before
this section -> after it: Meta Knight 1,397,440 -> 1,270,976, Crash
1,474,304 -> 1,404,096, Banjo 1,364,096 -> 1,285,376, Dedede 1,317,056 ->
1,218,944, Peach 1,232,320 -> 1,184,768, Sonic 1,289,728 (unchanged:
his tail is the simulation, SRC P95 688K); the originals 1,070,464 ->
1,064,384.

- **The guard pose is memoized.** The source re-applies the shield pose to
  every joint every tic a fighter shields (NSP1 unpack, AObj add, parse,
  play, remove). The posed joints' rows are a function of the fighter, the
  pose package and sector, the shield angle and the animation speed, so
  four slots of 48 rows live in the framebuffer arena's tail (6,400 B past
  the packet arena) and a repeat copies them back (`nds_shield_pose.c`).
  Banjo -90K, Dedede -74K, Peach -55K. Sonic's memo never filled: the
  track walk that names what a script writes stopped at the original cast's
  58 scratch words, and two of his scripts run longer (his package's scratch
  is 148), so it read them as unbounded and the fill refused. It walks the
  package's own scratch now: four Sonics, 1,800 frames, 94 of 97 applies
  replayed (play-1012w; 0 before), every apply and play count unchanged.
- **VTX_10 at half scale.** A corner past +/-511 source units put the whole
  list on VTX_16 (two words a corner): Meta Knight, Dedede, Lanky and MRGAW
  carry 4-24 such vertices, and Meta Knight's 3,800-word lists outgrew an
  entry and went wide. A list whose corners fit +/-1023 now takes VTX_10
  with the unit halved (rounded up) and its translation rows and P' row 3
  at twice VTX_16 scale (`NDS_FTR_LEAN_ENTRY_VTX10H`); a vertex moves at
  most one source unit.
- **A list's walk is remembered by its key.** A slot-wide memory of the
  corner format and width was tried first and cost Meta Knight +88K: once
  any list needed VTX_16 (which never declines) or went wide, every later
  list of the slot did, and wide lists evict the other entry (walks 132 ->
  221 in 1,200 frames). The memory is per key now (16 24-bit tags a slot,
  `walk_memo`); a slot-wide default only starts new keys at half scale. A
  walk also stops at its first capacity fault or VTX_10 corner fail instead
  of finishing a list it is about to discard.
- **The parked-list pool holds up to sixteen** (was eight) where the battle
  heap has them above the spare floor (25,600 + 36,420 B): four Meta Knights
  still re-derived 43 of 93 lists at eight.
- **The walk's texture binds go through a memo.** A third of a P4
  materialization was the full texture resolver re-resolving a texture the
  walk or the slot's last walk had bound (Banjo 294K ticks a walk, Crash
  189K, Meta Knight 126K). Rows keyed by the run (tables, run index, battle
  slot), the owner memo's live inputs, the render tile and its TMEM load
  replay the resolver's cache-hit tail (`ndsFtrLeanBindTexture`; 256 rows,
  5 KB of the battle heap). The lab verify arm (`gNdsFtrLeanTexMemoVerify`)
  ran the full resolver on 3,846 hits over Meta Knight, Crash, Banjo, Sonic
  and the cast: 0 disagreements. Texture time a walk -38% (MK), -23%
  (Crash), -17% (Banjo). Later the same day the key dropped the run and
  the slot (the resolver reads neither; Banjo's 161 runs across four
  slots filled 703 rows in 1,200 frames) and ends in an avalanche (the row
  is the key's low byte, and aligned image addresses had crowded a few
  rows): misses Banjo 703 -> 139, Crash 366 -> 188, Meta Knight 314 -> 181;
  the verify arm agreed on 4,901 hits.
- **The select's panning.** A pan with the token carried measured 15-VBlank
  stalls and a two-VBlank frame on every portrait it crossed. Three causes:
  (1) every open of a P4 file walked `nitro:/reloc/p4`'s 2,161 FNT names,
  two ARM7 storage reads a name: the commit of Mad Piano's 11 KB motion file
  took 14 VBlanks in its five opens. One buffered pass at boot keeps the
  directory's (asset, file id) runs and the linker wraps
  `nitroromResolvePath` (P4 builds, `scripts/p4/p4_rules.mk`) so every
  resolver answers "reloc/p4/<id>" from them: 363 scanlines now. (2) The
  carried token re-populates the cards on every portrait, and that re-blit
  the mode label and BACK (5.8 KB from NitroFS, ~230K ticks): once per entry
  and mode now. (3) Streams read through libfat's FILE, whose buffer is not
  32-byte aligned, so each 1 KB refill went to the ARM7 as a bounced head
  and a body (27 round trips for a 10 KB card); a `nitro:/` stream reads by
  file id now, and the UI kit's staging is aligned with each read rounded
  to whole lines (8 round trips). Pan of 261 frames (play-1012o against
  play-1012m, probe `panprobe4`): one VBlank 256 (was 240), two 3 (was 18),
  three 2 (0), four or more 0 (3, worst 15 -> 4).

Gate with all of the above (build-gate-r1012,
`artifacts/performance/2026-10-12_lean-walks`): WORK P50 755,584, P95
1,034,304, 1,900 of 1,961 frames within two VBlanks, replay digest
identical (DGSA/DGSB as before).

## The stage select's 3D previews (owner bug, 2026-10-12)

The stage select showed only the panel's 2D art: the menu shell compiles
out mnMapsStartScene, which makes the source's preview camera and model.
`battleship_mnmaps.c` now builds the preview the way mnMapsMakePreview
does and the battle's native stage owner draws it from the stage's own
packet (`ndsMNMapsPreview*`, driven by the router's SSS frame).

- **The model is the battle's packet.** Each layer GObj is made like
  mnMapsMakeLayer (the source's per-stage scale, the first animation frame,
  mnMapsMakeModel's hides) but on grDisplayMakeGeometryLayer's procs and
  links, so the owner admits it; `ndsRendererAdapterSetStagePreviewLayers`
  names them in place of gGRCommonLayerGObjs. A packet that also captures a
  battle-only actor (Zebes' acid, Saffron's gate, Mushroom Kingdom's scale,
  Dream Land's Whispy and flowers) gets a stand-in built from the same map
  file as the stage's ground code builds it, with its root hidden: the
  source preview shows the four layers alone, and the owner admits a packet
  only whole.
- **The preview's camera.** mnMapsMakePreviewCamera's default camera
  (PerspFastF then LookAt) is accepted where the stage paths asked for the
  battle's 0x4C camera (the stage camera split and the billboards' MVP
  recalc); its window fit is the character select's. A billboard takes its
  ancestors' scale as the source's gGCScaleX does (Sector Z's engine glow
  and Saffron's Poke Ball drew five times too big). The scene's light is
  mnMapsFuncLights. In preview mode every binding is composed each draw
  (Zebes' rigid ground drew nothing on the rigid path) and the compiled
  stage GX program is not used (its worlds are the battle's).
- **Loading without a stall.** One load and first draw took 8-11M ticks
  (15-20 VBlanks) in one tic. The cursor resting six tics starts a load
  that takes one step a tic: each packet asset, then the map file's tree
  (now only what the assets left out, `ndsRelocExternTreeUnloadedSize`) and
  the packet, then the model. Its first draws upload at most four textures
  each (`gNdsNativeStageWarmUploadLimit`) and decline until whole; the 3D
  layer shows the model from the first whole draw, and the old model leaves
  as soon as the cursor moves. Dream Land, whose battle pins its textures
  up front, warms the same way. The select's first stage loads whole at
  entry.

play-1012p, each stage reached from the left of its row and held 60 tics
(probe `sssph_<slot>`): all nine draw natively; the slowest load step
0.74-2.64M ticks (one packet asset; Dream Land's and Sector Z's largest),
the slowest draw 0.70-1.87M (a warming draw), the select's tick HUD worst
3-5 VBlanks over the run (it was 13-22 with the whole load in one tic). N64
references (mupen64plus 2.5, the Maps scene cheat-booted per stage,
scratchpad `n64/setup_maps.py`) match the port's placement and size on
eight; Dream Land's model sits about 10 pixels low and its tree's stars
draw small (open).

Owner playtest of the first build, two more:

- **Stages went missing after a few.** A warm upload marks its texture
  `stage_warm`, which eviction skips for the scene (a battle's stage keeps
  its set). After about six previews the VRAM held the earlier stages and
  Zebes, Mushroom Kingdom, Yoshi's Island and Saffron warmed forever. The
  preview releases its stage's warm textures when it drops the model
  (`ndsRendererHardwareReleaseStageWarmTextures`). Each stage probed alone
  had passed; a browse of all ten (`shipbattle.ps1 -SssBrowse`, play-1012p)
  shows all nine models.
- **The labels in front of the model.** The source's preview camera
  (priority 65) draws the model in front of the two wallpapers and behind
  the icons, the cursor, the plaque, the labels and the name and emblem;
  on the DS the 3D layer sat in front of the whole BG2 screen, and Congo
  Jungle's trees covered the logo and the stage name. The kit bakes those
  parts again for BG3, keyed: `SSS_FRONT` at the entry and
  `SSS_PLAQUE_FRONT_<slot>` per cursor move (its box cleared first). Their
  colour is composited over the stone (`SurfaceSpec.matte`) while the key
  stays their own coverage, so their edges match BG2 where no model is
  behind them.

The owner's next pass, three more:

- **The bob.** mnMapsPreviewCameraThreadUpdate moves the camera's target
  +/-40 units over three seconds (+/-1.5 pixels on the N64, measured on
  mupen64plus; +/-1.2 on the DS). The preview never advanced the
  renderer's source-frame epoch, so the frame camera cache kept the first
  draw's camera for the whole visit: two captures 90 tics apart were the
  same image. The preview advances it each tic as the character select's
  does.
- **Saffron's speckle.** The preview draws a stage at a fraction of its
  battle size and the DS samples the nearest texel where the N64's
  bilinear tap averaged four: Saffron's window textures broke up. Each
  preview upload is low-passed once in the resolver's scratch (each texel
  the mean of its 2x2 block over the opaque ones; three 5-bit lanes added
  in one word), through a hook the preview sets around its prepares
  (`gNdsRendererTexturePrefilter`); battle never samples them, the preview
  releases its uploads. The slowest warming draw is 2.36M ticks (1.87M
  without the filter; a first 3x3 version cost 6.2M).
- **A CPU gate drew four fighters.** `osGetTime` returned `cpuGetTiming`,
  which is Calico's TIMER2 count times 64: its low six bits are always
  zero, and the source's time-seeded picks read `osGetTime() & 0xFF`, so a
  new CPU drew Kirby, Samus, MRGAW or Wario and the stage select's RANDOM
  four stages (the title's colours and the opening's fighters too). It
  returns the count itself now; every reader in this build is one of those
  picks.

Saffron's roof walkway, its garage roof and the Silph sign were still
missing after the low-pass. A probe that hid one binding at a time (a gdb
word poke on the DObj's flags) showed layer 3, the Silph front, painted
over them: the battle draws layers 0 and 3 without Z (a painter band), but
the source preview's mnMapsModelPri/SecProcDisplay set G_ZBUFFER and a ZB
render mode before every layer. While the preview is armed the packet's
no-Z runs keep their depth (`ndsRendererNativeStageLoadNoZMatrix`, the
clipped-vertex load too). Play-1012t against the mupen64plus references:
all eight stages match, Saffron's walkway and sign included.

The owner also saw the character select's previews missing after a return
from the stage select, sometimes. The first preview build never released
its warm textures, which eviction skips, so a few stages browsed left the
VRAM full for the select's fighters; the release above covers it. Eight
probes returning from the stage select (at rest, mid-load, after browsing
all ten, four contents and the cast) showed all four previews.

## The character select's Selected pose (owner bug, 2026-10-12)

Mad Piano played Mario's Selected pose (Win3). Remix keeps each
selection's pose in `selection_action_table`, which `add_to_css`
fills per character (0x0001000N: nFTDemoStatusWinN), and seven contents
differ from their parents (Mad Piano Win4; Ganondorf, Wario, Marth, Roy,
Crash, Lanky). `p4_css.py` emits the rows (`NDS_P4_CSS_SELECTED_ROWS`) and
both the slot's and the preview's Selected status take the content's row.
play-1012p, all eighteen contents and two originals selected (probes
`csss[a-e]`): every status is its row, every figatree loaded, 0 native
failures.

## Sonic's spring drawn (owner bug, 2026-10-12)

"Jump Pad Spring not visible." Two faults, each enough to keep it off the
screen:

- Its display link. Remix's `change_spring_room_` moves the spring to link
  11, the items' (`nds_p4_sonic.c`), and the battle camera's weapon test
  (`ndsStageGCDrawAllLoopIsWeaponDisplay`) admitted link 14 only, so the
  spring was never asked to draw and nothing failed. A weapon GObj on link 11
  is admitted too; the GObj-kind test keeps the items out.
- Its program. The spring's list calls segment E twice: E+0 before its
  LOADTLUT and E+8 before its LOADBLOCK. With one MObj, `gcDrawMObjForDObj`
  writes the MObj's body right after its one branch, so E+8 is MObj 0's
  material run again. The generator read it as a second MObj (material slot
  1), which the renderer's one-MObj admission refused (REJECTED_PROGRAM, two a
  frame while a spring lives), and with slot 1 unseeded the frames' TLUT came
  from the image's own bytes. P4 compiles alias slot 1 to slot 0
  (`P4_MATERIAL_ALIASES`, `generate_nds_entry_effects.py`): the two frames
  convert with their palettes (8 open, 48 coiled), and only the spring's group
  row and its two textures change in the packet.

The lab line above (p4al, "four Sonics make 11 springs ... 0 native
failures") counted springs that were never drawn. play-1012v, Sonic (human,
up-B every 90 ticks) against Fox on Dream Land: the spring drawn from its
first frame, 150 weapon submits in 420 frames, 0 native failures (play-1012u:
238).

## The select's rule bar and the item defaults (owner bugs, 2026-10-12)

"Missing Timer/Stock settings bar at middle/top of screen (should be host
changeable in multiplayer too)." The character select had left the source's
rule selector out as a narrowing (the VS menu sets the same rules). It is the
source's now: `mnPlayersVSMakeTimeSelect` / `MakeStockSelect` draw the 90x13
selector at (140,22) and the value in the dark digits, right-aligned by each
digit's width, or the Infinity sprite; the arrows' hit boxes (x 140-160 and
210-230 at the button hotspot, the header band) step the time through the
source's table (2, 3, 5, 10, 15, 30, 60, infinite) or the stocks through
0-98, before the mode label's test as in `mnPlayersVSCursorProcUpdate`, and
the commit writes both as `mnPlayersVSSetSceneData` does. The kit bakes two
selector states over the base and keyed digit and Infinity glyphs the
presenter places in one blit (`CSS_RULE_*`); the mode label's box is 113 wide
now, so a mode toggle leaves the selector alone. Wireless: only the host
steps it (the descriptor follows at once, so its lobby packets carry the new
rule); a guest shows the host's from the lobby snapshot
(`ndsNetLobbyGuestRules`). play-1012x, probe `cssrule2`: right twice 3 -> 5
-> 10, left three times 10 -> 5 -> 3 -> 2, the descriptor's time following.

"Original SSB64 defaults for Item settings should be active now, currently
Items default to none/off. Should be ALL at Medium freq." The shipped match's
descriptor turned items off; it takes the source's defaults now, every item
at the middle rate (scmanager.c:578,583), which the VS menu's item screen
starts from.

## Peach after a pause (owner bug, 2026-10-12)

"Pressing pause causes peach to be invisible for the rest of the match." The
pause close-up raises the pausing fighter to high detail and the unpause puts
the old detail back (`ftParamSetModelPartDetailAll`), which re-applies every
visible joint's model part: `modelparts[part][detail].dl`, not the
JointTree's list. A vanilla part 0 is the JointTree list itself; Remix's Peach
keeps her joint-17 hand twice, the part's copy (0xC250 high, 0xC470 low) with
its own normals, so after the round trip the joint named a list her native
owner had no root for, and the lean path declined her (validate code 3) for
the rest of the match, silently (0 native failures). `donor_variants`
(`p4_native_owner.py`) now carries part 0 as well wherever its list is not
the JointTree's, the same variant rows a part 1 takes (binding 9 for that
hand); the pins verify unchanged, and no other content has such a part 0.
play-1012y, Peach (human) against Fox, paused at GO and unpaused: drawn in
the close-up and after, 0 declines (play-1012x: 252 by frame 450).

"Peaches tennis racket looks off ... i can see it during idle and
walk/running." What showed was Fox's pistol: Peach's parent is Fox, and the
pistol sidecar's gate (Fox, joint 17 at part 0 or above) holds for her whole
match, her own hand being part 0 of that joint. A content now draws the held
gun only while joint 17's live list lives in the gun's file (the strip's own
test): Wolf's gun (WOLF_WEAPON) still draws in his neutral special (probe
`gunfile`), Peach's hand alone shows in the pause close-up (play-1012z). Her
racket proper is joint 18's part 1, set by the forward smash only.

## Falcon's and Ganondorf's punch and kick (owner bugs, 2026-10-12)

"Ganondorfs 'Falcon punch' and 'falcon kick' should have more purple VFX."
Ganondorf's special files 2 and 3 (GND files 7 and 8) loaded into Captain's
globals, and the effects drew through Captain's native owners (assets 350
and 333), whose programs carry Captain's textures; a match with both fighters
also left one file in the shared global. They are his own storage now
(`OWN_SPECIAL_FILES`): `src/port/nds_p4_ganondorf.c` runs Captain's three
makers (punch, kick, entry craft) on them at Captain's offsets, as Remix's
`captainshared.asm` structs do (a Kirby holding his power takes his punch,
`get_punch_anim_struct_`), through `NDSP4Overrides.captain_falcon_punch` /
`captain_falcon_kick` and an entry port of Captain's own case. The articles
(`p4_articles.py`) compile his punch (one list, three TEXID frames), his kick
(a tree whose drawn entry's MObjSub cycles two frames; a DObjDesc article may
carry a MObjSub table now) and his craft's lists. A P4 TEXID root may take a
MObj with flags 0x20 and TEXTURE (Falcon Punch's 0x00A1): the render tile
size and G_TEXTURE the generator baked from the MObjSub are admitted while the
live tile's origin and scale are the bake's. His craft is mostly empty lists
in his file (Remix reuses the Falcon Flyer's tree for timing); until now it
drew Captain's Blue Falcon.

"move VFX towards camera for 'Falcon punch' and 'Falcon Kick' so it doesn't
clip fighter." Both effects follow a joint (matrix kind 0x50); their makers
register with the PSI Magnet's toward-eye treatment, now a list of a match's
registered effects (`ndsEffectTowardEyeRegister`), so the 0x50 builder draws
them the shield's floor toward the camera. play-1013b, Ganondorf against
Captain on Dream Land: his punch and kick purple, Captain's kick orange in
the same match, 0 native failures (the pause menu's sprites aside).

## Roy's charge sound (owner bug, 2026-10-12)

"Down B charge SFX missing." Roy's DSP_WAIT script plays FGM 0x5C0
(RoyCharge1), which FGM.asm adds with `add_fgm` and its own UCD and
articulation microcode over vanilla sound 0x12B -- no add_sound row, so the P4
pack never held it and every play missed. `p4_audio.py` PROGRAM_SOUNDS now
injects both programs into copies of the vanilla FGM tables (the articulation's
trigger patched to the sound, as `add_sfx_fgm` does) and bakes them with the
vanilla pack's composite renderer: the root's four climbing notes under the
pitch sweep plus its fork, vanilla program 673, mixed; 600 ticks at 32 kHz,
55 KB of IMA, 16.4 dB, pause bit from the UCD's set_unk1F 226. One pass
outlasts the charge (21 levels, one per 9 frames, ~550 ticks), so the loop
the N64 would run is never reached. play-1013c, Roy holding down-B: the cue
starts at each DSP_WAIT entry on a live channel, no unsupported calls.

## The 1012y playtest (owner, 2026-10-12)

Two consoles, play-1012y; the multiplayer side is in
`docs/P3_Multiplayer/P3_STATUS.md` ("Third report set").

- **"Kirby hats for extra and remix characters make Kirby invisible."** Every
  VS battle draws low detail (owner 2026-10-01, `ndsFTManagerBattleLowDetail`),
  but the hat admission kept the source's rule -- low detail only for three or
  four fighters -- so in a match of one or two a power's Remix hats were
  admitted in high detail only, Kirby drew low, and the owner validator
  refused his head root (reject code 4: root 0 at 0x8F18, the Remix hat's
  list, where the program without a hat expects 0x27B0), every frame.
  `ndsKirbyHatPrepareMatch` admits the low detail in every VS battle, and a
  Kirby holding a content's power stays low in any VS battle's close-ups
  (`ftParamSetModelPartDetailAll`). Probe (play-1013e, Kirby taking Bowser's
  power in a match of two): the four Remix hats admitted low, 0 declines over
  300 frames of the power, Kirby drawn with the hat.
- **A data abort with Kirby holding Lanky's power** (found while probing the
  above; the frozen-console class the owner reports). Remix's own ids for its
  Mario clones are not Mario's, so the source's shared fireball routines set
  Luigi's copy statuses for them (`FTKIRBY_COPYMARIO_FIREBALL_CHECK_FTKIND`)
  and Remix's clone swaps name 0xE9/0xEA; the DS Kirby takes the content's
  parent kind, Mario, whose statuses (0xE7/0xE8) no swap matched. Landing from
  the air neutral special therefore entered Mario's copy status, whose
  accessory made Mario's fireball from `gFTMarioFileMain` -- never loaded in a
  match whose only Mario-kind fighter is a content -- and faulted on NULL in
  `wpManagerMakeWeapon`. `p4_kirby.py` maps those swaps onto Mario's statuses
  for a Mario-parent content (Lanky, Mad Piano: Piano's Kirby book throw now
  plays its own animation too), and `wpManagerMakeWeapon` makes no weapon
  from a file the match never loaded (`gNdsWeaponFileMissingCount`). Probe
  (play-1013e, the same four-fighter run that faulted at frame 346): Kirby in
  Lanky's statuses, no fault, 1,500 frames.
- **The owner's freeze photo (Kirby with Mad Piano's power).** The report
  (play-1012y) named a data abort reading NULL+4 with Kirby's TopN DObj in
  two registers; the same four-register picture came back in the harness
  from Kirby taking Piano's power and throwing his first book. Remix's
  `kirby_fix_` takes the book's origin from part 0x3 (`fp->joints[7]`), and
  Kirby's attributes never make that joint (setup_parts 0xEF7CFFC0 skips
  DObjDesc 3), so the position walk read a NULL joint. The book now leaves
  Kirby's fireball hand (part 0xD, the source's own origin) when joint 7 is
  absent, and the parts-position seam returns for a NULL joint
  (`gNdsCollisionNullJointCount`). Probe (play-1013h, the same match): Kirby
  in Piano's own Kirby statuses (0x139/0x13A, since the clone fix above),
  books thrown, 1,810 frames, no abort.
- **"Sometimes Luigi's nose changes to his hat color especially during
  results screen."** Luigi's HIGH head reads one costume script per MObj (six)
  from an array relocData declares with one entry (0x2C40): the source reads
  the next arrays' words for MObjs 1-5. The battle core pack copies objects by
  their declared size, so his nose and the skin MObjs beside it took other
  joints' scripts -- the hat's green -- wherever the high detail draws
  (Results, every KO or pause close-up). `generate_battle_core_packs.py` keeps
  each joint's costume array out to its MObj count, with the scripts it names;
  only Luigi's pack changes (+112 B).

## Kirby's copies of all eighteen (owner request, 2026-10-12)

Owner: exercise every Remix and EXTRA hat and copied attack. Probe: eighteen
matches of two on the shipping ROM, nine at a time in one melonDS-mp process,
a human Kirby (inhale, copy, ground and air neutral special, walking at his
opponent) against each content at level 9, 1,500 frames.

- **The clone swap ran the parent's row.** Since S12's action_replace_map
  seam, `ftMainSetStatus`'s wrapper copied `status_id` into `nds_p4_status`
  before S8's swap (`ndsP4KirbyCloneStatus`) rewrote it, called the source
  setter with the copy and stored the swapped id afterwards. A Kirby holding
  Falco's, Ganondorf's, Wolf's or Mad Piano's power therefore ran the parent's
  copy row -- its routines and motion -- under the power's status id: Fox's
  blaster for Falco, the Falcon Punch for Ganondorf, Mario's fireball for Mad
  Piano, and for Wolf Fox's blaster from Fox's special file, never loaded in a
  match whose Fox-kind fighter is Wolf, so every shot was refused (29 in 1,200
  frames, `gNdsWeaponFileMissingCount`). The swap now sets the status the
  setter runs. The Mad Piano probe in "The 1012y playtest" above read the
  status id only. Lab (play-1013k): Wolf's power in 0x13B/0x13C with motions
  288/289, 23 shots, none refused.
- **EXTRA's three are not copyable in their source.** EXTRA's
  `define_character` passes `bool_inhale_copy` from the config's `kirby_hat`
  (FALSE unless set; of its characters only DKUlt sets it), so the inhale row
  holds Kirby's own id and no hat (MRGAW's own patch writes hat 0x08, none,
  too). The DS Kirby took the parent's copy: the Falcon Punch for Snake (whose
  effect file is never loaded: 20 bare-effect refusals), Mario's fireball for
  MRGAW (refused) and Pound for Meta Knight. `ndsP4KirbyCatch` now gives any
  catcher Kirby's own id for a content with no power (a held power goes, as
  swallowing a Kirby without one does), the hat admission no longer loads
  their parents' hats (`ndsP4KirbyPlayerNoCopy`), and `p4_kirby.py` fails if
  a content without a power exports `bool_inhale_copy` TRUE. Snake's config
  keeps a grenade file "separated for Kirby use" that nothing reads.

Result (play-1013k): all fifteen Remix powers taken and used in their own
ground and air statuses with their own motions -- Falco 0x135/0x136 (the
Phantasm), Ganondorf 0x12F/0x130, Wolf 0x13B/0x13C, Bowser 0x137/0x138, Marth
0x13D-0x144, Roy 0x179-0x180, Wario 0x131-0x134, Peach 0x176/0x177, Crash
0x172/0x173, Lanky 0x187/0x188, Sheik 0x153-0x158, Banjo 0x16F/0x170, Sonic
0x14B-0x151, Dedede 0x159-0x15C, Mad Piano 0x139/0x13A -- each Kirby drawn
with the power's hat; Meta Knight, MRGAW and Snake caught with no copy. Every
match: 0 native failures, 0 weapon or effect refusals, 0 NULL joints, 0 lean
declines, 0 heap overflows, heap low-water 61.9 KB or more.

### Custom copies for EXTRA's three (owner request, 2026-10-12)

Asked whether Kirby could copy Meta Knight, MRGAW and Snake, the owner chose
Brawl-style copies over the source's none. They are custom content -- no
source has them -- built from each content's own move code and assets and
Kirby's own vanilla animations (`scripts/p4/p4_kirby.py` CUSTOM_POWERS, after
Remix's statuses; the catch takes them as any power):

- **Meta Knight: Mach Tornado and his mask.** Kirby's neutral-air spin at 1.5
  speed, three turns each with the spin's hitboxes and Meta Knight's whoosh,
  airborne from the ground with Meta Knight's 30 hop; B mashed every 4 frames
  lifts him as it lifts Meta Knight (his drift, wall bounce and gravity,
  `ndsP4MetaknightNSPLoopPhysics`/`Collision`), a special fall at the end.
- **MRGAW: Chef, and Kirby turns his black figure.** The fireball throw
  makes MRGAW's sausage from Kirby's hand at its flag; B pressed after the
  throw cooks again, four times more at most. His model has no chef's hat
  (his Chef variant is a face cutout), so as in Brawl Kirby is drawn black
  while he holds the power: the colour-animation fields a hit flash uses
  (`colanim.color1`, a full blend to black) set at every status change while
  no colour animation runs (`ndsP4MrgawKirbyOnStatus`, so a flash hands back
  to it) and cleared with the power (`ndsP4KirbyClearSilhouette`). A blend
  made in the renderer alone (`ndsRendererAdapterFighterColorModulate`) left
  him pink: the lean path re-tints a list only when its tint-tile set moves,
  which a colour animation drives.
- **Snake: a hand grenade and his bandana.** Snake's grenade, made at the
  Thunder Jolt toss's flag already thrown (his neutral throw; his forward one
  with the stick forward), two out at most for that Kirby, counted on the item
  list (Kirby keeps none of Snake's words). The Link copy's boomerang throw was
  tried first and drew no Kirby: its script enables Link's hidden joint 18, a
  root vector (8 roots for 7) no Kirby program without Link's copy carries, so
  the validator refused every frame of the throw (`nNDSFtrLeanDeclineValidate`).

The hats are P4 sidecar roots on joint 6 (`scripts/p4/p4_kirby_custom_hats.py`,
`ndsFighterDrawKirbyHatSidecar`), drawn over Kirby's plain head (part 0) and
only over it -- Stone and the inhale faces put another part there -- the way
the powers' weapons ride joint 17. Meta Knight's mask is the front of his body
list (material 2) and its eyes (material 1), each material call replaced by
its MObj's commands for costume 0 (`gcDrawMObjForDObj`: the palette image;
with the alpha flag the TLUT load and the sprite), scaled 1.08 onto Kirby's
head; Snake's low-detail head splits into no clean bandana (his texture
carries skin, band and hair in one), so his is a ring generated around
Kirby's forehead with two tails, on the band rows (15-19) of that texture.
Each file (0x1930 + content id, 2-9 KB) rides the power's
`hat_texture_file`, which the match loads.

Owner BUGS.md rows of 2026-10-12 against the first cut, fixed:

- **Meta Knight's copy is his whole neutral special.** The spin of Kirby's
  neutral air used none of Mach Tornado ("just use metaknights entire b
  attack"). Four statuses run Meta Knight's start, loop and ends on Kirby's
  motions (`nds_p4_metaknight.c`): the start's 11 tics; the loop's spin
  counted as his loop animation's speed (130, -2 a tic for 60 tics then -3,
  +7 and a lift for B every 4 tics, a whoosh at each turn of its 358 frames,
  floor dust every 8 tics, out at 60 with half the x speed); his
  `moveset_tornadoLoop` hitboxes word for word (joint 0, which Kirby has too:
  12% at size 600, then 8% from frame 1550 -- the measured MK ran his ends
  and mash exactly so); his drift, gravity and wall bounce; his 54-tic ends,
  the grounded one with `moveset_tornadoEnd`'s effects, the aerial one into a
  special fall. His tornado (model joint 34 part 1, the loop script's
  `SetModelPartID`) is the hat file's second list, a sidecar root on Kirby's
  root joint turned by the spin (`ndsFighterDrawKirbyTornadoSidecar`),
  carrying the content generator's deviations for those bytes (the cream
  vertex tint, full IA intensity) and the combine his native owner aliases.
  Each status replaces Kirby's motion script (`ndsP4KirbySetMotionScript`;
  the motion's first frame plays muted), so no neutral-air hitbox or swing
  sound comes along.
- **MRGAW's copy keeps Kirby black.** The figure came back only at status
  changes: every colour animation's end (`ftParamResetStatUpdateColAnim`)
  cleared it, so Kirby standing still stayed pink. The power's
  `kirby_colanim_end` sets it again there. The fireball motion's frame-16
  events -- the Mario fireball flash (colour animation 39, which also ended
  the figure), its sparkle and FGM -- are replaced by the toss alone (the
  food's flag and its rumble).
- **Snake's copy sounds like Snake.** The Thunder Jolt toss's first frame
  ("Pika" and the electric FGM) plays muted and the throw plays Snake's
  own throw FGM (`moveset_grenadeThrowN`'s 0x2A; with two out, ThrowEmpty's
  0x2B).

A build seeded from an earlier one kept the content directory's copy of the
hat file from before it gained the tornado list (`p4_articles.py` copied it
only when missing), so the entry packet baked the tornado's offset in the old
bytes and drew 0 triangles while reporting success; the copy is refreshed
whenever it differs now.

## Root blocks and lazy material rows (P95, 2026-10-13)

What four content mirrors spend over the gate (task37 census of four
Crashes, 126 over-gate frames of 1,200, 1.37M cycles a frame above the
rest): ~570K is the lean materializer -- corners 111K, the run prepare 78K,
the texture resolver 58K, the shade 40K, the state deltas 39K -- and ~100K
the event path around it. Key traces (DIAG sweep, 1,200 frames): four Crashes
materialize 71 lists for 53 distinct keys -- a blink, a mouth, a hand: one or
two of twenty roots moved -- so a bigger list cache could save at most a
quarter of them; the lab root census found 43-59% of every new list's words
identical, root for root, to the list beside it.

- **Root blocks.** A list is the head and one block per root. Every root of
  every P4 owner image (and some of the cast's) is self-contained: before its
  first run it sets every piece of RDP state its runs read that any root of
  the image writes (combine; for a textured run G_TEXTURE, image, load, the
  render tile's SETTILE/SETTILESIZE, a TLUT for a CI tile, the othermode_h
  bits; light colours and direction for a shaded epoch; a material counts as
  the root's own input). Such a root's block is a function of its tables,
  preamble, light preamble, material rows, GX slots and corner format, so a
  list records, per root, where the block sits and what it added to the
  packet's tables, under a hash of those inputs (`ndsFtrLeanRootKey`), in
  the packet's texgen site table (unused by a list without texgen sites); a
  later list of the slot copies a block whose key a donor (the other entry,
  the spare) holds instead of walking the root. Blocks start and end on a
  packed-command header boundary; a copy re-bases the packet's indices
  (seed, shade sites, tint binds), merges the textures (each still the cache
  entry it bound), re-derives the shade words for the list's modulate, and
  re-bases the polygon IDs: a run's POLYGON_ATTR carries the walk's combine
  count, which runs on across roots (`ndsFtrLeanRebasePolyIds`; the lab
  verify arm caught it as 1,489 differing words). Lab verify (every
  re-selected list against a full walk, `gNdsFtrLeanSlow` 16, the fresh walk
  copying nothing): four Crashes, Meta Knights, Banjos and the cast, 622
  comparisons over 2,161 copied roots, 0 differing words, shapes, textures or
  fences. `gNdsFtrLeanRootCopyOff` = 1 walks every root.
- **Lazy material rows.** Four fighters' material DObjs share 32 rows, so
  every lean event rebuilt most of its own rows (36K ticks a Crash event, 279
  events in 1,200 frames, 208 of them entry hits that never read a row). A
  row is a pure function of its MObj's material words (MObjSub, the texture
  ids, lfrac, the palette id), so those words are the entry key's word 0
  now, and the rows are built (`ndsFtrLeanBuildRows`) only for a list the
  event materializes.

Clean sweep ROMs, Dream Land, 1,961 frames, WORK P95 (build-sweep-p95a ->
build-sweep-p95e): four Crashes 1,403,328 -> 1,284,736, four Banjos 1,282,432
-> 1,249,792, four Meta Knights 1,272,192 -> 1,272,896 (their blocks copy
little: a model-part swap shifts the roots after it, and with them the GX
palette slots their keys carry), the cast 1,061,312 -> 1,065,216 (within one
run's spread).

The copies' own costs, trimmed next (the census put them at ~100K cycles an
over-gate frame): a block's key takes its root's material words from the
event path's per-root fold (`gNdsFtrLeanRootMaterialKey`) instead of hashing
the rows again; self-containment is cached per root and the inputs it reads;
the polygon-ID walk runs only when the IDs move, from a table of the packed
commands' parameter counts. WORK P95: Crash 1,257,344, Meta Knight 1,264,704,
Banjo 1,246,080, the cast 1,055,616; lab verify unchanged (0 mismatches).

Why Meta Knight copied little (gdb trace of every block key, 54 plans): not
the palette slots. His root programs (Live1..Live6, one per model-part
state) repeat the base program's root rows at other addresses, and the key
hashed the row's address; and the preamble's light direction flips between
two values, though a walk writes it only into the head's vector word (a copy
writes that word from the live preamble). The key now hashes the row's words
and leaves the direction out: 22% -> 66% of his roots have a key a recent
list holds. Lab verify on four Crashes, Meta Knights, Banjos and the cast:
622 comparisons, 0 differing words; Meta Knight's shade-site differences are
607 with copies and 607 without (the known modulate-0 re-record effect). WORK
P95: Meta Knight 1,264,704 -> 1,221,760, Crash 1,252,032, Banjo 1,237,888.

## P4 texture admission (residency, P95, 2026-10-13)

The 18 contents' four-fighter mirrors on Dream Land (clean sweep ROM,
build-sweep-p95g, 1,961 frames), WORK P95: Falco 1,027,712, Ganondorf
1,056,320, Wolf 1,074,048, Bowser 1,195,904, Marth 1,164,928, Roy 1,168,640,
Wario 1,020,800, Peach 1,076,864, Crash 1,245,120, Lanky 1,042,880, Sheik
1,077,952, Banjo 1,234,432, Sonic 1,257,536, Dedede 1,203,136, Mad Piano
923,392, Meta Knight 1,215,168, MRGAW 1,132,352, Snake 1,102,144: nine over
the gate.

A content's textures converted on first sight, mid-match. The cast's battle
load replays a generated table of every texture a kind can show
(scripts/fighters/generate_nds_fighter_admission.py, fighters/admission.bin),
so the cast uploads nothing after GO; a content was noted under its parent
kind, whose records name files a content's battle never loads. Four Crashes
uploaded 66 textures after GO (Bowser 50, Banjo 43, Dedede 28, Sonic 18, Meta
Knight 12, Marth and Roy 2), ~100K cycles each, inside the materialization of
the frame that first showed a face or a costume's palette -- 34% of a Crash
list's materialization.

- `scripts/p4/p4_admission.py` enumerates each content with the cast
  generator's own display-list state machine, material evaluator and record
  packer over the content's O2R files: the JointTree lists with their MObjSub
  chains and costume streams, every FTModelPart row with its streams, the
  access part, and the texture-part ids its motion scripts set
  (SetTexturePartID in the export's events). `fighters/p4_admission.bin`
  holds the records by content id x detail (p4_rules.mk). A costume's set is
  3-20 KB (Mr. Game & Watch 45-59 KB).
- The admission seam passes the content (`ndsFtrLeanAdmitNoteFighter`); the
  bases are the content's own FTData files, whose model is a compact battle
  pack that only its loaded data maps SOURCE offsets into. Cast fighters admit
  first, then the contents one record each in turn (a content's list is a
  superset of every face of every costume; four outgrow the 153-entry slot
  budget, which in fighter order would leave the last fighters none), to at
  most 64 KB of texels so bank D keeps room for the battle's own uploads.

Four Crashes (DIAG ROM): uploads after GO 66 -> 24, the texture binds' miss
cost 6.94M -> 2.23M cycles over 1,200 frames, the slot budget reached at 153.
WORK P95 (clean ROM): Crash 1,245,120 -> 1,161,728, Dedede 1,203,136 ->
1,184,576, Bowser 1,195,904 -> 1,187,328.

## Board 1 status (master plan Revision 3)

| Item | State |
|---|---|
| S1 content list | done: registry, Makefile templates, one native-owner emit, `NDS_P4_CONTENT_ROWS` in every shared site; Falco unchanged (lab: 0 declines, menu ROM preview) |
| S2 build throughput | done for the lab and playable builds (2026-10-11): a lab build writes its ITCM evictions into a copy of the hot-text script in its build directory and passes it as `NDS_HOT_TEXT_LINKER_SCRIPT`, so the tracked script never changes and lab and playable builds run at once (play-1010f beside build-lab-p4am). P4 generated-data rules live in `scripts/p4/p4_rules.mk`: every object depends on the Makefile, so a rule there does not recompile the build. Start one build alone after editing a shared P2 generator (`src/nds/generated` outputs are shared) |
| S3 table seams | 20 of 33 (Falco's 12, entry_action/entry_script, the six special-move starters, grounded_script) plus the jab tables (jab_3, jab_3_timer, jab_3_action, rapid_jab and its four rows, "Jab tables" above), yoshi_egg and pipe_turn. `NDSP4KindCases` (generated): the landing switch's case Remix's grounded_script row names (Marth, Roy and Sonic take Mario's; Banjo, Dedede and Wario none; Peach's own routine, a lab stand-in for now) and pipe_turn, Mario's turn in the Dokan statuses (Wario; Crash and Lanky, Mario's other children, take the default turn). The port's landing switch (`mpCommonSetFighterLandingParams`) had kept only Mario's case: Luigi's cyclone never rose again after its first aerial use and Samus's aerial charge-shot lift fell for the rest of the stock; it is the source's whole now. gfx_routine_end (2026-10-11): `NDSP4Overrides.colanim_end` replaces the parent's case of `ftParamResetStatUpdateColAnim` (Dedede's and Sheik's full-charge colours come back after another colour animation ends); the port's copy of that function had also lost the source's Kirby (copied Samus or Donkey Kong full charge) and Ness (PSI Magnet hold) cases, restored. Remix's extended colour animations (GFXRoutine.asm, ids 0x56 on) were declined by the port's table: `scripts/p4/p4_colanims.py` re-encodes the 17 the roster reaches (Marth's and Roy's blade, counter and entry flashes, Roy's up special and charge, Falco's Phantasm, Wolf's pink flash, Sonic's charge and attack, Sheik's charge, down special, shot and up-special end, Dedede's charge) in the port's LSB-first colour commands and appends them to `dGMColScriptsDescs`. Lab, Sheik forced to hold B: the up-special end flash (0x71) runs from her motion script, the full charge (0x6D) at six needles, and comes back after a damage flash. initial_script (2026-10-11): Remix gives Peach and Sonic the source's case that clears the first passive word (Fox's clears nothing) and Sheik Samus's, which clears two, so a float, a spent spring or her needles no longer outlive a stock (`on_init`, run at spawn and every rebirth); Marth's and Roy's equal Captain's, Dedede's is his port. label_height substitutes the id whose Results label offset a content takes, and the source's per-kind offsets (`mnVSResultsSetPlayerTagPosition`) are all zero, so it changes nothing; variant_original only feeds Remix's character-select variant icons. Left: the Kirby tables (S8) |
| S4 motion commands | done, 12 of 12. 0xD9 SET ENV COLOR (2026-10-10) is Remix's per-port `CharEnvColor.moveset_table` (`gNdsP4EnvColor`), cleared at every action change as `change_action_` clears it; the roster uses only its alpha, in the entries: Marth and Roy hide for the first 6 ticks of the beam (0xFFFFFF00, then 0xFFFFFFFF), Banjo hides 24 ticks and fades in over 16 (alpha 0x20 steps). The lean draw takes the alpha (`ndsP4EnvAlpha`): 0 draws nothing, anything under 0xFF scales the list's POLY_FORMAT alphas for its one DMA (`ndsFtrLeanPacketSetAlpha`), restored before the next lean draw or the frame end. play-1010b: Marth hidden at frame 76, Banjo translucent at frame 90 (captures `e7m76`, `e7b90`, local), 0 native failures. |
| S5 routine work lists | generated for all 14 (`scripts/p4/routine_worklist.py --lab <lab build>` into a build directory): 296 donor routines with Remix scope, file and line, size and users (Banjo 33, Bowser 10, Crash 29, Dedede 37, Lanky 29, Marth 18, Peach 22, Roy 21, Sheik 34, Sonic 29, Wario 23, Wolf 11; Falco and Ganondorf none), plus each content's id tests in Remix's shared code (17-39); the classes (behaviour, presentation, 1P, toggle) are still read by hand. Ported: Wolf 11 of 11 (`src/port/nds_p4_wolf.c`: WolfUSP, WolfDSP, WolfNSP with his own shot, the slash, reflector and Wolfen on his own files), Bowser 10 of 10 (`src/port/nds_p4_bowser.c`), Marth 18 of 18 and Roy 21 of 21 (`src/port/nds_p4_marth.c`, `nds_p4_roy.c`; MarthUSP and MarthNSP are both of theirs, `nds_p4_roy.c` compiles Marth's file when he is not enabled). Marth's counter is Remix's head of `ftParamUpdateDamage` (`NDSP4Overrides.update_damage`). Read as assembled: Dolphin Slash's landing lag keeps the low half of the Mario routine it copied (0x3EC05C29), and a stick short of 10 forward drives 20 (the compare's 10 is added again). Roy's early release from the charge's first status calls the end with whatever action id a1 held; DSP_BEGIN.bin never sets temp variable 2, so that path is dead. Lab ("p4aa", Dream Land, 0 native failures): four Marths' CPUs use up and neutral B; player 0 forced to down-B counters 27 times and strikes back 5 (1,500 frames); forced B taps chain Dancing Blade 26 times in 900 frames; four Roys forced to down-B charge and release 30 Flare Blades. Left for both: their entry effect (Link's entry case on their own file 0xDA7, S6). Wario 23 of 23 (`src/port/nds_p4_wario.c`: Body Slam with its recoil, Corkscrew, Ground Pound). Body Slam's bounce is Remix's head of `ftMainSetHitInteractStats` (`NDSP4Overrides.on_hit_interact`, which Sheik's and Banjo's recoils use too): the source defines and calls that function in `battleship_ftmain.c`, so its calls there take the hook through a macro that tells them from the definition by the first argument's first token, and the port's wrapper takes it for the other units. Read as assembled: the slam's wall test reads collision word 0xCC, `coll_data.mask_prev` -- the map callback runs after ftmain moves the current mask there and zeroes it, so it is last frame's walls; the recoil's fall slows by 1.25 for Remix's Wario id and 0.55 for the Kirby and Polygon copies; Corkscrew's landing lag keeps the low half of Mario's constant (0x3E805C29). Lab ("p4ab", Dream Land, 0 native failures, 0 lab fallbacks left): four Warios' CPUs slam 14 times (4 bounces), corkscrew 4 and ground-pound 7 times in 1,500 frames; player 0 forced to B slams 14 times in 900 frames (3 bounces, 5 run off the floor or jump out), forced to down-B pounds 10 times (7 from the ground, which hops) and lands the pound 6. Peach 19 of 22 (`src/port/nds_p4_peach.c`: Peach Bomber, the parasol, and her float), the three of PeachDSP (the turnip pull) waiting on Remix's turnip item (S6). The float is Remix patching the parent's common code on her id, each a new `NDSP4Overrides` seam: the gravity function's head (`gravity`), the Jump, JumpAerial, Fall and Pass interrupts' jump check (`air_jump_check`, tried where the aerial check ends in each import TU), Fall's status change (`fall_status`), the aerial item throw (`air_item_throw_block`), the damage routine's head (`on_damage`), the landing switch (`on_landing`: her grounded_script, which a ledge catch skips -- the cliff seam marks it, Remix's ledge_flag) and FallSpecial's status (`fall_special_status`, the parasol's fall); her status hook gives aerials started in a float no landing lag and a ledge get-up her float back. `GROUNDED_PORTS` (generator) lets a grounded_script routine with a port take the default case. Read as assembled: the float's start keeps 0.25 of her fall (the source comments 0.375); her parasol's landing lag keeps the low half of Mario's constant (0x3EC05C29). Lab ("p4ac", 0 native failures): four Peaches' CPUs bomb 10 times (2 bounces) and open the parasol; player 0 holding a jump button floats 3 times in 900 frames (340 float frames, float-on after aerials 11); forced up-B opens and floats the parasol 5 times, and driven by gdb closes it into her parasol fall 3 times and reopens it. Crash 29 of 29 (`src/port/nds_p4_crash.c`: the spin with its shield block and item fling, the belly flop, the dig and its dive), and the patches Remix makes on his id: the dig's collision box (his attributes' box shrinks in DSPWait/DSPTurn, every Crash sharing them as in Remix, and Size.asm's per-frame copy of the attributes' box into the fighter at `ftMainProcPhysicsMap`'s head, `before_physics_map`), the dig's grab immunity (`grabbable`; the patch reads the id through the hurtbox pointer the source's loop walks, so only his first hurtbox is exempt), his down-air bounce in Link's rehit case (`attack_air_lw_hit`), the tomato's eat sound (`on_eat_tomato`), and the blocked spin's lost hitlag (his shield routine zeroes the push the source charges after it, unless it rebounds). Read as assembled: the spin's and the dig's steering leave the velocity as it is when it is within 2 of the target (the donor's `mov.s f2, f4` meant the opposite); the belly flop's air start keeps 0x3D480000 of the fall (the source's "0.05"). S6 owns his spin and dig effects and what the donor runs only while the dig effect exists (its dust, sound and rumble, the dive landing's sounds). Lab ("p4ad", 0 native failures): four Crashes' CPUs spin, flop (2 landings), dig (2) and down-air bounce (2) in 1,500 frames; forced down-B with the stick digs 5 times (a turn, 4 ends); all four forced to B spin 132 times. Lanky 29 of 29 (`src/port/nds_p4_lanky.c`: the Grape Shooter's ammo and refire, the balloon with its steerable balloon damage, and OrangStand's 15 statuses -- walk, turn, jump squat and jump on his hands, platform drop, taunt and the cancel that replays the taps it buffered), and Remix's `damage_patch_`, which turns a DamageFly status (or, after an electric hit, the one it leads to) into his balloon damage while inflated (`damage_status`: `battleship_ftcommon_damage.c` tells `ftCommonDamageInitDamageVars`' status change from the file's other one by the first argument). Read as assembled: the balloon's air start masks a register the donor never loaded with byte 0x18D (the port clears the four flags the other air starts clear); OrangStand's end pushes 16 forward on the ground too (the branch skips one instruction, not the block); his jump is the rest of `ftDonkeyThrowFJumpSetStatus` entered at its status change with a frame speed of 1. His grape is his own weapon on his special file 1 (S6); S6 owns his entry barrel (dkshared.asm). Lab ("p4ae", 0 native failures): four Lankies' CPUs shoot (5), balloon (2 of 5 burst, the rest knocked out: 41 damage-status checks, 67 frames of steered balloon damage); forced down-B into a gdb-driven walk: 6 walks, 7 waits. Sheik 34 of 34 (`src/port/nds_p4_sheik.c`: the vanish with its grounded and aerial moves and ends, the needle charge on Samus's charge routines -- six needles, kept through hits -- and its throws, and Bouncing Fish with its one lift per landing, its tap buffer and its wall and contact recoil); the recoil on contact is Wario.asm's `body_slam_recoil_` (`on_hit_interact`). Read as assembled: the vanish aims straight up under a stick of 11 (cvt.w.s rounds: x*x + y*y <= 110); a hit out of the charge runs Samus's damage routine, which clears passive word 0 (the fish's lift), not the needles (word 1); a roll out of the ground charge passes the fighter object as the item-throw buffer, so a throw stays open the whole roll; the fish's recoil takes its status-change preserve word from the stack slot where it saved its return label, 0x80566ACC, and its landing a word the donor never writes (the DS preserves nothing there); the recoil's stick clamps are inverted (facing right, anything faster than -40 becomes -40 and the rest -20). The needle is his own weapon on his special file 1 (S6); S4 owns the vanish's environment colour, and the gfx-routine table the full charge's flash (Remix id 0x6D, which the DS colanim table declines). Lab ("p4af", 0 native failures): four Sheiks' CPUs vanish 5 times, charge and throw needles 6 times (259 charge frames, 160 throw frames) and fish 3 times with 39 recoil frames in 1,500 frames; player 0 forced to up-B vanishes 6 times in 900 frames (5 from the ground, 72 move frames). Banjo 33 of 33 (`src/port/nds_p4_banjo.c`: the eggs forward and backward, the Beak Bomb with its buffered attack, recoil and wall splat, Beak Barge and Bill Drill); the Beak Bomb's recoil on contact is Wario.asm's `body_slam_recoil_`. Read as assembled: the Beak Bomb's begin and attack test v0 for their air friction, and v0 still holds the physics routine's own address there (the source's `jalr`), so neither applies it; its maps are Mario's Super Jump Punch map keeping the low half of his landing constant (0x40005C29 and 0x3F805C29, "1.5" and "1.0"); its begin's special fall lands in 2.0, not the source's "1.5"; its start runs Captain's Falcon Dive status-vars routine as the status change's proc_status and plays its events through `ftMainSetStatus`' home slot; Bill Drill's air status copies Captain's without the TransN rotation store. His eggs are his own weapons on his special file 4 (S6); S6 owns his entry. Lab ("p4ag", 0 native failures): four Banjos' CPUs lay 8 eggs (67 forward, 209 backward frames) and drill 3 times in 1,500 frames; player 0 forced to up-B with B taps starts 11 Beak Bombs, attacks in 350 frames and recoils in 91; forced down-B barges 4 times from the ground; Sonic 29 of 29 (`src/port/nds_p4_sonic.c`: the Homing Attack with its target search over fighters and items, its move, end and recoil, the Spring Jump, and the Spin Dash's charge and roll on the ground and in the air). Read as assembled: the homing search skips himself, teammates without team attack and fighters below status 7, takes items whose hurtbox is live, and looks 2,208 ahead inside a cone of 1,000 + x/2; a spent spring keeps every aerial special from starting until he lands or is hit (`action_check_patch_`, a new `air_special_block` override at `ftCommonSpecialAirCheckInterruptCommon`'s head, and `SonicUSPRefresh` on damage); an ejected object stops being anyone's homing target (`destroyed_target_fix_` at `gcEjectGObj`'s head). The spring is his own weapon on his special file 1 (S6): it lands on Samus's bomb map routine, uncoils, and bounces every airborne fighter that falls onto it at 130 (into JumpF from the jumps, tumble, the shield-break falls and Remix's per-character recoveries, else with jump smoke), giving Sonic his up special back; read as assembled, its hit records' "clear" sets the hurt, shield and reflect flags with group 0, and the PSI Magnet case clears what Remix names the overlay, which in BattleShip's struct is the loop sound's handle and id, the first colour script and the colour flags, so a loop sound playing there outlives the jump's status change. Lab ("p4ah", 0 native failures): four Sonics' CPUs home in twice, spring 4 times and spin dash 7 times (68 ground and 187 air roll frames) in 1,500 frames; player 0 forced to up-B springs 14 times (7 from the ground); forced to B homes in through 107 move frames and recoils on 186; Dedede 37 of 37 (`src/port/nds_p4_dedede.c`: Super Dedede Jump with its cancel, landing and ceiling bonk; the inhale's own statuses around Kirby's, its walk, spit and the spat fighter's flight; the minion toss's charge, cancel and throw). His inhale runs Kirby's `ftkirbyspecialn.c` on his statuses: in the Kirby TU every status change goes through the content's `kirby_inhale_status` (Remix's 25 DededeNSP hooks), and the copy table it reads is the port's copy of Kirby's rows, his file or not in the match; a held fighter waits 250 plus its percent (`custom_initial_absorbed_timer_`), and a Remix victim's star deals its `kirby_inhale_struct` damage (the generator emits that row, `NDSP4KirbyInhale`, for every content). The inhale absorbs (Reflect.asm's custom kind): a weapon or item it catches runs its absorb or blast-zone routine and is destroyed, not reflected (dispatchers ahead of `wpProcessProcHitCollisions` and `itProcessProcHitCollisions`), Samus's bomb and Pikachu's thunder read reflectable for his search, and the reflect switch's custom routine sends him to the spit, which fires a Star Rod star. Remix's jump_fix_1-5 take him down Kirby's multi-jump branches with his heights (68, 58, 52) through dispatchers for the jump check, the multi-jump status and the aerial physics (`multi_jump_vel`); `initial_script_` clears his minion pointers and charge at spawn and on every stock (`on_init`); Kirby's wall and floor stars (`kirby_cpu_inhale_4`) and the inhale wind 1,000 ahead (`dedede_gfx_`) are his too. Read as assembled: the walk's "same walk" test reads code words, so it always changes status; the ground spit ends in the fall; the spat star's smash flag is a pointer's low byte, so it flies at the smash speed and lifetime with the tilt attributes; the up special's ceiling test after a ledge catch reads a temporary the catch leaves, zero since Remix's Dedede keeps the ledge. The landing stars are Yoshi's star weapon on his special file 2 (YoshiShared.asm `down_special_struct_fix`, S6), drawn as the item file's shared star quad; S6 owns his minions and his entry (Kirby's warp star, from Kirby's special file 2). Lab ("p4ai", 0 native failures): four Dededes' CPUs inhale 11 times (104 spit frames), start 3 tosses and multi-jump 21 times in 1,500 frames; player 0 forced to hold B against Mario, Samus and Fox swallows a shot and spits a star, and holds and spits a fighter; forced up-B jumps 5 times; forced down-B starts 11 times, charging over 891 frames and tossing over 216 |
| S6 articles | the path exists, Wolf proves it: own special-file storage (`OWN_SPECIAL_FILES`), the parent's effect makers a content replaces (`gNdsP4<Title>Overrides`), `scripts/p4/p4_articles.py` roots compiled by `generate_nds_entry_effects.py --p4` into the build's packet, native admission, TEXID frames, state-only lists, per-match texture preparation and the held-gun sidecar ("Own special files and articles" above). Bowser's Clown Copter draws from his own special2 (his flame is the Fire Flower's weapon). Weapons ported: Sheik's needle, Banjo's two eggs, Lanky's grape and Sonic's spring (`NDS_P4_WP_KIND_*`, one kind past the source's per weapon), and Dedede's landing stars (Yoshi's star kind on his file). Lab ("p4al", Dream Land, 900 frames, player 0 forced to the special, 0 native failures, 0 article misses): four Sheiks throw 18 needles (144 live frames); four Banjos lay 22 eggs (354 forward, 204 backward frames); four Lankies shoot 16 grapes (795 frames); four Sonics make 11 springs (2,460 frames) that bounce fighters 17 times, 12 of them into JumpF; four Dededes land 3 jumps and throw 6 stars. Remix built Sheik's, Banjo's and Dedede's Main from Captain's and retargeted the header words' file ids to their own special files without moving the offsets (Falcon Punch +0x760, Falcon Kick +0xB08, into 64- and 128-byte files): on the N64 they relocate past their file and nothing reads them, on the DS the match's extern restore halted on them (a match with any of the three never started, playtest 1008b-c). `p4_preview_pack.py` now writes such a row load-only (`NDS_BATTLE_EXTERN_LOAD_ONLY`): the file still loads, which is how the content's own special file reaches the status buffer its file setup reads, and the word stays NULL. A list with no TEXID frames that takes its TLUT through the MObj's segment-E branch (the eggs, the grape, the needle) is marked in its row and drawn with the live material (`NDS_P4_VARIANT_MATERIAL`). Entries (2026-10-10, "play-1010a"): each content's entry case runs on its own special file through `NDSP4Overrides.entry_case` and `ndsP4EntryMakeEffect` (the source maker's description with Remix's file and offsets, placed at the entry position): Marth and Roy (their entry routine flips a left arrival, then Link's wave and beam on MARTH_ENTRY_EFFECTS, special file 3), Banjo (Link's case on his file 8), Lanky (Donkey Kong's barrel case, the Tag Barrel on his file 7), Sonic (the barrel case, Tails' plane by arrival side on his file 8) and Crash (Samus's point case on his file 8). The Link-case lists branch to their DObj's one MObj (segment E), so their rows draw with the live material (`material` in `p4_articles.py`, flag bit 1). Two-fighter matches, entry window: Marth+Roy, Crash+Banjo and Lanky+Sonic 0 native failures, 0 declines, 0 entry misses (before the material flag Marth and Roy's wave and beam failed 80 times and Banjo's 2). Dedede's entry is Kirby's case (Remix's entry_script 0x8013DCF4, no patch): his Main externs Kirby's special file 2 (0x15C), so the file is resident and VS's file setup (`ftManagerSetupFilesPlayablesAll`) names it for Kirby's maker; his `entry_case` calls that maker (`ENTRY_PORTS`). Until 2026-10-10 his row was a lab stand-in with no effect. Crash's spin and dig effects (2026-10-10, play-1010c / build-lab-p4am): his special file 2 is his own storage (CRASH_SPIN_GFX); the spin is Link's spin attack description on it with each part's prim and env colour from `CrashNSP.spin_gfx_table` by costume; the dig effect lives in CRASH_DIG_GFX, the file special file 2's word 0xCC0 names (a P4 root may now name its file's base through such a word: `via_base`, material bit 2), attached by position to his top joint, turning with the slope, scrolling its texture with his speed, with Remix's wait, turn and end animation tracks (read from the donor's code segment: `RAM_DATA` in the generator) and the dust and dig sound every 16 and 32 frames; at the end the donor freezes the effect's joint matrix (XObj unk05 = 2), which the port's attach builder cannot, so it is remade free at the same place for the end track. The DS has no rumble. Lab, one Crash forced to B on Castle: 0 native failures, the spin drawn. With four Crashes on Dream Land the lab ROM ran out of texture VRAM at scene start and the spin's translucent lists found no texture (the shipping ROM prepared all nine of his textures in the same match): the worst-case search must check entry-texture VRAM with four contents. Sonic's special files 2 and 4 are read by no code of his profile (`UNREAD_SPECIAL_FILES`: Classic Sonic's model, read only by Remix's select-screen swap to Classic Sonic, and the spring graphic his special 1 names), so the slots read NULL in every build. Peach's turnip (2026-10-10, build-lab-p4am): PeachDSP's three routines (`PeachDSP.ground_initial_`, `air_initial_`, `main`) and Remix's turnip item (items/Turnip.asm) are ported in `nds_p4_peach.c`. Her special file 2 (PEACH_TURNIP_INFO) is her own storage; the turnip is the first P4 item kind, `NDS_P4_IT_KIND_TURNIP` = the source's count (Remix numbers its custom items from 0x2D too), made by the Bob-omb's maker with the turnip's knockback, sound and a face chosen 52/1/1/4 in 58 (plain 2, stitch 30, dot 12, wink 6 damage); its drop, throw and hold routines are Remix's item tables' (`NDSP4Overrides.item`, dispatched from `itMainSetFighterDrop/Throw/Hold` for kinds past the source's; the item attribute cache covers the P4 kinds). One pull in 128 is rare: a Beam Sword (1 in 6) or a Bob-omb (2 in 6) through the vanilla makers, or Mr. Saturn (3 in 6; ported 2026-10-11, "Mr. Saturn, Peach's rare pull" below). The turnip draws natively: a new article kind reads an item's ITAttributes (its DObjDesc tree in the file the attributes name, root admitted as that tree's address less its offset plus the list's: `via_bias`), item GObjs may ask the P4 admission, and the face is a TEXID frame (rows hold 8 frames now). Lab, Peach forced to down-B on Castle: pulled at frame 201, held, thrown at 240, 0 native failures. Dedede's minions (2026-10-10, build-lab-p4am): items/WaddleDee.asm, WaddleDoo.asm and Gordo.asm and DededeDSP's attach and toss are ported in `nds_p4_dedede.c`. His special file 1 (WADDLE_DEE_INFO, its models in WADDLE_DEE_GFX, which loads with it) is his own storage; the three are P4 item kinds (`NDS_P4_IT_KIND_WADDLE_DEE/DOO/GORDO`) made by the charge (0, 1, else) at Samus's charge-shot spot, held at his hand (joint 10) and thrown by the toss's flag (the item's `multi`); thrown ones take his two minion slots (a third ends the oldest). A Waddle Dee or Doo flies spinning, lands, walks on the Bob-omb's walk at the Bob-omb's offsets from its tree (Remix's `remove_bob_bomb_walk_fgm_` keeps the fuse sound to the Bob-omb: a seam in `itBombHeiWalkInitVars`), turns at ledges unless a target is ahead, and every 16 frames looks for a target in its cone: a Dee jumps at it, a Doo charges and fires its beam, four stars on the Star Rod star's attributes (a P4 weapon kind) chained each off the one before, swinging, shown and hitting two frames in four, whose knockback follows the star's facing (Remix's `beam_knockback_direction_fix`, in `ndsP4UpdateHitDamageStats`). A Gordo flies, bounces at 0.95 and sticks to a wall, a ceiling or a floor met under 32 (the Motion Sensor Bomb's surface stick, 5 damage). Read as assembled: a minion that meets a hurtbox or shield pops straight up at half its speed plus 5 (so a Gordo can bounce in place until its time runs out: the 0.95 bounce never gets under 32); the beam's x offset ignores the angle's cosine (each star sits 256 ahead of its parent, only its height swings) and every star starts from the same angle, start-up and lifetime (the maker restarts its stack copies each call); the beam's clang write lands on its hitbox position and is overwritten, so the stars keep the attributes' clang; its element is 3 (Slash; the source comments electric); the hit launch's angle call takes a stale register for the ground flag (never the air value); a Gordo grounded in its aerial state is destroyed without leaving his slots, as in Remix. The models draw natively through the item-attributes article with all nine TEXID frames (rows hold 10 now) and Remix's left-walk list (`extra_lists`: the tree plus 0xC8, the entry's material). Lab, one Dedede forced to down-B on Castle: Waddle Dees tossed, walking, jumping and expiring; Waddle Doos walking both ways and firing (4 stars a beam); Gordos flying and sticking; 0 native failures, drawn (artifacts/visibility/2026-10-10_p4watch-mnc*). Mr. Saturn (Peach's rare pull): ported 2026-10-11 ("Mr. Saturn, Peach's rare pull") |
| S7 generated CPU rows | done for the data: attack lists, ai_long_range and the Remix input routines they name are generated for all 14 (Falco's generated list equals his hand table row for row); attack-prevent, recovery and post-process stay hand-ported per fighter (Falco's; Bowser's prevent and post-process); every content's CPU runs Remix's interpreter extensions ("Remix's CPU commands" above) |
| S8 Kirby copies | done for the fourteen ("Kirby's copies" above): statuses, routines and Kirby branches, AI, Remix hats as native copy-hat images with their trio programs, the powers' weapons as sidecar roots; lab sweep 0 native failures, 0 declines but Crash's 1 |
| S9 select-screen preview | done for Falco on the menu ROM; pack facts come from the export (`p4_preview_pack.py --content`); the all-content ROM cannot hold P4 menu clips in the select's global reservation (see above) |
| S10 acceptance probe | lab probes exist per topic (`gNdsLabP4Content`); no single command |
| S11 whole-roster generation | all 14 generate, learn owner pins and build into one lab ROM (`NDS_P4_LAB_FALLBACK=1`); four-CPU mirror results in "Whole roster in the lab" |
| S12 EXTRA export | done 2026-10-11: the three export with 0 failures from EXTRA's tree (now every content's staging); Meta Knight ported and in the play ROM, MRGAW ported in the lab (Dango open), Snake next ("EXTRA characters" above) |
| S13 sword trails | done: Remix rows exported and generated, update and native draw; vanilla Link and Beam Sword trails drawn too |
| S14 own action arrays | done: content special-status tables (Falco verified unchanged; Ganondorf 0 stand-ins) |
| S15 P4 memory | open, and it blocks mixed matches: one content with three of the original cast overflows the heap at battle start, and so does any two-content mix; mirrors fit. Lab ("p4q", Dream Land): Donkey Kong's setup takes 49.9 KB (26.3 KB pack, 2.8 KB native shield pose, 11.6 KB animation heap), Bowser's 179.9 KB (a 145.3 KB file pool: model 112.5 KB, of which 37.1 KB geometry (4.5 KB of it model-part variants), 41 KB MObj texture frames and palettes, about 21 KB textures its lists load and other data, 12 KB structure; motion 11 KB, Clown Copter 11.3 KB, shield pose 5.1 KB; a 23.2 KB animation heap). Content file paths are now computed instead of stored (1,558 strings), which gives every match 47 KB: the four originals end setup with 53.2 KB, and Bowser with three originals gets past Kirby's load but runs out making parts. The match tables (motion descriptors, CPU attack lists and the Remix routines they name: 48 KB for the fourteen) then left the binary for `nitro:/p4/<name>.tab`, read into the heap only for the contents a scene makes fighters of (`ndsP4LoadTables`; "p4r": `nds_p4_data.o` 72.5 -> 25.0 KB, the arena 918.5 -> 992.0 KB). The four originals end a 300-frame lab run with 65.0 KB, after the caches that keep a free floor (stage GX body 25.3 KB, lean spare 17.7 KB, world caches 13.3 KB) took theirs; Bowser with three originals now makes all four fighters and stops at Kirby's hat admission with 17.3 KB free (the admission keeps the 25.6 KB floor). Bowser costs about 177 KB more than Donkey Kong (setup +133.5 KB, his native images +27 KB, 16.4 KB more before setup), against about 96 KB that match can give. A content's match now loads its select-screen pack ("p4s", `ndsP4SetupPackFiles`): Main whole, the model without the display lists its native owner image draws -- every model-part variant the image carries is pruned too, not only part 0 (Bowser's model 112.5 -> 75.4 KB kept, Banjo's 162.9 -> 86.9, Crash's 143.2 -> 79.7) -- and the pack's manifest (`fighters/preview/<kind>.ext`, the original cast's BEX1 rows) restores the Main pointers into the content's other files, loading them, before the source's `ftManagerSetupFilesKind` publishes the file globals. Bowser's file setup 183.4 -> 124.6 KB. One content beside Samus, Link and Kirby (Dream Land, 600 frames) now plays for nine of the fourteen, 0 native failures: low-water Falco 48.6 KB, Wario 48.4, Peach 25.8, Roy 24.8, Marth 15.0, Ganondorf 13.7, Bowser 11.3, Wolf 7.8 KB (the floor is 25.6 KB); Lanky, Sheik, Sonic and Dedede stop at Kirby's hat admission (33.6-44.9 KB free), Crash and Banjo run out at Kirby's setup. Per-content setup ("p4s"): Wolf 147.8 KB of which extern bank 0x6D 47.1 KB (only the Wolfen's four texture pointers name it, and its native entry packet carries those pixels, as Fox's Arwing's does) and his shield-pose file 29.1 KB; Dedede 152 KB of which pack 53.7, shield pose 20.4, own special files 28.3. The contents' own shield-pose files are 20-42 KB (Lanky 41.8, Sheik 38.1, Sonic 35.4, Wolf 29.2, Banjo 25.8, Marth and Roy 25.0, Crash 22.6, Peach 22.3, Dedede 20.4; Bowser 5.1) and run the source Event32 path; Falco, Ganondorf and Wario load their parent's raw file. Both are now done ("p4u"). The Wolfen's four texture pointers are generated as baked references (`NDSP4BakedRef`, `ndsP4NativeOwnsDependency`; an entry article's foreign pointers must all be G_SETTIMG images or the generator fails), so bank 0x6D never loads: Wolf's file setup 155.5 -> 108.4 KB. Each content guards with a native NSP1 package (`scripts/p4/p4_shield_pose.py`, which reproduces seven of the original cast's nine packages byte for byte and differs in the other two only by the token it reserves): Wolf 29.1 -> 3.0 KB, Sheik 38.0 -> 5.1, Lanky 41.8 -> 10.9, Sonic 35.3 -> 6.7, Banjo 25.7 -> 3.2, Marth and Roy 24.9 -> 7.1 (their guard poses reach 530 units, past Q6's s16, so a wide-value token carries an s32), Crash 22.5 -> 4.0, Peach 22.2 -> 4.4, Dedede 20.3 -> 2.3, Bowser 5.0 -> 1.6 KB; Falco and Ganondorf use Fox's and Captain's packages (same file, same nine targets); Wario keeps Mario's raw file (no Mario package). A content package keeps its dobj_lookup rows in the heap (the shared scratch is in DTCM, sized for the original cast; 40 rows overran the boot stack's margin) and, past 58 words, its own decode scratch (Lanky's longest script is 491 words). With player 0 held in shield, Wolf, Marth, Lanky, Falco, Ganondorf and Sonic mirrors apply the native pose about 200 times in 300 frames, 0 decode failures, 0 native failures. One content beside Samus, Link and Kirby, 600 frames, low-water: Falco 55.7 KB, Wolf 54.2, Wario 48.9, Peach 38.2, Roy 35.5 (above the floor); Marth 25.2, Ganondorf 20.0, Lanky 18.8, Sonic 14.7, Sheik 6.6 (below it); Bowser and Dedede stop at Kirby's hat admission (63.6 and 46.2 KB free: Kirby's three copy hats in both details take about 38 KB and the admission keeps the 25.6 KB floor), Crash and Banjo still run out at Kirby's setup. Owner 2026-10-08, asked with those numbers: lower content fidelity rather than a character-select budget. A content in a battle of three or four fighters now draws its low-detail model in the source's KO and pause close-ups too (`ndsP4ContentLowDetailOnly`; `ftParamSetModelPartDetailAll` keeps it low), so its high-detail owner image is never admitted; one- and two-fighter battles keep both. The largest-first image preload also passes the player now (a content outside port 0 preloaded its parent's images). "p4v": Bowser 43.8 KB, Dedede 30.7, Sheik 36.1, Sonic 44.3, Lanky 52.7, Ganondorf 45.8, Marth 46.2 -- twelve of the fourteen above the floor beside Samus, Link and Kirby; Crash stops at the hat admission with 53.6 KB free, Banjo with 15.3. Sonic's four-CPU mirror overflows the heap at fighter setup, Crash's leaves 7.7 KB ("Memory" above). The pack now prunes every alternate model part, not only the ones the owner image carries ("p4w"): a variant the image does not carry declines the native draw whether its slot names a source list or a root cell, and no ROM draws source lists, so its bytes were dead. Model kept: Banjo 86.9 -> 62.9 KB (40 variant roots), Crash 79.7 -> 72.6; packs Banjo 103.6 -> 78.9 KB, Crash 91.2 -> 83.9, Peach 52.1 -> 45.0. Beside Samus, Link and Kirby: Crash stops at the hat admission with 60.7 KB free (about 1 KB short), Banjo with 39.0; Peach's low-water is 41.9 KB. A special slot naming a file outside the content's closure now reads NULL unless another fighter of the match loaded it, as Remix's status-buffer lookup does; only a lab stand-in (the parent's file in place of a donor file) still loads whole (`OpenSpecialMask`). Banjo names Captain's Falcon Kick and car file (26 KB), which his closure never loads and nothing of his reads: "p4x" Banjo's setup 168.8 -> 142.7 KB. He and Crash still stop at the hat admission, with 64.0 and 59.8 KB free: Kirby's three hats in both details take about 38.5 KB, and the 88.5 KB frontend-overlay tail the largest-first preload fills first (Crash's low image 63.8 KB and Kirby's 24.3 KB) has 336 B left. Lanky's low-water is 51.6 KB, Sheik's 37.2. A battle of three or four fighters now loads each content's low-detail pack (`fighters/battle/<kind>.fpc`, `ndsP4LowDetailBattle`, `p4_preview_pack.py --battle-out`; "p4y"): the select pack less what only the high detail reads there -- the MObjs, matanims and textures of every joint whose low DObjDesc names its own list (the low detail takes a joint without one from the high rows: lbCommonSetupFighterPartsDObjs, ftParamSetModelPartDetailAll, the hidden parts) and of the high model-part rows -- with the slots naming them NULL; both JointTrees and the per-joint arrays stay whole. The select screen, one- and two-fighter battles and Results keep the select pack, and one manifest serves both. Packs: Crash 83.9 -> 53.5 KB, Bowser 83.9 -> 58.6, Banjo 78.9 -> 56.0, Lanky 62.0 -> 40.7, Ganondorf 60.5 -> 40.1, Dedede 59.2 -> 40.1, Sonic 58.1 -> 38.0, Peach 45.0 -> 31.2, Marth 53.9 -> 41.9, Roy 44.8 -> 33.0, Sheik 36.7 -> 26.3, Wario 45.5 -> 35.4, Falco 26.9 -> 21.8; Wolf's details share one tree. Beside Samus, Link and Kirby (600 frames, 0 native failures), low-water: Falco 63.1 KB, Wolf 62.3, Marth 55.3, Bowser 53.0, Peach 53.0, Ganondorf 50.2, Lanky 49.2, Roy 49.0, Sonic 49.0, Wario 47.0, Sheik 46.2, Crash 38.3, Dedede 38.2 -- thirteen of fourteen above the floor. Banjo passes the hat admission and ends at 19.8 KB: his match grows the AObj pool 6.5 KB in 150 frames (180 allocations, his Kazooie joints' animations), and the front-end tail is full before the admission (Kirby's two images, Link's high, 12.6 KB of other scene assets), so hats taken image by image from it gain nothing (measured, not kept). His own special3 (9.0 KB, a lab stand-in of 2.2 KB today) will cost a shipping build 6.8 KB more The shipping image with all fourteen (playtest "play-1008b") is 204 KB larger than the original cast's (.main +148 KB, .main.rw +12 KB, .main.bss +24 KB, the menu overlay +20 KB, of which newlib's floating-point formatter 18 KB for one `snprintf`, now `sniprintf`), and the VS select halted entering: its preview arenas (24 KiB shared, 80 KiB a slot) asked the heap for more than it had, and `syTaskmanMalloc` halts on overflow. The pool now asks `ndsSyMallocWouldFit` for the whole set and switches the previews off when it does not fit (the select opens with 304,936 B free). The owner-image binding is a table per image (`NDSImgBindDesc`) instead of each owner's inlined field assignments, and the P4 fighters' routine files are `-Os`: .main -21.5 KB. The P4 previews themselves need 100-150 KB a slot (pack plus high-detail mesh: Crash 148, Banjo 135, Bowser 117, against the original cast's 55 KB at most), so the select's previews need a budget per slot, not four 80 KiB arenas. 2026-10-11: every content's four-CPU mirror fits on the shipping ROM (34.5-129.2 KB, "Memory: mirrors, the event32 ledger, article data" above); the heavy four-content mix (7.3 KB) and Sonic's and Meta Knight's Zebes mirrors (12.6, 19.8 KB) stay under the floor. |

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

## Board 1 status (master plan Revision 3)

| Item | State |
|---|---|
| S1 content list | done: registry, Makefile templates, one native-owner emit, `NDS_P4_CONTENT_ROWS` in every shared site; Falco unchanged (lab: 0 declines, menu ROM preview) |
| S2 build throughput | not started; lab and menu builds still rewrite the shared linker script under a mutex |
| S3 table seams | 20 of 33 (Falco's 12, entry_action/entry_script, the six special-move starters, grounded_script) plus the jab tables (jab_3, jab_3_timer, jab_3_action, rapid_jab and its four rows, "Jab tables" above), yoshi_egg and pipe_turn. `NDSP4KindCases` (generated): the landing switch's case Remix's grounded_script row names (Marth, Roy and Sonic take Mario's; Banjo, Dedede and Wario none; Peach's own routine, a lab stand-in for now) and pipe_turn, Mario's turn in the Dokan statuses (Wario; Crash and Lanky, Mario's other children, take the default turn). The port's landing switch (`mpCommonSetFighterLandingParams`) had kept only Mario's case: Luigi's cyclone never rose again after its first aerial use and Samus's aerial charge-shot lift fell for the rest of the stock; it is the source's whole now. Left: initial_script (no-op zero writes in VS but Dedede's, a routine), gfx_routine_end (Dedede, Sheik), label_height (Results: Crash, Lanky, Peach), variant_original, the Kirby tables (S8) |
| S4 motion commands | 11 of the roster's 12: all but 0xD9 SET ENV COLOR, which needs the renderer's per-fighter environment colour (Banjo, Marth, Roy) |
| S5 routine work lists | generated for all 14 (`scripts/p4/routine_worklist.py --lab <lab build>` into a build directory): 296 donor routines with Remix scope, file and line, size and users (Banjo 33, Bowser 10, Crash 29, Dedede 37, Lanky 29, Marth 18, Peach 22, Roy 21, Sheik 34, Sonic 29, Wario 23, Wolf 11; Falco and Ganondorf none), plus each content's id tests in Remix's shared code (17-39); the classes (behaviour, presentation, 1P, toggle) are still read by hand. Ported: Wolf 11 of 11 (`src/port/nds_p4_wolf.c`: WolfUSP, WolfDSP, WolfNSP with his own shot, the slash, reflector and Wolfen on his own files), Bowser 10 of 10 (`src/port/nds_p4_bowser.c`), Marth 18 of 18 and Roy 21 of 21 (`src/port/nds_p4_marth.c`, `nds_p4_roy.c`; MarthUSP and MarthNSP are both of theirs, `nds_p4_roy.c` compiles Marth's file when he is not enabled). Marth's counter is Remix's head of `ftParamUpdateDamage` (`NDSP4Overrides.update_damage`). Read as assembled: Dolphin Slash's landing lag keeps the low half of the Mario routine it copied (0x3EC05C29), and a stick short of 10 forward drives 20 (the compare's 10 is added again). Roy's early release from the charge's first status calls the end with whatever action id a1 held; DSP_BEGIN.bin never sets temp variable 2, so that path is dead. Lab ("p4aa", Dream Land, 0 native failures): four Marths' CPUs use up and neutral B; player 0 forced to down-B counters 27 times and strikes back 5 (1,500 frames); forced B taps chain Dancing Blade 26 times in 900 frames; four Roys forced to down-B charge and release 30 Flare Blades. Left for both: their entry effect (Link's entry case on their own file 0xDA7, S6). Wario 23 of 23 (`src/port/nds_p4_wario.c`: Body Slam with its recoil, Corkscrew, Ground Pound). Body Slam's bounce is Remix's head of `ftMainSetHitInteractStats` (`NDSP4Overrides.on_hit_interact`, which Sheik's and Banjo's recoils use too): the source defines and calls that function in `battleship_ftmain.c`, so its calls there take the hook through a macro that tells them from the definition by the first argument's first token, and the port's wrapper takes it for the other units. Read as assembled: the slam's wall test reads collision word 0xCC, `coll_data.mask_prev` -- the map callback runs after ftmain moves the current mask there and zeroes it, so it is last frame's walls; the recoil's fall slows by 1.25 for Remix's Wario id and 0.55 for the Kirby and Polygon copies; Corkscrew's landing lag keeps the low half of Mario's constant (0x3E805C29). Lab ("p4ab", Dream Land, 0 native failures, 0 lab fallbacks left): four Warios' CPUs slam 14 times (4 bounces), corkscrew 4 and ground-pound 7 times in 1,500 frames; player 0 forced to B slams 14 times in 900 frames (3 bounces, 5 run off the floor or jump out), forced to down-B pounds 10 times (7 from the ground, which hops) and lands the pound 6. Peach 19 of 22 (`src/port/nds_p4_peach.c`: Peach Bomber, the parasol, and her float), the three of PeachDSP (the turnip pull) waiting on Remix's turnip item (S6). The float is Remix patching the parent's common code on her id, each a new `NDSP4Overrides` seam: the gravity function's head (`gravity`), the Jump, JumpAerial, Fall and Pass interrupts' jump check (`air_jump_check`, tried where the aerial check ends in each import TU), Fall's status change (`fall_status`), the aerial item throw (`air_item_throw_block`), the damage routine's head (`on_damage`), the landing switch (`on_landing`: her grounded_script, which a ledge catch skips -- the cliff seam marks it, Remix's ledge_flag) and FallSpecial's status (`fall_special_status`, the parasol's fall); her status hook gives aerials started in a float no landing lag and a ledge get-up her float back. `GROUNDED_PORTS` (generator) lets a grounded_script routine with a port take the default case. Read as assembled: the float's start keeps 0.25 of her fall (the source comments 0.375); her parasol's landing lag keeps the low half of Mario's constant (0x3EC05C29). Lab ("p4ac", 0 native failures): four Peaches' CPUs bomb 10 times (2 bounces) and open the parasol; player 0 holding a jump button floats 3 times in 900 frames (340 float frames, float-on after aerials 11); forced up-B opens and floats the parasol 5 times, and driven by gdb closes it into her parasol fall 3 times and reopens it. Crash 29 of 29 (`src/port/nds_p4_crash.c`: the spin with its shield block and item fling, the belly flop, the dig and its dive), and the patches Remix makes on his id: the dig's collision box (his attributes' box shrinks in DSPWait/DSPTurn, every Crash sharing them as in Remix, and Size.asm's per-frame copy of the attributes' box into the fighter at `ftMainProcPhysicsMap`'s head, `before_physics_map`), the dig's grab immunity (`grabbable`; the patch reads the id through the hurtbox pointer the source's loop walks, so only his first hurtbox is exempt), his down-air bounce in Link's rehit case (`attack_air_lw_hit`), the tomato's eat sound (`on_eat_tomato`), and the blocked spin's lost hitlag (his shield routine zeroes the push the source charges after it, unless it rebounds). Read as assembled: the spin's and the dig's steering leave the velocity as it is when it is within 2 of the target (the donor's `mov.s f2, f4` meant the opposite); the belly flop's air start keeps 0x3D480000 of the fall (the source's "0.05"). S6 owns his spin and dig effects and what the donor runs only while the dig effect exists (its dust, sound and rumble, the dive landing's sounds). Lab ("p4ad", 0 native failures): four Crashes' CPUs spin, flop (2 landings), dig (2) and down-air bounce (2) in 1,500 frames; forced down-B with the stick digs 5 times (a turn, 4 ends); all four forced to B spin 132 times. Lanky 29 of 29 (`src/port/nds_p4_lanky.c`: the Grape Shooter's ammo and refire, the balloon with its steerable balloon damage, and OrangStand's 15 statuses -- walk, turn, jump squat and jump on his hands, platform drop, taunt and the cancel that replays the taps it buffered), and Remix's `damage_patch_`, which turns a DamageFly status (or, after an electric hit, the one it leads to) into his balloon damage while inflated (`damage_status`: `battleship_ftcommon_damage.c` tells `ftCommonDamageInitDamageVars`' status change from the file's other one by the first argument). Read as assembled: the balloon's air start masks a register the donor never loaded with byte 0x18D (the port clears the four flags the other air starts clear); OrangStand's end pushes 16 forward on the ground too (the branch skips one instruction, not the block); his jump is the rest of `ftDonkeyThrowFJumpSetStatus` entered at its status change with a frame speed of 1. His grape is his own weapon on his special file 1 (S6); S6 owns his entry barrel (dkshared.asm). Lab ("p4ae", 0 native failures): four Lankies' CPUs shoot (5), balloon (2 of 5 burst, the rest knocked out: 41 damage-status checks, 67 frames of steered balloon damage); forced down-B into a gdb-driven walk: 6 walks, 7 waits. Sheik 34 of 34 (`src/port/nds_p4_sheik.c`: the vanish with its grounded and aerial moves and ends, the needle charge on Samus's charge routines -- six needles, kept through hits -- and its throws, and Bouncing Fish with its one lift per landing, its tap buffer and its wall and contact recoil); the recoil on contact is Wario.asm's `body_slam_recoil_` (`on_hit_interact`). Read as assembled: the vanish aims straight up under a stick of 11 (cvt.w.s rounds: x*x + y*y <= 110); a hit out of the charge runs Samus's damage routine, which clears passive word 0 (the fish's lift), not the needles (word 1); a roll out of the ground charge passes the fighter object as the item-throw buffer, so a throw stays open the whole roll; the fish's recoil takes its status-change preserve word from the stack slot where it saved its return label, 0x80566ACC, and its landing a word the donor never writes (the DS preserves nothing there); the recoil's stick clamps are inverted (facing right, anything faster than -40 becomes -40 and the rest -20). The needle is his own weapon on his special file 1 (S6); S4 owns the vanish's environment colour, and the gfx-routine table the full charge's flash (Remix id 0x6D, which the DS colanim table declines). Lab ("p4af", 0 native failures): four Sheiks' CPUs vanish 5 times, charge and throw needles 6 times (259 charge frames, 160 throw frames) and fish 3 times with 39 recoil frames in 1,500 frames; player 0 forced to up-B vanishes 6 times in 900 frames (5 from the ground, 72 move frames). Banjo 33 of 33 (`src/port/nds_p4_banjo.c`: the eggs forward and backward, the Beak Bomb with its buffered attack, recoil and wall splat, Beak Barge and Bill Drill); the Beak Bomb's recoil on contact is Wario.asm's `body_slam_recoil_`. Read as assembled: the Beak Bomb's begin and attack test v0 for their air friction, and v0 still holds the physics routine's own address there (the source's `jalr`), so neither applies it; its maps are Mario's Super Jump Punch map keeping the low half of his landing constant (0x40005C29 and 0x3F805C29, "1.5" and "1.0"); its begin's special fall lands in 2.0, not the source's "1.5"; its start runs Captain's Falcon Dive status-vars routine as the status change's proc_status and plays its events through `ftMainSetStatus`' home slot; Bill Drill's air status copies Captain's without the TransN rotation store. His eggs are his own weapons on his special file 4 (S6); S6 owns his entry. Lab ("p4ag", 0 native failures): four Banjos' CPUs lay 8 eggs (67 forward, 209 backward frames) and drill 3 times in 1,500 frames; player 0 forced to up-B with B taps starts 11 Beak Bombs, attacks in 350 frames and recoils in 91; forced down-B barges 4 times from the ground; Sonic 29 of 29 (`src/port/nds_p4_sonic.c`: the Homing Attack with its target search over fighters and items, its move, end and recoil, the Spring Jump, and the Spin Dash's charge and roll on the ground and in the air). Read as assembled: the homing search skips himself, teammates without team attack and fighters below status 7, takes items whose hurtbox is live, and looks 2,208 ahead inside a cone of 1,000 + x/2; a spent spring keeps every aerial special from starting until he lands or is hit (`action_check_patch_`, a new `air_special_block` override at `ftCommonSpecialAirCheckInterruptCommon`'s head, and `SonicUSPRefresh` on damage); an ejected object stops being anyone's homing target (`destroyed_target_fix_` at `gcEjectGObj`'s head). The spring is his own weapon on his special file 1 (S6): it lands on Samus's bomb map routine, uncoils, and bounces every airborne fighter that falls onto it at 130 (into JumpF from the jumps, tumble, the shield-break falls and Remix's per-character recoveries, else with jump smoke), giving Sonic his up special back; read as assembled, its hit records' "clear" sets the hurt, shield and reflect flags with group 0, and the PSI Magnet case clears what Remix names the overlay, which in BattleShip's struct is the loop sound's handle and id, the first colour script and the colour flags, so a loop sound playing there outlives the jump's status change. Lab ("p4ah", 0 native failures): four Sonics' CPUs home in twice, spring 4 times and spin dash 7 times (68 ground and 187 air roll frames) in 1,500 frames; player 0 forced to up-B springs 14 times (7 from the ground); forced to B homes in through 107 move frames and recoils on 186; Dedede 37 of 37 (`src/port/nds_p4_dedede.c`: Super Dedede Jump with its cancel, landing and ceiling bonk; the inhale's own statuses around Kirby's, its walk, spit and the spat fighter's flight; the minion toss's charge, cancel and throw). His inhale runs Kirby's `ftkirbyspecialn.c` on his statuses: in the Kirby TU every status change goes through the content's `kirby_inhale_status` (Remix's 25 DededeNSP hooks), and the copy table it reads is the port's copy of Kirby's rows, his file or not in the match; a held fighter waits 250 plus its percent (`custom_initial_absorbed_timer_`), and a Remix victim's star deals its `kirby_inhale_struct` damage (the generator emits that row, `NDSP4KirbyInhale`, for every content). The inhale absorbs (Reflect.asm's custom kind): a weapon or item it catches runs its absorb or blast-zone routine and is destroyed, not reflected (dispatchers ahead of `wpProcessProcHitCollisions` and `itProcessProcHitCollisions`), Samus's bomb and Pikachu's thunder read reflectable for his search, and the reflect switch's custom routine sends him to the spit, which fires a Star Rod star. Remix's jump_fix_1-5 take him down Kirby's multi-jump branches with his heights (68, 58, 52) through dispatchers for the jump check, the multi-jump status and the aerial physics (`multi_jump_vel`); `initial_script_` clears his minion pointers and charge at spawn and on every stock (`on_init`); Kirby's wall and floor stars (`kirby_cpu_inhale_4`) and the inhale wind 1,000 ahead (`dedede_gfx_`) are his too. Read as assembled: the walk's "same walk" test reads code words, so it always changes status; the ground spit ends in the fall; the spat star's smash flag is a pointer's low byte, so it flies at the smash speed and lifetime with the tilt attributes; the up special's ceiling test after a ledge catch reads a temporary the catch leaves, zero since Remix's Dedede keeps the ledge. The landing stars are Yoshi's star weapon on his special file 2 (YoshiShared.asm `down_special_struct_fix`, S6), drawn as the item file's shared star quad; S6 owns his minions and his entry (Kirby's warp star, from Kirby's special file 2). Lab ("p4ai", 0 native failures): four Dededes' CPUs inhale 11 times (104 spit frames), start 3 tosses and multi-jump 21 times in 1,500 frames; player 0 forced to hold B against Mario, Samus and Fox swallows a shot and spits a star, and holds and spits a fighter; forced up-B jumps 5 times; forced down-B starts 11 times, charging over 891 frames and tossing over 216 |
| S6 articles | the path exists, Wolf proves it: own special-file storage (`OWN_SPECIAL_FILES`), the parent's effect makers a content replaces (`gNdsP4<Title>Overrides`), `scripts/p4/p4_articles.py` roots compiled by `generate_nds_entry_effects.py --p4` into the build's packet, native admission, TEXID frames, state-only lists, per-match texture preparation and the held-gun sidecar ("Own special files and articles" above). Bowser's Clown Copter draws from his own special2 (his flame is the Fire Flower's weapon). Weapons ported: Sheik's needle, Banjo's two eggs, Lanky's grape and Sonic's spring (`NDS_P4_WP_KIND_*`, one kind past the source's per weapon), and Dedede's landing stars (Yoshi's star kind on his file). Lab ("p4al", Dream Land, 900 frames, player 0 forced to the special, 0 native failures, 0 article misses): four Sheiks throw 18 needles (144 live frames); four Banjos lay 22 eggs (354 forward, 204 backward frames); four Lankies shoot 16 grapes (795 frames); four Sonics make 11 springs (2,460 frames) that bounce fighters 17 times, 12 of them into JumpF; four Dededes land 3 jumps and throw 6 stars. Remix built Sheik's, Banjo's and Dedede's Main from Captain's and retargeted the header words' file ids to their own special files without moving the offsets (Falcon Punch +0x760, Falcon Kick +0xB08, into 64- and 128-byte files): on the N64 they relocate past their file and nothing reads them, on the DS the match's extern restore halted on them (a match with any of the three never started, playtest 1008b-c). `p4_preview_pack.py` now writes such a row load-only (`NDS_BATTLE_EXTERN_LOAD_ONLY`): the file still loads, which is how the content's own special file reaches the status buffer its file setup reads, and the word stays NULL. A list with no TEXID frames that takes its TLUT through the MObj's segment-E branch (the eggs, the grape, the needle) is marked in its row and drawn with the live material (`NDS_P4_VARIANT_MATERIAL`). Entries (2026-10-10, "play-1010a"): each content's entry case runs on its own special file through `NDSP4Overrides.entry_case` and `ndsP4EntryMakeEffect` (the source maker's description with Remix's file and offsets, placed at the entry position): Marth and Roy (their entry routine flips a left arrival, then Link's wave and beam on MARTH_ENTRY_EFFECTS, special file 3), Banjo (Link's case on his file 8), Lanky (Donkey Kong's barrel case, the Tag Barrel on his file 7), Sonic (the barrel case, Tails' plane by arrival side on his file 8) and Crash (Samus's point case on his file 8). The Link-case lists branch to their DObj's one MObj (segment E), so their rows draw with the live material (`material` in `p4_articles.py`, flag bit 1). Two-fighter matches, entry window: Marth+Roy, Crash+Banjo and Lanky+Sonic 0 native failures, 0 declines, 0 entry misses (before the material flag Marth and Roy's wave and beam failed 80 times and Banjo's 2). Left: the items (Peach's turnip and its rare pulls, Dedede's Waddle Dee, Waddle Doo and Gordo: native item kinds and item-GObj admission), Dedede's entry (Kirby's case) and Crash's spin and dig effects |
| S7 generated CPU rows | done for the data: attack lists, ai_long_range and the Remix input routines they name are generated for all 14 (Falco's generated list equals his hand table row for row); attack-prevent, recovery and post-process stay hand-ported per fighter (Falco's; Bowser's prevent and post-process); every content's CPU runs Remix's interpreter extensions ("Remix's CPU commands" above) |
| S8 Kirby copies | not started |
| S9 select-screen preview | done for Falco on the menu ROM; pack facts come from the export (`p4_preview_pack.py --content`); the all-content ROM cannot hold P4 menu clips in the select's global reservation (see above) |
| S10 acceptance probe | lab probes exist per topic (`gNdsLabP4Content`); no single command |
| S11 whole-roster generation | all 14 generate, learn owner pins and build into one lab ROM (`NDS_P4_LAB_FALLBACK=1`); four-CPU mirror results in "Whole roster in the lab" |
| S12 EXTRA export | not started; EXTRA's nested Remix gitlink not initialized |
| S13 sword trails | done: Remix rows exported and generated, update and native draw; vanilla Link and Beam Sword trails drawn too |
| S14 own action arrays | done: content special-status tables (Falco verified unchanged; Ganondorf 0 stand-ins) |
| S15 P4 memory | open, and it blocks mixed matches: one content with three of the original cast overflows the heap at battle start, and so does any two-content mix; mirrors fit. Lab ("p4q", Dream Land): Donkey Kong's setup takes 49.9 KB (26.3 KB pack, 2.8 KB native shield pose, 11.6 KB animation heap), Bowser's 179.9 KB (a 145.3 KB file pool: model 112.5 KB, of which 37.1 KB geometry (4.5 KB of it model-part variants), 41 KB MObj texture frames and palettes, about 21 KB textures its lists load and other data, 12 KB structure; motion 11 KB, Clown Copter 11.3 KB, shield pose 5.1 KB; a 23.2 KB animation heap). Content file paths are now computed instead of stored (1,558 strings), which gives every match 47 KB: the four originals end setup with 53.2 KB, and Bowser with three originals gets past Kirby's load but runs out making parts. The match tables (motion descriptors, CPU attack lists and the Remix routines they name: 48 KB for the fourteen) then left the binary for `nitro:/p4/<name>.tab`, read into the heap only for the contents a scene makes fighters of (`ndsP4LoadTables`; "p4r": `nds_p4_data.o` 72.5 -> 25.0 KB, the arena 918.5 -> 992.0 KB). The four originals end a 300-frame lab run with 65.0 KB, after the caches that keep a free floor (stage GX body 25.3 KB, lean spare 17.7 KB, world caches 13.3 KB) took theirs; Bowser with three originals now makes all four fighters and stops at Kirby's hat admission with 17.3 KB free (the admission keeps the 25.6 KB floor). Bowser costs about 177 KB more than Donkey Kong (setup +133.5 KB, his native images +27 KB, 16.4 KB more before setup), against about 96 KB that match can give. A content's match now loads its select-screen pack ("p4s", `ndsP4SetupPackFiles`): Main whole, the model without the display lists its native owner image draws -- every model-part variant the image carries is pruned too, not only part 0 (Bowser's model 112.5 -> 75.4 KB kept, Banjo's 162.9 -> 86.9, Crash's 143.2 -> 79.7) -- and the pack's manifest (`fighters/preview/<kind>.ext`, the original cast's BEX1 rows) restores the Main pointers into the content's other files, loading them, before the source's `ftManagerSetupFilesKind` publishes the file globals. Bowser's file setup 183.4 -> 124.6 KB. One content beside Samus, Link and Kirby (Dream Land, 600 frames) now plays for nine of the fourteen, 0 native failures: low-water Falco 48.6 KB, Wario 48.4, Peach 25.8, Roy 24.8, Marth 15.0, Ganondorf 13.7, Bowser 11.3, Wolf 7.8 KB (the floor is 25.6 KB); Lanky, Sheik, Sonic and Dedede stop at Kirby's hat admission (33.6-44.9 KB free), Crash and Banjo run out at Kirby's setup. Per-content setup ("p4s"): Wolf 147.8 KB of which extern bank 0x6D 47.1 KB (only the Wolfen's four texture pointers name it, and its native entry packet carries those pixels, as Fox's Arwing's does) and his shield-pose file 29.1 KB; Dedede 152 KB of which pack 53.7, shield pose 20.4, own special files 28.3. The contents' own shield-pose files are 20-42 KB (Lanky 41.8, Sheik 38.1, Sonic 35.4, Wolf 29.2, Banjo 25.8, Marth and Roy 25.0, Crash 22.6, Peach 22.3, Dedede 20.4; Bowser 5.1) and run the source Event32 path; Falco, Ganondorf and Wario load their parent's raw file. Both are now done ("p4u"). The Wolfen's four texture pointers are generated as baked references (`NDSP4BakedRef`, `ndsP4NativeOwnsDependency`; an entry article's foreign pointers must all be G_SETTIMG images or the generator fails), so bank 0x6D never loads: Wolf's file setup 155.5 -> 108.4 KB. Each content guards with a native NSP1 package (`scripts/p4/p4_shield_pose.py`, which reproduces seven of the original cast's nine packages byte for byte and differs in the other two only by the token it reserves): Wolf 29.1 -> 3.0 KB, Sheik 38.0 -> 5.1, Lanky 41.8 -> 10.9, Sonic 35.3 -> 6.7, Banjo 25.7 -> 3.2, Marth and Roy 24.9 -> 7.1 (their guard poses reach 530 units, past Q6's s16, so a wide-value token carries an s32), Crash 22.5 -> 4.0, Peach 22.2 -> 4.4, Dedede 20.3 -> 2.3, Bowser 5.0 -> 1.6 KB; Falco and Ganondorf use Fox's and Captain's packages (same file, same nine targets); Wario keeps Mario's raw file (no Mario package). A content package keeps its dobj_lookup rows in the heap (the shared scratch is in DTCM, sized for the original cast; 40 rows overran the boot stack's margin) and, past 58 words, its own decode scratch (Lanky's longest script is 491 words). With player 0 held in shield, Wolf, Marth, Lanky, Falco, Ganondorf and Sonic mirrors apply the native pose about 200 times in 300 frames, 0 decode failures, 0 native failures. One content beside Samus, Link and Kirby, 600 frames, low-water: Falco 55.7 KB, Wolf 54.2, Wario 48.9, Peach 38.2, Roy 35.5 (above the floor); Marth 25.2, Ganondorf 20.0, Lanky 18.8, Sonic 14.7, Sheik 6.6 (below it); Bowser and Dedede stop at Kirby's hat admission (63.6 and 46.2 KB free: Kirby's three copy hats in both details take about 38 KB and the admission keeps the 25.6 KB floor), Crash and Banjo still run out at Kirby's setup. Owner 2026-10-08, asked with those numbers: lower content fidelity rather than a character-select budget. A content in a battle of three or four fighters now draws its low-detail model in the source's KO and pause close-ups too (`ndsP4ContentLowDetailOnly`; `ftParamSetModelPartDetailAll` keeps it low), so its high-detail owner image is never admitted; one- and two-fighter battles keep both. The largest-first image preload also passes the player now (a content outside port 0 preloaded its parent's images). "p4v": Bowser 43.8 KB, Dedede 30.7, Sheik 36.1, Sonic 44.3, Lanky 52.7, Ganondorf 45.8, Marth 46.2 -- twelve of the fourteen above the floor beside Samus, Link and Kirby; Crash stops at the hat admission with 53.6 KB free, Banjo with 15.3. Sonic's four-CPU mirror overflows the heap at fighter setup, Crash's leaves 7.7 KB ("Memory" above). The pack now prunes every alternate model part, not only the ones the owner image carries ("p4w"): a variant the image does not carry declines the native draw whether its slot names a source list or a root cell, and no ROM draws source lists, so its bytes were dead. Model kept: Banjo 86.9 -> 62.9 KB (40 variant roots), Crash 79.7 -> 72.6; packs Banjo 103.6 -> 78.9 KB, Crash 91.2 -> 83.9, Peach 52.1 -> 45.0. Beside Samus, Link and Kirby: Crash stops at the hat admission with 60.7 KB free (about 1 KB short), Banjo with 39.0; Peach's low-water is 41.9 KB. A special slot naming a file outside the content's closure now reads NULL unless another fighter of the match loaded it, as Remix's status-buffer lookup does; only a lab stand-in (the parent's file in place of a donor file) still loads whole (`OpenSpecialMask`). Banjo names Captain's Falcon Kick and car file (26 KB), which his closure never loads and nothing of his reads: "p4x" Banjo's setup 168.8 -> 142.7 KB. He and Crash still stop at the hat admission, with 64.0 and 59.8 KB free: Kirby's three hats in both details take about 38.5 KB, and the 88.5 KB frontend-overlay tail the largest-first preload fills first (Crash's low image 63.8 KB and Kirby's 24.3 KB) has 336 B left. Lanky's low-water is 51.6 KB, Sheik's 37.2. A battle of three or four fighters now loads each content's low-detail pack (`fighters/battle/<kind>.fpc`, `ndsP4LowDetailBattle`, `p4_preview_pack.py --battle-out`; "p4y"): the select pack less what only the high detail reads there -- the MObjs, matanims and textures of every joint whose low DObjDesc names its own list (the low detail takes a joint without one from the high rows: lbCommonSetupFighterPartsDObjs, ftParamSetModelPartDetailAll, the hidden parts) and of the high model-part rows -- with the slots naming them NULL; both JointTrees and the per-joint arrays stay whole. The select screen, one- and two-fighter battles and Results keep the select pack, and one manifest serves both. Packs: Crash 83.9 -> 53.5 KB, Bowser 83.9 -> 58.6, Banjo 78.9 -> 56.0, Lanky 62.0 -> 40.7, Ganondorf 60.5 -> 40.1, Dedede 59.2 -> 40.1, Sonic 58.1 -> 38.0, Peach 45.0 -> 31.2, Marth 53.9 -> 41.9, Roy 44.8 -> 33.0, Sheik 36.7 -> 26.3, Wario 45.5 -> 35.4, Falco 26.9 -> 21.8; Wolf's details share one tree. Beside Samus, Link and Kirby (600 frames, 0 native failures), low-water: Falco 63.1 KB, Wolf 62.3, Marth 55.3, Bowser 53.0, Peach 53.0, Ganondorf 50.2, Lanky 49.2, Roy 49.0, Sonic 49.0, Wario 47.0, Sheik 46.2, Crash 38.3, Dedede 38.2 -- thirteen of fourteen above the floor. Banjo passes the hat admission and ends at 19.8 KB: his match grows the AObj pool 6.5 KB in 150 frames (180 allocations, his Kazooie joints' animations), and the front-end tail is full before the admission (Kirby's two images, Link's high, 12.6 KB of other scene assets), so hats taken image by image from it gain nothing (measured, not kept). His own special3 (9.0 KB, a lab stand-in of 2.2 KB today) will cost a shipping build 6.8 KB more The shipping image with all fourteen (playtest "play-1008b") is 204 KB larger than the original cast's (.main +148 KB, .main.rw +12 KB, .main.bss +24 KB, the menu overlay +20 KB, of which newlib's floating-point formatter 18 KB for one `snprintf`, now `sniprintf`), and the VS select halted entering: its preview arenas (24 KiB shared, 80 KiB a slot) asked the heap for more than it had, and `syTaskmanMalloc` halts on overflow. The pool now asks `ndsSyMallocWouldFit` for the whole set and switches the previews off when it does not fit (the select opens with 304,936 B free). The owner-image binding is a table per image (`NDSImgBindDesc`) instead of each owner's inlined field assignments, and the P4 fighters' routine files are `-Os`: .main -21.5 KB. The P4 previews themselves need 100-150 KB a slot (pack plus high-detail mesh: Crash 148, Banjo 135, Bowser 117, against the original cast's 55 KB at most), so the select's previews need a budget per slot, not four 80 KiB arenas. |

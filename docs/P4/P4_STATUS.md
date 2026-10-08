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

Open for Falco: the 3D preview on the select screen (S9, in progress),
Kirby's copy (S8: Kirby takes the parent's copy, Fox's blaster and hat;
Remix gives Kirby Falco's Phantasm as statuses 0xEB/0xEC with Kirby's own
FALCO_NSP animations and scripts, and hat 0x12 from its extended Kirby file),
the acceptance probe's witness block (S10).

## Board 1 status (master plan Revision 3)

| Item | State |
|---|---|
| S1 content list | not started; Falco is wired by hand in shared renderer, Makefile and P4 files |
| S2 build throughput | not started; lab and menu builds still rewrite the shared linker script under a mutex |
| S3 table seams | 12 of 33 (Falco's) |
| S4 motion commands | 2 of the roster's 12 (FSM, TopN) |
| S5 routine work lists | counts measured (master plan table); per-routine lists not generated |
| S6 articles | first source scan only |
| S7 generated CPU rows | Falco's rows read by hand from the assembled tables |
| S8 Kirby copies | not started |
| S9 select-screen preview | pack generator, loader, residency and CSS wiring written for Falco; anim-cache reservation under test; uncommitted |
| S10 acceptance probe | lab probes exist per topic (`gNdsLabP4Content`); no single command |
| S11 whole-roster generation | not started; all 14 export cleanly |
| S12 EXTRA export | not started; EXTRA's nested Remix gitlink not initialized |
| S13 sword trails | not started; Falco's Phantasm draws without Remix's cyan trail |

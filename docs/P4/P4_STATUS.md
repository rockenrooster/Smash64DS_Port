# P4 status — Smash Remix fighters on the DS

Owner of the live P4 state. Plans live beside it ([master](New_Characters.md),
[handoff](IMPLEMENTATION_HANDOFF.md), [shared plans](shared/), [cards](characters/)).

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
| Ganondorf | Captain | 14 | 54 | 0 | 15 | 2 (vanilla-RAM script root) |
| Wolf | Fox | 125 | 52 | 10 | 97 | 0 |
| Wario | Mario | 59 | 58 | 8 | 56 | 0 |
| Bowser | Yoshi | 193 | 60 | 9 | 150 | 0 |
| Marth | Captain | 211 | 124 | 26 | 162 | 0 |
| Roy | Captain | 213 | 126 | 28 | 163 | 0 |
| Sheik | Captain | 165 | 75 | 21 | 132 | 2 (vanilla-RAM script root) |
| Peach | Fox | 201 | 106 | 13 | 154 | 1 (vanilla-RAM pointer) |
| Dedede | Captain | 227 | 128 | 38 | 163 | 0 |
| Sonic | Fox | 118 | 100 | 20 | 96 | 0 |
| Banjo | Captain | 206 | 89 | 19 | 162 | 2 (vanilla-RAM script root) |
| Crash | Mario | 201 | 99 | 13 | 160 | 0 |
| Lanky | Mario | 216 | 101 | 23 | 170 | 0 |

The vanilla-RAM roots (0x80391E84, a shared script in a BattleShip overlay)
need a symbol-mapped lowering before those fighters admit. Metal Mario is the
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

Open for Falco: CSS/UI/HUD/results/audio, AI (Remix CPU attacks, recovery and
post-process), Kirby copy (hat 0x12, Phantasm), costumes and shield colours,
runtime witnesses on the card.

#!/usr/bin/env python3
"""Every model-part a motion installs must have a native bake.

THE FAILURE THIS CATCHES. A motion runs `SetModelPartID(joint, part)`. The live
root vector changes. If the new root has no native bake the owner declines, and
at `NDS_RENDERER_PROFILE_LEVEL 0` a declined owner draws **nothing** -- so the
whole fighter disappears for the length of the move, not just the swapped part.

It has happened twice and both were found by a human noticing a fighter vanish,
months apart:

  * Kirby's swallow-copy was native-broken for ten of eleven victims
    (`artifacts/performance/2026-09-17_p2-2p8-roster-variance/`).
  * Yoshi's grab throw made the whole fighter vanish, because
    `P2_MODEL_PART_ROOT_VARIANTS` had no yoshi entry at all
    (`artifacts/performance/2026-09-17_p2-3f52-yoshi-throw-variant/`).

Nothing connected the source mutation to the bake, because they live in
different worlds: the mutation is an `ftMotionCommand` in decomp reloc data and
the bake is a `(binding, offset)` tuple in the generator. This closes that gap by
resolving one to the other.

THE RESOLUTION CHAIN, all of it parsed rather than assumed:

  motion file  `SetModelPartID(joint, part)` -- the mutations that exist
  Main.c       `modelparts_container[i]` selects the descriptor; the source
               joint is `i + 4`
  descriptor   `FTModelPart` rows are `modelparts[part][detail]`, so the row
               is `part * 2 + detail`
  Model.c      the display list symbol's own header comment gives its offset

A resolved offset must then appear in one of the four places a root can
legitimately be baked:

  * `BASE_MODEL_PART_ROOT_VARIANTS` -- the Mario/Fox base-pair variants, merged
                                      into the owner image by
                                      `build_native_owner_payload`
  * `P2_MODEL_PART_ROOT_VARIANTS`  -- independently admissible per-binding swap
  * `P2_ROOT_PROGRAM_APPENDIX`     -- baked, but only reachable through a
                                      complete root program
  * a foreign model file           -- a mixed-file program root, which by
                                      definition is not in this owner's model

Usage:
    python scripts/fighters/check_model_part_mutation_coverage.py

Exit 0 when every mutation resolves to a bake; 1 otherwise, naming the fighter,
the motion, the joint, the part and the offset that has no bake.
"""
from __future__ import annotations

import io
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import _paths  # noqa: E402
import generate_nds_native_owners as native  # noqa: E402

REPO = _paths.REPO_ROOT
RELOC = REPO / "decomp/BattleShip-main/decomp/src/relocData"
SCSUBSYS = REPO / "decomp/BattleShip-main/decomp/src/sc/scsubsys"
# The CSS clips declare their arrays `s32 D_ovl1_XXXXXXXX[]`, not with the
# reloc data's ftMotionCommand/u32 spelling, so ARR above cannot name them.
CSS_ARR = re.compile(r"^\s*(?:static\s+)?[su]32\s+(\w+)\[\]")
# ONLY THE ROWS A SHIPPING SCENE CAN ACTUALLY PLAY. Scanning every array in the
# file over-reports: rows 6 and up belong to the opening movie, the 1P stage
# cards and the trophy/doll sequences, none of which this build reaches.
#
# `ftMainSetStatus` maps a demo status straight onto a submotion row: statuses
# 0x10000..0x1000E take the identity `D_ovl1_80390BE8` table and
# `motion_id = status - 0x10000` (decomp ft/ftmain.c:4559-4562,4608-4610), so
# row N *is* demo status N.  That fixes the two scenes exactly:
#
#   rows 0..4  nFTDemoStatusNull + Win1..Win4 -- the character select, which
#              plays row 0 and ONE Win row `mnPlayersVSGetStatusSelected` picks
#   rows 1..3  Win1..Win3 -- the VS Results winner, `mnVSResultsGetStatusWin`
#              (decomp mn/mnvsmode/mnvsresults.c:869-883); Kirby draws from
#              Win1/Win2 only, every other fighter from Win1/Win2/Win3
#   row 5      nFTDemoStatusLose -- `mnVSResultsGetStatusLose` (:886-889) for
#              every loser AND for every present fighter on a No Contest
#              (:892-897), which is the clapping pose
#
# ROW 5 WAS EXCLUDED UNTIL 2026-09-21 and the exclusion was wrong in scope, not
# in fact: it was argued from Mario visibly drawing on the character select,
# which says nothing about the Results screen that actually plays it.  Mario's
# row-5 Claps clip resolves to his two alternate hands, which ARE baked -- but
# only in `BASE_MODEL_PART_ROOT_VARIANTS`, a table `baked_offsets()` did not
# read either, so the two errors cancelled and hid each other.
SUBMOTION_TABLE = re.compile(
    r"FTMotionDesc dFT\w+SubMotionDescs\[\]\s*=\s*\{(.*?)\n\};", re.S)
DEMO_REACHABLE_ROWS = 6
SUBMOTION_FIELDS_PER_ROW = 3
# The ONE Win row the character select plays per fighter --
# `mnPlayersVSGetStatusSelected` (decomp mn/mnplayers/mnplayersvs.c:1553-1578),
# and the demo status is the row. Naming it makes a failure say which screen to
# go look at: Ness's row-3 clip is Results-only, which is exactly why a
# CSS-scoped census could never have seen it.
CSS_SELECTED_ROW = {
    "fox": 4, "samus": 4,
    "donkey": 1, "luigi": 1, "link": 1, "captain": 1,
    "yoshi": 2, "purin": 2, "ness": 2,
    "mario": 3, "kirby": 3,
}
CSS_DEFAULT_SELECTED_ROW = 1
# `mnVSResultsGetStatusWin` picks Win1/Win2/Win3, Win1/Win2 for Kirby;
# `mnVSResultsGetStatusLose` returns row 5 for every loser and for every present
# fighter on a No Contest (decomp mn/mnvsmode/mnvsresults.c:869-897).
RESULTS_WIN_ROWS = (1, 2, 3)
RESULTS_KIRBY_WIN_ROWS = (1, 2)
RESULTS_LOSE_ROW = 5


def row_scenes(owner: str, row: int) -> str:
    """Which shipping screens play this fighter's submotion row."""
    scenes = []
    if row == 0:
        scenes.append("CSS idle")
    if row == CSS_SELECTED_ROW.get(owner, CSS_DEFAULT_SELECTED_ROW):
        scenes.append("CSS selected")
    win_rows = (RESULTS_KIRBY_WIN_ROWS if owner == "kirby"
                else RESULTS_WIN_ROWS)
    if row in win_rows:
        scenes.append("Results Win%d" % row)
    if row == RESULTS_LOSE_ROW:
        scenes.append("Results Lose / No Contest claps")
    return ", ".join(scenes) if scenes else "row %d" % row


def demo_reachable_clips(text: str) -> dict[str, set[int]]:
    """Clip symbol -> submotion rows, for the rows a shipping scene plays.

    PARSE POSITIONALLY. A row is three comma-separated fields and the clip is
    the second, but it is frequently a literal rather than a symbol -- Mario's
    row 3 is `&llFTMarioAnimSelectedFileID, 0x80000000, 0x00000000`. Matching
    only `&anim, &clip` pairs SKIPS such rows and silently shifts every later
    one up, which once pulled a row-5 clip into the row-4 slot.

    THE `&` IS OPTIONAL AND THAT MATTERED. Mario writes `&D_ovl1_80390DE4`;
    Ness, Link, Kirby, Pikachu and the rest write the bare array name, because
    an array decays to a pointer either way. Requiring the `&` accepted Mario's
    and Luigi's clips and silently returned the EMPTY SET for every other
    fighter, so this checker read their demo files, found their raw
    SetModelPartID words, and discarded every one of them as unreachable. Ness's
    Results Win3 clip installs joint 17 part 1 at 0x6d90 with no native bake;
    that is what the empty set was hiding.
    """
    table = SUBMOTION_TABLE.search(text)
    if table is None:
        return {}
    fields = [f.strip() for f in table.group(1).split(",")]
    clips: dict[str, set[int]] = {}
    for row in range(DEMO_REACHABLE_ROWS):
        index = row * SUBMOTION_FIELDS_PER_ROW + 1
        if index >= len(fields):
            break
        clip = fields[index].lstrip("&")
        # `0x80000000` is the source's "no event script" sentinel, not a clip.
        if re.match(r"^[A-Za-z_]\w*$", clip):
            clips.setdefault(clip, set()).add(row)
    return clips

CMD = re.compile(r"ftMotionCommandSetModelPartID\((\d+),\s*(-?\d+)\)")
# THE MACRO IS NOT THE ONLY SPELLING, and the other one is where the character
# select lives. `sc/scsubsys/scsubsysdata*.c` writes its demo and Win clips as
# RAW HEX -- Link has fifteen, Fox seven, Luigi and Ness three, Donkey and Mario
# two, Samus one -- and `CMD` above cannot see a single one of them. This
# checker and its hidden-part sibling both globbed only `*MainMotion.c` and both
# ran GREEN while the character select mutated model parts through a path
# neither parsed. Link draws there only because his in-match `Entry` program
# happens to carry the same (20, 0) / (11, -1) pair his demo clip needs.
#
# Decoded against the real bitfield, `FTMotionEventSetModelPartID` in
# decomp/.../src/ft/fttypes.h:455 -- `opcode:6, joint_id:7 SIGNED,
# modelpart_id:19 SIGNED`, packed MSB-first:
#
#   0xA0A00000 -> opcode 40, joint 20, part  0
#   0xA05FFFFF -> opcode 40, joint 11, part -1
#
# Opcode 43 (0xAC...) is a different command, SetTexturePartID, and it writes
# only `mobj->texture_id_curr`. It does NOT change the root vector, so it must
# not be swept up here -- that misread cost a whole Jigglypuff investigation.
RAW_HEX = re.compile(r"0[xX]([0-9a-fA-F]{8})")
MOTION_EVENT_SET_MODELPART_ID = 40


def decode_raw_modelpart(word: int):
    """(joint, part) when `word` is a raw SetModelPartID command, else None."""
    if (word >> 26) & 0x3F != MOTION_EVENT_SET_MODELPART_ID:
        return None
    joint = (word >> 19) & 0x7F
    if joint & 0x40:
        joint -= 0x80
    part = word & 0x7FFFF
    if part & 0x40000:
        part -= 0x80000
    return joint, part
# Motion arrays are declared BOTH ways in the reloc data --
# `ftMotionCommand dLinkMainMotion_Catch[]` but `u32 dLinkMainMotion_CatchPull[]`
# -- and matching only the first attributes every u32-declared motion's commands
# to whichever ftMotionCommand-declared motion preceded it. Here that only
# mislabels a failure message, because the check is per (joint, part, detail)
# rather than per motion, but a wrong motion name in a report costs real time.
ARR = re.compile(r"^(?:ftMotionCommand|u32) (d\w+)\[\]")
# Region arms are deliberately NOT filtered here, unlike in
# check_hidden_part_root_coverage.py. This check asks only whether a bake
# exists, so reading the REGION_JP arm as well checks strictly more, and the
# ROM builds -DREGION_US: the worst case is a false red for a part the US build
# never installs, never a false green. There is where it matters, because
# absorbing a JP command into a US motion computes a live root vector no build
# ever reaches.
CONTAINER = re.compile(
    r"FTModelPartDesc \*d\w+?Main_modelparts_container\[\d+\] = \{(.*?)\};",
    re.S)
DESC = re.compile(
    r"FTModelPart d\w+?Main_modelparts_desc_(0x[0-9A-Fa-f]+)\[\d+\] = "
    r"\{(.*?)\n\};", re.S)
GFXROW = re.compile(r"\{\s*\(Gfx\*\)&?(\w+)")
DLHDR = re.compile(r"/\*[^*]*@ (0x[0-9A-Fa-f]+)[^*]*\*/\s*\nGfx (\w+)\[")

# A model part whose source display list lives in ANOTHER fighter's model file.
# These are mixed-file program roots and cannot resolve in this owner's model,
# so the resolver reporting "unresolved" for them is correct, not a miss.
FOREIGN_PROGRAM_ROOTS = {
    # Link's boomerang hand. dLinkMainMotion_MissingBoomerang installs joint 11
    # part 1, whose root is source-owned by LinkBoomerangModel and carried by
    # OWNER_ROOT_PROGRAMS["link"] "SpecialN" as a mixed-file program.
    ("link", 11, 1),
}


def baked_offsets() -> dict[str, dict[str, set[int]]]:
    """Offsets a root may legitimately resolve to, per owner and detail.

    BASE_MODEL_PART_ROOT_VARIANTS IS ONE OF THEM. Mario's and Fox's alternate
    hands live only there -- `build_native_owner_payload` merges that table into
    the base pair's owner image the same way the P2 tables are merged for the
    admitted fighters -- and omitting it here made this checker report a baked
    root as unbaked the moment Mario's Results Lose row was scanned.
    """
    out: dict[str, dict[str, set[int]]] = {}
    for table in (native.BASE_MODEL_PART_ROOT_VARIANTS,
                  native.P2_MODEL_PART_ROOT_VARIANTS,
                  native.P2_ROOT_PROGRAM_APPENDIX):
        for owner, details in table.items():
            for detail, rows in details.items():
                out.setdefault(owner, {}).setdefault(detail, set()).update(
                    offset for _binding, offset in rows)
    return out


def model_offsets(fighter_cap: str) -> dict[str, int]:
    # Anchor on the numeric prefix. "*YoshiModel.c" also matches
    # "304_NYoshiModel.c" -- the POLYGON Yoshi, a different model with
    # different symbols, which silently resolves every offset to nothing.
    for path in sorted(RELOC.glob("[0-9]*_%sModel.c" % fighter_cap)):
        text = path.read_text(encoding="utf-8", errors="replace")
        return {symbol: int(address, 16)
                for address, symbol in DLHDR.findall(text)}
    return {}


def main() -> int:
    baked = baked_offsets()
    failures: list[str] = []
    resolved = 0
    foreign = 0
    checked_fighters: list[str] = []

    for motion_path in sorted(RELOC.glob("*MainMotion.c")):
        cap = re.sub(r"^\d+_", "", motion_path.name).replace(
            "MainMotion.c", "")
        owner = cap.lower()
        text = motion_path.read_text(encoding="utf-8", errors="replace")
        mutations: list[tuple[str, int, int]] = []
        current = "?"
        for line in text.splitlines():
            match = ARR.match(line)
            if match:
                current = match.group(1)
                continue
            for joint, part in CMD.findall(line):
                # part 0 restores canonical; a negative or huge value is a
                # hidden-part mask rather than a model-part id.
                if 0 < int(part) < 64:
                    mutations.append((current, int(joint), int(part)))
        # AND THE DEMO CLIPS THE CHARACTER SELECT AND VS RESULTS PLAY. They live
        # in a different tree and in raw hex, so neither the glob above nor CMD
        # can reach them -- which is exactly how this checker ran GREEN while
        # those two screens mutated model parts unchecked. Same coverage rule,
        # same failure message; only the scene name in the label differs.
        demo_path = SCSUBSYS / ("scsubsysdata%s.c" % owner)
        if demo_path.is_file():
            demo_text = demo_path.read_text(encoding="utf-8", errors="replace")
            reachable = demo_reachable_clips(demo_text)
            demo_current = "?"
            for line in demo_text.splitlines():
                demo_match = CSS_ARR.match(line)
                if demo_match:
                    demo_current = demo_match.group(1)
                    continue
                for word in RAW_HEX.findall(line):
                    decoded = decode_raw_modelpart(int(word, 16))
                    if decoded is None:
                        continue
                    joint, part = decoded
                    # Same rule as the macro arm: part 0 restores canonical and
                    # a negative is a hidden-part mask, not a model-part id.
                    if 0 < part < 64 and demo_current in reachable:
                        scenes = ", ".join(
                            row_scenes(owner, row)
                            for row in sorted(reachable[demo_current]))
                        mutations.append(
                            ("demo %s [%s]" % (demo_current, scenes),
                             joint, part))

        if not mutations:
            continue
        checked_fighters.append(owner)

        main_path = next(RELOC.glob("[0-9]*_%sMain.c" % cap), None)
        if main_path is None:
            failures.append(f"{owner}: no Main.c to resolve its mutations")
            continue
        main_text = main_path.read_text(encoding="utf-8", errors="replace")
        container = CONTAINER.search(main_text)
        if container is None:
            failures.append(f"{owner}: no modelparts_container in Main.c")
            continue
        entries = [entry.strip()
                   for entry in container.group(1).replace("\n", " ").split(",")
                   if entry.strip()]
        descriptors = {
            "desc_%s" % tag.lower(): GFXROW.findall(body)
            for tag, body in DESC.findall(main_text)
        }
        offsets = model_offsets(cap)

        for motion, joint, part in sorted(set(mutations)):
            if (owner, joint, part) in FOREIGN_PROGRAM_ROOTS:
                foreign += 1
                continue
            index = joint - 4
            entry = entries[index] if 0 <= index < len(entries) else ""
            tag_match = re.search(r"modelparts_desc_(0x[0-9A-Fa-f]+)", entry)
            symbols = descriptors.get(
                "desc_%s" % tag_match.group(1).lower() if tag_match else "", [])
            for detail_index, detail in enumerate(("high", "low")):
                row = part * 2 + detail_index
                symbol = symbols[row] if row < len(symbols) else None
                offset = offsets.get(symbol) if symbol else None
                if offset is None:
                    failures.append(
                        f"{owner} {motion}: joint {joint} part {part} "
                        f"{detail} does not resolve to a display list "
                        f"(descriptor row {row} of {len(symbols)}) -- if this "
                        f"is a mixed-file program root, add it to "
                        f"FOREIGN_PROGRAM_ROOTS with its program")
                    continue
                if offset not in baked.get(owner, {}).get(detail, set()):
                    failures.append(
                        f"{owner} {motion}: joint {joint} part {part} "
                        f"{detail} is 0x{offset:x}, which has NO native bake -- "
                        f"this move makes the whole fighter stop drawing")
                    continue
                resolved += 1

    print(f"  model-part mutation coverage: {resolved} (joint, part, detail) "
          f"resolutions baked, {foreign} foreign program roots skipped, "
          f"{len(checked_fighters)} fighters with mutations")
    if failures:
        print("MODEL_PART_MUTATION_COVERAGE_FAIL")
        for failure in failures:
            print(f"  {failure}")
        return 1
    print("MODEL_PART_MUTATION_COVERAGE_OK every model part a motion installs "
          "has a native bake")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

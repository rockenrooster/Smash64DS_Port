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

A resolved offset must then appear in one of the three places a root can
legitimately be baked:

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

CMD = re.compile(r"ftMotionCommandSetModelPartID\((\d+),\s*(-?\d+)\)")
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
    """Offsets a root may legitimately resolve to, per owner and detail."""
    out: dict[str, dict[str, set[int]]] = {}
    for table in (native.P2_MODEL_PART_ROOT_VARIANTS,
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

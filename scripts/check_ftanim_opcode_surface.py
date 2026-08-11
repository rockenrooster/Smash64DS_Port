#!/usr/bin/env python3
"""Assert the figatree opcode surface is CLOSED and fully handled.

This is the prerequisite for slice 32, the AOT dense-track animation rewrite. A
generator that bakes animation tracks at build time can only be correct if it
covers every opcode the runtime parser covers -- and can only be PROVEN correct
if that set is finite and known. It is: 15 opcodes, all of them handled.

Two directions, and both matter:

  * An opcode DEFINED in decomp but NOT handled by
    `ndsR2FtAnimParseDObjFigatree` means the runtime silently ignores something
    the data can contain, so a generator that reproduces the runtime would
    inherit the same hole.
  * An opcode HANDLED by the port but NOT defined in decomp means the port has
    invented a case, which is either dead code or a name drift.

Either way the AOT generator's assumption -- "I have seen every command this
format can express" -- is broken, and it breaks silently, in generated data that
looks fine. This checker is what makes it break loudly instead.

It is a source-level check on purpose. The alternative, enumerating opcodes
found in the shipped animation data, would pass happily while the format still
allowed a case nobody had exercised yet; cycle 117 already lost a day to a
checker that could pass vacuously, so this one compares DECLARED surfaces.

`decomp/` is read-only reference and gitignored, so this reads it without
touching it and reports clearly when the tree is absent rather than pretending
to pass.
"""

from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
PARSER = ROOT / "src" / "import" / "battleship_ftanim.c"
DECOMP = ROOT / "decomp" / "BattleShip-main" / "decomp"
OPCODE = re.compile(r"\bnGCAnimEvent16\w+\b")


def parser_handled() -> set[str]:
    """Opcodes with a `case` label inside ndsR2FtAnimParseDObjFigatree."""
    text = PARSER.read_text(encoding="utf-8", errors="replace")
    start = text.find("void ndsR2FtAnimParseDObjFigatree")
    if start < 0:
        sys.exit("ndsR2FtAnimParseDObjFigatree not found in %s" % PARSER)
    end = text.find("\n}", start)
    body = text[start:end]
    return set(re.findall(r"case\s+(nGCAnimEvent16\w+)\s*:", body))


def decomp_defined() -> set[str]:
    """Every opcode name that appears anywhere in the reference tree."""
    found: set[str] = set()
    for path in DECOMP.rglob("*"):
        if path.suffix.lower() not in (".c", ".h", ".cpp", ".hpp"):
            continue
        try:
            found |= set(OPCODE.findall(
                path.read_text(encoding="utf-8", errors="replace")))
        except OSError:
            continue
    return found


def main() -> int:
    if not DECOMP.is_dir():
        print("SKIP: %s absent -- run scripts/fetch-battleship-reference.ps1"
              % DECOMP)
        return 0

    handled = parser_handled()
    defined = decomp_defined()
    if not defined:
        print("FAIL: no nGCAnimEvent16* names found in the reference tree; "
              "the search is broken, not the surface")
        return 1

    unhandled = sorted(defined - handled)
    invented = sorted(handled - defined)

    print("opcodes defined in decomp : %d" % len(defined))
    print("opcodes handled by parser : %d" % len(handled))
    if unhandled:
        print("DEFINED BUT NOT HANDLED (%d):" % len(unhandled))
        for name in unhandled:
            print("   ", name)
    if invented:
        print("HANDLED BUT NOT DEFINED (%d):" % len(invented))
        for name in invented:
            print("   ", name)
    if unhandled or invented:
        print("FAIL: the figatree opcode surface is not closed. An AOT "
              "generator cannot claim to cover a format it has not enumerated "
              "-- reconcile before baking tracks.")
        return 1

    print("FTANIM_OPCODE_SURFACE=CLOSED  all %d opcodes defined in the "
          "reference are handled by the runtime parser, and the parser invents "
          "none -- an AOT generator has a finite, known surface to cover"
          % len(defined))
    return 0


if __name__ == "__main__":
    sys.exit(main())

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


CASE_RE = re.compile(r"case\s+(nGCAnimEvent16\w+)\s*:")


def parser_cases() -> "list[tuple[list[str], str]]":
    """Every case group in the parser's switch, with its body.

    Depth is tracked from the SWITCH's own brace, not the function's, and a case
    ends only at a `break;`/`return` at that depth -- the flags loops inside
    several cases contain their own `break`, and an earlier version of this
    extraction ended each case there and reported "(none)" for opcodes that
    write seven fields.
    """
    text = PARSER.read_text(encoding="utf-8", errors="replace")
    start = text.find("void ndsR2FtAnimParseDObjFigatree")
    lines = text[start:text.find("\n}\n", start)].split("\n")
    sw = next(i for i, l in enumerate(lines) if "switch (command_kind)" in l)
    groups: list = []
    cur = None
    depth = 0
    for line in lines[sw + 1:]:
        s = line.strip()
        if depth == 1:
            m = CASE_RE.match(s)
            if m:
                if cur is None or cur[1]:
                    cur = ([], [])
                    groups.append(cur)
                cur[0].append(m.group(1))
                depth += line.count("{") - line.count("}")
                continue
        if cur is not None:
            cur[1].append(s)
            if depth == 1 and (s == "break;" or s.startswith("return")):
                cur = None
        depth += line.count("{") - line.count("}")
        if depth <= 0:
            break
    return [(labels, "\n".join(body)) for labels, body in groups]


def check_effect_surface() -> int:
    """Assert WHERE the parser escapes its own AObj state.

    The cycle-117 dense-track design has four parts precisely because the script
    does two things beyond writing AObj fields, and both were found by reading
    all fifteen bodies rather than by assumption:

      * `Loop` and `End` call `parent_gobj->func_anim` -- a GObj callback into
        GAMEPLAY code. A baked track must fire those at the same times, which is
        what makes this bake a gameplay change rather than a presentation one.
      * `SetFlags` writes `root_dobj->flags`, a DObj field rather than an AObj
        one.

    If a new callback site or DObj write appears in any other case, the baked
    format is silently incomplete -- it would reproduce every AObj value
    correctly and still diverge. That failure is invisible in field-by-field
    comparison, so it is guarded here instead.
    """
    expected_callback = {"nGCAnimEvent16Loop", "nGCAnimEvent16End"}
    expected_dobj_write = {"nGCAnimEvent16SetFlags"}
    callback: set = set()
    dobj_write: set = set()

    for labels, body in parser_cases():
        if "func_anim(" in body:
            callback |= set(labels)
        if re.search(r"root_dobj->flags\s*=[^=]", body):
            dobj_write |= set(labels)

    ok = True
    if callback != expected_callback:
        ok = False
        print("FAIL: gameplay-callback sites changed.")
        print("   expected:", sorted(expected_callback))
        print("   found   :", sorted(callback))
    if dobj_write != expected_dobj_write:
        ok = False
        print("FAIL: DObj-flag write sites changed.")
        print("   expected:", sorted(expected_dobj_write))
        print("   found   :", sorted(dobj_write))
    if not ok:
        print("The AOT dense-track format assumes exactly these escape points. "
              "A new one makes the baked control stream incomplete in a way "
              "field-by-field comparison cannot see -- update the format and "
              "this list together.")
        return 1
    print("FTANIM_EFFECT_SURFACE=STABLE  func_anim called only from {}, "
          "root_dobj->flags written only by {}".format(
              "+".join(sorted(expected_callback)),
              "+".join(sorted(expected_dobj_write))))
    return 0


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
    return check_effect_surface()


if __name__ == "__main__":
    sys.exit(main())

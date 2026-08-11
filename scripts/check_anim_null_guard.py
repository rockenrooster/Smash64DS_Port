#!/usr/bin/env python3
"""Assert every per-joint animation entry point is a TOTAL no-op when
`anim_wait == AOBJ_ANIM_NULL`.

Slice 33. `ftParamUpdateAnimKeys` calls two functions per joint per frame -- one
parser and one player -- and 31.5% of those joints have `anim_wait ==
AOBJ_ANIM_NULL`, so both calls set up a stack frame, save nine registers,
compare one word, and return. Measured off the shipped Thumb: 24 instructions
and 21 memory word accesses for the parser, 21 and 19 for the player. A
caller-side predicate deletes both for three instructions.

That deletion is only equivalent if the guard encloses the WHOLE body of every
function reachable from those two call sites. It does today, in all five, but
"it does today" is exactly the kind of premise that rots: someone adds a counter,
a cache invalidation, or an early frame-advance above the `if`, and the predicate
silently starts skipping it. The failure is invisible -- the animation still
plays, one joint's bookkeeping just stops happening on the frames it goes idle.

So the predicate does not rest on a reading. It rests on this checker.

Two of the five bodies live in `decomp/`, which is read-only reference and
gitignored; this reads them without touching them and SKIPS clearly rather than
passing vacuously when the tree is absent.

Instrumentation counters are the one allowed exception, listed explicitly in
`ALLOWED_BEFORE` so that an unrecognised statement above the guard fails.
"""

from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
DECOMP = ROOT / "decomp" / "BattleShip-main" / "decomp"

# (path, function, why it is on the hot per-joint path)
TARGETS = [
    (DECOMP / "src" / "sys" / "objanim.c", "gcParseDObjAnimJoint",
     "ftParamUpdateAnimKeys parse arm when is_anim_joint"),
    (DECOMP / "src" / "ft" / "ftanim.c", "ftAnimParseDObjFigatree",
     "ftParamUpdateAnimKeys parse arm otherwise (decomp body)"),
    (ROOT / "src" / "import" / "battleship_ftanim.c",
     "ndsR2FtAnimParseDObjFigatree",
     "the shipped port parser the shim selects"),
    (ROOT / "src" / "import" / "battleship_sys_objanim.c",
     "gcPlayDObjAnimJoint", "ftParamUpdateAnimKeys play arm"),
    (DECOMP / "src" / "lb" / "lbcommon.c",
     "lbCommonPlayTranslateScaledDObjAnim",
     "ftParamUpdateAnimKeys play arm when translate_scales is set"),
]

# Statements permitted above the guard. Each is instrumentation with no
# gameplay effect. A caller-side skip makes these count fewer calls, which is
# why `ftParamUpdateAnimKeys` counts its own skips -- calls + skips is the
# figure that stayed comparable across the change.
ALLOWED_BEFORE = [
    re.compile(r"^gNdsR2FtAnimParse\w+\+\+$"),
]

# Macros permitted in an initialiser above the guard. These read a `volatile`
# route word and nothing else, so a skipped call simply does not read it. A
# real function call is still rejected: it could have an effect the skip drops.
ALLOWED_INIT = re.compile(r"^[\w \t\*]+=\s*(?:NDS_R2_ANIM_CUT_ON\s*\([^()]*\)"
                          r"|[^()]*)\s*(?:\?[^()]*)?$")

GUARD = re.compile(
    r"if\s*\(\s*(?:NDS_FCMP_NE_C\s*\(\s*)?"
    r"(?:root_dobj|dobj)\s*->\s*anim_wait\s*(?:,|!=)\s*AOBJ_ANIM_NULL\s*\)")


def function_body(path: pathlib.Path, name: str):
    """The text between the function's opening and closing brace."""
    text = path.read_text(encoding="utf-8", errors="replace")
    m = re.search(r"^[A-Za-z_][\w \t\*]*\b%s\s*\(" % re.escape(name), text,
                  re.MULTILINE)
    if not m:
        return None
    open_brace = text.find("{", m.end())
    if open_brace < 0:
        return None
    depth = 0
    for i in range(open_brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[open_brace + 1:i]
    return None


def strip_noise(body: str) -> str:
    """Remove comments and preprocessor lines -- neither executes."""
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    body = re.sub(r"//[^\n]*", "", body)
    return "\n".join(l for l in body.split("\n")
                     if not l.lstrip().startswith("#"))


def is_declaration(stmt: str) -> bool:
    """A local declaration, with or without an initialiser.

    Initialisers are allowed only when they cannot escape the function: a
    call on the right-hand side could have a side effect the caller-side skip
    would drop, so it is rejected.
    """
    if not re.match(r"^(?:const\s+|static\s+|volatile\s+|register\s+)*"
                    r"[A-Za-z_]\w*(?:\s*\*)*\s+\*?\w+", stmt):
        return False
    if "=" in stmt and re.search(r"=[^=]*\w\s*\(", stmt):
        return bool(ALLOWED_INIT.match(stmt))   # only the listed route macros
    return True


def split_statements(head: str):
    """Statements above the guard, semicolon-separated at brace depth 0."""
    out, cur, depth = [], [], 0
    for ch in head:
        if ch in "{(":
            depth += 1
        elif ch in "})":
            depth -= 1
        if ch == ";" and depth == 0:
            out.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    tail = "".join(cur).strip()
    if tail:
        out.append(tail + "  <-- unterminated")
    return [s for s in out if s]


def check(path: pathlib.Path, name: str, why: str) -> int:
    body = function_body(path, name)
    if body is None:
        print("FAIL: %s not found in %s" % (name, path))
        return 1
    body = strip_noise(body)

    g = GUARD.search(body)
    if not g:
        print("FAIL: %s has no `anim_wait != AOBJ_ANIM_NULL` guard at all.\n"
              "      The caller-side skip in ftParamUpdateAnimKeys assumes one."
              % name)
        return 1

    # Everything before the guard must be a declaration or an allowed counter.
    bad = []
    for stmt in split_statements(body[:g.start()]):
        one = " ".join(stmt.split())
        if is_declaration(one):
            continue
        if any(p.match(one) for p in ALLOWED_BEFORE):
            continue
        bad.append(one)

    # Everything after the guarded block must be nothing at all.
    open_brace = body.find("{", g.end())
    if open_brace < 0:
        print("FAIL: %s guards a single statement, not a block; this checker "
              "cannot bound it." % name)
        return 1
    depth, close = 0, None
    for i in range(open_brace, len(body)):
        if body[i] == "{":
            depth += 1
        elif body[i] == "}":
            depth -= 1
            if depth == 0:
                close = i
                break
    trailing = [s for s in split_statements(body[close + 1:])] if close else []

    if bad or trailing:
        print("FAIL: %s does work outside its AOBJ_ANIM_NULL guard." % name)
        for s in bad:
            print("   before guard:", s)
        for s in trailing:
            print("   after guard :", s)
        print("   A caller-side `anim_wait != AOBJ_ANIM_NULL` skip would drop "
              "that work. Either move it inside the guard or drop the skip.")
        return 1

    print("  ok  %-34s %s" % (name, why))
    return 0


def main() -> int:
    if not DECOMP.is_dir():
        print("SKIP: %s absent -- run scripts/fetch-battleship-reference.ps1"
              % DECOMP)
        return 0

    rc = 0
    for path, name, why in TARGETS:
        rc |= check(path, name, why)
    if rc:
        return rc
    print("ANIM_NULL_GUARD=TOTAL  all %d per-joint animation entry points are "
          "no-ops when anim_wait == AOBJ_ANIM_NULL, so ftParamUpdateAnimKeys "
          "may skip the parse and the play together" % len(TARGETS))
    return 0


if __name__ == "__main__":
    sys.exit(main())

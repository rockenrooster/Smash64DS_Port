#!/usr/bin/env python3
"""Assert the port's compatibility headers never contradict the decomp source.

`INCLUDES := include $(BATTLESHIP_DECOMP)/src ...` puts the port's `include/`
ahead of the decomp tree, and the decomp headers include their own siblings with
angle brackets (`#include <gm/generic.h>`), so for the 40-odd header names that
exist in both trees the port copy wins the race -- in decomp translation units
too.  Most of those port headers are deliberate DS *replacements* (sys/rdp.h,
sys/video.h: the DS has no RDP).  Some are *mirrors*: they restate enumerators
and object-like macros that a decomp header already defines, so a unit reaching
both gets a wall of redeclarations, and -- much worse -- a unit reaching only
one gets whichever value that copy happens to carry.

The invariant this enforces is the one that actually matters:

    where the port and the decomp source both name a constant, they must agree
    on its value.

A disagreement is a real defect: the same expression compiles to two different
numbers depending on include order, which is exactly the class docs/BUGS.md
keeps logging.  Duplication alone is not failure -- it is reported so the count
is visible and so a new mirror is noticed when it appears.

Usage:
    python scripts/check-decomp-header-mirror.py [--list-mirrors]

Exit status is non-zero on any value disagreement.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PORT_INCLUDE = ROOT / "include"
DECOMP_SRC = ROOT / "decomp" / "BattleShip-main" / "decomp" / "src"

ENUM_RE = re.compile(r"\benum\b(?:\s+([A-Za-z_]\w*))?\s*\{")
DEFINE_RE = re.compile(r"^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)[ \t]+(.+?)[ \t]*$", re.M)


def blank_comments(text: str) -> str:
    """Replace comment bodies with spaces so offsets still map to line numbers."""
    def sub(m: re.Match) -> str:
        return re.sub(r"[^\n]", " ", m.group(0))

    text = re.sub(r"/\*.*?\*/", sub, text, flags=re.S)
    return re.sub(r"//[^\n]*", sub, text)


def literal(tok: str) -> int | None:
    """Integer literal, with its u/U/l/L suffix -- and nothing else.

    The suffix strip must never touch an identifier: `rstrip("uUlL")` turns
    nFTCommonStatusLandingAirNull into nFTCommonStatusLandingAirN, which is a
    real and *different* enumerator five slots earlier, so an earlier revision
    of this script silently mis-evaluated the alias and reported a phantom drift.
    """
    try:
        return int(tok, 0)
    except ValueError:
        pass
    m = re.fullmatch(r"(0[xX][0-9a-fA-F]+|\d+)[uUlL]+", tok)
    return int(m.group(1), 0) if m else None


def resolve(expr: str, seen: dict) -> int | None:
    """Evaluate the integer-constant subset that actually appears in these trees."""
    expr = expr.strip()
    if expr.startswith("(") and expr.endswith(")"):
        inner = expr[1:-1]
        if inner.count("(") == inner.count(")"):
            expr = inner.strip()
    val = literal(expr)
    if val is not None:
        return val
    if isinstance(seen.get(expr), int):
        return seen[expr]
    m = re.fullmatch(r"([A-Za-z_]\w*)\s*([+-])\s*(\d+)", expr)
    if m and isinstance(seen.get(m.group(1)), int):
        base = seen[m.group(1)]
        return base + int(m.group(3)) if m.group(2) == "+" else base - int(m.group(3))
    m = re.fullmatch(r"1\s*<<\s*([A-Za-z_]\w*|\d+)", expr)
    if m:
        shift = m.group(1)
        if shift.isdigit():
            return 1 << int(shift)
        if isinstance(seen.get(shift), int):
            return 1 << seen[shift]
    return None


def scan(path: Path, unresolved_blocks: list) -> dict[str, tuple[int, str, int]]:
    """name -> (value, relative path, line).  Enumerators and object-like macros."""
    try:
        src = blank_comments(path.read_text(encoding="utf-8", errors="replace"))
    except OSError:
        return {}
    rel = path.relative_to(ROOT).as_posix()
    out: dict[str, tuple[int, str, int]] = {}
    scope: dict[str, int] = {}

    for m in ENUM_RE.finditer(src):
        depth, i = 1, m.end()
        while i < len(src) and depth:
            if src[i] == "{":
                depth += 1
            elif src[i] == "}":
                depth -= 1
            i += 1
        line = src.count("\n", 0, m.start()) + 1

        # All-or-nothing per block.  An enumerator this evaluator cannot fold
        # leaves every implicit enumerator after it without an anchor, so
        # emitting a partial block would publish values that are simply wrong --
        # which is how an earlier revision of this script reported a phantom
        # 5-off drift in FTCommonStatus, an enum that is in fact identical in
        # both trees.  Drop the whole block instead and count it.
        pending: list[tuple[str, int]] = []
        local = dict(scope)
        nxt: int | None = 0
        usable = True
        for item in src[m.end():i - 1].split(","):
            item = item.strip()
            if not item:
                continue
            if "=" in item:
                lhs, rhs = item.split("=", 1)
                key = lhs.strip()
                val = resolve(rhs, local)
            else:
                key, val = item, nxt
            if val is None or not re.fullmatch(r"[A-Za-z_]\w*", key):
                usable = False
                break
            nxt = val + 1
            local[key] = val
            pending.append((key, val))
        if not usable:
            unresolved_blocks.append((rel, line))
            continue
        for key, val in pending:
            scope[key] = val
            out[key] = (val, rel, line)

    for m in DEFINE_RE.finditer(src):
        key, body = m.group(1), m.group(2)
        if "(" in key or body.endswith("\\"):
            continue  # function-like or continued: not an integer constant
        val = resolve(body, scope)
        if val is None:
            continue
        line = src.count("\n", 0, m.start()) + 1
        scope.setdefault(key, val)
        out.setdefault(key, (val, rel, line))

    return out


def collect(root: Path) -> tuple[dict[str, tuple[int, str, int]], list]:
    merged: dict[str, tuple[int, str, int]] = {}
    unresolved: list = []
    for path in sorted(root.rglob("*.h")):
        for key, entry in scan(path, unresolved).items():
            merged.setdefault(key, entry)
    return merged, unresolved


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--list-mirrors", action="store_true",
                    help="list the port headers that restate decomp constants")
    args = ap.parse_args()

    if not DECOMP_SRC.is_dir():
        print(f"decomp tree absent at {DECOMP_SRC}; nothing to check")
        return 0

    port, port_skipped = collect(PORT_INCLUDE)
    decomp, decomp_skipped = collect(DECOMP_SRC)
    shared = sorted(set(port) & set(decomp))

    disagree = [(k, port[k], decomp[k]) for k in shared if port[k][0] != decomp[k][0]]

    print(f"port constants {len(port)}, decomp constants {len(decomp)}, "
          f"shared names {len(shared)}, disagreements {len(disagree)}")
    print(f"enum blocks skipped as unfoldable: port {len(port_skipped)}, "
          f"decomp {len(decomp_skipped)} (their names are not checked)")

    if args.list_mirrors:
        by_file: dict[tuple[str, str], int] = {}
        for k in shared:
            by_file[(port[k][1], decomp[k][1])] = by_file.get((port[k][1], decomp[k][1]), 0) + 1
        print("\nmirrored constants by (port header, decomp header):")
        for (p, d), n in sorted(by_file.items(), key=lambda kv: -kv[1]):
            print(f"  {n:5d}  {p}  <-  {d}")

    if disagree:
        print("\nFAIL: the port and the decomp source disagree on these values.")
        print("The same name compiles to different numbers depending on include order.")
        for k, (pv, pf, pl), (dv, df, dl) in disagree:
            print(f"  {k}: {pf}:{pl} = {pv}   vs   {df}:{dl} = {dv}")
        return 1

    print("OK: every shared name agrees.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

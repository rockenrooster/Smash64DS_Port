#!/usr/bin/env python3
"""Census of the reloc symbols the imported BattleShip sources reference.

Every ``src/import/battleship_*.c`` that textually includes a decomp source
(``#include ".../decomp/BattleShip-main/decomp/src/<path>.c"`` or the
build's ``<battleship_overlay/src/<path>.c>`` copy) inherits that source's
``ll*`` reloc symbols -- file ids and offsets the DS runtime resolves through
``include/reloc_data.h``.  A symbol nobody rows is a link error the code-first
mode cannot see, and until 2026-09-05 the only record of them was a hand
count in each TU's header comment, which drifted (three TUs claimed ~90
unstaged rows that were staged; one claimed none and had six).

This prints, per TU, every referenced ``ll*`` symbol that no port file
defines: no ``X(... llName ...)`` row in ``include/reloc_data.h`` or a
generated header, no ``uintptr_t llName`` global, extern or static anywhere
under ``src/`` or ``include/``, no ``#define llName NDS_RELOC_LVALUE(...)``,
and no ``#define llName llOther`` rename in the including TU.
``llRelocFileCount`` is a linker constant and is skipped.  The source is read
the way the build compiles it: after its import-overlay patch
(``scripts/import-overlays/battleship/``) when one exists, and with the
``REGION_JP`` and ``!SSB64_TARGET_NDS`` conditional blocks dropped (the port
is a REGION_US, SSB64_TARGET_NDS build).

Usage:
    python scripts/menus/check_reloc_symbol_census.py            # report
    python scripts/menus/check_reloc_symbol_census.py --strict   # exit 1 if any
    python scripts/menus/check_reloc_symbol_census.py --tu battleship_grbonus3.c
    python scripts/menus/check_reloc_symbol_census.py --flag NDS_P2_1P_GAME

The report is the staging worklist for ``scripts/menus/stage_reloc_file.py``;
the map-file pattern is described in that script's docstring.
"""
from __future__ import annotations

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile

LL_RE = re.compile(r"\bll[A-Z][A-Za-z0-9_]*")
INC_RE = re.compile(
    r'#include\s+["<](?:[^">]*decomp/BattleShip-main/decomp/src/|battleship_overlay/src/)'
    r'([^">]+\.c)[">]'
)
GLOBAL_RE = re.compile(
    r"^\s*(?:(?:extern|static|const|volatile)\s+)*uintptr_t\s+(ll[A-Za-z0-9_]+)\s*[;=\[]",
    re.M,
)
LVALUE_RE = re.compile(r"#define\s+(ll[A-Za-z0-9_]+)\s+NDS_RELOC_LVALUE")
RENAME_RE = re.compile(r"#define\s+(ll[A-Za-z0-9_]+)\s+ll[A-Za-z0-9_]+")
SKIP = {"llRelocFileCount"}

# What the build defines.  A conditional on anything else keeps both arms.
DEFINED = {"REGION_US": True, "SSB64_TARGET_NDS": True, "REGION_JP": False}
COND_RE = re.compile(
    r"^\s*#\s*(?:(?P<ifdef>ifdef|ifndef)\s+(?P<name1>\w+)"
    r"|if\s+(?P<neg>!?)\s*defined\s*\(?\s*(?P<name2>\w+)\s*\)?)\s*(?://.*|/\*.*)?$"
)
IF_RE = re.compile(r"^\s*#\s*if")
ELSE_RE = re.compile(r"^\s*#\s*else\b")
ENDIF_RE = re.compile(r"^\s*#\s*endif\b")

# Symbols a TU references only from a source function the port never links
# (gc-sections drops it before the reference is resolved).  Keyed by TU.
ALLOW = {
    # mnTitleMakeSlash (mntitle.c:1293) is masked out of the title TU's
    # function table (battleship_mntitle.c, bit 3); nothing else names them.
    "battleship_mntitle.c": {"llMNTitleUnknownAnimJoint", "llMNTitleUnknownDObjDesc"},
}


def read(path: str) -> str:
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.read()


def as_built(text: str) -> str:
    """Drop the lines the port build never compiles.  Each conditional pushes
    (decided, dropping): a REGION_* / SSB64_TARGET_NDS test is decided from
    DEFINED and its false arm dropped; any other test keeps both arms.
    Nesting is tracked so the matching #endif is found."""
    out: list[str] = []
    stack: list[list] = []
    for line in text.splitlines():
        m = COND_RE.match(line)
        if m:
            name = m.group("name1") or m.group("name2")
            if name in DEFINED:
                value = DEFINED[name]
                if m.group("ifdef") == "ifndef" or m.group("neg") == "!":
                    value = not value
                stack.append([True, not value])
            else:
                stack.append([False, False])
            continue
        if IF_RE.match(line):
            stack.append([False, False])
        elif ELSE_RE.match(line) and stack:
            if stack[-1][0]:
                stack[-1][1] = not stack[-1][1]
        elif ENDIF_RE.match(line) and stack:
            stack.pop()
            continue
        if any(entry[1] for entry in stack):
            continue
        out.append(line)
    return "\n".join(out)


def patched_source(repo: str, rel: str, pristine: str) -> str:
    """Return the source text after the build's import-overlay patch, if one
    exists for this path; the pristine text otherwise."""
    # Patches are named after the source path under src/, e.g.
    # src_sc_sc1pmode_sc1pgame.patch for sc/sc1pmode/sc1pgame.c.
    patch = os.path.join(
        repo, "scripts", "import-overlays", "battleship",
        "src_" + rel.replace("/", "_").replace(".c", ".patch"),
    )
    if not os.path.exists(patch):
        return read(pristine)
    with tempfile.TemporaryDirectory() as tmp:
        # The patches are written against a/src/<rel>, so the scratch copy
        # must sit under src/ for git apply's -p1 to find it.
        dst = os.path.join(tmp, "src", rel)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(pristine, dst)
        proc = subprocess.run(
            ["git", "apply", "--whitespace=nowarn", patch],
            cwd=tmp, capture_output=True, text=True,
        )
        if proc.returncode != 0:
            sys.stderr.write(
                f"warning: {os.path.basename(patch)} did not apply: {proc.stderr.strip()}\n"
            )
            return read(pristine)
        return read(dst)


def known_symbols(repo: str) -> set[str]:
    known: set[str] = set()
    known |= set(LL_RE.findall(read(os.path.join(repo, "include", "reloc_data.h"))))
    for p in glob.glob(os.path.join(repo, "include", "nds", "generated", "*.h")):
        known |= set(LL_RE.findall(read(p)))
    paths = (
        glob.glob(os.path.join(repo, "src", "**", "*.c"), recursive=True)
        + glob.glob(os.path.join(repo, "src", "**", "*.h"), recursive=True)
        + glob.glob(os.path.join(repo, "include", "**", "*.h"), recursive=True)
    )
    for p in paths:
        text = read(p)
        known |= set(GLOBAL_RE.findall(text))
        known |= set(LVALUE_RE.findall(text))
    return known


def census(repo: str, only_tu: str | None, flag: str | None) -> dict[str, list[str]]:
    known = known_symbols(repo)
    decomp_src = os.path.join(repo, "decomp", "BattleShip-main", "decomp", "src")
    result: dict[str, list[str]] = {}
    for tu in sorted(glob.glob(os.path.join(repo, "src", "import", "battleship_*.c"))):
        name = os.path.basename(tu)
        if only_tu and name != only_tu:
            continue
        text = read(tu)
        if flag and flag not in text:
            continue
        local = set(RENAME_RE.findall(text)) | ALLOW.get(name, set())
        missing: set[str] = set()
        for m in INC_RE.finditer(text):
            rel = m.group(1)
            src = os.path.join(decomp_src, rel)
            if not os.path.exists(src):
                continue
            body = as_built(patched_source(repo, rel, src))
            syms = set(LL_RE.findall(body)) - SKIP
            missing |= {s for s in syms if s not in known and s not in local}
        if missing:
            result[name] = sorted(missing)
    return result


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--repo-root", default=os.path.join(os.path.dirname(__file__), "..", ".."))
    ap.add_argument("--tu", help="one src/import basename to report")
    ap.add_argument("--flag", help="only TUs mentioning this build flag (e.g. NDS_P2_1P_GAME)")
    ap.add_argument("--strict", action="store_true", help="exit 1 when any symbol is unrowed")
    ap.add_argument("--all", action="store_true", help="list every symbol, not the first eight")
    args = ap.parse_args()
    repo = os.path.abspath(args.repo_root)
    result = census(repo, args.tu, args.flag)
    total = 0
    for name, syms in sorted(result.items(), key=lambda kv: -len(kv[1])):
        total += len(syms)
        shown = syms if args.all else syms[:8]
        more = "" if args.all or len(syms) <= 8 else " ..."
        print(f"{name:<38} {len(syms):3d} unrowed: {', '.join(shown)}{more}")
    print(f"RELOC_SYMBOL_CENSUS unrowed={total} tus={len(result)}")
    if args.strict and total:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

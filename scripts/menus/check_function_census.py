#!/usr/bin/env python3
"""Census of the source-prefixed functions the compiled sources call.

Every ``src/import/battleship_*.c`` that textually includes a decomp source
inherits that source's calls, and the code-first mode links nothing until the
final pass -- so a call nobody defines is a link error with no other signal.
On 2026-09-05 a hand census of the ``NDS_P2_1P_GAME`` TUs found five such
functions (commit 43383ef3a37: ``mpCollisionSetBGM``,
``mpCollisionGetLineCountType``, ``mpCollisionGetLineIDsTypeCount``,
``ftCommonAppearSetPosition``, ``ftPhysicsSetAirVelTransN``).

This prints, per name, every function the compiled sources CALL that nothing
the port compiles DEFINES.  The compiled set is every ``src/**/*.c`` and
``src/**/*.inc``, every decomp file a ``src/import`` TU textually includes
(after its import-overlay patch when one exists, with the ``REGION_JP`` and
``!SSB64_TARGET_NDS`` arms dropped -- the port is a REGION_US,
SSB64_TARGET_NDS build), every decomp file the Makefile compiles directly
(the ``$(BATTLESHIP_DECOMP)/src/...`` and ``$(BATTLESHIP_SYS)/...`` lines,
plus CFILES basenames that resolve through SOURCES to decomp -- only
``sys/utils.c``/``sys/vector.c`` today; the rest of ``sys/`` is a search
path, not a compiled list), and every
``include/**/*.h`` for macros (plus the ``src`` and decomp headers those
bodies include, e.g. the decomp ``ft/fighter.h`` ``ftGetStruct`` macro).  C comments and string/character literals are
stripped from every body before scanning.

Calls are identifiers with a source prefix (``it|ef|wp|gr|ft|sc|mn|lb|gc|sy|
mp|if|gm|mv|db|al`` + uppercase + word chars) followed by an open
parenthesis.  Definitions are a function definition whose name opens the
declarator's parameter list with an opening brace after the closing paren
(brace possibly on the next line; multi-line declarators handled), or an
object-like or function-like macro of that name.  Prototypes, declarations,
and bare function-pointer assignments do not count as either.

Usage:
    python scripts/menus/check_function_census.py            # report
    python scripts/menus/check_function_census.py --strict   # exit 1 if any
    python scripts/menus/check_function_census.py --tu battleship_sc1pgame_runtime.c
    python scripts/menus/check_function_census.py --flag NDS_P2_1P_GAME
"""
from __future__ import annotations

import argparse
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_reloc_symbol_census import INC_RE, as_built, patched_source, read  # noqa: E402

CALL_RE = re.compile(
    r"\b((?:it|ef|wp|gr|ft|sc|mn|lb|gc|sy|mp|if|gm|mv|db|al)[A-Z]\w*)\s*\("
)
MACRO_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)", re.M)
DECL_KW_RE = re.compile(
    r"\b(if|for|while|switch|return|sizeof|typeof|typedef|else|case)\b"
)

# Names the port deliberately leaves out, each with its one-line reason.
ALLOW: dict[str, str] = {
}

DIRECT_RES = (
    re.compile(r"\$\(\s*PROJECT_ROOT\s*\)/\$\(\s*BATTLESHIP_DECOMP\s*\)/([^\s\"'\\]+\.c)"),
    re.compile(r"\$\(\s*BATTLESHIP_DECOMP\s*\)/([^\s\"'\\]+\.c)"),
    re.compile(r"\$\(\s*PROJECT_ROOT\s*\)/\$\(\s*BATTLESHIP_SYS\s*\)/([^\s\"'\\]+\.c)"),
    re.compile(r"\$\(\s*BATTLESHIP_SYS\s*\)/([^\s\"'\\]+\.c)"),
    re.compile(r"\$\(\s*PROJECT_ROOT\s*\)/decomp/BattleShip-main/decomp/([^\s\"'\\]+\.c)"),
    re.compile(r"(?<![\w/])decomp/BattleShip-main/decomp/src/([^\s\"'\\]+\.c)"),
)


def strip_comments_strings(text: str) -> str:
    """Replace comments and string/char literals with spaces, keeping newlines
    (and length) so match offsets still map to the original line numbers."""
    out = list(text)
    n = len(text)
    i = 0
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if c == "/" and nxt == "/":
            out[i] = " "
            out[i + 1] = " "
            i += 2
            while i < n and text[i] != "\n":
                out[i] = " "
                i += 1
        elif c == "/" and nxt == "*":
            out[i] = " "
            out[i + 1] = " "
            i += 2
            while i < n and not (text[i] == "*" and i + 1 < n and text[i + 1] == "/"):
                if text[i] != "\n":
                    out[i] = " "
                i += 1
            if i < n:
                out[i] = " "
                if i + 1 < n:
                    out[i + 1] = " "
                i += 2
        elif c == '"':
            out[i] = " "
            i += 1
            while i < n and text[i] != '"':
                if text[i] == "\\" and i + 1 < n:
                    if out[i] != "\n":
                        out[i] = " "
                    i += 1
                    if text[i] != "\n":
                        out[i] = " "
                    i += 1
                    continue
                if text[i] == "\n":
                    break
                out[i] = " "
                i += 1
            if i < n and text[i] == '"':
                out[i] = " "
                i += 1
        elif c == "'":
            out[i] = " "
            i += 1
            while i < n and text[i] != "'":
                if text[i] == "\\" and i + 1 < n:
                    out[i] = " "
                    i += 1
                    out[i] = " "
                    i += 1
                    continue
                if text[i] == "\n":
                    break
                out[i] = " "
                i += 1
            if i < n and text[i] == "'":
                out[i] = " "
                i += 1
        else:
            i += 1
    return "".join(out)


def paren_end(text: str, open_idx: int) -> int | None:
    depth = 0
    for j in range(open_idx, len(text)):
        if text[j] == "(":
            depth += 1
        elif text[j] == ")":
            depth -= 1
            if depth == 0:
                return j
    return None


TYPE_NAME_RE = re.compile(r"[A-Za-z_]\w*[\s\*]+[A-Za-z_]\w*\s*$")


def is_declarator(before: str, name: str) -> bool:
    before = strip_attrs(before)
    if "=" in before:
        return False
    if "(" in before or ")" in before:
        return False
    if DECL_KW_RE.search(before):
        return False
    # A bare call statement (mpFooBar();) leaves only the name behind the
    # boundary; a declaration/definition carries its type (void mpFooBar).
    # `before` ends where the name starts, so reattach it for the test.
    return TYPE_NAME_RE.search(before + name) is not None


DIRECTIVE_END_RE = re.compile(r"^\s*#(?:[^\n\\]|\\\n|\\[^\n])*$", re.M)
ATTR_HEAD_RE = re.compile(r"__attribute__")
CLOSE_NL_RE = re.compile(r"\)[ \t]*\r?\n")


def attr_spans(text: str) -> list[tuple[int, int]]:
    """Balanced spans of __attribute__((...)) groups (regex cannot nest)."""
    spans: list[tuple[int, int]] = []
    for m in ATTR_HEAD_RE.finditer(text):
        i = m.end()
        while i < len(text) and text[i] in " \t\r\n":
            i += 1
        if i < len(text) and text[i] == "(":
            j = paren_end(text, i)
            if j is not None:
                spans.append((m.start(), j + 1))
    return spans


def strip_attrs(text: str) -> str:
    spans = attr_spans(text)
    if not spans:
        return text
    out: list[str] = []
    prev = 0
    for a, b in spans:
        out.append(text[prev:a])
        prev = b
    out.append(text[prev:])
    return "".join(out)


FUNC_LIKE_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)\s*\(([^()\n]*)\)[ \t]*(.*?)[ \t]*$", re.M)


def generator_macros(stripped: str) -> dict[str, list[int]]:
    """Function-like macros whose body defines a function from a parameter,
    e.g. ``#define NDS_SCENE_STUB(name) void name(void) {...}`` -- every
    ``NDS_SCENE_STUB(dbBattleStartScene)`` invocation defines that function.
    Derived from the macro body (the parameter in a ``name(...) {`` shape),
    not a hardcoded name list.  Returns macro -> defining parameter indexes.
    """
    out: dict[str, list[int]] = {}
    for m in FUNC_LIKE_RE.finditer(stripped):
        macro, params, body = m.group(1), m.group(2), m.group(3)
        names = [p.strip() for p in params.split(",")]
        defining = [i for i, p in enumerate(names)
                    if p and re.search(r"\b" + re.escape(p) +
                                       r"\s*\([^;{}]*\)\s*\{", body)]
        if defining:
            out[macro] = defining
    return out


def generated_definitions(stripped: str,
                          generators: dict[str, list[int]]) -> set[str]:
    """Names the generator-macro invocations in this body define."""
    found: set[str] = set()
    for macro, indexes in generators.items():
        for m in re.finditer(r"\b" + re.escape(macro) + r"\s*\(", stripped):
            line_start = stripped.rfind("\n", 0, m.start()) + 1
            if stripped[line_start:m.start()].lstrip().startswith("#"):
                continue
            open_idx = m.end() - 1
            close_idx = paren_end(stripped, open_idx)
            if close_idx is None:
                continue
            args = stripped[open_idx + 1:close_idx].split(",")
            for i in indexes:
                if i < len(args):
                    arg = args[i].strip()
                    if re.fullmatch(r"[A-Za-z_]\w*", arg or "") \
                            and CALL_RE.match(arg + "("):
                        found.add(arg)
    return found


def scan_body(stripped: str) -> tuple[set[str], set[str], set[str]]:
    """Return (defined, called, macros) for one stripped body."""
    defined: set[str] = set()
    called: set[str] = set()
    macros: set[str] = set()
    for m in MACRO_RE.finditer(stripped):
        macros.add(m.group(1))
    dir_ends = [m.end() for m in DIRECTIVE_END_RE.finditer(stripped)]
    attr_spans_all = attr_spans(stripped)
    close_nl_ends = [m.end() for m in CLOSE_NL_RE.finditer(stripped)
                     if not any(a <= m.start() < b for a, b in attr_spans_all)]
    for m in CALL_RE.finditer(stripped):
        name = m.group(1)
        line_start = stripped.rfind("\n", 0, m.start()) + 1
        if stripped[line_start:m.start()].lstrip().startswith("#"):
            continue
        open_idx = m.end() - 1
        close_idx = paren_end(stripped, open_idx)
        if close_idx is None:
            called.add(name)
            continue
        after = stripped[close_idx + 1:]
        k = 0
        while True:
            while k < len(after) and after[k] in " \t\r\n":
                k += 1
            if after[k:k + 13] == "__attribute__":
                skipped = False
                for a, b in attr_spans_all:
                    base = close_idx + 1 + k
                    if a <= base < b:
                        k = b - (close_idx + 1)
                        skipped = True
                        break
                if not skipped:
                    break
                continue
            break
        nxt = after[k] if k < len(after) else ""
        bounds = [stripped.rfind(";", 0, m.start()),
                  stripped.rfind("{", 0, m.start()),
                  stripped.rfind("}", 0, m.start())]
        for ends in (dir_ends, close_nl_ends):
            for e in ends:
                # e is a match end (already past the boundary text); store
                # the last boundary char so prev + 1 lands after it.
                if e <= m.start():
                    bounds.append(e - 1)
                else:
                    break
        prev = max(bounds)
        before = stripped[prev + 1:m.start()]
        if nxt == "{" and is_declarator(before, name):
            defined.add(name)
        elif nxt == ";" and is_declarator(before, name):
            continue
        else:
            called.add(name)
    return defined, called, macros


def direct_decomp_files(repo: str) -> list[str]:
    # The decomp files the Makefile compiles directly: CFILES basenames that
    # resolve through SOURCES to decomp (only utils.c/vector.c today -- the
    # sys/ directory is a search path, not a compiled list, so e.g.
    # sys/audio.c's al* calls never compile), plus the explicit
    # $(BATTLESHIP_DECOMP)/src/... mentions (overlay generator inputs and
    # generator dependencies, whose pristine bodies match the compiled
    # overlay copies except for the compiled-out blocks as_built drops).
    found: set[str] = set()
    try:
        mk = read(os.path.join(repo, "Makefile"))
    except OSError:
        mk = ""
    base = os.path.join(repo, "decomp", "BattleShip-main", "decomp")
    for pat in DIRECT_RES:
        for m in pat.finditer(mk):
            rel = m.group(1)
            if rel.startswith("src/"):
                found.add(os.path.join(base, *rel.split("/")))
            elif rel.startswith("sys/"):
                found.add(os.path.join(base, "src", *rel.split("/")))
            elif rel.startswith("src/sys/"):
                found.add(os.path.join(base, *rel.split("/")))
            else:
                found.add(os.path.join(base, "src", *rel.split("/")))
    sysdir = os.path.join(repo, "decomp", "BattleShip-main", "decomp", "src", "sys")
    cfiles: set[str] = set()
    for line in re.sub(r"\\\r?\n", " ", mk).splitlines():
        s = line.strip()
        if s.startswith("CFILES"):
            for tok in s.split("=", 1)[1].split():
                if "$" in tok:
                    continue
                if tok.endswith(".c") or tok.endswith(".inc"):
                    cfiles.add(os.path.basename(tok))
    src_names: set[str] = set()
    for p in (glob.glob(os.path.join(repo, "src", "**", "*.c"), recursive=True)
              + glob.glob(os.path.join(repo, "src", "**", "*.inc"), recursive=True)):
        src_names.add(os.path.basename(p))
    for base in cfiles:
        if base not in src_names and os.path.exists(os.path.join(sysdir, base)):
            found.add(os.path.join(sysdir, base))
    return sorted(p for p in found if os.path.exists(p))


def census(repo: str, only_tu: str | None, flag: str | None,
           ) -> tuple[dict[str, list[str]], dict[str, list[str]]]:
    defined: set[str] = set()
    calls: dict[str, set[str]] = {}
    decomp_src = os.path.join(repo, "decomp", "BattleShip-main", "decomp", "src")
    tus = sorted(glob.glob(os.path.join(repo, "src", "import", "battleship_*.c")))
    # Definitions are always global (the whole compiled set); the filters
    # only choose whose calls are reported, like the reloc census.
    tu_flag_ok: dict[str, bool] = {}
    for tu in tus:
        text = read(tu)
        tu_flag_ok[os.path.basename(tu)] = (not flag) or (flag in text)

    # (label, stripped body, report its calls, header-only for macros+defs)
    work: list[tuple[str, str, bool, bool]] = []
    for tu in tus:
        name = os.path.basename(tu)
        text = read(tu)
        work.append((name, strip_comments_strings(as_built(text)),
                     tu_flag_ok[name], False))
        for m in INC_RE.finditer(text):
            rel = m.group(1)
            src = os.path.join(decomp_src, rel)
            if not os.path.exists(src):
                continue
            work.append((name, strip_comments_strings(
                as_built(patched_source(repo, rel, src))), tu_flag_ok[name],
                False))

    src_files = (
        glob.glob(os.path.join(repo, "src", "**", "*.c"), recursive=True)
        + glob.glob(os.path.join(repo, "src", "**", "*.inc"), recursive=True)
    )
    import_names = {os.path.basename(t) for t in tus}
    for p in sorted(src_files):
        base = os.path.basename(p)
        if base in import_names:
            continue
        own = read(p)
        work.append((base, strip_comments_strings(as_built(own)),
                     (not flag) or (flag in own), False))

    for p in direct_decomp_files(repo):
        rel = os.path.relpath(p, decomp_src).replace(os.sep, "/")
        raw = (patched_source(repo, rel, p)
               if not rel.startswith("..") else read(p))
        work.append((os.path.basename(p),
                     strip_comments_strings(as_built(raw)),
                     (not only_tu) and ((not flag) or (flag in raw)), False))

    for p in glob.glob(os.path.join(repo, "include", "**", "*.h"), recursive=True):
        try:
            body = read(p)
        except OSError:
            continue
        work.append((os.path.basename(p),
                     strip_comments_strings(as_built(body)), False, True))
    for p in (glob.glob(os.path.join(repo, "src", "**", "*.h"), recursive=True)
              + glob.glob(os.path.join(repo, "decomp", "BattleShip-main",
                                        "decomp", "src", "**", "*.h"), recursive=True)):
        try:
            body = read(p)
        except OSError:
            continue
        work.append((os.path.basename(p),
                     strip_comments_strings(as_built(body)), False, True))

    generators: dict[str, list[int]] = {}
    for _, stripped, _, _ in work:
        for macro, indexes in generator_macros(stripped).items():
            generators.setdefault(macro, indexes)

    for label, stripped, report_calls, _ in work:
        d, c, mac = scan_body(stripped)
        defined |= {n for n in d | mac if CALL_RE.match(n + "(")}
        defined |= {n for n in generated_definitions(stripped, generators)
                    if CALL_RE.match(n + "(")}
        if report_calls and ((not only_tu) or label == only_tu):
            for n in c:
                calls.setdefault(n, set()).add(label)

    missing = {n: sorted(t) for n, t in calls.items() if n not in defined}
    sites: dict[str, list[str]] = {}
    for n in missing:
        sites[n] = missing[n]
    return missing, sites


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--repo-root", default=os.path.join(os.path.dirname(__file__), "..", ".."))
    ap.add_argument("--tu", help="one src/import basename to report")
    ap.add_argument("--flag", help="only TUs mentioning this build flag (e.g. NDS_P2_1P_GAME)")
    ap.add_argument("--strict", action="store_true", help="exit 1 when any function is undefined")
    ap.add_argument("--all", action="store_true", help="list every name, not the first eight")
    args = ap.parse_args()
    repo = os.path.abspath(args.repo_root)
    missing, _ = census(repo, args.tu, args.flag)
    report = {n: t for n, t in missing.items() if n not in ALLOW}
    allowed_hit = sorted(n for n in missing if n in ALLOW)
    total = 0
    for name in sorted(report, key=lambda n: (-len(report[n]), n)):
        tus = report[name]
        total += 1
        shown = tus if args.all else tus[:8]
        more = "" if args.all or len(tus) <= 8 else " ..."
        print(f"{name:<38} called by: {', '.join(shown)}{more}")
    print(f"FUNCTION_CENSUS undefined={total} allowed={len(allowed_hit)}")
    if args.strict and total:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

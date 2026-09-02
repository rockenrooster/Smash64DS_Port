#!/usr/bin/env python3
"""P2-1j (a). Diff what the ORIGINAL draws on each shell screen against what WE
draw, and fail on any delta that is not written down.

WHY THIS EXISTS
---------------
Three owner visual passes in a row found missing on-screen elements, and every
time the finding arrived through the owner's eye rather than through tooling.
The owner named the root cause: *the source code is all there; verification must
come from it.*  Nothing in this tree compared a screen's source sprite list
against the sprite list we ship, so an element that was simply never converted
looked exactly like an element that was.  This does that comparison, per screen,
mechanically, and it is wired into `verify-all.ps1` so it runs on Boundary.

THE PARSE STRATEGY, and why each half is the robust choice
----------------------------------------------------------
SOURCE SIDE -- the identity of a drawn element is its RELOC SYMBOL.  Every
sprite BattleShip puts on a menu screen is constructed from a `Sprite*` resolved
out of an o2r container by symbol:

    lbCommonMakeSObjForGObj(gobj, lbRelocGetFileData(Sprite*, files[n],
                                                     &llMNCommonOptionTabLeftSprite))

and the desc TABLES a scene indexes into are arrays of those same addresses
(`intptr_t offsets[] = { &llMNPlayersPortraitsMarioSprite, ... }`).  So a
single lexical fact -- a reference to `&ll<Name>Sprite` -- covers direct
construct calls, per-state variant tables, and per-fighter tables alike, and it
cannot be defeated by a helper indirection the way matching on the CALL would
be.  Anything positional (function names, argument order) would rot; the symbol
is the asset's own name and is stable.

Two things a screen draws are NOT sprites, so they are extracted separately and
reported as their own element kinds: `gDPFillRectangle` panels (the VS menu's
own menu-name plate is one, and the title's fire field turned out to be one --
P2-1i) and `gDPSetPrimColor`-driven flat fills are not, because those modulate
a sprite that is already counted.

Only the US build is audited: the file is scanned with a real preprocessor
region mask, so `#if defined(REGION_JP)` blocks (the VS menu's whole subtitle,
for one) do not read as elements we are missing.  An `#if` this evaluator
cannot decide is treated as COMPILED IN, which is the conservative direction --
it can only add source elements, i.e. produce a delta to explain, never hide
one.

Dead source code is separated by REACHABILITY rather than by judgement: the
call graph is built over identifier references (not call syntax, so a
`gcAddGObjProcess(gobj, mnXProcUpdate, ...)` function-pointer registration is an
edge too) and walked from the scene's `mn<Scene>FuncStart`.  What that walk
cannot reach, the scene never builds; those symbols are reported as INFO and
never as a delta.

OUR SIDE -- two sources, both machine-read, neither transcribed:
  1. THE BAKE.  `generate_mn_ui_kit.py` is IMPORTED, not parsed, and its
     `IMAGE_SOURCES` / `SURFACE_SOURCES` / fire-atlas tables are read directly.
     They map each kit token back to the source symbol(s) it was converted
     from, which is exactly the join key the source side produces.
  2. THE SHELL.  `src/nds/nds_menu_shell.c` is the translation-unit root.  It
     and every local `.c` implementation fragment it includes are scanned for
     `NDS_MN_UI_KIT_IMAGE_*` / `NDS_MN_UI_KIT_SURFACE_*` references, each
     attributed to its enclosing function, and each function attributed to a
     screen by the file's own naming convention (`...Title...`, `...Mode...`,
     `...Vs...`, `...Css...`, `...Sss...`).  A function that references a token
     and matches no screen -- or two -- is a hard error, so a rename cannot
     silently empty a screen's inventory.  Backdrop surfaces come from the
     `kNdsMenu*Surfaces[]` arrays resolved through `ndsMenuShellEnterBackdrop`'s
     own switch, so the screen->surface mapping is the runtime's.

     A token referenced at FILE SCOPE is attributed by its enclosing TABLE's
     name under the same rule, because the shell keeps its state->surface maps
     there: `kNdsCssPanelSurface[slot][pkind]` is the character select's whole
     player-panel inventory and function attribution alone would report every
     panel as drawn nowhere.  One deliberate difference from functions -- a
     table matching NO screen is skipped rather than an error, because the
     backdrop switch already attributes the shared arrays by the case they are
     blitted under (`kNdsMenuStoneSurfaces` serves two screens and names
     neither).

Runtime expansions are declared rather than inferred when they are arithmetic
on a token id: `ndsUiKitSetNumber` draws `DIGIT_0..DIGIT_9`;
`IMAGE_MODE_ICON_1P + i` / `IMAGE_LABEL_HMN + pkind` walk their own consecutive
blocks; the CSS gate owner walks its generated [player][state] and
[team][player][state] ranges; and the CSS team selector walks the twelve
consecutive `SURFACE_CSS_TEAM_SELECT_*` rasters from `RED_0`.  See
`TOKEN_FAMILIES` / `HELPER_TOKENS`.

THE THREE DELTA CLASSES
  MISSING      the source draws it on this screen and we draw nothing from it
  EXTRA        we draw something on this screen the source does not draw there
  SUBSTITUTED  we draw a declared approximation instead (font text for a text
               sprite, an icon for a 3D preview, ...)

An unexplained delta of any class fails the run.  Explained ones live in
`mn_screen_coverage_allowlist.json`, every entry carrying a `ruling` that names
the board row or doc that decided it -- and a stale entry (one that no longer
matches a real delta) fails too, so a fixed delta cannot keep its excuse.

USAGE
  python scripts/menus/audit_mn_screen_coverage.py              # report + gate
  python scripts/menus/audit_mn_screen_coverage.py --json out.json
  python scripts/menus/audit_mn_screen_coverage.py --screen vs_mode
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

# ---------------------------------------------------------------------------
# Screen table -- the only hand-written mapping in the tool, and every name in
# it is checked to exist, so it cannot rot silently.
# ---------------------------------------------------------------------------


@dataclass(frozen=True)
class ScreenSpec:
    key: str
    title: str
    # Scene source files, repo-relative under decomp/BattleShip-main/decomp/src.
    sources: tuple[str, ...]
    # Substring of a shell function name that assigns it to this screen.
    shell_tag: str
    # The NDS_MENU_SHELL_SCREEN_* enumerator ndsMenuShellEnterBackdrop keys on.
    backdrop_case: str


SCREENS = (
    ScreenSpec("title", "Title screen",
               ("mn/mncommon/mntitle.c",), "Title", "TITLE"),
    ScreenSpec("mode_select", "Main menu (mode select)",
               ("mn/mncommon/mnmodeselect.c",), "Mode", "MODE"),
    ScreenSpec("vs_mode", "VS menu (rules)",
               ("mn/mnvsmode/mnvsmode.c",), "Vs", "VSMODE"),
    ScreenSpec("css", "Character select",
               ("mn/mnplayers/mnplayersvs.c",), "Css", "CSS"),
    ScreenSpec("sss", "Stage select",
               ("mn/mnmaps/mnmaps.c",), "Sss", "SSS"),
)

# `ndsUiKitSetNumber` fans one call out over the ten digit cells.
HELPER_TOKENS = {
    "ndsUiKitSetNumber": tuple(f"DIGIT_{d}" for d in range(10)),
}

# A token the shell indexes with `+ <expr>`: the whole consecutive block is
# reachable from that one reference.  Keyed by the block's FIRST token; the
# prefix (IMAGE_/SURFACE_) is kept from the reference, so one declaration
# serves both kinds.
# Must equal generate_mn_ui_kit.py's CSS_FIGHTER_TOKEN in order: the runtime
# indexes gate states by that ordinal (NDS_CSS_GATE_FIGHTERS), and a fighter
# missing here makes his baked name/emblem art invisible to this audit -- which
# is exactly how Link's name text sat allowlisted as "unproduced" after it was.
_CSS_GATE_FIGHTERS = ("MARIO", "FOX", "LUIGI", "DONKEY", "CAPTAIN", "SAMUS",
                      "LINK", "PIKACHU", "YOSHI", "NESS", "PURIN", "KIRBY")
_CSS_GATE_STATES = (("NA", "MAN", "COM") +
                    tuple(f"MAN_{fighter}" for fighter in _CSS_GATE_FIGHTERS) +
                    tuple(f"COM_{fighter}" for fighter in _CSS_GATE_FIGHTERS) +
                    tuple(f"HOLD_{fighter}" for fighter in _CSS_GATE_FIGHTERS))

TOKEN_FAMILIES = {
    "MODE_ICON_1P": ("MODE_ICON_1P", "MODE_ICON_VS", "MODE_ICON_OPTION",
                     "MODE_ICON_DATA"),
    "LABEL_HMN": ("LABEL_HMN", "LABEL_CP", "LABEL_NA"),
    "DIGIT_0": tuple(f"DIGIT_{d}" for d in range(10)),
    # nds_menu_shell_css.c indexes these exact generated contiguous blocks.
    # Its _Static_asserts pin player/team strides and the final HOLD_SAMUS id;
    # declare the same arithmetic expansion here so deleting the old literal
    # 4xN table does not make source-derived name/emblem art invisible to the
    # coverage audit.
    "CSS_GATE_0_NA": tuple(
        f"CSS_GATE_{player}_{state}"
        for player in range(4)
        for state in _CSS_GATE_STATES),
    "CSS_GATE_TEAM_RED_0_NA": tuple(
        f"CSS_GATE_TEAM_{team}_{player}_{state}"
        for team in ("RED", "BLUE", "GREEN")
        for player in range(4)
        for state in _CSS_GATE_STATES),
    # nds_menu_shell.c reaches the twelve team-selector rasters as
    # CSS_TEAM_SELECT_RED_0 + (team * NDS_CSS_TEAM_SELECT_STRIDE) + slot.
    # Contiguity in [team][player] order is compile-time proven there by the
    # _Static_asserts on BLUE_0/GREEN_3 and by the generator's contiguous
    # emit loop, so the family is declared rather than inferred.
    "CSS_TEAM_SELECT_RED_0": tuple(
        f"CSS_TEAM_SELECT_{team}_{player}"
        for team in ("RED", "BLUE", "GREEN")
        for player in range(4)),
}

SHELL_PATH = Path("src/nds/nds_menu_shell.c")
BAKE_PATH = Path("scripts/menus/generate_mn_ui_kit.py")
ALLOWLIST_PATH = Path("scripts/menus/mn_screen_coverage_allowlist.json")
DECOMP_SRC = Path("decomp/BattleShip-main/decomp/src")


class AuditError(RuntimeError):
    """A deterministic falsifier -- the tool refuses to guess."""


# ---------------------------------------------------------------------------
# C lexing helpers
# ---------------------------------------------------------------------------


def blank_comments_and_literals(text: str) -> str:
    """Replace comments and string/char literals with spaces, keeping offsets.

    Line count and column positions are preserved so every reported line number
    is the real one in the real file.
    """
    out = list(text)
    i = 0
    n = len(text)
    while i < n:
        ch = text[i]
        nxt = text[i + 1] if (i + 1) < n else ""
        if ch == "/" and nxt == "/":
            while i < n and text[i] != "\n":
                out[i] = " "
                i += 1
        elif ch == "/" and nxt == "*":
            out[i] = out[i + 1] = " "
            i += 2
            while i < n and not (text[i] == "*" and (i + 1) < n and
                                 text[i + 1] == "/"):
                if text[i] != "\n":
                    out[i] = " "
                i += 1
            if i < n:
                out[i] = " "
                if (i + 1) < n:
                    out[i + 1] = " "
                i += 2
        elif ch in ("\"", "'"):
            quote = ch
            out[i] = " "
            i += 1
            while i < n and text[i] != quote:
                if text[i] == "\\":
                    out[i] = " "
                    i += 1
                    if i < n:
                        out[i] = " "
                        i += 1
                    continue
                if text[i] != "\n":
                    out[i] = " "
                i += 1
            if i < n:
                out[i] = " "
                i += 1
        else:
            i += 1
    return "".join(out)


_DEFINED_RE = re.compile(r"defined\s*\(\s*(\w+)\s*\)|defined\s+(\w+)")
_REGIONS = {"REGION_US", "REGION_JP", "REGION_EU"}


def _eval_condition(expr: str) -> bool | None:
    """Evaluate a `#if` over REGION_* only.  None means 'cannot decide'."""
    expr = expr.strip()
    if expr in ("0",):
        return False
    if expr in ("1",):
        return True

    def sub(match: re.Match[str]) -> str:
        name = match.group(1) or match.group(2)
        if name not in _REGIONS:
            raise AuditError("undecidable")
        return "True" if name == "REGION_US" else "False"

    try:
        pythonic = _DEFINED_RE.sub(sub, expr)
    except AuditError:
        return None
    # Anything left that is not boolean grammar means another macro is involved.
    if re.search(r"[A-Za-z_]\w*", pythonic.replace("True", "")
                 .replace("False", "")):
        return None
    pythonic = (pythonic.replace("&&", " and ").replace("||", " or ")
                .replace("!", " not "))
    try:
        return bool(eval(pythonic, {"__builtins__": {}}, {}))  # noqa: S307
    except Exception:  # noqa: BLE001 -- an undecidable expression, not a crash
        return None


def region_mask(lines: list[str]) -> list[bool]:
    """Per-line: is this line compiled in a REGION_US build?

    An `#if` this cannot decide is treated as taken, which can only ADD source
    elements to explain.  `#ifdef`/`#ifndef` on a non-region macro is likewise
    treated as taken.
    """
    mask: list[bool] = []
    # stack entries: (active_now, any_branch_taken_yet, decidable)
    stack: list[list[bool]] = []

    def live() -> bool:
        return all(frame[0] for frame in stack)

    for raw in lines:
        stripped = raw.strip()
        directive = None
        if stripped.startswith("#"):
            directive = stripped[1:].strip()
        if directive is None:
            mask.append(live())
            continue
        if directive.startswith("if"):
            if directive.startswith("ifdef"):
                name = directive[5:].strip()
                value = True if name not in _REGIONS else (name == "REGION_US")
            elif directive.startswith("ifndef"):
                name = directive[6:].strip()
                value = True if name not in _REGIONS else (name != "REGION_US")
            else:
                decided = _eval_condition(directive[2:])
                value = True if decided is None else decided
            stack.append([value, value, True])
            mask.append(False)  # the directive line itself holds no element
            continue
        if directive.startswith("elif"):
            if not stack:
                raise AuditError("#elif without #if")
            frame = stack[-1]
            if frame[1]:
                frame[0] = False
            else:
                decided = _eval_condition(directive[4:])
                frame[0] = True if decided is None else decided
                frame[1] = frame[1] or frame[0]
            mask.append(False)
            continue
        if directive.startswith("else"):
            if not stack:
                raise AuditError("#else without #if")
            frame = stack[-1]
            frame[0] = not frame[1]
            frame[1] = True
            mask.append(False)
            continue
        if directive.startswith("endif"):
            if not stack:
                raise AuditError("#endif without #if")
            stack.pop()
            mask.append(False)
            continue
        mask.append(live())
    if stack:
        raise AuditError("unterminated #if")
    return mask


_FUNC_HEAD_RE = re.compile(r"(\w+)\s*\([^;{}]*\)\s*$")


@dataclass
class Function:
    name: str
    first_line: int
    last_line: int


def top_level_functions(clean: str) -> list[Function]:
    """Every function body in the file, by brace depth over cleaned text.

    A depth-0 `{` opens a function only when the declaration text leading up to
    it carries no `=`; otherwise it is a file-scope INITIALIZER (the shell's
    `kNdsMenu*Surfaces[] = { ... }` tables are exactly this) and the block is
    skipped rather than reported as an anonymous function.
    """
    lines = clean.split("\n")
    functions: list[Function] = []
    depth = 0
    pending: list[tuple[str, int]] = []
    decl = ""
    current: Function | None = None
    for index, line in enumerate(lines, start=1):
        if depth == 0 and not line.lstrip().startswith("#"):
            head = _FUNC_HEAD_RE.search(line.rstrip())
            if head is not None:
                pending.append((head.group(1), index))
                pending = pending[-2:]
        for ch in line:
            if depth == 0 and ch not in "{}":
                decl += ch
            if ch == "{":
                if depth == 0:
                    if pending and ("=" not in decl):
                        name, first = pending[-1]
                        current = Function(name, first, index)
                    else:
                        current = None
                    pending = []
                    decl = ""
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    if current is not None:
                        current.last_line = index
                        functions.append(current)
                        current = None
                    decl = ""
            elif depth == 0 and ch == ";":
                decl = ""
        if depth == 0:
            decl += "\n"
    return functions


# ---------------------------------------------------------------------------
# Source inventory
# ---------------------------------------------------------------------------


@dataclass(frozen=True)
class SourceElement:
    kind: str        # "sprite" | "fill"
    symbol: str      # the ll*Sprite name, or a synthetic id for a fill
    file: str
    line: int
    function: str
    reachable: bool


_SPRITE_REF_RE = re.compile(r"&\s*(ll\w+Sprite)\b")
_FILL_RE = re.compile(r"\bgDPFillRectangle\s*\(")
_IDENT_RE = re.compile(r"\b([A-Za-z_]\w*)\b")


def scan_source_file(path: Path, rel: str) -> list[SourceElement]:
    text = path.read_text(errors="replace")
    clean = blank_comments_and_literals(text)
    raw_lines = clean.split("\n")
    mask = region_mask(raw_lines)
    # THE MASK MUST BE APPLIED BEFORE THE BRACES ARE COUNTED, and this is not a
    # tidiness point: mntitle.c:628 is an `#if defined(REGION_US) ... #else ...
    # #endif` whose two branches carry a different number of braces, so counting
    # depth over the unmasked text goes negative at line 637 and every function
    # after it is mis-attributed.
    lines = [line if keep else "" for line, keep in zip(raw_lines, mask)]
    functions = top_level_functions("\n".join(lines))
    owner: dict[int, str] = {}
    for func in functions:
        for line_no in range(func.first_line, func.last_line + 1):
            owner[line_no] = func.name

    # Call graph over identifier references, so function-pointer registration
    # (gcAddGObjProcess/gcAddGObjDisplay) is an edge like a direct call is.
    by_name = {func.name: func for func in functions}
    edges: dict[str, set[str]] = {name: set() for name in by_name}
    for func in functions:
        body = "\n".join(lines[func.first_line - 1:func.last_line])
        for ident in set(_IDENT_RE.findall(body)):
            if ident in by_name and ident != func.name:
                edges[func.name].add(ident)
    roots = [name for name in by_name if name.endswith("FuncStart")]
    if not roots:
        raise AuditError(f"{rel}: no mn*FuncStart entry point found")
    reachable: set[str] = set()
    stack = list(roots)
    while stack:
        name = stack.pop()
        if name in reachable:
            continue
        reachable.add(name)
        stack.extend(edges.get(name, ()))

    elements: list[SourceElement] = []
    for index, line in enumerate(lines, start=1):
        if not mask[index - 1]:
            continue
        function = owner.get(index, "<file scope>")
        live = (function in reachable) or (function == "<file scope>")
        for match in _SPRITE_REF_RE.finditer(line):
            elements.append(SourceElement("sprite", match.group(1), rel, index,
                                          function, live))
        if _FILL_RE.search(line):
            elements.append(SourceElement("fill", f"{function}@fill:{index}",
                                          rel, index, function, live))
    return elements


# ---------------------------------------------------------------------------
# Our inventory
# ---------------------------------------------------------------------------


def load_bake_module(repo_root: Path):
    path = repo_root / BAKE_PATH
    spec = importlib.util.spec_from_file_location("mn_ui_kit_bake", path)
    if spec is None or spec.loader is None:
        raise AuditError(f"cannot import {BAKE_PATH}")
    module = importlib.util.module_from_spec(spec)
    sys.modules["mn_ui_kit_bake"] = module
    spec.loader.exec_module(module)
    return module


def bake_token_symbols(module) -> dict[str, set[str]]:
    """kit token -> the set of source symbols it was converted from."""
    out: dict[str, set[str]] = {}
    for entry in module.IMAGE_SOURCES:
        _, symbol, token = entry[0], entry[1], entry[2]
        out.setdefault(f"IMAGE_{token}", set()).add(symbol)
    for spec in module.SURFACE_SOURCES:
        target = out.setdefault(f"SURFACE_{spec.token}", set())
        for part in spec.parts:
            target.add(part.symbol)
    # A surface can exceptionally contain a source element in its composited
    # `under` tree when draw ordering requires that element to sit between two
    # already-baked layers.  Recursing through `under` would be wrong: it also
    # carries restoration pixels and clipped base art outside the token's box,
    # causing a tiny patch to claim an entire screen.  The bake therefore may
    # publish only the extra symbols it intentionally contributes.  Keeping the
    # declaration beside the bake makes this the same machine-read provenance
    # as IMAGE_SOURCES/SURFACE_SOURCES, not an acceptance allowlist.
    for token, symbols in getattr(module, "AUDIT_TOKEN_SYMBOLS", {}).items():
        out.setdefault(token, set()).update(symbols)
    # The title fire atlas is not a surface record: it is a BG3 sheet the
    # runtime enables directly, so its thirty frame symbols are attached to the
    # token the shell actually names for it.
    frames = getattr(module, "FIRE_FRAMES", 0)
    if frames:
        out.setdefault("FIRE_ATLAS", set()).update(
            f"llMNTitleFireAnimFrame{i + 1}Sprite" for i in range(frames))
    return out


_TOKEN_RE = re.compile(r"NDS_MN_UI_KIT_(IMAGE|SURFACE)_(\w+)")
_SURFACE_ARRAY_RE = re.compile(
    r"static\s+const\s+u8\s+(kNdsMenu\w+)\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;",
    re.DOTALL)
_CASE_RE = re.compile(r"case\s+NDS_MENU_SHELL_SCREEN_(\w+)\s*:")


@dataclass
class ShellInventory:
    per_screen: dict[str, set[str]] = field(default_factory=dict)
    sites: dict[tuple[str, str], list[str]] = field(default_factory=dict)
    fire_screens: set[str] = field(default_factory=set)


def scan_shell(repo_root: Path) -> ShellInventory:
    shell_path = repo_root / SHELL_PATH
    seen: set[Path] = set()

    def read_translation_unit(path: Path) -> str:
        resolved = path.resolve()
        if resolved in seen:
            return ""
        if not path.is_file():
            raise AuditError(f"{path}: shell translation-unit include is missing")
        seen.add(resolved)
        source = path.read_text(errors="replace")
        chunks = [source]
        for match in re.finditer(
                r'^\s*#include\s+"([^"\n]+\.c)"\s*$', source, re.MULTILINE):
            chunks.append(read_translation_unit(path.parent / match.group(1)))
        return "\n".join(chunks)

    # Since 33ff8b4e476 the root is intentionally a small aggregator.  Read the
    # same implementation surface the compiler sees; otherwise a source split
    # silently makes every moved function/token look deleted to this gate.
    text = read_translation_unit(shell_path)
    clean = blank_comments_and_literals(text)
    lines = clean.split("\n")
    functions = top_level_functions(clean)
    owner: dict[int, str] = {}
    for func in functions:
        for line_no in range(func.first_line, func.last_line + 1):
            owner[line_no] = func.name

    inventory = ShellInventory()
    for screen in SCREENS:
        inventory.per_screen[screen.key] = set()

    def screen_of(function: str, line: int) -> str:
        matches = [s.key for s in SCREENS if s.shell_tag in function]
        if len(matches) != 1:
            raise AuditError(
                f"{SHELL_PATH}:{line}: function '{function}' references a kit "
                f"token but maps to {len(matches)} screens {matches}. Rename it "
                "to carry exactly one of "
                f"{[s.shell_tag for s in SCREENS]}, or extend SCREENS.")
        return matches[0]

    def record(screen_key: str, token: str, site: str) -> None:
        expanded = set()
        prefix = None
        if token.startswith("IMAGE_"):
            prefix = "IMAGE_"
        elif token.startswith("SURFACE_"):
            prefix = "SURFACE_"
        family = (TOKEN_FAMILIES.get(token[len(prefix):])
                  if prefix is not None else None)
        if family:
            expanded.update(f"{prefix}{name}" for name in family)
        else:
            expanded.add(token)
        for name in expanded:
            inventory.per_screen[screen_key].add(name)
            inventory.sites.setdefault((screen_key, name), []).append(site)

    # 1. The backdrop switch: which surface arrays each screen blits.
    arrays: dict[str, list[str]] = {}
    for name, body in _SURFACE_ARRAY_RE.findall(clean):
        arrays[name] = [f"SURFACE_{tok}" for _, tok in _TOKEN_RE.findall(body)]
    backdrop = next((f for f in functions
                     if f.name == "ndsMenuShellEnterBackdrop"), None)
    if backdrop is None:
        raise AuditError(f"{SHELL_PATH}: ndsMenuShellEnterBackdrop not found")
    active: list[str] = []
    for line_no in range(backdrop.first_line, backdrop.last_line + 1):
        line = lines[line_no - 1]
        cases = _CASE_RE.findall(line)
        if cases:
            active = []
            for case in cases:
                match = [s.key for s in SCREENS if s.backdrop_case == case]
                if len(match) == 1:
                    active.append(match[0])
            # A run of adjacent labels shares one body; collect them all.
            continue
        for array_name in arrays:
            if array_name in line:
                for key in active:
                    for token in arrays[array_name]:
                        record(key, token, f"{SHELL_PATH}:{line_no}")
        for _, token in _TOKEN_RE.findall(line):
            for key in active:
                record(key, f"SURFACE_{token}", f"{SHELL_PATH}:{line_no}")
        if "ndsUiKitBlitFireAtlas" in line or \
                "ndsPlatformSetTitleFireEnabled" in line:
            for key in active:
                record(key, "FIRE_ATLAS", f"{SHELL_PATH}:{line_no}")

    # `case NDS_MENU_SHELL_SCREEN_CSS:` and `...SSS:` share a body, and the
    # loop above resets `active` on every label line, so re-run the label pairs
    # by scanning for adjacent labels.
    label_runs: list[list[str]] = []
    run: list[str] = []
    for line_no in range(backdrop.first_line, backdrop.last_line + 1):
        cases = _CASE_RE.findall(lines[line_no - 1])
        if cases:
            for case in cases:
                match = [s.key for s in SCREENS if s.backdrop_case == case]
                run.extend(match)
        elif lines[line_no - 1].strip():
            if run:
                label_runs.append(run)
                run = []
    if run:
        label_runs.append(run)
    for group in label_runs:
        if len(group) > 1:
            union: set[str] = set()
            for key in group:
                union |= inventory.per_screen[key]
            for key in group:
                for token in union:
                    if token not in inventory.per_screen[key]:
                        record(key, token, f"{SHELL_PATH}:shared case")

    # 2. Every other token reference, attributed by enclosing function -- or,
    #    at FILE SCOPE, by the enclosing table's own name.
    #
    #    The tables matter because the shell keeps its state->surface maps
    #    there: `kNdsCssPanelSurface[slot][pkind]` is the character select's
    #    whole player-panel inventory and it is a file-scope initializer, so
    #    attributing by function alone would report every panel surface as
    #    drawn nowhere. A table is attributed exactly like a function -- by the
    #    screen tag in its name -- with one deliberate difference: a table
    #    matching no screen is SKIPPED rather than an error, because the
    #    backdrop switch already attributes the shared ones by the case they
    #    are blitted under (kNdsMenuStoneSurfaces is on two screens and names
    #    neither).
    table = None
    for line_no, line in enumerate(lines, start=1):
        function = owner.get(line_no)
        if function is None:
            match = re.search(r"\b(kNds\w+)\s*\[", line)
            if match is not None:
                names = [s.key for s in SCREENS if s.shell_tag in match.group(1)]
                table = names[0] if len(names) == 1 else None
            if table is not None:
                for kind, token in _TOKEN_RE.findall(line):
                    record(table, f"{kind}_{token}", f"{SHELL_PATH}:{line_no}")
            if line.strip().startswith("};"):
                table = None
            continue
        if function == "ndsMenuShellEnterBackdrop":
            continue
        site = f"{SHELL_PATH}:{line_no}"
        for kind, token in _TOKEN_RE.findall(line):
            record(screen_of(function, line_no), f"{kind}_{token}", site)
        for helper, tokens in HELPER_TOKENS.items():
            if helper in line:
                for token in tokens:
                    record(screen_of(function, line_no), f"IMAGE_{token}", site)
    return inventory


# ---------------------------------------------------------------------------
# Allowlist
# ---------------------------------------------------------------------------


@dataclass
class AllowEntry:
    screen: str
    kind: str        # missing | extra | substituted
    key: str         # source symbol, or kit token for an `extra`
    reason: str
    ruling: str
    status: str      # ruled | open
    substitute: str
    used: bool = False


def load_allowlist(repo_root: Path) -> list[AllowEntry]:
    path = repo_root / ALLOWLIST_PATH
    if not path.exists():
        return []
    raw = json.loads(path.read_text(errors="replace"))
    entries: list[AllowEntry] = []
    screen_keys = {s.key for s in SCREENS}
    for index, item in enumerate(raw.get("entries", [])):
        for required in ("screen", "kind", "key", "reason", "ruling"):
            if not item.get(required):
                raise AuditError(
                    f"{ALLOWLIST_PATH}: entry {index} is missing '{required}'. "
                    "Every accepted delta names the ruling that accepted it.")
        if item["screen"] not in screen_keys:
            raise AuditError(
                f"{ALLOWLIST_PATH}: entry {index} screen '{item['screen']}' is "
                f"not one of {sorted(screen_keys)}")
        if item["kind"] not in ("missing", "extra", "substituted"):
            raise AuditError(
                f"{ALLOWLIST_PATH}: entry {index} kind '{item['kind']}' is not "
                "missing|extra|substituted")
        status = item.get("status", "ruled")
        if status not in ("ruled", "open"):
            raise AuditError(
                f"{ALLOWLIST_PATH}: entry {index} status '{status}' is not "
                "ruled|open")
        if item["kind"] == "substituted" and not item.get("substitute"):
            raise AuditError(
                f"{ALLOWLIST_PATH}: entry {index} is a substitution and must "
                "name what we draw instead in 'substitute'.")
        entries.append(AllowEntry(item["screen"], item["kind"], item["key"],
                                  item["reason"], item["ruling"], status,
                                  item.get("substitute", "")))
    return entries


# ---------------------------------------------------------------------------
# The audit
# ---------------------------------------------------------------------------


@dataclass
class Delta:
    screen: str
    kind: str
    key: str
    detail: str
    allowed: AllowEntry | None = None


def run_audit(repo_root: Path) -> tuple[list[Delta], list[AllowEntry], dict]:
    module = load_bake_module(repo_root)
    token_symbols = bake_token_symbols(module)
    shell = scan_shell(repo_root)
    allow = load_allowlist(repo_root)
    deltas: list[Delta] = []
    report: dict = {"screens": {}}

    for screen in SCREENS:
        source_elements: list[SourceElement] = []
        for rel in screen.sources:
            path = repo_root / DECOMP_SRC / rel
            if not path.exists():
                raise AuditError(
                    f"{rel} not found under {DECOMP_SRC}. The BattleShip "
                    "reference is fetched by "
                    "scripts/fetch-battleship-reference.ps1.")
            source_elements.extend(scan_source_file(path, rel))

        live_sprites: dict[str, SourceElement] = {}
        dead_sprites: dict[str, SourceElement] = {}
        fills_live = 0
        for element in source_elements:
            if element.kind == "fill":
                if element.reachable:
                    fills_live += 1
                continue
            target = live_sprites if element.reachable else dead_sprites
            target.setdefault(element.symbol, element)
        dead_only = {name: el for name, el in dead_sprites.items()
                     if name not in live_sprites}

        our_tokens = shell.per_screen[screen.key]
        our_symbols: dict[str, set[str]] = {}
        for token in our_tokens:
            for symbol in token_symbols.get(token, ()):
                our_symbols.setdefault(symbol, set()).add(token)

        missing = sorted(set(live_sprites) - set(our_symbols))
        extra_tokens = sorted(
            token for token in our_tokens
            if token_symbols.get(token) and
            not (token_symbols[token] & set(live_sprites)))

        for symbol in missing:
            element = live_sprites[symbol]
            deltas.append(Delta(
                screen.key, "MISSING", symbol,
                f"{element.file}:{element.line} in {element.function}()"))
        for token in extra_tokens:
            symbols = sorted(token_symbols.get(token, ()))
            deltas.append(Delta(
                screen.key, "EXTRA", token,
                "drawn at " + ", ".join(
                    shell.sites.get((screen.key, token), ["?"])[:2]) +
                f" from {symbols}"))

        report["screens"][screen.key] = {
            "title": screen.title,
            "source_sprites_live": len(live_sprites),
            "source_sprites_unreachable": len(dead_only),
            "source_fill_panels_live": fills_live,
            "our_tokens": sorted(our_tokens),
            "our_symbols": sorted(our_symbols),
            "missing": missing,
            "extra": extra_tokens,
        }

    # Match deltas against the allowlist; substitutions are matched too, so a
    # substitution entry that no longer describes a real gap goes stale.
    #
    # A key spelled `re:<pattern>` matches by regex, which is how a whole CLASS
    # that shares one ruling stays one entry: "every per-fighter portrait for a
    # fighter P2-3 has not produced" is one decision, not eleven, and eleven
    # copies of the same ruling would be eleven places for it to rot.
    exact: dict[tuple[str, str], list[AllowEntry]] = {}
    patterns: list[tuple[AllowEntry, re.Pattern[str]]] = []
    for entry in allow:
        if entry.key.startswith("re:"):
            patterns.append((entry, re.compile(entry.key[3:])))
        else:
            exact.setdefault((entry.screen, entry.key), []).append(entry)
    for delta in deltas:
        wanted = {"MISSING": ("missing", "substituted"),
                  "EXTRA": ("extra",)}[delta.kind]
        for entry in exact.get((delta.screen, delta.key), []):
            if (not entry.used) and (entry.kind in wanted):
                entry.used = True
                delta.allowed = entry
                break
        if delta.allowed is not None:
            continue
        for entry, pattern in patterns:
            if (entry.screen == delta.screen) and (entry.kind in wanted) and \
                    pattern.fullmatch(delta.key):
                entry.used = True
                delta.allowed = entry
                break
    return deltas, allow, report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--repo-root", type=Path,
                        default=Path(__file__).resolve().parents[2])
    parser.add_argument("--json", type=Path, default=None,
                        help="write the machine-readable report here")
    parser.add_argument("--screen", action="append", default=None,
                        help="limit the printed report to these screen keys")
    args = parser.parse_args()

    try:
        deltas, allow, report = run_audit(args.repo_root.resolve())
    except AuditError as error:
        print(f"AUDIT ERROR: {error}", file=sys.stderr)
        return 2

    wanted = set(args.screen) if args.screen else {s.key for s in SCREENS}
    unexplained = [d for d in deltas if d.allowed is None]
    stale = [e for e in allow if not e.used]

    print("=== P2-1j screen asset-coverage audit "
          "(source -> shipped) ===")
    for screen in SCREENS:
        if screen.key not in wanted:
            continue
        info = report["screens"][screen.key]
        print(f"\n-- {screen.key}: {screen.title}")
        print(f"   source sprites reachable from FuncStart: "
              f"{info['source_sprites_live']}  "
              f"(unreachable, not audited: {info['source_sprites_unreachable']})"
              f"  fill panels: {info['source_fill_panels_live']}")
        print(f"   we draw {len(info['our_tokens'])} kit tokens covering "
              f"{len(info['our_symbols'])} source sprites")
        rows = [d for d in deltas if d.screen == screen.key]
        if not rows:
            print("   no deltas")
        for delta in rows:
            if delta.allowed is None:
                print(f"   {delta.kind:11s} {delta.key}")
                print(f"               {delta.detail}")
            else:
                entry = delta.allowed
                label = ("SUBSTITUTED" if entry.kind == "substituted"
                         else f"allowed {delta.kind.lower()}")
                mark = "OPEN" if entry.status == "open" else "ruled"
                suffix = (f" -> {entry.substitute}"
                          if entry.substitute else "")
                print(f"   [{mark}] {label}: {delta.key}{suffix}")
                print(f"               {entry.reason} [{entry.ruling}]")

    open_count = sum(1 for e in allow if e.used and e.status == "open")
    ruled_count = sum(1 for e in allow if e.used and e.status == "ruled")
    print(f"\nallowlist: {ruled_count} ruled, {open_count} open, "
          f"{len(stale)} stale")
    print(f"unexplained deltas: {len(unexplained)}")

    if args.json is not None:
        report["deltas"] = [
            {"screen": d.screen, "kind": d.kind, "key": d.key,
             "detail": d.detail,
             "allowed": None if d.allowed is None else {
                 "status": d.allowed.status, "ruling": d.allowed.ruling,
                 "kind": d.allowed.kind}}
            for d in deltas]
        report["stale_allowlist"] = [
            {"screen": e.screen, "kind": e.kind, "key": e.key} for e in stale]
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2) + "\n")

    if stale:
        print("\nSTALE ALLOWLIST ENTRIES -- the delta they excuse no longer "
              "exists; delete them:", file=sys.stderr)
        for entry in stale:
            print(f"  {entry.screen}: {entry.kind} {entry.key} "
                  f"[{entry.ruling}]", file=sys.stderr)
    if unexplained:
        print("\nUNEXPLAINED DELTAS -- ship the element, or record the ruling "
              f"in {ALLOWLIST_PATH}:", file=sys.stderr)
        for delta in unexplained:
            print(f"  {delta.screen}: {delta.kind} {delta.key} "
                  f"({delta.detail})", file=sys.stderr)
    if stale or unexplained:
        return 1
    print("\nscreen asset coverage: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())

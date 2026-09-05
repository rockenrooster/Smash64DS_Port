#!/usr/bin/env python3
"""Census of the build-configuration macros the compiled sources test versus
the macros the build can define.

The build's configuration reaches C through exactly two channels: the
``-D`` flags in the Makefile's CFLAGS and the generated config header the
Makefile writes with one ``echo '#define NDS_... $(NDS_...)'`` line per flag
(``nds_build_config.h``, force-included into every ARM9 TU, plus the
``nds_scene_harness_config.h`` twin).  A source that tests a flag the build
never defines is silently off: ``#if NDS_X`` reads 0 and ``#ifdef NDS_X``
reads false, and the code-first mode (no compile until the final pass) never
sees the ``-Wundef`` warning.  Until 2026-09-05 nothing defined the five 1P
arenas' ``NDS_P2_STAGE_<VENUE>`` guards, so their renderer capture tables were
compiled out of every configuration although the Makefile said they rode
``NDS_P2_1P_GAME``; this checker exists so that cannot recur.  The opposite
trap is an echo whose Makefile variable is never assigned: the header then
carries ``#define NDS_X`` with an EMPTY value, and the first ``#if NDS_X`` is
a hard "#if with no expression" error (Makefile, the NDS_P2_STAGE_YOSTER
note) -- reported here as EMPTY, before a source tests it.

Tested:  every ``NDS_*`` token on a ``#if`` / ``#ifdef`` / ``#ifndef`` /
``#elif`` line in ``src/**`` (``.c``, ``.h``, ``.inc``, ``.s``/``.S``) and
``include/**/*.h``, plus the decomp sources the ``src/import`` TUs textually
include (read as the build compiles them: overlay-patched, JP and non-NDS
arms dropped -- the ``check_reloc_symbol_census`` helpers).

Defined: every ``#define NAME`` the Makefile echoes into a generated header
(``$(foreach ...)``-derived names expanded from the list variable), every
``-D`` in the Makefile and the ``scripts/**/*.ps1`` harnesses, every
``#define NDS_*`` in a header or ``.inc`` under ``include/`` or ``src/``, every
literal ``#define NDS_*`` a ``scripts/**/*.py`` generator emits (the durable
record of a gitignored generated header), and every ``#define NDS_*`` in a
``.c`` or an included decomp source -- visible to the whole unity TU that
includes it (``nds_renderer.c``, ``scene_backend.c`` and ``taskman_seam.c``
are aggregates of ``.c`` slices; the include closure is pooled per root).

A tested macro with no definition is a hole unless ``ALLOW`` names it with
the reason it is deliberately never defined.  A build-defined macro no source
mentions is a dead flag (informational; ``--all`` lists them, tagged
``make-only`` when the Makefile itself reads the variable and ``harness`` when
a ``.ps1`` names it).

Usage:
    python scripts/check_build_flag_census.py            # holes, EMPTY, allowed
    python scripts/check_build_flag_census.py --strict   # exit 1 on a hole/EMPTY
    python scripts/check_build_flag_census.py --all      # + dead flags, all sites
    python scripts/check_build_flag_census.py --macro NDS_P2_STAGE_METAL
"""
from __future__ import annotations

import argparse
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "menus"))
import check_reloc_symbol_census as reloc  # noqa: E402

PREFIX = "NDS_"
MACRO_RE = re.compile(r"\b(NDS_[A-Za-z0-9_]+)\b")
COND_LINE_RE = re.compile(r"^\s*#\s*(?:if|ifdef|ifndef|elif)\b(.*)$")
DEFINE_RE = re.compile(r"^\s*#\s*define\s+(NDS_[A-Za-z0-9_]+)", re.M)
INCLUDE_RE = re.compile(r'^\s*#\s*include\s+["<]([^">]+)[">]', re.M)
# Makefile channels.
ECHO_DEFINE_RE = re.compile(r"#define\s+(NDS_[A-Za-z0-9_$()]+)")
ECHO_VALUE_VAR_RE = re.compile(r"#define\s+NDS_[A-Za-z0-9_$()]+\s+\$\(([A-Za-z_][A-Za-z0-9_]*)\)")
FOREACH_RE = re.compile(r"\$\(foreach\s+(\w+)\s*,\s*\$\((\w+)\)\s*,")
MAKE_ASSIGN_RE = re.compile(r"^(?:override\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*[:?+]?=\s*(.*)$")
MAKE_EVAL_ASSIGN_RE = re.compile(r"\$\(eval\s+([A-Za-z_][A-Za-z0-9_]*)\s*[:?+]?=")
DASH_D_RE = re.compile(r"(?<![\w-])-D([A-Za-z_][A-Za-z0-9_]*)")
# Python generator emits: literal names only.  A templated name such as
# ``NDS_P2_{PU}`` must not become a wildcard -- that would hide exactly the
# kind of hole this checker exists for.
PY_DEFINE_RE = re.compile(r"#define\s+(NDS_[A-Za-z0-9_]+)\b(?!\{)")

# Macros a source tests that the build deliberately never defines.  One line
# of reason each; a name here is not a hole.
ALLOW: dict[str, str] = {
    # emit_native_stage_runtime_rows.py --maxima writes it (templated name,
    # NDS_NATIVE_STAGE_BLOB_MAX_{suffix}) into nds_native_stage_blob_maxima
    # .generated.h, which nds_renderer_assets.c includes under __has_include;
    # the #ifdef's absent arm is the designed no-blob-stages state.
    "NDS_NATIVE_STAGE_BLOB_MAX_SEGMENT_COUNT":
        "optional generated header (__has_include), absent arm is by design",
}

SRC_EXTS = (".c", ".h", ".inc", ".s", ".S")


def repo_files(repo: str) -> list[str]:
    out: list[str] = []
    for ext in SRC_EXTS:
        out += glob.glob(os.path.join(repo, "src", "**", "*" + ext), recursive=True)
    out += glob.glob(os.path.join(repo, "include", "**", "*.h"), recursive=True)
    return sorted(set(out))


def rel(repo: str, path: str) -> str:
    return os.path.relpath(path, repo).replace(os.sep, "/")


def cond_macros(text: str) -> list[tuple[int, str]]:
    """(line, macro) for every NDS_ token on a preprocessor condition line."""
    out: list[tuple[int, str]] = []
    for i, line in enumerate(text.splitlines(), 1):
        m = COND_LINE_RE.match(line)
        if not m:
            continue
        body = m.group(1).split("//")[0]
        for name in dict.fromkeys(MACRO_RE.findall(body)):
            out.append((i, name))
    return out


def included_decomp(repo: str, tu_text: str) -> list[tuple[str, str]]:
    """(rel path, as-built text) for every decomp source a TU textually includes."""
    decomp_src = os.path.join(repo, "decomp", "BattleShip-main", "decomp", "src")
    out: list[tuple[str, str]] = []
    for m in reloc.INC_RE.finditer(tu_text):
        path = m.group(1)
        src = os.path.join(decomp_src, path)
        if not os.path.exists(src):
            continue
        out.append((path, reloc.as_built(reloc.patched_source(repo, path, src))))
    return out


class Sources:
    """Every compiled source under src/ and include/, read once."""

    def __init__(self, repo: str) -> None:
        self.repo = repo
        self.text: dict[str, str] = {}
        self.sites: dict[str, list[str]] = {}
        self.local: dict[str, set[str]] = {}
        self.includes: dict[str, set[str]] = {}
        self.mentioned: set[str] = set()
        for path in repo_files(repo):
            r = rel(repo, path)
            text = reloc.read(path)
            self.text[r] = text
            self.local[r] = set(DEFINE_RE.findall(text))
            self.includes[r] = {os.path.basename(p) for p in INCLUDE_RE.findall(text)}
            self.mentioned |= set(MACRO_RE.findall(text))
            for line, name in cond_macros(text):
                self.sites.setdefault(name, []).append(f"{r}:{line}")
            if r.startswith("src/import/"):
                for dpath, body in included_decomp(repo, text):
                    self.local[r] |= set(DEFINE_RE.findall(body))
                    self.mentioned |= set(MACRO_RE.findall(body))
                    for line, name in cond_macros(body):
                        self.sites.setdefault(name, []).append(f"{r} <- {dpath}~{line}")
        self.pool = self._unity_pools()

    def _unity_pools(self) -> dict[str, set[str]]:
        """file -> every NDS_ macro #defined anywhere in a unity TU that
        (transitively) includes it, or that it includes.  scene_backend.c
        includes reloc_backend.c includes reloc_backend_movement.c: a macro
        reloc_backend_fighter_model.c defines is visible to every later
        slice of that TU, so the closure is pooled per root."""
        by_base: dict[str, list[str]] = {}
        for r in self.text:
            by_base.setdefault(os.path.basename(r), []).append(r)
        included: set[str] = set()
        closure: dict[str, set[str]] = {}
        for r in self.text:
            seen: set[str] = set()
            stack = [r]
            while stack:
                cur = stack.pop()
                for base in self.includes.get(cur, ()):
                    for nxt in by_base.get(base, ()):
                        if nxt not in seen and nxt != r:
                            seen.add(nxt)
                            stack.append(nxt)
            closure[r] = seen
            included |= seen
        pool: dict[str, set[str]] = {r: set(self.local[r]) for r in self.text}
        for root in self.text:
            if root in included:
                continue
            names: set[str] = set(self.local[root])
            for member in closure[root]:
                names |= self.local[member]
            for member in closure[root] | {root}:
                pool[member] |= names
        return pool


def make_variables(makefile: str) -> tuple[set[str], dict[str, str]]:
    """(every assigned variable name, name -> first assigned value)."""
    assigned: set[str] = set(MAKE_EVAL_ASSIGN_RE.findall(makefile))
    values: dict[str, str] = {}
    for line in makefile.splitlines():
        if line.lstrip().startswith("#"):
            continue
        m = MAKE_ASSIGN_RE.match(line.strip())
        if m:
            assigned.add(m.group(1))
            values.setdefault(m.group(1), m.group(2).strip())
    return assigned, values


def makefile_defines(makefile: str) -> tuple[dict[str, str], list[str], list[str]]:
    """(macro -> 'Makefile:<line>' for every #define the Makefile echoes into a
    generated header, unexpandable names, EMPTY findings).  foreach-derived
    names are expanded from the list variable; an echo whose value variable
    is never assigned in the Makefile is EMPTY."""
    assigned, values = make_variables(makefile)
    defined: dict[str, str] = {}
    unresolved: list[str] = []
    empty: list[str] = []
    for lineno, line in enumerate(makefile.splitlines(), 1):
        if line.lstrip().startswith("#") or "echo" not in line:
            continue
        fe = FOREACH_RE.search(line)
        for raw in ECHO_DEFINE_RE.findall(line):
            names = [raw]
            if fe and f"$({fe.group(1)})" in raw:
                words = values.get(fe.group(2), "").split()
                names = [raw.replace(f"$({fe.group(1)})", w) for w in words]
            for name in names:
                if "$" in name:
                    unresolved.append(f"Makefile:{lineno} {name}")
                else:
                    defined.setdefault(name, f"Makefile:{lineno}")
        for var in ECHO_VALUE_VAR_RE.findall(line):
            if var not in assigned and not (fe and var == fe.group(1)):
                empty.append(f"Makefile:{lineno} $({var}) is never assigned")
    return defined, unresolved, empty


def dash_d_defines(repo: str, makefile: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for lineno, line in enumerate(makefile.splitlines(), 1):
        if line.lstrip().startswith("#"):
            continue
        for name in DASH_D_RE.findall(line):
            out.setdefault(name, f"Makefile:{lineno} -D")
    for path in glob.glob(os.path.join(repo, "scripts", "**", "*.ps1"), recursive=True):
        for lineno, line in enumerate(reloc.read(path).splitlines(), 1):
            if line.lstrip().startswith("#"):
                continue
            for name in DASH_D_RE.findall(line):
                out.setdefault(name, f"{rel(repo, path)}:{lineno} -D")
    return out


def header_defines(sources: Sources) -> dict[str, str]:
    """Every #define NDS_* in a header or .inc anywhere under include/ or src/
    (global: any TU may include it)."""
    out: dict[str, str] = {}
    for r, names in sorted(sources.local.items()):
        if r.endswith((".c", ".s", ".S")):
            continue
        for name in names:
            out.setdefault(name, r)
    return out


def generator_defines(repo: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for path in sorted(glob.glob(os.path.join(repo, "scripts", "**", "*.py"), recursive=True)):
        if os.path.basename(path) == os.path.basename(__file__):
            continue
        for name in PY_DEFINE_RE.findall(reloc.read(path)):
            out.setdefault(name, rel(repo, path))
    return out


def harness_mentions(repo: str) -> set[str]:
    out: set[str] = set()
    for path in glob.glob(os.path.join(repo, "scripts", "**", "*.ps1"), recursive=True):
        out |= set(MACRO_RE.findall(reloc.read(path)))
    return out


def census(repo: str) -> dict:
    makefile = reloc.read(os.path.join(repo, "Makefile"))
    sources = Sources(repo)
    build, unresolved, empty = makefile_defines(makefile)
    dashd = dash_d_defines(repo, makefile)
    headers = header_defines(sources)
    generators = generator_defines(repo)
    defined: dict[str, str] = {}
    for table in (build, dashd, headers, generators):
        for name, where in table.items():
            defined.setdefault(name, where)

    def covered(name: str, site: str) -> bool:
        if name in defined:
            return True
        file = site.split(" <- ")[0].rsplit(":", 1)[0]
        return name in sources.pool.get(file, ())

    holes: dict[str, list[str]] = {}
    allowed: dict[str, list[str]] = {}
    for name, where in sorted(sources.sites.items()):
        open_sites = [s for s in where if not covered(name, s)]
        if not open_sites:
            continue
        (allowed if name in ALLOW else holes)[name] = open_sites

    # Dead: build-defined (config header or -D) but never mentioned by any
    # compiled source.  Include guards are the header's own business.
    ps1 = harness_mentions(repo)
    dead: dict[str, str] = {}
    for name in list(build) + list(dashd):
        if not name.startswith(PREFIX) or name.endswith("_H") or name in sources.mentioned:
            continue
        tags = []
        if len(re.findall(rf"\${{?\(?{name}\b", makefile)) > 1:
            tags.append("make-only")
        if name in ps1:
            tags.append("harness")
        dead[name] = " ".join(tags) or "unused"
    return {
        "sites": sources.sites, "defined": defined, "holes": holes, "allowed": allowed,
        "dead": dead, "unresolved": unresolved, "empty": empty,
        "build_count": len(build) + len(dashd),
        "local": {r: names for r, names in sources.local.items() if names},
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--repo-root", default=os.path.join(HERE, ".."))
    ap.add_argument("--strict", action="store_true", help="exit 1 on any hole or EMPTY echo")
    ap.add_argument("--all", action="store_true", help="list dead flags and every site")
    ap.add_argument("--macro", help="print every test site and the definition of one macro")
    args = ap.parse_args()
    repo = os.path.abspath(args.repo_root)
    result = census(repo)

    if args.macro:
        name = args.macro
        tu_local = sorted(r for r, names in result["local"].items() if name in names)
        where = result["defined"].get(name) or (
            f"{', '.join(tu_local)} (TU-local #define)" if tu_local else "(nowhere)")
        print(f"{name}: defined at {where}")
        for site in result["sites"].get(name, []):
            print(f"  tested {site}")
        return 0

    for name, where in sorted(result["holes"].items()):
        shown = where if args.all else where[:4]
        more = "" if args.all or len(where) <= 4 else f" (+{len(where) - 4})"
        print(f"HOLE  {name:<48} {len(where):3d} sites: {', '.join(shown)}{more}")
    for item in result["empty"]:
        print(f"EMPTY {item}")
    for item in result["unresolved"]:
        print(f"unresolved echo (extend makefile_defines): {item}")
    for name, where in sorted(result["allowed"].items()):
        print(f"allow {name:<48} {len(where):3d} sites: {ALLOW[name]}")
    if args.all:
        for name, tag in sorted(result["dead"].items()):
            print(f"dead  {name:<48} {tag:<18} {result['defined'][name]}")
    print(
        f"BUILD_FLAG_CENSUS holes={len(result['holes'])} empty={len(result['empty'])} "
        f"allowed={len(result['allowed'])} dead={len(result['dead'])} "
        f"tested={len(result['sites'])} build_defined={result['build_count']} "
        f"defined={len(result['defined'])}"
    )
    if args.strict and (result["holes"] or result["empty"] or result["unresolved"]):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

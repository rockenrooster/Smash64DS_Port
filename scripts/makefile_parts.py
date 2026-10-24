"""The Makefile as make reads it.

P8 G split the Makefile by topic: the root Makefile includes each fragment
in mk/ in place (`include $(PROJECT_ROOT)/mk/<name>.mk`). Tools that scan the
Makefile for rules, knobs or file lists read `makefile_text()`, the root file
with every such include replaced by its fragment; tools that edit it change
the one part that holds their anchor (`part_containing()`).
"""
from __future__ import annotations

import re
from pathlib import Path

_INCLUDE = re.compile(r"^include \$\(PROJECT_ROOT\)/mk/([\w.-]+\.mk)[ \t]*$", re.M)


def _read(path: Path, errors: str) -> str:
    return path.read_text(encoding="utf-8", errors=errors)


def makefile_parts(root) -> list[Path]:
    """The root Makefile, then each fragment in the order it is included."""
    root = Path(root)
    text = _read(root / "Makefile", "strict")
    return [root / "Makefile"] + [root / "mk" / name for name in _INCLUDE.findall(text)]


def expand_includes(text: str, read_fragment) -> str:
    """`text` (the root Makefile) with every mk/ include replaced by
    `read_fragment(name)`, e.g. a fragment read from the git index."""
    return _INCLUDE.sub(lambda m: read_fragment(m.group(1)).rstrip("\n"), text)


def makefile_text(root, errors: str = "strict") -> str:
    """The root Makefile with every mk/ include replaced by its fragment."""
    root = Path(root)
    return expand_includes(_read(root / "Makefile", errors),
                           lambda name: _read(root / "mk" / name, errors))


def makefile_sites(root) -> list[str]:
    """For each line of `makefile_text(root)`, the part and line it stands on
    (`mk/config.mk:12`), so a tool reporting a line names the file."""
    root = Path(root)
    sites = []
    for lineno, line in enumerate(_read(root / "Makefile", "replace").split("\n"), 1):
        m = _INCLUDE.fullmatch(line)
        if not m:
            sites.append(f"Makefile:{lineno}")
            continue
        body = _read(root / "mk" / m.group(1), "replace").rstrip("\n")
        sites.extend(f"mk/{m.group(1)}:{n}" for n in range(1, body.count("\n") + 2))
    return sites


def part_containing(root, needle: str) -> Path:
    """The one part whose text holds `needle` (an edit's anchor)."""
    hits = [p for p in makefile_parts(root) if needle in _read(p, "strict")]
    if len(hits) != 1:
        raise ValueError(
            f"{len(hits)} Makefile parts hold {needle[:60]!r}: "
            + ", ".join(str(p.relative_to(Path(root))) for p in hits))
    return hits[0]

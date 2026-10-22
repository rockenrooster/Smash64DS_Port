"""Small C-source extraction helpers shared by host regression tests."""
import re
from pathlib import Path


DECOMP = Path(__file__).resolve().parents[2] / "decomp/BattleShip-main/decomp/src"
SRC = Path(__file__).resolve().parents[2] / "src"

# Files split into topic slices of one translation unit (readability Phase C):
# the unity parent includes each one's slices right after it, in this order.
SPLIT_SOURCES = {
    "nds/nds_renderer_native_common.c": ("nds/nds_renderer.c", (
        "nds_renderer_native_rebirth_halo.c",
        "nds_renderer_native_dl_walk.c",
        "nds_renderer_native_entry.c",
        "nds_renderer_native_production_runs.c",
        "nds_renderer_native_fighter_packet.c",
        "nds_renderer_native_lean_lists.c",
        "nds_renderer_native_lean_entries.c",
        "nds_renderer_native_hierarchy.c",
    )),
}


def source(path):
    """A source file's text; a split file reads whole, its slices appended in
    the order the unity parent includes them."""
    path = Path(path).resolve()
    for name, (parent, slices) in SPLIT_SOURCES.items():
        if path == (SRC / name).resolve():
            break
    else:
        return path.read_text(encoding="utf-8")
    includes = re.findall(r'^#include "([^"]+)"',
                          (SRC / parent).read_text(encoding="utf-8"), re.M)
    at = includes.index(path.name)
    if tuple(includes[at + 1:at + 1 + len(slices)]) != slices:
        raise AssertionError(f"{parent} no longer includes the slices of "
                             f"{path.name} right after it, in order")
    return "".join(p.read_text(encoding="utf-8")
                   for p in [path] + [path.with_name(s) for s in slices])


def clean(source):
    return re.sub(r"/\*.*?\*/|//[^\n]*", lambda m: " " * len(m[0]),
                  source, flags=re.S)


def braced(source, pattern, semicolon=False):
    plain = clean(source)
    start = re.search(pattern, plain, re.M)
    if start is None:
        raise AssertionError(f"Missing source declaration: {pattern}")
    opening = plain.index("{", start.start())
    depth = 1
    end = opening + 1
    while depth:
        depth += (plain[end] == "{") - (plain[end] == "}")
        end += 1
    if semicolon:
        end = plain.index(";", end) + 1
    return source[start.start():end]


def function(source, name):
    return braced(source, rf"^[\w *]+\b{name}\([^;]*?\)\s*\{{")


def original_enum(path, name):
    return braced((DECOMP / path).read_text(), rf"typedef enum {name}\s*\{{", True)

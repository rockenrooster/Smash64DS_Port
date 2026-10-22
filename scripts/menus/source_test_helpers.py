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
    "port/reloc_backend_assets.c": ("port/reloc_backend.c", (
        "reloc_backend_symbol_tables.c",
        "reloc_backend_asset_ids.c",
        "reloc_backend_resolve.c",
        "reloc_backend_normalize.c",
        "reloc_backend_extern_trees.c",
        "reloc_backend_motion_cache.c",
        "reloc_backend_force_load.c",
    )),
}


def slice_paths(path):
    """The files a source reads from: itself, then its slices in the order
    the unity parent includes them (only itself when it is not split)."""
    path = Path(path).resolve()
    for name, (parent, slices) in SPLIT_SOURCES.items():
        if path.as_posix().endswith("/src/" + name):
            src = Path(path.as_posix()[:-len(name)])  # this tree's src/
            break
    else:
        return [path]
    includes = re.findall(r'^#include "([^"]+)"',
                          (src / parent).read_text(encoding="utf-8"), re.M)
    at = includes.index(path.name)
    if tuple(includes[at + 1:at + 1 + len(slices)]) != slices:
        raise AssertionError(f"{parent} no longer includes the slices of "
                             f"{path.name} right after it, in order")
    return [path] + [path.with_name(s) for s in slices]


def source(path):
    """A source file's text; a split file reads whole, its slices appended in
    the order the unity parent includes them."""
    return "".join(p.read_text(encoding="utf-8") for p in slice_paths(path))


SLICE_MARK = "/*@@slice {}@@*/\n"


def read_for_edit(path):
    """source() for a tool that edits the text: LF line endings, and a marker
    line before each slice so write_for_edit can put the pieces back."""
    parts = []
    for i, p in enumerate(slice_paths(path)):
        text = p.read_bytes().decode("utf-8").replace("\r\n", "\n")
        parts.append((SLICE_MARK.format(p.name) if i else "") + text)
    return "".join(parts)


def write_for_edit(path, text):
    """Write read_for_edit's text back, each slice to its own file with that
    file's line endings; untouched files are not rewritten."""
    paths = slice_paths(path)
    pieces = re.split(r"/\*@@slice ([\w.]+)@@\*/\n", text)
    names = [p.name for p in paths]
    if pieces[1::2] != names[1:]:
        raise AssertionError(f"{Path(path).name}: slice markers were edited")
    for p, piece in zip(paths, [pieces[0]] + pieces[2::2]):
        raw = p.read_bytes()
        if b"\r\n" in raw:
            piece = piece.replace("\n", "\r\n")
        if piece.encode("utf-8") != raw:
            p.write_bytes(piece.encode("utf-8"))


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

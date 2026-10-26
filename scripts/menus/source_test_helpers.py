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
        "nds_renderer_native_entry_textures.c",
        "nds_renderer_native_entry_packet.c",
        "nds_renderer_native_production_runs.c",
        "nds_renderer_native_fighter_packet.c",
        "nds_renderer_native_lean_lists.c",
        "nds_renderer_native_lean_entries.c",
        "nds_renderer_native_hierarchy.c",
    )),
    "nds/nds_renderer_textures_effects.c": ("nds/nds_renderer.c", (
        "nds_renderer_texture_scene.c",
        "nds_renderer_particles.c",
        "nds_renderer_world_quads.c",
        "nds_renderer_texture_convert.c",
        "nds_renderer_texture_resolve.c",
        "nds_renderer_submit.c",
        "nds_renderer_fighter_vram.c",
    )),
    "port/reloc_backend_source_shims.c": ("port/reloc_backend.c", (
        "reloc_backend_shims_fighter_status.c",
        "reloc_backend_shims_fighter_effects.c",
        "reloc_backend_shims_fighter_parts.c",
        "reloc_backend_shims_map_collision.c",
    )),
    "port/reloc_backend_mp_collision.c": ("port/reloc_backend.c", (
        "reloc_backend_mp_lines.c",
        "reloc_backend_mp_walls.c",
        "reloc_backend_mp_floors.c",
    )),
    "nds/nds_menu_shell_css.c": ("nds/nds_menu_shell.c", (
        "nds_menu_shell_css_panels.c",
        "nds_menu_shell_css_cursor.c",
        "nds_menu_shell_css_modes.c",
    )),
    # A translation unit of its own includes its slices last.
    "nds/nds_ifcommon_oam.c": ("nds/nds_ifcommon_oam.c", (
        "nds_ifcommon_oam_status.c",
        "nds_ifcommon_oam_sobjs.c",
        "nds_ifcommon_oam_pause.c",
    )),
    "port/reloc_backend_movement.c": ("port/reloc_backend.c", (
        "reloc_backend_movement_moveset.c",
        "reloc_backend_stage_draw_loop.c",
    )),
    "port/renderer_adapter_matrix.c": ("port/reloc_backend_renderer_dl.c", (
        "renderer_adapter_matrix_math.c",
        "renderer_adapter_matrix_world.c",
        "renderer_adapter_matrix_owner.c",
    )),
    "nds/nds_renderer_assets.c": ("nds/nds_renderer.c", (
        "nds_renderer_owner_tables.c",
        "nds_renderer_owner_runtime.c",
        "nds_renderer_native_pools.c",
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
    "port/renderer_adapter_stage.c": ("port/reloc_backend_renderer_dl.c", (
        "renderer_adapter_stage_packet.c",
        "renderer_adapter_stage_lanes.c",
        "renderer_adapter_stage_submit.c",
    )),
}

# Phases of a function too long for one file, which the function includes in
# place inside its body: source() reads each where its #include stands.
INLINE_FRAGMENTS = {
    "port/renderer_adapter_stage_submit.c": (
        "renderer_adapter_stage_claims.inc",
        "renderer_adapter_stage_submits.inc",
    ),
    "nds/nds_platform.c": (
        "nds_platform_gl_wraps.c",
    ),
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
    if parent == name:
        if tuple(includes[-len(slices):]) != slices:
            raise AssertionError(f"{name} no longer includes its slices last, in order")
        return [path] + [path.with_name(s) for s in slices]
    at = includes.index(path.name)
    if tuple(includes[at + 1:at + 1 + len(slices)]) != slices:
        raise AssertionError(f"{parent} no longer includes the slices of "
                             f"{path.name} right after it, in order")
    return [path] + [path.with_name(s) for s in slices]


def fragments(path):
    """The in-function fragments a file includes (INLINE_FRAGMENTS)."""
    for name, names in INLINE_FRAGMENTS.items():
        if Path(path).resolve().as_posix().endswith("/src/" + name):
            return names
    return ()


def inlined(path):
    """A file's text with its in-function fragments read in place."""
    text = Path(path).read_text(encoding="utf-8")
    for name in fragments(path):
        line = f'#include "{name}"\n'
        if text.count(line) != 1:
            raise AssertionError(f"{Path(path).name} no longer includes {name} once")
        text = text.replace(line, Path(path).with_name(name).read_text(encoding="utf-8"))
    return text


def source(path):
    """A source file's text; a split file reads whole, its slices appended in
    the order the unity parent includes them and its in-function fragments
    where they are included."""
    return "".join(inlined(p) for p in slice_paths(path))


SLICE_MARK = "/*@@slice {}@@*/\n"


def read_for_edit(path):
    """source() for a tool that edits the text: LF line endings, and a marker
    line before each slice so write_for_edit can put the pieces back."""
    parts = []
    for i, p in enumerate(slice_paths(path)):
        if fragments(p):
            raise AssertionError(f"{p.name} includes in-function fragments; edit them directly")
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
    """A function's definition, verbatim but for NDS_COLD and
    NDS_RENDERER_COLD (include/nds/nds_cold.h): those markers only place
    ARM code (Thumb, -Os) and mean nothing to a host harness."""
    return re.sub(r"\bNDS_(?:RENDERER_)?COLD\s+", "", _definition(source, name))


def _definition(source, name):
    try:
        return braced(source, rf"^[\w *]+\b{name}\([^;]*?\)\s*\{{")
    except AssertionError:
        pass
    # The name at column 0 with its return type and attributes on the lines
    # above (`NDS_RENDERER_COLD static s32 __attribute__((cold))` / `name(`):
    # the definition starts at the first of those lines.
    plain = clean(source)
    head = re.search(rf"^{name}\([^;]*?\)\s*\{{", plain, re.M)
    if head is None:
        raise AssertionError(f"Missing source declaration: {name}")
    start = head.start()
    while start > 0:
        previous = plain.rfind("\n", 0, start - 1) + 1
        line = plain[previous:start - 1]
        if (not line.strip()) or line.lstrip().startswith("#") or \
                line.rstrip().endswith((";", "}", "{")):
            break
        start = previous
    opening = plain.index("{", head.start())
    depth = 1
    end = opening + 1
    while depth:
        depth += (plain[end] == "{") - (plain[end] == "}")
        end += 1
    return source[start:end]


def original_enum(path, name):
    return braced((DECOMP / path).read_text(), rf"typedef enum {name}\s*\{{", True)

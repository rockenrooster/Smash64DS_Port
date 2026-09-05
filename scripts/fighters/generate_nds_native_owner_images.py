#!/usr/bin/env python3
"""Emit each P2-3 fighter's native-owner tables as ONE struct image.

WHY THIS EXISTS (board row P2-3r4, P2_PLAN law 3 -- the budget law).

Every owner's generated geometry currently lives in the ARM9 binary as a set of
`static const` arrays, and on this hardware the binary costs the taskman arena
one-for-one: the arena is `calloc`'d from whatever the heap has left. Measured
on the shell target at the battle's own high water, roster 0 leaves 91,664 B of
arena headroom, roster 1 (+Luigi) 57,136 B, and roster 2 (+Donkey Kong) 13,840 B
-- at which point the battle takes a data abort in
`ifCommonCountdownMakeInterface` because the countdown interface's allocation
comes back NULL. The native-owner tables are 87,576 B of that binary today with
two fighters and a variant; ten more fighters at this rate is 400+ KB that no DS
RAM budget can hold. The tables have to leave the image and be loaded for the
fighters a match actually uses.

WHY A STRUCT IMAGE RATHER THAN A HAND-WRITTEN SERIALIZER. The obvious approach
-- pack each array into a blob with a header of offsets -- means writing, by
hand, the byte layout of six different element structs and keeping it in step
with the C definitions forever. A wrong padding assumption there is silent: the
geometry still draws, just wrongly. So the layout stays the COMPILER's. Each
owner+detail becomes one `struct` whose members are exactly those arrays; the
image is that struct's own bytes, extracted from a standalone object file with
`objcopy`, and every offset the runtime needs is `offsetof` on the same type the
image was built from. There is no second description of the layout to drift.

WHAT IS DELIBERATELY NOT IN THE IMAGE.

  * `PreparedDense` is mutable scratch (the GX-packed vertex the draw path
    writes), not content. It stays in RAM and belongs to the SLOT a fighter
    occupies, not to the fighter kind -- four slots, not twelve kinds.
  * Mario and Fox keep their frozen combined export byte-identical, per
    `docs/p2/P2-3-fighter-production.md`'s own bootstrap contract. This tool
    reads P2-3 owners only.

The generated header is the single ABI the runtime and the image share.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import sys

_HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(_HERE))

import generate_nds_native_owners as owners  # noqa: E402
import _paths  # noqa: E402
from native_owner_image_arrays import NATIVE_OWNER_IMAGE_ARRAYS  # noqa: E402


def _owner_title(owner_name: str) -> str:
    """Shared C kind capitalization (see generate_nds_native_owners)."""
    return owners._owner_title(owner_name)


# The P2-3 owners whose tables this tool moves out of the ARM9 image. Mario and
# Fox are absent on purpose (frozen combined export).
P2_IMAGE_OWNERS = ("luigi", "donkey", "captain", "samus", "link", "pikachu",
                   "yoshi", "ness", "purin", "kirby", "mmario", "nmario", "nfox", "ndonkey", "nsamus", "nlink", "nyoshi", "ncaptain", "nkirby", "npikachu", "npurin", "nness")
DETAILS = ("high", "low")


def _rows(values: list[str]) -> list[str]:
    """One initializer row per line, in the generator's own house style."""
    return [f"    {value}," for value in values]


def _bake_dense_normal_word(rgba: int) -> int:
    """The runtime bake's output word for one dense vertex, computed here.

    Bit-identical replica of `ndsRendererR2BuildDenseNormals` in
    `src/nds/nds_renderer_native_common.c`: each of the packed normal bytes
    (rgba>>24, >>16, >>8 as s8) scales by 0x1FF/127 with C truncation toward
    zero, clamps to [-512, 511], and packs masked-10-bit. The bake input is
    the already-imaged `dense_vertices` rgba, so this is fully determined at
    generation time; the VERIFY path compares these bytes against the arrays
    the bake used to fill.
    """
    words: list[int] = []
    for shift in (24, 16, 8):
        src = (rgba >> shift) & 0xFF
        if src >= 0x80:
            src -= 0x100
        scaled = abs(src * 0x1FF) // 127
        if src < 0:
            scaled = -scaled
        if scaled > 511:
            scaled = 511
        if scaled < -512:
            scaled = -512
        words.append(scaled & 0x3FF)
    return ((words[0] | (words[1] << 10) | (words[2] << 20)) & 0xFFFFFFFF)


def _member_values(
        context: dict[str, object]) -> list[tuple[str, str, list[str], str]]:
    """(C type, member name, initializer rows, guard) for every array.

    The value formats are the ones `render_p2_owner_runtime_program` already
    emits; keeping them identical is what makes the image byte-comparable
    against the arrays it replaces.

    `guard` is a preprocessor condition (or "" for none) copied from the
    condition the RUNTIME tables struct uses for that same array. It has to be
    the same condition, not merely a similar one: an array the runtime does not
    reference must not be in the image either, or the image carries arena bytes
    nothing reads, and an array the runtime DOES reference must be present or
    the arrays cannot be deleted. Both sides read these conditions out of
    `nds_build_config.h`, so there is one answer per build.
    """
    owner_name = str(context["owner_name"])
    detail = str(context["detail"])
    state = context["state"]
    sequence = context["sequence"]
    vertex = context["vertex"]
    triangles = context["triangles"]
    runs = context["runs"]
    epochs = context["epochs"]
    dense_vertices = context["dense_vertices"]
    dense_color_sources = context["dense_color_sources"]
    action_dense_spans = context["action_dense_spans"]
    packed_corners = context["packed_corners"]
    run_first_corner = context["run_first_corner"]
    run_first_unique = context["run_first_unique"]
    run_unique_count = context["run_unique_count"]
    run_unique_dense = context["run_unique_dense"]
    direct_policies = context["direct_epoch_policies"]
    primitive_streams = context["primitive_streams"]

    color_guard = ("!NDS_R2_FIGHTER_HW_LIGHT || "
                   "NDS_RENDERER_M2_DETAILED_LEDGER")

    members: list[tuple[str, str, list[str], str]] = [
        ("NDSNativeStateDelta", "state_deltas",
         [f"{{ 0x{w0:08x}u, 0x{w1:08x}u, {effect}u, {{ 0u, 0u, 0u }} }}"
          for w0, w1, effect in state], ""),
        ("u8", "state_sequence", [f"{value}u" for value in sequence], ""),
        ("NDSNativeVertexAction", "vertex_actions",
         [f"{{ {kind}u, {command}u, {index}u, {count}u, "
          f"0x{offset:08x}u, {s}, {t} }}"
          for kind, command, index, count, offset, s, t in vertex], ""),
        ("u8", "epoch_direct_policy",
         [f"0x{value:02x}u" for value in direct_policies], ""),
        ("NDSNativeDenseVertex", "dense_vertices",
         ["{{ 0x{:08x}u, {}, {}, {}u, {}u, 0u }}".format(
             rgba, s, t, binding, cache_slot)
           for x, y, z, s, t, binding, cache_slot, rgba in dense_vertices], ""),
        ("u32", "dense_normals",
         [f"0x{_bake_dense_normal_word(rgba):08x}u"
          for x, y, z, s, t, binding, cache_slot, rgba in dense_vertices], ""),
        ("u16", "action_dense_spans",
         [f"0x{value:04x}u" for value in action_dense_spans], ""),
        ("u16", "dense_color_source",
         [f"{value}u" for value in dense_color_sources], color_guard),
        ("u16", "packed_corners",
         [f"0x{value:04x}u" for value in packed_corners], ""),
        ("u16", "run_first_corner",
         [f"{value}u" for value in run_first_corner], ""),
        ("u16", "run_first_unique",
         [f"{value}u" for value in run_first_unique], ""),
        ("u8", "run_unique_count",
         [f"{value}u" for value in run_unique_count], ""),
        ("u16", "run_unique_dense",
         [f"{value}u" for value in run_unique_dense], ""),
        ("u16", "triangles", [f"0x{value:04x}u" for value in triangles], ""),
        ("NDSNativeRun", "runs",
         [f"{{ {first}u, {count}u, {submit_class}u, 0x{mask:08x}u }}"
          for first, count, submit_class, mask in runs], ""),
    ]

    # Task 56 primitive streams. Both compiled modes are emitted under their
    # own guard, exactly as `render_p2_owner_runtime_program` emits the arrays:
    # the mode is a build flag, so which stream is in the image is decided by
    # the same `nds_build_config.h` the renderer reads.
    for mode in (1, 2):
        (run_group_first, run_group_count, group_type,
         group_first_vertex, group_vertex_count,
         primitive_vertices) = primitive_streams[mode]
        guard = f"NDS_TASK56_FIGHTER_PRIMITIVES == {mode}"
        members += [
            ("u16", f"primitive_group_first_m{mode}",
             [f"{value}u" for value in run_group_first], guard),
            ("u8", f"primitive_group_count_m{mode}",
             [f"{value}u" for value in run_group_count], guard),
            ("u8", f"primitive_group_type_m{mode}",
             [f"{value}u" for value in group_type], guard),
            ("u16", f"primitive_group_first_vertex_m{mode}",
             [f"{value}u" for value in group_first_vertex], guard),
            ("u8", f"primitive_group_vertex_count_m{mode}",
             [f"{value}u" for value in group_vertex_count], guard),
            ("u16", f"primitive_vertices_m{mode}",
             [f"0x{value:04x}u" for value in primitive_vertices], guard),
        ]

    members += [
        ("NDSNativeEpoch", "epochs",
         ["{{ {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u }}".format(*row)
          for row in epochs], ""),
    ]
    del owner_name, detail
    return members


def _array_symbol(owner_name: str, detail: str, member: str) -> str:
    """The in-binary array a member is the image copy of.

    Emitted into the header rather than reconstructed in C so the equivalence
    check names the arrays by the same rule that produced them. `_m1`/`_m2`
    mode suffixes are stripped: the runtime arrays are already guarded to a
    single mode and share one name.
    """
    suffix = "Low" if detail == "low" else ""
    return (f"sNdsNative{_owner_title(owner_name)}Fighter"
            f"{_array_stem(member)}{suffix}")


def _array_stem(member: str) -> str:
    """The owner generator's array-name stem for an image member.

    `_m1`/`_m2` mode suffixes are stripped: the runtime arrays are already
    guarded to a single Task 56 mode and share one name.
    """
    stem = member
    if stem.endswith("_m1") or stem.endswith("_m2"):
        stem = stem[:-3]
    return "".join(part.title() for part in stem.split("_"))


def _image_type(owner_name: str, detail: str) -> str:
    return f"NDSNative{_owner_title(owner_name)}{detail.title()}Image"


def render_header(contexts: dict[tuple[str, str], dict[str, object]]) -> str:
    """The one ABI the image and the runtime share."""
    lines = [
        "/* Generated by scripts/fighters/generate_nds_native_owner_images.py."
        "  Do not edit. */",
        "/* P2-3r4: each P2-3 owner's native tables as ONE struct, so the image",
        " * bytes are the compiler's own layout and every runtime offset is",
        " * offsetof on this same type.  See the generator's header comment for",
        " * why the tables leave the ARM9 binary at all. */",
        "",
        "#ifndef NDS_NATIVE_FIGHTER_IMAGE_GENERATED_H",
        "#define NDS_NATIVE_FIGHTER_IMAGE_GENERATED_H",
        "",
        "/* The element types this header builds arrays of, and the build",
        " * config whose flags decide which arrays exist.  Included here so a",
        " * translation unit can include this one alone and be correct, and so",
        " * the image TU and the renderer cannot be compiled against different",
        " * answers to the same question. */",
        "#include <nds_build_config.h>",
        "#include <nds/nds_native_fighter_tables.h>",
        "",
        "/* Image owner slots. Dense and independent of the renderer's own",
        " * owner numbering: only P2-3 owners have images. */",
    ] + [
        f"#define NDS_NATIVE_IMAGE_SLOT_{name.upper()} {index}u"
        for index, name in enumerate(P2_IMAGE_OWNERS)
    ] + [
        f"#define NDS_NATIVE_IMAGE_OWNER_SLOTS {len(P2_IMAGE_OWNERS)}u",
        "",
    ]
    for (owner_name, detail), context in sorted(contexts.items()):
        members = _member_values(context)
        type_name = _image_type(owner_name, detail)
        lines += [
            f"/* {_owner_title(owner_name)} {detail} native-owner image. */",
            f"typedef struct {type_name}",
            "{",
        ]
        for ctype, name, values, guard in members:
            if guard:
                lines.append(f"#if {guard}")
            lines.append(f"    {ctype} {name}[{len(values)}];")
            if guard:
                lines.append("#endif")
        lines += [
            f"}} {type_name};",
            "",
        ]
        for ctype, name, values, guard in members:
            macro = (f"NDS_NATIVE_IMAGE_{owner_name.upper()}_"
                     f"{detail.upper()}_{name.upper()}_COUNT")
            if guard:
                lines.append(f"#if {guard}")
            lines.append(f"#define {macro} {len(values)}u")
            if guard:
                lines.append("#endif")
        lines.append("")
        # The equivalence list: every member paired with the in-binary array
        # it is a copy of, generated from the same description that generated
        # the bytes so there is no hand-written second list to fall out of
        # step when an array is added.
        #
        # A preprocessor directive cannot live inside a macro definition, so a
        # guarded run of members becomes its OWN macro -- defined empty when
        # its guard is false -- and the top-level list invokes it in place.
        base_macro = (f"NDS_NATIVE_IMAGE_{owner_name.upper()}_"
                      f"{detail.upper()}_MEMBERS")
        # P2-3f49: dense_normals are BAKED, not copied. At VERIFY time the
        # bake arrays are still empty (the bake runs lazily on first draw,
        # after the fighter-creation VERIFY), so a byte compare against them
        # would always false-mismatch. They ride a sibling macro whose row
        # names the dense_vertices array the bake reads; the C side re-bakes
        # from it and compares word for word.
        normals_macro = f"{base_macro}_DENSE_NORMALS"
        segments: list[tuple[str, list[str]]] = []
        normals_rows: list[str] = []
        for _ctype, name, _values, guard in members:
            if name == "dense_normals":
                normals_rows.append(
                    f"    X({type_name}, {name}, "
                    f"{_array_symbol(owner_name, detail, 'dense_vertices')})")
                continue
            row = (f"    X({type_name}, {name}, "
                   f"{_array_symbol(owner_name, detail, name)})")
            if segments and segments[-1][0] == guard:
                segments[-1][1].append(row)
            else:
                segments.append((guard, [row]))

        top: list[str] = []
        for index, (guard, rows) in enumerate(segments):
            if not guard:
                top.extend(rows)
                continue
            sub = f"{base_macro}_G{index}"
            lines.append(f"#if {guard}")
            lines.append(f"#define {sub}(X) \\")
            for row_index, row in enumerate(rows):
                tail = "" if row_index == len(rows) - 1 else " \\"
                lines.append(row + tail)
            lines.append("#else")
            lines.append(f"#define {sub}(X)")
            lines.append("#endif")
            top.append(f"    {sub}(X)")
        lines.append(f"#define {base_macro}(X) \\")
        for row_index, row in enumerate(top):
            tail = "" if row_index == len(top) - 1 else " \\"
            lines.append(row + tail)
        lines.append("")
        lines.append(f"#define {normals_macro}(X) \\")
        for row_index, row in enumerate(normals_rows):
            tail = "" if row_index == len(normals_rows) - 1 else " \\"
            lines.append(row + tail)
        lines.append("")
    lines += ["#endif /* NDS_NATIVE_FIGHTER_IMAGE_GENERATED_H */", ""]
    return "\n".join(lines)


def render_image(owner_name: str, detail: str,
                 context: dict[str, object]) -> str:
    """The standalone translation unit whose bytes become the NitroFS payload.

    It is compiled but NOT linked into the ARM9 image: the build extracts its
    single object section with `objcopy -O binary`.
    """
    type_name = _image_type(owner_name, detail)
    members = _member_values(context)
    lines = [
        "/* Generated by scripts/fighters/generate_nds_native_owner_images.py."
        "  Do not edit. */",
        f"/* {_owner_title(owner_name)} {detail} native-owner table image.  This TU is"
        " compiled for its",
        " * BYTES, not for its symbols: the build objcopies it into a NitroFS"
        " payload and the",
        " * ARM9 binary never links it.  Keeping the initializer in C is the"
        " whole point --",
        " * the layout stays the compiler's, so the runtime's offsetof and these"
        " bytes cannot",
        " * disagree. */",
        "",
        "#include <PR/ultratypes.h>",
        "#include <nds/generated/nds_native_fighter_image.generated.h>",
        "",
        f"const {type_name} gNdsNative{_owner_title(owner_name)}{detail.title()}Image"
        " __attribute__((section(\".fighter_image\"), used)) =",
        "{",
    ]
    for _ctype, name, values, guard in members:
        if guard:
            lines.append(f"#if {guard}")
        lines.append(f"    .{name} =")
        lines.append("    {")
        lines += [f"    {row}" for row in _rows(values)]
        lines.append("    },")
        if guard:
            lines.append("#endif")
    lines += ["};", ""]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path, default=_paths.REPO_ROOT)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    repo_root = Path(args.repo_root).resolve()

    contexts: dict[tuple[str, str], dict[str, object]] = {}
    for owner_name in P2_IMAGE_OWNERS:
        for detail in DETAILS:
            contexts[(owner_name, detail)] = (
                owners.build_p2_owner_runtime_context(
                    repo_root, owner_name, detail))

    # The owners generator guards exactly `NATIVE_OWNER_IMAGE_ARRAYS` out of
    # the ARM9 binary. If this tool's member table ever names a different set,
    # an array is either duplicated (paying arena for nothing) or absent from
    # both places (a dangling table pointer in a draw path), so the two views
    # are compared before anything is written.
    sample = contexts[(P2_IMAGE_OWNERS[0], DETAILS[0])]
    described = {
        _array_stem(name) for _ctype, name, _values, _guard
        in _member_values(sample)
    }
    if described != set(NATIVE_OWNER_IMAGE_ARRAYS):
        raise SystemExit(
            "image member table and native_owner_image_arrays disagree:"
            + "\n  only in member table: "
            + repr(sorted(described - set(NATIVE_OWNER_IMAGE_ARRAYS)))
            + "\n  only in shared list:  "
            + repr(sorted(set(NATIVE_OWNER_IMAGE_ARRAYS) - described)))

    products: dict[Path, str] = {
        repo_root / "include" / "nds" / "generated"
        / "nds_native_fighter_image.generated.h": render_header(contexts),
    }
    for (owner_name, detail), context in contexts.items():
        path = (repo_root / "src" / "nds" / "generated"
                / f"nds_native_fighter_{owner_name}_{detail}.image.c")
        products[path] = render_image(owner_name, detail, context)

    stale: list[str] = []
    for path, text in products.items():
        if args.check:
            if not path.is_file() or path.read_text() != text:
                stale.append(str(path))
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        if not path.is_file() or path.read_text() != text:
            path.write_text(text)
    if stale:
        raise SystemExit("stale generated native-owner images:\n  " +
                         "\n  ".join(stale))
    if not args.check:
        for (owner_name, detail), context in sorted(contexts.items()):
            total = 0
            for _ctype, _name, values, _guard in _member_values(context):
                total += len(values)
            print(f"{owner_name} {detail}: {len(_member_values(context))} "
                  f"arrays, {total} elements")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

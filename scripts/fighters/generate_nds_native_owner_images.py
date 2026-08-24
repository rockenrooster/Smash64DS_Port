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


# The P2-3 owners whose tables this tool moves out of the ARM9 image. Mario and
# Fox are absent on purpose (frozen combined export).
P2_IMAGE_OWNERS = ("luigi", "donkey")
DETAILS = ("high", "low")


def _rows(values: list[str]) -> list[str]:
    """One initializer row per line, in the generator's own house style."""
    return [f"    {value}," for value in values]


def _member_values(context: dict[str, object]) -> list[tuple[str, str, list[str]]]:
    """(C type, member name, initializer rows) for every array in the image.

    The value formats are the ones `render_p2_owner_runtime_program` already
    emits; keeping them identical is what makes the image byte-comparable
    against the arrays it replaces.
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

    members: list[tuple[str, str, list[str]]] = [
        ("NDSNativeStateDelta", "state_deltas",
         [f"{{ 0x{w0:08x}u, 0x{w1:08x}u, {effect}u, {{ 0u, 0u, 0u }} }}"
          for w0, w1, effect in state]),
        ("u8", "state_sequence", [f"{value}u" for value in sequence]),
        ("NDSNativeVertexAction", "vertex_actions",
         [f"{{ {kind}u, {command}u, {index}u, {count}u, "
          f"0x{offset:08x}u, {s}, {t} }}"
          for kind, command, index, count, offset, s, t in vertex]),
        ("u8", "epoch_direct_policy",
         [f"0x{value:02x}u" for value in direct_policies]),
        ("NDSNativeDenseVertex", "dense_vertices",
         ["{{ 0x{:08x}u, {}, {}, {}u, {}u, 0u }}".format(
             rgba, s, t, binding, cache_slot)
          for x, y, z, s, t, binding, cache_slot, rgba in dense_vertices]),
        ("u16", "action_dense_spans",
         [f"0x{value:04x}u" for value in action_dense_spans]),
        ("u16", "dense_color_source",
         [f"{value}u" for value in dense_color_sources]),
        ("u16", "packed_corners",
         [f"0x{value:04x}u" for value in packed_corners]),
        ("u16", "run_first_corner",
         [f"{value}u" for value in run_first_corner]),
        ("u16", "run_first_unique",
         [f"{value}u" for value in run_first_unique]),
        ("u8", "run_unique_count",
         [f"{value}u" for value in run_unique_count]),
        ("u16", "run_unique_dense",
         [f"{value}u" for value in run_unique_dense]),
        ("u16", "triangles", [f"0x{value:04x}u" for value in triangles]),
        ("NDSNativeRun", "runs",
         [f"{{ {first}u, {count}u, {submit_class}u, 0x{mask:08x}u }}"
          for first, count, submit_class, mask in runs]),
        ("NDSNativeEpoch", "epochs",
         ["{{ {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u }}".format(*row)
          for row in epochs]),
    ]
    del owner_name, detail
    return members


def _image_type(owner_name: str, detail: str) -> str:
    return f"NDSNative{owner_name.title()}{detail.title()}Image"


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
    ]
    for (owner_name, detail), context in sorted(contexts.items()):
        members = _member_values(context)
        type_name = _image_type(owner_name, detail)
        lines += [
            f"/* {owner_name.title()} {detail} native-owner image. */",
            f"typedef struct {type_name}",
            "{",
        ]
        for ctype, name, values in members:
            lines.append(f"    {ctype} {name}[{len(values)}];")
        lines += [
            f"}} {type_name};",
            "",
        ]
        for ctype, name, values in members:
            macro = (f"NDS_NATIVE_IMAGE_{owner_name.upper()}_"
                     f"{detail.upper()}_{name.upper()}_COUNT")
            lines.append(f"#define {macro} {len(values)}u")
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
        f"/* {owner_name.title()} {detail} native-owner table image.  This TU is"
        " compiled for its",
        " * BYTES, not for its symbols: the build objcopies it into a NitroFS"
        " payload and the",
        " * ARM9 binary never links it.  Keeping the initializer in C is the"
        " whole point --",
        " * the layout stays the compiler's, so the runtime's offsetof and these"
        " bytes cannot",
        " * disagree. */",
        "",
        "#include \"nds_build_config.h\"",
        "",
        "#include <PR/ultratypes.h>",
        "#include <nds/nds_native_fighter_tables.h>",
        "#include <nds/generated/nds_native_fighter_image.generated.h>",
        "",
        f"const {type_name} gNdsNative{owner_name.title()}{detail.title()}Image"
        " __attribute__((section(\".fighter_image\"), used)) =",
        "{",
    ]
    for _ctype, name, values in members:
        lines.append(f"    .{name} =")
        lines.append("    {")
        lines += [f"    {row}" for row in _rows(values)]
        lines.append("    },")
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
            for ctype, _name, values in _member_values(context):
                total += len(values)
            print(f"{owner_name} {detail}: {len(_member_values(context))} "
                  f"arrays, {total} elements")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

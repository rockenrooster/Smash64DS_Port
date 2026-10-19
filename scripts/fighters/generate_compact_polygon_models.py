#!/usr/bin/env python3
"""Compact polygon models for the Fighting Polygon Team (P7 C1).

WHAT THIS BUILDS AND WHY
------------------------
The 1P game's Fighting Polygon Team stage loads all eleven polygon models
(NMarioModel ... NNessModel, NLuigi shares NMarioModel), 10-19 KB each. Almost
all of each file is N64 geometry -- Vtx arrays and the display lists that draw
them -- and the DS never reads it: the native renderer draws a polygon fighter
from its generated owner program (generate_nds_native_owners.py), keyed by the
display list's SOURCE offset, and reads only two things from the loaded file:
the DObjDesc joint tree (the fighter's skeleton and each joint's display list
pointer) and the identity of each display list a joint names. A content in the
campaign ran the heap out at that stage (Palutena, Snake, Ike, Dedede, Bowser:
P7 C1, 2026-10-19).

Each compact model keeps the joint tree byte for byte and replaces every
display list anything points at with an 8-byte identity cell -- {G_ENDDL,
source offset}, the cell the preview packs already use (reloc_preview_pack.c
ndsRelocNativeRootOffset) -- so a stray GBI walk ends at once and the native
lookup still gets the source offset. Payload layout (big-endian, like O2R):

    0x00  'NCPM'
    0x04  source payload size           (the record's data_size)
    0x08  X, the joint tree's source offset
    0x0C  cell count << 16 | S          (S = bytes before the joint tree)
    0x10  cells, 8 bytes each, sorted by source offset
          anchor words (see below), then zero pad to S
    S     the joint tree, source bytes [X, Y), zero padded to 16

The runtime (reloc_backend_assets.c, NDS_P2_COMPACT_POLYGON_MODELS) registers
the record with data = physical + S - X and data_size = the source size, so
every source offset keeps its meaning: the joint tree's own slots and every
other file's extern targets resolve unchanged, and only [X - S, X + P - S) is
backed. Chain words name those VIRTUAL word indices. A joint's display list
slot is re-aimed at its cell here; a main's extern slot aimed at a model
display list (Samus' and Ness' model parts) is mapped to the cell by the
runtime's extern fixup.

The geometry's extern slots (SETTIMG into NCommonTexture, file 302) go with
it, but the native IMAGE states name that file at runtime
(ndsRelocNativeForeignImageAddress), so it must stay in the model's tree: one
ANCHOR word per dependency the dropped slots named keeps an extern chain node
(target = the first dropped slot's), and the fixup writes an unused pointer.

The source files under decomp/ are read only. Output goes to the path named on
the command line (the Makefile passes the build's NitroFS copy of the model).

Usage:
  python generate_compact_polygon_models.py --model NMarioModel --output PATH
  python generate_compact_polygon_models.py --check
"""

from __future__ import annotations

import argparse
import bisect
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

_p = Path(__file__).resolve().parent
while _p.name != "scripts":
    _p = _p.parent
sys.path.insert(0, str(_p))
import _paths  # noqa: F401,E402
from generate_compact_ground_maps import (  # noqa: E402
    CompactMapError, O2RFile, chain_slots, parse_o2r, require, serialize_o2r)

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_O2R_ROOT = REPO_ROOT / "decomp" / "BattleShip-main" / "BattleShip_o2r"
DEFAULT_RELOCDATA_SRC = (REPO_ROOT / "decomp" / "BattleShip-main" / "decomp" /
                         "src" / "relocData")

MAGIC = 0x4E43504D          # 'NCPM'
HEADER_BYTES = 16
CELL_BYTES = 8
ANCHOR_BYTES = 4
DOBJDESC_BYTES = 44         # s32 id; Gfx *dl; Vec3f translate, rotate, scale
G_ENDDL = 0xDF
G_DL = 0xDE


@dataclass(frozen=True)
class PolygonModelSpec:
    name: str                       # reloc_fighters_main/<name>
    file_id: int                    # == the DS registry asset id
    referrers: tuple[str, ...]      # files whose extern slots name this one


MODELS: tuple[PolygonModelSpec, ...] = (
    PolygonModelSpec("NMarioModel", 301, ("NMarioMain", "NLuigiMain")),
    PolygonModelSpec("NFoxModel", 303, ("NFoxMain",)),
    PolygonModelSpec("NYoshiModel", 304, ("NYoshiMain",)),
    PolygonModelSpec("NKirbyModel", 305, ("NKirbyMain",)),
    PolygonModelSpec("NPurinModel", 306, ("NPurinMain",)),
    PolygonModelSpec("NPikachuModel", 307, ("NPikachuMain",)),
    PolygonModelSpec("NDonkeyModel", 308, ("NDonkeyMain",)),
    PolygonModelSpec("NSamusModel", 309, ("NSamusMain",)),
    PolygonModelSpec("NLinkModel", 310, ("NLinkMain",)),
    PolygonModelSpec("NCaptainModel", 311, ("NCaptainMain",)),
    PolygonModelSpec("NNessModel", 312, ("NNessMain",)),
)
MODELS_BY_NAME = {spec.name: spec for spec in MODELS}

# Each block of a relocData C file opens with "/* <kind> @ 0x<offset> ..." and,
# unless it is a raw tail, the declaration that holds it follows before the
# next block's opening comment.
BLOCK = re.compile(r"^/\* [^*@\n]*@ 0x([0-9A-Fa-f]+)", re.M)
DECLARATION = re.compile(r"^(\w+)\s*(\*?)\s*\w+\s*\[", re.M)


def be32(data: bytes, offset: int) -> int:
    return struct.unpack_from(">I", data, offset)[0]


def extern_rows(f: O2RFile) -> list[tuple[int, int, int]]:
    """[(slot, target, dependency file id)] in chain order."""
    slots = chain_slots(f.payload, f.external_head, f.name)
    require(len(slots) == len(f.extern_ids),
            f"{f.name}: {len(slots)} extern slots, {len(f.extern_ids)} ids")
    return [(slot, target, dep)
            for (slot, target), dep in zip(slots, f.extern_ids)]


@dataclass(frozen=True)
class Block:
    start: int
    end: int
    geometry: bool          # a Vtx array or a display list (Gfx array)
    display_list: bool


def model_blocks(spec: PolygonModelSpec, relocdata_src: Path,
                 source_size: int) -> list[Block]:
    """The file's blocks in offset order, from its relocData C layout: a
    block is geometry when it declares a Vtx or Gfx array; anything else -- a
    DObjDesc tree, a DObj.dls pointer table, an undeclared raw tail -- is
    data."""
    path = relocdata_src / f"{spec.file_id}_{spec.name}.c"
    text = path.read_text(encoding="utf-8", errors="replace")
    heads = [(int(m.group(1), 16), m.start()) for m in BLOCK.finditer(text)]
    blocks: list[Block] = []
    for i, (start, at) in enumerate(heads):
        if blocks and blocks[-1].start == start:
            continue            # a zero-length wrapper sharing its offset
        later = [s for s, _ in heads[i + 1:] if s > start]
        end = later[0] if later else source_size
        stop = heads[i + 1][1] if i + 1 < len(heads) else len(text)
        m = DECLARATION.search(text, at, stop)
        kind, pointer = (m.group(1), m.group(2)) if m else ("", "")
        display_list = (kind == "Gfx" and pointer == "")
        blocks.append(Block(start, end, kind == "Vtx" or display_list,
                            display_list))
    require(blocks and blocks[0].start == 0 and
            all(a.end == b.start for a, b in zip(blocks, blocks[1:])) and
            blocks[-1].end == source_size,
            f"{path.name}: blocks do not tile the {source_size}-byte file")
    return blocks


def display_list_end(payload: bytes, offset: int, name: str) -> int:
    """End of the display list at `offset`; raises unless it is one: every
    command an F3DEX2 or RDP opcode, ended by G_ENDDL or a branching G_DL."""
    require(offset % 8 == 0, f"{name}: display list 0x{offset:X} unaligned")
    pos = offset
    for _ in range(4096):
        require(pos + 8 <= len(payload),
                f"{name}: display list 0x{offset:X} runs off the file")
        op = payload[pos]
        require(op <= 0x07 or op >= 0xD3,
                f"{name}: 0x{offset:X} is not a display list (op 0x{op:02X} "
                f"at 0x{pos:X})")
        pos += 8
        if op == G_ENDDL or (op == G_DL and payload[pos - 7] == 1):
            return pos
    raise CompactMapError(f"{name}: display list 0x{offset:X} never ends")


@dataclass(frozen=True)
class CompactModel:
    data: bytes             # the complete O2R file
    source_size: int
    compact_size: int
    cells: int
    anchors: int


def build_compact_model(spec: PolygonModelSpec, o2r_root: Path,
                        relocdata_src: Path) -> CompactModel:
    folder = o2r_root / "reloc_fighters_main"
    m = parse_o2r((folder / spec.name).read_bytes(), spec.name)
    require(m.file_id == spec.file_id,
            f"{spec.name}: file id {m.file_id} != {spec.file_id}")
    source = bytes(m.payload)
    source_size = len(source)
    blocks = model_blocks(spec, relocdata_src, source_size)
    starts = [b.start for b in blocks]

    def block_of(offset: int) -> Block:
        return blocks[bisect.bisect_right(starts, offset) - 1]

    internal = chain_slots(source, m.internal_head, spec.name)
    referred: list[int] = []
    for referrer in spec.referrers:
        r = parse_o2r((folder / referrer).read_bytes(), referrer)
        named = [target for _, target, dep in extern_rows(r)
                 if dep == spec.file_id]
        require(named, f"{referrer} names no slot in {spec.name}")
        referred += named

    # What the DS reads: the data blocks the referrers name and, through their
    # slots, the data blocks those name. A display list anything names becomes
    # a cell; nothing may name a Vtx array or the inside of a display list.
    cells: set[int] = set()
    data: set[int] = set()
    work = list(referred)
    while work:
        target = work.pop()
        block = block_of(target)
        if block.display_list and target == block.start:
            display_list_end(source, target, spec.name)
            cells.add(target)
            continue
        require(not block.geometry,
                f"{spec.name}: 0x{target:X} names inside geometry")
        if block.start in data:
            continue
        data.add(block.start)
        work += [t for slot, t in internal if block.start <= slot < block.end]
    x = min(data)
    y = block_of(max(data)).end
    require(x % 4 == 0 and source_size % 16 == 0,
            f"{spec.name}: data [0x{x:X}, 0x{y:X}) / {source_size} B")

    def inside(offset: int) -> bool:
        return x <= offset < y

    def kept_slot(slot: int) -> bool:
        return block_of(slot).start in data

    for target in cells:
        require(not inside(target),
                f"{spec.name}: cell 0x{target:X} inside the kept range")
    kept_internal = [(slot, target) for slot, target in internal
                     if kept_slot(slot)]
    for slot, target in kept_internal:
        require(slot + 4 <= block_of(slot).end and
                (inside(target) or target in cells),
                f"{spec.name}: slot 0x{slot:X} -> 0x{target:X}")
    require(kept_internal, f"{spec.name}: the joint tree names no display list")

    kept_external: list[tuple[int, int, int]] = []
    anchors: dict[int, int] = {}            # dependency -> first target
    for slot, target, dep in extern_rows(m):
        if kept_slot(slot):
            kept_external.append((slot, target, dep))
        else:
            anchors.setdefault(dep, target)
    for _, _, dep in kept_external:
        anchors.pop(dep, None)

    cell_list = sorted(cells)
    s = HEADER_BYTES + CELL_BYTES * len(cell_list) + ANCHOR_BYTES * len(anchors)
    s += (x - s) % 16                       # S == X (mod 16): 16-byte phase
    require(s <= x, f"{spec.name}: {s} B of cells exceed X 0x{x:X}")
    base = x - s                            # virtual offset of physical 0
    cell_at = {off: base + HEADER_BYTES + CELL_BYTES * k
               for k, off in enumerate(cell_list)}
    anchor_at = [base + HEADER_BYTES + CELL_BYTES * len(cell_list) +
                 ANCHOR_BYTES * j for j in range(len(anchors))]

    kept = bytearray(source[x:y])
    kept += bytes((-(x + len(kept))) % 16)
    payload = bytearray(struct.pack(">IIII", MAGIC, source_size, x,
                                    (len(cell_list) << 16) | s))
    for off in cell_list:
        payload += struct.pack(">II", G_ENDDL << 24, off)
    payload += bytes(s - len(payload))
    payload += kept
    require(x + len(payload) - s <= source_size,
            f"{spec.name}: compact range runs past the source size")

    def link(rows: list[tuple[int, int]]) -> int:
        """Write a chain of (virtual slot, virtual target); returns its head."""
        for i, (slot, target) in enumerate(rows):
            following = rows[i + 1][0] // 4 if i + 1 < len(rows) else 0xFFFF
            require(slot % 4 == 0 and target % 4 == 0 and
                    target // 4 <= 0xFFFF and following <= 0xFFFF,
                    f"{spec.name}: chain index out of range")
            struct.pack_into(">I", payload, slot - base,
                             (following << 16) | (target // 4))
        return rows[0][0] // 4 if rows else 0xFFFF

    internal_head = link([(slot, target if inside(target) else cell_at[target])
                          for slot, target in kept_internal])
    external = ([(anchor_at[j], target)
                 for j, target in enumerate(anchors.values())] +
                [(slot, target) for slot, target, _ in kept_external])
    external_head = link(external)
    extern_ids = list(anchors) + [dep for _, _, dep in kept_external]

    out = O2RFile(spec.name, m.header, m.file_id, internal_head,
                  external_head, extern_ids, payload)
    return CompactModel(serialize_o2r(out), source_size, len(payload),
                        len(cell_list), len(anchors))


def check_referrers(o2r_root: Path) -> None:
    """No file outside each spec's referrers names a polygon model."""
    ids = {spec.file_id: spec for spec in MODELS}
    for path in sorted(o2r_root.rglob("*")):
        if not path.is_file():
            continue
        raw = path.read_bytes()
        try:
            f = parse_o2r(raw, path.name)
        except CompactMapError:
            continue
        for dep in set(f.extern_ids) & set(ids):
            spec = ids[dep]
            require(path.parent.name == "reloc_fighters_main" and
                    path.name in spec.referrers,
                    f"{path.relative_to(o2r_root)} names {spec.name} but is "
                    "not one of its referrers")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--o2r-root", type=Path, default=DEFAULT_O2R_ROOT)
    parser.add_argument("--relocdata-src", type=Path,
                        default=DEFAULT_RELOCDATA_SRC)
    parser.add_argument("--model", help="reloc_fighters_main/<name> or <name>")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.check:
            check_referrers(args.o2r_root)
            total = 0
            for spec in MODELS:
                c = build_compact_model(spec, args.o2r_root, args.relocdata_src)
                total += c.source_size - c.compact_size
                print(f"{spec.name:14s} {c.source_size:6d} -> "
                      f"{c.compact_size:5d} B ({c.cells} cells, "
                      f"{c.anchors} anchors)")
            print(f"saved {total} B over {len(MODELS)} models")
            return 0
        require(args.model is not None and args.output is not None,
                "--model and --output are required")
        name = args.model.rsplit("/", 1)[-1]
        require(name in MODELS_BY_NAME, f"{name}: not a polygon model")
        c = build_compact_model(MODELS_BY_NAME[name], args.o2r_root,
                                args.relocdata_src)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(c.data)
        return 0
    except CompactMapError as error:
        print(f"generate_compact_polygon_models: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

#!/usr/bin/env python3
"""P7 S0 (docs/P7/P7_Implementation_Plan.md S0, assumption A10): an owner
stage scale, applied to a Remix stage's files as the generator stages them.

A stage row in scripts/p5/stages.json may carry "owner_scale" (Bowser's
Stadium 1.5, P7 S1). The stage then grows about its main floor's centre
(x 0 at the floor's height, `floor_y`):

- its layers' root DObjDesc rows (the tree's id-0 rows) scale by it, their
  translation about that centre, so the model and every joint and material
  animation under them follow; the native packet is baked from the scaled
  rows, so it and the runtime's DObjs agree;
- its collision vertices scale about the centre: the floor keeps its
  height, its width and the walls below it grow;
- its map objects (spawn and item points, the respawn platform, the
  bombs' places) move out: x scales, y keeps its height above the floor;
- its header's side bounds -- camera, blast, and their team copies -- move
  out by as much as the ledges do; top and bottom stay (owner, 2026-10-13:
  the blast zones keep today's distance from the stage).

Nothing else in either file changes (scripts/p5/test_owner_scale.py proves
it by a round trip on the staged files). The select-screen preview's zoom
divides by the scale (p5_stages.py), and the stage's code constants read it
(src/port/nds_p5_stages.c: the bombs' x).
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field

DOBJ_DESC_SIZE = 44     # u32 id, Gfx *dl, Vec3f translate, rotate, scale
TREE_END = 18           # the sentinel row's id
# MPGroundData (src/mp/mptypes.h; scripts/p5/stage_export.py reads the same):
# camera, blast, team camera, team blast bounds, each top, bottom, right, left.
HEADER_BOUNDS = (0x6C, 0x74, 0x8A, 0x92)


class ScaleError(SystemExit):
    pass


@dataclass
class ScaleReport:
    scale: float
    floor_y: int
    ledges: tuple[int, int]
    roots: list[int] = field(default_factory=list)       # DObjDesc row offsets
    vertices: int = 0
    mapobjs: int = 0
    bounds: dict[str, list[int]] = field(default_factory=dict)


def _ptr(slots: dict[int, int], at: int, what: str) -> int:
    if at not in slots:
        raise ScaleError(f"owner scale: {what} at {at:#x} is not an internal pointer")
    return slots[at]


def geometry(payload: bytearray, slots: dict[int, int], layer_offsets: list[int],
             geometry_at: int, scale: float) -> ScaleReport:
    """Scale the geometry file's payload in place: the layers' root rows,
    the collision vertices and the map objects. `slots`: the payload's
    internal pointer slots (slot offset -> target offset)."""
    vdata = _ptr(slots, geometry_at + 4, "vertex data")
    vid = _ptr(slots, geometry_at + 8, "vertex ids")
    mapobj_count = struct.unpack_from(">H", payload, geometry_at + 20)[0]
    mapobjs = _ptr(slots, geometry_at + 24, "map objects")
    if vid <= vdata or (vid - vdata) % 6:
        raise ScaleError("owner scale: the vertex table does not end where the ids begin")
    count = (vid - vdata) // 6
    verts = [struct.unpack_from(">hh", payload, vdata + 6 * k) for k in range(count)]
    floor_y = max(y for _x, y in verts)
    xs = [x for x, _y in verts]
    report = ScaleReport(scale, floor_y, (min(xs), max(xs)))
    for at in layer_offsets:
        row = at
        while True:
            ident = struct.unpack_from(">I", payload, row)[0]
            if ident == TREE_END:
                break
            if (ident & 0xFFF) == 0:
                tx, ty, tz = struct.unpack_from(">3f", payload, row + 8)
                sx, sy, sz = struct.unpack_from(">3f", payload, row + 32)
                struct.pack_into(">3f", payload, row + 8, tx * scale,
                                 floor_y + (ty - floor_y) * scale, tz * scale)
                struct.pack_into(">3f", payload, row + 32, sx * scale, sy * scale, sz * scale)
                report.roots.append(row)
            row += DOBJ_DESC_SIZE
    for k, (x, y) in enumerate(verts):
        struct.pack_into(">hh", payload, vdata + 6 * k, _s16(x * scale),
                         _s16(floor_y + (y - floor_y) * scale))
    report.vertices = count
    for k in range(mapobj_count):
        at = mapobjs + 6 * k
        x = struct.unpack_from(">h", payload, at + 2)[0]
        struct.pack_into(">h", payload, at + 2, _s16(x * scale))
    report.mapobjs = mapobj_count
    return report


def header(payload: bytearray, header_at: int, report: ScaleReport) -> None:
    """Move the header's side bounds out by as much as the ledges move."""
    left, right = report.ledges
    d_left = round(left * (report.scale - 1.0))
    d_right = round(right * (report.scale - 1.0))
    for name, off in zip(("camera", "blast", "team_camera", "team_blast"), HEADER_BOUNDS):
        at = header_at + off
        top, bottom, r, l = struct.unpack_from(">4h", payload, at)
        new = [top, bottom, _s16(r + d_right), _s16(l + d_left)]
        struct.pack_into(">4h", payload, at, *new)
        report.bounds[name] = new


def _s16(v: float) -> int:
    n = int(round(v))
    if not -0x8000 <= n <= 0x7FFF:
        raise ScaleError(f"owner scale: {v} overflows s16")
    return n

"""P7 S0 round trip (docs/P7/P7_Implementation_Plan.md S0, A10): a scaled
stage differs from its source only in the fields A10 names -- its layers'
root rows' translation and scale, its collision vertices, its map objects'
x, and its header's side bounds -- and in nothing else, byte for byte.

Reads a staged Bowser's Stadium (a build's p5/stages/bowsers_stadium/o2r,
from the owner's local Remix build; skipped when no build has one).

    python -m pytest -q scripts/p5/test_owner_scale.py
"""
from __future__ import annotations

import json
import struct
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(ROOT / "scripts" / "stages"))

import owner_scale as S  # noqa: E402
import generate_compact_ground_maps as gcm  # noqa: E402


def _staged() -> Path | None:
    hits = sorted(ROOT.glob("builds/*/p5/stages/bowsers_stadium/stage.json"),
                  key=lambda p: p.stat().st_mtime, reverse=True)
    return hits[0].parent if hits else None


class OwnerScaleRoundTrip(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.dir = _staged()
        if cls.dir is None:
            raise unittest.SkipTest("no staged Bowser's Stadium (a P5 build's export)")
        cls.stage = json.loads((cls.dir / "stage.json").read_text(encoding="utf-8"))
        rows = json.loads((ROOT / "scripts/p5/stages.json").read_text(encoding="utf-8"))
        rows = rows if isinstance(rows, list) else rows.get("stages", [])
        cls.scale = float(next(r for r in rows if r.get("name") == "bowsers_stadium").get("owner_scale", 1.5))

    def test_geometry_changes_only_the_named_fields(self):
        geo_fid, geo_at = self.stage["map_geometry"]
        f = gcm.parse_o2r((self.dir / "o2r" / f"{geo_fid:04x}").read_bytes(), "geometry")
        before = bytes(f.payload)
        slots = dict(gcm.chain_slots(before, f.internal_head, "geometry"))
        layers = [L["dobjdesc"][1] for L in self.stage["layers"]
                  if L["dobjdesc"] and L["dobjdesc"][0] == geo_fid]
        payload = bytearray(before)
        rep = S.geometry(payload, slots, layers, geo_at, self.scale)
        allowed = set()
        for row in rep.roots:
            allowed |= set(range(row + 8, row + 20)) | set(range(row + 32, row + 44))
        vdata = slots[geo_at + 4]
        for k in range(rep.vertices):
            allowed |= set(range(vdata + 6 * k, vdata + 6 * k + 4))
        mapobjs = slots[geo_at + 24]
        for k in range(rep.mapobjs):
            allowed |= set(range(mapobjs + 6 * k + 2, mapobjs + 6 * k + 4))
        changed = {i for i, (a, b) in enumerate(zip(before, payload)) if a != b}
        self.assertTrue(changed, "the scale changed nothing")
        self.assertLessEqual(changed, allowed, f"bytes outside A10's fields: {sorted(changed - allowed)[:8]}")
        self.assertEqual(len(before), len(payload))
        # The floor keeps its height, its ends move out by the scale; the
        # map objects keep their heights.
        verts = [struct.unpack_from(">hh", payload, vdata + 6 * k) for k in range(rep.vertices)]
        self.assertEqual(max(y for _x, y in verts), rep.floor_y)
        self.assertEqual((min(x for x, _y in verts), max(x for x, _y in verts)),
                         (round(rep.ledges[0] * self.scale), round(rep.ledges[1] * self.scale)))
        for k in range(rep.mapobjs):
            y0 = struct.unpack_from(">h", before, mapobjs + 6 * k + 4)[0]
            self.assertEqual(struct.unpack_from(">h", payload, mapobjs + 6 * k + 4)[0], y0)
        for row in rep.roots:
            sx = struct.unpack_from(">f", payload, row + 32)[0]
            sx0 = struct.unpack_from(">f", before, row + 32)[0]
            self.assertAlmostEqual(sx, sx0 * self.scale, places=5)

    def test_header_moves_only_the_side_bounds(self):
        map_fid = self.stage["header"]["file"]
        hoff = self.stage["header"]["offset"]
        f = gcm.parse_o2r((self.dir / "o2r" / f"{map_fid:04x}").read_bytes(), "map")
        before = bytes(f.payload)
        payload = bytearray(before)
        rep = S.ScaleReport(self.scale, 0, (-3032, 3032))
        S.header(payload, hoff, rep)
        allowed = set()
        for off in S.HEADER_BOUNDS:
            allowed |= set(range(hoff + off + 4, hoff + off + 8))
        changed = {i for i, (a, b) in enumerate(zip(before, payload)) if a != b}
        self.assertLessEqual(changed, allowed)
        d = round(3032 * (self.scale - 1.0))
        for name, off in zip(("camera", "blast", "team_camera", "team_blast"), S.HEADER_BOUNDS):
            top, bottom, right, left = struct.unpack_from(">4h", before, hoff + off)
            self.assertEqual(rep.bounds[name], [top, bottom, right + d, left - d], name)


if __name__ == "__main__":
    unittest.main()

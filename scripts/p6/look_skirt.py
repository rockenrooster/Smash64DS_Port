#!/usr/bin/env python3
"""P6: Palutena's skirt on Peach's dress rows (owner playtest 2026-10-14:
in EVERY animation the dress surrounds her legs, except at the slit).

The look's skirt is a cone of long triangles from the bodice's waist ring
(row 2) to the hem (row 1, the pelvis), so a leg that swings away from the
pelvis leaves it (dress_check.py: 205 of 218 motions, many by more than 100
units). Peach's own gown hangs from her dress rows instead: an upper band on
row 25 and the hem on row 26, its triangles drawn by row 26 from row 25's
seam load, and her clips key both rows to keep the gown around her legs.
This remeshes Palutena's skirt the same way:

- the upper band, from the waist ring down to a ring at CUT_Y (row 1
  vertices), drawn by row 2 from row 1's seam load as before;
- the lower band, from a copy of that ring on row 25 (Peach's dress row,
  whose list is only that seam load; raised and drawn in a little so a
  tilted ring stays inside the upper band) to the hem ring on row 26, drawn
  by row 26, open at the slit.

The skirt's seam takes two of the owner's GX cross slots (rows 25 and 26;
the generator has 15, 16-30), and her look already used 14: each foot (rows
19 and 24) takes its own copy of the ankle ring it borrowed from the shin
(rows 18 and 23), so a foot draws without a seam and gives its slot back.
The foot row's origin is the ankle, where the ring sits, so a turned foot
keeps the ring within a few units of the shin's. (The hip seams stay: the
thigh rows' origins are the knees -- moving the thighs' top rings onto them
swung the thigh tops around the knee, away from the pelvis; owner,
2026-10-14.)

The rings are sampled at the texture's u (around the body, 0 and 1 the back
seam): the waist ring keeps its own vertices, the cut ring is where the old
surface crosses CUT_Y, the hem ring the old hem's; positions, uv and normals
are interpolated along each ring. The old skirt triangles leave the scene
(their vertices stay, unreferenced).

    look_skirt.py --scene <scene.json> [--scene ...] [--samples high|low]

Rewrites each scene in place; a scene whose skirt is already on row 26 is
left alone.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np

DRESS = "DS_dress"
WAIST_Y = 328.0
HEM_MAX_Y = 90.0
CUT_Y = 288.0
# Row 25's origin (the band's pivot) moved up from Peach's (y 252.6) to her
# hip joints' height, so the whole thigh hangs inside the band that turns.
PIVOT_Y = 280.0
# Rows: the pelvis, the bodice, Peach's dress rows.
ROW_PELVIS, ROW_BODICE, ROW_UPPER, ROW_HEM = 1, 2, 25, 26
# The row 25 ring: this much above the cut and this fraction of its radius,
# inside the upper band's lower edge.
OVERLAP_UP = 6.0
OVERLAP_IN = 0.96
# Foot rows that take their own copy of the ankle ring (their parent's).
FOOT_ROWS = (19, 24)
# The slit: the hem's open span in u (the old hem's two vertices beside it).
SLIT = (0.58333, 0.72222)
# The rings' samples in u: the lower band's (the hem's, with the slit's two
# edges) and the upper band's cut ring (fewer: the band is short).
SAMPLES = {
    "high": [0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.58333, 0.72222, 0.81481, 0.90741, 1.0],
    "low": [0.0, 0.125, 0.25, 0.375, 0.5, 0.58333, 0.72222, 0.86111, 1.0],
}
SAMPLES_UPPER = {
    "high": [0.0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1.0],
    "low": [0.0, 0.16667, 0.33333, 0.5, 0.66667, 0.83333, 1.0],
}


class SkirtError(SystemExit):
    pass


def _ring_interp(points: list[dict], u: float) -> dict:
    """Position, uv and normal at u along a ring sorted by u."""
    us = [p["u"] for p in points]
    if u <= us[0]:
        return points[0]
    if u >= us[-1]:
        return points[-1]
    k = next(i for i in range(1, len(us)) if us[i] >= u)
    a, b = points[k - 1], points[k]
    t = 0.0 if b["u"] == a["u"] else (u - a["u"]) / (b["u"] - a["u"])
    return {"u": u, "pos": a["pos"] + (b["pos"] - a["pos"]) * t,
            "uv": a["uv"] + (b["uv"] - a["uv"]) * t,
            "normal": a["normal"] + (b["normal"] - a["normal"]) * t}


def _point(v: dict) -> dict:
    return {"u": float(v["uv"][0]), "pos": np.array(v["model"], dtype=float),
            "uv": np.array(v["uv"], dtype=float), "normal": np.array(v["normal"], dtype=float)}


def _cut_ring(verts: list[dict], tris: list[dict], y: float) -> list[dict]:
    """Where the skirt's triangles cross y, as points sorted by u."""
    out = []
    for t in tris:
        p = [_point(verts[i]) for i in t["v"]]
        for a, b in ((0, 1), (1, 2), (2, 0)):
            ya, yb = p[a]["pos"][1], p[b]["pos"][1]
            if (ya - y) * (yb - y) >= 0.0 or ya == yb:
                continue
            s = (y - ya) / (yb - ya)
            q = {k: p[a][k] + (p[b][k] - p[a][k]) * s for k in ("pos", "uv", "normal")}
            q["u"] = float(q["uv"][0])
            out.append(q)
    out.sort(key=lambda q: q["u"])
    # One point per u (an edge two triangles share is crossed twice).
    ring = []
    for q in out:
        if ring and abs(ring[-1]["u"] - q["u"]) < 1e-4:
            continue
        ring.append(q)
    return ring


def remesh(scene: dict, detail: str) -> dict:
    verts, tris, joints = scene["vertices"], scene["triangles"], scene["joints"]
    if any(verts[i]["joint"] == ROW_HEM for t in tris for i in t["v"]):
        return {"skipped": "already on row 26"}
    skirt = [t for t in tris if t.get("object") == DRESS and
             any(verts[i]["model"][1] < WAIST_Y - 1.0 for i in t["v"])]
    if not skirt:
        raise SkirtError("no skirt triangles")
    material = skirt[0]["material"]
    used = sorted({i for t in skirt for i in t["v"]})
    waist = sorted((i for i in used if abs(verts[i]["model"][1] - WAIST_Y) < 1.0 and
                    verts[i]["joint"] == ROW_BODICE), key=lambda i: verts[i]["uv"][0])
    hem = sorted((_point(verts[i]) for i in used if verts[i]["model"][1] < HEM_MAX_Y),
                 key=lambda q: q["u"])
    cut = _cut_ring(verts, skirt, CUT_Y)
    if len(waist) < 4 or len(hem) < 4 or len(cut) < 4:
        raise SkirtError(f"rings: waist {len(waist)}, hem {len(hem)}, cut {len(cut)}")
    # Row 25 up to the pivot; row 26 keeps its offset from it.
    old25 = np.array(joints[ROW_UPPER]["world"], dtype=float)
    local26 = np.linalg.inv(old25) @ np.array(joints[ROW_HEM]["world"], dtype=float)
    new25 = old25.copy()
    new25[1, 3] = PIVOT_Y
    joints[ROW_UPPER]["world"] = new25.tolist()
    joints[ROW_HEM]["world"] = (new25 @ local26).tolist()
    world = {j: np.array(joints[j]["world"], dtype=float) for j in (ROW_PELVIS, ROW_UPPER, ROW_HEM)}
    inv = {j: np.linalg.inv(m) for j, m in world.items()}
    proto = verts[waist[0]]

    def add(row: int, q: dict, pos=None) -> int:
        p = q["pos"] if pos is None else pos
        n = q["normal"]
        n = n / max(np.linalg.norm(n), 1e-6) * 127.0
        local = (inv[row] @ np.append(p, 1.0))[:3]
        verts.append({"joint": row, "local": [round(float(x), 3) for x in local],
                      "model": [round(float(x), 3) for x in p],
                      "uv": [round(float(x), 5) for x in q["uv"]], "st": None,
                      "flag": proto.get("flag", 0), "normal": [int(round(float(x))) for x in n],
                      "rgba": list(proto["rgba"]), "object": DRESS})
        return len(verts) - 1

    samples = SAMPLES[detail]
    upper = SAMPLES_UPPER[detail]
    cut_pts = [_ring_interp(cut, u) for u in samples]
    hem_pts = [_ring_interp(hem, u) for u in samples]
    centre = np.mean([q["pos"] for q in cut_pts[:-1]], axis=0)
    ring1 = [add(ROW_PELVIS, _ring_interp(cut, u)) for u in upper]
    ring25 = []
    for q in cut_pts:
        p = q["pos"].copy()
        p[0] = centre[0] + (p[0] - centre[0]) * OVERLAP_IN
        p[2] = centre[2] + (p[2] - centre[2]) * OVERLAP_IN
        p[1] += OVERLAP_UP
        ring25.append(add(ROW_UPPER, q, p))
    ring26 = [add(ROW_HEM, q) for q in hem_pts]

    new_tris = []

    def tri(row: int, a: int, b: int, c: int) -> None:
        new_tris.append({"joint": row, "v": [a, b, c], "material": material, "object": DRESS})

    # The upper band: the waist ring (its own u) zipped to the cut ring.
    wu = [verts[i]["uv"][0] for i in waist]
    i = k = 0
    while i < len(waist) - 1 or k < len(ring1) - 1:
        adv_w = k >= len(ring1) - 1 or (i < len(waist) - 1 and wu[i + 1] <= upper[k + 1])
        if adv_w:
            tri(ROW_BODICE, waist[i], ring1[k], waist[i + 1])
            i += 1
        else:
            tri(ROW_BODICE, waist[i], ring1[k], ring1[k + 1])
            k += 1
    # The lower band: quads between the rings, but the slit's.
    for k in range(len(samples) - 1):
        if abs(samples[k] - SLIT[0]) < 1e-4 and abs(samples[k + 1] - SLIT[1]) < 1e-4:
            continue
        tri(ROW_HEM, ring25[k], ring26[k], ring25[k + 1])
        tri(ROW_HEM, ring25[k + 1], ring26[k], ring26[k + 1])
    old = {id(t) for t in skirt}
    scene["triangles"] = [t for t in tris if id(t) not in old] + new_tris
    copied = 0
    for foot in FOOT_ROWS:
        w_foot = np.linalg.inv(np.array(joints[foot]["world"], dtype=float))
        copies: dict[int, int] = {}
        for t in scene["triangles"]:
            if t["joint"] != foot:
                continue
            for k, i in enumerate(t["v"]):
                if verts[i]["joint"] == foot:
                    continue
                if i not in copies:
                    v = dict(verts[i])
                    v["joint"] = foot
                    v["local"] = [round(float(x), 3) for x in (w_foot @ np.append(v["model"], 1.0))[:3]]
                    verts.append(v)
                    copies[i] = len(verts) - 1
                t["v"][k] = copies[i]
        copied += len(copies)
    for j in (ROW_UPPER, ROW_HEM):
        joints[j]["has_list"] = True
    return {"removed": len(skirt), "added": len(new_tris), "waist": len(waist), "ring": len(samples),
            "ankle_vertices_copied": copied}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--scene", type=Path, action="append", required=True)
    args = ap.parse_args()
    for path in args.scene:
        scene = json.loads(path.read_text(encoding="utf-8"))
        detail = scene.get("detail") or ("low" if "low" in path.parent.name else "high")
        info = remesh(scene, "low" if detail == "low" else "high")
        path.write_bytes((json.dumps(scene, indent=0) + "\n").encode("utf-8"))
        print(f"look skirt: {path} {info}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

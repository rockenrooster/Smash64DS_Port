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
  by row 26, open at the slit: an upside-down V, as the approved model's,
  from the old skirt's slit top (a row 25 vertex on the band, below its
  ring) to the hem's two slit vertices (owner, 2026-10-14: the first cut
  left the whole span open, a flat-topped notch from the ring down).

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

Owner p6s (2026-10-15): "skirt behaves like a paper cone, instead of a heavy,
flowy dress" -- one straight band from the hips to the hem. The lower band is
split at MID_Y (about her knees) by a mid ring (split_band), so the dress
bends there:

- row 26 (Peach's hem row) holds the mid ring, its origin moved up the band
  to MID_Y; the hem moves to row 27, Peach's last tree row, hung from row 26
  where row 26's origin was (model_writer.py writes the tree's depth from
  the scene's parents and makes the row's DObj, FTAttributes.setup_parts);
- row 27 was the joint a grab holds the caught fighter by and a heavy item
  rides (FTAttributes.joint_itemheavy_id, Peach's hidden part 3: catch,
  throws, heavy items): that role moves to a new row 28 hung from row 0 as
  row 27 was (the scene's "joint_roles": model_writer.py writes the row and
  points the attributes at it; skirt_keys.py moves those clips' keys);
- the band from row 25's ring to the mid ring is drawn by row 26 from row
  25's seam load, the band from the mid ring to the hem by row 27 from row
  26's: each seam a parent and the child drawn next (row 27 is the last row
  of the tree, row 26's only child);
- the slit's V runs through both bands: the mid ring has a vertex on each of
  its edges (on the line from the slit's top to its hem vertex), the V open
  below them; owner p6t ("make the slit smaller", "a miniscule amount"): its
  top SLIT_DROP lower and its hem opening SLIT_HEM wide (two hem vertices of
  its own);
- the hem's rest radius HEM_SCALE of the approved model's about its centre
  (owner p6t: "hem radius should be 10% smaller at default rest ... while
  keeping A-line"), the mid ring on the line from row 25's ring to it, or
  CONCAVE of its radius in from that line (the dress's slight concave flare;
  the owner's pick of the candidates, 9%);
- the hem moved HEM_SHIFT (x, z), the band tilting about row 25's ring with
  it: her wait (the cloth's rest, skirt_keys.py) stands with one leg well
  behind the other, and the approved model's hem, centred on its bind pose's
  legs, met the back leg (owner p6t: "the dress cone should point more
  backwards to give the legs even space around them"; legclear.py measured
  the room in front of and behind her legs even with it).

    look_skirt.py --scene <scene.json> [--scene ...] [--samples high|low]

Rewrites each scene in place; a scene whose skirt is already on row 26 is
split (once: a skirt on row 27 is left alone).
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np

DRESS = "DS_dress"
# The waist ring's height: the dress's lowest bodice (row 2) vertices
# (look_blend.py's rule), about 328 re-aimed, 327 in the owner's pose.
WAIST_Y = 328.0
HEM_MAX_Y = 90.0
CUT_Y = 288.0
# Row 25's origin (the band's pivot) moved up from Peach's (y 252.6) to her
# hip joints' height, so the whole thigh hangs inside the band that turns.
PIVOT_Y = 280.0
# Rows: the pelvis, the bodice, Peach's dress rows (the hem's row before
# the split; split_band moves it to ROW_HEM3, the mid ring onto ROW_HEM).
ROW_PELVIS, ROW_BODICE, ROW_UPPER, ROW_HEM = 1, 2, 25, 26
ROW_MID, ROW_HEM3 = 26, 27
# Where row 27's role goes (the held joint: FTAttributes.joint_itemheavy_id,
# hidden part 3).
ROW_HELD, HELD_HIDDEN_PART = 28, 3
# The mid ring's height (model units; her knees are at 153, the hem at about
# 65, row 25's ring at 294).
MID_Y = 165.0
# The slit (owner p6t, "make the slit smaller ... a miniscule amount"): its
# opening at the hem in u about its top's u (the hem samples' span SLIT was
# 0.139), and how far down the band its top moves (model units).
SLIT_HEM = 0.125
SLIT_DROP = 12.0
# The hem's radius at rest (a fraction of the approved model's, about its
# centre) and the mid ring's inset from the straight line to it (a fraction
# of its radius; 0: a straight A-line).
HEM_SCALE = 0.9
CONCAVE = 0.09
# The hem's move at rest (model units: x to her left, z forward), the mid
# ring's along the band's line with it.
HEM_SHIFT = (-28.0, -46.0)
# The same rest hem for a look in the owner's own pose (look_blend.py
# --no-reaim): the re-aimed look's hips had turned its skirt 5 degrees
# forward, its hem about 21 units ahead of the owner's model's, before
# HEM_SHIFT; this puts the hem where that look's was (the hems' centres
# measured 2026-10-20), so skirt_keys.py's cloth hangs and keys her clips as
# the owner tuned them. (A rest even about her legs with the shift left to
# the clips' cloth over-tilted her run's dress: "a twitch" from her dash into
# it, the down air's skirt turned up to the camera, owner 2026-10-20.)
HEM_SHIFT_OWNER_POSE = (-33.3, -25.2)
# The row 25 ring: this much above the cut and this fraction of its radius,
# inside the upper band's lower edge.
OVERLAP_UP = 6.0
OVERLAP_IN = 0.96
# Foot rows that take their own copy of the ankle ring (their parent's).
FOOT_ROWS = (19, 24)
# The slit: the hem's open span in u, between the one pair of hem
# neighbours no triangle joins (_slit_span); its top is the highest skirt
# vertex between them above the hem. The approved model's span was (0.58333,
# 0.72222), its top at u 0.6583; the 2026-10-20 base's is (0.63056, 0.76944),
# its top at 0.70, under the belt's red gem (owner 2026-10-20: "slit should
# be more centered under the red gem on her left hip"), and its mid ring puts
# two more vertices between them, on the slit's edges.
# The rings' samples in u (ring_samples()): the lower band's -- the hem's, with
# the slit's two edges: FRONT to u 0.5, then as many as keep each step within
# FRONT_STEP up to the slit, then BACK_SPANS even steps from it round to u 1
# (the approved model's span gives the samples the look had) -- and the
# upper band's cut ring (fewer: the band is short).
FRONT = {"high": [0.0, 0.1, 0.2, 0.3, 0.4, 0.5], "low": [0.0, 0.125, 0.25, 0.375, 0.5]}
FRONT_STEP = {"high": 0.09, "low": 0.14}
BACK_SPANS = {"high": 3, "low": 2}
SAMPLES_UPPER = {
    "high": [0.0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1.0],
    "low": [0.0, 0.16667, 0.33333, 0.5, 0.66667, 0.83333, 1.0],
}


class SkirtError(SystemExit):
    pass


def ring_samples(detail: str, slit) -> list:
    """The lower band's samples in u about a slit span (u0, u1)."""
    out = list(FRONT[detail])
    gap = slit[0] - out[-1]
    n = max(1, int(np.ceil(gap / FRONT_STEP[detail] - 1e-9)))
    out += [round(out[-1] + gap * k / n, 5) for k in range(1, n)]
    out += [round(float(slit[0]), 5), round(float(slit[1]), 5)]
    m = BACK_SPANS[detail]
    out += [round(slit[1] + (1.0 - slit[1]) * k / m, 5) for k in range(1, m)]
    return out + [1.0]


def _slit_span(verts: list, tris: list, hem: list) -> tuple:
    """The hem's open span: the one pair of hem neighbours (by u; `hem` the
    hem's vertex ids) that no triangle joins."""
    joined = {(a, b) for t in tris for a in t["v"] for b in t["v"]}
    us = sorted(hem, key=lambda i: verts[i]["uv"][0])
    gaps = [(float(verts[a]["uv"][0]), float(verts[b]["uv"][0])) for a, b in zip(us, us[1:])
            if (a, b) not in joined and verts[b]["uv"][0] - verts[a]["uv"][0] > 1e-3]
    if len(gaps) != 1:
        raise SkirtError(f"the hem's open spans: {gaps}")
    return gaps[0]


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


def _centre(points: np.ndarray) -> np.ndarray:
    """The x/z centroid of a ring's polygon (its points in order; a last point
    on the first, the seam's copy, dropped)."""
    if len(points) > 2 and np.linalg.norm(points[0] - points[-1]) < 1e-3:
        points = points[:-1]
    x, z = points[:, 0], points[:, 2]
    x1, z1 = np.roll(x, -1), np.roll(z, -1)
    cross = x * z1 - x1 * z
    area = cross.sum() / 2.0
    if abs(area) < 1e-6:
        return np.array([x.mean(), z.mean()])
    return np.array([((x + x1) * cross).sum(), ((z + z1) * cross).sum()]) / (6.0 * area)


def split_band(scene: dict, hem_shift=HEM_SHIFT) -> dict:
    """The lower band (row 25's ring to the hem on row 26) split at MID_Y by
    a mid ring on row 26, the hem moved to row 27 (the module's note)."""
    verts, tris, joints = scene["vertices"], scene["triangles"], scene["joints"]
    if any(t["joint"] == ROW_HEM3 for t in tris):
        return {"skipped": "already split"}
    band = [t for t in tris if t["joint"] == ROW_HEM and t.get("object") == DRESS]
    if not band:
        raise SkirtError("no lower band on row 26 to split")
    material = band[0]["material"]
    used = sorted({i for t in band for i in t["v"]})
    on25 = [i for i in used if verts[i]["joint"] == ROW_UPPER]
    hem = sorted((i for i in used if verts[i]["joint"] == ROW_HEM), key=lambda i: verts[i]["uv"][0])
    y25 = max(verts[i]["model"][1] for i in on25)
    ring25 = sorted((i for i in on25 if verts[i]["model"][1] > y25 - 1.0), key=lambda i: verts[i]["uv"][0])
    apex = [i for i in on25 if verts[i]["model"][1] <= y25 - 1.0]
    if len(apex) != 1 or len(ring25) != len(hem):
        raise SkirtError(f"lower band: ring {len(ring25)}, hem {len(hem)}, slit top {len(apex)}")
    apex = apex[0]
    us = [verts[i]["uv"][0] for i in ring25]
    if any(abs(a - verts[h]["uv"][0]) > 1e-3 for a, h in zip(us, hem)):
        raise SkirtError("the band's ring and hem are not sampled at the same u")
    # The slit's span: the samples whose hem vertices no band triangle joins.
    joined = {(a, b) for t in band for a in t["v"] for b in t["v"]}
    open_k = [k for k in range(len(us) - 1) if (hem[k], hem[k + 1]) not in joined]
    if len(open_k) != 1:
        raise SkirtError(f"slit spans between the band's samples: {open_k}")
    slit_k = open_k[0]
    # Row 26 up the band to MID_Y (along row 25's -z, as row 26 hangs); row
    # 27 where row 26 was.
    w25 = np.array(joints[ROW_UPPER]["world"], dtype=float)
    w_hem = np.array(joints[ROW_HEM]["world"], dtype=float)
    down = (w25[1, 3] - MID_Y) / (w25[1, 3] - w_hem[1, 3])
    w_mid = w_hem.copy()
    w_mid[:3, 3] = w25[:3, 3] + (w_hem[:3, 3] - w25[:3, 3]) * down
    offset = lambda parent, child: [float(x) for x in (np.linalg.inv(parent) @ child)[:3, 3]]
    # Row 27's role (the held joint) to a new row 28, as row 27 was.
    if len(joints) != ROW_HEM3 + 1:
        raise SkirtError(f"the scene has {len(joints)} rows, not Peach's {ROW_HEM3 + 1}")
    joints.append(dict(joints[ROW_HEM3], index=ROW_HELD, base_row=ROW_HEM3, has_list=False))
    scene["joint_roles"] = [{"from": ROW_HEM3, "to": ROW_HELD, "hidden_parts": [HELD_HIDDEN_PART]}]
    joints[ROW_HEM3] = dict(joints[ROW_HEM3], parent=ROW_MID, depth=int(joints[ROW_MID]["depth"]) + 1,
                            world=w_hem.tolist(), translate=offset(w_mid, w_hem), rotate=[0.0, 0.0, 0.0],
                            scale=[1.0, 1.0, 1.0], has_list=True)
    joints[ROW_MID] = dict(joints[ROW_MID], world=w_mid.tolist(), translate=offset(w25, w_mid))
    inv_mid, inv_hem = np.linalg.inv(w_mid), np.linalg.inv(w_hem)
    # The hem's rest radius (owner p6t: "10% smaller at default rest"),
    # about its centre.
    hem_xz = _centre(np.array([verts[i]["model"] for i in hem], dtype=float))
    for i in hem:
        p = np.array(verts[i]["model"], dtype=float)
        p[[0, 2]] = hem_xz + (p[[0, 2]] - hem_xz) * HEM_SCALE + np.asarray(hem_shift, dtype=float)
        verts[i]["model"] = [round(float(x), 3) for x in p]
    for i in hem:
        verts[i]["joint"] = ROW_HEM3
        verts[i]["local"] = [round(float(x), 3) for x in (inv_hem @ np.append(verts[i]["model"], 1.0))[:3]]
    proto = verts[hem[0]]

    def at(a: int, b: int, y: float) -> dict:
        """Where the line from vertex a down to vertex b reaches height y."""
        pa, pb = np.array(verts[a]["model"], dtype=float), np.array(verts[b]["model"], dtype=float)
        s = (pa[1] - y) / (pa[1] - pb[1])
        lerp = lambda key: np.array(verts[a][key], dtype=float) + (np.array(verts[b][key], dtype=float) -
                                                                    np.array(verts[a][key], dtype=float)) * s
        return {"pos": pa + (pb - pa) * s, "uv": lerp("uv"), "normal": lerp("normal")}

    def add(q: dict) -> int:
        n = q["normal"]
        n = n / max(np.linalg.norm(n), 1e-6) * 127.0
        verts.append({"joint": ROW_MID, "local": [round(float(x), 3) for x in (inv_mid @ np.append(q["pos"], 1.0))[:3]],
                      "model": [round(float(x), 3) for x in q["pos"]],
                      "uv": [round(float(x), 5) for x in q["uv"]], "st": None,
                      "flag": proto.get("flag", 0), "normal": [int(round(float(x))) for x in n],
                      "rgba": list(proto["rgba"]), "object": DRESS})
        return len(verts) - 1

    # The mid ring on the line from row 25's ring to the hem (an A-line), or
    # CONCAVE of its radius in from it.
    mid_q = [at(a, h, MID_Y) for a, h in zip(ring25, hem)]
    mid_xz = _centre(np.array([q["pos"] for q in mid_q]))

    def inset(q: dict) -> dict:
        q["pos"][[0, 2]] = mid_xz + (q["pos"][[0, 2]] - mid_xz) * (1.0 - CONCAVE)
        return q

    mid = [add(inset(q)) for q in mid_q]
    # The slit (owner p6t: "make the slit smaller" -- a minuscule change):
    # its top moved SLIT_DROP down the band, its opening at the hem
    # SLIT_HEM wide about the top's u (two hem vertices of its own; it was
    # the hem samples' span), its edges through the mid ring.
    u_c = float(verts[apex]["uv"][0])

    def along(i0: int, i1: int, u: float, row: int) -> int:
        """A vertex on the ring between vertices i0 and i1 at u (row: its row)."""
        t = (u - verts[i0]["uv"][0]) / (verts[i1]["uv"][0] - verts[i0]["uv"][0])
        lerp = lambda key: (np.array(verts[i0][key], dtype=float) +
                            (np.array(verts[i1][key], dtype=float) - np.array(verts[i0][key], dtype=float)) * t)
        q = {"pos": lerp("model"), "uv": lerp("uv"), "normal": lerp("normal")}
        i = add(q)
        if row != ROW_MID:
            verts[i]["joint"] = row
            verts[i]["local"] = [round(float(x), 3) for x in (inv_hem @ np.append(q["pos"], 1.0))[:3]]
        return i
    hem_l = along(hem[slit_k], hem[slit_k + 1], u_c - SLIT_HEM / 2.0, ROW_HEM3)
    hem_r = along(hem[slit_k], hem[slit_k + 1], u_c + SLIT_HEM / 2.0, ROW_HEM3)
    hem_c = along(hem[slit_k], hem[slit_k + 1], u_c, ROW_HEM3)
    q = at(apex, hem_c, float(verts[apex]["model"][1]) - SLIT_DROP)
    w25 = np.array(joints[ROW_UPPER]["world"], dtype=float)
    verts[apex]["model"] = [round(float(x), 3) for x in q["pos"]]
    verts[apex]["local"] = [round(float(x), 3) for x in (np.linalg.inv(w25) @ np.append(q["pos"], 1.0))[:3]]
    verts[apex]["uv"] = [round(float(x), 5) for x in q["uv"]]
    v_left = add(inset(at(apex, hem_l, MID_Y)))
    v_right = add(inset(at(apex, hem_r, MID_Y)))
    new_tris = []

    def tri(row: int, a: int, b: int, c: int) -> None:
        new_tris.append({"joint": row, "v": [a, b, c], "material": material, "object": DRESS})

    for k in range(len(us) - 1):
        a, b, m0, m1, h0, h1 = ring25[k], ring25[k + 1], mid[k], mid[k + 1], hem[k], hem[k + 1]
        if k == slit_k:
            # Above the V's top, either side of it to the mid ring, then
            # either side of it to the hem; open between its edges.
            tri(ROW_MID, a, apex, b)
            tri(ROW_MID, a, m0, v_left)
            tri(ROW_MID, a, v_left, apex)
            tri(ROW_MID, apex, v_right, b)
            tri(ROW_MID, b, v_right, m1)
            tri(ROW_HEM3, m0, h0, hem_l)
            tri(ROW_HEM3, m0, hem_l, v_left)
            tri(ROW_HEM3, v_right, hem_r, m1)
            tri(ROW_HEM3, m1, hem_r, h1)
            continue
        tri(ROW_MID, a, m0, b)
        tri(ROW_MID, b, m0, m1)
        tri(ROW_HEM3, m0, h0, m1)
        tri(ROW_HEM3, m1, h0, h1)
    old = {id(t) for t in band}
    scene["triangles"] = [t for t in tris if id(t) not in old] + new_tris
    return {"split": len(band), "upper": sum(1 for t in new_tris if t["joint"] == ROW_MID),
            "lower": sum(1 for t in new_tris if t["joint"] == ROW_HEM3), "mid_ring": len(mid) + 2,
            "mid_y": MID_Y, "hem_scale": HEM_SCALE, "concave": CONCAVE, "hem_shift": [float(x) for x in hem_shift]}


def remesh(scene: dict, detail: str) -> dict:
    verts, tris, joints = scene["vertices"], scene["triangles"], scene["joints"]
    if any(verts[i]["joint"] in (ROW_HEM, ROW_HEM3) for t in tris for i in t["v"]):
        return {"skipped": "already on row 26"}
    waist_y = min((v["model"][1] for v in verts if v.get("object") == DRESS and v["joint"] == ROW_BODICE),
                  default=WAIST_Y)
    skirt = [t for t in tris if t.get("object") == DRESS and
             any(verts[i]["model"][1] < waist_y - 1.0 for i in t["v"])]
    if not skirt:
        raise SkirtError("no skirt triangles")
    material = skirt[0]["material"]
    used = sorted({i for t in skirt for i in t["v"]})
    waist = sorted((i for i in used if abs(verts[i]["model"][1] - waist_y) < 1.0 and
                    verts[i]["joint"] == ROW_BODICE), key=lambda i: verts[i]["uv"][0])
    hem_ids = [i for i in used if verts[i]["model"][1] < HEM_MAX_Y]
    hem = sorted((_point(verts[i]) for i in hem_ids), key=lambda q: q["u"])
    slit = _slit_span(verts, skirt, hem_ids)
    cut = _cut_ring(verts, skirt, CUT_Y)
    if len(waist) < 4 or len(hem) < 4 or len(cut) < 4:
        raise SkirtError(f"rings: waist {len(waist)}, hem {len(hem)}, cut {len(cut)}")
    apex = [_point(verts[i]) for i in used if slit[0] + 1e-4 < verts[i]["uv"][0] < slit[1] - 1e-4
            and HEM_MAX_Y < verts[i]["model"][1] < CUT_Y]
    if not apex:
        raise SkirtError("slit top: no vertex between the slit's hem vertices")
    apex = [max(apex, key=lambda q: q["pos"][1])]
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

    samples = ring_samples(detail, slit)
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
    # The slit's top on the lower band: where the band's line at its u (row
    # 25's ring to the hem) reaches its height, with its own uv (the dress
    # texture's slit trim was baked to it). On row 25, which only turns, so
    # a flared hem leaves it within a few units of the band.
    top = apex[0]
    q_top = _ring_interp(cut, top["u"])
    p_top = q_top["pos"].copy()
    p_top[0] = centre[0] + (p_top[0] - centre[0]) * OVERLAP_IN
    p_top[2] = centre[2] + (p_top[2] - centre[2]) * OVERLAP_IN
    p_top[1] += OVERLAP_UP
    q_hem = _ring_interp(hem, top["u"])
    along = (p_top[1] - top["pos"][1]) / (p_top[1] - q_hem["pos"][1])
    slit_top = add(ROW_UPPER, {"u": top["u"], "uv": top["uv"],
                               "normal": q_top["normal"] + (q_hem["normal"] - q_top["normal"]) * along},
                   p_top + (q_hem["pos"] - p_top) * along)

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
    # The lower band: quads between the rings; the slit's span closes above
    # and beside its V.
    for k in range(len(samples) - 1):
        if abs(samples[k] - slit[0]) < 1e-4 and abs(samples[k + 1] - slit[1]) < 1e-4:
            tri(ROW_HEM, ring25[k], slit_top, ring25[k + 1])
            tri(ROW_HEM, ring25[k], ring26[k], slit_top)
            tri(ROW_HEM, slit_top, ring26[k + 1], ring25[k + 1])
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
            "slit": [round(u, 5) for u in slit], "slit_top_u": round(top["u"], 5), "ankle_vertices_copied": copied}


# Her legs at rest: every leg point between the hem and the waist this far
# (model units) inside the dress as drawn (leg_clearance).
LEG_CLEAR = 2.0


def _ray_tri(o: np.ndarray, d: np.ndarray, a: np.ndarray, b: np.ndarray, c: np.ndarray):
    """Distance along the ray o + t d to triangle abc, or None."""
    e1, e2 = b - a, c - a
    p = np.cross(d, e2)
    det = float(e1 @ p)
    if abs(det) < 1e-9:
        return None
    s = o - a
    u = float(s @ p) / det
    if u < 0.0 or u > 1.0:
        return None
    q = np.cross(s, e1)
    v = float(d @ q) / det
    if v < 0.0 or u + v > 1.0:
        return None
    t = float(e2 @ q) / det
    return t if t > 0.0 else None


def leg_clearance(scene: dict) -> dict:
    """Her legs inside the dress at rest (owner 2026-10-20: "left thigh clips
    through dress" in the default pose; the approved model's thigh tops pass
    within half a unit of its dress, and this dress -- the hem's HEM_SCALE,
    the CONCAVE mid ring, row 25's ring drawn in -- sits a few units inside
    that surface just below the hips). A leg vertex between the hem and the
    waist that is not LEG_CLEAR inside the dress, measured along the level
    ray from the dress's axis at its height (the waist ring's centre to the
    hem's), moves in along that ray to LEG_CLEAR inside; a point the ray
    reaches through the slit is left (it shows through it, as the approved
    model's). Each copy of a point (its rows' and uvs') moves with it."""
    verts, tris, joints = scene["vertices"], scene["triangles"], scene["joints"]
    dress = [[np.array(verts[i]["model"], dtype=float) for i in t["v"]] for t in tris if t.get("object") == DRESS]
    used = sorted({i for t in tris if t.get("object") == DRESS for i in t["v"]})
    waist_y = min(verts[i]["model"][1] for i in used if verts[i]["joint"] == ROW_BODICE)
    waist = np.array([verts[i]["model"] for i in used if verts[i]["joint"] == ROW_BODICE
                      and verts[i]["model"][1] < waist_y + 1.0], dtype=float)
    hem = np.array([verts[i]["model"] for i in used if verts[i]["model"][1] < HEM_MAX_Y], dtype=float)
    cw, ch = waist.mean(axis=0), hem.mean(axis=0)
    legs = [i for i, v in enumerate(verts) if v.get("object") == "DS_legs"]
    by_pos: dict = {}
    for i in legs:
        by_pos.setdefault(tuple(round(float(x), 3) for x in verts[i]["model"]), []).append(i)
    inv = {}
    moved, worst = 0, 0.0
    for key, ids in by_pos.items():
        p = np.array(key, dtype=float)
        if not ch[1] + 5.0 < p[1] < waist_y - 2.0:
            continue
        t = (waist_y - p[1]) / (waist_y - ch[1])
        c = cw + (ch - cw) * t
        c[1] = p[1]
        d = p - c
        d[1] = 0.0
        r = float(np.linalg.norm(d))
        if r < 1e-6:
            continue
        d /= r
        hits = [h for h in (_ray_tri(c, d, *tri) for tri in dress) if h is not None]
        if not hits:
            continue
        want = min(hits) - LEG_CLEAR
        if r <= want:
            continue
        worst = max(worst, r - min(hits))
        q = c + d * want
        q[1] = p[1]
        for i in ids:
            row = verts[i]["joint"]
            if row not in inv:
                inv[row] = np.linalg.inv(np.array(joints[row]["world"], dtype=float))
            verts[i]["model"] = [round(float(x), 3) for x in q]
            verts[i]["local"] = [round(float(x), 3) for x in (inv[row] @ np.append(q, 1.0))[:3]]
        moved += 1
    return {"leg_points_moved": moved, "worst_outside": round(worst, 2), "clear": LEG_CLEAR}


def band_normals(scene: dict) -> dict:
    """The lower band's vertex normals from its own surface, in each
    vertex's row frame (owner 2026-10-19: "the lower half of her dress is
    darker" on the results screen).

    remesh and split_band carried each new vertex's normal over from the old
    dress vertices it was interpolated from: a normal in THEIR rows' frames,
    stored on rows 25, 26 and 27, whose rest frames are turned from those.
    The engine turns a normal by its own row's matrix, so the band's normals
    pointed off the surface -- some inward or down (measured outward share
    0.39-0.47, against the bodice's 0.93): the band lit dark wherever the
    light came from the front. Each normal is the area-weighted mean of its
    band triangles' (model space, each turned outward from the dress's
    axis), taken into its row's frame as look_blend.py takes the look's."""
    verts, tris, joints = scene["vertices"], scene["triangles"], scene["joints"]
    band = [t for t in tris if t.get("object") == DRESS and t["joint"] in (ROW_MID, ROW_HEM3)]
    if not band:
        raise SkirtError("no lower band on rows 26 and 27")
    used = sorted({i for t in band for i in t["v"]})
    rows = {verts[i]["joint"] for i in used}
    if not rows <= {ROW_UPPER, ROW_MID, ROW_HEM3}:
        raise SkirtError(f"lower band vertices on rows {sorted(rows)}")
    pos = {i: np.array(verts[i]["model"], dtype=float) for i in used}
    # Each ring's own axis (the hem sits back of the waist, HEM_SHIFT): a
    # vertex's outward is from its row's ring centre.
    centre = {j: _centre(np.array([pos[i] for i in used if verts[i]["joint"] == j]))
              for j in rows}
    axis_of = lambda i: centre[verts[i]["joint"]]
    nsum = {i: np.zeros(3) for i in used}
    for t in band:
        a, b, c = (pos[i] for i in t["v"])
        fn = np.cross(b - a, c - a)
        mid = (a + b + c) / 3.0
        ax = np.mean([axis_of(i) for i in t["v"]], axis=0)
        out = np.array([mid[0] - ax[0], 0.0, mid[2] - ax[1]])
        if float(np.dot(fn, out)) < 0.0:
            fn = -fn
        for i in t["v"]:
            nsum[i] += fn
    inv = {j: np.linalg.inv(np.array(joints[j]["world"], dtype=float)) for j in rows}
    outward = []
    for i in used:
        n = nsum[i] / (np.linalg.norm(nsum[i]) or 1.0)
        p, ax = pos[i], axis_of(i)
        out = np.array([p[0] - ax[0], 0.0, p[2] - ax[1]])
        outward.append(float(np.dot(n, out / (np.linalg.norm(out) or 1.0))))
        nl = inv[verts[i]["joint"]][:3, :3] @ n
        nl = nl / (np.linalg.norm(nl) or 1.0)
        verts[i]["normal"] = [int(round(float(c) * 127)) for c in nl]
    return {"vertices": len(used), "outward_min": round(min(outward), 2),
            "outward_mean": round(float(np.mean(outward)), 2)}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--scene", type=Path, action="append", required=True)
    ap.add_argument("--hem-shift", type=lambda t: tuple(float(x) for x in t.split(",")), default=None,
                    help="the hem's move at rest, x,z model units (default HEM_SHIFT, or HEM_SHIFT_OWNER_POSE "
                         "for a scene in the owner's pose)")
    args = ap.parse_args()
    for path in args.scene:
        scene = json.loads(path.read_text(encoding="utf-8"))
        detail = scene.get("detail") or ("low" if "low" in path.parent.name else "high")
        info = remesh(scene, "low" if detail == "low" else "high")
        hem_shift = args.hem_shift
        if hem_shift is None:
            hem_shift = HEM_SHIFT_OWNER_POSE if scene.get("reaim") is False else HEM_SHIFT
        info["mid"] = split_band(scene, hem_shift)
        info["legs"] = leg_clearance(scene)
        info["normals"] = band_normals(scene)
        path.write_bytes((json.dumps(scene, indent=0) + "\n").encode("utf-8"))
        print(f"look skirt: {path} {info}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

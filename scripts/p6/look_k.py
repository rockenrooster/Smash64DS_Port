#!/usr/bin/env python3
"""Track K (P6 A3b): the kit's model rebuilt by ring retopology.

Each part of the look is a tube along one of the base's bones (or an axis
between two points the ops name): rings at stations along the axis, each
fitted to the fitted kit surface's cross-section there, bridged into
triangles and capped. A ring vertex sits where the ray from the station's
centre at its angle leaves the kit's cross-section (the outermost crossing),
so a ring is the radial hull of the kit's slice. Rings a parent and child
share are the stitch rings: the child's first ring IS the parent's last, so
its first band names the parent's vertices (the writer's cross-joint
triangles) and bends stay closed.

Shapes: "tube" (rings, bands, optional caps and open quads for a slit),
"plate" (one ring, a double-sided fan: the shield), "ribbon" (two-vertex
rings: a hair card, double-sided material).

The ops (art/p6/<name>/look/k_<detail>.json, committed) hold only parameters:
parts, joints, axes, stations, sides, twist, the kit regions a part may slice,
caps, shapes, materials and textures (size, mapping) and per-vertex
hand-tuning offsets. Kit geometry and texels come from the fitted kit at build
time (plan D10). Axis points: a base joint row, {"joint", "offset"}, or
{"kit_pca": <kit joint>, "end": 0|1, "extend": f} -- the ends of the principal
axis of the kit vertices that joint dominates (the staff).

Textures are baked from the kit albedo (bake.py) and quantized to CI4; an
untextured material's colour is the kit albedo's mean over its faces.

Output: a model_scene.py-style scene (joints = the base's; vertices with
their joint, model and local position and UV; triangles with a material;
materials with prim colour or texture) for import_scene.py and the writer.
"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import look_r as R  # noqa: E402  (kit region labels, closest points)
import bake as B  # noqa: E402


def _basis(d: np.ndarray, hint=(0.0, 0.0, 1.0)):
    d = d / np.linalg.norm(d)
    h = np.array(hint, dtype=float)
    e1 = h - d * (h @ d)
    if np.linalg.norm(e1) < 1e-3:
        h = np.array((0.0, 1.0, 0.0))
        e1 = h - d * (h @ d)
    e1 /= np.linalg.norm(e1)
    e2 = np.cross(d, e1)
    return e1, e2


class KitSurface:
    def __init__(self, fit: dict, role: str):
        tris = R.kit_triangles(fit, role)
        self.region = np.array([t[5] for t in tris])
        self.A = np.array([t[2] for t in tris])
        self.B = np.array([t[3] for t in tris])
        self.C = np.array([t[4] for t in tris])
        self.fit = fit

    def select(self, regions):
        if regions is None:
            return np.ones(len(self.region), dtype=bool)
        return np.isin(self.region, list(regions))

    def slice_radii(self, centre, d, e1, e2, angles, mask):
        """Outermost crossing of each ray (centre, angle) with the kit's
        slice by the plane through `centre` normal to `d` (None: no crossing)."""
        A, B_, C = self.A[mask], self.B[mask], self.C[mask]
        sa, sb, sc = (A - centre) @ d, (B_ - centre) @ d, (C - centre) @ d
        ends = [[] for _ in range(len(A))]
        for (p, q, sp, sq) in ((A, B_, sa, sb), (B_, C, sb, sc), (C, A, sc, sa)):
            cross = (sp * sq) < 0
            t = sp[cross] / (sp[cross] - sq[cross])
            pts = p[cross] + (q[cross] - p[cross]) * t[:, None]
            for k, i in enumerate(np.nonzero(cross)[0]):
                ends[i].append(pts[k])
        P = [e[0] for e in ends if len(e) >= 2]
        Q = [e[1] for e in ends if len(e) >= 2]
        if not P:
            return [None] * len(angles)
        P = np.array(P) - centre
        Q = np.array(Q) - centre
        p2 = np.stack([P @ e1, P @ e2], 1)
        seg = np.stack([Q @ e1, Q @ e2], 1) - p2
        out = []
        for a in angles:
            r = np.array([math.cos(a), math.sin(a)])
            den = r[0] * seg[:, 1] - r[1] * seg[:, 0]
            ok = np.abs(den) > 1e-12
            safe = np.where(ok, den, 1)
            s = np.where(ok, (p2[:, 0] * seg[:, 1] - p2[:, 1] * seg[:, 0]) / safe, -1)
            u = np.where(ok, (p2[:, 0] * r[1] - p2[:, 1] * r[0]) / safe, -1)
            hit = ok & (s >= 0) & (u >= 0) & (u <= 1)
            out.append(float(s[hit].max()) if hit.any() else None)
        return out

    def kit_points(self, joint_name: str, mesh: str | None = None):
        pts = []
        for m in self.fit["meshes"]:
            if mesh is not None and m["name"] != mesh:
                continue
            for p, ws in zip(m["positions"], m["weights"]):
                if ws and max(ws, key=lambda x: x[1])[0] == joint_name:
                    pts.append(p)
        return np.array(pts, dtype=float)

    def kit_pca(self, joint_name: str, mesh: str | None = None):
        """Centre, principal axes (rows, longest first) and the extent along
        each, of the kit vertices `joint_name` dominates."""
        pts = self.kit_points(joint_name, mesh)
        c = pts.mean(0)
        _u, _s, vt = np.linalg.svd(pts - c)
        proj = (pts - c) @ vt.T
        return c, vt, proj.min(0), proj.max(0)

    def kit_joint(self, name: str):
        for j in self.fit["joints"]:
            if j["name"] == name:
                return np.array(j["pos"], dtype=float)
        raise SystemExit(f"no kit joint {name}")


# The bake region a ring part takes its texels from (bake_atlas.py: a face
# reads only the kit faces of its own region), by part name.
_PART_REGIONS = (("hair", "hair"), ("head", "head"), ("neck", "head"), ("tiara", "head"),
                 ("torso", "torso"), ("arm", "arms"), ("hand", "arms"), ("shin", "legs"),
                 ("thigh", "legs"), ("foot", "feet"), ("dress", "dress"), ("staff", "staff"),
                 ("shield", "shield"))


def _default_region(name: str) -> str:
    for key, region in _PART_REGIONS:
        if key in name:
            return region
    return "torso"


def _hex(c: str):
    return tuple(int(c[i:i + 2], 16) for i in (0, 2, 4))


def _drawn_texture(tex: dict, stem: str, out: Path) -> dict:
    from PIL import Image, ImageDraw
    w, h = tex["size"]
    k = 8
    img = Image.new("RGBA", (w * k, h * k), (0, 0, 0, 0))
    dr = ImageDraw.Draw(img)
    for shape in tex["draw"]:
        if "circle" in shape:
            cx, cy, r = (v * k for v in shape["circle"])
            dr.ellipse((cx - r, cy - r, cx + r, cy + r), fill=_hex(shape["fill"]) + (255,))
        elif "ring" in shape:
            cx, cy, r = (v * k for v in shape["ring"])
            dr.ellipse((cx - r, cy - r, cx + r, cy + r), outline=_hex(shape["color"]) + (255,),
                       width=int(shape["width"] * k))
        elif "arc" in shape:
            cx, cy, r, a0, a1 = shape["arc"]
            cx, cy, r = cx * k, cy * k, r * k
            dr.arc((cx - r, cy - r, cx + r, cy + r), a0, a1, fill=_hex(shape["color"]) + (255,),
                   width=int(shape["width"] * k))
        elif "rect" in shape:
            x0, y0, x1, y1 = (v * k for v in shape["rect"])
            dr.rectangle((x0, y0, x1 - 1, y1 - 1), fill=_hex(shape["fill"]) + (255,))
        elif "polygon" in shape:
            dr.polygon([(x * k, y * k) for x, y in shape["polygon"]], fill=_hex(shape["fill"]) + (255,))
    small = img.resize((w, h), Image.BOX)
    a = np.asarray(small, dtype=float)
    opaque = a[:, :, 3] >= 128
    # Un-premultiply the box filter's edge colours.
    rgb = np.where(a[:, :, 3:4] > 0, a[:, :, :3] * 255.0 / np.maximum(a[:, :, 3:4], 1), 0)
    colors = tex.get("colors", 16)
    q = Image.fromarray(rgb.clip(0, 255).astype(np.uint8), "RGB").quantize(
        colors=colors - 1, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    idx = np.asarray(q, dtype=np.uint8) + 1
    idx = np.where(opaque, idx, 0).astype(np.uint8)
    pal = q.getpalette()[:(colors - 1) * 3]
    pal += [0] * ((colors - 1) * 3 - len(pal))
    palette = [(0, 0, 0, 0)] + [(pal[i * 3], pal[i * 3 + 1], pal[i * 3 + 2], 255) for i in range(colors - 1)]
    name = f"{stem}_{w}x{h}.png"
    rgba = np.array([palette[i] for i in idx.flatten()], dtype=np.uint8).reshape(h, w, 4)
    Image.fromarray(rgba, "RGBA").save(out / name)
    return {"file": name, "width": w, "height": h, "fmt": 2, "siz": 0, "alpha": True,
            "palette": [list(c) for c in palette], "texels": idx.tolist(), "wrap": ["clamp", "clamp"]}


class Builder:
    def __init__(self, base: dict, fit: dict, ops: dict, role: str):
        self.base = base
        self.ops = ops
        self.surf = KitSurface(fit, role)
        # The base's joints at the kit's proportions (kit_fit.py `skeleton`,
        # no stretching): the look is built on, and bound to, these.
        worlds = [np.array(j["world"], dtype=float) for j in base["joints"]]
        if fit.get("skeleton"):
            for w, p in zip(worlds, fit["skeleton"]):
                w[:3, 3] = p
        self.worlds = worlds
        self.jpos = [w[:3, 3].copy() for w in worlds]
        self.inv = [np.linalg.inv(w) for w in worlds]
        self.positions = []     # (joint, xyz)
        self.faces = []         # (part, material, [(pos, uv_param)] * 3)
        self.rings = {}         # "part:station" -> [pos ids] (sides + 1, last = seam copy of first)

    def point_of(self, spec):
        if isinstance(spec, int):
            return self.jpos[spec]
        if isinstance(spec, dict) and "kit_pca" in spec:
            # An end of the principal axis (component `axis`, default the
            # longest) of the kit vertices a kit joint dominates.
            c, vt, lo, hi = self.surf.kit_pca(spec["kit_pca"], spec.get("mesh"))
            k = spec.get("axis", 0)
            ext = spec.get("extend", 0.0)
            span = hi[k] - lo[k]
            if spec.get("end", 0) == "centre":
                p = c
            elif spec.get("end", 0) == 0:
                p = c + vt[k] * (lo[k] - span * ext)
            else:
                p = c + vt[k] * (hi[k] + span * ext)
            return p + np.array(spec.get("offset", (0, 0, 0)), dtype=float)
        if isinstance(spec, dict) and "kit_joint" in spec:
            return self.surf.kit_joint(spec["kit_joint"]) + np.array(spec.get("offset", (0, 0, 0)), dtype=float)
        if isinstance(spec, dict):
            return self.jpos[spec["joint"]] + np.array(spec.get("offset", (0, 0, 0)), dtype=float)
        return np.array(spec, dtype=float)

    def add(self, joint, p):
        self.positions.append((joint, np.array(p, dtype=float)))
        return len(self.positions) - 1

    def part(self, part):
        shape = part.get("shape", "tube")
        joint = part["joint"]
        p0, p1 = self.point_of(part["axis"][0]), self.point_of(part["axis"][1])
        d = p1 - p0
        length = float(np.linalg.norm(d))
        d = d / length
        e1, e2 = _basis(d, part.get("facing", (0.0, 0.0, 1.0)))
        if shape == "sprite":
            # A textured quad, two triangles on a two-sided material (owner
            # 2026-10-13: the staff's wings and the shield as sprites):
            # centred on the axis start, `size` (width along the facing
            # hint, height across it) in the plane normal to the axis.
            # "billboard" marks one the runtime turns to the camera (the
            # staff's gem); the look shows it as built.
            w, h = part["size"]
            mat = part.get("material", part["name"])
            quad = [p0 - e1 * w / 2 - e2 * h / 2, p0 + e1 * w / 2 - e2 * h / 2,
                    p0 + e1 * w / 2 + e2 * h / 2, p0 - e1 * w / 2 + e2 * h / 2]
            ids = [self.add(joint, q) for q in quad]
            uvs = [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)]
            self.faces.append((part, mat, [(ids[0], uvs[0]), (ids[1], uvs[1]), (ids[2], uvs[2])]))
            self.faces.append((part, mat, [(ids[0], uvs[0]), (ids[2], uvs[2]), (ids[3], uvs[3])]))
            return
        sides = 2 if shape == "ribbon" else part["sides"]
        twist = math.radians(part.get("twist", 0.0))
        mask = self.surf.select(part.get("regions"))
        mat = part.get("material", part["name"])
        stations = part["stations"]
        tune = part.get("tune", {})          # "station:index" -> [dx, dy, dz]
        if shape == "ribbon":
            angles = [twist, twist + math.pi]
        else:
            angles = [twist + 2 * math.pi * i / sides for i in range(sides)]
        ring_ids = []
        prev_r = None
        for si, t in enumerate(stations):
            key = f"{part['name']}:{si}"
            share = part.get("share", {}).get(str(si))
            if share is not None:
                ring_ids.append(self.rings[share])
                self.rings[key] = self.rings[share]
                continue
            c = p0 + d * (t * length)
            if "radii" in part:
                # Authored radii, one per side (the staff's crest, the
                # shield: flat kit parts a slice cannot measure).
                radii = list(part["radii"])
            else:
                radii = self.surf.slice_radii(c, d, e1, e2, angles, mask)
            known = [r for r in radii if r is not None]
            fb = (sum(known) / len(known)) if known else (prev_r or part.get("radius", 10.0))
            scl = part.get("scale", 1.0)
            if isinstance(scl, list):
                scl = scl[si]
            radii = [max((r if r is not None else fb) * scl, part.get("min_radius", 0.5)) for r in radii]
            if "width" in part and shape == "ribbon":
                w = part["width"][si] if isinstance(part["width"], list) else part["width"]
                radii = [w / 2.0, w / 2.0]
            prev_r = sum(radii) / len(radii)
            ids = []
            for i, (a, r) in enumerate(zip(angles, radii)):
                p = c + (e1 * math.cos(a) + e2 * math.sin(a)) * r
                p = p + np.array(tune.get(f"{si}:{i}", (0, 0, 0)), dtype=float)
                ids.append(self.add(joint, p))
            ids.append(ids[0])      # the seam column (its own UV copy later)
            self.rings[key] = ids
            ring_ids.append(ids)
        n = len(angles)
        vfrac = [(t - stations[0]) / max(1e-9, stations[-1] - stations[0]) for t in stations]
        skip = set(part.get("open", ()))
        for k, (a_ids, b_ids) in enumerate(zip(ring_ids, ring_ids[1:])):
            if len(a_ids) != len(b_ids):
                raise SystemExit(f"{part['name']}: ring sizes differ ({len(a_ids)} / {len(b_ids)})")
            bands = 1 if shape == "ribbon" else n
            for i in range(bands):
                if i in skip:
                    continue
                ua, ub = i / bands, (i + 1) / bands
                a0, a1, b0, b1 = a_ids[i], a_ids[i + 1], b_ids[i], b_ids[i + 1]
                c00, c01 = (a0, (ua, vfrac[k])), (a1, (ub, vfrac[k]))
                c10, c11 = (b0, (ua, vfrac[k + 1])), (b1, (ub, vfrac[k + 1]))
                qmat = mat
                qregion = None
                if "front_region" in part and shape != "ribbon":
                    # Quads facing the part's `facing` direction read the
                    # front region in the bake (a face), the rest the part's.
                    mid = twist + 2 * math.pi * (i + 0.5) / n
                    off = math.degrees(math.atan2(math.sin(mid), math.cos(mid)))
                    if abs(off) <= part.get("front_span", 70.0):
                        qregion = part["front_region"]
                if "front_material" in part and shape != "ribbon":
                    # Quads facing the part's `facing` direction (angle 0)
                    # take the front material (a face on a head).
                    mid = twist + 2 * math.pi * (i + 0.5) / n
                    off = math.degrees(math.atan2(math.sin(mid), math.cos(mid)))
                    if abs(off) <= part.get("front_span", 70.0):
                        qmat = part["front_material"]
                fpart = part if qregion is None else dict(part, bake_region=qregion)
                self.faces.append((fpart, qmat, [c00, c01, c11]))
                self.faces.append((fpart, qmat, [c00, c11, c10]))
        for end in ("start", "end"):
            flag = part.get(f"cap_{end}")
            if not flag:
                continue
            ids = ring_ids[0] if end == "start" else ring_ids[-1]
            t = stations[0] if end == "start" else stations[-1]
            ext = flag if (isinstance(flag, (int, float)) and not isinstance(flag, bool)) else 0.0
            tip = self.add(joint, p0 + d * ((t + (ext if end == "end" else -ext)) * length))
            vt = 0.0 if end == "start" else 1.0
            for i in range(n):
                a, b = ids[i], ids[i + 1]
                corners = [(a, (i / n, vt)), (tip, ((i + 0.5) / n, vt)), (b, ((i + 1) / n, vt))]
                if end == "end":
                    corners = [corners[0], corners[2], corners[1]]
                self.faces.append((part, part.get("cap_material", mat), corners))
        if shape == "plate":
            ids = ring_ids[0]
            # Texture parameters: "disc" maps the plate onto a centred circle
            # (a shield's emblem); otherwise each fan triangle spans the image.
            disc = part.get("uv") == "disc"

            def duv(i):
                a = angles[i % n]
                return (0.5 + 0.5 * math.cos(a), 0.5 + 0.5 * math.sin(a))
            if part.get("fan", True):
                centre = self.add(joint, p0)
                for i in range(n):
                    a, b = ids[i], ids[i + 1]
                    ua, ub, uc = (duv(i), duv(i + 1), (0.5, 0.5)) if disc else ((0, 0), (1, 0), (0.5, 1))
                    for corners in ([(a, ua), (b, ub), (centre, uc)],
                                    [(a, ua), (centre, uc), (b, ub)]):
                        self.faces.append((part, mat, corners))
            else:
                # No centre vertex: the polygon as n - 2 triangles a side.
                for i in range(1, n - 1):
                    tri = [(ids[0], duv(0) if disc else (0, 0)), (ids[i], duv(i) if disc else (1, 0)),
                           (ids[i + 1], duv(i + 1) if disc else (0.5, 1))]
                    self.faces.append((part, mat, tri))
                    self.faces.append((part, mat, [tri[0], tri[2], tri[1]]))

    def finish(self, albedo: B.KitAlbedo | None, out: Path) -> dict:
        """Vertices per (position, material) with UVs; materials baked."""
        mats_ops = self.ops.get("materials", {})
        vertices, triangles, materials, mat_index, vkey = [], [], [], {}, {}
        by_mat = {}
        for part, mat, corners in self.faces:
            by_mat.setdefault(mat, []).append((part, corners))
        for mat, faces in by_mat.items():
            spec = mats_ops.get(mat, {})
            entry = {"name": mat, "combine": [0, 0], "lit": True, "prim": "ffffffff",
                     "texture": None, "two_sided": bool(spec.get("two_sided"))}
            tex = spec.get("texture")
            # Texture mapping: "cylinder" (the parts' ring parameters) or
            # "planar" (base x/y over the material's bounds: a face).
            def uv_of(pos_id, param):
                if tex and tex.get("uv") == "planar":
                    lo, hi = bounds
                    p = self.positions[pos_id][1]
                    return ((p[0] - lo[0]) / (hi[0] - lo[0]), (p[1] - lo[1]) / (hi[1] - lo[1]))
                if tex and "rect" in tex:
                    x0, y0, x1, y1 = tex["rect"]
                    return (x0 + param[0] * (x1 - x0), y0 + param[1] * (y1 - y0))
                return param
            pts = np.array([self.positions[c[0]][1] for _p, corners in faces for c in corners])
            bounds = (pts.min(0), pts.max(0))
            if tex and "bounds" in tex:
                lo = np.array(tex["bounds"][0], dtype=float)
                hi = np.array(tex["bounds"][1], dtype=float)
                bounds = (lo, hi)
            mi = len(materials)
            materials.append(entry)
            tri_uv, tri_xyz = [], []
            for part, corners in faces:
                vids = []
                for pos_id, param in corners:
                    uv = uv_of(pos_id, param)
                    key = (pos_id, mat, round(uv[0], 4), round(uv[1], 4))
                    if key not in vkey:
                        joint, p = self.positions[pos_id]
                        local = self.inv[joint] @ np.append(p, 1.0)
                        vkey[key] = len(vertices)
                        vertices.append({"joint": joint, "address": len(vertices) * 16,
                                         "local": [int(round(float(c))) for c in local[:3]],
                                         "model": [round(float(c), 3) for c in p],
                                         "uv": [float(uv[0]), float(uv[1])],
                                         "st": (0, 0), "flag": 0, "rgba": [0, 0, 0, 255],
                                         "normal": [0, 0, 0]})
                    vids.append(vkey[key])
                triangles.append({"joint": part["joint"], "v": vids, "material": mi,
                                  "part": part["name"],
                                  "region": part.get("bake_region", _default_region(part["name"]))})
                if part.get("billboard"):
                    triangles[-1]["billboard"] = True
                tri_uv.append([vertices[v]["uv"] for v in vids])
                tri_xyz.append([vertices[v]["model"] for v in vids])
            regions = spec.get("regions")
            if tex and "draw" in tex:
                # An authored sprite: shapes in texel units drawn at 8x and
                # reduced, cut out where less than half covered (index 0 is
                # the transparent colour) -- parameters, no kit texels.
                entry["texture"] = _drawn_texture(tex, f"k_{mat}", out)
            elif tex and "rings" in tex:
                # An authored disc (the shield's emblem): concentric rings
                # of colour by radius, the innermost shaded from a light to a
                # dark edge across the disc -- parameters, no kit texels.
                from PIL import Image
                w, h = tex["size"]
                img = np.zeros((h, w, 4), dtype=float)
                light = np.array([int(tex.get("light", "ffffff")[i:i + 2], 16) for i in (0, 2, 4)], dtype=float)
                for y in range(h):
                    for x in range(w):
                        u, v = (x + 0.5) / w - 0.5, (y + 0.5) / h - 0.5
                        r = 2.0 * math.hypot(u, v)
                        colour = None
                        for edge, hexrgb in tex["rings"]:
                            if r <= edge:
                                colour = np.array([int(hexrgb[i:i + 2], 16) for i in (0, 2, 4)], dtype=float)
                                break
                        if colour is None:
                            colour = np.array([int(tex["rings"][-1][1][i:i + 2], 16) for i in (0, 2, 4)], dtype=float)
                        if r <= tex["rings"][0][0] and "light" in tex:
                            # Lit from the upper left: up to half the way to `light`.
                            k = max(0.0, min(1.0, 0.5 - (u - v) * 1.2))
                            colour = colour + (light - colour) * k * 0.5
                        img[y, x, :3] = colour
                        img[y, x, 3] = 255
                ci, palette = B.quantize_ci(img.astype(np.uint8), tex.get("colors", 16))
                name = f"k_{mat}_{w}x{h}.png"
                pal_img = Image.fromarray(ci, "P")
                pal_img.putpalette([c for rgba in palette for c in rgba[:3]])
                pal_img.convert("RGBA").save(out / name)
                entry["texture"] = {"file": name, "width": w, "height": h, "fmt": 2, "siz": 0,
                                    "palette": [list(c) for c in palette], "texels": ci.tolist(),
                                    "wrap": ["clamp", "clamp"]}
            elif albedo is None:
                entry["prim"] = spec.get("color", "ffffffff")
            elif tex:
                w, h = tex["size"]
                img, covered = B.bake_texture(albedo, w, h, tri_uv, tri_xyz, regions)
                ci, palette = B.quantize_ci(img, tex.get("colors", 16))
                name = f"k_{mat}_{w}x{h}.png"
                from PIL import Image
                pal_img = Image.fromarray(ci, "P")
                pal_img.putpalette([c for rgba in palette for c in rgba[:3]])
                pal_img.convert("RGBA").save(out / name)
                entry["texture"] = {"file": name, "width": w, "height": h, "fmt": 2, "siz": 0,
                                    "palette": [list(c) for c in palette], "texels": ci.tolist()}
            else:
                txyz = np.array(tri_xyz, dtype=float)
                centroids = txyz.mean(1)
                n = np.cross(txyz[:, 1] - txyz[:, 0], txyz[:, 2] - txyz[:, 0])
                n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-9)
                if "color" in spec:
                    entry["prim"] = spec["color"]
                else:
                    c = albedo.dominant(centroids, regions, n, pick=spec.get("pick", "largest")) * spec.get("gain", 1.0)
                    entry["prim"] = "%02x%02x%02xff" % tuple(int(round(min(255.0, v))) for v in c)
        # Normals: area-weighted face normals in each vertex's joint frame.
        acc = np.zeros((len(vertices), 3))
        pos = np.array([v["model"] for v in vertices], dtype=float)
        for t in triangles:
            a, b, c = t["v"]
            n = np.cross(pos[b] - pos[a], pos[c] - pos[a])
            for k in (a, b, c):
                acc[k] += n
        for i, v in enumerate(vertices):
            n = acc[i]
            if np.linalg.norm(n) < 1e-9:
                continue
            local = np.array(self.base["joints"][v["joint"]]["world"], dtype=float)[:3, :3].T @ (n / np.linalg.norm(n))
            local /= max(np.linalg.norm(local), 1e-9)
            v["normal"] = [int(round(float(c) * 127)) for c in local]
        joints = []
        for j, w in zip(self.base["joints"], self.worlds):
            jj = dict(j)
            jj["world"] = w.tolist()
            joints.append(jj)
        return {"detail": self.ops.get("detail", "high"), "track": "K",
                "joints": joints, "vertices": vertices, "triangles": triangles,
                "materials": materials}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--base", type=Path, required=True)
    ap.add_argument("--fit", type=Path, required=True)
    ap.add_argument("--ops", type=Path, required=True)
    ap.add_argument("--role", required=True)
    ap.add_argument("--kit-dir", type=Path, help="bake textures and colours from the kit albedo")
    ap.add_argument("--out", type=Path, required=True, help="directory for scene.json")
    args = ap.parse_args()
    base = json.loads(args.base.read_text(encoding="utf-8"))
    fit = json.loads(args.fit.read_text(encoding="utf-8"))
    ops = json.loads(args.ops.read_text(encoding="utf-8"))
    b = Builder(base, fit, ops, args.role)
    for part in ops["parts"]:
        b.part(part)
    args.out.mkdir(parents=True, exist_ok=True)
    albedo = B.KitAlbedo(fit, args.kit_dir, args.role) if args.kit_dir else None
    scene = b.finish(albedo, args.out)
    (args.out / "scene.json").write_text(json.dumps(scene), encoding="utf-8")
    per = {}
    for t in scene["triangles"]:
        name = scene["materials"][t["material"]]["name"]
        per[name] = per.get(name, 0) + 1
    print(f"track K: {len(scene['vertices'])} vertices, {len(scene['triangles'])} triangles {per}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

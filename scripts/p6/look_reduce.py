#!/usr/bin/env python3
"""P6 low detail from a look's Blender dump (look_blend.py's input).

Every VS battle draws a fighter's low detail, so a look needs one at the
plan's budget (Palutena: 278 triangles against the high's 428). Each object
of the dump is reduced to its target by quadric-error half-edge collapses
(Garland-Heckbert): a vertex merges into a neighbour, so every kept vertex
keeps its exact position, bone weights and texture coordinates. A collapse is
taken only when it keeps

- the part's mirror symmetry (the mirrored collapse is taken with it; a
  centre-line vertex only moves along the centre line);
- the row a vertex is drawn by (look_blend.py's rules), so seams stay seams;
- the open borders (a border vertex only slides along its border);
- the texture: the merged vertex's texture coordinates are the kept vertex's
  own in each UV island the moved one was in (no collapse across an island
  edge that cannot carry them);
- a manifold surface (the link condition) and every surviving triangle's
  facing.

    look_reduce.py builds/p6-art/palutena/3ds2ds/work_dump.json \\
        --targets art/p6/palutena/look/low.json --out .../low_dump.json

The dump is kit-derived (local, D10); the targets are a committed recipe.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import look_blend as L  # noqa: E402

MIRROR_EPS = 2e-3
BORDER_WEIGHT = 50.0


class ReduceError(SystemExit):
    pass


def _uvkey(uv) -> tuple:
    return (round(uv[0], 5), round(uv[1], 5))


class Mesh:
    def __init__(self, obj: dict, relax_uv: bool = False, class_min: dict | None = None,
                 silhouette: bool = False, keep_classes=(), keep_points=()):
        self.name = obj["name"]
        self.relax_uv = relax_uv
        self.class_min = {int(k): int(v) for k, v in (class_min or {}).items()}
        self.obj = obj
        self.pos = [np.array(v["kit"], dtype=float) for v in obj["vertices"]]
        self.tris = [dict(t, v=list(t["v"]), uv=[list(u) for u in t["uv"]]) for t in obj["triangles"]]
        self.alive = [True] * len(self.tris)
        n = len(self.pos)
        self.cls = [self.vertex_class(i) for i in range(n)]
        self.mirror = self.mirror_map()
        # Vertices on the front view's outline (the face's width at the
        # cheeks and jaw: the owner's "skinnier from a distance") never move.
        self.fixed = set(self.outline()) if silhouette else set()
        # Classes (rows) whose vertices never move (keep_classes: the dress's
        # skirt, which look_skirt.py rebuilds from its hem and slit).
        self.fixed |= {i for i in range(n) if self.cls[i] in set(keep_classes)}
        # Vertices at named kit points never move (keep_points: the hair cap's
        # ring in front of and beside the face's crown point, which the cap
        # reduced without them let through, owner 2026-10-20 "head point
        # poking out of top of hair looking like a horn").
        for q in keep_points:
            near = [i for i in range(n) if np.linalg.norm(self.pos[i] - np.asarray(q, dtype=float)) < 1e-2]
            if not near:
                raise SystemExit(f"{self.name}: no vertex at keep point {q}")
            self.fixed |= set(near)

    def outline(self) -> list[int]:
        pts = sorted(range(len(self.pos)), key=lambda i: (self.pos[i][0], self.pos[i][1]))

        def cross(o, a, b):
            return ((self.pos[a][0] - self.pos[o][0]) * (self.pos[b][1] - self.pos[o][1]) -
                    (self.pos[a][1] - self.pos[o][1]) * (self.pos[b][0] - self.pos[o][0]))
        hull = []
        for seq in (pts, pts[::-1]):
            part = []
            for i in seq:
                while len(part) >= 2 and cross(part[-2], part[-1], i) <= 1e-9:
                    part.pop()
                part.append(i)
            hull += part[:-1]
        return hull

    # -- classes and symmetry ---------------------------------------------
    def vertex_class(self, i: int):
        v = self.obj["vertices"][i]
        rule = L.OBJECT_ROWS[self.name]
        if rule == "dress":
            return 2 if v["kit"][1] >= L.WAIST_Y - 1e-3 else 1
        if rule == "chest":
            return 7 if L.dominant(v["groups"]) == "NeckN" else 2
        return L.vertex_row(self.name, v)

    def mirror_map(self):
        m = []
        for p in self.pos:
            q = np.array([-p[0], p[1], p[2]])
            d = [np.linalg.norm(q - r) for r in self.pos]
            j = int(np.argmin(d))
            if d[j] > MIRROR_EPS:
                return None
            m.append(j)
        return m

    # -- topology ------------------------------------------------------------
    def live(self):
        return [t for t, a in zip(self.tris, self.alive) if a]

    def count(self) -> int:
        return sum(self.alive)

    def adjacency(self):
        vt = {}
        edges = {}
        for k, (t, a) in enumerate(zip(self.tris, self.alive)):
            if not a:
                continue
            for c in range(3):
                vt.setdefault(t["v"][c], []).append(k)
                e = tuple(sorted((t["v"][c], t["v"][(c + 1) % 3])))
                edges.setdefault(e, []).append(k)
        return vt, edges

    def normal(self, vs) -> np.ndarray:
        a, b, c = (self.pos[i] for i in vs)
        return np.cross(b - a, c - a)

    def quadrics(self, vt, edges):
        q = {}
        for i, ks in vt.items():
            acc = np.zeros((4, 4))
            for k in ks:
                n = self.normal(self.tris[k]["v"])
                area = np.linalg.norm(n) / 2
                if area < 1e-12:
                    continue
                n = n / (2 * area)
                p = np.append(n, -np.dot(n, self.pos[self.tris[k]["v"][0]]))
                acc += area * np.outer(p, p)
            q[i] = acc
        for e, ks in edges.items():
            if len(ks) != 1:
                continue
            a, b = e
            t = self.tris[ks[0]]
            n = self.normal(t["v"])
            d = self.pos[b] - self.pos[a]
            side = np.cross(d, n)
            if np.linalg.norm(side) < 1e-12:
                continue
            side /= np.linalg.norm(side)
            p = np.append(side, -np.dot(side, self.pos[a]))
            w = BORDER_WEIGHT * float(np.dot(d, d))
            for i in e:
                q[i] = q[i] + w * np.outer(p, p)
        return q

    # -- one collapse ----------------------------------------------------------
    def check(self, u: int, v: int, vt, edges):
        """The new uv of each of u's islands if u can merge into v, else None."""
        if u == v or self.cls[u] != self.cls[v] or u in self.fixed:
            return None
        floor = self.class_min.get(self.cls[u])
        if floor is not None and sum(1 for i in vt if self.cls[i] == self.cls[u]) <= floor:
            return None
        e = tuple(sorted((u, v)))
        if e not in edges:
            return None
        shared = edges[e]
        border_u = any(len(ks) == 1 for ee, ks in edges.items() if u in ee)
        if border_u and len(shared) != 1:
            return None
        # Link condition.
        nu = {w for k in vt[u] for w in self.tris[k]["v"]} - {u}
        nv = {w for k in vt[v] for w in self.tris[k]["v"]} - {v}
        third = {w for k in shared for w in self.tris[k]["v"]} - {u, v}
        if (nu & nv) != third:
            return None
        # Texture islands: u's uv in each shared triangle maps to v's there.
        remap = {}
        for k in shared:
            t = self.tris[k]
            a = _uvkey(t["uv"][t["v"].index(u)])
            b = _uvkey(t["uv"][t["v"].index(v)])
            if remap.setdefault(a, b) != b:
                return None
        for k in vt[u]:
            if k in shared:
                continue
            t = self.tris[k]
            key = _uvkey(t["uv"][t["v"].index(u)])
            if key not in remap:
                if not self.relax_uv:
                    return None
                # Relaxed (small parts whose islands block every collapse,
                # the shoes): v's uv in its island nearest u's.
                ours = {_uvkey(self.tris[j]["uv"][self.tris[j]["v"].index(v)]) for j in vt[v]}
                remap[key] = min(ours, key=lambda w: (w[0] - key[0]) ** 2 + (w[1] - key[1]) ** 2)
            # Facing: the triangle with v in u's place keeps its side.
            vs = [v if w == u else w for w in t["v"]]
            n0 = self.normal(t["v"])
            n1 = self.normal(vs)
            l0, l1 = np.linalg.norm(n0), np.linalg.norm(n1)
            if l1 < 1e-9 or l0 < 1e-12 or np.dot(n0, n1) / (l0 * l1) < 0.3:
                return None
        return remap

    def apply(self, u: int, v: int, remap: dict, vt, edges):
        for k in edges[tuple(sorted((u, v)))]:
            self.alive[k] = False
        for k in vt[u]:
            if not self.alive[k]:
                continue
            t = self.tris[k]
            c = t["v"].index(u)
            new = remap[_uvkey(t["uv"][c])]
            t["v"][c] = v
            t["uv"][c] = [new[0], new[1]]

    def reduce(self, target: int) -> None:
        forbidden = set()
        while self.count() > target:
            vt, edges = self.adjacency()
            q = self.quadrics(vt, edges)
            best = None
            for (a, b) in edges:
                for u, v in ((a, b), (b, a)):
                    if (u, v) in forbidden:
                        continue
                    pv = np.append(self.pos[v], 1.0)
                    cost = float(pv @ (q[u] + q[v]) @ pv)
                    if self.mirror is not None:
                        mu, mv = self.mirror[u], self.mirror[v]
                        if mu == u and mv != v:
                            continue            # a centre vertex leaves the centre line
                        if mu == v:
                            continue            # across the centre line
                        if mu != u:
                            if (mu, mv) == (u, v):
                                continue
                            pmv = np.append(self.pos[mv], 1.0)
                            cost += float(pmv @ (q.get(mu, 0) + q.get(mv, 0)) @ pmv) if mu in q and mv in q else 0.0
                    if best is not None and cost >= best[0]:
                        continue
                    remap = self.check(u, v, vt, edges)
                    if remap is None:
                        continue
                    best = (cost, u, v, remap)
            if best is None:
                print(f"{self.name}: no collapse keeps the rules past {self.count()} triangles (target {target})")
                return
            _, u, v, remap = best
            saved = ([dict(t, v=list(t["v"]), uv=[list(x) for x in t["uv"]]) for t in self.tris], list(self.alive))
            self.apply(u, v, remap, vt, edges)
            if self.mirror is not None and self.mirror[u] != u:
                mu, mv = self.mirror[u], self.mirror[v]
                vt, edges = self.adjacency()
                remap2 = self.check(mu, mv, vt, edges)
                if remap2 is None:
                    # The pair together breaks the surface: undo, never again.
                    self.tris, self.alive = saved
                    forbidden.add((u, v))
                    forbidden.add((mu, mv))
                    continue
                self.apply(mu, mv, remap2, vt, edges)

    def flip_slivers(self, min_angle: float) -> int:
        """Each live triangle with an angle under min_angle (degrees) turns
        its longest edge where the two new triangles' least angle is larger,
        both face their old pair's way, all four corners are one row's and
        each keeps one uv (owner
        2026-10-20: the low's "triangles under armpit can be simplified" --
        two slivers a side where the chest meets the arm)."""
        def least(vs) -> float:
            p = [self.pos[i] for i in vs]
            out = []
            for k in range(3):
                a, b, c = p[k], p[(k + 1) % 3], p[(k + 2) % 3]
                u, w = b - a, c - a
                den = np.linalg.norm(u) * np.linalg.norm(w)
                out.append(np.degrees(np.arccos(np.clip(float(u @ w) / den, -1.0, 1.0))) if den > 0 else 0.0)
            return min(out)
        flipped = 0
        for k, t in enumerate(self.tris):
            if not self.alive[k] or least(t["v"]) >= min_angle:
                continue
            vs = t["v"]
            a, b = max(((vs[i], vs[(i + 1) % 3]) for i in range(3)),
                       key=lambda e: np.linalg.norm(self.pos[e[0]] - self.pos[e[1]]))
            other = [j for j, u in enumerate(self.tris) if j != k and self.alive[j] and a in u["v"] and b in u["v"]]
            if len(other) != 1:
                continue
            j = other[0]
            u = self.tris[j]
            c = next(i for i in vs if i not in (a, b))
            d = next(i for i in u["v"] if i not in (a, b))
            uv = {}
            for tri in (t, u):
                for i, x in zip(tri["v"], tri["uv"]):
                    uv.setdefault(i, set()).add(_uvkey(x))
            if any(len(uv[i]) != 1 for i in (a, b, c, d)):
                continue
            # One row's surface only (a seam's triangles keep their rows).
            if len({self.cls[i] for i in (a, b, c, d)}) != 1:
                continue
            n_old = self.normal(t["v"]) + self.normal(u["v"])
            # t is (a, b, c) in its winding order: the new pair (c, a, d) and (c, d, b).
            ia = vs.index(a)
            if vs[(ia + 1) % 3] != b:
                a, b = b, a
            new = [[c, a, d], [c, d, b]]
            if min(least(n) for n in new) <= least(vs):
                continue
            if any(np.dot(self.normal(n), n_old) <= 0.5 * np.linalg.norm(self.normal(n)) * np.linalg.norm(n_old)
                   for n in new):
                continue
            for tri, n in ((t, new[0]), (u, new[1])):
                tri["v"] = list(n)
                tri["uv"] = [list(next(iter(uv[i]))) for i in n]
            flipped += 1
        return flipped

    def to_object(self) -> dict:
        tris = self.live()
        used = sorted({i for t in tris for i in t["v"]})
        index = {i: k for k, i in enumerate(used)}
        out = dict(self.obj)
        out["vertices"] = [self.obj["vertices"][i] for i in used]
        out["triangles"] = [dict(t, v=[index[i] for i in t["v"]]) for t in tris]
        return out


def drop_small_parts(obj: dict, most: int) -> dict:
    """The object without its separate pieces of at most `most` triangles
    (the stiletto heel blocks: about a pixel in battle)."""
    parent = list(range(len(obj["vertices"])))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for t in obj["triangles"]:
        a = find(t["v"][0])
        for i in t["v"][1:]:
            parent[find(i)] = a
    size = {}
    for t in obj["triangles"]:
        r = find(t["v"][0])
        size[r] = size.get(r, 0) + 1
    keep = [t for t in obj["triangles"] if size[find(t["v"][0])] > most]
    return dict(obj, triangles=keep)


def reduce_dump(dump: dict, recipe: dict) -> tuple[dict, dict]:
    targets = recipe.get("targets", {})
    out = dict(dump, objects=[])
    report = {}
    for obj in dump["objects"]:
        name = obj["name"]
        if name in (recipe.get("drop_parts_up_to") or {}):
            obj = drop_small_parts(obj, int(recipe["drop_parts_up_to"][name]))
        target = int(targets.get(name, len(obj["triangles"])))
        m = Mesh(obj, name in recipe.get("relax_uv", ()), (recipe.get("class_min") or {}).get(name),
                 name in recipe.get("keep_outline", ()), (recipe.get("keep_classes") or {}).get(name, ()),
                 (recipe.get("keep_points") or {}).get(name, ()))
        before = m.count()
        if target < before:
            m.reduce(target)
        if name in (recipe.get("flip_slivers") or {}):
            m.flip_slivers(float(recipe["flip_slivers"][name]))
        o = m.to_object()
        out["objects"].append(o)
        report[obj["name"]] = [before, len(o["triangles"]), "mirror" if m.mirror is not None else "-"]
    return out, report


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("dump", type=Path)
    ap.add_argument("--targets", type=Path, required=True,
                    help="the recipe: targets (object -> triangles), relax_uv, class_min, keep_outline, "
                         "keep_classes, keep_points")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    dump = json.loads(args.dump.read_text(encoding="utf-8"))
    recipe = json.loads(args.targets.read_text(encoding="utf-8"))
    out, report = reduce_dump(dump, recipe)
    args.out.write_text(json.dumps(out), encoding="utf-8")
    total = sum(r[1] for r in report.values())
    for name, r in report.items():
        print(f"{name:24s} {r[0]:3d} -> {r[1]:3d} {r[2]}")
    print(f"total {total} triangles -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

"""Track K, decimation route (P6 A3b, owner 2026-10-13): the fitted kit model
reduced to the DS budget region by region.

The owner's own baseline (Palutena_Owner_Attempt.blend) is the kit model
under Decimate modifiers; this script takes the same route to the budget:

1. import the fitted kit (kit_fit.py: uniform scale, no stretch) with its
   skin as vertex groups, keep the meshes `ops["meshes"]` names (the body,
   face, hands and weapon; not the expression overlays or alpha cards);
2. join and weld them, so the decimator sees one surface;
3. sort every vertex into a region (`ops["regions"]`: kit joints -> region;
   a region may split by the albedo's colour at the vertex: hair green, a
   gold tiara, skin);
4. decimate one region at a time (Collapse, X symmetry, triangulate) to its
   target (`ops["targets"]`): a vertex group gives weight 1 to the region's
   own vertices and 0 to everything else and to the region's border with
   any other, so only that region collapses and regions meet without cracks
   (Blender 5.2 never collapses a 0-weight vertex at factor 1000: measured);
5. report the counts per region.

Runs in the live Blender (MCP) or headless:
    blender -b --factory-startup --python scripts/p6/blender/kit_decimate.py -- \
        --fit <kit_low_fit.json> --kit-dir <kit folder> --ops art/p6/<name>/look/k_dec_<detail>.json \
        [--blend out.local.blend]
Kit-derived output stays under builds/ (plan D10).
"""
import argparse
import json
import sys
from pathlib import Path

import bpy
import bmesh

sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_scene as S  # noqa: E402
import import_fit as F  # noqa: E402


def _image_pixels(img):
    w, h = img.size
    px = list(img.pixels)
    return w, h, px


def _sample(img_cache, img, uv):
    w, h, px = img_cache[img.name]
    u = uv[0] % 2.0
    v = uv[1] % 2.0
    u = 2.0 - u if u > 1.0 else u      # mirrored repeat (the kit's Mirror UV.txt)
    v = 2.0 - v if v > 1.0 else v
    x = min(w - 1, max(0, int(u * w)))
    y = min(h - 1, max(0, int(v * h)))
    i = (y * w + x) * 4
    return px[i], px[i + 1], px[i + 2]


def classify(obj, ops):
    """Region name of every vertex: its dominant kit joint's region, split by
    the albedo colour where the region's rule says so."""
    me = obj.data
    names = {g.index: g.name for g in obj.vertex_groups}
    joint_region = {}
    for region, spec in ops["regions"].items():
        for j in spec.get("joints", ()):
            joint_region[j] = region
    # Per-vertex dominant joint.
    dom = []
    for v in me.vertices:
        best = max(v.groups, key=lambda g: g.weight, default=None) if len(v.groups) else None
        dom.append(names[best.group] if best is not None else None)
    # Colour per vertex: the albedo at its first loop's UV.
    img_cache = {}
    vcolour = [None] * len(me.vertices)
    uv = me.uv_layers.active
    for poly in me.polygons:
        mat = me.materials[poly.material_index] if me.materials else None
        img = None
        if mat and mat.node_tree:
            img = next((n.image for n in mat.node_tree.nodes if n.type == "TEX_IMAGE" and n.image), None)
        if img is None:
            continue
        if img.name not in img_cache:
            img_cache[img.name] = _image_pixels(img)
        for li in poly.loop_indices:
            vi = me.loops[li].vertex_index
            if vcolour[vi] is None and uv is not None:
                vcolour[vi] = _sample(img_cache, img, uv.data[li].uv)
    out = []
    for vi, j in enumerate(dom):
        region = joint_region.get(j, ops.get("default_region", "torso"))
        rule = ops["regions"].get(region, {}).get("colour_split")
        c = vcolour[vi]
        if rule and c is not None:
            r, g, b = c
            for target, test in rule:
                if test == "green" and g > r * 1.15 and g > b * 1.05:
                    region = target
                    break
                if test == "gold" and r > 0.45 and g > 0.3 and b < g * 0.75 and r >= g:
                    region = target
                    break
        out.append(region)
    return out


def cull_hidden(obj, spec) -> int:
    """Delete faces no battle view can see: a face stays when a ray from it,
    along one of `directions` sample directions inside the elevation band
    (`elevation` degrees: a fighter is seen from the side and from above,
    never from under the stage) on its outer side, escapes the mesh. The legs
    under a long dress, the body under clothing, the skull under the hair."""
    import math
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    lo, hi = spec.get("elevation", (-15.0, 75.0))
    count = spec.get("directions", 128)
    dirs = []
    golden = math.pi * (3.0 - math.sqrt(5.0))
    for i in range(count):
        z = 1.0 - 2.0 * (i + 0.5) / count
        r = math.sqrt(max(0.0, 1.0 - z * z))
        a = golden * i
        d = Vector((math.cos(a) * r, math.sin(a) * r, z))
        if lo <= math.degrees(math.asin(z)) <= hi:
            dirs.append(d)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    tree = BVHTree.FromBMesh(bm)
    eps = spec.get("eps", 0.002)
    hidden = []
    for f in bm.faces:
        c = f.calc_center_median()
        n = f.normal
        seen = False
        for d in dirs:
            if d.dot(n) <= 0.05:
                continue
            hit = tree.ray_cast(c + n * eps, d)
            if hit[0] is None:
                seen = True
                break
        if not seen:
            hidden.append(f)
    bmesh.ops.delete(bm, geom=hidden, context="FACES_ONLY")
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return len(hidden)


def remove_slivers(obj, spec) -> int:
    """Collapse the shortest edge of every triangle thinner than
    `min_angle` degrees (a sliver draws as a dark line at DS size), a few
    rounds, then dissolve what degenerated. Collapsing keeps the surface
    connected, so no crack opens."""
    import math
    min_angle = math.radians(spec.get("min_angle", 7.0))
    collapsed = 0
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for _ in range(spec.get("rounds", 4)):
        bm.faces.ensure_lookup_table()
        edges = set()
        for f in bm.faces:
            if len(f.verts) != 3:
                continue
            angles = []
            for loop in f.loops:
                a = loop.calc_angle()
                angles.append(a)
            if min(angles) < min_angle:
                e = min(f.edges, key=lambda e: e.calc_length())
                edges.add(e)
        # One edge a vertex a round, so collapses never chain.
        used = set()
        batch = []
        for e in sorted(edges, key=lambda e: e.calc_length()):
            if e.verts[0] in used or e.verts[1] in used:
                continue
            used.update(e.verts)
            batch.append(e)
        if not batch:
            break
        bmesh.ops.collapse(bm, edges=batch, uvs=True)
        collapsed += len(batch)
        bmesh.ops.dissolve_degenerate(bm, dist=spec.get("degenerate", 1e-5), edges=bm.edges)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return collapsed


def count_regions(me, vregion):
    counts = {}
    for poly in me.polygons:
        regs = [vregion[v] for v in poly.vertices]
        reg = max(set(regs), key=regs.count)
        counts[reg] = counts.get(reg, 0) + (len(poly.vertices) - 2)
    return counts


def build(fit: Path, kit_dir: Path, ops: dict, name: str = "k_dec", offset: float = 0.0):
    objs = F.build(fit, kit_dir, name, offset)
    keep = set(ops["meshes"])
    for o in list(objs):
        if o.get("kit_mesh") not in keep:
            bpy.data.objects.remove(o, do_unlink=True)
            objs.remove(o)
    for o in bpy.context.selected_objects:
        o.select_set(False)
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    # Weld the meshes (each carried the kit's whole pool).
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=ops.get("weld", 0.0005))
    bm.to_mesh(obj.data)
    bm.free()
    if ops.get("cull_hidden"):
        removed = cull_hidden(obj, ops["cull_hidden"])
        print(f"culled {removed} hidden faces")
    vregion = classify(obj, ops)
    regions = sorted(set(vregion))
    # One full-weight group a region: a collapse keeps a survivor's groups,
    # so the regions can be read back after every pass.
    rgroups = {r: obj.vertex_groups.new(name=f"R_{r}") for r in regions}
    for vi, r in enumerate(vregion):
        rgroups[r].add([vi], 1.0, "REPLACE")
    keep_group = obj.vertex_groups.new(name="keep")
    targets = ops["targets"]
    before = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    order = ops.get("order") or sorted(regions, key=lambda r: -vregion.count(r))
    for r in order:
        if r not in rgroups:
            continue
        vreg = _vertex_regions(obj, rgroups)
        freg = _face_regions(obj.data, vreg)
        have = sum(len(p.vertices) - 2 for p, fr in zip(obj.data.polygons, freg) if fr == r)
        want = targets.get(r, have)
        if have <= want:
            print(f"{r}: {have} <= {want}, kept")
            continue
        # Weight 1: the region's own vertices may collapse; 0: everything
        # else, and the region's border with any other, stays exactly.
        border = set()
        for poly, fr in zip(obj.data.polygons, freg):
            if fr != r:
                border.update(poly.vertices)
        rspec = ops["regions"].get(r, {})
        planar = rspec.get("planar", ops.get("planar"))
        if planar:
            # The owner's baseline route first: dissolve the region's nearly
            # flat interior (Decimate "Planar"), borders kept, so the collapse
            # spends its triangles on the shape, not on flat cloth.
            bm = bmesh.new()
            bm.from_mesh(obj.data)
            bm.faces.ensure_lookup_table()
            bm.verts.ensure_lookup_table()
            inner = [f for f, fr in zip(bm.faces, freg) if fr == r]
            inner_edges = {e for f in inner for e in f.edges
                           if all(v.index not in border for v in e.verts)}
            bmesh.ops.dissolve_limit(bm, angle_limit=__import__("math").radians(planar),
                                     use_dissolve_boundaries=False, verts=[], edges=list(inner_edges),
                                     delimit={"UV"})
            bmesh.ops.triangulate(bm, faces=bm.faces)
            bm.to_mesh(obj.data)
            bm.free()
            obj.data.update()
            vreg = _vertex_regions(obj, rgroups)
            freg = _face_regions(obj.data, vreg)
            have = sum(len(p.vertices) - 2 for p, fr in zip(obj.data.polygons, freg) if fr == r)
            border = set()
            for poly, fr in zip(obj.data.polygons, freg):
                if fr != r:
                    border.update(poly.vertices)
            if have <= want:
                print(f"{r}: planar {planar} deg -> {have} <= {want}")
                continue
        # The mesh is one welded surface, so a collapse never opens a crack;
        # the border is only partly protected (`border_weight`), else every
        # region keeps its full-resolution outline.
        keep_group = obj.vertex_groups["keep"]
        bw = ops.get("border_weight", 0.0)
        for v in obj.data.vertices:
            if vreg[v.index] == r:
                w = bw if v.index in border else 1.0
            else:
                w = 0.0
            keep_group.add([v.index], w, "REPLACE")
        total = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        mod = obj.modifiers.new("decimate", "DECIMATE")
        mod.decimate_type = "COLLAPSE"
        mod.ratio = max(0.01, (total - (have - want)) / total)
        mod.use_symmetry = bool(rspec.get("symmetry", ops.get("symmetry", True)))
        mod.symmetry_axis = "X"
        mod.use_collapse_triangulate = True
        mod.vertex_group = "keep"
        mod.vertex_group_factor = 1000.0
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=mod.name)
        vreg = _vertex_regions(obj, rgroups)
        freg = _face_regions(obj.data, vreg)
        got = sum(len(p.vertices) - 2 for p, fr in zip(obj.data.polygons, freg) if fr == r)
        print(f"{r}: {have} -> {got} (target {want})")
    if ops.get("slivers"):
        n = remove_slivers(obj, ops["slivers"])
        print(f"slivers: {n} edges collapsed")
    vreg = _vertex_regions(obj, rgroups)
    freg = _face_regions(obj.data, vreg)
    counts = {}
    for poly, fr in zip(obj.data.polygons, freg):
        counts[fr] = counts.get(fr, 0) + len(poly.vertices) - 2
    tris = sum(counts.values())
    obj["k_dec_counts"] = json.dumps(counts)
    print(f"{name}: {before} -> {tris} triangles: " + ", ".join(f"{r} {counts.get(r, 0)}/{targets.get(r, 0)}" for r in regions))
    return obj


def _vertex_regions(obj, rgroups):
    """Each vertex's region: its heaviest R_ group (looked up by name: a
    modifier apply leaves earlier VertexGroup handles stale)."""
    index_to_region = {obj.vertex_groups[f"R_{r}"].index: r for r in rgroups}
    out = []
    for v in obj.data.vertices:
        best, bw = None, -1.0
        for g in v.groups:
            r = index_to_region.get(g.group)
            if r is not None and g.weight > bw:
                best, bw = r, g.weight
        out.append(best)
    return out


def _face_regions(me, vreg):
    out = []
    for poly in me.polygons:
        regs = [vreg[i] for i in poly.vertices if vreg[i] is not None]
        out.append(max(set(regs), key=regs.count) if regs else None)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fit", type=Path, required=True)
    ap.add_argument("--kit-dir", type=Path, required=True)
    ap.add_argument("--ops", type=Path, required=True)
    ap.add_argument("--name", default="k_dec")
    ap.add_argument("--offset", type=float, default=0.0)
    ap.add_argument("--blend", type=Path)
    args = ap.parse_args(S.args_after_dashes())
    ops = json.loads(args.ops.read_text(encoding="utf-8"))
    build(args.fit, args.kit_dir, ops, args.name, args.offset)
    if args.blend:
        bpy.ops.wm.save_as_mainfile(filepath=str(args.blend))


if __name__ == "__main__":
    main()

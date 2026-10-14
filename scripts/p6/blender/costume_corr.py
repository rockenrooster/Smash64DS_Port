"""P6 costumes: where each texel of a look's textures came from on the kit.

Run headless on the 3DS-to-DS work file (the owner's session: collection
"3DS_to_DS_Work" holds the DS_* objects, "Baseline_Reference" the kit model
offset on x by its armature):

    blender -b <3DS_to_DS_Palutena.blend> --python scripts/p6/blender/costume_corr.py -- \\
        --out builds/p6-art/palutena/costumes/corr.npz

For every DS object's texture texel this projects the reference along the
face normal as the bake did (rays from just outside the surface inward,
cards below alpha 0.5 passed through, the nearest surface when no ray hits)
and records which kit image and which kit UV the texel sampled. The kit's
costumes are recolours on shared UVs, so p6_costumes.py samples each
alternate at the same places. Kit-derived: local output (D10). Imports only
bpy, mathutils, numpy and the standard library.
"""
import argparse
import math
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

# The projected parts (the sprites -- gem, staff wings, shield -- were cut
# from the kit's images, not projected).
DS_OBJECTS = ("DS_face", "DS_hair_cap", "DS_hair_panels", "DS_chest", "DS_dress", "DS_arms", "DS_hands",
              "DS_legs", "DS_staff")
DELTA, REACH, ALPHA_CUT, LAYERS = 0.5, 1.5, 0.5, 6


def args_after_dashes():
    argv = sys.argv
    return argv[argv.index("--") + 1:] if "--" in argv else []


def mir(t):
    t = t % 2.0
    return 2.0 - t if t > 1.0 else t


def bary(p, a, b, c):
    v0, v1, v2 = b - a, c - a, p - a
    d00, d01, d11, d20, d21 = v0.dot(v0), v0.dot(v1), v1.dot(v1), v2.dot(v0), v2.dot(v1)
    den = d00 * d11 - d01 * d01
    if abs(den) < 1e-12:
        return None
    v = (d11 * d20 - d01 * d21) / den
    w = (d00 * d21 - d01 * d20) / den
    return 1 - v - w, v, w


def reference():
    """The kit model's triangles in work space: vertices, faces, face UVs,
    face image index, and the images (name, pixels, width, height)."""
    rf = bpy.data.collections["Baseline_Reference"]
    off = Vector((bpy.data.objects["Armature_ref"].location.x, 0, 0))
    dg = bpy.context.evaluated_depsgraph_get()
    V, F, FUV, FIMG, names, pix = [], [], [], [], [], []
    for o in rf.objects:
        if o.type != "MESH":
            continue
        oe = o.evaluated_get(dg)
        m = oe.to_mesh()
        m.calc_loop_triangles()
        uvd = m.uv_layers.active.data
        base = len(V)
        V += [o.matrix_world @ v.co - off for v in m.vertices]
        for t in m.loop_triangles:
            mat = o.material_slots[m.polygons[t.polygon_index].material_index].material
            im = next(n.image for n in mat.node_tree.nodes if n.type == "TEX_IMAGE" and n.image)
            key = Path(im.filepath).name
            if key not in names:
                w, h = im.size
                names.append(key)
                pix.append(np.array(im.pixels[:], dtype=np.float32).reshape(h, w, 4))
            F.append(tuple(base + i for i in t.vertices))
            FUV.append([uvd[l].uv.copy() for l in t.loops])
            FIMG.append(names.index(key))
        oe.to_mesh_clear()
    return V, F, FUV, FIMG, names, pix


def alpha_at(px, u, v):
    h, w = px.shape[:2]
    x = min(w - 1, max(0, int(mir(u) * w)))
    y = min(h - 1, max(0, int(mir(v) * h)))
    return px[y, x, 3]


def correspond(target, ref):
    """Per texel of target's image: (image index, u, v) of the kit sample, or
    image -1 where no triangle of the target covers the texel."""
    V, F, FUV, FIMG, names, pix = ref
    bvh = BVHTree.FromPolygons(V, F)
    me = target.data
    mw = target.matrix_world
    uvd = me.uv_layers.active.data
    me.calc_loop_triangles()
    img = next(n.image for s in target.material_slots if s.material
               for n in s.material.node_tree.nodes if n.type == "TEX_IMAGE" and n.image)
    W, H = img.size
    out = np.full((H, W, 3), -1.0, dtype=np.float32)
    for t in me.loop_triangles:
        P3 = [mw @ me.vertices[i].co for i in t.vertices]
        n = (mw.to_3x3() @ t.normal).normalized()
        U = [uvd[l].uv for l in t.loops]
        xs = [u.x * W for u in U]
        ys = [u.y * H for u in U]
        for yy in range(max(0, int(math.floor(min(ys))) - 1), min(H, int(max(ys)) + 2)):
            for xx in range(int(math.floor(min(xs))) - 1, int(max(xs)) + 2):
                q = Vector(((xx + 0.5) / W, (yy + 0.5) / H, 0))
                b = bary(q, U[0].to_3d(), U[1].to_3d(), U[2].to_3d())
                if b is None or min(b) < -0.03:
                    continue
                p = P3[0] * b[0] + P3[1] * b[1] + P3[2] * b[2]
                o_ = p + n * DELTA
                found = None
                for _ in range(LAYERS):
                    h = bvh.ray_cast(o_, -n, REACH)
                    if h[0] is None:
                        break
                    fi = h[2]
                    a, bb, c = (V[k] for k in F[fi])
                    bw = bary(h[0], a, bb, c)
                    uu = FUV[fi][0] * bw[0] + FUV[fi][1] * bw[1] + FUV[fi][2] * bw[2]
                    if alpha_at(pix[FIMG[fi]], uu.x, uu.y) >= ALPHA_CUT:
                        found = (FIMG[fi], uu.x, uu.y)
                        break
                    o_ = h[0] - n * 1e-3
                if found is None:
                    h = bvh.find_nearest(p, 1.0)
                    if h[0] is None:
                        continue
                    fi = h[2]
                    a, bb, c = (V[k] for k in F[fi])
                    bw = bary(h[0], a, bb, c)
                    if bw is None:
                        continue
                    uu = FUV[fi][0] * bw[0] + FUV[fi][1] * bw[1] + FUV[fi][2] * bw[2]
                    found = (FIMG[fi], uu.x, uu.y)
                X = xx % W
                # Blender images are bottom-up: row 0 is the bottom.
                out[H - 1 - yy, X] = found
    return Path(img.filepath).name, out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args(args_after_dashes())
    ref = reference()
    work = bpy.data.collections["3DS_to_DS_Work"]
    arrays = {}
    for name in DS_OBJECTS:
        o = work.objects.get(name)
        if o is None:
            print("costume_corr: no", name)
            continue
        tex, corr = correspond(o, ref)
        arrays[name] = corr
        arrays[name + "__texture"] = np.array(tex)
        print("costume_corr", name, tex, corr.shape, int((corr[..., 0] >= 0).sum()), "texels")
    arrays["__images"] = np.array(ref[4])
    args.out.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(args.out, **arrays)
    print("costume_corr ->", args.out)


if __name__ == "__main__":
    main()

"""Dump a look's Blender meshes for look_blend.py / look_reduce.py (P6).

The owner's look is built by hand in Blender (Palutena: the "3DS to DS"
model, docs/P6/characters/palutena.md). This writes one collection's meshes
as the dump those scripts read: kit space (x, y up = Blender z, z = -Blender
y), each vertex's bone weights heaviest first, each triangle's corner uvs,
material index and its `part` face attribute, and each material's image and
flags. Run inside Blender 5.2 (headless or live):

    blender -b <look.blend> --python scripts/p6/blender/dump_look.py -- \\
        --out builds/p6-art/palutena/3ds2ds_v2/work_dump.json \\
        [--collection 3DS_to_DS_Work] [--local] [--strip L_] [--flags <old dump.json>]

--local reads each object's own coordinates (a copy placed beside the model
for viewing, as the v2 file's DS_low_work); --strip drops a name prefix;
--flags takes each material's two_sided/alpha from an earlier dump (else
two-sided, opaque, and the sprite materials' alpha by their object's
"sprite"/"billboard" property). Everything it writes is kit-derived: local,
under builds/ (D10).

Imports only bpy and the standard library.
"""
import argparse
import json
import sys

import bpy

FRAME = "kit (Y-up, kit units; world z = kit y, world y = -kit z)"


def args_after_dashes():
    argv = sys.argv
    return argv[argv.index("--") + 1:] if "--" in argv else []


def dump(collection: str, local: bool, strip: str, flags: dict) -> dict:
    objs = []
    for o in sorted(bpy.data.collections[collection].objects, key=lambda o: o.name):
        if o.type != "MESH":
            continue
        me = o.data
        me.calc_loop_triangles()
        mw = None if local else o.matrix_world
        names = {g.index: g.name for g in o.vertex_groups}
        verts = []
        for v in me.vertices:
            w = v.co if mw is None else mw @ v.co
            gs = sorted(((names[g.group], round(g.weight, 4)) for g in v.groups if g.weight > 1e-4),
                        key=lambda t: -t[1])
            verts.append({"kit": [round(w.x, 4), round(w.z, 4), round(-w.y, 4)], "groups": [list(g) for g in gs]})
        uvl = me.uv_layers.active
        part = me.attributes.get("part")
        tris = []
        for lt in me.loop_triangles:
            uv = [[round(uvl.data[li].uv[0], 5), round(uvl.data[li].uv[1], 5)] for li in lt.loops] if uvl else None
            attr = int(part.data[lt.polygon_index].value) if part is not None and part.domain == "FACE" else None
            tris.append({"v": list(lt.vertices), "uv": uv, "material": lt.material_index, "attr": attr})
        props = {k: o[k] for k in o.keys() if k in ("sprite", "billboard")}
        mats = []
        for m in me.materials:
            img = next((n.image for n in m.node_tree.nodes if n.type == "TEX_IMAGE" and n.image), None)
            old = flags.get(m.name, {})
            mats.append({"name": m.name,
                         "image": None if img is None else {"name": img.name, "file": bpy.path.abspath(img.filepath),
                                                            "size": list(img.size)},
                         "two_sided": old.get("two_sided", True),
                         "alpha": old.get("alpha", bool(props.get("sprite") or props.get("billboard")))})
        name = o.name[len(strip):] if strip and o.name.startswith(strip) else o.name
        objs.append({"name": name, "props": props, "vertices": verts, "triangles": tris, "materials": mats})
    return {"source": bpy.data.filepath, "frame": FRAME, "objects": objs}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--out", required=True)
    ap.add_argument("--collection", default="3DS_to_DS_Work")
    ap.add_argument("--local", action="store_true")
    ap.add_argument("--strip", default="")
    ap.add_argument("--flags", help="an earlier dump whose materials' two_sided/alpha to keep")
    args = ap.parse_args(args_after_dashes())
    flags = {}
    if args.flags:
        old = json.load(open(args.flags, encoding="utf-8"))
        flags = {m["name"]: m for o in old["objects"] for m in o["materials"]}
    out = dump(args.collection, args.local, args.strip, flags)
    with open(args.out, "w", encoding="utf-8", newline="") as f:
        json.dump(out, f)
    print({o["name"]: (len(o["vertices"]), len(o["triangles"])) for o in out["objects"]},
          sum(len(o["triangles"]) for o in out["objects"]), "->", args.out)


if __name__ == "__main__":
    main()

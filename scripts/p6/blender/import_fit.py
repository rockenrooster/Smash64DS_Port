"""Import a kit_fit.py result (the kit model posed on the base's skeleton) into
Blender, beside or over a base imported by import_scene.py (P6 A3).

    blender -b <base.local.blend> --python scripts/p6/blender/import_fit.py -- \
        --fit builds/p6-art/<name>/fit/kit_high_fit.json --kit-dir <kit folder> \
        --name kit_high [--offset 2.2] [--blend out.local.blend] [--render out.png]

One object a kit mesh, in the base's model space (same UNIT and Y-up to Z-up
as import_scene.py), with the kit's albedo texture. Kit-derived: everything it
writes stays under builds/ or artifacts/ (plan D10).
"""
import argparse
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_scene as S  # noqa: E402


def build(fit_path: Path, kit_dir: Path, name: str, offset: float):
    data = json.loads(fit_path.read_text(encoding="utf-8"))
    coll = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(coll)
    images = {}
    objs = []
    for m in data["meshes"]:
        mesh = bpy.data.meshes.new(f"{name}_{m['name']}")
        verts = [S.point(p) + Vector((offset, 0, 0)) for p in m["positions"]]
        mesh.from_pydata(verts, [], m["triangles"])
        mesh.update()
        if m["uvs"]:
            uv = mesh.uv_layers.new(name="UV")
            for poly in mesh.polygons:
                for li in poly.loop_indices:
                    vi = mesh.loops[li].vertex_index
                    uv.data[li].uv = m["uvs"][vi][:2]
        obj = bpy.data.objects.new(f"{name}_{m['name']}", mesh)
        coll.objects.link(obj)
        # The kit skin as vertex groups (kit joint names), for regions and
        # the binding to the base's joints.
        groups = {}
        for vi, ws in enumerate(m.get("weights", ())):
            for joint, w in ws:
                g = groups.get(joint)
                if g is None:
                    g = groups[joint] = obj.vertex_groups.new(name=joint)
                g.add([vi], float(w), "REPLACE")
        obj["kit_mesh"] = m["name"]
        mat = bpy.data.materials.new(f"{name}_{m['material']}")
        mat.use_nodes = True
        bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        bsdf.inputs["Roughness"].default_value = 1.0
        if m["image"] and (kit_dir / m["image"]).exists():
            img = images.get(m["image"])
            if img is None:
                img = images[m["image"]] = bpy.data.images.load(str(kit_dir / m["image"]))
            tex = mat.node_tree.nodes.new("ShaderNodeTexImage")
            tex.image = img
            # The kit's Palutena expects mirrored-repeat addressing (Mirror UV.txt).
            tex.extension = "MIRROR"
            mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
            mat.node_tree.links.new(tex.outputs["Alpha"], bsdf.inputs["Alpha"])
        mesh.materials.append(mat)
        objs.append(obj)
    return objs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fit", type=Path, required=True)
    ap.add_argument("--kit-dir", type=Path, required=True)
    ap.add_argument("--name", required=True)
    ap.add_argument("--offset", type=float, default=0.0, help="Blender units along +x")
    ap.add_argument("--blend", type=Path)
    ap.add_argument("--render", type=Path)
    args = ap.parse_args(S.args_after_dashes())
    objs = build(args.fit, args.kit_dir, args.name, args.offset)
    print(f"imported {len(objs)} kit meshes as {args.name}")
    if args.render:
        args.render.parent.mkdir(parents=True, exist_ok=True)
        targets = [o for o in bpy.context.scene.objects if o.type == "MESH"]
        for p in S.render(args.render, targets, views=("front", "side", "three_quarter")):
            print("render", p)
    if args.blend:
        bpy.ops.wm.save_as_mainfile(filepath=str(args.blend))
        print("saved", args.blend)


if __name__ == "__main__":
    main()

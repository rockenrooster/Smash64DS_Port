"""Print what a .blend holds (objects, meshes, rig, materials, images) and
optionally render it (P6 A3 review). Run inside Blender:

    blender -b <file.blend> --python scripts/p6/blender/inspect_blend.py -- [--render out.png]
"""
import argparse
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_scene as S  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--render", type=Path)
    args = ap.parse_args(S.args_after_dashes())
    print("FILE", bpy.data.filepath)
    print("SCENES", [s.name for s in bpy.data.scenes], "frame", bpy.context.scene.frame_start, bpy.context.scene.frame_end)
    for obj in bpy.data.objects:
        line = f"OBJ {obj.name!r} type={obj.type} parent={obj.parent.name if obj.parent else None}"
        line += f" loc={tuple(round(v, 3) for v in obj.location)} rot={tuple(round(v, 3) for v in obj.rotation_euler)}"
        line += f" scale={tuple(round(v, 3) for v in obj.scale)} dims={tuple(round(v, 3) for v in obj.dimensions)}"
        line += f" hidden={obj.hide_get() if obj.name in bpy.context.view_layer.objects else 'n/a'} render_hidden={obj.hide_render}"
        if obj.type == "MESH":
            me = obj.data
            tris = sum(len(p.vertices) - 2 for p in me.polygons)
            line += f" verts={len(me.vertices)} faces={len(me.polygons)} tris={tris}"
            line += f" uv={[u.name for u in me.uv_layers]} vcol={[c.name for c in me.color_attributes]}"
            line += f" mats={[m.name if m else None for m in me.materials]}"
            line += f" groups={[g.name for g in obj.vertex_groups][:40]}"
            line += f" mods={[(m.type, getattr(m, 'object', None) and m.object.name) for m in obj.modifiers]}"
        if obj.type == "ARMATURE":
            line += f" bones={[b.name for b in obj.data.bones]}"
        print(line)
    for mat in bpy.data.materials:
        imgs = []
        if mat.node_tree:
            imgs = [n.image.name for n in mat.node_tree.nodes if n.type == "TEX_IMAGE" and n.image]
        print(f"MAT {mat.name!r} users={mat.users} diffuse={tuple(round(c, 3) for c in mat.diffuse_color)} images={imgs}")
    for img in bpy.data.images:
        print(f"IMG {img.name!r} size={tuple(img.size)} file={img.filepath!r} packed={img.packed_file is not None}")
    for act in bpy.data.actions:
        print(f"ACTION {act.name!r} frames={tuple(act.frame_range)}")
    if args.render:
        args.render.parent.mkdir(parents=True, exist_ok=True)
        targets = [o for o in bpy.context.scene.objects if o.type == "MESH" and not o.hide_render]
        for p in S.render(args.render, targets, views=("front", "side", "back", "three_quarter")):
            print("render", p)


if __name__ == "__main__":
    main()

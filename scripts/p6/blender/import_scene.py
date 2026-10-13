"""Build a .blend from a model_scene.py export (P6 A3).

Run inside Blender 5.2 (headless or live):
    blender -b --factory-startup --python scripts/p6/blender/import_scene.py -- \
        --scene <dir with scene.json> --name peach_high [--blend out.blend] [--render out.png]

The armature carries the JointTree's rest pose (bones J00..Jnn, each placed by
its joint's model-space rest matrix), the mesh one vertex a source Vtx with a
full-weight group for the joint whose matrix places it, so seam triangles span
two groups as they span two lists on the console. SSB64 is Y-up in its own
units; the import is Z-up and scaled by UNIT (stored on the objects), so a
fighter stands about two Blender units tall. Materials: the decoded texture
(nearest filtering) times the primitive colour, or the primitive colour alone.

Imports only bpy, mathutils and the standard library (plan: scripts in
scripts/p6/blender import only bpy, dae_reader and the pure-Python codecs).
"""
import argparse
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Matrix, Vector

UNIT = 0.01
# Y-up (SSB64) to Z-up (Blender): x, y, z -> x, -z, y.
Y_UP = Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))


def args_after_dashes():
    argv = sys.argv
    return argv[argv.index("--") + 1:] if "--" in argv else []


def to_blender(m) -> Matrix:
    """A scene.json model-space matrix (rows, Y-up, SSB64 units) in Blender."""
    mm = Matrix([list(r) for r in m])
    mm.translation = mm.translation * UNIT
    return Y_UP @ mm @ Y_UP.inverted()


def point(p) -> Vector:
    x, y, z = p
    return Vector((x * UNIT, -z * UNIT, y * UNIT))


def build(scene_dir: Path, name: str):
    data = json.loads((scene_dir / "scene.json").read_text(encoding="utf-8"))
    joints = data["joints"]
    # Armature.
    arm_data = bpy.data.armatures.new(f"{name}_rig")
    rig = bpy.data.objects.new(f"{name}_rig", arm_data)
    bpy.context.scene.collection.objects.link(rig)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode="EDIT")
    bones = []
    for j in joints:
        bone = arm_data.edit_bones.new(f"J{j['index']:02d}")
        m = to_blender(j["world"])
        head = m.translation.copy()
        # Tail toward the first child, else along the joint's own +x.
        kids = [k for k in joints if k["parent"] == j["index"]]
        tail = None
        for k in kids:
            t = to_blender(k["world"]).translation
            if (t - head).length > 1e-4:
                tail = t
                break
        if tail is None:
            tail = head + (m.to_3x3() @ Vector((1, 0, 0))).normalized() * 0.08
        bone.head = head
        bone.tail = tail
        bone["ssb64_world"] = [v for row in j["world"] for v in row]
        bones.append(bone)
    for j, bone in zip(joints, bones):
        if j["parent"] >= 0:
            bone.parent = bones[j["parent"]]
    bpy.ops.object.mode_set(mode="OBJECT")
    rig["ssb64_unit"] = UNIT

    # Mesh.
    mesh = bpy.data.meshes.new(f"{name}_mesh")
    verts = [point(v["model"]) for v in data["vertices"]]
    faces = [t["v"] for t in data["triangles"]]
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    # Source identities that survive edits (capture_ops.py): each vertex's
    # and triangle's index in scene.json.
    # Stored plus one: an element Blender adds reads 0, never a real index.
    sv = mesh.attributes.new("src_vert", "INT", "POINT")
    sv.data.foreach_set("value", list(range(1, len(verts) + 1)))
    st = mesh.attributes.new("src_tri", "INT", "FACE")
    st.data.foreach_set("value", list(range(1, len(faces) + 1)))
    obj = bpy.data.objects.new(f"{name}_mesh", mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.parent = rig
    obj["ssb64_unit"] = UNIT
    obj["ssb64_scene"] = str(scene_dir)
    groups = {}
    for i, v in enumerate(data["vertices"]):
        g = groups.get(v["joint"])
        if g is None:
            g = groups[v["joint"]] = obj.vertex_groups.new(name=f"J{v['joint']:02d}")
        g.add([i], 1.0, "REPLACE")
    mod = obj.modifiers.new("rig", "ARMATURE")
    mod.object = rig
    # Bake regions (look_k.py triangles carry one): R_<region> groups, the
    # form kit_decimate.py and bake_atlas.py read.
    rgroups = {}
    for tri in data["triangles"]:
        region = tri.get("region")
        if region is None:
            continue
        g = rgroups.get(region)
        if g is None:
            g = rgroups[region] = obj.vertex_groups.new(name=f"R_{region}")
        g.add(list(tri["v"]), 1.0, "REPLACE")

    # Materials and UVs.
    uv = mesh.uv_layers.new(name="UV")
    mats = []
    for index, m in enumerate(data["materials"]):
        mat = bpy.data.materials.new(f"{name}_m{index:02d}")
        mat.use_nodes = True
        nodes = mat.node_tree.nodes
        bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
        prim = int(m["prim"], 16)
        # Primitive colours are display (sRGB) values, as the console shows
        # them; Blender's colour sockets are linear (textures convert on
        # load), so convert or every untextured part previews washed out.
        rgba = [((prim >> s) & 255) / 255.0 for s in (24, 16, 8, 0)]
        rgba = [(c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4) for c in rgba[:3]] + rgba[3:]
        if m["texture"]:
            tex = nodes.new("ShaderNodeTexImage")
            tex.image = bpy.data.images.load(str(scene_dir / m["texture"]["file"]))
            tex.interpolation = "Closest"
            # The tile's wrap (model_scene.py): clamped tiles (the mouth's
            # lips, drawn once) must not repeat across the face.
            wrap = m["texture"].get("wrap") or ["repeat", "repeat"]
            if "clamp" in wrap:
                tex.extension = "EXTEND"
            elif wrap == ["mirror", "mirror"]:
                tex.extension = "MIRROR"
            mix = nodes.new("ShaderNodeMix")
            mix.data_type = "RGBA"
            mix.blend_type = "MULTIPLY"
            mix.inputs["Factor"].default_value = 1.0
            mat.node_tree.links.new(tex.outputs["Color"], mix.inputs["A"])
            mix.inputs["B"].default_value = rgba
            mat.node_tree.links.new(mix.outputs["Result"], bsdf.inputs["Base Color"])
            mat.node_tree.links.new(tex.outputs["Alpha"], bsdf.inputs["Alpha"])
        else:
            bsdf.inputs["Base Color"].default_value = rgba
        bsdf.inputs["Roughness"].default_value = 1.0
        # Two-sided materials (sprites) draw both faces; the rest cull backs,
        # as the console does.
        mat.use_backface_culling = not m.get("two_sided", False)
        # Workbench (the review renders) shows diffuse_color for untextured faces.
        mat.diffuse_color = rgba
        mat["ssb64"] = json.dumps(m)
        mesh.materials.append(mat)
        mats.append(m)
    for poly, tri in zip(mesh.polygons, data["triangles"]):
        poly.material_index = tri["material"]
        m = mats[tri["material"]]
        w = h = 32
        if m["texture"]:
            w, h = m["texture"]["width"], m["texture"]["height"]
        for loop_index, vi in zip(poly.loop_indices, tri["v"]):
            vert = data["vertices"][vi]
            if "uv" in vert:
                uv.data[loop_index].uv = vert["uv"]
                continue
            s, t = vert["st"]
            uv.data[loop_index].uv = (s / 32.0 / w, 1.0 - t / 32.0 / h)
    return rig, obj


def render(path: Path, targets, views=("front", "side", "three_quarter")):
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_WORKBENCH"
    except TypeError:
        pass
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "TEXTURE"
    scene.render.resolution_x = 512
    scene.render.resolution_y = 512
    scene.render.film_transparent = False
    # Frame all targets.
    lo = Vector((1e9, 1e9, 1e9))
    hi = Vector((-1e9, -1e9, -1e9))
    for obj in targets:
        dg = bpy.context.evaluated_depsgraph_get()
        ev = obj.evaluated_get(dg)
        for v in ev.to_mesh().vertices:
            w = obj.matrix_world @ v.co
            lo = Vector(map(min, lo, w))
            hi = Vector(map(max, hi, w))
        ev.to_mesh_clear()
    centre = (lo + hi) / 2
    size = max((hi - lo).length, 0.1)
    cam_data = bpy.data.cameras.new("review_cam")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = size * 1.05
    cam = bpy.data.objects.new("review_cam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    directions = {"front": (0, -1, 0), "side": (1, 0, 0), "back": (0, 1, 0),
                  "three_quarter": (0.7, -0.7, 0.15)}
    out = []
    for view in views:
        d = Vector(directions[view]).normalized()
        cam.location = centre + d * size * 3
        cam.rotation_euler = (centre - cam.location).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = str(path.with_name(f"{path.stem}_{view}{path.suffix}"))
        bpy.ops.render.render(write_still=True)
        out.append(scene.render.filepath)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--scene", type=Path, required=True)
    ap.add_argument("--name", required=True)
    ap.add_argument("--blend", type=Path)
    ap.add_argument("--render", type=Path)
    ap.add_argument("--clear", action="store_true", help="start from an empty scene")
    args = ap.parse_args(args_after_dashes())
    if args.clear:
        for obj in list(bpy.data.objects):
            bpy.data.objects.remove(obj, do_unlink=True)
    rig, obj = build(args.scene, args.name)
    print(f"built {rig.name} ({len(rig.data.bones)} bones), {obj.name} "
          f"({len(obj.data.vertices)} vertices, {len(obj.data.polygons)} triangles)")
    if args.render:
        args.render.parent.mkdir(parents=True, exist_ok=True)
        for p in render(args.render, [obj]):
            print("render", p)
    if args.blend:
        args.blend.parent.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=str(args.blend))
        print("saved", args.blend)


if __name__ == "__main__":
    main()

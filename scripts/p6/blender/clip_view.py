"""Play a fighter clip on an imported look or base model (P6 A6).

Run inside Blender 5.2 (headless or live) after import_scene.build:

    import clip_view
    clip_view.render_frames(rig_name, frames_json, out_dir, view="side")

frames_json (anim.py world_frames, written by the host) lists, per frame,
every joint's world matrix in SSB64 model space. A vertex rides its joint
rigidly, so each bone's deformation is D = W_frame @ inverse(W_rest), and
its pose basis follows from its parent's: R^-1 @ D_parent^-1 @ D @ R, with R
the bone's rest matrix (armature space). Renders one PNG a frame from an
orthographic camera; review_sheet.py lays them out.

Imports only bpy, mathutils and the standard library.
"""
import json
from pathlib import Path

import bpy
from mathutils import Matrix, Vector

import import_scene as S


def deformations(rig, frame_worlds):
    """Blender-space deformation per bone for one frame."""
    out = {}
    for bone in rig.data.bones:
        index = int(bone.name[1:])
        rest = Matrix([bone["ssb64_world"][r * 4:r * 4 + 4] for r in range(4)])
        anim = Matrix(frame_worlds[index])
        out[bone.name] = S.to_blender(anim @ rest.inverted())
    return out


def apply_pose(rig, frame_worlds):
    d = deformations(rig, frame_worlds)
    for pb in rig.pose.bones:
        rest = pb.bone.matrix_local
        parent = d[pb.parent.name] if pb.parent else Matrix.Identity(4)
        pb.matrix_basis = rest.inverted() @ parent.inverted() @ d[pb.name] @ rest
    bpy.context.view_layer.update()


def clear_pose(rig):
    for pb in rig.pose.bones:
        pb.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()


VIEWS = {"side": Vector((1.0, 0.0, 0.0)), "front": Vector((0.0, -1.0, 0.0)),
         "three_quarter": Vector((0.7, -0.7, 0.15))}


def _spheres(rig, count):
    """A pool of hitbox spheres (magenta), parented to the rig's space."""
    mat = bpy.data.materials.get("_hitbox")
    if mat is None:
        # A one-texel image: the workbench's texture colour mode shows it.
        mat = bpy.data.materials.new("_hitbox")
        mat.diffuse_color = (1.0, 0.1, 0.6, 1.0)
        img = bpy.data.images.new("_hitbox_px", 1, 1)
        img.pixels = [1.0, 0.1, 0.6, 1.0]
        mat.use_nodes = True
        nodes = mat.node_tree.nodes
        bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
        tex = nodes.new("ShaderNodeTexImage")
        tex.image = img
        mat.node_tree.links.new(tex.outputs[0], bsdf.inputs[0])
    pool = []
    for i in range(count):
        name = f"_hitbox_{i}"
        ob = bpy.data.objects.get(name)
        if ob is None:
            me = bpy.data.meshes.new(name)
            import bmesh
            bm = bmesh.new()
            bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=1.0)
            bm.to_mesh(me)
            bm.free()
            me.materials.append(mat)
            ob = bpy.data.objects.new(name, me)
            bpy.context.scene.collection.objects.link(ob)
        ob.parent = rig
        pool.append(ob)
    return pool


def render_frames(rig_name, frames_json, out_dir, view="side", size=320, ortho=5.5,
                  centre=(0.0, 0.0, 2.2), step=1, hitboxes_json=None):
    """hitboxes_json: per frame a list of [x, y, z, radius] (model units)."""
    rig = bpy.data.objects[rig_name]
    frames = json.loads(Path(frames_json).read_text(encoding="utf-8"))
    hits = json.loads(Path(hitboxes_json).read_text(encoding="utf-8")) if hitboxes_json else None
    pool = _spheres(rig, max((len(h) for h in hits), default=0)) if hits else []
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    scene = bpy.context.scene
    cam_data = bpy.data.cameras.get("_clip_cam") or bpy.data.cameras.new("_clip_cam")
    cam = bpy.data.objects.get("_clip_cam") or bpy.data.objects.new("_clip_cam", cam_data)
    if cam.name not in scene.collection.objects:
        scene.collection.objects.link(cam)
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = ortho
    c = rig.matrix_world @ Vector(centre)
    d = VIEWS[view].normalized()
    cam.location = c + d * 20
    cam.rotation_euler = (c - cam.location).to_track_quat("-Z", "Y").to_euler()
    scene.camera = cam
    scene.render.resolution_x = scene.render.resolution_y = size
    try:
        scene.render.engine = "BLENDER_WORKBENCH"
    except TypeError:
        pass
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "TEXTURE"
    # Render only this rig's meshes.
    keep = {o.name for o in rig.children_recursive} | {rig.name, cam.name} | {o.name for o in pool}
    hidden = [o for o in scene.objects if o.name not in keep and not o.hide_render]
    for o in hidden:
        o.hide_render = True
    files = []
    try:
        for i in range(0, len(frames), step):
            apply_pose(rig, frames[i])
            now = hits[i] if hits and i < len(hits) else []
            for k, ob in enumerate(pool):
                if k < len(now):
                    x, y, z, r = now[k]
                    ob.location = S.point((x, y, z))
                    ob.scale = (r * S.UNIT,) * 3
                    ob.hide_render = False
                else:
                    ob.hide_render = True
            scene.render.filepath = str(out_dir / f"f{i:03d}.png")
            bpy.ops.render.render(write_still=True)
            files.append(f"f{i:03d}.png")
    finally:
        clear_pose(rig)
        for o in hidden:
            o.hide_render = False
        for ob in pool:
            ob.hide_render = True
    return files

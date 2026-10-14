"""A select portrait of a look (P6 A9): the head and shoulders from the front
three-quarter, rendered with a transparent background for p6_present.py to
frame as Remix's 32x32 portraits are (a dark gradient, its flash white).

    blender -b <look .blend from import_scene.py> --python scripts/p6/blender/portrait.py -- \\
        --rig pal_high_rig --out portrait.png [--size 128] [--head J08] [--frame 1.6] [--yaw 28]

The camera looks at the head joint's rest position from `yaw` degrees off
the front, a little above; `frame` is the shown height in head-joint-to-crown
units. Imports only bpy, mathutils and the standard library.
"""
import argparse
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_scene as S  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rig", required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--size", type=int, default=128)
    ap.add_argument("--head", default="J08")
    ap.add_argument("--yaw", type=float, default=28.0)
    ap.add_argument("--pitch", type=float, default=6.0)
    ap.add_argument("--frame", type=float, default=0.62, help="ortho height in Blender units")
    ap.add_argument("--lift", type=float, default=0.06, help="aim above the head joint (Blender units)")
    args = ap.parse_args(S.args_after_dashes())
    scene = bpy.context.scene
    rig = bpy.data.objects[args.rig]
    bone = rig.data.bones[args.head]
    target = rig.matrix_world @ bone.head_local + Vector((0.0, 0.0, args.lift))
    keep = {o.name for o in rig.children_recursive} | {rig.name}
    for o in scene.objects:
        if o.type in ("MESH", "ARMATURE") and o.name not in keep:
            o.hide_render = True
    cam_data = bpy.data.cameras.new("_portrait_cam")
    cam = bpy.data.objects.new("_portrait_cam", cam_data)
    scene.collection.objects.link(cam)
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = args.frame
    yaw, pitch = math.radians(args.yaw), math.radians(args.pitch)
    # The model faces -Y in Blender (its +Z forward); the camera sits in
    # front, turned toward her right.
    d = Vector((math.sin(yaw) * math.cos(pitch), -math.cos(yaw) * math.cos(pitch), math.sin(pitch)))
    cam.location = target + d * 10.0
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
    scene.camera = cam
    try:
        scene.render.engine = "BLENDER_WORKBENCH"
    except TypeError:
        pass
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "TEXTURE"
    scene.render.film_transparent = True
    scene.render.resolution_x = scene.render.resolution_y = args.size
    scene.render.resolution_percentage = 100
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.filepath = str(args.out)
    bpy.ops.render.render(write_still=True)
    print("portrait", args.out)


if __name__ == "__main__":
    main()

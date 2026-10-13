"""Review renders of one look object (P6 A6): front, three-quarter, side and
back close-ups, and the battle-size view (the DS screen: 256x192, a fighter
about 46 pixels tall, nearest-neighbour, then enlarged 3x), on one sheet.

    review_images(obj_name, out_dir) in the live Blender (MCP), or headless
    (scripts/p6/review_sheet.py joins the images on the host: Blender's
    Python has no Pillow):
    blender -b <scene.blend> --python scripts/p6/blender/review.py -- --object pal_low --out x.png
"""
import argparse
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_scene as S  # noqa: E402

VIEWS = {"front": (0.0, -1.0, 0.0), "three_quarter": (0.75, -0.65, 0.12),
         "side": (1.0, 0.0, 0.0), "back": (0.0, 1.0, 0.0)}


def _bounds(obj):
    pts = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return lo, hi


def _render(path, cam, res):
    scene = bpy.context.scene
    scene.camera = cam
    scene.render.resolution_x, scene.render.resolution_y = res
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)


def review_images(obj_name: str, out_dir: Path, extra_names=(), body_height: float | None = None):
    """Render the views of `obj_name` (others hidden) into out_dir; return
    the file names. The host joins them (review_sheet.py)."""
    scene = bpy.context.scene
    keep = {obj_name, *extra_names}
    hidden = []
    for o in scene.objects:
        if o.type in ("MESH", "FONT", "ARMATURE") and o.name not in keep and not o.hide_render:
            o.hide_render = True
            hidden.append(o)
    try:
        engine = scene.render.engine
        try:
            scene.render.engine = "BLENDER_WORKBENCH"
        except TypeError:
            pass
        scene.display.shading.light = "STUDIO"
        scene.display.shading.color_type = "TEXTURE"
        # Culling per material (import_scene.py: two-sided sprites draw
        # both faces, the rest cull backs), as the console draws them.
        scene.display.shading.show_backface_culling = False
        scene.render.film_transparent = False
        obj = bpy.data.objects[obj_name]
        lo, hi = _bounds(obj)
        centre = (lo + hi) / 2
        height = hi.z - lo.z
        cam = bpy.data.objects.get("_review_cam")
        if cam is None:
            cam = bpy.data.objects.new("_review_cam", bpy.data.cameras.new("_review_cam"))
            scene.collection.objects.link(cam)
        cam.data.type = "ORTHO"
        out_dir.mkdir(parents=True, exist_ok=True)
        files = []
        for view, d in VIEWS.items():
            d = Vector(d).normalized()
            cam.data.ortho_scale = max(hi.x - lo.x, height) * 1.08
            cam.location = centre + d * 20.0
            cam.rotation_euler = (centre - cam.location).to_track_quat("-Z", "Y").to_euler()
            path = out_dir / f"{view}.png"
            _render(path, cam, (512, 512))
            files.append(path.name)
        # Battle size: the default battle camera draws a fighter ~46 px tall
        # on the 192-px screen; the camera looks slightly down from the side.
        h = body_height if body_height else height
        cam.data.ortho_scale = h * 192.0 / 46.0
        d = Vector((0.0, -1.0, 0.18)).normalized()
        cam.location = centre + d * 20.0
        cam.rotation_euler = (centre - cam.location).to_track_quat("-Z", "Y").to_euler()
        scene.render.resolution_percentage = 100
        _render(out_dir / "battle_raw.png", cam, (256, 192))
        files.append("battle_raw.png")
        scene.render.engine = engine
        return files
    finally:
        for o in hidden:
            o.hide_render = False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--object", required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args(S.args_after_dashes())
    print(review_images(args.object, args.out))


if __name__ == "__main__":
    main()

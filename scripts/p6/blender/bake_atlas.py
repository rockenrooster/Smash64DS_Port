"""Bake the fitted kit's albedo onto a reduced look's own atlas (P6 A3b).

The reduced mesh gets fresh UVs (Smart UV Project) and one atlas image;
Cycles bakes the diffuse COLOUR (no lighting: SSB64 lights by vertex
normals) from the fitted kit, "selected to active", so the face, eyes and
mouth overlays, the trims and the hair all land in the atlas whatever the
decimation did to the kit's own UVs.

Region by region: a reduced face only takes texels from the kit faces of its
own region (kit_decimate.py's regions on both sides), so the back of the
dress never picks up the hair hanging over it and no face reads the legs
under the dress. Each region bakes into the same image (no clear).

    blender -b <scene.blend> --python scripts/p6/blender/bake_atlas.py -- \
        --target k_dec_low --fit <kit_high_fit.json> --kit-dir <kit folder> \
        --ops art/p6/palutena/look/k_dec_low.json --offset 12.6 --size 128 \
        --out builds/p6-art/palutena/k_dec/k_dec_low_atlas.png
Kit-derived output: builds/ only (plan D10).
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
import kit_decimate as KD  # noqa: E402

SOURCE_MESHES = ("body_mh_0_0", "body_mh_0_1", "face_mh_25_0", "Palutena_Eye_mh_19_0",
                 "Palutena_Mouth_mh_46_0")


def _keep_faces(obj, keep):
    """Delete obj's faces whose index is not in `keep`."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    gone = [f for f in bm.faces if f.index not in keep]
    bmesh.ops.delete(bm, geom=gone, context="FACES")
    bm.to_mesh(obj.data)
    bm.free()


def _duplicate(obj, name):
    dup = obj.copy()
    dup.data = obj.data.copy()
    dup.name = name
    bpy.context.scene.collection.objects.link(dup)
    return dup


def bake(target_name: str, fit: Path, kit_dir: Path, ops: dict, offset: float, size: int, out: Path,
         sources=SOURCE_MESHES, cage: float = 0.06, distance: float = 0.25, margin_px: int = 2):
    target = bpy.data.objects[target_name]
    # The kit sources, joined into one mesh with its regions.
    srcs = [o for o in F.build(fit, kit_dir, f"{target_name}_bake_src", offset)]
    for o in list(srcs):
        if o.get("kit_mesh") not in sources:
            bpy.data.objects.remove(o, do_unlink=True)
            srcs.remove(o)
    for o in bpy.context.selected_objects:
        o.select_set(False)
    for o in srcs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = srcs[0]
    bpy.ops.object.join()
    src = bpy.context.view_layer.objects.active
    src.name = f"{target_name}_bake_src"
    src_vreg = KD.classify(src, ops)
    src_freg = KD._face_regions(src.data, src_vreg)
    # Fresh UVs on the target: its own atlas layout.
    me = target.data
    uv = me.uv_layers.get("atlas") or me.uv_layers.new(name="atlas")
    me.uv_layers.active = uv
    for o in bpy.context.selected_objects:
        o.select_set(False)
    target.select_set(True)
    bpy.context.view_layer.objects.active = target
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.03, area_weight=0.0, scale_to_bounds=True)
    bpy.ops.object.mode_set(mode="OBJECT")
    # One material, one image node to bake into.
    img = bpy.data.images.get(out.stem)
    if img is None:
        img = bpy.data.images.new(out.stem, size, size, alpha=False)
    img.scale(size, size)
    img.generated_color = (0.0, 0.0, 0.0, 1.0)
    mat = bpy.data.materials.get(f"{target_name}_atlas") or bpy.data.materials.new(f"{target_name}_atlas")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    tex = next((n for n in nodes if n.type == "TEX_IMAGE"), None) or nodes.new("ShaderNodeTexImage")
    tex.image = img
    tex.interpolation = "Closest"
    mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    nodes.active = tex
    me.materials.clear()
    me.materials.append(mat)
    for poly in me.polygons:
        poly.material_index = 0
    # Region by region.
    rgroups = {g.name[2:]: g for g in target.vertex_groups if g.name.startswith("R_")}
    t_vreg = KD._vertex_regions(target, rgroups)
    t_freg = KD._face_regions(me, t_vreg)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 4
    scene.cycles.device = "CPU"
    first = True
    for region in sorted(set(r for r in t_freg if r is not None)):
        t_keep = {i for i, r in enumerate(t_freg) if r == region}
        s_keep = {i for i, r in enumerate(src_freg) if r == region}
        if not s_keep:
            s_keep = set(range(len(src.data.polygons)))
        tt = _duplicate(target, f"_bake_t_{region}")
        ss = _duplicate(src, f"_bake_s_{region}")
        _keep_faces(tt, t_keep)
        _keep_faces(ss, s_keep)
        for o in bpy.context.selected_objects:
            o.select_set(False)
        ss.select_set(True)
        tt.select_set(True)
        bpy.context.view_layer.objects.active = tt
        bpy.ops.object.bake(type="DIFFUSE", pass_filter={"COLOR"}, use_selected_to_active=True,
                            cage_extrusion=cage, max_ray_distance=distance, margin=margin_px,
                            uv_layer="atlas", use_clear=first)
        first = False
        bpy.data.objects.remove(tt, do_unlink=True)
        bpy.data.objects.remove(ss, do_unlink=True)
    out.parent.mkdir(parents=True, exist_ok=True)
    img.filepath_raw = str(out)
    img.file_format = "PNG"
    img.save()
    bpy.data.objects.remove(src, do_unlink=True)
    coll = bpy.data.collections.get(f"{target_name}_bake_src")
    if coll is not None:
        bpy.data.collections.remove(coll)
    return img


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--target", required=True)
    ap.add_argument("--fit", type=Path, required=True)
    ap.add_argument("--kit-dir", type=Path, required=True)
    ap.add_argument("--ops", type=Path, required=True)
    ap.add_argument("--offset", type=float, default=0.0)
    ap.add_argument("--size", type=int, default=128)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--blend", type=Path)
    args = ap.parse_args(S.args_after_dashes())
    ops = json.loads(args.ops.read_text(encoding="utf-8"))
    bake(args.target, args.fit, args.kit_dir, ops, args.offset, args.size, args.out)
    if args.blend:
        bpy.ops.wm.save_as_mainfile(filepath=str(args.blend))


if __name__ == "__main__":
    main()

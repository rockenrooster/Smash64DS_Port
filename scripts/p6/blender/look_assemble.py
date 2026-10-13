"""Assemble a K look from three sources (P6 A3b, owner 2026-10-13: the kit
route; regions mix as plan G0 allows):

- the reduced kit (kit_decimate.py) for the regions `keep_regions` names --
  the shapes only the 3DS model has (hair, dress, torso);
- built ring parts (look_k.py scene: head, neck, arms, legs, feet, staff,
  shield), each fitted to the kit's cross-sections;
- Peach's own hands (owner 2026-10-13: "the hands can be solid like
  peach"), moved onto the look's skeleton (kit proportions);

joined into one mesh with its R_<region> groups, then baked into one atlas
from the fitted kit (bake_atlas.py, region by region).

Runs in the live Blender (MCP) or headless; kit-derived output stays under
builds/ (plan D10).
"""
import json
import sys
from pathlib import Path

import bpy
import bmesh
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_scene as S  # noqa: E402
import kit_decimate as KD  # noqa: E402
import bake_atlas as BA  # noqa: E402


def _keep_regions(obj, keep):
    rgroups = {g.name[2:]: g for g in obj.vertex_groups if g.name.startswith("R_")}
    vreg = KD._vertex_regions(obj, rgroups)
    freg = KD._face_regions(obj.data, vreg)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    gone = [f for f, r in zip(bm.faces, freg) if r not in keep]
    bmesh.ops.delete(bm, geom=gone, context="FACES")
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bm.to_mesh(obj.data)
    bm.free()


def _world_mesh(obj):
    """Unparent keeping the world placement; drop modifiers."""
    mw = obj.matrix_world.copy()
    obj.parent = None
    obj.matrix_world = mw
    for m in list(obj.modifiers):
        obj.modifiers.remove(m)
    return obj


def peach_hands(peach_scene: Path, skeleton: list, joints, name: str, offset: float, scale: float = 1.0):
    """Peach's hand triangles (joints `joints`) moved onto the look's joints:
    each vertex keeps its place in its joint's frame (times `scale`: the look
    is taller than Peach), the joint stands where the look's skeleton puts it."""
    data = json.loads(peach_scene.read_text(encoding="utf-8"))
    tris = [t for t in data["triangles"] if t["joint"] in joints]
    used = sorted({v for t in tris for v in t["v"]})
    index = {v: i for i, v in enumerate(used)}
    verts = []
    for v in used:
        vert = data["vertices"][v]
        j = vert["joint"]
        w_peach = Matrix([list(r) for r in data["joints"][j]["world"]])
        w_look = w_peach.copy()
        w_look.translation = Vector(skeleton[j])
        local = w_peach.inverted() @ Vector(vert["model"])
        p = w_look @ (local * scale)
        verts.append(S.point(p) + Vector((offset, 0.0, 0.0)))
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts, [], [[index[v] for v in t["v"]] for t in tris])
    me.update()
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    g = obj.vertex_groups.new(name="R_hands")
    g.add(list(range(len(verts))), 1.0, "REPLACE")
    return obj


def assemble(name: str, offset: float, dec_fit: Path, kit_dir: Path, dec_ops: dict,
             parts_dir: Path, peach_scene: Path, skeleton_fit: Path):
    for o in [o for o in bpy.data.objects if o.name == name or o.name.startswith(name + "_")]:
        bpy.data.objects.remove(o, do_unlink=True)
    dec = KD.build(dec_fit, kit_dir, dec_ops, f"{name}_dec", offset)
    _keep_regions(dec, set(dec_ops["keep_regions"]))
    rig, parts = S.build(parts_dir, f"{name}_parts")
    rig.location.x = offset
    bpy.context.view_layer.update()
    parts = _world_mesh(parts)
    bpy.data.objects.remove(rig, do_unlink=True)
    skeleton = json.loads(skeleton_fit.read_text(encoding="utf-8"))["skeleton"]
    hands = peach_hands(peach_scene, skeleton, dec_ops.get("hand_joints", (6, 13)), f"{name}_hands", offset,
                        dec_ops.get("hand_scale", 1.0))
    for o in bpy.context.selected_objects:
        o.select_set(False)
    for o in (dec, parts, hands):
        o.select_set(True)
    bpy.context.view_layer.objects.active = dec
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    obj.data.name = name
    # Only the region groups travel on (the kit's joint groups did their job).
    for g in list(obj.vertex_groups):
        if not g.name.startswith("R_"):
            obj.vertex_groups.remove(g)
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    rgroups = {g.name[2:]: g for g in obj.vertex_groups}
    vreg = KD._vertex_regions(obj, rgroups)
    freg = KD._face_regions(obj.data, vreg)
    counts = {}
    for r in freg:
        counts[r] = counts.get(r, 0) + 1
    print(f"{name}: {tris} triangles " + json.dumps(dict(sorted(counts.items(), key=lambda kv: str(kv[0])))))
    return obj

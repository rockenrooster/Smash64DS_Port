"""Capture edits made in Blender to an imported look as ops (P6 A3, plan G0:
"hand-tuning is captured as ops, not stored as geometry").

A look object made by import_scene.py carries each vertex's and triangle's
scene.json index (attributes src_vert / src_tri, stored plus one). After
edits -- by hand in the live Blender or by script -- capture() compares the
object with its scene.json and writes:

- moved: source vertex -> offset (SSB64 units), for vertices that moved;
- deleted_tris: source triangles no longer there;
- added_vertices: new vertices (model position, joint from their J## group,
  texture coordinates from the loops that use them);
- added_tris: new triangles (vertex refs: ["s", source index] or ["a", new
  index]), each with its material and the joint of its first vertex;
- retint: source triangle -> material, for triangles kept but recoloured
  (retint_verts names them by vertex set, which replay prefers).

Materials are named by what they are ({"prim", "texture"}), not by slot.

look_r.py compose replays the file after everything else, so the edits stay
on top of every rebuild. Offsets and new positions are base-model data (the
look's own vertices), never kit data (D10).
"""
import json
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_scene as S  # noqa: E402


def _model(co):
    """Object-local Blender coordinates (Z-up, UNIT) -> SSB64 model space."""
    return [co.x / S.UNIT, co.z / S.UNIT, -co.y / S.UNIT]


def capture(obj_name: str, out: Path, tolerance: float = 0.05) -> dict:
    obj = bpy.data.objects[obj_name]
    scene_dir = Path(obj["ssb64_scene"])
    data = json.loads((scene_dir / "scene.json").read_text(encoding="utf-8"))
    me = obj.data
    sv = [0] * len(me.vertices)
    me.attributes["src_vert"].data.foreach_get("value", sv)
    st = [0] * len(me.polygons)
    me.attributes["src_tri"].data.foreach_get("value", st)
    joint_of_group = {g.index: int(g.name[1:]) for g in obj.vertex_groups if g.name[:1] == "J" and g.name[1:].isdigit()}
    seen_src = set()
    ref = {}
    added_vertices = []
    moved = {}
    uv = me.uv_layers.active
    loop_uv = {}
    if uv is not None:
        for poly in me.polygons:
            for li in poly.loop_indices:
                loop_uv.setdefault(me.loops[li].vertex_index, list(uv.data[li].uv))
    for v in me.vertices:
        src = sv[v.index] - 1
        pos = _model(v.co)
        if src >= 0 and src not in seen_src:
            seen_src.add(src)
            ref[v.index] = ["s", src]
            orig = data["vertices"][src]["model"]
            d = [pos[i] - orig[i] for i in range(3)]
            if max(abs(c) for c in d) > tolerance:
                moved[str(src)] = [round(c, 3) for c in d]
            continue
        joint = None
        for g in v.groups:
            if g.group in joint_of_group and g.weight > 0.5:
                joint = joint_of_group[g.group]
                break
        if joint is None and src >= 0:
            joint = data["vertices"][src]["joint"]
        ref[v.index] = ["a", len(added_vertices)]
        added_vertices.append({"joint": joint, "model": [round(c, 3) for c in pos],
                               "uv": loop_uv.get(v.index, [0.0, 0.0])})
    present = {t - 1 for t in st if t > 0}
    deleted = sorted(set(range(len(data["triangles"]))) - present)
    added_tris = []
    retint = {}
    # By vertex set too: the composed scene has its dropped triangles
    # removed, the replay has not (look_r.py removes them after the edits).
    retint_verts = []
    seen_tri = set()

    def material(mi):
        # By what it is, not by slot: a recolour rule added later shifts
        # the composed scene's material indices, never a material's colour.
        m = data["materials"][mi]
        return {"prim": m["prim"], "texture": (m.get("texture") or {}).get("file")}
    for poly, t in zip(me.polygons, st):
        src = t - 1
        if src >= 0 and src not in seen_tri and len(poly.vertices) == 3:
            same = [ref[i] for i in poly.vertices] == [["s", s_] for s_ in data["triangles"][src]["v"]]
            seen_tri.add(src)
            if same:
                if poly.material_index != data["triangles"][src]["material"]:
                    retint[str(src)] = material(poly.material_index)
                    retint_verts.append({"v": sorted(data["triangles"][src]["v"]),
                                         "material": material(poly.material_index)})
                continue
            deleted.append(src)
        vs = list(poly.vertices)
        tris = [vs] if len(vs) == 3 else [[vs[0], vs[i], vs[i + 1]] for i in range(1, len(vs) - 1)]
        for tri in tris:
            first = ref[tri[0]]
            joint = (data["vertices"][first[1]]["joint"] if first[0] == "s"
                     else added_vertices[first[1]]["joint"])
            added_tris.append({"v": [ref[i] for i in tri], "material": material(poly.material_index), "joint": joint})
    gone = sorted(set(deleted))
    ops = {"form": "p6-look-edits-v1", "scene": scene_dir.name, "moved": moved, "retint": retint,
           "retint_verts": retint_verts,
           "deleted_tris": gone,
           # By vertex set too: replay finds them whatever came before.
           "deleted_tri_verts": [sorted(data["triangles"][t]["v"]) for t in gone],
           "added_vertices": added_vertices, "added_tris": added_tris}
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(ops, indent=1) + "\n", encoding="utf-8")
    print(f"captured {obj_name}: {len(moved)} moved, {len(ops['deleted_tris'])} deleted, "
          f"{len(added_vertices)} added vertices, {len(added_tris)} added triangles -> {out}")
    return ops

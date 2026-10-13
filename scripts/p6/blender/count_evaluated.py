"""Triangle counts of every mesh after its modifiers (decimation), and each
Decimate modifier's settings (P6 A3 review). Run inside Blender:

    blender -b <file.blend> --python scripts/p6/blender/count_evaluated.py
"""
import bpy

dg = bpy.context.evaluated_depsgraph_get()
total_src = total_eval = 0
for obj in bpy.data.objects:
    if obj.type != "MESH":
        continue
    src = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    ev = obj.evaluated_get(dg)
    me = ev.to_mesh()
    tris = sum(len(p.vertices) - 2 for p in me.polygons)
    used = len({v for p in me.polygons for v in p.vertices})
    ev.to_mesh_clear()
    decs = [(m.decimate_type, round(m.ratio, 4), m.iterations, round(m.angle_limit, 4), m.show_viewport, m.show_render)
            for m in obj.modifiers if m.type == "DECIMATE"]
    total_src += src
    total_eval += tris
    print(f"COUNT {obj.name}: {src} -> {tris} tris, {used} vertices used, decimate {decs}")
print(f"TOTAL {total_src} -> {total_eval} tris")
arm = next((o for o in bpy.data.objects if o.type == "ARMATURE"), None)
if arm is not None:
    print("POSE", arm.data.pose_position, "rest-different:",
          any(b.matrix_basis != b.matrix_basis.Identity(4) for b in arm.pose.bones))
    for b in arm.pose.bones:
        if b.rotation_quaternion[0] < 0.9999 or any(abs(v) > 1e-4 for v in b.rotation_euler) or b.location.length > 1e-4:
            print("POSED", b.name, tuple(round(v, 3) for v in b.rotation_quaternion), tuple(round(v, 3) for v in b.location))

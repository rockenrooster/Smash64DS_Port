#!/usr/bin/env python3
"""Read the 3DS kit's COLLADA models (P6 A3): joints, skins, meshes, materials.

Pure Python (xml.etree), so Blender's interpreter and the host share it. The
kit's files (decomp/alt_assets/Fighters/Super Smash Bros 3DS/, local and
ignored: plan D10) come from one exporter and share its shape:

- the skeleton is a tree of JOINT nodes under the visual scene, each with
  translate, rotateZ, rotateY, rotateX and scale (or one matrix); skins name
  joints by their sid (HAL's names: TransN, RotN, HipN, ...);
- every geometry carries the model's whole vertex pool (positions, normals,
  texture coordinates, one index for all three) and its own triangles;
- every controller skins one geometry with at most two influences a vertex;
- the files are Y-up.

The face expressions and hand poses are separate meshes over the pool. The
neutral set -- body, face, the plain Eye/Eyebrow/Lashes/Mouth meshes, the first
hand pose a side and the weapon -- is what the fighter draws at once
(`neutral_meshes`); its triangle count is the kit row of P6_PLAN.md's table.

Nothing read here is written to a tracked file (plan D10): the readers' users
keep kit-derived data under builds/.
"""
from __future__ import annotations

import argparse
import math
import re
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path

NS = "{http://www.collada.org/2005/11/COLLADASchema}"

Matrix = tuple  # 16 floats, row-major, column vectors (COLLADA's convention)

IDENTITY: Matrix = (1.0, 0.0, 0.0, 0.0,
                    0.0, 1.0, 0.0, 0.0,
                    0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0)

# The neutral set a fighter draws at once (module docstring).
NEUTRAL_RE = re.compile(
    r"^(?:(?:body|double|face|weapon)_mh_\d+_\d+"
    r"|[A-Za-z]+_(?:Eye|Eyebrow|Lashes|Mouth)_mh_\d+_\d+"
    r"|[LR]hand_1_VIS_O_OBJ_\d+_\d+"
    r"|[A-Za-z]+_sword_VIS_O_OBJ_\d+_\d+)$")


def mat_mul(a: Matrix, b: Matrix) -> Matrix:
    return tuple(sum(a[r * 4 + k] * b[k * 4 + c] for k in range(4))
                 for r in range(4) for c in range(4))


def mat_point(m: Matrix, p) -> tuple:
    x, y, z = p
    return (m[0] * x + m[1] * y + m[2] * z + m[3],
            m[4] * x + m[5] * y + m[6] * z + m[7],
            m[8] * x + m[9] * y + m[10] * z + m[11])


def mat_dir(m: Matrix, d) -> tuple:
    x, y, z = d
    return (m[0] * x + m[1] * y + m[2] * z,
            m[4] * x + m[5] * y + m[6] * z,
            m[8] * x + m[9] * y + m[10] * z)


def translate(x: float, y: float, z: float) -> Matrix:
    return (1.0, 0.0, 0.0, x, 0.0, 1.0, 0.0, y, 0.0, 0.0, 1.0, z, 0.0, 0.0, 0.0, 1.0)


def scale(x: float, y: float, z: float) -> Matrix:
    return (x, 0.0, 0.0, 0.0, 0.0, y, 0.0, 0.0, 0.0, 0.0, z, 0.0, 0.0, 0.0, 0.0, 1.0)


def rotate(ax: float, ay: float, az: float, degrees: float) -> Matrix:
    """COLLADA <rotate>: axis, angle in degrees (right-handed)."""
    n = math.sqrt(ax * ax + ay * ay + az * az)
    if n == 0.0 or degrees == 0.0:
        return IDENTITY
    x, y, z = ax / n, ay / n, az / n
    a = math.radians(degrees)
    c, s, t = math.cos(a), math.sin(a), 1.0 - math.cos(a)
    return (t * x * x + c, t * x * y - s * z, t * x * z + s * y, 0.0,
            t * x * y + s * z, t * y * y + c, t * y * z - s * x, 0.0,
            t * x * z - s * y, t * y * z + s * x, t * z * z + c, 0.0,
            0.0, 0.0, 0.0, 1.0)


def mat_inverse_affine(m: Matrix) -> Matrix:
    """Inverse of an affine matrix (upper 3x3 invertible, last row 0 0 0 1)."""
    a, b, c, d = m[0], m[1], m[2], m[3]
    e, f, g, h = m[4], m[5], m[6], m[7]
    i, j, k, l_ = m[8], m[9], m[10], m[11]
    det = a * (f * k - g * j) - b * (e * k - g * i) + c * (e * j - f * i)
    if det == 0.0:
        raise ValueError("singular joint matrix")
    inv = 1.0 / det
    r = (
        (f * k - g * j) * inv, (c * j - b * k) * inv, (b * g - c * f) * inv,
        (g * i - e * k) * inv, (a * k - c * i) * inv, (c * e - a * g) * inv,
        (e * j - f * i) * inv, (b * i - a * j) * inv, (a * f - b * e) * inv,
    )
    tx = -(r[0] * d + r[1] * h + r[2] * l_)
    ty = -(r[3] * d + r[4] * h + r[5] * l_)
    tz = -(r[6] * d + r[7] * h + r[8] * l_)
    return (r[0], r[1], r[2], tx, r[3], r[4], r[5], ty, r[6], r[7], r[8], tz,
            0.0, 0.0, 0.0, 1.0)


# Y-up (the kit) to Z-up (Blender, and SSB64's model space is Y-up too, so
# only Blender's import applies this): x stays, y -> z, z -> -y.
Y_UP_TO_Z_UP: Matrix = (1.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, -1.0, 0.0,
                        0.0, 1.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 1.0)


@dataclass
class Joint:
    name: str                 # sid: HAL's joint name
    parent: int               # index into Model.joints, -1 for the root
    local: Matrix             # rest transform relative to the parent
    world: Matrix = IDENTITY  # rest transform in model space
    translate: tuple = (0.0, 0.0, 0.0)
    rotate_zyx: tuple = (0.0, 0.0, 0.0)   # degrees: rotateZ, rotateY, rotateX
    scale: tuple = (1.0, 1.0, 1.0)


@dataclass
class Pool:
    """A model's vertex pool: positions, normals and texture coordinates
    share one index."""
    positions: list
    normals: list
    uvs: list

    def same_as(self, other: "Pool") -> bool:
        return (self.positions == other.positions and self.normals == other.normals
                and self.uvs == other.uvs)


@dataclass
class Mesh:
    name: str                 # geometry name (body_mh_0_0, Lhand_1_VIS_O_OBJ_26_0, ...)
    material: str             # material id
    image: str | None         # its texture file name, relative to the .dae
    triangles: list           # (i0, i1, i2) into the pool
    pool: Pool
    weights: list             # per pool vertex: [(joint index, weight), ...]
    bind_shape: Matrix = IDENTITY

    def used_vertices(self) -> list[int]:
        """The pool vertices this mesh's triangles use, in first-use order."""
        seen: dict[int, None] = {}
        for tri in self.triangles:
            for index in tri:
                seen.setdefault(index, None)
        return list(seen)


@dataclass
class Model:
    path: Path
    joints: list[Joint]
    meshes: list[Mesh]
    images: dict[str, str] = field(default_factory=dict)   # image id -> file
    materials: dict[str, str | None] = field(default_factory=dict)  # material -> image file

    def joint_index(self, name: str) -> int:
        for index, joint in enumerate(self.joints):
            if joint.name == name:
                return index
        raise KeyError(name)

    def pool(self) -> Pool:
        """The shared vertex pool (every fighter model's geometry carries the
        same one; the gate's and the wings' meshes have their own)."""
        first = self.meshes[0].pool
        for mesh in self.meshes[1:]:
            if mesh.pool is not first:
                raise ValueError(f"{self.path.name}: {mesh.name} has its own pool")
        return first

    def pools(self) -> list[Pool]:
        out: list[Pool] = []
        for mesh in self.meshes:
            if all(mesh.pool is not p for p in out):
                out.append(mesh.pool)
        return out

    def neutral_meshes(self) -> list[Mesh]:
        return [mesh for mesh in self.meshes if NEUTRAL_RE.match(mesh.name)]

    def triangle_count(self, meshes: list[Mesh] | None = None) -> int:
        return sum(len(m.triangles) for m in (self.meshes if meshes is None else meshes))

    def skinned_position(self, mesh: Mesh, index: int, pose: dict[int, Matrix] | None = None) -> tuple:
        """A pool vertex placed by its skin: in the rest pose (pose None) the
        bind-pose position; with `pose` (joint index -> world matrix) the
        posed one."""
        p = mat_point(mesh.bind_shape, mesh.pool.positions[index])
        if pose is None:
            return p
        out = [0.0, 0.0, 0.0]
        for joint, weight in mesh.weights[index]:
            m = mat_mul(pose[joint], self.inverse_bind(joint))
            q = mat_point(m, p)
            out[0] += weight * q[0]
            out[1] += weight * q[1]
            out[2] += weight * q[2]
        return tuple(out)

    _inverse_bind: dict[int, Matrix] = field(default_factory=dict)

    def inverse_bind(self, joint: int) -> Matrix:
        return self._inverse_bind[joint]


def _floats(text: str | None) -> list[float]:
    return [float(t) for t in (text or "").split()]


def _ints(text: str | None) -> list[int]:
    return [int(t) for t in (text or "").split()]


def _find(elem, path: str):
    return elem.find(path.replace("c:", NS))


def _findall(elem, path: str):
    return elem.findall(path.replace("c:", NS))


def _source_values(mesh_elem, source_id: str) -> tuple[list[float], int]:
    for source in _findall(mesh_elem, "c:source"):
        if source.get("id") == source_id:
            array = _find(source, "c:float_array")
            accessor = _find(source, "c:technique_common/c:accessor")
            stride = int(accessor.get("stride", "1"))
            values = _floats(array.text)
            if len(values) != int(accessor.get("count")) * stride:
                raise ValueError(f"{source_id}: {len(values)} values for "
                                 f"{accessor.get('count')} x {stride}")
            return values, stride
    raise KeyError(source_id)


def _grouped(values: list[float], stride: int) -> list[tuple]:
    return [tuple(values[i:i + stride]) for i in range(0, len(values), stride)]


def _node_local(node) -> tuple[Matrix, tuple, tuple, tuple]:
    m = IDENTITY
    t, r, s = (0.0, 0.0, 0.0), {"rotateZ": 0.0, "rotateY": 0.0, "rotateX": 0.0}, (1.0, 1.0, 1.0)
    for child in node:
        tag = child.tag.replace(NS, "")
        if tag == "translate":
            t = tuple(_floats(child.text))
            m = mat_mul(m, translate(*t))
        elif tag == "rotate":
            ax, ay, az, deg = _floats(child.text)
            r[child.get("sid", "")] = deg
            m = mat_mul(m, rotate(ax, ay, az, deg))
        elif tag == "scale":
            s = tuple(_floats(child.text))
            m = mat_mul(m, scale(*s))
        elif tag == "matrix":
            m = mat_mul(m, tuple(_floats(child.text)))
    return m, t, (r.get("rotateZ", 0.0), r.get("rotateY", 0.0), r.get("rotateX", 0.0)), s


def read(path: Path) -> Model:
    root = ET.parse(path).getroot()
    images = {img.get("id"): (_find(img, "c:init_from").text or "").lstrip("./")
              for img in _findall(root, "c:library_images/c:image")}
    # material -> effect -> sampler -> surface -> image
    effect_image: dict[str, str | None] = {}
    for effect in _findall(root, "c:library_effects/c:effect"):
        params = {}
        for newparam in _findall(effect, ".//c:newparam"):
            surface = _find(newparam, "c:surface/c:init_from")
            sampler = _find(newparam, "c:sampler2D/c:source")
            params[newparam.get("sid")] = (surface.text if surface is not None else None,
                                           sampler.text if sampler is not None else None)
        image = None
        texture = _find(effect, ".//c:diffuse/c:texture")
        if texture is not None:
            sampler = params.get(texture.get("texture"), (None, None))[1]
            surface = params.get(sampler, (None, None))[0] if sampler else None
            image = images.get(surface or texture.get("texture"))
        effect_image["#" + effect.get("id")] = image
    materials = {}
    for material in _findall(root, "c:library_materials/c:material"):
        inst = _find(material, "c:instance_effect")
        materials[material.get("id")] = effect_image.get(inst.get("url")) if inst is not None else None

    # Skeleton: the visual scene's JOINT nodes, depth first.
    joints: list[Joint] = []

    def walk(node, parent: int, parent_world: Matrix) -> None:
        if node.get("type") == "JOINT":
            local, t, r, s = _node_local(node)
            world = mat_mul(parent_world, local)
            joints.append(Joint(node.get("sid") or node.get("name"), parent, local, world, t, r, s))
            me = len(joints) - 1
            for child in _findall(node, "c:node"):
                walk(child, me, world)

    scene = _find(root, "c:library_visual_scenes/c:visual_scene")
    for node in _findall(scene, "c:node"):
        walk(node, -1, IDENTITY)
    by_name = {j.name: i for i, j in enumerate(joints)}

    # Geometries, with one shared pool object when the arrays match.
    pools: list[Pool] = []
    geometries: dict[str, tuple[str, str, list, Pool]] = {}
    for geometry in _findall(root, "c:library_geometries/c:geometry"):
        mesh_elem = _find(geometry, "c:mesh")
        vertices = _find(mesh_elem, "c:vertices")
        sources = {inp.get("semantic"): inp.get("source").lstrip("#")
                   for inp in _findall(vertices, "c:input")}
        tris = _find(mesh_elem, "c:triangles")
        inputs = _findall(tris, "c:input")
        offsets = {inp.get("semantic"): int(inp.get("offset", "0")) for inp in inputs}
        for inp in inputs:
            if inp.get("semantic") == "TEXCOORD":
                sources["TEXCOORD"] = inp.get("source").lstrip("#")
            elif inp.get("semantic") == "NORMAL":
                sources["NORMAL"] = inp.get("source").lstrip("#")
        if set(offsets.values()) != {0}:
            raise ValueError(f"{geometry.get('name')}: inputs at several offsets {offsets}")
        positions = _grouped(*_source_values(mesh_elem, sources["POSITION"]))
        normals = _grouped(*_source_values(mesh_elem, sources["NORMAL"])) if "NORMAL" in sources else []
        uvs = _grouped(*_source_values(mesh_elem, sources["TEXCOORD"])) if "TEXCOORD" in sources else []
        pool = Pool(positions, normals, uvs)
        for known in pools:
            if known.same_as(pool):
                pool = known
                break
        else:
            pools.append(pool)
        indices = _ints(_find(tris, "c:p").text)
        count = int(tris.get("count"))
        if len(indices) != count * 3:
            raise ValueError(f"{geometry.get('name')}: {len(indices)} indices for {count} triangles")
        triangles = [tuple(indices[i:i + 3]) for i in range(0, len(indices), 3)]
        geometries[geometry.get("id")] = (geometry.get("name"), tris.get("material"), triangles, pool)

    # Controllers: one skin per geometry.
    inverse_bind: dict[int, Matrix] = {}
    skins: dict[str, tuple[str, Matrix, list]] = {}
    for controller in _findall(root, "c:library_controllers/c:controller"):
        skin = _find(controller, "c:skin")
        geo_id = skin.get("source").lstrip("#")
        bind_shape = tuple(_floats(_find(skin, "c:bind_shape_matrix").text)) or IDENTITY
        names = poses = weights = None
        for source in _findall(skin, "c:source"):
            name_array = _find(source, "c:Name_array")
            float_array = _find(source, "c:float_array")
            param = _find(source, "c:technique_common/c:accessor/c:param")
            kind = param.get("name") if param is not None else ""
            if name_array is not None:
                names = (name_array.text or "").split()
            elif kind == "TRANSFORM":
                poses = _floats(float_array.text)
            elif kind == "WEIGHT":
                weights = _floats(float_array.text)
        if names is None or poses is None or weights is None:
            raise ValueError(f"{controller.get('id')}: incomplete skin")
        joint_of = [by_name[n] for n in names]
        for k, joint in enumerate(joint_of):
            m = tuple(poses[k * 16:(k + 1) * 16])
            if joint in inverse_bind and any(abs(a - b) > 1e-4 for a, b in zip(inverse_bind[joint], m)):
                raise ValueError(f"{path.name}: joint {names[k]} has two inverse binds")
            inverse_bind[joint] = m
        vw = _find(skin, "c:vertex_weights")
        vw_inputs = {inp.get("semantic"): int(inp.get("offset")) for inp in _findall(vw, "c:input")}
        stride = max(vw_inputs.values()) + 1
        vcount = _ints(_find(vw, "c:vcount").text)
        v = _ints(_find(vw, "c:v").text)
        per_vertex = []
        at = 0
        for n in vcount:
            influences = []
            for _ in range(n):
                joint = joint_of[v[at + vw_inputs["JOINT"]]]
                weight = weights[v[at + vw_inputs["WEIGHT"]]]
                influences.append((joint, weight))
                at += stride
            per_vertex.append(influences)
        skins[geo_id] = (controller.get("id"), bind_shape, per_vertex)

    # Mesh nodes: instance_controller (or instance_geometry) with materials.
    meshes: list[Mesh] = []
    controller_geo = {cid: geo for geo, (cid, _b, _w) in skins.items()}
    for node in scene.iter(f"{NS}node"):
        inst = _find(node, "c:instance_controller")
        geo_id = None
        if inst is not None:
            geo_id = controller_geo[inst.get("url").lstrip("#")]
        else:
            inst = _find(node, "c:instance_geometry")
            if inst is not None:
                geo_id = inst.get("url").lstrip("#")
        if geo_id is None:
            continue
        bound = {im.get("symbol"): im.get("target").lstrip("#")
                 for im in inst.iter(f"{NS}instance_material")}
        name, symbol, triangles, pool = geometries[geo_id]
        material = bound.get(symbol, symbol)
        _cid, bind_shape, weights = skins.get(geo_id, (None, IDENTITY, [[] for _ in pool.positions]))
        if len(weights) != len(pool.positions):
            raise ValueError(f"{name}: {len(weights)} weights for {len(pool.positions)} vertices")
        meshes.append(Mesh(name, material, materials.get(material), triangles, pool, weights, bind_shape))

    model = Model(path, joints, meshes, images, materials)
    model._inverse_bind = inverse_bind
    return model


def summary(model: Model) -> list[str]:
    neutral = model.neutral_meshes()
    pools = model.pools()
    lines = [f"{model.path.name}: {len(model.joints)} joints, {len(model.meshes)} meshes, "
             f"{sum(len(p.positions) for p in pools)} vertices in {len(pools)} pool(s), "
             f"{model.triangle_count()} triangles in all, "
             f"{model.triangle_count(neutral)} drawn at once ({len(neutral)} neutral meshes)"]
    max_influences = max((len(w) for m in model.meshes for w in m.weights), default=0)
    lines.append(f"  influences: at most {max_influences} a vertex")
    lines.append("  joints: " + " ".join(
        f"{j.name}<{model.joints[j.parent].name if j.parent >= 0 else '-'}" for j in model.joints))
    for mesh in model.meshes:
        mark = "*" if mesh in neutral else " "
        lines.append(f"  {mark} {mesh.name}: {len(mesh.triangles)} tris, "
                     f"{len(mesh.used_vertices())} vertices, {mesh.material} ({mesh.image})")
    return lines


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("dae", type=Path, nargs="+")
    ap.add_argument("--meshes", action="store_true", help="list every mesh")
    args = ap.parse_args()
    for path in args.dae:
        lines = summary(read(path))
        print("\n".join(lines if args.meshes else lines[:2]))
    return 0


if __name__ == "__main__":
    sys.exit(main())

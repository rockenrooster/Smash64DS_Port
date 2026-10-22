#!/usr/bin/env python3
"""P6: a clip from an Ultimate motion (anim.py recipe "ultimate"; owner
2026-10-15, with Palutena's p6p review: "I also have some source files for
palutenas animations for SSB Ultimate which may help immensely").

    {"ultimate": {"fighter": "palutena", "motion": "c00attack11",
     "skeleton": "peach", "scene": "assets/p6-art/<name>/.../scene.json",
     "header": "0x...", "length": 24, "first_slot": 0, "loop": null,
     "time": [[0, 0], [4, 7], [9, 10], [24, 26]],
     "transn": false, "staff": {"slide": 61.0}}}

The motion (a .nuanmb, read with ssbh_data_py) and the skeleton it plays on
(the owner's DAE rip of the fighter: joint names, parents, rest transforms)
are the owner's local files, never committed (P6 D1: Ultimate rips stay out
of the public repo); the clip is a build output like every other. The motion
directory is $P6_ULTIMATE_ROOT/<fighter>_motion/fighter/<fighter>/motion/body/
c00 (default root: ../ssbu-recomp/build beside the repo, where the owner's
extractor writes it).

Both rigs' binds are T-poses facing +z (x her left, y up), so each mapped
row takes its bone's world rotation relative to the bone's bind, applied to
the row's own bind (W_row = W_bone B_bone^-1 B_row): the same turn of every
limb from the same rest. Her proportions are Ultimate's within a few
percent at the scale of her legs (k, ~24.9 model units an Ultimate unit),
but the trees pivot differently: Peach's torso turns about the pelvis
origin, below her hips, where Ultimate's turns at the waist. So the pelvis,
the torso, the shoulders and the neck are put where Ultimate's scaled hips,
waist, shoulders and neck are (a row's keyed translation, divided by its
translate_scale as keyposes.py writes them), the torso's move bounded
by "torso_limit", the others' by "anchor_limit"; the ankles then reach the scaled ankles by two-bone IK (knee
toward where the turn put it), the soles kept.

The staff (the fighter's "item" row) takes the item bone's pose through a
calibration of the two staffs' frames (their length and the ornament's
plane); it sits in the hand where Ultimate's does ("slide": her staff drawn
down through the hand, her grip nearer the gem) and leaves the hand as
Ultimate's does (the up tilt's and neutral air's floating staff). The
shield (a sprite on the forearm) turns with the forearm twisted about its
length so its face follows Ultimate's Shield bone, the hand kept.

"time" maps the clip's frames onto the motion's (piecewise linear between
[frame, motion frame] anchors: Ultimate's hit frames onto the script's);
"key_step": N keys every N frames (the frames between interpolated), so a
dense clip fits its fighter's figatree heap. "transn" writes the motion's
Trans travel (scaled; or a "travel" curve, or a clip's slot) into the TransN slot
("first_slot": 1, AttackDash); otherwise the Trans bone is held at rest and
the clip plays in place (the walks' and run's travel is the game's).

"raise_max", "split_max" and "straighten" shape a leg after the turns
(_legs: a thigh held to at most a raise from the pelvis's down or an angle
from the other thigh, a knee straightened);
"dress" ({"slit_free": ..., "cloth_follow": ..., "slit_turn": ...}) goes
into the skirt guide (skirt_keys.py: a kick that leaves the dress at the
slit).
"""
from __future__ import annotations

import json
import math
import os
import xml.etree.ElementTree as ET
from pathlib import Path

import numpy as np

import anim as A
import keyposes as K
import pose_ops as P

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
NS = "{http://www.collada.org/2005/11/COLLADASchema}"

FIGHTERS = {
    # Ike (owner p6p, 2026-10-15: his Ultimate motions extracted beside
    # hers) on Roy's tree, whose bind is not a T-pose: his arms hang and his
    # legs stand apart, so "align" turns each limb row's bind onto Ultimate's
    # bind direction first (W_row = W_bone B_bone^-1 A_row B_row, A_row the
    # least turn from the row's bind segment to the bone's). His skeleton is
    # the game's own .nusktb (no DAE rip of him), extracted beside the motions.
    "ike": {
        "skel": "ike_motion/fighter/ike/model/body/c00/model.nusktb",
        "map": {1: "Hip", 2: "Bust", 3: "ClavicleL", 4: "ShoulderL", 5: "ArmL", 6: "HandL",
                8: "Neck", 9: "Head", 10: "ClavicleR", 11: "ShoulderR", 12: "ArmR", 13: "HandR",
                15: "S_MantleC1", 16: "S_MantleC2", 17: "S_MantleC3", 18: "S_MantleC4",
                22: "LegL", 23: "KneeL", 24: "FootL", 27: "LegR", 28: "KneeR", 29: "FootR"},
        "align": {4: 5, 5: 6, 6: None, 11: 12, 12: 13, 13: None, 16: 17, 17: 18, 18: None,
                  22: 23, 23: 24, 24: None, 27: 28, 28: 29, 29: None},
        "pelvis": {"row": 1, "bone": "Hip", "legs": [21, 26]},
        "torso": {"row": 2, "bone": "Waist"},
        "anchors": [{"row": 3, "bone": "ShoulderL"}, {"row": 10, "bone": "ShoulderR"},
                    {"row": 8, "bone": "Neck"}],
        "legs": [[22, 23, 24, "FootL"], [27, 28, 29, "FootR"]],
        # Ragnell: row 14 (grip -> tip along local z, the blade's face local
        # y); Ultimate's sword model hangs from the Sword bone, its blade
        # along +y and its face x.
        "item": {"row": 14, "hand": 13, "bone": "Sword", "have": "HaveR",
                 "axis": [0.0, 0.0, 1.0], "normal": [0.0, 1.0, 0.0],
                 "bone_axis": [0.0, 1.0, 0.0], "bone_normal": [1.0, 0.0, 0.0], "slide": 0.0,
                 # The sword model's own motion, where a motion has one (Aether's
                 # throw, the taunt): its HaveR (the blade's root) on the Sword bone.
                 "model": "sword", "model_bone": "HaveR"},
    },
    "palutena": {
        "dae": "decomp/alt_assets/Fighters/Super Smash Bros Ultimate/Fighters/Palutena/palutena.dae",
        # Her smash wings' own model (p6_wings.py: the kit's godwing is the same rig).
        "godwing_dae": "decomp/alt_assets/Fighters/Super Smash Bros Ultimate/Fighters/Palutena/godwing.dae",
        # Our row -> its Ultimate bone (world rotation from the bind).
        "map": {1: "Hip", 2: "Bust", 3: "ClavicleL", 4: "ShoulderL", 5: "ArmL", 6: "HandL",
                7: "Neck", 8: "Head", 10: "ClavicleR", 11: "ShoulderR", 12: "ArmR", 13: "HandR",
                16: "LegL", 17: "KneeL", 18: "FootL", 21: "LegR", 22: "KneeR", 23: "FootR"},
        "pelvis": {"row": 1, "bone": "Hip", "legs": [15, 20]},
        "torso": {"row": 2, "bone": "Waist"},
        "anchors": [{"row": 3, "bone": "ShoulderL"}, {"row": 10, "bone": "ShoulderR"},
                    {"row": 7, "bone": "Neck"}],
        "legs": [[16, 17, 18, "FootL"], [21, 22, 23, "FootR"]],
        # The staff: row 14 (local grip -> gem, the ornament's normal); the
        # Stick bone's (+y to the gem, ornament normal x); the hand's Have.
        "item": {"row": 14, "hand": 13, "bone": "Stick", "have": "HaveR",
                 "axis": [0.103, 0.681, -0.725], "normal": [0.934, -0.316, -0.164],
                 "bone_axis": [0.0, 1.0, 0.0], "bone_normal": [1.0, 0.0, 0.0], "slide": 61.0},
        # The shield sprite on the forearm (row 5, its face local +z) and
        # Ultimate's Shield bone (face local -z: the ornamented side).
        "shield": {"row": 5, "hand": 6, "bone": "Shield", "normal": [0.0, 0.0, 1.0],
                   "bone_normal": [0.0, 0.0, -1.0]},
        # Her hair panels' row (look_blend.py: the head's child) and Ultimate's
        # two back hair chains (root, end) with the head they hang from.
        "hair": {"row": 9, "head": "Head", "chains": [["S_HairBL1", "S_HairBL5_null"],
                                                      ["S_HairBR1", "S_HairBR5_null"]]},
        # The skirt's eight chains' ends (the hem; skirt_keys.py's guide) and
        # their third joints (about her knees: the dress's mid ring, owner
        # p6s).
        "skirt": [f"S_Skirt{d}4_null" for d in ("F", "FL", "L", "BL", "B", "BR", "R", "FR")],
        "skirt_mid": [f"S_Skirt{d}3" for d in ("F", "FL", "L", "BL", "B", "BR", "R", "FR")],
        # Where the skirt's moves are measured from: her standing wait.
        "skirt_rest": ["a00wait1", 0],
        # The held joint (row 28: the grab's and heavy items' hold, look_skirt.py
        # "joint_roles") where Ultimate's Throw bone is, in a recipe that asks
        # ("held": the catch, the throws, heavy items).
        "held": {"row": 28, "bone": "Throw"},
    },
}
DEFAULT_ROOT = REPO.parent / "ssbu-recomp" / "build"
# The hair card's angle from hanging (degrees) its large angles ease toward.
HAIR_SOFT = 60.0


class UltimateError(SystemExit):
    pass


def motion_dir(fighter: str, model: str = "body") -> Path:
    root = Path(os.environ.get("P6_ULTIMATE_ROOT", DEFAULT_ROOT))
    return root / f"{fighter}_motion" / "fighter" / fighter / "motion" / model / "c00"


# -- the Ultimate side --------------------------------------------------------

def load_skeleton(dae: Path) -> list:
    """The DAE's joints in document order: name, parent index, rest local
    and world matrices (each joint's <matrix>/<translate>/<rotate>/<scale>
    composed in order)."""
    root = ET.parse(dae).getroot()
    joints = []

    def walk(node, parent):
        idx = parent
        if node.get("type") == "JOINT":
            m = np.eye(4)
            for el in node:
                tag = el.tag.split("}")[1]
                vals = [float(x) for x in (el.text or "").split()]
                if tag == "matrix":
                    m = m @ np.array(vals).reshape(4, 4)
                elif tag == "translate":
                    t = np.eye(4); t[:3, 3] = vals; m = m @ t
                elif tag == "rotate":
                    ax = np.array(vals[:3]); n = np.linalg.norm(ax)
                    if n > 1e-12:
                        r = np.eye(4); r[:3, :3] = _axis_angle(ax / n, math.radians(vals[3])); m = m @ r
                elif tag == "scale":
                    s = np.eye(4); s[0, 0], s[1, 1], s[2, 2] = vals; m = m @ s
            joints.append({"name": node.get("name") or node.get("id"), "parent": parent, "local": m})
            idx = len(joints) - 1
        for ch in node.findall(NS + "node"):
            walk(ch, idx)
    for vs in root.iter(NS + "visual_scene"):
        for n in vs.findall(NS + "node"):
            walk(n, -1)
    for j in joints:
        p = j["parent"]
        j["world"] = j["local"] if p < 0 else joints[p]["world"] @ j["local"]
    return joints


def load_nusktb(path: Path) -> list:
    """An SSBH skeleton's joints as load_skeleton gives them: name, parent,
    rest local (the file's row-major transform transposed) and world (each
    after its parent, whatever the file's order)."""
    import ssbh_data_py
    sk = ssbh_data_py.skel_data.read_skel(str(path))
    joints = [{"name": b.name, "parent": -1 if b.parent_index is None else int(b.parent_index),
               "local": np.array(b.transform, dtype=float).T} for b in sk.bones]

    def world(i):
        j = joints[i]
        if "world" not in j:
            j["world"] = j["local"] if j["parent"] < 0 else world(j["parent"]) @ j["local"]
        return j["world"]
    for i in range(len(joints)):
        world(i)
    return joints


def _order(joints: list) -> list:
    """Joint indices with every parent before its children."""
    done, out = set(), []

    def visit(i):
        if i in done:
            return
        p = joints[i]["parent"]
        if p >= 0:
            visit(p)
        done.add(i)
        out.append(i)
    for i in range(len(joints)):
        visit(i)
    return out


def _axis_angle(a, th):
    k = np.array([[0, -a[2], a[1]], [a[2], 0, -a[0]], [-a[1], a[0], 0]])
    return np.eye(3) + math.sin(th) * k + (1 - math.cos(th)) * (k @ k)


def _quat_mat(q):
    x, y, z, w = (float(v) for v in q)
    n = x * x + y * y + z * z + w * w
    if n < 1e-12:
        return np.eye(3)
    s = 2.0 / n
    return np.array([[1 - s * (y * y + z * z), s * (x * y - z * w), s * (x * z + y * w)],
                     [s * (x * y + z * w), 1 - s * (x * x + z * z), s * (y * z - x * w)],
                     [s * (x * z - y * w), s * (y * z + x * w), 1 - s * (x * x + y * y)]])


def _slerp(q0, q1, t):
    q0 = np.asarray(q0, float); q1 = np.asarray(q1, float)
    d = float(np.dot(q0, q1))
    if d < 0:
        q1, d = -q1, -d
    if d > 0.9995:
        q = q0 + t * (q1 - q0)
        return q / np.linalg.norm(q)
    th = math.acos(d)
    return (math.sin((1 - t) * th) * q0 + math.sin(t * th) * q1) / math.sin(th)


def _rot(m):
    """The rotation of a (possibly scaled) matrix."""
    u, _s, vt = np.linalg.svd(np.asarray(m, float)[:3, :3])
    r = u @ vt
    if np.linalg.det(r) < 0:
        u[:, -1] *= -1
        r = u @ vt
    return r


_SKELETONS: dict = {}


class ItemMotion:
    """An item model's motion (motion/<model>/c00/<motion>.nuanmb): one
    bone's local transform at fractional frames (the model's root sits on
    the body's item bone; its bone carries the mesh)."""

    def __init__(self, fighter: str, model: str, motion: str, bone: str):
        import ssbh_data_py
        path = motion_dir(fighter, model) / f"{motion}.nuanmb"
        self.vals = None
        if path.exists():
            anim = ssbh_data_py.anim_data.read_anim(str(path))
            for g in anim.groups:
                if "Transform" not in str(g.group_type):
                    continue
                for node in g.nodes:
                    if node.name == bone:
                        self.vals = node.tracks[0].values

    def local(self, u):
        if not self.vals:
            return np.eye(4)
        n = len(self.vals)
        u = min(max(float(u), 0.0), n - 1.0)
        f0 = int(math.floor(u)); f1 = min(f0 + 1, n - 1); t = u - f0
        a, b = self.vals[f0], self.vals[f1]
        m = np.eye(4)
        sc = (1 - t) * np.asarray(a.scale, float) + t * np.asarray(b.scale, float)
        m[:3, :3] = _quat_mat(_slerp(a.rotation, b.rotation, t)) @ np.diag(sc)
        m[:3, 3] = (1 - t) * np.asarray(a.translation, float) + t * np.asarray(b.translation, float)
        return m


class Source:
    """An Ultimate motion on its skeleton, sampled at fractional frames."""

    def __init__(self, fighter: str, motion: str, keep_trans: bool = False, model: str = "body"):
        cfg = FIGHTERS[fighter]
        if model == "body" and "skel" in cfg:
            dae = Path(os.environ.get("P6_ULTIMATE_ROOT", DEFAULT_ROOT)) / cfg["skel"]
        else:
            dae = REPO / cfg["dae" if model == "body" else f"{model}_dae"]
        path = motion_dir(fighter, model) / f"{motion}.nuanmb"
        if not dae.exists() or not path.exists():
            raise UltimateError(f"ultimate: {fighter} {motion} needs the owner's local Ultimate files "
                                f"({dae if not dae.exists() else path}); P6 D1 keeps them out of the repo")
        try:
            import ssbh_data_py
        except ImportError as e:
            raise UltimateError("ultimate: needs ssbh_data_py (pip install ssbh_data_py)") from e
        if dae not in _SKELETONS:
            _SKELETONS[dae] = load_nusktb(dae) if dae.suffix == ".nusktb" else load_skeleton(dae)
        self.skel = _SKELETONS[dae]
        self.order = _order(self.skel)
        self.idx = {j["name"]: i for i, j in enumerate(self.skel)}
        anim = ssbh_data_py.anim_data.read_anim(str(path))
        self.frames = int(round(anim.final_frame_index)) + 1
        self.tracks = {}
        for g in anim.groups:
            if "Transform" not in str(g.group_type):
                continue
            for node in g.nodes:
                for tr in node.tracks:
                    self.tracks[node.name] = tr.values
        self.keep_trans = keep_trans
        self.fighter = fighter
        it = cfg.get("item") or {}
        self.item = (ItemMotion(fighter, it["model"], motion, it["model_bone"])
                     if model == "body" and it.get("model") else None)
        self.bind = [_rot(j["world"]) for j in self.skel]
        self.bind_pos = [np.asarray(j["world"], float)[:3, 3].copy() for j in self.skel]

    def _local(self, name, u):
        vals = self.tracks.get(name)
        if vals is None:
            return self.skel[self.idx[name]]["local"]
        n = len(vals)
        u = min(max(float(u), 0.0), n - 1.0)
        f0 = int(math.floor(u)); f1 = min(f0 + 1, n - 1); t = u - f0
        a, b = vals[f0], vals[f1]
        m = np.eye(4)
        sc = (1 - t) * np.asarray(a.scale, float) + t * np.asarray(b.scale, float)
        m[:3, :3] = _quat_mat(_slerp(a.rotation, b.rotation, t)) @ np.diag(sc)
        m[:3, 3] = (1 - t) * np.asarray(a.translation, float) + t * np.asarray(b.translation, float)
        return m

    def worlds(self, u):
        """World matrices of every joint at motion frame u (the Trans bone
        held at rest unless kept)."""
        out = [None] * len(self.skel)
        for k in self.order:
            j = self.skel[k]
            if j["name"] == "Trans" and not self.keep_trans:
                m = j["local"]
            else:
                m = self._local(j["name"], u)
            p = j["parent"]
            out[k] = m if p < 0 else out[p] @ m
        return out

    def bind_world(self, name):
        return self.skel[self.idx[name]]["world"]

    def trans_z(self, u):
        return float(self._local("Trans", u)[2, 3])


# -- our side -----------------------------------------------------------------

def time_map(anchors, g: float) -> float:
    pts = sorted((float(a), float(b)) for a, b in anchors)
    if g <= pts[0][0]:
        return pts[0][1] + (g - pts[0][0])
    for (a0, b0), (a1, b1) in zip(pts, pts[1:]):
        if g <= a1:
            return b0 + (b1 - b0) * (g - a0) / max(a1 - a0, 1e-9)
    return pts[-1][1] + (g - pts[-1][0])


def _frame(axis, normal):
    a = np.asarray(axis, float); a = a / np.linalg.norm(a)
    n = np.asarray(normal, float); n = n - np.dot(n, a) * a; n = n / np.linalg.norm(n)
    return np.column_stack([a, n, np.cross(a, n)])


def _twist_about(w, axis, have, want):
    """w turned about `axis` (a model direction) so `have` (a direction w
    carries) lies toward `want` in the plane across the axis."""
    ax = np.asarray(axis, float); ax = ax / np.linalg.norm(ax)
    h = have - np.dot(have, ax) * ax
    m = want - np.dot(want, ax) * ax
    if np.linalg.norm(h) < 1e-6 or np.linalg.norm(m) < 1e-6:
        return w
    h = h / np.linalg.norm(h); m = m / np.linalg.norm(m)
    th = math.atan2(float(np.dot(np.cross(h, m), ax)), float(np.dot(h, m)))
    return _axis_angle(ax, th) @ w


class Retarget:
    def __init__(self, skel: A.Skeleton, scales, src: Source, cfg: dict, spec: dict):
        self.skel, self.scales, self.src, self.cfg, self.spec = skel, scales, src, cfg, spec
        self.rows = len(skel.parents)
        self.bind_local = [A.euler(*r) for r in skel.rotate]
        self.bind, self.bind_pos = K._worlds(skel, self.bind_local, [list(t) for t in skel.translate])
        i = src.idx
        bones = set(cfg["map"].values()) | {cfg["item"]["bone"]}
        if cfg.get("shield"):
            bones.add(cfg["shield"]["bone"])
        sb = {n: _rot(src.bind_world(n)) for n in bones}
        self.src_bind_inv = {n: m.T for n, m in sb.items()}
        self.src_bind_pos = {n: np.asarray(src.bind_world(n), float)[:3, 3] for n in src.idx}
        a_, b_, c_, foot = cfg["legs"][0]
        ours = (np.linalg.norm(self.bind_pos[b_] - self.bind_pos[a_])
                + np.linalg.norm(self.bind_pos[c_] - self.bind_pos[b_]))
        sp = self.src_bind_pos
        theirs = (np.linalg.norm(sp[cfg["map"][b_]] - sp[cfg["map"][a_]])
                  + np.linalg.norm(sp[foot] - sp[cfg["map"][b_]]))
        self.k = float(spec.get("k", ours / theirs))
        # A rig whose bind is not a T-pose: each "align" row's bind turned
        # onto Ultimate's bind direction (its segment to the child named, or
        # its parent's turn), the binds the retarget then works from.
        self.aligned = list(self.bind)
        turns = {}
        for r in sorted(cfg.get("align", {}), key=lambda r: K._depth(skel, r)):
            c = cfg["align"][r]
            if c is None:
                turns[r] = turns.get(skel.parents[r], np.eye(3))
            else:
                d_o = self.bind_pos[c] - self.bind_pos[r]
                d_u = sp[cfg["map"][c]] - sp[cfg["map"][r]]
                turns[r] = K._turn_between(d_o, d_u)
            self.aligned[r] = turns[r] @ self.bind[r]
        it = cfg["item"]
        ours_f = _frame(self.bind[it["row"]] @ np.asarray(it["axis"]), self.bind[it["row"]] @ np.asarray(it["normal"]))
        sgn = -1.0 if (spec.get("staff") or {}).get("flip") else 1.0
        theirs_f = _frame(sb[it["bone"]] @ np.asarray(it["bone_axis"]),
                          sb[it["bone"]] @ (sgn * np.asarray(it["bone_normal"])))
        # W_item = W_bone C (C: the bone's bind -> our staff's bind, carried).
        self.item_cal = sb[it["bone"]].T @ (theirs_f @ ours_f.T) @ self.bind[it["row"]]
        # The staff's point the hand holds (bone local), from the bind.
        hb = np.asarray(src.bind_world(it["have"]), float)[:3, 3]
        self.item_grip = sb[it["bone"]].T @ (hb - self.src_bind_pos[it["bone"]])
        st = spec.get("staff") or {}
        self.slide = float(st.get("slide", it.get("slide", 0.0)))
        self.float_scale = float(st.get("float", 1.0))
        pel = cfg["pelvis"]
        self.hip_anchor = 0.5 * (self.bind_pos[pel["legs"][0]] + self.bind_pos[pel["legs"][1]])
        self.limit = float(spec.get("anchor_limit", 25.0))
        self.torso_limit = float(spec.get("torso_limit", 150.0))

    def scaled(self, P, name, base_name, base_ours):
        """Bone `name`'s position relative to `base_name` (Ultimate), scaled,
        from our point base_ours."""
        return base_ours + (P[name] - P[base_name]) * self.k

    def pose(self, u, src=None, yaw=0.0):
        """Our rows' local rotations and translations at motion frame u of
        src (default: the calibration's), the motion turned `yaw` degrees
        about the vertical through her origin first."""
        src = src or self.src
        cfg, skel = self.cfg, self.skel
        W = src.worlds(u)
        if yaw:
            Y = np.eye(4)
            Y[:3, :3] = _axis_angle(np.array([0.0, 1.0, 0.0]), math.radians(yaw))
            W = [Y @ w for w in W]
        i = src.idx
        it = cfg["item"]
        if src.item is not None:
            # The item model's bone on the body's item bone (the sword thrown).
            W = list(W)
            W[i[it["bone"]]] = W[i[it["bone"]]] @ src.item.local(u)
        P = {n: np.asarray(W[i[n]], float)[:3, 3] for n in src.idx}
        R = {n: _rot(W[i[n]]) for n in self.src_bind_inv}
        world = [None] * self.rows
        for r, b in cfg["map"].items():
            world[r] = R[b] @ self.src_bind_inv[b] @ self.aligned[r]
        it = cfg["item"]
        world[it["row"]] = R[it["bone"]] @ self.item_cal
        trans = [list(map(float, t)) for t in skel.translate]
        rots = [None] * self.rows
        pos = [None] * self.rows
        local = [None] * self.rows

        def settle(r):
            p = skel.parents[r]
            pw = rots[p] if p >= 0 else np.eye(3)
            pp = pos[p] if p >= 0 else np.zeros(3)
            if world[r] is None:
                local[r] = self.bind_local[r]
                rots[r] = pw @ local[r]
            else:
                local[r] = pw.T @ world[r]
                rots[r] = world[r]
            pos[r] = pp + pw @ np.asarray(trans[r], float)

        def place(r, point_bind, target, limit):
            """Row r's translation putting its point (a bind model position)
            at target, moved at most `limit`."""
            p = skel.parents[r]
            pw = rots[p] if p >= 0 else np.eye(3)
            pp = pos[p] if p >= 0 else np.zeros(3)
            off = self.bind[r].T @ (np.asarray(point_bind) - self.bind_pos[r])
            want = target - world_of(r) @ off
            cur = pp + pw @ np.asarray(trans[r], float)
            d = want - cur
            n = float(np.linalg.norm(d))
            if n > limit:
                d *= limit / n
            trans[r] = [float(v) for v in pw.T @ (cur + d - pp)]

        def world_of(r):
            if world[r] is not None:
                return world[r]
            p = skel.parents[r]
            return (rots[p] if p >= 0 else np.eye(3)) @ self.bind_local[r]

        # The hips: our hip joints' midpoint where Ultimate's scaled Hip is.
        pel = cfg["pelvis"]
        hip_target = self.hip_anchor + (P[pel["bone"]] - self.src_bind_pos[pel["bone"]]) * self.k
        torso = cfg["torso"]
        waist_bind = self.hip_anchor + (self.src_bind_pos[torso["bone"]] - self.src_bind_pos[pel["bone"]]) * self.k
        waist_target = hip_target + (P[torso["bone"]] - P[pel["bone"]]) * self.k
        anchors = {a["row"]: a for a in cfg.get("anchors", ())}
        for r in range(self.rows):
            if r == pel["row"]:
                p = skel.parents[r]
                pw = rots[p] if p >= 0 else np.eye(3)
                pp = pos[p] if p >= 0 else np.zeros(3)
                off = self.bind[r].T @ (self.hip_anchor - self.bind_pos[r])
                want = hip_target - world[r] @ off
                trans[r] = [float(v) for v in pw.T @ (want - pp)]
            elif r == torso["row"]:
                place(r, waist_bind, waist_target, self.torso_limit)
            elif r in anchors and not self.spec.get("no_anchors"):
                # The row's joint follows Ultimate's bone about the waist (a
                # shrug, the neck drawn in), from our own rest: the rests'
                # difference turns with the torso.
                b = anchors[r]["bone"]
                tr_ = torso["row"]
                mis = (self.bind_pos[r] - waist_bind) - (self.src_bind_pos[b] - self.src_bind_pos[torso["bone"]]) * self.k
                turn = rots[tr_] @ self.bind[tr_].T
                place(r, self.bind_pos[r], waist_target + (P[b] - P[torso["bone"]]) * self.k + turn @ mis,
                      self.limit)
            settle(r)
        # The shield's face: the forearm twisted about its length, the hand kept.
        sh = cfg.get("shield")
        if sh and not self.spec.get("no_shield_twist"):
            fr = sh["row"]
            axis = rots[fr] @ (self.bind[fr].T @ (self.bind_pos[sh["hand"]] - self.bind_pos[fr]))
            have = rots[fr] @ np.asarray(sh["normal"], float)
            want = R[sh["bone"]] @ np.asarray(sh["bone_normal"], float)
            hand_w = rots[sh["hand"]]
            world[fr] = _twist_about(rots[fr], axis, have, want)
            world[sh["hand"]] = hand_w
            for r in range(fr, self.rows):
                if r == fr or self._below(r, fr):
                    settle(r)
        # The staff in the hand: Ultimate's grip, drawn down by the slide,
        # leaving the hand as Ultimate's leaves its Have.
        ir, hr = it["row"], it["hand"]
        a = rots[ir] @ (np.asarray(it["axis"], float) / np.linalg.norm(it["axis"]))
        grip_pt = P[it["bone"]] + R[it["bone"]] @ self.item_grip
        floating = (P[it["have"]] - grip_pt) * self.k * self.float_scale
        hand_pt = pos[hr] + rots[hr] @ np.asarray(skel.translate[ir], float)
        origin = hand_pt - floating - self.slide * a
        trans[ir] = [float(v) for v in rots[hr].T @ (origin - pos[hr])]
        settle(ir)
        # The hair hangs along Ultimate's back hair chains (their mean from
        # root to end), turned about it with the head.
        hc = cfg.get("hair")
        if hc and not self.spec.get("no_hair"):
            hr = hc["row"]
            world[hr] = self._hair_frame(W, src) @ self._hair_bind_inv @ self.bind[hr]
            # Ultimate's long hair flies out as she drops; the panels are one
            # stiff card, which then stands out from her head like a sheet
            # (owner p6t, down air: "head/chest/arm areas seem out of
            # sorts"): its angle from hanging eased toward HAIR_SOFT
            # (a -> S * (1 - exp(-a / S))).
            hang = world[hr] @ self.bind[hr].T @ np.array([0.0, -1.0, 0.0])
            down = np.array([0.0, -1.0, 0.0])
            a = float(np.arccos(np.clip(np.dot(hang, down) / max(np.linalg.norm(hang), 1e-9), -1.0, 1.0)))
            if a > 1e-3:
                soft = math.radians(HAIR_SOFT)
                ax = np.cross(hang, down)
                ax = ax / max(np.linalg.norm(ax), 1e-9)
                world[hr] = _axis_angle(ax, a - soft * (1.0 - math.exp(-a / soft))) @ world[hr]
            settle(hr)
        # The feet where Ultimate's scaled ankles are, the soles kept.
        if self.spec.get("plant", True):
            for a_, b_, c_, foot in cfg["legs"]:
                tgt = hip_target + (P[foot] - P[pel["bone"]]) * self.k \
                    + (self.bind_pos[c_] - self.hip_anchor - (self.src_bind_pos[foot] - self.src_bind_pos[pel["bone"]]) * self.k)
                foot_w = rots[c_]
                rr, pp = K._worlds(skel, local, trans)
                pole = pp[b_] - pp[a_]
                d = tgt - pp[a_]
                pole = pole - np.dot(pole, d) / max(float(np.dot(d, d)), 1e-9) * d
                if np.linalg.norm(pole) < 1e-6:
                    pole = rr[a_] @ np.array([0.0, 0.0, 1.0])
                K._ik(skel, local, trans, [a_, b_, c_], tgt, pole, [1.0, 0.0, 0.0])
                rr, pp = K._worlds(skel, local, trans)
                local[c_] = rr[b_].T @ foot_w
        # The held joint where Ultimate's Throw bone is (scaled about the
        # hips as her body is): the fighter she holds, or the crate.
        hd = cfg.get("held")
        if hd and self.spec.get("held") and hd["row"] < self.rows:
            hr = hd["row"]
            target = hip_target + (P[hd["bone"]] - P[pel["bone"]]) * self.k
            rr, pp = K._worlds(skel, local, trans)
            par = skel.parents[hr]
            pw_r = rr[par] if par >= 0 else np.eye(3)
            pw_p = pp[par] if par >= 0 else np.zeros(3)
            trans[hr] = [float(v) for v in pw_r.T @ (target - pw_p)]
        return local, trans

    def _hair_frame_of(self, pos, head_rot):
        hc = self.cfg["hair"]
        roots = np.mean([pos(a) for a, _b in hc["chains"]], axis=0)
        ends = np.mean([pos(b) for _a, b in hc["chains"]], axis=0)
        return _frame(ends - roots, head_rot @ np.array([1.0, 0.0, 0.0]))

    def _hair_frame(self, W, src):
        i = src.idx
        head = self.cfg["hair"]["head"]
        H = _rot(W[i[head]]) @ _rot(src.bind_world(head)).T
        return self._hair_frame_of(lambda n: np.asarray(W[i[n]], float)[:3, 3], H)

    @property
    def _hair_bind_inv(self):
        if not hasattr(self, "_hbi"):
            self._hbi = self._hair_frame_of(lambda n: self.src_bind_pos[n], np.eye(3)).T
        return self._hbi

    def _skirt_points(self, u, src, yaw, names):
        """The named skirt joints in the hips' frame (our pelvis's: both
        binds face +z), Ultimate's units, at frame u."""
        W = src.worlds(u)
        if yaw:
            Y = np.eye(4)
            Y[:3, :3] = _axis_angle(np.array([0.0, 1.0, 0.0]), math.radians(yaw))
            W = [Y @ w for w in W]
        i = src.idx
        hip = self.cfg["pelvis"]["bone"]
        H = _rot(W[i[hip]]) @ _rot(src.bind_world(hip)).T
        ph = np.asarray(W[i[hip]], float)[:3, 3]
        return [H.T @ (np.asarray(W[i[n]], float)[:3, 3] - ph) for n in names]

    def skirt_moves(self, u, src, yaw=0.0, names=None):
        """Ultimate's hem points' (or the named skirt joints') moves in the
        hips' frame, in our units, at frame u: from where her standing wait
        has them ("skirt_rest", owner p6t: the dress in its natural pose is
        the look's A-line, and Ultimate's own skirt hangs wider at the knees
        standing than in its bind), or from the bind."""
        names = names or self.cfg["skirt"]
        now = self._skirt_points(u, src, yaw, names)
        sr = self.cfg.get("skirt_rest")
        if sr:
            key = (sr[0], float(sr[1]), tuple(names))
            cache = self.__dict__.setdefault("_skirt_rest", {})
            if key not in cache:
                cache[key] = self._skirt_points(float(sr[1]), Source(src.fighter, sr[0]), 0.0, names)
            rest = cache[key]
        else:
            hip = self.cfg["pelvis"]["bone"]
            rest = [self.src_bind_pos[n] - self.src_bind_pos[hip] for n in names]
        return [[float(v) for v in (a - b) * self.k] for a, b in zip(now, rest)]

    def skirt_azimuths(self, names=None):
        pts = np.array([self.src_bind_pos[n] for n in (names or self.cfg["skirt"])])
        c = pts.mean(axis=0)
        return [float(a) for a in np.arctan2(pts[:, 0] - c[0], pts[:, 2] - c[2])]

    def _below(self, r, top):
        while r >= 0:
            if r == top:
                return True
            r = self.skel.parents[r]
        return False


def _keyed(keys, g: float, default: float = 0.0) -> float:
    """A piecewise linear [[frame, value], ...] at frame g (held past its ends)."""
    if not keys:
        return default
    pts = sorted((float(a), float(b)) for a, b in keys)
    if g <= pts[0][0]:
        return pts[0][1]
    for (a0, b0), (a1, b1) in zip(pts, pts[1:]):
        if g <= a1:
            t = (g - a0) / max(a1 - a0, 1e-9)
            t = t * t * (3.0 - 2.0 * t)
            return b0 + (b1 - b0) * t
    return pts[-1][1]


def _parts(spec: dict, length: int) -> list:
    """The recipe's motion parts: each {"motion", "time"} covers our frames
    from its first anchor to its last ("motion" + "time" make one part)."""
    if spec.get("parts"):
        return [{"motion": p["motion"], "time": p["time"]} for p in spec["parts"]]
    return [{"motion": spec["motion"], "time": spec.get("time") or [[0, 0], [length, length]]}]


def _part_at(parts: list, g: int) -> dict:
    for p in parts:
        if g <= max(a for a, _b in p["time"]):
            return p
    return parts[-1]


def _window(n: int, g: int, half: int, loop) -> list:
    """The frames of a centred window about g: wrapped through a loop's
    period (frames loop..n-1), clamped otherwise."""
    if loop is not None and 0 <= loop < n - 1:
        period = n - 1 - loop
        out = []
        for d in range(-half, half + 1):
            k = g + d
            if k > n - 1:
                k = loop + (k - loop) % period
            elif k < loop:
                k = (n - 1) - (loop - k) % period if g >= loop else max(k, 0)
            out.append(k)
        return out
    return [min(max(g + d, 0), n - 1) for d in range(-half, half + 1)]


def _mean_rot(mats) -> np.ndarray:
    qs = [P._quat(m) for m in mats]
    ref = qs[0]
    acc = sum((q if np.dot(q, ref) >= 0 else -q) for q in qs)
    return P._mat(acc / max(np.linalg.norm(acc), 1e-9))


def _still(st: dict, per_rot: list, per_tr: list, length: int, loop) -> None:
    """Rows held near their mean pose ("still": owner p6r, the floating run's
    "legs ... look twitchy should be smooth and feet hardly need to move"):
    each row's rotation a centred average over "window" frames (wrapped
    through a loop), then drawn toward the clip's mean by all but "keep" of
    its swing; translations alike."""
    n = length + 1
    half = int(st.get("window", 9)) // 2
    keep = float(st.get("keep", 0.25))
    for r in st["rows"]:
        rots = [per_rot[g][r] for g in range(n)]
        trs = [np.asarray(per_tr[g][r], float) for g in range(n)]
        mean_r, mean_t = _mean_rot(rots), np.mean(trs, axis=0)
        for g in range(n):
            w = _window(n, g, half, loop)
            per_rot[g][r] = P.slerp(mean_r, _mean_rot([rots[k] for k in w]), keep)
            per_tr[g][r] = [float(v) for v in mean_t + (np.mean([trs[k] for k in w], axis=0) - mean_t) * keep]


def _turns(skel: A.Skeleton, turns: list, per_rot: list, per_tr: list, length: int) -> None:
    """Rows turned toward her facing or away ("turn": [{"row": r, "keys":
    [[frame, degrees], ...]}, ...], linear between keys; + leans the row's
    top forward, about the model's x; a row's turn carries its children, so a
    later row's adds to it): owner p6s, her up tilt "leaning back far, facing
    upwards ... like the limbo" -- Ultimate's backbend eased. "axis": "up"
    twists the row about its own up as posed (its lean kept; + turns its
    front toward the model's +x), "y" about the model's vertical: owner p6v,
    her down air's hips and torso turned toward each other."""
    bind = None
    for t in turns:
        r = int(t["row"])
        about = t.get("axis", "x")
        if about == "up" and bind is None:
            bind, _bp = K._worlds(skel, [A.euler(*e) for e in skel.rotate], [list(v) for v in skel.translate])
        for g in range(length + 1):
            deg = _keyed(t["keys"], g)
            if not deg:
                continue
            rr, _pp = K._worlds(skel, per_rot[g], per_tr[g])
            if about == "up":
                ax = rr[r] @ bind[r].T @ np.array([0.0, 1.0, 0.0])
            else:
                ax = np.array({"x": [1.0, 0.0, 0.0], "y": [0.0, 1.0, 0.0]}[about])
            turn = _axis_angle(ax / np.linalg.norm(ax), math.radians(deg))
            parent = skel.parents[r]
            pw = rr[parent] if parent >= 0 else np.eye(3)
            per_rot[g][r] = pw.T @ (turn @ rr[r])


def _legs(skel: A.Skeleton, spec: dict, per_rot: list, per_tr: list, length: int) -> None:
    """A leg's kick shaped (owner 2026-10-20, her down air: "free f24 looks
    perfect as the max angle for the thigh for full impact, full kick impact
    though should also have the knee fully extended"; "free f13-f20 should
    look more like f24"). "raise_max": [{"row": hip row, "deg": d, "keys":
    [[frame, weight], ...]}]: the thigh (the row's bone to its first child)
    turned toward the pelvis's down (row 1's, as posed) by that weight of
    its raise past d degrees. "straighten": [{"row": knee row, "keys":
    [[frame, weight], ...]}]: the row turned about its bend by that weight
    of its bend past the rest's, so its bone runs on from its parent's as at
    rest. "split_max": [{"row": hip row, "other": the other hip row, "deg":
    d, "keys": ...}]: the thigh turned toward the other's by that weight of
    the angle between them past d degrees (owner 2026-10-20, her back
    throw's arabesque: the legs 90 degrees apart, not 180). Weights eased
    between keys, held past their ends."""
    bind, bpos = K._worlds(skel, [A.euler(*e) for e in skel.rotate], [list(v) for v in skel.translate])
    down0 = bind[1].T @ np.array([0.0, -1.0, 0.0])

    def bone(pos, r):
        v = np.asarray(pos[K._first_child(skel, r)], float) - np.asarray(pos[r], float)
        return v / max(float(np.linalg.norm(v)), 1e-9)

    def angle(u, v):
        return math.acos(max(-1.0, min(1.0, float(u @ v))))

    def turn_row(g, r, axis, rad, rr):
        n = float(np.linalg.norm(axis))
        if n < 1e-9 or abs(rad) < 1e-9:
            return
        parent = skel.parents[r]
        pw = rr[parent] if parent >= 0 else np.eye(3)
        per_rot[g][r] = pw.T @ (_axis_angle(axis / n, rad) @ rr[r])

    for t in spec.get("raise_max", ()):
        r, cap = int(t["row"]), math.radians(float(t["deg"]))
        for g in range(length + 1):
            w = _keyed(t["keys"], g)
            if not w:
                continue
            rr, pp = K._worlds(skel, per_rot[g], per_tr[g])
            down = rr[1] @ down0
            b = bone(pp, r)
            over = angle(b, down) - cap
            if over > 0.0:
                turn_row(g, r, np.cross(b, down), over * w, rr)
    for t in spec.get("split_max", ()):
        r, o, cap = int(t["row"]), int(t["other"]), math.radians(float(t["deg"]))
        for g in range(length + 1):
            w = _keyed(t["keys"], g)
            if not w:
                continue
            rr, pp = K._worlds(skel, per_rot[g], per_tr[g])
            b, c = bone(pp, r), bone(pp, o)
            over = angle(b, c) - cap
            if over > 0.0:
                turn_row(g, r, np.cross(b, c), over * w, rr)
    for t in spec.get("straighten", ()):
        r = int(t["row"])
        p = skel.parents[r]
        rest = angle(bone(bpos, p), bone(bpos, r))
        for g in range(length + 1):
            w = _keyed(t["keys"], g)
            if not w:
                continue
            rr, pp = K._worlds(skel, per_rot[g], per_tr[g])
            a, b = bone(pp, p), bone(pp, r)
            bend = angle(a, b) - rest
            if bend > 0.0:
                turn_row(g, r, np.cross(b, a), bend * w, rr)


def _item_pitch(skel: A.Skeleton, cfg: dict, keys, per_rot: list, per_tr: list, length: int) -> None:
    """The item turned about its grip so its tip drops ("item_pitch":
    [[frame, degrees], ...], eased between keys): about the horizontal axis
    across the blade (owner p6s: Ike's forward smash "slam the sword 100%
    flat against the ground, maybe go slightly past the ground a couple of
    degrees")."""
    it = cfg["item"]
    ir = it["row"]
    axis = np.asarray(it["axis"], float) / np.linalg.norm(it["axis"])
    for g in range(length + 1):
        deg = _keyed(keys, g)
        if not deg:
            continue
        rr, _pp = K._worlds(skel, per_rot[g], per_tr[g])
        blade = rr[ir] @ axis
        across = np.cross(blade, np.array([0.0, 1.0, 0.0]))
        n = float(np.linalg.norm(across))
        if n < 1e-6:
            continue
        # Turning by -deg about blade x up takes the tip down.
        turn = _axis_angle(across / n, -math.radians(deg))
        parent = skel.parents[ir]
        pw = rr[parent] if parent >= 0 else np.eye(3)
        per_rot[g][ir] = pw.T @ (turn @ rr[ir])


def _item_path(skel: A.Skeleton, cfg: dict, ip: dict, per_rot: list, per_tr: list, length: int) -> None:
    """The item thrown up and caught (Aether's Ragnell, owner p6r: Ultimate's
    sword model motion left the blade wandering): from the hand at "release"
    [r0, r1] it flies, eased over "throw" frames, to where the hand meets it
    at "catch" [c0, c1], turning about the lateral axis (whole turns at about
    "spin" degrees a frame, ending as the hand holds it), and hangs there in
    the world while the fighter rises from "takeoff": "rise" game units each
    frame after it (the status code's table), or "vel" up less "gravity" a
    frame ("tvel" the fastest fall); game units over the fighter's "size" are
    the model's. Its point "centre" along the blade is the one that flies;
    [c0, c1] eases it back into the hand. "ahead" (model units, his facing)
    hangs it that much farther in front of him than where the hand meets it
    (owner p6v: Aether's thrown sword "a little more in front of Ike, to
    match the downward long swipe")."""
    it = cfg["item"]
    ir, hr = it["row"], it["hand"]
    r0, r1 = (int(x) for x in ip["release"])
    c0, c1 = (int(x) for x in ip["catch"])
    axis = np.asarray(it["axis"], float) / np.linalg.norm(it["axis"])
    mid = axis * float(ip.get("centre", 0.0))
    size = float(ip.get("size", 1.0))
    jump = int(ip["takeoff"])
    table = [float(v) for v in ip.get("rise", ())]

    def rise(g):
        n = max(g - jump, 0)
        if table:
            return sum(table[:n]) / size
        h, v, grav, tvel = 0.0, float(ip["vel"]), float(ip["gravity"]), float(ip.get("tvel", 1e9))
        for _ in range(n):
            h += v
            v = max(v - grav, -tvel)
        return h / size

    def rot_x(phi):
        c, s = math.cos(phi), math.sin(phi)
        return np.array([[1.0, 0.0, 0.0], [0.0, c, -s], [0.0, s, c]])

    worlds = [K._worlds(skel, per_rot[g], per_tr[g]) for g in range(length + 1)]
    held = [(rr[ir], pp[ir] + rr[ir] @ mid) for rr, pp in worlds]
    r_rel, p_rel = held[r0]
    hang = held[c1][1] + np.array([0.0, rise(c1), float(ip.get("ahead", 0.0))])
    # The blade's angle in the camera's plane at release and at the catch:
    # rot_x(phi) takes an angle atan2(y, z) down by phi.
    b0, b1 = r_rel @ axis, held[c1][0] @ axis
    turn = (math.atan2(b0[1], b0[2]) - math.atan2(b1[1], b1[2])) % (2.0 * math.pi)
    span = max(c1 - r0, 1)
    turns = max(round(math.radians(float(ip.get("spin", 24.0))) * span / (2.0 * math.pi)), 0)
    total = turn + 2.0 * math.pi * turns
    throw = max(float(ip.get("throw", 4.0)), 1.0)
    for g in range(r0, min(c1, length) + 1):
        tau = g - r0
        e = min(tau / throw, 1.0)
        e = 1.0 - (1.0 - e) * (1.0 - e)
        centre = p_rel + (hang - p_rel) * e - np.array([0.0, rise(g), 0.0])
        rot = rot_x(total * tau / span) @ r_rel
        hr_, hp_ = held[g]
        if r1 > r0 and g < r1:
            w = (g - r0) / float(r1 - r0)
            rot, centre = P.slerp(hr_, rot, w), hp_ + (centre - hp_) * w
        if c1 > c0 and g > c0:
            w = (g - c0) / float(c1 - c0)
            w = w * w * (3.0 - 2.0 * w)
            rot, centre = P.slerp(rot, hr_, w), centre + (hp_ - centre) * w
        rr, pp = worlds[g]
        origin = centre - rot @ mid
        per_rot[g][ir] = rr[hr].T @ rot
        per_tr[g][ir] = [float(v) for v in rr[hr].T @ (origin - pp[hr])]


def _frames(skel: A.Skeleton, spec: dict, scales=None):
    """A recipe's rows over its frames (each a rotation matrix and a
    translation) and its skirt guide (the hem's and the mid ring's moves),
    as build() keys them, with its Retarget and its parts."""
    fighter = spec.get("fighter", "palutena")
    cfg = FIGHTERS[fighter]
    length = int(spec["length"])
    parts = _parts(spec, length)
    sources = {}
    for p in parts:
        if p["motion"] not in sources:
            # "keep_trans": the motion's Trans moves the body in place (a
            # step in and back that the game's position does not take).
            sources[p["motion"]] = Source(fighter, p["motion"], keep_trans=bool(spec.get("keep_trans")))
            if spec.get("item_motion") is False:
                # The item in the hand throughout: its own model's motion
                # left out ("item_path" throws it instead).
                sources[p["motion"]].item = None
    rt = Retarget(skel, scales, sources[parts[0]["motion"]], cfg, spec)
    per_rot, per_tr, hems, mids = [], [], [], []
    mid_names = cfg.get("skirt_mid")
    for g in range(length + 1):
        part = _part_at(parts, g)
        u, src, yaw = time_map(part["time"], g), sources[part["motion"]], _keyed(spec.get("yaw"), g)
        lr, tr = rt.pose(u, src, yaw)
        per_rot.append(lr)
        per_tr.append(tr)
        hems.append(rt.skirt_moves(u, src, yaw) if cfg.get("skirt") else None)
        mids.append(rt.skirt_moves(u, src, yaw, mid_names) if mid_names else None)
    if spec.get("item_path"):
        _item_path(skel, cfg, spec["item_path"], per_rot, per_tr, length)
    if spec.get("item_pitch"):
        _item_pitch(skel, cfg, spec["item_pitch"], per_rot, per_tr, length)
    if spec.get("turn"):
        _turns(skel, spec["turn"], per_rot, per_tr, length)
    if spec.get("raise_max") or spec.get("straighten") or spec.get("split_max"):
        _legs(skel, spec, per_rot, per_tr, length)
    if spec.get("still"):
        _still(spec["still"], per_rot, per_tr, length, spec.get("loop"))
    if spec.get("skirt_smooth") and cfg.get("skirt"):
        # The guide's hem a centred average over that many frames (Ultimate's
        # skirt chains are baked physics: their jitter is the dress's twitch).
        half = int(spec["skirt_smooth"]) // 2
        raw = [np.asarray(h, float) for h in hems]
        hems = [np.mean([raw[k] for k in _window(len(raw), g, half, spec.get("loop"))], axis=0).tolist()
                for g in range(len(raw))]
        if mid_names:
            raw = [np.asarray(h, float) for h in mids]
            mids = [np.mean([raw[k] for k in _window(len(raw), g, half, spec.get("loop"))], axis=0).tolist()
                    for g in range(len(raw))]
    bf = spec.get("blend_from")
    if bf:
        # The first frames eased in from another motion's pose (the wait or
        # the fall she starts from: Ultimate blends into a motion, the game
        # cuts to it).
        fsrc = Source(fighter, bf["motion"])
        lr_f, tr_f = rt.pose(float(bf.get("frame", 0)), fsrc, float(bf.get("yaw", 0.0)))
        hem_f = (np.asarray(rt.skirt_moves(float(bf.get("frame", 0)), fsrc, float(bf.get("yaw", 0.0))))
                 if cfg.get("skirt") else None)
        mid_f = (np.asarray(rt.skirt_moves(float(bf.get("frame", 0)), fsrc, float(bf.get("yaw", 0.0)), mid_names))
                 if mid_names else None)
        last = int(bf.get("frames", 3))
        for g in range(0, min(last, length + 1)):
            w = g / float(last)
            w = w * w * (3.0 - 2.0 * w)
            per_rot[g] = [P.slerp(b, a, w) for a, b in zip(per_rot[g], lr_f)]
            per_tr[g] = [[b[c] + (a[c] - b[c]) * w for c in range(3)] for a, b in zip(per_tr[g], tr_f)]
            if hem_f is not None:
                hems[g] = (w * np.asarray(hems[g]) + (1.0 - w) * hem_f).tolist()
            if mid_f is not None:
                mids[g] = (w * np.asarray(mids[g]) + (1.0 - w) * mid_f).tolist()
    bt = spec.get("blend_to")
    if bt:
        # The last frames drawn toward another motion's pose (the wait she
        # returns to): rotations slerped, translations lerped, eased.
        tsrc = Source(fighter, bt["motion"])
        lr_t, tr_t = rt.pose(float(bt.get("frame", 0)), tsrc, float(bt.get("yaw", 0.0)))
        hem_t = (np.asarray(rt.skirt_moves(float(bt.get("frame", 0)), tsrc, float(bt.get("yaw", 0.0))))
                 if cfg.get("skirt") else None)
        mid_t = (np.asarray(rt.skirt_moves(float(bt.get("frame", 0)), tsrc, float(bt.get("yaw", 0.0)), mid_names))
                 if mid_names else None)
        first, last = (int(x) for x in bt["frames"])
        for g in range(max(first, 0), length + 1):
            w = 1.0 if g >= last else (g - first) / max(last - first, 1)
            w = w * w * (3.0 - 2.0 * w)
            per_rot[g] = [P.slerp(a, b, w) for a, b in zip(per_rot[g], lr_t)]
            per_tr[g] = [[a[c] + (b[c] - a[c]) * w for c in range(3)] for a, b in zip(per_tr[g], tr_t)]
            if hem_t is not None:
                hems[g] = ((1.0 - w) * np.asarray(hems[g]) + w * hem_t).tolist()
            if mid_t is not None:
                mids[g] = ((1.0 - w) * np.asarray(mids[g]) + w * mid_t).tolist()
    bi = spec.get("blend_into")
    if bi:
        # The frames before the game's cut to another motion eased into that
        # motion's own frames, moving with them, so the cut lands on its
        # first: her dash, which the game leaves for her run at frame "cut"
        # (dash_to_run; the dash's frame before it is the last one shown).
        # Owner 2026-10-20: "the end of the start run animation should be
        # pretty much the exact same as the 1st frame of the running
        # animation". "frames": [first, last], the ease in (the other's frames
        # in full from `last`, so the frame before the cut is the one before
        # its first: a loop's last); "leave": the frame a clip played on past
        # the cut (a tapped dash) is its own again. The other is a recipe of
        # the same tree and scene (art/p6/<fighter>/anims/<recipe>.json).
        other = json.loads((REPO / "art" / "p6" / fighter / "anims" / f"{bi['recipe']}.json")
                           .read_text(encoding="utf-8"))["ultimate"]
        if (other.get("skeleton"), other.get("scene")) != (spec.get("skeleton"), spec.get("scene")):
            raise UltimateError(f"blend_into {bi['recipe']}: another tree or scene")
        o_rot, o_tr, o_hem, o_mid, _o_rt, _o_parts = _frames(skel, other, scales)
        o_len, o_loop = int(other["length"]), other.get("loop")
        cut, leave = int(bi["cut"]), int(bi.get("leave", length + 1))
        first, last = (int(x) for x in bi["frames"])
        for g in range(max(first, 0), min(leave, length + 1)):
            if g < last:
                w = (g - first) / float(max(last - first, 1))
            elif g <= cut:
                w = 1.0
            else:
                w = 1.0 - (g - cut) / float(max(leave - cut, 1))
            w = w * w * (3.0 - 2.0 * w)
            k = g - cut
            if o_loop is not None:
                k = int(o_loop) + (k - int(o_loop)) % (o_len - int(o_loop))
            else:
                k = min(max(k, 0), o_len)
            per_rot[g] = [P.slerp(a, b, w) for a, b in zip(per_rot[g], o_rot[k])]
            per_tr[g] = [[a[c] + (b[c] - a[c]) * w for c in range(3)] for a, b in zip(per_tr[g], o_tr[k])]
            if hems[g] is not None and o_hem[k] is not None:
                hems[g] = ((1.0 - w) * np.asarray(hems[g]) + w * np.asarray(o_hem[k])).tolist()
            if mids[g] is not None and o_mid[k] is not None:
                mids[g] = ((1.0 - w) * np.asarray(mids[g]) + w * np.asarray(o_mid[k])).tolist()
    return per_rot, per_tr, hems, mids, rt, parts


def build(skel: A.Skeleton, spec: dict, scales=None, rom=None, extras: dict | None = None) -> A.Clip:
    fighter = spec.get("fighter", "palutena")
    cfg = FIGHTERS[fighter]
    length = int(spec["length"])
    per_rot, per_tr, hems, mids, rt, parts = _frames(skel, spec, scales)
    mid_names = cfg.get("skirt_mid")
    if extras is not None and spec.get("skirt", True) and cfg.get("skirt"):
        # The dress follows Ultimate's skirt (skirt_keys.py guide), its last
        # frames drawn back with the pose (blend_to) by the same weights.
        extras["skirt_guide"] = {"azimuths": rt.skirt_azimuths(), "hem": hems}
        if mid_names:
            extras["skirt_guide"].update(mid_azimuths=rt.skirt_azimuths(mid_names), mid=mids)
        # Her feet on a floor (skirt_keys.py keeps the hem on it): not in an
        # aerial, a jump, a fall or a tumble (the recipe's "floor" decides).
        names = [p["motion"] for p in parts]
        extras["skirt_guide"]["floor"] = bool(spec.get("floor", not any(
            w in n for n in names for w in ("air", "jump", "fall", "fly", "cliff", "passive", "escapeair"))))
        # The dress's own keys for a kick that leaves it (skirt_keys.py).
        extras["skirt_guide"].update(spec.get("dress") or {})
    step = int(spec.get("key_step", 1))
    if step > 1:
        # Keys every `step` frames, the frames between them interpolated: a
        # dense clip compacts to about 1/step of its keys (a figatree heap).
        keep = set(range(0, length + 1, step)) | {length}
        # "key_dense": [[first, last], ...] frames keyed every frame (a fast
        # swing keeps its snap).
        for a_, b_ in spec.get("key_dense", ()):
            keep |= set(range(max(int(a_), 0), min(int(b_), length) + 1))
        grid = sorted(keep)
        for g in range(length + 1):
            if g in keep:
                continue
            a = max(x for x in grid if x < g)
            b = min(x for x in grid if x > g)
            w = (g - a) / float(b - a)
            per_rot[g] = [P.slerp(x, y, w) for x, y in zip(per_rot[a], per_rot[b])]
            per_tr[g] = [[x[c] + (y[c] - x[c]) * w for c in range(3)] for x, y in zip(per_tr[a], per_tr[b])]
    first_slot = int(spec.get("first_slot", 0))
    clip = frames_to_clip(skel, scales, per_rot, per_tr, length, spec.get("loop"), first_slot,
                          bool(spec.get("held")))
    tn = spec.get("transn")
    if tn:
        if first_slot < 1:
            raise ValueError("ultimate transn needs first_slot 1")
        if isinstance(tn, dict) and "travel" in tn:
            # A forward travel curve (keyposes "travel"): the game's slide kept.
            z = K.travel_curve(tn["travel"], length)
            st = A.Stream(length, None)
            for c in range(3):
                st.tracks[c] = A.Track([0] * (length + 1), 0)
            st.tracks[4] = A.Track([0] * (length + 1), 0)
            st.tracks[5] = A.Track([0] * (length + 1), 0)
            st.tracks[6] = A.Track([round(v * A.TRANS_UNIT) for v in z], 0)
            clip.streams[0] = st
        elif isinstance(tn, dict) and tn.get("absolute"):
            # The motion's Trans where it is (its height and its travel,
            # scaled), not its travel from its first frame: the ledge's, where
            # Ultimate hangs her from the corner (her hands on it) and the
            # second part's TransN steps carry her onto the stage
            # (ftCommonCliffCommon2ProcPhysics).
            part = parts[0]
            tsrc = Source(fighter, part["motion"], keep_trans=True)
            yz = [[float(tsrc._local("Trans", time_map(part["time"], g))[c, 3]) * rt.k for c in (1, 2)]
                  for g in range(length + 1)]
            st = A.Stream(length, spec.get("loop"))
            for c in range(3):
                st.tracks[c] = A.Track([0] * (length + 1), 0)
            st.tracks[4] = A.Track([0] * (length + 1), 0)
            st.tracks[5] = A.Track([round(v[0] * A.TRANS_UNIT) for v in yz], 0)
            st.tracks[6] = A.Track([round(v[1] * A.TRANS_UNIT) for v in yz], 0)
            clip.streams[0] = st
        elif isinstance(tn, dict) and "clip" in tn:
            # The TransN slot of a clip of the tree (a roll's travel kept the game's).
            src_st = A.decode(rom.o2r(int(str(tn["clip"]), 0))).streams[0]
            fac = float(tn.get("scale", 1.0))
            st = A.Stream(src_st.length, src_st.loop, loop_skips_steps=src_st.loop_skips_steps)
            for k, t in src_st.tracks.items():
                vals = [round(v * fac) for v in t.values] if 4 <= k <= 6 else list(t.values)
                st.tracks[k] = A.Track(vals, t.lead, t.step)
            clip.streams[0] = st
        else:
            part = parts[0]
            tsrc = Source(fighter, part["motion"], keep_trans=True)
            z0 = tsrc.trans_z(time_map(part["time"], 0))
            fac = float(tn.get("scale", 1.0)) if isinstance(tn, dict) else 1.0
            z = [(tsrc.trans_z(time_map(part["time"], g)) - z0) * rt.k * fac for g in range(length + 1)]
            if isinstance(tn, dict) and tn.get("boost"):
                # The travel over frames [a, b] times `factor`, the frames
                # after carried the extra (owner p6v: Ike's dash attack
                # lunging 1.25x as far through its rising swing).
                a, b, k = int(tn["boost"][0]), int(tn["boost"][1]), float(tn["boost"][2])
                za, zb = z[a], z[b]
                z = [v if g <= a else (za + (v - za) * k if g <= b else v + (zb - za) * (k - 1.0))
                     for g, v in enumerate(z)]
            st = A.Stream(length, None)
            for c in range(3):
                st.tracks[c] = A.Track([0] * (length + 1), 0)
            st.tracks[4] = A.Track([0] * (length + 1), 0)
            st.tracks[5] = A.Track([0] * (length + 1), 0)
            st.tracks[6] = A.Track([round(v * A.TRANS_UNIT) for v in z], 0)
            clip.streams[0] = st
    return clip


def frames_to_clip(skel: A.Skeleton, scales, per_rot, per_tr, length: int, loop, first_slot: int,
                   last_row: bool = False) -> A.Clip:
    """A clip of per-frame row poses (keyposes.py's writer: a row the poses
    move gets its rotation tracks, and its translation tracks divided by its
    translate_scale when it moves). The tree's last row is a hidden part's
    (Peach's held joint, Palutena's row 28): a clip keys it only when it
    asks (`last_row`: a hidden part's DObj hangs after the made rows)."""
    rows = len(skel.parents)
    slots = rows - (0 if last_row else 1) + first_slot
    clip = A.Clip(slots)
    for r in range(rows):
        slot = r + first_slot
        if slot >= slots:
            continue
        rest = A.euler(*skel.rotate[r])
        moved = any(np.abs(per_rot[f][r] - rest).max() > 1e-4 for f in range(length + 1))
        tmoved = any(max(abs(per_tr[f][r][c] - skel.translate[r][c]) for c in range(3)) > 1e-3
                     for f in range(length + 1))
        if not (moved or tmoved):
            continue
        st = A.Stream(length, loop)
        prev = tuple(skel.rotate[r])
        eul = []
        for f in range(length + 1):
            prev = A.to_euler(per_rot[f][r], prev)
            eul.append(prev)
        for c in range(3):
            st.tracks[c] = A.Track([round(e[c] * A.ROT_UNIT) for e in eul], 0)
        if tmoved:
            s = scales[r] if scales is not None else (1.0, 1.0, 1.0)
            for c in range(3):
                d = s[c] if abs(s[c]) > 1e-6 else 1.0
                st.tracks[4 + c] = A.Track([round(per_tr[f][r][c] / d * A.TRANS_UNIT)
                                            for f in range(length + 1)], 0)
        clip.streams[slot] = st
    return clip

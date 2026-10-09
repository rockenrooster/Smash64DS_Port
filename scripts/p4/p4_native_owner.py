#!/usr/bin/env python3
"""P4 native owners: feed a donor fighter model to the existing owner generator.

scripts/fighters/generate_nds_native_owners.py compiles a fighter model into
the DS-native owner IR from per-owner tables it keeps for the original cast
(JointTree offsets, setup mask, plan counts, cross-binding slots, census pins).
For a donor the source tables are not BattleShip C files, so this module reads
the same facts from the donor's own main/model files and registers the owner
at run time. Census pins live in scripts/p4/owners/<name>.json (counts only, no
asset bytes); `--learn` re-derives them from the generator's own validators
and must be re-run deliberately when the donor model changes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(REPO / "scripts"))
sys.path.insert(0, str(REPO / "scripts" / "fighters"))
import _paths  # noqa: E402,F401
import generate_nds_native_owners as O  # noqa: E402
import ft_layout  # noqa: E402

PINS_DIR = HERE / "owners"
DOBJ_DESC_SIZE = 0x2C
CROSS_SLOT_FIRST = 16

# name -> {"main": file id, "model": file id}. Filled from each content's
# export (scripts/p4/p4_contents.py export_facts) before any owner registers.
DONORS: dict[str, dict] = {}


def load_donor(name: str, export_root: Path) -> dict:
    import p4_contents  # noqa: E402

    facts = p4_contents.export_facts(export_root, name)
    DONORS[name] = {"main": facts["main"], "model": facts["model"],
                    "attributes": facts["attributes"],
                    "script_parts": script_model_parts(export_root / name / "events.json"),
                    "motion_scripts": motion_scripts(export_root / name)}
    return DONORS[name]


def motion_scripts(export_dir: Path) -> list[tuple[str, int, tuple[tuple[tuple, ...], ...]]]:
    """(label, anim-desc word, threads) for each of the donor's main and menu
    motions. A thread -- the motion's script, then each parallel script it
    starts -- is its model-part events in run order: ("part", joint, part),
    ("hide",) for HideModelPartAll, ("reset",) for ResetModelPartAll and
    ("wait",) where the script yields a frame. A subroutine's commands run at
    its call, a goto ends its block and a loop runs once; a thrown fighter's
    script is not this one's."""
    resolved_path = export_dir / "resolved.json"
    if not resolved_path.exists():
        return []
    resolved = json.loads(resolved_path.read_text(encoding="utf-8"))
    events_path = export_dir / "events.json"
    blocks = (json.loads(events_path.read_text(encoding="utf-8"))["blocks"]
              if events_path.exists() else {})

    def run(block: str, out: list, parallels: list, active: set) -> None:
        if block not in blocks or block in active:
            return
        active.add(block)
        targets = {"call": [], "goto": [], "parallel": []}
        for edge in blocks[block].get("edges", ()):
            if isinstance(edge, dict) and edge.get("kind") in targets:
                targets[edge["kind"]].append(edge["to"])
        for command in blocks[block]["commands"]:
            op = command["op"]
            if op == "SetModelPartID":
                word = command["words"][0]
                part = word & 0x7FFFF
                if part & 0x40000:
                    part -= 0x80000
                out.append(("part", (word >> 19) & 0x7F, part))
            elif op == "HideModelPartAll":
                out.append(("hide",))
            elif op == "ResetModelPartAll":
                out.append(("reset",))
            elif op in ("AsyncWait", "SyncWait"):
                out.append(("wait",))
            elif op == "Subroutine":
                if targets["call"]:
                    run(targets["call"].pop(0), out, parallels, active)
            elif op == "SetParallelScript":
                if targets["parallel"]:
                    parallels.append(targets["parallel"].pop(0))
            elif op in ("Goto", "RemixGotoMovesetFile"):
                if targets["goto"]:
                    run(targets["goto"].pop(0), out, parallels, active)
                break
            elif op in ("End", "Return"):
                break
        active.discard(block)

    def threads(root: str) -> tuple[tuple[tuple, ...], ...]:
        result = []
        pending = [root]
        started = set()
        while pending:
            script = pending.pop(0)
            if script in started:
                continue
            started.add(script)
            out: list = []
            run(script, out, pending, set())
            result.append(tuple(out))
        return tuple(result)

    rows = []
    for table, tag in (("motions", "Main"), ("menu_motions", "Sub")):
        for motion in resolved.get(table, ()):
            root = motion.get("script_root")
            rows.append((f"{tag}{motion['index']}", int(motion["anim_flags"]),
                         threads(root) if root else ()))
    return rows


def script_model_parts(events_path: Path) -> set[tuple[int, int]]:
    """(joint, model part) pairs the donor's own motion scripts set."""
    parts: set[tuple[int, int]] = set()
    if not events_path.exists():
        return parts
    events = json.loads(events_path.read_text(encoding="utf-8"))
    for block in events["blocks"].values():
        for command in block["commands"]:
            if command["op"] != "SetModelPartID":
                continue
            word = command["words"][0]
            part = word & 0x7FFFF
            if part & 0x40000:
                part -= 0x80000
            parts.add(((word >> 19) & 0x7F, part))
    return parts


class O2RFile:
    def __init__(self, path: Path):
        blob = path.read_bytes()
        self.file_id, self.intern, self.extern, n = struct.unpack_from("<IHHI", blob, 0x40)
        self.externs = list(struct.unpack_from(f"<{n}H", blob, 0x4C))
        off = 0x4C + 2 * n
        size = struct.unpack_from("<I", blob, off)[0]
        self.data = blob[off + 4:off + 4 + size]
        self.slots: dict[int, tuple] = {}
        w = self.intern
        while w != 0xFFFF:
            val = struct.unpack_from(">I", self.data, w * 4)[0]
            self.slots[w * 4] = ("intern", self.file_id, (val & 0xFFFF) * 4)
            w = val >> 16
        w, k = self.extern, 0
        while w != 0xFFFF:
            val = struct.unpack_from(">I", self.data, w * 4)[0]
            self.slots[w * 4] = ("extern", self.externs[k], (val & 0xFFFF) * 4)
            k += 1
            w = val >> 16

    def ptr(self, off: int) -> tuple | None:
        return self.slots.get(off)

    def u32(self, off: int) -> int:
        return struct.unpack_from(">I", self.data, off)[0]


def donor_tables(o2r_dir: Path, main_id: int, model_id: int, attr_offset: int) -> dict:
    lay = ft_layout.layout()
    main = O2RFile(o2r_dir / f"{main_id:04x}")
    model = O2RFile(o2r_dir / f"{model_id:04x}")
    container = main.ptr(attr_offset + lay["FTAttributes.commonparts_container"])
    setup = main.ptr(attr_offset + lay["FTAttributes.setup_parts"])
    if container is None or setup is None or container[0] != "intern" or setup[0] != "intern":
        raise SystemExit("donor main: commonparts/setup_parts are not internal pointers")
    trees = []
    # FTCommonPart.flags bit 0: each DObjDesc.dl is a `Gfx *dls[2]` pair
    # (ftdisplaymain.c), Yoshi's form, which Bowser keeps.
    flags = {main.data[container[2] + detail * lay["sizeof(FTCommonPart)"] +
                       lay["FTCommonPart.flags"]] & 1 for detail in range(2)}
    if len(flags) != 1:
        raise SystemExit("donor main: the two details disagree on the DL pair form")
    for detail in range(2):
        ref = main.ptr(container[2] + detail * lay["sizeof(FTCommonPart)"] + lay["FTCommonPart.dobjdesc"])
        if ref is None or ref[0] != "extern" or ref[1] != model_id:
            raise SystemExit(f"donor main: detail {detail} JointTree is not in the model file")
        count = 0
        while True:
            if model.u32(ref[2] + count * DOBJ_DESC_SIZE) == 18:
                count += 1
                break
            count += 1
            if count > 64:
                raise SystemExit("donor JointTree has no depth-18 sentinel")
        trees.append((ref[2], count))
    mask = (main.u32(setup[2]), main.u32(setup[2] + 4))
    return {"trees": trees, "setup": mask, "dl_pairs": flags == {1}}


# name -> detail -> [(binding, offset, joint, part, vertices, scripted)] for
# every variant donor_variants found, dropped ones included (learn's choice).
VARIANT_INFO: dict[str, dict[str, list[tuple]]] = {}


def _display_vertices(payload: bytes, offset: int) -> int:
    """Vertices a display list loads (G_VTX counts up to its G_ENDDL)."""
    total = 0
    for at in range(offset, len(payload) - 7, 8):
        op = payload[at]
        if op == 0x01:
            total += (struct.unpack_from(">I", payload, at)[0] >> 12) & 0xFF
        elif op == 0xDF:
            break
    return total


def donor_variants(name: str, o2r_dir: Path, main_id: int, model_id: int,
                   attr_offset: int, dropped: dict | None = None) -> dict:
    """Every alternate model part a donor's joints carry, per detail, as
    (logical binding, display offset) rows for P2_MODEL_PART_ROOT_VARIANTS.

    ftParamSetModelPartID writes modelparts[joint - 4][part][detail].dl into
    the live DObj, and a P4 fighter's parts are set by its own scripts and by
    its parent's code (Bowser's jaw by Yoshi's specials), so the owner carries
    every part, not only the ones a script names. A joint's FTModelPartDesc
    runs from its row to the next descriptor (or the container itself)."""
    lay = ft_layout.layout()
    main = O2RFile(o2r_dir / f"{main_id:04x}")
    container = main.ptr(attr_offset + lay["FTAttributes.modelparts_container"])
    if container is None:
        return {}
    payload = O.load_o2r_payload(REPO, name)
    scripted = DONORS[name].get("script_parts", set())
    dropped = dropped or {}
    result = {}
    VARIANT_INFO[name] = {}
    for detail in ("high", "low"):
        descriptors = O._owner_joint_descriptors(payload, name, detail)[:-1]
        selected = O._owner_selected_descriptor_indices(name, len(descriptors))
        roots = [i for i in selected if descriptors[i][1] is not None]
        rows_by_joint = {}
        for index in range(min(len(descriptors), 37 - 4)):
            ref = main.ptr(container[2] + 4 * index)
            if ref is not None and ref[0] == "intern":
                rows_by_joint[index] = ref[2]
        starts = sorted(set(rows_by_joint.values()) | {container[2]})
        info = []
        for binding, index in enumerate(roots):
            base = rows_by_joint.get(index)
            if base is None:
                continue
            bound = min([s for s in starts if s > base], default=len(main.data))
            part = 1
            while base + (part + 1) * 2 * 20 <= bound:
                ref = main.ptr(base + (part * 2 + (detail == "low")) * 20)
                if (ref is not None and ref[0] == "extern" and ref[1] == model_id and
                        ref[2] != descriptors[index][1] and
                        all(row[1] != ref[2] or row[0] != binding for row in info)):
                    info.append((binding, ref[2], index + 4, part,
                                 _display_vertices(payload, ref[2]),
                                 (index + 4, part) in scripted))
                part += 1
        VARIANT_INFO[name][detail] = info
        rows = tuple((row[0], row[1]) for row in info
                     if row[1] not in set(dropped.get(detail, ())))
        if rows:
            result[detail] = rows
    return result


# Hidden parts 0-2 are the TransN/XRotN/YRotN joints (FTHiddenPart kind 3),
# which never carry a display list.
HIDDEN_PART_FIRST = 3
# The anim-desc word's low five bits are flags (FTANIM_FLAG_*); bit 31 - i
# above them installs hidden part i (ftMainSetStatus).
ANIM_DESC_FLAG_BITS = 0x1F

# name -> [(program name, model-part -1 writes, motion labels)] (donor_superset).
PROGRAM_INFO: dict[str, list[tuple[str, tuple, tuple]]] = {}


def _hidden_part_ids(word: int) -> list[int]:
    mask = word & ~ANIM_DESC_FLAG_BITS & 0xFFFFFFFF
    return [i for i in range(32) if (mask & (1 << (31 - i))) and i >= HIDDEN_PART_FIRST]


def donor_superset(name: str, o2r_dir: Path, main_id: int, model_id: int,
                   attr_offset: int) -> dict:
    """The joints a donor's motions make draw beyond its setup_parts, and the
    live root vectors that leaves.

    The original cast carries these as root programs over the setup_parts
    vector (OWNER_ROOT_PROGRAMS: Ness's yo-yo and bat, Samus's grapple,
    Yoshi's grab). A donor's come from its own motions: a motion's anim-desc
    mask installs hidden parts (ftMainUpdateHiddenPartID: the JointTree list,
    the Low detail taking the High one when its own is NULL) and its
    SetModelPartID commands change parts (ftParamSetModelPartID: the part's
    list at the fighter's detail, or the JointTree's for a joint with no part
    table). Every joint some motion makes draw joins the canonical vector --
    a hidden part through setup_parts, a list-less joint through the first
    part that draws it -- so each live vector is the canonical one less some
    roots: a program of model-part -1 writes, and the joints' other parts are
    ordinary variants. Banjo's Kazooie (hidden parts 4-10, 55 motions) draws
    this way, and so do Crash's, Peach's, Lanky's and Dedede's articles."""
    lay = ft_layout.layout()
    main = O2RFile(o2r_dir / f"{main_id:04x}")
    payload = O.load_o2r_payload(REPO, name)
    states = DONORS[name].get("motion_scripts", [])
    raw = {d: O._owner_raw_joint_descriptors(payload, name, d)[:-1] for d in ("high", "low")}
    count = len(raw["high"])
    selected = set(O._owner_selected_descriptor_indices(name, count))
    container = main.ptr(attr_offset + lay["FTAttributes.modelparts_container"])
    hidden_ids = sorted({i for _label, word, _events in states for i in _hidden_part_ids(word)})
    hidden_rows: dict[int, tuple[int, int, int, int]] = {}
    if hidden_ids:
        ref = main.ptr(attr_offset + lay["FTAttributes.hiddenparts"])
        if ref is None or ref[0] != "intern":
            raise SystemExit(f"{name}: motions install hidden parts but FTAttributes."
                             "hiddenparts names no table")
        for i in hidden_ids:
            hidden_rows[i] = struct.unpack_from(">iiii", main.data, ref[2] + 16 * i)

    def part_list(joint: int, part: int, detail: str) -> int | None:
        if part < 0:
            return None
        table = None
        if container is not None and container[0] == "intern":
            desc = main.ptr(container[2] + 4 * (joint - 4))
            table = desc[2] if desc is not None and desc[0] == "intern" else None
        if table is None:
            return raw[detail][joint - 4][1]
        ref = main.ptr(table + (part * 2 + (detail == "low")) * 20)
        if ref is None:
            return None
        if ref[0] != "extern" or ref[1] != model_id:
            # A list in another file (Falco's, Wolf's and Sonic's pistol on
            # joint 17) draws beside the body: the renderer strips that DObj
            # from the owner's vector (the gun sidecar).
            return None
        if name in O.OWNER_DL_PAIR_MODE:
            raise SystemExit(f"{name}: a pair-mode donor's part lists are not derived yet")
        return None if payload[ref[2]] == O.SOURCE_END_DL else ref[2]

    hidden_joints: dict[int, tuple[int, int]] = {}
    snapshots: list[tuple[str, frozenset, frozenset]] = []

    def simulate(label: str, word: int, events, detail: str) -> list[frozenset]:
        """The joints drawing wherever the script yields a frame (and at its
        end): the canonical joints and the mask's hidden parts, then the
        thread's events."""
        parts: dict[int, int] = {}
        lists: dict[int, int | None] = {}
        for index in selected:
            lists[index + 4] = raw[detail][index][1]
            parts[index + 4] = 0 if lists[index + 4] is not None else -1
        for i in _hidden_part_ids(word):
            joint, parent, _partindex, kind = hidden_rows[i]
            if joint < 4:
                continue
            if joint - 4 in selected or not (4 <= joint < 4 + count):
                raise SystemExit(f"{name} {label}: hidden part {i} is joint {joint}")
            hidden_joints[joint] = (parent, kind)
            lists[joint] = raw[detail][joint - 4][1]
            parts[joint] = 0 if lists[joint] is not None else -1
        base_parts = dict(parts)
        base_lists = dict(lists)
        snaps = [frozenset(j for j, dl in lists.items() if dl is not None)]
        for event in tuple(events) + (("wait",),):
            if event[0] == "part":
                _kind, joint, part = event
                if joint in parts and parts[joint] != part:
                    parts[joint] = part
                    lists[joint] = part_list(joint, part, detail)
            elif event[0] == "hide":
                for joint in parts:
                    parts[joint] = -1
                    lists[joint] = None
            elif event[0] == "reset":
                parts.update(base_parts)
                lists.update(base_lists)
            else:
                snaps.append(frozenset(j for j, dl in lists.items() if dl is not None))
        return snaps

    for label, word, threads in states:
        # Each thread from the motion's start, and all of them in a row (a
        # parallel script's writes land on the motion script's).
        for events in (tuple(threads) or ((),)) + (
                (tuple(e for t in threads for e in t),) if len(threads) > 1 else ()):
            per = {d: simulate(label, word, events, d) for d in ("high", "low")}
            for high, low in dict.fromkeys(zip(per["high"], per["low"])):
                if high or low:
                    snapshots.append((label, high, low))

    canonical = {d: frozenset(i + 4 for i in selected if raw[d][i][1] is not None)
                 for d in ("high", "low")}
    superset = {d: set(canonical[d]) for d in ("high", "low")}
    for _label, high, low in snapshots:
        superset["high"] |= high
        superset["low"] |= low
    extra_hidden = sorted(j for j in hidden_joints if j in superset["high"] | superset["low"])

    # The live tree: setup_parts' JointTree walk (lbCommonSetupFighterPartsDObjs:
    # a joint hangs from the last selected one a level up), then each hidden
    # part appended as its parent's last child in install order. Its preorder
    # must be the descriptor order the owner bakes in.
    children: dict[int, list[int]] = {}
    active: list[int | None] = [None] * 19
    for index in sorted(selected | {j - 4 for j in extra_hidden}):
        depth = raw["high"][index][0]
        parent = 3 if depth == 0 else (active[depth - 1] + 4 if active[depth - 1] is not None else None)
        if parent is None:
            raise SystemExit(f"{name}: joint {index + 4} has no selected parent")
        if index + 4 in hidden_joints:
            want, kind = hidden_joints[index + 4]
            if kind != 0 or want != parent:
                raise SystemExit(f"{name}: hidden joint {index + 4} hangs from {want} "
                                 f"(kind {kind}), not the JointTree's {parent}")
        else:
            children.setdefault(parent, []).append(index + 4)
        active[depth] = index
    for i in hidden_ids:
        joint = hidden_rows[i][0]
        if joint in extra_hidden:
            children.setdefault(hidden_rows[i][1], []).append(joint)
    order: list[int] = []
    stack = [4]
    while stack:
        joint = stack.pop()
        order.append(joint)
        stack.extend(reversed(children.get(joint, [])))
    union = superset["high"] | superset["low"]
    if [j for j in order if j in union] != sorted(union):
        raise SystemExit(f"{name}: live walk {[j for j in order if j in union]} is not "
                         "the JointTree's descriptor order")

    used_parts: dict[int, set[int]] = {}
    for _label, _word, threads in states:
        for events in threads:
            for event in events:
                if event[0] == "part" and event[2] >= 0:
                    used_parts.setdefault(event[1], set()).add(event[2])
    base: dict[str, dict[int, int]] = {"high": {}, "low": {}}
    for detail in ("high", "low"):
        for joint in sorted(superset[detail] - canonical[detail]):
            if raw[detail][joint - 4][1] is not None:
                continue
            drawing = sorted((p for p in used_parts.get(joint, ())
                              if part_list(joint, p, detail) is not None),
                             key=lambda p: (p != 0, p))
            if not drawing:
                raise SystemExit(f"{name}: joint {joint} draws with no part list at {detail}")
            base[detail][joint - 4] = part_list(joint, drawing[0], detail)

    programs: dict[frozenset, list[str]] = {}
    for label, high, low in snapshots:
        removed = (superset["high"] - high) | (superset["low"] - low)
        if ((superset["high"] - removed) != high) or ((superset["low"] - removed) != low):
            raise SystemExit(f"{name} {label}: the details' live vectors differ")
        if removed:
            programs.setdefault(frozenset(removed), []).append(label)
    rows = []
    for k, (removed, labels) in enumerate(sorted(programs.items(),
                                                 key=lambda kv: (len(kv[0]), sorted(kv[0])))):
        rows.append((f"Live{k + 1}", tuple((j, -1) for j in sorted(removed)),
                     tuple(dict.fromkeys(labels))))
    return {"setup": [j - 4 for j in extra_hidden], "base": base, "programs": rows,
            "container": container[2] if container is not None else None}


def _o2r_rel(repo: Path, path: Path) -> Path:
    try:
        return path.resolve().relative_to(repo.resolve())
    except ValueError:
        raise SystemExit(f"{path}: O2R inputs must live under the repository (builds/)")


def register(name: str, o2r_dir: Path, attr_offset: int, pins: dict | None) -> dict:
    """Register `name` with the generator; returns the derived source tables."""
    spec = DONORS[name]
    model_path = o2r_dir / f"{spec['model']:04x}"
    sha = hashlib.sha256(model_path.read_bytes()).hexdigest()
    tables = donor_tables(o2r_dir, spec["main"], spec["model"], attr_offset)
    O.P4_DONOR_OWNERS.add(name)
    if tables["dl_pairs"]:
        O.OWNER_DL_PAIR_MODE = O.OWNER_DL_PAIR_MODE | {name}
    O.P2_O2R_ASSETS[name] = (_o2r_rel(REPO, model_path), spec["model"], sha)
    O.OWNER_JOINT_TREES[name] = tuple(tables["trees"][0])
    O.OWNER_JOINT_TREES_LOW[name] = tuple(tables["trees"][1])
    O.OWNER_SETUP_PARTS[name] = tuple(tables["setup"])
    O.P4_CANONICAL_DISPLAY_OVERRIDES.pop(name, None)
    superset = donor_superset(name, o2r_dir, spec["main"], spec["model"], attr_offset)
    if superset["setup"]:
        words = list(O.OWNER_SETUP_PARTS[name])
        for index in superset["setup"]:
            words[index // 32] |= 1 << (31 - (index & 31))
        O.OWNER_SETUP_PARTS[name] = tuple(words)
    if superset["base"]["high"] or superset["base"]["low"]:
        O.P4_CANONICAL_DISPLAY_OVERRIDES[name] = superset["base"]
    if superset["programs"]:
        PROGRAM_INFO[name] = superset["programs"]
        O.OWNER_ROOT_PROGRAMS[name] = tuple((row[0], row[1]) for row in superset["programs"])
        O.OWNER_ROOT_PROGRAM_SOURCES[name] = (
            _o2r_rel(REPO, o2r_dir / f"{spec['main']:04x}"), spec["main"], superset["container"])
    else:
        PROGRAM_INFO.pop(name, None)
        O.OWNER_ROOT_PROGRAMS.pop(name, None)
        O.OWNER_ROOT_PROGRAM_SOURCES.pop(name, None)
    pins = pins or {}
    O.OWNER_PLAN_COUNTS[name] = tuple(pins.get("plan", (0, 0)))
    O.OWNER_CROSS_BINDING_SLOTS[name] = tuple(tuple(x) for x in pins.get("cross_high", ()))
    if "cross_low" in pins:
        O.OWNER_CROSS_BINDING_SLOTS_LOW[name] = tuple(tuple(x) for x in pins["cross_low"])
    O.OWNER_GX_PLAN_COUNTS[name] = tuple(pins.get("gx", (1, 0, 0, 0, 0)))
    for detail in ("high", "low"):
        O.DETAIL_GX_PLAN_COUNTS[detail][name] = tuple(pins.get(f"gx_{detail}", (1, 0, 0, 0, 0)))
    O.P2_OWNER_MODEL_CENSUS[name] = {d: tuple(pins.get(f"census_{d}", ())) for d in ("high", "low")}
    variants = donor_variants(name, o2r_dir, spec["main"], spec["model"], attr_offset,
                              pins.get("variants_dropped"))
    if variants:
        O.P2_MODEL_PART_ROOT_VARIANTS[name] = variants
    else:
        O.P2_MODEL_PART_ROOT_VARIANTS.pop(name, None)
    return tables


def _cross_bindings(name: str, detail: str) -> list[int]:
    data = O.build_p2_owner_source_export(REPO, name, detail)
    vertex = O.unpack_many("<BBBBIhh", data["vertex"])
    vb = dict(O.unpack_many("<HH", data.get("vertex_bindings", b"")))
    tris = [i[0] for i in O.unpack_many("<H", data["triangles"])]
    runs = O.unpack_many("<HBBI", data["runs"])
    epochs = O.unpack_many("<HHHHBBBBBBBB", data["epochs"])
    roots = O.unpack_many("<IHHHBBBB2x", data[f"{name}_roots"])
    geo = O.build_dense_geometry(vertex, tris, runs, epochs, ((name, roots),), REPO, action_bindings=vb)
    cross: set[int] = set()
    for index, run in enumerate(runs):
        if run[2] == 1:
            cross |= set(geo[8][index])
    return sorted(cross)


def learn(name: str, o2r_dir: Path, attr_offset: int) -> dict:
    """Derive every pin from the donor model and the generator's validators."""
    tables = register(name, o2r_dir, attr_offset, None)
    payload = O.load_o2r_payload(REPO, name)
    selected = O._owner_selected_descriptor_indices(name, tables["trees"][0][1] - 1)
    descriptors = O._owner_joint_descriptors(payload, name, "high")[:-1]
    roots = [descriptors[i][1] for i in selected if descriptors[i][1] is not None]
    pins: dict = {"plan": [1 + len(selected), len(roots)]}
    register(name, o2r_dir, attr_offset, pins)
    for detail in ("high", "low"):
        cross = _cross_bindings(name, detail)
        pins[f"cross_{detail}"] = [[b, CROSS_SLOT_FIRST + i] for i, b in enumerate(cross)]
    if pins["cross_low"] == pins["cross_high"]:
        del pins["cross_low"]
    pins["gx"] = [1, 0, 0, len(pins["cross_high"]), 0]
    for detail in ("high", "low"):
        cross = pins.get(f"cross_{detail}", pins["cross_high"])
        pins[f"gx_{detail}"] = [1, 0, 0, len(cross), 0]
    pins["census_high"] = pins["census_low"] = [0] * 13
    patterns = (
        (r"seed/push/pop=1/(\d+)/(\d+)", lambda m: _set_gx(pins, push=int(m[1]), pop=int(m[2]))),
        (r"(high|low) GX store count (\d+) !=", lambda m: _set_detail(pins, m[1], 3, int(m[2]))),
        (r"(high|low) GX restore count (\d+) !=", lambda m: _set_detail(pins, m[1], 4, int(m[2]))),
        (r"packet store/restore (\d+)/(\d+) !=", None),
        (r"(high|low) (?:native-model|runtime context) census \(([^)]*)\) !=",
         lambda m: pins.__setitem__(f"census_{m[1]}", [int(x) for x in m[2].split(",")])),
        # A model-part variant's cross run names a binding no canonical run
        # crosses: it takes the next physical slot.
        (r"(high|low) (?:current )?binding (\d+) has no (?:restorable )?GX palette slot",
         lambda m: _add_cross(pins, m[1], int(m[2]))),
    )
    building = ["high"]
    for _attempt in range(40):
        register(name, o2r_dir, attr_offset, pins)
        try:
            for detail in ("high", "low"):
                building[0] = detail
                O.build_p2_owner_runtime_context(REPO, name, detail)
            O.build_p2_owner_model_inventory(REPO, name)
            if pins.get("cross_low") == pins["cross_high"]:
                del pins["cross_low"]
            return pins
        except ValueError as error:
            text = str(error)
            if re.search(r"dense IDs exceed the 11-bit direct ABI", text):
                _drop_variant(name, pins, building[0])
                continue
            for pattern, apply in patterns:
                m = re.search(pattern, text)
                if m:
                    if apply is None:
                        raise SystemExit(f"learn: unhandled pin: {text}")
                    apply(m)
                    break
            else:
                m = re.search(r"restore(?:s| count)? (\d+)", text)
                raise SystemExit(f"learn: {text}")
    raise SystemExit("learn: pins did not converge")


def _drop_variant(name: str, pins: dict, detail: str) -> None:
    """The owner's dense vertices exceed the packed corner's 11-bit ID: leave
    out the largest model-part variant no donor script sets (then the largest
    left). A dropped part declines when the parent's code sets it; the pins
    name every one."""
    dropped = pins.setdefault("variants_dropped", {}).setdefault(detail, [])
    kept = [row for row in VARIANT_INFO[name][detail] if row[1] not in dropped]
    if not kept:
        raise SystemExit(f"learn: {name} {detail} exceeds the dense ID ABI without variants")
    victim = max(kept, key=lambda row: (not row[5], row[4]))
    dropped.append(victim[1])


def _set_gx(pins: dict, push: int, pop: int) -> None:
    pins["gx"][1:3] = [push, pop]
    for detail in ("high", "low"):
        pins[f"gx_{detail}"][1:3] = [push, pop]


def _set_detail(pins: dict, detail: str, index: int, value: int) -> None:
    pins[f"gx_{detail}"][index] = value


# The GX matrix stack's last physical slot: 0..15 are the camera seed and the
# hierarchy stack, so cross bindings own 16..30.
CROSS_SLOT_LAST = 30


def _add_cross(pins: dict, detail: str, binding: int) -> None:
    if "cross_low" not in pins:
        pins["cross_low"] = [list(row) for row in pins["cross_high"]]
    rows = pins[f"cross_{detail}"]
    if any(b == binding for b, _slot in rows):
        raise SystemExit(f"learn: {detail} binding {binding} already has a GX slot")
    if CROSS_SLOT_FIRST + len(rows) > CROSS_SLOT_LAST:
        raise SystemExit(f"learn: {detail} cross bindings exceed the GX palette")
    rows.append([binding, CROSS_SLOT_FIRST + len(rows)])


def _merge_light_preambles(name: str, high: dict, low: dict) -> None:
    """Both details' owners share the one root-light preamble table the high
    pass emits, so it is the union of both, with the low roots re-indexed into
    it (generate_nds_native_owners.py does the same for the original cast:
    Captain's low model has a preamble his high model lacks, as Wario's has)."""
    merged = list(high["light_preambles"])
    for preamble in low["light_preambles"]:
        if preamble not in merged:
            merged.append(preamble)
    if len(merged) > 0xFF:
        raise SystemExit(f"{name}: merged root-light preamble index exceeds u8")
    remap = [merged.index(preamble) for preamble in low["light_preambles"]]
    low["light_preamble_indices"] = [remap[i] for i in low["light_preamble_indices"]]
    high["light_preambles"] = merged
    low["light_preambles"] = merged
    low["high_light_preambles"] = merged


def emit(out_dir: Path, owners: dict[str, tuple[Path, int]]) -> dict[str, list[Path]]:
    """Write the build-local owner products for every enabled donor owner.

    `owners` is name -> (O2R directory, attributes offset) in registry order,
    which is the image slot order. For each owner: its runtime program (.inc,
    the same rows the original cast's owners get in
    src/nds/nds_native_fighter_owner.generated.inc), its two image TUs, and
    one shared header carrying every owner's image ABI rows. Image slots and
    renderer owner slots follow the original cast's (P4 owners append after
    NDS_NATIVE_IMAGE_OWNER_SLOTS and the renderer's base slot)."""
    import generate_nds_native_owner_images as images  # noqa: E402

    out_dir.mkdir(parents=True, exist_ok=True)
    P4_IMAGE_OWNERS = tuple(owners)
    products: dict[str, list[Path]] = {}
    header = [
        "/* Generated by scripts/p4/p4_native_owner.py. Build output: derived from the",
        " * user's ROM through the staged donor build. Do not edit. */",
        "#ifndef NDS_P4_NATIVE_IMAGE_GENERATED_H",
        "#define NDS_P4_NATIVE_IMAGE_GENERATED_H",
        "",
        "#include <nds/generated/nds_native_fighter_image.generated.h>",
        "",
    ]
    inc = ["/* Generated by scripts/p4/p4_native_owner.py. Do not edit. */", ""]
    rows = []
    for index, name in enumerate(P4_IMAGE_OWNERS):
        if name not in owners:
            continue
        o2r_dir, attr = owners[name]
        register(name, o2r_dir, attr, load_pins(name))
        guard = f"NDS_P4_{name.upper()}"
        contexts = {d: O.build_p2_owner_runtime_context(REPO, name, d) for d in ("high", "low")}
        _merge_light_preambles(name, contexts["high"], contexts["low"])
        if name in PROGRAM_INFO:
            for detail in ("high", "low"):
                contexts[detail]["root_programs"] = O.build_owner_root_programs(
                    REPO, contexts[detail])
        header += [f"#if {guard}",
                   f"#define NDS_NATIVE_IMAGE_SLOT_{name.upper()} (NDS_NATIVE_IMAGE_OWNER_SLOTS + {index}u)",
                   f"#define NDS_NATIVE_OWNER_IMAGE_{name.upper()} 1"]
        for detail in ("high", "low"):
            header += images.render_owner_image_types(name, detail, contexts[detail])
        header += ["#endif", ""]
        rows += [
            f"#if {guard}",
            f"#define NDS_P4_NATIVE_OWNER_IMAGE_ROW_{name.upper()}(X) \\",
            f"    X(NDS_NATIVE_IMAGE_SLOT_{name.upper()}, \"nitro:/fighters/{name}_high.bin\", "
            f"\"nitro:/fighters/{name}_low.bin\", {images._image_type(name, 'high')}, "
            f"{images._image_type(name, 'low')})",
            "#else",
            f"#define NDS_P4_NATIVE_OWNER_IMAGE_ROW_{name.upper()}(X)",
            "#endif",
        ]
        inc += [f"#if {guard}", f"/* P4: donor-derived {O._owner_title(name)} runtime owner. */", ""]
        for detail in ("high", "low"):
            inc += O.render_p2_owner_runtime_program(contexts[detail])
        # The model-part variants the program emitted (donor_variants), by
        # name for the renderer's resolver; NULL and 0 when it has none.
        for detail, suffix in (("high", ""), ("low", "Low")):
            count = len(O._p2_owner_variant_specs(name, detail))
            macro = f"NDS_P4_NATIVE_{name.upper()}_ROOT_VARIANTS{'_LOW' if detail == 'low' else ''}"
            array = f"sNdsNative{O._owner_title(name)}RootVariants{suffix}" if count else "NULL"
            inc += [f"#define {macro} {array}", f"#define {macro}_COUNT {count}u"]
        inc += _render_programs(name, contexts)
        inc += [f"#endif  /* {guard} */", ""]
        files = []
        for detail in ("high", "low"):
            text = images.render_image(name, detail, contexts[detail]).replace(
                "#include <nds/generated/nds_native_fighter_image.generated.h>",
                "#include <nds_p4_native_image.generated.h>")
            path = out_dir / f"nds_native_fighter_{name}_{detail}.image.c"
            path.write_text(text, encoding="utf-8", newline="\n")
            files.append(path)
        products[name] = files
    header += ["/* One row per P4 image owner (NDS_NATIVE_OWNER_IMAGE_ROWS's shape). */"]
    header += [r for r in rows]
    header += ["#define NDS_P4_NATIVE_OWNER_IMAGE_ROWS(X) \\"]
    header += [f"    NDS_P4_NATIVE_OWNER_IMAGE_ROW_{n.upper()}(X) \\" for n in P4_IMAGE_OWNERS if n in owners]
    header += ["    /* end */", f"#define NDS_P4_NATIVE_IMAGE_SLOTS {len(P4_IMAGE_OWNERS)}u", "",
               "#endif /* NDS_P4_NATIVE_IMAGE_GENERATED_H */", ""]
    (out_dir / "nds_p4_native_image.generated.h").write_text(
        "\n".join(header), encoding="utf-8", newline="\n")
    (out_dir / "nds_p4_native_owner.generated.inc").write_text(
        "\n".join(inc), encoding="utf-8", newline="\n")
    return products


def _render_programs(name: str, contexts: dict) -> list[str]:
    """The donor's root programs (donor_superset) for the renderer: the
    program list, and each program's canonical binding per live root, which
    keys its roots' model-part variants (the variant rows name canonical
    bindings; a program's roots after a removed one sit at lower ordinals)."""
    title = O._owner_title(name)
    upper = name.upper()
    programs = {d: contexts[d].get("root_programs", ()) for d in ("high", "low")}
    if not programs["high"]:
        return [f"#define NDS_P4_NATIVE_{upper}_PROGRAM_COUNT 0u",
                f"#define NDS_P4_NATIVE_{upper}_PROGRAMS(X)", ""]
    names = [p["name"] for p in programs["high"]]
    if names != [p["name"] for p in programs["low"]]:
        raise SystemExit(f"{name}: High and Low programs differ")
    lines = []
    labels = {row[0]: row[2] for row in PROGRAM_INFO.get(name, ())}
    for high, low in zip(programs["high"], programs["low"]):
        if tuple(high["root_joints"]) != tuple(low["root_joints"]):
            raise SystemExit(f"{name} {high['name']}: High/Low live joints differ "
                             f"{high['root_joints']} / {low['root_joints']}")
        shown = labels.get(high["name"], ())
        lines.append(f"/* {high['name']}: joints {', '.join(map(str, high['root_joints']))}; "
                     f"{len(shown)} motions ({', '.join(shown[:8])}{', ...' if len(shown) > 8 else ''}). */")
        for detail, program, suffix in (("high", high, ""), ("low", low, "Low")):
            rows = ", ".join(f"{b}u" for b in program["root_bindings"])
            lines.append(f"static const u8 sNdsNative{title}{program['name']}RootBindings{suffix}"
                         f"[{len(program['root_bindings'])}] = {{ {rows} }};")
    lines.append(f"#define NDS_P4_NATIVE_{upper}_PROGRAM_COUNT {len(names)}u")
    lines.append(f"#define NDS_P4_NATIVE_{upper}_PROGRAMS(X) \\")
    lines += [f"    X({title}, {upper}, {program}) \\" for program in names]
    lines += ["    /* end */", ""]
    return lines


def load_pins(name: str) -> dict | None:
    path = PINS_DIR / f"{name}.json"
    return json.loads(path.read_text(encoding="utf-8")) if path.exists() else None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--export-root", type=Path, required=True,
                    help="remix_export.py output root (each content's main/model ids)")
    ap.add_argument("--owner", help="one owner, for --learn or the pin check")
    ap.add_argument("--o2r", type=Path, help="generate_p4_fighter.py o2r/ directory (--owner)")
    ap.add_argument("--contents", help="--emit: the enabled contents, space separated")
    ap.add_argument("--gen-root", type=Path, help="--emit: the build's P4 directory (<name>/o2r)")
    ap.add_argument("--learn", action="store_true")
    ap.add_argument("--emit", type=Path, help="write the build-local owner products here")
    args = ap.parse_args()
    if args.emit:
        import p4_contents  # noqa: E402

        if args.contents is None or args.gen_root is None:
            ap.error("--emit needs --contents and --gen-root")
        owners = {}
        for row in p4_contents.enabled_rows(args.contents.split()):
            donor = load_donor(row["name"], args.export_root)
            owners[row["name"]] = (args.gen_root / row["name"] / "o2r", donor["attributes"])
        products = emit(args.emit, owners)
        for name, files in products.items():
            print(f"{name}: {[str(p.name) for p in files]}")
        return 0
    if args.owner is None or args.o2r is None:
        ap.error("--owner and --o2r are required without --emit")
    args.attributes = load_donor(args.owner, args.export_root)["attributes"]
    if args.learn:
        pins = learn(args.owner, args.o2r, args.attributes)
        PINS_DIR.mkdir(exist_ok=True)
        (PINS_DIR / f"{args.owner}.json").write_text(json.dumps(pins, indent=1) + "\n",
                                                     encoding="utf-8", newline="\n")
        print(json.dumps(pins))
        return 0
    pins = load_pins(args.owner)
    if pins is None:
        raise SystemExit(f"no pins for {args.owner}; run --learn once")
    register(args.owner, args.o2r, args.attributes, pins)
    for detail in ("high", "low"):
        O.build_p2_owner_runtime_context(REPO, args.owner, detail)
    O.build_p2_owner_model_inventory(REPO, args.owner)
    print(f"{args.owner}: native owner verified against pins")
    return 0


if __name__ == "__main__":
    sys.exit(main())

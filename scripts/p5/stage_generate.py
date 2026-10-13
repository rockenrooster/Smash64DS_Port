#!/usr/bin/env python3
"""P5 T4: a Remix stage's native packet, through the stage generator's own
pipeline in donor mode.

scripts/stages/generate_nds_native_stage.py builds a vanilla stage from a
hand-written descriptor (scripts/stages/native_stage_descriptors/): pinned
O2R inputs, the decomp's typed C as a contract, owner specs, material
sources, and counts pinned after the first generation so that any drift
falsifies. A Remix stage has no typed C, so its descriptor is DERIVED from
the export (scripts/p5/stage_export.py stage.json): the four display layers'
owner specs (DObjDesc array, live count, link 4/6/13/17, the primary or the
DLLink secondary callback from `layer_mask`), the MObjSub material sources,
and the header offset the light angle is read from. The pins -- counts,
segment partition, material partition, the O2R hashes and the include's
SHA256 -- live in scripts/p5/pins/<stage>.json; `--learn` writes them from a
generation and every later run must reproduce them exactly.

Outputs (all ROM-derived, so build tree only): the namespaced include the
runtime's counts and maxima read, the NitroFS blob, and the GX template.

    python scripts/p5/stage_generate.py --stage first_destination \
        --stages-root builds/<build>/p5/stages --runtime-out builds/<build>/p5 \
        [--learn]

Nothing here edits the shared generator: its four name-keyed tables (blob
gkind, rigid and animated masks, joint tables) take the donor's rows at run
time, the light-angle reader is wrapped for a header given by offset, and the
anim-joint reader is replaced by one that follows branches.
"""
from __future__ import annotations

import argparse
import ast
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))
sys.path.insert(0, str(REPO / "scripts"))
import generate_nds_native_stage as gen  # noqa: E402
import compile_nds_stage_gx as gx  # noqa: E402
from native_stage_descriptors import StageDescriptor  # noqa: E402

PINS = HERE / "pins"
LINKS = (4, 6, 13, 17)   # grdisplay.c dGRDisplayDescs: layer 0..3 display links
GRDISPLAY = "decomp/BattleShip-main/decomp/src/gr/grdisplay.c"
# The C names the runtime knows each Remix stage by (file stem, symbol stem):
# the registry's c_name and c_symbol.
sys.path.insert(0, str(HERE))
import p5_stages  # noqa: E402
CNAMES = {r["name"]: (r["c_name"], r["c_symbol"])
          for r in p5_stages.load_registry() if r["source"] == "remix"}


class DonorError(SystemExit):
    pass


def _aobj_script_moves_joint(resource, offset: int) -> bool:
    """gen.aobj_script_moves_joint, following branches.

    The shared reader refuses a Jump or SetAnim because no vanilla joint
    table has one. Dinosaur Land's movers loop that way: SetAnim back to the
    script's own start, or to a point inside it. The payload word is the next
    script through the fixup table; a command already walked closes the loop
    with nothing that moves the joint, and a NULL target stops the stream as
    the runtime does. Everything else is the shared reader's walk."""
    payload = resource.payload
    cursor = int(offset)
    walked = {cursor}
    for _ in range(gen.AOBJ_MAX_COMMANDS):
        if cursor < 0 or cursor + 4 > len(payload):
            raise gen.falsify(f"anim-joint script at 0x{offset:X} runs past "
                              f"{resource.spec.path}")
        word = struct.unpack_from(">I", payload, cursor)[0]
        opcode = (word >> gen.AOBJ_OPCODE_SHIFT) & gen.AOBJ_OPCODE_MASK
        flags = (word >> gen.AOBJ_FLAGS_SHIFT) & gen.AOBJ_FLAGS_MASK
        if opcode in gen.AOBJ_JOINT_TRACK_OPCODES and flags != 0:
            return True
        if opcode == gen.AOBJ_EVENT_END:
            return False
        if opcode in gen.AOBJ_BRANCH_OPCODES:
            target = resource.pointer_at(cursor + 4)
            if target is None:
                return False
            if target.asset_id != resource.file_id:
                raise gen.falsify(f"anim-joint script at 0x{offset:X} branches into "
                                  f"file 0x{target.asset_id:X}")
            if target.offset in walked:
                return False
            cursor = target.offset
            walked.add(cursor)
            continue
        words = 0
        if opcode in gen.AOBJ_JOINT_TRACK_OPCODES:
            words = bin(flags).count("1")
            if opcode in gen.AOBJ_DOUBLE_VALUE_OPCODES:
                words *= 2
            if opcode == 13:  # SetInterp on a DObj carries one descriptor word
                words = 1
        cursor += 4 * (1 + words)
        walked.add(cursor)
    raise gen.falsify(f"anim-joint script at 0x{offset:X} has no End command")


gen.aobj_script_moves_joint = _aobj_script_moves_joint


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _rel(path: Path) -> str:
    return path.resolve().relative_to(REPO.resolve()).as_posix()


def _o2r_entry(path: Path, fid: int) -> dict:
    spec = gen.InputSpec(_rel(path), _sha256(path.read_bytes()), fid)
    res = gen.load_o2r(REPO, spec)
    return {"path": spec.path, "sha256": spec.sha256, "file_id": fid,
            "internal_fixups": len(res.internal), "external_fixups": len(res.external),
            "payload_sha256": _sha256(res.payload)}


def _material_sources(stage: dict, resources: dict[int, "gen.O2RResource"]) -> tuple:
    """(asset_id, binding_root, mobj_offset, segment_index) per MObjSub, as
    the vanilla descriptors list them: layer by layer, DObj by DObj, and
    segment 8*i for the DObj's MObj i (gcDrawMObjForDObj's branch slot). A
    DLLink DObj's MObjs feed each of its lists."""
    rows = []
    for layer in stage["layers"]:
        tables = layer["p_mobjsubs"]
        if tables is None or layer["dobjdesc"] is None:
            continue
        desc_file, desc_off = layer["dobjdesc"]
        dres = resources[desc_file]
        tres = resources[tables[0]]
        for j in range(layer["dobj_count"]):
            dl = dres.pointer_at(desc_off + j * 0x2C + 4)
            lst = tres.pointer_at(tables[1] + j * 4)
            if dl is None or lst is None:
                continue
            roots = []
            if layer["secondary"]:
                lres = resources[dl.asset_id]
                o = dl.offset
                while True:
                    head = struct.unpack_from(">I", lres.payload, o)[0]
                    if head == 4:
                        break
                    tgt = lres.pointer_at(o + 4)
                    if tgt is not None:
                        roots.append(tgt.offset)
                    o += 8
            else:
                roots.append(dl.offset)
            lres = resources[lst.asset_id]
            subs = []
            k = 0
            while True:
                sub = lres.pointer_at(lst.offset + k * 4)
                if sub is None:
                    break
                subs.append(sub)
                k += 1
            for root in roots:
                for k, sub in enumerate(subs):
                    rows.append((dl.asset_id if not layer["secondary"] else desc_file,
                                 root, sub.offset, 8 * k))
    return tuple(rows)


def build_descriptor(stage: dict, stage_dir: Path, pins: dict | None) -> StageDescriptor:
    name, cname = CNAMES[stage["name"]]
    files = stage["files"]
    map_fid = stage["header"]["file"]
    geo_fid = stage["map_geometry"][0]
    o2r = {
        "stage_geometry": _o2r_entry(stage_dir / "o2r" / f"{geo_fid:04x}", geo_fid),
        "stage_map": _o2r_entry(stage_dir / "o2r" / f"{map_fid:04x}", map_fid),
    }
    by_file = {geo_fid: "stage_geometry", map_fid: "stage_map"}
    owners, callbacks = [], []
    for layer in stage["layers"]:
        if layer["dobjdesc"] is None:
            continue
        fid, off = layer["dobjdesc"]
        if fid not in by_file:
            raise DonorError(f"{stage['name']}: layer {layer['index']} DObjs in file {fid:#x}")
        i = layer["index"]
        cb = f"grDisplayLayer{i}{'Sec' if layer['secondary'] else 'Pri'}ProcDisplay"
        owners.append((i, f"layer{i}", by_file[fid], off, layer["dobj_count"] + 1, LINKS[i], cb,
                       bool(layer["secondary"])))
        callbacks.append((f"layer{i}", cb, LINKS[i]))
    resources = {fid: gen.load_o2r(REPO, gen.InputSpec(e["path"], e["sha256"], fid))
                 for fid, e in ((o2r[k]["file_id"], o2r[k]) for k in o2r)}
    materials = _material_sources(stage, resources)
    pins = pins or {}
    grd = REPO / GRDISPLAY
    geo_size = len(resources[geo_fid].payload)
    map_size = len(resources[map_fid].payload)
    return StageDescriptor(
        name=name,
        include_sha=pins.get("include_sha", "TO_BE_FILLED"),
        generated_segment_index=-1,
        symbol_prefix=cname,
        macro_prefix=name.upper() + "_",
        expected_counts=dict(pins.get("expected_counts", {})),
        o2r_inputs=o2r,
        text_inputs={"grdisplay": {"path": GRDISPLAY, "sha256": _sha256(grd.read_bytes())}},
        text_contract_tokens={"grdisplay": ("grDisplayMakeGeometryLayer",)},
        map_constructor_text_key="grdisplay",
        map_constructor_token="grDisplayMakeGeometryLayer(",
        map_constructor_min_count=1,
        asset_order=(("stage_geometry", 1), ("stage_map", 4)),
        owner_specs=tuple(owners),
        material_sources=materials,
        material_command_partition=tuple(pins.get("material_command_partition", ())),
        segment_partition=tuple(tuple(r) for r in pins.get("segment_partition", ())),
        callback_partition=tuple(callbacks),
        segment0={},
        adapter_segment_count=len(owners),
        adapter_dobj_count=int(pins.get("expected_counts", {}).get("dobjs", 0)),
        adapter_binding_count=int(pins.get("expected_counts", {}).get("bindings", 0)),
        adapter_asset_count=2,
        adapter_material_count=len(materials),
        adapter_asset_ids=(geo_fid, map_fid),
        adapter_asset_sizes=(geo_size, map_size),
    )


# A Remix stage's GX template may run past the vanilla VS stages' 36,992-byte
# body: the runtime's own check is the free heap, keeping its floor, and it
# draws the stage uncompiled when a body does not fit (nds_stage_gx.exec.inc).
DONOR_BODY_MAX = 40 * 1024


def trim_dense_vertices(packet):
    """Drop the dense vertices no triangle corner names.

    The generator keeps every vertex a G_VTX loads, and Remix's stages load
    overlapping ranges: Hyrule Temple loads 4,317 for 527 triangles. Each
    binding keeps its referenced vertices in load order -- its range stays
    contiguous and inside the packet, which is all the runtime validates
    (nds_renderer_native_owners.c, the binding vertex range) -- and every
    corner is renumbered. A vertex keeps its own fields (matrix binding,
    cache slot, colour), so a corner that reaches back into an earlier
    binding's load still resolves to the same data. Packets with clone
    vertices outside the bindings' load ranges are left whole."""
    import dataclasses
    covered = sum(b.source_vertex_count for b in packet.bindings)
    if covered != len(packet.vertices):
        return packet
    cursor = 0
    for b in packet.bindings:
        if b.first_vertex != cursor:
            return packet
        cursor += b.source_vertex_count
    used = set(packet.corners)
    remap = {}
    vertices = []
    bindings = []
    for b in packet.bindings:
        first = len(vertices)
        for old in range(b.first_vertex, b.first_vertex + b.source_vertex_count):
            if old in used:
                remap[old] = len(vertices)
                vertices.append(packet.vertices[old])
        bindings.append(dataclasses.replace(b, first_vertex=first,
                                            source_vertex_count=len(vertices) - first))
    return dataclasses.replace(packet, vertices=tuple(vertices), bindings=tuple(bindings),
                               corners=tuple(remap[c] for c in packet.corners))


_DONOR_HEADERS: dict[str, tuple[int, int]] = {}
_light_angles = gen.stage_light_angles


def _donor_light_angles(desc, texts, resources):
    """MPGroundData.light_angle (+0x60) from the donor's header offset."""
    hdr = _DONOR_HEADERS.get(getattr(desc, "name", ""))
    if hdr is None:
        return _light_angles(desc, texts, resources)
    res = resources.get("stage_map")
    if res is None or hdr[1] + 0x6C > len(res.payload):
        raise DonorError(f"{desc.name}: map header {hdr[1]:#x} runs past the map payload")
    return struct.unpack_from(">ff", res.payload, hdr[1] + 0x60)


gen.stage_light_angles = _donor_light_angles

# Falsifier messages that name the actual value, in the order generation meets them.
_LEARN = (
    (r"selected bindings (\d+) != ", lambda m, p: p["expected_counts"].__setitem__("bindings", int(m[1]))),
    (r"live DObjs (\d+) != ", lambda m, p: p["expected_counts"].__setitem__("dobjs", int(m[1]))),
    (r"segment-E command partition changed: (\(.*\))",
     lambda m, p: p.__setitem__("material_command_partition", list(ast.literal_eval(m[1])))),
    (r"MODIFYVTX commands (\d+) != ",
     lambda m, p: p["expected_counts"].__setitem__("modify_vertex_commands", int(m[1]))),
    (r"alpha ramp/uniform triangles \((\d+), (\d+)\) != ",
     lambda m, p: (p["expected_counts"].__setitem__("alpha_ramp_triangles", int(m[1])),
                   p["expected_counts"].__setitem__("alpha_uniform_triangles", int(m[2])))),
    (r"packet counts (\(.*?\)) != ", lambda m, p: p["expected_counts"].update(dict(zip(
        ("callbacks", "dobjs", "bindings", "commands", "vertex_commands", "source_vertices",
         "triangle_commands", "triangles", "runs", "texture_epochs", "material_events"),
        ast.literal_eval(m[1]))))),
    (r"dense vertices (\d+) != ", lambda m, p: p["expected_counts"].__setitem__(
        "alpha_clone_vertices", int(m[1]) - p["expected_counts"]["source_vertices"]
        - p["expected_counts"]["modify_vertex_commands"])),
    (r"submit classes (\(.*?\)) != ",
     lambda m, p: p["expected_counts"].__setitem__("submit_classes", list(ast.literal_eval(m[1])))),
    (r"segment partition (\(.*\)) != ",
     lambda m, p: p.__setitem__("segment_partition", [list(r) for r in ast.literal_eval(m[1])])),
    (r"state packet counts \((\d+), (\d+), (\d+), (\d+)\) != ", lambda m, p: p["expected_counts"].update(
        {"state_deltas": int(m[1]), "state_events": int(m[2]), "sync_events": int(m[4])})),
    (r"cross-matrix counts \((\d+), (\d+), (\d+), (\d+)\) != ", lambda m, p: p["expected_counts"].update(
        {"cross_runs": int(m[1]), "cross_tris": int(m[2]), "cross_corners": int(m[3])})),
    (r"alpha ramp triangles (\d+) != ",
     lambda m, p: p["expected_counts"].__setitem__("alpha_ramp_triangles", int(m[1]))),
)
_ZERO_COUNTS = ("callbacks", "dobjs", "bindings", "commands", "vertex_commands", "source_vertices",
                "modify_vertex_commands", "triangle_commands", "triangles", "runs",
                "texture_epochs", "material_events", "cross_runs", "cross_tris",
                "cross_corners", "state_events", "state_deltas", "sync_events")


def learn(stage: dict, stage_dir: Path) -> dict:
    pins = {"expected_counts": {k: 0 for k in _ZERO_COUNTS},
            "material_command_partition": [], "segment_partition": []}
    pins["expected_counts"]["submit_classes"] = [0, 0, 0]
    for _attempt in range(40):
        desc = build_descriptor(stage, stage_dir, pins)
        try:
            gen.generate(REPO, desc)
            break
        except gen.Falsifier as exc:
            text = str(exc)
            for pattern, apply in _LEARN:
                # Anchored: "state packet counts" also contains "packet counts".
                m = re.search("FALSIFIER: " + pattern, text)
                if m:
                    apply(m, pins)
                    break
            else:
                raise DonorError(f"{stage['name']}: generation fails: {text}")
    else:
        raise DonorError(f"{stage['name']}: the pins did not converge")
    desc = build_descriptor(stage, stage_dir, pins)
    packet = trim_dense_vertices(gen.generate(REPO, desc))
    pins["include_sha"] = _sha256(gen.render_include(packet, desc))
    pins["o2r"] = {k: v["sha256"] for k, v in desc.o2r_inputs.items()}
    return pins


def build_stage(stage: dict, stage_dir: Path, learn_pins: bool) -> dict:
    """Generate one Remix stage's packet (learning its pins when asked) and
    write its include, blob, GX template and converted wallpaper into its
    export directory. Returns the facts the runtime's C rows need."""
    name, cname = CNAMES[stage["name"]]
    gen._BLOB_GKIND[name] = stage["gkind"]
    if name not in gx.STAGES:
        gx.STAGES = tuple(gx.STAGES) + (name,)
        vanilla_body_max = gx.body_max
        gx.body_max = (lambda n, f=vanilla_body_max, d=name:
                       DONOR_BODY_MAX if n == d else f(n))
    _DONOR_HEADERS[name] = (stage["header"]["file"], stage["header"]["offset"])
    # A layer whose AnimJoint table moves its DObjs (Dinosaur Land's movers)
    # keeps those bindings out of the baked constant world, as Saffron City's
    # gate does: the generator reads the joint tables by owner name.
    files = {stage["map_geometry"][0]: "stage_geometry", stage["header"]["file"]: "stage_map"}
    joints = tuple((f"layer{layer['index']}", files[layer["anim_joints"][0]], layer["anim_joints"][1])
                   for layer in stage["layers"]
                   if layer["anim_joints"] is not None and layer["dobjdesc"] is not None
                   and layer["anim_joints"][0] in files)
    if joints:
        gen._ANIMATED_JOINT_TABLES[name] = joints
    pin_path = PINS / f"{stage['name']}.json"
    if learn_pins:
        pins = learn(stage, stage_dir)
        PINS.mkdir(parents=True, exist_ok=True)
        pin_path.write_text(json.dumps(pins, indent=1) + "\n", encoding="utf-8", newline="\n")
        print(f"stage_generate: learned {pin_path}")
    if not pin_path.is_file():
        raise DonorError(f"{pin_path} absent: run with --learn once")
    pins = json.loads(pin_path.read_text(encoding="utf-8"))
    desc = build_descriptor(stage, stage_dir, pins)
    for key, entry in desc.o2r_inputs.items():
        if pins.get("o2r", {}).get(key) != entry["sha256"]:
            raise DonorError(f"{stage['name']}: {key} O2R differs from its pin (a different Remix build?)")
    try:
        whole = gen.generate(REPO, desc)
        packet = trim_dense_vertices(whole)
        if joints:
            resources = {key: gen.load_o2r(REPO, gen.InputSpec(e["path"], e["sha256"], e["file_id"]))
                         for key, e in desc.o2r_inputs.items()}
            gen._ANIMATED_BINDING_MASKS[name] = gen.derive_animated_binding_mask(
                resources, packet, desc)
        include = gen.render_include(packet, desc)
        if _sha256(include) != desc.include_sha:
            raise DonorError(f"{stage['name']}: include SHA256 {_sha256(include)} != pin {desc.include_sha}")
        blob = gen.build_stage_blob(packet, desc)
        gxp = gx.compile_packet(packet, name)
        gx.decode(gxp)
    except gen.Falsifier as exc:
        raise DonorError(f"{stage['name']}: {exc}")
    from p5_stages import write_if_changed  # noqa: E402
    write_if_changed(stage_dir / f"nds_native_stage_{name}.generated.inc", include)
    write_if_changed(stage_dir / f"native_stage_{name}.bin", blob)
    write_if_changed(stage_dir / f"{name}.gxp", gxp)
    wallpaper = convert_wallpaper(stage, stage_dir, name)
    nitro = stage_nitro_files(stage, stage_dir)
    owners = {o[0]: o for o in desc.owner_specs}
    capture = []
    for seg in packet.segments:
        owner = owners[seg.owner]
        capture.append((seg.owner, seg.link, seg.owner, seg.dobj_count, seg.owner,
                        1 if owner[7] else 0))
    print(f"stage_generate: {stage['name']} as {name}: dobjs {len(packet.dobjs)} bindings "
          f"{len(packet.bindings)} runs {len(packet.runs)} triangles {len(packet.corners) // 3} "
          f"dense {len(packet.vertices)} (of {len(whole.vertices)} loaded) slab {packet.slab_bytes()} "
          f"blob {len(blob)} gxp {len(gxp)}")
    return {
        "gkind": stage["gkind"], "name": name, "cname": cname,
        "map_file": stage["header"]["file"], "map_offset": stage["header"]["offset"],
        "files": nitro,
        "wallpaper_file": stage["wallpaper"][0],
        "segments": len(packet.segments), "dobjs": len(packet.dobjs),
        "bindings": len(packet.bindings), "runs": len(packet.runs),
        "dense": len(packet.vertices), "materials": len(packet.materials),
        "assets": list(desc.adapter_asset_ids), "sizes": list(desc.adapter_asset_sizes),
        "capture": capture, "wallpaper": wallpaper,
    }


def stage_nitro_files(stage: dict, stage_dir: Path) -> list[int]:
    """The files the build stages in nitro:/reloc/p4, under <stage>/nitro/:
    the map as a compact ground map and the rest of its tree but the
    wallpaper container. A map's extern tree is what the battle loads, and
    the container's 158,928 bytes of pixels -- which the DS never reads; its
    battle wallpaper is the converted image -- left four fighters no heap
    (First Destination, 2026-10-13). The compaction is the vanilla stages'
    own (scripts/stages/generate_compact_ground_maps.py): the map carries the
    container's Sprite and Bitmap table (pixels NULL) and drops it from its
    extern list, and the runtime keys the wallpaper back to the container."""
    import generate_compact_ground_maps as gcm  # noqa: E402
    from p5_stages import write_if_changed  # noqa: E402
    map_fid = stage["header"]["file"]
    wall_fid = stage["wallpaper"][0]
    o2r = stage_dir / "o2r"
    spec = gcm.CompactMapSpec(stage["name"], stage["gkind"], f"{map_fid:04x}", map_fid,
                              f"{wall_fid:04x}", wall_fid, wall_fid, stage["header"]["offset"])
    compact = gcm.build_compact_map(spec, (o2r / f"{map_fid:04x}").read_bytes(),
                                    (o2r / f"{wall_fid:04x}").read_bytes())
    nitro = stage_dir / "nitro"
    nitro.mkdir(parents=True, exist_ok=True)
    write_if_changed(nitro / f"{map_fid:04x}", compact.data)
    files = [map_fid]
    for fid in stage["files"]:
        if fid in (map_fid, wall_fid):
            continue
        write_if_changed(nitro / f"{fid:04x}", (o2r / f"{fid:04x}").read_bytes())
        files.append(fid)
    return sorted(files)


def convert_wallpaper(stage: dict, stage_dir: Path, name: str) -> dict:
    """The stage's 300x220 wallpaper at the DS's 4/5, as the vanilla arenas'
    are converted (scripts/stages/generate_native_wallpapers.py), keyed by
    the donor file the loaded Sprite's bitmap belongs to."""
    import generate_native_wallpapers as wp  # noqa: E402
    fid, off = stage["wallpaper"]
    source = wp.WallpaperSource(f"p5_{name}", str((stage_dir / "o2r" / f"{fid:04x}").resolve()),
                                fid, off, True)
    asset = wp.convert_source(REPO, source)
    from p5_stages import write_if_changed  # noqa: E402
    write_if_changed(stage_dir / asset.filename, asset.payload)
    return {"file": fid, "bitmap": asset.bitmap_offset, "src_w": asset.src_w, "src_h": asset.src_h,
            "native_w": asset.native_w, "native_h": asset.native_h, "format": asset.format,
            "filename": asset.filename}


def write_runtime(out: Path, facts: list[dict]) -> None:
    """The C rows the runtime reads for the Remix stages this build carries:
    nds_p5_native_stages.generated.h (rows, the reloc files, the packet
    maxima folded into the blob maxima) and nds_p5_adapter.generated.inc (the
    adapter's capture and descriptor tables, as emit_native_stage_runtime_rows
    prints them for a vanilla stage)."""
    from p5_stages import write_if_changed  # noqa: E402
    files = sorted({f for s in facts for f in s["files"]})
    h = ["/* Generated by scripts/p5/stage_generate.py. Do not edit. */",
         "#ifndef NDS_P5_NATIVE_STAGES_GENERATED_H",
         "#define NDS_P5_NATIVE_STAGES_GENERATED_H",
         "",
         "/* X(gkind, Cname, name, map_file, map_offset): the Remix stages whose",
         " * packet, files and wallpaper this build carries. */",
         "#define NDS_P5_NATIVE_STAGE_ROWS(X) \\"]
    h += [f"    X({s['gkind']}, {s['cname']}, {s['name']}, {s['map_file']:#x}u, {s['map_offset']:#x}u) \\"
          for s in facts]
    h += ["    /* end */", "",
          "/* Their donor files, ascending (nitro:/reloc/p4/<id>). */",
          f"#define NDS_P5_RELOC_FILE_COUNT {len(files)}u",
          "#define NDS_P5_RELOC_FILE_IDS " + ", ".join(f"{f:#x}u" for f in files), "",
          "/* X(map file, wallpaper container, header offset): their compact",
          " * ground maps, as reloc_backend_assets.c lists the vanilla stages'. */",
          "#define NDS_P5_COMPACT_MAP_ROWS(X) \\"]
    h += [f"    X({s['map_file']:#x}u, {s['wallpaper_file']:#x}u, {s['map_offset']:#x}u) \\" for s in facts]
    h += ["    /* end */", ""]
    if facts:
        h += ["/* Their packets join the blob maxima the workspaces are sized by. */"]
        for key, field in (("SEGMENT_COUNT", "segments"), ("RUN_COUNT", "runs"),
                           ("DENSE_VERTEX_COUNT", "dense")):
            v = max(s[field] for s in facts)
            h += [f"#if defined(NDS_NATIVE_STAGE_BLOB_MAX_{key}) && (NDS_NATIVE_STAGE_BLOB_MAX_{key} < {v}u)",
                  f"#undef NDS_NATIVE_STAGE_BLOB_MAX_{key}",
                  f"#define NDS_NATIVE_STAGE_BLOB_MAX_{key} {v}u",
                  "#endif"]
    h += ["", "#endif", ""]
    write_if_changed(out / "nds_p5_native_stages.generated.h", "\n".join(h))

    a = ["/* Generated by scripts/p5/stage_generate.py. Do not edit. */",
         "/* The Remix stages' adapter rows (renderer_adapter_matrix.c). */"]
    for s in facts:
        layer0 = sum(1 for c in s["capture"] if c[1] == 4)
        a.append("static const NDSRendererAdapterNativeStageCaptureSegment")
        a.append(f"    sNdsRendererAdapterNativeStageCapture{s['cname']}[{len(s['capture'])}] = {{")
        rows = []
        for (index, link, layer, dobjs, owner, dl_links) in s["capture"]:
            rows.append(f"        {{ NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, {index}u, {link:2d}u, "
                        f"{layer}u, {dobjs:3d}u, {owner}u, {dl_links}u }}")
        a.append(",\n".join(rows))
        a.append("    };")
        assets = s["assets"] + [0] * (4 - len(s["assets"]))
        sizes = s["sizes"] + [0] * (4 - len(s["sizes"]))
        a.append("static const NDSRendererAdapterNativeStageDescriptor")
        a.append(f"    sNdsRendererAdapterNativeStage{s['cname']} = {{")
        a.append(f"        {s['segments']}u, {s['dobjs']}u, {s['bindings']}u, {len(s['assets'])}u, {s['materials']}u,")
        a.append("        { " + ", ".join(f"{x}u" for x in assets) + " },")
        a.append("        { " + ", ".join(f"{x}u" for x in sizes) + " },")
        a.append(f"        sNdsRendererAdapterNativeStageCapture{s['cname']},")
        a.append(f"        {len(s['capture'])}u,")
        a.append(f"        {layer0}u")
        a.append("    };")
    a.append("/* X(gkind, Cname) for the adapter table. */")
    a.append("#define NDS_P5_ADAPTER_ROWS(X) \\")
    a += [f"    X({s['gkind']}, {s['cname']}) \\" for s in facts]
    a += ["    /* end */", ""]
    write_if_changed(out / "nds_p5_adapter.generated.inc", "\n".join(a))

    w = ["/* Generated by scripts/p5/stage_generate.py. Do not edit. */",
         "/* The Remix stages' converted battle wallpapers (nds_native_wallpaper.c),",
         " * rows as generate_native_wallpapers.py writes the vanilla ones. */",
         "static const NDSNativeWallpaper kNDSNativeWallpapersP5[] = {"]
    for s in facts:
        x = s["wallpaper"]
        w.append(f'    {{ 0x{x["file"]:X}, 0x{x["file"]:X}, 0x{x["bitmap"]:05X}, {x["src_w"]}, {x["src_h"]}, '
                 f'{x["native_w"]}, {x["native_h"]}, {x["format"]}, "{x["filename"]}" }}, // {s["name"]}')
    if not facts:
        w.append("    { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, \"\" }")
    w += ["};", f"static const uint32_t kNDSNativeWallpaperP5Count = {len(facts)}u;", ""]
    write_if_changed(out / "nds_p5_wallpapers.generated.inc", "\n".join(w))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--stages-root", type=Path, required=True,
                    help="scripts/p5/stage_export.py's --out (one directory per stage)")
    ap.add_argument("--runtime-out", type=Path, required=True,
                    help="where the C rows go (the build's p5 directory)")
    ap.add_argument("--stage", action="append", default=[],
                    help="registry name; default every landed Remix stage")
    ap.add_argument("--learn", action="store_true",
                    help="derive and write scripts/p5/pins/<stage>.json first")
    args = ap.parse_args()
    rows = [r for r in p5_stages.load_registry() if r["source"] == "remix"]
    if args.stage:
        rows = [r for r in rows if r["name"] in set(args.stage)]
    else:
        rows = [r for r in rows if r["landed"]]
    facts = []
    for row in rows:
        stage_dir = args.stages_root / row["name"]
        stage = json.loads((stage_dir / "stage.json").read_text(encoding="utf-8"))
        facts.append(build_stage(stage, stage_dir, args.learn))
    write_runtime(args.runtime_out, facts)
    return 0


if __name__ == "__main__":
    sys.exit(main())

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
        --stage-dir builds/<build>/p5/stages/first_destination \
        --out-dir builds/<build>/p5/stages/first_destination [--learn]

Nothing here edits the shared generator: its four name-keyed tables (blob
gkind, rigid and animated masks, joint tables) take the donor's rows at run
time, and the light-angle reader is wrapped for a header given by offset.
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
# The C names the runtime knows each Remix stage by (symbol and macro stems).
CNAMES = {
    "first_destination": ("firstdest", "FirstDest"),
    "bowsers_stadium": ("bowserst", "BowserSt"),
    "hyrule_temple": ("hyruletemple", "HyruleTemple"),
    "dinosaur_land": ("dinoland", "DinoLand"),
}


class DonorError(SystemExit):
    pass


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
                m = re.search(pattern, text)
                if m:
                    apply(m, pins)
                    break
            else:
                raise DonorError(f"{stage['name']}: generation fails: {text}")
    else:
        raise DonorError(f"{stage['name']}: the pins did not converge")
    desc = build_descriptor(stage, stage_dir, pins)
    packet = gen.generate(REPO, desc)
    pins["include_sha"] = _sha256(gen.render_include(packet, desc))
    pins["o2r"] = {k: v["sha256"] for k, v in desc.o2r_inputs.items()}
    return pins


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--stage", required=True, choices=sorted(CNAMES))
    ap.add_argument("--stage-dir", type=Path, required=True,
                    help="the stage's export (stage.json, o2r/)")
    ap.add_argument("--out-dir", type=Path, required=True)
    ap.add_argument("--learn", action="store_true",
                    help="derive and write scripts/p5/pins/<stage>.json")
    args = ap.parse_args()
    stage = json.loads((args.stage_dir / "stage.json").read_text(encoding="utf-8"))
    name, _cname = CNAMES[args.stage]
    gen._BLOB_GKIND[name] = stage["gkind"]
    _DONOR_HEADERS[name] = (stage["header"]["file"], stage["header"]["offset"])
    pin_path = PINS / f"{args.stage}.json"
    if args.learn:
        pins = learn(stage, args.stage_dir)
        PINS.mkdir(parents=True, exist_ok=True)
        pin_path.write_text(json.dumps(pins, indent=1) + "\n", encoding="utf-8", newline="\n")
        print(f"stage_generate: learned {pin_path}")
    if not pin_path.is_file():
        raise DonorError(f"{pin_path} absent: run with --learn once")
    pins = json.loads(pin_path.read_text(encoding="utf-8"))
    desc = build_descriptor(stage, args.stage_dir, pins)
    for key, entry in desc.o2r_inputs.items():
        if pins.get("o2r", {}).get(key) != entry["sha256"]:
            raise DonorError(f"{args.stage}: {key} O2R differs from its pin (a different Remix build?)")
    try:
        packet = gen.generate(REPO, desc)
        include = gen.render_include(packet, desc)
        if _sha256(include) != desc.include_sha:
            raise DonorError(f"{args.stage}: include SHA256 {_sha256(include)} != pin {desc.include_sha}")
        blob = gen.build_stage_blob(packet, desc)
        gxp = gx.compile_packet(packet, name)
        gx.decode(gxp)
    except gen.Falsifier as exc:
        raise DonorError(f"{args.stage}: {exc}")
    out = args.out_dir
    out.mkdir(parents=True, exist_ok=True)
    from p5_stages import write_if_changed  # noqa: E402
    write_if_changed(out / f"nds_native_stage_{name}.generated.inc", include)
    write_if_changed(out / f"native_stage_{name}.bin", blob)
    write_if_changed(out / f"{name}.gxp", gxp)
    print(f"stage_generate: {args.stage} as {name}: dobjs {len(packet.dobjs)} bindings "
          f"{len(packet.bindings)} runs {len(packet.runs)} triangles {len(packet.corners) // 3} "
          f"slab {packet.slab_bytes()} blob {len(blob)} gxp {len(gxp)} -> {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

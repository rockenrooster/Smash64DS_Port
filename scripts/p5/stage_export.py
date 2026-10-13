#!/usr/bin/env python3
"""P5 T3: export a Remix stage from the staged Smash Remix build.

For each stage row in scripts/p5/stages.json whose source is "remix", read
the stage's map header from the staged ROM (Stages.asm `stage_file_table`:
map file and MPGroundData offset), follow its relocations to the file
closure (map, geometry, wallpaper), and write under <out>/<name>/:

    o2r/<id as 4 hex digits>   each file of the closure as an O2R container,
                               the reloc loader's own format (scripts/p4/
                               remix_rom.py Rom.o2r)
    stage.json                 the resolved header: each layer's DObjDesc,
                               AnimJoint, MObjSub and MatAnim tables as
                               (file, offset), the collision geometry, layer
                               mask, wallpaper, fog, emblem colours, light
                               angle, bounds, song, item weights, whistle
                               altitude, and the Remix table rows

Everything under <out> derives from the user's ROM: a build directory only.
Two runs write identical bytes (checked by --verify-repeat).

    python scripts/p5/stage_export.py --staging builds/p4-staging/extra-96621af \
        --out builds/<build>/p5/stages [--stage first_destination]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "p4"))
import remix_rom  # noqa: E402
import p5_stages  # noqa: E402

DOBJ_DESC_BYTES = 0x2C   # objtypes.h DObjDesc: id, dl, translate, rotate, scale
DOBJ_ARRAY_END = 18      # objtypes.h DOBJ_ARRAY_MAX: the array's sentinel id
HEADER_BYTES = 0xA8      # mptypes.h MPGroundData


class ExportError(SystemExit):
    pass


class Slots:
    """(file, offset) a relocated pointer word points at, per file."""

    def __init__(self, rom: remix_rom.Rom):
        self.rom = rom
        self._slots: dict[int, dict] = {}

    def __call__(self, fid: int, off: int):
        if fid not in self._slots:
            self._slots[fid] = self.rom.reloc_slots(fid)
        s = self._slots[fid].get(off)
        if s is None:
            return None
        return (fid, s[1]) if s[0] == "intern" else (s[1], s[2])


def _ref(ref):
    return None if ref is None else [ref[0], ref[1]]


def dobj_count(rom: remix_rom.Rom, ref) -> int:
    """Live DObjDesc entries before the array's sentinel."""
    data = rom.file_bytes(ref[0])
    off, count = ref[1], 0
    while True:
        if off + DOBJ_DESC_BYTES > len(data):
            raise ExportError(f"file {ref[0]:#x}: DObjDesc array at {ref[1]:#x} runs off the file")
        if struct.unpack_from(">i", data, off)[0] == DOBJ_ARRAY_END:
            return count
        count += 1
        off += DOBJ_DESC_BYTES


def _default_music(rom: remix_rom.Rom, table, rid: int):
    if table is None:
        return None
    value = struct.unpack(">h", rom.read_ram(table("default_music_track_table") + rid * 2, 2))[0]
    return (value - 1) if value != 0 else None


def closure(rom: remix_rom.Rom, first: int) -> list[int]:
    seen, todo = set(), [first]
    while todo:
        f = todo.pop()
        if f not in seen:
            seen.add(f)
            todo.extend(x for x in rom.extern_ids(f) if x not in seen)
    return sorted(seen)


def export_stage(rom: remix_rom.Rom, u32, u8, f32, row: dict, out: Path,
                 table=None) -> dict:
    rid = row["remix_id"]
    map_fid = u32("stage_file_table", rid * 2)
    hoff = u32("stage_file_table", rid * 2 + 1)
    if u8("class_table", rid) != 0:
        raise ExportError(f"{row['name']}: Remix class {u8('class_table', rid)} is not BATTLE")
    slot = Slots(rom)
    data = rom.file_bytes(map_fid)
    if hoff + HEADER_BYTES > len(data):
        raise ExportError(f"{row['name']}: header {hoff:#x} runs past map file {map_fid:#x}")
    mask = data[hoff + 0x44]
    layers = []
    for i in range(4):
        base = hoff + i * 16
        dd = slot(map_fid, base)
        layers.append({
            "index": i,
            "dobjdesc": _ref(dd),
            "dobj_count": dobj_count(rom, dd) if dd is not None else 0,
            "anim_joints": _ref(slot(map_fid, base + 4)),
            "p_mobjsubs": _ref(slot(map_fid, base + 8)),
            "p_matanim_joints": _ref(slot(map_fid, base + 12)),
            "secondary": bool(mask & (1 << i)),
        })
    geometry = slot(map_fid, hoff + 0x40)
    wallpaper = slot(map_fid, hoff + 0x48)
    if geometry is None or wallpaper is None:
        raise ExportError(f"{row['name']}: header has no collision geometry or wallpaper")
    files = closure(rom, map_fid)
    # A hazard's own files (the registry's hazard_files: Bowser's Stadium's
    # bomb header and the model it externs), outside the map's closure.
    hazard_files = sorted({f for h in row.get("hazard_files", ()) for f in closure(rom, h)}
                          - set(files))
    o2r = out / "o2r"
    o2r.mkdir(parents=True, exist_ok=True)
    hashes = {}
    for fid in files + hazard_files:
        blob = rom.o2r(fid)
        p5_stages.write_if_changed(o2r / f"{fid:04x}", blob)
        hashes[f"{fid:04x}"] = {"sha256": hashlib.sha256(blob).hexdigest(),
                                "payload_bytes": len(rom.file_bytes(fid))}
    stage = {
        "schema": "p5-stage-v1",
        "name": row["name"], "token": row["token"], "gkind": row["gkind"],
        "remix_id": rid,
        "header": {"file": map_fid, "offset": hoff},
        "files": files,
        "hazard_files": hazard_files,
        "hazard_roots": list(row.get("hazard_files", ())),
        "o2r": hashes,
        "layer_mask": mask,
        "layers": layers,
        "map_geometry": _ref(geometry),
        "wallpaper": _ref(wallpaper),
        "fog_color": list(data[hoff + 0x4C:hoff + 0x4F]),
        "fog_alpha": data[hoff + 0x4F],
        "emblem_colors": [list(data[hoff + 0x50 + 3 * k:hoff + 0x53 + 3 * k]) for k in range(4)],
        "light_angle": list(struct.unpack_from(">3f", data, hoff + 0x60)),
        "camera_bounds": list(struct.unpack_from(">4h", data, hoff + 0x6C)),
        "map_bounds": list(struct.unpack_from(">4h", data, hoff + 0x74)),
        "bgm_id": struct.unpack_from(">I", data, hoff + 0x7C)[0],
        # BGM.asm: Stages.default_music_track_table holds track + 1 per stage;
        # a nonzero entry replaces the header's song when the stage loads
        # (Hyrule Temple's header names Hyrule Castle's track, 9).
        "remix_bgm": _default_music(rom, table, rid),
        "map_nodes": _ref(slot(map_fid, hoff + 0x80)),
        "item_weights": _ref(slot(map_fid, hoff + 0x84)),
        "alt_warning": struct.unpack_from(">h", data, hoff + 0x88)[0],
        "team_camera_bounds": list(struct.unpack_from(">4h", data, hoff + 0x8A)),
        "team_map_bounds": list(struct.unpack_from(">4h", data, hoff + 0x92)),
        "zoom_start": list(struct.unpack_from(">3h", data, hoff + 0x9A)),
        "zoom_end": list(struct.unpack_from(">3h", data, hoff + 0xA0)),
        "remix": {
            "zoom": round(f32("zoom_table", rid), 4),
            "background_parent": u8("background_table", rid),
            "icon": u32("icon_offset_table", rid),
            "class": u8("class_table", rid),
        },
    }
    p5_stages.write_if_changed(out / "stage.json", json.dumps(stage, indent=1) + "\n")
    return stage


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--stage", action="append", default=[],
                    help="registry name; default every Remix stage")
    ap.add_argument("--landed", action="store_true",
                    help="only the Remix stages the registry marks landed")
    ap.add_argument("--verify-repeat", action="store_true",
                    help="export twice and require identical bytes")
    args = ap.parse_args()
    rows = [r for r in p5_stages.load_registry() if r["source"] == "remix"]
    if args.landed:
        rows = [r for r in rows if r["landed"]]
    if args.stage:
        names = set(args.stage)
        rows = [r for r in rows if r["name"] in names]
        missing = names - {r["name"] for r in rows}
        if missing:
            raise ExportError(f"not Remix stages in stages.json: {sorted(missing)}")
    rom, u32, u8, f32, table = p5_stages.remix_tables(args.staging)
    for row in rows:
        out = args.out / row["name"]
        stage = export_stage(rom, u32, u8, f32, row, out, table)
        if args.verify_repeat:
            first = {p.name: p.read_bytes() for p in sorted((out / "o2r").iterdir())}
            first_json = (out / "stage.json").read_bytes()
            export_stage(rom, u32, u8, f32, row, out, table)
            again = {p.name: p.read_bytes() for p in sorted((out / "o2r").iterdir())}
            if again != first or (out / "stage.json").read_bytes() != first_json:
                raise ExportError(f"{row['name']}: a second export differs")
        lay = " ".join(f"{l['dobj_count']}{'S' if l['secondary'] else ''}" for l in stage["layers"])
        print(f"stage_export: {row['name']} map {stage['header']['file']:#x}+{stage['header']['offset']:#x} "
              f"files {' '.join(f'{f:#x}' for f in stage['files'])} layers {lay} -> {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

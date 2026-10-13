#!/usr/bin/env python3
"""P5.0 census: measure stage files and songs in the staged Smash Remix build.

Read-only. For each stage it reads the MPGroundData header in the map file,
walks the four layers' DObjDesc arrays and display lists (F3DEX2; a layer
whose `layer_mask` bit is set holds DObjDLLink arrays), and reads the
collision geometry, bounds, wallpaper and file closure. Triangle and vertex
counts are planning estimates: unique vertex loads, not the native
generator's pinned counts.

It also checks that the 1P venues Remix plays in VS are byte-identical to
the vanilla ROM, and lists the instrument programs each song uses (Remix
appends instruments to the music bank from program 0x2A; the DS BGM renderer
reads only the vanilla bank today).

    python scripts/p5/stage_census.py \
        --staging builds/p4-staging/remix-5e04fe7 [--json out.json] \
        [--p4-victory-inc builds/<build>/p4/audio/nds_p4_bgm.generated.inc]

ROM-derived output: write --json under builds/ only.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
import re
import struct
import sys
from pathlib import Path

_p = Path(__file__).resolve().parent
while _p.name != "scripts":
    _p = _p.parent
sys.path.insert(0, str(_p))
import _paths  # noqa: E402,F401

sys.path.insert(0, str(_paths.SCRIPTS_ROOT / "p4"))
import remix_rom  # noqa: E402

# (name, map file id, MPGroundData offset = Remix `type`), Stages.asm header/type scopes.
STAGES = (
    ("Peach's Castle", 0x103, 0x14), ("Sector Z", 0x106, 0x14),
    ("Congo Jungle", 0x105, 0x14), ("Planet Zebes", 0x101, 0x14),
    ("Hyrule Castle", 0x109, 0x14), ("Yoshi's Island", 0x107, 0x14),
    ("Dream Land", 0xFF, 0x14), ("Saffron City", 0x108, 0x14),
    ("Mushroom Kingdom", 0x104, 0x14),
    ("Meta Crystal", 0x10D, 0x14), ("Duel Zone", 0x10C, 0x14),
    ("Final Destination", 0x10A, 0x00),
    ("First Destination", 0x877, 0x14), ("Bowser's Stadium", 0x8CA, 0x14),
    ("Hyrule Temple", 0x981, 0x14), ("Dinosaur Land", 0x1247, 0x14),
)
# The 1P venues' map, geometry and wallpaper files.
VANILLA_VENUE_FILES = (0x10D, 0x75, 0x62, 0x10C, 0x74, 0x61, 0x10A, 0x72, 0x60)
# P5 default songs (Stages.asm add_stage rows), src/music/<file>.
P5_SONGS = {
    "FIRST_DESTINATION": "FIRST_DESTINATION.bin",
    "BOWSERBOSS": "bowserboss.bin",
    "HYRULE_TEMPLE": "hyrule_temple.bin",
    "SMW_ATHLETIC": "smw_athletic.bin",
}
VANILLA_PROGRAMS = 0x2A  # midi.asm: instrument_count starts here
DOBJ_ARRAY_END = 18      # objtypes.h DOBJ_ARRAY_MAX


class Census:
    def __init__(self, rom: remix_rom.Rom):
        self.rom = rom
        self._slots: dict[int, dict] = {}

    def slot(self, fid: int, off: int):
        """(file, offset) a relocated pointer word points at, or None."""
        if fid not in self._slots:
            self._slots[fid] = self.rom.reloc_slots(fid)
        s = self._slots[fid].get(off)
        if s is None:
            return None
        return (fid, s[1]) if s[0] == "intern" else (s[1], s[2])


def walk_dl(c: Census, fid: int, off: int, acc: dict, seen: set, depth: int = 0) -> None:
    if (fid, off) in seen or depth > 16:
        return
    seen.add((fid, off))
    data = c.rom.file_bytes(fid)
    siz, timg = 2, None
    while off + 8 <= len(data):
        w0, w1 = struct.unpack_from(">II", data, off)
        op = w0 >> 24
        if op == 0x01:                      # G_VTX
            n = (w0 >> 12) & 0xFF
            acc["vtx_loads"] += n
            tgt = c.slot(fid, off + 4)
            if tgt:
                acc["vtx"].add((tgt[0], tgt[1], n))
        elif op == 0x05:                    # G_TRI1
            acc["tris"] += 1
        elif op in (0x06, 0x07):            # G_TRI2, G_QUAD
            acc["tris"] += 2
        elif op == 0xFD:                    # G_SETTIMG
            siz = (w0 >> 19) & 3
            timg = c.slot(fid, off + 4)
        elif op == 0xF3 and timg:           # G_LOADBLOCK
            nbytes = (((w1 >> 12) & 0xFFF) + 1 << siz) >> 1
            acc["tex"][timg] = max(acc["tex"].get(timg, 0), nbytes)
        elif op == 0xF4 and timg:           # G_LOADTILE (10.2 fixed-point corners)
            w = (((w1 >> 12) & 0xFFF) - ((w0 >> 12) & 0xFFF)) // 4 + 1
            h = ((w1 & 0xFFF) - (w0 & 0xFFF)) // 4 + 1
            acc["tex"][timg] = max(acc["tex"].get(timg, 0), (w * h << siz) >> 1)
        elif op == 0xF0 and timg:           # G_LOADTLUT
            acc["tlut"][timg] = (((w1 >> 14) & 0x3FF) + 1) * 2
        elif op == 0xDE:                    # G_DL
            tgt = c.slot(fid, off + 4)
            if tgt:
                walk_dl(c, tgt[0], tgt[1], acc, seen, depth + 1)
            elif (w1 >> 24) == 0x0E:
                acc["seg_e"] += 1
            if (w0 >> 16) & 1:              # branch: no return
                return
        elif op == 0xDF:                    # G_ENDDL
            return
        off += 8


def measure(c: Census, name: str, fid: int, hoff: int) -> dict:
    data = c.rom.file_bytes(fid)
    acc = {"vtx_loads": 0, "vtx": set(), "tris": 0, "tex": {}, "tlut": {}, "seg_e": 0}
    seen: set = set()
    mask = data[hoff + 0x44]
    layers = []
    for i in range(4):
        base = hoff + i * 16
        dd = c.slot(fid, base)
        if dd is None:
            layers.append(None)
            continue
        before = acc["tris"]
        ddata = c.rom.file_bytes(dd[0])
        off, count = dd[1], 0
        while off + 0x2C <= len(ddata) and struct.unpack_from(">i", ddata, off)[0] != DOBJ_ARRAY_END:
            count += 1
            dl = c.slot(dd[0], off + 4)
            if dl and mask & (1 << i):      # DObjDLLink {s32 list_id; Gfx *dl}, ends at list 4
                ld = c.rom.file_bytes(dl[0])
                o = dl[1]
                while o + 8 <= len(ld) and 0 <= struct.unpack_from(">i", ld, o)[0] <= 3:
                    sub = c.slot(dl[0], o + 4)
                    if sub:
                        walk_dl(c, sub[0], sub[1], acc, seen)
                    o += 8
            elif dl:
                walk_dl(c, dl[0], dl[1], acc, seen)
            off += 0x2C
        layers.append({"dobjs": count, "tris": acc["tris"] - before,
                       "anim": c.slot(fid, base + 4) is not None,
                       "matanim": c.slot(fid, base + 12) is not None,
                       "secondary": bool(mask & (1 << i))})
    row = {"name": name, "map": f"{fid:#x}", "layers": layers,
           "tris": acc["tris"], "vtx_unique": sum(n for _f, _o, n in acc["vtx"]),
           "vtx_loads": acc["vtx_loads"], "textures": len(acc["tex"]),
           "tex_bytes": sum(acc["tex"].values()), "tlut_bytes": sum(acc["tlut"].values()),
           "seg_e_calls": acc["seg_e"],
           "camera_bounds": struct.unpack_from(">4h", data, hoff + 0x6C),
           "blast_bounds": struct.unpack_from(">4h", data, hoff + 0x74),
           "bgm_id": struct.unpack_from(">I", data, hoff + 0x7C)[0],
           "item_weights": c.slot(fid, hoff + 0x84) is not None}
    wp = c.slot(fid, hoff + 0x48)
    if wp:
        wd = c.rom.file_bytes(wp[0])
        row["wallpaper"] = {"file": f"{wp[0]:#x}", "w": struct.unpack_from(">h", wd, wp[1] + 4)[0],
                            "h": struct.unpack_from(">h", wd, wp[1] + 6)[0]}
    geo = c.slot(fid, hoff + 0x40)
    if geo:
        gd = c.rom.file_bytes(geo[0])
        groups = struct.unpack_from(">H", gd, geo[1])[0]
        li, lines = c.slot(geo[0], geo[1] + 0x10), 0
        if li:
            ld = c.rom.file_bytes(li[0])
            for k in range(groups):
                for j in range(4):          # MPLineInfo: u16 group, 4 x (u16 id, u16 count)
                    lines += struct.unpack_from(">H", ld, li[1] + k * 18 + 4 + j * 4)[0]
        row["collision"] = {"groups": groups, "lines": lines,
                            "map_objects": struct.unpack_from(">H", gd, geo[1] + 0x14)[0]}
    closure, todo = set(), [fid]
    while todo:
        f = todo.pop()
        if f not in closure:
            closure.add(f)
            todo.extend(x for x in c.rom.extern_ids(f) if x not in closure)
    row["files"] = [f"{f:#x}" for f in sorted(closure)]
    row["file_bytes"] = sum(len(c.rom.file_bytes(f)) for f in closure)
    return row


def song_programs(cseq, path: Path) -> list[int]:
    seq = path.read_bytes()
    offs = sorted((o for o in struct.unpack_from(">16I", seq, 0) if o))
    progs: set[int] = set()
    for i, o in enumerate(offs):
        end = offs[i + 1] if i + 1 < len(offs) else len(seq)
        for _tick, _key, ev in cseq.parse_track_to_events(seq, o, end):
            if ev[0] == "midi" and (ev[1] & 0xF0) == 0xC0:
                progs.add(int(ev[2]))
    return sorted(progs)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--staging", type=Path,
                    default=_paths.REPO_ROOT / "builds" / "p4-staging" / "remix-5e04fe7")
    ap.add_argument("--vanilla", type=Path, help="vanilla US z64 (default: <staging>/roms/ssb.rom)")
    ap.add_argument("--json", type=Path)
    ap.add_argument("--p4-victory-inc", type=Path,
                    help="a build's nds_p4_bgm.generated.inc: audit its songs' programs too")
    args = ap.parse_args()

    rom = remix_rom.Rom(args.staging / "ssb64asm.z64")
    c = Census(rom)
    rows = [measure(c, *s) for s in STAGES]
    for r in rows:
        lay = " ".join("-" if l is None else
                       f"{l['dobjs']}/{l['tris']}{'S' if l['secondary'] else ''}"
                       f"{'A' if l['anim'] else ''}{'M' if l['matanim'] else ''}" for l in r["layers"])
        col = r.get("collision", {})
        print(f"{r['name']:<18} {lay:<28} tris {r['tris']:>4} vtx {r['vtx_unique']:>5} "
              f"tex {r['textures']:>2}/{r['tex_bytes']:>6} B coll {col.get('groups')}/{col.get('lines')}/"
              f"{col.get('map_objects')} cam {r['camera_bounds']} blast {r['blast_bounds']} "
              f"files {' '.join(r['files'])} weights {'yes' if r['item_weights'] else 'NULL'}")

    vanilla = remix_rom.Rom(args.vanilla or args.staging / "roms" / "ssb.rom")
    same = {f"{f:#x}": rom.file_bytes(f) == vanilla.file_bytes(f) for f in VANILLA_VENUE_FILES}
    print("1P venue files identical to vanilla:", all(same.values()), same)

    spec = importlib.util.spec_from_file_location(
        "cseq_to_mid", _paths.REPO_ROOT / "decomp/BattleShip-main/decomp/tools/cseq_to_mid.py")
    cseq = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(cseq)
    music = args.staging / "src" / "music"
    songs = {name: music / f for name, f in P5_SONGS.items()}
    if args.p4_victory_inc:
        for rel in re.findall(r"/\* (src/music/[^ ]+\.bin) \*/", args.p4_victory_inc.read_text()):
            songs[Path(rel).stem] = args.staging / rel
    programs = {}
    for name, path in songs.items():
        progs = song_programs(cseq, path)
        programs[name] = progs
        print(f"song {name:<18} programs {progs} Remix-added {[p for p in progs if p >= VANILLA_PROGRAMS]}")

    if args.json:
        args.json.write_text(json.dumps({"stages": rows, "venues_identical": same,
                                         "song_programs": programs}, indent=1),
                             encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

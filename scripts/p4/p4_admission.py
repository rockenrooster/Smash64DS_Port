#!/usr/bin/env python3
"""The P4 contents' fighter texture admission records.

The cast's table (scripts/fighters/generate_nds_fighter_admission.py) names
every texture a kind can show, as the RDP state its draw presents, and the
battle load replays each record through the texture cache, so no fighter
texture converts or uploads after GO. A content was noted under its parent
kind, whose records name files a content's battle never loads: its own
textures converted on first sight, mid-match, inside the materialization of
the frame that first showed them (four Crashes: 66 uploads after GO, ~100K
cycles each -- a new face, a new costume's palette).

This enumerates each content the same way, with the cast generator's own
display-list state machine, material evaluator and record packer, over the
content's O2R files: the common parts' JointTree lists with their MObjSub
chains and costume streams (LOW falling back to HIGH per joint), every
FTModelPart row with its costume and main streams, the access part, and the
texture-part ids the content's motion scripts set (SetTexturePartID in its
export's events). Records keep the cast's format; their file ids are the
content's own.

Output: NitroFS fighters/p4_admission.bin -- magic, version, the index's
content count, record words; (first, count) per content id x detail (0 HIGH,
1 LOW); the records.

Usage:
  python scripts/p4/p4_admission.py --gen <build>/p4 --export-root <export> \
      --contents "falco ..." --out <nitrofs>/fighters/p4_admission.bin [--report]
"""

from __future__ import annotations

import argparse
import collections
import itertools
import json
import struct
import sys
from pathlib import Path
from types import SimpleNamespace

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(REPO / "scripts"))
sys.path.insert(0, str(REPO / "scripts" / "fighters"))
import _paths  # noqa: E402,F401
import ft_layout  # noqa: E402
import p4_contents  # noqa: E402
from p4_native_owner import O2RFile  # noqa: E402
import generate_nds_fighter_admission as A  # noqa: E402

MAGIC = 0x34504441  # 'ADP4'
VERSION = 1
MODELPARTS_ENTRIES = 37 - 4  # nFTPartsJointNumMax - nFTPartsJointCommonStart
TREE_SENTINEL_DEPTH = 18


class Closure:
    """The cast generator's closure view (payload, pointer map, the Gfx a walk
    may enter) over one content's O2R files."""

    def __init__(self, o2r_dir: Path):
        self.o2r: dict[int, O2RFile] = {}
        self.files: dict[int, SimpleNamespace] = {}
        self.objects: dict = {}
        for path in sorted(o2r_dir.iterdir()):
            try:
                fid = int(path.name, 16)
            except ValueError:
                continue
            f = O2RFile(path)
            self.o2r[fid] = f
            self.files[fid] = SimpleNamespace(source={"payload": f.data})

    def payload(self, fid: int) -> bytes:
        return self.o2r[fid].data

    def ptr(self, fid: int, slot: int):
        f = self.o2r.get(fid)
        p = None if f is None else f.slots.get(slot)
        return None if p is None else (p[1], p[2])

    def u16(self, fid: int, off: int) -> int:
        return struct.unpack_from(">H", self.payload(fid), off)[0]

    def u32(self, fid: int, off: int) -> int:
        return struct.unpack_from(">I", self.payload(fid), off)[0]

    def obj_at(self, fid: int, off: int):
        # A list the walk is handed runs to its G_ENDDL (A.run stops there);
        # the file's end bounds a malformed one.
        f = self.o2r.get(fid)
        if f is None or off >= len(f.data):
            return None
        return SimpleNamespace(type_name="Gfx", offset=off, size=len(f.data) - off,
                               symbol=None)

    def raw_at(self, ref, index: int):
        return None if ref is None else self.ptr(ref[0], ref[1] + 4 * index)

    def mobjsubs(self, ref) -> list:
        """The MObjSub* list lbCommonAddMObjForFighterPartsDObj walks, to NULL."""
        out = []
        for i in range(32):
            p = self.ptr(ref[0], ref[1] + 4 * i)
            if p is None:
                break
            out.append(p)
        return out

    def part_dl_roots(self, dl, flags: int) -> list:
        """FTParts.flags & 0xF == 1 (Yoshi's form, which Bowser keeps): the
        DObjDesc dl names a Gfx *[2] pair."""
        if (flags & 0xF) == 1:
            return [p for p in (self.ptr(dl[0], dl[1]), self.ptr(dl[0], dl[1] + 4))
                    if p is not None]
        return [tuple(dl)]


def joint_tree_count(c: Closure, ref) -> int:
    """DObjDesc rows through the depth-18 sentinel (donor_tables' count)."""
    for count in range(1, 66):
        if c.u32(ref[0], ref[1] + (count - 1) * A.DOBJDESC) == TREE_SENTINEL_DEPTH:
            return count
    raise SystemExit("JointTree has no depth-18 sentinel")


def part_rows(c: Closure, main: int, attr: int, d: int) -> list:
    """generate_nds_fighter_admission.part_rows over a content's FTAttributes:
    [(dl root, [MObjSub refs], [costume stream refs], [main stream refs], where)]."""
    lay = ft_layout.layout()
    out = []
    cont = c.ptr(main, attr + lay["FTAttributes.commonparts_container"])
    if cont is not None:
        rows = {}
        for dd in (0, 1):
            base = cont[1] + lay["sizeof(FTCommonPart)"] * dd
            desc = c.ptr(main, base + lay["FTCommonPart.dobjdesc"])
            pm = c.ptr(main, base + lay["FTCommonPart.p_mobjsubs"])
            pc = c.ptr(main, base + lay["FTCommonPart.p_costume_matanim_joints"])
            flags = c.payload(main)[base + lay["FTCommonPart.flags"]]
            dls = []
            if desc is not None and desc[0] in c.o2r:
                n = joint_tree_count(c, desc)
                dls = [c.ptr(desc[0], desc[1] + A.DOBJDESC * i + 4) for i in range(n)]
            rows[dd] = (dls, pm, pc, flags)
        for j in range(max(len(rows[0][0]), len(rows[1][0]))):
            dl1 = rows[1][0][j] if j < len(rows[1][0]) else None
            dd = 0 if (d == 0 or not dl1) else 1
            dls, pm, pc, flags = rows[dd]
            dl = dls[j] if j < len(dls) else None
            if not dl:
                continue
            ms = c.raw_at(pm, j) if pm else None
            cs = c.raw_at(pc, j) if pc else None
            mobjs = c.mobjsubs(ms) if ms else []
            cl = [c.raw_at(cs, k) for k in range(len(mobjs))] if cs else []
            for r in c.part_dl_roots(dl, flags):
                out.append((r, mobjs, cl, [], ("joint", j)))
    mp = c.ptr(main, attr + lay["FTAttributes.modelparts_container"])
    if mp is not None and mp[0] == main:
        # A desc's FTModelPart rows (part x detail) run to the next object a
        # pointer in Main names.
        targets = sorted({p[2] for p in c.o2r[main].slots.values() if p[0] == "intern"}
                         | {len(c.payload(main))})
        for ji in range(MODELPARTS_ENTRIES):
            desc = c.ptr(main, mp[1] + 4 * ji)
            if desc is None or desc[0] != main:
                continue
            bound = min(t for t in targets if t > desc[1])
            for i in range((bound - desc[1]) // A.FTMODELPART):
                if i % 2 != d:
                    continue
                slot = desc[1] + A.FTMODELPART * i
                dl = c.ptr(main, slot + lay["FTModelPart.dl"])
                if dl is None:
                    continue
                ms = c.ptr(main, slot + lay["FTModelPart.mobjsubs"])
                cs = c.ptr(main, slot + lay["FTModelPart.costume_matanim_joints"])
                mn = c.ptr(main, slot + lay["FTModelPart.main_matanim_joints"])
                flags = c.payload(main)[slot + lay["FTModelPart.flags"]]
                mobjs = c.mobjsubs(ms) if ms else []
                cl = [c.raw_at(cs, k) for k in range(len(mobjs))] if cs else []
                ml = [c.raw_at(mn, k) for k in range(len(mobjs))] if mn else []
                for r in c.part_dl_roots(dl, flags):
                    out.append((r, mobjs, cl, ml, ("mp", ji, i // 2, tuple(desc))))
    ap = c.ptr(main, attr + lay["FTAttributes.accesspart"])
    if ap is not None and ap[0] in c.o2r:
        joint_id = c.u32(ap[0], ap[1])
        dl = c.ptr(ap[0], ap[1] + 4)
        ms = c.ptr(ap[0], ap[1] + 8)
        cs = c.ptr(ap[0], ap[1] + 12)
        if dl is not None:
            mobjs = c.mobjsubs(ms) if ms else []
            cl = [c.raw_at(cs, k) for k in range(len(mobjs))] if cs else []
            out.append((tuple(dl), mobjs, cl, [], ("access", joint_id - A.COMMON_JOINT_START)))
    return out


def texture_part_targets(c: Closure, main: int, attr: int, events_path: Path):
    """{(common joint index, detail): {MObj index: ids}} from the content's
    FTTexturePartContainer and every SetTexturePartID its scripts run."""
    lay = ft_layout.layout()
    ids = collections.defaultdict(set)
    if events_path.exists():
        blocks = json.loads(events_path.read_text(encoding="utf-8"))["blocks"]
        for block in blocks.values():
            for command in block["commands"]:
                if command["op"] == "SetTexturePartID":
                    w = command["words"][0]
                    part, tid = (w >> 20) & 0x3F, w & 0xFFFFF
                    if part < 2 and tid < 64:
                        ids[part].add(tid)
    targets = collections.defaultdict(lambda: collections.defaultdict(set))
    tp = c.ptr(main, attr + lay["FTAttributes.textureparts_container"])
    if tp is None or tp[0] not in c.o2r:
        return targets
    data = c.payload(tp[0])
    for part, tids in ids.items():
        base = tp[1] + 3 * part
        jc = data[base] - A.COMMON_JOINT_START
        for d in (0, 1):
            targets[(jc, d)][data[base + 1 + d]] |= tids | {0}
    return targets


def enumerate_content(c: Closure, main: int, attr: int, events_path: Path) -> dict:
    """{detail: {record key: record}} -- generate_nds_fighter_admission's
    enumerate_kind over a content's rows (no Kirby hats)."""
    out = {}
    targets = texture_part_targets(c, main, attr, events_path)
    for d in (0, 1):
        recs = collections.OrderedDict()
        for root, mobjs, cl, ml, where in part_rows(c, main, attr, d):
            infos = [A.mobj_info(c, m) for m in mobjs]
            tp = targets.get((where[1], d), {}) if where[0] in ("joint", "mp") else {}
            per = [A.material_choices(c, info, cl[k] if k < len(cl) else None,
                                      ml[k] if k < len(ml) else None, tp.get(k, ()))
                   for k, info in enumerate(infos)]
            combos = collections.OrderedDict()
            for costume in range(A.COSTUMES):
                if where[0] == "access" and costume == 0:
                    continue
                lists = [sorted(p[costume]) for p in per]
                n = 1
                for lst in lists:
                    n *= len(lst)
                if n > 20000:
                    raise SystemExit(f"{where!r}: {n} material combinations")
                for combo in itertools.product(*lists):
                    combos[combo] = combos.get(combo, 0) | (1 << costume)
            for combo, cmask in combos.items():
                def hook(st, idx, combo=combo):
                    if idx < len(infos):
                        A.mobj_branch(c, infos[idx], combo[idx], st)

                def emit(st, via_mobj=bool(infos), cmask=cmask):
                    tile_i = (st["tex"][0] >> 8) & 7
                    tile = st["tiles"].get(tile_i, {})
                    if "set" not in tile:
                        return
                    rw0, rw1 = tile["set"]
                    load = st["tmem"].get(rw0 & 0x1FF)
                    if load is None or load["timg"] is None or load["timg"][1] is None:
                        return
                    img = tuple(load["timg"][1])
                    fmt, siz = (rw0 >> 21) & 7, (rw0 >> 19) & 3
                    tlut = st["tlut"] if (fmt == 2 and st["tlut"] and st["tlut"]["timg"]
                                          and st["tlut"]["timg"][1]) else None
                    ts = tile.get("size", ((0xF2 << 24), (tile_i << 24)))
                    tw = (((ts[1] >> 12) & 0xFFF) - ((ts[0] >> 12) & 0xFFF) >> 2) + 1
                    th = (((ts[1]) & 0xFFF) - ((ts[0]) & 0xFFF) >> 2) + 1
                    dims = A.ds_dims(fmt, siz, (rw0 >> 9) & 0x1FF, tw, th, load["texels"],
                                     (rw1 >> 8) & 3, (rw1 >> 18) & 3, (rw1 >> 4) & 15,
                                     (rw1 >> 14) & 15)
                    if dims is None:
                        return
                    tl_ref = tuple(tlut["timg"][1]) if tlut else None
                    rec = dict(
                        img=img, tlut=tl_ref, settimg=load["timg"][0],
                        load_set=load["settile"], load_op=load["op"],
                        load_w=(load["w0"], load["w1"]),
                        tlut_settimg=(tlut["timg"][0] if tlut else 0),
                        tlut_set=(tlut["settile"] if tlut else (0, 0)),
                        loadtlut_w1=(tlut["w1"] if tlut else 0),
                        render_set=(rw0, rw1), tilesize=ts, tex=st["tex"],
                        combine=st["combine"], omh=st["omh"], oml=st["oml"],
                        prim=st["prim"], env=st["env"], hat=None, mobj=via_mobj,
                        dims=dims, costume=cmask)
                    key = (img, tl_ref, rec["load_set"], rec["load_w"], rec["render_set"],
                           rec["tilesize"], rec["combine"], rec["omh"], rec["oml"],
                           rec["prim"], rec["env"], rec["tex"][0])
                    if key in recs:
                        recs[key]["costume"] |= rec["costume"]
                    else:
                        recs[key] = rec

                A.run(c, root[0], root[1], A.fresh_state(), emit, 0, hook)
        out[d] = recs
    return out


def build(gen: Path, export_root: Path, names: list[str]):
    rows = {row["name"]: row["id"] for row in p4_contents.load_registry()}
    tables = {}
    for name in names:
        facts = p4_contents.export_facts(export_root, name)
        c = Closure(gen / name / "o2r")
        if facts["main"] not in c.o2r:
            raise SystemExit(f"{name}: no main file {facts['main']:#x} in {gen / name / 'o2r'}")
        tables[rows[name]] = (name, enumerate_content(
            c, facts["main"], facts["attributes"], export_root / name / "events.json"))
    top = max(tables) if tables else 0
    index = []
    records = b""
    count = 0
    for cid in range(top + 1):
        for d in (0, 1):
            recs = list(tables[cid][1][d].values()) if cid in tables else []
            index.append((count, len(recs)))
            for r in recs:
                records += A.pack_record(r, 0)
            count += len(recs)
    head = struct.pack("<4I", MAGIC, VERSION, top + 1, A.RECORD_WORDS)
    head += b"".join(struct.pack("<2I", a, b) for a, b in index)
    return tables, head + records


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--gen", type=Path, required=True, help="the build's P4 generation directory")
    ap.add_argument("--export-root", type=Path, required=True)
    ap.add_argument("--contents", required=True, help="the enabled contents, space separated")
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--report", action="store_true")
    args = ap.parse_args()
    tables, payload = build(args.gen, args.export_root, args.contents.split())
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(payload)
    if args.report:
        for cid, (name, recs) in sorted(tables.items()):
            for d in (1, 0):
                rs = list(recs[d].values())
                ds = sum(r["dims"][0] * r["dims"][1] // 2 for r in rs)
                print(f"{cid:2d} {name:<11} {'LOW ' if d else 'HIGH'} {len(rs):4d} records "
                      f"{ds:7,d} B at 4 bpp")
    return 0


if __name__ == "__main__":
    sys.exit(main())

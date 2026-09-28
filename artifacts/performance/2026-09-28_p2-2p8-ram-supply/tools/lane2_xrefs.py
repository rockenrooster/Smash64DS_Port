#!/usr/bin/env python3
"""Lane 2: whole-tree referrer census.  Every O2R container under
BattleShip_o2r/reloc_* is parsed; every EXTERNAL fixup whose dependency is one
of the files under study is recorded (source file, slot, target offset).  This
is the only way another file can point into these files by relocation, so it
bounds what "unreferenced" can mean.  (Numeric ll-symbol arithmetic in code is
covered separately by the root tables.)
"""
import json
import struct
import sys
from collections import defaultdict
from pathlib import Path

import lane2_o2r as o2r

STUDY = {82: "IFCommonGameStatus", 83: "EFCommonEffects1", 84: "EFCommonEffects2",
         85: "EFCommonEffects3", 86: "MiscData086", 87: "IFCommonItem", 251: "ITCommonData",
         164: "IFCommonPlayerDamage", 165: "IFCommonTimer", 36: "IFCommonDigits",
         197: "IFCommonBattlePause", 38: "IFCommonPlayerTags", 37: "IFCommonAnnounceCommon",
         166: "IFCommonPlayer"}


def main():
    files = sorted(p for p in o2r.O2R_ROOT.glob("reloc_*/*") if p.is_file())
    refs = defaultdict(list)
    n = 0
    for p in files:
        raw = p.read_bytes()
        if len(raw) < 0x50 or raw[4:8] != b"OLER":
            continue
        n += 1
        fid, ih, eh, ec = struct.unpack_from("<IHHI", raw, 0x40)
        if ec == 0 or eh == 0xFFFF:
            continue
        ids = list(struct.unpack_from(f"<{ec}H", raw, 0x4C))
        ds_off = 0x4C + ec * 2
        dsz = struct.unpack_from("<I", raw, ds_off)[0]
        payload = raw[ds_off + 4: ds_off + 4 + dsz]
        cur = eh
        idx = 0
        guard = len(payload) // 4 + 1
        while cur != 0xFFFF and guard > 0 and idx < ec:
            guard -= 1
            slot = cur * 4
            w = struct.unpack_from(">I", payload, slot)[0]
            dep = ids[idx]
            if dep in STUDY:
                refs[dep].append((p.name, fid, slot, (w & 0xFFFF) * 4))
            idx += 1
            cur = w >> 16
    print("containers parsed:", n)
    out = {}
    for dep, name in STUDY.items():
        r = refs.get(dep, [])
        srcs = sorted({x[0] for x in r})
        print(f"{name:24s} (fid {dep:3d}) external referrers: {len(r):4d} slots from {len(srcs)} files: {srcs[:6]}")
        out[name] = {"slots": len(r), "files": srcs,
                     "targets": sorted({hex(x[3]) for x in r})}
    Path(__file__).with_name("lane2_xrefs.json").write_text(json.dumps(out, indent=1))


if __name__ == "__main__":
    main()

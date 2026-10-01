"""Enumerate DL roots reachable from ITCommonData item/weapon attributes."""
import re, struct, sys
from pathlib import Path
REPO = Path("D:/Stuff/DevFolder/Smash64DS_Port")
sys.path.insert(0, str(REPO / "scripts" / "stages"))
import generate_nds_native_stage as sm
m = sm.load_o2r(REPO, sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscData086",
    "96e987d81b24497ad0c314372edb79f123016b59187b5f94b57b73e4bb122c04", 86, 402, 0,
    "1c642e60401ca5e6df2fe1b0d6ddeb6e322b0288f4c13a1e608875322fbf9438"))
a = sm.load_o2r(REPO, sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_items/ITCommonData",
    "8ad38613162d33e712f79f9d5584540dadd3fbddad90287dedf8cfc59cc76f32", 251, 0, 68,
    "264b5ef35dfe41bd22cc94a82a80034e6913b9d1d827839392a6328ac9a60145"))
names = {}
for line in open(REPO / "decomp/BattleShip-main/include/reloc_data.us.h"):
    g = re.match(r"#define (llITCommonData(\w+?)(Item|Weapon)Attributes) \(\(intptr_t\)(0x[0-9A-Fa-f]+)\)", line)
    if g:
        names[int(g.group(4), 16)] = g.group(2) + g.group(3)
owned = set()
for h in (REPO / "include/nds/generated").glob("nds_native_item_*.generated.h"):
    for line in open(h):
        g = re.match(r"#define NDS_NATIVE_ITEM_\w*ROOT\w* (0x[0-9a-fA-F]+)u", line)
        if g:
            owned.add(int(g.group(1), 16))
def looks_dl(off):
    if off + 8 > len(m.payload):
        return False
    return (struct.unpack_from(">I", m.payload, off)[0] >> 24) in (0xE7, 0xD9, 0xDE, 0xFC, 0xE3, 0xE2, 0xDB, 0xD7, 0x01, 0xFA, 0xFB)
def dls_of(target):
    """DL roots behind an attribute data pointer: a DL, a DObjDesc array, or DL links."""
    if looks_dl(target):
        return [target]
    out = []
    o = target
    for _ in range(64):  # DObjDesc entries (0x2C) until id 0x12
        idw = struct.unpack_from(">I", m.payload, o)[0]
        if idw == 0x12:
            break
        r = m.pointer_at(o + 4)
        if r is not None and r.asset_id == 86:
            if looks_dl(r.offset):
                out.append(r.offset)
            else:  # DL link list: (list id, dl) pairs until id 4
                q = r.offset
                for _ in range(8):
                    lid = struct.unpack_from(">I", m.payload, q)[0]
                    rr = m.pointer_at(q + 4)
                    if lid == 4 or rr is None:
                        break
                    if looks_dl(rr.offset):
                        out.append(rr.offset)
                    q += 8
        o += 0x2C
    return out
for off in sorted(names):
    r = a.pointer_at(off)
    if r is None or r.asset_id != 86:
        print(f"{names[off]:28s} data -> {r}")
        continue
    roots = dls_of(r.offset)
    tag = " ".join(f"{x:#07x}{'*' if x in owned else ''}" for x in roots)
    print(f"{names[off]:28s} data 86:{r.offset:#07x}  roots: {tag}")

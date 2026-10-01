"""Dump an O2R file's display list from a root: opcodes, operands, pointer refs.
usage: python dldump.py <o2r relative path> <sha> <asset> <a> <b> <payload sha> <root hex> [max]"""
import struct
import sys
from pathlib import Path
REPO = Path("D:/Stuff/DevFolder/Smash64DS_Port")
sys.path.insert(0, str(REPO / "scripts" / "stages"))
import generate_nds_native_stage as sm  # noqa: E402

NAMES = {0x01: "G_VTX", 0x05: "G_TRI1", 0x06: "G_TRI2", 0x07: "G_QUAD", 0xD7: "G_TEXTURE",
         0xD9: "G_GEOMETRYMODE", 0xDA: "G_MTX", 0xDB: "G_MOVEWORD", 0xDC: "G_MOVEMEM",
         0xDE: "G_DL", 0xDF: "G_ENDDL", 0xE1: "G_RDPHALF_1", 0xE2: "G_SETOTHERMODE_L",
         0xE3: "G_SETOTHERMODE_H", 0xE4: "G_TEXRECT", 0xE6: "G_RDPLOADSYNC", 0xE7: "G_RDPPIPESYNC",
         0xE8: "G_RDPTILESYNC", 0xE9: "G_RDPFULLSYNC", 0xF0: "G_LOADTLUT", 0xF2: "G_SETTILESIZE",
         0xF3: "G_LOADBLOCK", 0xF4: "G_LOADTILE", 0xF5: "G_SETTILE", 0xF6: "G_FILLRECT",
         0xF7: "G_SETFILLCOLOR", 0xF8: "G_SETFOGCOLOR", 0xF9: "G_SETBLENDCOLOR", 0xFA: "G_SETPRIMCOLOR",
         0xFB: "G_SETENVCOLOR", 0xFC: "G_SETCOMBINE", 0xFD: "G_SETTIMG", 0xFE: "G_SETZIMG", 0xFF: "G_SETCIMG",
         0xD8: "G_POPMTX", 0xD6: "G_DMA_IO", 0xD5: "G_SPECIAL", 0x00: "G_NOOP", 0x02: "G_MODIFYVTX",
         0x03: "G_CULLDL", 0x04: "G_BRANCH_Z"}
path, sha, asset, a, b, psha, root = sys.argv[1:8]
mx = int(sys.argv[8]) if len(sys.argv) > 8 else 200
spec = sm.InputSpec(path, sha, int(asset), None if a == "-" else int(a), None if b == "-" else int(b), None if psha == "-" else psha)
m = sm.load_o2r(REPO, spec)
seen = set()
def walk(off, depth):
    if off in seen or depth > 4:
        return
    seen.add(off)
    for i in range(mx):
        o = off + i * 8
        w0, w1 = struct.unpack_from(">II", m.payload, o)
        op = w0 >> 24
        ref = m.pointer_at(o + 4)
        rs = f" -> {ref.asset_id}:{ref.offset:#06x}" if ref is not None else ""
        print(f"{'  ' * depth}{o:#06x}: {w0:08x} {w1:08x} {NAMES.get(op, hex(op))}{rs}")
        if op == 0xDE and ref is not None and ref.asset_id == int(asset):
            walk(ref.offset, depth + 1)
            if (w0 >> 16) & 0xff == 1:
                return
        if op == 0xDF:
            return
walk(int(root, 16), 0)

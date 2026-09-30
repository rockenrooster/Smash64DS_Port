import struct
p = r"D:\Stuff\DevFolder\Smash64DS_Port\decomp\BattleShip-main\BattleShip_o2r\reloc_extern_data\ExternDataBank109"
s = open(p, "rb").read()
fid, ih, eh, ec = struct.unpack_from("<IHHI", s, 0x40)
pay = s[0x4C + ec * 2 + 4:]
def tgt(w): return (w & 0xFFFF) * 4
dlo = 0x7638
vtx = None
for i in range(60):
    w0, w1 = struct.unpack_from(">II", pay, dlo + i * 8)
    op = w0 >> 24
    note = ""
    if op == 0x01:
        n = (w0 >> 12) & 0xFF
        vtx = tgt(w1); note = f"VTX n={n} -> {vtx:05x}"
    elif op == 0x05: note = "TRI1"
    elif op == 0x06: note = "TRI2"
    elif op == 0xFC: note = "SETCOMBINE"
    elif op == 0xE2: note = "SETOTHERMODE_L"
    elif op == 0xE3: note = "SETOTHERMODE_H"
    elif op == 0xFA: note = f"PRIMCOLOR {w1:08x}"
    elif op == 0xFB: note = f"ENVCOLOR {w1:08x}"
    elif op == 0xD9: note = "GEOMETRYMODE"
    elif op == 0xFD: note = "SETTIMG"
    elif op == 0xDE: note = f"DL -> {tgt(w1):05x}"
    print(f"  {dlo + i*8:05x}: {w0:08x} {w1:08x} {note}")
    if op == 0xDF:
        break
if vtx is not None:
    print("vertices:")
    for k in range(8):
        x, y, z, f, s_, t, r, g, b, a = struct.unpack_from(">hhhHhhBBBB", pay, vtx + k * 16)
        print(f"   v{k}: pos=({x},{y},{z}) st=({s_},{t}) rgba=({r},{g},{b},{a})")

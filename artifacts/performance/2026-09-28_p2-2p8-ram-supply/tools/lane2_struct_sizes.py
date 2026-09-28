#!/usr/bin/env python3
"""Lane 2: sizeof() of pool element structs, read from the DWARF of the saved
shipping-like ELF with a static (no target, nothing executes) gdb session.

  builds/build-fp-argmax/smash64ds-p2-shell-freeplay-hwtri.elf
"""
from __future__ import annotations

import json
import re
import subprocess
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
ELF = REPO / "builds/build-fp-argmax/smash64ds-p2-shell-freeplay-hwtri.elf"
GDB = Path("C:/devkitPro/devkitARM/bin/arm-none-eabi-gdb.exe")
TYPES = ["EFStruct", "ITStruct", "WPStruct", "LBParticle", "LBGenerator",
         "LBTransform", "Sprite", "Bitmap", "GObj", "DObj", "SObj", "MObj",
         "AObj", "Vtx", "Gfx", "MObjSub", "DObjDesc", "DObjDLLink",
         "ITAttributes", "FTStruct", "LBFileNode", "NDSVisualTemplate"]


def main():
    out = {}
    with tempfile.NamedTemporaryFile("w", suffix=".gdb", delete=False) as f:
        f.write("set pagination off\n")
        for t in TYPES:
            f.write(f'echo SIZEOF:{t}:\\n\nprint sizeof({t})\n')
        cmds = f.name
    res = subprocess.run([str(GDB), "-batch", "-x", cmds, str(ELF)],
                         capture_output=True, text=True, timeout=300)
    text = res.stdout + res.stderr
    cur = None
    for line in text.splitlines():
        m = re.match(r"SIZEOF:(\w+):", line)
        if m:
            cur = m.group(1)
            continue
        m = re.match(r"\$\d+ = (\d+)", line)
        if m and cur:
            out[cur] = int(m.group(1))
            cur = None
    print(json.dumps(out, indent=1))
    (Path(__file__).with_name("lane2_struct_sizes.json")).write_text(json.dumps(out, indent=1))


if __name__ == "__main__":
    main()

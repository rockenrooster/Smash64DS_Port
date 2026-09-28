#!/usr/bin/env python3
"""Lane 2: prove the shipping-like ARM9 ELF links no display-list interpreter.

Reads the FORBIDDEN symbol set from scripts/check_native_only_rom.py (the
build's own guard, run at Makefile:7515) and looks each name up in the symbol
table of builds/build-fp-argmax/smash64ds-p2-shell-freeplay-hwtri.elf with
arm-none-eabi-nm.  Also lists any linked function whose name suggests a display
list walker (GBI decode / scan / execute).
"""
import ast
import re
import subprocess
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
ELF = REPO / "builds/build-fp-argmax/smash64ds-p2-shell-freeplay-hwtri.elf"
NM = "C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe"


def main():
    import datetime
    st = ELF.stat()
    print(f"ELF {ELF.relative_to(REPO)} size={st.st_size} mtime={datetime.datetime.fromtimestamp(st.st_mtime):%Y-%m-%d %H:%M:%S}")
    src = (REPO / "scripts/check_native_only_rom.py").read_text()
    m = re.search(r"FORBIDDEN\s*=\s*frozenset\(\((.*?)\)\)", src, re.S)
    names = ast.literal_eval("(" + m.group(1) + ")")
    out = subprocess.run([NM, str(ELF)], capture_output=True, text=True).stdout.splitlines()
    syms = {ln.split()[-1] for ln in out if len(ln.split()) >= 3}
    present = sorted(n for n in names if n in syms)
    print(f"forbidden symbols in check_native_only_rom.py: {len(names)}; present in ELF: {len(present)} {present}")
    sus = sorted(s for s in syms if re.search(r"(ScanDisplayList|ExecuteDisplayList|GBIDecode|DecodeF3DEX2|RendererScan)", s))
    print("walker-like symbols present:", sus)
    print("ELF symbols total:", len(syms))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Illustrative freestanding ARM9 codegen only. Not a devkitARM/SDK build."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def main() -> None:
    clang = os.environ.get("CLANG", "clang")
    if shutil.which(clang) is None:
        print("SKIP: Clang unavailable; no illustrative ARM codegen check.")
        raise SystemExit(2)
    with tempfile.TemporaryDirectory(prefix="nds-clang-probes-") as name:
        work = Path(name)
        # No ARM C library is installed here. Only NDEBUG assert is stubbed;
        # integer/types headers come from Clang's freestanding resource headers.
        (work/"assert.h").write_text("#pragma once\n#define assert(expression) ((void)0)\n")
        assembly = work/"portable.s"
        cmd = [clang, "--target=arm-none-eabi", "-mcpu=arm946e-s", "-marm",
               "-ffreestanding", "-std=c11", "-O2", "-DNDEBUG", "-Wall", "-Wextra", "-Werror",
               "-I", str(work), "-S", str(ROOT/"tests/portable_codegen.c"), "-o", str(assembly)]
        subprocess.run(cmd, check=True)
        text = assembly.read_text()
        for symbol in ("probe_mul_trunc", "probe_mul_round", "probe_div", "probe_copy16", "probe_protocol"):
            if f"{symbol}:" not in text:
                raise SystemExit(f"FAIL: missing exported probe {symbol}")
        print(subprocess.check_output([clang,"--version"], text=True).splitlines()[0])
        print("PASS: exported portable helpers compiled for ARM946E-S in ARM mode")
        print("Observed wide multiply instruction:", any(x in text.lower() for x in ("smull", "smlal")))
        print("Observed software signed 64-bit division helper:", "__aeabi_ldivmod" in text)
        print("Observed explicit halfword store:", "strh" in text.lower())
        print("Observations are compiler-specific, not performance pass/fail gates.")
        print("No SDK headers, target link, MMIO execution, emulator, or hardware timing tested.")

if __name__ == "__main__":
    main()

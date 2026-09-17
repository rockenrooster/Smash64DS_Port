#!/usr/bin/env python3
"""Prove that every static the linker script INTENDS for DTCM actually landed there.

A linker input-section pattern that matches nothing is not an error. It gathers
zero bytes, the link succeeds, the symbol stays in main RAM, and the only
evidence is a performance arm that reads as noise -- which is indistinguishable
from the lever being wrong. That failure has cost this project a red gate more
than once by the neighbouring mechanism (`--gc-sections` silently dropping
counters), so a DTCM gather is not allowed to be believed without proof.

The intended set is parsed from `linker/nds_hot_text.ld` itself, between

    /* DTCM-RESIDENT HOT SCALARS: BEGIN */
    ...
    /* DTCM-RESIDENT HOT SCALARS: END */

so the check cannot drift from the thing it checks. Each line inside is an
ordinary input-section spec naming exactly one section, e.g.

    *(.bss.sNdsRendererTask36CaptureActive)

The symbol name is the part after the last `.`. Every one of them must resolve
in the linked ELF to an address inside the DTCM window, and the block must be
non-empty when it exists at all.

Usage:
    python scripts/check-dtcm-residency.py [--elf PATH] [--ld PATH]

Exit 0 when every intended symbol is resident (or when the block is absent, so
this is safe to wire into a gate before the lane lands). Exit 1 otherwise, with
every offending symbol named.
"""
from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _paths  # noqa: E402

REPO = _paths.REPO_ROOT

# `linker/nds_hot_text.ld:164-171`: DTCM data grows up from 0x02ff0000 and the
# ASSERT caps it at the boot stack's measured low-water mark. Anything outside
# this window is, by definition, not DTCM-resident.
DTCM_BASE = 0x02FF0000
DTCM_CEILING = 0x02FF3000

BEGIN = "/* DTCM-RESIDENT HOT SCALARS: BEGIN */"
END = "/* DTCM-RESIDENT HOT SCALARS: END */"

DEFAULT_LD = REPO / "linker" / "nds_hot_text.ld"
DEFAULT_ELF = (REPO / "builds" / "build-p2-fourcpu-tickhud" /
               "smash64ds-p2-fourcpu-tickhud-hwtri.elf")


def find_nm() -> str:
    for candidate in ("arm-none-eabi-nm",
                      r"C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe"):
        resolved = shutil.which(candidate) or (
            candidate if Path(candidate).exists() else None)
        if resolved:
            return resolved
    raise SystemExit("check-dtcm-residency: arm-none-eabi-nm not found")


def intended_symbols(ld_path: Path) -> list[tuple[str, str]]:
    """Return [(section, symbol)] the script asks the linker to put in DTCM."""
    text = ld_path.read_text(encoding="utf-8", errors="replace")
    if BEGIN not in text:
        return []
    if END not in text:
        raise SystemExit(
            f"check-dtcm-residency: {ld_path.name} opens the hot-scalar block "
            f"but never closes it")
    if text.count(BEGIN) != text.count(END):
        raise SystemExit(
            f"check-dtcm-residency: {ld_path.name} has {text.count(BEGIN)} "
            f"BEGIN markers and {text.count(END)} END markers")
    # There is more than one block on purpose: a `.data` static must land in
    # the LOADED .dtcm output section, and a `.bss` one in NOLOAD .dtcm.bss.
    # Putting a `.data` symbol in the NOLOAD section links fine and silently
    # drops its initialiser, so the two lists cannot be merged.
    body = "\n".join(chunk.split(END, 1)[0]
                     for chunk in text.split(BEGIN)[1:])
    out: list[tuple[str, str]] = []
    for raw in body.splitlines():
        line = raw.split("/*", 1)[0].strip()
        if not line:
            continue
        found = re.findall(r"\*\(\s*([^)\s]+)\s*\)", line)
        if not found:
            raise SystemExit(
                f"check-dtcm-residency: cannot parse input section from "
                f"{ld_path.name}: {raw.strip()!r}")
        for section in found:
            if "*" in section or "?" in section:
                raise SystemExit(
                    f"check-dtcm-residency: {section!r} is a wildcard. This "
                    f"block must name one section per entry so a gather that "
                    f"matches nothing can be detected.")
            out.append((section, section.rsplit(".", 1)[-1]))
    return out


def elf_symbol_addresses(nm: str, elf: Path) -> dict[str, int]:
    proc = subprocess.run([nm, str(elf)], capture_output=True, text=True)
    if proc.returncode != 0:
        raise SystemExit(f"check-dtcm-residency: nm failed on {elf}")
    table: dict[str, int] = {}
    for line in proc.stdout.splitlines():
        parts = line.split()
        if len(parts) != 3:
            continue
        addr, _kind, name = parts
        try:
            table[name] = int(addr, 16)
        except ValueError:
            continue
    return table


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--elf", type=Path, default=DEFAULT_ELF)
    parser.add_argument("--ld", type=Path, default=DEFAULT_LD)
    args = parser.parse_args()

    if not args.ld.exists():
        raise SystemExit(f"check-dtcm-residency: missing {args.ld}")
    intended = intended_symbols(args.ld)
    if not intended:
        print("DTCM_RESIDENCY_SKIP no hot-scalar block in "
              f"{args.ld.name}; nothing claimed, nothing to prove")
        return 0
    if not args.elf.exists():
        print(f"DTCM_RESIDENCY_FAIL {args.elf} does not exist; build it "
              f"before claiming residency")
        return 1

    addresses = elf_symbol_addresses(find_nm(), args.elf)
    missing: list[str] = []
    stranded: list[tuple[str, int]] = []
    resident: list[tuple[str, int]] = []
    for _section, symbol in intended:
        addr = addresses.get(symbol)
        if addr is None:
            missing.append(symbol)
        elif DTCM_BASE <= addr < DTCM_CEILING:
            resident.append((symbol, addr))
        else:
            stranded.append((symbol, addr))

    print(f"  DTCM residency: {len(resident)} of {len(intended)} intended "
          f"symbols inside [0x{DTCM_BASE:08x}, 0x{DTCM_CEILING:08x})")
    if resident:
        low = min(a for _s, a in resident)
        high = max(a for _s, a in resident)
        print(f"  span 0x{low:08x}..0x{high:08x}")
    if not missing and not stranded:
        print("DTCM_RESIDENCY_OK every symbol the linker script claims for "
              "DTCM is in DTCM")
        return 0

    print("DTCM_RESIDENCY_FAIL")
    for symbol in missing:
        print(f"  {symbol}: no such symbol in the ELF -- its input section "
              f"matched nothing and the gather silently did nothing")
    for symbol, addr in stranded:
        print(f"  {symbol}: at 0x{addr:08x}, outside DTCM -- claimed by the "
              f"script but left in main RAM")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())

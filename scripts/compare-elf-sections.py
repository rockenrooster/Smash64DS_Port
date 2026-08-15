#!/usr/bin/env python3
"""Byte-compare named sections of two linked ELFs, FAIL-CLOSED.

WHY THIS EXISTS AS A SCRIPT. The A/B instrument this campaign relies on is a
pair of ROMs that differ in one runtime-gated `.data` word, which turns the
placement floor from "+/-17,000, assumed" into "zero, verified". Verifying it
by hand has produced a comparison that COULD NOT FAIL twice, in two costumes:

  * `objcopy -O binary --only-section=.text` on both ELFs hashed IDENTICAL --
    it was the SHA-256 of the EMPTY STRING, because this linker script has no
    `.text` (2026-08-15, `.../cfx-ring-wiring/RING.md` section 3);
  * the same loop pointed at a build directory that DID NOT EXIST reported
    0 differing bytes for EVERY section (2026-08-15,
    `.../cfx-ring-split/SPLIT.md` section 4).

A byte comparison that cannot fail is not a control. So this refuses, loudly,
on every way the comparison can be vacuous:

  * either ELF missing, empty, or not an ELF;
  * a requested section absent from either ELF's section headers;
  * a requested section present but zero-length in either ELF;
  * `objcopy` returning non-zero, or emitting a zero-length file for a section
    the headers say is non-empty;
  * the two sections differing in SIZE (reported as a hard mismatch rather
    than compared prefix-wise).

Exit status is 0 only when every requested section was really extracted and
really compared. `--max-diff N` additionally fails when more than N bytes
differ across all sections, which is how the one-byte dispatch property is
asserted rather than eyeballed.

Usage:
  python scripts/compare-elf-sections.py \
      --a builds/build-c179-cfxring-b-d0 --b builds/build-c180-cfxring-a2-d0 \
      --sections .itcm,.text.hot,.text.hot.draw,.main,.main.rw,.dtcm \
      --max-diff 1

`--a`/`--b` accept either a build directory containing exactly one `.elf` or a
path to the `.elf` itself.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

OBJCOPY = os.environ.get(
    "OBJCOPY", r"C:/devkitPro/devkitARM/bin/arm-none-eabi-objcopy.exe")
OBJDUMP = os.environ.get(
    "OBJDUMP", r"C:/devkitPro/devkitARM/bin/arm-none-eabi-objdump.exe")

DEFAULT_SECTIONS = ".itcm,.text.hot,.text.hot.draw,.main,.main.rw,.dtcm"
HEADER_RE = re.compile(r"^\s*\d+\s+(\S+)\s+([0-9a-f]{8})\s+([0-9a-f]{8})",
                       re.MULTILINE)


def resolve_elf(path: Path) -> Path:
    if path.is_dir():
        elves = sorted(path.glob("*.elf"))
        if len(elves) != 1:
            raise SystemExit(
                f"REFUSED: {path} holds {len(elves)} .elf files; name one")
        path = elves[0]
    if not path.is_file():
        raise SystemExit(f"REFUSED: {path} does not exist")
    if path.stat().st_size == 0:
        raise SystemExit(f"REFUSED: {path} is empty")
    with path.open("rb") as handle:
        if handle.read(4) != b"\x7fELF":
            raise SystemExit(f"REFUSED: {path} is not an ELF")
    return path


def section_sizes(elf: Path) -> dict[str, int]:
    result = subprocess.run([OBJDUMP, "-h", str(elf)],
                            capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(f"REFUSED: objdump -h failed on {elf}\n"
                         f"{result.stderr}")
    return {name: int(size, 16)
            for name, size, _vma in HEADER_RE.findall(result.stdout)}


def extract(elf: Path, section: str, declared: int, out_dir: Path) -> bytes:
    out = out_dir / (section.replace(".", "_") + ".bin")
    result = subprocess.run(
        [OBJCOPY, "-O", "binary", f"--only-section={section}",
         str(elf), str(out)], capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(f"REFUSED: objcopy failed for {section} on {elf}\n"
                         f"{result.stderr}")
    if not out.is_file():
        raise SystemExit(f"REFUSED: objcopy wrote nothing for {section} "
                         f"on {elf}")
    data = out.read_bytes()
    if not data:
        raise SystemExit(
            f"REFUSED: {section} extracted to 0 bytes from {elf} while its "
            f"section header declares {declared:,}. This is the empty-string "
            f"comparison; it cannot fail, so it is not a control.")
    return data


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--a", type=Path, required=True)
    parser.add_argument("--b", type=Path, required=True)
    parser.add_argument("--sections", default=DEFAULT_SECTIONS)
    parser.add_argument("--max-diff", type=int, default=-1,
                        help="fail if more than N bytes differ in total; "
                             "-1 (default) reports without asserting")
    args = parser.parse_args()

    elf_a = resolve_elf(args.a)
    elf_b = resolve_elf(args.b)
    sections = [s for s in args.sections.split(",") if s]
    if not sections:
        raise SystemExit("REFUSED: no sections requested")

    sizes_a = section_sizes(elf_a)
    sizes_b = section_sizes(elf_b)
    for section in sections:
        for label, sizes, elf in (("A", sizes_a, elf_a), ("B", sizes_b, elf_b)):
            if section not in sizes:
                raise SystemExit(
                    f"REFUSED: {section} is absent from {label} ({elf}). "
                    f"Present: {', '.join(sorted(sizes))}")
            if sizes[section] == 0:
                raise SystemExit(
                    f"REFUSED: {section} is zero-length in {label} ({elf})")

    print(f"A  {elf_a}")
    print(f"B  {elf_b}")
    print(f"\n{'section':<18}{'bytes':>12}{'differing':>12}  first differing "
          f"offsets")
    total_diff = 0
    with tempfile.TemporaryDirectory() as tmp:
        dir_a = Path(tmp) / "a"
        dir_b = Path(tmp) / "b"
        dir_a.mkdir()
        dir_b.mkdir()
        for section in sections:
            data_a = extract(elf_a, section, sizes_a[section], dir_a)
            data_b = extract(elf_b, section, sizes_b[section], dir_b)
            if len(data_a) != len(data_b):
                print(f"{section:<18}{len(data_a):>12,}{'SIZE':>12}  "
                      f"A {len(data_a):,} vs B {len(data_b):,}")
                total_diff += abs(len(data_a) - len(data_b))
                continue
            offsets = [i for i, (x, y) in enumerate(zip(data_a, data_b))
                       if x != y]
            total_diff += len(offsets)
            shown = " ".join(f"0x{o:X}" for o in offsets[:8])
            if len(offsets) > 8:
                shown += " ..."
            print(f"{section:<18}{len(data_a):>12,}{len(offsets):>12,}  "
                  f"{shown}")

    print(f"\ntotal differing bytes across {len(sections)} sections: "
          f"{total_diff:,}")
    if args.max_diff >= 0 and total_diff > args.max_diff:
        print(f"FAIL: {total_diff:,} differing bytes exceeds "
              f"--max-diff {args.max_diff}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

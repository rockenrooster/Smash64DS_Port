#!/usr/bin/env python3
"""P5 T7: the music bank as Smash Remix plays it.

Remix appends instruments to the sequence bank (midi.asm add_instrument):
each new ALInstrument, ALSound, ALEnvelope, ALKeyMap, ALWaveTable, loop and
predictor book is assembled into Remix's code region, every pointer is an
offset from the bank file's RAM base (INST_CTL_TABLE_PC, 0x800472D0), the
bank's instrument array is moved there and extended past program 0x2A, and
the bank file's first bank offset is pointed at the moved copy. The new
samples' raw ADPCM is appended to the ROM, addressed from the sample table's
own start.

`remix_bank(staging)` returns (ctl, tbl) the vanilla BGM renderer
(scripts/sfx/bgm/render-audio-bgm.py) reads in place of the BattleShip
o2r's B1_sounds1 pair: the bank file at its RAM base with Remix's code region
mapped at its own RAM offsets, so every offset in it resolves, and the sample
table from its ROM start to the image's end. The vanilla instruments are the
same bytes either way (checked against the o2r's bank).
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "p4"))
import remix_rom  # noqa: E402

CTL_TABLE_PTR = 0x3D75C       # midi.asm INST_CTL_TABLE: read32 from the vanilla image
SAMPLE_DATA_PTR = 0x3D760     # midi.asm INST_SAMPLE_DATA
CTL_RAM = 0x800472D0          # midi.asm INST_CTL_TABLE_PC
RAM_END = 0x80800000          # the expansion pak's top: no bank pointer lies past it


class BankError(SystemExit):
    pass


def remix_bank(staging: Path) -> tuple[bytes, bytes]:
    rom = remix_rom.Rom(staging / "ssb64asm.z64")
    data = rom.data
    ctl_rom = struct.unpack_from(">I", data, CTL_TABLE_PTR)[0]
    tbl_rom = struct.unpack_from(">I", data, SAMPLE_DATA_PTR)[0]
    if not (0 < ctl_rom < tbl_rom < len(data)):
        raise BankError(f"bank pointers {ctl_rom:#x}/{tbl_rom:#x} outside the image")
    image = bytearray(RAM_END - CTL_RAM)
    # The bank file itself, up to the sample table that follows it in ROM.
    ctl_len = tbl_rom - ctl_rom
    image[0:ctl_len] = data[ctl_rom:tbl_rom]
    # Remix's code region at its RAM offset.
    code_at = remix_rom.REMIX_CODE_RAM - CTL_RAM
    code_len = min(len(data) - remix_rom.REMIX_CODE_ROM, len(image) - code_at)
    image[code_at:code_at + code_len] = data[remix_rom.REMIX_CODE_ROM:
                                             remix_rom.REMIX_CODE_ROM + code_len]
    return bytes(image), bytes(data[tbl_rom:])


def main() -> int:
    import argparse
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--staging", type=Path, required=True)
    args = ap.parse_args()
    ctl, tbl = remix_bank(args.staging)
    rev, banks = struct.unpack_from(">hh", ctl, 0)
    bank = struct.unpack_from(">I", ctl, 4)[0]
    count = struct.unpack_from(">h", ctl, bank)[0]
    print(f"remix_bank: revision {rev:#x}, {banks} bank(s), bank at +{bank:#x}, "
          f"{count} instruments, tbl {len(tbl)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())

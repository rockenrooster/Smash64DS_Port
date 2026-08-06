"""Unpack a Nintendo DS ROM into its parts.

Reads a .nds file the user owns and writes ARM9/ARM7 binaries, overlay modules,
and the internal filesystem into extracted/ (which is gitignored). Nothing here
is committed -- this is the local-only extraction step.

Usage:
    python tools/unpack.py path/to/rom.nds

Requires: pip install ndspy
"""
import sys
import pathlib

try:
    import ndspy.rom
    import ndspy.codeCompression as codeComp
except ImportError:
    sys.exit("ndspy not installed. Run: pip install ndspy")


def maybe_decompress(data: bytes) -> bytes:
    """SM64DS ships a BLZ-compressed ARM9 (and some overlays). Decompress when
    that produces a larger, valid image; otherwise return the data unchanged."""
    try:
        out = codeComp.decompress(data)
        return out if len(out) >= len(data) else data
    except Exception:
        return data


def main(rom_path: str) -> None:
    rom_path = pathlib.Path(rom_path)
    if not rom_path.is_file():
        sys.exit(f"ROM not found: {rom_path}")

    out = pathlib.Path(__file__).resolve().parent.parent / "extracted"
    out.mkdir(exist_ok=True)

    rom = ndspy.rom.NintendoDSRom.fromFile(str(rom_path))

    # Overlay tables are raw bytes (32 bytes per entry); load the actual modules.
    arm9_overlays = rom.loadArm9Overlays()
    arm7_overlays = rom.loadArm7Overlays()

    # Cartridge header summary -- confirms we're reading the right game.
    print("=== ROM header ===")
    print(f"  Title code : {rom.name!r}")
    print(f"  Game code  : {rom.idCode!r}")
    print(f"  Maker      : {rom.developerCode!r}")
    print(f"  ARM9 size  : {len(rom.arm9):,} bytes  (load 0x{rom.arm9RamAddress:08X})")
    print(f"  ARM7 size  : {len(rom.arm7):,} bytes  (load 0x{rom.arm7RamAddress:08X})")
    print(f"  Overlays   : {len(arm9_overlays)} ARM9 / {len(arm7_overlays)} ARM7")
    print(f"  Filesystem : {len(rom.files)} files")

    (out / "arm9.bin").write_bytes(rom.arm9)
    (out / "arm7.bin").write_bytes(rom.arm7)

    # ARM9 is BLZ-compressed in the ROM; also emit the decompressed image, which
    # is what carries readable code, strings, and symbols for disassembly/RE.
    arm9_dec = maybe_decompress(rom.arm9)
    if len(arm9_dec) != len(rom.arm9):
        (out / "arm9_dec.bin").write_bytes(arm9_dec)
        print(f"  ARM9 decompressed: {len(rom.arm9):,} -> {len(arm9_dec):,} bytes -> arm9_dec.bin")

    ov_dir = out / "overlays"
    ov_dir.mkdir(exist_ok=True)
    for ov_id, ov in arm9_overlays.items():
        (ov_dir / f"overlay_{ov_id:04d}.bin").write_bytes(ov.data)

    print(f"\nWrote arm9.bin, arm7.bin and {len(arm9_overlays)} overlays to {out}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])

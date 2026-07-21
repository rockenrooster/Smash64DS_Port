#!/usr/bin/env python3
"""Bake the exact EFCommon hit-spark frames into 16x16 DS OBJ cells."""

from __future__ import annotations

import hashlib
import struct
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SCB = ROOT / "decomp/BattleShip-main/BattleShip_o2r/particles/efcommon_particle_scb"
TXB = ROOT / "decomp/BattleShip-main/BattleShip_o2r/particles/efcommon_particle_txb"
SCB_SHA256 = "4c639924f0c1ce6e4b3d0c5b3d6b49605d237ff7b79816ddd26ff8631ab0eb1d"
TXB_SHA256 = "8bffc07309693cb79b29f4e4d1faf3fd29cb42a115ccb4ae143d9308480bc860"
LIGHT_TEXTURES = (33, 34, 35, 36)
HEAVY_TEXTURE = 41
HEAVY_ENV = ((255, 0, 0), (0, 255, 0), (0, 0, 255), (120, 120, 120))
ALPHA_THRESHOLD = 32


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def payload(path: Path, expected: str) -> bytes:
    actual = sha256(path)
    if actual != expected:
        raise SystemExit(f"{path}: SHA-256 {actual}, expected {expected}")
    data = path.read_bytes()
    if len(data) < 0x48 or struct.unpack_from(">I", data, 0x44)[0] > 1024:
        raise SystemExit(f"{path}: missing expected 0x44 O2R wrapper")
    return data[0x44:]


def texture(bank: bytes, texture_id: int) -> tuple[tuple[int, ...], tuple[int, ...]]:
    count = struct.unpack_from(">I", bank, 0)[0]
    if texture_id >= count:
        raise SystemExit(f"texture {texture_id} outside {count}-row bank")
    offset = struct.unpack_from(">I", bank, 4 + texture_id * 4)[0]
    header = struct.unpack_from(">6I", bank, offset)
    frames, fmt, _siz, _width, _height, flags = header
    pointer_count = frames + (1 if fmt == 2 and flags & 1 else frames if fmt == 2 else 0)
    pointers = struct.unpack_from(f">{pointer_count}I", bank, offset + 0x18)
    return header, pointers


def rgba16(value: int) -> tuple[int, int, int, int]:
    return (
        ((value >> 11) & 31) * 255 // 31,
        ((value >> 6) & 31) * 255 // 31,
        ((value >> 1) & 31) * 255 // 31,
        255 if value & 1 else 0,
    )


def decode_ci4(bank: bytes, texture_id: int, frame: int) -> Image.Image:
    header, pointers = texture(bank, texture_id)
    count, fmt, siz, width, height, _flags = header
    if (fmt, siz, width, height) != (2, 0, 32, 32) or frame >= min(count, 8):
        raise SystemExit(f"unexpected light texture {texture_id}: {header}")
    pixels = pointers[frame]
    palette = pointers[count + frame]
    colors = [rgba16(struct.unpack_from(">H", bank, palette + i * 2)[0]) for i in range(16)]
    image = Image.new("RGBA", (32, 32))
    out = image.load()
    for y in range(32):
        for x in range(32):
            source_x = x ^ (8 if y & 1 else 0)
            value = bank[pixels + y * 16 + source_x // 2]
            out[x, y] = colors[(value >> (4 if source_x % 2 == 0 else 0)) & 15]
    return image


def decode_ia16(bank: bytes, frame: int, env: tuple[int, int, int]) -> Image.Image:
    header, pointers = texture(bank, HEAVY_TEXTURE)
    count, fmt, siz, width, height, _flags = header
    if (count, fmt, siz, width, height) != (3, 3, 2, 32, 32) or frame >= count:
        raise SystemExit(f"unexpected heavy texture {HEAVY_TEXTURE}: {header}")
    pixels = pointers[frame]
    image = Image.new("RGBA", (32, 32))
    out = image.load()
    for y in range(32):
        for x in range(32):
            source_x = x ^ (2 if y & 1 else 0)
            intensity, alpha = struct.unpack_from("BB", bank, pixels + (y * 32 + source_x) * 2)
            out[x, y] = tuple(
                (env[channel] * (255 - intensity) + 255 * intensity + 127) // 255
                for channel in range(3)
            ) + (alpha,)
    return image


def pack_ds(image: Image.Image) -> bytes:
    source = image.load()
    packed = bytearray()
    for y in range(16):
        for x in range(16):
            taps = [source[x * 2 + dx, y * 2 + dy] for dy in range(2) for dx in range(2)]
            alpha_sum = sum(pixel[3] for pixel in taps)
            alpha = (alpha_sum + 2) // 4
            if alpha < ALPHA_THRESHOLD or alpha_sum == 0:
                value = 0
            else:
                rgb = [sum(pixel[c] * pixel[3] for pixel in taps) // alpha_sum for c in range(3)]
                value = 0x8000 | (rgb[0] >> 3) | ((rgb[1] >> 3) << 5) | ((rgb[2] >> 3) << 10)
            packed += struct.pack("<H", value)
    return bytes(packed)


def unpack_ds(data: bytes) -> Image.Image:
    image = Image.new("RGBA", (16, 16))
    out = image.load()
    for index, (value,) in enumerate(struct.iter_unpack("<H", data)):
        out[index % 16, index // 16] = (
            (value & 31) * 255 // 31,
            ((value >> 5) & 31) * 255 // 31,
            ((value >> 10) & 31) * 255 // 31,
            255 if value & 0x8000 else 0,
        )
    return image


def sheet(images: list[list[Image.Image]], cell: int) -> Image.Image:
    result = Image.new("RGBA", (8 * cell, 8 * cell))
    for row, frames in enumerate(images):
        for column, frame in enumerate(frames):
            result.alpha_composite(frame, (column * cell, row * cell))
    return result


def write_inc(data: bytes, digest: str) -> None:
    path = ROOT / "src/nds/generated/task39_hit_sparks.generated.inc"
    path.parent.mkdir(parents=True, exist_ok=True)
    rows = [", ".join(f"0x{byte:02x}" for byte in data[i : i + 16]) for i in range(0, len(data), 16)]
    text = (
        "/* Generated by scripts/generate_task39_hit_sparks.py. */\n"
        f"/* RGB555+A1 SHA256: {digest}. */\n"
        f"static const u8 sNdsTask39HitSparkPixels[{len(data)}] __attribute__((aligned(4))) = {{\n"
        + "\n".join(f"    {row}," for row in rows)
        + "\n};\n"
    )
    path.write_text(text, encoding="ascii", newline="\n")


def main() -> None:
    payload(SCB, SCB_SHA256)
    bank = payload(TXB, TXB_SHA256)
    light = [[decode_ci4(bank, texture_id, frame) for frame in range(8)] for texture_id in LIGHT_TEXTURES]
    heavy = [[decode_ia16(bank, frame, env) for frame in range(3)] for env in HEAVY_ENV]
    source_rows = light + heavy
    packed_frames = [pack_ds(frame) for row in source_rows for frame in row]
    data = b"".join(packed_frames)
    if len(data) != 22528 or not any(data):
        raise SystemExit(f"unexpected DS payload size/content: {len(data)}")
    digest = hashlib.sha256(data).hexdigest()

    binary = ROOT / "assets/effects/task39_hit_sparks.rgb5a1.bin"
    binary.parent.mkdir(parents=True, exist_ok=True)
    binary.write_bytes(data)
    write_inc(data, digest)

    visibility = ROOT / "artifacts/visibility"
    visibility.mkdir(parents=True, exist_ok=True)
    sheet(source_rows, 32).save(visibility / "2026-07-21_task39-hit-sparks-source.png")
    preview_rows = [[unpack_ds(pack_ds(frame)) for frame in row] for row in source_rows]
    sheet(preview_rows, 16).save(visibility / "2026-07-21_task39-hit-sparks-ds-preview.png")
    print(f"TASK39_HIT_SPARKS=PASS bytes={len(data)} sha256={digest}")


if __name__ == "__main__":
    main()

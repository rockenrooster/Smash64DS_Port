"""Read-only, source-qualified Meta Knight UI inputs for the native UI bakes."""

from __future__ import annotations

import hashlib
import importlib.util
import os
from pathlib import Path
import sys


SOURCE_RELATIVE = Path("decomp/smashremix-plus-extra/extra_characters/MetaKnight")
SOURCE_IMAGES = {
    "portrait.png": ((32, 32), "e51d85eea276eb516901902db70e0dc02eaa3408f6c594a1cf8ba14c32918c61", "RGBA5551"),
    "portrait_flash.png": ((32, 32), "834111b9c31284198cea3132b807fb3038002bcba2cac0957c07632bc831ddc6", "RGBA5551"),
    "nameplate.png": ((72, 16), "ffffc4128e63f3f2e0706cb88b31f73934a1ccaec86278d48459ce2b0256d151", "IA8"),
}
SOURCE_RAW_SHA256 = {
    "main.bin": "9b51e7065e9b44a4a276ab96f18c69ba794ea1ca0e52adcaf9f5cae1422c4f07",
    "character.bin": "59cefdb861812208253763953d6bdbff25590ddf14b381b14b933063733eaf86",
}


def enabled() -> bool:
    return os.environ.get("NDS_P4_METAKNIGHT") == "1"


def source_dir(repo_root: Path) -> Path:
    configured = os.environ.get("META_KNIGHT_SOURCE_DIR")
    return Path(configured).resolve() if configured else Path(repo_root) / SOURCE_RELATIVE


def _qualified_bytes(path: Path, expected: str) -> bytes:
    raw = path.read_bytes()
    actual = hashlib.sha256(raw).hexdigest()
    if actual != expected:
        raise ValueError(f"Meta Knight UI source drift: {path.name}: {actual}")
    return raw


def load_image(repo_root: Path, name: str):
    """Match EXTRA's image_appender RGBA5551/IA8 conversion before DS scaling."""
    from PIL import Image

    if name not in SOURCE_IMAGES:
        raise ValueError(f"unqualified Meta Knight UI image: {name}")
    dimensions, digest, mode = SOURCE_IMAGES[name]
    path = source_dir(repo_root) / name
    _qualified_bytes(path, digest)
    with Image.open(path) as image:
        if image.size != dimensions:
            raise ValueError(f"Meta Knight {name}: expected {dimensions}, got {image.size}")
        image = image.convert("RGBA")
        pixels = [image.getpixel((x, y)) for y in range(image.height)
                  for x in range(image.width)]
    converted = []
    for r, g, b, a in pixels:
        if mode == "RGBA5551":
            converted.append(tuple(((c >> 3) * 255) // 31 for c in (r, g, b)) +
                             (255 if a > 0 else 0,))
        else:
            intensity, alpha = (r >> 4) * 17, (a >> 4) * 17
            converted.append((intensity, intensity, intensity, alpha))
    width, height = dimensions
    return [converted[y * width:(y + 1) * width] for y in range(height)]


def load_stock(repo_root: Path):
    """Use the typed raw adapter's FTSprites closure, never a donor-parent icon."""
    source = source_dir(repo_root)
    raw = {name: _qualified_bytes(source / name, digest)
           for name, digest in SOURCE_RAW_SHA256.items()}
    adapter_path = Path(repo_root) / "scripts/fighters/extra_resource_adapter.py"
    spec = importlib.util.spec_from_file_location("nds_extra_ui_adapter", adapter_path)
    if spec is None or spec.loader is None:
        raise ValueError(f"cannot import typed stock adapter: {adapter_path}")
    adapter = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = adapter
    spec.loader.exec_module(adapter)
    stock = adapter.build_meta_knight_inventory(source, 0x624)["model"]["stock"]
    sprite = stock["stock_sprite"]
    bitmaps = sprite["bitmaps"]
    palettes = stock["stock_luts"]["palettes"]
    if ((sprite["width"], sprite["height"], sprite["fmt"], sprite["siz"]) !=
            (8, 10, 2, 0) or len(bitmaps) != 1 or len(palettes) != 6 or
            (sprite["attr"] & 0x200) == 0):
        raise ValueError("Meta Knight stock: CI4 8x10/six-costume contract drift")
    bitmap = bitmaps[0]
    if (bitmap["width_img"], bitmap["actual_height"]) != (16, 10):
        raise ValueError("Meta Knight stock: padded bitmap layout drift")
    pixels = bitmap["pixels"]
    if pixels["resource"] != "CHARACTER" or pixels["bytes"] != 80:
        raise ValueError("Meta Knight stock: unclassified pixel resource")
    character = raw["character.bin"]
    texture = character[pixels["offset"]:pixels["offset"] + pixels["bytes"]]
    palette_bytes = []
    for palette in palettes:
        if palette["resource"] != "CHARACTER" or palette["required_indexed_bytes"] != 32:
            raise ValueError("Meta Knight stock: unclassified costume palette")
        offset = palette["offset"]
        palette_bytes.append(character[offset:offset + 32])
    return texture, palette_bytes

#!/usr/bin/env python3
"""Prove the exact BattleShip IFCommon hybrid OBJ packing contract."""

from __future__ import annotations

import hashlib
import math
import re
import struct
import sys
from collections import Counter, deque
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


EXPECTED_O2R_SHA256 = (
    "aee5419f4515b03c06985f4d697db7e002604b617c7fbd5eca9bdc8207ad4efe"
)
EXPECTED_PALETTE = (
    0, 36996, 61307, 41224, 32768, 39110, 63421, 65535, 43338, 34882,
    45452, 59193, 52851, 54965, 50737, 47566, 57079, 41092, 40035, 39011,
    42149, 43173, 43206, 44263, 32774, 32778, 32780, 32782, 32772, 32836,
    32970, 33036, 33070, 32936, 36897, 41058, 45219, 47268, 43139, 38977,
    34816,
)
EXPECTED_STATS = {
    "assets": 16,
    "tiles": 25,
    "bitmap_bytes": 20480,
    "indexed_bytes": 10624,
    "alignment_waste_bytes": 64,
    "obj_span_bytes": 31168,
    "parity_pixels": 20864,
}
EXPECTED_SOURCE_ALPHA = (255,) * 16
EXPECTED_GO_SOURCE_ORIGINS = ((82, 93), (144, 93), (214, 93))
EXPECTED_GO_DS_ORIGINS = ((66, 74), (115, 74), (171, 74))
EXPECTED_GO_ATLAS_RECTS = ((0, 0, 50, 58), (50, 0, 56, 59), (106, 0, 19, 58))
GO_KERNEL_THRESHOLD_DEFAULT = 112
TRAFFIC_ALPHA_THRESHOLD = 8
GO_KERNEL_NATIVE_CROP = (58, 62, 141, 77)
GO_KERNEL_REVIEW_SIZE = (220, 120)
GO_KERNEL_REVIEW_SCALE = 3
GO_KERNEL_SKY_RGBA = (117, 199, 237, 255)
EXPECTED_GO_RGB555_SHA256 = (
    "05330f478fc2c1f0dd42f1563ebd0cfa4aa7814a1739c7e58ba8db7a3eb7b17c"
)
EXPECTED_TRAFFIC_ATLAS_RECTS = (
    (0, 0, 6, 42), (6, 0, 78, 26), (84, 0, 12, 9),
    (96, 0, 12, 9), (0, 42, 12, 12), (12, 42, 9, 9),
    (21, 42, 15, 15),
)
EXPECTED_CLOUD_ATLAS_SPECS = (
    (10, "light", 0, 197, 39, 26, 33, 0, 0, 4),
    (11, "light", 1, 99, 0, 24, 26, 0, 0, 4),
    (12, "light", 0, 197, 0, 37, 39, 0, 0, 4),
    (13, "contour", 0, 103, 0, 94, 114, 2, 0, 1),
    (14, "contour", 1, 0, 0, 99, 106, 6, 0, 2),
    (15, "contour", 0, 0, 0, 103, 108, 4, 2, 3),
)
EXPECTED_TRAFFIC_PALETTE = (
    0, 4228, 0, 2114, 6342, 5285, 14500, 3171,
    7399, 1057, 8456, 14, 15855, 9513, 6243, 302,
    24311, 10570, 13741, 9380, 11627, 12684, 20083, 14798,
    22197, 11, 19026, 26425, 17969, 30653, 168, 6,
)
EXPECTED_TRAFFIC_DISTINCT_COLORS = 80
EXPECTED_TRAFFIC_PALETTE_MAX_ERROR = 13
EXPECTED_TRAFFIC_NONZERO = (176, 2026, 97, 97, 132, 75, 201)
EXPECTED_GO_STROKE_RUNS = (
    (0, 13, ((5, 52),)),
    (1, 13, ((6, 52),)),
    (1, 40, ((4, 53),)),
    (2, 9, ((1, 39), (41, 55))),
)
EXPECTED_VISIBLE_PIXELS = (
    1963, 1906, 786, 229, 2950, 115, 115, 193, 109, 303,
    189, 92, 298, 0, 0, 0,
)
OBJ_ALIGNMENT = 128
OBJ_BANK_E_BYTES = 64 * 1024
PALETTE_ENTRIES = 256
SCREEN_SCALE_Q16 = 52429


def fail(message: str) -> None:
    raise AssertionError(message)


def number(token: str) -> int:
    return int(token.rstrip("uU"), 0)


def parse_manifest(source: str) -> list[dict[str, object]]:
    begin = source.index("static const NDSIFCommonAssetSpec")
    end = source.index("\n};\n\n#undef TILE", begin) + 3
    block = source[begin:end]
    token = r"(?:0x[0-9a-fA-F]+|[0-9]+)u?"
    asset_pattern = re.compile(
        r"\{\s*(" + token + r")\s*,\s*"
        + r"\s*,\s*".join([r"(" + token + r")"] * 7)
        + r"\s*,\s*\{(.*?)\}\s*\}",
        re.DOTALL,
    )
    assets: list[dict[str, object]] = []
    for match in asset_pattern.finditer(block):
        fields = [number(match.group(i)) for i in range(1, 9)]
        tiles = []
        for kind, tile_text in re.findall(
            r"\b(CLOUD_TILE|TILE)\(([^)]*)\)", match.group(9)
        ):
            tile = tuple(number(item.strip()) for item in tile_text.split(","))
            if kind == "TILE":
                if len(tile) != 8:
                    fail("hybrid OAM TILE does not have eight fields")
                tile += (0, 0, 15)
            else:
                if len(tile) != 9:
                    fail("hybrid OAM CLOUD_TILE does not have nine fields")
                tile = tile[:6] + (0, 0) + tile[6:]
            tiles.append(tile)
        tile_count = fields[7]
        if len(tiles) != tile_count:
            fail(
                f"asset {fields[0]:#x} declares {tile_count} live tiles, "
                f"parsed {len(tiles)}"
            )
        assets.append(
            {
                "offset": fields[0],
                "prim": tuple(fields[1:4]),
                "env": tuple(fields[4:7]),
                "tiles": tiles,
            }
        )
    if len(assets) != EXPECTED_STATS["assets"]:
        fail(f"expected 16 hybrid assets, parsed {len(assets)}")
    return assets


def load_o2r(path: Path) -> bytes:
    raw = path.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != EXPECTED_O2R_SHA256:
        fail(f"IFCommonGameStatus SHA-256 changed: {digest}")
    if raw[4:8] != b"OLER":
        fail("IFCommonGameStatus is not an O2R resource")
    if int.from_bytes(raw[0x40:0x44], "little") != 0x52:
        fail("IFCommonGameStatus O2R file id is not 0x52")
    extern_count = int.from_bytes(raw[0x48:0x4C], "little")
    size_offset = 0x40 + 12 + (extern_count * 2)
    data_size = int.from_bytes(raw[size_offset:size_offset + 4], "little")
    data = raw[size_offset + 4:size_offset + 4 + data_size]
    if len(data) != data_size:
        fail("IFCommonGameStatus O2R data payload is truncated")
    return data


def be_s16(data: bytes, offset: int) -> int:
    return struct.unpack_from(">h", data, offset)[0]


def reloc_target(data: bytes, offset: int) -> int:
    return (struct.unpack_from(">I", data, offset)[0] & 0xFFFF) * 4


def sprite_at(data: bytes, offset: int) -> dict[str, int]:
    if offset + 68 > len(data):
        fail(f"Sprite at {offset:#x} exceeds O2R data")
    sprite = {
        "width": be_s16(data, offset + 4),
        "height": be_s16(data, offset + 6),
        "attr": struct.unpack_from(">H", data, offset + 20)[0],
        "alpha": data[offset + 27],
        "nbitmaps": be_s16(data, offset + 40),
        "bmheight": be_s16(data, offset + 44),
        "format": data[offset + 48],
        "size": data[offset + 49],
        "bitmap": reloc_target(data, offset + 52),
    }
    if sprite["nbitmaps"] <= 0 or not (sprite["attr"] & 0x0200):
        fail(f"Sprite at {offset:#x} is not the expected TEXSHUF manifest")
    return sprite


def round_q16_half_up(value: int) -> int:
    return (value + 0x8000) >> 16


def bitmap_at(data: bytes, sprite: dict[str, int], index: int) -> dict[str, int]:
    offset = sprite["bitmap"] + (index * 16)
    if offset + 16 > len(data):
        fail(f"Bitmap {index} at {offset:#x} exceeds O2R data")
    return {
        "width": be_s16(data, offset),
        "width_img": be_s16(data, offset + 2),
        "buffer": reloc_target(data, offset + 8),
        "height": be_s16(data, offset + 12),
    }


def rgb15(red: int, green: int, blue: int) -> int:
    return 0x8000 | (red >> 3) | ((green >> 3) << 5) | ((blue >> 3) << 10)


def lerp_rgb15(prim: tuple[int, ...], env: tuple[int, ...], intensity: int) -> int:
    inverse = 255 - intensity
    color = tuple(
        ((prim[channel] * intensity) + (env[channel] * inverse) + 127) // 255
        for channel in range(3)
    )
    return rgb15(*color)


def decode_pixel(
    data: bytes,
    sprite: dict[str, int],
    prim: tuple[int, ...],
    env: tuple[int, ...],
    source_x: int,
    source_y: int,
) -> int:
    out_y = 0
    for bitmap_index in range(sprite["nbitmaps"]):
        bitmap = bitmap_at(data, sprite, bitmap_index)
        width = bitmap["width"]
        width_img = bitmap["width_img"] or width
        height = bitmap["height"] or sprite["bmheight"]
        advance = sprite["bmheight"] or height
        if width == 0:
            break
        if not (out_y <= source_y < out_y + height and source_x < width):
            out_y += advance
            continue

        local_y = source_y - out_y
        shuffled_x = source_x
        if local_y & 1:
            if sprite["size"] == 0:
                shuffled_x ^= 8
            elif sprite["size"] == 1:
                shuffled_x ^= 4
            elif sprite["format"] == 0 and sprite["size"] == 3:
                # RGBA32 TEXSHUF swaps the two 64-bit halves of each
                # 128-bit odd-row block: two pixels, not adjacent pixels.
                shuffled_x ^= 2

        buffer = bitmap["buffer"]
        if sprite["format"] == 0 and sprite["size"] == 3:
            offset = buffer + ((local_y * width_img + shuffled_x) * 4)
            rgba = struct.unpack_from(">I", data, offset)[0]
            return rgb15(rgba >> 24, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF) \
                if (rgba & 0xFF) >= 0x80 else 0
        if sprite["format"] == 3 and sprite["size"] == 1:
            value = data[buffer + (local_y * width_img) + shuffled_x]
            return lerp_rgb15(prim, env, (value >> 4) * 17) \
                if (value & 0xF) >= 8 else 0
        if sprite["format"] == 4 and sprite["size"] == 1:
            value = data[buffer + (local_y * width_img) + shuffled_x]
            return premultiplied_intensity(prim, value)
        if sprite["format"] == 4 and sprite["size"] == 0:
            row_bytes = (width_img + 1) // 2
            packed = data[buffer + (local_y * row_bytes) + (shuffled_x >> 1)]
            value = packed >> 4 if not (shuffled_x & 1) else packed & 0xF
            return premultiplied_intensity(prim, value * 17)
        fail(
            f"unsupported IFCommon format {sprite['format']}/{sprite['size']}"
        )
    return 0


def read_rgba32(
    data: bytes, sprite: dict[str, int], source_x: int, source_y: int
) -> int:
    out_y = 0
    for bitmap_index in range(sprite["nbitmaps"]):
        bitmap = bitmap_at(data, sprite, bitmap_index)
        width = bitmap["width"]
        width_img = bitmap["width_img"] or width
        height = bitmap["height"] or sprite["bmheight"]
        advance = sprite["bmheight"] or height
        if width == 0:
            break
        if out_y <= source_y < out_y + height and source_x < width:
            local_y = source_y - out_y
            shuffled_x = source_x ^ (2 if local_y & 1 else 0)
            offset = bitmap["buffer"] + (
                (local_y * width_img + shuffled_x) * 4
            )
            return struct.unpack_from(">I", data, offset)[0]
        out_y += advance
    fail(f"RGBA32 source coordinate is outside the sprite: {source_x},{source_y}")


def bilerp_channel(
    c00: int, c10: int, c01: int, c11: int,
    fraction_x: int, fraction_y: int,
) -> int:
    inverse_x = 256 - fraction_x
    inverse_y = 256 - fraction_y
    top = c00 * inverse_x + c10 * fraction_x
    bottom = c01 * inverse_x + c11 * fraction_x
    return (top * inverse_y + bottom * fraction_y + 0x8000) >> 16


def sample_prefiltered_go_pixel(
    data: bytes, sprite: dict[str, int], destination_x: int, destination_y: int
) -> tuple[int, int, int, int]:
    source_x_q8 = destination_x * 320 + 32
    source_y_q8 = destination_y * 320 + 32
    source_x = source_x_q8 >> 8
    source_y = source_y_q8 >> 8
    next_x = min(source_x + 1, sprite["width"] - 1)
    next_y = min(source_y + 1, sprite["height"] - 1)
    rgba = (
        read_rgba32(data, sprite, source_x, source_y),
        read_rgba32(data, sprite, next_x, source_y),
        read_rgba32(data, sprite, source_x, next_y),
        read_rgba32(data, sprite, next_x, next_y),
    )
    fraction_x = source_x_q8 & 0xFF
    fraction_y = source_y_q8 & 0xFF
    weights = (
        (256 - fraction_x) * (256 - fraction_y),
        fraction_x * (256 - fraction_y),
        (256 - fraction_x) * fraction_y,
        fraction_x * fraction_y,
    )
    weighted_alpha = sum(
        (value & 0xFF) * weight for value, weight in zip(rgba, weights)
    )
    alpha = (weighted_alpha + 0x8000) >> 16
    if not weighted_alpha:
        return 0, 0, 0, alpha
    channels = tuple(
        (
            sum(
                ((value >> shift) & 0xFF) * (value & 0xFF) * weight
                for value, weight in zip(rgba, weights)
            ) + weighted_alpha // 2
        ) // weighted_alpha
        for shift in (24, 16, 8)
    )
    return *channels, alpha


def decode_prefiltered_go_pixel(
    data: bytes, sprite: dict[str, int], destination_x: int, destination_y: int
) -> int:
    channels = sample_prefiltered_go_pixel(
        data, sprite, destination_x, destination_y
    )
    return rgb15(*channels[:3]) if channels[3] >= GO_KERNEL_THRESHOLD_DEFAULT else 0


def go_rgb15(red: int, green: int, blue: int) -> int:
    return (red >> 3) | ((green >> 3) << 5) | ((blue >> 3) << 10)


def go_color_distance(left: int, right: int) -> int:
    return sum(
        (((left >> shift) & 31) - ((right >> shift) & 31)) ** 2
        for shift in (0, 5, 10)
    )


def build_go_palette(
    samples: list[tuple[int, int, int, int]],
) -> tuple[tuple[int, ...], int, int]:
    frequencies = Counter(
        go_rgb15(*rgba[:3]) for rgba in samples
        if (rgba[3] * 7 + 127) // 255
    )
    colors = sorted(frequencies.items())
    if len(colors) < 31:
        fail("GO fixture has fewer than 31 visible RGB555 colors")

    centroids: list[int] = []
    for centroid_index in range(31):
        best_score = -1
        best_color = colors[0][0]
        for color, frequency in colors:
            score = frequency if centroid_index == 0 else frequency * min(
                go_color_distance(color, centroid) for centroid in centroids
            )
            if score > best_score or (
                score == best_score and color < best_color
            ):
                best_score = score
                best_color = color
        centroids.append(best_color)

    cluster_frequency = [0] * 31
    for _ in range(8):
        cluster_frequency = [0] * 31
        sums = [[0, 0, 0] for _ in range(31)]
        for color, frequency in colors:
            best_index = min(
                range(31),
                key=lambda index: go_color_distance(color, centroids[index]),
            )
            cluster_frequency[best_index] += frequency
            for channel, shift in enumerate((0, 5, 10)):
                sums[best_index][channel] += (
                    ((color >> shift) & 31) * frequency
                )
        for index, frequency in enumerate(cluster_frequency):
            if frequency:
                centroids[index] = sum(
                    ((sums[index][channel] + frequency // 2) // frequency)
                    << shift
                    for channel, shift in enumerate((0, 5, 10))
                )

    ranked = sorted(
        zip(cluster_frequency, centroids),
        key=lambda item: (-item[0], item[1]),
    )
    palette = (0, *(color for _, color in ranked))
    max_error = max(
        min(go_color_distance(color, entry) for entry in palette[1:])
        for color in frequencies
    )
    return palette, len(frequencies), max_error


def go_palette_index(palette: tuple[int, ...], rgba: tuple[int, ...]) -> int:
    color = go_rgb15(*rgba[:3])
    return min(
        range(1, 32),
        key=lambda index: go_color_distance(color, palette[index]),
    )


def source_rgba_image(
    data: bytes, sprite: dict[str, int]
) -> Image.Image:
    image = Image.new("RGBA", (sprite["width"], sprite["height"]))
    image.putdata([
        tuple((read_rgba32(data, sprite, x, y) >> shift) & 0xFF
              for shift in (24, 16, 8, 0))
        for y in range(sprite["height"])
        for x in range(sprite["width"])
    ])
    return image


def mitchell_kernel(value: float) -> float:
    value = abs(value)
    b = 1.0 / 3.0
    c = 1.0 / 3.0
    if value < 1.0:
        return (
            (12.0 - 9.0 * b - 6.0 * c) * value ** 3 +
            (-18.0 + 12.0 * b + 6.0 * c) * value ** 2 +
            (6.0 - 2.0 * b)
        ) / 6.0
    if value < 2.0:
        return (
            (-b - 6.0 * c) * value ** 3 +
            (6.0 * b + 30.0 * c) * value ** 2 +
            (-12.0 * b - 48.0 * c) * value +
            (8.0 * b + 24.0 * c)
        ) / 6.0
    return 0.0


def lanczos2_kernel(value: float) -> float:
    value = abs(value)
    if value == 0.0:
        return 1.0
    if value >= 2.0:
        return 0.0
    return (
        math.sin(math.pi * value) / (math.pi * value) *
        math.sin(math.pi * value / 2.0) / (math.pi * value / 2.0)
    )


def area_contributions(
    source_extent: int, destination_extent: int, destination: int
) -> tuple[tuple[int, float], ...]:
    left = destination * source_extent / destination_extent
    right = (destination + 1) * source_extent / destination_extent
    contributions = []
    for source in range(math.floor(left), math.ceil(right)):
        overlap = min(right, source + 1.0) - max(left, source)
        if overlap > 0.0:
            contributions.append((source, overlap))
    total = sum(weight for _, weight in contributions)
    return tuple((source, weight / total) for source, weight in contributions)


def reconstruction_contributions(
    source_extent: int,
    destination_extent: int,
    destination: int,
    kernel,
) -> tuple[tuple[int, float], ...]:
    scale = destination_extent / source_extent
    filter_scale = min(1.0, scale)
    center = (destination + 0.5) / scale
    support = 2.0 / filter_scale
    first = math.floor(center - support - 0.5)
    last = math.ceil(center + support - 0.5)
    combined: dict[int, float] = {}
    for source in range(first, last + 1):
        weight = kernel((center - (source + 0.5)) * filter_scale)
        if weight == 0.0:
            continue
        clamped = min(max(source, 0), source_extent - 1)
        combined[clamped] = combined.get(clamped, 0.0) + weight
    total = sum(combined.values())
    if abs(total) < 1.0e-12:
        fail("GO reconstruction kernel has zero total weight")
    return tuple(
        (source, weight / total)
        for source, weight in sorted(combined.items())
    )


def rgb555_opaque(red: int, green: int, blue: int) -> int:
    return 0x8000 | go_rgb15(red, green, blue)


def rgb555_rgba(value: int) -> tuple[int, int, int, int]:
    if not value & 0x8000:
        return 0, 0, 0, 0
    channels = tuple(
        (((value >> shift) & 31) << 3) | (((value >> shift) & 31) >> 2)
        for shift in (0, 5, 10)
    )
    return *channels, 255


def resample_go_rgb555(
    source: Image.Image,
    destination_size: tuple[int, int],
    kernel_name: str,
    alpha_threshold: int,
) -> tuple[Image.Image, bytes, int]:
    source = source.convert("RGBA")
    source_width, source_height = source.size
    destination_width, destination_height = destination_size
    source_pixels = list(source.getdata())
    if kernel_name == "area-box":
        x_taps = [
            area_contributions(source_width, destination_width, x)
            for x in range(destination_width)
        ]
        y_taps = [
            area_contributions(source_height, destination_height, y)
            for y in range(destination_height)
        ]
    else:
        kernel = {
            "mitchell": mitchell_kernel,
            "lanczos2": lanczos2_kernel,
        }[kernel_name]
        x_taps = [
            reconstruction_contributions(
                source_width, destination_width, x, kernel
            )
            for x in range(destination_width)
        ]
        y_taps = [
            reconstruction_contributions(
                source_height, destination_height, y, kernel
            )
            for y in range(destination_height)
        ]

    encoded: list[int] = []
    visible = 0
    for y_contributions in y_taps:
        for x_contributions in x_taps:
            alpha = 0.0
            premultiplied = [0.0, 0.0, 0.0]
            for source_y, weight_y in y_contributions:
                for source_x, weight_x in x_contributions:
                    red, green, blue, source_alpha = source_pixels[
                        source_y * source_width + source_x
                    ]
                    weight = weight_x * weight_y
                    source_alpha_unit = source_alpha / 255.0
                    alpha += source_alpha_unit * weight
                    for channel, value in enumerate((red, green, blue)):
                        premultiplied[channel] += (
                            value * source_alpha_unit * weight
                        )
            alpha = min(max(alpha, 0.0), 1.0)
            if alpha * 255.0 < alpha_threshold or alpha == 0.0:
                encoded.append(0)
                continue
            color = tuple(
                min(255, max(0, round(
                    min(max(value, 0.0), alpha * 255.0) / alpha
                )))
                for value in premultiplied
            )
            encoded.append(rgb555_opaque(*color))
            visible += 1

    image = Image.new("RGBA", destination_size)
    image.putdata([rgb555_rgba(value) for value in encoded])
    payload = b"".join(struct.pack("<H", value) for value in encoded)
    return image, payload, visible


def render_go_kernel_bakeoff(
    root: Path,
    data: bytes,
    sprites: list[dict[str, int]],
    alpha_threshold: int,
) -> None:
    visibility = root / "artifacts/visibility"
    visibility.mkdir(parents=True, exist_ok=True)
    crop_x, crop_y, crop_width, crop_height = GO_KERNEL_NATIVE_CROP
    kernels = (
        ("k1-area-box", "area-box"),
        ("k2-mitchell", "mitchell"),
        ("k3-lanczos2", "lanczos2"),
    )
    candidate_panels: list[tuple[str, Image.Image]] = []
    for label, kernel_name in kernels:
        frame = Image.new(
            "RGBA", (crop_width, crop_height), GO_KERNEL_SKY_RGBA
        )
        payload = bytearray()
        visible_counts = []
        for asset_index, (_, _, width, height) in enumerate(
            EXPECTED_GO_ATLAS_RECTS
        ):
            glyph, glyph_payload, visible = resample_go_rgb555(
                source_rgba_image(data, sprites[asset_index]),
                (width, height),
                kernel_name,
                alpha_threshold,
            )
            origin_x, origin_y = EXPECTED_GO_DS_ORIGINS[asset_index]
            frame.alpha_composite(
                glyph, (origin_x - crop_x, origin_y - crop_y)
            )
            payload.extend(glyph_payload)
            visible_counts.append(visible)
        review = frame.resize(
            GO_KERNEL_REVIEW_SIZE, Image.Resampling.NEAREST
        ).resize(
            tuple(
                extent * GO_KERNEL_REVIEW_SCALE
                for extent in GO_KERNEL_REVIEW_SIZE
            ),
            Image.Resampling.NEAREST,
        ).convert("RGB")
        output = visibility / (
            f"task11-kernels-{label}-t{alpha_threshold}-3x.png"
        )
        review.save(output)
        candidate_panels.append((label.replace("-", " ").upper(), review))
        print(
            f"  {label}: visible={tuple(visible_counts)}, "
            f"RGB555 SHA-256={hashlib.sha256(payload).hexdigest()}, "
            f"PNG={output.relative_to(root)}"
        )

    references = (
        (
            "BG3 FALLBACK",
            visibility /
            "task11-2026-07-16_ifcommon-go-fallback-frame198-3x.png",
            visibility / "task11-kernels-reference-bg3-fallback-3x.png",
        ),
        (
            "CURRENT A3I5 QUAD",
            visibility /
            "task11-2026-07-16_ifcommon-go-accepted-frame198-3x.png",
            visibility / "task11-kernels-reference-current-quad-3x.png",
        ),
    )
    reference_panels = []
    for label, source_path, output_path in references:
        if not source_path.exists():
            fail(f"GO kernel reference is missing: {source_path}")
        reference = Image.open(source_path).convert("RGB")
        if reference.size != tuple(
            extent * GO_KERNEL_REVIEW_SCALE
            for extent in GO_KERNEL_REVIEW_SIZE
        ):
            fail(f"GO kernel reference has unexpected size: {source_path}")
        reference.save(output_path)
        reference_panels.append((label, reference))

    panels = candidate_panels + reference_panels
    panel_width, panel_height = candidate_panels[0][1].size
    label_height = 30
    contact = Image.new(
        "RGB", (panel_width * 3, (panel_height + label_height) * 2), "black"
    )
    draw = ImageDraw.Draw(contact)
    try:
        font = ImageFont.load_default(size=20)
    except TypeError:
        font = ImageFont.load_default()
    for index, (label, panel) in enumerate(panels):
        column = index % 3
        row = index // 3
        left = column * panel_width
        top = row * (panel_height + label_height)
        draw.text((left + 8, top + 5), label, fill="white", font=font)
        contact.paste(panel, (left, top + label_height))
    draw.text(
        (panel_width * 2 + 8, panel_height + label_height + 5),
        f"T={alpha_threshold}; opaque RGB555",
        fill="white",
        font=font,
    )
    contact_path = visibility / (
        f"task11-kernels-contact-t{alpha_threshold}.png"
    )
    contact.save(contact_path)
    print(f"  contact: {contact_path.relative_to(root)}")


def connected_components(
    mask: list[bool], width: int, height: int
) -> list[set[tuple[int, int]]]:
    remaining = {
        (x, y) for y in range(height) for x in range(width)
        if mask[y * width + x]
    }
    components: list[set[tuple[int, int]]] = []
    while remaining:
        start = min(remaining, key=lambda point: (point[1], point[0]))
        queue = deque([start])
        component = {start}
        remaining.remove(start)
        while queue:
            x, y = queue.popleft()
            for next_y in range(max(0, y - 1), min(height, y + 2)):
                for next_x in range(max(0, x - 1), min(width, x + 2)):
                    point = next_x, next_y
                    if point in remaining:
                        remaining.remove(point)
                        component.add(point)
                        queue.append(point)
        components.append(component)
    return sorted(components, key=len, reverse=True)


def border_points(
    mask: list[bool], width: int, height: int
) -> set[tuple[int, int]]:
    border: set[tuple[int, int]] = set()
    for y in range(height):
        for x in range(width):
            if not mask[y * width + x]:
                continue
            if any(
                next_x < 0 or next_x >= width or
                next_y < 0 or next_y >= height or
                not mask[next_y * width + next_x]
                for next_x, next_y in (
                    (x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)
                )
            ):
                border.add((x, y))
    return border


def borders_within_one_pixel(
    left: set[tuple[int, int]], right: set[tuple[int, int]]
) -> bool:
    return all(
        any(abs(x - other_x) <= 1 and abs(y - other_y) <= 1
            for other_x, other_y in right)
        for x, y in left
    )


def largest_connected_run(points: set[tuple[int, int]]) -> int:
    remaining = set(points)
    largest = 0
    while remaining:
        start = remaining.pop()
        queue = deque([start])
        size = 1
        while queue:
            x, y = queue.popleft()
            for next_y in range(y - 1, y + 2):
                for next_x in range(x - 1, x + 2):
                    point = next_x, next_y
                    if point in remaining:
                        remaining.remove(point)
                        queue.append(point)
                        size += 1
        largest = max(largest, size)
    return largest


def read_i8(
    data: bytes, sprite: dict[str, int], source_x: int, source_y: int
) -> int:
    out_y = 0
    for bitmap_index in range(sprite["nbitmaps"]):
        bitmap = bitmap_at(data, sprite, bitmap_index)
        width = bitmap["width"]
        width_img = bitmap["width_img"] or width
        height = bitmap["height"] or sprite["bmheight"]
        advance = sprite["bmheight"] or height
        if width == 0:
            break
        if out_y <= source_y < out_y + height and source_x < width:
            local_y = source_y - out_y
            shuffled_x = source_x ^ (4 if local_y & 1 else 0)
            return data[bitmap["buffer"] + local_y * width_img + shuffled_x]
        out_y += advance
    fail(f"I8 source coordinate is outside the sprite: {source_x},{source_y}")


def read_i4(
    data: bytes, sprite: dict[str, int], source_x: int, source_y: int
) -> int:
    out_y = 0
    for bitmap_index in range(sprite["nbitmaps"]):
        bitmap = bitmap_at(data, sprite, bitmap_index)
        width = bitmap["width"]
        width_img = bitmap["width_img"] or width
        height = bitmap["height"] or sprite["bmheight"]
        advance = sprite["bmheight"] or height
        if width == 0:
            break
        if out_y <= source_y < out_y + height and source_x < width:
            local_y = source_y - out_y
            shuffled_x = source_x ^ (8 if local_y & 1 else 0)
            row_bytes = (width_img + 1) // 2
            packed = data[
                bitmap["buffer"] + local_y * row_bytes + (shuffled_x >> 1)
            ]
            return packed >> 4 if not shuffled_x & 1 else packed & 0xF
        out_y += advance
    fail(f"I4 source coordinate is outside the sprite: {source_x},{source_y}")


def traffic_rgba(
    data: bytes, sprite: dict[str, int], asset: dict[str, object],
    source_x: int, source_y: int,
) -> tuple[int, int, int, int]:
    if sprite["format"] == 3 and sprite["size"] == 1:
        value = read_i8(data, sprite, source_x, source_y)
        intensity = (value >> 4) * 17
        inverse = 255 - intensity
        rgb = tuple(
            (asset["prim"][channel] * intensity +
             asset["env"][channel] * inverse + 127) // 255
            for channel in range(3)
        )
        return *rgb, (value & 0xF) * 17
    if sprite["format"] == 4 and sprite["size"] == 0:
        return (
            *asset["prim"],
            read_i4(data, sprite, source_x, source_y) * 17,
        )
    fail(
        f"unsupported traffic format {sprite['format']}/{sprite['size']}"
    )


def sample_prefiltered_traffic_pixel(
    data: bytes, sprite: dict[str, int], asset: dict[str, object],
    destination_x: int, destination_y: int,
) -> tuple[int, int, int, int]:
    source_x_q8 = destination_x * 320 + 32
    source_y_q8 = destination_y * 320 + 32
    source_x = source_x_q8 >> 8
    source_y = source_y_q8 >> 8
    next_x = min(source_x + 1, sprite["width"] - 1)
    next_y = min(source_y + 1, sprite["height"] - 1)
    taps = (
        traffic_rgba(data, sprite, asset, source_x, source_y),
        traffic_rgba(data, sprite, asset, next_x, source_y),
        traffic_rgba(data, sprite, asset, source_x, next_y),
        traffic_rgba(data, sprite, asset, next_x, next_y),
    )
    fraction_x = source_x_q8 & 0xFF
    fraction_y = source_y_q8 & 0xFF
    weights = (
        (256 - fraction_x) * (256 - fraction_y),
        fraction_x * (256 - fraction_y),
        (256 - fraction_x) * fraction_y,
        fraction_x * fraction_y,
    )
    weighted_alpha = sum(
        rgba[3] * weight for rgba, weight in zip(taps, weights)
    )
    alpha = (weighted_alpha + 0x8000) >> 16
    if not weighted_alpha:
        return 0, 0, 0, alpha
    rgb = tuple(
        (
            sum(
                rgba[channel] * rgba[3] * weight
                for rgba, weight in zip(taps, weights)
            ) + weighted_alpha // 2
        ) // weighted_alpha
        for channel in range(3)
    )
    return *rgb, alpha


def opaque_traffic_rgba(
    rgba: tuple[int, int, int, int]
) -> tuple[int, int, int, int]:
    if rgba[3] < TRAFFIC_ALPHA_THRESHOLD:
        return 0, 0, 0, 0
    return *tuple(
        (channel * rgba[3] + 127) // 255 for channel in rgba[:3]
    ), 255


def sample_i8_q8(
    data: bytes, sprite: dict[str, int], source_x_q8: int, source_y_q8: int
) -> int:
    def axis(value: int, extent: int) -> tuple[int, int, int]:
        if value <= 0:
            return 0, 0, 0
        source = value >> 8
        fraction = value & 0xFF
        if source + 1 >= extent:
            source = extent - 1
            return source, source, 0
        return source, source + 1, fraction

    source_x, next_x, fraction_x = axis(source_x_q8, sprite["width"])
    source_y, next_y, fraction_y = axis(source_y_q8, sprite["height"])
    return bilerp_channel(
        read_i8(data, sprite, source_x, source_y),
        read_i8(data, sprite, next_x, source_y),
        read_i8(data, sprite, source_x, next_y),
        read_i8(data, sprite, next_x, next_y),
        fraction_x,
        fraction_y,
    )


def prefiltered_i8(
    data: bytes, sprite: dict[str, int], destination_x: int, destination_y: int
) -> int:
    return sample_i8_q8(
        data, sprite, destination_x * 320 + 32, destination_y * 320 + 32
    )


def decode_prefiltered_light_pixel(
    data: bytes,
    sprite: dict[str, int],
    prim: tuple[int, ...],
    destination_x: int,
    destination_y: int,
) -> int:
    intensity = prefiltered_i8(data, sprite, destination_x, destination_y)
    return rgb15(255, 255, 255) if intensity >= 112 else 0


def premultiplied_intensity(prim: tuple[int, ...], intensity: int) -> int:
    level = (intensity * 15 + 127) // 255
    if not level:
        return 0
    return rgb15(*((channel * level + 7) // 15 for channel in prim))


def decode_asset_pixel(
    data: bytes,
    sprite: dict[str, int],
    asset: dict[str, object],
    tile: tuple[int, ...],
    asset_index: int,
    destination_x: int,
    destination_y: int,
) -> int:
    if asset_index < 3:
        return decode_prefiltered_go_pixel(
            data, sprite, destination_x, destination_y
        )
    if 10 <= asset_index <= 12:
        return decode_prefiltered_light_pixel(
            data, sprite, asset["prim"], destination_x, destination_y
        )
    return decode_pixel(
        data, sprite, asset["prim"], asset["env"],
        destination_x, destination_y,
    )


def tile_offset(width: int, x: int, y: int) -> int:
    return (((y >> 3) * (width >> 3) + (x >> 3)) * 64) + \
        ((y & 7) * 8) + (x & 7)


def opaque_runs(values: list[bool]) -> tuple[tuple[int, int], ...]:
    runs: list[tuple[int, int]] = []
    start = -1
    for index, visible in enumerate(values + [False]):
        if visible and start < 0:
            start = index
        elif not visible and start >= 0:
            runs.append((start, index - 1))
            start = -1
    return tuple(runs)


def composite_images(
    placements: list[tuple[Image.Image, int, int]]
) -> Image.Image:
    left = min(x for _, x, _ in placements)
    top = min(y for _, _, y in placements)
    right = max(x + image.width for image, x, _ in placements)
    bottom = max(y + image.height for image, _, y in placements)
    output = Image.new("RGBA", (right - left, bottom - top))
    for source, x, y in placements:
        output.alpha_composite(source, (x - left, y - top))
    return output


def traffic_source_image(
    data: bytes, sprite: dict[str, int], asset: dict[str, object]
) -> Image.Image:
    image = Image.new("RGBA", (sprite["width"], sprite["height"]))
    image.putdata([
        traffic_rgba(data, sprite, asset, x, y)
        for y in range(sprite["height"])
        for x in range(sprite["width"])
    ])
    return image


def flare_source_image(
    data: bytes, sprite: dict[str, int], color: tuple[int, ...]
) -> Image.Image:
    image = Image.new("RGBA", (sprite["width"], sprite["height"]))
    image.putdata([
        (*color, read_i8(data, sprite, x, y))
        for y in range(sprite["height"])
        for x in range(sprite["width"])
    ])
    return image


def direct_go_image(
    data: bytes, sprite: dict[str, int], width: int, height: int
) -> Image.Image:
    image = Image.new("RGBA", (width, height))
    image.putdata([
        rgb555_rgba(decode_prefiltered_go_pixel(data, sprite, x, y))
        for y in range(height)
        for x in range(width)
    ])
    return image


def traffic_atlas_image(
    samples: list[tuple[int, int, int, int]], width: int, height: int,
    palette: tuple[int, ...],
) -> Image.Image:
    image = Image.new("RGBA", (width, height))
    image.putdata([
        (0, 0, 0, 0) if not rgba[3] else
        rgb555_rgba(0x8000 | palette[go_palette_index(palette, rgba)])
        for rgba in samples
    ])
    return image


def prepared_flare_image(
    data: bytes, sprite: dict[str, int], spec: tuple[object, ...]
) -> Image.Image:
    _, kind, _, _, _, width, height, source_x, source_y, palette_index = spec
    palette = (
        (0, 0, 0), (255, 57, 57), (255, 165, 0),
        (33, 99, 255), (255, 255, 255),
    )
    pixels = []
    for y in range(height):
        for x in range(width):
            destination_x = source_x + x
            destination_y = source_y + y
            if kind == "light":
                intensity = sample_i8_q8(
                    data, sprite,
                    destination_x * 320 + 32,
                    destination_y * 320 + 32,
                )
            else:
                intensity = sample_i8_q8(
                    data, sprite,
                    destination_x * 128 - 64,
                    destination_y * 128 - 64,
                )
            alpha = 0 if intensity < 8 else (intensity * 31 + 127) // 255
            pixels.append(
                (0, 0, 0, 0) if not alpha else
                (*palette[palette_index], (alpha << 3) | (alpha >> 2))
            )
    image = Image.new("RGBA", (width, height))
    image.putdata(pixels)
    return image


def export_source_assets(
    root: Path, data: bytes, sprites: list[dict[str, int]],
    assets: list[dict[str, object]],
    traffic_samples: list[list[tuple[int, int, int, int]]],
    traffic_palette: tuple[int, ...],
) -> None:
    visibility = root / "artifacts/visibility"
    visibility.mkdir(parents=True, exist_ok=True)

    go_source = composite_images([
        (source_rgba_image(data, sprites[index]), *origin)
        for index, origin in enumerate(EXPECTED_GO_SOURCE_ORIGINS)
    ])
    go_source_path = visibility / \
        "task11-original-n64-go-rgba32-deinterleaved.png"
    go_source.save(go_source_path)
    go_ds = composite_images([
        (direct_go_image(data, sprites[index], width, height), *origin)
        for index, (origin, (_, _, width, height)) in enumerate(zip(
            EXPECTED_GO_DS_ORIGINS, EXPECTED_GO_ATLAS_RECTS
        ))
    ])
    go_ds_path = visibility / "task11-source-clean-go-ds-rgb555.png"
    go_ds.save(go_ds_path)
    go_ds.resize(
        (go_ds.width * 4, go_ds.height * 4), Image.Resampling.NEAREST
    ).save(visibility / "task11-source-clean-go-ds-rgb555-4x.png")

    traffic_source = {
        index: traffic_source_image(data, sprites[index], assets[index])
        for index in range(3, 10)
    }
    source_positions = (
        (3, 103, -4), (4, 111, 30),
        (7, 123, 40), (8, 140, 42), (8, 153, 42),
        (8, 166, 42), (9, 180, 38),
    )
    for label, shadow_index in (("initial", 5), ("go", 6)):
        light_box = composite_images([
            (traffic_source[index], x, y)
            for index, x, y in source_positions
        ] + [(traffic_source[shadow_index], 182, 42)])
        light_box.save(
            visibility /
            f"task11-original-n64-traffic-light-box-{label}.png"
        )

    traffic_prepared = {
        index: traffic_atlas_image(
            traffic_samples[index - 3],
            EXPECTED_TRAFFIC_ATLAS_RECTS[index - 3][2],
            EXPECTED_TRAFFIC_ATLAS_RECTS[index - 3][3],
            traffic_palette,
        )
        for index in range(3, 10)
    }
    traffic_atlas = Image.new("RGBA", (128, 64))
    for index, (atlas_x, atlas_y, _, _) in enumerate(
        EXPECTED_TRAFFIC_ATLAS_RECTS, start=3
    ):
        traffic_atlas.alpha_composite(
            traffic_prepared[index], (atlas_x, atlas_y)
        )
    traffic_atlas.save(
        visibility / "task11-source-clean-traffic-light-ds-a3i5-atlas.png"
    )
    ds_positions = [
        (index,
         (x * SCREEN_SCALE_Q16 + 0x8000) >> 16,
         ((y * SCREEN_SCALE_Q16 + 0x8000) >> 16) if y >= 0 else
         -(((-y * SCREEN_SCALE_Q16) + 0x8000) >> 16))
        for index, x, y in source_positions
    ]
    traffic_ds = composite_images([
        (traffic_prepared[index], x, y) for index, x, y in ds_positions
    ] + [(traffic_prepared[6], 146, 34)])
    traffic_ds.save(
        visibility / "task11-source-clean-traffic-light-box-ds.png"
    )
    traffic_ds.resize(
        (traffic_ds.width * 4, traffic_ds.height * 4),
        Image.Resampling.NEAREST,
    ).save(visibility / "task11-source-clean-traffic-light-box-ds-4x.png")

    flare_source = {
        index: flare_source_image(
            data, sprites[index],
            (255, 255, 255) if index < 13 else assets[index]["prim"],
        )
        for index in range(10, 16)
    }
    flare_pairs = [
        composite_images([
            (flare_source[contour], 0, 0),
            (flare_source[light], offset_x, offset_y),
        ])
        for light, contour, offset_x, offset_y in (
            (10, 13, 8, 7), (11, 14, 12, 11), (12, 15, 5, 3)
        )
    ]
    flare_contact = composite_images([
        (image, sum(pair.width + 8 for pair in flare_pairs[:index]), 0)
        for index, image in enumerate(flare_pairs)
    ])
    flare_contact.save(
        visibility / "task11-original-n64-flare-source.png"
    )

    flare_atlases = [Image.new("RGBA", (256, 128)) for _ in range(2)]
    for spec in EXPECTED_CLOUD_ATLAS_SPECS:
        asset_index, _, atlas_index, atlas_x, atlas_y, *_ = spec
        flare_atlases[atlas_index].alpha_composite(
            prepared_flare_image(data, sprites[asset_index], spec),
            (atlas_x, atlas_y),
        )
    for atlas_index, atlas in enumerate(flare_atlases):
        atlas.save(
            visibility /
            f"task11-source-clean-flare-ds-a5i3-atlas{atlas_index}.png"
        )

    print("  source export:")
    for path in (
        go_source_path, go_ds_path,
        visibility / "task11-original-n64-traffic-light-box-go.png",
        visibility / "task11-original-n64-flare-source.png",
        visibility / "task11-source-clean-traffic-light-ds-a3i5-atlas.png",
        visibility / "task11-source-clean-flare-ds-a5i3-atlas0.png",
        visibility / "task11-source-clean-flare-ds-a5i3-atlas1.png",
    ):
        print(f"    {path.relative_to(root)}")


def check_runtime_contract(root: Path, source: str) -> None:
    header = (root / "include/nds/nds_ifcommon_oam.h").read_text()
    renderer = (root / "src/nds/nds_renderer.c").read_text()
    platform = (root / "src/nds/nds_platform.c").read_text()
    taskman = (root / "src/port/taskman_seam.c").read_text()
    if "#ifndef NDS_IFCOMMON_HYBRID_OAM\n#define NDS_IFCOMMON_HYBRID_OAM 0" not in header:
        fail("NDS_IFCOMMON_HYBRID_OAM is not default-off in the public seam")
    for fragment in (
        "SpriteMapping_Bmp_1D_128",
        "SpriteColorFormat_256Color",
        "NDS_IFCOMMON_OBJ_GFX_ALIGNMENT",
        "memcpy(SPRITE_PALETTE, palette_storage",
        "ndsIFCommonSamplePrefilteredGoPixel(",
        "ndsIFCommonSamplePrefilteredTrafficPixel(",
        "weighted_premultiplied",
        "NDS_IFCOMMON_GO_ALPHA_THRESHOLD 112u",
        "NDS_IFCOMMON_TRAFFIC_ALPHA_THRESHOLD 8u",
        "texshuf_x ^= 2u",
        "SpriteColorFormat_Bmp",
        "(7u << 5)",
        "ndsIFCommonDecodePrefilteredLightPixel(",
        "ndsIFCommonPremultipliedIntensity(",
        "ndsIFCommonPrefilterCloudIntensity(",
        "ndsIFCommonPrefilterLightIntensity(",
        "ndsRendererHardwareDrawIFCommonCloudAtlas(",
        "NDS_IFCOMMON_TRAFFIC_ATLAS_BYTES",
        "NDS_IFCOMMON_CLOUD_SPEC_COUNT 6u",
        "(sobj->sprite.alpha != 255u)",
        "ndsIFCommonRoundFloatHalfUp(",
        "ndsIFCommonRoundQ16HalfUp(",
        "ndsIFCommonNativeOamReleasePreGoTextures(void)",
        "gNdsIFCommonNativeOamPreGoReleaseBytes +=",
        "(rgba & 0xffu) < 0x80u",
        "((ia & 0x0fu) >= 8u)",
    ):
        if fragment not in source:
            fail(f"hybrid owner is missing runtime contract: {fragment}")
    for fragment in (
        "vramSetBankE(VRAM_E_MAIN_SPRITE);",
        "vramSetBankF(VRAM_F_TEX_PALETTE_SLOT0);",
        "vramSetBankG(VRAM_G_TEX_PALETTE_SLOT1);",
    ):
        if fragment not in platform:
            fail(f"platform is missing hybrid F/G ownership: {fragment}")
    for fragment in (
        "gNdsRendererIFCommonCloudQueuedCount++",
        "gNdsRendererIFCommonCloudEmittedCount++",
        "gNdsRendererIFCommonCloudQueuedCount = 0u",
        "gNdsRendererIFCommonCloudEmittedCount = 0u",
        "NDS_RENDERER_IFCOMMON_CLOUD_QUEUE_COUNT 16u",
        "-4080 - (s32)draw_index",
        "GL_RGB32_A3",
        "ndsRendererHardwarePrepareIFCommonA3I5Atlas(",
    ):
        if fragment not in renderer:
            fail(f"renderer is missing IFCommon queue proof: {fragment}")
    for fragment in (
        "gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[16]",
        "gNdsIFCommonNativeOamPreGoReleaseCount",
        "gNdsIFCommonNativeOamPreGoReleaseBytes",
    ):
        if fragment not in header:
            fail(f"IFCommon header is missing atlas proof: {fragment}")
    release = taskman.find("ndsIFCommonNativeOamReleasePreGoTextures();")
    arm = taskman.find("ndsRendererHardwareArmBattleStaticTextures();", release)
    if release < 0 or arm < release:
        fail("yellow-atlas release and M4 arm are not source ordered")
    for obsolete in (
        "sNdsIFCommonGoTextureName",
        "ndsIFCommonBuildGoPalette(",
        "NDS_IFCOMMON_GO_ATLAS_BYTES",
    ):
        if obsolete in source:
            fail(f"obsolete GO palette/quad route remains: {obsolete}")
    gameplay = source[source.index("void ndsIFCommonNativeOamBeginFrame") :]
    for forbidden in (
        "ndsIFCommonDecodePixel(",
        "SPRITE_GFX",
        "SPRITE_PALETTE",
        "dmaFill",
        "dmaCopy",
    ):
        if forbidden in gameplay:
            fail(f"post-prepare IFCommon path contains forbidden work: {forbidden}")


def verify(
    root: Path,
    render_kernel_bakeoff: bool = False,
    kernel_threshold: int = GO_KERNEL_THRESHOLD_DEFAULT,
    export_sources: bool = False,
) -> None:
    source = (root / "src/nds/nds_ifcommon_oam.c").read_text()
    assets = parse_manifest(source)
    if any(not assets[index]["tiles"] for index in range(13)):
        fail("GO/traffic OAM asset is missing its source-ordered tiles")
    if any(assets[index]["tiles"] for index in range(13, 16)):
        fail("source-alpha Contour unexpectedly consumes OBJ tiles")
    data = load_o2r(
        root / "decomp/BattleShip-main/BattleShip_o2r/reloc_interface/"
        "IFCommonGameStatus"
    )
    sprites = [sprite_at(data, int(asset["offset"])) for asset in assets]
    source_alpha = tuple(sprite["alpha"] for sprite in sprites)
    if source_alpha != EXPECTED_SOURCE_ALPHA:
        fail(f"IFCommon source Sprite alpha changed: {source_alpha}")
    go_origins = tuple(
        (round_q16_half_up(x * SCREEN_SCALE_Q16),
         round_q16_half_up(y * SCREEN_SCALE_Q16))
        for x, y in EXPECTED_GO_SOURCE_ORIGINS
    )
    if go_origins != EXPECTED_GO_DS_ORIGINS:
        fail(f"GO integer-aligned origins changed: {go_origins}")
    for asset_index, (_, _, width, height) in enumerate(
        EXPECTED_GO_ATLAS_RECTS
    ):
        if assets[asset_index]["tiles"][0][:4] != (0, 0, width, height):
            fail(f"GO bitmap footprint changed for asset {asset_index}")

    go_payload = bytearray()
    go_visible = []
    for asset_index, (_, _, width, height) in enumerate(
        EXPECTED_GO_ATLAS_RECTS
    ):
        pixels = [
            decode_prefiltered_go_pixel(
                data, sprites[asset_index], x, y
            )
            for y in range(height)
            for x in range(width)
        ]
        go_payload.extend(
            b"".join(struct.pack("<H", pixel) for pixel in pixels)
        )
        go_visible.append(sum(pixel != 0 for pixel in pixels))
    go_digest = hashlib.sha256(go_payload).hexdigest()
    if go_digest != EXPECTED_GO_RGB555_SHA256:
        fail(f"direct GO RGB555 payload changed: {go_digest}")

    traffic_samples: list[list[tuple[int, int, int, int]]] = []
    for asset_index, rect in enumerate(
        EXPECTED_TRAFFIC_ATLAS_RECTS, start=3
    ):
        _, _, width, height = rect
        traffic_samples.append([
            opaque_traffic_rgba(sample_prefiltered_traffic_pixel(
                data, sprites[asset_index], assets[asset_index], x, y,
            ))
            for y in range(height)
            for x in range(width)
        ])
    traffic_palette, traffic_distinct, traffic_max_error = build_go_palette(
        [pixel for sprite in traffic_samples for pixel in sprite]
    )
    if traffic_palette != EXPECTED_TRAFFIC_PALETTE:
        fail(f"source-derived traffic palette changed: {traffic_palette}")
    if traffic_distinct != EXPECTED_TRAFFIC_DISTINCT_COLORS:
        fail(
            f"traffic distinct colors changed: {traffic_distinct} != "
            f"{EXPECTED_TRAFFIC_DISTINCT_COLORS}"
        )
    if traffic_max_error > EXPECTED_TRAFFIC_PALETTE_MAX_ERROR:
        fail(
            f"traffic palette error {traffic_max_error} exceeds "
            f"{EXPECTED_TRAFFIC_PALETTE_MAX_ERROR}"
        )
    traffic_nonzero = tuple(
        sum(pixel[3] != 0 for pixel in samples)
        for samples in traffic_samples
    )
    if traffic_nonzero != EXPECTED_TRAFFIC_NONZERO:
        fail(f"traffic visible texels changed: {traffic_nonzero}")
    for lamp_index, samples in enumerate(traffic_samples[4:], start=7):
        colors = {go_rgb15(*pixel[:3]) for pixel in samples if pixel[3]}
        if len(colors) < 3:
            fail(f"dim lamp {lamp_index} flattened to {sorted(colors)}")

    palette_match = re.search(
        r"static const u16 sNdsIFCommonTrafficPalette\[32\]\s*=\s*"
        r"\{(.*?)\};",
        source,
        re.DOTALL,
    )
    if palette_match is None:
        fail("traffic A3I5 palette declaration is missing")
    c_traffic_palette = tuple(
        number(token) for token in re.findall(
            r"(?:0x[0-9a-fA-F]+|[0-9]+)u?", palette_match.group(1)
        )
    )
    if c_traffic_palette != traffic_palette:
        fail(f"runtime traffic palette differs from host fixture: {c_traffic_palette}")

    visible_pixels = []
    for asset_index, asset in enumerate(assets):
        count = 0
        if asset_index < 3:
            _, _, width, height = EXPECTED_GO_ATLAS_RECTS[asset_index]
            count = sum(
                decode_prefiltered_go_pixel(
                    data, sprites[asset_index], x, y
                ) != 0
                for y in range(height)
                for x in range(width)
            )
        else:
            for tile in asset["tiles"]:
                source_x, source_y, width, height = tile[:4]
                count += sum(
                    decode_asset_pixel(
                        data, sprites[asset_index], asset, tile, asset_index,
                        source_x + x, source_y + y,
                    ) != 0
                    for y in range(height)
                    for x in range(width)
                )
        visible_pixels.append(count)
    if tuple(visible_pixels) != EXPECTED_VISIBLE_PIXELS:
        fail(f"source-alpha visible pixels changed: {tuple(visible_pixels)}")

    go_stroke_runs = []
    for asset_index, column, expected_runs in EXPECTED_GO_STROKE_RUNS:
        asset = assets[asset_index]
        height = EXPECTED_GO_ATLAS_RECTS[asset_index][3]
        runs = opaque_runs([
            decode_prefiltered_go_pixel(
                data, sprites[asset_index], column, y,
            ) != 0
            for y in range(height)
        ])
        go_stroke_runs.append((asset_index, column, runs))
        if runs != expected_runs:
            fail(
                f"GO stroke column {asset_index}:{column} regained a row "
                f"discontinuity: {runs} != {expected_runs}"
            )

    palette = [0]
    palette_index = {0: 0}
    for asset_index, asset in enumerate(assets[3:], start=3):
        for tile in asset["tiles"]:
            source_x, source_y, width, height = tile[:4]
            for y in range(height):
                for x in range(width):
                    color = decode_asset_pixel(
                        data, sprites[asset_index], asset, tile, asset_index,
                        source_x + x, source_y + y,
                    )
                    if color not in palette_index:
                        if len(palette) >= PALETTE_ENTRIES:
                            fail("IFCommon indexed colors exceed one OBJ palette")
                        palette_index[color] = len(palette)
                        palette.append(color)
    if tuple(palette) != EXPECTED_PALETTE:
        fail(f"deterministic IFCommon RGB15 palette changed: {tuple(palette)}")

    stats = {
        "assets": len(assets),
        "tiles": 0,
        "bitmap_bytes": 0,
        "indexed_bytes": 0,
        "alignment_waste_bytes": 0,
        "obj_span_bytes": 0,
        "parity_pixels": 0,
    }
    cursor = 0
    for asset_index, asset in enumerate(assets):
        sprite = sprites[asset_index]
        for tile in asset["tiles"]:
            source_x, source_y, width, height, cell_width, cell_height, \
                pad_x, pad_y, _, _, _ = tile
            if cell_width % 8 or cell_height % 8:
                fail("standard OBJ cell is not an 8-pixel multiple")
            aligned = (cursor + OBJ_ALIGNMENT - 1) & ~(OBJ_ALIGNMENT - 1)
            stats["alignment_waste_bytes"] += aligned - cursor
            cursor = aligned
            if cursor % OBJ_ALIGNMENT:
                fail("hybrid OBJ tile start is not 128-byte aligned")

            reference = [0] * (cell_width * cell_height)
            for y in range(height):
                for x in range(width):
                    reference[(pad_y + y) * cell_width + pad_x + x] = \
                        decode_asset_pixel(
                            data, sprite, asset, tile, asset_index,
                            source_x + x, source_y + y,
                        )

            indexed = asset_index >= 3
            if indexed:
                encoded = bytearray(len(reference))
                for y in range(cell_height):
                    for x in range(cell_width):
                        encoded[tile_offset(cell_width, x, y)] = palette_index[
                            reference[y * cell_width + x]
                        ]
                reconstructed = [
                    palette[encoded[tile_offset(cell_width, x, y)]]
                    for y in range(cell_height)
                    for x in range(cell_width)
                ]
                byte_count = len(encoded)
                stats["indexed_bytes"] += byte_count
            else:
                encoded = b"".join(
                    struct.pack("<H", pixel) for pixel in reference
                )
                reconstructed = list(struct.unpack(
                    f"<{len(reference)}H", encoded
                ))
                byte_count = len(encoded)
                stats["bitmap_bytes"] += byte_count
            if reconstructed != reference:
                fail(f"RGB15 parity failed for asset {asset['offset']:#x}")
            cursor += byte_count
            stats["tiles"] += 1
            stats["parity_pixels"] += len(reference)

    stats["obj_span_bytes"] = cursor
    if stats != EXPECTED_STATS:
        fail(f"hybrid OBJ byte contract changed: {stats}")
    if cursor > OBJ_BANK_E_BYTES:
        fail(f"hybrid OBJ span {cursor} exceeds 64 KiB bank E")
    check_runtime_contract(root, source)
    if render_kernel_bakeoff:
        render_go_kernel_bakeoff(root, data, sprites, kernel_threshold)
    if export_sources:
        export_source_assets(
            root, data, sprites, assets, traffic_samples, traffic_palette
        )

    print("IFCommon hybrid OAM: PASS")
    print(f"  O2R SHA-256: {EXPECTED_O2R_SHA256}")
    print(
        f"  assets/tiles/parity pixels: {stats['assets']}/"
        f"{stats['tiles']}/{stats['parity_pixels']}"
    )
    print(
        f"  bitmap/indexed/alignment bytes: {stats['bitmap_bytes']}/"
        f"{stats['indexed_bytes']}/{stats['alignment_waste_bytes']}"
    )
    print(
        f"  OBJ span/headroom: {cursor}/{OBJ_BANK_E_BYTES - cursor} bytes"
    )
    print(
        f"  standard OBJ palette: {len(palette)} used entries "
        f"({len(palette) - 1} visible), {PALETTE_ENTRIES * 2} upload bytes"
    )
    print(
        f"  source alpha: {sorted(set(source_alpha))}"
    )
    print(f"  GO origins/bitmap footprints: {go_origins}/{EXPECTED_GO_ATLAS_RECTS}")
    print(f"  GO continuous stroke runs: {tuple(go_stroke_runs)}")
    print(
        f"  threshold-visible pixels GO/all: {sum(visible_pixels[:3])}/"
        f"{sum(visible_pixels)}"
    )
    print(f"  GO direct RGB555 SHA-256: {go_digest}")
    print(
        f"  traffic palette: {traffic_distinct} source colors -> 31, "
        f"max RGB555 squared error {traffic_max_error}, visible "
        f"{traffic_nonzero}"
    )
    print("  flare: two prepare-once A5I3 source-alpha atlases")
    print("  traffic: prepare-once opaque A3I5 cutout with RGB shading")
    print("  GO: prepare-once direct RGB555+A1 bitmap OAM")
    print("  overlay texture residency: 73728 bytes, three palettes")
    print("  post-prepare conversion/upload: 0/0")


if __name__ == "__main__":
    try:
        arguments = sys.argv[1:]
        allowed = {"--export-source", "--kernel-bakeoff", "--kernel-threshold"}
        unknown = [
            argument for argument in arguments
            if argument.startswith("--") and argument not in allowed
        ]
        if unknown:
            fail(f"unknown fixture option: {unknown[0]}")
        threshold = GO_KERNEL_THRESHOLD_DEFAULT
        if "--kernel-threshold" in arguments:
            threshold_index = arguments.index("--kernel-threshold")
            if threshold_index + 1 >= len(arguments):
                fail("--kernel-threshold requires an integer")
            threshold = int(arguments[threshold_index + 1])
        if not 0 <= threshold <= 255:
            fail(f"GO kernel threshold is outside 0..255: {threshold}")
        verify(
            Path(__file__).resolve().parents[1],
            "--kernel-bakeoff" in arguments,
            threshold,
            "--export-source" in arguments,
        )
    except (AssertionError, OSError, ValueError, struct.error) as error:
        print(f"IFCommon hybrid OAM: FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)

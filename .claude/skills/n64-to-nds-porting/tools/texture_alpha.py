"""Normalized texel/alpha teaching helpers; no ROM/TMEM, combiner or rasterizer.

Inputs are tightly packed logical texels with a resolved CI4 palette bank.
Encoding expects the intended RGBA after the caller resolves source material
semantics. RGB is explicitly reduced by taking its top five bits. Alpha is
lossless unless allow_alpha_quantization=True is explicitly requested.
"""
from __future__ import annotations
from dataclasses import dataclass
from typing import Iterable

RGBA = tuple[int, int, int, int]
MAX_PIXELS = 1024 * 1024
FORMATS = {"GL_RGB4": 2, "GL_RGB16": 4, "GL_RGB256": 8,
           "GL_RGB32_A3": 8, "GL_RGB8_A5": 8, "GL_RGBA": 16, "GL_RGB": 16}


def _integer(value: int, lo: int, hi: int, name: str) -> int:
    if type(value) is not int or not lo <= value <= hi:
        raise ValueError(f"{name} must be an integer in [{lo}, {hi}]")
    return value


def _expand(value: int, bits: int) -> int:
    if bits == 1: return value * 255
    if bits == 3: return (value << 5) | (value << 2) | (value >> 1)
    if bits == 4: return value * 17
    if bits == 5: return (value << 3) | (value >> 2)
    if bits == 8: return value
    raise ValueError("unsupported channel width")


# Quantize to the actual native alpha levels, not ideal uniform 3-bit levels.
# Deterministic nearest representable alpha; equal-distance ties choose lower.
_ALPHA_LEVELS = {3: tuple(_expand((c << 2) | (c >> 1), 5) for c in range(8)),
                 5: tuple(_expand(c, 5) for c in range(32))}
_ALPHA_CODES = {bits: tuple(min(range(len(levels)), key=lambda c: abs(levels[c]-a))
                            for a in range(256)) for bits, levels in _ALPHA_LEVELS.items()}


def _rgba5551(word: int) -> RGBA:
    return (_expand(word >> 11, 5), _expand((word >> 6) & 31, 5),
            _expand((word >> 1) & 31, 5), 255 if word & 1 else 0)


def decode_n64(data: bytes, fmt: str, count: int, *,
               tlut: bytes | None = None, tlut_format: str | None = None) -> tuple[RGBA, ...]:
    """Decode normalized N64 texels, high nibble first for 4-bit formats.

    CI4 requires the already-selected 16-entry bank; CI8 requires 256 entries.
    TLUT entries are big-endian RGBA16 or IA16. No source state is guessed.
    Extra row padding/TMEM bank layout must be normalized by the caller first.
    """
    _integer(count, 1, MAX_PIXELS, "count")
    widths = {"RGBA16": 16, "RGBA32": 32, "CI4": 4, "CI8": 8,
              "IA4": 4, "IA8": 8, "IA16": 16, "I4": 4, "I8": 8}
    if fmt not in widths: raise ValueError("unsupported normalized N64 format")
    bpp = widths[fmt]
    if type(data) is not bytes or len(data) != (count * bpp + 7) // 8:
        raise ValueError("data must contain exactly the requested packed texels")
    palette: list[RGBA] = []
    if fmt in ("CI4", "CI8"):
        entries = 16 if fmt == "CI4" else 256
        if type(tlut) is not bytes or len(tlut) != entries * 2:
            raise ValueError("CI requires a complete, explicitly selected TLUT")
        if tlut_format not in ("RGBA16", "IA16"):
            raise ValueError("CI requires explicit RGBA16 or IA16 TLUT mode")
        for pos in range(0, len(tlut), 2):
            word = int.from_bytes(tlut[pos:pos+2], "big")
            palette.append(_rgba5551(word) if tlut_format == "RGBA16" else
                           (word >> 8, word >> 8, word >> 8, word & 255))
    elif tlut is not None or tlut_format is not None:
        raise ValueError("TLUT is only accepted for CI formats")
    out: list[RGBA] = []
    for i in range(count):
        if bpp == 4: value = (data[i // 2] >> (4 if i % 2 == 0 else 0)) & 15
        elif bpp == 8: value = data[i]
        else: value = int.from_bytes(data[i * (bpp // 8):(i+1) * (bpp // 8)], "big")
        if fmt.startswith("CI"): rgba = palette[value]
        elif fmt == "RGBA16": rgba = _rgba5551(value)
        elif fmt == "RGBA32": rgba = tuple(data[i*4:i*4+4])
        elif fmt.startswith("I") and not fmt.startswith("IA"):
            intensity = _expand(value, bpp)
            rgba = (intensity, intensity, intensity, intensity)
        else:
            ibits, abits = {"IA4": (3, 1), "IA8": (4, 4), "IA16": (8, 8)}[fmt]
            intensity = _expand(value >> abits, ibits)
            rgba = (intensity, intensity, intensity, _expand(value & ((1 << abits)-1), abits))
        out.append(rgba)
    return tuple(out)


@dataclass(frozen=True)
class NativeTexture:
    format: str
    count: int
    texels: bytes
    palette: tuple[int, ...]  # RGB15 only; unused slots zero-padded.
    color0_transparent: bool

    def __post_init__(self) -> None:
        _integer(self.count, 1, MAX_PIXELS, "count")
        if self.format not in FORMATS: raise ValueError("unsupported native format")
        if type(self.color0_transparent) is not bool: raise ValueError("flag must be bool")
        if type(self.texels) is not bytes or len(self.texels) != (self.count * FORMATS[self.format]+7)//8:
            raise ValueError("native texel byte count mismatch")
        capacity = {"GL_RGB4": 4, "GL_RGB16": 16, "GL_RGB256": 256,
                    "GL_RGB32_A3": 32, "GL_RGB8_A5": 8}.get(self.format, 0)
        if type(self.palette) is not tuple or len(self.palette) != capacity:
            raise ValueError("native palette capacity mismatch")
        for word in self.palette: _integer(word, 0, 0x7fff, "RGB15 palette entry")
        if self.color0_transparent and self.format not in ("GL_RGB4", "GL_RGB16", "GL_RGB256"):
            raise ValueError("color-zero flag is only modeled for ordinary indexed formats")


def encode_ds(pixels: Iterable[RGBA], fmt: str, *,
              allow_alpha_quantization: bool = False) -> NativeTexture:
    """Encode the documented subset; fail rather than silently lose alpha/colors.

    This is a teaching converter, not a palette optimizer. It deduplicates exact
    quantized RGB15 colors, pads to native palette capacity and rejects overflow.
    Animated remaps/material equations and spatial dimensions are caller-owned.
    """
    if fmt not in FORMATS: raise ValueError("unsupported native format")
    if type(allow_alpha_quantization) is not bool: raise ValueError("quantization flag must be bool")
    rows: list[RGBA] = []
    for row in pixels:
        if len(rows) == MAX_PIXELS: raise ValueError("too many texels")
        if not isinstance(row, (tuple, list)) or len(row) != 4:
            raise ValueError("each texel must be RGBA8")
        for component in row: _integer(component, 0, 255, "RGBA8 component")
        rows.append(tuple(row))
    _integer(len(rows), 1, MAX_PIXELS, "count")
    binary = all(p[3] in (0, 255) for p in rows)
    ordinary = fmt in ("GL_RGB4", "GL_RGB16", "GL_RGB256")
    if (ordinary or fmt in ("GL_RGBA", "GL_RGB")) and not binary:
        raise ValueError("binary format cannot preserve graded alpha; no implicit threshold")
    if fmt == "GL_RGB" and any(p[3] != 255 for p in rows):
        raise ValueError("GL_RGB upload forces alpha on; use GL_RGBA for cutouts")
    reserve = ordinary and any(p[3] == 0 for p in rows)
    capacity = {"GL_RGB4": 4, "GL_RGB16": 16, "GL_RGB256": 256,
                "GL_RGB32_A3": 32, "GL_RGB8_A5": 8}.get(fmt, 0)
    palette: list[int] = [0] if reserve else []
    mapping: dict[int, int] = {}  # Opaque black must not alias reserved transparent zero.
    values: list[int] = []
    for r, g, b, alpha in rows:
        color = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)
        if not capacity:
            values.append(color | (0x8000 if alpha else 0))
            continue
        if reserve and alpha == 0:
            values.append(0)
            continue
        if color not in mapping:
            if len(palette) == capacity: raise ValueError("opaque/color palette capacity exceeded")
            mapping[color] = len(palette)
            palette.append(color)
        index = mapping[color]
        if ordinary: values.append(index)
        else:
            abits, ibits = (3, 5) if fmt == "GL_RGB32_A3" else (5, 3)
            code = _ALPHA_CODES[abits][alpha]
            alpha5 = ((code << 2) | (code >> 1)) if abits == 3 else code
            if not allow_alpha_quantization and _expand(alpha5, 5) != alpha:
                raise ValueError("alpha needs explicit quantization permission")
            values.append(index | (code << ibits))
    bpp = FORMATS[fmt]
    payload = bytearray((len(values)*bpp+7)//8)
    for i, value in enumerate(values):
        if bpp == 16: payload[i*2:i*2+2] = value.to_bytes(2, "little")
        else: payload[(i*bpp)//8] |= value << ((i*bpp)%8)
    palette.extend([0] * (capacity-len(palette)))
    return NativeTexture(fmt, len(rows), bytes(payload), tuple(palette), reserve)


def decode_ds(texture: NativeTexture) -> tuple[RGBA, ...]:
    """Independent unpacking for the supported formats, not a GX rasterizer."""
    out: list[RGBA] = []
    bpp = FORMATS[texture.format]
    for i in range(texture.count):
        if bpp == 16:
            value = int.from_bytes(texture.texels[i*2:i*2+2], "little")
            color, alpha = value & 0x7fff, 255 if value & 0x8000 else 0
            # GL_RGB represents the upload contract, even for externally constructed input.
            if texture.format == "GL_RGB": alpha = 255
        else:
            value = (texture.texels[(i*bpp)//8] >> ((i*bpp)%8)) & ((1 << bpp)-1)
            if texture.format == "GL_RGB32_A3":
                code = value >> 5
                index, alpha = value & 31, _expand((code << 2) | (code >> 1), 5)
            elif texture.format == "GL_RGB8_A5": index, alpha = value & 7, _expand(value >> 3, 5)
            else: index, alpha = value, 0 if value == 0 and texture.color0_transparent else 255
            color = texture.palette[index]
        out.append((_expand(color & 31, 5), _expand((color >> 5) & 31, 5),
                    _expand((color >> 10) & 31, 5), alpha))
    return tuple(out)


def validate_texture_alpha_draw(fmt: str, alpha_class: str, *, color0_transparent: bool,
                                polygon_mode: str, polygon_alpha: int,
                                blend_enabled: bool, texture_enabled: bool = True,
                                alpha_test_threshold: int = 0) -> None:
    """Fail-closed contract for a simple texture-alpha modulation recipe only.

    Not a general material validator: it requires a zero-threshold global policy
    and does not validate palette allocation, depth, IDs, order, 2D compositing,
    N64 source equations, actual register writes or hardware execution.
    """
    if fmt not in FORMATS or alpha_class not in ("opaque", "cutout", "graded"):
        raise ValueError("unsupported format/alpha class")
    for flag in (color0_transparent, blend_enabled, texture_enabled):
        if type(flag) is not bool: raise ValueError("state flags must be bool")
    _integer(polygon_alpha, 0, 31, "polygon alpha")
    _integer(alpha_test_threshold, 0, 31, "threshold")
    if polygon_alpha == 0: raise ValueError("POLY_ALPHA(0) is wireframe, not invisible")
    if not texture_enabled: raise ValueError("global texturing is disabled")
    if polygon_mode != "modulation": raise ValueError("this texture-alpha recipe requires modulation, not decal")
    if alpha_test_threshold != 0: raise ValueError("nonzero global threshold needs a separately validated mask policy")
    if fmt == "GL_RGB" and alpha_class != "opaque": raise ValueError("GL_RGB forces alpha on")
    if fmt in ("GL_RGB4", "GL_RGB16", "GL_RGB256"):
        if alpha_class == "cutout" and not color0_transparent: raise ValueError("missing color-zero transparency flag")
        if alpha_class == "graded": raise ValueError("indexed texels do not encode graded alpha")
    elif color0_transparent: raise ValueError("wrong alpha mechanism for this format")
    if alpha_class == "graded" and fmt not in ("GL_RGB32_A3", "GL_RGB8_A5"):
        raise ValueError("format cannot preserve graded texel alpha")
    if (alpha_class == "graded" or polygon_alpha < 31) and not blend_enabled:
        raise ValueError("graded compositing requires blending in this recipe")

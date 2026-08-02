"""Render the particle quad atlas to PNG, at the shipping caps and at the all-admit caps.

The quad sheet's admission argument had been carried in prose for a long time
-- "13 textures are excluded and no packing wins them" -- and prose cannot show
that the excluded 13 are roughly half the effect vocabulary, or that the
all-admit sheet is the SAME 8,192 bytes and therefore costs no VRAM at all. Two
pictures settle in seconds what a byte table argues about indefinitely, which is
the same reason the audio rows ship WAVs (see scripts/sfx/export-fgm-cue-wav.py).

Writes nothing into the shared generated paths: it drives build_pack in memory
and only emits PNGs to the directory given as argv[2]. Safe to run at any time,
including against a dirty tree, and it never needs a ROM build.

A5I3 is 5 bits alpha over a 3-bit index into an 8-entry BGR555 palette, and the
output composites over mid grey so both the near-white dust and the dark leaf
silhouettes stay readable in one image.

    python scripts/2d_vfx/render-particle-atlas-png.py . artifacts/visibility/atlas-<date>
"""
import importlib.util, sys, struct, zlib
from pathlib import Path

ROOT = Path(sys.argv[1]).resolve()
OUT = Path(sys.argv[2]).resolve()
OUT.mkdir(parents=True, exist_ok=True)

spec = importlib.util.spec_from_file_location(
    "gen", ROOT / "scripts/generate_nds_particle_banks.py")
gen = importlib.util.module_from_spec(spec)
sys.modules["gen"] = gen
spec.loader.exec_module(gen)


def write_png(path, width, height, rgba, scale=6):
    w, h = width * scale, height * scale
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        sy = y // scale
        for x in range(w):
            raw += bytes(rgba[sy * width + (x // scale)])
    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
           + chunk(b"IEND", b""))
    path.write_bytes(png)


def atlas_rgba(pack):
    q = pack["quads"]
    width, height = q["width"], q["height"]
    payload = q["payload"]
    texels = payload[:q["atlas_bytes"]]
    palette = []
    for i in range(q["palette_entries"]):
        base = q["palette_offset"] + i * 2
        palette.append(payload[base] | (payload[base + 1] << 8))
    out = []
    for byte in texels:
        # A5I3: high 5 bits alpha, low 3 bits palette index.
        alpha = (byte >> 3) & 0x1F
        entry = palette[byte & 0x07] if (byte & 0x07) < len(palette) else 0
        r = (entry & 0x1F) << 3
        g = ((entry >> 5) & 0x1F) << 3
        b = ((entry >> 10) & 0x1F) << 3
        # Composite over a mid grey so both dark and light particles read.
        a = alpha / 31.0
        out.append((int(r * a + 60 * (1 - a)),
                    int(g * a + 60 * (1 - a)),
                    int(b * a + 60 * (1 - a)), 255))
    while len(out) < width * height:
        out.append((30, 30, 30, 255))
    return width, height, out


def build(caps=None):
    if caps is None:
        gen.QUAD_CELL_MAX, gen.QUAD_LONG_ANIMATION_CELL_MAX = 16, 8
        if hasattr(gen, "_orig_dims"):
            gen.quad_cell_dims = gen._orig_dims
    else:
        if not hasattr(gen, "_orig_dims"):
            gen._orig_dims = gen.quad_cell_dims
        orig = gen._orig_dims
        def patched(width, height, cell_max=16, _caps=caps, _o=orig):
            return _o(width, height, _caps.get((width, height), cell_max))
        gen.quad_cell_dims = patched
        gen.QUAD_CELL_MAX, gen.QUAD_LONG_ANIMATION_CELL_MAX = 8, 4
    return gen.build_pack(ROOT)


base = build()
q = base["quads"]
print(f"baseline   admitted={len(q['admitted']):>2} excluded={len(q['excluded']):>2} "
      f"frames={sum(r['frames'] for r in q['admitted'])}")
w, h, rgba = atlas_rgba(base)
write_png(OUT / "atlas-baseline-23of36.png", w, h, rgba)

allin = build(caps={})
q2 = allin["quads"]
print(f"all-admit  admitted={len(q2['admitted']):>2} excluded={len(q2['excluded']):>2} "
      f"frames={sum(r['frames'] for r in q2['admitted'])}")
w, h, rgba = atlas_rgba(allin)
write_png(OUT / "atlas-all-36-admitted.png", w, h, rgba)
print("wrote both PNGs to", OUT)

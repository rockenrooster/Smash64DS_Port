#!/usr/bin/env python3
"""
generate_arrow_svg.py — Generate a CSS scroll-arrow PNG with a bright-orange
                        right-pointing chevron ">" and bake it to a C header.

Usage (standalone):
    python3 tools/generate_arrow_svg.py <output.png> [W H]

    W, H default to 6 and 24 (px).  With W=6 H=24:
      apex angle = 2 * atan(H / (2*W)) * (180/pi)
                 = 2 * atan(24/12) * 57.296
                 = 2 * 63.43 deg = 126.87 deg  (wide opening chevron)

    To get ~150 deg: H / (2*W) = tan(75 deg) = 3.732 -> H = 7.46 * W.
    At integer pixels with W=6: H = 44 (angle = 2*atan(44/12) = 2*74.7 = 149.4 deg).
    The default below uses W=6, H=44 for ~150 deg apex.

CMake integration: this script is invoked via add_custom_command with
    OUTPUT <generated>/arrow_data.h
    COMMAND python3 tools/generate_arrow_svg.py <tmp_png> <output.h> [W H]

The script renders the arrow with PIL supersampling (8x) so edges are
anti-aliased even at small sizes — no external cairosvg dependency needed.

Geometry:
    The ">" chevron polygon vertices (for right-pointing arrow):
        apex    = (W, H/2)            -- rightmost tip
        top     = (0, 0)              -- upper-left corner
        bottom  = (0, H)             -- lower-left corner
    Three vertices produce a filled triangle. The apex angle is
    2*atan((H/2)/W).  The visual "chevron" look vs solid triangle is the
    same at small sizes; a hollow chevron needs antialiasing that PIL can
    provide only via supersampling.
"""

import sys
import math
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    sys.exit("ERROR: Pillow is required.  Install with: pip install Pillow")


# ---------------------------------------------------------------------------
# Arrow geometry
# ---------------------------------------------------------------------------

# Default: W=6, H=44 gives apex angle ~149 degrees
DEFAULT_W = 6
DEFAULT_H = 44
ARROW_COLOR = (0xFF, 0x80, 0x00, 0xFF)  # bright orange, fully opaque
SUPERSAMPLE = 8                          # render at 8x, then downsample


def compute_apex_angle_deg(W: int, H: int) -> float:
    """Return the apex angle in degrees for a triangle apex at (W, H/2)."""
    # Half-angle = atan((H/2) / W)
    return 2.0 * math.degrees(math.atan((H / 2.0) / W))


def render_arrow_png(out_path: str, W: int = DEFAULT_W, H: int = DEFAULT_H,
                     mirror: bool = False) -> None:
    """Render an orange ">" (or "<" if mirror) chevron to a PNG file.

    The chevron's *visual* geometry is W x H — those are the arguments the apex
    angle math uses. The actual PNG canvas is padded horizontally so its width
    is a multiple of 4 px, which keeps row stride at 8-byte multiples for
    RGBA16 (TMEM line size). Without this padding, odd-width sprites like the
    original W=6 produce a 12-byte/row stride; Fast3D treats each row as
    16-byte aligned and the per-row source offset slips → diagonal shear.

    Uses 8x supersampling for anti-aliased edges. Set mirror=True to flip
    horizontally for the left-pointing variant.
    """
    angle = compute_apex_angle_deg(W, H)

    # Pad canvas width to next multiple of 4 (RGBA16 row stride alignment).
    Wcanvas = ((W + 3) // 4) * 4
    pad_left = (Wcanvas - W) // 2  # center the chevron horizontally in the canvas

    direction = "left" if mirror else "right"
    print(f"generate_arrow_svg: poly={W}x{H} canvas={Wcanvas}x{H} "
          f"pad_left={pad_left} apex={angle:.1f} deg dir={direction} -> {out_path}")

    # Render at 8x resolution, then downsample with LANCZOS for AA
    sw, sh = Wcanvas * SUPERSAMPLE, H * SUPERSAMPLE
    img_hi = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img_hi)

    # Chevron polygon in super-res coords, centered horizontally in the padded
    # canvas. base_x_lo = left x of the W-wide polygon footprint.
    base_x_lo_super = pad_left * SUPERSAMPLE
    base_x_hi_super = (pad_left + W) * SUPERSAMPLE
    poly = [
        (base_x_lo_super, 0),
        (base_x_hi_super, sh // 2),
        (base_x_lo_super, sh),
    ]
    draw.polygon(poly, fill=ARROW_COLOR)

    img_lo = img_hi.resize((Wcanvas, H), Image.LANCZOS)
    if mirror:
        img_lo = img_lo.transpose(Image.FLIP_LEFT_RIGHT)

    out = Path(out_path)
    out.parent.mkdir(parents=True, exist_ok=True)
    img_lo.save(str(out), "PNG")
    print(f"  Saved arrow PNG: {out} ({Wcanvas}x{H}, apex={angle:.1f} deg, {direction})")


# ---------------------------------------------------------------------------
# Entry points
# ---------------------------------------------------------------------------

def main_png_only(argv: list) -> int:
    """Mode 1: just generate the PNG.  Used by the bake step via CMake.

    Usage:
        generate_arrow_svg.py <output.png> [W [H [--mirror]]]
    """
    if len(argv) < 2:
        print(__doc__)
        return 1

    # Strip out --mirror flag from anywhere in args
    mirror = False
    args = []
    for a in argv[1:]:
        if a == "--mirror":
            mirror = True
        else:
            args.append(a)

    out_png = args[0]
    W = int(args[1]) if len(args) > 1 else DEFAULT_W
    H = int(args[2]) if len(args) > 2 else DEFAULT_H
    render_arrow_png(out_png, W, H, mirror=mirror)
    return 0


if __name__ == "__main__":
    sys.exit(main_png_only(sys.argv))

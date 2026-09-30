"""Pixel diff of same-geometry window captures: count and bounding boxes of
differing pixels, split at a y boundary (top screen vs the rest)."""
import sys
from PIL import Image, ImageChops

a_path, b_path = sys.argv[1], sys.argv[2]
split_y = int(sys.argv[3]) if len(sys.argv) > 3 else 355
a = Image.open(a_path).convert("RGB")
b = Image.open(b_path).convert("RGB")
if a.size != b.size:
    print("SIZE", a.size, b.size)
    sys.exit(1)
w, h = a.size
diff = ImageChops.difference(a, b)
px = diff.load()
top = []
rest = []
for y in range(h):
    for x in range(w):
        r, g, bl = px[x, y]
        if r or g or bl:
            (top if y < split_y else rest).append((x, y))


def box(points):
    if not points:
        return None
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    return (min(xs), min(ys), max(xs), max(ys))


print(f"{a_path.split(chr(92))[-1]} vs {b_path.split(chr(92))[-1]}: "
      f"size={w}x{h} top(y<{split_y}) diff={len(top)} box={box(top)} "
      f"rest diff={len(rest)} box={box(rest)}")

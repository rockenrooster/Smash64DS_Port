#!/usr/bin/env python3
"""P8 V4: a census lab ROM's texture VRAM map as one static HTML page.

Input: a vram_map.ps1 probe log ("X ROW name addr bytes bucket format size
pal_bytes" per live texture name, "X SITE index pc" per creating site) and the
ROM's ELF (site PCs to function names). Output: banks A-D as bars (each
texture a segment coloured by owner, free runs grey), and tables by owner,
by format, per bank (free bytes, largest free run) and the largest textures.
No JavaScript: the page renders from a double-click (memory
owner-wants-static-html-plus-ps1)."""

from __future__ import annotations

import argparse
import html
import re
import subprocess
from collections import defaultdict
from pathlib import Path

# libnds's texture VRAM: the LCD aliases of banks A-D, 128 KB each.
BANKS = (("A", 0x06800000), ("B", 0x06820000), ("C", 0x06840000), ("D", 0x06860000))
BANK_BYTES = 0x20000
FORMATS = {0: "none", 1: "A3I5", 2: "PAL4", 3: "PAL16", 4: "PAL256", 5: "4x4", 6: "A5I3", 7: "RGBA"}
# nds_renderer_texture_scene.c's census buckets, then one per creating site.
BUCKETS = ["cache: stage", "cache: static", "cache: stage warm", "cache: slot 0", "cache: slot 1",
           "cache: slot 2", "cache: slot 3", "cache: other", "entry: startup", "entry: kept",
           "fighter tint", "halo + shield"]
SITE_BASE = len(BUCKETS)
PALETTE = ["#4e79a7", "#f28e2b", "#59a14f", "#e15759", "#76b7b2", "#edc948", "#b07aa1",
           "#ff9da7", "#9c755f", "#86bcb6", "#d37295", "#8cd17d", "#499894", "#f1ce63",
           "#a0cbe8", "#ffbe7d", "#79706e", "#d4a6c8", "#b6992d", "#fabfd2"]


def site_names(elf: Path, pcs: dict[int, int]) -> dict[int, str]:
    names = {}
    if not pcs or not elf.is_file():
        return names
    tool = "arm-none-eabi-addr2line"
    for base in (r"C:\devkitPro\devkitARM\bin", "/opt/devkitpro/devkitARM/bin"):
        candidate = Path(base) / (tool + ".exe")
        if candidate.is_file():
            tool = str(candidate)
            break
    try:
        out = subprocess.run([tool, "-f", "-C", "-e", str(elf)] + [hex(pc) for pc in pcs.values()],
                             capture_output=True, text=True, timeout=60).stdout.splitlines()
    except (OSError, subprocess.TimeoutExpired):
        return names
    for (index, _), function in zip(pcs.items(), out[0::2]):
        names[index] = function.strip()
    return names


# ndsVramCensusContent's flags (nds_renderer_texture_scene.c NDS_VC_F_*).
F_COLOR0_TRANSPARENT, F_TRANSPARENT_SHOWN, F_4X4, F_HARD_ALPHA, F_MANY = 1, 2, 4, 8, 16
BITS = {1: 8, 2: 2, 3: 4, 4: 8, 5: 2, 6: 8, 7: 16}


def dims(r):
    return 8 << (r["size"] & 7), 8 << ((r["size"] >> 3) & 7)


def smallest_exact(r):
    """(format name, bytes) of the smallest format that shows the same
    texels: paletted by the colours shown (colour 0 kept for transparency),
    A3I5/A5I3 and direct colour paletted only when their alpha is none or
    full; otherwise the texture's own."""
    w, h = dims(r)
    fmt = r["format"]
    hard = (r["flags"] & F_HARD_ALPHA) != 0
    slots = r["colors"] + (1 if r["flags"] & F_TRANSPARENT_SHOWN else 0)
    if fmt in (2, 3, 4) or (fmt in (1, 6, 7) and hard and not r["flags"] & F_MANY):
        for name, bits, cap in (("PAL4", 2, 4), ("PAL16", 4, 16), ("PAL256", 8, 256)):
            if slots <= cap:
                return name, w * h * bits // 8
    return FORMATS.get(fmt, str(fmt)), r["bytes"]


def content_tables(rows, owner, table):
    """V7 (smallest exact format), V8 (4x4) and V9 (duplicate texels) from the
    content census; empty for a log without it."""
    rows = [r for r in rows if r.get("hash") is not None]
    if not rows:
        return ""
    smaller, saved = [], 0
    for r in rows:
        name, size = smallest_exact(r)
        if size < r["bytes"]:
            smaller.append((r, name, size))
            saved += r["bytes"] - size
    four, four_saved = [], 0
    for r in rows:
        if r["flags"] & F_4X4 and r["format"] != 5:
            w, h = dims(r)
            size = w * h * 3 // 8
            if size < r["bytes"]:
                four.append((r, size))
                four_saved += r["bytes"] - size
    groups = defaultdict(list)
    for r in rows:
        groups[(r["hash"], r["format"], r["size"])].append(r)
    dups = [g for g in groups.values() if len(g) > 1]
    dup_saved = sum(g[0]["bytes"] * (len(g) - 1) for g in dups)
    out = [f"<h2>Content (P8 V4: V7 formats, V8 4x4, V9 duplicates)</h2>"
           f"<p>{len(rows)} textures read. Smallest exact format: {len(smaller)} textures, "
           f"{saved:,} B. Every 4x4 block exact in the compressed format: {len(four)} textures, "
           f"{four_saved:,} B (texel and index data; slot rules aside). Duplicate texels: "
           f"{len(dups)} groups, {dup_saved:,} B.</p>"]
    if smaller:
        smaller.sort(key=lambda t: t[0]["bytes"] - t[2], reverse=True)
        out.append(table(["name", "owner", "format", "size", "colours", "alpha levels", "smallest", "bytes", "saved"],
                         [(r["name"], html.escape(owner(r["bucket"])), FORMATS.get(r["format"]),
                           "%dx%d" % dims(r), r["colors"], r["alphas"], n, f'{r["bytes"]:,}',
                           f'{r["bytes"] - s:,}') for r, n, s in smaller[:40]]))
    if dups:
        dups.sort(key=lambda g: g[0]["bytes"] * (len(g) - 1), reverse=True)
        out.append("<h3>Duplicate texels</h3>")
        out.append(table(["names", "owners", "format", "size", "bytes each", "saved"],
                         [(" ".join(str(r["name"]) for r in g),
                           html.escape(", ".join(sorted({owner(r["bucket"]) for r in g}))),
                           FORMATS.get(g[0]["format"]), "%dx%d" % dims(g[0]), f'{g[0]["bytes"]:,}',
                           f'{g[0]["bytes"] * (len(g) - 1):,}') for g in dups[:40]]))
    return "".join(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--log", type=Path, required=True)
    ap.add_argument("--elf", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--title", default="texture VRAM")
    args = ap.parse_args()

    rows, pcs, header, blocks = [], {}, "", []
    for line in args.log.read_text(errors="ignore").splitlines():
        m = re.match(r"X BLOCK ([0-9a-f]+) (\d+) (\d+)", line)
        if m:
            blocks.append((int(m.group(1), 16), int(m.group(2)), int(m.group(3))))
            continue
        m = re.match(r"X ROW (\d+) ([0-9a-f]+) (\d+) (\d+) (\d+) (\d+) (\d+)"
                     r"(?: ([0-9a-f]+) (\d+) (\d+) (\d+))?", line)
        if m:
            name, addr, size_bytes, bucket, fmt, size, pal, hsh, colors, alphas, flags = m.groups()
            rows.append(dict(name=int(name), addr=int(addr, 16), bytes=int(size_bytes),
                             bucket=int(bucket), format=int(fmt), size=int(size), pal=int(pal),
                             hash=int(hsh, 16) if hsh else None, colors=int(colors or 0),
                             alphas=int(alphas or 0), flags=int(flags or 0)))
            continue
        m = re.match(r"X SITE (\d+) ([0-9a-f]+)", line)
        if m:
            pcs[int(m.group(1))] = int(m.group(2), 16)
            continue
        if line.startswith("X MAPHDR"):
            header = line[len("X MAPHDR "):]
    if not rows:
        raise SystemExit(f"{args.log}: no X ROW lines")
    sites = site_names(args.elf, pcs)

    def owner(bucket: int) -> str:
        if bucket < SITE_BASE:
            return BUCKETS[bucket]
        site = bucket - SITE_BASE
        return "site: " + sites.get(site, f"#{site}" if site in pcs else "untagged")

    owners = sorted({owner(r["bucket"]) for r in rows})
    colour = {o: PALETTE[i % len(PALETTE)] for i, o in enumerate(owners)}

    by_owner = defaultdict(lambda: [0, 0, 0])
    by_format = defaultdict(lambda: [0, 0])
    for r in rows:
        o = by_owner[owner(r["bucket"])]
        o[0] += 1
        o[1] += r["bytes"]
        o[2] += r["pal"]
        f = by_format[FORMATS.get(r["format"], str(r["format"]))]
        f[0] += 1
        f[1] += r["bytes"]

    bars, bank_rows, unnamed = [], [], {}
    for bank, base in BANKS:
        inside = sorted((r for r in rows if base <= r["addr"] < base + BANK_BYTES), key=lambda r: r["addr"])
        if not inside:
            continue
        used = sum(r["bytes"] for r in inside)
        free_runs, cursor = [], base
        for r in inside:
            if r["addr"] > cursor:
                free_runs.append(r["addr"] - cursor)
            cursor = max(cursor, r["addr"] + r["bytes"])
        if cursor < base + BANK_BYTES:
            free_runs.append(base + BANK_BYTES - cursor)
        width = 960.0
        scale = width / BANK_BYTES
        rects = [f'<rect x="0" y="0" width="{width:.0f}" height="28" fill="#d9d9d9"/>']
        if blocks:
            # The allocator's own view: what it holds in this bank, named or
            # not (the fighter admission's carve regions have no name).
            free_runs = []
            for lo, size, used_flag in blocks:
                a, z = max(lo, base), min(lo + size, base + BANK_BYTES)
                if a >= z:
                    continue
                if used_flag:
                    rects.append(f'<rect x="{(a - base) * scale:.2f}" y="0" width="{(z - a) * scale:.2f}" '
                                 f'height="28" fill="#5b5b5b"><title>allocated without a name '
                                 f'{z - a:,} B</title></rect>')
                else:
                    free_runs.append(z - a)
            held = sum(min(lo + size, base + BANK_BYTES) - max(lo, base) for lo, size, u in blocks
                       if u and max(lo, base) < min(lo + size, base + BANK_BYTES))
            unnamed[bank] = max(held - used, 0)
        bank_rows.append((bank, len(inside), used, sum(free_runs), max(free_runs, default=0)))
        for r in inside:
            x = (r["addr"] - base) * scale
            w = max(r["bytes"] * scale, 0.5)
            tip = html.escape(f'name {r["name"]} {owner(r["bucket"])} {FORMATS.get(r["format"])} '
                              f'{8 << (r["size"] & 7)}x{8 << ((r["size"] >> 3) & 7)} {r["bytes"]} B '
                              f'@{r["addr"] - base:#07x}')
            rects.append(f'<rect x="{x:.2f}" y="0" width="{w:.2f}" height="28" '
                         f'fill="{colour[owner(r["bucket"])]}" stroke="#fff" stroke-width="0.3">'
                         f'<title>{tip}</title></rect>')
        bars.append(f'<h3>Bank {bank} -- {used:,} B in {len(inside)} names'
                    f'{f", {unnamed[bank]:,} B held without a name" if unnamed.get(bank) else ""}, '
                    f'{sum(free_runs):,} B free, largest run {max(free_runs, default=0):,} B</h3>'
                    f'<svg width="{width:.0f}" height="28" role="img">{"".join(rects)}</svg>')

    def table(headers, body):
        head = "".join(f"<th>{html.escape(h)}</th>" for h in headers)
        lines = "".join("<tr>" + "".join(f"<td>{c}</td>" for c in row) + "</tr>" for row in body)
        return f"<table><tr>{head}</tr>{lines}</table>"

    owner_table = table(["owner", "names", "texel bytes", "palette bytes"],
                        [(f'<span class="sw" style="background:{colour[o]}"></span>{html.escape(o)}',
                          n, f"{b:,}", f"{p:,}") for o, (n, b, p) in
                         sorted(by_owner.items(), key=lambda kv: -kv[1][1])])
    format_table = table(["format", "names", "texel bytes"],
                         [(f, n, f"{b:,}") for f, (n, b) in sorted(by_format.items(), key=lambda kv: -kv[1][1])])
    bank_table = table(["bank", "names", "used", "free", "largest free run"],
                       [(b, n, f"{u:,}", f"{f:,}", f"{l:,}") for b, n, u, f, l in bank_rows])
    top = sorted(rows, key=lambda r: -r["bytes"])[:30]
    top_table = table(["name", "owner", "format", "size", "bytes", "bank offset"],
                      [(r["name"], html.escape(owner(r["bucket"])), FORMATS.get(r["format"]),
                        f'{8 << (r["size"] & 7)}x{8 << ((r["size"] >> 3) & 7)}', f'{r["bytes"]:,}',
                        next((f'{b} {r["addr"] - base:#07x}' for b, base in BANKS
                              if base <= r["addr"] < base + BANK_BYTES), hex(r["addr"])))
                       for r in top])
    content_html = content_tables(rows, owner, table)
    total = sum(r["bytes"] for r in rows)
    page = f"""<!doctype html><html><head><meta charset="utf-8">
<title>VRAM map</title><meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{{font:14px/1.4 system-ui,sans-serif;margin:16px;color:#1b1b1b;background:#fff}}
h1{{font-size:20px}} h2{{font-size:16px;margin-top:24px}} h3{{font-size:13px;margin:12px 0 4px;font-weight:600}}
table{{border-collapse:collapse;margin:6px 0 12px}} td,th{{border:1px solid #ccc;padding:3px 8px;text-align:right}}
td:first-child,th:first-child{{text-align:left}} .sw{{display:inline-block;width:10px;height:10px;margin-right:6px}}
svg{{max-width:100%;height:auto;display:block}}
</style></head><body>
<h1>{html.escape(args.title)}</h1>
<p>{len(rows)} live texture names, {total:,} texel bytes. Census: {html.escape(header)}</p>
<h2>Banks (texture slots; hover a segment for its name)</h2>{"".join(bars)}
<h2>By owner</h2>{owner_table}<h2>By format</h2>{format_table}
<h2>Per bank</h2>{bank_table}<h2>Largest textures</h2>{top_table}
{content_html}
</body></html>"""
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(page, encoding="utf-8")
    print(f"VRAM_MAP {args.out} names={len(rows)} bytes={total}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

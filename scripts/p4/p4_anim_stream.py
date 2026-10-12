#!/usr/bin/env python3
"""The P4 contents' FIGATREE clips as one BPS1 stream pack.

The cast's clips load from animation/ftanim_stream_pack.bin
(scripts/generate_battlepack_anim.py --stream-out): one storage command a
clip, its command words already in native order, its slot table already
offsets -- no FAT path walk, O2R header, byte swap, relocation chain or AObj16
normalization at run time. A content's clips were loose O2R files in
nitro:/reloc/p4 and paid all of that on every animation-cache miss: four
Banjos' over-gate frames spent ~330K cycles in storage reads and ~190K in the
normalizers (2026-10-12 census, build-prof-banjo).

This packs every content's (and Kirby's power) animation the same way, with
the same generator code (read_clip, emit_stream_pack), into
animation/ftanim_p4_stream.bin; nds_reloc_assets.c reads it as the cast's
pack's sibling. An AObjEvent32 clip (bit 15 of the content's Anims row) or a
spline clip stays a loose file and loads as before. The loose files stay in
NitroFS either way: the menus and the event32 clips still name them.

Usage:
  python scripts/p4/p4_anim_stream.py --gen <build>/p4 --contents "falco ..." \
      --out <nitrofs>/animation/ftanim_p4_stream.bin [--json report.json]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

import generate_battlepack_anim as B  # noqa: E402

ANIMS_RE = re.compile(r"const u16 gNdsP4\w*?Anims\[\d*\]\s*=\s*\{([^}]*)\}")


def anim_rows(generated: pathlib.Path) -> list[tuple[int, bool]]:
    """The (id, event32) rows of a generated content's Anims table."""
    m = ANIMS_RE.search(generated.read_text())
    if m is None:
        raise SystemExit(f"{generated}: no Anims table")
    vals = [int(v, 0) for v in re.findall(r"0x[0-9a-fA-F]+|\d+", m.group(1))]
    return [(v & 0x7FFF, bool(v & 0x8000)) for v in vals if (v & 0x7FFF) != 0]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--gen", type=pathlib.Path, required=True,
                    help="the build's p4 generation directory")
    ap.add_argument("--contents", required=True,
                    help="the enabled contents, space separated")
    ap.add_argument("--out", type=pathlib.Path, required=True)
    ap.add_argument("--json", type=pathlib.Path)
    args = ap.parse_args()

    sources: list[tuple[pathlib.Path, pathlib.Path]] = []
    for name in args.contents.split():
        sources.append((args.gen / name / f"nds_p4_{name}.generated.c",
                        args.gen / name / "o2r"))
    kirby = args.gen / "kirby" / "nds_p4_kirby.generated.c"
    if kirby.is_file():
        sources.append((kirby, args.gen / "kirby" / "o2r"))

    probe = B._load("ftanim_reloc_probe")
    directory = []
    seen: dict[int, bytes] = {}
    skipped: list[tuple[str, str]] = []
    for generated, o2r in sources:
        for asset_id, event32 in anim_rows(generated):
            path = o2r / f"{asset_id:04x}"
            if event32:
                skipped.append((path.name, "AObjEvent32"))
                continue
            raw = path.read_bytes()
            if asset_id in seen:
                # Contents share clips (Marth's and Roy's): one file, one row.
                if seen[asset_id] != raw:
                    raise SystemExit(f"P4 clip {asset_id:#06x} differs between contents")
                continue
            seen[asset_id] = raw
            clip = B.read_clip(probe, path)
            if clip is None:
                skipped.append((path.name, "unreadable"))
                continue
            if clip["aobj32"] or clip["spline"]:
                skipped.append((path.name, "spline" if clip["spline"] else "AObjEvent32"))
                continue
            if clip["asset_id"] != asset_id:
                raise SystemExit(f"{path}: file id {clip['asset_id']:#06x} is not its name")
            directory.append(clip)
    directory.sort(key=lambda c: c["asset_id"])
    blob, stats = B.emit_stream_pack(directory)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    if (not args.out.exists()) or (args.out.read_bytes() != blob):
        args.out.write_bytes(blob)
    stats["skipped"] = skipped
    if args.json is not None:
        args.json.write_text(json.dumps(stats, indent=1))
    print(f"P4_ANIM_STREAM_OK clips={stats['stream_clips']} bytes={stats['stream_bytes']} "
          f"ids={stats['stream_first_id']:#06x}..{stats['stream_last_id']:#06x} "
          f"skipped={len(skipped)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

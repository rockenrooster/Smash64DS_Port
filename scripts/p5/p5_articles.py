#!/usr/bin/env python3
"""P5: the native draw roots of the landed Remix stages' hazard items.

Bowser's Stadium's bombs are the source's Race to the Finish bomb
(nITKindTaruBomb) made from Remix's own header file (Hazards.asm add_item_,
File.BOWSER_BOMB_HEADER 0x9B2): its ITAttributes at 0xA8 name the bomb's
DObjDesc tree in the model file 0x9B1. Each entry's list is a root the
runtime admits through the attributes' word, exactly as a P4 content's item
(scripts/p4/p4_articles.py, "itattributes"): the storage is the hazard file
the stage loads (gNdsP5HazardFile<Cname>), and the roots join the P4 article
rows in the build's entry-effect packet
(scripts/3d_vfx/generate_nds_entry_effects.py --p4). A root carries its
stage's ground kind, so the runtime loads its corners and prepares its
textures only on that stage.

    python scripts/p5/p5_articles.py --stages-root builds/<build>/p5/stages \
        --out builds/<build>/p5/stages/articles.json

The roots are offsets into the user's files: build output only. The O2R
files the roots read are copied beside the output (o2r/), where the packet
generator looks for a row's file.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "p4"))
import p4_articles  # noqa: E402
import p5_stages  # noqa: E402

# Per hazard file: where its ITAttributes sit (the source's descriptor offset:
# battleship_item_tarubomb.c llGRBonus3MapTaruBombItemAttributes, 0xA8).
HAZARD_ITEMS = {
    "bowsers_stadium": ({"name": "bomb", "itattributes": 0xA8},),
}


def resolve(row: dict, stage_dir: Path, out_o2r: Path) -> list[dict]:
    if row["name"] not in HAZARD_ITEMS:
        return []
    stage = json.loads((stage_dir / "stage.json").read_text(encoding="utf-8"))
    roots = []
    for fid in stage.get("hazard_roots", ()):
        res = p4_articles.load(stage_dir / "o2r" / f"{fid:04x}", fid)
        for article in HAZARD_ITEMS.get(row["name"], ()):
            at = article["itattributes"]
            tree = res.pointer_at(at)
            if tree is None:
                raise SystemExit(f"{row['name']} {article['name']}: attributes name no tree")
            if res.pointer_at(at + 4) is not None:
                raise SystemExit(f"{row['name']} {article['name']}: a MObjSub table "
                                 "(TEXID frames) is not handled")
            # A translucent item (the flags word's is_display_xlu, bit 31 of
            # +0x10) draws through DObjDLLinks (itDisplayXLUProcDisplay): an
            # entry's list is then its DObjDLLink array, one root per link.
            xlu = (p4_articles.u32(res, at + 0x10) >> 31) & 1
            target = p4_articles.load(stage_dir / "o2r" / f"{tree.asset_id:04x}", tree.asset_id)
            for _index, dl in p4_articles.dobjdesc_entries(target, tree.offset):
                lists = p4_articles.dllink_lists(target, dl) if xlu else [(0, dl)]
                for head, root in lists:
                    roots.append({
                        "article": f"{row['name']}_{article['name']}", "file_id": target.file_id,
                        "storage": f"gNdsP5HazardFile{row['c_symbol']}", "via": at + 1,
                        "offset": root, "entry": False, "texid": None, "palette": None,
                        "head": head, "sidecar_joint": None, "material": False,
                        "via_base": True, "via_bias": tree.offset,
                        "stage_gkind": row["gkind"]})
            for f in (fid, tree.asset_id):
                p5_stages.write_if_changed(out_o2r / f"{f:04x}",
                                           (stage_dir / "o2r" / f"{f:04x}").read_bytes())
    return roots


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--stages-root", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    rows = [r for r in p5_stages.load_registry() if r["source"] == "remix" and r["landed"]]
    out_o2r = args.out.parent / "o2r"
    out_o2r.mkdir(parents=True, exist_ok=True)
    roots = []
    for row in sorted(rows, key=lambda r: r["gkind"]):
        roots += resolve(row, args.stages_root / row["name"], out_o2r)
    gkinds = sorted({r["stage_gkind"] for r in roots})
    p5_stages.write_if_changed(args.out, json.dumps(
        {"content": "p5", "stage_gkinds": gkinds, "roots": roots}, indent=1) + "\n")
    print(f"p5_articles: {len(roots)} hazard roots on {len(gkinds)} stages")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

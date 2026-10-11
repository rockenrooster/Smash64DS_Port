#!/usr/bin/env python3
"""P4 character select: the preview pack of a source kind P4 makes selectable.

Metal Mario is the source MMario kind (scripts/p4/p4_css.py KINDS). His VS
select preview takes the FPC1 pack a content's does (p4_preview_pack.py),
built from his source Main and Model in BattleShip's O2R extraction and the
files his Main names; the native owner image his campaign stage uses draws
it. Main, Model and the attributes offset are his FTData's
(decomp ft/ftdata.c), the file ids BattleShip's reloc symbol table's.

Writes --out (fighters/preview/<kind>.fpc) and stages the O2R files under
--work.
"""
from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import remix_rom as R  # noqa: E402
import p4_native_owner as P  # noqa: E402
import p4_preview_pack as PP  # noqa: E402

BATTLESHIP = HERE.parents[1] / "decomp" / "BattleShip-main"
O2R = BATTLESHIP / "BattleShip_o2r"
YAMLS = BATTLESHIP / "yamls" / "us"
FTDATA = BATTLESHIP / "decomp" / "src" / "ft" / "ftdata.c"
SYMBOLS = BATTLESHIP / "tools" / "reloc_data_symbols.us.txt"
# Source kinds P4 selects as themselves: FTKind -> FTData name.
KINDS = {13: "MMario"}


class SourcePackError(SystemExit):
    pass


def ftdata_row(name: str) -> tuple[int, int, int]:
    """(Main file id, Model file id, o_attributes) of dFT<name>Data."""
    text = FTDATA.read_text(encoding="utf-8", errors="replace")
    m = re.search(r"FTData\s+dFT" + name + r"Data\s*=\s*\{(.*?)\};", text, re.S)
    if m is None:
        raise SourcePackError(f"ftdata.c: no dFT{name}Data")
    words = [w.strip() for w in m.group(1).split(",") if w.strip()]
    main_sym, model_sym = words[0].lstrip("&"), words[3].lstrip("&")
    at = words.index(f"&dFT{name}MotionDescs")
    attributes = int(words[at - 1], 0)
    symbols = {}
    for line in SYMBOLS.read_text(encoding="utf-8").splitlines():
        sm = re.match(r"(\w+)\s*=\s*(0x[0-9a-fA-F]+);", line.strip())
        if sm:
            symbols[sm.group(1)] = int(sm.group(2), 16)
    if main_sym not in symbols or model_sym not in symbols:
        raise SourcePackError(f"dFT{name}Data: {main_sym}/{model_sym} not in the symbol table")
    return symbols[main_sym], symbols[model_sym], attributes


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--kind", type=int, required=True, choices=sorted(KINDS))
    ap.add_argument("--work", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()

    main_id, model_id, attributes = ftdata_row(KINDS[args.kind])
    index = R.battleship_o2r_index(YAMLS)
    o2r = args.work / "o2r"
    o2r.mkdir(parents=True, exist_ok=True)

    def stage(fid: int) -> None:
        if fid not in index:
            raise SourcePackError(f"file {fid:#x}: not in BattleShip's yamls")
        group, symbol = index[fid]
        shutil.copyfile(O2R / group / symbol, o2r / f"{fid:04x}")

    stage(main_id)
    stage(model_id)
    main_file = P.O2RFile(o2r / f"{main_id:04x}")
    for fid in sorted({t[1] for t in main_file.slots.values() if t[0] == "extern"}):
        stage(fid)
    try:
        blob, meta, _ext = PP.build(o2r, main_id, model_id, attributes, args.kind)
    except PP.PackError as error:
        print(f"p4_source_pack: {error}", file=sys.stderr)
        return 1
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(blob)
    print(f"p4_source_pack: kind {args.kind} ({KINDS[args.kind]}) {meta['file_bytes']} B, "
          f"{meta['root_cells']} root cells -> {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

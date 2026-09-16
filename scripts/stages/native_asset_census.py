#!/usr/bin/env python3
"""Shared O2R asset census for the native item/effect generators.

`census()` was byte-identical in four generators — the three non-wave-1 item
generators and the Yamabuki marumine one — differing only in the module-level
`ASSET` each closed over. Four copies of a corpus scan is four places to forget
when the O2R root moves or the OLER header check changes, and a stale copy
reports a smaller corpus rather than failing, so the drift is silent.

The asset id is a parameter here instead of a captured global, which is the only
change needed to make the four bodies one.
"""

from __future__ import annotations

import hashlib


def census(repo, sm, asset_id: int) -> tuple:
    """Every O2R file in the image, for the root and the DObjDesc.

    `sm` is the generate_nds_native_stage module; each caller imports it under
    its own sys.path setup, so it is passed in rather than imported here.
    Returns `(scanned, hits)` where `hits` is a sorted tuple of
    `(file_id, slot, offset)` for every external reference to `asset_id`.
    """
    root = repo / "decomp/BattleShip-main/BattleShip_o2r"
    hits = []
    scanned = 0
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        blob = path.read_bytes()
        if len(blob) < 0x50 or blob[4:8] != b"OLER":
            continue
        scanned += 1
        rel = str(path.relative_to(repo)).replace("\\", "/")
        res = sm.load_o2r(repo, sm.InputSpec(rel, hashlib.sha256(blob).hexdigest()))
        for slot, ref in res.external.items():
            if ref.asset_id == asset_id:
                hits.append((res.file_id, slot, ref.offset))
    return scanned, tuple(sorted(hits))

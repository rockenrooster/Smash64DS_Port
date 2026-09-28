#!/usr/bin/env python3
"""Lane 1 helper: O2R reloc-file index + parser (reuses the stage generator's parser).

Everything here is READ-ONLY over decomp/BattleShip-main/BattleShip_o2r.  The
per-file parser is generate_nds_native_stage.load_o2r (header at 0x40: file_id,
internal chain head, external chain head, extern count; u16 extern-id table;
u32 payload size; big-endian payload whose relocation slots hold
(next_slot_word_index << 16) | (target_byte_offset >> 2)).
"""
from __future__ import annotations

import hashlib
import os
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]  # .../Smash64DS_Port
SCRIPTS = REPO / "scripts"
if str(SCRIPTS) not in sys.path:
    sys.path.insert(0, str(SCRIPTS))
STAGES = SCRIPTS / "stages"
if str(STAGES) not in sys.path:
    sys.path.insert(0, str(STAGES))

import _paths  # noqa: E402,F401  (puts every scripts/ area folder on sys.path)
import generate_nds_native_stage as gen  # noqa: E402

O2R_ROOT = REPO / "decomp" / "BattleShip-main" / "BattleShip_o2r"


@dataclass
class OFile:
    path: Path
    rel: str            # path relative to O2R_ROOT with forward slashes
    file_id: int
    size_on_disk: int
    data_size: int      # payload bytes (what the DS runtime allocates, before 16-byte align)
    extern_ids: list
    internal: dict      # slot byte offset -> PointerRef
    external: dict      # slot byte offset -> PointerRef
    payload: bytes

    @property
    def name(self) -> str:
        return self.path.name

    @property
    def aligned(self) -> int:
        return (self.data_size + 15) & ~15


_INDEX = None


def index() -> dict:
    """file_id -> OFile for every reloc file under BattleShip_o2r/reloc_*.

    File ids are unique per region build (US); duplicates raise.
    """
    global _INDEX
    if _INDEX is not None:
        return _INDEX
    out = {}
    for sub in sorted(os.listdir(O2R_ROOT)):
        d = O2R_ROOT / sub
        if not sub.startswith("reloc_") or not d.is_dir():
            continue
        for fn in sorted(os.listdir(d)):
            p = d / fn
            if not p.is_file():
                continue
            raw = p.read_bytes()
            if len(raw) < 0x50 or raw[4:8] != b"OLER":
                continue
            spec = gen.InputSpec(str(p.relative_to(REPO)).replace("\\", "/"),
                                 hashlib.sha256(raw).hexdigest())
            try:
                res = gen.load_o2r(REPO, spec)
            except Exception as exc:  # noqa: BLE001 - report and skip
                print(f"# skip {p}: {exc}", file=sys.stderr)
                continue
            ext_ids = list(struct.unpack_from(
                f"<{struct.unpack_from('<I', raw, 0x48)[0]}H", raw, 0x4C)) if struct.unpack_from('<I', raw, 0x48)[0] else []
            of = OFile(p, f"{sub}/{fn}", res.file_id, len(raw), len(res.payload),
                       ext_ids, res.internal, res.external, res.payload)
            if of.file_id in out:
                raise RuntimeError(f"duplicate file id {of.file_id}: {of.rel} vs {out[of.file_id].rel}")
            out[of.file_id] = of
    _INDEX = out
    return out


def tree(root_id: int, skip: set | None = None) -> list:
    """DFS extern tree exactly like ndsRelocLoadExternTreeAsset /
    ndsRelocExternTreeAllocSize: the file itself first, then each extern id in
    table order (recursively), a file already seen (or in `skip`) contributing 0.
    Returns the list of OFile in load order.
    """
    idx = index()
    seen = set(skip or ())
    order = []

    def rec(fid: int) -> None:
        if fid in seen:
            return
        seen.add(fid)
        f = idx[fid]
        order.append(f)
        for e in f.extern_ids:
            rec(e)

    rec(root_id)
    return order


def tree_alloc(order: list) -> int:
    """Size the runtime asks syTaskmanMalloc for: NDS_RELOC_ALIGN(size) summed with
    a 16-byte align before each dependency (ndsRelocExternTreeAllocSize)."""
    total = 0
    for i, f in enumerate(order):
        if i == 0:
            total = f.aligned
        else:
            total = (total + 15) & ~15
            total += f.aligned
    return total


if __name__ == "__main__":
    idx = index()
    print(f"{len(idx)} reloc files indexed")
    for fid in sorted(idx):
        f = idx[fid]
        print(f"{fid:5d} 0x{fid:03x}  {f.data_size:8d}  {f.rel}  ext={f.extern_ids}")

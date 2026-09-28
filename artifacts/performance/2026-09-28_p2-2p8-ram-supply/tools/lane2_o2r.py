#!/usr/bin/env python3
"""Lane 2 helper: O2R container parser and relocation-chain walker.

Format (as read by ndsRelocAssetReadHeaderFromFile, src/nds/nds_reloc_assets.c,
and by load_o2r in scripts/stages/generate_nds_native_stage.py):

  0x40  u32 LE file_id
  0x44  u16 LE internal-fixup chain head (word index, 0xffff = none)
  0x46  u16 LE external-fixup chain head (word index, 0xffff = none)
  0x48  u32 LE extern_count
  0x4C  extern_count * u16 LE extern file ids
  next  u32 LE data_size
  next  data_size bytes of big-endian N64 payload

Each chain link is one payload word (big-endian): high 16 bits = next word
index, low 16 bits = target word index (target offset = low16 * 4).  The
chain visits the slot at word index `cursor`.  For the extern chain the i-th
visited slot belongs to extern_ids[i].
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
O2R_ROOT = REPO / "decomp/BattleShip-main/BattleShip_o2r"
NITRO_ROOT = REPO / "builds/build-fp-argmax/nitrofs/reloc"


@dataclass
class O2R:
    path: Path
    file_id: int
    intern_head: int
    extern_head: int
    extern_ids: list
    data_size: int
    payload: bytes
    container_size: int
    internal: dict = field(default_factory=dict)  # slot offset -> target offset
    external: dict = field(default_factory=dict)  # slot offset -> (dep file id, target offset)

    @property
    def name(self) -> str:
        return self.path.name

    def be32(self, off: int) -> int:
        return struct.unpack_from(">I", self.payload, off)[0]

    def be16(self, off: int) -> int:
        return struct.unpack_from(">H", self.payload, off)[0]

    def s16(self, off: int) -> int:
        return struct.unpack_from(">h", self.payload, off)[0]

    def f32(self, off: int) -> float:
        return struct.unpack_from(">f", self.payload, off)[0]


def _walk(payload: bytes, head: int, ids):
    out = {}
    cursor = head
    guard = len(payload) // 4 + 1
    idx = 0
    while cursor != 0xFFFF:
        slot = cursor * 4
        if guard == 0 or slot + 4 > len(payload) or slot in out:
            raise ValueError("malformed relocation chain")
        guard -= 1
        word = struct.unpack_from(">I", payload, slot)[0]
        target = (word & 0xFFFF) * 4
        if ids is None:
            out[slot] = target
        else:
            out[slot] = (ids[idx], target)
            idx += 1
        cursor = word >> 16
    return out


def load(path) -> O2R:
    path = Path(path)
    raw = path.read_bytes()
    if len(raw) < 0x50 or raw[4:8] != b"OLER":
        raise ValueError(f"{path}: not an O2R container")
    file_id, ih, eh, ec = struct.unpack_from("<IHHI", raw, 0x40)
    ids = list(struct.unpack_from(f"<{ec}H", raw, 0x4C)) if ec else []
    ds_off = 0x4C + ec * 2
    (data_size,) = struct.unpack_from("<I", raw, ds_off)
    payload = raw[ds_off + 4: ds_off + 4 + data_size]
    if len(payload) != data_size:
        raise ValueError(f"{path}: short payload")
    o = O2R(path, file_id, ih, eh, ids, data_size, payload, len(raw))
    o.internal = _walk(payload, ih, None)
    o.external = _walk(payload, eh, ids)
    return o


_INDEX = None


def index():
    """file_id -> path for every reloc_* O2R container."""
    global _INDEX
    if _INDEX is None:
        _INDEX = {}
        for p in sorted(O2R_ROOT.glob("reloc_*/*")):
            if not p.is_file():
                continue
            with open(p, "rb") as f:
                head = f.read(0x50)
            if len(head) < 0x50 or head[4:8] != b"OLER":
                continue
            fid = struct.unpack_from("<I", head, 0x40)[0]
            _INDEX.setdefault(fid, p)
    return _INDEX


def by_id(fid: int) -> O2R:
    return load(index()[fid])


def by_name(name: str) -> O2R:
    for p in O2R_ROOT.glob(f"reloc_*/{name}"):
        return load(p)
    raise KeyError(name)


def align16(n: int) -> int:
    return (n + 15) & ~15


if __name__ == "__main__":
    import sys
    for a in sys.argv[1:]:
        o = by_name(a)
        print(a, "fid", o.file_id, "payload", o.data_size, "internal", len(o.internal),
              "external", len(o.external), "externs", sorted(set(o.extern_ids)))

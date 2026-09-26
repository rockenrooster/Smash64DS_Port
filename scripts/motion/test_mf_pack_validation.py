"""Independent corrupt-pack cases against the actual MFP2 source checker."""
from pathlib import Path
import json
import struct
import subprocess
import sys
import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import price_mf_rosters as pricing

ROOT = Path(__file__).resolve().parents[2]
PACK = ROOT / "builds/p2-2p8-mf2/ftanim_mf_pack.bin"
HEADER = struct.Struct("<IHH10I")
ENTRY = struct.Struct("<IIIIIBBHHH")
KIND = struct.Struct("<8sHHI")


def checked_pack():
    assert PACK.is_file(), "Emit builds/p2-2p8-mf2 with mf_emit.py first"
    data = PACK.read_bytes()
    assert data[:4] == b"MFP2"
    return data


def run_checker(path):
    return subprocess.run(
        [sys.executable, str(ROOT / "scripts/motion/check_mf_pack.py"), str(path)],
        cwd=ROOT, text=True, capture_output=True, timeout=90,
    )


def reassign_clip_owner(data):
    """Keep bytes, masks, row order and spans valid; falsify one owner only."""
    header = HEADER.unpack_from(data)
    rows = [list(ENTRY.unpack_from(data, header[5] + i * ENTRY.size))
            for i in range(header[6])]
    streams = {row[0]: data[header[9] + row[1]:
                           header[9] + row[1] + (((row[2] + 7) // 8 + 3) & ~3)]
               for row in rows}
    # Captain's unreferenced always-needed clips rely on owner admission.
    row = next(row for row in rows if row[5] == 7 and row[6] == 0 and row[8] == 0)
    changed_id = row[0]
    row[5] = 0
    rows.sort(key=lambda row: (row[5], row[6], row[7], row[0]))
    out = bytearray(data[:header[9]])
    stream_data = bytearray()
    for index, row in enumerate(rows):
        row[1] = len(stream_data)
        ENTRY.pack_into(out, header[5] + index * ENTRY.size, *row)
        stream_data.extend(streams[row[0]])
    for kind in range(header[2]):
        name, _first, _count, reserved = KIND.unpack_from(data, header[7] + kind * KIND.size)
        first = sum(row[5] < kind for row in rows)
        count = sum(row[5] == kind for row in rows)
        KIND.pack_into(out, header[7] + kind * KIND.size, name, first, count, reserved)
    by_id = sorted(range(len(rows)), key=lambda index: rows[index][0])
    struct.pack_into("<%dH" % len(rows), out, header[8], *by_id)
    assert len(stream_data) == header[10]
    return out + stream_data, changed_id


def test_rejects_validly_repacked_wrong_owner(tmp_path):
    data, changed_id = reassign_clip_owner(checked_pack())
    path = tmp_path / "wrong-owner.mfp"
    path.write_bytes(data)
    result = run_checker(path)
    assert result.returncode != 0, (hex(changed_id), result.stdout, result.stderr)
    assert "owner" in result.stdout.lower(), result.stdout + result.stderr


def test_rejects_unclaimed_stream_tail(tmp_path):
    data = bytearray(checked_pack())
    header = list(HEADER.unpack_from(data))
    data.extend(b"TAIL")
    header[10] += 4
    HEADER.pack_into(data, 0, *header)
    path = tmp_path / "unused-tail.mfp"
    path.write_bytes(data)
    result = run_checker(path)
    assert result.returncode != 0, result.stdout + result.stderr
    assert "stream" in result.stdout.lower(), result.stdout + result.stderr


def test_roster_pricing_rejects_stale_or_wrong_pack_receipt(tmp_path):
    checked_pack()
    current = json.loads(PACK.with_name("audited-check.json").read_text())
    for key in ("pack_sha256", "validator_sha256", "corpus_inputs_sha256"):
        stale = dict(current, **{key: "0" * 64})
        path = tmp_path / (key + ".json")
        path.write_text(json.dumps(stale))
        with pytest.raises(ValueError, match="exact pack"):
            pricing.load_checked_pack(PACK, path)

"""Runtime owner identities must address the original relocated model payload."""
import re
import struct
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_nds_native_owners as native

ROOT = Path(__file__).resolve().parents[2]


@pytest.mark.parametrize("owner", ("yoshi", "nyoshi", "npikachu", "boss", "link", "pikachu"))
@pytest.mark.parametrize("detail", ("high", "low"))
def test_emitted_identity_matches_original_model(owner, detail):
    source = (ROOT / native.P2_O2R_ASSETS[owner][0]).read_bytes()
    externs = struct.unpack_from("<I", source, 0x48)[0]
    size_offset = 0x4C + externs * 2
    size = struct.unpack_from("<I", source, size_offset)[0]
    payload = source[size_offset + 4:]
    assert len(payload) == size

    context = native.build_p2_owner_runtime_context(ROOT, owner, detail)
    decode_roots = tuple(context["roots"])
    emitted = "\n".join(native.render_p2_owner_runtime_program(context))
    block = re.search(
        r"static const NDSNativeRoot \w+\[\d+\] =\s*\{(.*?)\n\};",
        emitted, re.S,
    )
    assert block, owner
    offsets = [int(value, 16) for value in re.findall(r"\{ 0x([0-9a-f]+)u,", block[1])]
    descriptors = native._owner_raw_joint_descriptors(payload, owner, detail)
    selected = native._owner_selected_descriptor_indices(owner, len(descriptors))
    expected = [descriptors[i][1] for i in selected if descriptors[i][1] is not None]
    assert offsets == expected
    assert all(offset + 8 <= size for offset in offsets)
    assert context["asset_data_size"] == size
    if detail == "high":
        match = re.search(r"#define NDS_NATIVE_\w+_MODEL_DATA_SIZE 0x([0-9a-f]+)u", emitted)
        assert match and int(match[1], 16) == size

    # Emission must not rewrite the compiler roots used by geometry oracles.
    assert tuple(context["roots"]) == decode_roots
    if owner == "yoshi":
        assert size == 0xACE0
        assert len(offsets) == 18
        assert any(row[0] >= size for row in decode_roots)
        assert (0x2248 if detail == "high" else 0x5BC8) in offsets
        assert (0x2C50 if detail == "high" else 0x6308) in offsets

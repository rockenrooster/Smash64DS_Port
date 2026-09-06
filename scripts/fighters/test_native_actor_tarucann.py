"""Host tests for the Congo Jungle barrel cannon native actor packet.

The barrel (grJungleMakeTaruCann, grjungle.c:107) runs source logic but never
submits a triangle. Its render path is a per-actor native owner packet beside
the fighter owners (stage-actor census, design A). These tests pin the packet
to its BattleShip inputs and prove the frozen fighter owners are untouched.
They never call the full fighter generate(): the polygon program is landing
concurrently in the same file, and these tests must stay green through it.
"""

from __future__ import annotations

import hashlib
import re
import struct
import sys
import shutil
import subprocess
from pathlib import Path

import pytest

_scripts_root = Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in sys.path:
    sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402

import generate_nds_native_owners as native  # noqa: E402
import check_nds_native_owner_packet as packet_check  # noqa: E402


REPO_ROOT = _paths.REPO_ROOT
PACKET_PATH = (
    REPO_ROOT / "src" / "nds" / "generated"
    / "nds_native_actor_tarucann.generated.inc"
)
HEADER_PATH = (
    REPO_ROOT / "include" / "nds" / "generated"
    / "nds_native_actor_tarucann.generated.h"
)
FIGHTER_INC = (
    REPO_ROOT / "src" / "nds" / "nds_native_fighter_owner.generated.inc"
)


def _parse_u32_words(text: str, symbol: str) -> list[int]:
    pattern = re.compile(
        rf"static const u32 {re.escape(symbol)}\[(\d+)\]\s*=\s*\n\{{\n(.*?)\n\}};",
        re.DOTALL,
    )
    matches = pattern.findall(text)
    assert len(matches) == 1, f"array {symbol} is missing/ambiguous"
    values = [
        int(value[:-1], 0)
        for value in re.findall(r"(?:0x[0-9a-fA-F]+|\d+)u", matches[0][1])
    ]
    assert len(values) == int(matches[0][0])
    return values


def test_actor_pins_come_from_jungle_descriptor():
    """Pins must be jungle.py's, not copies: the descriptor is the authority."""
    assert native.TARUCANN_JUNGLE_DESCRIPTOR is not None
    desc = native.TARUCANN_JUNGLE_DESCRIPTOR
    bank = desc.o2r_inputs["stage_actors"]
    assert bank["file_id"] == 158
    assert "MiscDataBank158" in bank["path"]
    assert desc.text_inputs["actors_typed"]["path"].endswith(
        "158_StageJungleFile3.c")
    assert desc.text_inputs["jungle"]["path"].endswith("grjungle.c")


def test_actor_decode_counts():
    parsed = native.decode_tarucann_actor(REPO_ROOT)
    assert parsed["counts"] == {
        "joints": 2,
        "bindings": 2,
        "runs": 1,
        "triangles": 2,
        "verts": 6,
        "texture_epochs": 1,
    }
    assert native._tarucann_slab_bytes(parsed) == 136


def test_actor_packet_and_header_regenerate_byte_exact():
    packet, header = native.generate_tarucann_actor(REPO_ROOT)
    assert PACKET_PATH.is_file()
    assert HEADER_PATH.is_file()
    assert PACKET_PATH.read_text() == packet
    assert HEADER_PATH.read_text() == header


def test_actor_checker_accepts_packet():
    packet_check.check_tarucann_actor(REPO_ROOT, PACKET_PATH, HEADER_PATH)


def test_actor_checker_rejects_swapped_triangle(tmp_path):
    """A permuted triangle must fail the byte-regeneration drift check."""
    packet = PACKET_PATH.read_text()
    pattern = re.compile(
        r"(static const u16 sNdsNativeActorTaruCannTriIndices\[6\]\s*=\s*\n\{\n)"
        r"(.*?)\n(\};)",
        re.DOTALL,
    )
    match = pattern.search(packet)
    assert match is not None
    rows = [line for line in match.group(2).splitlines() if line.strip()]
    assert len(rows) == 6
    rows[0], rows[1] = rows[1], rows[0]
    corrupted = (
        packet[:match.start(2)] + "\n".join(rows) + packet[match.end(2):]
    )
    corrupted_path = tmp_path / "tarucann.generated.inc"
    corrupted_path.write_text(corrupted)
    with pytest.raises(ValueError, match="stale generated native actor"):
        packet_check.check_tarucann_actor(
            REPO_ROOT, corrupted_path, HEADER_PATH)


def test_frozen_fighter_hashes_unchanged_on_disk():
    """Mario/Fox words still hash to the frozen values despite concurrent
    polygon work elsewhere in the generator file."""
    text = FIGHTER_INC.read_text()
    for owner, expected_hash in (
            ("Mario", 0x40F586C1), ("Fox", 0x791EB7A6)):
        words = _parse_u32_words(text, f"sNdsNative{owner}FifoWords")
        digest = struct.pack(f"<{len(words)}I", *words)
        assert int.from_bytes(hashlib.sha256(digest).digest()[:4],
                              "little") == expected_hash


def test_native_executor_uses_live_matrices_and_source_corners(tmp_path):
    """Compile the actual native executor against a recording GX interface."""
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler is not None, "native actor host check requires a C compiler"
    source = (REPO_ROOT / "src/nds/nds_renderer_native_owners.c").read_text()
    actor = source[:source.index("s32 ndsRendererExecuteNativeFighterOwnerHierarchy(")]
    actor = re.sub(r"^#include .*\n", "", actor, flags=re.MULTILINE)
    header = "\n".join(line for line in HEADER_PATH.read_text().splitlines()
                       if line.startswith("#define NDS_NATIVE_ACTOR_TARUCANN_"))
    harness = (Path(__file__).with_name("native_actor_tarucann_host.c")).read_text()
    implementation = header + "\n" + PACKET_PATH.read_text() + "\n" + actor
    test_c = tmp_path / "tarucann.c"
    test_c.write_text(harness.replace("/* NATIVE_ACTOR_IMPLEMENTATION */", implementation))
    executable = tmp_path / "tarucann.exe"
    subprocess.run([compiler, "-std=c11", "-Werror=implicit-function-declaration",
                    str(test_c), "-o", str(executable)], check=True, capture_output=True)
    subprocess.run([str(executable)], check=True, capture_output=True)


def test_native_state_compiler_rejects_unknown_command():
    resource = native._tarucann_bank(REPO_ROOT)
    with pytest.raises(ValueError, match="unsupported native state opcode"):
        native._tarucann_native_state(resource, [(0xAB000000, 0)], 0)


def test_barrel_route_has_no_interpreter_fallback():
    text = (REPO_ROOT / "src/port/reloc_backend_movement.c").read_text()
    start = text.index("static void ndsStageGCDrawAllLoopSubmitTaruCannDObj(")
    body = text[start:text.index("\n#endif", start)]
    assert "ndsRendererAdapterSubmitNativeTaruCann(" in body
    assert "SubmitItemDObjTree" not in body

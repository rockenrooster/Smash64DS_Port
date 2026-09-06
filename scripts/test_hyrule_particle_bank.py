"""Hyrule source bank closure, lifetime and native image checks (no ROM build)."""
import hashlib
from pathlib import Path
import struct
import re
import shutil
import subprocess

import generate_nds_particle_banks as native

ROOT = Path(__file__).resolve().parents[1]


def test_hyrule_preserves_complete_script_lifetimes():
    bank = native.build_hyrule_bank(ROOT)
    raw = native.load_o2r_blob(ROOT, *native.HYRULE_SCRIPT_BANK)
    assert bank["script_payload"] == raw
    assert len(raw) == 800
    scripts = bank["scripts"]
    assert len(scripts) == 8
    assert native.spawned_scripts(scripts[3], 8) == {0, 1, 2}
    assert native.spawned_scripts(scripts[7], 8) == {4, 5, 6}
    assert [struct.unpack_from(">HH", raw, s["offset"] + 4)
            for s in scripts] == [
        (0, 70), (0, 15), (0, 32), (60, 60),
        (60, 60), (60, 15), (60, 32), (60, 60)]
    assert bank["offsets"] == [36, 112, 204, 324, 384, 456, 548, 732]


def test_all_hyrule_frames_decode_with_source_colour_and_alpha():
    bank = native.build_hyrule_bank(ROOT)
    raw = native.load_o2r_blob(ROOT, *native.HYRULE_TEXTURE_BANK)
    source = native.parse_texture_bank(raw)
    payload = bank["payload"]
    assert len(payload) == 6816
    assert bank["texture_bytes"] == 6272
    assert bank["palette_bytes"] == 544
    assert [row["frames"] for row in bank["textures"]] == [1, 1, 5]
    assert [row["ds_format"] for row in bank["textures"]] == [
        native.DS_PAL16, native.DS_A5I3, native.DS_PAL256]
    frame_hashes = []
    for texture_id, row in enumerate(bank["textures"]):
        assert row["data_offset"] % 4 == 0
        assert row["palette_offset"] % 16 == 0
        palette = struct.unpack_from(
            f"<{row['palette_entries']}H", payload, row["palette_offset"])
        actual, expected = [], []
        for frame in range(row["frames"]):
            offset = row["data_offset"] + frame * row["frame_bytes"]
            data = payload[offset:offset + row["frame_bytes"]]
            actual.append(native.decode_ds_frame(data, row["ds_format"], palette,
                                                 row["width"] * row["height"]))
            expected.append(native.decode_texture_frame(raw, source[texture_id], frame))
            if texture_id == 2:
                frame_hashes.append(hashlib.sha256(data).digest())
        mean, maximum = native.measure_error(expected, actual)
        assert mean == row["mean_error"] and maximum == row["max_error"]
        assert maximum <= 4.0
        if texture_id == 2:
            assert maximum == 0.0
    assert len(set(frame_hashes)) == 5  # no frame-0 substitution or alias


def test_hyrule_emission_has_bounded_source_metadata():
    bank = native.build_hyrule_bank(ROOT)
    header = native.render_hyrule_header(bank)
    inc = native.render_hyrule_inc(bank)
    assert "NDS_HYRULE_NATIVE_FRAME_COUNT 7u" in header
    assert "NDS_HYRULE_NATIVE_ASSET_BYTES 6816u" in header
    assert "#if NDS_P2_STAGE_HYRULE" in inc
    assert "gNdsHyruleScriptOffsets" in inc
    assert len(bank["script_payload"]) + 4 * len(bank["offsets"]) + \
        20 * len(bank["textures"]) == 892


def c_function(source, name):
    """Copy a production function without its surrounding translation unit."""
    clean = re.sub(r"/\*.*?\*/|//[^\n]*", "", source, flags=re.S)
    match = re.search(r"(?:static\s+)?(?:void|s32|u32)\s+" + name + r"\s*\(", clean)
    assert match, f"missing production function {name}"
    opening = clean.index("{", match.start())
    depth = 1
    closing = opening + 1
    while depth:
        depth += (clean[closing] == "{") - (clean[closing] == "}")
        closing += 1
    return clean[match.start():closing]


def run_texture_host(source, tmp_path):
    bank = native.build_hyrule_bank(ROOT)
    first = source.index("typedef struct NDSHyruleTextureFill")
    last = source.index("\n#endif", first)
    declarations = source[source.index("static u32 sNdsHyruleNativeNames"):
                          source.index("\n#endif", source.index("static u32 sNdsHyruleNativeNames"))]
    code = native.render_hyrule_header(bank) + native.render_hyrule_inc(bank)
    code += declarations + "\n"
    code += c_function(source, "ndsRendererHardwareReleaseIFCommonCloudAtlas") + "\n"
    code += c_function(source, "ndsRendererHardwarePrepareIFCommonAtlas") + "\n"
    code += source[first:last]
    harness = Path(__file__).with_name("hyrule_texture_host.c").read_text()
    target = tmp_path / "hyrule.c"
    executable = tmp_path / "hyrule.exe"
    payload = tmp_path / "hyrule.bin"
    target.write_text(harness.replace("/* HYRULE_PRODUCTION_CODE */", code))
    payload.write_bytes(bank["payload"])
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler, "Hyrule texture host check requires a C compiler"
    subprocess.run([compiler, "-std=c11", "-Werror=implicit-function-declaration",
                    str(target), "-o", str(executable)], check=True, capture_output=True)
    subprocess.run([str(executable), str(payload)], check=True, capture_output=True)


def test_hyrule_texture_upload_release_and_partial_failure(tmp_path):
    run_texture_host((ROOT / "src/nds/nds_renderer_textures_effects.c").read_text(), tmp_path)

#!/usr/bin/env python3
"""Generate the exact Pupupu on-demand RGB256 microbenchmark corpus.

This is an offline falsifier, not a renderer asset.  It derives every case
from the pinned BattleShip water scripts and compares a hardware-visible
RGB256 representation against the existing RGB5A1 oracle.  Texture index zero
is transparent; opaque CI4 pairs use stable indices 1..N and an active RGB555
palette.  Hidden RGB bits on transparent oracle pixels are intentionally not
observable on Nintendo DS hardware and are reported separately.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from statistics import median
from typing import Iterable, Sequence

import generate_pupupu_water_aot as aot


DEFAULT_FIXTURE = (
    Path(__file__).resolve().parent
    / "fixtures"
    / "pupupu_water_rgb256_expected.json"
)

PAIR_INDEX_ENTRIES = 256
ACTIVE_PALETTE_ENTRY_BYTES = 2
TRANSPARENT_INDEX = 0
PAIR_PHASE_LUT_BYTES = 256 * 16
CLASS_TABLE_BYTES = 256 * 4
AXIS_CLASS_BYTES = (128 + 128) * 2
AXIS_REPRESENTATIVE_BYTES = 128 + 128
SOURCE_INDEX_CACHE_BYTES = 2 * 1024
OUTPUT_BUFFER_BYTES = 128 * 128
DS_RGB256_PALETTE_RESERVATION_BYTES = 256 * 2

# An intentionally conservative analytical estimate.  The checker also emits
# ARM946E-S/O3 disassembly and code/stack sizes; these are not device timing.
# The weights cover loop control and ordinary sequential memory access and are
# used only to decide whether a short runtime falsifier is warranted.
ARM_ESTIMATE_WEIGHTS = {
    "map_fixed": 128,
    "axis_point": 16,
    "class_table_clear_word": 3,
    "hash_probe": 8,
    "hash_collision_probe": 5,
    "unique_pixel": 22,
    "repeated_column_pixel": 11,
    "copied_row_word": 5,
    "palette_fixed": 128,
    "palette_pair_scan": 30,
    "palette_used_pair": 64,
    "source_decode_fixed": 32,
    "source_decode_packed_byte": 10,
}


def fnv1a32(payload: bytes) -> int:
    value = 2166136261
    for byte in payload:
        value ^= byte
        value = (value * 16777619) & 0xFFFFFFFF
    return value


def percentile_nearest_rank(values: Sequence[int], percent: int) -> int:
    if not values:
        return 0
    ordered = sorted(values)
    return ordered[max(0, math.ceil(len(ordered) * percent / 100) - 1)]


def transition_count(values: Sequence[object]) -> int:
    return sum(
        values[index] != values[(index + 1) % len(values)]
        for index in range(len(values))
    )


def summarize_nonzero(values: Sequence[int]) -> dict[str, int]:
    active = [value for value in values if value]
    return {
        "active_frames": len(active),
        "cycle_total": sum(values),
        "average_per_all_frames": sum(values) // len(values),
        "median_per_active_frame": int(median(active)) if active else 0,
        "p95_per_active_frame": percentile_nearest_rank(active, 95),
        "worst_frame": max(values, default=0),
    }


@dataclass(frozen=True)
class Case:
    owner: int
    texture0: int
    texture1: int
    fraction: int
    delta_s: int
    delta_t: int
    map_payload: bytes
    palette_payload: bytes
    estimate_cycles: int
    unique_s: int
    unique_t: int
    hash_probes: int
    hash_collisions: int


@dataclass(frozen=True)
class Model:
    artifacts: aot.GeneratedArtifacts
    pair_to_index: bytes
    cases: tuple[Case, ...]
    summary: dict[str, object]

    @property
    def map_oracle(self) -> bytes:
        return b"".join(case.map_payload for case in self.cases)

    @property
    def palette_oracle(self) -> bytes:
        return b"".join(case.palette_payload for case in self.cases)


def source_texture_index(spec: aot.MObjSpec, texture_id: int) -> int:
    if texture_id >= len(spec.sprite_offsets):
        raise aot.falsify("RGB256 microbench texture ID left the source table")
    try:
        return aot.TEXTURE_OFFSETS.index(spec.sprite_offsets[texture_id])
    except ValueError as exc:
        raise aot.falsify("RGB256 microbench found an unknown CI4 source") from exc


def pair_map(
    artifacts: aot.GeneratedArtifacts,
    spec: aot.MObjSpec,
    key: aot.WaterKey,
) -> bytes:
    texture0, texture1 = aot.key_sources(artifacts.corpus, spec, key)
    source0_s, source1_s, source0_t, source1_t = aot.render_coordinate_maps(
        spec, key
    )
    output = bytearray(spec.width * spec.height)
    offset = 0
    for y in range(spec.height):
        row0 = source0_t[y] * aot.CI4_SOURCE_WIDTH
        row1 = source1_t[y] * aot.CI4_SOURCE_WIDTH
        for x in range(spec.width):
            index0 = aot.ci4_index(texture0, row0 + source0_s[x])
            index1 = aot.ci4_index(texture1, row1 + source1_s[x])
            output[offset] = (index0 << 4) | index1
            offset += 1
    return bytes(output)


def axis_class(
    owner: int,
    extent: int,
    delta: int,
    coord: int,
    is_s: bool,
) -> int:
    if owner == aot.OWNER_LARGE and is_s:
        local = coord & 31
        source0 = 31 - local if ((coord >> 5) & 1) else local
    else:
        source0 = coord & 31
    shifted = coord + delta
    if shifted < 0:
        shifted = 0
    elif shifted >= extent:
        shifted = extent - 1
    source1 = shifted & 31
    return source0 | (source1 << 5)


def representative_axis(
    owner: int, extent: int, delta: int, is_s: bool
) -> tuple[list[int], list[int], int, int]:
    classes = [axis_class(owner, extent, delta, i, is_s) for i in range(extent)]
    table = [0] * 256
    representatives: list[int] = []
    probes = 0
    collisions = 0
    for index, value in enumerate(classes):
        key = value | ((index & 3) << 10)
        stored_key = key + 1
        slot = ((key * 0x9E3779B1) & 0xFFFFFFFF) >> 24
        while table[slot]:
            probes += 1
            entry = table[slot]
            if (entry & 0x1FFF) == stored_key:
                representatives.append(entry >> 13)
                break
            collisions += 1
            slot = (slot + 1) & 255
        else:
            probes += 1
            table[slot] = (index << 13) | stored_key
            representatives.append(index)
    return classes, representatives, probes, collisions


def estimate_map(spec: aot.MObjSpec, key: aot.WaterKey) -> dict[str, int]:
    delta_s = aot.quarter_to_texel(
        aot.tile_origin_delta(key.tile0_uls, key.tile1_uls)
    )
    delta_t = aot.quarter_to_texel(
        aot.tile_origin_delta(key.tile0_ult, key.tile1_ult)
    )
    _, rep_s, probes_s, collisions_s = representative_axis(
        spec.owner, spec.width, delta_s, True
    )
    _, rep_t, probes_t, collisions_t = representative_axis(
        spec.owner, spec.height, delta_t, False
    )
    unique_s = sum(index == value for index, value in enumerate(rep_s))
    unique_t = sum(index == value for index, value in enumerate(rep_t))
    unique_pixels = unique_s * unique_t
    repeated_columns = (spec.width - unique_s) * unique_t
    copied_row_words = (spec.height - unique_t) * (spec.width // 4)
    weights = ARM_ESTIMATE_WEIGHTS
    probes = probes_s + probes_t
    collisions = collisions_s + collisions_t
    cycles = (
        weights["map_fixed"]
        + (spec.width + spec.height) * weights["axis_point"]
        + 512 * weights["class_table_clear_word"]
        + probes * weights["hash_probe"]
        + collisions * weights["hash_collision_probe"]
        + unique_pixels * weights["unique_pixel"]
        + repeated_columns * weights["repeated_column_pixel"]
        + copied_row_words * weights["copied_row_word"]
    )
    return {
        "cycles": cycles,
        "unique_s": unique_s,
        "unique_t": unique_t,
        "hash_probes": probes,
        "hash_collisions": collisions,
        "unique_pixels": unique_pixels,
        "repeated_column_pixels": repeated_columns,
        "copied_row_words": copied_row_words,
    }


def palette_prepare_estimate(used_pairs: int) -> int:
    weights = ARM_ESTIMATE_WEIGHTS
    return (
        weights["palette_fixed"]
        + PAIR_INDEX_ENTRIES * weights["palette_pair_scan"]
        + used_pairs * weights["palette_used_pair"]
    )


def source_decode_estimate() -> int:
    weights = ARM_ESTIMATE_WEIGHTS
    return (
        weights["source_decode_fixed"]
        + aot.CI4_SOURCE_BYTES * weights["source_decode_packed_byte"]
    )


def frame_models(
    artifacts: aot.GeneratedArtifacts,
    cases_by_key: dict[tuple[int, aot.WaterKey], Case],
    palette_bytes: int,
    palette_cycles: int,
) -> dict[str, object]:
    cycles = [
        owner.census.sequence[: aot.EXPECTED_CYCLE_FRAMES]
        for owner in artifacts.owners
    ]
    fractions = [key.fraction for key in cycles[0]]
    if [key.fraction for key in cycles[1]] != fractions:
        raise aot.falsify("RGB256 microbench owner fractions diverged")

    case_maps = [
        [cases_by_key[(owner.census.spec.owner, key)] for key in sequence]
        for owner, sequence in zip(artifacts.owners, cycles)
    ]
    conservative_bytes: list[int] = []
    conservative_cycles: list[int] = []
    visible_bytes: list[int] = []
    visible_cycles: list[int] = []
    for frame in range(aot.EXPECTED_CYCLE_FRAMES):
        next_frame = (frame + 1) % aot.EXPECTED_CYCLE_FRAMES
        fraction_changed = fractions[frame] != fractions[next_frame]
        base_bytes = palette_bytes if fraction_changed else 0
        base_cycles = palette_cycles if fraction_changed else 0
        raw_bytes = base_bytes
        raw_cycles = base_cycles
        dedup_bytes = base_bytes
        dedup_cycles = base_cycles
        for owner_index, owner in enumerate(artifacts.owners):
            current_key = cycles[owner_index][frame]
            next_key = cycles[owner_index][next_frame]
            next_case = case_maps[owner_index][next_frame]
            map_bytes = owner.census.spec.width * owner.census.spec.height
            if current_key != next_key:
                raw_bytes += map_bytes
                raw_cycles += next_case.estimate_cycles
            current_map = case_maps[owner_index][frame].map_payload
            if current_map != next_case.map_payload:
                dedup_bytes += map_bytes
                dedup_cycles += next_case.estimate_cycles
        conservative_bytes.append(raw_bytes)
        conservative_cycles.append(raw_cycles)
        visible_bytes.append(dedup_bytes)
        visible_cycles.append(dedup_cycles)

    def histogram(values: Sequence[int]) -> dict[str, int]:
        return {str(value): count for value, count in sorted(Counter(values).items())}

    return {
        "conservative_key_change": {
            "bytes": summarize_nonzero(conservative_bytes),
            "cycles": summarize_nonzero(conservative_cycles),
            "bytes_per_frame_histogram": histogram(conservative_bytes),
        },
        "visible_map_dedup": {
            "bytes": summarize_nonzero(visible_bytes),
            "cycles": summarize_nonzero(visible_cycles),
            "bytes_per_frame_histogram": histogram(visible_bytes),
        },
        "owner_key_changes": {
            owner.census.spec.name: transition_count(sequence)
            for owner, sequence in zip(artifacts.owners, cycles)
        },
        "owner_visible_map_changes": {
            owner.census.spec.name: transition_count(
                [case.map_payload for case in owner_cases]
            )
            for owner, owner_cases in zip(artifacts.owners, case_maps)
        },
        "palette_changes": transition_count(fractions),
    }


def build_model(repo_root: Path) -> Model:
    artifacts = aot.generate(repo_root.resolve())
    raw_cases: list[tuple[aot.OwnerOutputs, aot.WaterKey, bytes]] = []
    used_pairs: set[int] = set()
    for owner in artifacts.owners:
        for key in owner.census.unique_keys:
            pairs = pair_map(artifacts, owner.census.spec, key)
            used_pairs.update(pairs)
            raw_cases.append((owner, key, pairs))
    ordered_pairs = sorted(used_pairs)
    if len(ordered_pairs) > 255:
        raise aot.falsify("RGB256 microbench needs more than 255 opaque indices")
    pair_to_index = bytearray(256)
    for index, pair in enumerate(ordered_pairs, 1):
        pair_to_index[pair] = index

    cases: list[Case] = []
    cases_by_key: dict[tuple[int, aot.WaterKey], Case] = {}
    oracle_pixels = 0
    alpha_mismatches = 0
    opaque_rgb_mismatches = 0
    visible_pixel_mismatches = 0
    index_byte_mismatches = 0
    hidden_transparent_rgb_pixels = 0
    for owner, key, pairs in raw_cases:
        spec = owner.census.spec
        pair_lut = aot.build_pair_lut(artifacts.corpus.palette, key.fraction)
        active_palette = [0]
        active_palette.extend(pair_lut[pair] & 0x7FFF for pair in ordered_pairs)
        palette_payload = struct.pack(
            f"<{len(active_palette)}H", *active_palette
        )
        expected = owner.images[owner.key_to_local_image[key]].expanded
        expected_pixels = struct.unpack(f"<{len(expected) // 2}H", expected)
        output = bytearray(len(pairs))
        for offset, (pair, expected_pixel) in enumerate(zip(pairs, expected_pixels)):
            x = offset % spec.width
            y = offset // spec.width
            resolved = aot.resolve_pair(pair_lut[pair], x, y)
            alpha_mismatches += (resolved >> 15) != (expected_pixel >> 15)
            opaque = resolved >> 15
            index = pair_to_index[pair] if opaque else TRANSPARENT_INDEX
            output[offset] = index
            expected_index = (
                pair_to_index[pair] if (expected_pixel >> 15) else TRANSPARENT_INDEX
            )
            index_byte_mismatches += index != expected_index
            if opaque:
                reconstructed = active_palette[index] | 0x8000
                opaque_rgb_mismatches += (
                    (reconstructed & 0x7FFF) != (expected_pixel & 0x7FFF)
                )
                visible_pixel_mismatches += reconstructed != expected_pixel
            else:
                hidden_transparent_rgb_pixels += (
                    (expected_pixel & 0x7FFF) != active_palette[0]
                )
                visible_pixel_mismatches += (expected_pixel >> 15) != 0
        oracle_pixels += len(pairs)
        estimate = estimate_map(spec, key)
        case = Case(
            owner=spec.owner,
            texture0=source_texture_index(spec, key.texture0),
            texture1=source_texture_index(spec, key.texture1),
            fraction=key.fraction,
            delta_s=aot.quarter_to_texel(
                aot.tile_origin_delta(key.tile0_uls, key.tile1_uls)
            ),
            delta_t=aot.quarter_to_texel(
                aot.tile_origin_delta(key.tile0_ult, key.tile1_ult)
            ),
            map_payload=bytes(output),
            palette_payload=palette_payload,
            estimate_cycles=estimate["cycles"],
            unique_s=estimate["unique_s"],
            unique_t=estimate["unique_t"],
            hash_probes=estimate["hash_probes"],
            hash_collisions=estimate["hash_collisions"],
        )
        cases.append(case)
        cases_by_key[(spec.owner, key)] = case

    case_tuple = tuple(cases)
    map_oracle = b"".join(case.map_payload for case in case_tuple)
    palette_oracle = b"".join(case.palette_payload for case in case_tuple)
    active_palette_bytes = (len(ordered_pairs) + 1) * 2
    prepare_cycles = palette_prepare_estimate(len(ordered_pairs))
    frames = frame_models(
        artifacts, cases_by_key, active_palette_bytes, prepare_cycles
    )
    owner_estimates: dict[str, object] = {}
    for owner in artifacts.owners:
        owner_cases = [case for case in case_tuple if case.owner == owner.census.spec.owner]
        values = [case.estimate_cycles for case in owner_cases]
        owner_estimates[owner.census.spec.name] = {
            "cases": len(owner_cases),
            "map_bytes": owner.census.spec.width * owner.census.spec.height,
            "median_cycles": int(median(values)),
            "p95_cycles": percentile_nearest_rank(values, 95),
            "worst_cycles": max(values),
            "unique_s_min": min(case.unique_s for case in owner_cases),
            "unique_s_max": max(case.unique_s for case in owner_cases),
            "unique_t_min": min(case.unique_t for case in owner_cases),
            "unique_t_max": max(case.unique_t for case in owner_cases),
            "unique_visible_maps": len(
                {case.map_payload for case in owner_cases}
            ),
        }

    independent_scratch = (
        PAIR_PHASE_LUT_BYTES
        + CLASS_TABLE_BYTES
        + AXIS_CLASS_BYTES
        + AXIS_REPRESENTATIVE_BYTES
        + SOURCE_INDEX_CACHE_BYTES
        + active_palette_bytes
    )
    decode_cycles = source_decode_estimate()
    conservative_cycles = frames["conservative_key_change"]["cycles"]  # type: ignore[index]
    conservative_bytes = frames["conservative_key_change"]["bytes"]  # type: ignore[index]
    retained_median = 168064 + 4416
    retained_p95 = 184576 + 4480
    candidate_median = conservative_cycles["median_per_active_frame"]
    candidate_worst = conservative_cycles["worst_frame"]
    summary: dict[str, object] = {
        "schema": 1,
        "source": {
            "bank103_sha256": artifacts.corpus.bank103.source_sha256,
            "bank104_sha256": artifacts.corpus.bank104.source_sha256,
            "cycle_frames": aot.EXPECTED_CYCLE_FRAMES,
            "cases": len(case_tuple),
            "oracle_pixels": oracle_pixels,
            "map_oracle_bytes": len(map_oracle),
            "map_oracle_sha256": hashlib.sha256(map_oracle).hexdigest(),
            "palette_oracle_bytes": len(palette_oracle),
            "palette_oracle_sha256": hashlib.sha256(palette_oracle).hexdigest(),
            "fractions": len({case.fraction for case in case_tuple}),
            "observed_ci4_pairs": len(ordered_pairs),
        },
        "parity": {
            "index_byte_mismatches": index_byte_mismatches,
            "alpha_mismatches": alpha_mismatches,
            "opaque_rgb555_mismatches": opaque_rgb_mismatches,
            "visible_pixel_mismatches": visible_pixel_mismatches,
            "transparent_hidden_rgb_nonzero_pixels": hidden_transparent_rgb_pixels,
            "transparent_hidden_rgb_is_hardware_observable": False,
        },
        "hardware_contract": {
            "texture_format": "GL_RGB256",
            "transparent_index": TRANSPARENT_INDEX,
            "opaque_index_first": 1,
            "opaque_index_last": len(ordered_pairs),
            "active_palette_entries": len(ordered_pairs) + 1,
            "active_palette_upload_bytes": active_palette_bytes,
            "palette_vram_reservation_bytes": DS_RGB256_PALETTE_RESERVATION_BYTES,
            "maximum_map_upload_bytes": OUTPUT_BUFFER_BYTES,
            "maximum_combined_upload_bytes": (
                OUTPUT_BUFFER_BYTES + 32 * 64 + active_palette_bytes
            ),
            "alpha_semantics": "BattleShip 4x4 ordered coverage; exact phase mask",
            "source0_s_semantics": "32-texel mask with mirror for large owner",
            "source0_t_semantics": "32-texel masked rows",
            "source1_semantics": "logical clamp followed by 32-texel mask",
        },
        "production_footprint": {
            "existing_source_ci4_bytes": len(artifacts.corpus.textures) * aot.CI4_SOURCE_BYTES,
            "existing_source_palette_bytes": len(artifacts.corpus.palette) * 2,
            "new_constant_pair_index_bytes": len(pair_to_index),
            "new_constant_alpha_prefix_bytes": len(aot.ALPHA_PHASE_PREFIX) * 2,
            "independent_pair_phase_lut_bytes": PAIR_PHASE_LUT_BYTES,
            "independent_class_table_bytes": CLASS_TABLE_BYTES,
            "independent_axis_bytes": AXIS_CLASS_BYTES + AXIS_REPRESENTATIVE_BYTES,
            "independent_source_index_cache_bytes": SOURCE_INDEX_CACHE_BYTES,
            "active_palette_bytes": active_palette_bytes,
            "independent_working_bytes_unpadded": independent_scratch,
            "reusable_output_buffer_bytes": OUTPUT_BUFFER_BYTES,
            "integration_reuse_note": (
                "current renderer already owns a 1 KiB pair LUT, 1 KiB class "
                "table, axis arrays, two 1 KiB CI4 index caches, and a >=16 KiB "
                "texture arena; this candidate replaces the pair LUT with 4 KiB"
            ),
        },
        "arm_static_estimator": {
            "kind": "conservative analytical estimate; not device measurement",
            "target": "ARM946E-S ARM state O3",
            "weights": ARM_ESTIMATE_WEIGHTS,
            "palette_prepare_cycles": prepare_cycles,
            "cold_source_decode_cycles_per_texture": decode_cycles,
            "owners": owner_estimates,
            "cold_both_maps_plus_palette_cycles": (
                prepare_cycles
                + (2 * decode_cycles)
                + max(
                    case.estimate_cycles
                    for case in case_tuple
                    if case.owner == aot.OWNER_LARGE
                )
                + max(
                    case.estimate_cycles
                    for case in case_tuple
                    if case.owner == aot.OWNER_SMALL
                )
            ),
        },
        "cycle_update_model": frames,
        "comparison_to_retained_rgb16": {
            "source": "docs/PERF_LEDGER.md compact-direct retained profile-1 frames 220..347",
            "rgb16_convert_median_ticks": 168064,
            "rgb16_convert_p95_ticks": 184576,
            "rgb16_stage_median_ticks": 4416,
            "rgb16_stage_p95_ticks": 4480,
            "rgb16_convert_plus_stage_median_ticks": retained_median,
            "rgb16_convert_plus_stage_p95_ticks": retained_p95,
            "rgb16_worst_full_map_upload_bytes": 36864,
            "rgb256_conservative_active_median_estimated_cycles": candidate_median,
            "rgb256_conservative_worst_estimated_cycles": candidate_worst,
            "rgb256_worst_map_plus_palette_upload_bytes": conservative_bytes[
                "worst_frame"
            ],
            "median_estimate_delta_ticks": candidate_median - retained_median,
            "worst_estimate_vs_retained_p95_delta_ticks": (
                candidate_worst - retained_p95
            ),
        },
        "verdict": {
            "milestone_4_complete": False,
            "runtime_8_frame_falsifier": (
                "ELIGIBLE_IF_ARM946E_S_NO_GX_AND_EXACT_BYTE_CHECKS_PASS"
            ),
            "palette_vram_mapping_still_required": True,
            "reason": (
                "the conservative active-frame median is below the retained "
                "RGB16 conversion-plus-stage median, its worst estimate is within "
                "10 percent of retained P95, and peak upload is approximately half; "
                "only an eight-frame synchronized device falsifier can resolve the "
                "remaining estimate uncertainty"
            ),
        },
    }
    return Model(
        artifacts=artifacts,
        pair_to_index=bytes(pair_to_index),
        cases=case_tuple,
        summary=summary,
    )


def c_bytes(name: str, payload: bytes, columns: int = 16) -> Iterable[str]:
    yield f"static const uint8_t {name}[{len(payload)}] = {{"
    for offset in range(0, len(payload), columns):
        row = ", ".join(f"0x{value:02x}u" for value in payload[offset : offset + columns])
        yield f"    {row},"
    yield "};"


def render_header(model: Model) -> str:
    palette = model.artifacts.corpus.palette
    lines = [
        "#ifndef PUPUPU_WATER_RGB256_CORPUS_GENERATED_H",
        "#define PUPUPU_WATER_RGB256_CORPUS_GENERATED_H",
        "",
        "#include <stdint.h>",
        "",
        f"#define PUPUPU_WATER_RGB256_CASE_COUNT {len(model.cases)}u",
        f"#define PUPUPU_WATER_RGB256_ORACLE_BYTES {len(model.map_oracle)}u",
        f"#define PUPUPU_WATER_RGB256_PAIR_COUNT {max(model.pair_to_index)}u",
        f"#define PUPUPU_WATER_RGB256_PALETTE_ENTRIES {max(model.pair_to_index) + 1}u",
        "#define PUPUPU_WATER_RGB256_MAX_MAP_BYTES 16384u",
        f'#define PUPUPU_WATER_RGB256_MAP_SHA256 "{model.summary["source"]["map_oracle_sha256"]}"',  # type: ignore[index]
        "",
        "typedef struct PupupuWaterRgb256Case",
        "{",
        "    uint8_t owner;",
        "    uint8_t texture0;",
        "    uint8_t texture1;",
        "    uint8_t fraction;",
        "    int16_t delta_s;",
        "    int16_t delta_t;",
        "    uint32_t map_fnv1a;",
        "    uint32_t palette_fnv1a;",
        "} PupupuWaterRgb256Case;",
        "",
    ]
    lines.extend(c_bytes("gPupupuWaterRgb256PairToIndex", model.pair_to_index))
    lines.extend(["", "static const uint16_t gPupupuWaterRgb256SourcePalette[16] = {"])
    for offset in range(0, len(palette), 8):
        lines.append(
            "    "
            + ", ".join(f"0x{value:04x}u" for value in palette[offset : offset + 8])
            + ","
        )
    lines.extend(["};", "", "static const uint8_t gPupupuWaterRgb256Textures[3][512] = {"])
    for texture in model.artifacts.corpus.textures:
        lines.append("    {")
        for offset in range(0, len(texture), 16):
            lines.append(
                "        "
                + ", ".join(
                    f"0x{value:02x}u" for value in texture[offset : offset + 16]
                )
                + ","
            )
        lines.append("    },")
    lines.extend(["};", "", "static const PupupuWaterRgb256Case gPupupuWaterRgb256Cases[] = {"])
    for case in model.cases:
        lines.append(
            "    { "
            f"{case.owner}u, {case.texture0}u, {case.texture1}u, {case.fraction}u, "
            f"{case.delta_s}, {case.delta_t}, "
            f"0x{fnv1a32(case.map_payload):08x}u, "
            f"0x{fnv1a32(case.palette_payload):08x}u "
            "},"
        )
    lines.extend(["};", "", "#endif", ""])
    return "\n".join(lines)


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument("--fixture", type=Path, default=DEFAULT_FIXTURE)
    parser.add_argument("--write-fixture", action="store_true")
    parser.add_argument("--emit-header", type=Path)
    parser.add_argument("--emit-map-oracle", type=Path)
    parser.add_argument("--emit-palette-oracle", type=Path)
    return parser.parse_args(argv)


def write_payload(path: Path, payload: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(payload)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        model = build_model(args.repo_root)
        fixture = args.fixture.resolve()
        if args.write_fixture:
            fixture.parent.mkdir(parents=True, exist_ok=True)
            fixture.write_text(
                json.dumps(model.summary, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
        else:
            expected = json.loads(fixture.read_text(encoding="utf-8"))
            if model.summary != expected:
                raise aot.falsify("RGB256 microbench differs from pinned fixture")
        if args.emit_header:
            args.emit_header.write_text(render_header(model), encoding="utf-8")
        if args.emit_map_oracle:
            write_payload(args.emit_map_oracle, model.map_oracle)
        if args.emit_palette_oracle:
            write_payload(args.emit_palette_oracle, model.palette_oracle)
    except (aot.Falsifier, OSError, ValueError, KeyError) as exc:
        print(f"PUPUPU_WATER_RGB256_GENERATOR_FAIL: {exc}")
        return 1

    source = model.summary["source"]
    parity = model.summary["parity"]
    estimates = model.summary["arm_static_estimator"]
    assert isinstance(source, dict)
    assert isinstance(parity, dict)
    assert isinstance(estimates, dict)
    print(
        "PUPUPU_WATER_RGB256_GENERATOR_OK "
        f"cases={source['cases']} pixels={source['oracle_pixels']} "
        f"pairs={source['observed_ci4_pairs']} "
        f"index_mismatches={parity['index_byte_mismatches']} "
        f"visible_mismatches={parity['visible_pixel_mismatches']} "
        f"palette_estimate={estimates['palette_prepare_cycles']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

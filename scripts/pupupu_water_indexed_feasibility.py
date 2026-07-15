#!/usr/bin/env python3
"""Exact offline feasibility model for DS-indexed Pupupu water textures.

This module deliberately produces no runtime asset.  It starts from the
source-faithful AOT corpus, separates every output texel into the two CI4
source indices, and tests whether Nintendo DS paletted texture formats can
represent the result without gameplay conversion or file I/O.

The important hardware distinction is that a DS ``GL_RGB256`` palette stores
RGB555 colors.  Only texture index zero can be selected as transparent; bit 15
is not an independent alpha bit for every palette entry.  The BattleShip
water, meanwhile, alpha-dithers the blend of its one transparent CI4 source
color over a 4x4 phase matrix.  The model therefore reports both:

* an exact software pair-index expansion using a 16-phase alpha mask; and
* the closest hardware-visible RGB256 encoding, where every transparent texel
  maps to index zero and opaque colors remain exact.

Neither is promoted here.  The pinned fixture is a falsifiable go/no-go input
for a later renderer design.
"""

from __future__ import annotations

import argparse
import json
import struct
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence

import generate_pupupu_water_aot as aot


DEFAULT_FIXTURE = (
    Path(__file__).resolve().parent
    / "fixtures"
    / "pupupu_water_indexed_expected.json"
)

# Cut G currently maps only 128 KiB banks A/B as texture VRAM.  Banks C/D are
# the two retained affine BG bitmaps; E/F/G are the native countdown/GO OBJ
# aperture.  The maximum figures are the physical DS contracts, not a proposal
# to remap those live banks.
CUT_G_TEXTURE_VRAM_BYTES = 2 * 128 * 1024
DS_MAX_TEXTURE_VRAM_BYTES = 4 * 128 * 1024
CUT_G_TEXTURE_PALETTE_VRAM_BYTES = 0
DS_MAX_TEXTURE_PALETTE_VRAM_BYTES = 64 * 1024 + 16 * 1024 + 16 * 1024

PAIR_PALETTE_ENTRIES = 256
RGB5A1_ENTRY_BYTES = 2
PHASE_LUT_ENTRY_BYTES = 4
RGB256_TRANSPARENT_INDICES = 1


@dataclass(frozen=True)
class KeyPixels:
    pair_map: bytes
    expected: bytes


@dataclass(frozen=True)
class OwnerModel:
    owner: aot.OwnerOutputs
    key_pixels: dict[aot.WaterKey, KeyPixels]
    pair_maps: tuple[bytes, ...]
    pair_map_id: dict[bytes, int]


def _pixel_values(payload: bytes) -> tuple[int, ...]:
    if len(payload) & 1:
        raise aot.falsify("indexed feasibility: odd RGB5A1 byte count")
    return struct.unpack(f"<{len(payload) // 2}H", payload)


def _pair_map(
    corpus: aot.SourceCorpus,
    spec: aot.MObjSpec,
    key: aot.WaterKey,
) -> bytes:
    texture0, texture1 = aot.key_sources(corpus, spec, key)
    source0_s, source1_s, source0_t, source1_t = aot.render_coordinate_maps(
        spec, key
    )
    result = bytearray(spec.width * spec.height)
    offset = 0
    for y in range(spec.height):
        row0 = source0_t[y] * aot.CI4_SOURCE_WIDTH
        row1 = source1_t[y] * aot.CI4_SOURCE_WIDTH
        for x in range(spec.width):
            index0 = aot.ci4_index(texture0, row0 + source0_s[x])
            index1 = aot.ci4_index(texture1, row1 + source1_s[x])
            result[offset] = (index0 << 4) | index1
            offset += 1
    return bytes(result)


def _build_owner_model(
    artifacts: aot.GeneratedArtifacts,
    owner: aot.OwnerOutputs,
) -> OwnerModel:
    key_pixels: dict[aot.WaterKey, KeyPixels] = {}
    pair_map_id: dict[bytes, int] = {}
    pair_maps: list[bytes] = []
    for key in owner.census.unique_keys:
        pair_map = _pair_map(artifacts.corpus, owner.census.spec, key)
        image = owner.images[owner.key_to_local_image[key]]
        if pair_map not in pair_map_id:
            pair_map_id[pair_map] = len(pair_maps)
            pair_maps.append(pair_map)
        key_pixels[key] = KeyPixels(pair_map=pair_map, expected=image.expanded)
    return OwnerModel(
        owner=owner,
        key_pixels=key_pixels,
        pair_maps=tuple(pair_maps),
        pair_map_id=pair_map_id,
    )


def _global_row_archive(pair_maps: Sequence[bytes], width: int, height: int) -> dict[str, int]:
    rows: dict[bytes, int] = {}
    for pair_map in pair_maps:
        if len(pair_map) != width * height:
            raise aot.falsify("indexed feasibility: pair-map shape changed")
        for y in range(height):
            row = pair_map[y * width : (y + 1) * width]
            rows.setdefault(row, len(rows))
    row_id_bytes = 1 if len(rows) <= 256 else 2
    dictionary_bytes = len(rows) * width
    row_map_bytes = len(pair_maps) * height * row_id_bytes
    return {
        "global_rows": len(rows),
        "row_id_bytes": row_id_bytes,
        "dictionary_bytes": dictionary_bytes,
        "row_maps_bytes": row_map_bytes,
        "total_bytes": dictionary_bytes + row_map_bytes,
    }


def _periodic_mismatches(
    model: OwnerModel,
    period_width: int,
    period_height: int,
) -> dict[str, int]:
    spec = model.owner.census.spec
    unique_crops: set[bytes] = set()
    unique_pair_mismatches = 0
    exact_unique_maps = 0
    for pair_map in model.pair_maps:
        crop = bytes(
            pair_map[y * spec.width + x]
            for y in range(period_height)
            for x in range(period_width)
        )
        unique_crops.add(crop)
        mismatches = sum(
            pair_map[y * spec.width + x]
            != crop[(y % period_height) * period_width + (x % period_width)]
            for y in range(spec.height)
            for x in range(spec.width)
        )
        unique_pair_mismatches += mismatches
        exact_unique_maps += mismatches == 0

    oracle_pair_mismatches = 0
    oracle_output_mismatches = 0
    for key in model.owner.census.unique_keys:
        pixels = model.key_pixels[key]
        crop = bytes(
            pixels.pair_map[y * spec.width + x]
            for y in range(period_height)
            for x in range(period_width)
        )
        expected = _pixel_values(pixels.expected)
        pair_lut = model_pair_lut(model, key.fraction)
        for y in range(spec.height):
            for x in range(spec.width):
                offset = y * spec.width + x
                actual_pair = pixels.pair_map[offset]
                periodic_pair = crop[
                    (y % period_height) * period_width + (x % period_width)
                ]
                oracle_pair_mismatches += actual_pair != periodic_pair
                oracle_output_mismatches += (
                    aot.resolve_pair(pair_lut[periodic_pair], x, y)
                    != expected[offset]
                )
    return {
        "width": period_width,
        "height": period_height,
        "unique_crops": len(unique_crops),
        "resident_crop_bytes": len(unique_crops) * period_width * period_height,
        "exact_unique_maps": exact_unique_maps,
        "unique_pair_mismatch_pixels": unique_pair_mismatches,
        "oracle_pair_mismatch_pixels": oracle_pair_mismatches,
        "oracle_output_mismatch_pixels": oracle_output_mismatches,
    }


# Set while analyze() is active.  Keeping the exact source palette out of the
# public dataclasses makes accidental serialization impossible.
_ACTIVE_PALETTE: tuple[int, ...] | None = None


def model_pair_lut(model: OwnerModel, fraction: int) -> tuple[int, ...]:
    del model
    if _ACTIVE_PALETTE is None:
        raise aot.falsify("indexed feasibility: source palette is unavailable")
    return aot.build_pair_lut(_ACTIVE_PALETTE, fraction)


def _transition_count(values: Sequence[object]) -> int:
    return sum(
        values[index] != values[(index + 1) % len(values)]
        for index in range(len(values))
    )


def _owner_summary(model: OwnerModel) -> dict[str, object]:
    spec = model.owner.census.spec
    row_archive = _global_row_archive(model.pair_maps, spec.width, spec.height)
    cycle = model.owner.census.sequence[: aot.EXPECTED_CYCLE_FRAMES]
    map_sequence = [
        model.pair_map_id[model.key_pixels[key].pair_map] for key in cycle
    ]
    fraction_sequence = [key.fraction for key in cycle]
    requested = _periodic_mismatches(model, 32, 32)
    natural_width = 64 if spec.owner == aot.OWNER_LARGE else 32
    natural = _periodic_mismatches(model, natural_width, 32)
    return {
        "keys": len(model.owner.census.unique_keys),
        "width": spec.width,
        "height": spec.height,
        "unique_pair_maps": len(model.pair_maps),
        "resident_pair_map_bytes": len(model.pair_maps) * spec.width * spec.height,
        "global_row_archive": row_archive,
        "requested_32x32": requested,
        "natural_mask_period": natural,
        "map_changes_per_cycle": _transition_count(map_sequence),
        "fraction_changes_per_cycle": _transition_count(fraction_sequence),
    }


def _canonical_index_maps(
    models: Sequence[OwnerModel],
    classes_by_fraction: dict[int, set[tuple[int, int]]],
) -> tuple[dict[str, object], int, int]:
    class_ids = {
        fraction: {entry: index for index, entry in enumerate(sorted(entries))}
        for fraction, entries in classes_by_fraction.items()
    }
    canonical_mismatches = 0
    visible_mismatches = 0
    owner_summaries: dict[str, object] = {}

    for model in models:
        spec = model.owner.census.spec
        canonical_maps: set[bytes] = set()
        visible_maps: set[bytes] = set()
        canonical_by_key: dict[aot.WaterKey, bytes] = {}
        visible_by_key: dict[aot.WaterKey, bytes] = {}
        for key in model.owner.census.unique_keys:
            pixels = model.key_pixels[key]
            expected = _pixel_values(pixels.expected)
            pair_lut = model_pair_lut(model, key.fraction)
            ids = class_ids[key.fraction]
            opaque_pairs = sorted(
                pair for pair, alpha in classes_by_fraction[key.fraction] if alpha
            )
            opaque_ids = {pair: index + 1 for index, pair in enumerate(opaque_pairs)}
            canonical = bytearray(len(pixels.pair_map))
            visible = bytearray(len(pixels.pair_map))
            palette = [
                (pair_lut[pair] & 0x7FFF) | (alpha << 15)
                for pair, alpha in sorted(classes_by_fraction[key.fraction])
            ]
            for offset, pair in enumerate(pixels.pair_map):
                value = expected[offset]
                alpha = value >> 15
                index = ids[(pair, alpha)]
                canonical[offset] = index
                canonical_mismatches += palette[index] != value
                if alpha:
                    visible[offset] = opaque_ids[pair]
                    visible_mismatches += (pair_lut[pair] & 0x7FFF) != (value & 0x7FFF)
                else:
                    visible[offset] = 0
                    visible_mismatches += alpha != 0
            canonical_bytes = bytes(canonical)
            visible_bytes = bytes(visible)
            canonical_maps.add(canonical_bytes)
            visible_maps.add(visible_bytes)
            canonical_by_key[key] = canonical_bytes
            visible_by_key[key] = visible_bytes

        canonical_archive = _global_row_archive(
            tuple(sorted(canonical_maps)), spec.width, spec.height
        )
        visible_archive = _global_row_archive(
            tuple(sorted(visible_maps)), spec.width, spec.height
        )
        cycle = model.owner.census.sequence[: aot.EXPECTED_CYCLE_FRAMES]
        canonical_ids = {
            payload: index for index, payload in enumerate(sorted(canonical_maps))
        }
        visible_ids = {
            payload: index for index, payload in enumerate(sorted(visible_maps))
        }
        canonical_sequence = [canonical_ids[canonical_by_key[key]] for key in cycle]
        visible_sequence = [visible_ids[visible_by_key[key]] for key in cycle]
        owner_summaries[spec.name] = {
            "canonical_rgb5a1_maps": len(canonical_maps),
            "canonical_resident_bytes": len(canonical_maps) * spec.width * spec.height,
            "canonical_global_row_archive": canonical_archive,
            "canonical_map_changes_per_cycle": _transition_count(canonical_sequence),
            "rgb256_visible_maps": len(visible_maps),
            "rgb256_visible_resident_bytes": len(visible_maps) * spec.width * spec.height,
            "rgb256_visible_global_row_archive": visible_archive,
            "rgb256_visible_map_changes_per_cycle": _transition_count(visible_sequence),
        }
    return owner_summaries, canonical_mismatches, visible_mismatches


def analyze(artifacts: aot.GeneratedArtifacts) -> dict[str, object]:
    global _ACTIVE_PALETTE
    _ACTIVE_PALETTE = artifacts.corpus.palette
    try:
        models = tuple(
            _build_owner_model(artifacts, owner) for owner in artifacts.owners
        )
        owner_summaries = {
            model.owner.census.spec.name: _owner_summary(model) for model in models
        }

        fractions = sorted(
            {key.fraction for model in models for key in model.owner.census.unique_keys}
        )
        classes_by_fraction: dict[int, set[tuple[int, int]]] = defaultdict(set)
        alpha_counts: dict[tuple[int, int], list[int]] = defaultdict(lambda: [0, 0])
        exact_phase_mismatches = 0
        mixed_alpha_keys = 0
        per_key_mixed_entries = 0
        per_key_minimum_mismatches = 0
        max_key_classes = {"large": 0, "small": 0}
        oracle_pixels = 0

        for model in models:
            spec = model.owner.census.spec
            for key in model.owner.census.unique_keys:
                pixels = model.key_pixels[key]
                expected = _pixel_values(pixels.expected)
                pair_lut = model_pair_lut(model, key.fraction)
                key_alphas: set[int] = set()
                key_classes: set[tuple[int, int]] = set()
                key_alpha_counts: dict[int, list[int]] = defaultdict(
                    lambda: [0, 0]
                )
                for offset, pair in enumerate(pixels.pair_map):
                    value = aot.resolve_pair(
                        pair_lut[pair],
                        offset % spec.width,
                        offset // spec.width,
                    )
                    exact_phase_mismatches += value != expected[offset]
                    alpha = expected[offset] >> 15
                    key_alphas.add(alpha)
                    key_classes.add((pair, alpha))
                    classes_by_fraction[key.fraction].add((pair, alpha))
                    alpha_counts[(key.fraction, pair)][alpha] += 1
                    key_alpha_counts[pair][alpha] += 1
                mixed_alpha_keys += key_alphas == {0, 1}
                per_key_mixed_entries += sum(
                    1 for counts in key_alpha_counts.values() if all(counts)
                )
                per_key_minimum_mismatches += sum(
                    min(counts) for counts in key_alpha_counts.values()
                )
                max_key_classes[spec.name] = max(
                    max_key_classes[spec.name], len(key_classes)
                )
                oracle_pixels += len(pixels.pair_map)

        canonical, canonical_mismatches, visible_mismatches = _canonical_index_maps(
            models, classes_by_fraction
        )
        for name, summary in canonical.items():
            owner_summaries[name].update(summary)  # type: ignore[union-attr]

        mixed_entries = sum(1 for counts in alpha_counts.values() if all(counts))
        literal_minimum_mismatches = sum(min(counts) for counts in alpha_counts.values())
        tight_palette_bytes = sum(
            len(entries) * RGB5A1_ENTRY_BYTES
            for entries in classes_by_fraction.values()
        )
        pair_row_archive_bytes = sum(
            summary["global_row_archive"]["total_bytes"]  # type: ignore[index]
            for summary in owner_summaries.values()
        )
        pair_selection_bytes = sum(
            len(model.owner.census.unique_keys) * 2 for model in models
        )
        exact_phase_lut_bytes = len(fractions) * PAIR_PALETTE_ENTRIES * PHASE_LUT_ENTRY_BYTES
        literal_palette_bytes = len(fractions) * PAIR_PALETTE_ENTRIES * RGB5A1_ENTRY_BYTES
        exact_pair_archive_bytes = (
            pair_row_archive_bytes + pair_selection_bytes + exact_phase_lut_bytes
        )
        literal_pair_archive_bytes = (
            pair_row_archive_bytes + pair_selection_bytes + literal_palette_bytes
        )
        resident_pair_maps = sum(
            summary["resident_pair_map_bytes"]  # type: ignore[index]
            for summary in owner_summaries.values()
        )
        resident_pair_with_palettes = resident_pair_maps + literal_palette_bytes
        requested_crop_bytes = sum(
            summary["requested_32x32"]["resident_crop_bytes"]  # type: ignore[index]
            for summary in owner_summaries.values()
        )
        requested_crop_mismatches = sum(
            summary["requested_32x32"]["oracle_output_mismatch_pixels"]  # type: ignore[index]
            for summary in owner_summaries.values()
        )
        natural_crop_bytes = sum(
            summary["natural_mask_period"]["resident_crop_bytes"]  # type: ignore[index]
            for summary in owner_summaries.values()
        )
        natural_crop_mismatches = sum(
            summary["natural_mask_period"]["oracle_output_mismatch_pixels"]  # type: ignore[index]
            for summary in owner_summaries.values()
        )
        canonical_resident_bytes = sum(
            summary["canonical_resident_bytes"]  # type: ignore[index]
            for summary in owner_summaries.values()
        )
        canonical_archive_bytes = sum(
            summary["canonical_global_row_archive"]["total_bytes"]  # type: ignore[index]
            for summary in owner_summaries.values()
        ) + tight_palette_bytes
        visible_resident_bytes = sum(
            summary["rgb256_visible_resident_bytes"]  # type: ignore[index]
            for summary in owner_summaries.values()
        ) + literal_palette_bytes
        visible_archive_bytes = sum(
            summary["rgb256_visible_global_row_archive"]["total_bytes"]  # type: ignore[index]
            for summary in owner_summaries.values()
        ) + literal_palette_bytes

        cycles = [
            model.owner.census.sequence[: aot.EXPECTED_CYCLE_FRAMES]
            for model in models
        ]
        pair_sequences = [
            [model.pair_map_id[model.key_pixels[key].pair_map] for key in cycle]
            for model, cycle in zip(models, cycles)
        ]
        fraction_sequence = [key.fraction for key in cycles[0]]
        if [key.fraction for key in cycles[1]] != fraction_sequence:
            raise aot.falsify("indexed feasibility: owner blend fractions diverged")
        update_bytes: list[int] = []
        for frame in range(aot.EXPECTED_CYCLE_FRAMES):
            next_frame = (frame + 1) % aot.EXPECTED_CYCLE_FRAMES
            count = 0
            for model, sequence in zip(models, pair_sequences):
                if sequence[frame] != sequence[next_frame]:
                    spec = model.owner.census.spec
                    count += spec.width * spec.height
            if fraction_sequence[frame] != fraction_sequence[next_frame]:
                count += PAIR_PALETTE_ENTRIES * RGB5A1_ENTRY_BYTES
            update_bytes.append(count)
        palette_changes = _transition_count(fraction_sequence)
        visible_update_bytes = sum(
            owner_summaries[model.owner.census.spec.name][
                "rgb256_visible_map_changes_per_cycle"
            ]
            * model.owner.census.spec.width
            * model.owner.census.spec.height
            for model in models
        ) + palette_changes * PAIR_PALETTE_ENTRIES * RGB5A1_ENTRY_BYTES

        return {
            "schema": 1,
            "source": {
                "cycle_frames": aot.EXPECTED_CYCLE_FRAMES,
                "oracle_pixels": oracle_pixels,
                "fractions": len(fractions),
                "transparent_ci4_colors": sum(
                    1 for color in artifacts.corpus.palette if (color >> 15) == 0
                ),
            },
            "ds_contract": {
                "rgb256_bits_per_texel": 8,
                "rgb256_palette_entries": PAIR_PALETTE_ENTRIES,
                "rgb256_transparent_indices": RGB256_TRANSPARENT_INDICES,
                "cut_g_texture_vram_bytes": CUT_G_TEXTURE_VRAM_BYTES,
                "ds_max_texture_vram_bytes": DS_MAX_TEXTURE_VRAM_BYTES,
                "cut_g_texture_palette_vram_bytes": CUT_G_TEXTURE_PALETTE_VRAM_BYTES,
                "ds_max_texture_palette_vram_bytes": DS_MAX_TEXTURE_PALETTE_VRAM_BYTES,
            },
            "oracle": {
                "phase_lut_expansion_mismatches": exact_phase_mismatches,
                "canonical_rgb5a1_expansion_mismatches": canonical_mismatches,
                "rgb256_visible_semantic_mismatches": visible_mismatches,
                "mixed_alpha_keys": mixed_alpha_keys,
                "pair_fraction_entries": len(alpha_counts),
                "mixed_alpha_pair_fraction_entries": mixed_entries,
                "literal_pair_rgb5a1_per_key_minimum_mismatches": (
                    per_key_minimum_mismatches
                ),
                "literal_pair_rgb5a1_shared_fraction_minimum_mismatches": (
                    literal_minimum_mismatches
                ),
                "per_key_mixed_alpha_pair_entries": per_key_mixed_entries,
                "max_key_pair_alpha_classes": max_key_classes,
                "fraction_pair_alpha_classes_min": min(
                    map(len, classes_by_fraction.values())
                ),
                "fraction_pair_alpha_classes_max": max(
                    map(len, classes_by_fraction.values())
                ),
            },
            "owners": owner_summaries,
            "footprint": {
                "literal_pair_palettes_bytes": literal_palette_bytes,
                "exact_phase_luts_bytes": exact_phase_lut_bytes,
                "tight_canonical_palettes_bytes": tight_palette_bytes,
                "pair_row_archive_bytes": pair_row_archive_bytes,
                "pair_selection_bytes": pair_selection_bytes,
                "literal_pair_archive_bytes": literal_pair_archive_bytes,
                "exact_pair_archive_bytes": exact_pair_archive_bytes,
                "resident_pair_maps_bytes": resident_pair_maps,
                "resident_pair_maps_plus_palettes_bytes": resident_pair_with_palettes,
                "requested_32x32_resident_bytes": requested_crop_bytes + literal_palette_bytes,
                "requested_32x32_output_mismatch_pixels": requested_crop_mismatches,
                "natural_period_resident_bytes": (
                    natural_crop_bytes + literal_palette_bytes
                ),
                "natural_period_output_mismatch_pixels": natural_crop_mismatches,
                "canonical_rgb5a1_resident_bytes": (
                    canonical_resident_bytes + tight_palette_bytes
                ),
                "canonical_rgb5a1_archive_bytes": canonical_archive_bytes,
                "rgb256_visible_resident_bytes": visible_resident_bytes,
                "rgb256_visible_archive_bytes": visible_archive_bytes,
            },
            "update_model": {
                "pair_map_changes_large": owner_summaries["large"][  # type: ignore[index]
                    "map_changes_per_cycle"
                ],
                "pair_map_changes_small": owner_summaries["small"][  # type: ignore[index]
                    "map_changes_per_cycle"
                ],
                "palette_changes": palette_changes,
                "active_frames": sum(value != 0 for value in update_bytes),
                "bytes_per_cycle": sum(update_bytes),
                "average_bytes_per_frame_numerator": sum(update_bytes),
                "average_bytes_per_frame_denominator": aot.EXPECTED_CYCLE_FRAMES,
                "peak_bytes_per_frame": max(update_bytes),
                "rgb256_visible_bytes_per_cycle": visible_update_bytes,
                "rgb256_visible_average_bytes_per_frame_numerator": (
                    visible_update_bytes
                ),
                "rgb256_visible_average_bytes_per_frame_denominator": (
                    aot.EXPECTED_CYCLE_FRAMES
                ),
                "bytes_per_frame_histogram": {
                    str(value): count for value, count in sorted(Counter(update_bytes).items())
                },
            },
            "verdict": {
                "literal_pair_rgb5a1": "NO_GO_ALPHA_PHASE",
                "periodic_32x32": "NO_GO_CLAMP_MIRROR",
                "resident_pair_maps": "NO_GO_TEXTURE_VRAM",
                "zero_io_archive": "NO_GO_CURRENT_RAM_RESERVE",
                "rgb256_visible_variant": "NO_GO_RESIDENCY",
            },
        }
    finally:
        _ACTIVE_PALETTE = None


def _load_fixture(path: Path) -> dict[str, object]:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="checkout containing the pinned BattleShip O2R corpus",
    )
    parser.add_argument(
        "--fixture",
        type=Path,
        default=DEFAULT_FIXTURE,
        help="pinned indexed-feasibility result",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        summary = analyze(aot.generate(args.repo_root.resolve()))
        expected = _load_fixture(args.fixture.resolve())
        if summary != expected:
            raise aot.falsify("indexed feasibility differs from its pinned fixture")
    except (aot.Falsifier, OSError, ValueError) as exc:
        print(f"PUPUPU_WATER_INDEXED_FAIL: {exc}")
        return 1
    footprint = summary["footprint"]
    oracle = summary["oracle"]
    print(
        "PUPUPU_WATER_INDEXED_OK "
        f"oracle_pixels={summary['source']['oracle_pixels']} "  # type: ignore[index]
        f"exact_mismatches={oracle['phase_lut_expansion_mismatches']} "  # type: ignore[index]
        "literal_mismatches="
        f"{oracle['literal_pair_rgb5a1_per_key_minimum_mismatches']} "  # type: ignore[index]
        f"archive_bytes={footprint['exact_pair_archive_bytes']} "  # type: ignore[index]
        f"resident_bytes={footprint['resident_pair_maps_plus_palettes_bytes']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

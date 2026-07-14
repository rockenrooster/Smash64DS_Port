#!/usr/bin/env python3
"""Generate the no-GX native-fighter lighting/t16 parity corpus.

The corpus is a necessary-condition test for two proposed GX ownership cuts.
It deliberately does not submit GX commands or change the runtime renderer:

* RGB15 covers every canonical (binding, signed normal) pair under the
  source-backed fighter light seed and an identity-direction probe.
* t16 covers every immutable ST pair used by the nine textured native-owner
  epochs, with the exact source display-list scale/origin/filter transform.

The generated header is consumed by one freestanding C kernel compiled for
both the host and ARM9.  A single RGB15 mismatch demotes direct hardware-light
ownership.  A single synthesized t16 mismatch demotes texture-matrix ownership.
"""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

import generate_nds_native_owners as native


RGB_DIRECTION = (0, 0, 127)
RGB_DIFFUSE = 0xFF
RGB_AMBIENT = 0x4C
DS_V10_SCALE = 4
DS_REFERENCE_MATERIAL = (31, 10, 31)  # light, ambient, diffuse
T16_FILTER_OFFSET = 16
S16_MIN = -(1 << 15)
S16_MAX = (1 << 15) - 1


def signed_byte(value: int) -> int:
    return value - 256 if value >= 128 else value


def source_rgb5(normal: tuple[int, int, int]) -> int:
    dot = sum(component * direction
              for component, direction in zip(normal, RGB_DIRECTION))
    if dot <= 0:
        diffuse_numer = 0
    elif dot > 127 * 127:
        diffuse_numer = 127
    else:
        diffuse_numer = dot // 127
    shade = min(255, RGB_AMBIENT + (RGB_DIFFUSE * diffuse_numer) // 127)
    return shade >> 3


def ds_rgb5(normal: tuple[int, int, int], material: tuple[int, int, int]) -> int:
    """Model the documented DS one-light diffuse/ambient integer pipeline.

    Source s8 vectors are mapped losslessly to signed v10 with x4.  The light
    command uses the opposite vector because GX negates it after applying the
    directional matrix.  The identity probe therefore leaves a +508 internal
    Z direction.  Each dot-product lane discards nine bits before addition,
    matching the geometry engine order.
    """

    light_color, ambient_material, diffuse_material = material
    internal_light = tuple(direction * DS_V10_SCALE
                           for direction in RGB_DIRECTION)
    packed_normal = tuple(component * DS_V10_SCALE for component in normal)
    dot = sum((light * component) >> 9
              for light, component in zip(internal_light, packed_normal))
    vertex = (ambient_material << 9) * light_color
    if dot > 0:
        diffdot = ((dot + (1 << 10)) & 0x7FF) - (1 << 10)
        vertex += (diffuse_material * light_color * diffdot) & 0xFFFFF
    return min(31, vertex >> 14)


def pack_rgb15(channel: int) -> int:
    return channel | (channel << 5) | (channel << 10)


def optimize_ds_materials(rgb_cases: list[tuple[int, int, int, int]]):
    by_binding: dict[int, Counter[int]] = {}
    for binding, _, _, nz in rgb_cases:
        by_binding.setdefault(binding, Counter())[nz] += 1

    binding_count = max(by_binding) + 1
    source_by_nz = {
        nz: source_rgb5((0, 0, nz))
        for counts in by_binding.values() for nz in counts
    }
    dot_by_nz = {
        nz: ((RGB_DIRECTION[2] * DS_V10_SCALE) *
             (nz * DS_V10_SCALE)) >> 9
        for nz in source_by_nz
    }
    bindings_by_nz: dict[int, list[tuple[int, int]]] = {
        nz: [] for nz in source_by_nz
    }
    for binding, counts in by_binding.items():
        for nz, count in counts.items():
            bindings_by_nz[nz].append((binding, count))

    best = [None] * binding_count
    ref_light, ref_ambient, ref_diffuse = DS_REFERENCE_MATERIAL
    for light_color in range(32):
        for ambient_material in range(32):
            base = (ambient_material << 9) * light_color
            for diffuse_material in range(32):
                slope = diffuse_material * light_color
                mismatches = [0] * binding_count
                for nz, source in source_by_nz.items():
                    dot = dot_by_nz[nz]
                    vertex = base
                    if dot > 0:
                        vertex += (slope * dot) & 0xFFFFF
                    device = min(31, vertex >> 14)
                    if device != source:
                        for binding, count in bindings_by_nz[nz]:
                            mismatches[binding] += count
                distance = (
                    abs(light_color - ref_light) +
                    abs(ambient_material - ref_ambient) +
                    abs(diffuse_material - ref_diffuse)
                )
                material = (
                    light_color, ambient_material, diffuse_material,
                )
                for binding, mismatch in enumerate(mismatches):
                    score = (
                        mismatch, distance, -light_color,
                        ambient_material, -diffuse_material,
                    )
                    if best[binding] is None or score < best[binding][0]:
                        best[binding] = (score, material)
    if any(item is None for item in best):
        raise ValueError("failed to optimize every canonical binding")
    chosen = [item[1] for item in best]
    binding_mismatches = [item[0][0] for item in best]
    return chosen, binding_mismatches


def apply_othermode_h(current: int, w0: int, w1: int) -> int:
    opcode = w0 >> 24
    if opcode == 0xEF:  # RDPSETOTHERMODE
        return w0 & 0x00FFFFFF
    if opcode != 0xE3:  # SETOTHERMODE_H
        return current
    bits = (w0 & 0xFF) + 1
    position = (w0 >> 8) & 0xFF
    if bits > 32 or position >= 32 or bits + position > 32:
        raise ValueError("invalid canonical SETOTHERMODE_H span")
    shift = 32 - position - bits
    mask = 0xFFFFFFFF if bits == 32 else ((1 << bits) - 1) << shift
    return (current & ~mask) | (w1 & mask)


def epoch_texture_transform(epoch_index, epoch, state, sequence):
    textures = []
    tile_sizes = []
    othermode_h = 0
    spans = ((epoch[0], epoch[4]), (epoch[1], epoch[5]))
    for first, count in spans:
        for sequence_index in range(first, first + count):
            w0, w1, effect = state[sequence[sequence_index]]
            if effect == 2:
                othermode_h = apply_othermode_h(othermode_h, w0, w1)
            elif effect == 4:
                textures.append(((w1 >> 16) & 0xFFFF, w1 & 0xFFFF))
            elif effect == 10 and ((w1 >> 24) & 7) == 0:
                tile_sizes.append(((w0 >> 12) & 0xFFF, w0 & 0xFFF))
    if len(textures) != 1 or len(tile_sizes) != 1:
        raise ValueError(
            f"textured epoch {epoch_index}: expected one immutable texture "
            f"and render-tile origin, got {len(textures)}/{len(tile_sizes)}"
        )
    scale_s, scale_t = textures[0]
    origin_s, origin_t = tile_sizes[0]
    offset = T16_FILTER_OFFSET if ((othermode_h >> 12) & 3) != 0 else 0
    return scale_s, scale_t, origin_s, origin_t, offset


def source_t16(coord: int, scale: int, origin: int, offset: int) -> int:
    return (coord * scale >> 17) - (origin << 2) + offset


def matrix_interval(
        coords: list[tuple[int, int]], targets: list[int], coefficient: int,
        axis: int):
    low = -(1 << 63)
    high = (1 << 63) - 1
    for coord, target in zip(coords, targets):
        source = coord[axis]
        product = source * coefficient
        low = max(low, (target << 12) - product)
        high = min(high, ((target + 1) << 12) - 1 - product)
        if low > high:
            return None
    return low, high


def synthesize_axis(
        coords: list[tuple[int, int]], targets: list[int], scale: int,
        translation: int, axis: int):
    floor_coefficient = scale >> 5
    coefficients = [floor_coefficient]
    ceil_coefficient = (scale + 31) >> 5
    if ceil_coefficient != floor_coefficient:
        coefficients.append(ceil_coefficient)
    candidates = []
    ideal_bias = translation << 12
    for coefficient in coefficients:
        interval = matrix_interval(coords, targets, coefficient, axis)
        if interval is None:
            continue
        low, high = interval
        bias = min(max(ideal_bias, low), high)
        score = (abs(coefficient * 32 - scale), abs(bias - ideal_bias),
                 coefficient, bias)
        candidates.append((score, coefficient, bias))
    if candidates:
        _, coefficient, bias = min(candidates)
        return coefficient, bias, True
    return floor_coefficient, ideal_bias, False


def ds_t16(coord: int, coefficient: int, bias: int) -> int:
    return (coord * coefficient + bias) >> 12


def canonical_data(source_root: Path):
    data = native.decode_export()
    state = native.unpack_many("<IIB3x", data["state"])
    sequence = list(data["sequence"])
    vertex = native.unpack_many("<BBBBIhh", data["vertex"])
    triangles = [item[0] for item in native.unpack_many("<H", data["triangles"])]
    runs = native.unpack_many("<HBBI", data["runs"])
    epochs = native.unpack_many("<HHHHBBBBBBBB", data["epochs"])
    mario_roots = native.unpack_many("<IHHHBBBB2x", data["mario_roots"])
    fox_roots = native.unpack_many("<IHHHBBBB2x", data["fox_roots"])
    owners = (("mario", mario_roots), ("fox", fox_roots))
    (
        dense_vertices,
        _,
        _,
        dense_corners,
        _,
        run_first_corner,
        _,
        _,
        _,
    ) = native.build_dense_geometry(
        vertex, triangles, runs, epochs, owners, source_root,
    )
    if len(dense_vertices) != 541:
        raise ValueError("canonical dense fighter vertex count changed")

    rgb_cases = sorted({
        (
            dense[5],
            signed_byte((dense[7] >> 24) & 0xFF),
            signed_byte((dense[7] >> 16) & 0xFF),
            signed_byte((dense[7] >> 8) & 0xFF),
        )
        for dense in dense_vertices
    })
    unique_normals = {case[1:] for case in rgb_cases}
    if len(rgb_cases) != 413 or len(unique_normals) != 411:
        raise ValueError(
            "canonical normal corpus changed: "
            f"{len(rgb_cases)} binding-normal / {len(unique_normals)} normal"
        )

    materials, binding_mismatches = optimize_ds_materials(rgb_cases)
    rgb_mismatches = sum(
        source_rgb5(case[1:]) != ds_rgb5(case[1:], materials[case[0]])
        for case in rgb_cases
    )
    reference_mismatches = sum(
        source_rgb5(case[1:]) != ds_rgb5(case[1:], DS_REFERENCE_MATERIAL)
        for case in rgb_cases
    )
    if rgb_mismatches != sum(binding_mismatches):
        raise ValueError("binding RGB15 mismatch census is not additive")

    transforms = []
    t16_cases = []
    naive_mismatches = 0
    synthesized_mismatches = 0
    textured_runs = 0
    textured_corners = 0
    textured_dense_ids = set()
    for epoch_index in sorted(native.DIRECT_POLICY_TEXTURED_EPOCHS):
        epoch = epochs[epoch_index]
        scale_s, scale_t, origin_s, origin_t, offset = (
            epoch_texture_transform(epoch_index, epoch, state, sequence)
        )
        dense_ids = []
        for run_index in range(epoch[3], epoch[3] + epoch[9]):
            corner_count = runs[run_index][1] * 3
            first_corner = run_first_corner[run_index]
            dense_ids.extend(
                dense_corners[first_corner:first_corner + corner_count]
            )
            textured_runs += 1
            textured_corners += corner_count
        textured_dense_ids.update(dense_ids)
        coords = sorted({
            (dense_vertices[dense_id][3], dense_vertices[dense_id][4])
            for dense_id in dense_ids
        })
        targets_s = [
            source_t16(s, scale_s, origin_s, offset) for s, _ in coords
        ]
        targets_t = [
            source_t16(t, scale_t, origin_t, offset) for _, t in coords
        ]
        if any(value < S16_MIN or value > S16_MAX
               for value in targets_s + targets_t):
            raise ValueError(f"textured epoch {epoch_index}: t16 output overflow")
        translation_s = -(origin_s << 2) + offset
        translation_t = -(origin_t << 2) + offset
        matrix_s, bias_s, exact_s = synthesize_axis(
            coords, targets_s, scale_s, translation_s, 0,
        )
        matrix_t, bias_t, exact_t = synthesize_axis(
            coords, targets_t, scale_t, translation_t, 1,
        )
        transform_index = len(transforms)
        transforms.append((
            scale_s, scale_t, origin_s, origin_t, offset, epoch_index,
            matrix_s, matrix_t, bias_s, bias_t,
            int(exact_s and exact_t),
        ))
        for (s, t), expected_s, expected_t in zip(
                coords, targets_s, targets_t):
            naive_s = ds_t16(
                s, scale_s >> 5, translation_s << 12,
            )
            naive_t = ds_t16(
                t, scale_t >> 5, translation_t << 12,
            )
            actual_s = ds_t16(s, matrix_s, bias_s)
            actual_t = ds_t16(t, matrix_t, bias_t)
            naive_mismatches += (naive_s, naive_t) != (expected_s, expected_t)
            synthesized_mismatches += (
                (actual_s, actual_t) != (expected_s, expected_t)
            )
            t16_cases.append((s, t, transform_index))

    if (len(native.DIRECT_POLICY_TEXTURED_EPOCHS), textured_runs,
            textured_corners, len(textured_dense_ids), len(t16_cases)) != (
            9, 15, 381, 106, 99):
        raise ValueError(
            "canonical textured corpus changed: "
            f"{len(native.DIRECT_POLICY_TEXTURED_EPOCHS)} epochs / "
            f"{textured_runs} runs / {textured_corners} corners / "
            f"{len(textured_dense_ids)} dense / {len(t16_cases)} cases"
        )

    return {
        "rgb_cases": rgb_cases,
        "rgb_unique_normals": len(unique_normals),
        "materials": materials,
        "binding_mismatches": binding_mismatches,
        "rgb_mismatches": rgb_mismatches,
        "reference_mismatches": reference_mismatches,
        "transforms": transforms,
        "t16_cases": t16_cases,
        "naive_mismatches": naive_mismatches,
        "synthesized_mismatches": synthesized_mismatches,
        "textured_runs": textured_runs,
        "textured_corners": textured_corners,
        "textured_dense_ids": len(textured_dense_ids),
    }


def emit_rows(type_name: str, name: str, rows: list[str]) -> list[str]:
    lines = [f"static const {type_name} {name}[{len(rows)}] =", "{"]
    lines.extend(f"    {row}," for row in rows)
    lines.extend(("};", ""))
    return lines


def generate(source_root: Path):
    corpus = canonical_data(source_root)
    digest_payload = json.dumps(
        corpus, sort_keys=True, separators=(",", ":"),
    ).encode("ascii")
    digest = hashlib.sha256(digest_payload).hexdigest()
    rgb_demoted = int(corpus["rgb_mismatches"] != 0)
    t16_demoted = int(corpus["synthesized_mismatches"] != 0)

    lines = [
        "/* Generated by scripts/generate_renderer_parity_corpus.py. */",
        "/* No GX submission; canonical native-owner necessary-condition corpus. */",
        "/* RGB uses each binding's exhaustive best one-light 5-bit material. */",
        "/* t16 uses generated floor/ceil 20.12 coefficients and fractional bias. */",
        f"/* Contract SHA256: {digest}. */",
        "#ifndef SMASH64DS_RENDERER_PARITY_CORPUS_GENERATED_H",
        "#define SMASH64DS_RENDERER_PARITY_CORPUS_GENERATED_H",
        "",
        "#include <stdint.h>",
        "",
        "typedef struct RendererParityRgbCase",
        "{",
        "    uint8_t binding;",
        "    int8_t nx;",
        "    int8_t ny;",
        "    int8_t nz;",
        "} RendererParityRgbCase;",
        "",
        "typedef struct RendererParityDsMaterial",
        "{",
        "    uint8_t light_color;",
        "    uint8_t ambient_material;",
        "    uint8_t diffuse_material;",
        "} RendererParityDsMaterial;",
        "",
        "typedef struct RendererParityT16Transform",
        "{",
        "    uint16_t scale_s;",
        "    uint16_t scale_t;",
        "    uint16_t origin_s;",
        "    uint16_t origin_t;",
        "    int16_t offset;",
        "    uint8_t epoch;",
        "    uint8_t exact_synthesis;",
        "    int32_t matrix_s;",
        "    int32_t matrix_t;",
        "    int32_t bias_s;",
        "    int32_t bias_t;",
        "} RendererParityT16Transform;",
        "",
        "typedef struct RendererParityT16Case",
        "{",
        "    int16_t s;",
        "    int16_t t;",
        "    uint8_t transform_index;",
        "    uint8_t reserved;",
        "} RendererParityT16Case;",
        "",
        f'#define RENDERER_PARITY_CORPUS_SHA256 "{digest}"',
        f'#define RENDERER_PARITY_RGB_CASE_COUNT {len(corpus["rgb_cases"])}u',
        f'#define RENDERER_PARITY_RGB_UNIQUE_NORMAL_COUNT {corpus["rgb_unique_normals"]}u',
        f'#define RENDERER_PARITY_RGB_BINDING_COUNT {len(corpus["materials"])}u',
        f'#define RENDERER_PARITY_RGB_EXPECTED_MISMATCH_COUNT {corpus["rgb_mismatches"]}u',
        f'#define RENDERER_PARITY_RGB_REFERENCE_MISMATCH_COUNT {corpus["reference_mismatches"]}u',
        f'#define RENDERER_PARITY_RGB_DEMOTE_HARDWARE_LIGHT {rgb_demoted}u',
        f'#define RENDERER_PARITY_T16_TRANSFORM_COUNT {len(corpus["transforms"])}u',
        f'#define RENDERER_PARITY_T16_CASE_COUNT {len(corpus["t16_cases"])}u',
        f'#define RENDERER_PARITY_T16_NAIVE_MISMATCH_COUNT {corpus["naive_mismatches"]}u',
        f'#define RENDERER_PARITY_T16_EXPECTED_MISMATCH_COUNT {corpus["synthesized_mismatches"]}u',
        f'#define RENDERER_PARITY_T16_DEMOTE_TEXTURE_MATRIX {t16_demoted}u',
        "",
    ]
    lines += emit_rows(
        "RendererParityRgbCase", "gRendererParityRgbCases",
        [f"{{ {binding}u, {nx}, {ny}, {nz} }}"
         for binding, nx, ny, nz in corpus["rgb_cases"]],
    )
    lines += emit_rows(
        "RendererParityDsMaterial", "gRendererParityDsMaterials",
        [f"{{ {light}u, {ambient}u, {diffuse}u }}"
         for light, ambient, diffuse in corpus["materials"]],
    )
    lines += emit_rows(
        "uint8_t", "gRendererParityBindingMismatchCounts",
        [f"{count}u" for count in corpus["binding_mismatches"]],
    )
    transform_rows = []
    for transform in corpus["transforms"]:
        (scale_s, scale_t, origin_s, origin_t, offset, epoch,
         matrix_s, matrix_t, bias_s, bias_t, exact) = transform
        transform_rows.append(
            "{{ {}u, {}u, {}u, {}u, {}, {}u, {}u, {}, {}, {}, {} }}".format(
                scale_s, scale_t, origin_s, origin_t, offset, epoch, exact,
                matrix_s, matrix_t, bias_s, bias_t,
            )
        )
    lines += emit_rows(
        "RendererParityT16Transform", "gRendererParityT16Transforms",
        transform_rows,
    )
    lines += emit_rows(
        "RendererParityT16Case", "gRendererParityT16Cases",
        [f"{{ {s}, {t}, {transform}u, 0u }}"
         for s, t, transform in corpus["t16_cases"]],
    )
    lines.extend((
        "#endif /* SMASH64DS_RENDERER_PARITY_CORPUS_GENERATED_H */",
        "",
    ))
    return "\n".join(lines), corpus, digest


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--source-root", type=Path, default=repo_root,
        help="repo root containing the read-only BattleShip O2R inputs",
    )
    parser.add_argument(
        "--output", type=Path,
        default=Path(__file__).with_name(
            "renderer_parity_corpus.generated.h"
        ),
    )
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    generated, corpus, digest = generate(args.source_root.resolve())
    if args.check:
        if not args.output.is_file() or args.output.read_text() != generated:
            raise SystemExit(f"stale renderer parity corpus: {args.output}")
    else:
        if not args.output.is_file() or args.output.read_text() != generated:
            args.output.write_text(generated)
    print(
        "renderer parity corpus: "
        f"rgb={len(corpus['rgb_cases'])}/mismatch={corpus['rgb_mismatches']}, "
        f"t16={len(corpus['t16_cases'])}/mismatch="
        f"{corpus['synthesized_mismatches']}/naive="
        f"{corpus['naive_mismatches']}, sha256={digest[:16]}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

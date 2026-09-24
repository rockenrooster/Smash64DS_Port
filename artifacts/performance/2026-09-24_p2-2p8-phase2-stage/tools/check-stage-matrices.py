"""Compare captured GX transforms with the old composed-matrix path in DS pixels.

Integer GX operation order: melonDS GPU3D.cpp MatrixMult4x4,
UpdateClipMatrix and SubmitVertex (upstream source inspected 2026-09-24):
https://github.com/melonDS-emu/melonDS/blob/master/src/GPU3D.cpp
This checks the captured inputs, not every camera state or rasterizer behavior.
"""
import argparse
from fractions import Fraction
import json
import math
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / 'scripts/stages'))
import compile_nds_stage_gx as gx


def s32(value):
    return (value + 0x80000000) % 0x100000000 - 0x80000000


def multiply(a, b):
    return [s32(sum(a[r*4+k] * b[k*4+c] for k in range(4)) >> 12)
            for r in range(4) for c in range(4)]


def project(v, m):
    clip = [s32(sum(v[k] * m[k*4+c] for k in range(4)) >> 12) for c in range(4)]
    assert clip[3] > 0, 'Captured corner needs the near clipper'
    return (clip[0] / clip[3] * 128, clip[1] / clip[3] * 96)


def clip_mask(v, m):
    x, y, z, w = [s32(sum(v[k] * m[k*4+c] for k in range(4)) >> 12) for c in range(4)]
    return sum(1 << i for i, distance in enumerate((w+x, w-x, w+y, w-y, w+z, w-z, w)) if distance < 0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('prefix', type=Path)
    parser.add_argument('--limit', type=float, default=1.0)
    parser.add_argument('--template', type=Path, default=ROOT / 'builds/build-p2p8-s7/nitrofs/stages/dreamland.gxp')
    args = parser.parse_args()
    prefix = str(args.prefix)
    _, runs, patches, _ = gx.decode(args.template.read_bytes())
    packet = gx.stage.generate(ROOT, 'dreamland')
    data = Path(prefix + '-new.bin').read_bytes()
    words = struct.unpack(f'<{len(data)//4}I', data)
    composed = struct.unpack('<672i', Path(prefix + '-composed.bin').read_bytes())
    rigid, = struct.unpack('<Q', Path(prefix + '-rigid.bin').read_bytes())
    hidden_path = Path(prefix + '-hidden.bin')
    hidden = struct.unpack('<Q', hidden_path.read_bytes())[0] if hidden_path.exists() else 0
    errors = []
    clip_changes, cull_changes = [], []
    noz_triangles = corner_triangles = 0
    for index, run in enumerate(runs):
        first, size = run[:2]
        native = packet.runs[index]
        if not size or hidden & (1 << native.binding_index):
            continue
        depths = []
        for offset, kind, _, aux in patches[run[2]:run[2]+run[3]]:
            if kind not in (gx.NOZ, gx.COMPOSED_NOZ, gx.CORNER_NOZ):
                continue
            matrix = list(map(s32, words[offset:offset+16]))
            row = max(range(4), key=lambda i: abs(matrix[i*4+3]))
            assert matrix[row*4+3] != 0
            guess = round(Fraction(matrix[row*4+2] * 4096, matrix[row*4+3]))
            candidates = [d for d in range(guess-1, guess+2) if all(
                matrix[i*4+2] == s32(gx.stage.round_shift_signed(matrix[i*4+3]*d, 12))
                for i in range(4))]
            assert len(candidates) == 1, (index, 'ambiguous painter depth', candidates)
            triangle = aux >> 3 if kind == gx.CORNER_NOZ else len(depths)
            if triangle == len(depths):
                depths.append(set())
            depths[triangle].add(candidates[0])
        if native.submit_class == gx.stage.SUBMIT_PROJECTED_NO_Z:
            assert len(depths) == native.triangle_count and all(len(d) == 1 for d in depths)
            values = [next(iter(d)) for d in depths]
            assert all(b == a-1 for a, b in zip(values, values[1:])), (index, 'depth sequence')
            noz_triangles += len(depths)
            if native.flags & gx.stage.RUN_FLAG_PROJECTED_CROSS_MATRIX:
                corner_triangles += len(depths)
        mode, stack, corner = 2, [], 0
        view = projection = None
        triangle_xy = []
        for op, params in gx.commands(words[first:first+size]):
            if op == 0x10:
                mode = params[0]
            elif op == 0x15:
                assert mode == 2
                view = [4096 if i % 5 == 0 else 0 for i in range(16)]
            elif op == 0x16:
                if mode == 0:
                    projection = list(map(s32, params))
                else:
                    view = list(map(s32, params))
            elif op == 0x11:
                stack.append(view[:])
            elif op == 0x12:
                assert params[0] == 1
                view = stack.pop()
            elif op == 0x18:
                view = multiply(list(map(s32, params)), view)
            elif op == 0x23:
                dense = packet.vertices[packet.corners[native.first_corner + corner]]
                if rigid & (1 << dense.matrix_binding):
                    corner += 1
                    continue  # the old rigid path already uses GX composition
                xyz = [struct.unpack('<h', struct.pack('<H', n))[0] for n in
                       (params[0] & 65535, params[0] >> 16, params[1] & 65535)] + [4096]
                actual = project(xyz, multiply(view, projection))
                start = native.first_corner + (corner // 3) * 3
                shift = gx.run_shift(packet, native, packet.corners[start:start+3])
                if native.flags & gx.stage.RUN_FLAG_PROJECTED_CROSS_MATRIX and len({
                    packet.vertices[i].matrix_binding for i in packet.corners[start:start+3]}) != 1:
                    shift = gx.stage.stage_vertex_coordinate_shift(dense)
                base = dense.matrix_binding * 16
                reference = list(composed[base:base+16])
                reference[:12] = [n << shift for n in reference[:12]]
                reference[12:] = [gx.stage.round_shift_signed(n, 8) for n in reference[12:]]
                try:
                    expected = project(xyz, reference)
                except AssertionError as exc:
                    raise AssertionError((index, native.binding_index, corner, xyz, reference)) from exc
                if native.submit_class != gx.stage.SUBMIT_PROJECTED_NO_Z:
                    old_mask, new_mask = clip_mask(xyz, reference), clip_mask(xyz, multiply(view, projection))
                    if old_mask != new_mask:
                        clip_changes.append(dict(run=index, corner=corner, old=old_mask, new=new_mask))
                    triangle_xy.append((expected, actual))
                    if len(triangle_xy) == 3:
                        areas = []
                        for side in (0, 1):
                            a, b, c = [v[side] for v in triangle_xy]
                            areas.append((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]))
                        if (areas[0] > 0) != (areas[1] > 0) or (areas[0] < 0) != (areas[1] < 0):
                            cull_changes.append(dict(run=index, triangle=corner//3, areas=areas))
                        triangle_xy = []
                error = math.hypot(actual[0]-expected[0], actual[1]-expected[1])
                errors.append(dict(run=index, corner=corner, pixels=error))
                corner += 1
        assert corner == native.triangle_count * 3 and not stack
    assert errors
    result = dict(corners=len(errors), max_pixels=max(e['pixels'] for e in errors),
                  noz_triangles=noz_triangles, corner_program_triangles=corner_triangles,
                  clip_changes=clip_changes, cull_changes=cull_changes,
                  limit=args.limit, worst=sorted(errors, key=lambda e: e['pixels'], reverse=True)[:10])
    Path(prefix + '-comparison.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
    assert result['max_pixels'] <= args.limit


if __name__ == '__main__':
    main()

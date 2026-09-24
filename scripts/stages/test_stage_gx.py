"""Decode generated GX and compare every submitted corner to native source tables."""
from dataclasses import replace
import struct

import pytest

import compile_nds_stage_gx as gx


def test_compiled_corners_patch_coverage_and_stack():
    packet = gx.stage.generate(gx.stage._paths.REPO_ROOT, 'dreamland')
    blob = gx.compile_packet(packet)
    header, runs, patches, words = gx.decode(blob)
    assert header[6] == 0xA0 and header[7] == gx.signature(packet)
    triangles = 0
    for index, (first, count, pf, pc, nt, segment) in enumerate(runs):
        if not count:
            continue
        assert segment in (5, 7)
        run = packet.runs[index]
        assert nt == run.triangle_count
        scope = patches[pf:pf + pc]
        assert sum(p[1] == gx.VIEW for p in scope) == 1
        assert sum(p[1] == gx.NOZ for p in scope) == nt
        assert sum(p[1] == gx.COLOR for p in scope) == nt * 3
        assert sum(p[1] == gx.UV for p in scope) == nt * 3
        assert all(first <= p[0] and p[0] + (16 if p[1] <= gx.NOZ else 1) <= first + count for p in scope)
        actual, depth = [], 0
        for op, args in gx.commands(words[first:first + count]):
            if op == 0x11:
                depth += 1
            elif op == 0x12:
                depth -= args[0]
                assert depth >= 0
            elif op == 0x23:
                assert depth == 1
                actual.append(tuple(struct.unpack('<h', struct.pack('<H', n))[0]
                                    for n in (args[0] & 65535, args[0] >> 16, args[1] & 65535)))
        expected = []
        for t in range(nt):
            vertices = [packet.vertices[i] for i in packet.corners[run.first_corner + 3*t:run.first_corner + 3*t + 3]]
            shift = max(gx.stage.stage_vertex_coordinate_shift(v) for v in vertices)
            for v in vertices:
                # Independent nearest/away-from-zero arithmetic, not the compiler helper.
                expected.append(tuple(((-1 if n < 0 else 1) * ((abs(n) + (1 << (shift-1))) >> shift) if shift else n) * 16
                                      for n in (v.x, v.y, v.z)))
        assert actual == expected and depth == 0
        triangles += nt
    assert triangles == 45
    damaged = bytearray(blob)
    damaged[-1] ^= 1
    with pytest.raises(ValueError):
        gx.decode(damaged)
    bad = list(packet.runs)
    first = packet.segments[5].first_run
    bad[first] = replace(bad[first], flags=bad[first].flags | gx.stage.RUN_FLAG_PROJECTED_CROSS_MATRIX)
    with pytest.raises(ValueError):
        gx.compile_packet(replace(packet, runs=tuple(bad)))

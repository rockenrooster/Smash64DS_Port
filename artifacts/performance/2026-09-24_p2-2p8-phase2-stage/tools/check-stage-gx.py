"""Compare submitted corners/attributes to the live Task36 capture, per run."""
import argparse
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / 'scripts/stages'))
import compile_nds_stage_gx as gx


def read_words(path):
    data = path.read_bytes()
    return struct.unpack(f'<{len(data)//4}I', data)


def corners(words, textured):
    color = uv = None
    result = []
    for op, args in gx.commands(words):
        if op == 0x20:
            color = args[0] & 0x7FFF
        elif op == 0x22:
            uv = args[0]
        elif op == 0x23:
            assert color is not None and (not textured or uv is not None)
            result.append((args[0], args[1] & 0xFFFF, color, uv if textured else None))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('prefix', type=Path)
    parser.add_argument('--template', type=Path, default=ROOT / 'builds/build-p2p8-s7/nitrofs/stages/dreamland.gxp')
    args = parser.parse_args()
    prefix = str(args.prefix)
    log = Path(prefix + '.gdb.out').read_text()
    old = read_words(Path(prefix + '-old.bin'))
    new = read_words(Path(prefix + '-new.bin'))
    header, runs, _, _ = gx.decode(args.template.read_bytes())
    assert len(new) == header[4]
    metadata = {int(i): (int(f), int(n), int(t)) for i, f, n, t in
                re.findall(r'GX_RUN=(\d+),(\d+),(\d+),(\d+)', log)}
    gx.PARAMS.update({0x13: 1, 0x14: 1, 0x15: 0, 0x19: 12})
    count = vertices = 0
    for index, record in enumerate(runs):
        start, size, _, _, triangles, _ = record[:6]
        if not size:
            continue
        first, length, textured = metadata[index]
        assert length > 0, f'run {index}: no baseline capture'
        expected = corners(old[first:first+length], textured)
        actual = corners(new[start:start+size], textured)
        assert len(actual) == triangles * 3, f'run {index}: missing corners'
        assert actual == expected, f'run {index}: GX corner/attribute mismatch'
        vertices += len(actual)
        count += 1
    assert count and vertices
    result = dict(runs=count, corners=vertices, triangles=vertices//3,
                  corner_attribute_mismatches=0, matrices='separate live-camera visual gate')
    Path(prefix + '-comparison.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()

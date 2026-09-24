"""Compile native stage tables to GX templates; no N64 commands at runtime.

First admitted batch: Dream Land segments 5/7. Prepared colour/UV, world/view
matrices and painter depth are explicit patch sites, not captured frame state.
"""
import argparse
import struct
from pathlib import Path

import generate_nds_native_stage as stage

MAGIC = 0x31505847  # GXP1
VIEW, WORLD, NOZ, COLOR, UV = range(1, 6)
HEADER = struct.Struct('<12I')
RUN = struct.Struct('<6H')
PATCH = struct.Struct('<4H')
PARAMS = {0: 0, 0x10: 1, 0x11: 0, 0x12: 1, 0x16: 16, 0x18: 16,
          0x20: 1, 0x22: 1, 0x23: 2, 0x40: 1, 0x41: 0}


def signature(packet):
    values = [len(packet.vertices), len(packet.runs), len(packet.corners)]
    for v in packet.vertices:
        values.extend((v.x, v.y, v.z, v.matrix_binding,
                       stage.stage_vertex_coordinate_shift(v)))
    values.extend(packet.corners)
    return stage.fnv1a_u32(values)


def compile_packet(packet, name='dreamland'):
    if name != 'dreamland':
        raise ValueError('Only the proved Dream Land batch is admitted yet')
    words, patches = [], []
    runs = [(0, 0, 0, 0, 0, 0)] * len(packet.runs)
    for segment_id in (5, 7):
        segment = packet.segments[segment_id]
        for run_id in range(segment.first_run, segment.first_run + segment.run_count):
            run = packet.runs[run_id]
            if run.submit_class != stage.SUBMIT_PROJECTED_NO_Z or run.flags & stage.RUN_FLAG_PROJECTED_CROSS_MATRIX:
                raise ValueError('Compiled batch requires single-binding no-Z runs')
            first_word, first_patch = len(words), len(patches)
            lane, command = 0, 0

            def emit(op, *args):
                nonlocal lane, command
                assert len(args) == PARAMS[op]
                if lane == 0:
                    command = len(words)
                    words.append(0)
                words[command] |= op << (lane * 8)
                offset = len(words)
                words.extend(a & 0xFFFFFFFF for a in args)
                lane = (lane + 1) % 4
                return offset

            def patch(op, kind, index=0, aux=0):
                offset = emit(op, *([0] * PARAMS[op]))
                patches.append((offset, kind, index, aux))

            emit(0x10, 2)  # position/vector matrix
            patch(0x16, VIEW)
            last_shift = None
            for triangle in range(run.triangle_count):
                indices = packet.corners[run.first_corner + triangle * 3:run.first_corner + triangle * 3 + 3]
                vertices = [packet.vertices[i] for i in indices]
                if any(v.matrix_binding != run.binding_index for v in vertices):
                    raise ValueError('Foreign-binding corner needs a cross program')
                shift = max(stage.stage_vertex_coordinate_shift(v) for v in vertices)
                if shift != last_shift:
                    if last_shift is not None:
                        emit(0x12, 1)
                    emit(0x11)
                    patch(0x18, WORLD, run.binding_index, shift)
                    last_shift = shift
                if triangle == 0:
                    emit(0x40, 0)  # triangles
                emit(0x10, 0)
                patch(0x16, NOZ, triangle)
                emit(0x10, 2)
                for index, v in zip(indices, vertices):
                    patch(0x20, COLOR, index)
                    patch(0x22, UV, index)
                    xyz = [stage.round_shift_signed(x, shift) * 16 for x in (v.x, v.y, v.z)]
                    if not all(-32768 <= x <= 32767 for x in xyz):
                        raise ValueError('Vertex does not fit DS VTX16')
                    emit(0x23, (xyz[0] & 0xFFFF) | ((xyz[1] & 0xFFFF) << 16), xyz[2] & 0xFFFF)
            emit(0x41)
            emit(0x12, 1)
            runs[run_id] = (first_word, len(words) - first_word, first_patch,
                            len(patches) - first_patch, run.triangle_count, segment_id)
    if len(words) > 16384 or len(patches) > 4096:
        raise ValueError('GX template exceeds bounded runtime format')
    body = b''.join(RUN.pack(*r) for r in runs)
    body += b''.join(PATCH.pack(*p) for p in patches)
    body += struct.pack(f'<{len(words)}I', *words)
    return HEADER.pack(MAGIC, 1, stage.blob_gkind(name), len(runs), len(words),
                       len(patches), 0xA0, signature(packet), len(body),
                       stage.fnv1a_bytes(body), 0, 0) + body


def decode(blob):
    header = HEADER.unpack_from(blob)
    magic, version, _, nr, nw, np, _, _, nb, checksum, _, _ = header
    if magic != MAGIC or version != 1 or len(blob) != HEADER.size + nb:
        raise ValueError('Invalid GX header/length')
    if nb != nr * RUN.size + np * PATCH.size + nw * 4 or stage.fnv1a_bytes(blob[HEADER.size:]) != checksum:
        raise ValueError('Invalid GX body')
    pos = HEADER.size
    runs = [RUN.unpack_from(blob, pos + i * RUN.size) for i in range(nr)]
    pos += nr * RUN.size
    patches = [PATCH.unpack_from(blob, pos + i * PATCH.size) for i in range(np)]
    pos += np * PATCH.size
    words = struct.unpack_from(f'<{nw}I', blob, pos)
    return header, runs, patches, words


def commands(words):
    cursor = 0
    while cursor < len(words):
        ops = words[cursor]
        cursor += 1
        for lane in range(4):
            op = (ops >> (8 * lane)) & 255
            if op not in PARAMS or cursor + PARAMS[op] > len(words):
                raise ValueError('Malformed GX command span')
            yield op, words[cursor:cursor + PARAMS[op]]
            cursor += PARAMS[op]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    packet = stage.generate(stage._paths.REPO_ROOT, 'dreamland')
    blob = compile_packet(packet)
    decode(blob)
    if args.check:
        if args.output.read_bytes() != blob:
            raise SystemExit('Stale stage GX template')
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(blob)
    h = HEADER.unpack_from(blob)
    print(f'STAGE_GX_COMPILED bytes={len(blob)} words={h[4]} patches={h[5]} triangles=45')


if __name__ == '__main__':
    main()

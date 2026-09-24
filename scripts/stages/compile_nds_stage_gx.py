"""Compile native stage tables to GX templates; no N64 commands at runtime.

Dream Land's eight native segments. Constant worlds and known colour/UV are
baked; animated inputs, camera matrices and painter depth use patch sites.
"""
import argparse
import struct
from pathlib import Path

import generate_nds_native_stage as stage

MAGIC = 0x31505847  # GXP1
VIEW, WORLD, NOZ, COLOR, UV, PROJECTION, COMPOSED_NOZ, CORNER_NOZ = range(1, 9)
SEGMENTS = tuple(range(8))
HEADER = struct.Struct('<12I')
RUN_V1 = struct.Struct('<6H')
RUN_V2 = struct.Struct('<6H6h')
RUN = struct.Struct('<6H6h2I')
PATCH = struct.Struct('<4H')
PARAMS = {0: 0, 0x10: 1, 0x11: 0, 0x12: 1, 0x15: 0, 0x16: 16, 0x18: 16,
          0x20: 1, 0x22: 1, 0x23: 2, 0x40: 1, 0x41: 0}


def signature(packet):
    values = [len(packet.vertices), len(packet.runs), len(packet.corners)]
    for v in packet.vertices:
        values.extend((v.x, v.y, v.z, v.matrix_binding,
                       stage.stage_vertex_coordinate_shift(v)))
    values.extend(packet.corners)
    return stage.fnv1a_u32(values)


def world_words(packet, binding, shift):
    words = list(packet.baked_world_matrices[binding])
    words[:12] = [value << shift for value in words[:12]]
    words[12:15] = [stage.round_shift_signed(value, 8) for value in words[12:15]]
    if not all(-0x80000000 <= value <= 0x7FFFFFFF for value in words):
        raise ValueError('Baked world exceeds GX matrix range')
    return words


def run_shift(packet, run, triangle_indices):
    if run.submit_class == stage.SUBMIT_RAW_CURRENT:
        return 0
    indices = (packet.corners[run.first_corner:run.first_corner + run.triangle_count * 3]
               if run.submit_class == stage.SUBMIT_PROJECTED_RANGE_OR_MATRIX else triangle_indices)
    shift = max(stage.stage_vertex_coordinate_shift(packet.vertices[i]) for i in indices)
    if run.submit_class == stage.SUBMIT_PROJECTED_RANGE_OR_MATRIX and shift == 0:
        raise ValueError('Range run needs an explicit coordinate scale')
    return shift


def vertex_colour(policy, rgba):
    """Fold the vertex/white cases of the renderer's colour selector.

    Material-dependent colour keeps its live patch. Stage config starts from
    zero (renderer_adapter_stage.c), so fighter flash modulation is absent.
    """
    w0, w1 = policy.combine_w0, policy.combine_w1
    first = ((w0 >> 20) & 15, (w1 >> 28) & 15, (w0 >> 15) & 31, (w1 >> 15) & 7)
    second = ((w0 >> 5) & 15, (w1 >> 24) & 15, w0 & 31, (w1 >> 6) & 7)
    shade = 4 in first + second
    lit = bool(policy.geometry_mode & 0x20000) and shade
    if lit:
        if first[1] == 15 and first[3] == 7 and (first[0], first[2]) in ((3, 4), (4, 3)):
            return None
    else:
        used = first[2:] if policy.othermode_h & (3 << 20) != 1 << 20 else second[2:]
        if policy.othermode_h & (3 << 20) == 1 << 20 and 0 in used:
            used += first[2:]
        if 3 in used or 5 in used:
            return None
    if not (lit or shade or not (w0 or w1)):
        return 0x7FFF
    return ((rgba >> 27) & 31) | (((rgba >> 19) & 31) << 5) | (((rgba >> 11) & 31) << 10)


def vertex_attributes(packet):
    """Resolve first-use colour/UV inputs from the native state spans.

    Only explicit texture-on state is baked here. Material-driven UVs and
    implicit-texture cases retain their producer's live prepared values.
    """
    result = {}
    for segment in packet.segments:
        tile, scales, enabled = 0, (0, 0), False
        origins = [(0, 0)] * 8

        def apply(span):
            nonlocal tile, scales, enabled, origins
            for pos in range(span.first_state, span.first_state + span.state_count):
                delta = packet.state_deltas[packet.state_sequence[pos]]
                op = delta.w0 >> 24
                if delta.material_event != stage.INVALID_U8:
                    if op == stage.OP_TEXTURE:
                        enabled = False
                    elif op == stage.OP_SETTILESIZE:
                        origins = [None] * 8
                elif op == stage.OP_TEXTURE:
                    tile = (delta.w0 >> 8) & 7
                    scales = (delta.w1 >> 16, delta.w1 & 65535)
                    enabled = bool((delta.w0 >> 1) & 127)
                elif op == stage.OP_SETTILESIZE:
                    origins[(delta.w1 >> 24) & 7] = ((delta.w0 >> 12) & 4095, delta.w0 & 4095)

        for binding_index in range(segment.first_binding, segment.first_binding + segment.binding_count):
            binding = packet.bindings[binding_index]
            for index in range(binding.first_run, binding.first_run + binding.run_count):
                run = packet.runs[index]
                apply(packet.state_spans[index])
                policy = packet.policies[run.state_policy]
                epoch = packet.epochs[run.texture_epoch]
                for dense in packet.corners[run.first_corner:run.first_corner + run.triangle_count * 3]:
                    if dense in result:
                        continue
                    v = packet.vertices[dense]
                    uv = None
                    if (enabled and origins[tile] is not None and
                        epoch.material_event == stage.INVALID_U8 and texture_used(policy)):
                        offset = 16 if policy.othermode_h & (3 << 12) else 0
                        s = ((v.s * scales[0]) >> 17) - origins[tile][0] * 4 + offset
                        t = ((v.t * scales[1]) >> 17) - origins[tile][1] * 4 + offset
                        uv = (s & 65535) | ((t & 65535) << 16)
                    result[dense] = (vertex_colour(policy, v.rgba), uv)
            apply(packet.state_spans[len(packet.runs) + binding_index])
    return result


def texture_used(policy):
    """Texture-on is checked by the caller; mirror the renderer's texel test."""
    w0, w1 = policy.combine_w0, policy.combine_w1
    color = ((w0 >> 20) & 15, (w1 >> 28) & 15, (w0 >> 15) & 31, (w1 >> 15) & 7,
             (w0 >> 5) & 15, (w1 >> 24) & 15, w0 & 31, (w1 >> 6) & 7)
    if 1 in color:
        return True
    first = ((w0 >> 9) & 7, (w1 >> 9) & 7)
    if policy.othermode_h & (3 << 20) != 1 << 20:
        return 1 in first
    second = ((w1 >> 18) & 7, w1 & 7)
    return 1 in second or (0 in second and 1 in first)


def compile_packet(packet, name='dreamland'):
    if name != 'dreamland':
        raise ValueError('Only the proved Dream Land batch is admitted yet')
    words, patches = [], []
    runs = [(0,) * 14] * len(packet.runs)
    static_mask = stage.blob_rigid_mask(name) & ~stage.blob_camera_mask(packet)
    attributes = vertex_attributes(packet)
    baked_mask = 0
    for segment_id in SEGMENTS:
        segment = packet.segments[segment_id]
        for run_id in range(segment.first_run, segment.first_run + segment.run_count):
            run = packet.runs[run_id]
            cross = bool(run.flags & stage.RUN_FLAG_PROJECTED_CROSS_MATRIX)
            if (run.submit_class not in (stage.SUBMIT_RAW_CURRENT, stage.SUBMIT_PROJECTED_NO_Z, stage.SUBMIT_PROJECTED_RANGE_OR_MATRIX)
                or (cross and run.submit_class != stage.SUBMIT_PROJECTED_NO_Z)):
                raise ValueError('Unsupported stage run class')
            all_vertices = [packet.vertices[i] for i in packet.corners[
                run.first_corner:run.first_corner + run.triangle_count * 3]]
            bounds = tuple(min(getattr(v, axis) for v in all_vertices) for axis in ('x', 'y', 'z'))
            bounds += tuple(max(getattr(v, axis) for v in all_vertices) for axis in ('x', 'y', 'z'))
            binding_mask = sum(1 << b for b in {v.matrix_binding for v in all_vertices})
            composed_noz = (run.submit_class == stage.SUBMIT_PROJECTED_NO_Z and
                            (cross or not static_mask & (1 << run.binding_index)))
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

            if run.submit_class != stage.SUBMIT_PROJECTED_NO_Z:
                emit(0x10, 0)
                patch(0x16, PROJECTION)
            emit(0x10, 2)  # position/vector matrix
            if composed_noz:
                emit(0x15)  # preserve the CPU's composed precision for billboards
            else:
                patch(0x16, VIEW)
            last_shift = None
            for triangle in range(run.triangle_count):
                indices = packet.corners[run.first_corner + triangle * 3:run.first_corner + triangle * 3 + 3]
                vertices = [packet.vertices[i] for i in indices]
                if not cross and any(v.matrix_binding != run.binding_index for v in vertices):
                    raise ValueError('Foreign-binding corner needs a cross program')
                shift = run_shift(packet, run, indices)
                if shift != last_shift:
                    if last_shift is not None:
                        emit(0x12, 1)
                    emit(0x11)
                    if composed_noz:
                        pass  # the full transform is the triangle's projection
                    elif static_mask & (1 << run.binding_index):
                        emit(0x18, *world_words(packet, run.binding_index, shift))
                        baked_mask |= 1 << run.binding_index
                    else:
                        patch(0x18, WORLD, run.binding_index, shift)
                    last_shift = shift
                if triangle == 0:
                    emit(0x40, 0)  # triangles
                if run.submit_class == stage.SUBMIT_PROJECTED_NO_Z and not cross:
                    emit(0x10, 0)
                    if composed_noz:
                        patch(0x16, COMPOSED_NOZ, run.binding_index, shift)
                    else:
                        patch(0x16, NOZ, triangle)
                    emit(0x10, 2)
                for index, v in zip(indices, vertices):
                    vertex_shift = shift
                    if cross:
                        if len({v.matrix_binding for v in vertices}) != 1:
                            vertex_shift = stage.stage_vertex_coordinate_shift(v)
                        emit(0x10, 0)
                        patch(0x16, CORNER_NOZ, index, (triangle << 3) | vertex_shift)
                        emit(0x10, 2)
                    color, uv = attributes[index]
                    if color is None:
                        patch(0x20, COLOR, index)
                    else:
                        emit(0x20, color)
                    if uv is None:
                        patch(0x22, UV, index)
                    else:
                        emit(0x22, uv)
                    xyz = [stage.round_shift_signed(x, vertex_shift) * 16 for x in (v.x, v.y, v.z)]
                    if not all(-32768 <= x <= 32767 for x in xyz):
                        raise ValueError('Vertex does not fit DS VTX16')
                    emit(0x23, (xyz[0] & 0xFFFF) | ((xyz[1] & 0xFFFF) << 16), xyz[2] & 0xFFFF)
            emit(0x41)
            emit(0x12, 1)
            runs[run_id] = (first_word, len(words) - first_word, first_patch,
                            len(patches) - first_patch, run.triangle_count, segment_id) + bounds + (
                                binding_mask & 0xFFFFFFFF, binding_mask >> 32)
    if len(words) > 16384 or len(patches) > 4096:
        raise ValueError('GX template exceeds bounded runtime format')
    body = b''.join(RUN.pack(*r) for r in runs)
    body += b''.join(PATCH.pack(*p) for p in patches)
    body += struct.pack(f'<{len(words)}I', *words)
    return HEADER.pack(MAGIC, 3, stage.blob_gkind(name), len(runs), len(words),
                       len(patches), sum(1 << s for s in SEGMENTS), signature(packet), len(body),
                       stage.fnv1a_bytes(body), baked_mask & 0xFFFFFFFF, baked_mask >> 32) + body


def decode(blob):
    header = HEADER.unpack_from(blob)
    magic, version, _, nr, nw, np, _, _, nb, checksum, _, _ = header
    if magic != MAGIC or version not in (1, 2, 3) or len(blob) != HEADER.size + nb:
        raise ValueError('Invalid GX header/length')
    record = {1: RUN_V1, 2: RUN_V2, 3: RUN}[version]
    if nb != nr * record.size + np * PATCH.size + nw * 4 or stage.fnv1a_bytes(blob[HEADER.size:]) != checksum:
        raise ValueError('Invalid GX body')
    pos = HEADER.size
    runs = [record.unpack_from(blob, pos + i * record.size) for i in range(nr)]
    pos += nr * record.size
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
    triangles = sum(packet.runs[i].triangle_count for s in SEGMENTS for i in
                    range(packet.segments[s].first_run, packet.segments[s].first_run + packet.segments[s].run_count))
    print(f'STAGE_GX_COMPILED bytes={len(blob)} words={h[4]} patches={h[5]} triangles={triangles}')


if __name__ == '__main__':
    main()

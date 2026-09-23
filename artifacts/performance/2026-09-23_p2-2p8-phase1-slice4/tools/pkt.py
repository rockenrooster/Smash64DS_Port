"""Runtime-recorded fighter packet reader (slice 4).

A dump is two files from tools/dump-packets.ps1: pNNN-sS-struct.bin (the
NDSFighterPacket, layout from the ELF's DWARF: ptype /o NDSFighterPacket) and
pNNN-sS-words.bin (the packed GX FIFO words).

decode(words) -> list of (word_index_of_first_param, cmd, params) in FIFO
order, from the packed format (a command word carries up to four command
bytes, their parameters follow in order; 0x00 bytes are NOPs).
"""
import glob
import os
import re
import struct

PARAMS = {
    0x00: 0, 0x10: 1, 0x11: 0, 0x12: 1, 0x13: 1, 0x14: 1, 0x15: 0, 0x16: 16,
    0x17: 12, 0x18: 16, 0x19: 12, 0x1a: 9, 0x1b: 3, 0x1c: 3, 0x20: 1, 0x21: 1,
    0x22: 1, 0x23: 2, 0x24: 1, 0x25: 1, 0x26: 1, 0x27: 1, 0x28: 1, 0x29: 1,
    0x2a: 1, 0x2b: 1, 0x30: 1, 0x31: 1, 0x32: 1, 0x33: 1, 0x34: 32, 0x40: 1,
    0x41: 0, 0x50: 1, 0x60: 1, 0x70: 3, 0x71: 2, 0x72: 1,
}
NAMES = {
    0x10: 'MTX_MODE', 0x11: 'MTX_PUSH', 0x12: 'MTX_POP', 0x13: 'MTX_STORE',
    0x14: 'MTX_RESTORE', 0x15: 'MTX_IDENTITY', 0x16: 'MTX_LOAD_4x4',
    0x17: 'MTX_LOAD_4x3', 0x18: 'MTX_MULT_4x4', 0x19: 'MTX_MULT_4x3',
    0x1a: 'MTX_MULT_3x3', 0x1b: 'MTX_SCALE', 0x1c: 'MTX_TRANS', 0x20: 'COLOR',
    0x21: 'NORMAL', 0x22: 'TEXCOORD', 0x23: 'VTX_16', 0x24: 'VTX_10',
    0x25: 'VTX_XY', 0x26: 'VTX_XZ', 0x27: 'VTX_YZ', 0x28: 'VTX_DIFF',
    0x29: 'POLYGON_ATTR', 0x2a: 'TEXIMAGE_PARAM', 0x2b: 'PLTT_BASE',
    0x30: 'DIF_AMB', 0x31: 'SPE_EMI', 0x32: 'LIGHT_VECTOR', 0x33: 'LIGHT_COLOR',
    0x34: 'SHININESS', 0x40: 'BEGIN_VTXS', 0x41: 'END_VTXS',
}
INDEX_NONE = 0xffff


def decode(words):
    """Yield (cmd_word_index, byte_index, param_word_index, cmd, params)."""
    out = []
    i = 0
    n = len(words)
    while i < n:
        cw = words[i]
        cmds = [(cw >> (8 * b)) & 0xff for b in range(4)]
        ci = i
        i += 1
        for b, c in enumerate(cmds):
            if c not in PARAMS:
                raise ValueError('unknown command 0x%02x at word %d' % (c, ci))
            k = PARAMS[c]
            if c == 0 and k == 0:
                continue
            out.append((ci, b, i, c, list(words[i:i + k])))
            i += k
    if i != n:
        raise ValueError('decode overran %d != %d' % (i, n))
    return out


def load_words(path):
    b = open(path, 'rb').read()
    return list(struct.unpack('<%dI' % (len(b) // 4), b))


def u8(b, o):
    return b[o]


def u16(b, o):
    return struct.unpack_from('<H', b, o)[0]


def u32(b, o):
    return struct.unpack_from('<I', b, o)[0]


def load_struct(path):
    b = open(path, 'rb').read()
    assert len(b) == 3840, len(b)
    p = {
        'valid': u32(b, 0),
        'key': [u32(b, 4 + 4 * i) for i in range(6)],
        'words_ptr': u32(b, 28),
        'word_count': u32(b, 32),
        'root_count': u32(b, 40),
        'projection_index': u16(b, 44),
        'light_index': u16(b, 46),
        'light_root': b[48],
        'light_valid': b[49],
        'needs_fence': b[50],
        'texture_count': b[51],
        'texgen_group_count': b[52],
        'texgen_site_count': u16(b, 54),
        'triangle_count': u32(b, 56),
        'run_count': u32(b, 60),
        'site_count': u32(b, 112),
        'tint_modulate': u32(b, 116),
        'tint_prim_hash': u32(b, 120),
        'tint_bind_count': b[3548],
        'tint_bind_overflow': b[3549],
        'fence_other': b[3550],
        'tint_bind_seen': b[3551],
    }
    for k, o in (('raw_triangles', 64), ('raw_reuse', 68), ('cross_triangles', 72),
                 ('cross_reuse', 76), ('batch_begin', 80), ('batch_reuse', 84),
                 ('batch_end', 88), ('prepare_begin', 92), ('prepare_reuse', 96),
                 ('matrix_loads', 100), ('texture_binds', 104), ('vertex_loads', 108)):
        p[k] = u32(b, o)
    roots = []
    for r in range(32):
        o = 124 + 22 * r
        roots.append({
            'seed_index': u16(b, o),
            'local_index': [u16(b, o + 2 + 2 * j) for j in range(8)],
            'local_count': b[o + 18], 'parent_slot': b[o + 19],
            'store_slot': b[o + 20], 'seed_is_identity': b[o + 21],
        })
    p['roots'] = roots[:p['root_count']]
    sites = []
    for s in range(p['site_count']):
        o = 828 + 20 * s
        sites.append({
            'index': u16(b, o), 'root': b[o + 2], 'use_material': b[o + 3],
            'prim_from_root': b[o + 4], 'reserved': list(b[o + 5:o + 8]),
            'light_color_1': u32(b, o + 8), 'light_color_2': u32(b, o + 12),
            'material_color': u32(b, o + 16),
        })
    p['sites'] = sites
    p['textures'] = [{'slot_plus1': u16(b, 2108 + 8 * t), 'name': u16(b, 2110 + 8 * t),
                      'key_generation': u32(b, 2112 + 8 * t)}
                     for t in range(p['texture_count'])]
    groups = []
    for g in range(p['texgen_group_count']):
        o = 2300 + 28 * g
        groups.append({
            'scale_s': u32(b, o), 'scale_t': u32(b, o + 4), 'origin_s': u32(b, o + 8),
            'origin_t': u32(b, o + 12), 'offset': struct.unpack_from('<i', b, o + 16)[0],
            'first_site': u16(b, o + 20), 'site_count': u16(b, o + 22), 'root': b[o + 24],
        })
    p['texgen_groups'] = groups
    p['texgen_sites'] = [{'index': u16(b, 2524 + 4 * s), 'dense_id': u16(b, 2526 + 4 * s)}
                         for s in range(p['texgen_site_count'])]
    binds = []
    for t in range(min(p['tint_bind_count'], 24)):
        o = 3552 + 12 * t
        binds.append({'tex_index': u16(b, o), 'pal_index': u16(b, o + 2), 'root': b[o + 4],
                      'prim_from_root': b[o + 5], 'rgb': u32(b, o + 8)})
    p['tint_binds'] = binds
    return p


def dumps(label='r0'):
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    d = os.path.join(here, 'dumps', label)
    meta = {}
    for line in open(os.path.join(d, 'gdb.out'), errors='replace'):
        m = re.match(r'PKT n=(\d+) slot=(\d+) frame=(\d+) wc=(\d+)', line)
        if m:
            meta[int(m.group(1))] = {'slot': int(m.group(2)), 'frame': int(m.group(3)),
                                     'wc': int(m.group(4))}
    out = []
    for n in sorted(meta):
        s = meta[n]['slot']
        sp = os.path.join(d, 'p%03d-s%d-struct.bin' % (n, s))
        wp = os.path.join(d, 'p%03d-s%d-words.bin' % (n, s))
        if not (os.path.exists(sp) and os.path.exists(wp)):
            continue
        p = load_struct(sp)
        p['words'] = load_words(wp)
        p.update(meta[n])
        p['n'] = n
        out.append(p)
    return out

#!/usr/bin/env python3
"""lane3_relocgraph.py -- exact reference graph from the ELF's own relocations.

The link is run with --emit-relocs, so every output section has a .rel.<sec> table:
(r_offset, r_info) = where the reference sits and which symbol/type it names.  That is
the linker's own view (what --gc-sections used), free of the coincidences the literal
scan can pick up and complete for movw/movt pairs.

Usage:
  lane3_relocgraph.py build <elf> <nm-sysv.txt> <out.pkl>
  lane3_relocgraph.py compare <reloc.pkl> <regex-graph.pkl> [--cut REGEX]
The pickle has the same layout as lane3_refgraph's (syms/out/inn/kinds) so every
tool (lane3_unit.py, lane3_boundary.py ...) can run on it via --graph.
Reloc types handled: ABS32 (2), REL32(3), THM_CALL(10), CALL(28), JUMP24(29),
THM_JUMP24(30), THM_JUMP19(51), MOVW/MOVT ABS/PREL, TARGET1(38), GOT-less only.
A relocation against a SECTION symbol is resolved through the value stored at the site
(ABS32) or the branch offset (calls); unresolved ones are counted and reported.
"""
import bisect
import collections
import pickle
import struct
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lane3_symtab as st  # noqa: E402
import lane3_refgraph as rg  # noqa: E402

R_ARM_ABS32, R_ARM_REL32 = 2, 3
R_THM_CALL, R_ARM_CALL, R_ARM_JUMP24, R_THM_JUMP24, R_THM_JUMP19 = 10, 28, 29, 30, 51
R_TARGET1 = 38
R_MOVW_ABS_NC, R_MOVT_ABS = 43, 44
R_THM_MOVW_ABS_NC, R_THM_MOVT_ABS = 47, 48
R_PREL31 = 42


def read_elf(path):
    data = open(path, 'rb').read()
    e_shoff = struct.unpack_from('<I', data, 0x20)[0]
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHH', data, 0x2E)
    secs = []
    for i in range(e_shnum):
        o = e_shoff + i * e_shentsize
        secs.append(struct.unpack_from('<IIIIIIIIII', data, o))
    stro = secs[e_shstrndx][4]
    names = []
    for s in secs:
        end = data.index(b'\0', stro + s[0])
        names.append(data[stro + s[0]:end].decode())
    return data, secs, names


def main_build(elf, nm, outp):
    data, secs, names = read_elf(elf)
    syms = st.load(nm)
    keep = [s for s in syms if s['size'] > 0 and not s['name'].startswith('$')]
    keep.sort(key=lambda s: (s['addr'], -s['size']))
    g = rg.Graph()
    g.syms = keep
    g.by_addr = [(s['addr'], i) for i, s in enumerate(keep)]
    g.addrs = [a for a, _ in g.by_addr]
    # ELF symtab (for the reloc symbol values)
    symtab_idx = next(i for i, n in enumerate(names) if n == '.symtab')
    strtab_idx = secs[symtab_idx][6]
    st_off, st_size = secs[symtab_idx][4], secs[symtab_idx][5]
    str_off = secs[strtab_idx][4]
    elfsyms = []
    for o in range(st_off, st_off + st_size, 16):
        name_off, value, size, info, other, shndx = struct.unpack_from('<IIIBBH', data, o)
        elfsyms.append((value, size, info & 0xf, shndx))
    sec_addr = [s[3] for s in secs]
    sec_name = names
    unresolved = collections.Counter()
    total = collections.Counter()
    for si, s in enumerate(secs):
        if s[1] != 9:            # SHT_REL
            continue
        target_sec = s[7]        # sh_info
        tname = sec_name[target_sec]
        if tname.startswith('.debug') or tname.startswith('.ARM.') or tname in ('.comment',):
            continue
        base_addr = sec_addr[target_sec]
        base_off = secs[target_sec][4]
        if secs[target_sec][1] == 8 or secs[target_sec][5] == 0:
            continue
        for o in range(s[4], s[4] + s[5], 8):
            r_off, r_info = struct.unpack_from('<II', data, o)
            rtype, rsym = r_info & 0xff, r_info >> 8
            at = r_off                      # ET_EXEC: r_offset is the virtual address
            src = g.lookup(at)
            if rsym >= len(elfsyms):
                continue
            value, size, stype, shndx = elfsyms[rsym]
            total[rtype] += 1
            site = struct.unpack_from('<I', data, base_off + (r_off - base_addr))[0]
            dst_addr = None
            if rtype in (R_ARM_ABS32, R_TARGET1, R_ARM_REL32, R_PREL31):
                if stype == 3:                   # SECTION symbol: value+addend stored at the site
                    dst_addr = (site if rtype != R_ARM_REL32 else (at + site)) & ~1
                    if rtype == R_PREL31:
                        dst_addr = None
                else:
                    dst_addr = value & ~1
            elif rtype in (R_THM_CALL, R_ARM_CALL, R_ARM_JUMP24, R_THM_JUMP24, R_THM_JUMP19):
                if stype != 3:
                    dst_addr = value & ~1
                else:
                    unresolved['section-branch'] += 1
            elif rtype in (R_MOVW_ABS_NC, R_MOVT_ABS, R_THM_MOVW_ABS_NC, R_THM_MOVT_ABS):
                if stype != 3:
                    dst_addr = value & ~1
                else:
                    unresolved['section-movwt'] += 1
            else:
                unresolved['type%d' % rtype] += 1
            if dst_addr is None:
                continue
            dst = g.lookup(dst_addr)
            kind = 'call' if rtype in (R_THM_CALL, R_ARM_CALL, R_ARM_JUMP24, R_THM_JUMP24, R_THM_JUMP19) else 'word'
            g.add(src, dst, kind)
    with open(outp, 'wb') as f:
        pickle.dump({'syms': g.syms, 'out': dict(g.out), 'inn': dict(g.inn),
                     'kinds': {k: sorted(v) for k, v in g.kinds.items()}}, f)
    print('reloc types seen:', dict(total))
    print('unresolved:', dict(unresolved))
    print('symbols', len(g.syms), 'edges', sum(len(v) for v in g.out.values()))


def main_compare(a_path, b_path, cutre=None):
    import re
    A = rg.Loaded(a_path)      # reloc graph
    B = rg.Loaded(b_path)      # literal-scan graph
    key = lambda L, i: (L.syms[i]['name'], L.syms[i]['addr'])
    idxA = {key(A, i): i for i in range(len(A.syms))}
    idxB = {key(B, i): i for i in range(len(B.syms))}
    common = set(idxA) & set(idxB)
    missing_in_B = 0
    extra_in_B = 0
    ex_missing = []
    ex_extra = []
    for k in common:
        ia, ib = idxA[k], idxB[k]
        ra = {key(A, x) for x in A.inn.get(ia, ())}
        rb = {key(B, x) for x in B.inn.get(ib, ())}
        m = ra - rb
        e = rb - ra
        missing_in_B += len(m)
        extra_in_B += len(e)
        if m and len(ex_missing) < 12:
            ex_missing.append((k[0], sorted(x[0] for x in m)[:3]))
        if e and len(ex_extra) < 12:
            ex_extra.append((k[0], sorted(x[0] for x in e)[:3]))
    print('symbols common', len(common))
    print('edges in reloc graph but missing from the literal scan:', missing_in_B)
    print('edges in the literal scan but not in relocs (coincidence / interior refs):', extra_in_B)
    print('examples missing:', ex_missing)
    print('examples extra:', ex_extra)


if __name__ == '__main__':
    if sys.argv[1] == 'build':
        main_build(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        main_compare(sys.argv[2], sys.argv[3])

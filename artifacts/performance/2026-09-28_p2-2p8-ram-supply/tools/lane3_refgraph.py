#!/usr/bin/env python3
"""lane3_refgraph.py -- static reference graph of a linked ELF.

Build:  lane3_refgraph.py build <elf> <nm-sysv.txt> <objdump-d.txt> <out.pkl>
Query:  lane3_refgraph.py callers <pkl> <symbol-regex> [--depth N]
        lane3_refgraph.py reach   <pkl> <root-name,...> [--cut REGEX] [--out list.txt]
        lane3_refgraph.py family  <pkl> <member-regex> [--roots main]

Edges (src -> dst), both are symbol *indices*:
  * direct branches (bl / blx / b<cond> to another symbol) parsed from objdump -d
  * literal-pool words in code (objdump `.word` lines) that point into a symbol
  * 4-byte words inside OBJECT symbols (rodata/data tables) that point into a symbol
Words inside instruction streams are NOT scanned (they alias addresses).

Limitations (stated in the report): no decoding of computed jumps, `bx rX` tail
calls through a register, or addresses built from two immediates (movw/movt).
Function pointers stored as literals/tables are covered.
"""
import bisect
import collections
import pickle
import re
import struct
import sys

sys.path.insert(0, __file__.rsplit('\\', 1)[0] if '\\' in __file__ else __file__.rsplit('/', 1)[0])
import lane3_symtab as st  # noqa: E402

RE_LABEL = re.compile(r'^([0-9a-f]{8}) <(.+)>:$')
RE_INSN = re.compile(r'^\s*([0-9a-f]+):\t(\S+)(?:\s+(.*))?$')
RE_BR = re.compile(r'^(bl|blx|b)(eq|ne|cs|hs|cc|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le|al)?(\.n|\.w)?$')
RE_TARGET = re.compile(r'^([0-9a-f]+)\s+<')
RE_WORD = re.compile(r'^0x([0-9a-f]+)')


def read_elf_sections(path):
    with open(path, 'rb') as f:
        data = f.read()
    assert data[:4] == b'\x7fELF'
    e_shoff = struct.unpack_from('<I', data, 0x20)[0]
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHH', data, 0x2E)
    secs = []
    for i in range(e_shnum):
        o = e_shoff + i * e_shentsize
        (sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link, sh_info,
         sh_addralign, sh_entsize) = struct.unpack_from('<IIIIIIIIII', data, o)
        secs.append([sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size])
    stro = secs[e_shstrndx][4]
    out = {}
    for s in secs:
        n = s[0]
        end = data.index(b'\0', stro + n)
        name = data[stro + n:end].decode()
        out[name] = {'type': s[1], 'flags': s[2], 'addr': s[3], 'off': s[4], 'size': s[5]}
    return data, out


class Graph:
    def __init__(self):
        self.syms = []          # list of dicts
        self.by_addr = []       # sorted list of (addr, idx)
        self.addrs = []
        self.out = collections.defaultdict(set)
        self.inn = collections.defaultdict(set)
        self.kinds = collections.defaultdict(set)   # (src,dst) -> {'call','word','data'}

    def lookup(self, a):
        i = bisect.bisect_right(self.addrs, a) - 1
        # walk back over zero-size / non-covering aliases
        j = i
        while j >= 0 and j > i - 6:
            idx = self.by_addr[j][1]
            s = self.syms[idx]
            if s['addr'] <= a < s['addr'] + max(s['size'], 1):
                return idx
            j -= 1
        return None

    def add(self, src, dst, kind):
        if src is None or dst is None or src == dst:
            return
        self.out[src].add(dst)
        self.inn[dst].add(src)
        self.kinds[(src, dst)].add(kind)


def build(elf, nm, dis, outp):
    g = Graph()
    syms = st.load(nm)
    # keep sized FUNC/OBJECT symbols only, drop mapping symbols
    keep = []
    for s in syms:
        if s['size'] <= 0:
            continue
        if s['name'].startswith('$'):
            continue
        keep.append(s)
    keep.sort(key=lambda s: (s['addr'], -s['size']))
    g.syms = keep
    g.by_addr = [(s['addr'], i) for i, s in enumerate(keep)]
    g.addrs = [a for a, _ in g.by_addr]
    data, secs = read_elf_sections(elf)

    # 1) objdump: branches and pool words
    cur_fn_addr = None
    with open(dis, errors='replace') as f:
        for line in f:
            line = line.rstrip('\n')
            m = RE_INSN.match(line)
            if not m:
                continue
            addr = int(m.group(1), 16)
            mn = m.group(2)
            rest = m.group(3) or ''
            if mn == '.word':
                w = RE_WORD.match(rest)
                if not w:
                    continue
                val = int(w.group(1), 16)
                if 0x01ff8000 <= val < 0x03000000 or 0x02000000 <= val:
                    src = g.lookup(addr)
                    dst = g.lookup(val & ~1)
                    g.add(src, dst, 'word')
                continue
            b = RE_BR.match(mn)
            if b:
                t = RE_TARGET.match(rest)
                if t:
                    tv = int(t.group(1), 16)
                    src = g.lookup(addr)
                    dst = g.lookup(tv)
                    g.add(src, dst, 'call')
    # 2) OBJECT symbols: scan words in initialised data / rodata
    secinfo = []
    for name, s in secs.items():
        if s['type'] == 1 and s['flags'] & 2 and s['size'] > 0 and s['addr'] != 0:  # PROGBITS + ALLOC
            secinfo.append((s['addr'], s['addr'] + s['size'], s['off'], name))
    secinfo.sort()

    def read_word(a):
        for lo, hi, off, name in secinfo:
            if lo <= a < hi:
                return struct.unpack_from('<I', data, off + (a - lo))[0]
        return None

    for idx, s in enumerate(keep):
        if s['typ'] != 'OBJECT':
            continue
        if s['sec'] in ('.main.bss', '.dtcm.bss', '.ovl.frontend.bss'):
            continue
        a0 = s['addr'] & ~3
        for a in range(a0, s['addr'] + s['size'] - 3, 4):
            w = read_word(a)
            if w is None:
                break
            if 0x01ff8000 <= w < 0x03000000 or 0x02000000 <= w:
                # numeric tables alias addresses: accept only an exact symbol
                # start (thumb bit optional) as a pointer stored in data.
                dst = g.lookup(w & ~1)
                if dst is not None and g.syms[dst]['addr'] == (w & ~1):
                    g.add(idx, dst, 'data')
    with open(outp, 'wb') as f:
        pickle.dump({'syms': g.syms, 'out': dict(g.out), 'inn': dict(g.inn),
                     'kinds': {k: sorted(v) for k, v in g.kinds.items()}}, f)
    print('symbols', len(g.syms), 'edges', sum(len(v) for v in g.out.values()))


class Loaded:
    def __init__(self, path):
        d = pickle.load(open(path, 'rb'))
        self.syms = d['syms']
        self.out = d['out']
        self.inn = d['inn']
        self.kinds = d['kinds']
        self.byname = collections.defaultdict(list)
        for i, s in enumerate(self.syms):
            self.byname[s['name']].append(i)

    def idx(self, name):
        return self.byname.get(name, [])

    def reach(self, roots, cut=None, cut_edges=None):
        """cut: regex of node names removed; cut_edges: list of (src_re, dst_re) compiled
        regex pairs -- an edge u->v is ignored when name(u) matches src_re and name(v) matches dst_re."""
        seen = set()
        stack = []
        for r in roots:
            for i in self.idx(r):
                stack.append(i)
        while stack:
            u = stack.pop()
            if u in seen:
                continue
            seen.add(u)
            un = self.syms[u]['name']
            for v in self.out.get(u, ()):
                if v in seen:
                    continue
                vn = self.syms[v]['name']
                if cut and cut.search(vn):
                    continue
                if cut_edges and any(a.search(un) and b.search(vn) for a, b in cut_edges):
                    continue
                stack.append(v)
        return seen


def cmd_callers(pkl, rx, depth=1):
    L = Loaded(pkl)
    r = re.compile(rx)
    for i, s in enumerate(L.syms):
        if r.search(s['name']):
            print('== %s (%d B, %s)' % (s['name'], s['size'], s['sec']))
            front = {i}
            seen = {i}
            for d in range(depth):
                nxt = set()
                for u in front:
                    for c in sorted(L.inn.get(u, ())):
                        if c in seen:
                            continue
                        seen.add(c)
                        nxt.add(c)
                        cs = L.syms[c]
                        print('  ' * (d + 1) + '<- %s (%d B, %s) [%s]' % (
                            cs['name'], cs['size'], cs['sec'], ','.join(L.kinds.get((c, u), []))))
                front = nxt


def main():
    cmd = sys.argv[1]
    if cmd == 'build':
        build(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5])
    elif cmd == 'callers':
        depth = 1
        if '--depth' in sys.argv:
            depth = int(sys.argv[sys.argv.index('--depth') + 1])
        cmd_callers(sys.argv[2], sys.argv[3], depth)
    elif cmd == 'reach':
        L = Loaded(sys.argv[2])
        roots = sys.argv[3].split(',')
        cut = None
        if '--cut' in sys.argv:
            cut = re.compile(sys.argv[sys.argv.index('--cut') + 1])
        seen = L.reach(roots, cut)
        tot = collections.Counter()
        for i in seen:
            s = L.syms[i]
            tot[(s['sec'], s['kind'])] += s['size']
        for k in sorted(tot):
            print(k, tot[k])
        print('reached', len(seen), 'of', len(L.syms))
        if '--out' in sys.argv:
            with open(sys.argv[sys.argv.index('--out') + 1], 'w') as f:
                for i in sorted(seen):
                    f.write(L.syms[i]['name'] + '\n')


if __name__ == '__main__':
    main()

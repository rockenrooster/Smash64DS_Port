#!/usr/bin/env python3
"""Host proof of the P2-2p8 Phase 1 slice 4 fighter lists.

The lean path materializes a fighter's list at runtime from the generated
owner tables (src/nds/nds_renderer_native_common.c, ndsFtrLeanMaterialize) in
the LOAD4x3 + P' layout.  Its host twin is scripts/fighters/
fighter_list_emitter.py.  This module proves that twin against packets the
runtime RECORDED (tools/dump-packets.ps1 of the slice 4 artifacts dumps the
recorder's own packet at TryReplay's post-patch point: the NDSFighterPacket
struct and its packed GX FIFO words), and prices the layout change:

  1. word compare, per kind (Samus, DK, Link, Kirby): the host's recorded-
     layout emission of the dump's root vector must equal the dump word for
     word, every command and every parameter, with only the words the tables
     cannot fix masked (the matrices, the light vector, DIF_AMB, the texture
     VRAM words TEXIMAGE_PARAM / PLTT_BASE, texgen coordinates).  The two
     material-state structure decisions (does an untextured material epoch
     take the tint tile; does a texture carry a palette word) are read from
     the dump, exactly as the runtime reads them from the live material.  A
     dump that matches only under production's run texture memo (keyed by the
     run index alone, so a donor-table root replays the owner root's texture
     and UV parameters) is reported as that named production defect.
  2. lean layout (Task 49 method): the host's LOAD4x3 + P' twin of the same
     draw must carry every non-matrix command of the dump unchanged (tier 1,
     masked as above), and the matrices it loads -- P' = the dump's projection
     with row 3 >> 8, LOAD4x3 = the first three columns of each root's split
     modelview -- must transform every vertex of the draw where the recorded
     packet does (tier 2): a geometry-engine model (matrix mode, projection,
     position matrix, the absolute store/restore stack) runs both streams and
     compares each vertex's clip coordinates, x/w, y/w, z/w and its screen
     position.
  3. list RAM: words per kind x detail x variant in both layouts against the
     entry capacity, the template store a resident-template design would need,
     and the owner images the lists would replace.
"""
from __future__ import annotations

import collections
import copy
import json
import os
import re
import struct
import sys
from pathlib import Path

_scripts_root = Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in sys.path:
    sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402,F401

import fighter_list_emitter as fle  # noqa: E402

ROOT = _scripts_root.parent
KIND_BY_SLOT = {0: 'donkey', 1: 'samus', 2: 'link', 3: 'kirby'}
ORDER = ('samus', 'donkey', 'link', 'kirby')
MASKED = ('proj', 'mv', 'light', 'texgen', 'teximage', 'pltt', 'shade')
DEFAULT_PRE = dict(geometry_mode=0x1 | 0x4 | 0x400 | 0x20000 | 0x200000,
                   cycle_type=1 << 20, render_mode=0xC4112078,
                   prim=0xffffffff, env=0xffffffff, light_valid=1,
                   light_dir=(0, 0, 0))
# NDSFighterPacket (DWARF: ptype /o NDSFighterPacket), 3,840 bytes.
PACKET_BYTES = 3840
INDEX_NONE = 0xffff
ENTRY_CAPACITY_WORDS = 8840 // 2 - 960
PARAMS = {
    0x00: 0, 0x10: 1, 0x11: 0, 0x12: 1, 0x13: 1, 0x14: 1, 0x15: 0, 0x16: 16,
    0x17: 12, 0x18: 16, 0x19: 12, 0x1a: 9, 0x1b: 3, 0x1c: 3, 0x20: 1, 0x21: 1,
    0x22: 1, 0x23: 2, 0x24: 1, 0x25: 1, 0x26: 1, 0x27: 1, 0x28: 1, 0x29: 1,
    0x2a: 1, 0x2b: 1, 0x30: 1, 0x31: 1, 0x32: 1, 0x33: 1, 0x34: 32, 0x40: 1,
    0x41: 0, 0x50: 1, 0x60: 1, 0x70: 3, 0x71: 2, 0x72: 1,
}


# ---- runtime packet dumps ---------------------------------------------------

def decode(words):
    """[(param_index, cmd, params)] in FIFO order (a command word carries up
    to four command bytes whose parameters follow in order; 0x00 is NOP, and
    the dummy word after a parameterless header decodes as four NOPs)."""
    out = []
    i = 0
    n = len(words)
    while i < n:
        cw = words[i]
        i += 1
        for b in range(4):
            c = (cw >> (8 * b)) & 0xff
            if c not in PARAMS:
                raise ValueError('unknown command 0x%02x at word %d' % (c, i - 1))
            k = PARAMS[c]
            if c == 0:
                continue
            out.append((i, c, list(words[i:i + k])))
            i += k
    if i != n:
        raise ValueError('decode overran %d != %d' % (i, n))
    return out


def _u16(b, o):
    return struct.unpack_from('<H', b, o)[0]


def _u32(b, o):
    return struct.unpack_from('<I', b, o)[0]


def load_struct(path):
    b = Path(path).read_bytes()
    if len(b) != PACKET_BYTES:
        raise ValueError('%s: %d bytes, not an NDSFighterPacket' % (path, len(b)))
    p = {'word_count': _u32(b, 32), 'root_count': _u32(b, 40),
         'light_index': _u16(b, 46), 'light_root': b[48],
         'site_count': _u32(b, 112), 'texgen_site_count': _u16(b, 54),
         'tint_bind_count': b[3548]}
    p['roots'] = [{'seed_index': _u16(b, 124 + 22 * r),
                   'local_index': [_u16(b, 126 + 22 * r + 2 * j) for j in range(8)]}
                  for r in range(p['root_count'])]
    return p


def load_dumps(dump_dir):
    d = Path(dump_dir)
    meta = {}
    for line in (d / 'gdb.out').read_text(errors='replace').splitlines():
        m = re.match(r'PKT n=(\d+) slot=(\d+) frame=(\d+) wc=(\d+)', line)
        if m:
            meta[int(m.group(1))] = dict(slot=int(m.group(2)), frame=int(m.group(3)),
                                         wc=int(m.group(4)))
    out = []
    for n in sorted(meta):
        s = meta[n]['slot']
        sp = d / ('p%03d-s%d-struct.bin' % (n, s))
        wp = d / ('p%03d-s%d-words.bin' % (n, s))
        if not (sp.exists() and wp.exists()):
            continue
        p = load_struct(sp)
        raw = wp.read_bytes()
        p['words'] = list(struct.unpack('<%dI' % (len(raw) // 4), raw))
        p.update(meta[n])
        p['n'] = n
        out.append(p)
    return out


# ---- 1. word compare ---------------------------------------------------------

def dump_cmds(words):
    return [(c, prm, pi) for (pi, c, prm) in decode(words)]


def host_cmds(em, first_cmd=0):
    out = []
    for (first, op, n) in em.pk.cmds[first_cmd:]:
        out.append((op, [em.tags.get(first + k, em.pk.words[first + k])
                         for k in range(n)], first))
    return out


def cmd_equal(host, dump_cmd):
    hop, hparams = host[0], host[1]
    if hop != dump_cmd[0]:
        return False
    for hv, dv in zip(hparams, dump_cmd[1]):
        if isinstance(hv, fle.Tag):
            if hv.kind in MASKED:
                continue
            return False
        if hv != dv:
            return False
    return True


def candidates(ctx):
    """Every executable root the kind can draw: its own rows, then every
    program root with the table set that owns it (mixed-file donors)."""
    out = []
    seen = set()
    for r in range(len(ctx['roots'])):
        key = (id(ctx), tuple(ctx['roots'][r]))
        if key not in seen:
            seen.add(key)
            out.append((ctx, r, None, None, ctx['root_bindings'][r]))
    for prog in ctx.get('root_programs', []):
        vcs = prog.get('verification_contexts') or [ctx] * len(prog['roots'])
        for k, row in enumerate(prog['roots']):
            rctx = vcs[k]
            if 'direct_epoch_policies' not in rctx:
                rctx = ctx
            key = (id(rctx), tuple(row))
            if key in seen:
                continue
            seen.add(key)
            out.append((rctx, None, tuple(row), prog['light_indices'][k], -1))
    return out


def root_blocks(cmds):
    """Index of every root's first command: the recorder opens each root with
    MTX_MODE(0) (its split projection load)."""
    return [k for k, (op, prm, _pi) in enumerate(cmds)
            if op == fle.C_MTX_MODE and prm[0] == 0]


def match_dump(p, ctx, memo=False):
    """Recover the dump's root vector block by block: for each block every
    candidate root is emitted from the running host state; a candidate matches
    when every command and every unmasked word agrees."""
    cmds = dump_cmds(p['words'])
    starts = root_blocks(cmds)
    ends = starts[1:] + [len(cmds)]
    em = fle.Emitter(ctx, decide=fle.GuidedDecider([(c[0], c[1]) for c in cmds]),
                     memo=memo)
    chosen = []
    cands = candidates(ctx)
    for ri, (a, b) in enumerate(zip(starts, ends)):
        block = cmds[a:b]
        slot = 31
        for k in range(min(5, len(block))):
            if block[k][0] == fle.C_MTX_STORE:
                slot = block[k][1][0]
                break
        found = None
        best = (-1, None)
        order = sorted(range(len(cands)), key=lambda c: (cands[c][4] != ri, c))
        for ci in order:
            rctx, rindex, row, light, _b = cands[ci]
            trial = copy.deepcopy(em)
            n0 = len(trial.pk.cmds)
            try:
                trial.emit_root(ri, rindex, DEFAULT_PRE, None, slot, ctx=rctx,
                                row=row, light_index=light)
            except Exception:
                continue
            hc = host_cmds(trial, n0)
            k = 0
            while k < len(hc) and k < len(block) and cmd_equal(hc[k], block[k]):
                k += 1
            if k == len(hc) == len(block):
                found = (ci, trial)
                break
            if k > best[0]:
                best = (k, ci)
        if found is None:
            return dict(ok=False, roots=chosen, fail_root=ri, best=best,
                        blocks=len(starts))
        chosen.append(found[0])
        em = found[1]
    pk, _tags = em.finish()
    # The raw words: the packing (header layout, dummy words) must agree too,
    # everywhere outside the masked parameter words.
    masked = set(i for i, t in em.tags.items() if t.kind in MASKED)
    raw_ok = (len(pk.words) == len(p['words'])) and all(
        (i in masked) or (pk.words[i] == p['words'][i]) for i in range(len(pk.words)))
    return dict(ok=True, roots=chosen, emitter=em, raw_ok=raw_ok,
                masked=len(masked), words=len(pk.words))


def lean_twin(p, ctx, roots, memo=False):
    """The same draw in the lean layout, the dump's structure decisions
    replayed from the recorded twin's."""
    cmds = dump_cmds(p['words'])
    starts = root_blocks(cmds)
    cands = candidates(ctx)
    rec = fle.Emitter(ctx, decide=fle.GuidedDecider([(c[0], c[1]) for c in cmds]),
                      memo=memo)
    decisions = []

    class Recorder(fle.GuidedDecider):
        def tint(self, pk, ri, e, default):
            v = fle.GuidedDecider.tint(self, pk, ri, e, default)
            decisions.append(('tint', v))
            return v

        def pltt(self, pk, ri, r):
            v = fle.GuidedDecider.pltt(self, pk, ri, r)
            decisions.append(('pltt', v))
            return v

    rec.decide = Recorder([(c[0], c[1]) for c in cmds])
    for ri, ci in enumerate(roots):
        block = cmds[starts[ri]:]
        slot = 31
        for k in range(min(5, len(block))):
            if block[k][0] == fle.C_MTX_STORE:
                slot = block[k][1][0]
                break
        rctx, rindex, row, light, _b = cands[ci]
        rec.emit_root(ri, rindex, DEFAULT_PRE, None, slot, ctx=rctx, row=row,
                      light_index=light)

    class Replay(object):
        def __init__(self):
            self.i = 0

        def _next(self, what):
            kind, v = decisions[self.i]
            self.i += 1
            assert kind == what, (kind, what)
            return v

        def tint(self, pk, ri, e, default):
            return self._next('tint')

        def pltt(self, pk, ri, r):
            return self._next('pltt')

    lean = fle.Emitter(ctx, decide=Replay(), layout='lean', lit=True, memo=memo)
    for ri, ci in enumerate(roots):
        block = cmds[starts[ri]:]
        slot = 31
        for k in range(min(5, len(block))):
            if block[k][0] == fle.C_MTX_STORE:
                slot = block[k][1][0]
                break
        rctx, rindex, row, light, _b = cands[ci]
        lean.emit_root(ri, rindex, DEFAULT_PRE, None, slot, ctx=rctx, row=row,
                       light_index=light)
    lean.finish()
    return lean


# ---- 2. the Task 49 geometry model -------------------------------------------

def _s32(v):
    v &= 0xffffffff
    return v - (1 << 32) if v & 0x80000000 else v


def _s16(v):
    v &= 0xffff
    return v - 0x10000 if v & 0x8000 else v


def _mul44(a, b):
    return [[(sum(a[r][k] * b[k][c] for k in range(4))) >> 12 for c in range(4)]
            for r in range(4)]


def _identity():
    return [[4096 if r == c else 0 for c in range(4)] for r in range(4)]


def _round_shift8(v):
    # ndsRendererRoundShiftS32Signed(v, 8): round half away from zero.
    mag = -v if v < 0 else v
    r = (mag + 0x80) >> 8
    return -r if v < 0 else r


class GxModel(object):
    """The geometry engine's matrix state: the mode, the projection, the
    position matrix, the position stack (PUSH/POP at the pointer, STORE and
    RESTORE at absolute levels); every VTX_16 is transformed by
    position x projection."""

    def __init__(self):
        self.mode = 0
        self.proj = _identity()
        self.pos = _identity()
        self.stack = [_identity() for _ in range(32)]
        self.sp = 0
        self.clip = None
        self.vertices = []

    def _load(self, m):
        if self.mode == 0:
            self.proj = m
        elif self.mode in (1, 2):
            self.pos = m
        self.clip = None

    def run(self, cmds):
        for op, prm in cmds:
            if op == 0x10:
                self.mode = prm[0] & 3
            elif op == 0x16:
                w = [_s32(x) for x in prm]
                self._load([w[0:4], w[4:8], w[8:12], w[12:16]])
            elif op == 0x17:
                w = [_s32(x) for x in prm]
                self._load([w[0:3] + [0], w[3:6] + [0], w[6:9] + [0], w[9:12] + [4096]])
            elif op == 0x15:
                self._load(_identity())
            elif op == 0x11:
                if self.mode in (1, 2):
                    self.stack[self.sp & 31] = self.pos
                    self.sp += 1
            elif op == 0x12:
                if self.mode in (1, 2):
                    off = prm[0] & 0x3f
                    off = off - 64 if off & 0x20 else off
                    self.sp -= off
                    self.pos = self.stack[self.sp & 31]
                    self.clip = None
            elif op == 0x13:
                if self.mode in (1, 2):
                    self.stack[prm[0] & 31] = self.pos
            elif op == 0x14:
                if self.mode in (1, 2):
                    self.pos = self.stack[prm[0] & 31]
                    self.clip = None
            elif op == 0x23:
                if self.clip is None:
                    self.clip = _mul44(self.pos, self.proj)
                x = _s16(prm[0])
                y = _s16(prm[0] >> 16)
                z = _s16(prm[1])
                v = (x, y, z, 4096)
                self.vertices.append(tuple(
                    sum(v[i] * self.clip[i][c] for i in range(4)) >> 12
                    for c in range(4)))
        return self.vertices


def lean_matrices(dump, lean_em):
    """The lean twin's command stream with its matrix sites filled from the
    dump's recorded matrices (P' from root 0's projection, each LOAD4x3 from
    its root's split modelview)."""
    words = dump['words']
    proj = words[dump['roots'][0]['local_index'][0]:][:16]
    cmds = []
    for (first, op, n) in lean_em.pk.cmds:
        prm = list(lean_em.pk.words[first:first + n])
        for k in range(n):
            t = lean_em.tags.get(first + k)
            if t is None:
                continue
            if t.kind == 'pproj':
                v = _s32(proj[t.info])
                prm[k] = (_round_shift8(v) if t.info >= 12 else v) & 0xffffffff
            elif t.kind == 'mv43':
                ri, j = t.info
                seed = dump['roots'][ri]['seed_index']
                row, col = divmod(j, 3)
                prm[k] = words[seed + row * 4 + col]
            elif t.kind == 'light':
                prm[k] = words[dump['light_index']] if dump['light_index'] != INDEX_NONE else 0
        cmds.append((op, prm))
    return cmds


def screen(clip):
    x, y, z, w = clip
    if w == 0:
        return None
    return ((x / w + 1.0) * 128.0, (1.0 - y / w) * 96.0, z / w)


def tier1(dump_cmds_list, lean_em):
    """Non-matrix commands of both layouts, in order; masked sites skipped."""
    skip = (0x10, 0x11, 0x12, 0x15, 0x16, 0x17, 0x32)
    rec = [c for c in dump_cmds_list if c[0] not in skip]
    lean = [c for c in host_cmds(lean_em) if c[0] not in skip]
    if len(rec) != len(lean):
        return 1, len(rec), len(lean)
    bad = 0
    for h, d in zip(lean, rec):
        if not cmd_equal(h, d):
            bad += 1
    return bad, len(rec), len(lean)


def tier2(dump, lean_em):
    rec_cmds = [(c, prm) for (c, prm, _pi) in dump_cmds(dump['words'])]
    lean_cmds = lean_matrices(dump, lean_em)
    a = GxModel().run(rec_cmds)
    b = GxModel().run(lean_cmds)
    st = dict(vertices=len(a), count_equal=(len(a) == len(b)), clip_max=[0, 0, 0, 0],
              xw_max=0.0, yw_max=0.0, zw_max=0.0, px_max=0.0, px_moved=0, clipped=0)
    if len(a) != len(b):
        return st
    for va, vb in zip(a, b):
        for c in range(4):
            st['clip_max'][c] = max(st['clip_max'][c], abs(va[c] - vb[c]))
        sa, sb = screen(va), screen(vb)
        if sa is None or sb is None:
            continue
        inside = all(abs(va[c]) <= abs(va[3]) for c in range(3))
        if not inside:
            st['clipped'] += 1
        dx, dy, dz = abs(sa[0] - sb[0]), abs(sa[1] - sb[1]), abs(sa[2] - sb[2])
        st['xw_max'] = max(st['xw_max'], dx / 128.0)
        st['yw_max'] = max(st['yw_max'], dy / 96.0)
        st['zw_max'] = max(st['zw_max'], dz)
        if inside:
            st['px_max'] = max(st['px_max'], dx, dy)
            if (int(sa[0]) != int(sb[0])) or (int(sa[1]) != int(sb[1])):
                st['px_moved'] += 1
    return st


# ---- 3. list RAM ---------------------------------------------------------------

def list_words(kind, detail):
    """{variant: (lean words, recorded words, roots)} for the canonical vector
    and every root program of the kind at this detail (host emission, the
    structure decisions at their maximum: every tint-capable epoch tinted,
    every texture paletted)."""
    ctx = fle.context(kind, detail)

    class Max(object):
        def tint(self, pk, ri, e, default):
            return True

        def pltt(self, pk, ri, r):
            return True

    vectors = [('canonical', [(ctx, r, None, None)
                              for r in range(ctx['canonical_root_count'])])]
    for prog in ctx.get('root_programs', []):
        vcs = prog.get('verification_contexts') or [ctx] * len(prog['roots'])
        v = []
        for k, row in enumerate(prog['roots']):
            rctx = vcs[k] if 'direct_epoch_policies' in vcs[k] else ctx
            v.append((rctx, None, tuple(row), prog['light_indices'][k]))
        vectors.append((prog['name'], v))
    out = {}
    for name, vec in vectors:
        sizes = []
        for layout in ('lean', 'recorded'):
            em = fle.Emitter(ctx, decide=Max(), layout=layout, lit=True)
            for ri, (rctx, rindex, row, light) in enumerate(vec):
                em.emit_root(ri, rindex, DEFAULT_PRE, None, 31, ctx=rctx,
                             row=row, light_index=light)
            pk, _ = em.finish()
            sizes.append(len(pk.words))
        out[name] = (sizes[0], sizes[1], len(vec))
    return out


def owner_image_bytes(build_dir):
    d = Path(build_dir) / 'nitrofs' / 'fighters'
    out = {}
    for kind in ORDER:
        for detail in ('low', 'high'):
            f = d / ('%s_%s.bin' % (kind, detail))
            out['%s_%s' % (kind, detail)] = f.stat().st_size if f.exists() else None
    hats = sorted(d.glob('kirby_hat_*.bin')) if d.exists() else []
    out['kirby_hats'] = sum(f.stat().st_size for f in hats)
    out['kirby_hat_files'] = len(hats)
    return out


# ---- driver ----------------------------------------------------------------------

def run(dump_dir, build_dir=None, json_out=None, verbose=True):
    dumps = load_dumps(dump_dir)
    result = {'dumps': len(dumps), 'kinds': {}}
    ctxs = {}
    failures = 0
    for kind in ORDER:
        kd = [p for p in dumps if KIND_BY_SLOT.get(p['slot']) == kind]
        if kind not in ctxs:
            ctxs[kind] = fle.context(kind, 'low')
        ctx = ctxs[kind]
        k = dict(dumps=len(kd), exact=0, raw_exact=0, memo_exact=0, fail=0,
                 tier1_bad=0, tier1_cmds=0, tier2=dict(vertices=0, clip_max=[0, 0, 0, 0],
                                                       xw_max=0.0, yw_max=0.0, zw_max=0.0,
                                                       px_max=0.0, px_moved=0,
                                                       count_mismatch=0),
                 vectors=collections.Counter(), failed=[])
        for p in kd:
            res = match_dump(p, ctx)
            memo = False
            if not res['ok']:
                res = match_dump(p, ctx, memo=True)
                memo = res['ok']
            if not res['ok']:
                k['fail'] += 1
                k['failed'].append(dict(n=p['n'], frame=p['frame'], root=res['fail_root'],
                                        best=res['best']))
                failures += 1
                continue
            if memo:
                k['memo_exact'] += 1
            else:
                k['exact'] += 1
            if res['raw_ok']:
                k['raw_exact'] += 1
            k['vectors'][tuple(res['roots'])] += 1
            lean = lean_twin(p, ctx, res['roots'], memo=memo)
            bad, nrec, _nlean = tier1(dump_cmds(p['words']), lean)
            k['tier1_bad'] += bad
            k['tier1_cmds'] += nrec
            t2 = tier2(p, lean)
            k['tier2']['vertices'] += t2['vertices']
            if not t2['count_equal']:
                k['tier2']['count_mismatch'] += 1
            for c in range(4):
                k['tier2']['clip_max'][c] = max(k['tier2']['clip_max'][c], t2['clip_max'][c])
            for f in ('xw_max', 'yw_max', 'zw_max', 'px_max'):
                k['tier2'][f] = max(k['tier2'][f], t2[f])
            k['tier2']['px_moved'] += t2['px_moved']
        k['vectors'] = len(k['vectors'])
        result['kinds'][kind] = k
        if verbose:
            print('%-7s dumps %3d  exact %3d (raw words %3d)  memo-defect %d  fail %d  '
                  'tier1 %d/%d cmds differ  tier2 %d vertices: clip max %s, '
                  'x/w %.2e y/w %.2e z/w %.2e, max %.4f px, %d vertices moved a pixel' % (
                      kind, k['dumps'], k['exact'], k['raw_exact'], k['memo_exact'], k['fail'],
                      k['tier1_bad'], k['tier1_cmds'], k['tier2']['vertices'],
                      k['tier2']['clip_max'], k['tier2']['xw_max'], k['tier2']['yw_max'],
                      k['tier2']['zw_max'], k['tier2']['px_max'], k['tier2']['px_moved']))
            for f in k['failed'][:4]:
                print('    FAIL', f)
    ram = {}
    for kind in ORDER:
        for detail in ('low', 'high'):
            ram['%s_%s' % (kind, detail)] = list_words(kind, detail)
    result['list_words'] = ram
    if build_dir is not None:
        result['owner_images'] = owner_image_bytes(build_dir)
    result['entry_capacity_words'] = ENTRY_CAPACITY_WORDS
    if verbose:
        for key, variants in ram.items():
            worst = max(variants.items(), key=lambda kv: kv[1][0])
            canon = variants['canonical']
            print('%-12s canonical lean %5d / recorded %5d words; %2d variants, worst %s %d '
                  '(%s the %d-word entry)' % (
                      key, canon[0], canon[1], len(variants), worst[0], worst[1][0],
                      'fits' if worst[1][0] <= ENTRY_CAPACITY_WORDS else 'EXCEEDS',
                      ENTRY_CAPACITY_WORDS))
        if 'owner_images' in result:
            print('owner images', result['owner_images'])
    if json_out:
        Path(json_out).write_text(json.dumps(result, indent=1, default=str))
    return 1 if failures else 0


if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('dumps')
    ap.add_argument('--build', default=None)
    ap.add_argument('--json', default=None)
    a = ap.parse_args()
    sys.exit(run(a.dumps, a.build, a.json))
